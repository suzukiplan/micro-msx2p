all:
	cd test/performance && make

format:
	make execute-format FILENAME=ay8910.hpp
	make execute-format FILENAME=msx2.hpp
	make execute-format FILENAME=msx2clock.hpp
	make execute-format FILENAME=msx2def.h
	make execute-format FILENAME=msx2kanji.hpp
	make execute-format FILENAME=msx2mmu.hpp
	make execute-format FILENAME=scc.hpp
	make execute-format FILENAME=tc8566af.hpp
	make execute-format FILENAME=v9958.hpp
	cp ./msx2-osx/core/* ./msx2-android/msx2/src/main/cpp/core
	cp ./msx2-osx/lz4/* ./msx2-android/msx2/src/main/cpp/lz4

execute-format:
	clang-format -style=file < ./msx2-osx/core/${FILENAME} > ./msx2-osx/core/${FILENAME}.bak
	cat ./msx2-osx/core/${FILENAME}.bak > ./msx2-osx/core/${FILENAME}
	rm ./msx2-osx/core/${FILENAME}.bak
