/**
 * micro MSX2+ - V9958 (MSX-VIDEO)
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
#ifndef INCLUDE_V9958_HPP
#define INCLUDE_V9958_HPP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#define COMMAND_DEBUG

class V9958
{
  private:
    enum HorizontalEventType {
        ActiveDisplayH,
        RightBorder,
        RightErase,
        SyncRight,
        LeftErase,
        LeftBorder
    };

    enum VerticalEventType {
        VerticalSync,
        TopErase,
        TopBorder,
        ActiveDisplayV,
        BottomBorder,
        BottomErase
    };

    struct EventTables {
        HorizontalEventType ht[1368];
        VerticalEventType vt[262];
        int hi[1368];
        int vi[262];
    } evt;

    void updateEventTableH()
    {
        for (int i = 0; i < 1368; i++) {
            int ii = i + this->getAdjustX();
            if (ii < 0) {
                ii += 1368;
            } else if (1368 <= ii) {
                ii -= 1368;
            }
            if (i < 1024) {
                this->evt.ht[ii] = HorizontalEventType::ActiveDisplayH;
                this->evt.hi[ii] = i;
            } else if (i < 1024 + 56) {
                // note: Actually RightBorder is 59Hz, but 56Hz for optimize the performance
                this->evt.ht[ii] = HorizontalEventType::RightBorder;
                this->evt.hi[ii] = i - 1024;
            } else if (i < 1024 + 56 + 30) {
                // note: Actually RightErase is 27Hz, but 30Hz for optimize the performance
                this->evt.ht[ii] = HorizontalEventType::RightErase;
                this->evt.hi[ii] = i - 1024 - 56;
            } else if (i < 1024 + 56 + 30 + 100) {
                this->evt.ht[ii] = HorizontalEventType::SyncRight;
                this->evt.hi[ii] = i - 1024 - 56 - 30;
            } else if (i < 1024 + 56 + 30 + 100 + 102) {
                this->evt.ht[ii] = HorizontalEventType::LeftErase;
                this->evt.hi[ii] = i - 1024 - 56 - 30 - 100;
            } else {
                this->evt.ht[ii] = HorizontalEventType::LeftBorder;
                this->evt.hi[ii] = i - 1024 - 56 - 30 - 100 - 102;
            }
        }
    }

    void updateEventTableV()
    {
        for (int i = 0; i < 262; i++) {
            int ii = i + this->getAdjustY();
            if (ii < 0) {
                ii += 262;
            } else if (262 <= ii) {
                ii -= 262;
            }
            if (i < 3) {
                this->evt.vt[ii] = VerticalEventType::VerticalSync;
                this->evt.vi[ii] = i;
            } else if (i < 3 + 13) {
                this->evt.vt[ii] = VerticalEventType::TopErase;
                this->evt.vi[ii] = i - 3;
            } else if (i < 3 + 13 + this->getTopBorder()) {
                this->evt.vt[ii] = VerticalEventType::TopBorder;
                this->evt.vi[ii] = i - 3 - 13;
            } else if (i < 3 + 13 + this->getTopBorder() + this->getLineNumber()) {
                this->evt.vt[ii] = VerticalEventType::ActiveDisplayV;
                this->evt.vi[ii] = i - 3 - 13 - this->getTopBorder();
            } else if (i < 3 + 13 + this->getTopBorder() + this->getLineNumber() + this->getBottomBorder()) {
                this->evt.vt[ii] = VerticalEventType::BottomBorder;
                this->evt.vi[ii] = i - 3 - 13 - this->getTopBorder() - this->getLineNumber();
            } else {
                this->evt.vt[ii] = VerticalEventType::BottomErase;
                this->evt.vi[ii] = i - 3 - 13 - this->getTopBorder() - this->getLineNumber() - this->getBottomBorder();
            }
        }
    }

    unsigned short yjkColor[32][64][64];
    const unsigned char regMask[64] = {
        0x7e, 0x7b, 0x7f, 0xff, 0x3f, 0xff, 0x3f, 0xff,
        0xfb, 0xbf, 0x07, 0x03, 0xff, 0xff, 0x07, 0x0f,
        0x0f, 0xbf, 0xff, 0xff, 0x3f, 0x3f, 0x3f, 0xff,
        0x00, 0x7f, 0x3f, 0x07, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    int colorMode;
    void* arg;
    void (*detectInterrupt)(void* arg, int ie);
    void (*cancelInterrupt)(void* arg);
    void (*detectBreak)(void* arg);
    inline int min(int a, int b) { return a < b ? a : b; }
    inline int max(int a, int b) { return a > b ? a : b; }
    const int adjust[16] = {0, 1, 2, 3, 4, 5, 6, 7, -8, -7, -6, -5, -4, -3, -2, -1};

  public:
    bool renderLimitOverSprites = true;
    unsigned short display[568 * 240];
    unsigned short palette[16];
    unsigned char lastRenderScanline;

    struct CommandContext {
        int wait;
        int sx;
        int sy;
        int dx;
        int dy;
        int nx;
        int ny;
        int dix;
        int diy;
        int maj;
        int min;
        double majF;
        double minF;
    };

    struct Context {
        int bobo;
        int countH;
        int countV;
        unsigned int addr;
        struct CommandContext cmd;
        unsigned char ram[0x20000];
        unsigned char reg[64];
        unsigned char pal[16][2];
        unsigned char tmpAddr[2];
        unsigned char stat[16];
        unsigned char latch1;
        unsigned char latch2;
        unsigned char readBuffer;
        unsigned char command;
        unsigned char commandL;
        unsigned char reverseVdpR9Bit4;
        unsigned char reverseVdpR9Bit5;
        unsigned char hardwareResetFlag;
        unsigned int counter;
        unsigned char lineIE1;
        unsigned char reserved[7];
    } ctx;

    V9958()
    {
        memset(palette, 0, sizeof(palette));
        this->reset();
    }

    void initialize(int colorMode, void* arg, void (*detectInterrupt)(void*, int), void (*cancelInterrupt)(void*), void (*detectBreak)(void*))
    {
        this->colorMode = colorMode;
        this->arg = arg;
        this->detectInterrupt = detectInterrupt;
        this->cancelInterrupt = cancelInterrupt;
        this->detectBreak = detectBreak;
        this->initYjkColorTable();
        this->reset();
    }

    void initYjkColorTable()
    {
        for (int y = 0; y < 32; y++) {
            for (int J = 0; J < 64; J++) {
                for (int K = 0; K < 64; K++) {
                    int j = (J & 0x1f) - (J & 0x20);
                    int k = (K & 0x1f) - (K & 0x20);
                    int r = 255 * (y + j) / 31;
                    int g = 255 * (y + k) / 31;
                    int b = 255 * ((5 * y - 2 * j - k) / 4) / 31;
                    r = this->min(255, this->max(0, r));
                    g = this->min(255, this->max(0, g));
                    b = this->min(255, this->max(0, b));
                    r = (r & 0b11111000) << (7 + colorMode);
                    g = (g & 0b11111000) << (2 + colorMode);
                    b = (b & 0b11111000) >> 3;
                    this->yjkColor[y][J][K] = r | g | b;
                }
            }
        }
    }

    void updateAllPalettes()
    {
        for (int i = 0; i < 16; i++) {
            this->updatePaletteCacheFromRegister(i);
        }
    }

    void reset()
    {
        static unsigned int rgb[16] = {
            0x000000, 0x000000,
            //***     ***     ***
            0b110000000010000000100000, // 6 1 1
            0b111000000110000001100000, // 7 3 3
            0b001000000010000011100000, // 1 1 7
            0b011000000100000011100000, // 3 2 7
            0b001000010100000000100000, // 1 5 1
            0b110000001000000011100000, // 6 2 7
            0b001000011100000000100000, // 1 7 1
            0b011000011100000001100000, // 3 7 3
            0b110000011000000000100000, // 6 6 1
            0b110000011000000010000000, // 6 6 4
            0b100000000100000000100000, // 4 1 1
            0b010000011000000010100000, // 2 6 5
            0b101000101000000010100000, // 5 5 5
            0b111000111000000011100000  // 7 7 7
        };
        static unsigned char stat[16] = {
            0x1f, 0x00, 0xcc, 0x40, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        static unsigned char reg[64] = {
            0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x9b, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        memset(this->display, 0, sizeof(this->display));
        memset(&this->ctx, 0, sizeof(this->ctx));
        memcpy(&this->ctx.stat, stat, sizeof(stat));
        memcpy(&this->ctx.reg, reg, sizeof(reg));
        this->ctx.hardwareResetFlag = 0xFF;
        for (int i = 0; i < 16; i++) {
            this->ctx.pal[i][0] = 0;
            this->ctx.pal[i][1] = 0;
            this->ctx.pal[i][0] |= (rgb[i] & 0b111000000000000000000000) >> 17;
            this->ctx.pal[i][1] |= (rgb[i] & 0b000000001110000000000000) >> 13;
            this->ctx.pal[i][0] |= (rgb[i] & 0b000000000000000011100000) >> 5;
            updatePaletteCacheFromRegister(i);
        }
        this->updateEventTables();
    }

    inline int getScreenMode()
    {
        int mode = this->ctx.reg[1] & 0b00011000;
        mode |= (this->ctx.reg[0] & 0b00001110) >> 1;
        return mode;
    }

    inline int getVideoMode()
    {
        int mode = 0;
        if (ctx.reg[1] & 0b00010000) mode |= 0b00001;
        if (ctx.reg[1] & 0b00001000) mode |= 0b00010;
        if (ctx.reg[0] & 0b00000010) mode |= 0b00100;
        if (ctx.reg[0] & 0b00000100) mode |= 0b01000;
        if (ctx.reg[0] & 0b00001000) mode |= 0b10000;
        return mode;
    }

    void updateEventTables()
    {
        this->updateEventTableH();
        this->updateEventTableV();
    }

    inline bool isExpansionRAM() { return ctx.reg[45] & 0b01000000 ? true : false; }
    inline int getSpriteSize() { return ctx.reg[1] & 0b00000010 ? 16 : 8; }
    inline bool isSpriteMag() { return ctx.reg[1] & 0b00000001 ? true : false; }
    inline bool isEnabledExternalVideoInput() { return ctx.reg[0] & 0b00000001 ? true : false; }
    inline bool isEnabledScreen() { return ctx.reg[1] & 0b01000000 ? true : false; }
    inline bool isIE0() { return ctx.reg[1] & 0b00100000 ? true : false; }
    inline bool isIE1() { return ctx.reg[0] & 0b00010000 ? true : false; }
    inline bool isIE2() { return ctx.reg[0] & 0b00100000 ? true : false; }
    inline bool isEnabledMouse() { return ctx.reg[8] & 0b10000000 ? true : false; }
    inline bool isEnabledLightPen() { return ctx.reg[8] & 0b01000000 ? true : false; }
    inline bool isYAE() { return ctx.reg[25] & 0b00010000 ? true : false; }
    inline bool isYJK() { return ctx.reg[25] & 0b00001000 ? true : false; }
    inline bool isMaskLeft8px() { return ctx.reg[25] & 0b00000010 ? true : false; }
    inline bool getSP2() { return ctx.reg[25] & 0b00000001; }
    inline int getOnTime() { return (ctx.reg[13] & 0xF0) >> 4; }
    inline int getOffTime() { return ctx.reg[13] & 0x0F; }
    inline int getSyncMode() { return (ctx.reg[9] & 0b00110000) >> 4; }
    inline int getTopBorder() { return 26 - ((this->ctx.reg[9] & 0b10000000) >> 7) * 10; }
    inline int getBottomBorder() { return 25 - ((this->ctx.reg[9] & 0b10000000) >> 7) * 10; }
    inline int getLineNumber() { return 192 + ((this->ctx.reg[9] & 0b10000000) >> 7) * 20; }
    inline int isInterlaceMode() { return this->ctx.reg[9] & 0b00001000 ? true : false; }
    inline int isEvenOrderMode() { return this->ctx.reg[9] & 0b00000100 ? true : false; }
    inline bool isSprite16px() { return this->ctx.reg[1] & 0b00000010 ? true : false; }
    inline bool isSprite2x() { return this->ctx.reg[1] & 0b00000001 ? true : false; }
    inline bool isSpriteDisplay() { return this->ctx.reg[8] & 0b00000010 ? false : true; }
    inline int getAdjustX() { return this->adjust[this->ctx.reg[18] & 0x0F]; }
    inline int getAdjustY() { return this->adjust[(this->ctx.reg[18] & 0xF0) >> 4]; }
    inline bool isF() { return this->ctx.stat[0] & 0x80; }
    inline void setF() { this->ctx.stat[0] |= 0x80; }
    inline void resetF() { this->ctx.stat[0] &= 0x7F; }
    inline void reset5S() { this->ctx.stat[0] &= 0b10111111; }
    inline void resetCollision() { this->ctx.stat[0] &= 0b11011111; }

    inline void set5S(bool f, int n)
    {
        this->ctx.stat[0] &= 0b11100000;
        this->ctx.stat[0] |= (f ? 0b01000000 : 0) | (n & 0b00011111);
    }

    inline void setCollision(int x, int y)
    {
        this->ctx.stat[0] |= 0b00100000;
        x += 12;
        y += 8;
        this->ctx.stat[3] = x & 0xFF;
        this->ctx.stat[4] = (x & 0x0100) >> 8;
        this->ctx.stat[5] = y & 0xFF;
        this->ctx.stat[6] = (y & 0x0300) >> 8;
    }

    inline bool isFH() { return this->ctx.stat[1] & 0x01; }
    inline void setFH() { this->ctx.stat[1] |= 0b00000001; }
    inline void resetFH() { this->ctx.stat[1] &= 0b11111110; }
    inline void setTR() { this->ctx.stat[2] |= 0b10000000; }
    inline void resetTR() { this->ctx.stat[2] &= 0b01111111; }
    inline void setVR() { this->ctx.stat[2] |= 0b01000000; }
    inline void resetVR() { this->ctx.stat[2] &= 0b10111111; }
    inline void setHR() { this->ctx.stat[2] |= 0b00100000; }
    inline void resetHR() { this->ctx.stat[2] &= 0b11011111; }
    inline void setBD() { this->ctx.stat[2] |= 0b00010000; }
    inline void resetBD() { this->ctx.stat[2] &= 0b11101111; }
    inline void setEO() { this->ctx.stat[2] |= 0b00000010; }
    inline void resetEO() { this->ctx.stat[2] &= 0b11111101; }
    inline void setCE() { this->ctx.stat[2] |= 0b00000001; }
    inline void resetCE() { this->ctx.stat[2] &= 0b11111110; }

    inline int getSpriteAttributeTableM1()
    {
        int addr = this->ctx.reg[11] & 0b00000011;
        addr <<= 15;
        addr |= (this->ctx.reg[5] & 0b11111111) << 7;
        return addr;
    }

    inline int getSpriteAttributeTableM2()
    {
        int addr = this->ctx.reg[11] & 0b00000011;
        addr <<= 15;
        addr |= (this->ctx.reg[5] & 0b11111100) << 7;
        return addr;
    }

    inline int getSpriteColorTable()
    {
        int addr = this->ctx.reg[11] & 0b00000011;
        addr <<= 15;
        addr |= (this->ctx.reg[5] & 0b11111000) << 7;
        return addr;
    }

    inline int getSpriteGeneratorTable()
    {
        int addr = this->ctx.reg[6] & 0b00111111;
        addr <<= 11;
        return addr;
    }

    inline int getAddressMask()
    {
        switch (this->getScreenMode()) {
            case 0b00000: // GRAPHIC1
            case 0b00001: // GRAPHIC2
            case 0b01000: // MULTI COLOR
            case 0b10000: // TEXT1
                return 0x3FFF;
            case 0b00010: // GRAPHIC3
            case 0b00011: // GRAPHIC4
            case 0b00100: // GRAPHIC5
            case 0b00101: // GRAPHIC6
            case 0b00111: // GRAPHIC7
            case 0b10010: // TEXT2
                return 0x1FFFF;
            default:
                return 0;
        }
    }

    inline unsigned short getBackdropColor()
    {
        switch (this->getScreenMode()) {
            case 0b00111: return this->convertColor_8bit_to_16bit(this->ctx.reg[7]);
            case 0b00100: return this->palette[this->ctx.reg[7] & 0b00000011];
            default: return this->palette[this->ctx.reg[7] & 0b00001111];
        }
    }

    inline unsigned short getTextColor()
    {
        if (this->getScreenMode() == 0b00111) {
            return 0;
        } else {
            return this->palette[(this->ctx.reg[7] & 0b11110000) >> 4];
        }
    }

    inline int getNameTableAddress()
    {
        switch (this->getScreenMode()) {
            case 0b00000: // GRAPHIC1
            case 0b00001: // GRAPHIC2
            case 0b00010: // GRAPHIC3
            case 0b01000: // MULTI COLOR
            case 0b10000: // TEXT1
                return (this->ctx.reg[2] & 0b01111111) << 10;
            case 0b00011: // GRAPHIC4
            case 0b00100: // GRAPHIC5
                return (this->ctx.reg[2] & 0b01100000) << 10;
            case 0b00101: // GRAPHIC6
            case 0b00111: // GRAPHIC7
                return (this->ctx.reg[2] & 0b00100000) << 11;
            case 0b10010: // TEXT2
                return (this->ctx.reg[2] & 0b01111100) << 10;
            default:
                return 0;
        }
    }

    inline int getNameTableSize()
    {
        switch (this->getScreenMode()) {
            case 0b00000: return 768; // GRAPHIC1
            case 0b00001: return 768; // GRAPHIC2
            case 0b00010: return 768; // GRAPHIC3
            case 0b00011:             // GRAPHIC4
            case 0b00100:             // GRAPHIC5
                return this->getLineNumber() == 192 ? 0x6000 : 0x6A00;
            case 0b00101: // GRAPHIC6
            case 0b00111: // GRAPHIC7
                return this->getLineNumber() == 192 ? 0xC000 : 0xD400;
            case 0b01000: return 768;     // MULTI COLOR
            case 0b10000: return 40 * 24; // TEXT1
            case 0b10010: return 80 * 27; // TEXT2
            default: return 0;            // n/a
        }
    }

    inline int getPatternGeneratorAddress()
    {
        switch (this->getScreenMode()) {
            case 0b00000: // GRAPHIC1
            case 0b01000: // MULTI COLOR
            case 0b10000: // TEXT1
            case 0b10010: // TEXT2
                return (this->ctx.reg[4] & 0b00111111) << 11;
            case 0b00001: // GRAPHIC2
            case 0b00010: // GRAPHIC3
                return (this->ctx.reg[4] & 0b00111100) << 11;
            default:
                return 0; // n/a
        }
    }

    inline int getPatternGeneratorSize()
    {
        switch (this->getScreenMode()) {
            case 0b00000: return 2048; // GRAPHIC1
            case 0b00001: return 6144; // GRAPHIC2
            case 0b00010: return 6144; // GRAPHIC3
            case 0b01000: return 2048; // MULTI COLOR
            case 0b10000: return 2048; // TEXT1
            case 0b10010: return 2048; // TEXT2
            default: return 0;         // n/a
        }
    }

    inline int getColorTableAddress()
    {
        switch (this->getScreenMode()) {
            case 0b00000: // GRAPHIC1
            {
                int r3 = this->ctx.reg[3];
                int r10 = this->ctx.reg[10] & 0b00000111;
                return (r3 << 6) + (r10 << 14);
            }
            case 0b00001: // GRAPHIC2
            case 0b00010: // GRAPHIC3
            {
                int r3 = this->ctx.reg[3] & 0b10000000;
                int r10 = this->ctx.reg[10] & 0b00000111;
                return (r3 << 6) + (r10 << 14);
            }
            case 0b10010: // TEXT2 (Blink Table)
            {
                int r3 = this->ctx.reg[3] & 0b11111000;
                int r10 = this->ctx.reg[10] & 0b00000111;
                return (r3 << 6) + (r10 << 14);
            }
            default:
                return 0; // n/a
        }
    }

    inline int getColorTableSize()
    {
        switch (this->getScreenMode()) {
            case 0b00000: return 32;   // GRAPHIC1
            case 0b00001: return 6144; // GRAPHIC2
            case 0b00010: return 6144; // GRAPHIC3
            case 0b10010: return 240;  // TEXT2 (Blink Table)
            default: return 0;         // n/a
        }
    }

    inline void tick()
    {
        // execute command
        if (this->ctx.cmd.wait) {
            this->ctx.cmd.wait--;
        }
        if (this->ctx.command && 0 == this->ctx.cmd.wait) {
            switch (this->ctx.command) {
                case 0b1101: this->executeCommandHMMM(false); break;
                case 0b1100: this->executeCommandHMMV(false); break;
                case 0b1001: this->executeCommandLMMM(false); break;
                case 0b1000: this->executeCommandLMMV(false); break;
                case 0b0111: this->executeCommandLINE(false); break;
                case 0b0110: this->executeCommandSRCH(false); break;
                case 0b0101: this->executeCommandPSET(false); break;
                case 0b0100: this->executeCommandPOINT(false); break;
            }
        }

        // count up (H)
        auto htPrev = this->evt.ht[this->ctx.countH];
        this->ctx.countH++;
        this->ctx.countH %= 1368;
        if (htPrev != this->evt.ht[this->ctx.countH]) {
            tick_checkHorizontalEvents();
        }
    }

    inline void tick_checkHorizontalEvents()
    {
        switch (this->evt.ht[this->ctx.countH]) {
            case HorizontalEventType::LeftErase:
                break;
            case HorizontalEventType::LeftBorder:
                break;
            case HorizontalEventType::ActiveDisplayH:
                this->resetHR();                                   // horizontal active
                if (this->isF() || this->isFH()) this->checkIRQ(); // fire IRQ if F|FH
                break;
            case HorizontalEventType::RightBorder:
                this->setHR(); // horizontal blanking
                if (this->isIE1()) {
                    this->tick_checkIntH(); // set F if match Reg.19
                } else {
                    this->ctx.stat[1] &= 0xFE; // Reset FH if is not IE1
                }
                break;
            case HorizontalEventType::RightErase:
                this->tick_display();
                break;
            case HorizontalEventType::SyncRight: {
                // count up (V)
                auto vtPrev = this->evt.vt[this->ctx.countV];
                this->ctx.countV++;
                this->ctx.countV %= 262;
                if (vtPrev != this->evt.vt[this->ctx.countV]) {
                    tick_checkVerticalEvents();
                }
                break;
            }
        }
    }

    inline void tick_checkVerticalEvents()
    {
        switch (this->evt.vt[this->ctx.countV]) {
            case VerticalEventType::VerticalSync:
                this->ctx.counter++;
                this->detectBreak(this->arg);
                break;
            case VerticalEventType::TopErase:
                break;
            case VerticalEventType::TopBorder:
                break;
            case VerticalEventType::ActiveDisplayV:
                this->resetVR();
                break;
            case VerticalEventType::BottomBorder:
                this->setVR();
                this->setF(); // NOTE: do not callback at this timing (callback after HR reset)
                break;
            case VerticalEventType::BottomErase:
                break;
        }
    }

    inline void tick_display()
    {
        int scanline = this->evt.vi[this->ctx.countV] - this->getAdjustY();
        switch (this->evt.vt[this->ctx.countV]) {
            case VerticalEventType::TopBorder:
                break;
            case VerticalEventType::ActiveDisplayV:
                scanline += this->getTopBorder();
                break;
            case VerticalEventType::BottomBorder:
                scanline += this->getTopBorder() + this->getLineNumber();
                break;
            default: return;
        }
        if (scanline < 0) {
            scanline += 240;
        } else if (240 <= scanline) {
            return;
        }
        // render backdrop
        auto renderPosition = &this->display[scanline * 568];
        if (0b00100 == this->getScreenMode()) {
            unsigned short ec = this->palette[(this->ctx.reg[7] & 0b00001100) >> 2];
            unsigned short oc = this->palette[this->ctx.reg[7] & 0b00000011];
            for (int x = 0; x < 568; x += 2) {
                renderPosition[x] = ec;
                renderPosition[x + 1] = oc;
            }
        } else {
            unsigned short bc = this->getBackdropColor();
            for (int x = 0; x < 568; x++) {
                renderPosition[x] = bc;
            }
        }
        // render main display
        if (this->getTopBorder() - this->getAdjustY() <= scanline) {
            int renderLine = scanline - (this->getTopBorder() - this->getAdjustY());
            if (renderLine < this->getLineNumber()) {
                this->renderScanline(renderLine, &renderPosition[26 - (this->getAdjustX() << 1)]);
            }
        }
    }

    inline void tick_checkIntH()
    {
        if (!this->isFH()) {
            int scanline;
            switch (this->evt.vt[this->ctx.countV]) {
                case VerticalEventType::ActiveDisplayV:
                    scanline = this->evt.vi[this->ctx.countV];
                    break;
                case VerticalEventType::BottomBorder:
                    scanline = this->evt.vi[this->ctx.countV];
                    scanline += this->getLineNumber();
                    break;
                case VerticalEventType::TopBorder:
                    scanline = this->evt.vi[this->ctx.countV];
                    scanline += this->getLineNumber();
                    scanline += this->getBottomBorder();
                    break;
                default:
                    return;
            }
            if (scanline == this->ctx.lineIE1) {
                this->setFH();
            }
        }
    }

    inline unsigned char inPort98()
    {
        unsigned char result = this->ctx.readBuffer;
        this->readVideoMemory();
        this->ctx.latch1 = 0;
        return result;
    }

    inline unsigned char inPort99()
    {
        int sn = this->ctx.reg[15] & 0b00001111;
        unsigned char result = this->ctx.stat[sn];
        switch (sn) {
            case 0: // |F|S5|C|5#|5#|5#|5#|5#|
                this->reset5S();
                this->resetCollision();
                if (this->isF()) {
                    this->resetF();
                    this->checkIRQ();
                }
                break;
            case 1: // |*|*|ID|ID|ID|ID|ID|FH|
                if (this->isFH() && this->isIE1()) {
                    this->resetFH();
                    this->checkIRQ();
                }
                result &= 0b00000001;
                result |= 0b00000100;
                break;
            case 2: // |TR|VR|HR|BD|1|1|EO|CE|
                result |= 0b00001100;
                break;
            case 5:
                this->ctx.stat[3] = 0;
                this->ctx.stat[4] = 0;
                this->ctx.stat[5] = 0;
                this->ctx.stat[6] = 0;
                break;
            case 7:
                if (this->ctx.command == 0b1010) {
                    result = this->ctx.stat[7];
                    executeCommandLMCM(false);
                }
                break;
        }
        this->ctx.latch1 = 0;
        return result;
    }

    inline void outPort98(unsigned char value)
    {
        this->ctx.readBuffer = value;
        this->ctx.ram[this->ctx.addr] = this->ctx.readBuffer;
        this->incrementAddress();
        this->ctx.latch1 = 0;
    }

    inline void outPort99(unsigned char value)
    {
        this->ctx.latch1 &= 1;
        this->ctx.tmpAddr[this->ctx.latch1++] = value;
        if (2 == this->ctx.latch1) {
            if ((this->ctx.tmpAddr[1] & 0b11000000) == 0b10000000) {
                // Direct access to VDP registers
                this->updateRegister(this->ctx.tmpAddr[1] & 0b00111111, this->ctx.tmpAddr[0]);
            } else if (this->ctx.tmpAddr[1] & 0b01000000) {
                this->updateAddress();
            } else {
                this->updateAddress();
                this->readVideoMemory();
            }
        }
    }

    inline void outPort9A(unsigned char value)
    {
        this->ctx.latch2 &= 1;
        int pn = this->ctx.reg[16] & 0b00001111;
        this->ctx.pal[pn][this->ctx.latch2++] = value;
        if (2 == this->ctx.latch2) {
            updatePaletteCacheFromRegister(pn);
            this->ctx.reg[16]++;
            this->ctx.reg[16] &= 0b00001111;
        }
    }

    inline void outPort9B(unsigned char value)
    {
        unsigned char r17 = this->ctx.reg[17];
        if (17 != (r17 & 0b00111111)) {
            this->updateRegister(r17 & 0b00111111, value);
        }
        if (0 == (r17 & 0b10000000)) {
            this->ctx.reg[17] = (r17 + 1) & 0b00111111;
        }
    }

#if 0 // このポートを実装すると一部ゲームで表示がバグるので実装しない
    inline void outPortF3(unsigned char value) {
        /*
         b0    M3
         b1    M4
         b2    M5
         b3    M2
         b4    M1
         b5    TP
         b6    YUV ... not support yet
         b7    YAE ... not support yet
         */
        // update M3/M4/M5
        unsigned char r0 = this->ctx.reg[0] & 0b11110001;
        r0 |= (value & 0b00000111) << 1;
        this->updateRegister(0, r0);
        // update M2/M1
        unsigned char r1 = this->ctx.reg[1] & 0b11100111;
        r1 |= value & 0b00011000;
        this->updateRegister(1, r1);
        // update TP
        unsigned char r8 = this->ctx.reg[8] & 0b11011111;
        r8 |= value & 0b00100000;
        this->updateRegister(8, r8);
    }
