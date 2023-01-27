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
    unsigned char logo[0x4000];
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
    msx2.setup(0, 0, 4, true, ram + 0x8000, 0x8000, "RAM");
    msx2.setup(3, 0, 0, true, ram, 0x10000, "RAM");
    if (ext && 0x4000 == extSize) {
        memcpy(bios.ext, ext, 0x4000);
        msx2.setup(3, 1, 0, false, bios.ext, 0x4000, "SUB");
    }
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

extern "C" void emu_init_cbios(const void* main, size_t mainSize,
                               const void* logo, size_t logoSize,
                               const void* sub, size_t subSize)
{
    if (main && 0x8000 == mainSize) {
        memcpy(bios.main, main, 0x8000);
        msx2.setup(0, 0, 0, false, bios.main, 0x8000, "MAIN");
    }
    if (logo && 0x4000 == logoSize) {
        memcpy(bios.logo, logo, 0x4000);
        msx2.setup(0, 0, 4, false, bios.logo, 0x4000, "LOGO");
    }
    if (sub && 0x4000 == subSize) {
        memcpy(bios.ext, sub, 0x4000);
        msx2.setup(3, 0, 0, false, bios.ext, 0x4000, "SUB");
    }
    msx2.setup(3, 2, 0, true, ram, 0x10000, "RAM");
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

static void dump(const char* label, unsigned char* vram, int addr, int size)
{
    char line[1024];
    char buf[16];
    char ascii[17];
    printf("%s ($%04X~$%04X)\n", label, addr, addr + size - 1);
    while (0 < size) {
        snprintf(line, sizeof(line), "[$%05X]", addr);
        memset(ascii, 0, sizeof(ascii));
        int i;
        for (i = 0; 0 < size && i < 16; i++) {
            if (i != 7) {
                snprintf(buf, sizeof(buf), " %02X", vram[addr]);
            } else {
                snprintf(buf, sizeof(buf), " - %02X", vram[addr]);
            }
            if (isprint(vram[addr])) {
                ascii[i] = (char)vram[addr];
            } else {
                ascii[i] = '.';
            }
            strcat(line, buf);
            addr++;
            size--;
        }
        for (; i < 16; i++) {
            if (i != 7) {
                strcat(line, "   ");
            } else {
                strcat(line, "     ");
            }
        }
        printf("%s %s\n", line, ascii);
    }
}

extern "C" void emu_dumpVideoMemory()
{
    unsigned char* vram = msx2.vdp.ctx.ram;
    VDP* vdp = &msx2.vdp;
    printf("ScreenMode = %d\n", vdp->getScreenMode());
    dump("PatternNameTable", vram, vdp->getNameTableAddress(), vdp->getNameTableSize());
    dump("PatternGeneratorTable", vram, vdp->getPatternGeneratorAddress(), vdp->getPatternGeneratorSize());
}


