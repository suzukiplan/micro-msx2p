#include "msx1.hpp"
#include <M5Core2.h>
#include <M5GFX.h>

static MSX1* msx1;
static uint8_t ram[0x4000];
static TMS9918A::Context vram;
static bool booted;
static struct Roms {
    uint8_t* main;
    uint8_t* logo;
    uint8_t* game;
    int mainSize;
    int logoSize;
    int gameSize;
} roms;
static M5GFX gfx;
static M5Canvas canvas(&gfx);
static uint16_t displayBuffer[256];
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
        gfx.startWrite();
        gfx.println(buf); // いちいちscreenコマンドでチェックするのが面倒なので暫定的にLCDにデバッグ表示しておく
        gfx.endWrite();
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
    const uint16_t m5w = gfx.width();
    const uint16_t m5h = gfx.height();
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
            gfx.startWrite();
            if (backdropColor != backdropPrev) {
                backdropPrev = backdropColor;
                gfx.fillRect(0, 0, cx, m5h, backdropPrev);
                gfx.fillRect(cx + m1w, 0, cx, m5h, backdropPrev);
                gfx.fillRect(cx, 0, m1w, cy, backdropPrev);
                gfx.fillRect(cx, cy + m1h, m1w, cy, backdropPrev);
            }
            canvas.pushSprite(cx, cy);
            gfx.endWrite();
        }
    }
}

void setup() {
    M5.begin();
    gfx.begin();
    gfx.setColorDepth(16);
    gfx.fillScreen(TFT_BLACK);
    canvas.setColorDepth(16);
    canvas.createSprite(256, 192);
    SPIFFS.begin();
    Serial.begin(115200);
    bootMessage("Loading micro MSX2+ (using MSX1 core) for M5Stack...");
    msx1 = new MSX1(MSX1::ColorMode::RGB565, ram, sizeof(ram), &vram, [](void* arg, int frame, int lineNumber, uint16_t* display) {
        if (0 == (frame & 1)) {
            canvas.pushImage(0, lineNumber, 256, 1, display);
            if (191 == lineNumber) {
                backdropColor = ((MSX1*)arg)->getBackdropColor();
                pauseRenderer = false;
            }
        }
    });
    msx1->vdp->useOwnDisplayBuffer(displayBuffer, sizeof(displayBuffer));
    roms.main = readRom("/cbios_main_msx1.rom", &roms.mainSize);
    roms.logo = readRom("/cbios_logo_msx1.rom", &roms.logoSize);
    msx1->setup(0, 0, roms.main, roms.mainSize, "MAIN");
    msx1->setup(0, 4, roms.logo, roms.logoSize, "LOGO");
    roms.game = readRom("/game.rom", &roms.gameSize);
    msx1->loadRom(roms.game, roms.gameSize, MSX1_ROM_TYPE_NORMAL);
    bootMessage("Setup finished.");
    booted = true;
    usleep(1000000);
    gfx.clear();
    xTaskCreatePinnedToCore(ticker, "ticker", 4096, nullptr, 25, nullptr, 1);
    xTaskCreatePinnedToCore(renderer, "renderer", 4096, nullptr, 25, nullptr, 0);
}

void loop() {
    // ここでは、
    // - システムボタンを使ってホットキー操作
    //   - 左ボタン: ホットキーメニュー
    //     - リセット
    //     - 音量調整
    //     - セーブスロット選択 (1, 2, 3)
    //     - 中ボタンの割当変更 (Reset, Save, Load)
    //     - 右ボタンの割当変更 (Reset, Save, Load)
    //   - 中ボタン: クイックロード
    //   - 右ボタン: クイックセーブ
    // - ゲームパッドの入力をエミュレータに渡す
    // あたりを実装予定
}
