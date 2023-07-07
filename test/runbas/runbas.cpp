/**
 * MSX-BASIC Runner for CLI
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
#include "../../src/msx2.hpp"

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

void* loadBinaryFile(const char* path, size_t* sizeResult = nullptr)
{
    FILE* fp = fopen(path, "rb");
    if (!fp) {
        return nullptr;
    }
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    if (size < 1) {
        fclose(fp);
        return nullptr;
    }
    void* buf = malloc(size);
    if (!buf) {
        fclose(fp);
        return nullptr;
    }
    fseek(fp, 0, SEEK_SET);
    if (size != fread(buf, 1, size, fp)) {
        fclose(fp);
        free(buf);
        return nullptr;
    }
    fclose(fp);
    if (sizeResult) *sizeResult = (size_t)size;
    return buf;
}

void waitFrames(MSX2* msx2, int frames) {
    for (int i = 0; i < frames; i++) {
        msx2->tick(0, 0, 0);
        size_t soundSize;
        msx2->getSound(&soundSize);
    }
}

unsigned char bit5_to_bit8(unsigned char col)
{
    unsigned char result = (col & 0b00011111) << 3;
    result |= (col & 0b10000000) >> 5;
    result |= (col & 0b01000000) >> 5;
    result |= (col & 0b00100000) >> 5;
    return result;
}

const void* getBitmapScreen(MSX2* msx2, size_t* size) {
    static unsigned char buf[14 + 40 + 568 * 480 * 4];
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
    header.width = 568;
    header.height = 480;
    header.planes = 1;
    header.bits = 32;
    header.ctype = 0;
    header.gsize = 568 * 480 * 4;
    header.xppm = 1;
    header.yppm = 1;
    header.cnum = 0;
    header.inum = 0;
    memcpy(&buf[ptr], &header, sizeof(header));
    ptr += sizeof(header);
    for (int y = 0; y < 240; y++) {
        for (int x = 0; x < 568; x++) {
            auto col = msx2->getDisplay()[(239 - y) * 568 + x];
            buf[ptr++] = bit5_to_bit8(col & 0b0000000000011111);
            buf[ptr++] = bit5_to_bit8((col & 0b0000001111100000) >> 5);
            buf[ptr++] = bit5_to_bit8((col & 0b0111110000000000) >> 10);
            buf[ptr++] = 0x00;
        }
        memcpy(&buf[ptr], &buf[ptr - 568 * 4], 568 * 4);
        ptr += 568 * 4;
    }
    return buf;
}

void typeText(MSX2* msx2, const char* text)
{
    size_t soundSize;
    while (*text) {
        msx2->ctx.readKey = 0;
        while (msx2->ctx.readKey < 1) {
            msx2->tick(0, 0, *text);
            msx2->getSound(&soundSize);
        }
        msx2->tick(0, 0, 0);
        msx2->getSound(&soundSize);
        msx2->tick(0, 0, 0);
        msx2->getSound(&soundSize);
        text++;
    }
}

void trimstring(char* src)
{
    int i;
    for (i = 0; ' ' == src[i] || '\t' == src[i]; i++) {
        ;
    }
    if (i) {
        memmove(src, &src[i], strlen(&src[i]) + 1);
    }
    for (int len = (int)strlen(src) - 1; 0 <= len && (' ' == src[len] || '\t' == src[len]); len--) {
        src[len] = '\0';
    }
}

// カレントディレクトリにスクショ（result.bmp）を書き出す
void writeResultBitmap(MSX2* msx2, const char* output)
{
    printf("Writing %s...\n", output);
    size_t bitmapSize;
    const void* bitmap = getBitmapScreen(msx2, &bitmapSize);
    FILE* fp = fopen(output, "wb");
    fwrite(bitmap, 1, bitmapSize, fp);
    fclose(fp);
}

int main(int argc, char* argv[])
{
    struct Options {
        const char* basFile;
        const char* error;
        const char* output;
        const char* diskImage;
        void* diskImageRaw;
        size_t diskImageSize;
        int frames;
    } opt;
    memset(&opt, 0, sizeof(opt));
    opt.frames = 600;
    opt.output = "result.bmp";
    bool optError = false;
    for (int i = 1; i < argc; i++) {
        if ('-' == argv[i][0]) {
            i++;
            if (argc <= i) {
                optError = true;
            } else {
                switch (argv[i - 1][1]) {
                    case 'f': opt.frames = atoi(argv[i]); break;
                    case 'e': opt.error = argv[i]; break;
                    case 'd': opt.diskImage = argv[i]; break;
                    case 'o': opt.output = argv[i]; break;
                    default: optError = true;
                }
            }
        } else {
            if (opt.basFile) {
                optError = true;
                break;
            } else {
                opt.basFile = argv[i];
            }
        }
    } 
    if (optError) {
        puts("usage: runbas [-f frames]");
        puts("              [-e message]");
        puts("              [-d /path/to/image.dsk]");
        puts("              [-o /path/to/result.bmp]");
        puts("              [/path/to/file.bas]");
        return -1;
    }
    FILE* bas = opt.basFile ? fopen(argv[1], "r") : stdin;
    if (!bas) {
        printf("%s not found.\n", argv[1]);
        return -1;
    }

    char path[4096];
    char* basePath = getenv("RUNBAS_PATH");

    // MSX2P.ROM をロード（required）
    if (basePath) {
        snprintf(path, sizeof(path), "%s/MSX2P.ROM", basePath);
    } else {
        strcpy(path, "MSX2P.ROM");
    }
    unsigned char* msx2p = (unsigned char*)loadBinaryFile(path);
    if (!msx2p) {
        puts("Could not open: MSX2P.ROM");
        fclose(bas);
        return -1;
    }

    // MSX2PEXT.ROM をロード（required）
    if (basePath) {
        snprintf(path, sizeof(path), "%s/MSX2PEXT.ROM", basePath);
    } else {
        strcpy(path, "MSX2PEXT.ROM");
    }
    unsigned char* msx2pext = (unsigned char*)loadBinaryFile(path);
    if (!msx2pext) {
        puts("Could not open: MSX2PEXT.ROM");
        free(msx2p);
        fclose(bas);
        return -1;
    }

    // DISK.ROM をロード（-dオプション指定時のみ）
    unsigned char* disk = nullptr;
    unsigned char empty[0x4000];
    memset(empty, 0, sizeof(empty));
    if (opt.diskImage) {
        if (basePath) {
            snprintf(path, sizeof(path), "%s/DISK.ROM", basePath);
        } else {
            strcpy(path, "DISK.ROM");
        }
        disk = (unsigned char*)loadBinaryFile(path);
        if (!disk) {
            puts("Could not open: DISK.ROM");
            free(msx2pext);
            free(msx2p);
            fclose(bas);
            return -1;
        }
        opt.diskImageRaw = loadBinaryFile(opt.diskImage, &opt.diskImageSize);
        if (!opt.diskImageRaw) {
            printf("Could not open: %s\n", opt.diskImage);
            free(msx2pext);
            free(msx2p);
            fclose(bas);
            return -1;
        }
    }

    MSX2 msx2(MSX2_COLOR_MODE_RGB555);
    msx2.setupSecondaryExist(false, false, false, true);
    msx2.setupRAM(3, 0);
    msx2.setup(0, 0, 0, msx2p, 0x8000, "MAIN");
    msx2.setup(3, 1, 0, msx2pext, 0x4000, "SUB");
    bool loaded = false;
    if (disk) {
        // ディスク使用時はシステムに認識させるため常にステートロードしない
        msx2.setup(3, 2, 0, empty, 0x4000, "DISK");
        msx2.setup(3, 2, 2, disk, 0x4000, "DISK");
        msx2.setup(3, 2, 4, empty, 0x4000, "DISK");
        msx2.setup(3, 2, 6, empty, 0x4000, "DISK");
        msx2.insertDisk(0, opt.diskImageRaw, opt.diskImageSize, false);
        printf("Insert disk: %s\n", opt.diskImage);
    } else {
        // runbas.sav があればロード
        if (basePath) {
            snprintf(path, sizeof(path), "%s/runbas.sav", basePath);
        } else {
            strcpy(path, "runbas.sav");
        }
        FILE* sav = fopen(path, "rb");
        if (sav) {
            fseek(sav, 0, SEEK_END);
            long saveSize = ftell(sav);
            if (0 < saveSize) {
                fseek(sav, 0, SEEK_SET);
                void* saveData = malloc(saveSize);
                if (saveData) {
                    fread(saveData, 1, saveSize, sav);
                    msx2.quickLoad(saveData, saveSize);
                    free(saveData);
                    loaded = true;
                    puts("Loaded runbas.sav");
                }
            }
            fclose(sav);
        }
    }

    if (!loaded) {
        // BASICの起動を待機
        puts("Waiting for launch MSX-BASIC...");
        msx2.reset();
        waitFrames(&msx2, 600);

        // BASIC起動後の状態を runbas.sav に保持
        if (!disk) {
            size_t saveSize;
            const void* saveData = msx2.quickSave(&saveSize);
            FILE* sav = fopen(path, "wb");
            if (sav) {
                fwrite(saveData, 1, saveSize, sav);
                fclose(sav);
                puts("Saved runbas.sav");
            }
        }
    }

    // プログラムを打ち込む
    if (opt.basFile) {
        printf("Typing %s...\n", opt.basFile);
    } else {
        puts("[STDIN mode]");
    }
    char buf[65536];
    while (fgets(buf, sizeof(buf), bas)) {
        // CRLF -> LF
        char* cr = strchr(buf, '\r');
        if (cr) {
            *cr = '\n';
            cr++;
            *cr = 0;
        }
        trimstring(buf);
        // 標準入力モードの場合は空行検出で抜ける
        if (!opt.basFile && (0 == buf[0] || '\n' == buf[0])) {
            break;
        } else {
            typeText(&msx2, buf);
        }
    }
    if (opt.basFile) {
        fclose(bas);
    }

    // プログラムを実行
    typeText(&msx2, "\n");
    puts("---------- START ----------");
    int errorIndex = 0;
    msx2.cpu->addBreakPoint(0xFDA4, [&](void* arg) {
        char c = (char)(((MSX2*)arg)->cpu->reg.pair.A);
        putc(c, stdout);
        if (opt.error) {
            if (c == opt.error[errorIndex]) {
                errorIndex++;
                if (0 == opt.error[errorIndex]) {
                    puts("\n[ABORT]");
                    exit(-1);
                }
            } else {
                errorIndex = 0;
            }
        }
    });
    typeText(&msx2, "RUN\n");
    waitFrames(&msx2, opt.frames);
    puts("----------- END -----------");
    writeResultBitmap(&msx2, opt.output);
    free(msx2p);
    free(msx2pext);
    if (disk) free(disk);
    if (opt.diskImageRaw) free(opt.diskImageRaw);
    return 0;
}
