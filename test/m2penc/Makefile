all: emu2413.o lz4.o
	clang++ -std=c++11 -Os -o m2penc src/m2penc.cpp emu2413.o lz4.o

emu2413.o: ../../msx2-osx/core/emu2413.c
	clang -Os -c ../../msx2-osx/core/emu2413.c

lz4.o: ../../msx2-osx/lz4/lz4.c
	clang -Os -c ../../msx2-osx/lz4/lz4.c
