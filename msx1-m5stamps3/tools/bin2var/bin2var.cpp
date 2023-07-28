#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "bin2var /path/to/binary.rom\n");
        return -1;
    }
    FILE* fp = fopen(argv[1], "rb");
    if (!fp) {
        fprintf(stderr, "file open error\n");
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    int size = (int)ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char varName[1024];
    char* cp = strrchr(argv[1], '/');
    if (cp) {
        strcpy(varName, cp + 1);
    } else {
        cp = strrchr(argv[1], '\\');
        if (cp) {
            strcpy(varName, cp + 1);
        } else {
            strcpy(varName, argv[1]);
        }
    }
    cp = strchr(varName, '.');
    if (cp) *cp = 0;
    printf("const unsigned char rom_%s[%d] = {\n", varName, size);
    bool firstLine = true;
    while (1) {
        unsigned char buf[16];
        int readSize = (int)fread(buf, 1, sizeof(buf), fp);
        if (readSize < 1) {
            printf("\n");
            break;
        }
        if (firstLine) {
            firstLine = false;
        } else {
            printf(",\n");
        }
        printf("    ");
        for (int i = 0; i < readSize; i++) {
            if (i) {
                printf(", 0x%02X", buf[i]);
            } else {
                printf("0x%02X", buf[i]);
            }
        }
    }
    printf("};\n");
    fclose(fp);
    return 0;
}
