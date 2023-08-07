
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
  public:
    enum class ColorMode {
        RGB555,
        RGB565,
        RGB565_Swap,
    };

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
        int isRenderingLine;
        int reverved32[3];
        unsigned char ram[0x4000];
        unsigned char reg[8];
        unsigned char tmpAddr[2];
        unsigned short addr;
        unsigned short writeAddr;
        unsigned char stat;
        unsigned char latch;
        unsigned char readBuffer;
        unsigned char reserved8[1];
    } Context;
    Context* ctx;
    bool ctxNeedFree;

    unsigned short swap16(unsigned short src)
    {
        auto work = (src & 0xFF00) >> 8;
        return ((src & 0x00FF) << 8) | work;
    }

    void initialize(ColorMode colorMode, void* arg, void (*detectBlank)(void*), void (*detectBreak)(void*), void (*displayCallback)(void*, int, int, unsigned short*) = nullptr, Context* vram = nullptr)
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
            case ColorMode::RGB555:
                for (int i = 0; i < 16; i++) {
                    this->palette[i] = 0;
                    this->palette[i] |= (this->rgb888[i] & 0b111110000000000000000000) >> 9;
                    this->palette[i] |= (this->rgb888[i] & 0b000000001111100000000000) >> 6;
                    this->palette[i] |= (this->rgb888[i] & 0b000000000000000011111000) >> 3;
                }
                break;
            case ColorMode::RGB565:
                for (int i = 0; i < 16; i++) {
                    this->palette[i] = 0;
                    this->palette[i] |= (this->rgb888[i] & 0b111110000000000000000000) >> 8;
                    this->palette[i] |= (this->rgb888[i] & 0b000000001111110000000000) >> 5;
                    this->palette[i] |= (this->rgb888[i] & 0b000000000000000011111000) >> 3;
                }
                break;
            case ColorMode::RGB565_Swap:
                for (int i = 0; i < 16; i++) {
                    this->palette[i] = 0;
                    this->palette[i] |= (this->rgb888[i] & 0b111110000000000000000000) >> 8;
                    this->palette[i] |= (this->rgb888[i] & 0b000000001111110000000000) >> 5;
                    this->palette[i] |= (this->rgb888[i] & 0b000000000000000011111000) >> 3;
                    this->palette[i] = swap16(this->palette[i]);
                }
                break;
            default:
                memset(this->palette, 0, sizeof(this->palette));
        }
        this->initRedneringLineTable();
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
        this->refresh();
    }

    void refresh() { this->acUpdate(); }
    inline bool isEnabledScreen() { return ac.isEnabledScreen; }
    inline bool isEnabledInterrupt() { return ac.isEnabledInterrupt; }
    inline unsigned short getBackdropColor() { return ac.backdropColor; }
    inline unsigned short getBackdropColor(bool swap) { return swap ? this->swap16(this->ac.backdropColor) : this->ac.backdropColor; }

    inline void tick(int tickCount)
    {
        for (int i = 0; i < tickCount; i++) {
            this->ctx->countH++;
            // render backdrop border
            if (this->ctx->isRenderingLine) {
                if (24 + TMS9918A_SCREEN_WIDTH == this->ctx->countH) {
                    this->renderScanline(this->ctx->countV - 27);
                }
            }
            // sync blank or end-of-frame
            if (342 == this->ctx->countH) {
                this->ctx->countH = 0;
                this->ctx->countV++;
                this->ctx->isRenderingLine = this->renderingLineTable[this->ctx->countV];
                if (238 == this->ctx->countV) {
                    this->ctx->stat |= 0x80;
                    if (this->isEnabledInterrupt()) {
                        this->detectBlank(this->arg);
                    }
                } else if (262 == this->ctx->countV) {
                    this->ctx->countV = 0;
                    this->detectBreak(this->arg);
                    this->ctx->frame++;
                    this->ctx->frame &= 0xFFFF;
                }
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
        this->ctx->addr &= 0x3FFF;
        this->ctx->readBuffer = value;
        this->ctx->writeAddr = this->ctx->addr++;
        this->ctx->ram[this->ctx->writeAddr] = this->ctx->readBuffer;
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
        } else {
            this->ctx->addr &= 0xFF00;
            this->ctx->addr |= this->ctx->tmpAddr[0];
        }
    }

  private:
    struct AddressCache {
        int mode;
        bool isEnabledScreen;
        bool isEnabledInterrupt;
        unsigned short backdropColor;
        int si;
        int mag;
        int mag8;
        int mag16;
        int pn;
        int ct0;
        int ct2;
        int pg0;
        int pg2;
        int pmask;
        int cmask;
        int sa;
        int sg;
        int bd;
    } ac;

    inline void acUpdateMode()
    {
        if (this->ctx->reg[1] & 0b00010000) {
            this->ac.mode = 1;
        } else if (this->ctx->reg[0] & 0b00000010) {
            this->ac.mode = 2;
        } else if (this->ctx->reg[1] & 0b00001000) {
            this->ac.mode = 3;
        } else {
            this->ac.mode = 0;
        }
    }

    static inline void acUpdate0(TMS9918A* tms)
    {
        tms->acUpdateMode();
    }

    static inline void acUpdate1(TMS9918A* tms)
    {
        tms->ac.si = tms->ctx->reg[1] & 0b00000010 ? 16 : 8;
        tms->ac.mag = tms->ctx->reg[1] & 0b00000001 ? 2 : 1;
        tms->ac.mag8 = tms->ac.mag * 8;
        tms->ac.mag16 = tms->ac.mag * 16;
        tms->ac.isEnabledScreen = tms->ctx->reg[1] & 0b01000000 ? true : false;
        tms->ac.isEnabledInterrupt = tms->ctx->reg[1] & 0b00100000 ? true : false;
        tms->acUpdateMode();
    }

    static inline void acUpdate2(TMS9918A* tms)
    {
        tms->ac.pn = (tms->ctx->reg[2] & 0b00001111) << 10;
    }

    static inline void acUpdate3(TMS9918A* tms)
    {
        tms->ac.ct0 = tms->ctx->reg[3] << 6;
        tms->ac.ct2 = (tms->ctx->reg[3] & 0b10000000) << 6;
        tms->ac.cmask = tms->ctx->reg[3] & 0b01111111;
        tms->ac.cmask <<= 3;
        tms->ac.cmask |= 0x07;
    }

    static inline void acUpdate4(TMS9918A* tms)
    {
        tms->ac.pg0 = (tms->ctx->reg[4] & 0b00000111) << 11;
        tms->ac.pg2 = (tms->ctx->reg[4] & 0b00000100) << 11;
        tms->ac.pmask = tms->ctx->reg[4] & 0b00000011;
        tms->ac.pmask <<= 8;
        tms->ac.pmask |= 0xFF;
    }

    static inline void acUpdate5(TMS9918A* tms)
    {
        tms->ac.sa = (tms->ctx->reg[5] & 0b01111111) << 7;
    }

    static inline void acUpdate6(TMS9918A* tms)
    {
        tms->ac.sg = (tms->ctx->reg[6] & 0b00000111) << 11;
    }

    static inline void acUpdate7(TMS9918A* tms)
    {
        tms->ac.bd = tms->ctx->reg[7] & 0b00001111;
        tms->ac.backdropColor = tms->palette[tms->ac.bd];
    }

    void (*updateTable[8])(TMS9918A*) = {acUpdate0, acUpdate1, acUpdate2, acUpdate3, acUpdate4, acUpdate5, acUpdate6, acUpdate7};
    inline void acUpdate(int n) { updateTable[n & 7](this); }
    inline void acUpdate()
    {
        for (int i = 0; i < 8; i++) acUpdate(i);
    }

    int renderingLineTable[263];
    void initRedneringLineTable()
    {
        for (int i = 0; i < 263; i++) {
            if (27 <= i && i < 27 + 192) {
                this->renderingLineTable[i] = 1;
            } else {
                this->renderingLineTable[i] = 0;
            }
        }
    }

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
#ifdef TMS9918A_SKIP_ODD_FRAME_RENDERING
        bool isEvenFrame = 0 == (this->ctx->frame & 1);
#endif
        // TODO: Several modes (1, 3, undocumented) are not implemented
        if (this->isEnabledScreen()) {
            switch (this->ac.mode) {
#ifdef TMS9918A_SKIP_ODD_FRAME_RENDERING
                case 0: this->renderScanlineMode0(lineNumber, isEvenFrame); break;
                case 2: this->renderScanlineMode2(lineNumber, isEvenFrame); break;
#else
                case 0: this->renderScanlineMode0(lineNumber); break;
                case 2: this->renderScanlineMode2(lineNumber); break;
#endif
            }
        } else {
#ifdef TMS9918A_SKIP_ODD_FRAME_RENDERING
            if (isEvenFrame) {
                int dcur = this->getDisplayPtr(lineNumber);
                for (int i = 0; i < 256; i++) {
                    this->display[dcur++] = this->ac.backdropColor;
                }
            }
#else
            int dcur = this->getDisplayPtr(lineNumber);
            for (int i = 0; i < 256; i++) {
                this->display[dcur++] = this->ac.backdropColor;
            }
#endif
        }
#ifdef TMS9918A_SKIP_ODD_FRAME_RENDERING
        if (this->displayCallback && isEvenFrame) {
            this->displayCallback(this->arg, this->ctx->frame, lineNumber, this->display);
        }
#else
        if (this->displayCallback) {
            this->displayCallback(this->arg, this->ctx->frame, lineNumber, this->display);
        }
#endif
    }

    inline void updateAddress()
    {
        this->ctx->addr = this->ctx->tmpAddr[1];
        this->ctx->addr <<= 8;
        this->ctx->addr |= this->ctx->tmpAddr[0];
        this->ctx->addr &= 0x3FFF;
    }

    inline void readVideoMemory()
    {
        this->ctx->addr &= 0x3FFF;
        this->ctx->readBuffer = this->ctx->ram[this->ctx->addr++];
    }

    inline void updateRegister()
    {
        bool previousInterrupt = this->isEnabledInterrupt();
        int r = this->ctx->tmpAddr[1] & 0b00001111;
        this->ctx->reg[r] = this->ctx->tmpAddr[0];
        this->acUpdate(r);
        if (!previousInterrupt && this->isEnabledInterrupt() && this->ctx->stat & 0x80) {
            this->detectBlank(this->arg);
        }
    }

    inline int getDisplayPtr(int lineNumber)
    {
        return this->displayCallback ? 0 : lineNumber * 256;
    }

    inline void renderScanlineMode0(int lineNumber, bool rendering = true)
    {
        int dcur = this->getDisplayPtr(lineNumber);
        int dcur0 = dcur;
        if (rendering) {
            int pixelLine = lineNumber % 8;
            unsigned char* nam = &this->ctx->ram[ac.pn + lineNumber / 8 * 32];
            int ptn;
            int c;
            int cc[2];
            for (int i = 0; i < 32; i++) {
                ptn = this->ctx->ram[ac.pg0 + nam[i] * 8 + pixelLine];
                c = this->ctx->ram[ac.ct0 + nam[i] / 8];
                cc[1] = (c & 0xF0) >> 4;
                cc[1] = this->palette[cc[1] ? cc[1] : ac.bd];
                cc[0] = c & 0x0F;
                cc[0] = this->palette[cc[0] ? cc[0] : ac.bd];
                this->display[dcur++] = cc[(ptn & 0b10000000) >> 7];
                this->display[dcur++] = cc[(ptn & 0b01000000) >> 6];
                this->display[dcur++] = cc[(ptn & 0b00100000) >> 5];
                this->display[dcur++] = cc[(ptn & 0b00010000) >> 4];
                this->display[dcur++] = cc[(ptn & 0b00001000) >> 3];
                this->display[dcur++] = cc[(ptn & 0b00000100) >> 2];
                this->display[dcur++] = cc[(ptn & 0b00000010) >> 1];
                this->display[dcur++] = cc[ptn & 0b00000001];
            }
        }
        renderSprites(lineNumber, &display[dcur0], rendering);
    }

    inline void renderScanlineMode2(int lineNumber, bool rendering = true)
    {
        int dcur = this->getDisplayPtr(lineNumber);
        int dcur0 = dcur;
        if (rendering) {
            int pixelLine = lineNumber % 8;
            unsigned char* nam = &this->ctx->ram[ac.pn + lineNumber / 8 * 32];
            int ci = (lineNumber / 64) * 256;
            int ptn;
            int c;
            int cc[2];
            for (int i = 0; i < 32; i++) {
                ptn = this->ctx->ram[ac.pg2 + ((nam[i] + ci) & ac.pmask) * 8 + pixelLine];
                c = this->ctx->ram[ac.ct2 + ((nam[i] + ci) & ac.cmask) * 8 + pixelLine];
                cc[1] = (c & 0xF0) >> 4;
                cc[1] = this->palette[cc[1] ? cc[1] : ac.bd];
                cc[0] = c & 0x0F;
                cc[0] = this->palette[cc[0] ? cc[0] : ac.bd];
                this->display[dcur++] = cc[(ptn & 0b10000000) >> 7];
                this->display[dcur++] = cc[(ptn & 0b01000000) >> 6];
                this->display[dcur++] = cc[(ptn & 0b00100000) >> 5];
                this->display[dcur++] = cc[(ptn & 0b00010000) >> 4];
                this->display[dcur++] = cc[(ptn & 0b00001000) >> 3];
                this->display[dcur++] = cc[(ptn & 0b00000100) >> 2];
                this->display[dcur++] = cc[(ptn & 0b00000010) >> 1];
                this->display[dcur++] = cc[ptn & 0b00000001];
            }
        }
        renderSprites(lineNumber, &display[dcur0], rendering);
    }

    inline void renderSprites(int lineNumber, unsigned short* renderPosition, bool rendering)
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
        bool si = this->ctx->reg[1] & 0b00000010;
        bool mag = this->ctx->reg[1] & 0b00000001;
        int sn = 0;
        int tsn = 0;
        unsigned char dlog[256];
        unsigned char wlog[256];
        memset(dlog, 0, sizeof(dlog));
        memset(wlog, 0, sizeof(wlog));
        bool limitOver = false;
        for (int i = 0; i < 32; i++) {
            int cur = ac.sa + i * 4;
            unsigned char y = this->ctx->ram[cur++];
            if (208 == y) break;
            int x = this->ctx->ram[cur++];
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
                        if (!col) tsn++;
                        if (5 == sn) {
                            this->ctx->stat &= 0b11100000;
                            this->ctx->stat |= 0b01000000 | i;
                            if (4 <= tsn) break;
                            limitOver = true;
                        } else if (sn < 5) {
                            this->ctx->stat &= 0b11100000;
                            this->ctx->stat |= i;
                        }
                        int pixelLine = lineNumber - y;
                        cur = ac.sg + (ptn & 252) * 8 + pixelLine % 16 / 2 + (pixelLine < 16 ? 0 : 8);
                        bool overflow = false;
                        for (int j = 0; !overflow && j < 16; j++, x++) {
                            if (x < 0) continue;
                            if (wlog[x] && !limitOver) {
                                this->ctx->stat |= 0b00100000;
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx->ram[cur] & bit[j / 2]) {
                                    if (col && rendering) renderPosition[x] = this->palette[col];
                                    dlog[x] = col;
                                    wlog[x] = 1;
                                }
                            }
                            overflow = x == 0xFF;
                        }
                        cur += 16;
                        for (int j = 0; !overflow && j < 16; j++, x++) {
                            if (x < 0) continue;
                            if (wlog[x] && !limitOver) {
                                this->ctx->stat |= 0b00100000;
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx->ram[cur] & bit[j / 2]) {
                                    if (col && rendering) renderPosition[x] = this->palette[col];
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
                        if (!col) tsn++;
                        if (5 == sn) {
                            this->ctx->stat &= 0b11100000;
                            this->ctx->stat |= 0b01000000 | i;
                            if (4 <= tsn) break;
                            limitOver = true;
                        } else if (sn < 5) {
                            this->ctx->stat &= 0b11100000;
                            this->ctx->stat |= i;
                        }
                        cur = ac.sg + ptn * 8 + lineNumber % 8;
                        bool overflow = false;
                        for (int j = 0; !overflow && j < 16; j++, x++) {
                            if (x < 0) continue;
                            if (wlog[x] && !limitOver) {
                                this->ctx->stat |= 0b00100000;
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx->ram[cur] & bit[j / 2]) {
                                    if (col && rendering) renderPosition[x] = this->palette[col];
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
                        if (!col) tsn++;
                        if (5 == sn) {
                            this->ctx->stat &= 0b11100000;
                            this->ctx->stat |= 0b01000000 | i;
                            if (4 <= tsn) break;
                            limitOver = true;
                        } else if (sn < 5) {
                            this->ctx->stat &= 0b11100000;
                            this->ctx->stat |= i;
                        }
                        int pixelLine = lineNumber - y;
                        cur = ac.sg + (ptn & 252) * 8 + pixelLine % 8 + (pixelLine < 8 ? 0 : 8);
                        bool overflow = false;
                        for (int j = 0; !overflow && j < 8; j++, x++) {
                            if (x < 0) continue;
                            if (wlog[x] && !limitOver) {
                                this->ctx->stat |= 0b00100000;
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx->ram[cur] & bit[j]) {
                                    if (col && rendering) renderPosition[x] = this->palette[col];
                                    dlog[x] = col;
                                    wlog[x] = 1;
                                }
                            }
                            overflow = x == 0xFF;
                        }
                        cur += 16;
                        for (int j = 0; !overflow && j < 8; j++, x++) {
                            if (x < 0) continue;
                            if (wlog[x] && !limitOver) {
                                this->ctx->stat |= 0b00100000;
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx->ram[cur] & bit[j]) {
                                    if (col && rendering) renderPosition[x] = this->palette[col];
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
                        if (!col) tsn++;
                        if (5 == sn) {
                            this->ctx->stat &= 0b11100000;
                            this->ctx->stat |= 0b01000000 | i;
                            if (4 <= tsn) break;
                            limitOver = true;
                        } else if (sn < 5) {
                            this->ctx->stat &= 0b11100000;
                            this->ctx->stat |= i;
                        }
                        cur = ac.sg + ptn * 8 + lineNumber % 8;
                        bool overflow = false;
                        for (int j = 0; !overflow && j < 8; j++, x++) {
                            if (x < 0) continue;
                            if (wlog[x] && !limitOver) {
                                this->ctx->stat |= 0b00100000;
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx->ram[cur] & bit[j]) {
                                    if (col && rendering) renderPosition[x] = this->palette[col];
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
