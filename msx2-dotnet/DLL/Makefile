all:
	make all-`uname`

all-Darwin: libMSX2.dylib

all-Linux: libMSX2.so

clean:
	rm -f libMSX2.* emu2413.o lz4.o

libMSX2.dylib: msx2dll.cpp msx2dll.h emu2413.o lz4.o
	clang++ -std=c++11 -Os -fPIC -I../../src -shared -o libMSX2.dylib msx2dll.cpp emu2413.o lz4.o

libMSX2.so: msx2dll.cpp msx2dll.h emu2413.o lz4.o
	clang++ -std=c++11 -Os -fPIC -I../../src -shared -o libMSX2.so msx2dll.cpp emu2413.o lz4.o

emu2413.o: ../../src/emu2413.c
	clang -Os -fPIC -c ../../src/emu2413.c

lz4.o: ../../src/lz4.c
	clang -Os -fPIC -c ../../src/lz4.c
