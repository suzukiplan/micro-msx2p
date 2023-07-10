/**
 * Performance Tester
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
#include "../../src1/msx1.hpp"
#include <chrono>

typedef struct BitmapHeader_ {
    int isize;             /* 情報ヘッダサイズ */
    int width;             /* 幅 */
    int height;            /* 高さ */
    unsigned short planes; /* プレーン数 */
    unsigned short bits;   /* 色ビット数 */
    unsigned int ctype;    /* 圧縮形式 */
    unsigned int gsize;    /* 画像データサイズ */
    int xppm;              /* X方向解像度 */
    int yppm;              /* Y方向解像度 */
    unsigned int cnum;     /* 使用色数 */
    unsigned int inum;     /* 重要色数 */
} BitmapHeader;

void* loadFile(const char* path, size_t* size)
{
    FILE* fp = fopen(path, "rb");
    if (!fp) {
        printf("File not found: %s\n", path);
        return nullptr;
    }
    fseek(fp, 0, SEEK_END);
    *size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    void* result = malloc(*size);
    if (!result) {
        puts("No memory");
        fclose(fp);
        return nullptr;
    }
    if (*size != fread(result, 1, *size, fp)) {
        printf("Read error: %s\n", path);
        fclose(fp);
        free(result);
        return nullptr;
    }
    fclose(fp);
    return result;
}

void* init(MSX1* msx1, int pri, int idx, const char* path, const char* label)
{
    size_t size;
    void* data = loadFile(path, &size);
    if (0 == strcmp(label, "CART")) {
        msx1->loadRom(data, size, MSX1_ROM_TYPE_NORMAL);
    } else {
        msx1->setup(pri, idx, data, size, label);
    }
    return data;
}

unsigned char bit5_to_bit8(unsigned char col)
{
    unsigned char result = (col & 0b00011111) << 3;
    result |= (col & 0b10000000) >> 5;
    result |= (col & 0b01000000) >> 5;
    result |= (col & 0b00100000) >> 5;
    return result;
}

const void* getBitmapScreen(MSX1* msx1, size_t* size) {
    static unsigned char buf[14 + 40 + 256 * 192 * 4];
    int iSize = (int)sizeof(buf);
    *size = iSize;
    memset(buf, 0, sizeof(buf));
    int ptr = 0;
    buf[ptr++] = 'B';
    buf[ptr++] = 'M';
    memcpy(&buf[ptr], &iSize, 4);
    ptr += 4;
    ptr += 4;
    iSize = 14 + 40;
    memcpy(&buf[ptr], &iSize, 4);
    ptr += 4;
    BitmapHeader header;
    header.isize = 40;
    header.width = 256;
    header.height = 192;
    header.planes = 1;
    header.bits = 32;
    header.ctype = 0;
    header.gsize = 256 * 192 * 4;
    header.xppm = 1;
    header.yppm = 1;
    header.cnum = 0;
    header.inum = 0;
    memcpy(&buf[ptr], &header, sizeof(header));
    ptr += sizeof(header);
    for (int y = 0; y < 192; y++) {
        for (int x = 0; x < 256; x++) {
            auto col = msx1->getDisplay()[(191 - y) * 256 + x];
            buf[ptr++] = bit5_to_bit8(col & 0b0000000000011111);
            buf[ptr++] = bit5_to_bit8((col & 0b0000001111100000) >> 5);
            buf[ptr++] = bit5_to_bit8((col & 0b0111110000000000) >> 10);
            buf[ptr++] = 0x00;
        }
    }
    return buf;
}

int main()
{
    unsigned char ram[0x2000];
    MSX1* msx1 = new MSX1(MSX1::ColorMode::RGB555, ram, sizeof(ram));
    void* main = init(msx1, 0, 0, "cbios_main_msx1.rom", "MAIN");
    void* logo = init(msx1, 0, 4, "cbios_logo_msx1.rom", "LOGO");
    void* game = init(msx1, 0, 4, "game.rom", "CART");

    // execute 3600 frames (1minute)
    auto start = std::chrono::system_clock::now();
    void* data;
    size_t size;
    for (int i = 0; i < 3600; i++) {
        msx1->tick(0, 0, 0);
        data = msx1->getSound(&size); // clear sound buffer
    }
    auto end = std::chrono::system_clock::now();
    auto time = end - start;
    int msec = std::chrono::duration_cast<std::chrono::milliseconds>(time).count();
    printf("Total time: %dms\n", msec);
    printf("Frame average: %fms\n", msec / 3600.0);
    printf("Frame usage: %.2f%%\n", (msec / 3600.0) / (1000.0 / 60.0) * 100.0);

    FILE* fp = fopen("result.bmp", "wb");
    if (fp) {
        size_t size;
        auto bmp = getBitmapScreen(msx1, &size);
        fwrite(bmp, 1, size, fp);
        fclose(fp);
    }

    delete msx1;
    free(main);
    free(logo);
    free(game);
    return 0;
}
