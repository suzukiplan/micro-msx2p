/**
 * Performance Tester with Google Benchmark
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
#include "benchmark/benchmark.h"
#include "msx1.hpp"

static unsigned char ram[0x2000];
static MSX1* msx1;
static void* mainRom;
static void* logoRom;
static void* gameRom;

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

void* init(int pri, int idx, const char* path, const char* label)
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

static void MSX1Init(benchmark::State& state)
{
    for (auto _ : state) {
        if (!msx1) {
            msx1 = new MSX1(TMS9918A::ColorMode::RGB555, ram, sizeof(ram), nullptr);
            mainRom = init(0, 0, "cbios_main_msx1.rom", "MAIN");
            logoRom = init(0, 4, "cbios_logo_msx1.rom", "LOGO");
            gameRom = init(0, 4, "game.rom", "CART");
        }
    }
}

static void MSX1Execute1Tick(benchmark::State& state)
{
    for (auto _ : state) {
        msx1->tick(0, 0, 0);
    }
}

static void MSX1Execute60Ticks(benchmark::State& state)
{
    for (auto _ : state) {
        for (int i = 0; i < 60; i++) {
            msx1->tick(0, 0, 0);
        }
    }
}

static void MSX1Term(benchmark::State& state)
{
    for (auto _ : state) {
        if (msx1) {
            delete msx1;
            msx1 = nullptr;
            free(gameRom);
            free(logoRom);
            free(mainRom);
        }
    }
}

BENCHMARK(MSX1Init);
BENCHMARK(MSX1Execute1Tick);
BENCHMARK(MSX1Execute60Ticks);
BENCHMARK(MSX1Term);
BENCHMARK_MAIN();
