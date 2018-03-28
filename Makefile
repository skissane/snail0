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
	rm -f snail snail.exe
	CC=i586-pc-msdosdjgpp-gcc make
	rm -rf BUILD.DOS
	mkdir -p BUILD.DOS/SNAIL
	cp ports/dos/CWSDPMI.EXE BUILD.DOS/SNAIL
	cp snail.exe BUILD.DOS/SNAIL/SNAIL.EXE
	make clean snail
	./snail - snail.dos.build
	cp fern.bmp BUILD.DOS/SNAIL
	(cd BUILD.DOS && zip -r SNAIL.ZIP SNAIL)
	mformat -i ./BUILD.DOS/SNAIL.IMG -C -f 1440 ::
	mcopy -i ./BUILD.DOS/SNAIL.IMG ./BUILD.DOS/SNAIL.ZIP ::

test: snail
	./testsnail

linux:
	./ports/linux/create

allports: clean test djgpp linux

.PHONY: clean djgpp test linux allports
