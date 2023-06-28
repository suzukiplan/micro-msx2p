all:
	clang -Os -c ../../src/core/emu2413.c
	clang++ -Os -std=c++11 -I../../src/lz4 -o test test.cpp emu2413.o
	./test
