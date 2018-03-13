/***---FUNCTIONS---***/
snailBuffer *snailBufferCreate(int initSize) {
	snailBuffer *buf = snailMalloc(sizeof(snailBuffer));
	buf->bytes = malloc(initSize);
	buf->allocated = initSize;
	buf->length = 0;
	return buf;
}

void snailBufferDestroy(snailBuffer *buf) {
	free(buf->bytes);
	free(buf);
}

void snailBufferEmpty(snailBuffer *buf) {
	buf->length = 0;
	memset(buf->bytes, 0, buf->allocated);
}

void snailBufferGrow(snailBuffer *buf, int newSize) {
	if (newSize <= buf->allocated)
		return;
	buf->bytes = snailRealloc(buf->bytes, buf->allocated, newSize);
	buf->allocated = newSize;
}


void snailBufferSet(snailBuffer *buf, char *str) {
	snailBufferEmpty(buf);
	size_t len = strlen(str);
	snailBufferGrow(buf, len+1);
	strcpy(buf->bytes, str);
}

void snailBufferAddChar(snailBuffer *buf, char ch) {
	snailBufferGrow(buf, buf->length + 1);
	buf->bytes[buf->length++] = ch;
}

void snailBufferAddString(snailBuffer *buf, char *str) {
	int length = strlen(str);
	snailBufferGrow(buf, buf->length + length + 1);
	while (*str != 0) {
		buf->bytes[buf->length++] = *(str++);
	}
}

char *snailDupString(char *str) {
	size_t size = strlen(str)+1;
	char *dup = snailMalloc(size);
	memcpy(dup,str,size);
	return dup;
}

uint64_t snailHashBytes(uint8_t* data, uint32_t length) {
	uint64_t hash = 0xcbf29ce484222325;
	for (uint32_t i = 0; i < length; i++) {
		hash = hash ^ data[i];
		hash = hash * 0x100000001b3;
	}
	return hash;
}

uint64_t snailHashString(char *str) {
	return snailHashBytes((uint8_t*) str, strlen(str));
}

noreturn void snailPanic(char *msg) {
	fprintf(stderr, "PANIC: %s\n", msg);
	exit(1);
}

void snailArgMust(bool cond) {
	if (!cond)
		snailPanic("illegal arguments");
}

void * snailMalloc(size_t size) {
	void * r = calloc(1, size);
	if (r != NULL)
		return r;
	snailPanic("out of memory");
}

void * snailRealloc(void *buf, size_t oldSize, size_t newSize) {
	snailArgMust(buf != NULL && oldSize > 0 && newSize > oldSize);
	void *newBuf = realloc(buf, newSize);
	if (newBuf == NULL)
		snailPanic("out of memory");
	size_t diff = newSize - oldSize;
	int8_t *start = newBuf + oldSize;
	memset(start, 0, diff);
	return newBuf;
}

char *snailReadFile(const char *filename) {
	FILE *f;
	long len,r;

	if(!(f = fopen(filename, "rb")))
		return NULL;

	fseek(f, 0, SEEK_END);
	len = ftell(f);
	rewind(f);

	char *str = snailMalloc(len + 2);
	r = fread(str, 1, len, f);

	if(r != len) {
		free(str);
		return NULL;
	}

	fclose(f);
	str[len] = '\0';
	return str;
}

snailParseTool *snailParseCreate(char *script) {
	snailParseTool *p = snailMalloc(sizeof(snailParseTool));
	p->script = script;
	p->length = strlen(script);
	p->word = snailBufferCreate(p->length+1);
	p->context = snailBufferCreate(16);
	p->error = snailBufferCreate(64);
	return p;
}

void snailParseDestroy(snailParseTool *parser) {
	snailBufferDestroy(parser->word);
	snailBufferDestroy(parser->context);
	snailBufferDestroy(parser->error);
	free(parser);
}

bool snailParseDump(char *script) {
	snailParseTool *tool = snailParseCreate(script);
	for (int i = 0; ; i++) {
		snailParseResult r = snailParseNext(tool);
		switch (r) {
		case snailParseResultEnd:
			printf("%d END {%s}\n", i, tool->word->bytes);
			snailParseDestroy(tool);
			return true;
		case snailParseResultIncomplete:
			printf("%d INCOMPLETE {%s}\n", i, tool->word->bytes);
			snailParseDestroy(tool);
			return true;
		case snailParseResultToken:
			printf("%d Token {%s}\n", i, tool->word->bytes);
			break;
		case snailParseResultError:
			printf("%d ERROR {%s}\n", i, tool->word->bytes);
			snailParseDestroy(tool);
			return false;
		}
	}
}

