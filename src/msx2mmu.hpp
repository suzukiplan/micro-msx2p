/**
 * micro MSX2+ - Memory Management Unit (MSX-SLOT)
 * -----------------------------------------------------------------------------
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Yoji Suzuki.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -----------------------------------------------------------------------------
 */
#ifndef INCLUDE_MSX2MMU_HPP
#define INCLUDE_MSX2MMU_HPP

#include "msx2def.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class MSX2MMU
{
  public:
    // MSX slots are separated by 16KB, but MegaROMs are separated by 8KB or 16KB, so data blocks are managed by 8KB
    struct DataBlock8KB {
        char label[8];
        unsigned char* ptr;
        bool isRAM;
        bool isCartridge;
        bool isDiskBios;
        bool isFmBios;
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

    struct Callback {
        void* arg;
        unsigned char (*sccRead)(void* arg, unsigned short addr);
        void (*sccWrite)(void* arg, unsigned short addr, unsigned char value);
        unsigned char (*diskRead)(void* arg, unsigned short addr);
        void (*diskWrite)(void* arg, unsigned short addr, unsigned char value);
        void (*fmWrite)(void* arg, unsigned short addr, unsigned char value);
    } CB;

    struct Context {
        struct PrimarySlotState {
            unsigned char pri;
            unsigned char sec;
            unsigned char reg;
            unsigned char reserved;
        } pslot[4];
        unsigned char pri[4];
        unsigned char sec[4];
        unsigned char mmap[4];
        unsigned char reserved[4];
        unsigned char cpos[2][4]; // cartridge position register (0x2000 * n)
        unsigned char isSelectSRAM[8];
    } ctx;

    bool sccEnabled;
    bool sramEnabled;
    unsigned char sram[0x2000];
    unsigned char pac[0x2000];
    unsigned char ram[0x10000];

    MSX2MMU()
    {
        memset(&this->slots, 0, sizeof(this->slots));
        memset(&this->secondaryExist, 0, sizeof(this->secondaryExist));
        memset(this->sram, 0, sizeof(this->sram));
        memset(this->pac, 0, sizeof(this->pac));
        this->sramEnabled = false;
    }

    void setupCallbacks(void* arg,
                        unsigned char (*sccRead)(void* arg, unsigned short addr),
                        void (*sccWrite)(void* arg, unsigned short addr, unsigned char value),
                        unsigned char (*diskRead)(void* arg, unsigned short addr),
                        void (*diskWrite)(void* arg, unsigned short addr, unsigned char value),
                        void (*fmWrite)(void* arg, unsigned short addr, unsigned char value))
    {
        this->CB.arg = arg;
        this->CB.sccRead = sccRead;
        this->CB.sccWrite = sccWrite;
        this->CB.diskRead = diskRead;
        this->CB.diskWrite = diskWrite;
        this->CB.fmWrite = fmWrite;
    }

    void setupSecondaryExist(bool page0, bool page1, bool page2, bool page3)
    {
        secondaryExist[0] = page0;
        secondaryExist[1] = page1;
        secondaryExist[2] = page2;
        secondaryExist[3] = page3;
    }

    void reset()
    {
        memset(&this->ctx, 0, sizeof(this->ctx));
        memset(this->ram, 0, sizeof(this->ram));
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 4; j++) {
                this->ctx.cpos[i][j] = j;
            }
        }
        this->ctx.mmap[0] = 3;
        this->ctx.mmap[1] = 2;
        this->ctx.mmap[2] = 1;
        this->ctx.mmap[3] = 0;
    }

    void clearCartridge()
    {
        this->cartridge.ptr = nullptr;
        this->cartridge.size = 0;
        this->cartridge.romType = 0;
        memset(this->ctx.cpos, 0, sizeof(this->ctx.cpos));
        this->sccEnabled = false;
        for (int pri = 1; pri <= 2; pri++) {
            memset(&this->slots[pri][0], 0, sizeof(Slot));
        }
    }

    void setupCartridge(int pri, int sec, int idx, void* data, size_t size, int romType)
    {
        this->cartridge.ptr = (unsigned char*)data;
        this->cartridge.size = size;
        this->cartridge.romType = romType;
        setup(pri, sec, idx, this->cartridge.ptr, this->cartridge.size < 0x8000 ? 0x4000 : 0x8000, "CART");
        this->sramEnabled = false;
        this->sccEnabled = false;
        switch (romType) {
            case MSX2_ROM_TYPE_NORMAL:
                if (size == 0x4000) {
                    setup(pri, sec, idx + 2, this->cartridge.ptr, 0x4000, "CART/M");
                    for (int i = 0; i < 4; i++) {
                        this->ctx.cpos[pri - 1][i] = i & 1;
                    }
                } else {
                    for (int i = 0; i < 4; i++) {
                        this->ctx.cpos[pri - 1][i] = i;
                    }
                }
                break;
            case MSX2_ROM_TYPE_KONAMI:
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
            case MSX2_ROM_TYPE_KONAMI_SCC:
                for (int i = 0; i < 4; i++) {
                    this->ctx.cpos[pri - 1][i] = 0;
                }
                this->sccEnabled = true;
                break;
            case MSX2_ROM_TYPE_ASC8_SRAM2:
            case MSX2_ROM_TYPE_ASC16_SRAM2:
                for (int i = 0; i < 4; i++) {
                    this->ctx.cpos[pri - 1][i] = 0;
                }
                this->sramEnabled = true;
                memset(this->sram, 0, sizeof(this->sram));
                break;
            default:
                printf("UNKNOWN ROM TYPE: %d\n", romType);
                exit(-1);
        }
    }

    void setupRAM(int pri, int sec)
    {
#ifdef DEBUG
        printf("Setup SLOT %d-%d $%04X~$%04X = %s\n", pri, sec, 0, 0xFFFF, "RAM");
#endif
        for (int i = 0; i < 8; i++) {
            strcpy(this->slots[pri][sec].data[i].label, "RAM");
            this->slots[pri][sec].data[i].isRAM = true;
            this->slots[pri][sec].data[i].isCartridge = false;
            this->slots[pri][sec].data[i].isDiskBios = false;
            this->slots[pri][sec].data[i].isFmBios = false;
            this->slots[pri][sec].data[i].ptr = &this->ram[i * 0x2000];
        }
    }

    void setup(int pri, int sec, int idx, unsigned char* data, int size, const char* label)
    {
#ifdef DEBUG
        if (label) {
            printf("Setup SLOT %d-%d $%04X~$%04X = %s\n", pri, sec, idx * 0x2000, idx * 0x2000 + size - 1, label);
        }
#endif
        do {
            memset(this->slots[pri][sec].data[idx].label, 0, sizeof(this->slots[pri][sec].data[idx].label));
            if (label) {
                strncpy(this->slots[pri][sec].data[idx].label, label, 4);
            }
            this->slots[pri][sec].data[idx].isRAM = false;
            this->slots[pri][sec].data[idx].isCartridge = NULL != label && 0 == strcmp(label, "CART");
            this->slots[pri][sec].data[idx].isDiskBios = NULL != label && 0 == strcmp(label, "DISK");
            this->slots[pri][sec].data[idx].isFmBios = NULL != label && 0 == strcmp(label, "FM");
            if (!this->slots[pri][sec].data[idx].isCartridge) {
                this->slots[pri][sec].data[idx].ptr = data;
            }
            size -= 0x2000;
            data += 0x2000;
            idx++;
        } while (0 < size);
        this->bankSwitchover();
    }

    inline void bankSwitchover()
    {
        for (int i = 1; i < 3; i++) {
            for (int j = 2; j < 6; j += 2) {
                if (this->slots[i][0].data[j].isCartridge) {
                    for (int k = 0; k < 2; k++) {
                        // printf("this->slots[%d][0].data[%d].ptr=ROM[%05X]\n",i,j+k,this->ctx.cpos[i - 1][j - 2 + k] * 0x2000);
                        if (this->ctx.isSelectSRAM[j + k]) {
                            this->slots[i][0].data[j + k].ptr = sram;
                            this->slots[i][0].data[j + k].isRAM = true;
                        } else {
                            this->slots[i][0].data[j + k].ptr = &this->cartridge.ptr[this->ctx.cpos[i - 1][j - 2 + k] * 0x2000];
                            this->slots[i][0].data[j + k].isRAM = false;
                        }
                    }
                }
            }
        }
    }

    inline void updateMemoryMapper(int page, unsigned char value)
    {
        // printf("update memory mapper: page %d = %d\n", page, value);
        this->ctx.mmap[page] = value;
    }

    inline unsigned char getPrimary()
    {
        return ((this->ctx.pri[3] << 6) |
                (this->ctx.pri[2] << 4) |
                (this->ctx.pri[1] << 2) |
                this->ctx.pri[0]);
    }

    inline void updatePrimary(unsigned char value)
    {
        for (int page = 0; page < 4; page++) {
            int pri = value & 0b11;
            this->ctx.pslot[page].pri = pri;
            this->ctx.pslot[page].sec = (this->ctx.pslot[pri].reg >> (page * 2)) & 0b11;
            int sec = this->secondaryExist[pri] ? this->ctx.pslot[page].sec : 0;
            this->ctx.pri[page] = pri;
            this->ctx.sec[page] = sec;
            value >>= 2;
        }
    }

    inline unsigned char getSecondary()
    {
        unsigned char pri3 = this->ctx.pslot[3].pri;
        if (this->secondaryExist[pri3]) {
            return ~this->ctx.pslot[pri3].reg;
        } else {
            return 0xFF;
        }
    }

    inline void updateSecondary(unsigned char value)
    {
        unsigned char pri = this->ctx.pslot[3].pri;
        if (this->secondaryExist[pri]) {
            this->ctx.pslot[pri].reg = value;
            for (int page = 0; page < 4; page++) {
                if (this->ctx.pslot[page].pri == pri) {
                    unsigned char sec = value & 0b11;
                    this->ctx.pslot[page].sec = sec;
                    this->ctx.pri[page] = pri;
                    this->ctx.sec[page] = sec;
                }
                value >>= 2;
            }
        }
    }

    struct DataBlock8KB* getDataBlock(unsigned short addr)
    {
        int page = (addr & 0b1100000000000000) >> 14;
        int pri = this->ctx.pri[page];
        int sec = this->ctx.sec[page];
        int idx = addr / 0x2000;
        auto s = &this->slots[pri][sec];
        return &s->data[idx];
    }

    inline unsigned char read(unsigned short addr)
    {
        if (addr == 0xFFFF) {
            return this->getSecondary();
        }
        int page = (addr & 0b1100000000000000) >> 14;
        int pri = this->ctx.pri[page];
        int sec = this->ctx.sec[page];
        int idx = addr / 0x2000;
        auto s = &this->slots[pri][sec];
        auto ptr = s->data[idx].ptr;
        if (ptr) {
            if (s->data[idx].isDiskBios) {
                if (0x3FF0 <= (addr & 0x3FFF)) {
                    return CB.diskRead(CB.arg, addr & 0x3FFF);
                }
            } else if (s->data[idx].isFmBios && this->isEnabledPacSRAM()) {
                if ((addr & 0x3FFF) < 0x1FFE) {
                    return this->pac[addr & 0x1FFF];
                }
            }
        }
        return ptr ? ptr[addr & 0x1FFF] : 0xFF;
    }

    inline bool isEnabledPacSRAM()
    {
        return this->pac[0x1FFE] == 0x4D && this->pac[0x1FFF] == 0x69;
    }

    inline void write(unsigned short addr, unsigned char value)
    {
        if (addr == 0xFFFF) {
            this->updateSecondary(value);
            return;
        }
        int page = (addr & 0b1100000000000000) >> 14;
        int pri = this->ctx.pri[page];
        int sec = this->ctx.sec[page];
        int idx = addr / 0x2000;
        auto s = &this->slots[pri][sec];
        auto data = &s->data[idx];
        if (data->isRAM && data->ptr) {
            data->ptr[addr & 0x1FFF] = value;
        } else if (data->isDiskBios) {
            CB.diskWrite(CB.arg, addr & 0x3FFF, value);
        } else if (data->isFmBios) {
            switch (addr & 0x3FFF) {
                case 0x1FFE: this->pac[0x1FFE] = value; break;
                case 0x1FFF: this->pac[0x1FFF] = value; break;
                default:
                    if (this->isEnabledPacSRAM() && (addr & 0x3FFF) < 0x2000) {
                        this->pac[addr & 0x1FFF] = value;
                    } else {
                        this->CB.fmWrite(CB.arg, addr & 0x3FFF, value);
                    }
            }
        } else if (data->isCartridge) {
            switch (this->cartridge.romType) {
                case MSX2_ROM_TYPE_NORMAL: return;
                case MSX2_ROM_TYPE_ASC8: this->asc8(pri - 1, addr, value); return;
                case MSX2_ROM_TYPE_ASC8_SRAM2: this->asc8sram2(pri - 1, addr, value); return;
                case MSX2_ROM_TYPE_ASC16: this->asc16(pri - 1, addr, value); return;
                case MSX2_ROM_TYPE_ASC16_SRAM2: this->asc16sram2(pri - 1, addr, value); return;
                case MSX2_ROM_TYPE_KONAMI_SCC: this->konamiSCC(pri - 1, addr, value); return;
                case MSX2_ROM_TYPE_KONAMI: this->konami(pri - 1, addr, value); return;
            }
            puts("DETECT ROM WRITE");
            exit(-1);
        }
    }

    inline void asc8(int idx, unsigned short addr, unsigned char value)
    {
        switch (addr & 0x7800) {
            case 0x6000: this->ctx.cpos[idx][0] = value; break;
            case 0x6800: this->ctx.cpos[idx][1] = value; break;
            case 0x7000: this->ctx.cpos[idx][2] = value; break;
            case 0x7800: this->ctx.cpos[idx][3] = value; break;
        }
        this->bankSwitchover();
    }

    inline void asc8sram2(int idx, unsigned short addr, unsigned char value)
    {
        this->ctx.isSelectSRAM[4] = value & 0b11110000 ? 1 : 0;
        value &= 0b00001111;
        this->asc8(idx, addr, value);
    }

    inline void asc16(int idx, unsigned short addr, unsigned char value)
    {
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

    inline void asc16sram2(int idx, unsigned short addr, unsigned char value)
    {
        this->ctx.isSelectSRAM[4] = value & 0b00010000 ? 1 : 0;
        value &= 0b00001111;
        this->asc16(idx, addr, value);
    }

    inline void konamiSCC(int idx, unsigned short addr, unsigned char value)
    {
        switch (addr & 0xF000) {
            case 0x5000: this->ctx.cpos[idx][0] = value; break;
            case 0x7000: this->ctx.cpos[idx][1] = value; break;
            case 0xB000: this->ctx.cpos[idx][3] = value; break;
            case 0x9000:
                if (addr < 0x9800) {
                    this->ctx.cpos[idx][2] = value;
                } else {
                    this->CB.sccWrite(this->CB.arg, addr, value);
                    return;
                }
                break;
        }
        this->bankSwitchover();
    }

    inline void konami(int idx, unsigned short addr, unsigned char value)
    {
        switch (addr & 0xF000) {
            case 0x6000: this->ctx.cpos[idx][1] = value; break;
            case 0x7000: this->ctx.cpos[idx][1] = value; break;
            case 0x8000: this->ctx.cpos[idx][2] = value; break;
            case 0x9000: this->ctx.cpos[idx][2] = value; break;
            case 0xA000: this->ctx.cpos[idx][3] = value; break;
            case 0xB000: this->ctx.cpos[idx][3] = value; break;
        }
        this->bankSwitchover();
    }
};

#endif // INCLUDE_MMU_HPP
