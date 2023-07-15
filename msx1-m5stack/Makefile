ROMS = src/rom_cbios_main_msx1.c\
	src/rom_cbios_logo_msx1.c\
	src/rom_game.c\
	src/rom_guide_normal.c\
	src/rom_guide_menu.c\
	src/rom_menu_cursor.c

all: copy-core
	pio run -t upload

build: copy-core
	pio run

copy-core:
	cp ../src1/*.h include
	cp ../src1/*.hpp include
	cd tools/bin2var && make
	cd tools/bmp2img && make
	make ${ROMS}
	make include/roms.hpp

src/rom_cbios_main_msx1.c: bios/cbios_main_msx1.rom
	tools/bin2var/bin2var bios/cbios_main_msx1.rom > src/rom_cbios_main_msx1.c

src/rom_cbios_logo_msx1.c: bios/cbios_logo_msx1.rom
	tools/bin2var/bin2var bios/cbios_logo_msx1.rom > src/rom_cbios_logo_msx1.c

src/rom_game.c: bios/game.rom
	tools/bin2var/bin2var bios/game.rom > src/rom_game.c

src/rom_guide_normal.c: bios/guide_normal.bmp
	tools/bmp2img/bmp2img bios/guide_normal.bmp > src/rom_guide_normal.c

src/rom_guide_menu.c: bios/guide_menu.bmp
	tools/bmp2img/bmp2img bios/guide_menu.bmp > src/rom_guide_menu.c

src/rom_menu_cursor.c: bios/menu_cursor.bmp
	tools/bmp2img/bmp2img bios/menu_cursor.bmp > src/rom_menu_cursor.c

include/roms.hpp: ${ROMS}
	cd tools/varext && make
	tools/varext/varext ${ROMS} >include/roms.hpp
