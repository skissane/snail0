/***---NATIVES: IMPLEMENT---***/

#define NATIVE_ARG_MIN(_min) do { \
		if (!snailArgCountMinimum(snail, cmdName, _min, argCount)) \
			return snailStatusError; \
	} while(0)

#define NATIVE_ARG_MUSTINT(_index) do { \
		if (!snailIsInt(args[_index])) { \
			snailSetResult(snail, "\"argument is not an integer\""); \
			return snailStatusError; \
		} \
	} while(0)

#define NATIVE_ARG_QSTRINGMUST(_index) do { \
		if (args[_index][0] != '"') { \
			snailSetResult(snail, "\"argument is not a quoted string\""); \
			return snailStatusError; \
		} \
	} while(0)

#define NATIVE_ARG_MUSTCLASS(_index,_class) do { \
		if (snailTokenClassify(args[_index]) != _class) { \
			snailSetResult(snail,"argument " #_index " must have type " #_class); \
			return snailStatusError; \
		} \
	} while(0)

NATIVE(error, 1) {
	snailSetResult(snail, args[0]);
	return snailStatusError;
}

NATIVE(list, VARIADIC) {
	snailBuffer *result = snailBufferCreate(16);
	snailBufferAddChar(result,'{');
	for (int i = 0; i < argCount; i++) {
		if (i > 0)
			snailBufferAddChar(result, ' ');
		char * trim = snailTrimString(args[i]);
		snailBufferAddString(result, trim);
		free(trim);
	}
	snailBufferAddChar(result,'}');
	snailBufferAddChar(result,0);
	snailSetResult(snail, result->bytes);
	snailBufferDestroy(result);
	return snailStatusOk;
}

NATIVE(string_is_bool, 1) {
	snailSetResultBool(snail, snailIsBool(args[0]));
	return snailStatusOk;
}

NATIVE(string_is_true, 1) {
	snailSetResultBool(snail, snailIsTrue(args[0]));
	return snailStatusOk;
}

NATIVE(string_is_false, 1) {
	snailSetResultBool(snail, snailIsFalse(args[0]));
	return snailStatusOk;
}

NATIVE(string_is_blank, 1) {
	snailSetResultBool(snail, snailIsBlank(args[0]));
	return snailStatusOk;
}

NATIVE(string_is_int, 1) {
	snailSetResultBool(snail, snailIsInt(args[0]));
	return snailStatusOk;
}

NATIVE(string_is_digits, 1) {
	snailSetResultBool(snail, snailIsDigits(args[0]));
	return snailStatusOk;
}

NATIVE(string_length, 1) {
	NATIVE_ARG_QSTRINGMUST(0);
	char *unquote = snailTokenUnquote(args[0]);
	char *result = snailI64ToStr(strlen(unquote));
	snailSetResult(snail, result);
	free(result);
	free(unquote);
	return snailStatusOk;
}

NATIVE(string_cmp, 2) {
	NATIVE_ARG_QSTRINGMUST(0);
	NATIVE_ARG_QSTRINGMUST(1);
	char *unquote0 = snailTokenUnquote(args[0]);
	char *unquote1 = snailTokenUnquote(args[1]);
	int r = strcmp(unquote0,unquote1);
	free(unquote0);
	free(unquote1);
	snailSetResult(snail, r < 0 ? "-1" : r > 0 ? "1" : "0");
	return snailStatusOk;
}

NATIVE(string_casecmp, 2) {
	NATIVE_ARG_QSTRINGMUST(0);
	NATIVE_ARG_QSTRINGMUST(1);
	char *unquote0 = snailTokenUnquote(args[0]);
	char *unquote1 = snailTokenUnquote(args[1]);
	int r = strcasecmp(unquote0,unquote1);
	free(unquote0);
	free(unquote1);
	snailSetResult(snail, r < 0 ? "-1" : r > 0 ? "1" : "0");
	return snailStatusOk;
}

NATIVE(add, VARIADIC) {
	NATIVE_ARG_MIN(2);

	int64_t c = 0;
	for (int i = 0; i < argCount; i++) {
		if (!snailIsInt(args[i])) {
			snailSetResult(snail, "\"+: argument is not an integer\"");
			return snailStatusError;
		}
		c += strtoll(args[i], NULL, 10);
	}
	char *result = snailI64ToStr(c);
	snailSetResult(snail, result);
	free(result);
	return snailStatusOk;
}

