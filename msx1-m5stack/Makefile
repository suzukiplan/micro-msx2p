all: copy-core
	pio run -t upload

build: copy-core
	pio run

copy-core:
	cp ../src1/*.h include
	cp ../src1/*.hpp include
	cd tools/bin2var && make
	make src/rom_cbios_main_msx1.c
	make src/rom_cbios_logo_msx1.c
	make src/rom_game.c
	make include/roms.hpp

src/rom_cbios_main_msx1.c: bios/cbios_main_msx1.rom
	tools/bin2var/bin2var bios/cbios_main_msx1.rom > src/rom_cbios_main_msx1.c

src/rom_cbios_logo_msx1.c: bios/cbios_logo_msx1.rom
	tools/bin2var/bin2var bios/cbios_logo_msx1.rom > src/rom_cbios_logo_msx1.c

src/rom_game.c: bios/game.rom
	tools/bin2var/bin2var bios/game.rom > src/rom_game.c

include/roms.hpp: src/rom_cbios_main_msx1.c src/rom_cbios_logo_msx1.c src/rom_game.c
	cd tools/varext && make
	tools/varext/varext src/rom_cbios_main_msx1.c src/rom_cbios_logo_msx1.c src/rom_game.c >include/roms.hpp
