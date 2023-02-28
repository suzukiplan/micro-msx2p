#ifndef INCLUDE_MSX2_HPP
#define INCLUDE_MSX2_HPP
#include "z80.hpp"
#include "v9958.hpp"
#include "ay8910.hpp"
#include "msx2mmu.hpp"
#include "msx2def.h"
#include "msx2clock.hpp"
#include "msx2kanji.hpp"
#include "scc.hpp"
#include "tc8566af.hpp"
#include "emu2413.h"
#include "lz4.h"

class MSX2 {
private:
    const int CPU_CLOCK = 3579545;
    const int VDP_CLOCK = 5370863;
    const int PSG_CLOCK = 44100;
    short soundBuffer[65536];
    unsigned short soundBufferCursor;
    char quickSaveBuffer[1024 * 1024 * 4];
    char quickSaveBufferCompressed[1024 * 1024 * 4];
    size_t quickSaveBufferSize;
    
    struct KeyCode {
        bool exist;
        int x;
        int y;
    } keyCodes[0x80];
    
    void initKeyCode(char code, int x, int y) {
        code &= 0x7F;
        keyCodes[code].exist = true;
        keyCodes[code].x = x;
        keyCodes[code].y = y;
    }
    
public:
    Z80* cpu;
    MSX2MMU mmu;
    V9958 vdp;
    AY8910 psg;
    MSX2Clock clock;
    MSX2Kanji kanji;
    SCC scc;
    TC8566AF fdc;
    OPLL* ym2413;
    
    struct Context {
        unsigned char io[256];
        unsigned char key;
    } ctx;
    
    ~MSX2() {
        OPLL_delete(this->ym2413);
        delete this->cpu;
    }
    
