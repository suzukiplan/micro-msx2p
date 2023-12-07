CIRCLEHOME = ../tools/circle
CHECK_DEPS = 0
STANDARD = -std=c++17
CFLAGS = -O3
CPPFLAGS = -O3
CPPFLAGS += -DZ80_DISABLE_DEBUG
CPPFLAGS += -DZ80_DISABLE_BREAKPOINT
CPPFLAGS += -DZ80_DISABLE_NESTCHECK
CPPFLAGS += -DZ80_NO_FUNCTIONAL
CPPFLAGS += -DZ80_NO_EXCEPTION
CPPFLAGS += -DMSX2_DISPLAY_HALF_HORIZONTAL
OBJS = main.o\
	std.o\
	kernel.o\
	lz4.o\
	emu2413.o\
	rom_cbios_main_msx2p.o\
	rom_cbios_logo_msx2p.o\
	rom_cbios_sub.o\
	rom_game.o
LIBS = $(CIRCLEHOME)/lib/libcircle.a
include $(CIRCLEHOME)/Rules.mk
-include $(DEPS)
