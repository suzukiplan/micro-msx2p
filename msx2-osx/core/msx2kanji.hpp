#ifndef INCLUDE_MSX2KANJI_HPP
#define INCLUDE_MSX2KANJI_HPP

class MSX2Kanji {
private:
    unsigned char font[0x40000];
public:
    struct Context {
        unsigned int address[2];
        unsigned char index[2];
    } ctx;

    void loadFont(const void* data, size_t size) {
        memcpy(this->font, data, size < sizeof(this->font) ? size : sizeof(this->font));
    }

    MSX2Kanji() {
        this->reset();
    }

    void reset() {
        memset(&this->ctx, 0, sizeof(this->ctx));
    }

    void outPortD8(unsigned char value) {
        this->ctx.index[0] = 0;
        this->ctx.address[0] = (this->ctx.address[0] & 0x1F800) | (value << 5);
    }
    
    void outPortD9(unsigned char value) {
        this->ctx.index[0] = 0;
        this->ctx.address[0] = (this->ctx.address[0] & 0x007E0) | (value << 11);
    }
    
    void outPortDA(unsigned char value) {
        this->ctx.index[1] = 0;
        this->ctx.address[1] = (this->ctx.address[1] & 0x1F800) | (value << 5);
    }
    
    void outPortDB(unsigned char value) {
        this->ctx.index[1] = 0;
        this->ctx.address[1] = (this->ctx.address[1] & 0x007E0) | (value << 11);
    }
    
    unsigned char inPortD9() {
        auto result = this->font[this->ctx.address[0] + this->ctx.index[0]];
        this->ctx.index[0]++;
        this->ctx.index[0] &= 0x1F;
        return result;
    }

    unsigned char inPortDB() {
        auto result = this->font[0x20000 + this->ctx.address[1] + this->ctx.index[1]];
        this->ctx.index[1]++;
        this->ctx.index[1] &= 0x1F;
        return result;
    }
};

#endif
