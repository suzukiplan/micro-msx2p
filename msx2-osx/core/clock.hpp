#ifndef INCLUDE_CLOCK_HPP
#define INCLUDE_CLOCK_HPP
#include <time.h>

class Clock {
private:
    const unsigned char mask[4][16] = {
        {0xf, 0x7, 0xf, 0x7, 0xf, 0x3, 0x7, 0xf, 0x3, 0xf, 0x1, 0xf, 0xf, 0xf, 0xf, 0xf},
        {0x0, 0x0, 0xf, 0x7, 0xf, 0x3, 0x7, 0xf, 0x3, 0x0, 0x1, 0x3, 0x0, 0x0, 0x0, 0x0},
        {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0x0, 0x0, 0x0},
        {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0x0, 0x0, 0x0}
    };
    const int daysPerMonth[2][24] = {
        {
            3, 1,
            2, 8,
            3, 1,
            3, 0,
            3, 1,
            3, 0,
            3, 1,
            3, 1,
            3, 0,
            3, 1,
            3, 0,
            3, 1
        }, {
            3, 1,
            2, 9,
            3, 1,
            3, 0,
            3, 1,
            3, 0,
            3, 1,
            3, 1,
            3, 0,
            3, 1,
            3, 0,
            3, 1
        }
    };

    void tickYear() {
        this->ctx.block[1][0xb] = (this->ctx.block[1][0xb] + 1) & 3;
        if(this->ctx.block[0][0xb] == 9) {
            this->ctx.block[0][0xb] = 0;
            if(this->ctx.block[0][0xc] == 9) {
                this->ctx.block[0][0xc] = 0;
            } else this->ctx.block[0][0xc]++;
        } else this->ctx.block[0][0xb]++;
    }

    void tickMonth() {
        if (this->ctx.block[0][0xa] == 1) {
            if(this->ctx.block[0][0x9] == 2) {
                this->ctx.block[0][0x9] = 1;
                this->ctx.block[0][0xa] = 0;
                tickYear();
            } else this->ctx.block[0][0x9]++;
        } else {
            if (this->ctx.block[0][0x9] == 9) {
                this->ctx.block[0][0x9] = 0;
                this->ctx.block[0][0xa] = 1;
            } else this->ctx.block[0][0x9]++;
        }
    }

    void tickDay() {
        int monthIdx = (((this->ctx.block[0][0xa] * 10) + this->ctx.block[0][0x9]) - 1) << 1;
        this->ctx.block[0][0x6] = (this->ctx.block[0][0x6] + 1) & 7;
        if (daysPerMonth[(this->ctx.block[1][0xb]) ? 0 : 1][monthIdx] == this->ctx.block[0][0x8] && daysPerMonth[(this->ctx.block[1][0xb]) ? 0 : 1][monthIdx + 1] == this->ctx.block[0][0x7]) {
            this->ctx.block[0][0x7] = 1;
            this->ctx.block[0][0x8] = 0;
            tickMonth();
        } else if(this->ctx.block[0][0x7] == 9) {
            this->ctx.block[0][0x8]++;
            this->ctx.block[0][0x7] = 0;
        } else this->ctx.block[0][0x7]++;
    }

    void tickHour() {
        if (this->ctx.block[1][0xa] & 1) {
            if (this->ctx.block[0][0x4] == 3) {
                if (this->ctx.block[0][0x5] == 2) {
                    this->ctx.block[0][0x4] = 0;
                    this->ctx.block[0][0x5] = 0;
                    tickDay();
                } else this->ctx.block[0][0x4]++;
            } else if(this->ctx.block[0][0x4] == 9) {
                this->ctx.block[0][0x4] = 0;
                this->ctx.block[0][0x5]++;
            } else this->ctx.block[0][0x4]++;
        } else {
            puts("Unsupported 12-hour system!");
            exit(-1);
        }
    }

    void tickMinute() {
        if (this->ctx.block[0][0x2] == 9) {
            this->ctx.block[0][0x2] = 0;
            if (this->ctx.block[0][0x3] == 5) {
                this->ctx.block[0][0x3] = 0;
                tickHour();
            } else this->ctx.block[0][0x3]++;
        } else this->ctx.block[0][0x2]++;
    }

public:
    struct Context {
        int bobo;
        unsigned char latch;
        unsigned char mode;
        unsigned char test;
        unsigned char reset;
        unsigned char block[4][16];
    } ctx;
    
    Clock() {
        reset();
    }

    void reset() {
        memset(&this->ctx, 0, sizeof(this->ctx));
        time_t t1 = time(NULL);
        struct tm* t2 = localtime(&t1);
        int sec = t2->tm_sec;
        int min = t2->tm_min;
        int hour = t2->tm_hour;
#if 0
        int wday = 2;
        int mday = 1;
        int month = 1;
        int year = 1991 - 1980;
#else
        int wday = t2->tm_wday;
        int mday = t2->tm_mday;
        int month = t2->tm_mon + 1;
        int year = t2->tm_year + 80;
#endif
        this->ctx.block[0][0x0] = sec % 10;
        this->ctx.block[0][0x1] = sec / 10;
        this->ctx.block[0][0x2] = min % 10;
        this->ctx.block[0][0x3] = min / 10;
        this->ctx.block[0][0x4] = hour % 10;
        this->ctx.block[0][0x5] = hour / 10;
        this->ctx.block[0][0x6] = wday;
        this->ctx.block[0][0x7] = mday % 10;
        this->ctx.block[0][0x8] = mday / 10;
        this->ctx.block[0][0x9] = month % 10;
        this->ctx.block[0][0xa] = month / 10;
        this->ctx.block[0][0xb] = year % 10;
        this->ctx.block[0][0xc] = year / 10;
    }

    void tick() {
        if (this->ctx.block[0][0x0] == 9) {
            this->ctx.block[0][0x0] = 0;
            if (this->ctx.block[0][0x1] == 5) {
                this->ctx.block[0][0x1] = 0;
                if (this->ctx.block[0][0xd] & 8) {
                    tickMinute();
                }
            } else this->ctx.block[0][0x1]++;
        } else this->ctx.block[0][0x0]++;
    }

    unsigned char inPortB5() {
        switch (this->ctx.latch) {
            case 0x0D:
                return this->ctx.mode | 0xF0;
            case 0x0E:
            case 0x0F:
                return 0xFF;
            default: {
                int idx = this->ctx.mode & 0b11;
                return (this->ctx.block[idx][this->ctx.latch] & this->mask[idx][this->ctx.latch]) | 0xF0;
            }
        }
    }
    
    void outPortB4(unsigned char value) {
        this->ctx.latch = value & 0x0F;
    }

    void outPortB5(unsigned char value) {
        switch (this->ctx.latch) {
            case 0x0D:
                this->ctx.mode = value;
                return;
            case 0x0E:
                this->ctx.test = value;
                return;
            case 0x0F:
                this->ctx.reset = value;
                if (value & 0b00000001) {
                    for (int i = 2; i <= 8; i++) {
                        this->ctx.block[1][i] = 0;
                    }
                }
                return;
        }
        int mode = this->ctx.mode & 0b11;
        this->ctx.block[mode][this->ctx.latch] = value & this->mask[mode][this->ctx.latch];
    }
};

#endif // INCLUDE_CLOCK_HPP
