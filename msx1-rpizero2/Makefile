RASPPI = 3
AARCH = 64
CIRCLE = circle8
CIRCLEHOME = ../tools/$(CIRCLE)
KERNEL_IMG = kernel8.img

ROMS =\
	rom_cbios_main_msx1.c\
	rom_cbios_logo_msx1.c\
	rom_game.c

all:
	cd ../tools && git submodule update --init $(CIRCLE)
	cp ../src1/* .
	cd ../tools/bin2var && make
	cd ../tools/bmp2img && make
	cd ../tools/varext && make
	make $(ROMS)
	make roms.hpp
	cd $(CIRCLEHOME) && ./makeall --nosample RASPPI=$(RASPPI) AARCH=$(AARCH)
	cd $(CIRCLEHOME)/addon/linux && make RASPPI=$(RASPPI) AARCH=$(AARCH)
	cd $(CIRCLEHOME)/addon/vc4 && ./makeall --nosample RASPPI=$(RASPPI) AARCH=$(AARCH)
	make -f Build.mak CIRCLEHOME=$(CIRCLEHOME) RASPPI=$(RASPPI) AARCH=$(AARCH)
	@echo Build succeed!
	@echo -------------------------------------------------------------------------------
	@echo Congratulations 🎉
	@echo Place the bootcode.bin, start.elf, and kernel image in the root directory of
	@echo your SD-card and insert it into RaspberryPi Zero to lanch your MSX game.
	@echo -------------------------------------------------------------------------------
	@echo - bootcode.bin: https://github.com/raspberrypi/firmware/blob/master/boot/bootcode.bin
	@echo - start.elf: https://github.com/raspberrypi/firmware/blob/master/boot/start.elf
	@echo - kernel image: `ls -l $(KERNEL_IMG) | grep $(KERNEL_IMG)`

clean:
	make -f Build.mak clean CIRCLEHOME=$(CIRCLEHOME) RASPPI=$(RASPPI) AARCH=$(AARCH)
	cd $(CIRCLEHOME) && ./makeall clean
	cd $(CIRCLEHOME)/addon/linux && make clean
	cd $(CIRCLEHOME)/addon/vc4 && ./makeall clean

rom_cbios_main_msx1.c: bios/cbios_main_msx1.rom
	../tools/bin2var/bin2var bios/cbios_main_msx1.rom > rom_cbios_main_msx1.c

rom_cbios_logo_msx1.c: bios/cbios_logo_msx1.rom
	../tools/bin2var/bin2var bios/cbios_logo_msx1.rom > rom_cbios_logo_msx1.c

rom_game.c: bios/game.rom
	../tools/bin2var/bin2var bios/game.rom > rom_game.c

roms.hpp: $(ROMS)
	../tools/varext/varext ${ROMS} >roms.hpp
