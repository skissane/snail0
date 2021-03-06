/***---FUNCTIONS---***/
snailBuffer *snailBufferCreate(int initSize) {
	snailBuffer *buf = snailMalloc(sizeof(snailBuffer));
	buf->bytes = snailMalloc(initSize);
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

void snailBufferAddString(snailBuffer *buf, const char *str) {
	int length = strlen(str);
	snailBufferGrow(buf, buf->length + length + 1);
	while (*str != 0) {
		buf->bytes[buf->length++] = *(str++);
	}
}

void snailBufferAddData(snailBuffer *buf, const void *data, size_t length) {
	snailBufferGrow(buf, buf->length + length + 1);
	for (size_t i = 0; i < length; i++) {
		buf->bytes[buf->length++] = ((char*)data)[i];
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

char *snailReadFileHex(const char *filename) {
	FILE *f;
	long len,r;

	if(!(f = fopen(filename, "rb")))
		return NULL;

	fseek(f, 0, SEEK_END);
	len = ftell(f);
	rewind(f);

	char *buf = snailMalloc(len + 2);
	r = fread(buf, 1, len, f);

	if(r != len) {
		free(buf);
		return NULL;
	}

	fclose(f);
	char *hex = snailHexEncode(buf,len);
	free(buf);
	return hex;
}

snailParseTool *snailParseCreate(const char *script) {
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
				case '\r':
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
		case '%': // could be dict or unquoted token
			switch (ch) {
				case '{': // Start of dictionary
					parser->pos++;
					snailBufferAddChar(parser->word,ch);
					snailBufferAddChar(parser->context,ch);
					state = '0';
					goto nextChar;
				case '[': // disallowed characters
				case '$':
				case '"':
					snailBufferAddString(parser->error,"illegal character sequence inside token: '%");
					snailBufferAddChar(parser->error,ch);
					snailBufferAddString(parser->error,"'");
					snailBufferAddChar(parser->error,0);
					return snailParseResultError;
				// Valid token terminators
				case 0:
				case '}':
				case ']':
				case ' ':
				case '\t':
				case '\r':
				case '\n':
					if (parser->context->length == 0) {
						snailBufferAddChar(parser->error,0);
						return snailParseResultToken;
					}
					state = '0';
					goto nextChar;
				// Any other character, start an unquoted token
				default:
					parser->pos++;
					snailBufferAddChar(parser->word,ch);
					state = '_';
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
				case '\r':
					if (parser->context->length > 0) {
						char last = parser->word->bytes[parser->word->length-1];
						if (last != ' ' && last != '\t' && last != '\r' && last != '{' && last != '[')
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

snailStatus snailExec(snailInterp *snail, const char *script) {
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
	case snailParseResultIncomplete:
		snailSetResult(snail, "unexpected EOF");
		snailParseDestroy(tool);
		snailArrayDestroy(words, free);
		return snailStatusError;
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

#ifdef __DJGPP__
#define REPL_EXIT_CHAR 'Z'
#else
#define REPL_EXIT_CHAR 'D'
#endif

void snailRepl(snailInterp *snail) {
	char *buffer;

	printf("=(@)= Snail interpreter =(@)=\n\n");
	printf("Interactive mode; press Ctrl-%c to exit\n%s", REPL_EXIT_CHAR, snail->repl->prompt);
	fflush(stdout);
	for (;;) {
loopRepl:
		buffer = NULL;
		size_t n = 0;
		ssize_t rc = snailReplRead(snail, &buffer, &n);
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
			buffer = snailReplContext(snail, context, buffer);
		}
		if (buffer == NULL)
			goto exitRepl;

		if (snailIsBlank(buffer)) {
			free(buffer);
			printf("%s", snail->repl->prompt);
			fflush(stdout);
			goto loopRepl;
		}
		snailReplHistoryAdd(snail->repl, buffer);
		snailStatus status = snailExec(snail, buffer);
		snailPrintResult(snail,status);
		printf("%s", snail->repl->prompt);
		fflush(stdout);
		free(buffer);
	}
exitRepl:
	printf("\n");
}

bool snailRunScript(snailInterp *snail, char *script) {
	snailStatus status = snailExec(snail, script);
	snailPrintResult(snail, status);
	return status != snailStatusError;
}

int snailRunFile(snailInterp *snail, char *fileName) {
	char *script = snailReadFile(fileName);
	if (!script) {
		fprintf(stderr, "error: unable to read %s\n", fileName);
		return 1;
	}
	snailStatus status = snailExec(snail, snailStripShebang(script));
	if(status == snailStatusError) {
		fprintf(stderr, "error: %s\n", snailGetResult(snail));
		return 1;
	}
	free(script);
	return 0;
}

void snailPrintResult(snailInterp *snail, snailStatus status) {
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
	snailCallFrame *frame = snail->frames->elems[snail->frames->length-1];
	return snailHashTableGet(frame->vars,name);
}

void snailSetVar(snailInterp *snail, char *name, char *value) {
	snailCallFrame *frame = snail->frames->elems[snail->frames->length-1];
	free(snailHashTablePut(frame->vars,name,value));
}

bool snailSetUpVar(snailInterp *snail, int level, char *name, char *value) {
	if (level < 0)
		return false;
	int off = snail->frames->length-1 - level;
	if (off < 0)
		return false;
	snailCallFrame *frame = snail->frames->elems[off];
	free(snailHashTablePut(frame->vars,name,value));
	return true;
}

char * snailGetUpVar(snailInterp *snail, int level, char *name) {
	if (level < 0)
		return NULL;
	int off = snail->frames->length-1 - level;
	if (off < 0)
		return NULL;
	snailCallFrame *frame = snail->frames->elems[off];
	return snailHashTableGet(frame->vars,name);
}

void snailRunInitScript(snailInterp *snail) {
	snailStatus ss = snailExec(snail, snailInitScript);
	if (ss != snailStatusOk) {
		snailBuffer *msg = snailBufferCreate(16);
		snailBufferAddString(msg,"execution of snail init script failed: ");
		snailBufferAddString(msg,snail->result);
		snailBufferAddChar(msg,0);
		snailPanic(msg->bytes);
	}
}

snailInterp *snailCreate(void) {
	snailInterp *snail = snailMalloc(sizeof(snailInterp));
	snail->startupTime = snailTimeNow();
	snail->frames = snailArrayCreate(snailInitialFrameCount);
	snailArrayAdd(snail->frames, snailCallFrameCreate("(top)"));
	snail->globals = snailHashTableCreate(snailInitialGlobalTableSize);
	snail->commands =  snailHashTableCreate(snailCommandTableSize);
	snail->repl = snailReplStateCreate();
	snail->channelDrivers = snailHashTableCreate(16);
	snail->channels = snailHashTableCreate(16);
	snailRegisterNatives(snail);
	snailChannelSetup(snail);
	snailRunInitScript(snail);
	return snail;
}

void snailDestroyCommand(snailCommand *cmd) {
	free(cmd->name);
	free(cmd->args);
	free(cmd->script);
	if (cmd->meta != NULL)
		snailHashTableDestroy(cmd->meta,free);
	free(cmd);
}

void snailDestroy(snailInterp *snail) {
	if (snail == NULL)
		return;
	if (snail->atExit != NULL) {
		for (int i = 0; i < snail->atExit->length; i++) {
			snailStatus ss = snailExec(snail,snail->atExit->elems[i]);
			if (ss != snailStatusOk) {
				printf("error: exit handler {%s} failed with result: %s\n", (char*)(snail->atExit->elems[i]), snail->result);
			}
		}
	}
	free(snail->result);
	snailArrayDestroy(snail->frames,(void*)snailCallFrameDestroy);
	snailHashTableDestroy(snail->globals,free);
	snailHashTableDestroy(snail->commands, (void*)snailDestroyCommand);
	snailReplStateDestroy(snail->repl);
	snailHashTableDestroy(snail->channels, (void*)snailChannelClose);
	snailHashTableDestroy(snail->channelDrivers, (void*)snailChannelDriverDestroy);
	if (snail->atExit != NULL)
		snailArrayDestroy(snail->atExit,free);
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
	snprintf(s, 20, "%" PRIu64, n);
	return s;
}

char *snailI64ToStr(int64_t n) {
	char *s = snailMalloc(20);
	snprintf(s, 20, "%" PRId64, n);
	return s;
}

char *snailU64ToStr16(uint64_t n) {
	char *s = snailMalloc(20);
	snprintf(s, 20, "%" PRIX64, n);
	return s;
}

uint32_t snail2PowMinus1U32(uint32_t n) {
	return n ? (~(uint32_t)0) >> (32u - n) : 0;
}

void snailBufferReverse(snailBuffer *buf) {
	if (buf->length <= 1)
		return;
	int i = buf->length - 1;
	int j = 0;
	while (i > j) {
		char temp = buf->bytes[i];
		buf->bytes[i] = buf->bytes[j];
		buf->bytes[j] = temp;
		i--;
		j++;
	}
}

char *snailI32ToStrRadix(int32_t n, uint8_t base) {
	const char digits[] = "0123456789ABCDEF";
	snailBuffer *buf = snailBufferCreate(16);
	bool negative = false;
	if (base < 2 || base > 16) {
		return NULL;
	}
	if (n < 0) {
		negative = true;
		n = -n;
	}
	if (n == 0)
		snailBufferAddChar(buf,'0');
	else {
		for (int32_t i = 30; n && i; --i, n /= base)
			snailBufferAddChar(buf, digits[n % base]);
		if (negative)
			snailBufferAddChar(buf,'-');
		snailBufferReverse(buf);
	}
	snailBufferAddChar(buf,0);
	char *q = snailMakeQuoted(buf->bytes);
	snailBufferDestroy(buf);
	return q;
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
	buf->elems = snailMalloc(initSize * sizeof(void*));
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

	// Try unknown command handler if command does not exist
	if (cmd == NULL) {
		snailCommand *cmdUnknown = snailHashTableGet(snail->commands, "unknown");
		if (cmdUnknown != NULL) {
			snailArray *args2 = snailArrayCreate(argCount);
			for (int i = 0; i < argCount; i++) {
				snailArrayAdd(args2, args[i]);
			}
			char *result = snailQuoteList(args2);
			snailArrayDestroy(args2, NULL);
			char * words[2];
			words[0] = snailDupString(cmdName);
			words[1] = snailDupString(result);
			snailStatus statusUnknown = snailRunCommand(snail, "unknown", 2, words);
			free(result);
			return statusUnknown;
		}
	}

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
			snailSetVar(snail, argDef+1, snailDupString(args[j++]));
		}
	}
	snailStatus ss = snailExec(snail, cmd->script);
	snailArrayShift(snail->frames);
	snailCallFrameDestroy(frame);
	snailArrayDestroy(argDefs,free);
	if (ss == snailStatusReturn)
		ss = snailStatusOk;
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

bool snailIsBlank(const char *str) {
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

bool snailIsDigits(const char *str) {
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

bool snailIsDigitsAllowInitialZeroes(const char *str) {
	if (str[0] == 0)
		return false;
	for (int i = 0; str[i] != 0; i++) {
		if (!isdigit(str[i]))
			return false;
	}
	return true;
}

bool snailIsInt(const char *str) {
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

bool snailArgCountMaximum(snailInterp *snail, char *cmdName, int maximum, int actual) {
	if (actual <= maximum)
		return true;
	char *iMaximum = snailI64ToStr(maximum);
	char *iActual = snailI64ToStr(actual);

	snailBuffer *msg = snailBufferCreate(16);
	snailBufferAddString(msg, "command \"");
	snailBufferAddString(msg, cmdName);
	snailBufferAddString(msg, "\" permits maximum ");
	snailBufferAddString(msg, iMaximum);
	snailBufferAddString(msg, " arguments, got ");
	snailBufferAddString(msg, iActual);
	snailBufferAddString(msg, " instead");
	snailBufferAddChar(msg, 0);

	free(iMaximum);
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

bool snailIsBool(const char *str) {
	return snailIsTrue(str) || snailIsFalse(str);
}

bool snailIsTrue(const char *str) {
	return strcasecmp(str,"t") == 0;
}

bool snailIsFalse(const char *str) {
	return strcasecmp(str,"f") == 0;
}

ssize_t snailReplRead(snailInterp *snail, char **buffer, size_t *n) {
	if (snail->repl->readScript == NULL) {
		return getline(buffer, n, stdin);
	}
	snailStatus status = snailExec(snail, snail->repl->readScript);
	if (status == snailStatusError) {
		snailBuffer *msg = snailBufferCreate(16);
		snailBufferAddString(msg,"crash in REPL read script {");
		snailBufferAddString(msg,snail->repl->readScript);
		snailBufferAddString(msg,"} due to error {");
		snailBufferAddString(msg,snail->result);
		snailBufferAddString(msg,"}");
		snailBufferAddChar(msg,0);
		snailPanic(msg->bytes);
	}
	if (status != snailStatusOk) {
		snailBuffer *msg = snailBufferCreate(16);
		snailBufferAddString(msg,"crash in REPL read script {");
		snailBufferAddString(msg,snail->repl->readScript);
		snailBufferAddString(msg,"} due to unexpected status");
		snailBufferAddChar(msg,0);
		snailPanic(msg->bytes);
	}
	if (snail->result[0] == 0) {
		// EOF condition
		snailSetResult(snail,"");
		return -1;
	}
	char *text = snailTokenUnquote(snail->result);
	if (text == NULL) {
		snailBuffer *msg = snailBufferCreate(16);
		snailBufferAddString(msg,"crash in REPL read script {");
		snailBufferAddString(msg,snail->repl->readScript);
		snailBufferAddString(msg,"} due to returnin non-string result {");
		snailBufferAddString(msg,snail->result);
		snailBufferAddString(msg,"}");
		snailBufferAddChar(msg,0);
		snailPanic(msg->bytes);
	}
	snailSetResult(snail,"");
	snailBuffer *r = snailBufferCreate(strlen(text)+2);
	snailBufferAddString(r,text);
	free(text);
	snailBufferAddString(r,"\n");
	snailBufferAddChar(r,0);
	*buffer = snailDupString(r->bytes);
	snailBufferDestroy(r);
	*n = strlen(*buffer) + 1;
	return (*n) - 1;
}

char * snailReplContext(snailInterp *snail, char *context, char *buffer) {
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
		ssize_t rc = snailReplRead(snail, &buffer, &n);
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
			snailParseDestroy(t);
			return NULL;
		}
	}
	snailParseDestroy(t);
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
	char r = 0;
	switch (script[0]) {
		case '"':
			r = 'Q';
			break;
		case '{':
			r = 'L';
			break;
		case '[':
			r = 'X';
			break;
		case '%':
			r = script[1] == '{' ? 'D' : 'U';
			break;
		case '$':
			r = 'V';
			break;
		case '\n':
			r = 'N';
			break;
		default:
			r = 'U';
	}
	free(script);
	return r;
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
					snailBufferAddChar(b, 0);
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

char *snailMakeQuoted(const char *str) {
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

snailStatus snailExecListUp(snailInterp *snail, int64_t level, char *code) {
	if (level == 0)
		return snailExecList(snail, code);
	if (level < 0 || level >= snail->frames->length) {
		snailSetResult(snail, "illegal level number");
		return snailStatusError;
	}
	snailArray *savedFrames = snailArrayCreate(level);
	for (int i = 0; i < level; i++) {
		snailArrayAdd(savedFrames,snailArrayShift(snail->frames));
	}
	snailStatus ss = snailExecList(snail,code);
	for (int i = 0; i < level; i++) {
		snailArrayAdd(snail->frames, savedFrames->elems[i]);
	}
	snailArrayDestroy(savedFrames,NULL);
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
	snailArrayAdd(repl->history,snailMakeQuoted(buffer));
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
	array->elems[array->length-1] = NULL;
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
	snailQuickSort(list->elems, list->length,  sizeof(void *), cmp, (void*)snailArraySortCmp);
}

int snailArraySortCmp(void *thunk, const void **a, const void **b) {
	snailArrayComparator *cmp = thunk;
	return cmp(*a, *b);
}

int64_t snailTimeNow(void) {
	struct timeval tp;
	gettimeofday(&tp, NULL);
	int64_t msec = (((int64_t)tp.tv_sec) * 1000) + (((int64_t)tp.tv_usec) / 1000);
	return msec;
}

snailArray *snailUnquoteDict(char *str) {
	int len = strlen(str);
	if (len < 3) // %{} is shortest possible valid dict
		return NULL;
	if (str[0] != '%' || str[1] != '{' || str[len-1] != '}') {
		return NULL;
	}
	char *body = snailDupString(str+2);
	body[len-3] = 0;
	snailArray *array = snailParseList(body);
	free(body);
	return array;
}

snailHashTable *snailParseDict(char *str) {
	snailArray *list = snailUnquoteDict(str);
	if (list == NULL)
		return NULL;
	snailHashTable *ht = snailHashTableCreate(16);
	for (int i = 0; i < list->length; i+=2) {
		char *key = list->elems[i];
		char *value = i + 1 < list->length ? list->elems[i+1] : "";
		free(snailHashTablePut(ht,key,snailDupString(value)));
	}
	snailArrayDestroy(list,free);
	return ht;
}

snailArray *snailHashTableKeys(snailHashTable *ht) {
	snailArray *keys = snailArrayCreate(16);
	if (ht != NULL) {
		char *key = snailHashTableFirst(ht);
		while (key != NULL) {
			snailArrayAdd(keys,snailDupString(key));
			key = snailHashTableNext(ht, key);
		}
	}
	return keys;
}

char *snailQuoteDict(snailHashTable *ht) {
	if (ht == NULL)
		return NULL;
	snailBuffer *buf = snailBufferCreate(64);
	snailBufferAddChar(buf,'%');
	snailBufferAddChar(buf,'{');
	char *key = snailHashTableFirst(ht);
	while (key != NULL) {
		char *value = snailHashTableGet(ht, key);
		if (buf->length > 2)
			snailBufferAddChar(buf, ' ');
		snailBufferAddString(buf, key[0]!=0?key:"\"\"");
		snailBufferAddChar(buf, ' ');
		snailBufferAddString(buf, value[0]!=0?value:"\"\"");
		key = snailHashTableNext(ht, key);
	}
	snailBufferAddChar(buf,'}');
	snailBufferAddChar(buf, 0);
	char *result = snailDupString(buf->bytes);
	snailBufferDestroy(buf);
	return result;
}

char *snailStringReplace(char *target, char *find, char *sub) {
	snailBuffer *out = snailBufferCreate(strlen(target)+1);
	if (find == NULL || find[0] == 0) {
		snailBufferAddString(out, target);
		snailBufferAddChar(out, 0);
		char *p = snailDupString(out->bytes);
		snailBufferDestroy(out);
		return p;
	}
	if (sub == NULL)
		sub = "";
	int32_t off = 0;
	while (true) {
		char *found = strstr(target + off, find);
		if (found == NULL) {
			snailBufferAddString(out, target + off);
			snailBufferAddChar(out, 0);
			char *p = snailDupString(out->bytes);
			snailBufferDestroy(out);
			return p;
		}
		for (int32_t i = off; i < (found - (target)); i++) {
			snailBufferAddChar(out, target[i]);
		}
		snailBufferAddString(out, sub);
		off += (found - (target + off)) + strlen(find);

		snailBufferAddChar(out, 0);
		out->length--;
	}
}

const char *snailStringFindRev(const char *haystack, const char *needle) {
	size_t hlen = strlen(haystack);
	size_t nlen = strlen(needle);
	for (ssize_t i = hlen-1; i >= 0; i--) {
		for (size_t j = 0; j < nlen && i+j < hlen; j++) {
			if (haystack[i+j] != needle[j])
				goto nextOuter;
		}
		return haystack + i;
nextOuter:
		;
	}
	return NULL;
}

char *snailGetCmdUp(snailInterp *snail, int level) {
	if (level < 0)
		return NULL;
	int off = snail->frames->length-1 - level;
	if (off < 0)
		return NULL;
	snailCallFrame *frame = snail->frames->elems[off];
	return frame->cmdName;
}

char *snailWriteFile(const char *filename, char *text) {
	size_t len = strlen(text);
	return snailWriteFileBinary(filename,text,len,false);
}

char *snailWriteFileBinary(const char *filename, char *text, size_t len, bool binaryMode) {
	FILE *f;
	if(!(f = fopen(filename, binaryMode ? "wb" : "w"))) {
		int e = errno;
		snailBuffer *msg = snailBufferCreate(16);
		snailBufferAddString(msg,"snailWriteFile: I/O error ");
		char *s = snailI64ToStr(e);
		snailBufferAddString(msg,s);
		free(s);
		snailBufferAddString(msg," attempting to open ");
		snailBufferAddString(msg,filename);
		snailBufferAddChar(msg,0);
		char *r = snailDupString(msg->bytes);
		snailBufferDestroy(msg);
		return r;
	}
	if (fwrite(text, 1, len, f) != len) {
		int e = errno;
		fclose(f);
		snailBuffer *msg = snailBufferCreate(16);
		snailBufferAddString(msg,"snailWriteFile: I/O error ");
		char *s = snailI64ToStr(e);
		snailBufferAddString(msg,s);
		free(s);
		snailBufferAddString(msg," attempting to write ");
		char *s_len = snailI64ToStr(len);
		snailBufferAddString(msg,s_len);
		free(s_len);
		snailBufferAddString(msg," bytes to file ");
		snailBufferAddString(msg,filename);
		snailBufferAddChar(msg,0);
		char *r = snailDupString(msg->bytes);
		snailBufferDestroy(msg);
		return r;
	}
	if (fclose(f)!=0) {
		int e = errno;
		snailBuffer *msg = snailBufferCreate(16);
		snailBufferAddString(msg,"snailWriteFile: I/O error ");
		char *s = snailI64ToStr(e);
		snailBufferAddString(msg,s);
		free(s);
		snailBufferAddString(msg," attempting to close handle to file ");
		snailBufferAddString(msg,filename);
		snailBufferAddChar(msg,0);
		char *r = snailDupString(msg->bytes);
		snailBufferDestroy(msg);
		return r;
	}
	return NULL;
}

void snailRandomBytes(void *buf, size_t count) {
#ifdef __DJGPP__
	char *cbuf = buf;
	for (ssize_t i = 0; i < count; i++) {
		cbuf[i] = rand();
	}
#else
	FILE *fh = fopen("/dev/urandom","r");
	if (fh == NULL)
		snailPanic("RNG failure [opening random device]");
	if (fread(buf, 1, count, fh) != count)
		snailPanic("RNG failure [reading from random device]");
	if (fclose(fh)!=0)
		snailPanic("RNG failure [closing random device]");
#endif
}

void snailBufferAddI64(snailBuffer *buf, int64_t n) {
	char *s = snailI64ToStr(n);
	snailBufferAddString(buf,s);
	free(s);
}

// Based on qsort routine in Paul Edwards' PDPCLIB. Which in turn is
// based on that in libnix. Both are public domain.
void snailQuickSort(void *base, size_t nElems, size_t width, void *thunk, snailQuickSortCmp *cmp) {
	char *base2 = (char *)base;
	size_t i,a,b,c;
	while (nElems > 1) {
		a = 0;
		b = nElems-1;
		c = (a+b)/2; /* Middle element */
		for (;;) {
			while ((*cmp)(thunk,&base2[width*c],&base2[width*a]) > 0) {
				a++; /* Look for one >= middle */
			}
			while ((*cmp)(thunk,&base2[width*c],&base2[width*b]) < 0) {
				b--; /* Look for one <= middle */
			}
			if (a >= b) {
				break; /* We found no pair */
			}
			for (i=0; i<width; i++) { /* swap them */
				char tmp=base2[width*a+i];
				base2[width*a+i]=base2[width*b+i];
				base2[width*b+i]=tmp;
			}
			if (c == a) { /* Keep track of middle element */
				c = b;
			} else if (c == b) {
				c = a;
			}
			a++; /* These two are already sorted */
			b--;
		} /* a points to first element of right interval now
		     (b to last of left) */
		b++;
		if (b < nElems-b) { /* do recursion on smaller interval and
				    iteration on larger one */
			snailQuickSort(base2,b,width,thunk,cmp);
			base2=&base2[width*b];
			nElems=nElems-b;
		} else {
			snailQuickSort(&base2[width*b],nElems-b,width,thunk,cmp);
			nElems=b;
		}
	}
	return;
}

uint32_t snailRandomU32(void) {
	uint32_t result;
	snailRandomBytes(&result, sizeof(uint32_t));
	return result;
}

uint32_t snailRandomU32Uniform(uint32_t bound) {
	if (bound < 2)
		return 0;
	uint32_t bits = 32 - __builtin_clz(bound);
	uint32_t mask = snail2PowMinus1U32(bits);
	for (;;) {
		uint32_t result = snailRandomU32();
		result &= mask;
		if (result < bound)
			return result;
	}
}

void snailArrayShuffle(snailArray *array) {
	for (uint32_t i = 0; i < array->length; i++) {
		uint32_t j = i + snailRandomU32Uniform(array->length - i);
		void *temp = array->elems[i];
		array->elems[i] = array->elems[j];
		array->elems[j] = temp;
	}
}

int snailNaturalCmp(const char *a, const char *b) {
	if (!snailIsInt(a) || !snailIsInt(b))
		return strcmp(a,b);
	int32_t na = strtol(a,NULL,10);
	int32_t nb = strtol(b,NULL,10);
	char sa[12];
	char sb[12];
	sprintf(sa, "%" PRId32,na);
	sprintf(sb, "%" PRId32,nb);
	if (strcmp(a,sa)!=0 || strcmp(b,sb)!=0)
		return strcmp(a,b);
	return na - nb;
}

bool snailCopyFile(FILE *from, FILE *to) {
	char buffer[1024];
	for (;;) {
		size_t n = fread(buffer, sizeof(char), sizeof(buffer), from);
		if (n == 0)
			return feof(from);
		if (fwrite(buffer, sizeof(char), n, to) != n)
			return false;
	}
}

char *snailGetPlatformType(void) {
#if defined(__APPLE__)
	return "macos";
#elif defined(__DJGPP__)
	return "djgpp";
#elif defined(__linux__)
	return "linux";
#else
	return "unknown";
#endif
}

void snailExit(snailInterp *snail, int exitCode) {
	if (snail->noExit) {
		fprintf(stdout,"NOTICE: Exit requested with code %d; refused since noExit flag is set\n", exitCode);
	}
	else {
		snailDestroy(snail);
		exit(exitCode);
	}
}

char *snailQuoteArgv(char **argv) {
	int count = 0;
	while (argv[count] != NULL)
		count++;
	snailArray *args = snailArrayCreate(count);
	for (int i = 0; argv[i] != NULL; i++) {
		char *quoted = snailMakeQuoted(argv[i]);
		snailArrayAdd(args,quoted);
	}
	char *result = snailQuoteList(args);
	snailArrayDestroy(args,free);
	return result;
}

char *snailStripShebang(char *script) {
	if (script[0] != '#' || script[1] != '!')
		return script;
	while (script[0] != '\n' && script[0] != 0)
		script++;
	return script;
}

void snailDictBufSetInt(snailBuffer *buf, const char *name, int64_t n) {
	char *v = snailI64ToStr(n);
	snailDictBufSetToken(buf,name,v);
	free(v);
}

void snailDictBufSetToken(snailBuffer *buf, const char *name, const char *value) {
	snailBufferAddChar(buf,' ');
	snailBufferAddString(buf,name);
	snailBufferAddChar(buf,' ');
	snailBufferAddString(buf,value);
}

void snailDictBufSetQuoted(snailBuffer *buf, const char *name, const char *value) {
	char *q = snailMakeQuoted(value);
	snailDictBufSetToken(buf,name,q);
	free(q);
}

char *snailTimeToDict(struct tm *time, int32_t millis) {
	snailBuffer *buf = snailBufferCreate(64);
	snailBufferAddString(buf,"%{");
	snailDictBufSetInt(buf,"millis",millis);
	snailDictBufSetInt(buf,"sec",time->tm_sec);
	snailDictBufSetInt(buf,"min",time->tm_min);
	snailDictBufSetInt(buf,"hour",time->tm_hour);
	snailDictBufSetInt(buf,"dom",time->tm_mday);
	snailDictBufSetInt(buf,"dow",time->tm_wday==0 ? 7 : time->tm_wday);
	snailDictBufSetInt(buf,"doy",time->tm_yday+1);
	snailDictBufSetInt(buf,"month",time->tm_mon+1);
	snailDictBufSetInt(buf,"year",time->tm_year+1900);
	snailDictBufSetToken(buf,"dst",time->tm_isdst ? "t" : "f");
	if (time->tm_zone != NULL)
		snailDictBufSetQuoted(buf,"zone",time->tm_zone);
	snailDictBufSetInt(buf,"utcOffsetMinutes",((int32_t)time->tm_gmtoff) / 60);
	snailBufferAddChar(buf,'}');
	snailBufferAddChar(buf,0);
	char *r = snailDupString(buf->bytes);
	snailBufferDestroy(buf);
	return r;
}

bool snailTimeFieldGet(snailInterp *snail, char *cmdName, snailHashTable *dict, char *name, int64_t *r, bool mandatory, int64_t min, int64_t max) {
	char *value = snailHashTableGet(dict,name);
	if (value == NULL && !mandatory) {
		*r = 0;
	} else {
		if (value == NULL || !snailIsInt(value)) {
			snailBuffer *msg = snailBufferCreate(32);
			snailBufferAddString(msg,cmdName);
			snailBufferAddString(msg,": mandatory field '");
			snailBufferAddString(msg,name);
			snailBufferAddString(msg,"' is missing or has invalid value");
			snailBufferAddChar(msg,0);
			snailSetResult(snail,msg->bytes);
			snailBufferDestroy(msg);
			return false;
		}
		*r = strtoll(value,NULL,10);
	}
	if (*r < min || *r > max) {
		snailBuffer *msg = snailBufferCreate(32);
		snailBufferAddString(msg,cmdName);
		snailBufferAddString(msg,": field '");
		snailBufferAddString(msg,name);
		snailBufferAddString(msg,"' value ");
		snailBufferAddI64(msg,*r);
		snailBufferAddString(msg," is out of valid range ");
		snailBufferAddI64(msg,min);
		snailBufferAddString(msg,"...");
		snailBufferAddI64(msg,max);
		snailBufferAddChar(msg,0);
		snailSetResult(snail,msg->bytes);
		snailBufferDestroy(msg);
		return false;
	}
	return true;
}

char *snailHexDecode(const char *hex) {
	size_t len = strlen(hex);
	if (len == 0 || (len % 2) != 0)
		return NULL;
	uint8_t *buf = snailMalloc(len / 2);
	for (size_t i = 0; i < (len/2); i++) {
		char hexHi = hex[(2*i)+0];
		char hexLo = hex[(2*i)+1];
		if (!snailIsHexDigit(hexHi) || !snailIsHexDigit(hexLo)) {
			free(buf);
			return NULL;
		}
		uint8_t nHi = snailHexDecodeNibble(hexHi);
		uint8_t nLo = snailHexDecodeNibble(hexLo);
		uint8_t n = (nHi << 4) | nLo;
		buf[i] = n;
	}
	return (char*)buf;
}

bool snailIsHexDigit(char ch) {
	return (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f');
}

uint8_t snailHexDecodeNibble(char ch) {
	if (ch >= 'A' && ch <= 'F')
		return (ch - 'A') + 0xA;
	if (ch >= 'a' && ch <= 'f')
		return (ch - 'a') + 0xA;
	return ch - '0';
}

char *snailHexEncode(const char *buf, size_t len) {
	char *hex = snailMalloc((2*len)+1);
	size_t j = 0;
	for (size_t i = 0; i < len; i++) {
		uint8_t byte = buf[i];
		uint8_t nibLo = (byte & 0x0f);
		uint8_t nibHi = ((byte >> 4) & 0x0f);
		char hexLo = snailHexEncodeNibble(nibLo);
		char hexHi = snailHexEncodeNibble(nibHi);
		hex[j++] = hexHi;
		hex[j++] = hexLo;
	}
	hex[j++] = 0;
	return hex;
}

char snailHexEncodeNibble(uint8_t nibble) {
	return nibble < 10 ? '0' + nibble : 'A' + (nibble - 10);
}

snailStatus snailDoChannelRead(snailInterp *snail, char *channelName, int64_t bytes, bool hexEncode) {
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
	if (hexEncode) {
		char *hex = snailHexEncode(buf, read);
		snailBufferAddString(out, hex);
		free(hex);
	} else {
		for (int i = 0; i < read; i++)
			snailBufferAddChar(out, buf[i]);
	}
	snailBufferAddChar(out, 0);
	free(buf);
	char *quoted = snailMakeQuoted(out->bytes);
	snailBufferDestroy(out);
	snailSetResult(snail,quoted);
	free(quoted);
	return snailStatusOk;
}
