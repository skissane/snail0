/***---TYPES---***/
typedef enum snailParseResult snailParseResult;
typedef enum snailParseState snailParseState;
typedef enum snailStatus snailStatus;
typedef struct snailArray snailArray;
typedef struct snailBuffer snailBuffer;
typedef struct snailCallFrame snailCallFrame;
typedef struct snailCommand snailCommand;
typedef struct snailHashCell snailHashCell;
typedef struct snailHashTable snailHashTable;
typedef struct snailInterp snailInterp;
typedef struct snailParseTool snailParseTool;
typedef struct snailReplState snailReplState;

typedef snailStatus snailNative (snailInterp *snail, char *name, int argCount, char **args);

typedef struct snailBuffer {
	int length;
	int allocated;
	char *bytes;
} snailBuffer;

typedef enum snailStatus {
	snailStatusOk,
	snailStatusError,
	snailStatusReturn,
	snailStatusContinue,
	snailStatusBreak
} snailStatus;

typedef struct snailHashCell {
	char *key;
	void *value;
	struct snailHashCell *next;
} snailHashCell;

typedef struct snailHashTable {
	snailHashCell **buckets;
	int numberOfBuckets;
	int numberOfCells;
} snailHashTable;

typedef struct snailCallFrame {
	char *cmdName;
	snailHashTable *vars;
} snailCallFrame;

typedef struct snailCommand {
	char *name;
	int arity;
	char *args;
	char *script;
	snailNative *native;
} snailCommand;

typedef struct snailInterp {
	char *result;
	snailArray *frames;
	snailHashTable *globals;
	snailHashTable *commands;
	snailReplState *repl;
	int64_t startupTime;
} snailInterp;

typedef struct snailReplState {
	int historyMax;
	snailArray *history;
	char *prompt;
} snailReplState;

typedef struct snailParseTool {
	const char *script;
	int length;
	int pos;
	snailBuffer *word;
	snailBuffer *context;
	snailBuffer *error;
} snailParseTool;

typedef void snailDestructor (void *value);

typedef enum snailParseResult {
	snailParseResultEnd,
	snailParseResultToken,
	snailParseResultIncomplete,
	snailParseResultError
} snailParseResult;

typedef struct snailArray {
	int allocated;
	int length;
	void **elems;
} snailArray;

typedef int snailArrayComparator (const void *, const void *);
