#include "emu.h"
#include "msx2.hpp"
#include "vgsspu_al.h"
#include "lz4.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define PLAYLOG_MAX_TICK_COUNT 655360

extern "C" {
unsigned short emu_vram[VRAM_WIDTH * VRAM_HEIGHT];
unsigned char emu_key;
unsigned char emu_keycode;
};

static MSX2 msx2(0);
static unsigned char* rom;
static void* spu;
pthread_mutex_t sound_locker;
static short sound_buffer[65536 * 2];
static unsigned short sound_cursor;

static struct TypeWriter {
    char buf[65536];
    int index;
    int size;
    int wait;
} tw;

static struct Playlog {
    bool isRecording;
    bool isReplay;
    const void* disk1;
    size_t disk1Size;
    bool disk1ReadOnly;
    const void* disk2;
    size_t disk2Size;
    bool disk2ReadOnly;
    const void* rom;
    size_t romSize;
    int romType;
    void* save;
    size_t saveSize;
    int tickCount;
    unsigned char tickPad1[PLAYLOG_MAX_TICK_COUNT];
    unsigned char tickPad2[PLAYLOG_MAX_TICK_COUNT];
    unsigned char tickKey[PLAYLOG_MAX_TICK_COUNT];
} playlog;

static void* replayBuffer;
static int replayTickPtr;

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
    msx2.setupRAM(3, 0);
    msx2.setup(0, 0, 0, data.main, sizeof(data.main), "MAIN");
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
    } else if (strstr(fileName, "yureikun")) {
        type = MSX2_ROM_TYPE_ASC8;
    } else if (strstr(fileName, "rickmick")) {
        type = MSX2_ROM_TYPE_ASC8;
    } else if (strstr(fileName, "Hydlide III ") || strstr(fileName, "hydlide3")) {
        type = MSX2_ROM_TYPE_ASC8;
    } else if (strstr(fileName, "Hydlide II ") || strstr(fileName, "hydlide2")) {
        type = MSX2_ROM_TYPE_ASC16_SRAM2;
    } else if (strstr(fileName, "Dragon Buster")) {
        type = MSX2_ROM_TYPE_ASC8;
    } else if (strstr(fileName, "Dragon Slayer 4")) {
        type = MSX2_ROM_TYPE_ASC8;
    } else if (strstr(fileName, "Mad Rider")) {
        type = MSX2_ROM_TYPE_ASC8;
    } else if (strstr(fileName, "Metal Gear 2")) {
        type = MSX2_ROM_TYPE_KONAMI_SCC; // currently cannot execute
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

static void tick(unsigned char pad1, unsigned char pad2, unsigned char key)
{
    if (playlog.isRecording && playlog.tickCount < PLAYLOG_MAX_TICK_COUNT) {
        playlog.tickPad1[playlog.tickCount] = pad1;
        playlog.tickPad2[playlog.tickCount] = pad2;
        playlog.tickKey[playlog.tickCount] = key;
        playlog.tickCount++;
    }
    msx2.tick(pad1, pad2, key);
}

extern "C" void emu_vsync()
{
    static bool initialized = false;
    if (!initialized) {
        pthread_mutex_init(&sound_locker, NULL);
        spu = vgsspu_start2(44100, 16, 2, 23520, sound_callback);
        initialized = true;
    }
    if (playlog.isReplay) {
        if (replayTickPtr < playlog.tickCount) {
            tick(playlog.tickPad1[replayTickPtr],
                 playlog.tickPad2[replayTickPtr],
                 playlog.tickKey[replayTickPtr]);
            replayTickPtr++;
        }
    } else if (0 < tw.size) {
        if (tw.wait) {
            if (msx2.ctx.readKey < 1) {
                tick(0, 0, tw.buf[tw.index]);
            } else {
                tick(0, 0, 0);
                tw.wait++;
                if (3 <= tw.wait) {
                    tw.wait = 0;
                    tw.index++;
                    tw.size--;
                }
            }
        } else {
            msx2.ctx.readKey = 0;
            tick(0, 0, tw.buf[tw.index]);
            tw.wait = 1;
        }
    } else {
        tick(emu_key, 0, emu_keycode);
    }
    memcpy(emu_vram, msx2.getDisplay(), sizeof(emu_vram));
    
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
    unsigned char* vram = msx2.vdp->ctx.ram;
    auto vdp = msx2.vdp;
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
               msx2->mmu->ctx.pri[0], msx2->mmu->ctx.sec[0],
               msx2->mmu->ctx.pri[1], msx2->mmu->ctx.sec[1],
               msx2->mmu->ctx.pri[2], msx2->mmu->ctx.sec[2],
               msx2->mmu->ctx.pri[3], msx2->mmu->ctx.sec[3],
               msx2->vdp->ctx.countV,
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

const void* emu_getRAM(void)
{
    return msx2.mmu->ram;
}

const void* emu_getVRAM(void)
{
    return msx2.vdp->ctx.ram;
}

typedef struct BitmapHeader_ {
    int isize;             /* 情報ヘッダサイズ */
    int width;             /* 幅 */
    int height;            /* 高さ */
    unsigned short planes; /* プレーン数 */
    unsigned short bits;   /* 色ビット数 */
    unsigned int ctype;    /* 圧縮形式 */
    unsigned int gsize;    /* 画像データサイズ */
    int xppm;              /* X方向解像度 */
    int yppm;              /* Y方向解像度 */
    unsigned int cnum;     /* 使用色数 */
    unsigned int inum;     /* 重要色数 */
} BitmapHeader;

const void* emu_getBitmapSprite(size_t* size) {
    static const unsigned char bit[8] = {
        0b10000000,
        0b01000000,
        0b00100000,
        0b00010000,
        0b00001000,
        0b00000100,
        0b00000010,
        0b00000001};
    static unsigned char buf[14 + 40 + 4 * 256 + 128 * 128];
    int sg = msx2.vdp->getSpriteGeneratorTable();
    unsigned char* vram = msx2.vdp->ctx.ram;
    int iSize = (int)sizeof(buf);
    *size = iSize;
    memset(buf, 0, sizeof(buf));
    int ptr = 0;
    buf[ptr++] = 'B';
    buf[ptr++] = 'M';
    memcpy(&buf[ptr], &iSize, 4);
    ptr += 4;
    ptr += 4;
    iSize = 40 + 256 * 4;
    memcpy(&buf[ptr], &iSize, 4);
    ptr += 4;
    BitmapHeader header;
    header.isize = 40;
    header.width = 128;
    header.height = -128;
    header.planes = 1;
    header.bits = 8;
    header.ctype = 0;
    header.gsize = 128 * 128;
    header.xppm = 1;
    header.yppm = 1;
    header.cnum = 0;
    header.inum = 0;
    memcpy(&buf[ptr], &header, sizeof(header));
    ptr += sizeof(header);
    for (int i = 0; i < 256; i++) {
        if (i == 1) {
            buf[ptr++] = 0xFF;
            buf[ptr++] = 0xFF;
            buf[ptr++] = 0xFF;
            buf[ptr++] = 0x00;
        } else {
            buf[ptr++] = 0x00;
            buf[ptr++] = 0x00;
            buf[ptr++] = 0x00;
            buf[ptr++] = 0x00;
        }
    }
    for (int i = 0; i < 256; i++) {
        for (int y = 0; y < 8; y++) {
            unsigned char ptn = vram[sg + i * 8 + y];
            for (int x = 0; x < 8; x++) {
                if (ptn & bit[x]) {
                    buf[ptr + i % 16 * 8 + i / 16 * 128 * 8 + y * 128 + x] = 1;
                }
            }
        }
    }
    *size = sizeof(buf);
    return buf;
}

const void* emu_getBitmapVRAM(size_t* size) {
    auto screenMode = msx2.vdp->getScreenMode();
    switch (screenMode) {
        case 0b00011: // Graphic4
        {
            puts("VRAM Type: Graphic4");
            static unsigned char buf[14 + 40 + 4 * 256 + 256 * 1024];
            int iSize = (int)sizeof(buf);
            *size = iSize;
            memset(buf, 0, sizeof(buf));
            int ptr = 0;
            buf[ptr++] = 'B';
            buf[ptr++] = 'M';
            memcpy(&buf[ptr], &iSize, 4);
            ptr += 4;
            ptr += 4;
            iSize = 40 + 256 * 4;
            memcpy(&buf[ptr], &iSize, 4);
            ptr += 4;
            BitmapHeader header;
            header.isize = 40;
            header.width = 256;
            header.height = 1024;
            header.planes = 1;
            header.bits = 8;
            header.ctype = 0;
            header.gsize = 256 * 1024;
            header.xppm = 1;
            header.yppm = 1;
            header.cnum = 0;
            header.inum = 0;
            memcpy(&buf[ptr], &header, sizeof(header));
            ptr += sizeof(header);
            for (int i = 0; i < 16; i++) {
                unsigned short pal = msx2.vdp->palette[i];
                buf[ptr++] = (unsigned char)((pal & 0b0000000000011111) << 3);
                buf[ptr++] = (unsigned char)((pal & 0b0000001111100000) >> 2);
                buf[ptr++] = (unsigned char)((pal & 0b0111110000000000) >> 7);
                buf[ptr++] = 0x00;
            }
            for (int i = 16; i < 256; i++) {
                buf[ptr++] = 0x00;
                buf[ptr++] = 0x00;
                buf[ptr++] = 0x00;
                buf[ptr++] = 0x00;
            }
            for (int vptr = 1023 * 0x80; 0 <= vptr; vptr -= 0x80) {
                for (int i = 0; i < 256; i++) {
                    unsigned char v = msx2.vdp->ctx.ram[vptr + i / 2];
                    v >>= i & 1 ? 0 : 4;
                    v &= 0x0F;
                    buf[ptr++] = v;
                }
            }
            return buf;
        }
        case 0b00100: // Graphic5
        {
            puts("VRAM Type: Graphic5");
            static unsigned char buf[14 + 40 + 4 * 256 + 512 * 1024];
            int iSize = (int)sizeof(buf);
            *size = iSize;
            memset(buf, 0, sizeof(buf));
            int ptr = 0;
            buf[ptr++] = 'B';
            buf[ptr++] = 'M';
            memcpy(&buf[ptr], &iSize, 4);
            ptr += 4;
            ptr += 4;
            iSize = 40 + 256 * 4;
            memcpy(&buf[ptr], &iSize, 4);
            ptr += 4;
            BitmapHeader header;
            header.isize = 40;
            header.width = 512;
            header.height = 1024;
            header.planes = 1;
            header.bits = 8;
            header.ctype = 0;
            header.gsize = 512 * 1024;
            header.xppm = 1;
            header.yppm = 1;
            header.cnum = 0;
            header.inum = 0;
            memcpy(&buf[ptr], &header, sizeof(header));
            ptr += sizeof(header);
            for (int i = 0; i < 4; i++) {
                unsigned short pal = msx2.vdp->palette[i];
                buf[ptr++] = (unsigned char)((pal & 0b0000000000011111) << 3);
                buf[ptr++] = (unsigned char)((pal & 0b0000001111100000) >> 2);
                buf[ptr++] = (unsigned char)((pal & 0b0111110000000000) >> 7);
                buf[ptr++] = 0x00;
            }
            for (int i = 4; i < 256; i++) {
                buf[ptr++] = 0x00;
                buf[ptr++] = 0x00;
                buf[ptr++] = 0x00;
                buf[ptr++] = 0x00;
            }
            for (int vptr = 1023 * 0x80; 0 <= vptr; vptr -= 0x80) {
                for (int i = 0; i < 512; i++) {
                    unsigned char v = msx2.vdp->ctx.ram[vptr + i / 4];
                    switch (i & 3) {
                        case 0: v >>= 6; break;
                        case 1: v >>= 4; break;
                        case 2: v >>= 2; break;
                    }
                    v &= 0x03;
                    buf[ptr++] = v;
                }
            }
            return buf;
        }
        case 0b00101: // Graphic6
        {
            puts("VRAM Type: Graphic6");
            static unsigned char buf[14 + 40 + 4 * 256 + 512 * 512];
            int iSize = (int)sizeof(buf);
            *size = iSize;
            memset(buf, 0, sizeof(buf));
            int ptr = 0;
            buf[ptr++] = 'B';
            buf[ptr++] = 'M';
            memcpy(&buf[ptr], &iSize, 4);
            ptr += 4;
            ptr += 4;
            iSize = 40 + 256 * 4;
            memcpy(&buf[ptr], &iSize, 4);
            ptr += 4;
            BitmapHeader header;
            header.isize = 40;
            header.width = 512;
            header.height = 512;
            header.planes = 1;
            header.bits = 8;
            header.ctype = 0;
            header.gsize = 512 * 512;
            header.xppm = 1;
            header.yppm = 1;
            header.cnum = 0;
            header.inum = 0;
            memcpy(&buf[ptr], &header, sizeof(header));
            ptr += sizeof(header);
            for (int i = 0; i < 16; i++) {
                unsigned short pal = msx2.vdp->palette[i];
                buf[ptr++] = (unsigned char)((pal & 0b0000000000011111) << 3);
                buf[ptr++] = (unsigned char)((pal & 0b0000001111100000) >> 2);
                buf[ptr++] = (unsigned char)((pal & 0b0111110000000000) >> 7);
                buf[ptr++] = 0x00;
            }
            for (int i = 16; i < 256; i++) {
                buf[ptr++] = 0x00;
                buf[ptr++] = 0x00;
                buf[ptr++] = 0x00;
                buf[ptr++] = 0x00;
            }
            for (int vptr = 511 * 0x100; 0 <= vptr; vptr -= 0x100) {
                for (int i = 0; i < 512; i++) {
                    unsigned char v = msx2.vdp->ctx.ram[vptr + i / 2];
                    v >>= i & 1 ? 0 : 4;
                    v &= 0x0F;
                    buf[ptr++] = v;
                }
            }
            return buf;
        }
        case 0b00111: // Graphic7
        {
            puts("VRAM Type: Graphic7");
            static unsigned char buf[14 + 40 + 4 * 256 + 256 * 512];
            int iSize = (int)sizeof(buf);
            *size = iSize;
            memset(buf, 0, sizeof(buf));
            int ptr = 0;
            buf[ptr++] = 'B';
            buf[ptr++] = 'M';
            memcpy(&buf[ptr], &iSize, 4);
            ptr += 4;
            ptr += 4;
            iSize = 40 + 256 * 4;
            memcpy(&buf[ptr], &iSize, 4);
            ptr += 4;
            BitmapHeader header;
            header.isize = 40;
            header.width = 256;
            header.height = 512;
            header.planes = 1;
            header.bits = 8;
            header.ctype = 0;
            header.gsize = 256 * 512;
            header.xppm = 1;
            header.yppm = 1;
            header.cnum = 0;
            header.inum = 0;
            memcpy(&buf[ptr], &header, sizeof(header));
            ptr += sizeof(header);
            for (int i = 0; i < 256; i++) {
                unsigned short pal = msx2.vdp->convertColor_8bit_to_16bit((unsigned char) i);
                buf[ptr++] = (unsigned char)((pal & 0b0000000000011111) << 3);
                buf[ptr++] = (unsigned char)((pal & 0b0000001111100000) >> 2);
                buf[ptr++] = (unsigned char)((pal & 0b0111110000000000) >> 7);
                buf[ptr++] = 0x00;
            }
            for (int vptr = 511 * 0x100; 0 <= vptr; vptr -= 0x100) {
                for (int i = 0; i < 256; i++) {
                    buf[ptr++] = msx2.vdp->ctx.ram[vptr + i];
                }
            }
            return buf;
        }
        default:
            printf("Unsupported screen mode: %d\n", screenMode);
    }
    return nullptr;
}

static unsigned char bit5_to_bit8(unsigned char col)
{
    unsigned char result = (col & 0b00011111) << 3;
    result |= (col & 0b10000000) >> 5;
    result |= (col & 0b01000000) >> 5;
    result |= (col & 0b00100000) >> 5;
    return result;
}

const void* emu_getBitmapScreen(size_t* size) {
    static unsigned char buf[14 + 40 + 568 * 480 * 4];
    int iSize = (int)sizeof(buf);
    *size = iSize;
    memset(buf, 0, sizeof(buf));
    int ptr = 0;
    buf[ptr++] = 'B';
    buf[ptr++] = 'M';
    memcpy(&buf[ptr], &iSize, 4);
    ptr += 4;
    ptr += 4;
    iSize = 14 + 40;
    memcpy(&buf[ptr], &iSize, 4);
    ptr += 4;
    BitmapHeader header;
    header.isize = 40;
    header.width = 568;
    header.height = 480;
    header.planes = 1;
    header.bits = 32;
    header.ctype = 0;
    header.gsize = 568 * 480 * 4;
    header.xppm = 1;
    header.yppm = 1;
    header.cnum = 0;
    header.inum = 0;
    memcpy(&buf[ptr], &header, sizeof(header));
    ptr += sizeof(header);
    for (int y = 0; y < 240; y++) {
        for (int x = 0; x < 568; x++) {
            auto col = msx2.getDisplay()[(239 - y) * 568 + x];
            buf[ptr++] = bit5_to_bit8(col & 0b0000000000011111);
            buf[ptr++] = bit5_to_bit8((col & 0b0000001111100000) >> 5);
            buf[ptr++] = bit5_to_bit8((col & 0b0111110000000000) >> 10);
            buf[ptr++] = 0x00;
        }
        memcpy(&buf[ptr], &buf[ptr - 568 * 4], 568 * 4);
        ptr += 568 * 4;
    }
    return buf;
}

void emu_startTypeWriter(const char* text)
{
    tw.size = (int)strlen(text);
    if (65536 <= tw.size) {
        tw.size = 0;
        return;
    }
    memcpy(tw.buf, text, tw.size);
    tw.index = 0;
    tw.wait = 0;
    msx2.ctx.readKey = 0;
    }

static bool onceLog[65536];
static unsigned char hextbl[256];
static int nestLevel = 0;

void emu_loggingOnce(void)
{
    memset(onceLog, 0, sizeof(onceLog));
    hextbl['0'] = 0;
    hextbl['1'] = 1;
    hextbl['2'] = 2;
    hextbl['3'] = 3;
    hextbl['4'] = 4;
    hextbl['5'] = 5;
    hextbl['6'] = 6;
    hextbl['7'] = 7;
    hextbl['8'] = 8;
    hextbl['9'] = 9;
    hextbl['A'] = 10;
    hextbl['B'] = 11;
    hextbl['C'] = 12;
    hextbl['D'] = 13;
    hextbl['E'] = 14;
    hextbl['F'] = 15;
    msx2.cpu->addCallHandler([](void* arg) {
        nestLevel++;
    });
    msx2.cpu->addReturnHandler([](void* arg) {
        nestLevel--;
    });
    msx2.cpu->setDebugMessage([](void* arg, const char* msg) {
        unsigned short pc = hextbl[msg[1]];
        pc <<= 4;
        pc |= hextbl[msg[2]];
        pc <<= 4;
        pc |= hextbl[msg[3]];
        pc <<= 4;
        pc |= hextbl[msg[4]];
        if (!onceLog[pc]) {
            onceLog[pc] = true;
            ((MSX2*)arg)->putlog("[nest:%d] %s", nestLevel, msg);
        }
    });
}

static size_t serializePlaylog(void* buf)
{
    char* ptr = (char*)(buf ? buf : nullptr);
    unsigned int chunkSize = 0;
    size_t size = 0;
    if (playlog.disk1 && 0 < playlog.disk1Size) {
        chunkSize = (unsigned int)(1 + playlog.disk1Size);
        size += chunkSize + 6;
        if (ptr) {
            memcpy(ptr, "D1", 2);
            ptr += 2;
            memcpy(ptr, &chunkSize, 4);
            ptr += 4;
            *ptr = playlog.disk1ReadOnly ? 1 : 0;
            ptr += 1;
            memcpy(ptr, playlog.disk1, playlog.disk1Size);
            ptr += playlog.disk1Size;
        }
    }
    if (playlog.disk2 && 0 < playlog.disk2Size) {
        chunkSize = (unsigned int)(1 + playlog.disk2Size);
        size += chunkSize + 6;
        if (ptr) {
            memcpy(ptr, "D2", 2);
            ptr += 2;
            memcpy(ptr, &chunkSize, 4);
            ptr += 4;
            *ptr = playlog.disk2ReadOnly ? 1 : 0;
            ptr += 1;
            memcpy(ptr, playlog.disk2, playlog.disk2Size);
            ptr += playlog.disk2Size;
        }
    }
    if (playlog.rom && 0 < playlog.romSize) {
        chunkSize = (unsigned int)(4 + playlog.romSize);
        size += chunkSize + 6;
        if (ptr) {
            memcpy(ptr, "RO", 2);
            ptr += 2;
            memcpy(ptr, &chunkSize, 4);
            ptr += 4;
            memcpy(ptr, &playlog.romType, 4);
            ptr += 4;
            memcpy(ptr, playlog.rom, playlog.romSize);
            ptr += playlog.romSize;
        }
    }
    if (playlog.save && 0 < playlog.saveSize) {
        chunkSize = (unsigned int)playlog.saveSize;
        size += chunkSize + 6;
        if (ptr) {
            memcpy(ptr, "SV", 2);
            ptr += 2;
            memcpy(ptr, &chunkSize, 4);
            ptr += 4;
            memcpy(ptr, playlog.save, playlog.saveSize);
            ptr += playlog.saveSize;
        }
    }
    if (0 < playlog.tickCount) {
        chunkSize = (unsigned int)playlog.tickCount;
        size += (chunkSize + 6) * 3;
        if (ptr) {
            memcpy(ptr, "T1", 2);
            ptr += 2;
            memcpy(ptr, &chunkSize, 4);
            ptr += 4;
            memcpy(ptr, playlog.tickPad1, playlog.tickCount);
            ptr += playlog.tickCount;
        }
        if (ptr) {
            memcpy(ptr, "T2", 2);
            ptr += 2;
            memcpy(ptr, &chunkSize, 4);
            ptr += 4;
            memcpy(ptr, playlog.tickPad2, playlog.tickCount);
            ptr += playlog.tickCount;
        }
        if (ptr) {
            memcpy(ptr, "TK", 2);
            ptr += 2;
            memcpy(ptr, &chunkSize, 4);
            ptr += 4;
            memcpy(ptr, playlog.tickKey, playlog.tickCount);
            ptr += playlog.tickCount;
        }
    }
    return size;
}

void emu_startRecording(void)
{
    if (playlog.save) free(playlog.save);
    memset(&playlog, 0, sizeof(playlog));
    if (msx2.fdc) {
        playlog.disk1 = msx2.fdc->getDriveData(0, &playlog.disk1Size, &playlog.disk1ReadOnly);
        playlog.disk2 = msx2.fdc->getDriveData(0, &playlog.disk2Size, &playlog.disk2ReadOnly);
    }
    playlog.rom = msx2.mmu->cartridge.ptr;
    playlog.romSize = msx2.mmu->cartridge.size;
    playlog.romType = msx2.mmu->cartridge.romType;
    auto saveTmp = msx2.quickSave(&playlog.saveSize);
    playlog.save = malloc(playlog.saveSize);
    memcpy(playlog.save, saveTmp, playlog.saveSize);
    playlog.isRecording = true;
}

void* emu_stopPlaylog(size_t* size)
{
    size_t orgSize = serializePlaylog(nullptr);
    void* resultUncompressed = malloc(orgSize);
    serializePlaylog(resultUncompressed);
    if (playlog.save) {
        free(playlog.save);
        playlog.save = nullptr;
    }
    void* resultCompressed = malloc(orgSize * 2);
    *size = LZ4_compress_default((const char*)resultUncompressed,
                                 (char*)resultCompressed,
                                 (int)orgSize,
                                 (int)(orgSize * 2));
    free(resultUncompressed);
    playlog.isRecording = false;
    return resultCompressed;
}

void emu_startReplay(const void* data, size_t size)
{
    if (playlog.save) free(playlog.save);
    memset(&playlog, 0, sizeof(playlog));
    replayBuffer = malloc(1024 * 1024 * 16);
    char* ptr = (char*)replayBuffer;
    int dsize = LZ4_decompress_safe((const char*)data,
                                    ptr,
                                    (int)size,
                                    1024 * 1024 * 16);
    while (0 < dsize) {
        unsigned int uiSize;
        if (0 == memcmp(ptr, "D1", 2)) {
            ptr += 2;
            dsize -= 2;
            memcpy(&uiSize, ptr, 4);
            uiSize -= 1;
            ptr += 4;
            dsize -= 4;
            playlog.disk1Size = (size_t)uiSize;
            playlog.disk1ReadOnly = (*ptr) ? true : false;
            ptr++;
            dsize--;
            playlog.disk1 = ptr;
            ptr += uiSize;
            dsize -= uiSize;
        } else if (0 == memcmp(ptr, "D2", 2)) {
            ptr += 2;
            dsize -= 2;
            memcpy(&uiSize, ptr, 4);
            uiSize -= 1;
            ptr += 4;
            dsize -= 4;
            playlog.disk2Size = (size_t)uiSize;
            playlog.disk2ReadOnly = (*ptr) ? true : false;
            ptr++;
            dsize--;
            playlog.disk2 = ptr;
            ptr += uiSize;
            dsize -= uiSize;
        } else if (0 == memcmp(ptr, "RO", 2)) {
            ptr += 2;
            dsize -= 2;
            memcpy(&uiSize, ptr, 4);
            uiSize -= 4;
            ptr += 4;
            dsize -= 4;
            playlog.romSize = (size_t)uiSize;
            memcpy(&playlog.romType, ptr, 4);
            ptr += 4;
            dsize -= 4;
            playlog.rom = ptr;
            ptr += uiSize;
            dsize -= uiSize;
        } else if (0 == memcmp(ptr, "SV", 2)) {
            ptr += 2;
            dsize -= 2;
            memcpy(&uiSize, ptr, 4);
            ptr += 4;
            dsize -= 4;
            playlog.saveSize = (size_t)uiSize;
            playlog.save = ptr;
            ptr += uiSize;
            dsize -= uiSize;
        } else if (0 == memcmp(ptr, "T1", 2)) {
            ptr += 2;
            dsize -= 2;
            memcpy(&uiSize, ptr, 4);
            ptr += 4;
            dsize -= 4;
            playlog.tickCount = (int)uiSize;
            memcpy(playlog.tickPad1, ptr, playlog.tickCount);
            ptr += uiSize;
            dsize -= uiSize;
        } else if (0 == memcmp(ptr, "T2", 2)) {
            ptr += 2;
            dsize -= 2;
            memcpy(&uiSize, ptr, 4);
            ptr += 4;
            dsize -= 4;
            playlog.tickCount = (int)uiSize;
            memcpy(playlog.tickPad2, ptr, playlog.tickCount);
            ptr += uiSize;
            dsize -= uiSize;
        } else if (0 == memcmp(ptr, "TK", 2)) {
            ptr += 2;
            dsize -= 2;
            memcpy(&uiSize, ptr, 4);
            ptr += 4;
            dsize -= 4;
            playlog.tickCount = (int)uiSize;
            memcpy(playlog.tickKey, ptr, playlog.tickCount);
            ptr += uiSize;
            dsize -= uiSize;
        }
    }
    msx2.reset();
    if (playlog.rom) {
        msx2.loadRom((void*)playlog.rom, (int)playlog.romSize, playlog.romType);
        msx2.ejectDisk(0);
        msx2.ejectDisk(1);
    } else {
        msx2.ejectRom();
        if (playlog.disk1) {
            msx2.insertDisk(0, playlog.disk1, playlog.disk1Size, playlog.disk1ReadOnly);
        }
        if (playlog.disk2) {
            msx2.insertDisk(0, playlog.disk2, playlog.disk2Size, playlog.disk2ReadOnly);
        }
    }
    if (playlog.save) {
        msx2.quickLoad(playlog.save, playlog.saveSize);
        playlog.save = nullptr;
        playlog.saveSize = 0;
    }
    replayTickPtr = 0;
    playlog.isReplay = true;
}

void emu_stopReplay()
{
    if (replayBuffer) free(replayBuffer);
    playlog.isReplay = false;
}
