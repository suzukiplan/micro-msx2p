
/**
 * SUZUKI PLAN - TinyMSX - TMS9918A Emulator
 * -----------------------------------------------------------------------------
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Yoji Suzuki.
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
#ifndef INCLUDE_TMS9918A_HPP
#define INCLUDE_TMS9918A_HPP

#include <string.h>

#define TMS9918A_SCREEN_WIDTH 284
#define TMS9918A_SCREEN_HEIGHT 240

/**
 * Note about the Screen Resolution: 284 x 240
 * =================================================
 * Pixel (horizontal) display timings:
 *   Left blanking:   2Hz (skip)
 *     Color burst:  14Hz (skip)
 *   Left blanking:   8Hz (skip)
 *     Left border:  13Hz (RENDER)
 *  Active display: 256Hz (RENDER)
 *    Right border:  15Hz (RENDER)
 *  Right blanking:   8Hz (skip)
 * Horizontal sync:  26Hz (skip)
 *           Total: 342Hz (render: 284 pixels)
 * =================================================
 * Scanline (vertical) display timings:
 *    Top blanking:  13 lines (skip)
 *      Top border:   3 lines (skip)
 *      Top border:  24 lines (RENDER)
 *  Active display: 192 lines (RENDER)
 *   Bottom border:  24 lines (RENDER)
 * Bottom blanking:   3 lines (skip)
 *   Vertical sync:   3 lines (skip)
 *           Total: 262 lines (render: 240 lines)
 * =================================================
 */

class TMS9918A
{
  private:
    void* arg;
    void (*detectBlank)(void* arg);
    void (*detectBreak)(void* arg);
    void (*displayCallback)(void* arg, int frame, int line, unsigned short* display);

  public:
    unsigned short* display;
    size_t displaySize;
    bool displayNeedFree;
    unsigned short palette[16];
    const unsigned int rgb888[16] = {0x000000, 0x000000, 0x3EB849, 0x74D07D, 0x5955E0, 0x8076F1, 0xB95E51, 0x65DBEF, 0xDB6559, 0xFF897D, 0xCCC35E, 0xDED087, 0x3AA241, 0xB766B5, 0xCCCCCC, 0xFFFFFF};

    typedef struct Context_ {
        int bobo;
        int countH;
        int countV;
        int frame;
        unsigned char ram[0x4000];
        unsigned char reg[8];
        unsigned char tmpAddr[2];
        unsigned short addr;
        unsigned short writeAddr;
        unsigned char stat;
        unsigned char latch;
        unsigned char readBuffer;
        unsigned char writeWait;
    } Context;
    Context* ctx;
    bool ctxNeedFree;

    TMS9918A(int colorMode, void* arg, void (*detectBlank)(void*), void (*detectBreak)(void*), void (*displayCallback)(void*, int, int, unsigned short*) = nullptr, Context* vram = nullptr)
    {
        this->arg = arg;
        this->detectBlank = detectBlank;
        this->detectBreak = detectBreak;
        this->displayCallback = displayCallback;
        this->displaySize = (this->displayCallback ? 256 : 256 * 192) << 1;
        this->display = (unsigned short*)malloc(this->displaySize);
        this->displayNeedFree = true;
        this->ctx = vram ? vram : (Context*)malloc(sizeof(Context));
        this->ctxNeedFree = vram ? false : true;
        memset(this->ctx, 0, sizeof(Context));

        switch (colorMode) {
            case 0: // RGB555
                for (int i = 0; i < 16; i++) {
                    this->palette[i] = 0;
                    this->palette[i] |= (this->rgb888[i] & 0b111110000000000000000000) >> 9;
                    this->palette[i] |= (this->rgb888[i] & 0b000000001111100000000000) >> 6;
                    this->palette[i] |= (this->rgb888[i] & 0b000000000000000011111000) >> 3;
                }
                break;
            case 1: // RGB565
                for (int i = 0; i < 16; i++) {
                    this->palette[i] = 0;
                    this->palette[i] |= (this->rgb888[i] & 0b111110000000000000000000) >> 8;
                    this->palette[i] |= (this->rgb888[i] & 0b000000001111110000000000) >> 5;
                    this->palette[i] |= (this->rgb888[i] & 0b000000000000000011111000) >> 3;
                }
                break;
            case 2: // BGR565
                for (int i = 0; i < 16; i++) {
                    this->palette[i] = 0;
                    this->palette[i] |= (this->rgb888[i] & 0b111110000000000000000000) >> 19;
                    this->palette[i] |= (this->rgb888[i] & 0b000000001111110000000000) >> 5;
                    this->palette[i] |= (this->rgb888[i] & 0b000000000000000011111000) << 8;
                }
                break;
            default:
                memset(this->palette, 0, sizeof(this->palette));
        }
        this->reset();
    }

