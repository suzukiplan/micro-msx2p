CXXFLAGS=-g -O2 \
	-fno-omit-frame-pointer \
	-fsanitize=address \
	-fno-sanitize-recover=all \
	-DZ80_DISABLE_DEBUG \
	-DZ80_DISABLE_BREAKPOINT \
	-DZ80_DISABLE_NESTCHECK \
	-DZ80_CALLBACK_WITHOUT_CHECK \
	-DZ80_CALLBACK_PER_INSTRUCTION \
	-DZ80_UNSUPPORT_16BIT_PORT \
	-std=c++11 \
	-I../../src1 \
	-I./benchmark/include \
	-L ./benchmark/build/src

all: ./benchmark/build/src
	g++ $(CXXFLAGS) test.cpp -o test -lbenchmark -lpthread
	@echo ===== First Execution =====
	./test >result1.txt 2>&1
	cat result1.txt
	@echo ===== Second Execution =====
	./test >result2.txt 2>&1
	cat result2.txt
	@echo ===== Third Execution =====
	./test >result3.txt 2>&1
	cat result3.txt

./benchmark/build/src:
	cd benchmark && cmake -DBENCHMARK_DOWNLOAD_DEPENDENCIES=on -DCMAKE_BUILD_TYPE=Release -S . -B "build"
	cd benchmark && cmake --build "build" --config Release
