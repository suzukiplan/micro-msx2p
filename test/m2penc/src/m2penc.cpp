#include <stdio.h>
#include <string>
#include <fstream>
#include <vector>
#include <map>
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
    biosTable["empty"] = (unsigned char*)malloc(0x4000);
    MSX2 msx2(0);
    parse(&msx2, "settings.json");

    return 0;
}