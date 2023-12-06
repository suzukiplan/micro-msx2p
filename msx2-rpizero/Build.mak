CIRCLEHOME = ./circle
STANDARD = -std=c++17
CPPFLAGS = -DZ80_DISABLE_DEBUG -DZ80_DISABLE_BREAKPOINT -DZ80_DISABLE_NESTCHECK -DZ80_NO_FUNCTIONAL -DZ80_NO_EXCEPTION
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
