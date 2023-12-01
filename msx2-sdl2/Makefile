CFLAGS = -O2 -I./src -I../src -I/usr/include/SDL2 -I/usr/local/include/SDL2 -I/opt/X11/include -D_THREAD_SAFE
CPPFLAGS = -std=c++11 $(CFLAGS)
CC = clang $(CFLAGS)
CPP = clang++ $(CPPFLAGS)
OBJECTS = lz4.o emu2413.o BufferQueue.o app.o
HEADER_FILES =\
	../src/ay8910.hpp\
	../src/emu2413.h\
	../src/lz4.h\
	../src/msx2.hpp\
	../src/msx2clock.hpp\
	../src/msx2def.h\
	../src/msx2kanji.hpp\
	../src/msx2mmu.hpp\
	../src/scc.hpp\
	../src/tc8566af.hpp\
	../src/v9958.hpp\
	../src/z80.hpp\
	./src/BufferQueue.h

all: app

clean:
	rm -f $(OBJECTS) app

app: $(OBJECTS)
	$(CPP) -o app $(OBJECTS) -L/usr/local/lib -lSDL2

lz4.o: ../src/lz4.c $(HEADER_FILES) ./Makefile
	$(CC) -c ../src/lz4.c

emu2413.o: ../src/emu2413.c $(HEADER_FILES) ./Makefile
	$(CC) -c ../src/emu2413.c

BufferQueue.o: src/BufferQueue.cpp $(HEADER_FILES) ./Makefile
	$(CPP) -c src/BufferQueue.cpp

app.o: src/app.cpp $(HEADER_FILES) ./Makefile
	$(CPP) -c src/app.cpp
