all: emu2413.o lz4.o pngwriter.o
	clang++ -std=c++11 -Os -DNO_FREETYPE -o m2penc src/m2penc.cpp emu2413.o lz4.o pngwriter.o -lpng

emu2413.o: ../../src/core/emu2413.c
	clang -Os -c ../../src/core/emu2413.c

lz4.o: ../../src/lz4/lz4.c
	clang -Os -c ../../src/lz4/lz4.c

pngwriter.o: src/pngwriter/pngwriter.cc src/pngwriter/pngwriter.h
	clang++ -Os -DNO_FREETYPE -c src/pngwriter/pngwriter.cc
