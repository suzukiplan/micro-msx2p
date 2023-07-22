/**
 * micro MSX2+ - machine
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
#ifndef INCLUDE_MSX2_HPP
#define INCLUDE_MSX2_HPP
#include "ay8910.hpp"
#ifndef MSX2_REMOVE_OPLL
#include "emu2413.h"
#endif
#include "lz4.h"
#include "msx2clock.hpp"
#include "msx2def.h"
#include "msx2kanji.hpp"
#include "msx2mmu.hpp"
#include "scc.hpp"
#include "tc8566af.hpp"
#include "v9958.hpp"
#include "z80.hpp"

class MSX2
{
  private:
    const int CPU_CLOCK = 3584160;
    const int VDP_CLOCK = 21504960;
    const int PSG_CLOCK = 44100;
    class InternalBuffer
    {
      public:
        short soundBuffer[16384];
        unsigned short soundBufferCursor;
        char* quickSaveBuffer;
        char* quickSaveBufferCompressed;
        size_t quickSaveBufferPtr;
        size_t quickSaveBufferHeapSize;

        InternalBuffer()
        {
            memset(this->soundBuffer, 0, sizeof(this->soundBuffer));
            this->soundBufferCursor = 0;
            this->quickSaveBuffer = nullptr;
            this->quickSaveBufferCompressed = nullptr;
            this->quickSaveBufferPtr = 0;
            this->quickSaveBufferHeapSize = 0;
        }

        ~InternalBuffer()
        {
            this->safeReleaseQuickSaveBuffer();
        }

        void safeReleaseQuickSaveBuffer()
        {
            if (this->quickSaveBuffer) {
                free(this->quickSaveBuffer);
                this->quickSaveBuffer = nullptr;
            }
            if (this->quickSaveBufferCompressed) {
                free(this->quickSaveBufferCompressed);
                this->quickSaveBufferCompressed = nullptr;
            }
            this->quickSaveBufferHeapSize = 0;
        }

        bool allocateQuickSaveBuffer(size_t size)
        {
            if (size == this->quickSaveBufferHeapSize) {
                return true;
            }
            this->safeReleaseQuickSaveBuffer();
            this->quickSaveBuffer = (char*)malloc(size);
            if (!this->quickSaveBuffer) {
                return false;
            }
            this->quickSaveBufferCompressed = (char*)malloc(size);
            if (!this->quickSaveBufferCompressed) {
                this->safeReleaseQuickSaveBuffer();
                return false;
            }
            this->quickSaveBufferHeapSize = size;
            return true;
        }
    };

    InternalBuffer* ib;
    bool debug;

    struct KeyCode {
        int num;
        bool exist;
        int x[2];
        int y[2];
        bool shift;
    } keyCodes[0x100];

    struct KeyAssign {
        KeyCode* s1;
        KeyCode* s2;
    } keyAssign[2];

    void initKeyCode(unsigned char code, int x, int y, bool shift = false)
    {
        keyCodes[code].exist = true;
        keyCodes[code].x[0] = x;
        keyCodes[code].y[0] = y;
        keyCodes[code].shift = shift;
        keyCodes[code].num = 1;
    }

    void initKeyCode2(unsigned char code, int x1, int y1, int x2, int y2, bool shift = false)
    {
        keyCodes[code].exist = true;
        keyCodes[code].x[0] = x1;
        keyCodes[code].y[0] = y1;
        keyCodes[code].x[1] = x2;
        keyCodes[code].y[1] = y2;
        keyCodes[code].shift = shift;
        keyCodes[code].num = 2;
    }

  public:
    Z80* cpu;
    MSX2MMU* mmu;
    V9958* vdp;
    AY8910* psg;
    MSX2Clock* clock;
    MSX2Kanji* kanji;
    SCC* scc;
    TC8566AF* fdc;
#ifndef MSX2_REMOVE_OPLL
    OPLL* ym2413;
#endif

    struct Context {
        unsigned char io[256];
        unsigned char key;
        unsigned char readKey;
        unsigned char regC;
        unsigned char selectedKeyRow;
    } ctx;
    unsigned char* keyCodeMap;

    ~MSX2()
    {
        if (this->fdc) delete fdc;
        if (this->scc) delete scc;
#ifndef MSX2_REMOVE_OPLL
        if (this->ym2413) OPLL_delete(this->ym2413);
#endif
        delete this->cpu;
        delete this->vdp;
        delete this->mmu;
        delete this->psg;
        delete this->clock;
        delete this->kanji;
        delete this->ib;
    }

    MSX2(int colorMode, bool ym2413Enabled = false)
    {
#ifdef DEBUG
        this->debug = true;
#else
        this->debug = false;
#endif
        memset(&this->keyAssign, 0, sizeof(this->keyAssign));
        this->ib = new InternalBuffer();
        this->mmu = new MSX2MMU();
        this->vdp = new V9958();
        this->psg = new AY8910();
        this->clock = new MSX2Clock();
        this->kanji = new MSX2Kanji();
        this->cpu = new Z80([](void* arg, unsigned short addr) { return ((MSX2*)arg)->mmu->read(addr); }, [](void* arg, unsigned short addr, unsigned char value) { ((MSX2*)arg)->mmu->write(addr, value); }, [](void* arg, unsigned short port) { return ((MSX2*)arg)->inPort((unsigned char)port); }, [](void* arg, unsigned short port, unsigned char value) { ((MSX2*)arg)->outPort((unsigned char)port, value); }, this, false);
        this->cpu->wtc.fetch = 1;
        this->cpu->wtc.fetchM = 1;
        this->scc = nullptr;
#ifndef MSX2_REMOVE_OPLL
        if (ym2413Enabled) {
            this->putlog("create YM2413 instance");
            this->ym2413 = OPLL_new(CPU_CLOCK, 44100);
        } else {
            this->ym2413 = nullptr;
        }
#endif
        this->vdp->initialize(
            colorMode, this, [](void* arg, int ie) {
            //((MSX2*)arg)->putlog("Detect IE%d (vf:%d, hf:%d)", ie, ((MSX2*)arg)->vdp->ctx.stat[0] & 0x80 ? 1 : 0,((MSX2*)arg)->vdp->ctx.stat[1] & 0x01);
            //((MSX2*)arg)->cpu->resetDebugMessage();
            ((MSX2*)arg)->cpu->generateIRQ(0x07); }, [](void* arg) { ((MSX2*)arg)->cpu->cancelIRQ(); }, [](void* arg) { ((MSX2*)arg)->cpu->requestBreak(); });
        this->mmu->setupCallbacks(
            this, [](void* arg, unsigned short addr) { return ((MSX2*)arg)->scc->read(addr); }, [](void* arg, unsigned short addr, unsigned char value) { ((MSX2*)arg)->scc->write(addr, value); }, [](void* arg, unsigned short addr) {
            switch (addr) {
                case 0x3FFA: return ((MSX2*)arg)->fdc->read(4);
                case 0x3FFB: return ((MSX2*)arg)->fdc->read(5);
                default: return (unsigned char)0xFF;
            } }, [](void* arg, unsigned short addr, unsigned char value) {
            switch (addr) {
                case 0x3FF8: ((MSX2*)arg)->fdc->write(2, value); break;
                case 0x3FF9: ((MSX2*)arg)->fdc->write(3, value); break;
                case 0x3FFA: ((MSX2*)arg)->fdc->write(4, value); break;
                case 0x3FFB: ((MSX2*)arg)->fdc->write(5, value); break;
            } }, [](void* arg, unsigned short addr, unsigned char value) {
            switch (addr) {
#ifndef MSX2_REMOVE_OPLL
                case 0x3FF4: OPLL_writeIO(((MSX2*)arg)->ym2413, 0, value); break;
                case 0x3FF5: OPLL_writeIO(((MSX2*)arg)->ym2413, 1, value); break;
#endif
            } });
        this->fdc = nullptr;
#if 0
        // CALL命令をページ3にRAMを割り当てていない状態で実行した時に落とす
        this->cpu->addBreakOperand(0xCD, [](void* arg, unsigned char* op, int size) {
            int pri3 = ((MSX2*)arg)->mmu->ctx.pri[3];
            int sec3 = ((MSX2*)arg)->mmu->ctx.sec[3];
            auto data = &((MSX2*)arg)->mmu->slots[pri3][sec3].data[7];
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
#endif
        this->cpu->setConsumeClockCallbackFP([](void* arg, int cpuClocks) {
            ((MSX2*)arg)->consumeClock(cpuClocks);
        });
        memset(&keyCodes, 0, sizeof(keyCodes));
        initKeyCode('0', 0, 0);
        initKeyCode('1', 1, 0);
        initKeyCode('!', 1, 0, true);
        initKeyCode('2', 2, 0);
        initKeyCode('\"', 2, 0, true);
        initKeyCode('3', 3, 0);
        initKeyCode('#', 3, 0, true);
        initKeyCode('4', 4, 0);
        initKeyCode('$', 4, 0, true);
        initKeyCode('5', 5, 0);
        initKeyCode('%', 5, 0, true);
        initKeyCode('6', 6, 0);
        initKeyCode('&', 6, 0, true);
        initKeyCode('7', 7, 0);
        initKeyCode('\'', 7, 0, true);
        initKeyCode('8', 0, 1);
        initKeyCode('(', 0, 1, true);
        initKeyCode('9', 1, 1);
        initKeyCode(')', 1, 1, true);
        initKeyCode('-', 2, 1);
        initKeyCode('=', 2, 1, true);
        initKeyCode('^', 3, 1);
        initKeyCode('~', 3, 1, true);
        initKeyCode('\\', 4, 1);
        initKeyCode('|', 4, 1, true);
        initKeyCode('@', 5, 1);
        initKeyCode('`', 5, 1, true);
        initKeyCode('[', 6, 1);
        initKeyCode('{', 6, 1, true);
        initKeyCode(';', 7, 1);
        initKeyCode('+', 7, 1, true);
        initKeyCode(':', 0, 2);
        initKeyCode('*', 0, 2, true);
        initKeyCode(']', 1, 2);
        initKeyCode('}', 1, 2, true);
        initKeyCode(',', 2, 2);
        initKeyCode('<', 2, 2, true);
        initKeyCode('.', 3, 2);
        initKeyCode('>', 3, 2, true);
        initKeyCode('/', 4, 2);
        initKeyCode('?', 4, 2, true);
        initKeyCode('_', 5, 2);
        initKeyCode('A', 6, 2, true);
        initKeyCode('B', 7, 2, true);
        initKeyCode('C', 0, 3, true);
        initKeyCode('D', 1, 3, true);
        initKeyCode('E', 2, 3, true);
        initKeyCode('F', 3, 3, true);
        initKeyCode('G', 4, 3, true);
        initKeyCode('H', 5, 3, true);
        initKeyCode('I', 6, 3, true);
        initKeyCode('J', 7, 3, true);
        initKeyCode('K', 0, 4, true);
        initKeyCode('L', 1, 4, true);
        initKeyCode('M', 2, 4, true);
        initKeyCode('N', 3, 4, true);
        initKeyCode('O', 4, 4, true);
        initKeyCode('P', 5, 4, true);
        initKeyCode('Q', 6, 4, true);
        initKeyCode('R', 7, 4, true);
        initKeyCode('S', 0, 5, true);
        initKeyCode('T', 1, 5, true);
        initKeyCode('U', 2, 5, true);
        initKeyCode('V', 3, 5, true);
        initKeyCode('W', 4, 5, true);
        initKeyCode('X', 5, 5, true);
        initKeyCode('Y', 6, 5, true);
        initKeyCode('Z', 7, 5, true);
        initKeyCode('a', 6, 2, false);
        initKeyCode('b', 7, 2, false);
        initKeyCode('c', 0, 3, false);
        initKeyCode('d', 1, 3, false);
        initKeyCode('e', 2, 3, false);
        initKeyCode('f', 3, 3, false);
        initKeyCode('g', 4, 3, false);
        initKeyCode('h', 5, 3, false);
        initKeyCode('i', 6, 3, false);
        initKeyCode('j', 7, 3, false);
        initKeyCode('k', 0, 4, false);
        initKeyCode('l', 1, 4, false);
        initKeyCode('m', 2, 4, false);
        initKeyCode('n', 3, 4, false);
        initKeyCode('o', 4, 4, false);
        initKeyCode('p', 5, 4, false);
        initKeyCode('q', 6, 4, false);
        initKeyCode('r', 7, 4, false);
        initKeyCode('s', 0, 5, false);
        initKeyCode('t', 1, 5, false);
        initKeyCode('u', 2, 5, false);
        initKeyCode('v', 3, 5, false);
        initKeyCode('w', 4, 5, false);
        initKeyCode('x', 5, 5, false);
        initKeyCode('y', 6, 5, false);
        initKeyCode('z', 7, 5, false);
        initKeyCode('\r', 7, 7);
        initKeyCode('\n', 7, 7);
        initKeyCode('\t', 3, 7);
        initKeyCode(' ', 0, 8);
        initKeyCode2(0x18, 1, 6, 4, 7); // CTRL + STOP
        initKeyCode(0x1B, 2, 7);        // ESC
        initKeyCode(0x7F, 5, 7);        // DEL as Back Space
        initKeyCode(0xC0, 5, 8);        // up cursor
        initKeyCode(0xC1, 6, 8);        // down cursor
        initKeyCode(0xC2, 4, 8);        // left cursor
        initKeyCode(0xC3, 7, 8);        // right cursor
        initKeyCode(0xF1, 5, 6);        // f1
        initKeyCode(0xF2, 6, 6);        // f2
        initKeyCode(0xF3, 7, 6);        // f3
        initKeyCode(0xF4, 0, 7);        // f4
        initKeyCode(0xF5, 1, 7);        // f5
        initKeyCode(0xF6, 5, 6, true);  // f6
        initKeyCode(0xF7, 6, 6, true);  // f7
        initKeyCode(0xF8, 7, 6, true);  // f8
        initKeyCode(0xF9, 0, 7, true);  // f9
        initKeyCode(0xFA, 1, 7, true);  // f10
        this->reset();
    }

    void setupKeyAssign(int player, int code, unsigned char assign)
    {
        if (player != 0 && player != 1) return;
        if (!this->keyCodes[assign].exist) return;
        if (MSX2_JOY_S1 == code) {
            this->keyAssign[player].s1 = &this->keyCodes[assign];
        } else if (MSX2_JOY_S2 == code) {
            this->keyAssign[player].s2 = &this->keyCodes[assign];
        }
    }

    void reset()
    {
        memset(this->ib->soundBuffer, 0, sizeof(this->ib->soundBuffer));
        this->ib->soundBufferCursor = 0;
        memset(&this->cpu->reg, 0, sizeof(this->cpu->reg));
        memset(&this->cpu->reg.pair, 0xFF, sizeof(this->cpu->reg.pair));
        memset(&this->cpu->reg.back, 0xFF, sizeof(this->cpu->reg.back));
        memset(&this->ctx, 0, sizeof(this->ctx));
        this->ctx.regC = 0x50;
        this->keyCodeMap = nullptr;
        this->cpu->reg.SP = 0xF000;
        this->cpu->reg.IX = 0xFFFF;
        this->cpu->reg.IY = 0xFFFF;
        this->mmu->reset();
        this->vdp->reset();
        this->psg->reset(27);
        this->clock->reset();
        this->kanji->reset();
        if (this->scc) this->scc->reset();
        if (this->fdc) this->fdc->reset();
#ifndef MSX2_REMOVE_OPLL
        if (this->ym2413) OPLL_reset(this->ym2413);
#endif
    }

    void putlog(const char* fmt, ...)
    {
        if (!this->debug) return;
        static int seqno = 0;
        char buf[256];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        char addr[256];
        auto db = this->mmu->getDataBlock(this->cpu->reg.PC);
        if (db->isCartridge) {
            int romAddr = (int)(db->ptr - this->mmu->cartridge.ptr);
            snprintf(addr, sizeof(addr), "[PC=%04X:%X+%04X,SP=%04X]", this->cpu->reg.PC, this->cpu->reg.SP, romAddr, this->cpu->reg.PC & 0x1FFF);
        } else {
            snprintf(addr, sizeof(addr), "[PC=%04X,SP=%04X]", this->cpu->reg.PC, this->cpu->reg.SP);
        }
        printf("%7d %s %d-%d:%d-%d:%d-%d:%d-%d F:%02d,V:%03d %s\n", ++seqno, addr,
               this->mmu->ctx.pri[0], this->mmu->ctx.sec[0],
               this->mmu->ctx.pri[1], this->mmu->ctx.sec[1],
               this->mmu->ctx.pri[2], this->mmu->ctx.sec[2],
               this->mmu->ctx.pri[3], this->mmu->ctx.sec[3],
               this->vdp->ctx.counter % 100,
               this->vdp->ctx.countV, buf);
    }

    void loadFont(const void* font, size_t fontSize)
    {
        this->kanji->loadFont(font, fontSize);
    }

    void setupSecondaryExist(bool page0, bool page1, bool page2, bool page3)
    {
        this->mmu->setupSecondaryExist(page0, page1, page2, page3);
    }

    void setupRAM(int pri, int sec)
    {
        this->mmu->setupRAM(pri, sec);
    }

    void setup(int pri, int sec, int idx, void* data, int size, const char* label = NULL)
    {
        if (0 == strcmp(label, "FM")) {
#ifndef MSX2_REMOVE_OPLL
            if (!this->ym2413) {
                this->putlog("create YM2413 instance");
                this->ym2413 = OPLL_new(CPU_CLOCK, 44100);
            }
#endif
        } else if (0 == strcmp(label, "DISK")) {
            if (!this->fdc) {
                this->putlog("create FDC instance");
                this->fdc = new TC8566AF();
                this->fdc->setDiskReadListener(this, [](void* arg, int driveId, int sector) {
                    //((MSX2*)arg)->putlog("DiskRead: drive=%d, sector=%d", driveId, sector);
                });
                this->fdc->setDiskWriteListener(this, [](void* arg, int driveId, int sector) {
                    //((MSX2*)arg)->putlog("DiskWrite: drive=%d, sector=%d", driveId, sector);
                });
            }
        }
        this->mmu->setup(pri, sec, idx, (unsigned char*)data, size, label);
    }

    void loadRom(void* data, int size, int romType)
    {
        this->mmu->setupCartridge(1, 0, 2, data, size, romType);
        if (romType == MSX2_ROM_TYPE_KONAMI_SCC) {
            if (!this->scc) {
                this->putlog("create SCC instance");
                this->scc = new SCC();
                this->scc->enabled = true;
            }
        } else if (this->scc) {
            this->putlog("remove SCC instance");
            delete this->scc;
            this->scc = nullptr;
        }
        this->reset();
    }

    void ejectRom()
    {
        this->mmu->clearCartridge();
        if (this->scc) {
            this->putlog("remove SCC instance");
            delete this->scc;
            this->scc = nullptr;
        }
        this->reset();
    }

    void insertDisk(int driveId, const void* data, size_t size, bool readOnly)
    {
        if (this->fdc) {
            this->fdc->insertDisk(driveId, data, size, readOnly);
        } else {
            this->putlog("Cannot insert disk (FDC is not available)");
        }
    }

    void ejectDisk(int driveId)
    {
        if (this->fdc) {
            this->fdc->ejectDisk(driveId);
        } else {
            this->putlog("Cannot insert disk (FDC is not available)");
        }
    }

    void tick(unsigned char pad1, unsigned char pad2, unsigned char key)
    {
        this->psg->setPads(pad1, pad2);
        this->ctx.key = key;
        this->keyCodeMap = nullptr;
        this->cpu->execute(0x7FFFFFFF);
    }

    void tickWithKeyCodeMap(unsigned char pad1, unsigned char pad2, unsigned char* keyCodeMap)
    {
        this->psg->setPads(pad1, pad2);
        this->ctx.key = 0;
        this->keyCodeMap = keyCodeMap;
        this->cpu->execute(0x7FFFFFFF);
    }

    size_t getMaxSoundSize()
    {
        return sizeof(this->ib->soundBuffer);
    }

    size_t getCurrentSoundSize()
    {
        return this->ib->soundBufferCursor * 2;
    }

    void* getSound(size_t* soundSize)
    {
        *soundSize = this->ib->soundBufferCursor * 2;
        this->ib->soundBufferCursor = 0;
        return this->ib->soundBuffer;
    }

    inline unsigned short* getDisplay() { return this->vdp->display; }
    inline int getDisplayWidth() { return vdp->displayWidth(); }
    inline int getDisplayHeight() { return 240; }

    inline void consumeClock(int cpuClocks)
    {
        // Asynchronous with PSG/SCC
        this->psg->ctx.bobo += cpuClocks * this->PSG_CLOCK;
        while (0 < this->psg->ctx.bobo) {
            this->psg->ctx.bobo -= this->CPU_CLOCK;
            this->psg->tick(&this->ib->soundBuffer[this->ib->soundBufferCursor], &this->ib->soundBuffer[this->ib->soundBufferCursor + 1], 81);
            if (this->scc) {
                this->scc->tick(&this->ib->soundBuffer[this->ib->soundBufferCursor], &this->ib->soundBuffer[this->ib->soundBufferCursor + 1], 81);
            }
#ifndef MSX2_REMOVE_OPLL
            if (this->ym2413) {
                auto opllWav = OPLL_calc(this->ym2413);
                int l = this->ib->soundBuffer[this->ib->soundBufferCursor];
                int r = this->ib->soundBuffer[this->ib->soundBufferCursor + 1];
                l += opllWav;
                r += opllWav;
                if (32767 < l)
                    l = 32767;
                else if (l < -32768)
                    l = -32768;
                if (32767 < r)
                    r = 32767;
                else if (r < -32768)
                    r = -32768;
                this->ib->soundBuffer[this->ib->soundBufferCursor] = (short)l;
                this->ib->soundBuffer[this->ib->soundBufferCursor + 1] = (short)r;
            }
#endif
            this->ib->soundBufferCursor += 2;
            this->ib->soundBufferCursor &= sizeof(this->ib->soundBuffer) - 1;
        }
        // Asynchronous with VDP
        this->vdp->ctx.bobo += cpuClocks * VDP_CLOCK;
        while (0 < this->vdp->ctx.bobo) {
            this->vdp->ctx.bobo -= CPU_CLOCK;
            this->vdp->tick();
        }
        // Asynchronous with Clock IC
        this->clock->ctx.bobo += cpuClocks;
        while (CPU_CLOCK <= this->clock->ctx.bobo) {
            this->clock->ctx.bobo -= CPU_CLOCK;
            this->clock->tick();
        }
    }

    inline unsigned char inPort(unsigned char port)
    {
        switch (port) {
            case 0x81: return 0xFF; // 8251 status command
            case 0x88: return this->vdp->inPort98();
            case 0x89: return this->vdp->inPort99();
            case 0x90: return 0x00; // printer
            case 0x98: return this->vdp->inPort98();
            case 0x99: return this->vdp->inPort99();
            case 0xA2: {
                unsigned char result = this->psg->read();
                if (14 == this->psg->ctx.latch || 15 == this->psg->ctx.latch) {
                    result |= 0b11000000; // unpush S1/S2
                }
                return result;
            }
            case 0xA8: return this->mmu->getPrimary();
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
                if (this->keyCodeMap) {
                    result |= this->keyCodeMap[this->ctx.selectedKeyRow];
                } else {
                    if (this->ctx.key && this->keyCodes[this->ctx.key].exist) {
                        if (this->keyCodes[this->ctx.key].shift) {
                            if (this->ctx.selectedKeyRow == 6) {
                                result |= bit[0];
                            }
                        }
                        for (int i = 0; i < this->keyCodes[this->ctx.key].num; i++) {
                            if (this->ctx.selectedKeyRow == this->keyCodes[this->ctx.key].y[i]) {
                                this->ctx.readKey++;
                                result |= bit[this->keyCodes[this->ctx.key].x[i]];
                            }
                        }
                    }
                }
                if (this->keyAssign[0].s1 && 0 == (this->psg->getPad1() & MSX2_JOY_S1)) {
                    if (this->ctx.selectedKeyRow == this->keyAssign[0].s1->y[0]) {
                        result |= bit[this->keyAssign[0].s1->x[0]];
                    }
                }
                if (this->keyAssign[0].s2 && 0 == (this->psg->getPad1() & MSX2_JOY_S2)) {
                    if (this->ctx.selectedKeyRow == this->keyAssign[0].s2->y[0]) {
                        result |= bit[this->keyAssign[0].s2->x[0]];
                    }
                }
                if (this->keyAssign[1].s1 && 0 == (this->psg->getPad2() & MSX2_JOY_S1)) {
                    if (this->ctx.selectedKeyRow == this->keyAssign[1].s1->y[0]) {
                        result |= bit[this->keyAssign[1].s1->x[0]];
                    }
                }
                if (this->keyAssign[1].s2 && 0 == (this->psg->getPad2() & MSX2_JOY_S2)) {
                    if (this->ctx.selectedKeyRow == this->keyAssign[1].s2->y[0]) {
                        result |= bit[this->keyAssign[1].s2->x[0]];
                    }
                }
                return ~result;
            }
            case 0xAA: return this->ctx.regC;
            case 0xB5: return this->clock->inPortB5();
            case 0xB8: return 0x00;                    // light pen
            case 0xB9: return 0x00;                    // light pen
            case 0xBA: return 0x00;                    // light pen
            case 0xBB: return 0xFF;                    // light pen
            case 0xC0: return 0xFF;                    // MSX-Audio (Y8950?)
            case 0xC8: return 0xFF;                    // MSX interface
            case 0xC9: return 0x00;                    // MSX interface
            case 0xCA: return 0x00;                    // MSX interface
            case 0xCB: return 0x00;                    // MSX interface
            case 0xCC: return 0x00;                    // MSX interface
            case 0xCD: return 0x00;                    // MSX interface
            case 0xCE: return 0x00;                    // MSX interface
            case 0xCF: return 0x00;                    // MSX interface
            case 0xD9: return this->kanji->inPortD9(); // kanji
            case 0xDB: return this->kanji->inPortDB(); // kanji
            case 0xF4: return this->vdp->inPortF4();
            case 0xF7: return 0xFF; // AV control
            default: this->putlog("ignore an unknown input port $%02X\n", port);
        }
        return this->ctx.io[port];
    }

    inline void outPort(unsigned char port, unsigned char value)
    {
        this->ctx.io[port] = value;
        switch (port) {
#ifdef MSX2_REMOVE_OPLL
            case 0x7C: break;
            case 0x7D: break;
#else
            case 0x7C:
                if (this->ym2413) OPLL_writeIO(this->ym2413, 0, value);
                break;
            case 0x7D:
                if (this->ym2413) OPLL_writeIO(this->ym2413, 1, value);
                break;
#endif
            case 0x81: break; // 8251 status command
            case 0x88: this->vdp->outPort98(value); break;
            case 0x89: this->vdp->outPort99(value); break;
            case 0x8A: this->vdp->outPort9A(value); break;
            case 0x8B: this->vdp->outPort9B(value); break;
            case 0x90: break; // printer
            case 0x91: break; // printer
            case 0x98: this->vdp->outPort98(value); break;
            case 0x99: this->vdp->outPort99(value); break;
            case 0x9A: this->vdp->outPort9A(value); break;
            case 0x9B: this->vdp->outPort9B(value); break;
            case 0xA0: this->psg->latch(value); break;
            case 0xA1: this->psg->write(value); break;
            case 0xA8: this->mmu->updatePrimary(value); break;
            case 0xAA: {
                unsigned char mod = this->ctx.regC ^ value;
                if (mod) {
                    this->ctx.regC = value;
                    if (mod & 0x0F) {
                        this->ctx.selectedKeyRow = this->ctx.regC & 0x0F;
                    }
                    if (mod & 0xA0) {
                        // TODO: update pluse signal
                    }
                    if (mod & 0x40) {
                        // TODO: update caps led
                    }
                }
                break;
            }
            case 0xAB: {
                if (0 == (value & 0x80)) {
                    unsigned char bit = (value & 0x0E) >> 1;
                    if (value & 0x01) {
                        this->ctx.regC |= 1 << bit;
                    } else {
                        this->ctx.regC &= ~(1 << bit);
                    }
                    if (bit <= 3) {
                        this->ctx.selectedKeyRow = this->ctx.regC & 0x0F;
                    } else if (5 == bit || 7 == bit) {
                        // TODO: update pulse signal
                    } else if (6 == bit) {
                        // TODO: update caps led
                    }
                }
                break;
            }
            case 0xB4: this->clock->outPortB4(value); break;
            case 0xB5: this->clock->outPortB5(value); break;
            case 0xB8: break; // light pen
            case 0xB9: break; // light pen
            case 0xBA: break; // light pen
            case 0xBB: break; // light pen
            case 0xD8: this->kanji->outPortD8(value); break;
            case 0xD9: this->kanji->outPortD9(value); break;
            case 0xDA: this->kanji->outPortDA(value); break;
            case 0xDB: this->kanji->outPortDB(value); break;
            case 0xF3: break;
            case 0xF4: this->vdp->outPortF4(value); break;
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
                this->vdp->ctx.reverseVdpR9Bit4 = value & 0b01000000 ? 1 : 0;
                this->vdp->ctx.reverseVdpR9Bit5 = value & 0b10000000 ? 1 : 0;
                break;
            }
            case 0xFC: this->mmu->updateMemoryMapper(0, value); break;
            case 0xFD: this->mmu->updateMemoryMapper(1, value); break;
            case 0xFE: this->mmu->updateMemoryMapper(2, value); break;
            case 0xFF: this->mmu->updateMemoryMapper(3, value); break;
            default: this->putlog("ignore an unknown out port $%02X <- $%02X\n", port, value);
        }
    }

    const void* quickSave(size_t* size)
    {
        if (!this->ib->allocateQuickSaveBuffer(this->calcQuickSaveSize())) {
            return nullptr;
        }
        this->ib->quickSaveBufferPtr = 0;
        this->writeSaveChunk("BRD", &this->ctx, (int)sizeof(this->ctx));
        this->writeSaveChunk("Z80", &this->cpu->reg, (int)sizeof(this->cpu->reg));
        this->writeSaveChunk("MMU", &this->mmu->ctx, (int)sizeof(this->mmu->ctx));
        this->writeSaveChunk("PAC", &this->mmu->pac, (int)sizeof(this->mmu->pac));
        this->writeSaveChunk("R:0", &this->mmu->ram, (int)sizeof(this->mmu->ram));
        if (this->mmu->sramEnabled) {
            this->writeSaveChunk("SRM", &this->mmu->sram, (int)sizeof(this->mmu->sram));
        }
        if (this->mmu->sccEnabled && this->scc) {
            this->writeSaveChunk("SCC", &this->scc->ctx, (int)sizeof(this->scc->ctx));
        }
        this->writeSaveChunk("PSG", &this->psg->ctx, (int)sizeof(this->psg->ctx));
        this->writeSaveChunk("RTC", &this->clock->ctx, (int)sizeof(this->clock->ctx));
        this->writeSaveChunk("KNJ", &this->kanji->ctx, (int)sizeof(this->kanji->ctx));
        this->writeSaveChunk("VDP", &this->vdp->ctx, (int)sizeof(this->vdp->ctx));
        if (this->fdc) {
            this->writeSaveChunk("FDC", &this->fdc->ctx, (int)sizeof(this->fdc->ctx));
            this->writeSaveChunk("JCT", &this->fdc->journalCount, (int)sizeof(this->fdc->journalCount));
            this->writeSaveChunk("JDT", &this->fdc->journal, (int)sizeof(this->fdc->journal[0]) * this->fdc->journalCount);
        }
#ifndef MSX2_REMOVE_OPLL
        if (this->ym2413) {
            this->writeSaveChunk("OPL", this->ym2413, (int)sizeof(OPLL));
        }
#endif
        *size = LZ4_compress_default(this->ib->quickSaveBuffer,
                                     this->ib->quickSaveBufferCompressed,
                                     (int)this->ib->quickSaveBufferPtr,
                                     (int)this->ib->quickSaveBufferHeapSize);
        return this->ib->quickSaveBufferCompressed;
    }

    void quickLoad(const void* buffer, size_t bufferSize)
    {
        if (!this->ib->allocateQuickSaveBuffer(this->calcQuickSaveSize())) {
            return;
        }
        this->reset();
        int size = LZ4_decompress_safe((const char*)buffer,
                                       this->ib->quickSaveBuffer,
                                       (int)bufferSize,
                                       (int)this->ib->quickSaveBufferHeapSize);
        const char* ptr = this->ib->quickSaveBuffer;
        while (8 <= size) {
            char chunk[4];
            int chunkSize;
            strncpy(chunk, ptr, 4);
            if ('\0' != chunk[3]) break;
            ptr += 4;
            memcpy(&chunkSize, ptr, 4);
            if (chunkSize < 1) break;
            ptr += 4;
            if (chunkSize < 0) break;
            if (0 == strcmp(chunk, "BRD")) {
                putlog("extract BRD (%d bytes)", chunkSize);
                memcpy(&this->ctx, ptr, chunkSize);
            } else if (0 == strcmp(chunk, "Z80")) {
                putlog("extract Z80 (%d bytes)", chunkSize);
                memcpy(&this->cpu->reg, ptr, chunkSize);
            } else if (0 == strcmp(chunk, "MMU")) {
                putlog("extract MMU (%d bytes)", chunkSize);
                memcpy(&this->mmu->ctx, ptr, chunkSize);
                this->mmu->bankSwitchover();
            } else if (0 == strcmp(chunk, "PAC")) {
                putlog("extract PAC (%d bytes)", chunkSize);
                memcpy(&this->mmu->pac, ptr, chunkSize);
            } else if (0 == strcmp(chunk, "R:0")) {
                putlog("extract R:0 (%d bytes)", chunkSize);
                memcpy(&this->mmu->ram, ptr, chunkSize);
            } else if (0 == strcmp(chunk, "SRM")) {
                putlog("extract SRM (%d bytes)", chunkSize);
                memcpy(&this->mmu->sram, ptr, chunkSize);
            } else if (0 == strcmp(chunk, "SCC")) {
                if (this->scc) {
                    putlog("extract SCC (%d bytes)", chunkSize);
                    memcpy(&this->scc->ctx, ptr, chunkSize);
                } else {
                    putlog("ignored SCC (%d bytes)", chunkSize);
                }
            } else if (0 == strcmp(chunk, "PSG")) {
                putlog("extract PSG (%d bytes)", chunkSize);
                memcpy(&this->psg->ctx, ptr, chunkSize);
            } else if (0 == strcmp(chunk, "RTC")) {
                putlog("extract RTC (%d bytes)", chunkSize);
                memcpy(&this->clock->ctx, ptr, chunkSize);
            } else if (0 == strcmp(chunk, "KNJ")) {
                putlog("extract KNJ (%d bytes)", chunkSize);
                memcpy(&this->kanji->ctx, ptr, chunkSize);
            } else if (0 == strcmp(chunk, "VDP")) {
                putlog("extract VDP (%d bytes)", chunkSize);
                memcpy(&this->vdp->ctx, ptr, chunkSize);
                this->vdp->updateAllPalettes();
                this->vdp->updateEventTables();
            } else if (0 == strcmp(chunk, "FDC")) {
                if (this->fdc) {
                    putlog("extract FDC (%d bytes)", chunkSize);
                    memcpy(&this->fdc->ctx, ptr, chunkSize);
                } else {
                    putlog("ignored FDC (%d bytes)", chunkSize);
                }
            } else if (0 == strcmp(chunk, "JCT")) {
                if (this->fdc) {
                    putlog("extract JCT (%d bytes)", chunkSize);
                    memcpy(&this->fdc->journalCount, ptr, chunkSize);
                } else {
                    putlog("ignored JCT (%d bytes)", chunkSize);
                }
            } else if (0 == strcmp(chunk, "JDT")) {
                if (this->fdc) {
                    putlog("extract JDT (%d bytes)", chunkSize);
                    memset(&this->fdc->journal, 0, sizeof(this->fdc->journal));
                    memcpy(&this->fdc->journal, ptr, chunkSize);
                } else {
                    putlog("ignored JDT (%d bytes)", chunkSize);
                }
#ifndef MSX2_REMOVE_OPLL
            } else if (0 == strcmp(chunk, "OPL")) {
                if (this->ym2413) {
                    putlog("extract OPL (%d bytes)", chunkSize);
                    memcpy(this->ym2413, ptr, chunkSize);
                } else {
                    putlog("ignored OPL (%d bytes)", chunkSize);
                }
#endif
            }
            ptr += chunkSize;
            size -= chunkSize;
        }
    }

    unsigned short getBackdropColor()
    {
        return this->vdp ? this->vdp->getBackdropColor() : 0;
    }

  private:
    void writeSaveChunk(const char* name, const void* data, int size)
    {
        memcpy(&this->ib->quickSaveBuffer[this->ib->quickSaveBufferPtr], name, 4);
        this->ib->quickSaveBufferPtr += 4;
        memcpy(&this->ib->quickSaveBuffer[this->ib->quickSaveBufferPtr], &size, 4);
        this->ib->quickSaveBufferPtr += 4;
        memcpy(&this->ib->quickSaveBuffer[this->ib->quickSaveBufferPtr], data, size);
        this->ib->quickSaveBufferPtr += size;
    }

    size_t calcQuickSaveSize()
    {
        size_t size = 0;
        size += sizeof(this->ctx) + 8;                                               // BRD
        size += sizeof(this->cpu->reg) + 8;                                          // Z80
        size += sizeof(this->mmu->ctx) + 8;                                          // MMU
        size += sizeof(this->mmu->pac) + 8;                                          // PAC
        size += sizeof(this->mmu->ram) + 8;                                          // R:0
        size += this->mmu->sramEnabled ? sizeof(this->mmu->sram) + 8 : 0;            // SRM
        size += this->mmu->sccEnabled && this->scc ? sizeof(this->scc->ctx) + 8 : 0; // SCC
        size += sizeof(this->psg->ctx) + 8;                                          // PSG
        size += sizeof(this->clock->ctx) + 8;                                        // RTC
        size += sizeof(this->kanji->ctx) + 8;                                        // KNJ
        size += sizeof(this->vdp->ctx) + 8;                                          // VDP
        if (this->fdc) {
            size += sizeof(this->fdc->ctx) + 8;                                  // FDC
            size += sizeof(sizeof(this->fdc->journalCount)) + 8;                 // JCT
            size += sizeof(this->fdc->journal[0]) * this->fdc->journalCount + 8; // JDT
        }
#ifndef MSX2_REMOVE_OPLL
        if (this->ym2413) {
            size += sizeof(OPLL) + 8; // OPL
        }
#endif
        return size;
    }
};

#endif /* INCLUDE_MSX2_HPP */
