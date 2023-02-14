#include "emu.h"
#include "msx2.hpp"
#include <string.h>

extern "C" {
unsigned short emu_vram[VRAM_WIDTH * 2 * VRAM_HEIGHT];
unsigned char emu_key;
unsigned char emu_keycode;
};

struct BIOS {
    unsigned char main[0x8000];
    unsigned char logo[0x4000];
    unsigned char ext[0x4000];
    unsigned char knj[0x8000];
    unsigned char disk[0x4000];
    unsigned char fm[0x4000];
    unsigned char font[0x40000];
};
static struct BIOS bios;
static MSX2 msx2(0);
static unsigned char* rom;
static unsigned char ram[0x10000];

extern "C" void emu_init_bios(const void* main, size_t mainSize,
                              const void* ext, size_t extSize,
                              const void* disk, size_t diskSize,
                              const void* fm, size_t fmSize,
                              const void* knj, size_t knjSize,
                              const void* font, size_t fontSize)
{
    msx2.setupSecondaryExist(false, false, false, true);
    if (main && 0x8000 == mainSize) {
        memcpy(bios.main, main, 0x8000);
        msx2.setup(0, 0, 0, false, bios.main, 0x8000, "MAIN");
    }
    msx2.setup(3, 3, 0, true, ram, 0x10000, "RAM");
    if (ext && 0x4000 == extSize) {
        memcpy(bios.ext, ext, 0x4000);
        msx2.setup(3, 0, 0, false, bios.ext, 0x4000, "SUB");
    }
    if (knj && 0x8000 == knjSize) {
        memcpy(bios.knj, knj, 0x8000);
        msx2.setup(3, 0, 2, false, bios.knj, 0x8000, "KNJ");
    }
    if (disk && 0x4000 <= diskSize) {
        memcpy(bios.disk, disk, 0x4000);
        msx2.setup(3, 1, 2, false, bios.disk, 0x4000, "DISK");
    }
    if (fm && 0x4000 <= fmSize) {
        memcpy(bios.fm, fm, 0x4000);
        msx2.setup(3, 2, 2, false, bios.fm, 0x4000, "FM");
    }
    if (font && 0 < fontSize) {
        msx2.loadFont(font, fontSize);
    }
    emu_reset();
}

extern "C" void emu_init_cbios(const void* main, size_t mainSize,
                               const void* logo, size_t logoSize,
                               const void* sub, size_t subSize)
{
    memcpy(bios.main, main, 0x8000);
    memcpy(bios.ext, sub, 0x4000);
    memcpy(bios.logo, logo, 0x4000);
    msx2.setupSecondaryExist(false, false, false, true);
    msx2.setup(0, 0, 0, false, bios.main, 0x8000, "MAIN");
    msx2.setup(0, 0, 4, false, bios.logo, 0x4000, "LOGO");
    msx2.setup(3, 0, 0, false, bios.ext, 0x4000, "SUB");
    msx2.setup(3, 3, 0, true, ram, 0x10000, "RAM");
    emu_reset();
}

extern "C" void emu_init_bios_tm1p(const void* tm1pbios,
                                   const void* tm1pext,
                                   const void* tm1pkdr,
                                   const void* tm1pdesk1,
                                   const void* tm1pdesk2,
                                   const void* font,
                                   size_t fontSize)
{
    /*
     0 0 0 4 20 "Machines/MSX2+ - Toshiba FS-TM1+\tm1pbios.rom" ""
     3 0 0 8 23 "" ""
     3 1 0 2 20 "Machines/MSX2+ - Toshiba FS-TM1+\tm1pext.rom" ""
     3 1 2 4 42 "Machines/MSX2+ - Toshiba FS-TM1+\tm1pkdr.rom" ""
     3 2 2 4 42 "Machines/MSX2+ - Toshiba FS-TM1+\tm1pdesk1.rom" ""
     3 3 2 4 42 "Machines/MSX2+ - Toshiba FS-TM1+\tm1pdesk2.rom" ""
     */
    static unsigned char tm1pbios_[0x8000];
    static unsigned char tm1pext_[0x4000];
    static unsigned char tm1pkdr_[0x8000];
    static unsigned char tm1pdesk1_[0x8000];
    static unsigned char tm1pdesk2_[0x8000];
    memcpy(tm1pbios_, tm1pbios, sizeof(tm1pbios_));
    memcpy(tm1pext_, tm1pext, sizeof(tm1pext_));
    memcpy(tm1pkdr_, tm1pkdr, sizeof(tm1pkdr_));
    memcpy(tm1pdesk1_, tm1pdesk1, sizeof(tm1pdesk1_));
    memcpy(tm1pdesk2_, tm1pdesk2, sizeof(tm1pdesk2_));
    msx2.setupSecondaryExist(false, false, false, true);
    msx2.setup(0, 0, 0, false, tm1pbios_, sizeof(tm1pbios_), "MAIN");
    msx2.setup(3, 0, 0, true, ram, sizeof(ram), "RAM");
    msx2.setup(3, 1, 0, false, tm1pext_, sizeof(tm1pext_), "SUB");
    msx2.setup(3, 1, 2, false, tm1pkdr_, sizeof(tm1pkdr_), "KNJ");
    msx2.setup(3, 2, 2, false, tm1pdesk1_, sizeof(tm1pdesk1_), "DSK1");
    msx2.setup(3, 3, 2, false, tm1pdesk2_, sizeof(tm1pdesk2_), "DSK2");
    msx2.loadFont(font, fontSize);
    emu_reset();
}

