#ifndef INCLUDE_MSX2CLOCK_HPP
#define INCLUDE_MSX2CLOCK_HPP
#include <time.h>

class MSX2Clock {
public:
    struct Context {
        unsigned char modeReg;
        unsigned char testReg;
        unsigned char resetReg;
        unsigned char latch;
        unsigned char reg[4][13];
        time_t time;
        int bobo;
        int seconds;
        int minutes;
        int hours;
        int dayWeek;
        int days;
        int months;
        int years;
        int leapYear;
    } ctx;

private:
    const unsigned char mask[4][13] = {
        { 0x0f, 0x07, 0x0f, 0x07, 0x0f, 0x03, 0x07, 0x0f, 0x03, 0x0f, 0x01, 0x0f, 0x0f },
        { 0x00, 0x00, 0x0f, 0x07, 0x0f, 0x03, 0x07, 0x0f, 0x03, 0x00, 0x01, 0x03, 0x00 },
        { 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f },
        { 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f }
    };

    inline void setRegisters() {
        int hours = this->ctx.hours;
        if (!this->ctx.reg[1][10]) {
            if (hours >= 12) {
                hours = (hours - 12) + 20;
            }
        }
        this->ctx.reg[0][0]   =  this->ctx.seconds % 10;
        this->ctx.reg[0][1]   =  this->ctx.seconds / 10;
        this->ctx.reg[0][2]   =  this->ctx.minutes % 10;
        this->ctx.reg[0][3]   =  this->ctx.minutes / 10;
        this->ctx.reg[0][4]   =  hours % 10;
        this->ctx.reg[0][5]   =  hours / 10;
        this->ctx.reg[0][6]   =  this->ctx.dayWeek;
        this->ctx.reg[0][7]   = (this->ctx.days + 1) % 10;
        this->ctx.reg[0][8]   = (this->ctx.days + 1) / 10;
        this->ctx.reg[0][9]   = (this->ctx.months + 1) % 10;
        this->ctx.reg[0][10]  = (this->ctx.months + 1) / 10;
        this->ctx.reg[0][11]  =  this->ctx.years % 10;
        this->ctx.reg[0][12]  =  this->ctx.years / 10;
        this->ctx.reg[1][11] =  this->ctx.leapYear;
    }

    inline void update() {
        struct tm* t2 = localtime(&this->ctx.time);
        this->ctx.seconds  = t2->tm_sec;
        this->ctx.minutes  = t2->tm_min;
        this->ctx.hours    = t2->tm_hour;
        this->ctx.dayWeek  = t2->tm_wday;
        this->ctx.days     = t2->tm_mday - 1;
        this->ctx.months   = t2->tm_mon;
        this->ctx.years    = t2->tm_year - 80;
        this->ctx.leapYear = t2->tm_year % 4;
        this->setRegisters();
    }

public:
    MSX2Clock() { reset(); }

    void reset() {
        memset(&this->ctx, 0, sizeof(this->ctx));
        this->ctx.time = time(NULL);
        this->ctx.modeReg = 0x08;
        this->update();
    }

    void tick() {
        this->ctx.time++;
        this->update();
    }

    unsigned char inPortB5() {
        switch (this->ctx.latch) {
            case 0x0d:
                return this->ctx.modeReg | 0xf0;
            case 0x0e:
            case 0x0f:
                return 0xff;
        }
        int block = this->ctx.modeReg & 0x03;
        return (this->ctx.reg[block][this->ctx.latch] & mask[block][this->ctx.latch]) | 0xf0;
    }
    
    void outPortB4(unsigned char value) {
        this->ctx.latch = value & 0x0F;
    }

    void outPortB5(unsigned char value) {
        switch (this->ctx.latch) {
            case 0x0d:
                this->ctx.modeReg = value;
                return;
            case 0x0e:
                this->ctx.testReg = value;
                return;
            case 0x0f:
                this->ctx.resetReg = value;
                if (value & 0x01) {
                    for (int i = 2; i <= 8; i++) {
                        this->ctx.reg[1][i] = 0;
                    }
                }
                if (value & 0x02) {
                    this->ctx.bobo = 0;
                }
                return;
        }
        int block = this->ctx.modeReg & 0x03;
        this->ctx.reg[block][this->ctx.latch] = value & mask[block][this->ctx.latch];
    }
};

#endif // INCLUDE_MSX2CLOCK_HPP
