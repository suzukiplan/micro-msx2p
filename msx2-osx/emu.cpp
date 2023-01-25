#include "emu.h"
#include "msx2.hpp"
#include <string.h>

extern "C" {
unsigned short emu_vram[VRAM_WIDTH * VRAM_HEIGHT];
unsigned char emu_key;
unsigned char emu_keycode;
};

struct BIOS {
    unsigned char main[0x8000];
    unsigned char ext[0x4000];
    unsigned char disk[0x4000];
    unsigned char fm[0x4000];
};
static struct BIOS bios;
static MSX2 msx2(0);
static unsigned char* rom;
static unsigned char ram[0x10000];

extern "C" void emu_init_bios(const void* main, size_t mainSize,
                              const void* ext, size_t extSize,
                              const void* disk, size_t diskSize,
                              const void* fm, size_t fmSize)
{
    if (main && 0x8000 == mainSize) {
        memcpy(bios.main, main, 0x8000);
        msx2.setup(0, 0, 0, false, bios.main, 0x8000, "MAIN");
    }
    if (ext && 0x4000 == extSize) {
        memcpy(bios.ext, ext, 0x4000);
        msx2.setup(0, 3, 0, false, bios.ext, 0x4000, "SUB");
    }
    msx2.setup(3, 0, 0, true, ram, 0x10000, "RAM");
    if (disk && 0x4000 <= diskSize) {
        memcpy(bios.disk, disk, 0x4000);
        msx2.setup(3, 2, 2, false, bios.disk, 0x4000, "DISK");
    }
    if (fm && 0x4000 <= fmSize) {
        memcpy(bios.fm, fm, 0x4000);
        msx2.setup(3, 3, 2, false, bios.fm, 0x4000, "FM");
    }
    msx2.reset();
}

void emu_loadRom(const void* rom_, size_t romSize)
{
    if (rom) free(rom);
    rom = (unsigned char*)malloc(romSize);
    memcpy(rom, rom_, romSize);
    msx2.loadRom(rom, (int)romSize);
}

extern "C" void emu_reset()
{
    msx2.reset();
}

extern "C" void emu_vsync()
{
    msx2.tick();
    memcpy(emu_vram, msx2.vdp.display, sizeof(emu_vram));
}

extern "C" void emu_destroy()
{
}

