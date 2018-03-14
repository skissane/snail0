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
#include "snailFuncs.h"

/***---MAIN FUNCTION---***/
int main(int argc, char *argv[]) {
	snailInterp *snail = snailCreate();
	int rc = 0;
	if(argc < 2)
		snailRepl(snail);
	else 
		rc = snailRunFile(snail, argv[1]);
	snailDestroy(snail);
	return rc;
}
