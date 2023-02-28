#include "emu.h"
#include "msx2.hpp"
#include "vgsspu_al.h"
#include <string.h>
#include <unistd.h>

extern "C" {
unsigned short emu_vram[VRAM_WIDTH * 2 * VRAM_HEIGHT * 2];
unsigned char emu_key;
unsigned char emu_keycode;
};

static MSX2 msx2(0);
static unsigned char* rom;
static void* spu;
pthread_mutex_t sound_locker;
static short sound_buffer[65536 * 2];
static unsigned short sound_cursor;

extern "C" void emu_init_bios(const void* main,
                              const void* sub,
                              const void* disk,
                              const void* fm,
                              const void* knj,
                              const void* font)
{
    static struct StaticData {
        unsigned char main[0x8000];
        unsigned char sub[0x4000];
        unsigned char fm[0x4000];
        unsigned char disk[0x4000];
        unsigned char knj[0x8000];
        unsigned char font[0x40000];
        unsigned char empty[0x4000];
    } data;
    memcpy(data.main, main, sizeof(data.main));
    memcpy(data.sub, sub, sizeof(data.sub));
    memcpy(data.fm, fm, sizeof(data.fm));
    memcpy(data.disk, disk, sizeof(data.disk));
    memcpy(data.font, font, sizeof(data.font));
    memcpy(data.knj, knj, sizeof(data.knj));
    msx2.setupSecondaryExist(false, false, false, true);
    msx2.setup(0, 0, 0, data.main, sizeof(data.main), "MAIN");
    msx2.setupRAM(3, 0);
    msx2.setup(3, 1, 0, data.sub, sizeof(data.sub), "SUB");
    msx2.setup(3, 1, 2, data.knj, sizeof(data.knj), "KNJ");
    msx2.setup(3, 2, 0, data.empty, sizeof(data.empty), "DISK");
    msx2.setup(3, 2, 2, data.disk, sizeof(data.disk), "DISK");
    msx2.setup(3, 2, 4, data.empty, sizeof(data.empty), "DISK");
    msx2.setup(3, 2, 6, data.empty, sizeof(data.empty), "DISK");
    msx2.setup(3, 3, 2, data.fm, sizeof(data.fm), "FM");
    msx2.loadFont(data.font, sizeof(data.font));
    emu_reset();
}

extern "C" void emu_init_cbios(const void* main,
                               const void* logo,
                               const void* sub)
{
    static struct StaticData {
        unsigned char main[0x8000];
        unsigned char ext[0x4000];
        unsigned char logo[0x4000];
    } data;
    memcpy(data.main, main, 0x8000);
    memcpy(data.ext, sub, 0x4000);
    memcpy(data.logo, logo, 0x4000);
    msx2.setupSecondaryExist(false, false, false, true);
    msx2.setup(0, 0, 0, data.main, 0x8000, "MAIN");
    msx2.setup(0, 0, 4, data.logo, 0x4000, "LOGO");
    msx2.setup(3, 0, 0, data.ext, 0x4000, "SUB");
    msx2.setupRAM(3, 3);
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
    msx2.setup(0, 0, 0, tm1pbios_, sizeof(tm1pbios_), "MAIN");
    msx2.setupRAM(3, 0);
    msx2.setup(3, 1, 0, tm1pext_, sizeof(tm1pext_), "SUB");
    msx2.setup(3, 1, 2, tm1pkdr_, sizeof(tm1pkdr_), "KNJ");
    msx2.setup(3, 2, 2, tm1pdesk1_, sizeof(tm1pdesk1_), "DSK1");
    msx2.setup(3, 3, 2, tm1pdesk2_, sizeof(tm1pdesk2_), "DSK2");
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
    static unsigned char empty[0x4000];
    //static unsigned char firm_[0x10000];

    memcpy(msx2p_, msx2p, sizeof(msx2p_));
    memcpy(msx2pext_, msx2pext, sizeof(msx2pext_));
    memcpy(msx2pmus_, msx2pmus, sizeof(msx2pmus_));
    memcpy(disk_, disk, sizeof(disk_));
    memcpy(msxkanji_, msxkanji, sizeof(msxkanji_));
    memcpy(kanji_, kanji, sizeof(kanji_));
    //memcpy(firm_, firm, sizeof(firm_));

    msx2.setupSecondaryExist(true, false, false, true);
    msx2.setupRAM(3, 0);
    msx2.setup(0, 0, 0, msx2p_, sizeof(msx2p_), "MAIN");
    msx2.setup(3, 1, 0, msx2pext_, sizeof(msx2pext_), "SUB");
    msx2.setup(0, 2, 2, msx2pmus_, sizeof(msx2pmus_), "FM");
    msx2.setup(3, 2, 0, empty, sizeof(empty), "DISK");
    msx2.setup(3, 2, 2, disk_, sizeof(disk_), "DISK");
    msx2.setup(3, 2, 4, empty, sizeof(empty), "DISK");
    msx2.setup(3, 2, 6, empty, sizeof(empty), "DISK");
    msx2.setup(3, 1, 2, msxkanji_, sizeof(msxkanji_), "KNJ");
    //msx2.setup(3, 3, 0, false, firm_, sizeof(firm_), "FIRM");
    msx2.loadFont(kanji_, sizeof(kanji_));
    emu_reset();
}

