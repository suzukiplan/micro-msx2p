all:
	clang -Os -c ../../src/emu2413.c
	clang++ -Os -std=c++11 -I../../src -o test test.cpp emu2413.o
	./test
