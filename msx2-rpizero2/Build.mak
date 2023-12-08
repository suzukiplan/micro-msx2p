RASPPI = 3
AARCH = 64
CIRCLEHOME = ../tools/circle64
CHECK_DEPS = 0
STANDARD = -std=c++17
CFLAGS = -O3
CFLAGS += -Wno-stringop-overflow
CPPFLAGS = -O3
CPPFLAGS += -DZ80_DISABLE_DEBUG
CPPFLAGS += -DZ80_DISABLE_BREAKPOINT
CPPFLAGS += -DZ80_DISABLE_NESTCHECK
CPPFLAGS += -DZ80_CALLBACK_WITHOUT_CHECK
CPPFLAGS += -DZ80_CALLBACK_PER_INSTRUCTION
CPPFLAGS += -DZ80_UNSUPPORT_16BIT_PORT
CPPFLAGS += -DZ80_NO_FUNCTIONAL
CPPFLAGS += -DZ80_NO_EXCEPTION
CPPFLAGS += -D_TIME_T_DECLARED
CPPFLAGS += -Wno-int-to-pointer-cast
#CPPFLAGS += -DMSX2_DISPLAY_HALF_HORIZONTAL
OBJS =\
	main.o\
	std.o\
	kernel.o\
	kernel_run.o\
	lz4.o\
	emu2413.o\
	rom_cbios_main_msx2p.o\
	rom_cbios_logo_msx2p.o\
	rom_cbios_sub.o\
	rom_game.o
LIBS =\
	$(CIRCLEHOME)/lib/libcircle.a\
	$(CIRCLEHOME)/lib/sound/libsound.a\
	$(CIRCLEHOME)/lib/sched/libsched.a\
	$(CIRCLEHOME)/addon/vc4/vchiq/libvchiq.a\
	$(CIRCLEHOME)/addon/vc4/sound/libvchiqsound.a\
	$(CIRCLEHOME)/addon/linux/liblinuxemu.a\
	$(CIRCLEHOME)/addon/linux/liblinuxemu.a\
	$(CIRCLEHOME)/lib/input/libinput.a\
	$(CIRCLEHOME)/lib/fs/libfs.a\
	$(CIRCLEHOME)/lib/usb/libusb.a
include $(CIRCLEHOME)/Rules.mk
-include $(DEPS)
