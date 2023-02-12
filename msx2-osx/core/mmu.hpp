#ifndef INCLUDE_MMU_HPP
#define INCLUDE_MMU_HPP

#include <stdio.h>
#include <string.h>
#include "msx2def.h"

//#define MMU_DEBUG_SHOW_PAGE_LAYOUT

class MMU
{
  public:
    // MSX slots are separated by 16KB, but MegaROMs are separated by 8KB or 16KB, so data blocks are managed by 8KB
    struct DataBlock8KB {
        char label[8];
        unsigned char* ptr;
        bool isRAM;
        bool isCartridge;
    };

    struct Slot {
        struct DataBlock8KB data[8];
    } slots[4][4];

    bool secondaryExist[4];

    struct Cartridge {
        unsigned char* ptr;
        size_t size;
        int romType;
    } cartridge;

    struct Context {
        unsigned char primary[4];
        unsigned char secondary[4];
        unsigned char segment[4];
        unsigned char cpos[2][4]; // cartridge position register (0x2000 * n)
        unsigned char reserved[12 + 32];
    } ctx;

    MMU() {
        memset(&this->slots, 0, sizeof(this->slots));
    }

    void setupSecondaryExist(bool page0, bool page1, bool page2, bool page3) {
        secondaryExist[0] = page0;
        secondaryExist[1] = page1;
        secondaryExist[2] = page2;
        secondaryExist[3] = page3;
    }

    void reset()
    {
        memset(&this->ctx, 0, sizeof(this->ctx));
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 4; j++) {
                this->ctx.cpos[i][j] = j;
            }
        }
#ifdef MMU_DEBUG_SHOW_PAGE_LAYOUT
        dumpPageLayout("reset", 0);
#endif
    }

    inline int primaryNumber(int page) { return this->ctx.primary[page & 0b11]; }
    inline int secondaryNumber(int page) { return this->ctx.secondary[page & 0b11]; }

    void setupCartridge(int pri, int sec, int idx, void* data, size_t size, int romType)
    {
        this->cartridge.ptr = (unsigned char*)data;
        this->cartridge.size = size;
        this->cartridge.romType = romType;
        setup(pri, sec, idx, false, this->cartridge.ptr, this->cartridge.size < 0x8000 ? 0x4000 : 0x8000, "CART");
        switch (romType) {
            case MSX2_ROM_TYPE_NORMAL:
                for (int i = 0; i < 4; i++) {
                    this->ctx.cpos[pri - 1][i] = i;
                }
                break;
            case MSX2_ROM_TYPE_ASC8:
            case MSX2_ROM_TYPE_ASC16:
                for (int i = 0; i < 4; i++) {
                    this->ctx.cpos[pri - 1][i] = 0;
                }
                break;
            default:
                printf("UNKNOWN ROM TYPE: %d\n", romType);
                exit(-1);
        }
    }

    void setup(int pri, int sec, int idx, bool isRAM, unsigned char* data, int size, const char* label)
    {
        if (label) {
            printf("Setup SLOT %d-%d $%04X~$%04X = %s\n", pri, sec, idx * 0x2000, idx * 0x2000 + size - 1, label);
        }
        do {
            memset(this->slots[pri][sec].data[idx].label, 0, sizeof(this->slots[pri][sec].data[idx].label));
            if (label) {
                strncpy(this->slots[pri][sec].data[idx].label, label, 4);
            }
            this->slots[pri][sec].data[idx].isRAM = isRAM;
            this->slots[pri][sec].data[idx].isCartridge = NULL != label && 0 == strcmp(label, "CART");
            if (!this->slots[pri][sec].data[idx].isCartridge) {
                this->slots[pri][sec].data[idx].ptr = data;
            }
            size -= 0x2000;
            data += 0x2000;
            idx++;
        } while (0 < size);
        this->bankSwitchover();
    }

    inline void bankSwitchover() {
        for (int i = 1; i < 3; i++) {
            for (int j = 2; j < 6; j += 2) {
                if (this->slots[i][0].data[j].isCartridge) {
                    for (int k = 0; k < 2; k++) {
                        //printf("this->slots[%d][0].data[%d].ptr=ROM[%05X]\n",i,j+k,this->ctx.cpos[i - 1][j - 2 + k] * 0x2000);
                        this->slots[i][0].data[j + k].ptr = &this->cartridge.ptr[this->ctx.cpos[i - 1][j - 2 + k] * 0x2000];
                    }
                }
            }
        }
    }

    inline unsigned char getPrimary()
    {
        return ((this->ctx.primary[3] << 6) |
                (this->ctx.primary[2] << 4) |
                (this->ctx.primary[1] << 2) |
                this->ctx.primary[0]);
    }

#ifdef MMU_DEBUG_SHOW_PAGE_LAYOUT
    inline void dumpPageLayout(const char* msg, unsigned char value) {
        auto pri = getPrimary();
        auto sec = getSecondary() ^ 0xFF;
        int p[4];
        int s[4];
        for (int i = 0; i < 4; i++) {
            p[i] = pri & 0b11;
            s[i] = sec & 0b11;
            pri >>= 2;
            sec >>= 2;
        }
        auto page0 = slots[p[0]][s[0]];
        auto page1 = slots[p[1]][s[1]];
        auto page2 = slots[p[2]][s[2]];
        auto page3 = slots[p[3]][s[3]];
        printf("Pages #0:%d-%d(%s), #1:%d-%d(%s), #2:%d-%d(%s), #3:%d-%d(%s) <%s:$%02X>\n"
               , p[0], s[0], page0.data[0].label[0] ? page0.data[0].label : "n/a"
               , p[1], s[1], page1.data[2].label[0] ? page1.data[2].label : "n/a"
               , p[2], s[2], page2.data[4].label[0] ? page2.data[4].label : "n/a"
               , p[3], s[3], page3.data[6].label[0] ? page3.data[6].label : "n/a"
               , msg, value);
    }
