all: emu2413.o lz4.o
	clang++ -Os -std=c++11 -I../../src -o runbas runbas.cpp emu2413.o lz4.o

emu2413.o: ../../src/emu2413.c
	clang -Os -c ../../src/emu2413.c

lz4.o: ../../src/lz4.c
	clang -Os -c ../../src/lz4.c

png:
	make png-`uname`

png-Darwin:
	sips -s format png result.bmp --out result.png

png-Linux:
	mogrify -format png result.bmp