void emu_init_bios_fsa1wsx(const void* msx2p,
                           const void* msx2pext,
                           const void* msx2pmus,
                           const void* disk,
                           const void* msxkanji,
                           const void* kanji,
                           const void* firm)
{
    static unsigned char msx2p_[0x8000];
    static unsigned char msx2pext_[0x4000];
    static unsigned char msx2pmus_[0x4000];
    static unsigned char disk_[0x4000];
    static unsigned char msxkanji_[0x8000];
    static unsigned char kanji_[0x40000];
    static unsigned char firm_[0x10000];

    memcpy(msx2p_, msx2p, sizeof(msx2p_));
    memcpy(msx2pext_, msx2pext, sizeof(msx2pext_));
    memcpy(msx2pmus_, msx2pmus, sizeof(msx2pmus_));
    memcpy(disk_, disk, sizeof(disk_));
    memcpy(msxkanji_, msxkanji, sizeof(msxkanji_));
    memcpy(kanji_, kanji, sizeof(kanji_));
    memcpy(firm_, firm, sizeof(firm_));

    msx2.setupSecondaryExist(true, false, false, true);
    msx2.setup(0, 0, 0, false, msx2p_, sizeof(msx2p_), "MAIN");
    msx2.setup(3, 0, 0, true, ram, sizeof(ram), "RAM");
    msx2.setup(3, 1, 0, false, msx2pext_, sizeof(msx2pext_), "SUB");
    msx2.setup(0, 2, 2, false, msx2pmus_, sizeof(msx2pmus_), "MUS");
    msx2.setup(3, 2, 2, false, disk_, sizeof(disk_), "DISK");
    msx2.setup(3, 1, 2, false, msxkanji_, sizeof(msxkanji_), "KNJ");
    msx2.setup(3, 3, 0, false, firm_, sizeof(firm_), "FIRM");
    msx2.loadFont(kanji_, sizeof(kanji_));
    emu_reset();
}

void emu_loadRom(const void* rom_, size_t romSize)
{
    if (rom) free(rom);
    rom = (unsigned char*)malloc(romSize);
    memcpy(rom, rom_, romSize);
    msx2.loadRom(rom, (int)romSize, MSX2_ROM_TYPE_KONAMI_SCC);
    emu_reset();
}

extern "C" void emu_reset()
{
    msx2.reset();
    memset(ram, 0xFF, sizeof(ram));
}

extern "C" void emu_vsync()
{
    msx2.tick(emu_key, 0, emu_keycode);
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
            if (i != 8) {
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
    dump("ColorTable", vram, vdp->getColorTableAddress(), vdp->getColorTableSize());
    dump("SpriteAttributeTabel", vram, vdp->getSpriteAttributeTable(), 4 * 32);
    dump("SpriteColorTable", vram, vdp->getSpriteColorTable(), 512);
    dump("SpriteGenerator", vram, vdp->getSpriteGeneratorTable(), 8 * 256);
}

extern "C" void emu_startDebug()
{
    msx2.cpu->setDebugMessage([](void* arg, const char* msg) {
        auto msx2 = (MSX2*)arg;
        printf("[%d:%d:%d:%d] L=%3d %s\n",
               msx2->mmu.ctx.primary[0],
               msx2->mmu.ctx.primary[1],
               msx2->mmu.ctx.primary[2],
               msx2->mmu.ctx.primary[3],
               msx2->vdp.ctx.countV,
               msg);
    });
}

