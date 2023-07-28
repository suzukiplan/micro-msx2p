all:
	pio run -t upload

init:
	mkdir data
	pio run --target uploadfs

build:
	pio run

format:
	make execute-format FILENAME=./src/app.cpp
	make execute-format FILENAME=./include/ILI9431.hpp

execute-format:
	clang-format -style=file < ${FILENAME} > ${FILENAME}.bak
	cat ${FILENAME}.bak > ${FILENAME}
	rm ${FILENAME}.bak
