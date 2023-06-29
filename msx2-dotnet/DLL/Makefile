all: libMSX2.so

libMSX2.so: msx2dll.cpp msx2dll.h emu2413.o lz4.o
	clang++ -std=c++11 -Os -I../../src/core -I../../src/lz4 -shared -undefined dynamic_lookup -o libMSX2.so msx2dll.cpp

emu2413.o: ../../src/core/emu2413.c
	clang -Os -c ../../src/core/emu2413.c

lz4.o: ../../src/lz4/lz4.c
	clang -Os -c ../../src/lz4/lz4.c
