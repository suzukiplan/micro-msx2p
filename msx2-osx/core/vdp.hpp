#ifndef INCLUDE_VDP_HPP
#define INCLUDE_VDP_HPP

#include <string.h>

class VDP
{
  private:
    int colorMode;
    void* arg;
    void (*detectInterrupt)(void* arg, int ie);
    void (*detectBreak)(void* arg);

    struct DebugTool {
        void (*registerUpdateListener)(void* arg, int number, unsigned char value);
        void (*vramReadListener)(void* arg, int addr, unsigned char value);
        void (*vramWriteListener)(void* arg, int addr, unsigned char value);
        void* arg;
    } debug;

  public:
    unsigned short display[284 * 240];
    unsigned short palette[16];
    unsigned short paletteG7[16];

    struct Context {
        int bobo;
        int countH;
        int countV;
        int reserved;
        unsigned char ram[0x20000];
        unsigned char reg[64];
        unsigned char pal[16][2];
        unsigned char tmpAddr[2];
        unsigned char stat[16];
        unsigned int addr;
        unsigned char latch1;
        unsigned char latch2;
        unsigned char readBuffer;
        unsigned char command;
        unsigned short commandDX;
        unsigned short commandDY;
        unsigned short commandNX;
        unsigned short commandNY;
        unsigned char commandL;
        unsigned short commandX; // TODO: remove
        unsigned short commandY; // TODO: remove
    } ctx;

    void setRegisterUpdateListener(void* arg, void (*listener)(void* arg, int number, unsigned char value)) {
        debug.arg = arg;
        debug.registerUpdateListener = listener;
    }

    void setVramReadListener(void* arg, void (*listener)(void* arg, int addr, unsigned char value)) {
        debug.arg = arg;
        debug.vramReadListener = listener;
    }

    void setVramWriteListener(void* arg, void (*listener)(void* arg, int addr, unsigned char value)) {
        debug.arg = arg;
        debug.vramWriteListener = listener;
    }

    VDP()
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
        memset(display, 0, sizeof(display));
        memset(&ctx, 0, sizeof(ctx));
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
    inline int getVramSize() { return sizeof(ctx.ram); }
    inline int getSpriteSize() { return ctx.reg[1] & 0b00000010 ? 16 : 8; }
    inline bool isSpriteMag() { return ctx.reg[1] & 0b00000001 ? true : false; }
    inline bool isEnabledExternalVideoInput() { return ctx.reg[0] & 0b00000001 ? true : false; }
    inline bool isEnabledScreen() { return ctx.reg[1] & 0b01000000 ? true : false; }
    inline bool isEnabledInterrupt0() { return ctx.reg[1] & 0b00100000 ? true : false; }
    inline bool isEnabledInterrupt1() { return ctx.reg[0] & 0b00010000 ? true : false; }
    inline bool isEnabledInterrupt2() { return ctx.reg[0] & 0b00100000 ? true : false; }
    inline bool isEnabledMouse() { return ctx.reg[8] & 0b10000000 ? true : false; }
    inline bool isEnabledLightPen() { return ctx.reg[8] & 0b01000000 ? true : false; }

    inline unsigned short getBackdropColor() {
        if (this->getScreenMode() == 0b00111) {
            return convertColor_8bit_to_16bit(ctx.reg[7]);
        } else {
            return palette[ctx.reg[7] & 0b00001111];
        }
    }

