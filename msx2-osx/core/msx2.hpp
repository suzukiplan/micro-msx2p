#ifndef INCLUDE_MSX2_HPP
#define INCLUDE_MSX2_HPP
#include "z80.hpp"
#include "vdp.hpp"
#include "ay8910.hpp"
#include "mmu.hpp"

#define MSX2_COLOR_MODE_RGB555 0
#define MSX2_COLOR_MODE_RGB565 1

class MSX2 {
private:
    const int CPU_CLOCK = 3579545;
    const int VDP_CLOCK = 5370863;
    const int PSG_CLOCK = 44100;
    short soundBuffer[65536];
    unsigned short soundBufferCursor;
    
public:
    Z80* cpu;
    MMU mmu;
    VDP vdp;
    AY8910 psg;
    
    struct Context {
        unsigned char io[256];
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
            printf("Execute Interrupt %d from VDP\n", ie);
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
        /*
        this->cpu->addBreakPoint(0x015F, [](void* arg) {
            puts("Called $015F");
            ((MSX2*)arg)->cpu->setDebugMessage([](void* arg, const char* msg) {
                puts(msg);
                if (strstr(msg, "RET to $8") != NULL) {
                    ((MSX2*)arg)->cpu->resetDebugMessage();
                }
            });
        });
         */
        this->cpu->setConsumeClockCallbackFP([](void* arg, int cpuClocks) {
            ((MSX2*)arg)->consumeClock(cpuClocks);
        });
    }
    
    void reset() {
        memset(this->soundBuffer, 0, sizeof(this->soundBuffer));
        this->soundBufferCursor = 0;
        memset(&this->cpu->reg, 0, sizeof(this->cpu->reg));
        memset(&this->cpu->reg.pair, 0xFF, sizeof(this->cpu->reg.pair));
        memset(&this->cpu->reg.back, 0xFF, sizeof(this->cpu->reg.back));
        this->cpu->reg.SP = 0xF000;
        this->cpu->reg.IX = 0xFFFF;
        this->cpu->reg.IY = 0xFFFF;
        this->mmu.reset();
        this->vdp.reset();
        this->psg.reset(27);
    }
    
    void setup(int pri, int sec, int idx, bool isRAM, void* data, int size, const char* label = NULL) {
        mmu.setup(pri, sec, idx, isRAM, (unsigned char*)data, size, label);
    }
    
    void loadRom(void* data, int size) {
        mmu.setupCartridge(1, 0, 2, data, size);
        reset();
    }
    
    void tick() {
        cpu->execute(0x7FFFFFFF);
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
    }
    
    inline unsigned char inPort(unsigned char port) {
        switch (port) {
            case 0x98: return this->vdp.readPort0();
            case 0x99: return this->vdp.readPort1();
            case 0xA2: return this->psg.read();
            case 0xA8: return this->mmu.getPrimary();
            //case 0xA9: return 0xFF;
            case 0xAA: return 0x00;
            default: printf("ignore an unknown input port $%02X\n", port);
        }
        return this->ctx.io[port];
    }
    
    inline void outPort(unsigned char port, unsigned char value) {
        this->ctx.io[port] = value;
        switch (port) {
            case 0x98: this->vdp.writePort0(value); break;
            case 0x99: this->vdp.writePort1(value); break;
            case 0x9A: this->vdp.writePort2(value); break;
            case 0x9B:this->vdp.writePort3(value); break;
            case 0xA0: this->psg.latch(value); break;
            case 0xA1: this->psg.write(value); break;
            case 0xA8: this->mmu.updatePrimary(value); break;
            //case 0xAA: break;
            //case 0xAB: break;
            case 0xFC: this->mmu.updateSegment(3, value); break;
            case 0xFD: this->mmu.updateSegment(2, value); break;
            case 0xFE: this->mmu.updateSegment(1, value); break;
            case 0xFF: this->mmu.updateSegment(0, value); break;
            default: printf("ignore an unknown out port $%02X <- $%02X\n", port, value);
        }
    }
};

#endif /* INCLUDE_MSX2_HPP */