    MSX2(int colorMode) {
        this->cpu = new Z80([](void* arg, unsigned short addr) {
            return ((MSX2*)arg)->mmu.read(addr);
        }, [](void* arg, unsigned short addr, unsigned char value) {
            ((MSX2*)arg)->mmu.write(addr, value);
        }, [](void* arg, unsigned short port) {
            return ((MSX2*)arg)->inPort((unsigned char) port);
        }, [](void* arg, unsigned short port, unsigned char value) {
            ((MSX2*)arg)->outPort((unsigned char) port, value);
        }, this, false);
        this->ym2413 = OPLL_new(CPU_CLOCK, 44100);
        this->vdp.initialize(colorMode, this, [](void* arg, int ie) {
            ((MSX2*)arg)->cpu->resetDebugMessage();
            ((MSX2*)arg)->cpu->generateIRQ(0x07);
        }, [](void* arg) {
            ((MSX2*)arg)->cpu->requestBreak();
        });
        this->mmu.setupCallbacks(this, [](void* arg, unsigned short addr) {
            return ((MSX2*)arg)->scc.read(addr);
        }, [](void* arg, unsigned short addr, unsigned char value) {
            ((MSX2*)arg)->scc.write(addr, value);
        }, [](void* arg, unsigned short addr) {
            switch (addr) {
                case 0x3FFA: return ((MSX2*)arg)->fdc.read(4);
                case 0x3FFB: return ((MSX2*)arg)->fdc.read(5);
                default: return (unsigned char)0xFF;
            }
        }, [](void* arg, unsigned short addr, unsigned char value) {
            switch (addr) {
                case 0x3FF8: ((MSX2*)arg)->fdc.write(2, value); break;
                case 0x3FF9: ((MSX2*)arg)->fdc.write(3, value); break;
                case 0x3FFA: ((MSX2*)arg)->fdc.write(4, value); break;
                case 0x3FFB: ((MSX2*)arg)->fdc.write(5, value); break;
            }
        }, [](void* arg, unsigned short addr, unsigned char value) {
            switch (addr) {
                case 0x3FF4: OPLL_writeIO(((MSX2*)arg)->ym2413, 0, value); break;
                case 0x3FF5: OPLL_writeIO(((MSX2*)arg)->ym2413, 1, value); break;
            }
        });
        this->fdc.setDiskReadListener(this, [](void* arg, int driveId, int sector) {
            //((MSX2*)arg)->putlog("DiskRead: drive=%d, sector=%d", driveId, sector);
        });
        this->fdc.setDiskWriteListener(this, [](void* arg, int driveId, int sector) {
            //((MSX2*)arg)->putlog("DiskWrite: drive=%d, sector=%d", driveId, sector);
        });
#if 0
        this->vdp.setRegisterUpdateListener(this, [](void* arg, int rn, unsigned char value) {
            auto this_ = (MSX2*)arg;
            //printf("Update VDP register #%d = $%02X (PC:$%04X,bobo:%d)\n", rn, value, this_->cpu->reg.PC, this_->vdp.ctx.bobo);
            int ax = this_->vdp.getAdjustX();
            int ay = this_->vdp.getAdjustY();
            int screenMode = this_->vdp.getScreenMode();
            bool screen = this_->vdp.isEnabledScreen();
            bool externalVideoInput = this_->vdp.isEnabledExternalVideoInput();
            int patternGeneratorAddsress = this_->vdp.getPatternGeneratorAddress();
            int nameTableAddress = this_->vdp.getNameTableAddress();
            int colorTableAddress = this_->vdp.getColorTableAddress();
            //bool ie0 = this_->vdp.isIE0();
            bool ie1 = this_->vdp.isIE1();
            int spriteSize = this_->vdp.getSpriteSize();
            bool spriteMag = this_->vdp.isSpriteMag();
            unsigned short backdropColor = this_->vdp.getBackdropColor();
            unsigned short textColor = this_->vdp.getTextColor();
            int onTime = this_->vdp.getOnTime();
            int offTime = this_->vdp.getOffTime();
            int syncMode = this_->vdp.getSyncMode();
            int lineNumber = this_->vdp.getLineNumber();
            int ie1Line = this_->vdp.ctx.reg[19];
            int scrollV = this_->vdp.ctx.reg[23];
            int scrollH = (this_->vdp.ctx.reg[26] & 0b00111111) * -8 + (this_->vdp.ctx.reg[27] & 0b00000111);
            bool spriteDisplay = this_->vdp.isSpriteDisplay();
            int sa = this_->vdp.getSpriteAttributeTableM2();
            int sg = this_->vdp.getSpriteGeneratorTable();
            unsigned char r14 = this_->vdp.ctx.reg[14];
            
            this_->vdp.ctx.reg[rn] = value;
            
            if (screen != this_->vdp.isEnabledScreen()) {
                this_->putlog("Change VDP screen enabled: %s", this_->vdp.isEnabledScreen() ? "ENABLED" : "DISABLED");
            }
            if (screenMode != this_->vdp.getScreenMode()) {
                this_->putlog("Screen Mode Changed: %d -> %d", screenMode, this_->vdp.getScreenMode());
            }
            if (patternGeneratorAddsress != this_->vdp.getPatternGeneratorAddress()) {
                this_->putlog("Pattern Generator Address: $%04X -> $%04X", patternGeneratorAddsress, this_->vdp.getPatternGeneratorAddress());
            }
            if (nameTableAddress != this_->vdp.getNameTableAddress()) {
                this_->putlog("Name Table Address: $%04X -> $%04X", nameTableAddress, this_->vdp.getNameTableAddress());
            }
            if (sa != this_->vdp.getSpriteAttributeTableM2()) {
                this_->putlog("Sprite Attribute Table Address: $%04X -> $%04X", sa, this_->vdp.getSpriteAttributeTableM2());
            }
            if (sg != this_->vdp.getSpriteGeneratorTable()) {
                this_->putlog("Sprite Generator Table Address: $%04X -> $%04X", sg, this_->vdp.getSpriteGeneratorTable());
            }
            if (colorTableAddress != this_->vdp.getColorTableAddress()) {
                this_->putlog("Color Table Address: $%04X -> $%04X", colorTableAddress, this_->vdp.getColorTableAddress());
            }
            if (externalVideoInput != this_->vdp.isEnabledExternalVideoInput()) {
                this_->putlog("Change VDP external video input enabled: %s", this_->vdp.isEnabledExternalVideoInput() ? "ENABLED" : "DISABLED");
            }
            if (r14 != this_->vdp.ctx.reg[14]) {
                this_->putlog("AddressCounter R#14: $%02X -> $%02X", r14, this_->vdp.ctx.reg[14]);
            }
            /*
             if (ie0 != this_->vdp.isIE0()) {
             this_->putlog("Change EI0: %s", ie0 ? "OFF" : "ON");
             }
             */
            if (ie1 != this_->vdp.isIE1()) {
                this_->putlog("Change IE1 enabled: %s", ie1 ? "OFF" : "ON");
            }
            if (ie1Line != this_->vdp.ctx.reg[19]) {
                this_->putlog("Change IE1 Line: %d -> %d", ie1Line, this_->vdp.ctx.reg[19]);
            }
            if (spriteSize != this_->vdp.getSpriteSize()) {
                this_->putlog("Change Sprite Size: %d -> %d", spriteSize, this_->vdp.getSpriteSize());
            }
            if (spriteMag != this_->vdp.isSpriteMag()) {
                this_->putlog("Change Sprite MAG mode: %s", spriteMag ? "OFF" : "ON");
            }
            if (backdropColor != this_->vdp.getBackdropColor()) {
                this_->putlog("Change Backdrop Color: $%04X -> $%04X", backdropColor, this_->vdp.getBackdropColor());
            }
            if (textColor != this_->vdp.getTextColor()) {
                this_->putlog("Change Text Color: $%04X -> $%04X", textColor, this_->vdp.getTextColor());
            }
            if (onTime != this_->vdp.getOnTime()) {
                this_->putlog("OnTime changed: %d -> %d", onTime, this_->vdp.getOnTime());
            }
            if (offTime != this_->vdp.getOffTime()) {
                this_->putlog("OffTime changed: %d -> %d", offTime, this_->vdp.getOffTime());
            }
            if (syncMode != this_->vdp.getSyncMode()) {
                this_->putlog("SyncMode changed: %d -> %d", syncMode, this_->vdp.getSyncMode());
            }
            if (lineNumber != this_->vdp.getLineNumber()) {
                this_->putlog("LineNumber changed: %d -> %d", lineNumber, this_->vdp.getLineNumber());
            }
            if (scrollV != this_->vdp.ctx.reg[23]) {
                this_->putlog("Vertical Scroll changed: %d -> %d", scrollV, this_->vdp.ctx.reg[23]);
            }
            int scrollH2 = (this_->vdp.ctx.reg[26] & 0b00111111) * -8 + (this_->vdp.ctx.reg[27] & 0b00000111);
            if (scrollH != scrollH2) {
                this_->putlog("Horizontal Scroll changed: %d -> %d", scrollH, scrollH2);
            }
            
            if (spriteDisplay != this_->vdp.isSpriteDisplay()) {
                this_->putlog("Change Sprite Display: %s", spriteDisplay ? "OFF" : "ON");
            }
            if (ax != this_->vdp.getAdjustX() || ay != this_->vdp.getAdjustY()) {
                this_->putlog("Adjust(%d,%d)", this_->vdp.getAdjustX(), this_->vdp.getAdjustY());
            }
        });
#endif
        
        this->cpu->addBreakPoint(0, [](void* arg) {
            puts("RESET!");
        });
        // CALL命令をページ3にRAMを割り当てていない状態で実行した時に落とす
        this->cpu->addBreakOperand(0xCD, [](void* arg, unsigned char* op, int size) {
            int pri3 = ((MSX2*)arg)->mmu.ctx.pri[3];
            int sec3 = ((MSX2*)arg)->mmu.ctx.sec[3];
            auto data = &((MSX2*)arg)->mmu.slots[pri3][sec3].data[7];
            if (!data->isRAM) {
                ((MSX2*)arg)->putlog("invalid call $%02X%02X", op[2], op[1]);
                exit(-1);
            }
        });
        // RST命令のループ状態になったら落とす
        this->cpu->addBreakOperand(0xFF, [](void* arg, unsigned char* op, int size) {
            if (((MSX2*)arg)->cpu->reg.PC == 0x0038 + size) {
                ((MSX2*)arg)->putlog("RST loop detected");
                exit(-1);
            }
        });
        
        this->cpu->setConsumeClockCallbackFP([](void* arg, int cpuClocks) {
            ((MSX2*)arg)->consumeClock(cpuClocks);
        });
        memset(&keyCodes, 0, sizeof(keyCodes));
        initKeyCode('0', 0, 0);
        initKeyCode('1', 1, 0);
        initKeyCode('2', 2, 0);
        initKeyCode('3', 3, 0);
        initKeyCode('4', 4, 0);
        initKeyCode('5', 5, 0);
        initKeyCode('6', 6, 0);
        initKeyCode('7', 7, 0);
        initKeyCode('8', 0, 1);
        initKeyCode('9', 1, 1);
        initKeyCode('-', 2, 1);
        initKeyCode('^', 3, 1);
        initKeyCode('\\', 4, 1);
        initKeyCode('@', 5, 1);
        initKeyCode('[', 6, 1);
        initKeyCode(';', 7, 1);
        initKeyCode(':', 0, 2);
        initKeyCode(']', 1, 2);
        initKeyCode(',', 2, 2);
        initKeyCode('.', 3, 2);
        initKeyCode('/', 4, 2);
        initKeyCode('_', 5, 2);
        initKeyCode('A', 6, 2);
        initKeyCode('B', 7, 2);
        initKeyCode('C', 0, 3);
        initKeyCode('D', 1, 3);
        initKeyCode('E', 2, 3);
        initKeyCode('F', 3, 3);
        initKeyCode('G', 4, 3);
        initKeyCode('H', 5, 3);
        initKeyCode('I', 6, 3);
        initKeyCode('J', 7, 3);
        initKeyCode('K', 0, 4);
        initKeyCode('L', 1, 4);
        initKeyCode('M', 2, 4);
        initKeyCode('N', 3, 4);
        initKeyCode('O', 4, 4);
        initKeyCode('P', 5, 4);
        initKeyCode('Q', 6, 4);
        initKeyCode('R', 7, 4);
        initKeyCode('S', 0, 5);
        initKeyCode('T', 1, 5);
        initKeyCode('U', 2, 5);
        initKeyCode('V', 3, 5);
        initKeyCode('W', 4, 5);
        initKeyCode('X', 5, 5);
        initKeyCode('Y', 6, 5);
        initKeyCode('Z', 7, 5);
        initKeyCode('\r', 7, 7);
        initKeyCode('\n', 7, 7);
        initKeyCode(' ', 0, 8);
        initKeyCode(0x1B, 2, 7);
        this->reset();
    }
    
