all:
	cd test/performance && make
	cd msx2-dotnet && make clean all

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

execute-format:
	clang-format -style=file < ./src/${FILENAME} > ./src/${FILENAME}.bak
	cat ./src/${FILENAME}.bak > ./src/${FILENAME}
	rm ./src/${FILENAME}.bak
