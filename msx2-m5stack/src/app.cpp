#include <mutex>
#include "msx2.hpp"
#include <M5Core2.h>

static MSX2* msx2;
static std::mutex mutex;
static bool booted;
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

static void bootMessage(const char* format, ...)
{
    char buf[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    Serial.println(buf);
    if (!booted) {
        M5.Lcd.println(buf); // いちいちscreenコマンドでチェックするのが面倒なので暫定的にLCDにデバッグ表示しておく
    }
}

static uint8_t* readRom(const char* path, int* size)
{
    File file = SPIFFS.open(path);
    if (!file) return nullptr;
    *size = file.available();
    bootMessage("Read %s (%d bytes)", path, *size);
    uint8_t* result = (uint8_t*)malloc(*size);
    if (!result) return nullptr;
    for (int i = 0; i < *size; i++) {
        result[i] = file.read() & 0xFF;
    }
    return result;    
}

void ticker(void* arg)
{
    unsigned long start;
    int procTime;
    void* soundData;
    size_t soundSize;
    while (1) {
        start = millis();
        mutex.lock();
        msx2->tick(0, 0, 0);
        mutex.unlock();
        soundData = msx2->getSound(&soundSize);
        procTime = (int)(millis() - start);
        bootMessage("%d", procTime);
        vTaskDelay(10);
    }
}

void setup() {
    M5.begin();
    SPIFFS.begin();
    Serial.begin(115200);
    bootMessage("Loading micro MSX2+ for M5Stack...");
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
    bootMessage("Setup finished.");
    booted = true;
    usleep(1000000);
    M5.Lcd.clear(0);
    xTaskCreatePinnedToCore(ticker, "ticker", 4096, nullptr, 16, nullptr, 1);
}

void loop() {
    static const int m5w = M5.Lcd.width();
    static const int m5h = M5.Lcd.height();
    static const int m2w = msx2->getDisplayWidth();
    static const int m2h = msx2->getDisplayHeight();
    static const int cx = (m5w - m2w) / 2;
    static const int cy = (m5h - m2h) / 2;
    static const size_t displaySize = m2w * m2h * 2;
    static uint16_t* display = nullptr;
    if (!display) {
        display = (uint16_t*)malloc(displaySize);
    }
    mutex.lock();
    memcpy(display, msx2->getDisplay(), displaySize);
    mutex.unlock();
    M5.Lcd.drawBitmap(cx, cy, m2w, m2h, display);
}