    void reset() {
        memset(this->soundBuffer, 0, sizeof(this->soundBuffer));
        this->soundBufferCursor = 0;
        memset(&this->cpu->reg, 0, sizeof(this->cpu->reg));
        memset(&this->cpu->reg.pair, 0xFF, sizeof(this->cpu->reg.pair));
        memset(&this->cpu->reg.back, 0xFF, sizeof(this->cpu->reg.back));
        memset(&this->ctx, 0, sizeof(this->ctx));
        this->cpu->reg.SP = 0xF000;
        this->cpu->reg.IX = 0xFFFF;
        this->cpu->reg.IY = 0xFFFF;
        this->mmu.reset();
        this->vdp.reset();
        this->psg.reset(27);
        this->clock.reset();
        this->kanji.reset();
        this->scc.reset();
        this->fdc.reset();
        OPLL_reset(this->ym2413);
    }
    
    void putlog(const char* fmt, ...) {
        static int seqno = 0;
        char buf[256];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        char addr[256];
        auto db = this->mmu.getDataBlock(this->cpu->reg.PC);
        if (db->isCartridge) {
            int romAddr = (int)(db->ptr - this->mmu.cartridge.ptr);
            snprintf(addr, sizeof(addr), "[PC=%04X:%X+%04X,SP=%04X]", this->cpu->reg.PC, this->cpu->reg.SP, romAddr, this->cpu->reg.PC & 0x1FFF);
        } else {
            snprintf(addr, sizeof(addr), "[PC=%04X,SP=%04X]", this->cpu->reg.PC, this->cpu->reg.SP);
        }
        printf("%7d %s %d-%d:%d-%d:%d-%d:%d-%d V:%03d %s\n", ++seqno, addr,
               this->mmu.ctx.pri[0], this->mmu.ctx.sec[0],
               this->mmu.ctx.pri[1], this->mmu.ctx.sec[1],
               this->mmu.ctx.pri[2], this->mmu.ctx.sec[2],
               this->mmu.ctx.pri[3], this->mmu.ctx.sec[3],
               this->vdp.lastRenderScanline, buf);
    }
    