snailParseResult snailParseNext(snailParseTool *parser) {
	char ch;
	snailBufferEmpty(parser->word);
	snailBufferEmpty(parser->error);
	char state = '0';
nextChar:
	ch = parser->pos >= parser->length ? 0 : parser->script[parser->pos];
	switch (state) {
		case '\\': // backslash inside quoted token
			switch (ch) {
				case 0:
					snailBufferAddString(parser->error,"unterminated quoted string");
					snailBufferAddChar(parser->error,0);
					return snailParseResultError;
				case '\\':
				case '"':
				case 'n':
				case 'r':
				case 't':
					snailBufferAddChar(parser->word,ch);
					parser->pos++;
					state = '"';
					goto nextChar;
				default:
					snailBufferAddString(parser->error,"illegal backslash escape: '\\");
					snailBufferAddChar(parser->error,ch);
					snailBufferAddString(parser->error,"'");
					snailBufferAddChar(parser->error,0);
					return snailParseResultError;
			}
		case '"': // quoted token
			switch (ch) {
				case 0:
					snailBufferAddString(parser->error,"unterminated quoted string");
					snailBufferAddChar(parser->error,0);
					return snailParseResultError;
				case '\\':
					snailBufferAddString(parser->word,"\\");
					state = '\\';
					parser->pos++;
					goto nextChar;
				case '"':
					snailBufferAddString(parser->word,"\"");
					parser->pos++;
					if (parser->context->length == 0) {
						snailBufferAddChar(parser->error,0);
						return snailParseResultToken;
					}
					state = '0';
					goto nextChar;
				default:
					snailBufferAddChar(parser->word,ch);
					parser->pos++;
					goto nextChar;
			}
		case '$': // variable
		case '_': // unquoted token
			switch (ch) {
				// Valid token terminators
				case 0:
				case '}':
				case ']':
				case ' ':
				case '\t':
				case '\n':
					if (parser->context->length == 0) {
						snailBufferAddChar(parser->error,0);
						return snailParseResultToken;
					}
					state = '0';
					goto nextChar;
				// Prohibited characters
				case '"':
				case '\\':
				case '{':
				case '[':
				case '(':
				case ')':
					snailBufferAddString(parser->error,"illegal character inside token: '");
					snailBufferAddChar(parser->error,ch);
					snailBufferAddString(parser->error,"'");
					snailBufferAddChar(parser->error,0);
					return snailParseResultError;
				// Allowed character
				default:
					snailBufferAddChar(parser->word,ch);
					parser->pos++;
					goto nextChar;

			}
		case '0': // initial state
			switch (ch) {
				// EOF in initial state
				case 0:
					if (parser->context->length == 0) {
						snailBufferAddChar(parser->error,0);
						return snailParseResultEnd;
					}
					snailBufferAddChar(parser->word,0);
					snailBufferAddChar(parser->context,0);
					return snailParseResultIncomplete;
				// whitespace at start of token, skip outside of list
				case ' ':
				case '\t':
					if (parser->context->length > 0) {
						char last = parser->word->bytes[parser->word->length-1];
						if (last != ' ' && last != '\t' && last != '{' && last != '[')
							snailBufferAddChar(parser->word,' ');
					}
					parser->pos++;
					goto nextChar;
				// newline token
				case '\n':
					parser->pos++;
					if (parser->context->length == 0) {
						snailBufferSet(parser->word,"\n");
						return snailParseResultToken;
					}
					snailBufferAddChar(parser->word,ch);
					state = '0';
					goto nextChar;
				// " - beginning of double quoted string
				// $ - beginning of variable
				// % - could be dictionary or start of unquoted token
				case '$':
				case '"':
				case '%':
					parser->pos++;
					snailBufferAddChar(parser->word,ch);
					state = ch;
					goto nextChar;
				// { - beginning of list
				// [ - beginning of code list
				case '{':
				case '[':
					parser->pos++;
					snailBufferAddChar(parser->word,ch);
					snailBufferAddChar(parser->context,ch);
					goto nextChar;
				// } - end of list
				// ] - end of code list
				case '}':
				case ']':
					if (parser->context->length == 0 ||
					    parser->context->bytes[parser->context->length-1] != (ch == '}' ? '{' : '[')) {
						snailBufferAddString(parser->error,"unbalanced '");
						snailBufferAddChar(parser->error,ch);
						snailBufferAddString(parser->error,"'");
						snailBufferAddChar(parser->error,0);
						return snailParseResultError;
					}
					parser->context->length--;
					parser->pos++;
					char endpre = parser->word->bytes[parser->word->length-1];
					if (endpre == ' ' || endpre == '\t')
						parser->word->length--;
					snailBufferAddChar(parser->word,ch);
					if (parser->context->length == 0) {
						snailBufferAddChar(parser->word,0);
						return snailParseResultToken;
					}
					goto nextChar;
				// Reserved characters, raise an error - one day these may do something
				case '(':
				case ')':
				case '\\':
					snailBufferAddString(parser->error,"illegal character at start of token: '");
					snailBufferAddChar(parser->error,ch);
					snailBufferAddString(parser->error,"'");
					snailBufferAddChar(parser->error,0);
					return snailParseResultError;
				// Any other character, start an unquoted token
				default:
					parser->pos++;
					snailBufferAddChar(parser->word,ch);
					state = '_';
					goto nextChar;
			}
		default:
			snailBufferAddString(parser->error,"unexpected parser state '");
			snailBufferAddChar(parser->error,state);
			snailBufferAddString(parser->error,"'");
			snailBufferAddChar(parser->error,0);
			return snailParseResultError;
	}
}

