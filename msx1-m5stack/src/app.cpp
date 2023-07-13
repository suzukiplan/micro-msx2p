#include "msx1.hpp"
#include <M5Core2.h>
#include <M5GFX.h>
#include "roms.hpp"

class CustomCanvas : public lgfx::LGFX_Sprite {
  public:
    CustomCanvas() : LGFX_Sprite() {}
    CustomCanvas(LovyanGFX* parent) : LGFX_Sprite(parent) { _psram = false; }
    void* frameBuffer(uint8_t) { return getBuffer(); }
};

static uint8_t ram[0x4000];
static TMS9918A::Context vram;
static bool booted;
static M5GFX gfx;
static CustomCanvas canvas(&gfx);
static uint16_t displayBuffer[256];
static xSemaphoreHandle displayMutex;
static unsigned short backdropColor;
static int fps;
static MSX1 msx1(TMS9918A::ColorMode::RGB565_Swap, ram, sizeof(ram), &vram, [](void* arg, int frame, int lineNumber, uint16_t* display) {
    if (0 == lineNumber) {
        xSemaphoreTake(displayMutex, portMAX_DELAY);
    }
    canvas.pushImage(0, lineNumber, 256, 1, display);
    if (191 == lineNumber) {
        backdropColor = ((MSX1*)arg)->getBackdropColor(true);
        xSemaphoreGive(displayMutex);
    }
});

static void displayMessage(const char* format, ...)
{
    char buf[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    gfx.startWrite();
    gfx.println(buf); // いちいちscreenコマンドでチェックするのが面倒なので暫定的にLCDにデバッグ表示しておく
    gfx.endWrite();
    vTaskDelay(100);
}

void ticker(void* arg)
{
    void* soundData;
    size_t soundSize;
    static long int min = 0x7FFFFFFF;
    static long int max = -1;
    static long start;
    static long procTime = 0;
    static const long interval[3] = { 33, 33, 34 };
    static int loopCount = 0;
    static long expect;
    while (1) {
        start = millis();
        msx1.tick(0, 0, 0); // even frame (rendering)
        soundData = msx1.getSound(&soundSize);
        msx1.tick(0, 0, 0); // odd frame (skip rendering)
        soundData = msx1.getSound(&soundSize);
        procTime = millis() - start;
        if (procTime < min) min = procTime;
        if (max < procTime) max = procTime;
        expect = interval[loopCount];
        if (procTime < expect) {
            fps = 60;
            delay(expect - procTime);
        } else {
            fps = 2000 / procTime;
        }
        loopCount++;
        loopCount %= 3;
    }
}

void renderer(void* arg)
{
    static uint16_t backdropPrev = 0;
    static char buf[80];
    static long start;
    static long procTime;
    static int renderFps;
    while (1) {
        start = millis();
        gfx.startWrite();
        if (backdropColor != backdropPrev) {
            backdropPrev = backdropColor;
            gfx.fillRect(0, 0, 320, 24, backdropPrev);
            gfx.fillRect(0, 216, 320, 24, backdropPrev);
            gfx.fillRect(0, 24, 32, 192, backdropPrev);
            gfx.fillRect(288, 24, 32, 192, backdropPrev);
        }
        gfx.setCursor(0, 0);
        sprintf(buf, "Emu: %d/60fps, Lcd: %d/30fps", fps, renderFps);
        gfx.print(buf);
        xSemaphoreTake(displayMutex, portMAX_DELAY);
        canvas.pushSprite(32, 24);
        xSemaphoreGive(displayMutex);
        gfx.endWrite();
        procTime = millis() - start;
        if (procTime < 33) {
            delay(33 - procTime);
            renderFps = 1000 / 33;
        } else {
            renderFps = 1000 / procTime;
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
    displayMutex = xSemaphoreCreateMutex();
    displayMessage("Checking memory usage before launch MSX...");
    displayMessage("- HEAP: %d", esp_get_free_heap_size());
    displayMessage("- MALLOC_CAP_EXEC: %d", heap_caps_get_free_size(MALLOC_CAP_EXEC));
    displayMessage("- MALLOC_CAP_DMA-L: %d", heap_caps_get_largest_free_block(MALLOC_CAP_DMA));
    displayMessage("- MALLOC_CAP_INTERNAL: %d", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    displayMessage("Loading micro MSX2+ (MSX1-core) for M5Stack...");
    msx1.vdp.useOwnDisplayBuffer(displayBuffer, sizeof(displayBuffer));
    msx1.setup(0, 0, (void*)rom_cbios_main_msx1, sizeof(rom_cbios_main_msx1), "MAIN");
    msx1.setup(0, 4, (void*)rom_cbios_logo_msx1, sizeof(rom_cbios_logo_msx1), "LOGO");
    msx1.loadRom((void*)rom_game, sizeof(rom_game), MSX1_ROM_TYPE_NORMAL);
    displayMessage("Setup finished.");
    booted = true;
    usleep(1000000);
    gfx.clear();
    disableCore0WDT();
    xTaskCreatePinnedToCore(ticker, "ticker", 4096, nullptr, 25, nullptr, 0);
    xTaskCreatePinnedToCore(renderer, "renderer", 4096, nullptr, 25, nullptr, 1);
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