#endif

    inline void updatePrimary(unsigned char value)
    {
#ifdef MMU_DEBUG_SHOW_PAGE_LAYOUT
        unsigned char value_ = value;
        unsigned char prev[4];
        memcpy(prev, this->ctx.primary, 4);
#endif
        for (int i = 0; i < 4; i++) {
            this->ctx.primary[i] = value & 0b11;
            value >>= 2;
        }
#ifdef MMU_DEBUG_SHOW_PAGE_LAYOUT
        if (prev[0] != this->ctx.primary[0] ||
            prev[1] != this->ctx.primary[1] ||
            prev[2] != this->ctx.primary[2] ||
            prev[3] != this->ctx.primary[3]) {
            dumpPageLayout("updatePrimary", value_);
        }
#endif
    }

    inline void updateSegment(int page, unsigned char value)
    {
        printf("update segment: page %d = %d\n", page, value);
        this->ctx.segment[page] = value;
    }

    inline unsigned char getSecondary()
    {
        unsigned char sec[4];
        for (int i = 0; i < 4; i++) {
            if (this->secondaryExist[this->ctx.primary[i]]) {
                sec[i] = this->ctx.secondary[i];
            } else {
                sec[i] = 0;
            }
        }
        return 0xFF ^ ((sec[3] << 6) | (sec[2] << 4) | (sec[1] << 2) | sec[0]);
    }

    inline void updateSecondary(unsigned char value)
    {
#ifdef MMU_DEBUG_SHOW_PAGE_LAYOUT
        unsigned char value_ = value;
        unsigned char prev[4];
        memcpy(prev, this->ctx.secondary, 4);
#endif
        for (int i = 0; i < 4; i++) {
            this->ctx.secondary[i] = value & 0b11;
            value >>= 2;
        }
#ifdef MMU_DEBUG_SHOW_PAGE_LAYOUT
        if (prev[0] != this->ctx.secondary[0] ||
            prev[1] != this->ctx.secondary[1] ||
            prev[2] != this->ctx.secondary[2] ||
            prev[3] != this->ctx.secondary[3]) {
            dumpPageLayout("updateSecondary", value_);
        }
#endif
    }

    inline unsigned char read(unsigned short addr)
    {
        if (addr == 0xFFFF) {
            return this->getSecondary();
        }
        int page = (addr & 0b1100000000000000) >> 14;
        int pri = this->ctx.primary[page];
        int sec = this->secondaryExist[pri] ? this->ctx.secondary[page] : 0;
        int idx = addr / 0x2000;
        auto s = &this->slots[pri][sec];
        auto ptr = s->data[idx].ptr;
        return ptr ? ptr[addr & 0x1FFF] : 0xFF;
    }

    inline void write(unsigned short addr, unsigned char value)
    {
        if (addr == 0xFFFF) {
            this->updateSecondary(value);
            return;
        }
        int page = (addr & 0b1100000000000000) >> 14;
        int pri = this->ctx.primary[page];
        int sec = this->secondaryExist[pri] ? this->ctx.secondary[page] : 0;
        int idx = addr / 0x2000;
        auto s = &this->slots[pri][sec];
        auto data = &s->data[idx];
        if (data->isRAM && data->ptr) {
            data->ptr[addr & 0x1FFF] = value;
        } else if (data->isCartridge) {
            switch (this->cartridge.romType) {
                case MSX2_ROM_TYPE_NORMAL: return;
                case MSX2_ROM_TYPE_ASC8: this->asc8(pri - 1, addr, value); return;
                case MSX2_ROM_TYPE_ASC16: this->asc16(pri - 1, addr, value); return;
            }
            puts("DETECT ROM WRITE");
            exit(-1);
        }
    }

    inline void asc8(int idx, unsigned short addr, unsigned char value) {
        switch (addr & 0x7800) {
            case 0x6000: this->ctx.cpos[idx][0] = value; break;
            case 0x6800: this->ctx.cpos[idx][1] = value; break;
            case 0x7000: this->ctx.cpos[idx][2] = value; break;
            case 0x7800: this->ctx.cpos[idx][3] = value; break;
        }
        this->bankSwitchover();
    }

    inline void asc16(int idx, unsigned short addr, unsigned char value) {
        if (0x6000 <= addr && addr < 0x6800) {
            this->ctx.cpos[idx][0] = value * 2;
            this->ctx.cpos[idx][1] = value * 2 + 1;
            this->bankSwitchover();
        } else if (0x7000 <= addr && addr < 0x7800) {
            this->ctx.cpos[idx][2] = value * 2;
            this->ctx.cpos[idx][3] = value * 2 + 1;
            this->bankSwitchover();
        }
    }
};

#endif // INCLUDE_MMU_HPP
