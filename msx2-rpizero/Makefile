ROMS =\
	rom_cbios_main_msx2p.c\
	rom_cbios_logo_msx2p.c\
	rom_cbios_sub.c\
	rom_game.c

all: ../tools/circle
	cp ../src/* .
	cd ../tools/bin2var && make
	cd ../tools/bmp2img && make
	cd ../tools/varext && make
	make $(ROMS)
	make roms.hpp
	cd ../tools/circle && ./makeall
	make -f Build.mak

../tools/circle:
	cd ../tools && git submodule update --init circle

clean:
	make -f Build.mak clean

rom_cbios_main_msx2p.c: bios/cbios_main_msx2p.rom
	../tools/bin2var/bin2var bios/cbios_main_msx2p.rom > rom_cbios_main_msx2p.c

rom_cbios_logo_msx2p.c: bios/cbios_logo_msx2p.rom
	../tools/bin2var/bin2var bios/cbios_logo_msx2p.rom > rom_cbios_logo_msx2p.c

rom_cbios_sub.c: bios/cbios_sub.rom
	../tools/bin2var/bin2var bios/cbios_sub.rom > rom_cbios_sub.c

rom_game.c: bios/game.rom
	../tools/bin2var/bin2var bios/game.rom > rom_game.c

roms.hpp: $(ROMS)
	../tools/varext/varext ${ROMS} >roms.hpp