char * snailParseGetContext(char *script) {
	snailParseTool *tool = snailParseCreate(script);
	char *context;
	for (;;) {
		snailParseResult result = snailParseNext(tool);
		switch (result) {
			case snailParseResultEnd:
			case snailParseResultError:
				snailParseDestroy(tool);
				return NULL;
			case snailParseResultIncomplete:
				context = snailDupString(tool->context->bytes);
				snailParseDestroy(tool);
				return context;
			default:
				continue;
		}
	}
}

snailStatus snailExec(snailInterp *snail, char *script) {
	snailClearResult(snail);
	snailParseTool *tool = snailParseCreate(script);
	snailArray *words = snailArrayCreate(8);
	snailParseResult result;
nextWord:
	result = snailParseNext(tool);
	switch (result) {
	case snailParseResultError:
		snailSetResult(snail, tool->error->bytes);
		snailParseDestroy(tool);
		snailArrayDestroy(words, free);
		return snailStatusError;
	case snailParseResultEnd:
		goto stmtDone;
	case snailParseResultToken:
		if (tool->word->bytes[0] == '\n') {
			goto stmtDone;
		}
		if (tool->word->bytes[0] == '$') {
			char *word = snailGetVar(snail, tool->word->bytes+1);
			if (word == NULL) {
				char *msg = snailMakeVarNotFoundError(tool->word->bytes);
				snailSetResult(snail, msg);
				free(msg);
				snailParseDestroy(tool);
				snailArrayDestroy(words, free);
				return snailStatusError;
			}
			snailArrayAdd(words, word);
			word = NULL;
			goto nextWord;
		} else if (tool->word->bytes[0] == '[') {
			snailStatus ss = snailExecSub(snail,tool->word->bytes);
			if (ss != snailStatusOk) {
				snailParseDestroy(tool);
				snailArrayDestroy(words, free);
				return ss;
			}
			char *word = snailDupString(snail->result);
			snailArrayAdd(words, word);
			word = NULL;
			goto nextWord;
		} else {
			char *word = snailDupString(tool->word->bytes);
			snailArrayAdd(words, word);
			word = NULL;
			goto nextWord;
		}
	default:
		printf("Parse unknown result [%d]\n", result);
		snailPanic("Parse unknown result");
	}
stmtDone:
	if (words->length == 0) {
		if (result == snailParseResultEnd) {
			snailParseDestroy(tool);
			snailArrayDestroy(words, free);
			return snailStatusOk;
		}
		goto nextWord;
	}

	char *cmdName = words->elems[0];
	int argCount = words->length - 1;
	char **args = (char**) &(words->elems[1]);
	snailStatus status = snailRunCommand(snail, cmdName, argCount, args);
	snailArrayEmpty(words, free);

	switch (status) {
	case snailStatusError:
	case snailStatusReturn:
	case snailStatusBreak:
	case snailStatusContinue:
		snailParseDestroy(tool);
		snailArrayDestroy(words, free);
		return status;
	case snailStatusOk:
		goto nextWord;
	default:
		snailSetResult(snail, "unexpected command result");
		snailParseDestroy(tool);
		snailArrayDestroy(words, free);
		return snailStatusError;
	}
}

void snailRepl(snailInterp *snail) {
	char *buffer;

	printf("=(@)= Snail interpreter =(@)=\n\n");
	printf("Interactive mode; press Ctrl-D to exit\n%s", snail->repl->prompt);
	for (;;) {
loopRepl:
		buffer = NULL;
		size_t n = 0;
		ssize_t rc = getline(&buffer, &n, stdin);
		if (rc < 0) {
			free(buffer);
			goto exitRepl;
		}
		if (rc == 0) {
			free(buffer);
			goto loopRepl;
		}

		char *context = snailParseGetContext(buffer);
		if (context != NULL) {
			buffer = snailReplContext(context, buffer);
		}
		if (buffer == NULL)
			goto exitRepl;

		if (snailIsBlank(buffer)) {
			free(buffer);
			printf("%s", snail->repl->prompt);
			goto loopRepl;
		}
		snailStatus status = snailExec(snail, buffer);
		if (status == snailStatusOk)
			printf("ok: %s\n", snailGetResult(snail));
		else if (status == snailStatusError)
			printf("error: %s\n", snailGetResult(snail));
		else if (status == snailStatusReturn)
			printf("return: %s\n", snailGetResult(snail));
		else if (status == snailStatusBreak)
			printf("break: %s\n", snailGetResult(snail));
		else if (status == snailStatusContinue)
			printf("continue: %s\n", snailGetResult(snail));
		else
			snailPanic("unexpected status returned by snailExec");
		printf("%s", snail->repl->prompt);
		free(buffer);
	}
exitRepl:
	printf("\n");
}