NATIVE(sub, VARIADIC) {
	NATIVE_ARG_MIN(1);

	int64_t c = 0;
	for (int i = 0; i < argCount; i++) {
		if (!snailIsInt(args[i])) {
			snailSetResult(snail, "\"-: argument is not an integer\"");
			return snailStatusError;
		}
		int64_t n = strtoll(args[i], NULL, 10);
		if (i == 0 && argCount > 1)
			c = n;
		else
			c -= n;
	}
	char *result = snailI64ToStr(c);
	snailSetResult(snail, result);
	free(result);
	return snailStatusOk;
}

NATIVE(mul, VARIADIC) {
	NATIVE_ARG_MIN(2);

	int64_t c = 1;
	for (int i = 0; i < argCount; i++) {
		if (!snailIsInt(args[i])) {
			snailSetResult(snail, "\"*: argument is not an integer\"");
			return snailStatusError;
		}
		c *= strtoll(args[i], NULL, 10);
	}
	char *result = snailI64ToStr(c);
	snailSetResult(snail, result);
	free(result);
	return snailStatusOk;
}

NATIVE(div, VARIADIC) {
	NATIVE_ARG_MIN(2);

	int64_t c = 0;
	for (int i = 0; i < argCount; i++) {
		if (!snailIsInt(args[i])) {
			snailSetResult(snail, "\"/: argument is not an integer\"");
			return snailStatusError;
		}
		int64_t n = strtoll(args[i], NULL, 10);
		if (i == 0 && argCount > 1)
			c = n;
		else if (n == 0) {
			snailSetResult(snail, "\"/: division by zero\"");
			return snailStatusError;
		}
		else
			c /= n;
	}
	char *result = snailI64ToStr(c);
	snailSetResult(snail, result);
	free(result);
	return snailStatusOk;
}

NATIVE(lt, 2) {
	NATIVE_ARG_MUSTINT(0);
	NATIVE_ARG_MUSTINT(1);
	int64_t a = strtoll(args[0], NULL, 10);
	int64_t b = strtoll(args[1], NULL, 10);
	snailSetResultBool(snail, a < b);
	return snailStatusOk;
}

NATIVE(gt, 2) {
	NATIVE_ARG_MUSTINT(0);
	NATIVE_ARG_MUSTINT(1);
	int64_t a = strtoll(args[0], NULL, 10);
	int64_t b = strtoll(args[1], NULL, 10);
	snailSetResultBool(snail, a > b);
	return snailStatusOk;
}

NATIVE(lte, 2) {
	NATIVE_ARG_MUSTINT(0);
	NATIVE_ARG_MUSTINT(1);
	int64_t a = strtoll(args[0], NULL, 10);
	int64_t b = strtoll(args[1], NULL, 10);
	snailSetResultBool(snail, a <= b);
	return snailStatusOk;
}

NATIVE(gte, 2) {
	NATIVE_ARG_MUSTINT(0);
	NATIVE_ARG_MUSTINT(1);
	int64_t a = strtoll(args[0], NULL, 10);
	int64_t b = strtoll(args[1], NULL, 10);
	snailSetResultBool(snail, a >= b);
	return snailStatusOk;
}

NATIVE(eq, 2) {
	snailSetResultBool(snail, strcmp(args[0], args[1]) == 0);
	return snailStatusOk;
}

NATIVE(info_cmds, 0) {
	snailBuffer *buf = snailBufferCreate(1024);
	snailBufferAddChar(buf,'{');
	char *cmd = snailHashTableFirst(snail->commands);
	while (cmd != NULL) {
		if (buf->length > 1)
			snailBufferAddChar(buf, ' ');
		snailBufferAddString(buf, cmd);
		cmd = snailHashTableNext(snail->commands, cmd);
	}
	snailBufferAddChar(buf,'}');
	snailBufferAddChar(buf, 0);
	snailSetResult(snail, buf->bytes);
	snailBufferDestroy(buf);
	return snailStatusOk;
}

