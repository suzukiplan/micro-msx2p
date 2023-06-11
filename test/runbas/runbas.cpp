// MSX-BASIC Runner
// License: Public Domain
#include "../../msx2-osx/core/msx2.hpp"

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
            auto col = msx2->vdp.display[(239 - y) * 568 + x];
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
    while (*text) {
        msx2->ctx.readKey = 0;
        while (msx2->ctx.readKey < 1) {
            msx2->tick(0, 0, *text);
        }
        msx2->tick(0, 0, 0);
        msx2->tick(0, 0, 0);
        text++;
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        puts("usage: runbas /path/to/file.bas [frames]");
        return -1;
    }
    FILE* bas = fopen(argv[1], "r");
    if (!bas) {
        printf("%s not found.\n", argv[1]);
        return -1;
    }
    const int frames = 3 <= argc ? atoi(argv[2]) : 600;

    unsigned char* msx2p = (unsigned char*)loadBinaryFile("MSX2P.ROM");
    if (!msx2p) {
        puts("Could not open: MSX2P.ROM");
        fclose(bas);
        return -1;
    }
    unsigned char* msx2pext = (unsigned char*)loadBinaryFile("MSX2PEXT.ROM");
    if (!msx2pext) {
        puts("Could not open: MSX2PEXT.ROM");
        free(msx2p);
        fclose(bas);
        return -1;
    }
    MSX2* msx2 = new MSX2(MSX2_COLOR_MODE_RGB555);
    msx2->setupSecondaryExist(false, false, false, true);
    msx2->setupRAM(3, 0);
    msx2->setup(0, 0, 0, msx2p, 0x8000, "MAIN");
    msx2->setup(3, 1, 0, msx2pext, 0x4000, "SUB");

    // runbas.sav があればロード
    FILE* sav = fopen("runbas.sav", "rb");
    bool loaded = false;
    if (sav) {
        fseek(sav, 0, SEEK_END);
        long saveSize = ftell(sav);
        if (0 < saveSize) {
            fseek(sav, 0, SEEK_SET);
            void* saveData = malloc(saveSize);
            if (saveData) {
                fread(saveData, 1, saveSize, sav);
                msx2->quickLoad(saveData, saveSize);
                free(saveData);
                loaded = true;
                puts("Loaded runbas.sav");
            }
        }
        fclose(sav);
    }

    if (!loaded) {
        // BASICの起動を待機
        puts("Waiting for launch MSX-BASIC...");
        msx2->reset();
        waitFrames(msx2, 600);

        // BASIC起動後の状態を runbas.sav に保持
        size_t saveSize;
        const void* saveData = msx2->quickSave(&saveSize);
        sav = fopen("runbas.sav", "wb");
        if (sav) {
            fwrite(saveData, 1, saveSize, sav);
            fclose(sav);
        }
    }

    // プログラムを打ち込む
    printf("Typing %s...\n", argv[1]);
    char buf[65536];
    while (fgets(buf, sizeof(buf), bas)) {
        // CRLF -> LF
        char* cr = strchr(buf, '\r');
        if (cr) {
            *cr = '\n';
            cr++;
            *cr = 0;
        }
        typeText(msx2, buf);
    }
    fclose(bas);

    // プログラムを実行
    typeText(msx2, "\n");
    puts("---------- START ----------");
    msx2->cpu->addBreakPoint(0xFDA4, [](void* arg) { putc(((MSX2*)arg)->cpu->reg.pair.A, stdout); });
    typeText(msx2, "RUN\n");
    waitFrames(msx2, frames);
    puts("----------- END -----------");

    // スクショをresult.bmpに保存
    puts("Writing result.bmp...");
    size_t bitmapSize;
    const void* bitmap = getBitmapScreen(msx2, &bitmapSize);
    FILE* fp = fopen("result.bmp", "wb");
    fwrite(bitmap, 1, bitmapSize, fp);
    fclose(fp);

    free(msx2p);
    free(msx2pext);
    delete msx2;
    return 0;
}