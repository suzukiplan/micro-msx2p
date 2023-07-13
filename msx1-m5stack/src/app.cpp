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
    canvas.pushImage(0, lineNumber, 256, 1, display);
    if (191 == lineNumber) {
        backdropColor = ((MSX1*)arg)->getBackdropColor(true);
    }
}, [](void* arg, void* buffer, size_t size) {
    i2s_write(I2S_NUM_0, buffer, size, &size, portMAX_DELAY);
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
    static void* soundData;
    static size_t soundSize;
    static long start;
    static long procTime = 0;
    static const long interval[3] = { 16, 17, 17 };
    static int loopCount = 0;
    static int fpsCounter;
    static long sec = 0;
    while (1) {
        start = millis();
        // calc fps
        if (sec != start / 1000) {
            sec = start / 1000;
            fps = fpsCounter;
            fpsCounter = 0;
        }

        // execute even frame (rendering display buffer)
        xSemaphoreTake(displayMutex, portMAX_DELAY);
        msx1.tick(0, 0, 0);
        xSemaphoreGive(displayMutex);
        fpsCounter++;

        // wait
        procTime = millis() - start;
        if (procTime < interval[loopCount]) {
            ets_delay_us((interval[loopCount] - procTime) * 1000);
        }
        loopCount++;
        loopCount %= 3;

        // execute odd frame (skip rendering)
        start = millis();
        msx1.tick(0, 0, 0); 
        fpsCounter++;

        // wait
        procTime = millis() - start;
        if (procTime < interval[loopCount]) {
            ets_delay_us((interval[loopCount] - procTime) * 1000);
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
        vTaskDelay(3);
        gfx.startWrite();
        if (backdropColor != backdropPrev) {
            backdropPrev = backdropColor;
            gfx.fillRect(0, 0, 320, 24, backdropPrev);
            gfx.fillRect(0, 216, 320, 24, backdropPrev);
            gfx.fillRect(0, 24, 32, 192, backdropPrev);
            gfx.fillRect(288, 24, 32, 192, backdropPrev);
        }
        gfx.setCursor(0, 0);
        sprintf(buf, "EMU:%d/60fps  LCD:%d/30fps ", fps, renderFps);
        gfx.print(buf);
        xSemaphoreTake(displayMutex, portMAX_DELAY);
        canvas.pushSprite(32, 24);
        xSemaphoreGive(displayMutex);
        gfx.endWrite();
        procTime = millis() - start;
        if (procTime < 33) {
            ets_delay_us((33 - procTime) * 1000);
        }
        renderFps = 1000 / (millis() - start);
    }
}

void setup() {
    i2s_config_t audioConfig = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = I2S_COMM_FORMAT_STAND_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 1024,
        .use_apll = false,
        .tx_desc_auto_clear = true
    };
    i2s_driver_install(I2S_NUM_0, &audioConfig, 0, nullptr);
    i2s_set_dac_mode(I2S_DAC_CHANNEL_RIGHT_EN);
    i2s_set_clk(I2S_NUM_0, 44100, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
    i2s_zero_dma_buffer(I2S_NUM_0);
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