    ~TMS9918A()
    {
        this->releaseDisplayBuffer();
        this->releaseContext();
    }

    void useOwnDisplayBuffer(unsigned short* displayBuffer, size_t displayBufferSize)
    {
        this->releaseDisplayBuffer();
        this->display = displayBuffer;
        this->displaySize = displayBufferSize;
    }

    void reset()
    {
        memset(this->display, 0, this->displaySize);
        memset(this->ctx, 0, sizeof(Context));
    }

    inline int getVideoMode()
    {
        // NOTE: undocumented mode is not support
        if (ctx->reg[1] & 0b00010000) return 1; // Mode 1
        if (ctx->reg[0] & 0b00000010) return 2; // Mode 2
        if (ctx->reg[1] & 0b00001000) return 3; // Mode 3
        return 0;                               // Mode 0
    }

    inline int getVramSize() { return ctx->reg[1] & 0b10000000 ? 0x4000 : 0x4000; }
    inline bool isEnabledScreen() { return ctx->reg[1] & 0b01000000 ? true : false; }
    inline bool isEnabledInterrupt() { return ctx->reg[1] & 0b00100000 ? true : false; }
    inline unsigned short getBackdropColor() { return palette[ctx->reg[7] & 0b00001111]; }

    inline void tick()
    {
        this->ctx->countH++;
        // render backdrop border
        if (27 <= this->ctx->countV && this->ctx->countV < 27 + 192) {
            if (24 + TMS9918A_SCREEN_WIDTH == this->ctx->countH) {
                this->renderScanline(this->ctx->countV - 27);
            }
        }
        // delay write the VRAM
        if (this->ctx->writeWait) {
            this->ctx->writeWait--;
            if (0 == this->ctx->writeWait) {
                this->ctx->ram[this->ctx->writeAddr] = this->ctx->readBuffer;
            }
        }
        // sync blank or end-of-frame
        if (342 == this->ctx->countH) {
            this->ctx->countH -= 342;
            switch (++this->ctx->countV) {
                case 238:
                    this->ctx->stat |= 0x80;
                    if (this->isEnabledInterrupt()) {
                        this->detectBlank(this->arg);
                    }
                    break;
                case 262:
                    this->ctx->countV -= 262;
                    this->detectBreak(this->arg);
                    this->ctx->frame++;
                    this->ctx->frame &= 0xFFFF;
                    break;
            }
        }
    }

    inline unsigned char readData()
    {
        unsigned char result = this->ctx->readBuffer;
        this->readVideoMemory();
        this->ctx->latch = 0;
        return result;
    }

    inline unsigned char readStatus()
    {
        unsigned char result = this->ctx->stat;
        this->ctx->stat &= 0b01011111;
        this->ctx->latch = 0;
        return result;
    }

    inline void writeData(unsigned char value)
    {
        this->ctx->addr &= this->getVramSize() - 1;
        this->ctx->readBuffer = value;
        this->ctx->writeAddr = this->ctx->addr++;
        this->ctx->writeWait = 10; // write back to VRAM from readBuffer after 10Hz (about 1.86 micro second delay)
        this->ctx->latch = 0;
    }

