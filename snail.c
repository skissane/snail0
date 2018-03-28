/* Snail programming language */

/***---INCLUDES---***/
#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

extern char **environ;

/***---DJGPP COMPATIBILITY---***/
#ifdef __DJGPP__
#include <sys/movedata.h>
#include <dpmi.h>
#include <go32.h>
#include <sys/farptr.h>
#include "snailDJGPP.h"
#endif

/***---Constants---***/
const int VARIADIC = -1;

/***---INIT SCRIPT---***/
static const char snailInitScript[] = {
#include "snailInit.h"
};

/***---TYPES---***/
#include "snailTypes.h"

/***---PROTOTYPES---***/
#include "snailProtos.h"
#ifdef __DJGPP__
#	include "snailProtosDOS.h"
#endif

/***---NATIVES: PROTOTYPES---***/
#define NATIVE(_name, _arity) \
	snailStatus snailNative_##_name(snailInterp *snail, char *cmdName, int argCount, char **args)

#include "snailNativeProtos.h"
#ifdef __DJGPP__
#	include "snailNativeProtosDOS.h"
#endif

/***---NATIVES: REGISTER---***/
#undef NATIVE
#define NATIVE(_name, _arity) \
	snailRegisterNative(snail, #_name, _arity, snailNative_##_name);

void snailRegisterNatives(snailInterp *snail) {
#	include "snailNativeProtos.h"
#	ifdef __DJGPP__
#		include "snailNativeProtosDOS.h"
#	endif
}

/***---NATIVES: IMPLEMENT---***/
#undef NATIVE
#define NATIVE(_name, _arity) \
	snailStatus snailNative_##_name(snailInterp *snail, char *cmdName, int argCount, char **args)
#include "snailNativeImpl.h"
#ifdef __DJGPP__
#	include "snailNativeImplDOS.h"
#endif

/***---FUNCTIONS: PLATFORM SPECIFIC---***/
#ifdef __DJGPP__
#	include "snailFuncsDOS.h"
#endif

/***---FUNCTIONS: PLATFORM GENERIC---***/
#include "snailFuncsChannel.h"
#include "snailFuncs.h"

/***---MAIN FUNCTION---***/
int main(int argc, char *argv[]) {
	tzset();
	snailInterp *snail = snailCreate();
	snailHashTablePut(snail->globals,"sys.args",snailQuoteArgv(argv));

	int rc = 0;
	if(argc < 2)
		snailRepl(snail);
	else if (strcmp(argv[1],"-") == 0) {
		for (int i = 2; argv[i] != NULL; i++) {
			if (!snailRunScript(snail,argv[i])) {
				rc = 1;
				break;
			}
		}
	}
	else {
		rc = snailRunFile(snail, argv[1]);
	}
	snailDestroy(snail);
	return rc;
}
