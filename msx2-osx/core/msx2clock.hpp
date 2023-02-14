#ifndef INCLUDE_MSX2CLOCK_HPP
#define INCLUDE_MSX2CLOCK_HPP
#include <time.h>

class MSX2Clock {
private:
    const unsigned char mask[4][16] = {
        {0xf, 0x7, 0xf, 0x7, 0xf, 0x3, 0x7, 0xf, 0x3, 0xf, 0x1, 0xf, 0xf, 0xf, 0xf, 0xf},
        {0x0, 0x0, 0xf, 0x7, 0xf, 0x3, 0x7, 0xf, 0x3, 0x0, 0x1, 0x3, 0x0, 0x0, 0x0, 0x0},
        {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0x0, 0x0, 0x0},
        {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0x0, 0x0, 0x0}
    };

    inline void updateTimeBlocks() {
        struct tm* t2 = localtime(&this->ctx.time);
        int sec = t2->tm_sec;
        int min = t2->tm_min;
        int hour = t2->tm_hour;
        int wday = t2->tm_wday;
        int mday = t2->tm_mday;
        int month = t2->tm_mon + 1;
        int year = t2->tm_year + 80;
        this->ctx.block[0][0x0] = sec % 10;
        this->ctx.block[0][0x1] = sec / 10;
        this->ctx.block[0][0x2] = min % 10;
        this->ctx.block[0][0x3] = min / 10;
        if (this->ctx.block[1][0x0a]) {
            // 24 hour mode
            this->ctx.block[0][0x4] = hour % 10;
            this->ctx.block[0][0x5] = hour / 10;
        } else {
            // 12 hour mode
            this->ctx.block[0][0x4] = hour % 12 % 10;
            this->ctx.block[0][0x5] = hour % 12 / 10;
            if (hour >= 12) this->ctx.block[0][0x5] |= 2; // PM flag
        }
        this->ctx.block[0][0x6] = wday;
        this->ctx.block[0][0x7] = mday % 10;
        this->ctx.block[0][0x8] = mday / 10;
        this->ctx.block[0][0x9] = month % 10;
        this->ctx.block[0][0xa] = month / 10;
        this->ctx.block[0][0xb] = year % 10;
        this->ctx.block[0][0xc] = year / 10;
    }

public:
    struct Context {
        time_t time;
        int bobo;
        unsigned char latch;
        unsigned char mode;
        unsigned char test;
        unsigned char reset;
        unsigned char block[4][16];
    } ctx;
    
    MSX2Clock() {
        reset();
    }

    void reset() {
        memset(&this->ctx, 0, sizeof(this->ctx));
        this->ctx.time = time(NULL);
        this->updateTimeBlocks();
    }

    void tick() {
        this->ctx.time++;
        this->updateTimeBlocks();
    }

    unsigned char inPortB5() {
        switch (this->ctx.latch) {
            case 0x0D: return this->ctx.mode | 0xF0;
            case 0x0E: return 0xFF;
            case 0x0F: return 0xFF;
        }
        int idx = this->ctx.mode & 0b11;
        return (this->ctx.block[idx][this->ctx.latch] & this->mask[idx][this->ctx.latch]) | 0xF0;
    }
    
    void outPortB4(unsigned char value) {
        this->ctx.latch = value & 0x0F;
    }

    void outPortB5(unsigned char value) {
        switch (this->ctx.latch) {
            case 0x0D: this->ctx.mode = value; return;
            case 0x0E: this->ctx.test = value; return;
            case 0x0F: this->ctx.reset = value; return;
        }
        int mode = this->ctx.mode & 0b11;
        this->ctx.block[mode][this->ctx.latch] = value & this->mask[mode][this->ctx.latch];
    }
};

#endif // INCLUDE_MSX2CLOCK_HPP