int snailRunFile(snailInterp *snail, char *fileName) {
	char *script = snailReadFile(fileName);
	if (!script) {
		fprintf(stderr, "error: unable to read %s\n", fileName);
		return 1;
	}
	snailStatus status = snailExec(snail, script);
	if(status == snailStatusError) {
		fprintf(stderr, "error: %s\n", snailGetResult(snail));
		return 1;
	}
	free(script);
	return 0;
}

const int snailInitialFrameCount = 32;
const int snailInitialGlobalTableSize = 512;
const int snailCommandTableSize = 64;

snailCallFrame *snailCallFrameCreate(char *cmdName) {
	snailCallFrame *frame = snailMalloc(sizeof(snailCallFrame));
	frame->cmdName = snailDupString(cmdName);
	frame->vars = snailHashTableCreate(8);
	return frame;
}

void snailCallFrameDestroy(snailCallFrame *frame) {
	free(frame->cmdName);
	snailHashTableDestroy(frame->vars,free);
	free(frame);
}

char *snailGetVar(snailInterp *snail, char *name) {
	snailCallFrame *frame = snail->frames->elems[snail->framePointer];
	return snailHashTableGet(frame->vars,name);
}

void snailSetVar(snailInterp *snail, char *name, char *value) {
	snailCallFrame *frame = snail->frames->elems[snail->framePointer];
	free(snailHashTablePut(frame->vars,name,value));
}

snailInterp *snailCreate(void) {
	snailInterp *snail = snailMalloc(sizeof(snailInterp));
	snail->frames = snailArrayCreate(snailInitialFrameCount);
	snailArrayAdd(snail->frames, snailCallFrameCreate("(top)"));
	snail->globals = snailHashTableCreate(snailInitialGlobalTableSize);
	snail->commands =  snailHashTableCreate(snailCommandTableSize);
	snail->repl = snailReplStateCreate();
	snailRegisterNatives(snail);
	return snail;
}

void snailDestroyCommand(snailCommand *cmd) {
	free(cmd->name);
	free(cmd);
}

void snailDestroy(snailInterp *snail) {
	if (snail == NULL)
		return;
	free(snail->result);
	snailArrayDestroy(snail->frames,(void*)snailCallFrameDestroy);
	snailHashTableDestroy(snail->globals,free);
	snailHashTableDestroy(snail->commands, (void*)snailDestroyCommand);
	snailReplStateDestroy(snail->repl);
	free(snail);
}

void snailClearResult(snailInterp *snail) {
	snailSetResult(snail, "");
}

char *snailGetResult(snailInterp *snail) {
	if (snail->result == NULL)
		snailClearResult(snail);
	return snail->result;
}

snailHashTable * snailHashTableCreate(int size) {
	snailHashTable *ht = snailMalloc(sizeof(snailHashTable));
	ht->numberOfBuckets = size;
	ht->buckets = snailMalloc(size*sizeof(snailHashCell*));
	return ht;
}

char * snailHashTableFirst(snailHashTable *ht) {
	for (int i = 0; i < ht->numberOfBuckets; i++)
		if (ht->buckets[i] != NULL)
			return ht->buckets[i]->key;
	return NULL;
}

char * snailHashTableNext(snailHashTable *ht, char *key) {
	uint64_t index = snailHashString(key) % ht->numberOfBuckets;
	snailHashCell *cur = ht->buckets[index];
	while (cur != NULL) {
		if (strcmp(cur->key,key) == 0 && cur->next != NULL)
			return cur->next->key;
		cur = cur->next;
	}
	for (int i = index+1; i < ht->numberOfBuckets; i++) {
		if (ht->buckets[i] != NULL)
			return ht->buckets[i]->key;
	}
	return NULL;
}

void snailHashCellDestroy(snailHashCell *cell, snailDestructor *destructor) {
	for (;;) {
		if (cell == NULL)
			return;
		snailHashCell *next = cell->next;
		destructor(cell->value);
		free(cell->key);
		free(cell);
		cell = next;
	}
}

void snailHashTableDestroy(snailHashTable *ht, snailDestructor *destructor) {
	if (ht == NULL)
		return;
	for (int i = 0; i < ht->numberOfBuckets; i++)
		snailHashCellDestroy(ht->buckets[i], destructor);
	free(ht->buckets);
	free(ht);
}

