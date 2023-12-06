CROSS_PREFIX = arm-none-eabi
COMMON_FLAGS = 
ASFLAGS = --warn --fatal-warnings
CPPFLAGS = -Wall -Werror -std=c++17 -O2 -nostdlib -nostartfiles -ffreestanding -fno-exceptions
SOURCE =\
	./src/extern.h\
	./src/mailbox.hpp\
	./src/entry.cpp\
	./src/start.s

all: kernel.img

kernel.img: ./Makefile ./src/memmap $(SOURCE)
	$(CROSS_PREFIX)-as $(ASFLAGS) ./src/start.s -o start.o
	$(CROSS_PREFIX)-g++ $(CPPFLAGS) -c ./src/entry.cpp
	$(CROSS_PREFIX)-ld *.o -T ./src/memmap -o kernel.elf
	$(CROSS_PREFIX)-objdump -D kernel.elf > kernel.list
	$(CROSS_PREFIX)-objcopy --srec-forceS3 kernel.elf -O srec kernel.srec
	$(CROSS_PREFIX)-objcopy kernel.elf -O binary kernel.img

clean :
	rm -f *.o
	rm -f *.bin
	rm -f *.hex
	rm -f *.srec
	rm -f *.elf
	rm -f *.list
	rm -f *.img