    inline void writeAddress(unsigned char value)
    {
        this->ctx->latch &= 1;
        this->ctx->tmpAddr[this->ctx->latch++] = value;
        if (2 == this->ctx->latch) {
            if (this->ctx->tmpAddr[1] & 0b10000000) {
                this->updateRegister();
            } else if (this->ctx->tmpAddr[1] & 0b01000000) {
                this->updateAddress();
            } else {
                this->updateAddress();
                this->readVideoMemory();
            }
        } else if (1 == this->ctx->latch) {
            this->ctx->addr &= 0xFF00;
            this->ctx->addr |= this->ctx->tmpAddr[0];
        }
    }

  private:
    void releaseDisplayBuffer()
    {
        if (this->displayNeedFree) {
            free(this->display);
            this->display = nullptr;
            this->displaySize = 0;
            this->displayNeedFree = false;
        }
    }

    void releaseContext()
    {
        if (this->ctxNeedFree) {
            free(this->ctx);
            this->ctx = nullptr;
            this->ctxNeedFree = false;
        }
    }

    inline void renderScanline(int lineNumber)
    {
        // TODO: Several modes (1, 3, undocumented) are not implemented
        if (this->isEnabledScreen()) {
            switch (this->getVideoMode()) {
                case 0: this->renderScanlineMode0(lineNumber); break;
                case 2: this->renderScanlineMode2(lineNumber); break;
            }
        } else {
            int dcur = this->getDisplayPtr(lineNumber);
            unsigned short bd = this->getBackdropColor();
            for (int i = 0; i < 256; i++) {
                this->display[dcur++] = bd;
            }
        }
        if (this->displayCallback) {
            this->displayCallback(this->arg, this->ctx->frame, lineNumber, this->display);
        }
    }

    inline void updateAddress()
    {
        this->ctx->addr = this->ctx->tmpAddr[1];
        this->ctx->addr <<= 8;
        this->ctx->addr |= this->ctx->tmpAddr[0];
        this->ctx->addr &= this->getVramSize() - 1;
    }

    inline void readVideoMemory()
    {
        this->ctx->addr &= this->getVramSize() - 1;
        this->ctx->readBuffer = this->ctx->ram[this->ctx->addr++];
    }

    inline void updateRegister()
    {
        bool previousInterrupt = this->isEnabledInterrupt();
        this->ctx->reg[this->ctx->tmpAddr[1] & 0b00001111] = this->ctx->tmpAddr[0];
        if (!previousInterrupt && this->isEnabledInterrupt() && this->ctx->stat & 0x80) {
            this->detectBlank(this->arg);
        }
    }

    inline int getDisplayPtr(int lineNumber)
    {
        return this->displayCallback ? 0 : lineNumber * 256;
    }

    inline void renderScanlineMode0(int lineNumber)
    {
        int pn = (this->ctx->reg[2] & 0b00001111) << 10;
        int ct = this->ctx->reg[3] << 6;
        int pg = (this->ctx->reg[4] & 0b00000111) << 11;
        int bd = this->ctx->reg[7] & 0b00001111;
        int pixelLine = lineNumber % 8;
        unsigned char* nam = &this->ctx->ram[pn + lineNumber / 8 * 32];
        int dcur = this->getDisplayPtr(lineNumber);
        for (int i = 0; i < 32; i++) {
            unsigned char ptn = this->ctx->ram[pg + nam[i] * 8 + pixelLine];
            unsigned char c = this->ctx->ram[ct + nam[i] / 8];
            unsigned char cc[2];
            cc[1] = (c & 0xF0) >> 4;
            cc[1] = cc[1] ? cc[1] : bd;
            cc[0] = c & 0x0F;
            cc[0] = cc[0] ? cc[0] : bd;
            this->display[dcur++] = this->palette[cc[(ptn & 0b10000000) >> 7]];
            this->display[dcur++] = this->palette[cc[(ptn & 0b01000000) >> 6]];
            this->display[dcur++] = this->palette[cc[(ptn & 0b00100000) >> 5]];
            this->display[dcur++] = this->palette[cc[(ptn & 0b00010000) >> 4]];
            this->display[dcur++] = this->palette[cc[(ptn & 0b00001000) >> 3]];
            this->display[dcur++] = this->palette[cc[(ptn & 0b00000100) >> 2]];
            this->display[dcur++] = this->palette[cc[(ptn & 0b00000010) >> 1]];
            this->display[dcur++] = this->palette[cc[ptn & 0b00000001]];
        }
        renderSprites(lineNumber);
    }

