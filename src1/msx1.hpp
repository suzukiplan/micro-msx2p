/**
 * micro MSX2+ - machine for MSX1
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
#ifndef INCLUDE_MSX1_HPP
#define INCLUDE_MSX1_HPP
#include "ay8910.hpp"
#include "msx1def.h"
#include "msx1mmu.hpp"
#include "tms9918a.hpp"
#include "z80.hpp"

class MSX1
{
  public:
    enum class ColorMode {
        RGB555 = 0,
        RGB565 = 1,
        BGR565 = 2,
    };

  private:
    const int CPU_CLOCK = 3584160;
    const int VDP_CLOCK = 21504960;
    const int PSG_CLOCK = 44100;

    class InternalBuffer
    {
      public:
        short soundBuffer[1024]; // minimum: 735 (1frame)
        unsigned short soundBufferCursor;
        char* quickSaveBuffer;
        size_t quickSaveBufferPtr;
        size_t quickSaveBufferHeapSize;

        InternalBuffer()
        {
            memset(this->soundBuffer, 0, sizeof(this->soundBuffer));
            this->soundBufferCursor = 0;
            this->quickSaveBuffer = nullptr;
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
    MSX1MMU* mmu;
    TMS9918A* vdp;
    AY8910* psg;

    struct Context {
        unsigned char key;
        unsigned char readKey;
        unsigned char regC;
        unsigned char selectedKeyRow;
    } ctx;
    unsigned char* keyCodeMap;

    ~MSX1()
    {
        delete this->cpu;
        delete this->vdp;
        delete this->mmu;
        delete this->psg;
        delete this->ib;
    }

    MSX1(ColorMode colorMode, unsigned char* ram, size_t ramSize, TMS9918A::Context* vram, void (*displayCallback)(void*, int, int, unsigned short*) = nullptr)
    {
#ifdef DEBUG
        this->debug = true;
#else
        this->debug = false;
#endif
        memset(&this->keyAssign, 0, sizeof(this->keyAssign));
        this->ib = new InternalBuffer();
        this->mmu = new MSX1MMU(ram, ramSize);
        this->vdp = new TMS9918A((int)colorMode, this, [](void* arg) { ((MSX1*)arg)->cpu->generateIRQ(0x07); }, [](void* arg) { ((MSX1*)arg)->cpu->requestBreak(); }, displayCallback, vram);
        this->psg = new AY8910();
        this->cpu = new Z80([](void* arg, unsigned short addr) { return ((MSX1*)arg)->mmu->read(addr); }, [](void* arg, unsigned short addr, unsigned char value) { ((MSX1*)arg)->mmu->write(addr, value); }, [](void* arg, unsigned short port) { return ((MSX1*)arg)->inPort((unsigned char)port); }, [](void* arg, unsigned short port, unsigned char value) { ((MSX1*)arg)->outPort((unsigned char)port, value); }, this, false);
        this->cpu->wtc.fetch = 1;
        this->cpu->setConsumeClockCallbackFP([](void* arg, int cpuClocks) {
            ((MSX1*)arg)->consumeClock(cpuClocks);
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
        if (MSX1_JOY_S1 == code) {
            this->keyAssign[player].s1 = &this->keyCodes[assign];
        } else if (MSX1_JOY_S2 == code) {
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
    }

    void setup(int pri, int idx, void* data, int size, const char* label = NULL)
    {
        this->mmu->setup(pri, idx, (unsigned char*)data, size, label);
    }

    void loadRom(void* data, int size, int romType)
    {
        this->mmu->setupCartridge(1, 2, data, size, romType);
        this->reset();
    }

    void ejectRom()
    {
        this->mmu->clearCartridge();
        this->reset();
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
    inline int getDisplayWidth() { return 256; }
    inline int getDisplayHeight() { return 192; }

    inline void consumeClock(int cpuClocks)
    {
        // Asynchronous with PSG
        this->psg->ctx.bobo += cpuClocks * this->PSG_CLOCK;
        while (0 < this->psg->ctx.bobo) {
            this->psg->ctx.bobo -= this->CPU_CLOCK;
            this->psg->tick(&this->ib->soundBuffer[this->ib->soundBufferCursor], nullptr, 81);
            this->ib->soundBufferCursor++;
            this->ib->soundBufferCursor &= sizeof(this->ib->soundBuffer) - 1;
        }
        // Asynchronous with VDP
        this->vdp->ctx->bobo += cpuClocks * VDP_CLOCK;
        while (0 < this->vdp->ctx->bobo) {
            this->vdp->ctx->bobo -= CPU_CLOCK;
            this->vdp->tick();
        }
    }

    inline unsigned char inPort(unsigned char port)
    {
        switch (port) {
            case 0x90: return 0x00; // printer
            case 0x98: return this->vdp->readData();
            case 0x99: return this->vdp->readStatus();
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
                if (this->keyAssign[0].s1 && 0 == (this->psg->getPad1() & MSX1_JOY_S1)) {
                    if (this->ctx.selectedKeyRow == this->keyAssign[0].s1->y[0]) {
                        result |= bit[this->keyAssign[0].s1->x[0]];
                    }
                }
                if (this->keyAssign[0].s2 && 0 == (this->psg->getPad1() & MSX1_JOY_S2)) {
                    if (this->ctx.selectedKeyRow == this->keyAssign[0].s2->y[0]) {
                        result |= bit[this->keyAssign[0].s2->x[0]];
                    }
                }
                if (this->keyAssign[1].s1 && 0 == (this->psg->getPad2() & MSX1_JOY_S1)) {
                    if (this->ctx.selectedKeyRow == this->keyAssign[1].s1->y[0]) {
                        result |= bit[this->keyAssign[1].s1->x[0]];
                    }
                }
                if (this->keyAssign[1].s2 && 0 == (this->psg->getPad2() & MSX1_JOY_S2)) {
                    if (this->ctx.selectedKeyRow == this->keyAssign[1].s2->y[0]) {
                        result |= bit[this->keyAssign[1].s2->x[0]];
                    }
                }
                return ~result;
            }
            case 0xAA: return this->ctx.regC;
        }
        return 0xFF;
    }

    inline void outPort(unsigned char port, unsigned char value)
    {
        switch (port) {
            case 0x98: this->vdp->writeData(value); break;
            case 0x99: this->vdp->writeAddress(value); break;
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
                    }
                }
                break;
            }
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
        this->writeSaveChunk("RAM", this->mmu->ram, (int)this->mmu->ramSize);
        if (0 < this->mmu->sramSize) {
            this->writeSaveChunk("SRM", this->mmu->sram, (int)this->mmu->sramSize);
        }
        this->writeSaveChunk("PSG", &this->psg->ctx, (int)sizeof(this->psg->ctx));
        this->writeSaveChunk("VDP", this->vdp->ctx, (int)sizeof(TMS9918A::Context));
        *size = this->ib->quickSaveBufferPtr;
        return this->ib->quickSaveBuffer;
    }

    void quickLoad(const void* buffer, size_t bufferSize)
    {
        if (!this->ib->allocateQuickSaveBuffer(this->calcQuickSaveSize())) {
            return;
        }
        this->reset();
        const char* ptr = this->ib->quickSaveBuffer;
        int size = (int)bufferSize;
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
                memcpy(&this->ctx, ptr, chunkSize);
            } else if (0 == strcmp(chunk, "Z80")) {
                memcpy(&this->cpu->reg, ptr, chunkSize);
            } else if (0 == strcmp(chunk, "MMU")) {
                memcpy(&this->mmu->ctx, ptr, chunkSize);
                this->mmu->bankSwitchover();
            } else if (0 == strcmp(chunk, "RAM")) {
                memcpy(this->mmu->ram, ptr, chunkSize <= this->mmu->ramSize ? chunkSize : this->mmu->ramSize);
            } else if (0 == strcmp(chunk, "SRM") && this->mmu->sram) {
                memcpy(this->mmu->sram, ptr, chunkSize <= this->mmu->sramSize ? chunkSize : this->mmu->sramSize);
            } else if (0 == strcmp(chunk, "PSG")) {
                memcpy(&this->psg->ctx, ptr, chunkSize);
            } else if (0 == strcmp(chunk, "VDP")) {
                memcpy(this->vdp->ctx, ptr, chunkSize);
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
        size += sizeof(this->ctx) + 8;                         // BRD
        size += sizeof(this->cpu->reg) + 8;                    // Z80
        size += sizeof(this->mmu->ctx) + 8;                    // MMU
        size += this->mmu->ramSize + 8;                        // RAM
        size += this->mmu->sram ? this->mmu->sramSize + 8 : 0; // SRM
        size += sizeof(this->psg->ctx) + 8;                    // PSG
        size += sizeof(TMS9918A::Context) + 8;                 // VDP
        return size;
    }
};

#endif /* INCLUDE_MSX1_HPP */
