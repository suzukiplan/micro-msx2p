all:
	git submodule update --init --recursive
	cd test/performance && make
	cd test/performance1 && make
	cd msx2-dotnet && make clean all
	cd test/google-benchmark && make

format:
	make execute-format FILENAME=./src/ay8910.hpp
	make execute-format FILENAME=./src/msx2.hpp
	make execute-format FILENAME=./src/msx2clock.hpp
	make execute-format FILENAME=./src/msx2def.h
	make execute-format FILENAME=./src/msx2kanji.hpp
	make execute-format FILENAME=./src/msx2mmu.hpp
	make execute-format FILENAME=./src/scc.hpp
	make execute-format FILENAME=./src/tc8566af.hpp
	make execute-format FILENAME=./src/v9958.hpp
	make execute-format FILENAME=./src1/ay8910.hpp
	make execute-format FILENAME=./src1/msx1.hpp
	make execute-format FILENAME=./src1/msx1def.h
	make execute-format FILENAME=./src1/msx1mmu.hpp
	make execute-format FILENAME=./src1/tms9918a.hpp
	make execute-format FILENAME=./src1/z80.hpp
	make execute-format FILENAME=./msx2-sdl2/src/app.cpp

execute-format:
	clang-format -style=file < ${FILENAME} > ${FILENAME}.bak
	cat ${FILENAME}.bak > ${FILENAME}
	rm ${FILENAME}.bak