    void loadFont(const void* font, size_t fontSize) {
        this->kanji.loadFont(font, fontSize);
    }
    
    void setupSecondaryExist(bool page0, bool page1, bool page2, bool page3) {
        this->mmu.setupSecondaryExist(page0, page1, page2, page3);
    }

    void setupRAM(int pri, int sec) {
        this->mmu.setupRAM(pri, sec);
    }

    void setup(int pri, int sec, int idx, void* data, int size, const char* label = NULL) {
        this->mmu.setup(pri, sec, idx, (unsigned char*)data, size, label);
    }
    
    void loadRom(void* data, int size, int romType) {
        this->mmu.setupCartridge(1, 0, 2, data, size, romType);
        this->scc.enabled = romType == MSX2_ROM_TYPE_KONAMI_SCC;
        this->reset();
    }
    
    void insertDisk(int driveId, const void *data, size_t size, bool readOnly) {
        this->fdc.insertDisk(driveId, data, size, readOnly);
    }

    void ejectDisk(int driveId) {
        this->fdc.ejectDisk(driveId);
    }

    void tick(unsigned char pad1, unsigned char pad2, unsigned char key) {
        this->psg.setPads(pad1, pad2);
        this->ctx.key = toupper(key);
        this->cpu->execute(0x7FFFFFFF);
    }
    
