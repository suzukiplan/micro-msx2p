#ifndef INCLUDE_MMU_HPP
#define INCLUDE_MMU_HPP

#include <stdio.h>
#include <string.h>

class MMU
{
  private:
    struct DataBlock8KB {
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
        unsigned char ram[0x10000];
        unsigned char primary[4];
        unsigned char secondary[4];
    } ctx;

    MMU() {
        memset(&this->slots, 0, sizeof(this->slots));
    }

    void reset()
    {
        memset(&this->ctx, 0, sizeof(this->ctx));
        // initialize slot 3-0 as RAM
        for (int i = 0; i < 8; i++) {
            this->slots[3][0].data[i].ptr = &this->ctx.ram[i * 0x2000];
            this->slots[3][0].data[i].isRAM = true;
        }
        // initialize page layout: 0-0, 1-0, 2-0, 3-0 (TODO: is this correct?)
        for (int i = 0; i < 4; i++) this->ctx.primary[i] = i;
    }

    inline int primaryNumber(int page) { return this->ctx.primary[page & 0b11]; }
    inline int secondaryNumber(int page) { return this->ctx.secondary[page & 0b11]; }

    void setupCartridge(int pri, int sec, int idx, void* data, size_t size)
    {
        this->cartridge.ptr = (unsigned char*)data;
        this->cartridge.size = size;
        setup(pri, sec, idx, false, this->cartridge.ptr, this->cartridge.size < 0x8000 ? 0x4000 : 0x8000);
    }

    void setup(int pri, int sec, int idx, bool isRAM, unsigned char* data, int size)
    {
        do {
            this->slots[pri][sec].data[idx].ptr = data;
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
            result |= this->ctx.primary[i] & 0b11;
        }
        return result;
    }

    inline void updatePrimary(unsigned char value)
    {
        unsigned char previous[4];
        memcpy(previous, this->ctx.primary, 4);
        for (int i = 0; i < 4; i++) {
            this->ctx.primary[i] = value & 0b11;
            value >>= 2;
        }
        if (previous[0] != ctx.primary[0] ||
            previous[1] != ctx.primary[1] ||
            previous[2] != ctx.primary[2] ||
            previous[3] != ctx.primary[3]) {
            printf("update primary: %d, %d, %d, %d\n", ctx.primary[0], ctx.primary[1], ctx.primary[2], ctx.primary[3]);
        }
    }

    inline unsigned char getSecondary()
    {
        unsigned char result = 0;
        for (int i = 0; i < 4; i++) {
            result <<= 2;
            result |= this->ctx.secondary[i] & 0b11;
        }
        return result;
    }

    inline void updateSecondary(unsigned char value)
    {
        unsigned char previous[4];
        memcpy(previous, this->ctx.secondary, 4);
        for (int i = 0; i < 4; i++) {
            this->ctx.secondary[i] = value & 0b11;
            value >>= 2;
        }
        if (previous[0] != ctx.secondary[0] ||
            previous[1] != ctx.secondary[1] ||
            previous[2] != ctx.secondary[2] ||
            previous[3] != ctx.secondary[3]) {
            printf("update secondary: %d, %d, %d, %d\n", ctx.secondary[0], ctx.secondary[1], ctx.secondary[2], ctx.secondary[3]);
        }
    }

    inline unsigned char read(unsigned short addr)
    {
        int page = (addr & 0b1100000000000000) >> 14;
        auto s = &this->slots[this->ctx.primary[page]][this->ctx.secondary[page]];
        auto ptr = s->data[addr / 0x2000].ptr;
        return ptr ? ptr[addr & 0x1FFF] : 0xFF;
    }

    inline void write(unsigned short addr, unsigned char value)
    {
        if (addr == 0xFFFF) {
            updateSecondary(value);
            return;
        }
        int page = (addr & 0b1100000000000000) >> 14;
        auto s = &this->slots[this->ctx.primary[page]][this->ctx.secondary[page]];
        auto data = &s->data[addr / 0x2000];
        if (data->isRAM && data->ptr) data->ptr[addr & 0x1FFF] = value;
    }
};

#endif // INCLUDE_MMU_HPP