NATIVE(info_cmds_count, 0) {
	snailSetResultInt(snail, snail->commands->numberOfCells);
	return snailStatusOk;
}

NATIVE(puts, 1) {
	char *unquote = snailTokenUnquote(args[0]);
	printf("%s\n", unquote != NULL ? unquote : args[0]);
	free(unquote);
	snailSetResult(snail, "");
	return snailStatusOk;
}

NATIVE(puts_nonewline, 1) {
	char *unquote = snailTokenUnquote(args[0]);
	printf("%s", unquote != NULL ? unquote : args[0]);
	free(unquote);
	snailSetResult(snail, "");
	return snailStatusOk;
}

NATIVE(list_length, 1) {
	snailArray *list = snailUnquoteList(args[0]);
	if (list == NULL) {
		snailSetResult(snail, "list.length: argument is not a valid list");
		return snailStatusError;
	}
	int length = list->length;
	snailArrayDestroy(list, free);
	snailSetResultInt(snail, length);
	return snailStatusOk;
}

NATIVE(list_sort, 1) {
	snailArray *list = snailUnquoteList(args[0]);
	if (list == NULL) {
		snailSetResult(snail, "list.sort: argument is not a valid list");
		return snailStatusError;
	}
	snailArraySort(list, (snailArrayComparator*)strcmp);
	char *result = snailQuoteList(list);
	snailArrayDestroy(list, free);
	snailSetResult(snail, result);
	free(result);
	return snailStatusOk;
}

NATIVE(list_index, 2) {
	snailArray *list = snailUnquoteList(args[0]);
	if (list == NULL) {
		snailSetResult(snail, "list.index: argument is not a valid list");
		return snailStatusError;
	}
	NATIVE_ARG_MUSTINT(1);
	int index = strtoll(args[1], NULL, 10);
	if (index < 0 || index >= list->length) {
		snailArrayDestroy(list, free);
		snailSetResult(snail, "");
		return snailStatusOk;
	}
	snailSetResult(snail, list->elems[index]);
	snailArrayDestroy(list, free);
	return snailStatusOk;
}

NATIVE(set, 2) {
	snailSetVar(snail, args[0], snailDupString(args[1]));
	snailSetResult(snail, "");
	return snailStatusOk;
}

NATIVE(info_vars, 0) {
	snailCallFrame *frame = snail->frames->elems[snail->framePointer];
	snailBuffer *buf = snailBufferCreate(64);
	snailBufferAddChar(buf,'{');
	char *cmd = snailHashTableFirst(frame->vars);
	while (cmd != NULL) {
		if (buf->length > 1)
			snailBufferAddChar(buf, ' ');
		snailBufferAddString(buf, cmd);
		cmd = snailHashTableNext(frame->vars, cmd);
	}
	snailBufferAddChar(buf,'}');
	snailBufferAddChar(buf, 0);
	snailSetResult(snail, buf->bytes);
	snailBufferDestroy(buf);
	return snailStatusOk;
}

NATIVE(var_get, 1) {
	char *value = snailGetVar(snail, args[0]);
	if (value == NULL) {
		char *msg = snailMakeVarNotFoundError(args[0]);
		snailSetResult(snail, msg);
		free(msg);
		return snailStatusError;
	}
	snailSetResult(snail, value);
	return snailStatusOk;
}

NATIVE(var_del, 1) {
	snailCallFrame *frame = snail->frames->elems[snail->framePointer];
	snailSetResultBool(snail,snailHashTableDelete(frame->vars, args[0]) != NULL);
	return snailStatusOk;
}

NATIVE(var_has, 1) {
	char *value = snailGetVar(snail, args[0]);
	snailSetResultBool(snail, value != NULL);
	return snailStatusOk;
}

NATIVE(unsupported_parse_dump,1) {
	snailSetResult(snail, "");
	return snailParseDump(args[0]) ? snailStatusOk : snailStatusError;
}

NATIVE(pass,1) {
	snailSetResult(snail, args[0]);
	return snailStatusOk;
}

NATIVE(token_quote,1) {
	char *quoted = snailTokenQuote(args[0]);
	snailSetResult(snail, quoted);
	free(quoted);
	return snailStatusOk;
}

