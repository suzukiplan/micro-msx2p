ROMS = src/rom_cbios_main_msx1.c\
	src/rom_cbios_logo_msx1.c\
	src/rom_game.c

all: copy-core
	pio run -t upload

init:
	mkdir data
	pio run --target uploadfs

build: copy-core
	pio run

copy-core:
	cp ../src1/*.h include
	cp ../src1/*.hpp include
	cd ../tools/bin2var && make
	cd ../tools/bmp2img && make
	make ${ROMS}
	make include/roms.hpp

src/rom_cbios_main_msx1.c: bios/cbios_main_msx1.rom
	../tools/bin2var/bin2var bios/cbios_main_msx1.rom > src/rom_cbios_main_msx1.c

src/rom_cbios_logo_msx1.c: bios/cbios_logo_msx1.rom
	../tools/bin2var/bin2var bios/cbios_logo_msx1.rom > src/rom_cbios_logo_msx1.c

src/rom_game.c: bios/game.rom
	../tools/bin2var/bin2var bios/game.rom > src/rom_game.c

include/roms.hpp: ${ROMS}
	cd ../tools/varext && make
	../tools/varext/varext ${ROMS} >include/roms.hpp

format:
	make execute-format FILENAME=./src/app.cpp
	make execute-format FILENAME=./include/ILI9431.hpp

execute-format:
	clang-format -style=file < ${FILENAME} > ${FILENAME}.bak
	cat ${FILENAME}.bak > ${FILENAME}
	rm ${FILENAME}.bak
