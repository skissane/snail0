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
typedef struct snailChannelDriver snailChannelDriver;
typedef struct snailChannel snailChannel;

typedef snailStatus snailNative (snailInterp *snail, char *name, int argCount, char **args);
typedef char * snailChannel_READ (snailChannel *channel, void *buf, size_t len, size_t *read);
typedef char * snailChannel_WRITE (snailChannel *channel, void *buf, size_t len, size_t *written);
typedef char * snailChannel_OPEN (snailChannel *channel, void *driverArg);
typedef char * snailChannel_CLOSE (snailChannel *channel);
typedef char * snailChannel_FLUSH (snailChannel *channel);
typedef char * snailChannel_GETLINE (snailChannel *channel, char **bufOut);

typedef struct snailChannelDriver {
	char *name;
	snailChannel_READ *f_READ;
	snailChannel_WRITE *f_WRITE;
	snailChannel_OPEN *f_OPEN;
	snailChannel_CLOSE *f_CLOSE;
	snailChannel_FLUSH *f_FLUSH;
	snailChannel_GETLINE *f_GETLINE;
} snailChannelDriver;

typedef struct snailChannel {
	char *name;
	snailChannelDriver *driver;
	void *driverInfo;
} snailChannel;

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
	snailHashTable *channelDrivers;
	snailHashTable *channels;
	int64_t startupTime;
	int64_t autoId;
	bool noExit;
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
typedef int snailQuickSortCmp (void *thunk, const void *a, const void *b);