#endif

    inline void outPortF4(unsigned char value)
    {
        this->ctx.hardwareResetFlag = value;
    }

    inline unsigned char inPortF4()
    {
        return this->ctx.hardwareResetFlag;
    }

    inline unsigned short bit2to5(unsigned short n)
    {
        n <<= 3;
        n |= n & 0b01000 ? 1 : 0;
        n |= n & 0b10000 ? 2 : 0;
        n |= n & 0b11000 ? 4 : 0;
        return n;
    }

    inline unsigned short bit3to5(unsigned short n)
    {
        n <<= 2;
        n |= n & 0b01000 ? 1 : 0;
        n |= n & 0b10000 ? 2 : 0;
        return n;
    }

    inline unsigned short bit3to6(unsigned short n)
    {
        n <<= 3;
        n |= n & 0b001000 ? 1 : 0;
        n |= n & 0b010000 ? 2 : 0;
        n |= n & 0b100000 ? 4 : 0;
        return n;
    }

    inline void updatePaletteCacheFromRegister(int pn)
    {
        unsigned short r = (this->ctx.pal[pn][0] & 0b01110000) >> 4;
        unsigned short b = this->ctx.pal[pn][0] & 0b00000111;
        unsigned short g = this->ctx.pal[pn][1] & 0b00000111;
        switch (this->colorMode) {
            case 0: // RGB555
                r = this->bit3to5(r) << 10;
                g = this->bit3to5(g) << 5;
                b = this->bit3to5(b);
                break;
            case 1: // RGB565
                r = this->bit3to5(r) << 11;
                g = this->bit3to6(g) << 5;
                b = this->bit3to5(b);
                break;
            default:
                r = 0;
                g = 0;
                b = 0;
        }
        this->palette[pn] = r | g | b;
    }

    inline const char* where(int addr)
    {
        static const char* answer[] = {
            "UNKNOWN",
            "NameTable",
            "PatternGenerator",
            "ColorTable",
            "SpriteGenerator",
            "SpriteAttributeTable",
            "SpriteColorTable",
        };
        int nt = this->getNameTableAddress();
        int pg = this->getPatternGeneratorAddress();
        int ct = this->getColorTableAddress();
        int sg = this->getSpriteGeneratorTable();
        int sa = this->getSpriteAttributeTableM2();
        int sc = this->getSpriteColorTable();
        if (nt <= addr && addr < nt + this->getNameTableSize()) {
            return answer[1];
        }
        if (pg <= addr && addr < pg + this->getPatternGeneratorSize()) {
            return answer[2];
        }
        if (ct <= addr && addr < ct + this->getColorTableSize()) {
            return answer[3];
        }
        if (sg <= addr && addr < sg + 8 * 256) {
            return answer[4];
        }
        if (sa <= addr && addr < sa + 4 * 32) {
            return answer[5];
        }
        if (sc <= addr && addr < sc + 512) {
            return answer[6];
        }
        return answer[0];
    }

    inline void checkIRQ()
    {
        if (this->isIE0() && this->isF()) {
            this->detectInterrupt(this->arg, 0);
        } else if (this->isIE1() && this->isFH()) {
            this->detectInterrupt(this->arg, 1);
        } else {
            this->cancelInterrupt(this->arg);
        }
    }

    inline void updateAddress()
    {
        this->ctx.addr = this->ctx.tmpAddr[1] & 0b00111111;
        this->ctx.addr <<= 8;
        this->ctx.addr |= this->ctx.tmpAddr[0];
        this->ctx.addr |= ((int)(this->ctx.reg[14] & 0b00000111)) << 14;
    }

    inline void readVideoMemory()
    {
        this->ctx.readBuffer = this->ctx.ram[this->ctx.addr];
        this->incrementAddress();
    }

    inline void incrementAddress()
    {
        this->ctx.addr++;
        this->ctx.addr &= 0x1FFFF;
        unsigned char r14 = (this->ctx.addr >> 14) & 0b00000111;
        if (r14 != this->ctx.reg[14]) {
            this->ctx.reg[14] = r14;
        }
    }

    inline void updateRegister(int rn, unsigned char value)
    {
        value &= this->regMask[rn];
        unsigned char mod = this->ctx.reg[rn] ^ value;
        this->ctx.reg[rn] = value;
        switch (rn) {
            case 0:
                if (mod & 0x10) {
                    this->checkIRQ();
                }
                break;
            case 1:
                if (mod & 0x20) {
                    this->checkIRQ();
                }
                break;
            case 8:
                this->ctx.reg[8] &= 0b00111111; // force disable mouse & light-pen
                break;
            case 9:
                this->updateEventTableV();
                break;
            case 14:
                this->ctx.reg[14] &= 0b00000111;
                this->ctx.addr &= 0x3FFF;
                this->ctx.addr |= this->ctx.reg[14] << 14;
                break;
            /*case 15:
                printf("%3d,%3d update R#%d = $%02X\n",ctx.countV,ctx.counter%100,rn,value);
                break; */
            case 16:
                this->ctx.latch2 = 0;
                break;
            case 18:
                this->updateEventTables();
                break;
            case 19:
                this->ctx.lineIE1 = value;
                this->ctx.lineIE1 -= this->ctx.reg[23];
                this->ctx.lineIE1++;
                break;
            case 23:
                this->ctx.lineIE1 = this->ctx.reg[19];
                this->ctx.lineIE1 -= value;
                this->ctx.lineIE1++;
                break;
            case 44:
                switch (this->ctx.command) {
                    case 0b1111: this->executeCommandHMMC(false); break;
                    case 0b1011: this->executeCommandLMMC(false); break;
                }
                break;
            case 46:
                this->executeCommand((value & 0xF0) >> 4, value & 0x0F);
                break;
        }
    }

    inline void renderScanline(int lineNumber, unsigned short* renderPosition)
    {
        if (0 <= lineNumber && lineNumber < this->getLineNumber()) {
            this->lastRenderScanline = lineNumber;
            if (this->isEnabledScreen()) {
                // 00 000 : GRAPHIC1    256x192             Mode1   chr           16KB
                // 00 001 : GRAPHIC2    256x192             Mode1   chr           16KB
                // 00 010 : GRAPHIC3    256x192             Mode2   chr           16KB
                // 00 011 : GRAPHIC4    256x192 or 256x212  Mode2   bitmap(4bit)  64KB
                // 00 100 : GRAPHIC5    512x192 or 512x212  Mode2   bitmap(2bit)  64KB
                // 00 101 : GRAPHIC6    512x192 or 512x212  Mode2   bitmap(4bit)  128KB
                // 00 111 : GRAPHIC7    256x192 or 256x212  Mode2   bitmap(8bit)  128KB
                // 01 000 : MULTI COLOR 64x48 (4px/block)   Mode1   low bitmap    16KB
                // 10 000 : TEXT1       40x24 (6x8px/block) n/a     chr           16KB
                // 10 010 : TEXT2       80x24 (6x8px/block) n/a     chr           16KB
                switch (this->getScreenMode()) {
                    case 0b00000: // GRAPHIC1
                        this->renderScanlineModeG1(lineNumber, renderPosition);
                        break;
                    case 0b00001: // GRAPHIC2
                        this->renderScanlineModeG23(lineNumber, false, renderPosition);
                        break;
                    case 0b00010: // GRAPHIC3
                        this->renderScanlineModeG23(lineNumber, true, renderPosition);
                        break;
                    case 0b00011: // GRAPHIC4
                        this->renderScanlineModeG4(lineNumber, renderPosition);
                        break;
                    case 0b00100: // GRAPHIC5
                        this->renderScanlineModeG5(lineNumber, renderPosition);
                        break;
                    case 0b00101: // GRAPHIC6
                        this->renderScanlineModeG6(lineNumber, renderPosition);
                        break;
                    case 0b00111: // GRAPHIC7
                        this->renderScanlineModeG7(lineNumber, renderPosition);
                        break;
                    case 0b01000: // MULTI COLOR
                        this->renderScanlineModeMC(lineNumber, renderPosition);
                        break;
                    case 0b10000: // TEXT1
                        this->renderScanlineModeT1(lineNumber, renderPosition);
                        break;
                    case 0b10010: // TEXT2
                        this->renderScanlineModeT2(lineNumber, renderPosition);
                        break;
                    case 0b11011: // ???
                        break;
                    default:
                        printf("UNSUPPORTED SCREEN MODE! (%d)\n", this->getScreenMode());
                        exit(-1);
                        return;
                }
                if (this->isMaskLeft8px()) {
                    auto borderColor = this->getBackdropColor();
                    for (int i = 0; i < 16; i++) {
                        renderPosition[i] = borderColor;
                    }
                }
            } else
                return;
        }
    }

    inline void renderPixel1(unsigned short* renderPosition, int paletteNumber)
    {
        if (0 == (this->ctx.reg[8] & 0b00100000) && !paletteNumber) return;
        *renderPosition = this->palette[paletteNumber];
    }

    inline void renderPixel2(unsigned short* renderPosition, int paletteNumber)
    {
        if (0 == (this->ctx.reg[8] & 0b00100000) && !paletteNumber) return;
        *renderPosition = this->palette[paletteNumber];
        *(renderPosition + 1) = this->palette[paletteNumber];
    }

    inline void renderPixel2S1(unsigned short* renderPosition, int paletteNumber)
    {
        if (!paletteNumber) return;
        *renderPosition = this->palette[paletteNumber];
        *(renderPosition + 1) = this->palette[paletteNumber];
    }

    inline void renderPixel2S2(unsigned short* renderPosition, int paletteNumber)
    {
        if (!paletteNumber || !this->isSpriteDisplay()) return;
        *renderPosition = this->palette[paletteNumber];
        *(renderPosition + 1) = this->palette[paletteNumber];
    }

    inline void renderScanlineModeG1(int lineNumber, unsigned short* renderPosition)
    {
        int pn = this->getNameTableAddress();
        int ct = this->getColorTableAddress();
        int pg = this->getPatternGeneratorAddress();
        int lineNumberS = (lineNumber + this->ctx.reg[23]) & 0xFF;
        int lineNumberMod8 = lineNumberS & 0b111;
        unsigned char* nam = &this->ctx.ram[pn + lineNumberS / 8 * 32];
        int cur = 0;
        for (int i = 0; i < 32; i++) {
            unsigned char ptn = this->ctx.ram[pg + nam[i] * 8 + lineNumberMod8];
            unsigned char c = this->ctx.ram[ct + nam[i] / 8];
            unsigned char cc[2];
            cc[1] = (c & 0xF0) >> 4;
            cc[0] = c & 0x0F;
            this->renderPixel2(&renderPosition[cur], cc[(ptn & 0b10000000) >> 7]);
            cur += 2;
            this->renderPixel2(&renderPosition[cur], cc[(ptn & 0b01000000) >> 6]);
            cur += 2;
            this->renderPixel2(&renderPosition[cur], cc[(ptn & 0b00100000) >> 5]);
            cur += 2;
            this->renderPixel2(&renderPosition[cur], cc[(ptn & 0b00010000) >> 4]);
            cur += 2;
            this->renderPixel2(&renderPosition[cur], cc[(ptn & 0b00001000) >> 3]);
            cur += 2;
            this->renderPixel2(&renderPosition[cur], cc[(ptn & 0b00000100) >> 2]);
            cur += 2;
            this->renderPixel2(&renderPosition[cur], cc[(ptn & 0b00000010) >> 1]);
            cur += 2;
            this->renderPixel2(&renderPosition[cur], cc[ptn & 0b00000001]);
            cur += 2;
        }
        renderSpritesMode1(lineNumber, renderPosition);
    }

    inline void renderScanlineModeG23(int lineNumber, bool isSpriteMode2, unsigned short* renderPosition)
    {
        int sp2 = this->getSP2();
        int x = this->ctx.reg[26] & 0b00111111;
        int cur = this->ctx.reg[27] & 0b00000111;
        int lineNumberS = (lineNumber + this->ctx.reg[23]) & 0xFF;
        int pn = this->getNameTableAddress();
        int ct = this->getColorTableAddress();
        int cmask = this->ctx.reg[3] & 0b01111111;
        cmask <<= 3;
        cmask |= 0x07;
        int pg = this->getPatternGeneratorAddress();
        int pmask = this->ctx.reg[4] & 0b00000011;
        pmask <<= 8;
        pmask |= 0xFF;
        int bd = this->ctx.reg[7] & 0b00001111;
        int pixelLine = lineNumberS % 8;
        unsigned char* nt = &this->ctx.ram[pn + lineNumberS / 8 * 32];
        int ci = lineNumberS / 64 * 256;
        for (int i = 0; i < 32; i++, x++) {
            unsigned char nam;
            if (sp2) {
                if (x < 32) {
                    nam = nt[x];
                } else {
                    nam = nt[1024 + (x & 0x1F)];
                }
            } else {
                nam = nt[x & 0x1F];
            }
            unsigned char ptn = this->ctx.ram[pg + ((nam + ci) & pmask) * 8 + pixelLine];
            unsigned char c = this->ctx.ram[ct + ((nam + ci) & cmask) * 8 + pixelLine];
            unsigned char cc[2];
            cc[1] = (c & 0xF0) >> 4;
            cc[1] = cc[1] ? cc[1] : bd;
            cc[0] = c & 0x0F;
            cc[0] = cc[0] ? cc[0] : bd;
            this->renderPixel2(&renderPosition[cur], cc[(ptn & 0b10000000) >> 7]);
            cur += 2;
            if (512 <= cur) break;
            this->renderPixel2(&renderPosition[cur], cc[(ptn & 0b01000000) >> 6]);
            cur += 2;
            if (512 <= cur) break;
            this->renderPixel2(&renderPosition[cur], cc[(ptn & 0b00100000) >> 5]);
            cur += 2;
            if (512 <= cur) break;
            this->renderPixel2(&renderPosition[cur], cc[(ptn & 0b00010000) >> 4]);
            cur += 2;
            if (512 <= cur) break;
            this->renderPixel2(&renderPosition[cur], cc[(ptn & 0b00001000) >> 3]);
            cur += 2;
            if (512 <= cur) break;
            this->renderPixel2(&renderPosition[cur], cc[(ptn & 0b00000100) >> 2]);
            cur += 2;
            if (512 <= cur) break;
            this->renderPixel2(&renderPosition[cur], cc[(ptn & 0b00000010) >> 1]);
            cur += 2;
            if (512 <= cur) break;
            this->renderPixel2(&renderPosition[cur], cc[ptn & 0b00000001]);
            cur += 2;
            if (512 <= cur) break;
        }
        if (isSpriteMode2) {
            renderSpritesMode2(lineNumber, renderPosition);
        } else {
            renderSpritesMode1(lineNumber, renderPosition);
        }
    }

    inline void renderScanlineModeG4(int lineNumber, unsigned short* renderPosition)
    {
        int curD = this->ctx.reg[27] & 0b00000111;
        int addr = ((lineNumber + this->ctx.reg[23]) & 0xFF) * 128 + this->getNameTableAddress();
        int addr2 = 0;
        int sp2 = this->getSP2();
        int x = this->ctx.reg[26];
        if (sp2) {
            x &= 0b00111111;
            if (x < 32) {
                addr2 = addr;
                addr &= 0x17FFF;
            } else {
                addr |= 0x8000;
                addr2 = addr & 0x17FFF;
            }
        } else {
            x &= 0b00011111;
        }
        x <<= 2;
        if (this->isEvenOrderMode() && addr & 0x8000) {
            addr &= 0x17FFF;
            addr |= (this->ctx.counter & 1) << 15;
        }
        for (int i = 0; i < 128; i++) {
            this->renderPixel2(&renderPosition[curD], (this->ctx.ram[addr + x] & 0xF0) >> 4);
            curD += 2;
            if (512 <= curD) break;
            this->renderPixel2(&renderPosition[curD], this->ctx.ram[addr + x] & 0x0F);
            curD += 2;
            if (512 <= curD) break;
            x++;
            x &= 0x7F;
            if (0 == x && sp2) {
                addr = addr2;
            }
        }
        renderSpritesMode2(lineNumber, renderPosition);
    }

    inline void renderScanlineModeG5(int lineNumber, unsigned short* renderPosition)
    {
        int curD = (this->ctx.reg[27] & 0b00000111) << 1;
        int addr = ((lineNumber + this->ctx.reg[23]) & 0xFF) * 128 + this->getNameTableAddress();
        int sp2 = this->getSP2();
        int x = this->ctx.reg[26];
        if (sp2) {
            x &= 0b00111111;
            if (x < 32) {
                addr &= 0x17FFF;
            } else {
                addr |= 0x8000;
            }
        } else {
            x &= 0b00011111;
        }
        x <<= 2;
        for (int i = 0; i < 128; i++) {
            addr &= 0x1FFFF;
            this->renderPixel1(&renderPosition[curD++], (this->ctx.ram[addr + x] & 0xC0) >> 6);
            if (512 <= curD) break;
            this->renderPixel1(&renderPosition[curD++], (this->ctx.ram[addr + x] & 0x30) >> 4);
            if (512 <= curD) break;
            this->renderPixel1(&renderPosition[curD++], (this->ctx.ram[addr + x] & 0x0C) >> 2);
            if (512 <= curD) break;
            this->renderPixel1(&renderPosition[curD++], this->ctx.ram[addr + x] & 0x03);
            if (512 <= curD) break;
            x++;
            x &= 0x7F;
            if (0 == x && sp2) {
                addr ^= 0x8000;
            }
        }
        renderSpritesMode2(lineNumber, renderPosition);
    }

    inline void renderScanlineModeG6(int lineNumber, unsigned short* renderPosition)
    {
        int curD = (this->ctx.reg[27] & 0b00000111) << 1;
        int addr = ((lineNumber + this->ctx.reg[23]) & 0xFF) * 256 + this->getNameTableAddress();
        int sp2 = this->getSP2();
        int x = this->ctx.reg[26];
        if (this->isEvenOrderMode() && addr & 0x10000) {
            addr &= 0xFFFF;
            addr |= (this->ctx.counter & 1) << 16;
        }
        if (sp2) {
            x &= 0b00111111;
            if (x < 32) {
                addr &= 0x0FFFF;
            } else {
                addr |= 0x10000;
            }
        } else {
            x &= 0b00011111;
        }
        x <<= 3;
        for (int i = 0; i < 256 && curD < 512; i++) {
            this->renderPixel1(&renderPosition[curD++], (this->ctx.ram[addr + x] & 0xF0) >> 4);
            this->renderPixel1(&renderPosition[curD++], this->ctx.ram[addr + x] & 0x0F);
            x++;
            x &= 0xFF;
            addr ^= 0 == x && sp2 ? 0x10000 : 0;
        }
        renderSpritesMode2(lineNumber, renderPosition);
    }

    inline unsigned short convertColor_8bit_to_16bit(unsigned char c)
    {
        unsigned short g = (c & 0b11100000) >> 5;
        unsigned short r = (c & 0b00011100) >> 2;
        unsigned short b = c & 0b00000011;
        switch (this->colorMode) {
            case 0: {
                r = this->bit3to5(r) << 10;
                g = this->bit3to5(g) << 5;
                b = this->bit2to5(b);
                return r | g | b;
            }
            case 1: {
                r = this->bit3to5(r) << 11;
                g = this->bit3to6(g) << 5;
                b = this->bit2to5(b);
                return r | g | b;
            }
            default: return 0;
        }
    }

    inline void renderScanlineModeG7(int lineNumber, unsigned short* renderPosition)
    {
        int curD = 0;
        int curP = ((lineNumber + this->ctx.reg[23]) & 0xFF) * 256;
        curP += this->getNameTableAddress();
        if (this->isYJK()) {
            for (int i = 0; i < 256; i += 4) {
                unsigned char y[4];
                y[0] = this->ctx.ram[curP++];
                y[1] = this->ctx.ram[curP++];
                y[2] = this->ctx.ram[curP++];
                y[3] = this->ctx.ram[curP++];
                unsigned char k = (y[1] & 0b00000111) << 3;
                k |= y[0] & 0b00000111;
                unsigned char j = (y[3] & 0b00000111) << 3;
                j |= y[2] & 0b00000111;
                for (int n = 0; n < 4; n++) {
                    y[n] &= 0b11111000;
                    y[n] >>= 3;
                    if (this->isYAE() && (y[n] & 1)) {
                        renderPosition[curD] = this->palette[(y[n] >> 1) & 0x0F];
                    } else {
                        renderPosition[curD] = this->yjkColor[y[n]][j][k];
                    }
                    renderPosition[curD + 1] = renderPosition[curD];
                    curD += 2;
                }
            }
        } else {
            for (int i = 0; i < 256; i++) {
                renderPosition[curD] = convertColor_8bit_to_16bit(this->ctx.ram[curP++]);
                renderPosition[curD + 1] = renderPosition[curD];
                curD += 2;
            }
        }
        renderSpritesMode2(lineNumber, renderPosition);
    }

    inline void renderScanlineModeMC(int lineNumber, unsigned short* renderPosition)
    {
        int pn = this->getNameTableAddress();
        int pg = this->getPatternGeneratorAddress();
        unsigned char* nam = &this->ctx.ram[pn + lineNumber / 8 * 32];
        int cur = 0;
        for (int i = 0; i < 32; i++) {
            unsigned char ptn = this->ctx.ram[pg + nam[i] * 8 + lineNumber % 32 / 4];
            unsigned char col[2];
            col[0] = (ptn & 0xF0) >> 4;
            col[1] = ptn & 0x0F;
            for (int k = 0; k < 8; k++) {
                this->renderPixel2(&renderPosition[cur], col[k / 4]);
                cur += 2;
            }
        }
        renderSpritesMode1(lineNumber, renderPosition);
    }

    inline void renderScanlineModeT1(int lineNumber, unsigned short* renderPosition)
    {
        int pn = this->getNameTableAddress();
        int pg = this->getPatternGeneratorAddress();
        int lineNumberS = (lineNumber + this->ctx.reg[23]) & 0xFF;
        int lineNumberMod8 = lineNumberS & 0b111;
        unsigned char* nam = &this->ctx.ram[pn + lineNumberS / 8 * 40];
        int cur = 0;
        for (int i = 0; i < 40; i++) {
            unsigned char ptn = this->ctx.ram[pg + nam[i] * 8 + lineNumberMod8];
            this->renderPixel2(&renderPosition[cur], ptn & 0b10000000 ? (this->ctx.reg[7] & 0xF0) >> 4 : this->ctx.reg[7] & 0x0F);
            cur += 2;
            this->renderPixel2(&renderPosition[cur], ptn & 0b01000000 ? (this->ctx.reg[7] & 0xF0) >> 4 : this->ctx.reg[7] & 0x0F);
            cur += 2;
            this->renderPixel2(&renderPosition[cur], ptn & 0b00100000 ? (this->ctx.reg[7] & 0xF0) >> 4 : this->ctx.reg[7] & 0x0F);
            cur += 2;
            this->renderPixel2(&renderPosition[cur], ptn & 0b00010000 ? (this->ctx.reg[7] & 0xF0) >> 4 : this->ctx.reg[7] & 0x0F);
            cur += 2;
            this->renderPixel2(&renderPosition[cur], ptn & 0b00001000 ? (this->ctx.reg[7] & 0xF0) >> 4 : this->ctx.reg[7] & 0x0F);
            cur += 2;
            this->renderPixel2(&renderPosition[cur], ptn & 0b00000100 ? (this->ctx.reg[7] & 0xF0) >> 4 : this->ctx.reg[7] & 0x0F);
            cur += 2;
        }
    }

    inline void renderScanlineModeT2(int lineNumber, unsigned short* renderPosition)
    {
        int pn = this->getNameTableAddress();
        int pg = this->getPatternGeneratorAddress();
        int lineNumberS = (lineNumber + this->ctx.reg[23]) & 0xFF;
        int lineNumberMod8 = lineNumberS & 0b111;
        unsigned char* nam = &this->ctx.ram[pn + lineNumberS / 8 * 80];
        int cur = 0;
        for (int i = 0; i < 80; i++) {
            unsigned char ptn = this->ctx.ram[pg + nam[i] * 8 + lineNumberMod8];
            this->renderPixel1(&renderPosition[cur++], ptn & 0b10000000 ? (this->ctx.reg[7] & 0xF0) >> 4 : this->ctx.reg[7] & 0x0F);
            this->renderPixel1(&renderPosition[cur++], ptn & 0b01000000 ? (this->ctx.reg[7] & 0xF0) >> 4 : this->ctx.reg[7] & 0x0F);
            this->renderPixel1(&renderPosition[cur++], ptn & 0b00100000 ? (this->ctx.reg[7] & 0xF0) >> 4 : this->ctx.reg[7] & 0x0F);
            this->renderPixel1(&renderPosition[cur++], ptn & 0b00010000 ? (this->ctx.reg[7] & 0xF0) >> 4 : this->ctx.reg[7] & 0x0F);
            this->renderPixel1(&renderPosition[cur++], ptn & 0b00001000 ? (this->ctx.reg[7] & 0xF0) >> 4 : this->ctx.reg[7] & 0x0F);
            this->renderPixel1(&renderPosition[cur++], ptn & 0b00000100 ? (this->ctx.reg[7] & 0xF0) >> 4 : this->ctx.reg[7] & 0x0F);
        }
    }

    inline void renderSpritesMode1(int lineNumber, unsigned short* renderPosition)
    {
        static const unsigned char bit[8] = {
            0b10000000,
            0b01000000,
            0b00100000,
            0b00010000,
            0b00001000,
            0b00000100,
            0b00000010,
            0b00000001};
        int si = this->isSprite16px() ? 16 : 8;
        int mag = this->isSprite2x() ? 2 : 1;
        int sa = this->getSpriteAttributeTableM1();
        int sg = this->getSpriteGeneratorTable();
        int sn = 0;
        int tsn = 0;
        unsigned char dlog[256];
        unsigned char wlog[256];
        memset(dlog, 0, sizeof(dlog));
        memset(wlog, 0, sizeof(wlog));
        bool limitOver = false;
        for (int i = 0; i < 32; i++) {
            int cur = sa + i * 4;
            unsigned char y = this->ctx.ram[cur++];
            if (208 == y) break;
            int x = this->ctx.ram[cur++];
            unsigned char ptn = this->ctx.ram[cur++];
            unsigned char col = this->ctx.ram[cur++];
            if (col & 0x80) x -= 32;
            col &= 0b00001111;
            y += 1 + this->ctx.reg[23];
            if (y <= lineNumber && lineNumber < y + si * mag) {
                sn++;
                if (!col) tsn++;
                if (5 == sn) {
                    this->set5S(true, i);
                    if (!this->renderLimitOverSprites) {
                        break;
                    } else {
                        if (4 <= tsn) break;
                        limitOver = true;
                    }
                } else if (sn < 5) {
                    this->set5S(false, i);
                }
                int pixelLine = lineNumber - y;
                if (16 == si) {
                    cur = sg + (ptn & 252) * 8 + pixelLine % (8 * mag) / mag + (pixelLine < 8 * mag ? 0 : 8);
                } else {
                    cur = sg + ptn * 8 + lineNumber % (8 * mag) / mag;
                }
                for (int j = 0; x < 256 && j < 8 * mag; j++, x++) {
                    if (x < 0) continue;
                    if (wlog[x] && !limitOver) {
                        this->setCollision(x, lineNumber);
                    }
                    if (0 == dlog[x]) {
                        if (this->ctx.ram[cur] & bit[j / mag]) {
                            this->renderPixel2S1(&renderPosition[x << 1], col);
                            dlog[x] = col;
                            wlog[x] = 1;
                        }
                    }
                }
                if (16 == si) {
                    cur += 16 * mag;
                    for (int j = 0; x < 256 && j < 8 * mag; j++, x++) {
                        if (x < 0) continue;
                        if (wlog[x] && !limitOver) {
                            this->setCollision(x, lineNumber);
                        }
                        if (0 == dlog[x]) {
                            if (this->ctx.ram[cur] & bit[j / mag]) {
                                this->renderPixel2S1(&renderPosition[x << 1], col);
                                dlog[x] = col;
                                wlog[x] = 1;
                            }
                        }
                    }
                }
            }
        }
    }

    inline void renderSpritesMode2(int lineNumber, unsigned short* renderPosition)
    {
        static const unsigned char bit[8] = {
            0b10000000,
            0b01000000,
            0b00100000,
            0b00010000,
            0b00001000,
            0b00000100,
            0b00000010,
            0b00000001};
        int si = this->isSprite16px() ? 16 : 8;
        int mag = this->isSprite2x() ? 2 : 1;
        int sa = this->getSpriteAttributeTableM2();
        int ct = this->getSpriteColorTable();
        int sg = this->getSpriteGeneratorTable();
        int sn = 0;
        int tsn = 0;
        unsigned char paletteMask = this->getScreenMode() == 0b00100 ? 0x03 : 0x0F;
        unsigned char dlog[256];
        unsigned char wlog[256];
        unsigned char skip[256];
        memset(dlog, 0, sizeof(dlog));
        memset(wlog, 0, sizeof(wlog));
        memset(skip, 0, sizeof(skip));
        bool limitOver = false;
        for (int i = 0; i < 32; i++, ct += 16) {
            int cur = sa + i * 4;
            unsigned char y = this->ctx.ram[cur++];
            if (216 == y) break;
            int x = this->ctx.ram[cur++];
            unsigned char ptn = this->ctx.ram[cur++];
            y += 1 - this->ctx.reg[23];
            if (y <= lineNumber && lineNumber < y + si * mag) {
                sn++;
                int pixelLine = lineNumber - y;
                unsigned char col = this->ctx.ram[ct + pixelLine / mag];
                bool ic = col & 0x20;
                bool cc = col & 0x40;
                if (col & 0x80) x -= 32;
                col &= paletteMask;
                if (!col) tsn++;
                if (9 == sn) {
                    this->set5S(true, i);
                    if (!this->renderLimitOverSprites) {
                        break;
                    } else {
                        if (8 <= tsn) break;
                        limitOver = true;
                    }
                } else if (sn < 9) {
                    this->set5S(false, i);
                }
                if (16 == si) {
                    cur = sg + (ptn & 252) * 8 + pixelLine % (8 * mag) / mag + (pixelLine < 8 * mag ? 0 : 8);
                } else {
                    cur = sg + ptn * 8 + lineNumber % (8 * mag) / mag;
                }
                for (int j = 0; x < 256 && j < 8 * mag; j++, x++) {
                    if (x < 0) continue;
                    if (dlog[x] && !limitOver && !ic) {
                        this->setCollision(x, lineNumber);
                    }
                    if (0 == wlog[x] || cc) {
                        if (this->ctx.ram[cur] & bit[j / mag]) {
                            if (cc) {
                                if (!skip[x]) {
                                    this->renderPixel2S2(&renderPosition[x << 1], dlog[x] | col);
                                }
                            } else {
                                this->renderPixel2S2(&renderPosition[x << 1], col);
                                dlog[x] = col;
                            }
                            wlog[x] = 1;
                        }
                    } else {
                        skip[x] = 1;
                    }
                }
                if (16 == si) {
                    cur += 16;
                    for (int j = 0; x < 256 && j < 8 * mag; j++, x++) {
                        if (x < 0) continue;
                        if (dlog[x] && !limitOver && !ic) {
                            this->setCollision(x, lineNumber);
                        }
                        if (0 == wlog[x] || cc) {
                            if (this->ctx.ram[cur] & bit[j / mag]) {
                                if (cc) {
                                    if (!skip[x]) {
                                        this->renderPixel2S2(&renderPosition[x << 1], dlog[x] | col);
                                    }
                                } else {
                                    this->renderPixel2S2(&renderPosition[x << 1], col);
                                    dlog[x] = col;
                                }
                                wlog[x] = 1;
                            }
                        } else {
                            skip[x] = 1;
                        }
                    }
                }
            }
        }
    }

    inline void executeCommand(int cm, int lo)
    {
        if (cm) {
            this->setCE();
            this->ctx.command = cm;
            this->ctx.commandL = lo;
            switch (cm) {
                case 0b1111: this->executeCommandHMMC(true); break;
                case 0b1110: this->executeCommandYMMM(); break;
                case 0b1101: this->executeCommandHMMM(true); break;
                case 0b1100: this->executeCommandHMMV(true); break;
                case 0b1011: this->executeCommandLMMC(true); break;
                case 0b1001: this->executeCommandLMMM(true); break;
                case 0b1000: this->executeCommandLMMV(true); break;
                case 0b1010: this->executeCommandLMCM(true); break;
                case 0b0111: this->executeCommandLINE(true); break;
                case 0b0110: this->executeCommandSRCH(true); break;
                case 0b0101: this->executeCommandPSET(true); break;
                case 0b0100: this->executeCommandPOINT(true); break;
                default:
                    printf("UNKNOWN COMMAND: %d\n", cm);
                    exit(-1);
            }
        } else {
            this->ctx.cmd.wait = 0;
            this->setCommandEnd();
        }
    }

    inline int getDotPerByteX()
    {
        switch (this->getScreenMode()) {
            case 0b00011: return 2; // GRAPHIC4
            case 0b00100: return 4; // GRAPHIC5
            case 0b00101: return 2; // GRAPHIC6
            case 0b00111: return 1; // GRAPHIC7
            default: return 0;
        }
    }

    inline int getScreenWidth()
    {
        switch (this->getScreenMode()) {
            case 0b00000: return 256; // GRAPHIC1
            case 0b00001: return 256; // GRAPHIC2
            case 0b00010: return 256; // GRAPHIC3
            case 0b00011: return 256; // GRAPHIC4
            case 0b00100: return 512; // GRAPHIC5
            case 0b00101: return 512; // GRAPHIC6
            case 0b00111: return 256; // GRAPHIC7
            default: return 0;
        }
    }

    inline bool isBitmapMode()
    {
        switch (this->getScreenMode()) {
            case 0b00011: return true; // GRAPHIC4
            case 0b00100: return true; // GRAPHIC5
            case 0b00101: return true; // GRAPHIC6
            case 0b00111: return true; // GRAPHIC7
            default: return false;
        }
    }

    inline unsigned short getSX(unsigned short ignore = 0xFFFF)
    {
        unsigned short result = this->ctx.reg[33] & 1;
        result <<= 8;
        result |= this->ctx.reg[32];
        if (0xFFFF != ignore) {
            result &= ignore;
            this->setSX(result);
        }
        return result;
    }

    inline void setSX(int sx)
    {
        this->ctx.reg[33] = (sx & 0x100) >> 8;
        this->ctx.reg[32] = sx & 0xFF;
    }

    inline unsigned short getSY()
    {
        unsigned short result = this->ctx.reg[35] & 3;
        result <<= 8;
        result |= this->ctx.reg[34];
        return result;
    }

    inline void setSY(int sy)
    {
        this->ctx.reg[35] = (sy & 0x300) >> 8;
        this->ctx.reg[34] = sy & 0xFF;
    }

    inline unsigned short getDX(unsigned short ignore = 0xFFFF)
    {
        unsigned short result = this->ctx.reg[37] & 1;
        result <<= 8;
        result |= this->ctx.reg[36];
        if (0xFFFF != ignore) {
            result &= ignore;
            this->setDX(result);
        }
        return result;
    }

    inline void setDX(int dx)
    {
        this->ctx.reg[37] = (dx & 0x100) >> 8;
        this->ctx.reg[36] = dx & 0xFF;
    }

    inline unsigned short getDY()
    {
        unsigned short result = this->ctx.reg[39] & 3;
        result <<= 8;
        result |= this->ctx.reg[38];
        return result;
    }

    inline void setDY(int dy)
    {
        this->ctx.reg[39] = (dy & 0x300) >> 8;
        this->ctx.reg[38] = dy & 0xFF;
    }

    inline void setNX(int nx) { this->setMAJ(nx); }
    inline unsigned short getNX(unsigned short ignore = 0xFFFF)
    {
        unsigned short nx = this->getMAJ();
        if (ignore != 0xFFFF) {
            nx &= ignore;
            this->setNX(nx);
        }
        return 0 == nx ? 512 : nx;
    }

    inline void setNY(int ny) { this->setMIN(ny); }
    inline unsigned short getNY()
    {
        unsigned short ny = this->getMIN();
        return 0 == ny ? 1024 : ny;
    }

    inline unsigned short getMAJ()
    {
        unsigned short result = this->ctx.reg[41] & 1;
        result <<= 8;
        result |= this->ctx.reg[40];
        return result;
    }

    inline void setMAJ(int maj)
    {
        this->ctx.reg[41] = (maj & 0x100) >> 8;
        this->ctx.reg[40] = maj & 0xFF;
    }

    inline unsigned short getMIN()
    {
        unsigned short result = this->ctx.reg[43] & 3;
        result <<= 8;
        result |= this->ctx.reg[42];
        return result;
    }

    inline void setMIN(int min)
    {
        this->ctx.reg[43] = (min & 0x300) >> 8;
        this->ctx.reg[42] = min & 0xFF;
    }

    inline short getIgnoreMask()
    {
        switch (this->getScreenMode()) {
            case 0b00011: // GRAPHIC4
            case 0b00101: // GRAPHIC6
                return 0xFFFE;
            case 0b00100: // GRAPHIC5
                return 0xFFFC;
            default:
                return 0xFFFF;
        }
    }

    inline int getEQ() { return this->ctx.reg[45] & 0b00000010 ? 1 : 0; }
    inline int getDIX() { return this->ctx.reg[45] & 0b00000100 ? -1 : 1; }
    inline int getDIY() { return this->ctx.reg[45] & 0b00001000 ? -1 : 1; }
    inline int abs(int n) { return n < 0 ? -n : n; }
    inline void addCommandWait(int wait) { this->ctx.cmd.wait += wait; }

    inline void setCommandEnd()
    {
        this->ctx.command = 0;
        this->resetCE();
        this->resetTR();
    }

    inline void commandMoveD(int waitY)
    {
        this->ctx.cmd.dx += this->ctx.cmd.dix;
        this->ctx.cmd.nx -= this->abs(this->ctx.cmd.dix);
        if (this->ctx.cmd.nx <= 0 || this->getScreenWidth() <= this->ctx.cmd.dx || this->ctx.cmd.dx < 0) {
            this->ctx.cmd.dx = this->getDX();
            this->ctx.cmd.nx = this->getNX();
            this->ctx.cmd.dy += this->ctx.cmd.diy;
            this->ctx.cmd.ny--;
            if (this->ctx.cmd.ny <= 0 || 1024 <= this->ctx.cmd.dy || this->ctx.cmd.dy < 0) {
                this->setDX(this->ctx.cmd.dx);
                this->setDY(this->ctx.cmd.dy);
                this->setNX(this->ctx.cmd.nx);
                this->setNY(this->ctx.cmd.ny);
                this->setCommandEnd();
            } else {
                this->addCommandWait(waitY);
            }
        }
    }

    inline void commandMoveS(int waitY)
    {
        this->ctx.cmd.sx += this->ctx.cmd.dix;
        this->ctx.cmd.nx -= this->abs(this->ctx.cmd.dix);
        if (this->ctx.cmd.nx <= 0 || this->getScreenWidth() <= this->ctx.cmd.sx || this->ctx.cmd.sx < 0) {
            this->ctx.cmd.sx = this->getSX();
            this->ctx.cmd.nx = this->getNX();
            this->ctx.cmd.sy += this->ctx.cmd.diy;
            this->ctx.cmd.ny--;
            if (this->ctx.cmd.ny <= 0 || 1024 <= this->ctx.cmd.sy || this->ctx.cmd.sy < 0) {
                this->setSX(this->ctx.cmd.sx);
                this->setSY(this->ctx.cmd.sy);
                this->setNX(this->ctx.cmd.nx);
                this->setNY(this->ctx.cmd.ny);
                this->setCommandEnd();
            } else {
                this->addCommandWait(waitY);
            }
        }
    }

    inline void commandMoveDS(int waitY)
    {
        this->ctx.cmd.dx += this->ctx.cmd.dix;
        this->ctx.cmd.sx += this->ctx.cmd.dix;
        this->ctx.cmd.nx -= this->abs(this->ctx.cmd.dix);
        int w = this->getScreenWidth();
        if (this->ctx.cmd.nx <= 0 || w <= this->ctx.cmd.dx || this->ctx.cmd.dx < 0 || w <= this->ctx.cmd.sx || this->ctx.cmd.sx < 0) {
            this->ctx.cmd.dx = this->getDX();
            this->ctx.cmd.sx = this->getSX();
            this->ctx.cmd.nx = this->getNX();
            this->ctx.cmd.dy += this->ctx.cmd.diy;
            this->ctx.cmd.sy += this->ctx.cmd.diy;
            this->ctx.cmd.ny--;
            if (this->ctx.cmd.ny <= 0 || 1024 <= this->ctx.cmd.dy || this->ctx.cmd.dy < 0 || 1024 <= this->ctx.cmd.sy || this->ctx.cmd.sy < 0) {
                this->setDX(this->ctx.cmd.dx);
                this->setDY(this->ctx.cmd.dy);
                this->setSX(this->ctx.cmd.sx);
                this->setSY(this->ctx.cmd.sy);
                this->setNX(this->ctx.cmd.nx);
                this->setNY(this->ctx.cmd.ny);
                this->setCommandEnd();
            } else {
                this->addCommandWait(waitY);
            }
        }
    }

    inline void executeCommandHMMC(bool setup)
    {
        if (!this->isBitmapMode()) {
            printf("Error: HMMC was executed in invalid screen mode (%d)\n", this->getScreenMode());
            exit(-1);
        }
        int screenWidth = this->getScreenWidth();
        int dpb = this->getDotPerByteX();
        int lineBytes = screenWidth / dpb;
        if (setup) {
            auto ignore = this->getIgnoreMask();
            this->ctx.cmd.dx = this->getDX(ignore);
            this->ctx.cmd.dy = this->getDY();
            this->ctx.cmd.nx = this->getNX(ignore);
            this->ctx.cmd.ny = this->getNY();
            this->ctx.cmd.diy = this->getDIY();
            this->ctx.cmd.dix = dpb * this->getDIX();
#ifdef COMMAND_DEBUG
            printf("%d: ExecuteCommand<HMMC>: DX=%d, DY=%d, NX=%d, NY=%d, DIX=%d, DIY=%d, VAL=$%02X (SCREEN: %d)\n", ctx.countV, ctx.cmd.dx, ctx.cmd.dy, ctx.cmd.nx, ctx.cmd.ny, ctx.cmd.dix, ctx.cmd.diy, ctx.reg[44], getScreenMode());
#endif
        }
        int addr = this->ctx.cmd.dx / dpb + this->ctx.cmd.dy * lineBytes;
        this->ctx.ram[addr] = this->ctx.reg[44];
        this->commandMoveD(0);
        if (this->ctx.command) {
            this->setTR(); // set TR if keep executing
        }
    }

    inline void executeCommandYMMM()
    {
        if (!this->isBitmapMode()) {
            printf("Error: YMMM was executed in invalid screen mode (%d)\n", this->getScreenMode());
            exit(-1);
        }
        int screenWidth = this->getScreenWidth();
        int dpb = this->getDotPerByteX();
        int lineBytes = screenWidth / dpb;
        auto ignore = this->getIgnoreMask();
        int sy = this->getSY();
        int dx = this->getDX(ignore);
        int dy = this->getDY();
        int ny = this->getNY();
        int diy = this->getDIY();
        int dix = this->getDIX();
#ifdef COMMAND_DEBUG
        printf("%d: ExecuteCommand<YMMM>: SY=%d, DX=%d, DY=%d, NY=%d, DIX=%d, DIY=%d (SCREEN: %d)\n", ctx.countV, sy, dx, dy, ny, dix, diy, getScreenMode());
#endif
        int addrS = dx / dpb + sy * lineBytes;
        int addrD = dx / dpb + dy * lineBytes;
        int base = 0 < dix ? 0 : dx / dpb;
        addrS -= base;
        addrD -= base;
        int size = 0 < dix ? lineBytes - dx / dpb : dx / dpb;
        while (0 < ny) {
            this->addCommandWait(40);
            memmove(&ctx.ram[addrD], &ctx.ram[addrS], size);
            ny--;
            addrS += diy * lineBytes;
            addrD += diy * lineBytes;
        }
        this->setCommandEnd();
    }

    inline void executeCommandHMMM(bool setup)
    {
        if (!this->isBitmapMode()) {
            printf("Error: HMMM was executed in invalid screen mode (%d)\n", this->getScreenMode());
            exit(-1);
        }
        int screenWidth = this->getScreenWidth();
        int dpb = this->getDotPerByteX();
        int lineBytes = screenWidth / dpb;
        if (setup) {
            auto ignore = this->getIgnoreMask();
            this->ctx.cmd.sx = this->getSX(ignore);
            this->ctx.cmd.sy = this->getSY();
            this->ctx.cmd.dx = this->getDX(ignore);
            this->ctx.cmd.dy = this->getDY();
            this->ctx.cmd.nx = this->getNX(ignore);
            this->ctx.cmd.ny = this->getNY();
            this->ctx.cmd.diy = this->getDIY();
            this->ctx.cmd.dix = dpb * this->getDIX();
#ifdef COMMAND_DEBUG
            printf("%d: ExecuteCommand<HMMM>: SX=%d, SY=%d, DX=%d, DY=%d, NX=%d, NY=%d, DIX=%d, DIY=%d (SCREEN: %d)\n", ctx.countV, ctx.cmd.sx, ctx.cmd.sy, ctx.cmd.dx, ctx.cmd.dy, ctx.cmd.nx, ctx.cmd.ny, ctx.cmd.dix, ctx.cmd.diy, getScreenMode());
#endif
        }
        int addrS = this->ctx.cmd.sx / dpb + this->ctx.cmd.sy * lineBytes;
        int addrD = this->ctx.cmd.dx / dpb + this->ctx.cmd.dy * lineBytes;
        unsigned char d = this->ctx.ram[addrS];
        this->addCommandWait(64);
        this->ctx.ram[addrD] = d;
        this->addCommandWait(24);
        this->commandMoveDS(64);
    }

    inline void executeCommandHMMV(bool setup)
    {
        if (!this->isBitmapMode()) {
            printf("Error: HMMV was executed in invalid screen mode (%d)\n", this->getScreenMode());
            exit(-1);
        }
        int screenWidth = this->getScreenWidth();
        int dpb = this->getDotPerByteX();
        int lineBytes = screenWidth / dpb;
        if (setup) {
            auto ignore = this->getIgnoreMask();
            this->ctx.cmd.dx = this->getDX(ignore);
            this->ctx.cmd.dy = this->getDY();
            this->ctx.cmd.nx = this->getNX(ignore);
            this->ctx.cmd.ny = this->getNY();
            this->ctx.cmd.diy = this->getDIY();
            this->ctx.cmd.dix = dpb * this->getDIX();
#ifdef COMMAND_DEBUG
            printf("%d: ExecuteCommand<HMMV>: DX=%d, DY=%d, NX=%d, NY=%d, DIX=%d, DIY=%d, CLR=$%02X (SCREEN: %d)\n", ctx.countV, ctx.cmd.dx, ctx.cmd.dy, ctx.cmd.nx, ctx.cmd.ny, ctx.cmd.dix, ctx.cmd.diy, this->ctx.reg[44], getScreenMode());
#endif
        }
        int addr = this->ctx.cmd.dx / dpb + this->ctx.cmd.dy * lineBytes;
        this->ctx.ram[addr & 0x1FFFF] = this->ctx.reg[44];
        this->addCommandWait(48);
        this->commandMoveD(56);
    }

    inline unsigned char readLogicalPixel(int addr, int dpb, int sx)
    {
        unsigned char src = this->ctx.ram[addr & 0x1FFFF];
        switch (dpb) {
            case 1: return src;
            case 2: return sx & 1 ? src & 0x0F : (src & 0xF0) >> 4;
            case 4:
                switch (sx & 3) {
                    case 3: return src & 0b00000011;
                    case 2: return (src & 0b00001100) >> 2;
                    case 1: return (src & 0b00110000) >> 4;
                    default: return (src & 0b11000000) >> 6;
                }
            default: return 0;
        }
    }

    inline void renderLogicalPixel(int addr, int dpb, int dx, int clr, int lo)
    {
        if (clr || 0 == (lo & 0b1000)) {
            switch (dpb) {
                case 1:
                    switch (lo & 0b0111) {
                        case 0b0000: this->ctx.ram[addr & 0x1FFFF] = clr; break;        // IMP
                        case 0b0001: this->ctx.ram[addr & 0x1FFFF] &= clr; break;       // AND
                        case 0b0010: this->ctx.ram[addr & 0x1FFFF] |= clr; break;       // OR
                        case 0b0011: this->ctx.ram[addr & 0x1FFFF] ^= clr; break;       // EOR
                        case 0b0100: this->ctx.ram[addr & 0x1FFFF] = 0xFF ^ clr; break; // NOT
                    }
                    break;
                case 2: {
                    clr &= 0x0F;
                    unsigned char src = this->ctx.ram[addr & 0x1FFFF];
                    if (dx & 1) {
                        src &= 0x0F;
                        this->ctx.ram[addr & 0x1FFFF] &= 0xF0;
                    } else {
                        src &= 0xF0;
                        src >>= 4;
                        this->ctx.ram[addr & 0x1FFFF] &= 0x0F;
                    }
                    switch (lo & 0b0111) {
                        case 0b0000: src = clr; break;        // IMP
                        case 0b0001: src &= clr; break;       // AND
                        case 0b0010: src |= clr; break;       // OR
                        case 0b0011: src ^= clr; break;       // EOR
                        case 0b0100: src = 0xFF ^ clr; break; // NOT
                    }
                    src &= 0x0F;
                    if (dx & 1) {
                        this->ctx.ram[addr & 0x1FFFF] |= src;
                    } else {
                        src <<= 4;
                        this->ctx.ram[addr & 0x1FFFF] |= src;
                    }
                    break;
                }
                case 4: {
                    clr &= 0x03;
                    unsigned char src = this->ctx.ram[addr & 0x1FFFF];
                    switch (dx & 3) {
                        case 3:
                            src &= 0b00000011;
                            this->ctx.ram[addr & 0x1FFFF] &= 0b11111100;
                            break;
                        case 2:
                            src &= 0b00001100;
                            src >>= 2;
                            this->ctx.ram[addr & 0x1FFFF] &= 0b11110011;
                            break;
                        case 1:
                            src &= 0b00110000;
                            src >>= 4;
                            this->ctx.ram[addr & 0x1FFFF] &= 0b11001111;
                            break;
                        case 0:
                            src &= 0b11000000;
                            src >>= 6;
                            this->ctx.ram[addr & 0x1FFFF] &= 0b00111111;
                            break;
                    }
                    switch (lo & 0b0111) {
                        case 0b0000: src = clr; break;        // IMP
                        case 0b0001: src &= clr; break;       // AND
                        case 0b0010: src |= clr; break;       // OR
                        case 0b0011: src ^= clr; break;       // EOR
                        case 0b0100: src = 0xFF ^ clr; break; // NOT
                    }
                    src &= 0b00000011;
                    switch (dx & 3) {
                        case 3: break;
                        case 2: src <<= 2; break;
                        case 1: src <<= 4; break;
                        case 0: src <<= 6; break;
                    }
                    this->ctx.ram[addr & 0x1FFFF] |= src;
                    break;
                }
            }
        }
    }

    inline void executeCommandLMMC(bool setup)
    {
        if (!this->isBitmapMode()) {
            printf("Error: LMMC was executed in invalid screen mode (%d)\n", this->getScreenMode());
            exit(-1);
        }
        int dpb = this->getDotPerByteX();
        int lineBytes = this->getScreenWidth() / dpb;
        if (setup) {
            this->ctx.cmd.dx = this->getDX();
            this->ctx.cmd.dy = this->getDY();
            this->ctx.cmd.nx = this->getNX();
            this->ctx.cmd.ny = this->getNY();
            this->ctx.cmd.diy = this->getDIY();
            this->ctx.cmd.dix = this->getDIX();
#ifdef COMMAND_DEBUG
            printf("%d: ExecuteCommand<LMMC>: DX=%d, DY=%d, NX=%d, NY=%d, DIX=%d, DIY=%d, VAL=$%02X, LO=%X (SCREEN: %d)\n", ctx.countV, ctx.cmd.dx, ctx.cmd.dy, ctx.cmd.nx, ctx.cmd.ny, ctx.cmd.dix, ctx.cmd.diy, ctx.reg[44], ctx.commandL, getScreenMode());
#endif
        }
        int addr = this->ctx.cmd.dx / dpb + this->ctx.cmd.dy * lineBytes;
        renderLogicalPixel(addr, dpb, this->ctx.cmd.dx, this->ctx.reg[44], this->ctx.commandL);
        this->commandMoveD(0);
        if (this->ctx.command) {
            this->setTR(); // set TR if keep executing
        }
    }

    inline void executeCommandLMCM(bool setup)
    {
        if (!this->isBitmapMode()) {
            printf("Error: LMCM was executed in invalid screen mode (%d)\n", this->getScreenMode());
            exit(-1);
        }
        int dpb = this->getDotPerByteX();
        int lineBytes = this->getScreenWidth() / dpb;
        if (setup) {
            this->ctx.cmd.sx = this->getSX();
            this->ctx.cmd.sy = this->getSY();
            this->ctx.cmd.nx = this->getNX();
            this->ctx.cmd.ny = this->getNY();
            this->ctx.cmd.diy = this->getDIY();
            this->ctx.cmd.dix = this->getDIX();
#ifdef COMMAND_DEBUG
            printf("%d: ExecuteCommand<LMCM>: SX=%d, SY=%d, NX=%d, NY=%d, DIX=%d, DIY=%d, LO=%X (SCREEN: %d)\n", ctx.countV, ctx.cmd.sx, ctx.cmd.sy, ctx.cmd.nx, ctx.cmd.ny, ctx.cmd.dix, ctx.cmd.diy, ctx.commandL, getScreenMode());
#endif
        }
        int addr = this->ctx.cmd.sx / dpb + this->ctx.cmd.sy * lineBytes;
        this->ctx.stat[7] = this->readLogicalPixel(addr, dpb, ctx.cmd.sx);
        this->setTR();
        this->addCommandWait(72);
        this->commandMoveS(64);
    }

    inline void executeCommandLMMM(bool setup)
    {
        if (!this->isBitmapMode()) {
            printf("Error: LMMM was executed in invalid screen mode (%d)\n", this->getScreenMode());
            exit(-1);
        }
        int screenWidth = this->getScreenWidth();
        int dpb = this->getDotPerByteX();
        int lineBytes = screenWidth / dpb;
        if (setup) {
            this->ctx.cmd.sx = this->getSX();
            this->ctx.cmd.sy = this->getSY();
            this->ctx.cmd.dx = this->getDX();
            this->ctx.cmd.dy = this->getDY();
            this->ctx.cmd.nx = this->getNX();
            this->ctx.cmd.ny = this->getNY();
            this->ctx.cmd.diy = this->getDIY();
            this->ctx.cmd.dix = this->getDIX();
#ifdef COMMAND_DEBUG
            printf("%d: ExecuteCommand<LMMM>: DX=%d, DY=%d, NX=%d, NY=%d, DIX=%d, DIY=%d, LO=%d (SCREEN: %d)\n", ctx.countV, ctx.cmd.dx, ctx.cmd.dy, ctx.cmd.nx, ctx.cmd.ny, ctx.cmd.dix, ctx.cmd.diy, ctx.commandL, getScreenMode());
#endif
        }
        int addrS = this->ctx.cmd.sx / dpb + this->ctx.cmd.sy * lineBytes;
        int addrD = this->ctx.cmd.dx / dpb + this->ctx.cmd.dy * lineBytes;
        unsigned char d = this->readLogicalPixel(addrS, dpb, ctx.cmd.sx);
        this->addCommandWait(64);
        this->renderLogicalPixel(addrD, dpb, this->ctx.cmd.dx, d, ctx.commandL);
        this->addCommandWait(32);
        this->addCommandWait(24);
        this->commandMoveDS(64);
    }

    inline void executeCommandLMMV(bool setup)
    {
        if (!this->isBitmapMode()) {
            printf("Error: LMMV was executed in invalid screen mode (%d)\n", this->getScreenMode());
            exit(-1);
        }
        int screenWidth = this->getScreenWidth();
        int dpb = this->getDotPerByteX();
        int lineBytes = screenWidth / dpb;
        if (setup) {
            this->ctx.cmd.dx = this->getDX();
            this->ctx.cmd.dy = this->getDY();
            this->ctx.cmd.nx = this->getNX();
            this->ctx.cmd.ny = this->getNY();
            this->ctx.cmd.diy = this->getDIY();
            this->ctx.cmd.dix = this->getDIX();
#ifdef COMMAND_DEBUG
            printf("%d: ExecuteCommand<LMMV>: DX=%d, DY=%d, NX=%d, NY=%d, DIX=%d, DIY=%d, CLR=$%02X, LO=$%02X (SCREEN: %d)\n", ctx.countV, ctx.cmd.dx, ctx.cmd.dy, ctx.cmd.nx, ctx.cmd.ny, ctx.cmd.dix, ctx.cmd.diy, this->ctx.reg[44], ctx.commandL, getScreenMode());
#endif
        }
        int addr = this->ctx.cmd.dx / dpb + this->ctx.cmd.dy * lineBytes;
        this->renderLogicalPixel(addr, dpb, this->ctx.cmd.dx, this->ctx.reg[44], ctx.commandL);
        this->addCommandWait(72);
        this->addCommandWait(24);
        this->commandMoveD(64);
    }

    inline void executeCommandLINE(bool setup)
    {
        int dpb = this->getDotPerByteX();
        int lineBytes = this->getScreenWidth() / dpb;
        if (setup) {
            this->ctx.cmd.dx = this->getDX();
            this->ctx.cmd.dy = this->getDY();
            this->ctx.cmd.maj = this->getMAJ();
            this->ctx.cmd.min = this->getMIN();
            this->ctx.cmd.majF = (double)this->ctx.cmd.maj;
            this->ctx.cmd.minF = (double)this->ctx.cmd.min;
            this->ctx.cmd.diy = this->getDIY();
            this->ctx.cmd.dix = this->getDIX();
#ifdef COMMAND_DEBUG
            printf("%d: ExecuteCommand<LINE>: DX=%d, DY=%d, Maj=%d, Min=%d, DIX=%d, DIY=%d, MAJ=%s, CLR=$%02X, LO=%X (SCREEN: %d)\n", ctx.countV, ctx.cmd.dx, ctx.cmd.dy, (int)ctx.cmd.maj, (int)ctx.cmd.min, ctx.cmd.dix, ctx.cmd.diy, this->ctx.reg[45] & 1 ? "Y" : "X", this->ctx.reg[44], ctx.commandL, getScreenMode());
#endif
        }
        int addr = this->ctx.cmd.dx / dpb + this->ctx.cmd.dy * lineBytes;
        this->renderLogicalPixel(addr, dpb, this->ctx.cmd.dx, this->ctx.reg[44], ctx.commandL);
        this->addCommandWait(84);
        this->addCommandWait(24);
        if (0 < this->ctx.cmd.maj) {
            this->ctx.cmd.maj--;
            if (this->ctx.reg[45] & 1) {
                this->ctx.cmd.dy += this->ctx.cmd.diy;
                this->addCommandWait(32);
            } else {
                this->ctx.cmd.dx += this->ctx.cmd.dix;
            }
            if (0 < this->ctx.cmd.min) {
                int minN = (int)((this->ctx.cmd.maj / this->ctx.cmd.majF) * this->ctx.cmd.minF);
                if (minN != this->ctx.cmd.min) {
                    this->ctx.cmd.min = minN;
                    if (this->ctx.reg[45] & 1) {
                        this->ctx.cmd.dx += this->ctx.cmd.dix;
                    } else {
                        this->ctx.cmd.dy += this->ctx.cmd.diy;
                        this->addCommandWait(32);
                    }
                }
            }
        } else {
            this->setDX(this->ctx.cmd.dx);
            this->setDY(this->ctx.cmd.dy);
            this->setMAJ(this->ctx.cmd.maj);
            this->setMIN(this->ctx.cmd.min);
            this->setCommandEnd();
        }
    }

    inline void executeCommandSRCH(bool setup)
    {
        int dpb = this->getDotPerByteX();
        int lineBytes = this->getScreenWidth() / dpb;
        if (setup) {
            this->ctx.cmd.sx = this->getSX();
            this->ctx.cmd.sy = this->getSY();
            this->ctx.cmd.dix = this->getDIX();
#ifdef COMMAND_DEBUG
            printf("%d: ExecuteCommand<SRCH>: SX=%d, SY=%d, CLR=%d, DIX=%d, EQ=%d (SCREEN: %d)\n", ctx.countV, this->ctx.cmd.sx, this->ctx.cmd.sy, this->ctx.reg[44], this->ctx.cmd.dix, this->ctx.cmd.diy, getScreenMode());
#endif
        }
        if (0 <= this->ctx.cmd.sx && this->ctx.cmd.sx < 512) {
            int addr = (this->ctx.cmd.sy * lineBytes + this->ctx.cmd.sx / dpb) & 0x1FFFF;
            unsigned char clr = this->ctx.reg[44];
            if (2 == dpb) {
                clr &= 0x0F;
            } else if (4 == dpb) {
                clr &= 0x03;
            }
            unsigned char px = this->readLogicalPixel(addr, dpb, this->ctx.cmd.sx);
            if (!this->getEQ()) {
                if (px == clr) {
                    this->setBD();
                    this->ctx.stat[8] = this->ctx.cmd.sx & 0xFF;
                    this->ctx.stat[9] = ((this->ctx.cmd.sx & 0x300) >> 8) | 0xFC;
                    this->setSX(this->ctx.cmd.sx);
                    this->setCommandEnd();
                    return;
                }
            } else {
                if (px != clr) {
                    this->setBD();
                    this->ctx.stat[8] = this->ctx.cmd.sx & 0xFF;
                    this->ctx.stat[9] = ((this->ctx.cmd.sx & 0x300) >> 8) | 0xFC;
                    this->setSX(this->ctx.cmd.sx);
                    this->setCommandEnd();
                    return;
                }
            }
            this->ctx.cmd.sx += this->ctx.cmd.dix;
        } else {
            this->resetBD();
            this->setSX(this->ctx.cmd.sx);
            this->setCommandEnd();
        }
        this->addCommandWait(64);
    }

    inline void executeCommandPSET(bool setup)
    {
        int dpb = this->getDotPerByteX();
        int lineBytes = this->getScreenWidth() / dpb;
        if (setup) {
            this->ctx.cmd.dx = this->getDX();
            this->ctx.cmd.dy = this->getDY();
#ifdef COMMAND_DEBUG
            printf("%d: ExecuteCommand<PSET>: DX=%d, DY=%d, CLR=$%02X, LO=%X (SCREEN: %d)\n", ctx.countV, ctx.cmd.dx, ctx.cmd.dy, ctx.reg[44], ctx.commandL, getScreenMode());
#endif
        }
        int addr = this->ctx.cmd.dx / dpb + this->ctx.cmd.dy * lineBytes;
        this->renderLogicalPixel(addr, dpb, this->ctx.cmd.dx, this->ctx.reg[44], ctx.commandL);
        this->setCommandEnd();
        this->addCommandWait(64);
    }

    inline void executeCommandPOINT(bool setup)
    {
        puts("execute POINT (not implemented yet");
        exit(-1);
    }
};

#endif // INCLUDE_V9958_HPP