NATIVE(token_unquote,1) {
	char *unquoted = snailTokenUnquote(args[0]);
	if (unquoted == NULL) {
		snailSetResult(snail,"token.unquote: bad token syntax");
		return snailStatusError;
	}
	snailSetResult(snail,unquoted);
	free(unquoted);
	return snailStatusOk;
}

NATIVE(token_classify,1) {
	char *c = snailMalloc(2);
	c[0] = snailTokenClassify(args[0]);
	c[1] = 0;
	snailSetResult(snail,c);
	free(c);
	return snailStatusOk;
}

NATIVE(repl_prompt_get,0) {
	char *prompt = snailMakeQuoted(snail->repl->prompt);
	snailSetResult(snail,prompt);
	free(prompt);
	return snailStatusOk;
}

NATIVE(repl_prompt_set,1) {
	char *unquoted = snailTokenUnquote(args[0]);
	if (unquoted == NULL) {
		snailSetResult(snail,"repl.prompt.set: argument not a string");
		return snailStatusError;
	}
	free(snail->repl->prompt);
	snail->repl->prompt = unquoted;
	return snailStatusOk;
}

NATIVE(return,1) {
	snailSetResult(snail,args[0]);
	return snailStatusReturn;
}

NATIVE(break,0) {
	return snailStatusBreak;
}

NATIVE(continue,0) {
	return snailStatusContinue;
}

NATIVE(eval,1) {
	if (snailTokenClassify(args[0]) != 'L') {
		snailSetResult(snail,"eval argument 0 must be of type L");
		return snailStatusError;
	}
	return snailExecList(snail,args[0]);
}

NATIVE(loop,1) {
	if (snailTokenClassify(args[0]) != 'L') {
		snailSetResult(snail,"bad arguments");
		return snailStatusError;
	}
	for (;;) {
		snailStatus ss = snailExecList(snail,args[0]);
		if (ss == snailStatusOk || ss == snailStatusContinue)
			continue;
		if (ss == snailStatusBreak)
			return snailStatusOk;
		if (ss == snailStatusError || ss == snailStatusReturn)
			return ss;
		snailSetResult(snail,"unexpected status in loop");
		return snailStatusError;
	}
}

NATIVE(proc,3) {
	if (snailTokenClassify(args[0]) != 'U') {
		snailSetResult(snail,"bad procedure name");
		return snailStatusError;
	}
	if (snailTokenClassify(args[1]) != 'L') {
		snailSetResult(snail,"bad procedure arguments");
		return snailStatusError;
	}
	if (snailTokenClassify(args[2]) != 'L') {
		snailSetResult(snail,"bad procedure body");
		return snailStatusError;
	}
	if (snailHashTableGet(snail->commands, args[0]) != NULL) {
		snailSetResult(snail,"duplicate command name");
		return snailStatusError;
	}
	char *pname = snailDupString(args[0]);
	snailArray *pargs = snailUnquoteList(args[1]);
	char *pbody = snailDupString(args[2]+1);
	pbody[strlen(pbody)-1] = 0;

	int arity = 0;
	for (int i = 0; i < pargs->length; i++ ) {
		char *parg = pargs->elems[i];
		if (arity >= 0 && parg[0] == '$')
			arity++;
		if (strcmp(parg,"?") == 0 || strcmp(parg,"&") == 0)
			arity = VARIADIC;
	}

	snailCommand *cmd = snailMalloc(sizeof(snailCommand));
	cmd->name = pname;
	cmd->arity = arity;
	cmd->args = snailDupString(args[1]);
	cmd->script = pbody;
	snailHashTablePut(snail->commands, pname, cmd);
	snailArrayDestroy(pargs,free);
	return snailStatusOk;
}