    inline unsigned short getTextColor() {
        if (this->getScreenMode() == 0b00111) {
            return 0;
        } else {
            return palette[(ctx.reg[7] & 0b11110000) >> 4];
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
        if (0 <= y && y < 240 && 0 <= x && x < 284) {
            auto renderPosition = &this->display[y * 284];
            renderPosition[x] = this->getBackdropColor();
            if (283 == x) {
                this->renderScanline(y - 24, &renderPosition[13]);
            }
        }
        // increment H/V counter
        this->ctx.countH++;
        if (37 == this->ctx.countH) {
            this->ctx.stat[2] &= 0b11011111; // reset HR flag
            if (this->isEnabledInterrupt1()) {
                this->ctx.stat[1] |= this->ctx.countV == this->ctx.reg[19] ? 0x01 : 0x00;
                this->detectInterrupt(this->arg, 1);
            }
        } else if (293 == this->ctx.countH) {
            this->ctx.stat[2] |= 0b00100000; // set HR flag
        } else if (342 == this->ctx.countH) {
            this->ctx.countH -= 342;
            switch (++this->ctx.countV) {
                case 251:
                    this->ctx.stat[0] |= 0x80;
                    if (this->isEnabledInterrupt0()) {
                        this->detectInterrupt(this->arg, 0);
                    }
                    break;
                case 262:
                    this->ctx.countV -= 262;
                    this->detectBreak(this->arg);
                    break;
            }
        }
    }

    inline unsigned char readPort0()
    {
        if (debug.vramReadListener) {
            debug.vramReadListener(debug.arg, this->ctx.addr, this->ctx.readBuffer);
        }
        unsigned char result = this->ctx.readBuffer;
        this->readVideoMemory();
        this->ctx.latch1 = 0;
        return result;
    }

    inline unsigned char readPort1()
    {
        int sn = this->ctx.reg[15] & 0b00001111;
        unsigned char result = this->ctx.stat[sn];
        switch (sn) {
            case 0: this->ctx.stat[sn] &= 0b01011111; break;
            case 1: this->ctx.stat[sn] &= 0b11111110; break;
            case 2: result |= 0b10001100; break;
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

    inline void writePort0(unsigned char value)
    {
        this->ctx.addr &= this->getVramSize() - 1;
        this->ctx.readBuffer = value;
        //printf("VRAM[$%05X] = $%02X\n", this->ctx.addr, value);
        this->ctx.ram[this->ctx.addr] = this->ctx.readBuffer;
        if (this->debug.vramWriteListener) {
            this->debug.vramWriteListener(this->debug.arg, this->ctx.addr, this->ctx.readBuffer);
        }
        this->ctx.addr++;
        this->ctx.latch1 = 0;
    }

    inline void writePort1(unsigned char value)
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

    inline void writePort2(unsigned char value)
    {
        this->ctx.latch2 &= 1;
        int pn = this->ctx.reg[16] & 0b00001111;
        this->ctx.pal[pn][this->ctx.latch2++] = value;
        if (2 == this->ctx.latch2) {
            printf("update palette #%d ($%02X%02X)\n", pn, this->ctx.pal[pn][1], this->ctx.pal[pn][0]);
            updatePaletteCacheFromRegister(pn);
            this->ctx.reg[16]++;
            this->ctx.reg[16] &= 0b00001111;
        } else {
            this->ctx.pal[pn][1] = 0;
            updatePaletteCacheFromRegister(pn);
        }
    }

    inline void writePort3(unsigned char value)
    {
        // Indirect access to registers through R#17 (Control Register Pointer)
        this->updateRegister(this->ctx.reg[17] & 0b00111111, value);
        if ((this->ctx.reg[17] & 0b11000000) == 0b00000000) {
            this->ctx.reg[17]++;
            this->ctx.reg[17] &= 0b00111111;
        }
    }

  private:
    inline int getLineNumber()
    {
        int mode = this->getScreenMode();
        if (mode < 3 || 7 < mode) return 192;
        return this->ctx.reg[9] & 0x80 ? 212 : 192;
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

    inline void updateG7PaletteCache(int pn, unsigned short msxGRB)
    {
        unsigned short r = msxGRB & 0b000111000;
        unsigned short g = msxGRB & 0b111000000;
        unsigned short b = msxGRB & 0b000000111;
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
        this->paletteG7[pn] = r | g | b;
    }

    inline const char* where(int addr) {
        static const char* answer[] = {
            "UNKNOWN",
            "NameTable",
            "PatternGenerator",
            "ColorTable",
        };
        int nt = this->getNameTableAddress();
        int pg = this->getPatternGeneratorAddress();
        int ct = this->getColorTableAddress();
        if (nt <= addr && addr < nt + this->getNameTableSize()) {
            return answer[1];
        }
        if (pg <= addr && addr < pg + this->getPatternGeneratorSize()) {
            return answer[2];
        }
        if (ct <= addr && addr < ct + this->getColorTableSize()) {
            return answer[3];
        }
        return answer[0];
    }

    inline void updateAddress()
    {
        this->ctx.addr = this->ctx.tmpAddr[1] & 0b00111111;
        this->ctx.addr <<= 8;
        this->ctx.addr |= this->ctx.tmpAddr[0];
        unsigned int ha = this->ctx.reg[14] & 0b00000111;
        ha <<= 14;
        this->ctx.addr += ha;
        this->ctx.addr += this->ctx.reg[45] & 0b01000000 ? 0x10000 : 0;
        printf("update VRAM address: $%05X (%s)\n", this->ctx.addr, this->where(this->ctx.addr));
    }

    inline void readVideoMemory()
    {
        this->ctx.addr &= this->getVramSize() - 1;
        this->ctx.readBuffer = this->ctx.ram[this->ctx.addr & 0x1FFFF];
        this->ctx.addr++;
    }

    inline void updateRegister(int rn, unsigned char value)
    {
        if (debug.registerUpdateListener) {
            debug.registerUpdateListener(debug.arg, rn, value);
        }
#ifdef DEBUG
        int vramSize = getVramSize();
        int screenMode = this->getScreenMode();
        bool screen = this->isEnabledScreen();
        bool externalVideoInput = this->isEnabledExternalVideoInput();
        int patternGeneratorAddsress = this->getPatternGeneratorAddress();
        int nameTableAddress = this->getNameTableAddress();
        int colorTableAddress = this->getColorTableAddress();
        bool ie0 = this->isEnabledInterrupt0();
        bool ie1 = this->isEnabledInterrupt1();
        bool ie2 = this->isEnabledInterrupt2();
        int spriteSize = this->getSpriteSize();
        bool spriteMag = this->isSpriteMag();
        unsigned short backdropColor = this->getBackdropColor();
        unsigned short textColor = this->getTextColor();
#endif
        bool previousInterrupt = this->isEnabledInterrupt0();
        this->ctx.reg[rn] = value;
        if (!previousInterrupt && this->isEnabledInterrupt0() && this->ctx.stat[0] & 0x80) {
            this->detectInterrupt(this->arg, 0);
        }
        if (44 == rn && this->ctx.command) {
            switch (this->ctx.command) {
                case 0b1111: this->executeCommandHMMC(false); break;
                case 0b1011: this->executeCommandLMMC(false); break;
            }
        } else if (46 == rn) {
            this->executeCommand((value & 0xF0) >> 4, value & 0x0F);
        } else if (16 == rn) {
            this->ctx.latch2 = 0;
        }
#ifdef DEBUG
        if (vramSize != getVramSize()) {
            printf("Change VDP RAM size: %d -> %d\n", vramSize, getVramSize());
        }
        if (screen != this->isEnabledScreen()) {
            printf("Change VDP screen enabled: %s\n", this->isEnabledScreen() ? "ENABLED" : "DISABLED");
        }
        if (screenMode != this->getScreenMode()) {
            printf("Screen Mode Changed: %d -> %d\n", screenMode, this->getScreenMode());
        }
        if (patternGeneratorAddsress != this->getPatternGeneratorAddress()) {
            printf("Pattern Generator Address: $%04X -> $%04X\n", patternGeneratorAddsress, this->getPatternGeneratorAddress());
        }
        if (nameTableAddress != this->getNameTableAddress()) {
            printf("Name Table Address: $%04X -> $%04X\n", nameTableAddress, this->getNameTableAddress());
        }
        if (colorTableAddress != this->getColorTableAddress()) {
            printf("Color Table Address: $%04X -> $%04X\n", colorTableAddress, this->getColorTableAddress());
        }
        if (externalVideoInput != this->isEnabledExternalVideoInput()) {
            printf("Change VDP external video input enabled: %s\n", this->isEnabledExternalVideoInput() ? "ENABLED" : "DISABLED");
        }
        if (ie0 != this->isEnabledInterrupt0()) {
            printf("Change Enabled Interrupt 0: %s\n", ie0 ? "OFF" : "ON");
        }
        if (ie1 != this->isEnabledInterrupt1()) {
            printf("Change Enabled Interrupt 1: %s\n", ie0 ? "OFF" : "ON");
        }
        if (ie2 != this->isEnabledInterrupt2()) {
            printf("Change Enabled Interrupt 2: %s\n", ie0 ? "OFF" : "ON");
        }
        if (spriteSize != this->getSpriteSize()) {
            printf("Change Sprite Size: %d -> %d\n", spriteSize, this->getSpriteSize());
        }
        if (spriteMag != this->isSpriteMag()) {
            printf("Change Sprite MAG mode: %s\n", spriteMag ? "OFF" : "ON");
        }
        if (backdropColor != this->getBackdropColor()) {
            printf("Change Backdrop Color: $%04X -> $%04X\n", backdropColor, this->getBackdropColor());
        }
        if (textColor != this->getTextColor()) {
            printf("Change Text Color: $%04X -> $%04X\n", textColor, this->getTextColor());
        }
#endif
    }

    inline void renderScanline(int lineNumber, unsigned short* renderPosition)
    {
        if (0 <= lineNumber && lineNumber < this->getLineNumber()) {
            this->ctx.stat[2] &= 0b10111111; // reset VR flag
            if (this->isEnabledScreen()) {
                // 00 000 : GRAPHIC1    256x192             Mode1   chr           16KB
                // 00 001 : GRAPHIC2    256x192             Mode1   chr           16KB
                // 00 010 : GRAPHIC3    256x192             Mode2   chr           16KB
                // 00 011 : GRAPHIC4    256x292 or 256x212  Mode2   bitmap(4bit)  64KB
                // 00 100 : GRAPHIC5    512x292 or 512x212  Mode2   bitmap(2bit)  64KB
                // 00 101 : GRAPHIC6    512x292 or 512x212  Mode2   bitmap(4bit)  128KB
                // 00 111 : GRAPHIC7    512x292 or 512x212  Mode2   bitmap(8bit)  128KB
                // 01 000 : MULTI COLOR 64x48 (4px/block)   Mode1   low bitmap    16KB
                // 10 000 : TEXT1       40x24 (6x8px/block) n/a     chr           16KB
                // 10 010 : TEXT2       80x24 (6x8px/block) n/a     chr           16KB
                switch (this->getScreenMode()) {
                    case 0b00000: // GRAPHIC1
                        this->renderScanlineModeG1(lineNumber, renderPosition);
                        break;
                    case 0b00011: // GRAPHIC4
                        this->renderScanlineModeG4(lineNumber, renderPosition);
                        break;
                        /*
                    case 0b00001: // GRAPHIC2
                        this->renderScanlineModeG23(lineNumber, false, renderPosition);
                        break;
                    case 0b00010: // GRAPHIC3
                        this->renderScanlineModeG23(lineNumber, true, renderPosition);
                        break;
                    case 0b00111: // GRAPHIC7
                        this->renderScanlineModeG7(lineNumber, renderPosition);
                        break;
                         */
                    default:
                        puts("UNSUPPORTED SCREEN MODE!");
                        exit(-1);
                        return;
                }
            } else return;
        } else {
            this->ctx.stat[2] |= 0b01000000; // set VR flag
        }
    }

    inline void renderPixel(unsigned short* renderPosition, int paletteNumber) {
        //if (!this->palette[paletteNumber]) return;
        if (!paletteNumber) return;
        *renderPosition = this->palette[paletteNumber];
    }

    inline void renderScanlineModeG1(int lineNumber, unsigned short* renderPosition)
    {
        int pn = this->getNameTableAddress();
        int ct = this->getColorTableAddress();
        int pg = this->getPatternGeneratorAddress();
        int lineNumberMod8 = lineNumber & 0b111;
        unsigned char* nam = &this->ctx.ram[pn + lineNumber / 8 * 32];
        for (int i = 0; i < 32; i++) {
            unsigned char ptn = this->ctx.ram[pg + nam[i] * 8 + lineNumberMod8];
            unsigned char c = this->ctx.ram[ct + nam[i] / 8];
            unsigned char cc[2];
            cc[1] = (c & 0xF0) >> 4;
            cc[0] = c & 0x0F;
            this->renderPixel(renderPosition++, cc[(ptn & 0b10000000) >> 7]);
            this->renderPixel(renderPosition++, cc[(ptn & 0b01000000) >> 6]);
            this->renderPixel(renderPosition++, cc[(ptn & 0b00100000) >> 5]);
            this->renderPixel(renderPosition++, cc[(ptn & 0b00010000) >> 4]);
            this->renderPixel(renderPosition++, cc[(ptn & 0b00001000) >> 3]);
            this->renderPixel(renderPosition++, cc[(ptn & 0b00000100) >> 2]);
            this->renderPixel(renderPosition++, cc[(ptn & 0b00000010) >> 1]);
            this->renderPixel(renderPosition++, cc[ptn & 0b00000001]);
        }
        //renderSpritesMode1(lineNumber, renderPosition);
    }

    inline void renderScanlineModeG23(int lineNumber, bool isSpriteMode2, unsigned short* renderPosition)
    {
        int pn = (this->ctx.reg[2] & 0b00001111) << 10;
        int ct = (this->ctx.reg[3] & 0b10000000) << 6;
        int cmask = this->ctx.reg[3] & 0b01111111;
        cmask <<= 3;
        cmask |= 0x07;
        int pg = (this->ctx.reg[4] & 0b00000100) << 11;
        int pmask = this->ctx.reg[4] & 0b00000011;
        pmask <<= 8;
        pmask |= 0xFF;
        int bd = this->ctx.reg[7] & 0b00001111;
        int pixelLine = lineNumber % 8;
        unsigned char* nam = &this->ctx.ram[pn + lineNumber / 8 * 32];
        int cur = 0;
        int ci = (lineNumber / 64) * 256;
        for (int i = 0; i < 32; i++) {
            unsigned char ptn = this->ctx.ram[pg + ((nam[i] + ci) & pmask) * 8 + pixelLine];
            unsigned char c = this->ctx.ram[ct + ((nam[i] + ci) & cmask) * 8 + pixelLine];
            unsigned char cc[2];
            cc[1] = (c & 0xF0) >> 4;
            cc[1] = cc[1] ? cc[1] : bd;
            cc[0] = c & 0x0F;
            cc[0] = cc[0] ? cc[0] : bd;
            this->renderPixel(&renderPosition[cur++], cc[(ptn & 0b10000000) >> 7]);
            this->renderPixel(&renderPosition[cur++], cc[(ptn & 0b01000000) >> 6]);
            this->renderPixel(&renderPosition[cur++], cc[(ptn & 0b00100000) >> 5]);
            this->renderPixel(&renderPosition[cur++], cc[(ptn & 0b00010000) >> 4]);
            this->renderPixel(&renderPosition[cur++], cc[(ptn & 0b00001000) >> 3]);
            this->renderPixel(&renderPosition[cur++], cc[(ptn & 0b00000100) >> 2]);
            this->renderPixel(&renderPosition[cur++], cc[(ptn & 0b00000010) >> 1]);
            this->renderPixel(&renderPosition[cur++], cc[ptn & 0b00000001]);
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
        int curP = lineNumber * 128;
        for (int i = 0; i < 128; i++) {
            this->renderPixel(&renderPosition[curD++], (this->ctx.ram[curP] & 0xF0) >> 4);
            this->renderPixel(&renderPosition[curD++], this->ctx.ram[curP++] & 0x0F);
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
        //int pn = (this->ctx.reg[2] & 0b01111111) << 10;
        int curD = lineNumber * 256;
        int curP = lineNumber * 256;
        for (int i = 0; i < 256; i++) {
            this->display[curD++] = convertColor_8bit_to_16bit(this->ctx.ram[curP++]);
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
        bool si = this->ctx.reg[1] & 0b00000010 ? true : false;
        bool mag = this->ctx.reg[1] & 0b00000001 ? true : false;
        int sa = (this->ctx.reg[5] & 0b01111111) << 7;
        int sg = (this->ctx.reg[6] & 0b00000111) << 11;
        int sn = 0;
        unsigned char dlog[256];
        unsigned char wlog[256];
        memset(dlog, 0, sizeof(dlog));
        memset(wlog, 0, sizeof(wlog));
        for (int i = 0; i < 32; i++) {
            int cur = sa + i * 4;
            unsigned char y = this->ctx.ram[cur++];
            if (208 == y) break;
            unsigned char x = this->ctx.ram[cur++];
            unsigned char ptn = this->ctx.ram[cur++];
            unsigned char col = this->ctx.ram[cur++];
            if (col & 0x80) x -= 32;
            col &= 0b00001111;
            y++;
            if (mag) {
                if (si) {
                    // 16x16 x 2
                    if (y <= lineNumber && lineNumber < y + 32) {
                        sn++;
                        if (5 <= sn) {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= 0b01000000 | i;
                            break;
                        } else {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= i;
                        }
                        int pixelLine = lineNumber - y;
                        cur = sg + (ptn & 252) * 8 + pixelLine % 16 / 2 + (pixelLine < 16 ? 0 : 8);
                        for (int j = 0; j < 16; j++, x++) {
                            if (wlog[x]) {
                                this->ctx.stat[0] |= 0b00100000;
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx.ram[cur] & bit[j / 2]) {
                                    this->renderPixel(&renderPosition[x], col);
                                    dlog[x] = col;
                                    wlog[x] = 1;
                                }
                            }
                        }
                        cur += 16;
                        for (int j = 0; j < 16; j++, x++) {
                            if (wlog[x]) {
                                this->ctx.stat[0] |= 0b00100000;
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx.ram[cur] & bit[j / 2]) {
                                    this->renderPixel(&renderPosition[x], col);
                                    dlog[x] = col;
                                    wlog[x] = 1;
                                }
                            }
                        }
                    }
                } else {
                    // 8x8 x 2
                    if (y <= lineNumber && lineNumber < y + 16) {
                        sn++;
                        if (5 <= sn) {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= 0b01000000 | i;
                            break;
                        } else {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= i;
                        }
                        cur = sg + ptn * 8 + lineNumber % 8;
                        for (int j = 0; j < 16; j++, x++) {
                            if (wlog[x]) {
                                this->ctx.stat[0] |= 0b00100000;
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx.ram[cur] & bit[j / 2]) {
                                    this->renderPixel(&renderPosition[x], col);
                                    dlog[x] = col;
                                    wlog[x] = 1;
                                }
                            }
                        }
                    }
                }
            } else {
                if (si) {
                    // 16x16 x 1
                    if (y <= lineNumber && lineNumber < y + 16) {
                        sn++;
                        if (5 <= sn) {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= 0b01000000 | i;
                            break;
                        } else {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= i;
                        }
                        int pixelLine = lineNumber - y;
                        cur = sg + (ptn & 252) * 8 + pixelLine % 8 + (pixelLine < 8 ? 0 : 8);
                        for (int j = 0; j < 8; j++, x++) {
                            if (wlog[x]) {
                                this->ctx.stat[0] |= 0b00100000;
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx.ram[cur] & bit[j]) {
                                    this->renderPixel(&renderPosition[x], col);
                                    dlog[x] = col;
                                    wlog[x] = 1;
                                }
                            }
                        }
                        cur += 16;
                        for (int j = 0; j < 8; j++, x++) {
                            if (wlog[x]) {
                                this->ctx.stat[0] |= 0b00100000;
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx.ram[cur] & bit[j]) {
                                    this->renderPixel(&renderPosition[x], col);
                                    dlog[x] = col;
                                    wlog[x] = 1;
                                }
                            }
                        }
                    }
                } else {
                    // 8x8 x 1
                    if (y <= lineNumber && lineNumber < y + 8) {
                        sn++;
                        if (5 <= sn) {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= 0b01000000 | i;
                            break;
                        } else {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= i;
                        }
                        cur = sg + ptn * 8 + lineNumber % 8;
                        for (int j = 0; j < 8; j++, x++) {
                            if (wlog[x]) {
                                this->ctx.stat[0] |= 0b00100000;
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx.ram[cur] & bit[j]) {
                                    this->renderPixel(&renderPosition[x], col);
                                    dlog[x] = col;
                                    wlog[x] = 1;
                                }
                            }
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
        bool si = this->ctx.reg[1] & 0b00000010 ? true : false;
        bool mag = this->ctx.reg[1] & 0b00000001 ? true : false;
        int sa = this->ctx.reg[10];
        sa &= 0b00000011;
        sa <<= 8;
        sa |= this->ctx.reg[5];
        sa &= 0b11111000;
        sa |= 0b00000100;
        sa <<= 7;
        int sg = this->ctx.reg[6] << 11;
        int sn = 0;
        unsigned char dlog[256];
        unsigned char wlog[256];
        memset(dlog, 0, sizeof(dlog));
        memset(wlog, 0, sizeof(wlog));
        for (int i = 0; i < 32; i++) {
            int cur = sa + i * 4;
            unsigned char y = this->ctx.ram[cur++];
            if (216 == y) break;
            unsigned char x = this->ctx.ram[cur++];
            unsigned char ptn = this->ctx.ram[cur++];
            y++;
            if (mag) {
                if (si) {
                    // 16x16 x 2
                    if (y <= lineNumber && lineNumber < y + 32) {
                        sn++;
                        if (9 <= sn) {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= 0b01000000 | i;
                            break;
                        } else {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= i;
                        }
                        int pixelLine = lineNumber - y;
                        cur = sg + (ptn & 252) * 8 + pixelLine % 16 / 2 + (pixelLine < 16 ? 0 : 8);
                        int ct = sg + 0x80 + (ptn & 252) * 16 + pixelLine % 16 / 2 + (pixelLine < 16 ? 0 : 8);
                        x -= this->ctx.ram[ct] & 0b10000000 ? 32 : 0;
                        bool cc = this->ctx.ram[ct] & 0b01000000 ? true : false;
                        bool ic = this->ctx.ram[ct] & 0b00100000 ? true : false;
                        int col = this->ctx.ram[ct] & 0b00001111;
                        for (int j = 0; j < 16; j++, x++) {
                            if (wlog[x] && !ic) {
                                this->setCollision(x, y);
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx.ram[cur] & bit[j / 2]) {
                                    this->renderPixel(&renderPosition[x], col);
                                    dlog[x] = col;
                                    wlog[x] = 1;
                                }
                            } else if (cc) {
                                if (this->ctx.ram[cur] & bit[j / 2]) {
                                    col |= dlog[x];
                                    this->renderPixel(&renderPosition[x], col);
                                    dlog[x] = col;
                                    wlog[x] = 1;
                                }
                            }
                        }
                        cur += 16;
                        for (int j = 0; j < 16; j++, x++) {
                            if (wlog[x] && !ic) {
                                this->setCollision(x, y);
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx.ram[cur] & bit[j / 2]) {
                                    this->renderPixel(&renderPosition[x], col);
                                    dlog[x] = col;
                                    wlog[x] = 1;
                                }
                            } else if (cc) {
                                if (this->ctx.ram[cur] & bit[j / 2]) {
                                    col |= dlog[x];
                                    this->renderPixel(&renderPosition[x], col);
                                    dlog[x] = col;
                                    wlog[x] = 1;
                                }
                            }
                        }
                    }
                } else {
                    // 8x8 x 2
                    if (y <= lineNumber && lineNumber < y + 16) {
                        sn++;
                        if (9 <= sn) {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= 0b01000000 | i;
                            break;
                        } else {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= i;
                        }
                        int pixelLine = lineNumber - y;
                        cur = sg + ptn * 8 + lineNumber % 8;
                        int ct = sg + 0x80 + ptn * 16 + pixelLine % 16 / 2;
                        x -= this->ctx.ram[ct] & 0b10000000 ? 32 : 0;
                        bool cc = this->ctx.ram[ct] & 0b01000000 ? true : false;
                        bool ic = this->ctx.ram[ct] & 0b00100000 ? true : false;
                        int col = this->ctx.ram[ct] & 0b00001111;
                        for (int j = 0; j < 16; j++, x++) {
                            if (wlog[x] && !ic) {
                                this->setCollision(x, y);
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx.ram[cur] & bit[j / 2]) {
                                    this->renderPixel(&renderPosition[x], col);
                                    dlog[x] = col;
                                    wlog[x] = 1;
                                }
                            } else if (cc) {
                                if (this->ctx.ram[cur] & bit[j / 2]) {
                                    col |= dlog[x];
                                    this->renderPixel(&renderPosition[x], col);
                                    dlog[x] = col;
                                    wlog[x] = 1;
                                }
                            }
                        }
                    }
                }
            } else {
                if (si) {
                    // 16x16 x 1
                    if (y <= lineNumber && lineNumber < y + 16) {
                        sn++;
                        if (9 <= sn) {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= 0b01000000 | i;
                            break;
                        } else {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= i;
                        }
                        int pixelLine = lineNumber - y;
                        cur = sg + (ptn & 252) * 8 + pixelLine % 8 + (pixelLine < 8 ? 0 : 8);
                        int ct = sg + 0x80 + (ptn & 252) * 16 + pixelLine % 8 + (pixelLine < 8 ? 0 : 8);
                        x -= this->ctx.ram[ct] & 0b10000000 ? 32 : 0;
                        bool cc = this->ctx.ram[ct] & 0b01000000 ? true : false;
                        bool ic = this->ctx.ram[ct] & 0b00100000 ? true : false;
                        int col = this->ctx.ram[ct] & 0b00001111;
                        for (int j = 0; j < 8; j++, x++) {
                            if (wlog[x] && !ic) {
                                this->setCollision(x, y);
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx.ram[cur] & bit[j]) {
                                    this->renderPixel(&renderPosition[x], col);
                                    dlog[x] = col;
                                    wlog[x] = 1;
                                }
                            } else if (cc) {
                                if (this->ctx.ram[cur] & bit[j]) {
                                    col |= dlog[x];
                                    this->renderPixel(&renderPosition[x], col);
                                    dlog[x] = col;
                                    wlog[x] = 1;
                                }
                            }
                        }
                        cur += 16;
                        for (int j = 0; j < 8; j++, x++) {
                            if (wlog[x] && !ic) {
                                this->setCollision(x, y);
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx.ram[cur] & bit[j]) {
                                    this->renderPixel(&renderPosition[x], col);
                                    dlog[x] = col;
                                    wlog[x] = 1;
                                }
                            } else if (cc) {
                                if (this->ctx.ram[cur] & bit[j]) {
                                    col |= dlog[x];
                                    this->renderPixel(&renderPosition[x], col);
                                    dlog[x] = col;
                                    wlog[x] = 1;
                                }
                            }
                        }
                    }
                } else {
                    // 8x8 x 1
                    if (y <= lineNumber && lineNumber < y + 8) {
                        sn++;
                        if (9 <= sn) {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= 0b01000000 | i;
                            break;
                        } else {
                            this->ctx.stat[0] &= 0b11100000;
                            this->ctx.stat[0] |= i;
                        }
                        int pixelLine = lineNumber - y;
                        cur = sg + ptn * 8 + lineNumber % 8;
                        int ct = sg + 0x80 + ptn * 16 + pixelLine % 8;
                        x -= this->ctx.ram[ct] & 0b10000000 ? 32 : 0;
                        bool cc = this->ctx.ram[ct] & 0b01000000 ? true : false;
                        bool ic = this->ctx.ram[ct] & 0b00100000 ? true : false;
                        int col = this->ctx.ram[ct] & 0b00001111;
                        for (int j = 0; j < 8; j++, x++) {
                            if (wlog[x] && !ic) {
                                this->setCollision(x, y);
                            }
                            if (0 == dlog[x]) {
                                if (this->ctx.ram[cur] & bit[j]) {
                                    this->renderPixel(&renderPosition[x], col);
                                    dlog[x] = col;
                                    wlog[x] = 1;
                                }
                            } else if (cc) {
                                if (this->ctx.ram[cur] & bit[j]) {
                                    col |= dlog[x];
                                    this->renderPixel(&renderPosition[x], col);
                                    dlog[x] = col;
                                    wlog[x] = 1;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    inline void executeCommand(int cm, int lo)
    {
        if (cm) {
            if (this->ctx.stat[2] & 0b00000001) return; // already executing
            this->ctx.command = cm;
            this->ctx.commandL = lo;
            this->ctx.commandX = 0;
            this->ctx.commandY = 0;
            this->ctx.stat[2] |= 0b00000001;
            switch (cm) {
                case 0b1111: this->executeCommandHMMC(true); break;
                case 0b1110: this->executeCommandYMMM(); break;
                case 0b1101: this->executeCommandHMMM(); break;
                case 0b1100: this->executeCommandHMMV(); break;
                case 0b1011: this->executeCommandLMMC(true); break;
                case 0b1010: break;
                case 0b1001: this->executeCommandLMMM(); break;
                case 0b1000: this->executeCommandLMMV(); break;
                case 0b0111: this->executeCommandLINE(); break;
                case 0b0110: this->executeCommandSRCH(); break;
                case 0b0101: this->executeCommandPSET(); break;
                case 0b0100: this->executeCommandPOINT(); break;
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

    inline void executeCommandHMMC(bool resetPosition)
    {
        if (!this->isBitmapMode()) {
            printf("Error: HMMC was executed in invalid screen mode (%d)\n", this->getScreenMode());
            exit(-1);
        }
        int screenWidth = this->getScreenWidth();
        int dpb = this->getDotPerByteX();
        int lineBytes = screenWidth / dpb;
        if (resetPosition) {
            this->ctx.commandDX = (this->ctx.reg[37] & 1) * 256 + this->ctx.reg[36];
            this->ctx.commandDY = (this->ctx.reg[39] & 3) * 256 + this->ctx.reg[38];
            this->ctx.commandNX = (this->ctx.reg[41] & 1) * 256 + this->ctx.reg[40];
            this->ctx.commandNY = (this->ctx.reg[43] & 3) * 256 + this->ctx.reg[42];
        }
        int mxd = this->ctx.reg[45] & 0b00100000 ? 0x10000 : 0;
        int diy = this->ctx.reg[45] & 0b00001000 ? -1 : 1;
        int dix = dpb * (this->ctx.reg[45] & 0b00000100 ? -1 : 1);
        int addr = this->getNameTableAddress() + mxd + this->ctx.commandDX / dpb + this->ctx.commandDY * lineBytes;
#if 1
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
#if 1
                puts("End HMMC");
#endif
                this->ctx.command = 0;
                this->ctx.stat[2] &= 0b11111110;
            }
        }
    }

    inline void executeCommandYMMM()
    {
        puts("execute YMMM (not implemented yet");
        exit(-1);
        /*
        int ex, ey, dpb;
        getEdge(&ex, &ey, &dpb);
        switch (this->getCommandAddX()) {
            case 2: this->ctx.reg[36] &= 0b11111110; break;
            case 4: this->ctx.reg[36] &= 0b11111100; break;
        }
        int sy = this->getSourceY();
        int dx = this->getDestinationX();
        int dy = this->getDestinationY();
        int ny = this->getNumberOfDotsY();
        int baseX = this->ctx.reg[45] & 0b000000100 ? 0 : dx;
        int size = baseX ? ex - baseX : baseX;
        size /= dpb;
        // NOTE: in fact, YMMM command is not completed immediatly, but it is completed immediatly.
        while (ny--) {
            memmove(&this->ctx.ram[this->getDestinationAddr(baseX, dy, 0, 0)], &this->ctx.ram[this->getDestinationAddr(baseX, sy, 0, 0)], size);
            if (this->ctx.reg[45] & 0b000001000) {
                dy--;
                sy--;
                if (dy < 0) dy += ey;
                if (sy < 0) sy += ey;
            } else {
                dy++;
                sy++;
                if (ey <= dy) dy -= ey;
                if (ey <= sy) sy -= ey;
            }
        }
        this->ctx.command = 0;
        this->ctx.stat[2] &= 0b11111110;
         */
    }

    inline void executeCommandHMMM()
    {
        puts("execute HMMM (not implemented yet");
        exit(-1);
        /*
        int ex, ey, dpb;
        getEdge(&ex, &ey, &dpb);
        switch (dpb) {
            case 2:
                this->ctx.reg[32] &= 0b11111110;
                this->ctx.reg[36] &= 0b11111110;
                this->ctx.reg[40] &= 0b11111110;
                break;
            case 4:
                this->ctx.reg[32] &= 0b11111100;
                this->ctx.reg[36] &= 0b11111100;
                this->ctx.reg[40] &= 0b11111100;
                break;
        }
        int sx = this->getSourceX();
        int sy = this->getSourceY();
        int dx = this->getDestinationX();
        int dy = this->getDestinationY();
        int nx = this->getNumberOfDotsX();
        int ny = this->getNumberOfDotsY();
        int baseX = this->ctx.reg[45] & 0b000000100 ? sx - nx : sx;
        while (baseX < 0) baseX += ex;
        int size = nx / dpb;
        if (ex - dx < size) size = ex - dx;
        // NOTE: in fact, YMMM command is not completed immediatly, but it is completed immediatly.
        while (ny--) {
            memmove(&this->ctx.ram[this->getDestinationAddr(dx, dy, 0, 0)], &this->ctx.ram[this->getDestinationAddr(baseX, sy, 0, 0)], size);
            if (this->ctx.reg[45] & 0b000001000) {
                dy--;
                sy--;
                if (dy < 0) dy += ey;
                if (sy < 0) sy += ey;
            } else {
                dy++;
                sy++;
                if (ey <= dy) dy -= ey;
                if (ey <= sy) sy -= ey;
            }
        }
        this->ctx.command = 0;
        this->ctx.stat[2] &= 0b11111110;
         */
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
        unsigned char clr = this->ctx.reg[44];
        int mxd = this->ctx.reg[45] & 0b00100000 ? 0x10000 : 0;
        int diy = this->ctx.reg[45] & 0b00001000 ? -1 : 1;
        int dix = dpb * (this->ctx.reg[45] & 0b00000100 ? -1 : 1);
        int addr = this->getNameTableAddress() + mxd + dx / dpb + dy * lineBytes;
        printf("ExecuteCommand<HMMV>: DX=%d, DY=%d, NX=%d, NY=%d, DIX=%d, DIY=%d, ADDR=$%05X, CLR=$%02X (SCREEN: %d)\n", dx, dy, nx, ny, dix, diy, addr, clr, getScreenMode());
        while (0 < ny) {
            addr &= 0x1FFFF;
            if (0 < dix) {
                memset(&this->ctx.ram[addr], clr, nx);
            } else {
                memset(&this->ctx.ram[addr + nx / dpb], clr, nx);
            }
            addr += lineBytes * diy;
            ny--;
        }
        this->ctx.command = 0;
        this->ctx.stat[2] &= 0b11111110;
    }

    inline void executeCommandLMMC(bool resetPosition)
    {
        if (!this->isBitmapMode()) {
            printf("Error: LMMC was executed in invalid screen mode (%d)\n", this->getScreenMode());
            exit(-1);
        }
        int screenWidth = this->getScreenWidth();
        int dpb = this->getDotPerByteX();
        int lineBytes = screenWidth / dpb;
        if (resetPosition) {
            this->ctx.commandDX = (this->ctx.reg[37] & 1) * 256 + this->ctx.reg[36];
            this->ctx.commandDY = (this->ctx.reg[39] & 3) * 256 + this->ctx.reg[38];
            this->ctx.commandNX = (this->ctx.reg[41] & 1) * 256 + this->ctx.reg[40];
            this->ctx.commandNY = (this->ctx.reg[43] & 3) * 256 + this->ctx.reg[42];
        }
        int mxd = this->ctx.reg[45] & 0b00100000 ? 0x10000 : 0;
        int diy = this->ctx.reg[45] & 0b00001000 ? -1 : 1;
        int dix = this->ctx.reg[45] & 0b00000100 ? -1 : 1;
        int addr = this->getNameTableAddress() + mxd + this->ctx.commandDX / dpb + this->ctx.commandDY * lineBytes;
        int dst = this->ctx.reg[44];
        if (2 == dst) dst &= 0x0F;
        else if (4 == dst) dst &= 0b11;
#if 1
        printf("ExecuteCommand<LMMC>: DX=%d, DY=%d, NX=%d, NY=%d, DIX=%d, DIY=%d, ADDR=$%05X, VAL=$%02X, LO=%X (SCREEN: %d)\n", ctx.commandDX, ctx.commandDY, ctx.commandNX, ctx.commandNY, dix, diy, addr, dst, ctx.commandL, getScreenMode());
#endif
        if (dst || 0 == (this->ctx.commandL & 0b1000)) {
            switch (dpb) {
                case 1:
                    switch (this->ctx.commandL & 0b0111) {
                        case 0b0000: this->ctx.ram[addr & 0x1FFFF] = dst; break; // IMP
                        case 0b0001: this->ctx.ram[addr & 0x1FFFF] &= dst; break; // AND
                        case 0b0010: this->ctx.ram[addr & 0x1FFFF] |= dst; break; // OR
                        case 0b0011: this->ctx.ram[addr & 0x1FFFF] ^= dst; break;// EOR
                        case 0b0100: this->ctx.ram[addr & 0x1FFFF] = 0xFF ^ dst; break; // NOT
                    }
                    break;
                case 2: {
                    unsigned char src = this->ctx.ram[addr & 0x1FFFF];
                    if (this->ctx.commandDX & 1) {
                        src &= 0x0F;
                        this->ctx.ram[addr & 0x1FFFF] &= 0xF0;
                    } else {
                        src &= 0xF0;
                        src >>= 4;
                        this->ctx.ram[addr & 0x1FFFF] &= 0x0F;
                    }
                    switch (this->ctx.commandL & 0b0111) {
                        case 0b0000: src = dst; break; // IMP
                        case 0b0001: src &= dst; break; // AND
                        case 0b0010: src |= dst; break; // OR
                        case 0b0011: src ^= dst; break; // EOR
                        case 0b0100: src = 0xFF ^ dst; break; // NOT
                    }
                    src &= 0x0F;
                    if (this->ctx.commandDX & 1) {
                        this->ctx.ram[addr & 0x1FFFF] |= src;
                    } else {
                        src <<= 4;
                        this->ctx.ram[addr & 0x1FFFF] |= src;
                    }
                    break;
                }
                case 4: {
                    unsigned char src = this->ctx.ram[addr & 0x1FFFF];
                    switch (this->ctx.commandDX & 3) {
                        case 3:
                            src &= 0b00000011;
                            this->ctx.ram[addr & 0x1FFFF] &= 0b11000000;
                            break;
                        case 2:
                            src &= 0b00001100;
                            src >>= 2;
                            this->ctx.ram[addr & 0x1FFFF] &= 0b00110000;
                            break;
                        case 1:
                            src &= 0b00110000;
                            src >>= 4;
                            this->ctx.ram[addr & 0x1FFFF] &= 0b00001100;
                            break;
                        case 0 :
                            src &= 0b11000000;
                            src >>= 6;
                            this->ctx.ram[addr & 0x1FFFF] &= 0b00000011;
                            break;
                    }
                    switch (this->ctx.commandL & 0b0111) {
                        case 0b0000: src = dst; break; // IMP
                        case 0b0001: src &= dst; break; // AND
                        case 0b0010: src |= dst; break; // OR
                        case 0b0011: src ^= dst; break; // EOR
                        case 0b0100: src = 0xFF ^ dst; break; // NOT
                    }
                    src &= 0b00000011;
                    switch (this->ctx.commandDX & 3) {
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
        this->ctx.commandDX += dix;
        this->ctx.commandNX--;
        if (this->ctx.commandNX <= 0) {
            this->ctx.commandDX = (this->ctx.reg[37] & 1) * 256 + this->ctx.reg[36];
            this->ctx.commandNX = (this->ctx.reg[41] & 1) * 256 + this->ctx.reg[40];
            this->ctx.commandDY += diy;
            this->ctx.commandNY--;
            if (this->ctx.commandNY <= 0) {
#if 1
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
        /*
        int sx = this->getSourceX();
        int sy = this->getSourceY();
        int ox = this->ctx.reg[45] & 0b000000100 ? -this->ctx.commandX : this->ctx.commandX;
        int oy = this->ctx.reg[45] & 0b000001000 ? -this->ctx.commandY : this->ctx.commandY;
        int addr = this->getDestinationAddr(sx, sy, ox, oy);
        unsigned char result = 0;
        switch (this->getCommandAddX()) {
            case 1: result = this->ctx.ram[addr]; break;
            case 2: // G4, G6 (4bit)
                if ((sx + ox) & 1) {
                    result = this->ctx.ram[addr] & 0x0F;
                } else {
                    result = (this->ctx.ram[addr] & 0xF0) >> 4;
                }
                break;
            case 4: // G5 (2bit)
                switch ((sx + ox) & 3) {
                    case 0: result = (this->ctx.ram[addr] & 0b11000000) >> 6; break;
                    case 1: result = (this->ctx.ram[addr] & 0b00110000) >> 4; break;
                    case 2: result = (this->ctx.ram[addr] & 0b00001100) >> 2; break;
                    case 3: result = this->ctx.ram[addr] & 0b00000011; break;
                }
                break;
        }
        this->ctx.commandX++;
        if (this->getNumberOfDotsX() == this->ctx.commandX) {
            this->ctx.commandX = 0;
            this->ctx.commandY++;
            if (this->getNumberOfDotsY() == this->ctx.commandY) {
                this->ctx.command = 0;
                this->ctx.stat[2] &= 0b11111110;
            }
        }
        return result;
         */
    }

    inline void executeCommandLMMM()
    {
        puts("execute LMMM (not implemented yet");
        exit(-1);
        /*
        int ex, ey, dpb;
        getEdge(&ex, &ey, &dpb);
        int sx = this->getSourceX();
        int sy = this->getSourceY();
        int dx = this->getDestinationX();
        int dy = this->getDestinationY();
        const int nxc = this->getNumberOfDotsX();
        int ny = this->getNumberOfDotsY();
        if (ny && nxc) {
            // NOTE: in fact, YMMM command is not completed immediatly, but it is completed immediatly.
            int oy = 0;
            int dix = this->ctx.reg[45] & 0b000000100 ? -1 : 1;
            int diy = this->ctx.reg[45] & 0b000001000 ? -1 : 1;
            while (ny--) {
                int nx = nxc;
                int ox = 0;
                while (nx--) {
                    int sa = this->getDestinationAddr(sx, sy, ox, oy);
                    int da = this->getDestinationAddr(dx, dy, ox, oy);
                    unsigned char sc = this->ctx.ram[sa];
                    unsigned char dc = this->ctx.ram[da];
                    switch (dpb) {
                        case 1: this->ctx.ram[da] = this->logicalOperation(this->ctx.commandL, dc, sc); break;
                        case 2:
                            if ((sx + ox) & 1) {
                                sc = (sc & 0b11110000) >> 4;
                            } else {
                                sc = sc & 0b00001111;
                            }
                            if ((dx + ox) & 1) {
                                dc = (dc & 0b11110000) >> 4;
                                dc = this->logicalOperation(this->ctx.commandL, dc, sc);
                                dc <<= 4;
                                this->ctx.ram[da] &= 0b00001111;
                                this->ctx.ram[da] |= dc;
                            } else {
                                dc = dc & 0b00001111;
                                dc = this->logicalOperation(this->ctx.commandL, dc, sc);
                                this->ctx.ram[da] &= 0b11110000;
                                this->ctx.ram[da] |= dc;
                            }
                            break;
                        case 4:
                            switch ((sx + ox) & 3) {
                                case 0: sc = (sc & 0b11000000) >> 6; break;
                                case 1: sc = (sc & 0b00110000) >> 4; break;
                                case 2: sc = (sc & 0b00001100) >> 2; break;
                                case 3: sc &= 0b00000011; break;
                            }
                            switch ((dx + ox) & 3) {
                                case 0:
                                    dc = (dc & 0b11000000) >> 6;
                                    dc = this->logicalOperation(this->ctx.commandL, dc, sc);
                                    dc <<= 6;
                                    this->ctx.ram[da] &= 0b00111111;
                                    this->ctx.ram[da] |= dc;
                                    break;
                                case 1:
                                    dc = (dc & 0b00110000) >> 4;
                                    dc = this->logicalOperation(this->ctx.commandL, dc, sc);
                                    dc <<= 4;
                                    this->ctx.ram[da] &= 0b11001111;
                                    this->ctx.ram[da] |= dc;
                                    break;
                                case 2:
                                    dc = (dc & 0b00001100) >> 2;
                                    dc = this->logicalOperation(this->ctx.commandL, dc, sc);
                                    dc <<= 2;
                                    this->ctx.ram[da] &= 0b11110011;
                                    this->ctx.ram[da] |= dc;
                                    break;
                                case 3:
                                    dc = dc & 0b00000011;
                                    dc = this->logicalOperation(this->ctx.commandL, dc, sc);
                                    this->ctx.ram[da] &= 0b11111100;
                                    this->ctx.ram[da] |= dc;
                                    break;
                            }
                            break;
                    }
                    ox += dix;
                }
                oy += diy;
            }
        }
        this->ctx.command = 0;
        this->ctx.stat[2] &= 0b11111110;
         */
    }

    inline void executeCommandLMMV()
    {
        puts("execute LMMV (not implemented yet");
        exit(-1);
        /*
        int ex, ey, dpb;
        getEdge(&ex, &ey, &dpb);
        int dx = this->getDestinationX();
        int dy = this->getDestinationY();
        const int nxc = this->getNumberOfDotsX();
        int ny = this->getNumberOfDotsY();
        if (ny && nxc) {
            // NOTE: in fact, YMMM command is not completed immediatly, but it is completed immediatly.
            int oy = 0;
            int dix = this->ctx.reg[45] & 0b000000100 ? -1 : 1;
            int diy = this->ctx.reg[45] & 0b000001000 ? -1 : 1;
            while (ny--) {
                int nx = nxc;
                int ox = 0;
                while (nx--) {
                    this->drawLogicalPixel(dx, dy, ox, oy, dpb);
                    ox += dix;
                }
                oy += diy;
            }
        }
        this->ctx.command = 0;
        this->ctx.stat[2] &= 0b11111110;
         */
    }

    inline void executeCommandLINE()
    {
        puts("execute LINE (not implemented yet");
        exit(-1);
        /*
        int ex, ey, dpb;
        getEdge(&ex, &ey, &dpb);
        int x1, y1, x2, y2;
        int nx = this->getNumberOfDotsX();
        int ny = this->getNumberOfDotsY();
        bool isLongX = 0 == (this->ctx.reg[45] & 0b00000001) ? true : false;
        {
            int dx = this->getDestinationX();
            int dy = this->getDestinationY();
            int dix = this->ctx.reg[45] & 0b000000100 ? -1 : 1;
            int diy = this->ctx.reg[45] & 0b000001000 ? -1 : 1;
            if (0 < dix) {
                x1 = dx;
                x2 = dx + nx;
            } else {
                x1 = dx - nx;
                x2 = dx;
            }
            if (0 < diy) {
                y1 = dy;
                y2 = dy + ny;
            } else {
                y1 = dy - ny;
                y2 = dy;
            }
        }
        int dx, dy, x, y, e, dx2, dy2;
        dx = x2 - x1;
        dy = y2 - y1;
        dx2 = dx << 1;
        dy2 = dy << 1;
        // NOTE: in fact, YMMM command is not completed immediatly, but it is completed immediatly.
        if (isLongX) {
            for (x = 0, e = 0, y = 0; x <= dx; x++) {
                this->drawLogicalPixel(x1, y1, x, y, dpb);
                e += dy2;
                if (dx <= e) {
                    y++;
                    e -= dx2;
                }
            }
        } else {
            for (x = 0, e = 0, y = 0; y <= dy; y++) {
                this->drawLogicalPixel(x1, y1, x, y, dpb);
                e += dx2;
                if (dy <= e) {
                    x++;
                    e -= dy2;
                }
            }
        }
        this->ctx.command = 0;
        this->ctx.stat[2] &= 0b11111110;
         */
    }

    inline void executeCommandSRCH()
    {
        puts("execute SRCH (not implemented yet");
        exit(-1);
        /*
        int ex, ey, dpb;
        getEdge(&ex, &ey, &dpb);
        int sx = this->getSourceX();
        int sy = this->getSourceY();
        unsigned char sc = this->ctx.reg[44];
        switch (dpb) {
            case 2: sc &= 0b00001111; break;
            case 4: sc &= 0b00000011; break;
        }
        int dix = this->ctx.reg[45] & 0b000000100 ? -1 : 1;
        bool eq = this->ctx.reg[45] & 0b000000010 ? true : false;
        for (int ox = 0; 0 <= sx + ox && sx + ox < ex; ox += dix) {
            if (sc == this->getLogicalPixel(sx, sy, ox, 0, dpb)) {
                if (!eq) {
                    // found
                    this->ctx.command = 0;
                    this->ctx.stat[2] &= 0b11111110;
                    this->ctx.stat[2] |= 0b00010000;
                    this->ctx.stat[8] = (sx + ox) & 0xFF;
                    this->ctx.stat[9] = ((sx + ox) & 0xFF00) >> 8;
                    return;
                }
            } else {
                if (eq) {
                    // found
                    this->ctx.command = 0;
                    this->ctx.stat[2] &= 0b11111110;
                    this->ctx.stat[2] |= 0b00010000;
                    this->ctx.stat[8] = (sx + ox) & 0xFF;
                    this->ctx.stat[9] = ((sx + ox) & 0xFF00) >> 8;
                    return;
                }
            }
        }
        this->ctx.command = 0;
        this->ctx.stat[2] &= 0b11101110;
         */
    }

    inline void executeCommandPSET()
    {
        puts("execute PSET (not implemented yet");
        exit(-1);
        /*
        int ex, ey, dpb;
        getEdge(&ex, &ey, &dpb);
        int dx = this->getDestinationX();
        int dy = this->getDestinationY();
        this->drawLogicalPixel(dx, dy, 0, 0, dpb);
        this->ctx.command = 0;
        this->ctx.stat[2] &= 0b11111110;
         */
    }

    inline void executeCommandPOINT()
    {
        puts("execute POINT (not implemented yet");
        exit(-1);
        /*
        int ex, ey, dpb;
        getEdge(&ex, &ey, &dpb);
        int sx = this->getSourceX();
        int sy = this->getSourceY();
        this->ctx.stat[7] = this->getLogicalPixel(sx, sy, 0, 0, dpb);
        this->ctx.command = 0;
        this->ctx.stat[2] &= 0b11111110;
         */
    }

    inline void drawLogicalPixel(int x, int y, int ox, int oy, int dpb)
    {
        int da = this->getDestinationAddr(x, y, ox, oy);
        unsigned char sc = this->ctx.reg[44];
        unsigned char dc = this->ctx.ram[da];
        switch (dpb) {
            case 1: this->ctx.ram[da] = this->logicalOperation(this->ctx.commandL, dc, sc); break;
            case 2:
                if ((x + ox) & 1) {
                    dc = (dc & 0b11110000) >> 4;
                    dc = this->logicalOperation(this->ctx.commandL, dc, sc);
                    dc <<= 4;
                    this->ctx.ram[da] &= 0b00001111;
                    this->ctx.ram[da] |= dc;
                } else {
                    dc = dc & 0b00001111;
                    dc = this->logicalOperation(this->ctx.commandL, dc, sc);
                    this->ctx.ram[da] &= 0b11110000;
                    this->ctx.ram[da] |= dc;
                }
                break;
            case 4:
                switch ((x + ox) & 3) {
                    case 0:
                        dc = (dc & 0b11000000) >> 6;
                        dc = this->logicalOperation(this->ctx.commandL, dc, sc);
                        dc <<= 6;
                        this->ctx.ram[da] &= 0b00111111;
                        this->ctx.ram[da] |= dc;
                        break;
                    case 1:
                        dc = (dc & 0b00110000) >> 4;
                        dc = this->logicalOperation(this->ctx.commandL, dc, sc);
                        dc <<= 4;
                        this->ctx.ram[da] &= 0b11001111;
                        this->ctx.ram[da] |= dc;
                        break;
                    case 2:
                        dc = (dc & 0b00001100) >> 2;
                        dc = this->logicalOperation(this->ctx.commandL, dc, sc);
                        dc <<= 2;
                        this->ctx.ram[da] &= 0b11110011;
                        this->ctx.ram[da] |= dc;
                        break;
                    case 3:
                        dc = dc & 0b00000011;
                        dc = this->logicalOperation(this->ctx.commandL, dc, sc);
                        this->ctx.ram[da] &= 0b11111100;
                        this->ctx.ram[da] |= dc;
                        break;
                }
                break;
        }
    }

    inline unsigned char getLogicalPixel(int x, int y, int ox, int oy, int dpb)
    {
        int da = this->getDestinationAddr(x, y, ox, oy);
        unsigned char dc = this->ctx.ram[da];
        switch (dpb) {
            case 1: return dc;
            case 2:
                if ((x + ox) & 1) {
                    dc = (dc & 0b11110000) >> 4;
                } else {
                    dc = dc & 0b00001111;
                }
                return dc;
            case 4:
                switch ((x + ox) & 3) {
                    case 0: return (dc & 0b11000000) >> 6;
                    case 1: return (dc & 0b00110000) >> 4;
                    case 2: return (dc & 0b00001100) >> 2;
                    case 3: return dc & 0b00000011;
                }
                return 0;
        }
        return 0;
    }
};

#endif // INCLUDE_VDP_HPP
