#ifndef INCLUDE_MMU_HPP
#define INCLUDE_MMU_HPP

#include <stdio.h>
#include <string.h>

class MMU
{
  private:
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

    // TODO: Memorize cartridge addresses in case of MegaROMs support (need keep megarom type and switch bank routine at the write method)
    struct Cartridge {
        unsigned char* ptr;
        size_t size;
    } cartridge;

public:
    struct Context {
        unsigned char primary[4];
        unsigned char secondary[4];
        unsigned char segment[4];
        unsigned char reserved[4];
    } ctx;

    MMU() {
        memset(&this->slots, 0, sizeof(this->slots));
    }

    void reset()
    {
        memset(&this->ctx, 0, sizeof(this->ctx));
    }

    inline int primaryNumber(int page) { return this->ctx.primary[page & 0b11]; }
    inline int secondaryNumber(int page) { return this->ctx.secondary[page & 0b11]; }

    void setupCartridge(int pri, int sec, int idx, void* data, size_t size)
    {
        this->cartridge.ptr = (unsigned char*)data;
        this->cartridge.size = size;
        setup(pri, sec, idx, false, this->cartridge.ptr, this->cartridge.size < 0x8000 ? 0x4000 : 0x8000, "CART");
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
            this->slots[pri][sec].data[idx].ptr = data;
            this->slots[pri][sec].data[idx].isRAM = isRAM;
            size -= 0x2000;
            data += 0x2000;
            idx++;
        } while (0 < size);
    }

    inline unsigned char getPrimary()
    {
        unsigned char result = 0;
        for (int i = 0; i < 4; i++) {
            result <<= 2;
            result += this->ctx.primary[3 - i] & 0b11;
        }
        return result;
    }

    inline void dumpPageLayout(const char* msg) {
#if 0
        auto page0 = slots[ctx.primary[0]][ctx.secondary[0]];
        auto page1 = slots[ctx.primary[1]][ctx.secondary[1]];
        auto page2 = slots[ctx.primary[2]][ctx.secondary[2]];
        auto page3 = slots[ctx.primary[3]][ctx.secondary[3]];
        printf("Pages #0:%d-%d(%s), #1:%d-%d(%s), #2:%d-%d(%s), #3:%d-%d(%s) <%s>\n"
               , ctx.primary[0], ctx.secondary[0]
               , page0.data[0].label[0] ? page0.data[0].label : "n/a"
               , ctx.primary[1], ctx.secondary[1]
               , page1.data[2].label[0] ? page1.data[2].label : "n/a"
               , ctx.primary[2], ctx.secondary[2]
               , page2.data[4].label[0] ? page2.data[4].label : "n/a"
               , ctx.primary[3], ctx.secondary[3]
               , page3.data[6].label[0] ? page3.data[6].label : "n/a"
               , msg);
#endif
    }

    inline void updatePrimary(unsigned char value)
    {
#if 1
        unsigned char prev[4];
        memcpy(prev, this->ctx.primary, 4);
#endif
        for (int i = 0; i < 4; i++) {
            this->ctx.primary[i] = value & 0b11;
            value >>= 2;
        }
#if 1
        if (prev[0] != this->ctx.primary[0] ||
            prev[1] != this->ctx.primary[1] ||
            prev[2] != this->ctx.primary[2] ||
            prev[3] != this->ctx.primary[3]) {
            dumpPageLayout("updatePrimary");
        }
#endif
    }

    inline void updateSegment(int page, unsigned char value)
    {
        printf("update segment: page %d = %d\n", page, value);
        this->ctx.segment[page] = value; // TODO: segment not supported
    }

    inline unsigned char getSecondary()
    {
        unsigned char result = 0;
        for (int i = 0; i < 4; i++) {
            result <<= 2;
            result += this->ctx.secondary[3 - i] & 0b11;
        }
        return result;
    }

    inline void updateSecondary(unsigned char value)
    {
#if 1
        unsigned char prev[4];
        memcpy(prev, this->ctx.secondary, 4);
#endif
        for (int i = 0; i < 4; i++) {
            this->ctx.secondary[i] = value & 0b11;
            value >>= 2;
        }
#if 1
        if (prev[0] != this->ctx.secondary[0] ||
            prev[1] != this->ctx.secondary[1] ||
            prev[2] != this->ctx.secondary[2] ||
            prev[3] != this->ctx.secondary[3]) {
            dumpPageLayout("updateSecondary");
        }
#endif
    }

    inline unsigned char read(unsigned short addr)
    {
        if (addr == 0xFFFF) return this->getSecondary();
        int page = (addr & 0b1100000000000000) >> 14;
        int pri = this->ctx.primary[page];
        int sec = 3 == pri ? this->ctx.secondary[page] : 0;
        int idx = addr / 0x2000;
        auto s = &this->slots[pri][sec];
        auto ptr = s->data[idx].ptr;
        return ptr ? ptr[addr & 0x1FFF] : 0xFF;
    }

    inline void write(unsigned short addr, unsigned char value)
    {
        //printf("write memory $%04X <- $%02X\n", addr, value);
        if (addr == 0xFFFF) {
            this->updateSecondary(value);
            return;
        }
        int page = (addr & 0b1100000000000000) >> 14;
        int pri = this->ctx.primary[page];
        int sec = 3 == pri ? this->ctx.secondary[page] : 0;
        int idx = addr / 0x2000;
        auto s = &this->slots[pri][sec];
        auto data = &s->data[idx];
        if (data->isRAM && data->ptr) {
            data->ptr[addr & 0x1FFF] = value;
        }
    }
};

#endif // INCLUDE_MMU_HPP