    void* getSound(size_t* soundSize) {
        *soundSize = this->soundBufferCursor * 2;
        this->soundBufferCursor = 0;
        return this->soundBuffer;
    }
    
    inline void consumeClock(int cpuClocks) {
        // Asynchronous with PSG/SCC
        this->psg.ctx.bobo += cpuClocks * this->PSG_CLOCK;
        while (0 < this->psg.ctx.bobo) {
            this->psg.ctx.bobo -= this->CPU_CLOCK;
            this->psg.tick(&this->soundBuffer[this->soundBufferCursor], &this->soundBuffer[this->soundBufferCursor + 1], 81);
            this->scc.tick(&this->soundBuffer[this->soundBufferCursor], &this->soundBuffer[this->soundBufferCursor + 1], 81);
            auto opllWav = OPLL_calc(this->ym2413);
            int l = this->soundBuffer[this->soundBufferCursor];
            int r = this->soundBuffer[this->soundBufferCursor + 1];
            l += opllWav;
            r += opllWav;
            if (32767 < l) l = 32767;
            else if (l < -32768) l = -32768;
            if (32767 < r) r = 32767;
            else if (r < -32768) r = -32768;
            this->soundBuffer[this->soundBufferCursor] = (short)l;
            this->soundBuffer[this->soundBufferCursor + 1] = (short)r;
            this->soundBufferCursor += 2;
        }
        // Asynchronous with VDP
        this->vdp.ctx.bobo += cpuClocks * VDP_CLOCK;
        while (0 < this->vdp.ctx.bobo) {
            this->vdp.ctx.bobo -= CPU_CLOCK;
            this->vdp.tick();
        }
        // Asynchronous with Clock IC
        this->clock.ctx.bobo += cpuClocks;
        while (CPU_CLOCK <= this->clock.ctx.bobo) {
            this->clock.ctx.bobo -= CPU_CLOCK;
            this->clock.tick();
        }
    }
    
