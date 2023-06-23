/**
 * micro MSX2+ - Kanji Driver
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
#ifndef INCLUDE_MSX2KANJI_HPP
#define INCLUDE_MSX2KANJI_HPP

class MSX2Kanji
{
  private:
    unsigned char font[0x40000];

  public:
    struct Context {
        unsigned int address[2];
        unsigned char index[2];
    } ctx;

    void loadFont(const void* data, size_t size)
    {
        memcpy(this->font, data, size < sizeof(this->font) ? size : sizeof(this->font));
    }

    MSX2Kanji()
    {
        this->reset();
    }

    void reset()
    {
        memset(&this->ctx, 0, sizeof(this->ctx));
    }

    void outPortD8(unsigned char value)
    {
        this->ctx.index[0] = 0;
        this->ctx.address[0] = (this->ctx.address[0] & 0x1F800) | (value << 5);
    }

    void outPortD9(unsigned char value)
    {
        this->ctx.index[0] = 0;
        this->ctx.address[0] = (this->ctx.address[0] & 0x007E0) | (value << 11);
    }

    void outPortDA(unsigned char value)
    {
        this->ctx.index[1] = 0;
        this->ctx.address[1] = (this->ctx.address[1] & 0x1F800) | (value << 5);
    }

    void outPortDB(unsigned char value)
    {
        this->ctx.index[1] = 0;
        this->ctx.address[1] = (this->ctx.address[1] & 0x007E0) | (value << 11);
    }

    unsigned char inPortD9()
    {
        auto result = this->font[this->ctx.address[0] + this->ctx.index[0]];
        this->ctx.index[0]++;
        this->ctx.index[0] &= 0x1F;
        return result;
    }

    unsigned char inPortDB()
    {
        auto result = this->font[0x20000 + this->ctx.address[1] + this->ctx.index[1]];
        this->ctx.index[1]++;
        this->ctx.index[1] &= 0x1F;
        return result;
    }
};

#endif
