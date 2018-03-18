# Snail

Snail is a small programming language.

Snail is vaguely/loosely inspired by Tcl 7.x, but not compatible with it.

## Why is it called "Snail"?

Because it is slow. The current implementation is extremely inefficient.

## Supported Platforms

Snail has been tested on the following platforms:
* macOS
* Linux
* DOS (using DJGPP)

It most likely runs on other versions/distributions of the above platforms.

Under macOS, `make allports` will build and test under macOS and Alpine Linux
(using Linux). It will also build (but not test) the DOS port, for which you
need `i586-pc-msdosdjgpp-gcc` in your PATH. It also builds an installation
floppy image for which you need `mtools` insteadd.
