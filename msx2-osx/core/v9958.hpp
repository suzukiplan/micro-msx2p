#ifndef INCLUDE_V9958_HPP
#define INCLUDE_V9958_HPP

#include <string.h>
//#define COMMAND_DEBUG

class V9958
{
  private:
    int colorMode;
    void* arg;
    void (*detectInterrupt)(void* arg, int ie);
    void (*detectBreak)(void* arg);

    struct DebugTool {
        void (*registerUpdateListener)(void* arg, int number, unsigned char value);
        void (*vramAddrChangedListener)(void* arg, int addr);
        void (*vramReadListener)(void* arg, int addr, unsigned char value);
        void (*vramWriteListener)(void* arg, int addr, unsigned char value);
        void* arg;
    } debug;
    const int adjust[16] = { 0, -1, -2, -3, -4, -5, -6, -7, 1, 2, 3, 4, 5, 6, 7, 8 };

  public:
    bool renderLimitOverSprites = true;
    unsigned short display[568 * 240];
    unsigned short palette[16];
    unsigned char lastRenderScanline;

    struct Context {
        int bobo;
        int countH;
        int countV;
        unsigned int addr;
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
        unsigned short commandDX;
        unsigned short commandDY;
        unsigned short commandNX;
        unsigned short commandNY;
        int commandPending;
    } ctx;

    void setRegisterUpdateListener(void* arg, void (*listener)(void* arg, int number, unsigned char value)) {
        debug.arg = arg;
        debug.registerUpdateListener = listener;
    }

    void setVramAddrChangedListener(void* arg, void (*listener)(void* arg, int addr)) {
        debug.arg = arg;
        debug.vramAddrChangedListener = listener;
    }

    void setVramReadListener(void* arg, void (*listener)(void* arg, int addr, unsigned char value)) {
        debug.arg = arg;
        debug.vramReadListener = listener;
    }

    void setVramWriteListener(void* arg, void (*listener)(void* arg, int addr, unsigned char value)) {
        debug.arg = arg;
        debug.vramWriteListener = listener;
    }

    V9958()
    {
        memset(palette, 0, sizeof(palette));
        memset(&debug, 0, sizeof(debug));
        this->reset();
    }

    void initialize(int colorMode, void* arg, void (*detectInterrupt)(void*, int), void (*detectBreak)(void*))
    {
        this->colorMode = colorMode;
        this->arg = arg;
        this->detectInterrupt = detectInterrupt;
        this->detectBreak = detectBreak;
        this->reset();
    }