NATIVE(if,VARIADIC) {
	NATIVE_ARG_MIN(3);

	// if: argument validation
	if (args[0][0] != '{') {
		snailSetResult(snail,"if: IF condition must be a list");
		return snailStatusError;
	}
	if (strcmp(args[1],"then") != 0) {
		snailSetResult(snail,"if: expected THEN after IF condition");
		return snailStatusError;
	}
	if (args[2][0] != '{') {
		snailSetResult(snail,"if: IF body must be a list");
		return snailStatusError;
	}
	for (int i = 3; i < argCount; i++)
		if (strcmp(args[i],"elseif")==0) {
			i++;
			if (i >= argCount || args[i][0] != '{') {
				snailSetResult(snail,"if: ELSEIF condition must be a list");
				return snailStatusError;
			}
			i++;
			if (i >= argCount || strcmp(args[i],"then") != 0) {
				snailSetResult(snail,"if: expected THEN after ELSEIF condition");
				return snailStatusError;
			}
			i++;
			if (i >= argCount || args[i][0] != '{') {
				snailSetResult(snail,"if: ELSEIF body must be a list");
				return snailStatusError;
			}
		} else if (strcmp(args[i],"else")==0) {
			i++;
			if (i >= argCount || args[i][0] != '{') {
				snailSetResult(snail,"if: ELSE body must be a list");
				return snailStatusError;
			}
			i++;
			if (i < argCount) {
				snailSetResult(snail,"if: junk after body of ELSE clause");
				return snailStatusError;
			}
		}
		else {
			snailSetResult(snail,"if: expected ELSE or THEN");
			return snailStatusError;
		}

	// if: execution
	snailStatus ss = snailExecList(snail,args[0]);
	if (ss != snailStatusOk)
		return ss;
	if (!snailIsBool(snail->result)) {
		snailSetResult(snail,"if condition not boolean");
		return snailStatusError;
	}
	bool cond = snailIsTrue(snail->result);
	if (cond)
		return snailExecList(snail,args[2]);

	// Begin handling if and else clauses
	for (int i = 3; i < argCount; i++) {
		if (strcmp(args[i],"else") == 0) {
			i++;
			return snailExecList(snail,args[i]);
		}
		else if (strcmp(args[i],"elseif") == 0) {
			i++;
			ss = snailExecList(snail,args[i]);
			if (ss != snailStatusOk)
				return ss;
			if (!snailIsBool(snail->result)) {
				snailSetResult(snail,"elseif condition not boolean");
				return snailStatusError;
			}
			cond = snailIsTrue(snail->result);
			// Skip then
			i += 2;
			if (cond)
				return snailExecList(snail,args[i]);
			i++;
		}
	}

	// No branch taken, return empty
	snailSetResult(snail,"");
	return snailStatusOk;
}

NATIVE(foreach,3) {
	if (snailTokenClassify(args[0]) != 'U') {
		snailSetResult(snail,"argument 0 must be unquoted (U)");
		return snailStatusError;
	}
	if (snailTokenClassify(args[1]) != 'L') {
		snailSetResult(snail,"argument 1 must be list (L)");
		return snailStatusError;
	}
	if (snailTokenClassify(args[2]) != 'L') {
		snailSetResult(snail,"argument 2 must be list (L)");
		return snailStatusError;
	}
	snailSetResult(snail, "");
	snailArray *list = snailUnquoteList(args[1]);
	for (int i = 0; i < list->length; i++) {
		char *elem = list->elems[i];
		snailSetVar(snail, args[0], snailDupString(elem));
		snailStatus ss = snailExecList(snail,args[2]);
		if (ss == snailStatusOk || ss == snailStatusContinue)
			continue;
		if (ss == snailStatusBreak) {
			snailArrayDestroy(list,free);
			return snailStatusOk;
		}
		if (ss == snailStatusError || ss == snailStatusReturn) {
			snailArrayDestroy(list,free);
			return ss;
		}
		snailSetResult(snail,"unexpected status in loop");
		snailArrayDestroy(list,free);
		return snailStatusError;
	}
	snailArrayDestroy(list,free);
	return snailStatusOk;
}

NATIVE(time_now,0) {
	snailSetResultInt(snail, snailTimeNow());
	return snailStatusOk;
}

NATIVE(catch,3) {
	if (snailTokenClassify(args[0]) != 'L') {
		snailSetResult(snail,"argument 0 must have type L");
		return snailStatusError;
	}
	if (snailTokenClassify(args[1]) != 'U') {
		snailSetResult(snail,"argument 1 must have type U");
		return snailStatusError;
	}
	if (snailTokenClassify(args[2]) != 'L') {
		snailSetResult(snail,"argument 2 must have type U");
		return snailStatusError;
	}
	snailStatus ss = snailExecList(snail,args[0]);
	if (ss == snailStatusError) {
		snailSetVar(snail, args[1], snailDupString(snail->result));
		return snailExecList(snail,args[2]);
	}
	return ss;
}

