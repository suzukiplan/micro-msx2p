#include "msx2.hpp"
#include <M5Core2.h>
#include <map>

static MSX2* msx2;

static struct Roms {
    uint8_t* main;
    uint8_t* logo;
    uint8_t* sub;
    uint8_t* game;
    int mainSize;
    int logoSize;
    int subSize;
    int gameSize;
} roms;

static void putlog(const char* format, ...)
{
    char buf[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    Serial.println(buf);
    M5.Lcd.println(buf); // いちいちscreenコマンドでチェックするのが面倒なので暫定的にLCDにデバッグ表示しておく
}

static uint8_t* readRom(const char* path, int* size)
{
    File file = SPIFFS.open(path);
    if (!file) return nullptr;
    *size = file.available();
    putlog("Read %s (%d bytes)", path, *size);
    uint8_t* result = (uint8_t*)malloc(*size);
    if (!result) return nullptr;
    for (int i = 0; i < *size; i++) {
        result[i] = file.read() & 0xFF;
    }
    return result;    
}

void setup() {
    M5.begin();
    SPIFFS.begin();
    Serial.begin(115200);
    msx2 = new MSX2(MSX2_COLOR_MODE_RGB555);
    roms.main = readRom("/cbios_main_msx2+_jp.rom", &roms.mainSize);
    roms.logo = readRom("/cbios_logo_msx2+.rom", &roms.logoSize);
    roms.sub = readRom("/cbios_sub.rom", &roms.subSize);
    roms.game = readRom("/game.rom", &roms.gameSize);
}

void loop() {
}