void *snailHashTableGet(snailHashTable *ht, char *key) {
	uint64_t index = snailHashString(key) % ht->numberOfBuckets;
	if (ht->buckets[index] == NULL)
		return NULL;

	snailHashCell *cur = ht->buckets[index];
	while (cur != NULL) {
		if (strcmp(cur->key,key) == 0) {
			return cur->value;
		}
		cur = cur->next;
	}
	return NULL;
}

void *snailHashTablePut(snailHashTable *ht, char *key, void *value) {
	uint64_t index = snailHashString(key) % ht->numberOfBuckets;
	if (ht->buckets[index] == NULL) {
		snailHashCell *cell = snailMalloc(sizeof(snailHashCell));
		cell->key = snailDupString(key);
		cell->value = value;
		ht->buckets[index] = cell;
		ht->numberOfCells++;
		return NULL;
	}

	snailHashCell *cur = ht->buckets[index];
	while (cur != NULL) {
		if (strcmp(cur->key,key) == 0) {
			void *old = cur->value;
			cur->value = value;
			return old;
		}
		cur = cur->next;
	}

	snailHashCell *cell = snailMalloc(sizeof(snailHashCell));
	cell->key = snailDupString(key);
	cell->value = value;
	cell->next = ht->buckets[index];
	ht->buckets[index] = cell;
	ht->numberOfCells++;
	return NULL;
}

char *snailU64ToStr(uint64_t n) {
	char *s = snailMalloc(20);
	snprintf(s, 20, "%llu", n);
	return s;
}

char *snailI64ToStr(int64_t n) {
	char *s = snailMalloc(20);
	snprintf(s, 20, "%lld", n);
	return s;
}

bool snailArgCountExactly(snailInterp *snail, char *cmdName, int must, int actual) {
	if (must == actual)
		return true;
	char *iMust = snailI64ToStr(must);
	char *iActual = snailI64ToStr(actual);

	snailBuffer *msg = snailBufferCreate(16);
	snailBufferAddString(msg, "command \"");
	snailBufferAddString(msg, cmdName);
	snailBufferAddString(msg, "\" requires exactly ");
	snailBufferAddString(msg, iMust);
	snailBufferAddString(msg, " arguments, got ");
	snailBufferAddString(msg, iActual);
	snailBufferAddString(msg, " instead");

	free(iMust);
	free(iActual);

	snailSetResult(snail, msg->bytes);
	snailBufferDestroy(msg);
	return false;
}

void snailRegisterNative(snailInterp *snail, char *name, int arity, snailNative *impl) {
	// Translate internal native name to actual native name
	// This is used for native whose names are not allowed per C syntax, e.g. +, -, *, /, etc
	name = snailTranslateNative(name);

	// Convert _ to . in native command name
	name = snailDupString(name);
	for (int i = 0; name[i] != 0; i++) {
		if (name[i] == '_')
			name[i] = '.';
	}

	// Allocate and populate the command object
	snailCommand *cmd = snailMalloc(sizeof(snailCommand));
	cmd->name = name;
	cmd->arity = arity;
	cmd->native = impl;

	// Ensure no duplication of native commands
	if (snailHashTableGet(snail->commands, name) != NULL) {
		printf("ERROR: duplicate native command '%s'\n", name);
		snailPanic("duplicate native command");
	}

	// Register the native command
	snailHashTablePut(snail->commands, name, cmd);
}

snailArray *snailArrayCreate(int initSize) {
	snailArray *buf = snailMalloc(sizeof(snailArray));
	buf->elems = malloc(initSize * sizeof(void*));
	buf->allocated = initSize;
	buf->length = 0;
	return buf;
}

void snailArrayDestroy(snailArray *array, snailDestructor *destructor) {
	if (destructor != NULL)
		for (int i = 0; i < array->length; i++)
			if (array->elems[i] != NULL)
				destructor(array->elems[i]);
	free(array->elems);
	free(array);
}

void snailArrayEmpty(snailArray *array, snailDestructor *destructor) {
	array->length = 0;
	if (destructor != NULL)
		for (int i = 0; i < array->length; i++)
			if (array->elems[i] != NULL)
				destructor(array->elems[i]);
	memset(array->elems, 0, array->allocated * sizeof(void*));
}

void snailArrayGrow(snailArray *array, int newSize) {
	if (newSize <= array->allocated)
		return;
	array->elems = snailRealloc(array->elems, array->allocated * sizeof(void*), newSize * sizeof(void*));
	array->allocated = newSize;
}

void snailArrayAdd(snailArray *array, void *element) {
	snailArrayGrow(array, array->length + 1);
	array->elems[array->length++] = element;
}

