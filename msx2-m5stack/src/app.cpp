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
    putlog("Loading micro MSX2+ for M5Stack...");
    msx2 = new MSX2(MSX2_COLOR_MODE_RGB565);
    roms.main = readRom("/cbios_main_msx2+_jp.rom", &roms.mainSize);
    roms.logo = readRom("/cbios_logo_msx2+.rom", &roms.logoSize);
    roms.sub = readRom("/cbios_sub.rom", &roms.subSize);
    msx2->setupSecondaryExist(false, false, false, true);
    msx2->setup(0, 0, 0, roms.main, roms.mainSize, "MAIN");
    msx2->setup(0, 0, 4, roms.logo, roms.logoSize, "LOGO");
    msx2->setup(3, 0, 0, roms.sub, roms.subSize, "SUB");
    msx2->setupRAM(3, 3);
    roms.game = readRom("/game.rom", &roms.gameSize);
    msx2->loadRom(roms.game, roms.gameSize, MSX2_ROM_TYPE_NORMAL);
    putlog("Setup finished.");
    vTaskDelay(1000 / portTICK_RATE_MS);
    M5.Lcd.clear(0);
}

void loop() {
    msx2->tick(0, 0, 0);
    M5.Lcd.drawBitmap(
        (M5.Lcd.width() - msx2->getDisplayWidth()) / 2,
        (M5.Lcd.height() - msx2->getDisplayHeight()) / 2,
        msx2->getDisplayWidth(),
        msx2->getDisplayHeight(),
        msx2->getDisplay());
}