    inline unsigned char inPort(unsigned char port) {
        switch (port) {
            case 0x81: return 0xFF; // 8251 status command
            case 0x88: return this->vdp.inPort98();
            case 0x89: return this->vdp.inPort99();
            case 0x90: return 0x00; // printer
            case 0x98: return this->vdp.inPort98();
            case 0x99: return this->vdp.inPort99();
            case 0xA2: return this->psg.read();
            case 0xA8: return this->mmu.getPrimary();
            case 0xA9: {
                // to read the keyboard matrix row specified via the port AAh. (PPI's port B is used)
                static unsigned char bit[8] = {
                    0b00000001,
                    0b00000010,
                    0b00000100,
                    0b00001000,
                    0b00010000,
                    0b00100000,
                    0b01000000,
                    0b10000000};
                unsigned char result = 0;
                if (this->ctx.key && this->keyCodes[this->ctx.key].exist) {
                    if ((this->ctx.io[0xAA] & 0x0F) == this->keyCodes[this->ctx.key].y) {
                        result |= bit[this->keyCodes[this->ctx.key].x];
                    }
                }
                return ~result;
            }
            case 0xAA: return 0x00;
                puts("IN PPI PORT C");
                exit(-1);
                break;
            case 0xB5: return this->clock.inPortB5();
            case 0xB8: return 0x00; // light pen
            case 0xB9: return 0x00; // light pen
            case 0xBA: return 0x00; // light pen
            case 0xBB: return 0xFF; // light pen
            case 0xC0: return 0xFF; // MSX-Audio (Y8950?)
            case 0xC8: return 0xFF; // MSX interface
            case 0xC9: return 0x00; // MSX interface
            case 0xCA: return 0x00; // MSX interface
            case 0xCB: return 0x00; // MSX interface
            case 0xCC: return 0x00; // MSX interface
            case 0xCD: return 0x00; // MSX interface
            case 0xCE: return 0x00; // MSX interface
            case 0xCF: return 0x00; // MSX interface
            case 0xD9: return this->kanji.inPortD9(); // kanji
            case 0xDB: return this->kanji.inPortDB(); // kanji
            case 0xF4: return this->vdp.inPortF4();
            case 0xF7: return 0xFF; // AV control
            default: printf("ignore an unknown input port $%02X\n", port);
        }
        return this->ctx.io[port];
    }
    
    inline void outPort(unsigned char port, unsigned char value) {
        this->ctx.io[port] = value;
        switch (port) {
            case 0x7C: OPLL_writeIO(this->ym2413, 0, value); break;
            case 0x7D: OPLL_writeIO(this->ym2413, 1, value); break;
            case 0x81: break; // 8251 status command
            case 0x88: this->vdp.outPort98(value); break;
            case 0x89: this->vdp.outPort99(value); break;
            case 0x8A: this->vdp.outPort9A(value); break;
            case 0x8B: this->vdp.outPort9B(value); break;
            case 0x90: break; // printer
            case 0x91: break; // printer
            case 0x98: this->vdp.outPort98(value); break;
            case 0x99: this->vdp.outPort99(value); break;
            case 0x9A: this->vdp.outPort9A(value); break;
            case 0x9B: this->vdp.outPort9B(value); break;
            case 0xA0: this->psg.latch(value); break;
            case 0xA1: this->psg.write(value); break;
            case 0xA8: this->mmu.updatePrimary(value); break;
            case 0xAA: break;
            case 0xAB: {
                if (0 == (value & 0x80)) {
                    unsigned char bitmask = (unsigned char)(1 << ((value & 0x0E) >> 1));
                    if (value & 0x01) {
                        this->ctx.io[0xAA] = this->ctx.io[0xAA] | bitmask;
                    } else {
                        this->ctx.io[0xAA] = this->ctx.io[0xAA] & ~bitmask;
                    }
                }
                break;
            }
            case 0xB4: this->clock.outPortB4(value); break;
            case 0xB5: this->clock.outPortB5(value); break;
            case 0xB8: break; // light pen
            case 0xB9: break; // light pen
            case 0xBA: break; // light pen
            case 0xBB: break; // light pen
            case 0xD8: this->kanji.outPortD8(value); break;
            case 0xD9: this->kanji.outPortD9(value); break;
            case 0xDA: this->kanji.outPortDA(value); break;
            case 0xDB: this->kanji.outPortDB(value); break;
            case 0xF3: this->vdp.outPortF3(value); break;
            case 0xF4: this->vdp.outPortF4(value); break;
            case 0xF5: {
#if 0
                putlog("Update System Control:");
                putlog(" - Kanji ROM: %s", value & 0b00000001 ? "Yes" : "No");
                putlog(" - Kanji Reserved: %s", value & 0b00000010 ? "Yes" : "No");
                putlog(" - MSX Audio: %s", value & 0b00000100 ? "Yes" : "No");
                putlog(" - Superimpose: %s", value & 0b00001000 ? "Yes" : "No");
                putlog(" - MSX Interface: %s", value & 0b00010000 ? "Yes" : "No");
                putlog(" - RS-232C: %s", value & 0b00100000 ? "Yes" : "No");
                putlog(" - Light Pen: %s", value & 0b01000000 ? "Yes" : "No");
                putlog(" - Clock-IC: %s", value & 0b10000000 ? "Yes" : "No");
                //cpu->setDebugMessage([](void* arg, const char* msg) { puts(msg); });
#endif
                break; // System Control
            }
            case 0xF7: { // AV controll
#if 0
                bool audioRLMixingON = value & 0b00000001 ? true : false;
                bool audioLLMixingOFF = value & 0b00000010 ? true : false;
                bool videoInSelectL = value & 0b00000100 ? true : false;
                bool avControlL = value & 0b00010000 ? true : false;
                bool ymControlL = value & 0b00100000 ? true : false;
                bool reverseVdpR9Bit4 = value & 0b01000000 ? true : false;
                bool reverseVdpR9Bit5 = value & 0b10000000 ? true : false;
                putlog("Update AV Control:");
                putlog(" - audioRLMixingON: %s", audioRLMixingON ? "Yes" : "No");
                putlog(" - audioLLMixingOFF: %s", audioLLMixingOFF ? "Yes" : "No");
                putlog(" - videoInSelectL: %s", videoInSelectL ? "Yes" : "No");
                putlog(" - avControlL: %s", avControlL ? "Yes" : "No");
                putlog(" - ymControlL: %s", ymControlL ? "Yes" : "No");
                putlog(" - reverseVdpR9Bit4: %s", reverseVdpR9Bit4 ? "Yes" : "No");
                putlog(" - reverseVdpR9Bit5: %s", reverseVdpR9Bit5 ? "Yes" : "No");
#endif
                this->vdp.ctx.reverseVdpR9Bit4 = value & 0b01000000 ? 1 : 0;
                this->vdp.ctx.reverseVdpR9Bit5 = value & 0b10000000 ? 1 : 0;
                break;
            }
            case 0xFC: this->mmu.updateMemoryMapper(0, value); break;
            case 0xFD: this->mmu.updateMemoryMapper(1, value); break;
            case 0xFE: this->mmu.updateMemoryMapper(2, value); break;
            case 0xFF: this->mmu.updateMemoryMapper(3, value); break;
            default: printf("ignore an unknown out port $%02X <- $%02X\n", port, value);
        }
    }
    