snailStatus snailRunCommand(snailInterp *snail, char *cmdName, int argCount, char **args) {
	// Get command from command table
	snailCommand *cmd = snailHashTableGet(snail->commands, cmdName);

	// Raise error if command does not exist
	if (cmd == NULL) {
		snailBuffer *msg = snailBufferCreate(16);
		snailBufferAddString(msg, "unknown command \"");
		snailBufferAddString(msg, cmdName);
		snailBufferAddString(msg, "\"");
		snailBufferAddChar(msg, 0);
		snailSetResult(snail, msg->bytes);
		snailBufferDestroy(msg);
		return snailStatusError;
	}

	// Check command arity
	if (cmd->arity >= 0)
		if (!snailArgCountExactly(snail, cmdName, cmd->arity, argCount))
			return snailStatusError;

	// Handle native command
	if (cmd->native != NULL) {
		return cmd->native(snail, cmdName, argCount, args);
	}

	// Should never happen: command has neither native nor script
	if (cmd->script == NULL) {
		snailBuffer *msg = snailBufferCreate(16);
		snailBufferAddString(msg, "command \"");
		snailBufferAddString(msg, cmdName);
		snailBufferAddString(msg, "\" has neither script nor native");
		snailBufferAddChar(msg, 0);
		snailSetResult(snail, msg->bytes);
		snailBufferDestroy(msg);
		return snailStatusError;
	}

	// Handle script command
	snailCallFrame *frame = snailCallFrameCreate(cmdName);
	snailArrayAdd(snail->frames, frame);
	snailArray *argDefs = snailUnquoteList(cmd->args);
	for (int i = 0, j = 0; i < argDefs->length; i++) {
		char *argDef = argDefs->elems[i];
		if (j >= argCount)
			break;
		if (argDef[0] == '$') {
			snailSetVar(snail, argDef+1, args[j++]);
		}
	}
	snailStatus ss = snailExec(snail, cmd->script);
	snailArrayShift(snail->frames);
	snailCallFrameDestroy(frame);
	snailArrayDestroy(argDefs,free);
	return ss;
}

void snailSetResult(snailInterp *snail, char *result) {
	if (snail->result != NULL) {
		free(snail->result);
		snail->result = NULL;
	}
	if (result[0]!=0 && !snailTokenIsValid(result)) {
		snail->result = snailMakeQuoted(result);
	}
	else
		snail->result = snailDupString(result);
}

bool snailIsBlank(char *str) {
	for (;;) {
		char ch = *str;
		str++;
		switch (ch) {
		case 0:
			return true;
		case ' ':
		case '\t':
		case '\n':
		case '\r':
			continue;
		default:
			return false;
		}
	}
}

char *snailTrimString(char *str) {
	while (isspace(*str))
		str++;
	char *trim = snailDupString(str);
	int length = strlen(trim);
	for (int i = length-1; i >= 0; i--) {
		if (isspace(trim[i]))
			trim[i] = 0;
		else
			break;
	}
	return trim;
}

bool snailIsDigits(char *str) {
	if (str[0] == 0)
		return false;
	if (str[0] == '0')
		return str[1] == 0;
	for (int i = 0; str[i] != 0; i++) {
		if (!isdigit(str[i]))
			return false;
	}
	return true;
}

bool snailIsInt(char *str) {
	return (*str == '-' && snailIsDigits(str + 1)) || snailIsDigits(str);
}

#define NATIVE_NAME(_actual,_internal) \
	if (strcmp(str,_internal) == 0) return _actual

char *snailTranslateNative(char *str) {
#	include "snailNativeNames.h"
	return str;
}

bool snailArgCountMinimum(snailInterp *snail, char *cmdName, int minimum, int actual) {
	if (actual >= minimum)
		return true;
	char *iMinimum = snailI64ToStr(minimum);
	char *iActual = snailI64ToStr(actual);

	snailBuffer *msg = snailBufferCreate(16);
	snailBufferAddString(msg, "command \"");
	snailBufferAddString(msg, cmdName);
	snailBufferAddString(msg, "\" requires minimum ");
	snailBufferAddString(msg, iMinimum);
	snailBufferAddString(msg, " arguments, got ");
	snailBufferAddString(msg, iActual);
	snailBufferAddString(msg, " instead");
	snailBufferAddChar(msg, 0);

	free(iMinimum);
	free(iActual);

	snailSetResult(snail, msg->bytes);
	snailBufferDestroy(msg);
	return false;
}

void snailSetResultBool(snailInterp *snail, bool result) {
	snailSetResult(snail, result ? "t" : "f");
}

void snailSetResultInt(snailInterp *snail, int64_t result) {
	char *str = snailI64ToStr(result);
	snailSetResult(snail, str);
	free(str);
}

bool snailIsBool(char *str) {
	return snailIsTrue(str) || snailIsFalse(str);
}

bool snailIsTrue(char *str) {
	return strcasecmp(str,"t") == 0;
}

