#ifndef INCLUDE_RTC_HPP
#define INCLUDE_RTC_HPP
#include <time.h>

class RTC {
private:
    unsigned char mask[4][16] = {
        {0xf, 0x7, 0xf, 0x7, 0xf, 0x3, 0x7, 0xf, 0x3, 0xf, 0x1, 0xf, 0xf, 0xf, 0xf, 0xf},
        {0x0, 0x0, 0xf, 0x7, 0xf, 0x3, 0x7, 0xf, 0x3, 0x0, 0x1, 0x3, 0x0, 0x0, 0x0, 0x0},
        {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0x0, 0x0, 0x0},
        {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0x0, 0x0, 0x0}
    };
    int daysPerMonth[2][24] = {
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
        unsigned char reg;
        unsigned char reserved[3];
        unsigned char block[4][16];
    } ctx;
    
    RTC() {
        reset();
    }

    void reset() {
        memset(&this->ctx, 0, sizeof(this->ctx));
        time_t t1 = time(NULL);
        struct tm* t2 = localtime(&t1);
        this->ctx.block[0][0x0] = t2->tm_sec % 10;
        this->ctx.block[0][0x1] = t2->tm_sec / 10;
        this->ctx.block[0][0x2] = t2->tm_min % 10;
        this->ctx.block[0][0x3] = t2->tm_min / 10;
        this->ctx.block[0][0x4] = t2->tm_hour % 10;
        this->ctx.block[0][0x5] = t2->tm_hour / 10;
        this->ctx.block[0][0x6] = t2->tm_wday;
        this->ctx.block[0][0x7] = t2->tm_mday % 10;
        this->ctx.block[0][0x8] = t2->tm_mday / 10;
        this->ctx.block[0][0x9] = (t2->tm_mon + 1) % 10;
        this->ctx.block[0][0xa] = (t2->tm_mon + 1) / 10;
        this->ctx.block[0][0xb] = (t2->tm_year + 80) % 10;
        this->ctx.block[0][0xc] = (t2->tm_year + 80) / 10;
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

    unsigned char inPort() {
        return this->ctx.block[this->ctx.reg >= 0x0D ? 0 : this->ctx.block[0][0xD] & 3][this->ctx.reg];
    }
    
    void outPort0(unsigned char value) {
        this->ctx.reg = value & 0x0F;
    }

    void outPort1(unsigned char value) {
        int mode = this->ctx.reg >= 0x0D ? 0 : this->ctx.block[0][0xD] & 3;
        this->ctx.block[mode][this->ctx.reg] = value & this->mask[mode][this->ctx.reg];
    }
};

#endif // INCLUDE_RTC_HPP
