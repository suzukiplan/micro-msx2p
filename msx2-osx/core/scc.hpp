#ifndef INCLUDE_SCC_HPP
#define INCLUDE_SCC_HPP
#include <string.h>

class SCC {
public:
    bool enabled;
    struct Channel {
        signed char waveforms[32];
        int counter;
        unsigned short period;
        unsigned char volume;
        unsigned char index;
    };
    
    struct Context {
        struct Channel ch[5];
        int sw;
    } ctx;
    
    SCC() {
        this->reset();
        this->enabled = false;
    }
    
    void reset() { memset(&this->ctx, 0, sizeof(this->ctx)); }
    
    inline unsigned char read(unsigned short addr) {
        addr &= 0xFF;
        return addr < 0x80 ? this->ctx.ch[addr / 0x20].waveforms[addr & 0x1F] : 0xFF;
    }
    
    inline void write(unsigned short addr, unsigned char value) {
        addr &= 0xFF;
        if (addr & 0x80) {
            addr &= 0x3F;
            if (addr < 0x0A) {
                Channel* ch = &this->ctx.ch[addr >> 1];
                ch->period = addr & 0x01 ? (ch->period & 0xff) | ((value & 0x0F) << 8) : (ch->period & 0xf00) | value;
            } else if (addr < 0x0F) {
                this->ctx.ch[addr - 0x0A].volume = value & 0x0F;
            } else if (0x0F == addr) {
                this->ctx.sw = value & 0x1F;
            }
        } else {
            this->ctx.ch[addr >> 5].waveforms[addr & 0x1F] = (signed char)value;
        }
    }
    
    inline void tick(short* left, short* right, unsigned int cycles) {
        if (!this->enabled) return;
        int mix[5];
        int sw = this->ctx.sw;
        for(int i = 0; i < 5; i++) {
            Channel* ch = &this->ctx.ch[i];
            if (ch->period) {
                ch->counter += cycles;
                while (0 <= ch->counter) {
                    ch->counter -= ch->period;
                    ch->index++;
                    ch->index &= 0x1F;
                }
            } else {
                ch->index++;
                ch->index &= 0x1F;
            }
            mix[i] = sw & 1 ? this->ctx.ch[4 == i ? 3 : i].waveforms[ch->index] * ch->volume : 0;
            sw >>= 1;
        }
        int result = mix[0] + mix[1] + mix[2] + mix[3] + mix[4];
        *left = this->to_short((*left) + result);
        *right = this->to_short((*right) + result);
    }
    
private:
    inline short to_short(int i) {
        if (32767 < i) return (short)32767;
        if (i < -32768) return (short)-32768;
        return (short)i;
    }
};

#endif /* INCLUDE_SCC_HPP */