    const void* quickSave(size_t* size) {
        this->quickSaveBufferSize = 0;
        this->writeSaveChunk("Z80", &this->cpu->reg, (int)sizeof(this->cpu->reg));
        this->writeSaveChunk("MMU", &this->mmu.ctx, (int)sizeof(this->mmu.ctx));
        this->writeSaveChunk("PAC", &this->mmu.pac, (int)sizeof(this->mmu.pac));
        this->writeSaveChunk("R:0", &this->mmu.ram, (int)sizeof(this->mmu.ram));
        if (this->mmu.sramEnabled) {
            this->writeSaveChunk("SRM", &this->mmu.sram, (int)sizeof(this->mmu.sram));
        }
        if (this->mmu.sccEnabled) {
            this->writeSaveChunk("SCC", &this->scc.ctx, (int)sizeof(this->scc.ctx));
        }
        this->writeSaveChunk("PSG", &this->psg.ctx, (int)sizeof(this->psg.ctx));
        this->writeSaveChunk("RTC", &this->clock.ctx, (int)sizeof(this->clock.ctx));
        this->writeSaveChunk("KNJ", &this->kanji.ctx, (int)sizeof(this->kanji.ctx));
        this->writeSaveChunk("VDP", &this->vdp.ctx, (int)sizeof(this->vdp.ctx));
        this->writeSaveChunk("FDC", &this->fdc.ctx, (int)sizeof(this->fdc.ctx));
        this->writeSaveChunk("JCT", &this->fdc.journalCount, (int)sizeof(this->fdc.journalCount));
        this->writeSaveChunk("JDT", &this->fdc.journal, (int)sizeof(this->fdc.journal[0]) * this->fdc.journalCount);
        this->writeSaveChunk("OPL", this->ym2413, (int)sizeof(OPLL));
        *size = LZ4_compress_default(this->quickSaveBuffer,
                                     this->quickSaveBufferCompressed,
                                     (int)this->quickSaveBufferSize,
                                     sizeof(this->quickSaveBufferCompressed));
        return this->quickSaveBufferCompressed;
    }

