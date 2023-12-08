ROMS =\
	rom_cbios_main_msx2p.c\
	rom_cbios_logo_msx2p.c\
	rom_cbios_sub.c\
	rom_game.c

all:
	cd ../tools && git submodule update --init circle64
	cp ../src/* .
	cd ../tools/bin2var && make
	cd ../tools/bmp2img && make
	cd ../tools/varext && make
	make $(ROMS)
	make roms.hpp
	cd ../tools/circle64 && ./makeall --nosample RASPPI=3 AARCH=64
	cd ../tools/circle64/addon/linux && make RASPPI=3 AARCH=64
	cd ../tools/circle64/addon/vc4 && ./makeall --nosample RASPPI=3 AARCH=64
	make -f Build.mak
	@echo Build succeed!
	@echo -------------------------------------------------------------------------------
	@echo Congratulations 🎉
	@echo Place the bootcode.bin, start.elf, and kernel8.img in the root directory of
	@echo your SD-card and insert it into RaspberryPi Zero 2W to lanch your MSX game.
	@echo -------------------------------------------------------------------------------
	@echo - bootcode.bin: https://github.com/raspberrypi/firmware/blob/master/boot/bootcode.bin
	@echo - start4.elf: https://github.com/raspberrypi/firmware/blob/master/boot/start4.elf
	@echo - kernel8-rpi4.img: `ls -l kernel8-rpi4.img | grep kernel`

clean:
	make -f Build.mak clean
	cd ../tools/circle64 && ./makeall clean
	cd ../tools/circle64/addon/linux && make clean
	cd ../tools/circle64/addon/vc4 && ./makeall clean

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
