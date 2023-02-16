#ifndef INCLUDE_SCC_HPP
#define INCLUDE_SCC_HPP

class SCC {
public:
    bool enabled;

    struct Channel {
        signed char samples[32];
        int counter;
        unsigned short period;
        unsigned char volume;
        unsigned char index;
    };
    
    struct Context {
        struct Channel ch[5];
        int sw;
        int mode;
    } ctx;

    SCC() {
        this->reset();
        this->enabled = false;
    }

    void reset() { memset(&this->ctx, 0, sizeof(this->ctx)); }

    // NOTE: The pins to read are on the hardware, but the interface is probably not on the MSX.
    inline unsigned char read(unsigned short addr) {
        switch (addr & 0xFF00) {
            case 0x9800: {
                // Konami's MegaROMs with SCC
                addr &= 0xFF;
                if (addr < 0x20) {
                    // 32 bytes signed to define the envelope of the channel 1
                    return this->ctx.ch[0].samples[addr & 0x1F];
                } else if (addr < 0x40) {
                    // 32 bytes signed to define the envelope of the channel 2
                    return this->ctx.ch[1].samples[addr & 0x1F];
                } else if (addr < 0x60) {
                    // 32 bytes signed to define the envelope of the channel 3
                    return this->ctx.ch[2].samples[addr & 0x1F];
                } else if (addr < 0x80) {
                    // 32 bytes signed to define the envelope of the channels 4 and 5
                    return this->ctx.ch[3].samples[addr & 0x1F];
                } else {
                    return 0xFF;
                }
            }
            case 0xB800: {
                // Konami's Sound Cartridge for Snatcher or SD Snatcher
                addr &= 0xFF;
                if (addr < 0x20) {
                    // 32 bytes signed to define the envelope of the channel 1
                    return this->ctx.ch[0].samples[addr & 0x1F];
                } else if (addr < 0x40) {
                    // 32 bytes signed to define the envelope of the channel 2
                    return this->ctx.ch[1].samples[addr & 0x1F];
                } else if (addr < 0x60) {
                    // 32 bytes signed to define the envelope of the channel 3
                    return this->ctx.ch[2].samples[addr & 0x1F];
                } else if (addr < 0x80) {
                    // 32 bytes signed to define the envelope of the channels 4
                    return this->ctx.ch[3].samples[addr & 0x1F];
                } else if (addr < 0xA0) {
                    // 32 bytes signed to define the envelope of the channels 5
                    return this->ctx.ch[4].samples[addr & 0x1F];
                } else {
                    return 0xFF;
                }
            }
        }
        return 0xFF;
    }

    inline void write(unsigned short addr, unsigned char value) {
        switch (addr & 0xFF00) {
            case 0x9800: {
                // Konami's MegaROMs with SCC
                if (addr & 0x80) {
                    addr &= 0x3F;
                    switch (addr) {
                        case 0x00:
                        case 0x02:
                        case 0x04:
                        case 0x06:
                        case 0x08: {
                            auto ch = &this->ctx.ch[addr >> 1];
                            ch->period = (ch->period & 0xf00) | value;
                            break;
                        }
                        case 0x01:
                        case 0x03:
                        case 0x05:
                        case 0x07:
                        case 0x09: {
                            auto ch = &this->ctx.ch[addr >> 1];
                            ch->period = (ch->period & 0xff) | ((value & 0x0F) << 8);
                            break;
                        }
                        case 0x0A:
                        case 0x0B:
                        case 0x0C:
                        case 0x0D:
                        case 0x0E:
                            this->ctx.ch[addr - 0x0A].volume = value & 0x0F;
                            break;
                        case 0x0F:
                            this->ctx.sw = value & 0x1F;
                            break;
                    }
                } else {
                    addr &= 0xFF;
                    this->ctx.ch[addr >> 5].samples[addr & 0x1F] = (signed char)value;
                }
                break;
            }
            case 0xB800: {
                // Konami's Sound Cartridge for Snatcher or SD Snatcher
                addr &= 0xFF;
                if(0xA0 <= addr) {
                    addr -= 0xA0;
                    switch(addr) {
                        case 0x00:
                        case 0x02:
                        case 0x04:
                        case 0x06:
                        case 0x08: {
                            auto ch = &this->ctx.ch[addr >> 1];
                            ch->period = (ch->period & 0xF00) | value;
                            break;
                        }
                        case 0x01:
                        case 0x03:
                        case 0x05:
                        case 0x07:
                        case 0x09: {
                            auto ch = &this->ctx.ch[addr >> 1];
                            ch->period = ch->period & 0xFF;
                            ch->period |= (value & 0x0F) << 8;
                            break;
                        }
                        case 0x0A:
                        case 0x0B:
                        case 0x0C:
                        case 0x0D:
                        case 0x0E:
                            this->ctx.ch[addr - 0x0A].volume = value & 0x0F;
                            break;
                        case 0x0f:
                            this->ctx.sw = value & 0x1F;
                            break;
                    }
                } else {
                    this->ctx.ch[addr >> 5].samples[addr & 0x1F] = value;
                }
                break;
            }
        }
    }
    
    inline void setMode(unsigned char mode) {
        this->ctx.mode = mode;
    }

    inline void tick(short* left, short* right, unsigned int cycles) {
        if (!this->enabled) return;
        int mix[5];
        int i;
        auto sw = this->ctx.sw;
        for(i = 0; i < 5; i++) {
            auto ch = &this->ctx.ch[i];
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
            if (sw & 1) {
                int si = (4 == i && 0 == (this->ctx.mode & 0x20)) ? 3 : i;
                mix[i] = this->ctx.ch[si].samples[ch->index] * ch->volume;
            } else {
                mix[i] = 0;
            }
            sw >>= 1;
        }
        int result = (mix[0] + mix[1] + mix[2]) & 0xFFFF;
        result |= ((mix[0] + mix[3] + mix[4]) & 0xffff) << 16;
        *left = (short)((*left) + result);
        *right = (short)((*right) + result);
    }
};

#endif /* INCLUDE_SCC_HPP */
