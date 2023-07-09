#include "msx1.hpp"
#include <M5Core2.h>

static MSX1* msx1;
static bool booted;
static struct Roms {
    uint8_t* main;
    uint8_t* logo;
    uint8_t* game;
    int mainSize;
    int logoSize;
    int gameSize;
} roms;
static uint16_t bitmap[256 * 192];
static bool pauseRenderer;
static unsigned short backdropColor;

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
        msx1->tick(0, 0, 0);
        soundData = msx1->getSound(&soundSize);
        procTime = (int)(millis() - start);
        bootMessage("%d (free-heap: %d)", procTime, esp_get_free_heap_size());
        vTaskDelay(10);
    }
}

void renderer(void* arg)
{
    const uint16_t m5w = M5.Lcd.width();
    const uint16_t m5h = M5.Lcd.height();
    const uint16_t m1w = (uint16_t)msx1->getDisplayWidth();
    const uint16_t m1h = (uint16_t)msx1->getDisplayHeight();
    const uint16_t cx = (uint16_t)((m5w - m1w) / 2);
    const uint16_t cy = (uint16_t)((m5h - m1h) / 2);
    uint16_t backdropPrev = 0;
    pauseRenderer = true;
    while (1) {
        if (pauseRenderer) {
            vTaskDelay(100);
        } else {
            pauseRenderer = true;
            M5.Lcd.startWrite();
            if (backdropColor != backdropPrev) {
                backdropPrev = backdropColor;
                M5.Lcd.fillRect(0, 0, cx, m5h, backdropPrev);
                M5.Lcd.fillRect(cx + m1w, 0, cx, m5h, backdropPrev);
                M5.Lcd.fillRect(cx, 0, m1w, cy, backdropPrev);
                M5.Lcd.fillRect(cx, cy + m1h, m1w, cy, backdropPrev);
            }
            M5.Lcd.drawBitmap(cx, cy, m1w, m1h, bitmap);
            M5.Lcd.endWrite();
        }
    }
}

void setup() {
    M5.begin();
    SPIFFS.begin();
    Serial.begin(115200);
    bootMessage("Loading micro msx1+ for M5Stack...");
    msx1 = new MSX1(MSX1_COLOR_MODE_RGB565, [](void* arg, int frame, int lineNumber, uint16_t* display) {
        if (0 == (frame & 1)) {
            memcpy(&bitmap[lineNumber * 256], display, 512);
            if (191 == lineNumber) {
                backdropColor = ((MSX1*)arg)->getBackdropColor();
                pauseRenderer = false;
            }
        }
    });
    roms.main = readRom("/cbios_main_msx1.rom", &roms.mainSize);
    roms.logo = readRom("/cbios_logo_msx1.rom", &roms.logoSize);
    msx1->setup(0, 0, roms.main, roms.mainSize, "MAIN");
    msx1->setup(0, 4, roms.logo, roms.logoSize, "LOGO");
    roms.game = readRom("/game.rom", &roms.gameSize);
    msx1->loadRom(roms.game, roms.gameSize, MSX1_ROM_TYPE_NORMAL);
    bootMessage("Setup finished.");
    booted = true;
    usleep(1000000);
    M5.Lcd.clear(0);
    xTaskCreatePinnedToCore(ticker, "ticker", 4096, nullptr, 25, nullptr, 1);
    xTaskCreatePinnedToCore(renderer, "renderer", 4096, nullptr, 25, nullptr, 0);
}

void loop() {
}
