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
static bool pauseRenderer;
static unsigned short backdropColor;
static int fps;
static MSX1 msx1(TMS9918A::ColorMode::RGB565_Swap, ram, sizeof(ram), &vram, [](void* arg, int frame, int lineNumber, uint16_t* display) {
    if (0 == (frame & 1)) {
        canvas.pushImage(0, lineNumber, 256, 1, display);
        if (191 == lineNumber) {
            backdropColor = ((MSX1*)arg)->getBackdropColor(true);
            pauseRenderer = false;
        }
    }
});

static void putlog(const char* format, ...)
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
            fps = 30;
            vTaskDelay(expect - procTime);
        } else {
            fps = 1000 / (procTime + 1);
            vTaskDelay(1);
        }
        loopCount++;
        loopCount %= 3;
    }
}

void renderer(void* arg)
{
    uint16_t backdropPrev = 0;
    pauseRenderer = true;
    static long start;
    static long renderTime = 0;
    static const long interval[3] = { 33, 33, 34 };
    static int loopCount = 0;
    static long expect;
    static char buf[80];
    while (1) {
        while (pauseRenderer) {
            vTaskDelay(1);
        }
        start = millis();
        pauseRenderer = true;
        gfx.startWrite();
        if (backdropColor != backdropPrev) {
            backdropPrev = backdropColor;
            gfx.fillRect(0, 0, 320, 24, backdropPrev);
            gfx.fillRect(0, 216, 320, 24, backdropPrev);
            gfx.fillRect(0, 24, 32, 192, backdropPrev);
            gfx.fillRect(288, 24, 32, 192, backdropPrev);
        }
        gfx.setCursor(0, 0);
        sprintf(buf, "%dfps", fps);
        gfx.print(buf);
        canvas.pushSprite(32, 24);
        gfx.endWrite();
        renderTime = millis() - start;
        expect = interval[loopCount];
        if (renderTime < expect) {
            vTaskDelay(expect - renderTime);
        } else {
            vTaskDelay(1);
        }
        loopCount++;
        loopCount %= 3;
    }
}

void setup() {
    M5.begin();
    gfx.begin();
    gfx.setColorDepth(16);
    gfx.fillScreen(TFT_BLACK);
    canvas.setColorDepth(16);
    canvas.createSprite(256, 192);
    Serial.begin(115200);
    putlog("Checking memory usage before launch MSX...");
    putlog("- HEAP: %d", esp_get_free_heap_size());
    putlog("- MALLOC_CAP_EXEC: %d", heap_caps_get_free_size(MALLOC_CAP_EXEC));
    putlog("- MALLOC_CAP_DMA-L: %d", heap_caps_get_largest_free_block(MALLOC_CAP_DMA));
    putlog("- MALLOC_CAP_INTERNAL: %d", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    putlog("Loading micro MSX2+ (MSX1-core) for M5Stack...");
    msx1.vdp.useOwnDisplayBuffer(displayBuffer, sizeof(displayBuffer));
    msx1.setup(0, 0, (void*)rom_cbios_main_msx1, sizeof(rom_cbios_main_msx1), "MAIN");
    msx1.setup(0, 4, (void*)rom_cbios_logo_msx1, sizeof(rom_cbios_logo_msx1), "LOGO");
    msx1.loadRom((void*)rom_game, sizeof(rom_game), MSX1_ROM_TYPE_NORMAL);
    putlog("Setup finished.");
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
