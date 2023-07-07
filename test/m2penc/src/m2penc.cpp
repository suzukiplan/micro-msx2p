/**
 * Playlog to MP4 movie encoder
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
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <string>
#include <fstream>
#include <vector>
#include <map>
#include <sys/stat.h>
#include "json/json.hpp"
#include "pngwriter/pngwriter.h"
#include "../../../src/msx2.hpp"

static std::map<std::string, unsigned char*> biosTable;

static unsigned char* loadBinaryFile(const char* path, size_t* sizeResult = nullptr)
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
    unsigned char* buf = (unsigned char*)malloc(size);
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

static void parsePages(MSX2* msx2, int slot, int extra, const nlohmann::json& pages)
{
    int pageIndex = 0;
    for (auto page : pages) {
        if (page.find("label") != page.end()) {
            std::string data = page.find("data") != page.end() ? page["data"].get<std::string>() : "empty";
            int offset = page.find("offset") != page.end() ? page["offset"].get<int>() : 0;
            std::string label = page["label"].get<std::string>();
            printf("Setup Slot%d-%d Page#%d = %s <%s>\n", slot, extra, pageIndex, label.c_str(), data.c_str());
            if (biosTable.find(data) == biosTable.end()) {
                biosTable[data] = loadBinaryFile(data.c_str());
                if (!biosTable[data]) {
                    puts(("File not found: " + data).c_str());
                    exit(-1);
                }
            }
            msx2->setup(slot, extra, pageIndex * 2, biosTable[data] + offset * 0x4000, 0x4000, label.c_str());
        }
        pageIndex++;
    }
}

static void parse(MSX2* msx2, const char* path)
{
    std::ifstream i(path);
    nlohmann::json j = nlohmann::json::parse(i);

    auto font = j.find("font");
    if (font != j.end()) {
        auto fontPath = font->get<std::string>();
        size_t size;
        void* data = loadBinaryFile(fontPath.c_str(), &size);
        if (data) {
            msx2->loadFont(data, size);
            free(data);
            puts(("Font loaded: " + fontPath).c_str());
        } else {
            puts(("Cannot load font: " + fontPath).c_str());
        }
    } else {
        puts("Unuse font");
    }

    auto slots = j.find("slots");
    std::vector<bool> extra;
    if (slots != j.end()) {
        int slotIndex = 0;
        for (auto slot : j["slots"]) {
            extra.push_back(slot.find("extra") != slot.end() ? slot["extra"].get<bool>() : false);
            msx2->setupSecondaryExist(
                0 < extra.size() ? extra[0] : false,
                1 < extra.size() ? extra[1] : false,
                2 < extra.size() ? extra[2] : false,
                3 < extra.size() ? extra[3] : false
            );
            if (extra[slotIndex]) {
                int extraIndex = 0;
                for (auto extraPage : slot["extraPages"]) {
                    if (extraPage.find("ram") != extraPage.end()) {
                        if (extraPage["ram"].get<bool>()) {
                            msx2->setupRAM(slotIndex, extraIndex);
                        } else {
                            parsePages(msx2, slotIndex, extraIndex, extraPage["pages"]);
                        }
                    } else {
                        parsePages(msx2, slotIndex, extraIndex, extraPage["pages"]);
                    }
                    extraIndex++;
                }
            } else {
                if (slot.find("ram") != slot.end()) {
                    if (slot["ram"].get<bool>()) {
                        msx2->setupRAM(slotIndex, 0);
                    } else {
                        parsePages(msx2, slotIndex, 0, slot["pages"]);
                    }
                } else {
                    parsePages(msx2, slotIndex, 0, slot["pages"]);
                }
            }
            slotIndex++;
        }
    }
}

int main(int argc, char* argv[])
{
    // check ffmpeg conditions
    FILE* cmd = popen("ffmpeg -codecs 2>/dev/null", "r");
    if (!cmd) {
        puts("Cannot execute ffmpeg");
        return -1;
    }
    char buf[8192];
    bool libx264 = false;
    bool libfdk_aac = false;
    while (!feof(cmd)) {
        fgets(buf, sizeof(buf), cmd);
        if (!libx264 && strstr(buf, "libx264")) {
            libx264 = true;
        }
        if (!libfdk_aac && strstr(buf, "libfdk_aac")) {
            libfdk_aac = true;
        }
    }
    if (!libx264 || !libfdk_aac) {
        if (!libx264) puts("libx264 not installed in ffmpeg");
        if (!libfdk_aac) puts("libfdk_aac not installed in ffmpeg");
        return -1;
    } 

    // check command line options
    struct Options {
        std::string settings;
        char output[2048];
        std::string workdir;
        std::string input;
        int captureWidth;
        int captureHeight;
        int vidoeWidth;
        int videoHeight;
    } opt;
    memset(&opt, 0, sizeof(opt));
    opt.settings = "settings.json";
    opt.workdir = ".m2penc";
    opt.captureHeight = 480;
    opt.videoHeight = 480;
    bool error = false;
    for (int i = 1; !error && i < argc; i++) {
        if ('-' == argv[i][0]) {
            if (argc <= i + 1) {
                error = true;
                break;
            }
            switch (tolower(argv[i][1])) {
                case 's': opt.settings = argv[i + 1]; break;
                case 'o': strcpy(opt.output, argv[i + 1]); break;
                case 'w': opt.workdir = argv[i + 1]; break;
                case 'c': opt.captureHeight = atoi(argv[i + 1]); break;
                case 'x': opt.videoHeight = atoi(argv[i + 1]); break;
                default: error = true;
            }
            i++;
        } else {
            if (!opt.input.empty()) {
                error = true;
            } else {
                opt.input = argv[i];
            }
        }
    }
    if (0 == strcmp(opt.workdir.c_str(), "/")) {
        puts("!?");
        exit(-1);
    }
    if (!error) error = opt.captureHeight != 240 && opt.captureHeight != 480;
    if (!error) error = opt.videoHeight != 240 && opt.videoHeight != 480 && opt.videoHeight != 720 && opt.videoHeight != 960;
    if (!error) error = opt.input.empty();
    if (error) {
        puts("usage: m2penc [-o /path/to/output.mp4]");
        puts("              [-s /path/to/settings.json]");
        puts("              [-w /path/to/workdir]");
        puts("              [-c { 240 | 480 }] ............... capture height");
        puts("              [-x { 240 | 480 | 720 | 960 }] ... video height");
        puts("              /path/to/playlog.m2p");
        return -1;
    }
    opt.captureWidth = 284 * opt.captureHeight / 240;
    opt.vidoeWidth = 284 * opt.videoHeight / 240;
    if (0 == opt.output[0]) {
        strcpy(opt.output, opt.input.c_str());
        char* cp = strrchr(opt.output, '.');
        if (cp) *cp = 0;
        strcat(opt.output, ".mp4");
    }

    // cleanup workdir
    system(("rm -rf '" + opt.workdir + "'").c_str());
    mkdir(opt.workdir.c_str(), 0777);

    // init bios
    biosTable["empty"] = (unsigned char*)malloc(0x4000);
    memset(biosTable["empty"], 0, 0x4000);
    MSX2 msx2(0);
    parse(&msx2, opt.settings.c_str());

    // load playlog data
    size_t size;
    char* playlogUncompressed = (char*)loadBinaryFile(opt.input.c_str(), &size);
    if (!playlogUncompressed) {
        printf("Cannot load %s\n", opt.input.c_str());
        return -1;
    }
    char* playlog = (char*)malloc(1024 * 1024 * 16);
    if (!playlog) {
        puts("No memory");
        return -1;
    }
    int dsize = LZ4_decompress_safe(playlogUncompressed,
                                    playlog,
                                    (int)size,
                                    1024 * 1024 * 16);
    free(playlogUncompressed);

    // parse playlog data
    struct PlaylogData {
        char* disk1;
        size_t disk1Size;
        bool disk1ReadOnly;
        char* disk2;
        size_t disk2Size;
        bool disk2ReadOnly;
        char* rom;
        size_t romSize;
        int romType;
        char* save;
        size_t saveSize;
        unsigned int tickCount;
        char* t1;
        char* t2;
        char* tk;
    } pd;
    memset(&pd, 0, sizeof(pd));
    char* ptr = playlog;
    while (0 < dsize) {
        unsigned int uiSize;
        if (0 == memcmp(ptr, "D1", 2)) {
            ptr += 2;
            dsize -= 2;
            memcpy(&uiSize, ptr, 4);
            uiSize -= 1;
            ptr += 4;
            dsize -= 4;
            pd.disk1Size = (size_t)uiSize;
            pd.disk1ReadOnly = (*ptr) ? true : false;
            ptr++;
            dsize--;
            pd.disk1 = ptr;
            ptr += uiSize;
            dsize -= uiSize;
        } else if (0 == memcmp(ptr, "D2", 2)) {
            ptr += 2;
            dsize -= 2;
            memcpy(&uiSize, ptr, 4);
            uiSize -= 1;
            ptr += 4;
            dsize -= 4;
            pd.disk2Size = (size_t)uiSize;
            pd.disk2ReadOnly = (*ptr) ? true : false;
            ptr++;
            dsize--;
            pd.disk2 = ptr;
            ptr += uiSize;
            dsize -= uiSize;
        } else if (0 == memcmp(ptr, "RO", 2)) {
            ptr += 2;
            dsize -= 2;
            memcpy(&uiSize, ptr, 4);
            uiSize -= 4;
            ptr += 4;
            dsize -= 4;
            pd.romSize = (size_t)uiSize;
            memcpy(&pd.romType, ptr, 4);
            ptr += 4;
            dsize -= 4;
            pd.rom = ptr;
            ptr += uiSize;
            dsize -= uiSize;
        } else if (0 == memcmp(ptr, "SV", 2)) {
            ptr += 2;
            dsize -= 2;
            memcpy(&uiSize, ptr, 4);
            ptr += 4;
            dsize -= 4;
            pd.saveSize = (size_t)uiSize;
            pd.save = ptr;
            ptr += uiSize;
            dsize -= uiSize;
        } else if (0 == memcmp(ptr, "T1", 2)) {
            ptr += 2;
            dsize -= 2;
            memcpy(&uiSize, ptr, 4);
            ptr += 4;
            dsize -= 4;
            pd.tickCount = (int)uiSize;
            pd.t1 = ptr;
            ptr += uiSize;
            dsize -= uiSize;
        } else if (0 == memcmp(ptr, "T2", 2)) {
            ptr += 2;
            dsize -= 2;
            memcpy(&uiSize, ptr, 4);
            ptr += 4;
            dsize -= 4;
            pd.tickCount = (int)uiSize;
            pd.t2 = ptr;
            ptr += uiSize;
            dsize -= uiSize;
        } else if (0 == memcmp(ptr, "TK", 2)) {
            ptr += 2;
            dsize -= 2;
            memcpy(&uiSize, ptr, 4);
            ptr += 4;
            dsize -= 4;
            pd.tickCount = (int)uiSize;
            pd.tk = ptr;
            ptr += uiSize;
            dsize -= uiSize;
        }
    }
    if (!pd.save) {
        puts("Invalid playlog data");
        exit(-1);
    }
    if (pd.rom) {
        puts("insert ROM data");
        msx2.loadRom((void*)pd.rom, (int)pd.romSize, pd.romType);
    } else {
        if (pd.disk1) {
            puts("insert DISK1 data");
            msx2.insertDisk(0, pd.disk1, pd.disk1Size, pd.disk1ReadOnly);
        }
        if (pd.disk2) {
            puts("insert DISK2 data");
            msx2.insertDisk(0, pd.disk2, pd.disk2Size, pd.disk2ReadOnly);
        }
    }
    puts("load state");
    msx2.quickLoad(pd.save, pd.saveSize);

    std::string wavPath = opt.workdir + "/sound.wav";
    struct WavHeader {
        char riff[4];
        unsigned int fsize;
        char wave[4];
        char fmt[4];
        unsigned int bnum;
        unsigned short fid;
        unsigned short ch;
        unsigned int sample;
        unsigned int bps;
        unsigned short bsize;
        unsigned short bits;
        char data[4];
        unsigned int dsize;
    } wh;
    memset(&wh, 0, sizeof(wh));
    strncpy(wh.riff, "RIFF", 4);
    strncpy(wh.wave, "WAVE", 4);
    strncpy(wh.fmt, "fmt ", 4);
    strncpy(wh.data, "data", 4);
    wh.bnum = 16;
    wh.fid = 1;
    wh.ch = 2;
    wh.sample = 44100;
    wh.bsize = 2;
    wh.bits = 16;
    wh.bps = wh.sample * wh.ch * wh.bsize;
    wh.dsize = wh.bps / 60 * pd.tickCount;
    FILE* wav = fopen(wavPath.c_str(), "wb");
    fwrite(&wh, 1, sizeof(wh), wav);

    printf("Writing 0 of %d", pd.tickCount);
    for (int tick = 0; tick < pd.tickCount; tick++) {
        msx2.tick(pd.t1[tick], pd.t2[tick], pd.tk[tick]);
        // output screen as png
        unsigned short* display = msx2.getDisplay();
        char pngName[256];
        snprintf(pngName, sizeof(pngName), "/%08d.png", tick);
        pngwriter png(opt.captureWidth, opt.captureHeight, 0, (opt.workdir + pngName).c_str());
        for (int y = 0; y < 240; y++) {
            for (int x = 0; x < 568; x++) {
                unsigned short rgb555 = display[y * 568 + x];
                double r = ((rgb555 & 0b0111110000000000) >> 10) / 31.0;
                double g = ((rgb555 & 0b0000001111100000) >> 5) / 31.0;
                double b = (rgb555 & 0b0000000000011111) / 31.0;
                if (240 == opt.captureHeight) {
                    png.plot(x / 2, 240 - y, r, g, b);
                    x++;
                } else {
                    png.plot(x, 480 - y * 2, r, g, b);
                    png.plot(x, 480 - y * 2 + 1, r, g, b);
                }
            }
        }
        png.close();

        // TODO: output sound as wav
        size_t pcmSize;
        void* pcm = msx2.getSound(&pcmSize);
        fwrite(pcm, 1, pcmSize, wav);

        printf("\rWriting frame: %d of %d (%d%%)", 1 + tick, pd.tickCount, (1 + tick) * 100 / pd.tickCount);
        fflush(stdout);
    }
    printf(" ... done\n");
    fclose(wav);

    char vf[80];
    if (opt.captureHeight == opt.videoHeight) {
        vf[0] = 0;
    } else {
        snprintf(vf, sizeof(vf), " -vf scale=%d:%d", opt.vidoeWidth, opt.videoHeight);
    }
    std::string ffmpeg = "ffmpeg -y -r 60 -start_number 0 -i \"" + opt.workdir + "/%08d.png\" -i \"" + wavPath + "\" -acodec libfdk_aac -profile:a aac_he -afterburner 1 -vcodec libx264 -pix_fmt yuv420p -r 60" + vf + " \"" + opt.output + "\"";
    puts(ffmpeg.c_str());
    return system(ffmpeg.c_str());
}