NATIVE(file_read,1) {
	NATIVE_ARG_MUSTCLASS(0, 'Q');
	char *fileName = snailTokenUnquote(args[0]);
	char *script = snailReadFile(fileName);
	free(fileName);
	if (!script) {
		snailBuffer *msg = snailBufferCreate(16);
		snailBufferAddString(msg, "unable to read file ");
		snailBufferAddString(msg, args[0]);
		snailSetResult(snail, msg->bytes);
		snailBufferDestroy(msg);
		return snailStatusError;
	}
	char *quoted = snailMakeQuoted(script);
	snailSetResult(snail, quoted);
	free(script);
	return snailStatusOk;
}

NATIVE(file_run,1) {
	NATIVE_ARG_MUSTCLASS(0, 'Q');
	char *fileName = snailTokenUnquote(args[0]);
	char *script = snailReadFile(fileName);
	free(fileName);
	if (!script) {
		snailBuffer *msg = snailBufferCreate(16);
		snailBufferAddString(msg, "unable to read file ");
		snailBufferAddString(msg, args[0]);
		snailSetResult(snail, msg->bytes);
		snailBufferDestroy(msg);
		return snailStatusError;
	}
	snailStatus status = snailExec(snail, script);
	free(script);
	return status;
}

NATIVE(null,0) {
	snailSetResult(snail, "");
	return snailStatusOk;
}

NATIVE(is_null,1) {
	snailSetResultBool(snail, snailTokenClassify(args[0]) == 'Z');
	return snailStatusOk;
}

NATIVE(list_add, 2) {
	snailArray *list = snailUnquoteList(args[0]);
	if (list == NULL) {
		snailSetResult(snail, "list.add: argument 0 is not a valid list");
		return snailStatusError;
	}
	snailArrayAdd(list, snailDupString(args[1]));
	char *result = snailQuoteList(list);
	snailSetResult(snail, result);
	snailArrayDestroy(list, free);
	return snailStatusOk;
}

NATIVE(list_at, 2) {
	snailArray *list = snailUnquoteList(args[0]);
	if (list == NULL) {
		snailSetResult(snail, "list.at: argument 0 is not a valid list");
		return snailStatusError;
	}
	if (!snailIsInt(args[1])) {
		snailSetResult(snail, "list.at: argument 1 is not a integer");
		return snailStatusError;
	}
	int64_t at = strtoll(args[1], NULL, 10);
	if (at < 0 || at >= list->length) {
		snailSetResult(snail, "list.at: provided index is out of range");
		return snailStatusError;
	}
	snailSetResult(snail, list->elems[at]);
	snailArrayDestroy(list, free);
	return snailStatusOk;
}

NATIVE(list_concat, VARIADIC) {
	NATIVE_ARG_MIN(1);
	snailArray *list0 = snailUnquoteList(args[0]);
	if (list0 == NULL) {
		snailSetResult(snail, "list.concat: argument 0 is not a valid list");
		return snailStatusError;
	}
	for (int i = 1; i < argCount; i++) {
		snailArray *list1 = snailUnquoteList(args[i]);
		if (list1 == NULL) {
			snailBuffer *msg = snailBufferCreate(16);
			snailBufferAddString(msg, "list.concat: argument ");
			char *n = snailI64ToStr(i);
			snailBufferAddString(msg, n);
			free(n);
			snailBufferAddString(msg, " is not a valid list");
			snailSetResult(snail, msg->bytes);
			snailBufferDestroy(msg);
			return snailStatusError;
		}
		for (int j = 0; j < list1->length; j++) {
			snailArrayAdd(list0, snailDupString(list1->elems[j]));
		}
		snailArrayDestroy(list1, free);
	}
	char *result = snailQuoteList(list0);
	snailSetResult(snail, result);
	snailArrayDestroy(list0, free);
	free(result);
	return snailStatusOk;
}
