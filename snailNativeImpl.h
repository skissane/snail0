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

NATIVE(is_bool, 1) {
	snailSetResultBool(snail, snailIsBool(args[0]));
	return snailStatusOk;
}

NATIVE(is_true, 1) {
	snailSetResultBool(snail, snailIsTrue(args[0]));
	return snailStatusOk;
}

NATIVE(is_false, 1) {
	snailSetResultBool(snail, snailIsFalse(args[0]));
	return snailStatusOk;
}

NATIVE(string_is_blank, 1) {
	NATIVE_ARG_MUSTCLASS(0, 'Q');
	char *unquoted = snailTokenUnquote(args[0]);
	snailSetResultBool(snail, snailIsBlank(unquoted));
	free(unquoted);
	return snailStatusOk;
}

NATIVE(is_int, 1) {
	snailSetResultBool(snail, snailIsInt(args[0]));
	return snailStatusOk;
}

NATIVE(string_is_digits, 1) {
	NATIVE_ARG_MUSTCLASS(0, 'Q');
	char *unquoted = snailTokenUnquote(args[0]);
	snailSetResultBool(snail, snailIsDigits(unquoted));
	free(unquoted);
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

NATIVE(set_up, 3) {
	NATIVE_ARG_MUSTINT(0);
	int64_t level = strtoll(args[0], NULL, 10);
	if (!snailSetUpVar(snail, level, args[1], snailDupString(args[2]))) {
		snailBuffer *msg = snailBufferCreate(16);
		snailBufferAddString(msg, "set.up called with bad level '");
		snailBufferAddString(msg, args[0]);
		snailBufferAddString(msg, "' with fp=");
		char *fpStr = snailI64ToStr(snail->frames->length-1);
		snailBufferAddString(msg, fpStr);
		free(fpStr);
		snailBufferAddChar(msg, 0);
		snailSetResult(snail, msg->bytes);
		snailBufferDestroy(msg);
		return snailStatusError;
	}
	snailSetResult(snail, "");
	return snailStatusOk;
}

NATIVE(var_get_up, 2) {
	NATIVE_ARG_MUSTINT(0);
	int64_t level = strtoll(args[0], NULL, 10);
	char *value = snailGetUpVar(snail, level, args[1]);
	if (value == NULL) {
		snailBuffer *msg = snailBufferCreate(16);
		snailBufferAddString(msg, "var.get.up: variable $");
		snailBufferAddString(msg, args[1]);
		snailBufferAddString(msg, " not found in level ");
		snailBufferAddString(msg, args[0]);
		snailBufferAddChar(msg, 0);
		snailSetResult(snail, msg->bytes);
		snailBufferDestroy(msg);
		return snailStatusError;
	}
	snailSetResult(snail, value);
	return snailStatusOk;
}

NATIVE(info_vars, 0) {
	snailCallFrame *frame = snail->frames->elems[snail->frames->length-1];
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
	snailCallFrame *frame = snail->frames->elems[snail->frames->length-1];
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
			if (cond) {
				return snailExecList(snail,args[i]);
			}
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

NATIVE(time_startup,0) {
	snailSetResultInt(snail, snail->startupTime);
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

NATIVE(nvl, VARIADIC) {
	NATIVE_ARG_MIN(1);
	for (int i = 0; i < argCount; i++) {
		if (snailTokenClassify(args[i]) != 'Z') {
			snailSetResult(snail, args[i]);
			return snailStatusOk;
		}
	}
	snailSetResult(snail, "");
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

NATIVE(string_concat, VARIADIC) {
	NATIVE_ARG_MIN(1);
	snailBuffer *buf = snailBufferCreate(16);
	for (int i = 0; i < argCount; i++) {
		char * unquoted = snailTokenUnquote(args[i]);
		if (unquoted == NULL) {
			snailBufferAddString(buf, args[i]);
		}
		else {
			snailBufferAddString(buf, unquoted);
			free(unquoted);
		}
	}
	snailBufferAddChar(buf, 0);
	char *result = snailMakeQuoted(buf->bytes);
	snailSetResult(snail, result);
	free(result);
	return snailStatusOk;
}

NATIVE(string_starts_with, 2) {
	NATIVE_ARG_MUSTCLASS(0,'Q');
	NATIVE_ARG_MUSTCLASS(1,'Q');
	char *target = snailTokenUnquote(args[0]);
	char *starts = snailTokenUnquote(args[1]);
	int l_target = strlen(target);
	int l_starts = strlen(starts);
	if (l_target < l_starts) {
		free(target);
		free(starts);
		snailSetResultBool(snail, false);
		return snailStatusOk;
	}
	for (int i = 0; i < l_starts; i++) {
		if (target[i] != starts[i]) {
			free(target);
			free(starts);
			snailSetResultBool(snail, false);
			return snailStatusOk;
		}
	}
	free(target);
	free(starts);
	snailSetResultBool(snail, true);
	return snailStatusOk;
}

NATIVE(frame_count,0) {
	snailSetResultInt(snail, snail->frames->length);
	return snailStatusOk;
}

NATIVE(frame_cmds,0) {
	snailArray *r = snailArrayCreate(snail->frames->length);
	for (int i = snail->frames->length-1; i >= 0; i--) {
		snailCallFrame *frame = snail->frames->elems[i];
		char *name = frame->cmdName != NULL ? frame->cmdName : "";
		if (snailTokenIsValid(name))
			snailArrayAdd(r, snailDupString(name));
		else
			snailArrayAdd(r, snailMakeQuoted(name));
	}
	char *result = snailQuoteList(r);
	snailSetResult(snail, result);
	snailArrayDestroy(r, free);
	free(result);
	return snailStatusOk;
}

NATIVE(while,2) {
	NATIVE_ARG_MUSTCLASS(0,'L');
	NATIVE_ARG_MUSTCLASS(1,'L');
	for (;;) {
		// Evaluate conditional
		snailStatus ssCond = snailExecList(snail,args[0]);
		// Any error/etc in conditional, abort loop
		if (ssCond != snailStatusOk)
			return ssCond;
		// If conditional did not evaluate to true, abort loop
		if (!snailIsTrue(snail->result)) {
			snailSetResult(snail,"");
			return snailStatusOk;
		}

		// Conditional still true, another step around the loop
		snailStatus ss = snailExecList(snail,args[1]);
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

NATIVE(dict, VARIADIC) {
	if ((argCount % 2) != 0) {
		snailBuffer *msg = snailBufferCreate(16);
		snailBufferAddString(msg,"dict command takes even number of arguments, given ");
		char *s_argCount = snailI64ToStr(argCount);
		snailBufferAddString(msg,s_argCount);
		free(s_argCount);
		snailBufferAddString(msg," arguments");
		snailBufferAddChar(msg,0);
		snailSetResult(snail, msg->bytes);
		snailBufferDestroy(msg);
		return snailStatusError;
	}
	snailHashTable *ht = snailHashTableCreate(argCount/2);
	for (int i = 0; i < argCount; i += 2) {
		char *key = args[i];
		char *value = args[i+1];
		free(snailHashTablePut(ht, key, snailDupString(value)));
	}
	char *quoted = snailQuoteDict(ht);
	if (quoted == NULL) {
		snailSetResult(snail,"unexpected error in dict command");
		snailHashTableDestroy(ht,free);
		return snailStatusError;
	}
	snailHashTableDestroy(ht,free);
	snailSetResult(snail,quoted);
	free(quoted);
	return snailStatusOk;
}

NATIVE(dict_size,1) {
	NATIVE_ARG_MUSTCLASS(0,'D');
	snailHashTable *ht = snailParseDict(args[0]);
	if (ht == NULL) {
		snailSetResult(snail,"unexpected error in dict.size command");
		return snailStatusError;
	}
	snailSetResultInt(snail, ht->numberOfCells);
	snailHashTableDestroy(ht,free);
	return snailStatusOk;
}

NATIVE(dict_keys,1) {
	NATIVE_ARG_MUSTCLASS(0,'D');
	snailHashTable *ht = snailParseDict(args[0]);
	if (ht == NULL) {
		snailSetResult(snail,"unexpected error in dict.keys command");
		return snailStatusError;
	}
	snailArray *keys = snailHashTableKeys(ht);
	snailHashTableDestroy(ht, free);
	char *result = snailQuoteList(keys);
	snailArrayDestroy(keys, free);
	snailSetResult(snail, result);
	free(result);
	return snailStatusOk;
}

NATIVE(dict_has,2) {
	NATIVE_ARG_MUSTCLASS(0,'D');
	snailHashTable *ht = snailParseDict(args[0]);
	if (ht == NULL) {
		snailSetResult(snail,"unexpected error in dict.has command");
		return snailStatusError;
	}
	char *value = snailHashTableGet(ht, args[1]);
	snailSetResultBool(snail, value != NULL);
	snailHashTableDestroy(ht, free);
	return snailStatusOk;
}

NATIVE(dict_get,2) {
	NATIVE_ARG_MUSTCLASS(0,'D');
	snailHashTable *ht = snailParseDict(args[0]);
	if (ht == NULL) {
		snailSetResult(snail,"unexpected error in dict.has command");
		return snailStatusError;
	}
	char *value = snailHashTableGet(ht, args[1]);
	snailSetResult(snail, value == NULL ? "" : value);
	snailHashTableDestroy(ht, free);
	return snailStatusOk;
}

NATIVE(dict_set,3) {
	NATIVE_ARG_MUSTCLASS(0,'D');
	snailHashTable *ht = snailParseDict(args[0]);
	if (ht == NULL) {
		snailSetResult(snail,"unexpected error in dict.set command (parsing failed)");
		return snailStatusError;
	}
	free(snailHashTablePut(ht, args[1], snailDupString(args[2])));
	char *quoted = snailQuoteDict(ht);
	if (quoted == NULL) {
		snailSetResult(snail,"unexpected error in dict.set command (quoting failed)");
		snailHashTableDestroy(ht,free);
		return snailStatusError;
	}
	snailHashTableDestroy(ht,free);
	snailSetResult(snail,quoted);
	free(quoted);
	return snailStatusOk;
}

NATIVE(dict_set_all,2) {
	NATIVE_ARG_MUSTCLASS(0,'D');
	NATIVE_ARG_MUSTCLASS(1,'D');
	snailHashTable *ht0 = snailParseDict(args[0]);
	if (ht0 == NULL) {
		snailSetResult(snail,"unexpected error in dict.set.all command (parsing arg 0 failed)");
		return snailStatusError;
	}
	snailHashTable *ht1 = snailParseDict(args[1]);
	if (ht1 == NULL) {
		snailHashTableDestroy(ht0,free);
		snailSetResult(snail,"unexpected error in dict.set.all command (parsing arg 1 failed)");
		return snailStatusError;
	}
	char *key = snailHashTableFirst(ht1);
	while (key != NULL) {
		char *value = snailHashTableGet(ht1, key);
		free(snailHashTablePut(ht0, key, snailDupString(value)));
		key = snailHashTableNext(ht1, key);
	}
	char *quoted = snailQuoteDict(ht0);
	if (quoted == NULL) {
		snailSetResult(snail,"unexpected error in dict.set.all command (quoting failed)");
		snailHashTableDestroy(ht0,free);
		snailHashTableDestroy(ht1,free);
		return snailStatusError;
	}
	snailHashTableDestroy(ht0,free);
	snailHashTableDestroy(ht1,free);
	snailSetResult(snail,quoted);
	free(quoted);
	return snailStatusOk;
}

NATIVE(file_stat,1) {
	NATIVE_ARG_MUSTCLASS(0,'Q');
	char *fileName = snailTokenUnquote(args[0]);
	struct stat buf;
	int rc = stat(fileName,&buf);
	if (rc != 0) {
		int e = errno;
		snailBuffer *buf = snailBufferCreate(16);
		snailBufferAddString(buf,"file.stat failed for file ");
		snailBufferAddString(buf,args[0]);
		snailBufferAddString(buf," with OS error ");
		char *r = snailI64ToStr(e);
		snailBufferAddString(buf,r);
		free(r);
		snailBufferAddChar(buf,0);
		snailSetResult(snail,buf->bytes);
		snailBufferDestroy(buf);
		return snailStatusError;
	}
	snailHashTable *ht = snailHashTableCreate(16);
	snailHashTablePut(ht, "dev", snailI64ToStr(buf.st_dev));
	snailHashTablePut(ht, "ino", snailI64ToStr(buf.st_ino));
	snailHashTablePut(ht, "mode", snailI64ToStr(buf.st_mode));
	snailHashTablePut(ht, "nlink", snailI64ToStr(buf.st_nlink));
	snailHashTablePut(ht, "uid", snailI64ToStr(buf.st_uid));
	snailHashTablePut(ht, "gid", snailI64ToStr(buf.st_gid));
	snailHashTablePut(ht, "rdev", snailI64ToStr(buf.st_rdev));
	snailHashTablePut(ht, "size", snailI64ToStr(buf.st_size));
	snailHashTablePut(ht, "atime", snailI64ToStr(buf.st_atime));
	snailHashTablePut(ht, "mtime", snailI64ToStr(buf.st_mtime));
	snailHashTablePut(ht, "ctime", snailI64ToStr(buf.st_ctime));
	snailHashTablePut(ht, "blksize", snailI64ToStr(buf.st_blksize));
	snailHashTablePut(ht, "blocks", snailI64ToStr(buf.st_blocks));

	char *type = "other";
	if (S_ISBLK(buf.st_mode))
		type = "blk";
	else if (S_ISCHR(buf.st_mode))
		type = "chr";
	else if (S_ISDIR(buf.st_mode))
		type = "dir";
	else if (S_ISFIFO(buf.st_mode))
		type = "fifo";
	else if (S_ISREG(buf.st_mode))
		type = "reg";
	else if (S_ISLNK(buf.st_mode))
		type = "lnk";
	else if (S_ISSOCK(buf.st_mode))
		type = "sock";
	snailHashTablePut(ht, "type", snailDupString(type));

	char *quoted = snailQuoteDict(ht);
	snailHashTableDestroy(ht,free);
	snailSetResult(snail,quoted);
	free(quoted);
	return snailStatusOk;
}

NATIVE(info_about_cmd,1) {
	NATIVE_ARG_MUSTCLASS(0,'U');
	snailCommand *cmd = snailHashTableGet(snail->commands, args[0]);
	if (cmd == NULL) {
		snailSetResult(snail, "");
		return snailStatusOk;
	}
	snailHashTable *ht = snailHashTableCreate(16);
	snailHashTablePut(ht,"name",snailDupString(cmd->name));
	snailHashTablePut(ht,"arity",snailI64ToStr(cmd->arity));
	if (cmd->args != NULL)
		snailHashTablePut(ht,"args",snailDupString(cmd->args));
	if (cmd->script != NULL) {
		snailBuffer *s = snailBufferCreate(16);
		snailBufferAddChar(s,'{');
		snailBufferAddString(s,cmd->script);
		snailBufferAddChar(s,'}');
		snailBufferAddChar(s,0);
		snailHashTablePut(ht,"script",snailDupString(s->bytes));
		snailBufferDestroy(s);
	}
	if (cmd->native != NULL)
		snailHashTablePut(ht,"native",snailDupString("t"));
	char *quoted = snailQuoteDict(ht);
	snailHashTableDestroy(ht,free);
	snailSetResult(snail,quoted);
	free(quoted);
	return snailStatusOk;
}

NATIVE(global_set, 2) {
	NATIVE_ARG_MUSTCLASS(0,'U');
	free(snailHashTablePut(snail->globals,args[0],args[1]));
	snailSetResult(snail,"");
	return snailStatusOk;
}

NATIVE(global_get, 1) {
	NATIVE_ARG_MUSTCLASS(0,'U');
	char *value = snailHashTableGet(snail->globals,args[0]);
	snailSetResult(snail,value == NULL ? "" : value);
	return snailStatusOk;
}

NATIVE(info_globals, 0) {
	snailBuffer *buf = snailBufferCreate(64);
	snailBufferAddChar(buf,'{');
	char *cmd = snailHashTableFirst(snail->globals);
	while (cmd != NULL) {
		if (buf->length > 1)
			snailBufferAddChar(buf, ' ');
		snailBufferAddString(buf, cmd);
		cmd = snailHashTableNext(snail->globals, cmd);
	}
	snailBufferAddChar(buf,'}');
	snailBufferAddChar(buf, 0);
	snailSetResult(snail, buf->bytes);
	snailBufferDestroy(buf);
	return snailStatusOk;
}

NATIVE(string_find,2) {
	NATIVE_ARG_MUSTCLASS(0,'Q');
	NATIVE_ARG_MUSTCLASS(1,'Q');
	char *haystack = snailTokenUnquote(args[0]);
	char *needle = snailTokenUnquote(args[1]);
	char *r = strstr(haystack, needle);
	if (r == NULL) {
		snailSetResult(snail, "");
	}
	else {
		char *rs = snailI64ToStr(r - haystack);
		snailSetResult(snail,rs);
		free(rs);
	}
	free(haystack);
	free(needle);
	return snailStatusOk;
}

NATIVE(string_find_at,3) {
	NATIVE_ARG_MUSTCLASS(0,'Q');
	NATIVE_ARG_MUSTCLASS(1,'Q');
	NATIVE_ARG_MUSTINT(2);
	char *haystack = snailTokenUnquote(args[0]);
	char *needle = snailTokenUnquote(args[1]);
	int64_t off = strtoll(args[2],NULL,10);
	int length = strlen(haystack);
	if (off < 0 || off >= length) {
		snailSetResult(snail, "");
		free(haystack);
		free(needle);
		return snailStatusOk;
	}
	char *r = strstr(haystack + off, needle);
	if (r == NULL) {
		snailSetResult(snail, "");
	}
	else {
		char *rs = snailI64ToStr(r - haystack);
		snailSetResult(snail,rs);
		free(rs);
	}
	free(haystack);
	free(needle);
	return snailStatusOk;
}

NATIVE(string_replace,3) {
	NATIVE_ARG_MUSTCLASS(0,'Q');
	NATIVE_ARG_MUSTCLASS(1,'Q');
	NATIVE_ARG_MUSTCLASS(2,'Q');
	char *target = snailTokenUnquote(args[0]);
	char *find = snailTokenUnquote(args[1]);
	char *sub = snailTokenUnquote(args[2]);
	char *r = snailStringReplace(target, find, sub);
	free(target);
	free(find);
	free(sub);
	char *q = snailMakeQuoted(r);
	free(r);
	snailSetResult(snail,q);
	free(q);
	return snailStatusOk;
}

NATIVE(string_find_rev,2) {
	NATIVE_ARG_MUSTCLASS(0,'Q');
	NATIVE_ARG_MUSTCLASS(1,'Q');
	char *haystack = snailTokenUnquote(args[0]);
	char *needle = snailTokenUnquote(args[1]);
	const char *r = snailStringFindRev(haystack, needle);
	if (r == NULL) {
		snailSetResult(snail, "");
	}
	else {
		char *rs = snailI64ToStr(r - haystack);
		snailSetResult(snail,rs);
		free(rs);
	}
	free(haystack);
	free(needle);
	return snailStatusOk;
}

NATIVE(string_left,2) {
	NATIVE_ARG_MUSTCLASS(0,'Q');
	NATIVE_ARG_MUSTINT(1);
	int64_t off = strtoll(args[1],NULL,10);
	char *s = snailTokenUnquote(args[0]);
	if (off < 0) {
		free(s);
		char *q = snailMakeQuoted("");
		snailSetResult(snail,q);
		free(q);
		return snailStatusOk;
	}
	if (off >= strlen(s)) {
		free(s);
		snailSetResult(snail,args[0]);
		return snailStatusOk;
	}
	s[off] = 0;
	char *q = snailMakeQuoted(s);
	free(s);
	snailSetResult(snail,q);
	free(q);
	return snailStatusOk;
}

NATIVE(string_skip,2) {
	NATIVE_ARG_MUSTCLASS(0,'Q');
	NATIVE_ARG_MUSTINT(1);
	int64_t off = strtoll(args[1],NULL,10);
	char *s = snailTokenUnquote(args[0]);
	if (off < 0 || off >= strlen(s)) {
		free(s);
		char *q = snailMakeQuoted("");
		snailSetResult(snail,q);
		free(q);
		return snailStatusOk;
	}
	char *q = snailMakeQuoted(s+off);
	free(s);
	snailSetResult(snail,q);
	free(q);
	return snailStatusOk;
}

NATIVE(string_sub,3) {
	NATIVE_ARG_MUSTCLASS(0,'Q');
	NATIVE_ARG_MUSTINT(1);
	NATIVE_ARG_MUSTINT(2);
	int64_t off = strtoll(args[1],NULL,10);
	int64_t len = strtoll(args[2],NULL,10);
	char *s = snailTokenUnquote(args[0]);
	if (off < 0 || off >= strlen(s)) {
		free(s);
		char *q = snailMakeQuoted("");
		snailSetResult(snail,q);
		free(q);
		return snailStatusOk;
	}
	if ((off + len) < strlen(s))
		s[off+len] = 0;
	char *q = snailMakeQuoted(s+off);
	free(s);
	snailSetResult(snail,q);
	free(q);
	return snailStatusOk;
}

NATIVE(eval_up,2) {
	NATIVE_ARG_MUSTINT(0);
	NATIVE_ARG_MUSTCLASS(1,'L');
	int64_t level = strtoll(args[0], NULL, 10);
	return snailExecListUp(snail,level,args[1]);
}

NATIVE(frame_cmd_up,1) {
	NATIVE_ARG_MUSTINT(0);
	int64_t level = strtoll(args[0], NULL, 10);
	char *cmd = snailGetCmdUp(snail,level);
	if (cmd == NULL) {
		snailSetResult(snail,"no such level");
		return snailStatusError;
	}
	if (snailTokenIsValid(cmd)) {
		snailSetResult(snail,cmd);
	} else {
		char *q = snailMakeQuoted(cmd);
		snailSetResult(snail,q);
		free(q);
	}
	return snailStatusOk;
}

NATIVE(info_channel_drivers, 0) {
	snailBuffer *buf = snailBufferCreate(1024);
	snailBufferAddChar(buf,'{');
	char *name = snailHashTableFirst(snail->channelDrivers);
	while (name != NULL) {
		if (buf->length > 1)
			snailBufferAddChar(buf, ' ');
		snailBufferAddString(buf, name);
		name = snailHashTableNext(snail->channelDrivers, name);
	}
	snailBufferAddChar(buf,'}');
	snailBufferAddChar(buf, 0);
	snailSetResult(snail, buf->bytes);
	snailBufferDestroy(buf);
	return snailStatusOk;
}

NATIVE(info_channels, 0) {
	snailBuffer *buf = snailBufferCreate(1024);
	snailBufferAddString(buf,"%{");
	char *name = snailHashTableFirst(snail->channels);
	while (name != NULL) {
		if (buf->length > 2)
			snailBufferAddChar(buf, ' ');
		snailBufferAddString(buf, name);
		snailBufferAddChar(buf, ' ');
		snailChannel *channel = snailHashTableGet(snail->channels,name);
		snailBufferAddString(buf, channel->driver->name);
		name = snailHashTableNext(snail->channels, name);
	}
	snailBufferAddChar(buf,'}');
	snailBufferAddChar(buf, 0);
	snailSetResult(snail, buf->bytes);
	snailBufferDestroy(buf);
	return snailStatusOk;
}

NATIVE(channel_write,2) {
	NATIVE_ARG_MUSTCLASS(0,'U');
	NATIVE_ARG_MUSTCLASS(1,'Q');
	char *channelName = args[0];
	char *unquoted = snailTokenUnquote(args[1]);
	size_t w = 0;
	char *r=snailChannelWrite(snail, channelName, unquoted, strlen(unquoted), &w);
	free(unquoted);
	bool ok = r == NULL;
	snailSetResult(snail,ok ? "" : r);
	free(r);
	return ok ? snailStatusOk : snailStatusError;
}

NATIVE(channel_read,2) {
	NATIVE_ARG_MUSTCLASS(0,'U');
	NATIVE_ARG_MUSTINT(1);
	int64_t bytes = strtoll(args[1], NULL, 10);
	if (bytes <= 0) {
		snailSetResult(snail, "channel.read: $bytes must be greater than zero");
		return snailStatusError;
	}
	char *channelName = args[0];
	char *buf = snailMalloc(bytes);
	size_t read = 0;
	char *r = snailChannelRead(snail, channelName, buf, bytes, &read);
	if (r != NULL) {
		free(buf);
		snailSetResult(snail,r);
		free(r);
		return snailStatusError;
	}
	if (read == 0) {
		free(buf);
		snailSetResult(snail,"");
		return snailStatusOk;
	}
	snailBuffer *out = snailBufferCreate(read+1);
	for (int i = 0; i < read; i++)
		snailBufferAddChar(out, buf[i]);
	snailBufferAddChar(out, 0);
	free(buf);
	char *quoted = snailMakeQuoted(out->bytes);
	snailBufferDestroy(out);
	snailSetResult(snail,quoted);
	free(quoted);
	return snailStatusOk;
}

NATIVE(string_upper,1) {
	NATIVE_ARG_MUSTCLASS(0,'Q');
	char *unquote = snailTokenUnquote(args[0]);
	for (int i = 0; unquote[i] != 0; i++)
		unquote[i] = toupper(unquote[i]);
	char *quoted = snailMakeQuoted(unquote);
	free(unquote);
	snailSetResult(snail,quoted);
	free(quoted);
	return snailStatusOk;
}

NATIVE(string_lower,1) {
	NATIVE_ARG_MUSTCLASS(0,'Q');
	char *unquote = snailTokenUnquote(args[0]);
	for (int i = 0; unquote[i] != 0; i++)
		unquote[i] = tolower(unquote[i]);
	char *quoted = snailMakeQuoted(unquote);
	free(unquote);
	snailSetResult(snail,quoted);
	free(quoted);
	return snailStatusOk;
}

NATIVE(string_char_at,2) {
	NATIVE_ARG_MUSTCLASS(0,'Q');
	NATIVE_ARG_MUSTINT(1);
	int64_t off = strtoll(args[1], NULL, 10);
	char *unquote = snailTokenUnquote(args[0]);
	if (off < 0 || off >= strlen(unquote)) {
		free(unquote);
		snailSetResult(snail,"");
		return snailStatusOk;
	}
	char *s = snailU64ToStr(unquote[off]);
	free(unquote);
	snailSetResult(snail,s);
	free(s);
	return snailStatusOk;
}

NATIVE(string_chr,1) {
	NATIVE_ARG_MUSTINT(0);
	char buf[2];
	buf[0] = strtoll(args[0], NULL, 10);
	buf[1] = 0;
	char *q = snailMakeQuoted(buf);
	snailSetResult(snail,q);
	free(q);
	return snailStatusOk;
}

NATIVE(string_trim,1) {
	NATIVE_ARG_MUSTCLASS(0,'Q');
	char *u = snailTokenUnquote(args[0]);
	char *trim = snailTrimString(u);
	free(u);
	char *q = snailMakeQuoted(trim);
	free(trim);
	snailSetResult(snail,q);
	free(q);
	return snailStatusOk;
}

NATIVE(and,VARIADIC) {
	for (int i = 0; i < argCount; i++)
		NATIVE_ARG_MUSTCLASS(0,'L');
	for (int i = 0; i < argCount; i++) {
		snailStatus ss = snailExecList(snail,args[i]);
		if (ss != snailStatusOk)
			return ss;
		if (!snailIsBool(snail->result)) {
			snailSetResult(snail,"and condition not boolean");
			return snailStatusError;
		}
		if (!snailIsTrue(snail->result)) {
			snailSetResultBool(snail, false);
			return snailStatusOk;
		}
	}
	snailSetResultBool(snail, true);
	return snailStatusOk;
}

NATIVE(or,VARIADIC) {
	for (int i = 0; i < argCount; i++)
		NATIVE_ARG_MUSTCLASS(0,'L');
	for (int i = 0; i < argCount; i++) {
		snailStatus ss = snailExecList(snail,args[i]);
		if (ss != snailStatusOk)
			return ss;
		if (!snailIsBool(snail->result)) {
			snailSetResult(snail,"or condition not boolean");
			return snailStatusError;
		}
		if (snailIsTrue(snail->result)) {
			snailSetResultBool(snail, true);
			return snailStatusOk;
		}
	}
	snailSetResultBool(snail, false);
	return snailStatusOk;
}

NATIVE(string_ends_with, 2) {
	NATIVE_ARG_MUSTCLASS(0,'Q');
	NATIVE_ARG_MUSTCLASS(1,'Q');
	char *target = snailTokenUnquote(args[0]);
	char *ends = snailTokenUnquote(args[1]);
	int l_target = strlen(target);
	int l_ends = strlen(ends);
	if (l_target < l_ends) {
		free(target);
		free(ends);
		snailSetResultBool(snail, false);
		return snailStatusOk;
	}
	for (int i = 0; i < l_ends; i++) {
		if (target[l_target - l_ends + i] != ends[i]) {
			free(target);
			free(ends);
			snailSetResultBool(snail, false);
			return snailStatusOk;
		}
	}
	free(target);
	free(ends);
	snailSetResultBool(snail, true);
	return snailStatusOk;
}

NATIVE(file_write,2) {
	NATIVE_ARG_MUSTCLASS(0, 'Q');
	NATIVE_ARG_MUSTCLASS(1, 'Q');
	char *fileName = snailTokenUnquote(args[0]);
	char *text = snailTokenUnquote(args[1]);
	char *error = snailWriteFile(fileName,text);
	if (error == NULL) {
		snailSetResult(snail,"");
		return snailStatusOk;
	}
	snailSetResult(snail,error);
	free(error);
	return snailStatusError;
}
