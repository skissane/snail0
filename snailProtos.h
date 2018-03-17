/***---PROTOTYPES---***/
bool snailArgCountExactly(snailInterp *snail, char *cmdName, int must, int actual);
bool snailArgCountMinimum(snailInterp *snail, char *cmdName, int minimum, int actual);
bool snailIsBlank(char *str);
bool snailIsBool(char *str);
bool snailIsDigits(char *str);
bool snailIsFalse(char *str);
bool snailIsInt(char *str);
bool snailIsTrue(char *str);
bool snailParseDump(char *script);
bool snailRunScript(snailInterp *snail, char *script);
bool snailSetUpVar(snailInterp *snail, int level, char *name, char *value);
bool snailTokenIsNormalized(char *script);
bool snailTokenIsValid(char *script);
char *snailChannelClose(snailChannel *channel);
char *snailChannelDriverRegister(snailInterp *snail, snailChannelDriver *driver);
char *snailChannelMakeName(snailInterp *snail);
char *snailChannelRead(snailInterp *snail, char *channelName, void *buf, size_t len, size_t *read);
char *snailChannelRegister(snailInterp *snail, char *channelName, char *driverName, void *driverArg);
char *snailChannelWrite(snailInterp *snail, char *channelName, void *buf, size_t len, size_t *written);
char *snailChannel_OPEN_stdio (snailChannel *channel, void *driverArg);
char *snailChannel_READ_stdio (snailChannel *channel, void *buf, size_t len, size_t *read);
char *snailChannel_WRITE_stdio (snailChannel *channel, void *buf, size_t len, size_t *written);
char *snailDupString(char *str);
char *snailGetCmdUp(snailInterp *snail, int level);
char *snailGetResult(snailInterp *snail);
char *snailGetUpVar(snailInterp *snail, int level, char *name);
char *snailGetVar(snailInterp *snail, char *name);
char *snailHashTableFirst(snailHashTable *ht);
char *snailHashTableNext(snailHashTable *ht, char *key);
char *snailI32ToStrRadix(int32_t n, uint8_t base);
char *snailI64ToStr(int64_t n);
char *snailMakeQuoted(char *str);
char *snailMakeVarNotFoundError(char *name);
char *snailParseGetContext(char *str);
char *snailQuoteDict(snailHashTable *ht);
char *snailQuoteList(snailArray *list);
char *snailReadFile(const char *fileName);
char *snailReplContext(char *context, char *buffer);
char *snailStringReplace(char *target, char *find, char *sub);
char *snailTokenNormalize(char *script);
char *snailTokenQuote(char *script);
char *snailTokenUnquote(char *script);
char *snailTranslateNative(char *str);
char *snailTrimString(char *str);
char *snailU64ToStr(uint64_t n);
char *snailU64ToStr16(uint64_t n);
char *snailWriteFile(const char *filename, char *text);
char snailTokenClassify(char *script);
const char *snailStringFindRev(const char *haystack, const char *needle);
int snailArraySortCmp(void *thunk, const void **a, const void **b);
int snailRunFile(snailInterp *snail, char *fileName);
int64_t snailTimeNow(void);
noreturn void snailPanic(char *msg);
snailArray *snailArrayCreate(int initSize);
snailArray *snailHashTableKeys(snailHashTable *ht);
snailArray *snailParseList(char *str);
snailArray *snailUnquoteDict(char *str);
snailArray *snailUnquoteList(char *str);
snailBuffer *snailBufferCreate(int initSize);
snailHashTable *snailHashTableCreate(int initSize);
snailHashTable *snailParseDict(char *str);
snailInterp *snailCreate(void);
snailParseResult snailParseNext(snailParseTool *parser);
snailParseTool *snailParseCreate(const char *script);
snailReplState *snailReplStateCreate(void);
snailStatus snailExec(snailInterp *snail, const char *script);
snailStatus snailExecList(snailInterp *snail, char *code);
snailStatus snailExecListUp(snailInterp *snail, int64_t level, char *code);
snailStatus snailExecSub(snailInterp *snail, char *code);
snailStatus snailRunCommand(snailInterp *snail, char *cmdName, int argCount, char **args);
uint32_t snail2PowMinus1U32(uint32_t n);
uint32_t snailRandomU32(void);
uint32_t snailRandomU32Uniform(uint32_t bound);
void *snailArrayPop(snailArray *array);
void *snailArrayShift(snailArray *array);
void *snailHashTableDelete(snailHashTable *ht, char *key);
void *snailHashTableGet(snailHashTable *ht, char *key);
void *snailHashTablePut(snailHashTable *ht, char *key, void *value);
void *snailMalloc(size_t size);
void *snailRealloc(void *buf, size_t oldSize, size_t newSize);
void snailArgMust(bool cond);
void snailArrayAdd(snailArray *array, void *element);
void snailArrayDestroy(snailArray *array, snailDestructor *destructor);
void snailArrayEmpty(snailArray *array, snailDestructor *destructor);
void snailArrayGrow(snailArray *array, int newSize);
void snailArrayShuffle(snailArray *array);
void snailArraySort(snailArray *list, snailArrayComparator *cmp);
void snailBufferAddChar(snailBuffer *buf, char ch);
void snailBufferAddI64(snailBuffer *buf, int64_t n);
void snailBufferAddString(snailBuffer *buf, const char *str);
void snailBufferDestroy(snailBuffer *buf);
void snailBufferEmpty(snailBuffer *buf);
void snailBufferGrow(snailBuffer *buf, int newSize);
void snailBufferSet(snailBuffer *buf, char *str);
void snailChannelDriverDestroy(snailChannelDriver *channel);
void snailChannelSetup(snailInterp *snail);
void snailClearResult(snailInterp *snail);
void snailDestroy(snailInterp *snail);
void snailHashCellDestroy(snailHashCell *cell, snailDestructor *destructor);
void snailHashTableDestroy(snailHashTable *ht, snailDestructor *destructor);
void snailParseDestroy(snailParseTool *parser);
void snailPrintResult(snailInterp *snail, snailStatus status);
void snailQuickSort(void *base, size_t nElems, size_t width, void *thunk, snailQuickSortCmp *cmp);
void snailRandomBytes(void *buf, size_t count);
void snailRegisterNative(snailInterp *snail, char *name, int arity, snailNative *impl);
void snailRegisterNatives(snailInterp *snail);
void snailRepl(snailInterp *snail);
void snailReplHistoryAdd(snailReplState *repl, char *buffer);
void snailReplStateDestroy(snailReplState *repl);
void snailSetResult(snailInterp *snail, char *result);
void snailSetResultBool(snailInterp *snail, bool result);
void snailSetResultInt(snailInterp *snail, int64_t result);
void snailSetVar(snailInterp *snail, char *name, char *value);