bool snailIsFalse(char *str) {
	return strcasecmp(str,"f") == 0;
}

char * snailReplContext(char *context, char *buffer) {
	snailBuffer *b = snailBufferCreate(strlen(buffer)+1);
	snailBufferAddString(b,buffer);
	free(buffer);

	snailBuffer *p = snailBufferCreate(strlen(context)+1);
	snailBufferAddString(p,context);
	snailBufferAddChar(p,0);
	free(context);

	for (;;) {
		printf("%s ", p->bytes);

		buffer = NULL;
		size_t n = 0;
		ssize_t rc = getline(&buffer, &n, stdin);
		if (rc < 0) {
			snailBufferDestroy(b);
			snailBufferDestroy(p);
			return NULL;
		}
		if (rc == 0)
			continue;
		snailBufferAddString(b,buffer);
		snailBufferAddChar(b,0);
		context = snailParseGetContext(b->bytes);
		if (context == NULL) {
			free(buffer);
			buffer = snailDupString(b->bytes);
			snailBufferDestroy(b);
			snailBufferDestroy(p);
			return buffer;
		}
		snailBufferEmpty(p);
		snailBufferAddString(p,context);
		free(context);
		context = NULL;
		b->length--;
	}
}

snailArray *snailUnquoteList(char *str) {
	int len = strlen(str);
	if (str[0] != '{' || str[len-1] != '}') {
		return NULL;
	}
	char *body = snailDupString(str+1);
	body[len-2] = 0;
	snailArray *array = snailParseList(body);
	free(body);
	return array;
}

snailArray *snailParseList(char *str) {
	snailParseTool *t = snailParseCreate(str);
	snailArray *a = snailArrayCreate(8);
	for (;;) {
		snailParseResult r = snailParseNext(t);
		if (r == snailParseResultEnd) {
			break;
		}
		else if (r == snailParseResultToken) {
			snailArrayAdd(a, snailDupString(t->word->bytes));
		}
		else {
			snailArrayDestroy(a, free);
			return NULL;
		}
	}
	return a;
}

char *snailMakeVarNotFoundError(char *name) {
	snailBuffer *msg = snailBufferCreate(32);
	snailBufferAddString(msg, "variable not found: \"");
	snailBufferAddString(msg, name);
	snailBufferAddChar(msg, '"');
	snailBufferAddChar(msg, 0);
	char *r = snailDupString(msg->bytes);
	snailBufferDestroy(msg);
	return r;
}

void *snailHashTableDelete(snailHashTable *ht, char *key) {
	uint64_t index = snailHashString(key) % ht->numberOfBuckets;
	snailHashCell *cur = ht->buckets[index];
	if (cur == NULL)
		return NULL;
	if (strcmp(cur->key, key) == 0) {
		ht->buckets[index] = cur->next;
		void *old = cur->value;
		free(cur->key);
		free(cur);
		ht->numberOfCells--;
		return old;
	}
	snailHashCell *prev = cur;
	cur = cur->next;

	while (cur != NULL) {
		if (strcmp(cur->key,key) == 0) {
			prev->next = cur->next;
			void *old = cur->value;
			free(cur->key);
			free(cur);
			ht->numberOfCells--;
			return old;
		}
		prev = cur;
		cur = cur->next;
	}
	return NULL;
}

char snailTokenClassify(char *script) {
	if (script[0] == 0)
		return 'Z';
	script = snailTokenNormalize(script);
	if (script == NULL)
		return 0;
	switch (script[0]) {
		case '"':
			return 'Q';
		case '{':
			return 'L';
		case '[':
			return 'X';
		case '%':
			return script[1] == '{' ? 'D' : 'U';
		case '$':
			return 'V';
		case '\n':
			return 'N';
		default:
			return 'U';
	}
}

char *snailTokenUnquote(char *script) {
	if (script[0] != '"')
		return NULL;
	snailBuffer *b = snailBufferCreate(strlen(script)+3);
	for (int i = 1; ; i++) {
		switch (script[i]) {
			case '\\':
				i++;
				switch (script[i]) {
					case 'n':
						snailBufferAddChar(b,'\n');
						break;
					case 'r':
						snailBufferAddChar(b,'\r');
						break;
					case 't':
						snailBufferAddChar(b,'\t');
						break;
					case '\\':
					case '\"':
						snailBufferAddChar(b, script[i]);
						break;
					default:
						snailBufferDestroy(b);
						return NULL;
				}
				break;
			case '"':
				i++;
				if (script[i] == 0) {
					char *r = snailDupString(b->bytes);
					snailBufferDestroy(b);
					return r;
				}
				snailBufferDestroy(b);
				return NULL;
			default:
				snailBufferAddChar(b, script[i]);
				break;
		}
	}
}