    inline void renderScanlineMode2(int lineNumber)
    {
        int pn = (this->ctx->reg[2] & 0b00001111) << 10;
        int ct = (this->ctx->reg[3] & 0b10000000) << 6;
        int cmask = this->ctx->reg[3] & 0b01111111;
        cmask <<= 3;
        cmask |= 0x07;
        int pg = (this->ctx->reg[4] & 0b00000100) << 11;
        int pmask = this->ctx->reg[4] & 0b00000011;
        pmask <<= 8;
        pmask |= 0xFF;
        int bd = this->ctx->reg[7] & 0b00001111;
        int pixelLine = lineNumber % 8;
        unsigned char* nam = &this->ctx->ram[pn + lineNumber / 8 * 32];
        int dcur = this->getDisplayPtr(lineNumber);
        int ci = (lineNumber / 64) * 256;
        for (int i = 0; i < 32; i++) {
            unsigned char ptn = this->ctx->ram[pg + ((nam[i] + ci) & pmask) * 8 + pixelLine];
            unsigned char c = this->ctx->ram[ct + ((nam[i] + ci) & cmask) * 8 + pixelLine];
            unsigned char cc[2];
            cc[1] = (c & 0xF0) >> 4;
            cc[1] = cc[1] ? cc[1] : bd;
            cc[0] = c & 0x0F;
            cc[0] = cc[0] ? cc[0] : bd;
            this->display[dcur++] = this->palette[cc[(ptn & 0b10000000) >> 7]];
            this->display[dcur++] = this->palette[cc[(ptn & 0b01000000) >> 6]];
            this->display[dcur++] = this->palette[cc[(ptn & 0b00100000) >> 5]];
            this->display[dcur++] = this->palette[cc[(ptn & 0b00010000) >> 4]];
            this->display[dcur++] = this->palette[cc[(ptn & 0b00001000) >> 3]];
            this->display[dcur++] = this->palette[cc[(ptn & 0b00000100) >> 2]];
            this->display[dcur++] = this->palette[cc[(ptn & 0b00000010) >> 1]];
            this->display[dcur++] = this->palette[cc[ptn & 0b00000001]];
        }
        renderSprites(lineNumber);
    }

