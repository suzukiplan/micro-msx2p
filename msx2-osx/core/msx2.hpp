#ifndef INCLUDE_MSX2_HPP
#define INCLUDE_MSX2_HPP
#include "z80.hpp"
#include "vdp.hpp"
#include "ay8910.hpp"
#include "mmu.hpp"
#include "msx2def.h"
#include "rtc.hpp"

class MSX2 {
private:
    const int CPU_CLOCK = 3579545;
    const int VDP_CLOCK = 5370863;
    const int PSG_CLOCK = 44100;
    short soundBuffer[65536];
    unsigned short soundBufferCursor;
   
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
    MMU mmu;
    VDP vdp;
    AY8910 psg;
    RTC rtc;
    
    struct Context {
        unsigned char io[256];
        unsigned char key;
    } ctx;

    MSX2(int colorMode) {
        this->cpu = new Z80([](void* arg, unsigned short addr) {
            return ((MSX2*)arg)->mmu.read(addr);
        }, [](void* arg, unsigned short addr, unsigned char value) {
            ((MSX2*)arg)->mmu.write(addr, value);
        }, [](void* arg, unsigned short port) {
            return ((MSX2*)arg)->inPort((unsigned char) port);
        }, [](void* arg, unsigned short port, unsigned char value) {
            return ((MSX2*)arg)->outPort((unsigned char) port, value);
        }, this, false);
        this->vdp.initialize(colorMode, this, [](void* arg, int ie) {
            ((MSX2*)arg)->cpu->resetDebugMessage();
            ((MSX2*)arg)->cpu->generateIRQ(0x07);
        }, [](void* arg) {
            ((MSX2*)arg)->cpu->requestBreak();
        });
        /*
        this->vdp.setRegisterUpdateListener(this, [](void* arg, int rn, unsigned char value) {
            auto this_ = (MSX2*)arg;
            printf("Update VDP register #%d = $%02X (PC:$%04X)\n", rn, value, this_->cpu->reg.PC);
        });
         */

        // RDSLT
        //this->cpu->addBreakPoint(0x23D2, [](void* arg) {
        /*
        this->cpu->addBreakPoint(0x000C, [](void* arg) {
            unsigned char a = ((MSX2*)arg)->cpu->reg.pair.A;
            unsigned short hl = ((MSX2*)arg)->cpu->reg.pair.H;
            hl <<= 8;
            hl |= ((MSX2*)arg)->cpu->reg.pair.L;
            printf("RDSLT: isExpand=%s, pri=%d, sec=%d, HL=$%04X\n", a & 0x80 ? "YES" : "NO", a & 0x03, (a & 0x0C) >> 2, hl);
        });
         */

        // WRSLT
        //this->cpu->addBreakPoint(0x2413, [](void* arg) {
        /*
        this->cpu->addBreakPoint(0x0014, [](void* arg) {
            unsigned char a = ((MSX2*)arg)->cpu->reg.pair.A;
            unsigned short hl = ((MSX2*)arg)->cpu->reg.pair.H;
            hl <<= 8;
            hl |= ((MSX2*)arg)->cpu->reg.pair.L;
            unsigned char e = ((MSX2*)arg)->cpu->reg.pair.E;
            printf("WRSLT: isExpand=%s, pri=%d, sec=%d, HL=$%04X E=$%02X\n", a & 0x80 ? "YES" : "NO", a & 0x03, (a & 0x0C) >> 2, hl, e);
        });
         */

        // CALL命令をページ3にRAMを割り当てていない状態で実行した時に落とす
        this->cpu->addBreakOperand(0xCD, [](void* arg, unsigned char* op, int size) {
            int pri3 = ((MSX2*)arg)->mmu.ctx.primary[3];
            int sec3 = ((MSX2*)arg)->mmu.ctx.secondary[3];
            auto data = &((MSX2*)arg)->mmu.slots[pri3][sec3].data[7];
            if (!data->isRAM) {
                puts("invalid call");
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
        initKeyCode('\n', 7, 7);
        initKeyCode(' ', 0, 8);
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
        this->rtc.reset();
    }
    
    void setupSecondaryExist(bool page0, bool page1, bool page2, bool page3) {
        this->mmu.setupSecondaryExist(page0, page1, page2, page3);
    }

    void setup(int pri, int sec, int idx, bool isRAM, void* data, int size, const char* label = NULL) {
        this->mmu.setup(pri, sec, idx, isRAM, (unsigned char*)data, size, label);
    }
    
    void loadRom(void* data, int size, int romType) {
        this->mmu.setupCartridge(1, 0, 2, data, size, romType);
        this->reset();
    }
    
    void tick(unsigned char pad1, unsigned char pad2, unsigned char key) {
        this->psg.setPads(pad1, pad2);
        this->ctx.key = toupper(key);
        this->cpu->execute(0x7FFFFFFF);
    }

    inline void consumeClock(int cpuClocks) {
        // Asynchronous with PSG
        this->psg.ctx.bobo += cpuClocks * this->PSG_CLOCK;
        while (0 < this->psg.ctx.bobo) {
            this->psg.ctx.bobo -= this->CPU_CLOCK;
            this->psg.tick(&this->soundBuffer[this->soundBufferCursor], &this->soundBuffer[this->soundBufferCursor + 1], 81);
            this->soundBufferCursor += 2;
        }
        // Asynchronous with VDP
        this->vdp.ctx.bobo += cpuClocks * VDP_CLOCK;
        while (0 < this->vdp.ctx.bobo) {
            this->vdp.ctx.bobo -= CPU_CLOCK;
            this->vdp.tick();
        }
        // Asynchronous with RTC
        this->rtc.ctx.bobo += cpuClocks;
        while (CPU_CLOCK <= this->rtc.ctx.bobo) {
            this->rtc.ctx.bobo -= CPU_CLOCK;
            this->rtc.tick();
        }
    }
    
    inline unsigned char inPort(unsigned char port) {
        switch (port) {
            case 0x88: return this->vdp.readPort0();
            case 0x98: return this->vdp.readPort0();
            case 0x89: return this->vdp.readPort1();
            case 0x99: return this->vdp.readPort1();
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
            case 0xAA: break;
            case 0xB5: return this->rtc.inPort();
            case 0xB8: return 0x00; // light pen
            case 0xB9: return 0x00; // light pen
            case 0xBA: return 0x00; // light pen
            case 0xBB: return 0x00; // light pen
            default: printf("ignore an unknown input port $%02X\n", port);
        }
        return this->ctx.io[port];
    }
    
    inline void outPort(unsigned char port, unsigned char value) {
        this->ctx.io[port] = value;
        switch (port) {
            case 0x88: this->vdp.writePort0(value); break;
            case 0x98: this->vdp.writePort0(value); break;
            case 0x89: this->vdp.writePort1(value); break;
            case 0x99: this->vdp.writePort1(value); break;
            case 0x8A: this->vdp.writePort2(value); break;
            case 0x9A: this->vdp.writePort2(value); break;
            case 0x8B: this->vdp.writePort3(value); break;
            case 0x9B: this->vdp.writePort3(value); break;
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
            case 0xB4: this->rtc.outPort0(value); break;
            case 0xB5: this->rtc.outPort1(value); break;
            case 0xB8: break; // light pen
            case 0xB9: break; // light pen
            case 0xBA: break; // light pen
            case 0xBB: break; // light pen
            case 0xFC: this->mmu.updateSegment(3, value); break;
            case 0xFD: this->mmu.updateSegment(2, value); break;
            case 0xFE: this->mmu.updateSegment(1, value); break;
            case 0xFF: this->mmu.updateSegment(0, value); break;
            default: printf("ignore an unknown out port $%02X <- $%02X\n", port, value);
        }
    }
};

#endif /* INCLUDE_MSX2_HPP */