void emu_loadRom(const void* rom_, size_t romSize, const char* fileName)
{
    if (rom) free(rom);
    rom = (unsigned char*)malloc(romSize);
    memcpy(rom, rom_, romSize);
    int type = MSX2_ROM_TYPE_NORMAL;
    if (strstr(fileName, "Space Manbow")) {
        type = MSX2_ROM_TYPE_KONAMI_SCC;
    } else if (strstr(fileName, "Zanac-EX")) {
        type = MSX2_ROM_TYPE_ASC16;
    } else if (strstr(fileName, "Relics")) {
        type = MSX2_ROM_TYPE_ASC8;
    } else if (strstr(fileName, "Akumajyo")) {
        type = MSX2_ROM_TYPE_KONAMI;
    } else if (strstr(fileName, "Return of Jelda")) {
        type = MSX2_ROM_TYPE_ASC8;
    } else if (strstr(fileName, "DQUEST2")) {
        type = MSX2_ROM_TYPE_ASC8;
    } else if (strstr(fileName, "Dires")) {
        type = MSX2_ROM_TYPE_ASC8_SRAM2;
    } else if (strstr(fileName, "Fire Ball")) {
        type = MSX2_ROM_TYPE_ASC16;
    } else if (strstr(fileName, "Hydlide III ")) {
        type = MSX2_ROM_TYPE_ASC8;
    }
    printf("load game: %s (type:%d)\n", fileName, type);
    //msx2.loadRom(rom, (int)romSize, MSX2_ROM_TYPE_ASC8);
    //msx2.loadRom(rom, (int)romSize, MSX2_ROM_TYPE_ASC16);
    //msx2.loadRom(rom, (int)romSize, MSX2_ROM_TYPE_KONAMI);
    msx2.loadRom(rom, (int)romSize, type);
    emu_reset();
}

extern "C" void emu_reset()
{
    msx2.reset();
}

static void sound_callback(void* buffer, size_t size)
{
    while (sound_cursor < size / 2) usleep(100);
    pthread_mutex_lock(&sound_locker);
    memcpy(buffer, sound_buffer, size);
    if (size <= sound_cursor)
        sound_cursor -= size;
    else
        sound_cursor = 0;
    pthread_mutex_unlock(&sound_locker);
}

extern "C" void emu_vsync()
{
    static bool initialized = false;
    if (!initialized) {
        pthread_mutex_init(&sound_locker, NULL);
        spu = vgsspu_start2(44100, 16, 2, 23520, sound_callback);
        initialized = true;
    }
    msx2.tick(emu_key, 0, emu_keycode);
    memcpy(emu_vram, msx2.vdp.display, sizeof(emu_vram));
    
    size_t pcmSize;
    void* pcm = msx2.getSound(&pcmSize);
    pthread_mutex_lock(&sound_locker);
    if (pcmSize + sound_cursor < sizeof(sound_buffer)) {
        memcpy(&sound_buffer[sound_cursor], pcm, pcmSize);
        sound_cursor += pcmSize / 2;
    }
    pthread_mutex_unlock(&sound_locker);
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
    auto vdp = &msx2.vdp;
    printf("ScreenMode = %d\n", vdp->getScreenMode());
    printf("NameTable = $%04X\n", vdp->getNameTableAddress());
    printf("R#23(VS) = $%02X\n", vdp->ctx.reg[23]);
    printf("R#26(HS) = $%02X\n", vdp->ctx.reg[26]);
    printf("R#27(HS) = $%02X\n", vdp->ctx.reg[27]);
    dump("PatternNameTable", vram, vdp->getNameTableAddress(), vdp->getNameTableSize());
    dump("PatternGeneratorTable", vram, vdp->getPatternGeneratorAddress(), vdp->getPatternGeneratorSize());
    dump("ColorTable", vram, vdp->getColorTableAddress(), vdp->getColorTableSize());
    dump("SpriteAttributeTabel", vram, vdp->getSpriteAttributeTableM2(), 4 * 32);
    dump("SpriteColorTable", vram, vdp->getSpriteColorTable(), 512);
    dump("SpriteGenerator", vram, vdp->getSpriteGeneratorTable(), 8 * 256);
    /*
    vdp->setVramWriteListener(&msx2, [](void* arg, int addr, unsigned char value) {
        if (addr < 0x07F0)
        ((MSX2*)arg)->putlog("VRAM[%X] = $%02X", addr, value);
    });
     */
}

extern "C" void emu_startDebug()
{
    msx2.cpu->setDebugMessage([](void* arg, const char* msg) {
        auto msx2 = (MSX2*)arg;
        printf("[%d-%d:%d-%d:%d-%d:%d-%d] L=%3d %s\n",
               msx2->mmu.ctx.pri[0], msx2->mmu.ctx.sec[0],
               msx2->mmu.ctx.pri[1], msx2->mmu.ctx.sec[1],
               msx2->mmu.ctx.pri[2], msx2->mmu.ctx.sec[2],
               msx2->mmu.ctx.pri[3], msx2->mmu.ctx.sec[3],
               msx2->vdp.ctx.countV,
               msg);
    });
}

void emu_insertDisk(int driveId, const void* data, size_t size)
{
    msx2.insertDisk(driveId, data, size, false);
}

void emu_ejectDisk(int driveId)
{
    msx2.ejectDisk(driveId);
}

const void* emu_quickSave(size_t* size)
{
    return msx2.quickSave(size);
}

void emu_quickLoad(const void* data, size_t size)
{
    msx2.quickLoad(data, size);
}