    inline void renderSprites(int lineNumber)
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
        bool si = this->ctx->reg[1] & 0b00000010 ? true : false;
        bool mag = this->ctx->reg[1] & 0b00000001 ? true : false;
        int sa = (this->ctx->reg[5] & 0b01111111) << 7;
        int sg = (this->ctx->reg[6] & 0b00000111) << 11;
        int sn = 0;
        unsigned char dlog[256];
        unsigned char wlog[256];
        memset(dlog, 0, sizeof(dlog));
        memset(wlog, 0, sizeof(wlog));
        const int dcur = this->getDisplayPtr(lineNumber);
        for (int i = 0; i < 32; i++) {
            int cur = sa + i * 4;
            unsigned char y = this->ctx->ram[cur++];
            if (208 == y) break;
            unsigned char x = this->ctx->ram[cur++];
            unsigned char ptn = this->ctx->ram[cur++];
            unsigned char col = this->ctx->ram[cur++];
            if (col & 0x80) x -= 32;
            col &= 0b00001111;
            y++;
            if (mag) {
                if (si) {
                    // 16x16 x 2
                    if (y <= lineNumber && lineNumber < y + 32) {
                        sn++;
                        if (5 <= sn) {
                            this->ctx->stat &= 0b11100000;
                            this->ctx->stat |= 0b01000000 | i;
                            break;
                        } else {
                            this->ctx->stat &= 0b11100000;
                            this->ctx->stat |= i;
                        }
                        int pixelLine = lineNumber - y;
                        cur = sg + (ptn & 252) * 8 + pixelLine % 16 / 2 + (pixelLine < 16 ? 0 : 8);
                        bool overflow = false;
                        for (int j = 0; !overflow && j < 16; j++, x++) {
                            if (wlog[x]) {
                                this->ctx->stat |= 0b00100000;
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx->ram[cur] & bit[j / 2]) {
                                    this->display[dcur + x] = this->palette[col];
                                    dlog[x] = col;
                                    wlog[x] = 1;
                                }
                            }
                            overflow = x == 0xFF;
                        }
                        cur += 16;
                        for (int j = 0; !overflow && j < 16; j++, x++) {
                            if (wlog[x]) {
                                this->ctx->stat |= 0b00100000;
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx->ram[cur] & bit[j / 2]) {
                                    this->display[dcur + x] = this->palette[col];
                                    dlog[x] = col;
                                    wlog[x] = 1;
                                }
                            }
                            overflow = x == 0xFF;
                        }
                    }
                } else {
                    // 8x8 x 2
                    if (y <= lineNumber && lineNumber < y + 16) {
                        sn++;
                        if (5 <= sn) {
                            this->ctx->stat &= 0b11100000;
                            this->ctx->stat |= 0b01000000 | i;
                            break;
                        } else {
                            this->ctx->stat &= 0b11100000;
                            this->ctx->stat |= i;
                        }
                        cur = sg + ptn * 8 + lineNumber % 8;
                        bool overflow = false;
                        for (int j = 0; !overflow && j < 16; j++, x++) {
                            if (wlog[x]) {
                                this->ctx->stat |= 0b00100000;
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx->ram[cur] & bit[j / 2]) {
                                    this->display[dcur + x] = this->palette[col];
                                    dlog[x] = col;
                                    wlog[x] = 1;
                                }
                            }
                            overflow = x == 0xFF;
                        }
                    }
                }
            } else {
                if (si) {
                    // 16x16 x 1
                    if (y <= lineNumber && lineNumber < y + 16) {
                        sn++;
                        if (5 <= sn) {
                            this->ctx->stat &= 0b11100000;
                            this->ctx->stat |= 0b01000000 | i;
                            break;
                        } else {
                            this->ctx->stat &= 0b11100000;
                            this->ctx->stat |= i;
                        }
                        int pixelLine = lineNumber - y;
                        cur = sg + (ptn & 252) * 8 + pixelLine % 8 + (pixelLine < 8 ? 0 : 8);
                        bool overflow = false;
                        for (int j = 0; !overflow && j < 8; j++, x++) {
                            if (wlog[x]) {
                                this->ctx->stat |= 0b00100000;
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx->ram[cur] & bit[j]) {
                                    this->display[dcur + x] = this->palette[col];
                                    dlog[x] = col;
                                    wlog[x] = 1;
                                }
                            }
                            overflow = x == 0xFF;
                        }
                        cur += 16;
                        for (int j = 0; !overflow && j < 8; j++, x++) {
                            if (wlog[x]) {
                                this->ctx->stat |= 0b00100000;
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx->ram[cur] & bit[j]) {
                                    this->display[dcur + x] = this->palette[col];
                                    dlog[x] = col;
                                    wlog[x] = 1;
                                }
                            }
                            overflow = x == 0xFF;
                        }
                    }
                } else {
                    // 8x8 x 1
                    if (y <= lineNumber && lineNumber < y + 8) {
                        sn++;
                        if (5 <= sn) {
                            this->ctx->stat &= 0b11100000;
                            this->ctx->stat |= 0b01000000 | i;
                            break;
                        } else {
                            this->ctx->stat &= 0b11100000;
                            this->ctx->stat |= i;
                        }
                        cur = sg + ptn * 8 + lineNumber % 8;
                        bool overflow = false;
                        for (int j = 0; !overflow && j < 8; j++, x++) {
                            if (wlog[x]) {
                                this->ctx->stat |= 0b00100000;
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx->ram[cur] & bit[j]) {
                                    this->display[dcur + x] = this->palette[col];
                                    dlog[x] = col;
                                    wlog[x] = 1;
                                }
                            }
                            overflow = x == 0xFF;
                        }
                    }
                }
            }
        }
    }
};

#endif // INCLUDE_TMS9918A_HPP
