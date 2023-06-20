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
#include "../../../msx2-osx/core/msx2.hpp"

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
    } opt;
    memset(&opt, 0, sizeof(opt));
    opt.settings = "settings.json";
    opt.workdir = ".m2penc";
    bool error = false;
    for (int i = 1; !error && i < argc; i++) {
        if ('-' == argv[i][0]) {
            switch (tolower(argv[i][1])) {
                case 's': opt.settings = argv[i]; break;
                case 'o': strcpy(opt.output, argv[i]); break;
                case 'w': opt.workdir = argv[i]; break;
                default: error = true;
            }
        } else {
            if (!opt.input.empty()) {
                error = true;
            } else {
                opt.input = argv[i];
            }
        }
    }
    if (error || !opt.input.empty()) {
        puts("usage: m2penc [-o /path/to/output.avi]");
        puts("              [-s /path/to/settings.json]");
        puts("              [-w /path/to/workdir]");
        puts("              /path/to/playlog.m2p");
        return -1;
    }
    if (0 == opt.output[0]) {
        strcpy(opt.output, opt.input.c_str());
        char* cp = strrchr(opt.output, '.');
        if (cp) *cp = 0;
        strcat(opt.output, ".avi");
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
    if (pd.save) {
        puts("load state");
        msx2.quickLoad(pd.save, pd.saveSize);
    } else {
        msx2.reset();
    }


    return 0;
}