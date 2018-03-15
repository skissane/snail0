/* Snail programming language */

/***---INCLUDES---***/
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>

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

/***---NATIVES: PROTOTYPES---***/
#define NATIVE(_name, _arity) \
	snailStatus snailNative_##_name(snailInterp *snail, char *cmdName, int argCount, char **args)

#include "snailNativeProtos.h"

/***---NATIVES: REGISTER---***/
#undef NATIVE
#define NATIVE(_name, _arity) \
	snailRegisterNative(snail, #_name, _arity, snailNative_##_name);

void snailRegisterNatives(snailInterp *snail) {
#	include "snailNativeProtos.h"
}

/***---NATIVES: IMPLEMENT---***/
#undef NATIVE
#define NATIVE(_name, _arity) \
	snailStatus snailNative_##_name(snailInterp *snail, char *cmdName, int argCount, char **args)
#include "snailNativeImpl.h"

/***---FUNCTIONS---***/
#include "snailFuncsChannel.h"
#include "snailFuncs.h"

/***---MAIN FUNCTION---***/
int main(int argc, char *argv[]) {
	snailInterp *snail = snailCreate();
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
		for (int i = 1; argv[i] != NULL; i++) {
			rc = snailRunFile(snail, argv[i]);
			if (rc != 0)
				break;
		}
	}
	snailDestroy(snail);
	return rc;
}
