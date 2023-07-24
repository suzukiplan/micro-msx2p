/**
 * micro MSX2+ - Memory Management Unit (MSX-SLOT) for MSX1
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
#ifndef INCLUDE_MSX1MMU_HPP
#define INCLUDE_MSX1MMU_HPP

#include "msx1def.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class MSX1MMU
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
    } slots[4];

    struct Cartridge {
        unsigned char* ptr;
        size_t size;
        int romType;
    } cartridge;

    struct Context {
        unsigned char pri[4];
        unsigned char reserved[4];
        unsigned char cpos[2][4]; // cartridge position register (0x2000 * n)
        unsigned char isSelectSRAM[8];
    } ctx;

    bool sramEnabled;
    unsigned char* sram;
    unsigned char* ram;
    size_t sramSize;
    size_t ramSize;
    unsigned char empty[0x2000];

    MSX1MMU()
    {
        memset(this->empty, 0xFF, sizeof(this->empty));
        memset(&this->slots, 0, sizeof(this->slots));
        for (int i = 0; i < 4; i++) this->setupEmpty(i);
        this->sramEnabled = false;
        this->sram = nullptr;
        this->sramSize = 0;
    }

    ~MSX1MMU()
    {
        if (this->sram) {
            free(this->sram);
        }
    }

    void setupEmpty(int pri)
    {
        for (int idx = 0; idx < 8; idx++) {
            this->setupEmpty(pri, idx);
        }
    }

    void setupEmpty(int pri, int idx)
    {
        strcpy(slots[pri].data[idx].label, "(empty)");
        slots[pri].data[idx].ptr = empty;
        slots[pri].data[idx].isRAM = false;
        slots[pri].data[idx].isCartridge = false;
    }

    void setupRAM(unsigned char* ram, size_t ramSize)
    {
        this->ram = ram;
        this->ramSize = ramSize;
        int si = 0;
        switch (ramSize) {
            case 0x2000: si = 6; break;
            case 0x4000: si = 6; break;
            case 0x8000: si = 4; break;
            case 0x10000: si = 0; break;
            default: exit(-1); // invalid RAM size
        }
        for (int i = si; i < 8; i++) {
            strcpy(this->slots[3].data[i].label, "RAM");
            this->slots[3].data[i].isRAM = true;
            this->slots[3].data[i].isCartridge = false;
            this->slots[3].data[i].ptr = &this->ram[(i * 0x2000) & (this->ramSize - 1)];
        }
    }

    void reset()
    {
        memset(&this->ctx, 0, sizeof(this->ctx));
        memset(this->ram, 0, this->ramSize);
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 4; j++) {
                this->ctx.cpos[i][j] = j;
            }
        }
    }

    void clearCartridge()
    {
        this->cartridge.ptr = nullptr;
        this->cartridge.size = 0;
        this->cartridge.romType = 0;
        memset(this->ctx.cpos, 0, sizeof(this->ctx.cpos));
        for (int pri = 1; pri <= 2; pri++) {
            this->setupEmpty(pri);
        }
    }

    void setupCartridge(int pri, int idx, void* data, size_t size, int romType)
    {
        this->cartridge.ptr = (unsigned char*)data;
        this->cartridge.size = size;
        this->cartridge.romType = romType;
        setup(pri, idx, this->cartridge.ptr, this->cartridge.size < 0x8000 ? 0x4000 : 0x8000, "CART");
        this->sramEnabled = false;
        switch (romType) {
            case MSX1_ROM_TYPE_NORMAL:
                if (size == 0x4000) {
                    setup(pri, idx + 2, this->cartridge.ptr, 0x4000, "CART/M");
                    for (int i = 0; i < 4; i++) {
                        this->ctx.cpos[pri - 1][i] = i & 1;
                    }
                } else {
                    for (int i = 0; i < 4; i++) {
                        this->ctx.cpos[pri - 1][i] = i;
                    }
                }
                break;
            case MSX1_ROM_TYPE_KONAMI:
                for (int i = 0; i < 4; i++) {
                    this->ctx.cpos[pri - 1][i] = i;
                }
                break;
            case MSX1_ROM_TYPE_ASC8:
            case MSX1_ROM_TYPE_ASC16:
                for (int i = 0; i < 4; i++) {
                    this->ctx.cpos[pri - 1][i] = 0;
                }
                break;
            case MSX1_ROM_TYPE_ASC8_SRAM2:
            case MSX1_ROM_TYPE_ASC16_SRAM2:
                for (int i = 0; i < 4; i++) {
                    this->ctx.cpos[pri - 1][i] = 0;
                }
                this->sramEnabled = true;
                if (!this->sram) {
                    this->sram = (unsigned char*)malloc(0x2000);
                    this->sramSize = 0x2000;
                }
                memset(this->sram, 0, this->sramSize);
                break;
            default:
                printf("UNKNOWN ROM TYPE: %d\n", romType);
                exit(-1);
        }
    }

    void setup(int pri, int idx, unsigned char* data, int size, const char* label)
    {
        do {
            memset(this->slots[pri].data[idx].label, 0, sizeof(this->slots[pri].data[idx].label));
            if (label) {
                strncpy(this->slots[pri].data[idx].label, label, 4);
            }
            this->slots[pri].data[idx].isRAM = false;
            this->slots[pri].data[idx].isCartridge = NULL != label && 0 == strcmp(label, "CART");
            if (!this->slots[pri].data[idx].isCartridge) {
                this->slots[pri].data[idx].ptr = data;
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
                if (this->slots[i].data[j].isCartridge) {
                    for (int k = 0; k < 2; k++) {
                        if (this->ctx.isSelectSRAM[j + k]) {
                            this->slots[i].data[j + k].ptr = sram;
                            this->slots[i].data[j + k].isRAM = true;
                        } else {
                            this->slots[i].data[j + k].ptr = &this->cartridge.ptr[this->ctx.cpos[i - 1][j - 2 + k] * 0x2000];
                            this->slots[i].data[j + k].isRAM = false;
                        }
                    }
                }
            }
        }
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
            this->ctx.pri[page] = pri;
            value >>= 2;
        }
    }

    inline struct DataBlock8KB* getDataBlock(unsigned short addr)
    {
        return &this->slots[this->ctx.pri[(addr & 0b1100000000000000) >> 14]].data[addr / 0x2000];
    }

    inline unsigned char read(unsigned short addr)
    {
        return this->getDataBlock(addr)->ptr[addr & 0x1FFF];
    }

    inline void write(unsigned short addr, unsigned char value)
    {
        auto pri = this->ctx.pri[(addr & 0b1100000000000000) >> 14];
        auto data = this->getDataBlock(addr);
        if (data->isRAM) {
            data->ptr[addr & 0x1FFF] = value;
        } else if (data->isCartridge) {
            switch (this->cartridge.romType) {
                case MSX1_ROM_TYPE_NORMAL: return;
                case MSX1_ROM_TYPE_ASC8: this->asc8(pri - 1, addr, value); return;
                case MSX1_ROM_TYPE_ASC8_SRAM2: this->asc8sram2(pri - 1, addr, value); return;
                case MSX1_ROM_TYPE_ASC16: this->asc16(pri - 1, addr, value); return;
                case MSX1_ROM_TYPE_ASC16_SRAM2: this->asc16sram2(pri - 1, addr, value); return;
                case MSX1_ROM_TYPE_KONAMI: this->konami(pri - 1, addr, value); return;
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
        this->ctx.isSelectSRAM[5] = this->ctx.isSelectSRAM[4];
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
