#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[])
{
    printf("extern \"C\" {\n");
    for (int i = 1; i < argc; i++) {
        FILE* fp = fopen(argv[i], "rt");
        char buf[1024];
        fgets(buf, sizeof(buf), fp);
        fclose(fp);
        char* str = strstr(buf, " = {");
        if (str) {
            *str = 0;
            printf("    extern %s;\n", buf);
        }
    }
    printf("}\n");
    return 0;
}
