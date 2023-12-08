#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int main(int argc, char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "bmp2img /path/to/image.bmp\n");
        return -1;
    }
    char imgName[1024];
    char* cp = strrchr(argv[1], '/');
    if (cp) {
        strcpy(imgName, cp + 1);
    } else {
        cp = strrchr(argv[1], '\\');
        if (cp) {
            strcpy(imgName, cp + 1);
        } else {
            strcpy(imgName, argv[1]);
        }
    }
    cp = strchr(imgName, '.');
    if (cp) *cp = 0;

    FILE* fp = fopen(argv[1], "rb");
    if (!fp) {
        fprintf(stderr, "file open error\n");
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    int size = (int)ftell(fp);
    if (size < 64) {
        fprintf(stderr, "invalid file format\n");
        fclose(fp);
        return -1;
    }
    fseek(fp, 0, SEEK_SET);
    unsigned char* bmp = (unsigned char*)malloc(size);
    fread(bmp, 1, size, fp);
    fclose(fp);

    if (0 != memcmp(bmp, "BM", 2)) {
        fprintf(stderr, "invalid file format (BM)\n");
        free(bmp);
        return -1;
    }
    BitmapHeader head;
    memcpy(&head, &bmp[14], sizeof(head));
    if (8 != head.bits) {
        fprintf(stderr, "invalid file format (not 8bit color mode)\n");
        free(bmp);
        return -1;
    }
    if (0 != head.ctype) {
        fprintf(stderr, "invalid file format (not uncompressed)\n");
        free(bmp);
        return -1;
    }
    if (320 < head.width) {
        fprintf(stderr, "invalid file format (width 320+)\n");
        free(bmp);
        return -1;
    }
    if (240 < head.height) {
        fprintf(stderr, "invalid file format (height 240+)\n");
        free(bmp);
        return -1;
    }

    unsigned int offset;
    memcpy(&offset, &bmp[0xA], 4);
    unsigned char* palPtr = &bmp[14 + 40];
    unsigned short pal565[256];
    for (int i = 0; i < 256; i++, palPtr += 4) {
        pal565[i] = (palPtr[2] & 0b11111000) >> 3;
        pal565[i] <<= 6;
        pal565[i] |= (palPtr[1] & 0b11111100) >> 2;
        pal565[i] <<= 5;
        pal565[i] |= (palPtr[0] & 0b11111000) >> 3;
        unsigned short work = (pal565[i] & 0xFF00) >> 8;
        pal565[i] &= 0xFF;
        pal565[i] <<= 8;
        pal565[i] |= work;
    }
    int imgSize = head.width * head.height * 2;
    unsigned short* img = (unsigned short*)malloc(imgSize);
    unsigned short* imgPtr = img;
    unsigned char* bmpPtr = &bmp[offset];
    for (int y = 0; y < head.height; y++) {
        for (int x = 0; x < head.width; x++) {
            imgPtr[y * head.width + x] = pal565[bmpPtr[(head.height - y - 1) * head.width + x]];
        }
    }
    free(bmp);

    imgSize /= 2;
    printf("const unsigned short rom_%s[%d] = {\n", imgName, imgSize);
    bool firstLine = true;
    unsigned char* cptr = (unsigned char*)img;
    int left = imgSize;
    for (int i = 0; i < imgSize; i += 16, left -= 16) {
        if (firstLine) {
            firstLine = false;
        } else {
            printf(",\n");
        }
        printf("    ");
        for (int j = 0; j < (left < 16 ? left : 16); j++) {
            if (j) {
                printf(", 0x%04X", img[i + j]);
            } else {
                printf("0x%04X", img[i + j]);
            }
        }
    }
    printf("\n};\n");
    free(img);
    return 0;
}
