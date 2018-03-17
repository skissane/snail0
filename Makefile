CFLAGS += -Wall -Werror -std=gnu11 -g

snail: snail.c snailNativeImpl.h snailNativeProtos.h snailProtos.h \
       snailTypes.h snailFuncs.h snailNativeNames.h snailInit.h \
       snailFuncsChannel.h
	$(CC) snail.c -o $@ $(CFLAGS)

bin2c: bin2c.c
	$(CC) bin2c.c -o $@ $(CFLAGS)

snailInit.h: bin2c __init__.snail
	cat __init__.snail | ./bin2c > snailInit.h

clean:
	rm -f snail *.o snailNativeProtos.h bin2c snailInit.h *.gcda *.gcno *.cov.info *.exe

snailNativeProtos.h: snailNativeImpl.h makeNativeProtos.sh
	sh ./makeNativeProtos.sh

djgpp: clean bin2c
	CC=i586-pc-msdosdjgpp-gcc make

.PHONY: clean djgpp
