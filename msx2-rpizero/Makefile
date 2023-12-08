ROMS =\
	rom_cbios_main_msx2p.c\
	rom_cbios_logo_msx2p.c\
	rom_cbios_sub.c\
	rom_game.c

all:
	cd ../tools && git submodule update --init circle
	cp ../src/* .
	cd ../tools/bin2var && make
	cd ../tools/bmp2img && make
	cd ../tools/varext && make
	make $(ROMS)
	make roms.hpp
	cd ../tools/circle && ./makeall
	cd ../tools/circle/addon/linux && make
	cd ../tools/circle/addon/vc4 && ./makeall --nosample
	make -f Build.mak
	@echo Build succeed!
	@echo -------------------------------------------------------------------------------
	@echo Congratulations 🎉
	@echo Place the bootcode.bin, start.elf, and kernel.img in the root directory of
	@echo your SD-card and insert it into RaspberryPi Zero to lanch your MSX game.
	@echo -------------------------------------------------------------------------------
	@echo - bootcode.bin: https://github.com/raspberrypi/firmware/blob/master/boot/bootcode.bin
	@echo - start.elf: https://github.com/raspberrypi/firmware/blob/master/boot/start.elf
	@echo - kernel.img: `ls -l kernel.img | grep kernel`

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
