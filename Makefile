CFLAGS = -Wall -Werror -std=gnu11 -g

snail: snail.c snailNativeImpl.h snailNativeProtos.h snailProtos.h \
       snailTypes.h snailFuncs.h snailNativeNames.h
	$(CC) snail.c -o $@ $(CFLAGS)

clean:
	rm -f snail *.o snailNativeProtos.h

snailNativeProtos.h: snailNativeImpl.h makeNativeProtos.sh
	sh ./makeNativeProtos.sh

.PHONY: clean