char *snailTokenQuote(char *script) {
	script = snailTokenNormalize(script);
	if (script == NULL)
		return NULL;
	char *quoted = snailMakeQuoted(script);
	free(script);
	return quoted;
}

bool snailTokenIsNormalized(char *script) {
	char *normalized = snailTokenNormalize(script);
	bool r = normalized == NULL && strcmp(normalized,script) == 0;
	free(normalized);
	return r;
}

bool snailTokenIsValid(char *script) {
	char *token = snailTokenNormalize(script);
	if (token == NULL)
		return false;
	free(token);
	return true;
}

char * snailTokenNormalize(char *script) {
	snailParseTool *tool = snailParseCreate(script);
	snailParseResult r = snailParseNext(tool);
	if (r != snailParseResultToken) {
		snailParseDestroy(tool);
		return NULL;
	}
	char *token = snailDupString(tool->word->bytes);
	r = snailParseNext(tool);
	if (r != snailParseResultEnd) {
		free(token);
		snailParseDestroy(tool);
		return NULL;
	}
	snailParseDestroy(tool);
	return token;
}

char *snailMakeQuoted(char *str) {
	snailBuffer *b = snailBufferCreate(strlen(str)+3);
	snailBufferAddChar(b, '"');
	for (int i = 0; str[i] != 0; i++) {
		char c = str[i];
		switch (c) {
			case '\n':
				snailBufferAddString(b,"\\n");
				break;
			case '\r':
				snailBufferAddString(b,"\\r");
				break;
			case '\t':
				snailBufferAddString(b,"\\t");
				break;
			case '"':
				snailBufferAddString(b,"\\\"");
				break;
			case '\\':
				snailBufferAddString(b,"\\\\");
				break;
			default:
				snailBufferAddChar(b,c);
		}
	}
	snailBufferAddChar(b, '"');
	snailBufferAddChar(b, 0);
	char *result = snailDupString(b->bytes);
	snailBufferDestroy(b);
	return result;
}

snailStatus snailExecSub(snailInterp *snail, char *code) {
	int len = strlen(code);
	if (code[0] != '[' || code[len-1] != ']') {
		snailSetResult(snail, "bad code block syntax");
		return snailStatusError;
	}
	char *script = snailDupString(code);
	script[len-1] = 0;
	snailStatus ss = snailExec(snail, script+1);
	free(script);
	return ss;
}

snailStatus snailExecList(snailInterp *snail, char *code) {
	int len = strlen(code);
	if (code[0] != '{' || code[len-1] != '}') {
		snailSetResult(snail, "bad list syntax");
		return snailStatusError;
	}
	char *script = snailDupString(code);
	script[len-1] = 0;
	snailStatus ss = snailExec(snail, script+1);
	free(script);
	return ss;
}

snailReplState *snailReplStateCreate(void) {
	snailReplState *repl = snailMalloc(sizeof(snailReplState));
	repl->historyMax = 20;
	repl->history = snailArrayCreate(repl->historyMax);
	repl->prompt = snailDupString("(@) ");
	return repl;
}

void snailReplStateDestroy(snailReplState *repl) {
	snailArrayDestroy(repl->history,free);
	free(repl->prompt);
	free(repl);
}

void snailReplHistoryAdd(snailReplState *repl, char *buffer) {
	snailArrayAdd(repl->history,snailDupString(buffer));
	if (repl->historyMax >= 0)
		while (repl->history->length > repl->historyMax) {
			free(snailArrayPop(repl->history));
		}
}

void * snailArrayPop(snailArray *array) {
	if (array->length == 0)
		return NULL;
	void *first = array->elems[0];
	for (int i = 1; i < array->length; i++)
		array->elems[i-1] = array->elems[i];
	array->elems[array->length-1] = NULL;
	array->length--;
	return first;
}

void *snailArrayShift(snailArray *array) {
	if (array->length == 0)
		return NULL;
	void *last = array->elems[array->length-1];
	array->length--;
	return last;
}

char *snailQuoteList(snailArray *list) {
	snailBuffer *buf = snailBufferCreate(1024);
	snailBufferAddChar(buf,'{');
	for (int i = 0; i < list->length; i++) {
		if (buf->length > 1)
			snailBufferAddChar(buf, ' ');
		snailBufferAddString(buf, list->elems[i]);
	}
	snailBufferAddChar(buf,'}');
	snailBufferAddChar(buf, 0);
	char *r = snailDupString(buf->bytes);
	snailBufferDestroy(buf);
	return r;
}

void snailArraySort(snailArray *list, snailArrayComparator *cmp) {
	qsort_r(list->elems, list->length,  sizeof(void *), cmp, (void*)snailArraySortCmp);
}

int snailArraySortCmp(void *thunk, const void **a, const void **b) {
	snailArrayComparator *cmp = thunk;
	return cmp(*a, *b);
}