    void quickLoad(const void* buffer, size_t bufferSize) {
        this->reset();
        int size = LZ4_decompress_safe((const char*)buffer,
                                       this->quickSaveBuffer,
                                       (int)bufferSize,
                                       sizeof(this->quickSaveBuffer));
        const char* ptr = this->quickSaveBuffer;
        while (8 <= size) {
            char chunk[4];
            int chunkSize;
            strncpy(chunk, ptr, 4);
            if ('\0' != chunk[3]) break;
            ptr += 4;
            memcpy(&chunkSize, ptr, 4);
            ptr += 4;
            if (chunkSize < 0) break;
            if (0 == strcmp(chunk, "Z80")) {
                putlog("extract Z80 (%d bytes)", chunkSize);
                memcpy(&this->cpu->reg, ptr, chunkSize);
            } else if (0 == strcmp(chunk, "MMU")) {
                putlog("extract MMU (%d bytes)", chunkSize);
                memcpy(&this->mmu.ctx, ptr, chunkSize);
                this->mmu.bankSwitchover();
            } else if (0 == strcmp(chunk, "PAC")) {
                putlog("extract PAC (%d bytes)", chunkSize);
                memcpy(&this->mmu.pac, ptr, chunkSize);
            } else if (0 == strcmp(chunk, "R:0")) {
                putlog("extract R:0 (%d bytes)", chunkSize);
                memcpy(&this->mmu.ram, ptr, chunkSize);
            } else if (0 == strcmp(chunk, "SRM")) {
                putlog("extract SRM (%d bytes)", chunkSize);
                memcpy(&this->mmu.sram, ptr, chunkSize);
            } else if (0 == strcmp(chunk, "SCC")) {
                putlog("extract SCC (%d bytes)", chunkSize);
                memcpy(&this->scc.ctx, ptr, chunkSize);
            } else if (0 == strcmp(chunk, "PSG")) {
                putlog("extract PSG (%d bytes)", chunkSize);
                memcpy(&this->psg.ctx, ptr, chunkSize);
            } else if (0 == strcmp(chunk, "RTC")) {
                putlog("extract RTC (%d bytes)", chunkSize);
                memcpy(&this->clock.ctx, ptr, chunkSize);
            } else if (0 == strcmp(chunk, "KNJ")) {
                putlog("extract KNJ (%d bytes)", chunkSize);
                memcpy(&this->kanji.ctx, ptr, chunkSize);
            } else if (0 == strcmp(chunk, "VDP")) {
                putlog("extract VDP (%d bytes)", chunkSize);
                memcpy(&this->vdp.ctx, ptr, chunkSize);
                this->vdp.updateAllPalettes();
            } else if (0 == strcmp(chunk, "FDC")) {
                putlog("extract FDC (%d bytes)", chunkSize);
                memcpy(&this->fdc.ctx, ptr, chunkSize);
            } else if (0 == strcmp(chunk, "JCT")) {
                putlog("extract JCT (%d bytes)", chunkSize);
                memcpy(&this->fdc.journalCount, ptr, chunkSize);
            } else if (0 == strcmp(chunk, "JDT")) {
                putlog("extract JDT (%d bytes)", chunkSize);
                memset(&this->fdc.journal, 0, sizeof(this->fdc.journal));
                memcpy(&this->fdc.journal, ptr, chunkSize);
            } else if (0 == strcmp(chunk, "OPL")) {
                putlog("extract OPL (%d bytes)", chunkSize);
                memcpy(this->ym2413, ptr, chunkSize);
            }
            ptr += chunkSize;
            size -= chunkSize;
        }
    }

private:
    void writeSaveChunk(const char* name, const void* data, int size) {
        memcpy(&this->quickSaveBuffer[this->quickSaveBufferSize], name, 4);
        this->quickSaveBufferSize += 4;
        memcpy(&this->quickSaveBuffer[this->quickSaveBufferSize], &size, 4);
        this->quickSaveBufferSize += 4;
        memcpy(&this->quickSaveBuffer[this->quickSaveBufferSize], data, size);
        this->quickSaveBufferSize += size;
    }
};

#endif /* INCLUDE_MSX2_HPP */
