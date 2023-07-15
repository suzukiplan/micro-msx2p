ROMS = src/rom_cbios_main_msx1.c\
	src/rom_cbios_logo_msx1.c\
	src/rom_game.c\
	src/rom_guide_normal.c\
	src/rom_guide_menu.c\
	src/rom_sound_mute.c\
	src/rom_sound_low.c\
	src/rom_sound_mid.c\
	src/rom_sound_high.c\
	src/rom_slot1.c\
	src/rom_slot2.c\
	src/rom_slot3.c\
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

src/rom_sound_mute.c: bios/sound_mute.bmp
	tools/bmp2img/bmp2img bios/sound_mute.bmp > src/rom_sound_mute.c

src/rom_sound_low.c: bios/sound_low.bmp
	tools/bmp2img/bmp2img bios/sound_low.bmp > src/rom_sound_low.c

src/rom_sound_mid.c: bios/sound_mid.bmp
	tools/bmp2img/bmp2img bios/sound_mid.bmp > src/rom_sound_mid.c

src/rom_sound_high.c: bios/sound_high.bmp
	tools/bmp2img/bmp2img bios/sound_high.bmp > src/rom_sound_high.c

src/rom_slot1.c: bios/slot1.bmp
	tools/bmp2img/bmp2img bios/slot1.bmp > src/rom_slot1.c

src/rom_slot2.c: bios/slot2.bmp
	tools/bmp2img/bmp2img bios/slot2.bmp > src/rom_slot2.c

src/rom_slot3.c: bios/slot3.bmp
	tools/bmp2img/bmp2img bios/slot3.bmp > src/rom_slot3.c

include/roms.hpp: ${ROMS}
	cd tools/varext && make
	tools/varext/varext ${ROMS} >include/roms.hpp