    void reset()
    {
        static unsigned int rgb[16] = {0x000000, 0x000000,
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
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };
        static unsigned char reg[64] = {
            0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x9b, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };
        memset(this->display, 0, sizeof(this->display));
        memset(&this->ctx, 0, sizeof(this->ctx));
        memcpy(&this->ctx.stat , stat, sizeof(stat));
        memcpy(&this->ctx.reg , reg, sizeof(reg));
        for (int i = 0; i < 16; i++) {
            this->ctx.pal[i][0] = 0;
            this->ctx.pal[i][1] = 0;
            this->ctx.pal[i][0] |= (rgb[i] & 0b111000000000000000000000) >> 17;
            this->ctx.pal[i][1] |= (rgb[i] & 0b000000001110000000000000) >> 13;
            this->ctx.pal[i][0] |= (rgb[i] & 0b000000000000000011100000) >> 5;
            updatePaletteCacheFromRegister(i);
        }
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
    inline bool isMaskLeft8px() { return ctx.reg[25] & 0b00000010 ? true : false; }
    inline bool getSP2() { return ctx.reg[25] & 0b00000001; }
    inline int getOnTime() { return (ctx.reg[13] & 0xF0) >> 4; }
    inline int getOffTime() { return ctx.reg[13] & 0x0F; }
    inline int getSyncMode() { return (ctx.reg[9] & 0b00110000) >> 4; }
    inline int getLineNumber() { return this->ctx.reg[9] & 0x80 ? 212 : 192; }
    inline bool isSprite16px() { return this->ctx.reg[1] & 0b00000010 ? true : false; }
    inline bool isSprite2x() { return this->ctx.reg[1] & 0b00000001 ? true : false; }
    inline bool isSpriteDisplay() { return this->ctx.reg[8] & 0b00000010 ? false : true; }
    inline int getAdjustX() { return this->adjust[this->ctx.reg[18] & 0x0F]; }
    inline int getAdjustY() { return this->adjust[(this->ctx.reg[18] & 0xF0) >> 4]; }

    inline int getSpriteAttributeTable() {
        int addr = this->ctx.reg[11] & 0b00000011;
        addr <<= 15;
        addr |= (this->ctx.reg[5] & 0b11111100) << 7;
        return addr;
    }

    inline int getSpriteColorTable() {
        int addr = this->ctx.reg[11] & 0b00000011;
        addr <<= 15;
        addr |= (this->ctx.reg[5] & 0b11111000) << 7;
        return addr;
    }

    inline int getSpriteGeneratorTable() {
        int addr = this->ctx.reg[6] & 0b00111111;
        addr <<= 11;
        return addr;
    }

    inline int getAddressMask() {
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

    inline unsigned short getBackdropColor() {
        switch (this->getScreenMode()) {
            case 0b00111: return this->convertColor_8bit_to_16bit(this->ctx.reg[7]);
            case 0b00100: return this->palette[this->ctx.reg[7] & 0b00000011];
            default: return this->palette[this->ctx.reg[7] & 0b00001111];
        }
    }

    inline unsigned short getTextColor() {
        if (this->getScreenMode() == 0b00111) {
            return 0;
        } else {
            return this->palette[(this->ctx.reg[7] & 0b11110000) >> 4];
        }
    }
 
    inline int getNameTableAddress() {
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

    inline int getNameTableSize() {
        switch (this->getScreenMode()) {
            case 0b00000: return 768; // GRAPHIC1
            case 0b00001: return 768; // GRAPHIC2
            case 0b00010: return 768; // GRAPHIC3
            case 0b00011: // GRAPHIC4
            case 0b00100: // GRAPHIC5
                return this->getLineNumber() == 192 ? 0x6000 : 0x6A00;
            case 0b00101: // GRAPHIC6
            case 0b00111: // GRAPHIC7
                return this->getLineNumber() == 192 ? 0xC000 : 0xD400;
            case 0b01000: return 192; // MULTI COLOR
            case 0b10000: return 40 * 24; // TEXT1
            case 0b10010: return 80 * 27; // TEXT2
            default: return 0; // n/a
        }
    }

    inline int getPatternGeneratorAddress() {
        switch (this->getScreenMode()) {
            case 0b00000: // GRAPHIC1
                return (this->ctx.reg[4] & 0b00111111) << 11;
            case 0b00001: // GRAPHIC2
            case 0b00010: // GRAPHIC3
                return (this->ctx.reg[4] & 0b00111100) << 11;
            default:
                return 0; // n/a
        }
    }

    inline int getPatternGeneratorSize() {
        switch (this->getScreenMode()) {
            case 0b00000: return 2048; // GRAPHIC1
            case 0b00001: return 6144; // GRAPHIC2
            case 0b00010: return 6144; // GRAPHIC3
            case 0b10000: return 2048; // TEXT1
            case 0b10010: return 2048; // TEXT2
            default: return 0; // n/a
        }
    }

    inline int getColorTableAddress() {
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

    inline int getColorTableSize() {
        switch (this->getScreenMode()) {
            case 0b00000: return 32; // GRAPHIC1
            case 0b00001: return 6144; // GRAPHIC2
            case 0b00010: return 6144; // GRAPHIC3
            case 0b10010: return 240; // TEXT2 (Blink Table)
            default: return 0; // n/a
        }
    }

    inline void tick()
    {
        /*
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
         *  Active display:  20 lines (RENDER / *212 line mode)
         *   Bottom border:   4 lines (RENDER)
         * Bottom blanking:   3 lines (skip)
         *   Vertical sync:   3 lines (skip)
         *           Total: 262 lines (render: 240 lines)
         * =================================================
         */
        // rendering
        int x = this->ctx.countH - 24;
        int y = this->ctx.countV - 16;
        int x2 = x << 1;
        int scanline = y - 24 - this->getAdjustY();
        if (0 <= y && y < 240 && 0 <= x && x < 284) {
            auto renderPosition = &this->display[y * 284 * 2];
            switch (this->getScreenMode()) {
                case 0b00100: // GRAPHIC5
                    renderPosition[x2] = this->palette[(this->ctx.reg[7] & 0b00001100) >> 2];
                    renderPosition[x2 + 1] = this->palette[this->ctx.reg[7] & 0b00000011];
                    break;
                default:
                    renderPosition[x2] = this->getBackdropColor();
                    renderPosition[x2 + 1] = renderPosition[x2];
            }
            if (0 == x) {
                this->ctx.stat[2] &= 0b11011111; // Reset HR flag (Horizontal Active)
            } else if (283 == x) {
                this->renderScanline(scanline, &renderPosition[13 * 2 + this->getAdjustX()]);
                this->ctx.stat[2] |= 0b00100000; // Set HR flag (Horizontal Blanking)
            }
        }
        if (0 == x) {
            if (this->isIE1()) {
                int lineNumber = scanline - 1;
                if (0 <= lineNumber && lineNumber < this->getLineNumber()) {
                    lineNumber += this->ctx.reg[23];
                    lineNumber &= 0xFF;
                    if (lineNumber == this->ctx.reg[19]) {
                        this->ctx.stat[1] |= 0b00000001;
                        this->detectInterrupt(this->arg, 1);
                    }
                }
            }
        }
        // increment H/V counter
        this->ctx.countH++;
        if (342 == this->ctx.countH) {
            this->ctx.countH -= 342;
            switch (++this->ctx.countV) {
                case 251:
                    this->ctx.stat[0] |= 0x80;
                    if (this->isIE0()) {
                        this->detectInterrupt(this->arg, 0);
                    }
                    break;
                case 262:
                    this->ctx.countV -= 262;
                    this->detectBreak(this->arg);
                    break;
            }
        }
        if (this->ctx.commandPending) {
            this->ctx.commandPending--;
            if (0 == this->ctx.commandPending) {
                this->ctx.stat[2] &= 0b11111110;
                this->ctx.command = 0;
            }
        }
    }

    inline unsigned char inPort98()
    {
        if (debug.vramReadListener) {
            debug.vramReadListener(debug.arg, this->ctx.addr, this->ctx.readBuffer);
        }
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
            case 0:
                this->ctx.stat[sn] &= 0b01011111;
                break;
            case 1:
                if (this->isIE1()) {
                    this->ctx.stat[1] &= 0b11111110;
                }
                result &= 0b11000001;
                result |= 0b00000100;
                break;
            case 2:
                result |= 0b10001100;
                break;
            case 5:
                this->ctx.stat[3] = 0;
                this->ctx.stat[4] = 0;
                this->ctx.stat[5] = 0;
                this->ctx.stat[6] = 0;
                break;
            case 7:
                if (this->ctx.command == 0b1010) {
                    result = executeCommandLMCM();
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
        if (this->debug.vramWriteListener) {
            this->debug.vramWriteListener(this->debug.arg, this->ctx.addr, this->ctx.readBuffer);
        }
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
            //printf("update palette #%d ($%02X%02X)\n", pn, this->ctx.pal[pn][1], this->ctx.pal[pn][0]);
            updatePaletteCacheFromRegister(pn);
            this->ctx.reg[16]++;
            this->ctx.reg[16] &= 0b00001111;
        } else {
            this->ctx.pal[pn][1] = 0;
            updatePaletteCacheFromRegister(pn);
        }
    }

    inline void outPort9B(unsigned char value)
    {
        // Indirect access to registers through R#17 (Control Register Pointer)
        this->updateRegister(this->ctx.reg[17] & 0b00111111, value);
        if ((this->ctx.reg[17] & 0b11000000) == 0b00000000) {
            this->ctx.reg[17]++;
            this->ctx.reg[17] &= 0b00111111;
        }
    }

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
        this->ctx.reg[0] &= 0b11110001;
        this->ctx.reg[0] |= (value & 0b00000111) << 1;
        // update M2/M1
        this->ctx.reg[1] &= 0b11100111;
        this->ctx.reg[1] |= value & 0b00011000;
        // update TP
        this->ctx.reg[8] &= 0b11011111;
        this->ctx.reg[8] |= value & 0b00100000;
    }

    inline void outPortF4(unsigned char value) {
        this->ctx.hardwareResetFlag = value;
    }

    inline unsigned char inPortF4() {
        return this->ctx.hardwareResetFlag;
    }

    inline void updatePaletteCacheFromRegister(int pn)
    {
        unsigned short r = this->ctx.pal[pn][0] & 0b01110000;
        unsigned short b = this->ctx.pal[pn][0] & 0b00000111;
        unsigned short g = this->ctx.pal[pn][1] & 0b00000111;
        switch (this->colorMode) {
            case 0: // RGB555
                r <<= 8;
                g <<= 7;
                b <<= 2;
                break;
            case 1: // RGB565
                r <<= 9;
                g <<= 8;
                b <<= 2;
                break;
            default:
                r = 0;
                g = 0;
                b = 0;
        }
        this->palette[pn] = r | g | b;
    }

    inline const char* where(int addr) {
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
        int sa = this->getSpriteAttributeTable();
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

    inline void updateAddress()
    {
        this->ctx.addr = this->ctx.tmpAddr[1] & 0b00111111;
        this->ctx.addr <<= 8;
        this->ctx.addr |= this->ctx.tmpAddr[0];
        this->ctx.addr |= ((int)(this->ctx.reg[14] & 0b00000111)) << 14;
        if (this->debug.vramAddrChangedListener) {
            this->debug.vramAddrChangedListener(this->debug.arg, this->ctx.addr);
        }
    }

    inline void readVideoMemory()
    {
        this->ctx.readBuffer = this->ctx.ram[this->ctx.addr];
        this->incrementAddress();
    }

    inline void incrementAddress() {
        this->ctx.addr++;
        this->ctx.addr &= 0x1FFFF;
        unsigned char r14 = (this->ctx.addr >> 14) & 0b00000111;
        if (r14 != this->ctx.reg[14]) {
            if (debug.registerUpdateListener) {
                debug.registerUpdateListener(debug.arg, 14, r14);
            }
            this->ctx.reg[14] = r14;
        }
    }

    inline void updateRegister(int rn, unsigned char value)
    {
        bool prevIE0 = this->isIE0();
        if (debug.registerUpdateListener) {
            debug.registerUpdateListener(debug.arg, rn, value);
        }
        this->ctx.reg[rn] = value;
        if (!prevIE0 && this->isIE0() && this->ctx.stat[0] & 0x80) {
            this->detectInterrupt(this->arg, 0);
        }
        if (44 == rn && this->ctx.command) {
            switch (this->ctx.command) {
                case 0b1111: this->executeCommandHMMC(false); break;
                case 0b1011: this->executeCommandLMMC(false); break;
            }
        } else if (46 == rn) {
            this->executeCommand((value & 0xF0) >> 4, value & 0x0F);
        } else if (14 == rn) {
            this->ctx.reg[14] &= 0b00000111;
            this->ctx.addr &= 0x3FFF;
            this->ctx.addr |= this->ctx.reg[14] << 14;
        } else if (16 == rn) {
            this->ctx.latch2 = 0;
        } else if (8 == rn) {
            this->ctx.reg[8] &= 0b00111111; // force disable mouse & light-pen
        }
    }

    inline void renderScanline(int lineNumber, unsigned short* renderPosition)
    {
        if (0 <= lineNumber && lineNumber < this->getLineNumber()) {
            this->lastRenderScanline = lineNumber;
            this->ctx.stat[2] &= 0b10111111; // reset VR flag
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
            } else return;
        } else {
            this->ctx.stat[2] |= 0b01000000; // set VR flag
        }
    }

    inline void renderPixel1(unsigned short* renderPosition, int paletteNumber) {
        if (0 == (this->ctx.reg[8] & 0b00100000) && !paletteNumber) return;
        *renderPosition = this->palette[paletteNumber];
    }

    inline void renderPixel2(unsigned short* renderPosition, int paletteNumber) {
        if (0 == (this->ctx.reg[8] & 0b00100000) && !paletteNumber) return;
        *renderPosition = this->palette[paletteNumber];
        *(renderPosition + 1) = this->palette[paletteNumber];
    }

    inline void renderPixel2S1(unsigned short* renderPosition, int paletteNumber) {
        if (!paletteNumber) return;
        *renderPosition = this->palette[paletteNumber];
        *(renderPosition + 1) = this->palette[paletteNumber];
    }

    inline void renderPixel2S2(unsigned short* renderPosition, int paletteNumber) {
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
        int curD = 0;
        int curP = ((lineNumber + this->ctx.reg[23]) & 0xFF) * 128 + this->getNameTableAddress();
        for (int i = 0; i < 128; i++) {
            this->renderPixel2(&renderPosition[curD], (this->ctx.ram[curP] & 0xF0) >> 4);
            curD += 2;
            this->renderPixel2(&renderPosition[curD], this->ctx.ram[curP++] & 0x0F);
            curD += 2;
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
        switch (this->colorMode) {
            case 0: {
                unsigned short result = (c & 0b00011100) << 10;
                result |= (c & 0b11100000) >> 3;
                result |= (c & 0b00000011) << 3;
                return result;
            }
            case 1: {
                unsigned short result = (c & 0b00011100) << 11;
                result |= (c & 0b11100000) >> 2;
                result |= (c & 0b00000011) << 3;
                return result;
            }
            default: return 0;
        }
    }

    inline void renderScanlineModeG7(int lineNumber, unsigned short* renderPosition)
    {
        int curD = 0;
        int curP = ((lineNumber + this->ctx.reg[23]) & 0xFF) * 256;
        curP += this->getNameTableAddress();
        for (int i = 0; i < 256; i++) {
            renderPosition[curD] = convertColor_8bit_to_16bit(this->ctx.ram[curP++]);
            renderPosition[curD + 1] = renderPosition[curD];
            curD += 2;
        }
        renderSpritesMode2(lineNumber, renderPosition);
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
        bool si = this->isSprite16px();
        bool mag = this->isSprite2x();
        int sa = this->getSpriteAttributeTable();
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
            if (mag) {
                if (si) {
                    // 16x16 x 2
                    if (y <= lineNumber && lineNumber < y + 32) {
                        sn++;
                        if (!col) tsn++;
                        if (5 == sn) {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= 0b01000000 | i;
                            if (!this->renderLimitOverSprites) {
                                break;
                            } else {
                                if (4 <= tsn) break;
                                limitOver = true;
                            }
                        } else if (sn < 5) {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= i;
                        }
                        int pixelLine = lineNumber - y;
                        cur = sg + (ptn & 252) * 8 + pixelLine % 16 / 2 + (pixelLine < 16 ? 0 : 8);
                        bool overflow = false;
                        for (int j = 0; !overflow && j < 16; j++, x++) {
                            if (x < 0) continue;
                            if (wlog[x] && !limitOver) {
                                this->setCollision(x, lineNumber);
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx.ram[cur] & bit[j / 2]) {
                                    this->renderPixel2S1(&renderPosition[x << 1], col);
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
                                this->setCollision(x, lineNumber);
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx.ram[cur] & bit[j / 2]) {
                                    this->renderPixel2S1(&renderPosition[x << 1], col);
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
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= 0b01000000 | i;
                            if (!this->renderLimitOverSprites) {
                                break;
                            } else {
                                if (4 <= tsn) break;
                                limitOver = true;
                            }
                        } else if (sn < 5) {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= i;
                        }
                        cur = sg + ptn * 8 + lineNumber % 8;
                        bool overflow = false;
                        for (int j = 0; !overflow && j < 16; j++, x++) {
                            if (x < 0) continue;
                            if (wlog[x] && !limitOver) {
                                this->setCollision(x, lineNumber);
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx.ram[cur] & bit[j / 2]) {
                                    this->renderPixel2S1(&renderPosition[x << 1], col);
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
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= 0b01000000 | i;
                            if (!this->renderLimitOverSprites) {
                                break;
                            } else {
                                if (4 <= tsn) break;
                                limitOver = true;
                            }
                        } else if (sn < 5) {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= i;
                        }
                        int pixelLine = lineNumber - y;
                        cur = sg + (ptn & 252) * 8 + pixelLine % 8 + (pixelLine < 8 ? 0 : 8);
                        bool overflow = false;
                        for (int j = 0; !overflow && j < 8; j++, x++) {
                            if (x < 0) continue;
                            if (wlog[x] && !limitOver) {
                                this->setCollision(x, lineNumber);
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx.ram[cur] & bit[j]) {
                                    this->renderPixel2S1(&renderPosition[x << 1], col);
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
                                this->setCollision(x, lineNumber);
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx.ram[cur] & bit[j]) {
                                    this->renderPixel2S1(&renderPosition[x << 1], col);
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
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= 0b01000000 | i;
                            if (!this->renderLimitOverSprites) {
                                break;
                            } else {
                                if (4 <= tsn) break;
                                limitOver = true;
                            }
                        } else if (sn < 5) {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= i;
                        }
                        cur = sg + ptn * 8 + lineNumber % 8;
                        bool overflow = false;
                        for (int j = 0; !overflow && j < 8; j++, x++) {
                            if (x < 0) continue;
                            if (wlog[x] && !limitOver) {
                                this->setCollision(x, lineNumber);
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx.ram[cur] & bit[j]) {
                                    this->renderPixel2S1(&renderPosition[x << 1], col);
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
        bool si = this->isSprite16px();
        bool mag = this->isSprite2x();
        int sa = this->getSpriteAttributeTable();
        int ct = this->getSpriteColorTable();
        int sg = this->getSpriteGeneratorTable();
        int sn = 0;
        int tsn = 0;
        unsigned char paletteMask = this->getScreenMode() == 0b00100 ? 0x03 : 0x0F;
        unsigned char dlog[256];
        unsigned char wlog[256];
        unsigned char clog[256];
        memset(dlog, 0, sizeof(dlog));
        memset(wlog, 0, sizeof(wlog));
        memset(clog, 0, sizeof(clog));
        bool limitOver = false;
        for (int i = 0; i < 32; i++, ct += 16) {
            int cur = sa + i * 4;
            unsigned char y = this->ctx.ram[cur++];
            if (216 == y) break;
            int x = this->ctx.ram[cur++];
            unsigned char ptn = this->ctx.ram[cur++];
            y += 1 - this->ctx.reg[23];
            if (mag) {
                if (si) {
                    // 16x16 x 2
                    if (y <= lineNumber && lineNumber < y + 32) {
                        sn++;
                        int pixelLine = lineNumber - y;
                        unsigned char col = this->ctx.ram[ct + pixelLine / 2];
                        bool ic = col & 0x20;
                        bool cc = col & 0x40;
                        if (col & 0x80) x -= 32;
                        col &= paletteMask;
                        if (!col) tsn++;
                        if (9 == sn) {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= 0b01000000 | i;
                            if (!this->renderLimitOverSprites) {
                                break;
                            } else {
                                if (8 <= tsn) break;
                                limitOver = true;
                            }
                        } else if (sn < 9) {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= i;
                        }
                        cur = sg + (ptn & 252) * 8 + pixelLine % 16 / 2 + (pixelLine < 16 ? 0 : 8);
                        bool overflow = false;
                        for (int j = 0; !overflow && j < 16; j++, x++) {
                            if (x < 0) continue;
                            if (dlog[x] && !limitOver && !ic) {
                                this->setCollision(x, lineNumber);
                            }
                            if (0 == dlog[x] || cc) {
                                if (this->ctx.ram[cur] & bit[j / 2]) {
                                    if (cc) {
                                        clog[x] |= col;
                                        this->renderPixel2S2(&renderPosition[x << 1], clog[x]);
                                    } else {
                                        this->renderPixel2S2(&renderPosition[x << 1], col);
                                        dlog[x] = col;
                                        clog[x] = col;
                                    }
                                    wlog[x] = 1;
                                }
                            }
                            overflow = x == 0xFF;
                        }
                        cur += 16;
                        for (int j = 0; !overflow && j < 16; j++, x++) {
                            if (x < 0) continue;
                            if (dlog[x] && !limitOver && !ic) {
                                this->setCollision(x, lineNumber);
                            }
                            if (0 == dlog[x] || cc) {
                                if (this->ctx.ram[cur] & bit[j / 2]) {
                                    if (cc) {
                                        clog[x] |= col;
                                        this->renderPixel2S2(&renderPosition[x << 1], clog[x]);
                                    } else {
                                        this->renderPixel2S2(&renderPosition[x << 1], col);
                                        dlog[x] = col;
                                        clog[x] = col;
                                    }
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
                        int pixelLine = lineNumber - y;
                        unsigned char col = this->ctx.ram[ct + pixelLine / 2];
                        bool ic = col & 0x20;
                        bool cc = col & 0x40;
                        if (col & 0x80) x -= 32;
                        col &= paletteMask;
                        if (!col) tsn++;
                        if (9 == sn) {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= 0b01000000 | i;
                            if (!this->renderLimitOverSprites) {
                                break;
                            } else {
                                if (8 <= tsn) break;
                                limitOver = true;
                            }
                        } else if (sn < 9) {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= i;
                        }
                        cur = sg + ptn * 8 + lineNumber % 8;
                        bool overflow = false;
                        for (int j = 0; !overflow && j < 16; j++, x++) {
                            if (x < 0) continue;
                            if (dlog[x] && !limitOver && !ic) {
                                this->setCollision(x, lineNumber);
                            }
                            if (0 == dlog[x] || cc) {
                                if (this->ctx.ram[cur] & bit[j / 2]) {
                                    if (cc) {
                                        clog[x] |= col;
                                        this->renderPixel2S2(&renderPosition[x << 1], clog[x]);
                                    } else {
                                        this->renderPixel2S2(&renderPosition[x << 1], col);
                                        dlog[x] = col;
                                        clog[x] = col;
                                    }
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
                        int pixelLine = lineNumber - y;
                        unsigned char col = this->ctx.ram[ct + pixelLine];
                        bool ic = col & 0x20;
                        bool cc = col & 0x40;
                        if (col & 0x80) x -= 32;
                        col &= paletteMask;
                        if (!col) tsn++;
                        if (9 == sn) {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= 0b01000000 | i;
                            if (!this->renderLimitOverSprites) {
                                break;
                            } else {
                                if (8 <= tsn) break;
                                limitOver = true;
                            }
                        } else if (sn < 9) {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= i;
                        }
                        cur = sg + (ptn & 252) * 8 + pixelLine % 8 + (pixelLine < 8 ? 0 : 8);
                        bool overflow = false;
                        for (int j = 0; !overflow && j < 8; j++, x++) {
                            if (x < 0) continue;
                            if (dlog[x] && !limitOver && !ic) {
                                this->setCollision(x, lineNumber);
                            }
                            if (0 == dlog[x] || cc) {
                                if (this->ctx.ram[cur] & bit[j]) {
                                    if (cc) {
                                        clog[x] |= col;
                                        this->renderPixel2S2(&renderPosition[x << 1], clog[x]);
                                    } else {
                                        this->renderPixel2S2(&renderPosition[x << 1], col);
                                        dlog[x] = col;
                                        clog[x] = col;
                                    }
                                    wlog[x] = 1;
                                }
                            }
                            overflow = x == 0xFF;
                        }
                        cur += 16;
                        for (int j = 0; !overflow && j < 8; j++, x++) {
                            if (x < 0) continue;
                            if (dlog[x] && !limitOver && !ic) {
                                this->setCollision(x, lineNumber);
                            }
                            if (0 == dlog[x] || cc) {
                                if (this->ctx.ram[cur] & bit[j]) {
                                    if (cc) {
                                        clog[x] |= col;
                                        this->renderPixel2S2(&renderPosition[x << 1], clog[x]);
                                    } else {
                                        this->renderPixel2S2(&renderPosition[x << 1], col);
                                        dlog[x] = col;
                                        clog[x] = col;
                                    }
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
                        int pixelLine = lineNumber - y;
                        unsigned char col = this->ctx.ram[ct + pixelLine];
                        if (col & 0x80) x -= 32;
                        bool cc = col & 0x40;
                        bool ic = col & 0x20;
                        col &= paletteMask;
                        if (!col) tsn++;
                        if (9 == sn) {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= 0b01000000 | i;
                            if (!this->renderLimitOverSprites) {
                                break;
                            } else {
                                if (8 <= tsn) break;
                                limitOver = true;
                            }
                        } else if (sn < 9) {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= i;
                        }
                        cur = sg + ptn * 8 + lineNumber % 8;
                        bool overflow = false;
                        for (int j = 0; !overflow && j < 8; j++, x++) {
                            if (x < 0) continue;
                            if (dlog[x] && !limitOver && !ic) {
                                this->setCollision(x, lineNumber);
                            }
                            if (0 == dlog[x] || cc) {
                                if (this->ctx.ram[cur] & bit[j]) {
                                    if (cc) {
                                        clog[x] |= col;
                                        this->renderPixel2S2(&renderPosition[x << 1], clog[x]);
                                    } else {
                                        this->renderPixel2S2(&renderPosition[x << 1], col);
                                        dlog[x] = col;
                                        clog[x] = col;
                                    }
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

    inline void setCollision(int x, int y)
    {
        this->ctx.stat[0] |= 0b00100000;
        if (!this->isEnabledMouse() && !this->isEnabledLightPen()) {
            x += 12;
            y += 8;
            this->ctx.stat[3] = x & 0xFF;
            this->ctx.stat[4] = (x & 0x0100) >> 8;
            this->ctx.stat[5] = y & 0xFF;
            this->ctx.stat[6] = (y & 0x0300) >> 8;
        }
    }

    inline void executeCommand(int cm, int lo)
    {
        if (cm) {
            if (this->ctx.stat[2] & 0b00000001) return; // already executing
            this->ctx.command = cm;
            this->ctx.commandL = lo;
            this->ctx.stat[2] |= 0b00000001;
            switch (cm) {
                case 0b1111: this->executeCommandHMMC(true); break;
                case 0b1110: this->executeCommandYMMM(); break;
                case 0b1101: this->executeCommandHMMM(); break;
                case 0b1100: this->executeCommandHMMV(); break;
                case 0b1011: this->executeCommandLMMC(true); break;
                case 0b1001: this->executeCommandLMMM(); break;
                case 0b1000: this->executeCommandLMMV(); break;
                case 0b0111: this->executeCommandLINE(); break;
                case 0b0110: this->executeCommandSRCH(); break;
                case 0b0101: this->executeCommandPSET(); break;
                case 0b0100: this->executeCommandPOINT(); break;
                default:
                    printf("UNKNOWN COMMAND: %d\n", cm);
                    exit(-1);
            }
        } else {
            this->ctx.stat[2] &= 0b11111110;
            this->ctx.command = 0;
        }
    }

    inline unsigned char logicalOperation(int lo, unsigned char dc, unsigned char sc)
    {
        if ((lo & 0b1000) && sc == 0) return dc;
        switch (lo & 0b0111) {
            case 0b000: return sc;
            case 0b001: return dc & sc;
            case 0b010: return dc | sc;
            case 0b011: return dc ^ sc;
            case 0b100: return ~sc;
            default: return dc;
        }
    }

    inline unsigned short getInt16FromRegister(int rn)
    {
        unsigned short result = this->ctx.reg[rn + 1];
        result <<= 8;
        result |= this->ctx.reg[rn];
        return result;
    }

    inline unsigned short getSourceX() { return this->getInt16FromRegister(32) & 0x01FF; }
    inline unsigned short getSourceY() { return this->getInt16FromRegister(34) & 0x03FF; }
    inline unsigned short getDestinationX() { return this->getInt16FromRegister(36) & 0x01FF; }
    inline unsigned short getDestinationY() { return this->getInt16FromRegister(38) & 0x03FF; }
    inline unsigned short getNumberOfDotsX() { return this->getInt16FromRegister(40) & 0x01FF; }
    inline unsigned short getNumberOfDotsY() { return this->getInt16FromRegister(42) & 0x03FF; }

    inline void getEdge(int* ex, int* ey, int* dotsPerByte)
    {
        switch (this->getVideoMode()) {
            case 0b01100: // G4
                *ex = 256;
                *ey = 1024;
                *dotsPerByte = 2;
                break;
            case 0b10000: // G5
                *ex = 512;
                *ey = 1024;
                *dotsPerByte = 4;
                break;
            case 0b10100: // G6
                *ex = 256;
                *ey = 512;
                *dotsPerByte = 2;
                break;
            case 0b11100: // G7
                *ex = 256;
                *ey = 512;
                *dotsPerByte = 1;
                break;
            default:
                *ex = 0;
                *ey = 0;
                *dotsPerByte = 1;
        }
    }

    inline int getDestinationAddr(int dx, int dy, int ox, int oy)
    {
        int ex, ey, dpb;
        getEdge(&ex, &ey, &dpb);
        dx += ox;
        dy += oy;
        while (ex <= dx) dx -= ex;
        while (ex < 0) dx += ex;
        while (ey <= dy) dy -= ey;
        while (dy < 0) dy += ey;
        dx /= dpb;
        switch (this->getVideoMode()) {
            case 0b01100: return dy * 128 + dx;
            case 0b10000: return dy * 128 + dx;
            case 0b10100: return dy * 128 + dx;
            case 0b11100: return dy * 256 + dx;
            default: return 0;
        }
    }

    inline int getCommandAddX()
    {
        switch (this->getVideoMode()) {
            case 0b01100: return 2; // G4
            case 0b10000: return 4; // G5
            case 0b10100: return 2; // G6
            case 0b11100: return 1; // G7
            default: return 0;
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

    inline bool isBitmapMode() {
        switch (this->getScreenMode()) {
            case 0b00011: return true; // GRAPHIC4
            case 0b00100: return true; // GRAPHIC5
            case 0b00101: return true; // GRAPHIC6
            case 0b00111: return true; // GRAPHIC7
            default: return false;
        }
    }

    inline void incrementCommandPending(int n) {
        this->ctx.commandPending += n;
    }

    inline void executeCommandHMMC(bool resetPosition)
    {
        if (!this->isBitmapMode()) {
            printf("Error: HMMC was executed in invalid screen mode (%d)\n", this->getScreenMode());
            exit(-1);
        }
        int dpb = this->getDotPerByteX();
        int lineBytes = this->getScreenWidth() / dpb;
        if (resetPosition) {
            this->ctx.commandDX = (this->ctx.reg[37] & 1) * 256 + this->ctx.reg[36];
            this->ctx.commandDY = (this->ctx.reg[39] & 3) * 256 + this->ctx.reg[38];
            this->ctx.commandNX = (this->ctx.reg[41] & 1) * 256 + this->ctx.reg[40];
            this->ctx.commandNY = (this->ctx.reg[43] & 3) * 256 + this->ctx.reg[42];
            if (0 == ctx.commandNX) ctx.commandNX = 512;
            if (0 == ctx.commandNY) ctx.commandNY = 1024;
        }
        int diy = this->ctx.reg[45] & 0b00001000 ? -1 : 1;
        int dix = dpb * (this->ctx.reg[45] & 0b00000100 ? -1 : 1);
        int addr = this->ctx.commandDX / dpb + this->ctx.commandDY * lineBytes;
#ifdef COMMAND_DEBUG
        printf("ExecuteCommand<HMMC>: DX=%d, DY=%d, NX=%d, NY=%d, DIX=%d, DIY=%d, ADDR=$%05X, VAL=$%02X (SCREEN: %d)\n", ctx.commandDX, ctx.commandDY, ctx.commandNX, ctx.commandNY, dix, diy, addr, ctx.reg[44], getScreenMode());
#endif
        this->ctx.ram[addr & 0x1FFFF] = this->ctx.reg[44];
        this->ctx.commandDX += dix;
        this->ctx.commandNX -= dpb;
        if (this->ctx.commandNX <= 0) {
            this->ctx.commandDX = (this->ctx.reg[37] & 1) * 256 + this->ctx.reg[36];
            this->ctx.commandNX = (this->ctx.reg[41] & 1) * 256 + this->ctx.reg[40];
            this->ctx.commandDY += diy;
            this->ctx.commandNY--;
            if (this->ctx.commandNY <= 0) {
#ifdef COMMAND_DEBUG
                puts("End HMMC");
#endif
                this->ctx.command = 0;
                this->ctx.stat[2] &= 0b11111110;
            }
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
        int sy = (this->ctx.reg[35] & 3) * 256 + this->ctx.reg[34];
        int dx = (this->ctx.reg[37] & 1) * 256 + this->ctx.reg[36];
        int dy = (this->ctx.reg[39] & 3) * 256 + this->ctx.reg[38];
        int ny = (this->ctx.reg[43] & 3) * 256 + this->ctx.reg[42];
        if (0 == ny) ny = 1024;
        int diy = this->ctx.reg[45] & 0b00001000 ? -1 : 1;
        int dix = this->ctx.reg[45] & 0b00000100 ? -1 : 1;
        int addrS = dx / dpb + sy * lineBytes;
        int addrD = dx / dpb + dy * lineBytes;
        printf("ExecuteCommand<YMMM>: SY=%d, DX=%d, DY=%d, NY=%d, DIX=%d, DIY=%d, ADDR(S)=$%05X, ADDR(D)=$%05X (SCREEN: %d)\n", sy, dx, dy, ny, dix, diy, addrS, addrD, getScreenMode());
        int base = 0 < dix ? 0 : dx / dpb;
        addrS -= base;
        addrD -= base;
        int size = 0 < dix ? lineBytes - dx / dpb : dx / dpb;
        this->incrementCommandPending(1);
        while (0 < ny) {
            memmove(&ctx.ram[addrD], &ctx.ram[addrS], size);
            ny--;
            addrS += diy * lineBytes;
            addrD += diy * lineBytes;
            this->incrementCommandPending(size);
        }
    }

    inline void executeCommandHMMM()
    {
        if (!this->isBitmapMode()) {
            printf("Error: HMMM was executed in invalid screen mode (%d)\n", this->getScreenMode());
            exit(-1);
        }
        int screenWidth = this->getScreenWidth();
        int dpb = this->getDotPerByteX();
        int lineBytes = screenWidth / dpb;
        int sx = (this->ctx.reg[33] & 1) * 256 + this->ctx.reg[32];
        int sy = (this->ctx.reg[35] & 3) * 256 + this->ctx.reg[34];
        int dx = (this->ctx.reg[37] & 1) * 256 + this->ctx.reg[36];
        int dy = (this->ctx.reg[39] & 3) * 256 + this->ctx.reg[38];
        int nx = (this->ctx.reg[41] & 1) * 256 + this->ctx.reg[40];
        int ny = (this->ctx.reg[43] & 3) * 256 + this->ctx.reg[42];
        if (0 == nx) nx = 512;
        if (0 == ny) ny = 1024;
        int diy = this->ctx.reg[45] & 0b00001000 ? -1 : 1;
        int dix = this->ctx.reg[45] & 0b00000100 ? -1 : 1;
        int addrS = sx / dpb + sy * lineBytes;
        int addrD = dx / dpb + dy * lineBytes;
#ifdef COMMAND_DEBUG
        printf("ExecuteCommand<HMMM>: SX=%d, SY=%d, DX=%d, DY=%d, NX=%d, NY=%d, DIX=%d, DIY=%d, ADDR(S)=$%05X, ADDR(D)=$%05X (SCREEN: %d)\n", sx, sy, dx, dy, nx, ny, dix, diy, addrS, addrD, getScreenMode());
#endif
        int base = 0 < dix ? 0 : -nx / dpb;
        this->incrementCommandPending(1);
        while (0 < ny) {
            memmove(&ctx.ram[addrD + base], &ctx.ram[addrS + base], nx / dpb);
            ny--;
            addrS += diy * lineBytes;
            addrD += diy * lineBytes;
            this->incrementCommandPending(nx);
        }
    }

    inline void executeCommandHMMV()
    {
        if (!this->isBitmapMode()) {
            printf("Error: HMMV was executed in invalid screen mode (%d)\n", this->getScreenMode());
            exit(-1);
        }
        int screenWidth = this->getScreenWidth();
        int dpb = this->getDotPerByteX();
        int lineBytes = screenWidth / dpb;
        int dx = (this->ctx.reg[37] & 1) * 256 + this->ctx.reg[36];
        int dy = (this->ctx.reg[39] & 3) * 256 + this->ctx.reg[38];
        int nx = (this->ctx.reg[41] & 1) * 256 + this->ctx.reg[40];
        int ny = (this->ctx.reg[43] & 3) * 256 + this->ctx.reg[42];
        if (0 == nx) nx = 512;
        if (0 == ny) ny = 1024;
        unsigned char clr = this->ctx.reg[44];
        int diy = this->ctx.reg[45] & 0b00001000 ? -1 : 1;
        int dix = this->ctx.reg[45] & 0b00000100 ? -1 : 1;
        int addr = dx / dpb + dy * lineBytes;
#ifdef COMMAND_DEBUG
        printf("ExecuteCommand<HMMV>: DX=%d, DY=%d, NX=%d, NY=%d, DIX=%d, DIY=%d, ADDR=$%05X, CLR=$%02X (SCREEN: %d)\n", dx, dy, nx, ny, dix, diy, addr, clr, getScreenMode());
#endif
        this->incrementCommandPending(1);
        while (0 < ny) {
            addr &= 0x1FFFF;
            if (0 < dix) {
                memset(&this->ctx.ram[addr], clr, nx / dpb);
            } else {
                memset(&this->ctx.ram[addr + nx / dpb], clr, nx);
            }
            addr += lineBytes * diy;
            ny--;
            this->incrementCommandPending(nx);
        }
    }

    inline void renderLogicalPixel(int addr, int dpb, int dx, int clr, int lo) {
        if (clr || 0 == (lo & 0b1000)) {
            switch (dpb) {
                case 1:
                    switch (lo & 0b0111) {
                        case 0b0000: this->ctx.ram[addr & 0x1FFFF] = clr; break; // IMP
                        case 0b0001: this->ctx.ram[addr & 0x1FFFF] &= clr; break; // AND
                        case 0b0010: this->ctx.ram[addr & 0x1FFFF] |= clr; break; // OR
                        case 0b0011: this->ctx.ram[addr & 0x1FFFF] ^= clr; break;// EOR
                        case 0b0100: this->ctx.ram[addr & 0x1FFFF] = 0xFF ^ clr; break; // NOT
                    }
                    break;
                case 2: {
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
                        case 0b0000: src = clr; break; // IMP
                        case 0b0001: src &= clr; break; // AND
                        case 0b0010: src |= clr; break; // OR
                        case 0b0011: src ^= clr; break; // EOR
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
                        case 0 :
                            src &= 0b11000000;
                            src >>= 6;
                            this->ctx.ram[addr & 0x1FFFF] &= 0b00111111;
                            break;
                    }
                    switch (lo & 0b0111) {
                        case 0b0000: src = clr; break; // IMP
                        case 0b0001: src &= clr; break; // AND
                        case 0b0010: src |= clr; break; // OR
                        case 0b0011: src ^= clr; break; // EOR
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

    inline void executeCommandLMMC(bool resetPosition)
    {
        if (!this->isBitmapMode()) {
            printf("Error: LMMC was executed in invalid screen mode (%d)\n", this->getScreenMode());
            exit(-1);
        }
        int dpb = this->getDotPerByteX();
        int lineBytes = this->getScreenWidth() / dpb;
        if (resetPosition) {
            this->ctx.commandDX = (this->ctx.reg[37] & 1) * 256 + this->ctx.reg[36];
            this->ctx.commandDY = (this->ctx.reg[39] & 3) * 256 + this->ctx.reg[38];
            this->ctx.commandNX = (this->ctx.reg[41] & 1) * 256 + this->ctx.reg[40];
            this->ctx.commandNY = (this->ctx.reg[43] & 3) * 256 + this->ctx.reg[42];
            if (0 == this->ctx.commandNX) this->ctx.commandNX = 512;
            if (0 == this->ctx.commandNY) this->ctx.commandNY = 1024;
        }
        int diy = this->ctx.reg[45] & 0b00001000 ? -1 : 1;
        int dix = this->ctx.reg[45] & 0b00000100 ? -1 : 1;
        int addr = this->ctx.commandDX / dpb + this->ctx.commandDY * lineBytes;
        int dst = this->ctx.reg[44];
        if (2 == dpb) dst &= 0x0F;
        else if (4 == dpb) dst &= 0b11;
#ifdef COMMAND_DEBUG
        printf("ExecuteCommand<LMMC>: DX=%d, DY=%d, NX=%d, NY=%d, DIX=%d, DIY=%d, ADDR=$%05X, VAL=$%02X, LO=%X (SCREEN: %d)\n", ctx.commandDX, ctx.commandDY, ctx.commandNX, ctx.commandNY, dix, diy, addr, dst, ctx.commandL, getScreenMode());
#endif
        renderLogicalPixel(addr, dpb, ctx.commandDX, dst, ctx.commandL);
        this->ctx.commandDX += dix;
        this->ctx.commandNX--;
        if (this->ctx.commandNX <= 0) {
            this->ctx.commandDX = (this->ctx.reg[37] & 1) * 256 + this->ctx.reg[36];
            this->ctx.commandNX = (this->ctx.reg[41] & 1) * 256 + this->ctx.reg[40];
            this->ctx.commandDY += diy;
            this->ctx.commandNY--;
            if (this->ctx.commandNY <= 0) {
#ifdef COMMAND_DEBUG
                puts("End LMMC");
#endif
                this->ctx.command = 0;
                this->ctx.stat[2] &= 0b11111110;
            }
        }
    }

    inline unsigned char executeCommandLMCM()
    {
        puts("execute LMCM (not implemented yet");
        exit(-1);
        return 0;
    }

    inline void executeCommandLMMM()
    {
        if (!this->isBitmapMode()) {
            printf("Error: LMMM was executed in invalid screen mode (%d)\n", this->getScreenMode());
            exit(-1);
        }
        int screenWidth = this->getScreenWidth();
        int dpb = this->getDotPerByteX();
        int lineBytes = screenWidth / dpb;
        int sx = (this->ctx.reg[33] & 1) * 256 + this->ctx.reg[32];
        int sy = (this->ctx.reg[35] & 3) * 256 + this->ctx.reg[34];
        int dx = (this->ctx.reg[37] & 1) * 256 + this->ctx.reg[36];
        int dy = (this->ctx.reg[39] & 3) * 256 + this->ctx.reg[38];
        int nx = (this->ctx.reg[41] & 1) * 256 + this->ctx.reg[40];
        int ny = (this->ctx.reg[43] & 3) * 256 + this->ctx.reg[42];
        if (0 == nx) nx = 512;
        if (0 == ny) ny = 1024;
        int diy = this->ctx.reg[45] & 0b00001000 ? -1 : 1;
        int dix = dpb * (this->ctx.reg[45] & 0b00000100 ? -1 : 1);
        int addrS = sx / dpb + sy * lineBytes;
        int addrD = dx / dpb + dy * lineBytes;
#ifdef COMMAND_DEBUG
        printf("ExecuteCommand<LMMM>: DX=%d, DY=%d, NX=%d, NY=%d, DIX=%d, DIY=%d, ADDR(S)=$%05X, ADDR(D)=$%05X, LO=%d (SCREEN: %d)\n", dx, dy, nx, ny, dix, diy, addrS, addrD, ctx.commandL, getScreenMode());
#endif
        int base = 0 < dix ? 0 : -nx / dpb;
        this->incrementCommandPending(1);
        while (0 < ny) {
            for (int i = 0; i < nx / dpb; i++) {
                switch (dpb) {
                    case 1:
                        this->renderLogicalPixel(addrD + base + i, dpb, 0, ctx.ram[addrS + base + i], ctx.commandL);
                        this->incrementCommandPending(1);
                        break;
                    case 2:
                        this->renderLogicalPixel(addrD + base + i, dpb, 0, (ctx.ram[addrS + base + i] & 0xF0) >> 4, ctx.commandL);
                        this->renderLogicalPixel(addrD + base + i, dpb, 1, ctx.ram[addrS + base + i] & 0x0F, ctx.commandL);
                        this->incrementCommandPending(2);
                        break;
                    case 4:
                        this->renderLogicalPixel(addrD + base + i, dpb, 0, (ctx.ram[addrS + base + i] & 0xC0) >> 6, ctx.commandL);
                        this->renderLogicalPixel(addrD + base + i, dpb, 1, (ctx.ram[addrS + base + i] & 0x30) >> 4, ctx.commandL);
                        this->renderLogicalPixel(addrD + base + i, dpb, 2, (ctx.ram[addrS + base + i] & 0x0C) >> 2, ctx.commandL);
                        this->renderLogicalPixel(addrD + base + i, dpb, 3, ctx.ram[addrS + base + i] & 0x03, ctx.commandL);
                        this->incrementCommandPending(4);
                        break;
                }
            }
            ny--;
            addrS += diy * lineBytes;
            addrD += diy * lineBytes;
        }
    }

    inline void executeCommandLMMV()
    {
        if (!this->isBitmapMode()) {
            printf("Error: LMMV was executed in invalid screen mode (%d)\n", this->getScreenMode());
            exit(-1);
        }
        int screenWidth = this->getScreenWidth();
        int dpb = this->getDotPerByteX();
        int lineBytes = screenWidth / dpb;
        int dx = (this->ctx.reg[37] & 1) * 256 + this->ctx.reg[36];
        int dy = (this->ctx.reg[39] & 3) * 256 + this->ctx.reg[38];
        int nx = (this->ctx.reg[41] & 1) * 256 + this->ctx.reg[40];
        int ny = (this->ctx.reg[43] & 3) * 256 + this->ctx.reg[42];
        if (0 == nx) nx = 512;
        if (0 == ny) ny = 1024;
        const int nxc = nx;
        unsigned char clr = this->ctx.reg[44];
        int diy = this->ctx.reg[45] & 0b00001000 ? -1 : 1;
        int dix = this->ctx.reg[45] & 0b00000100 ? -1 : 1;
        int addr = dx / dpb + dy * lineBytes;
#ifdef COMMAND_DEBUG
        printf("ExecuteCommand<LMMV>: DX=%d, DY=%d, NX=%d, NY=%d, DIX=%d, DIY=%d, ADDR=$%05X, CLR=$%02X, LO=$%02X (SCREEN: %d)\n", dx, dy, nx, ny, dix, diy, addr, clr, ctx.commandL, getScreenMode());
#endif
        this->incrementCommandPending(1);
        while (0 < ny) {
            while (0 < nx) {
                addr &= 0x1FFFF;
                this->renderLogicalPixel((dx / dpb + dy * lineBytes) & 0x1FFFF, dpb, dx, clr, ctx.commandL);
                this->incrementCommandPending(1);
                dx += dix;
                nx--;
            }
            nx = nxc;
            dx -= dix * nx;
            dy += diy;
            ny--;
        }
    }

    inline void executeCommandLINE()
    {
        int dpb = this->getDotPerByteX();
        int lineBytes = this->getScreenWidth() / dpb;
        int dx = (this->ctx.reg[37] & 1) * 256 + this->ctx.reg[36];
        int dy = (this->ctx.reg[39] & 3) * 256 + this->ctx.reg[38];
        int maj = (this->ctx.reg[41] & 1) * 256 + this->ctx.reg[40];
        int min = (this->ctx.reg[43] & 3) * 256 + this->ctx.reg[42];
        unsigned char clr = this->ctx.reg[44];
        switch (this->getScreenMode()) {
            case 0b00011: clr &= 0x0F; break; // GRAPHIC4
            case 0b00100: clr &= 0x03; break; // GRAPHIC5
            case 0b00101: clr &= 0x0F; break; // GRAPHIC6
            case 0b00111: break; // GRAPHIC7
            default: return;
        }
        int diy = this->ctx.reg[45] & 0b00001000 ? -1 : 1;
        int dix = this->ctx.reg[45] & 0b00000100 ? -1 : 1;
        int m = this->ctx.reg[45] & 0b00000001;
#ifdef COMMAND_DEBUG
        printf("ExecuteCommand<LINE>: DX=%d, DY=%d, Maj=%d, Min=%d, DIX=%d, DIY=%d, MAJ=%s, CLR=$%02X, LO=%X (SCREEN: %d)\n", dx, dy, maj, min, dix, diy, m ? "Y" : "X", clr, ctx.commandL, getScreenMode());
#endif

        const double majF = (double)maj;
        const double minF = (double)min;
        this->incrementCommandPending(1);
        while (0 < maj) {
            this->renderLogicalPixel((dx / dpb + dy * lineBytes) & 0x1FFFF, dpb, dx, clr, ctx.commandL);
            this->incrementCommandPending(1);
            maj--;
            if (m) {
                dy += diy;
            } else {
                dx += dix;
            }
            if (0 < min) {
                int minN = (int)((maj / majF) * minF);
                if (minN != min) {
                    min = minN;
                    if (m) {
                        dx += dix;
                    } else {
                        dy += diy;
                    }
                }
            }
        }
    }

    inline void executeCommandSRCH()
    {
        puts("execute SRCH (not implemented yet");
        exit(-1);
    }

    inline void executeCommandPSET()
    {
        int dpb = this->getDotPerByteX();
        int lineBytes = this->getScreenWidth() / dpb;
        int dx = (this->ctx.reg[37] & 1) * 256 + this->ctx.reg[36];
        int dy = (this->ctx.reg[39] & 3) * 256 + this->ctx.reg[38];
        unsigned char clr = this->ctx.reg[44];
        switch (this->getScreenMode()) {
            case 0b00011: clr &= 0x0F; break; // GRAPHIC4
            case 0b00100: clr &= 0x03; break; // GRAPHIC5
            case 0b00101: clr &= 0x0F; break; // GRAPHIC6
            case 0b00111: break; // GRAPHIC7
            default: return;
        }
#ifdef COMMAND_DEBUG
        printf("ExecuteCommand<PSET>: DX=%d, DY=%d, CLR=$%02X, LO=%X (SCREEN: %d)\n", dx, dy, clr, ctx.commandL, getScreenMode());
#endif
        this->renderLogicalPixel((dx / dpb + dy * lineBytes) & 0x1FFFF, dpb, dx, clr, ctx.commandL);
        this->incrementCommandPending(1);
    }

    inline void executeCommandPOINT()
    {
        puts("execute POINT (not implemented yet");
        exit(-1);
    }
};

#endif // INCLUDE_V9958_HPP
