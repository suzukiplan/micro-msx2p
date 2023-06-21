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
#include "../../msx2-osx/core/msx2.hpp"
#include <chrono>

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

void* init(MSX2* msx2, int pri, int sec, int idx, const char* path, const char* label)
{
    size_t size;
    void* data = loadFile(path, &size);
    msx2->setup(pri, sec, idx, data, size, label);
    return data;
}

int main()
{
    MSX2* msx2 = new MSX2(0);
    msx2->setupSecondaryExist(false, false, false, true);
    void* main = init(msx2, 0, 0, 0, "../../msx2-osx/bios/cbios_main_msx2+_jp.rom", "MAIN");
    void* logo = init(msx2, 0, 0, 4, "../../msx2-osx/bios/cbios_logo_msx2+.rom", "LOGO");
    void* sub = init(msx2, 3, 0, 0, "../../msx2-osx/bios/cbios_sub.rom", "SUB");
    msx2->setupRAM(3, 3);

    // execute 3600 frames (1minute)
    auto start = std::chrono::system_clock::now();
    void* data;
    size_t size;
    for (int i = 0; i < 3600; i++) {
        msx2->tick(0, 0, 0);
        data = msx2->getSound(&size); // clear sound buffer
    }
    auto end = std::chrono::system_clock::now();
    auto time = end - start;
    int msec = std::chrono::duration_cast<std::chrono::milliseconds>(time).count();
    printf("Total time: %dms\n", msec);
    printf("Frame average: %fms\n", msec / 3600.0);
    printf("Frame usage: %.2f%%\n", (msec / 3600.0) / (1000.0 / 60.0) * 100.0);

    delete msx2;
    free(main);
    free(logo);
    free(sub);
    return 0;
}