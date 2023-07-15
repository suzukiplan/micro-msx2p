#include <map>
#include <vector>
#include <string>
#include "msx1.hpp"
#include "ay8910.hpp"
#include <M5Core2.h>
#include <M5GFX.h>
#include "roms.hpp"
#include "esp_freertos_hooks.h"

#if defined(CONFIG_ESP32_DEFAULT_CPU_FREQ_240)
static const uint64_t MaxIdleCalls = 1855000;
#elif defined(CONFIG_ESP32_DEFAULT_CPU_FREQ_160)
static const uint64_t MaxIdleCalls = 1233100;
#else
#error "Unsupported CPU frequency"
#endif

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
static int cpu0;
static int cpu1;
static uint64_t idle0 = 0;
static uint64_t idle1 = 0;
static AY8910 psg;
static bool pauseRequest;
static bool tickerPaused;
static bool psgTickerPaused;
static bool rendererPaused;

enum class GameState {
    None,
    Playing,
    Menu,
};

enum class ButtonPosition {
    Left,
    Center,
    Right
};

static std::map<ButtonPosition, Button*> buttons = {
    { ButtonPosition::Left, &M5.BtnA },
    { ButtonPosition::Center, &M5.BtnB },
    { ButtonPosition::Right, &M5.BtnC },
};

enum class MenuItem {
    Resume,
    Reset,
    SoundVolume,
    SelectSlot,
    Save,
    Load,
    Licenses,
};

static std::map<MenuItem, std::string> menuName = {
    { MenuItem::Resume, "Resume" },
    { MenuItem::Reset, "Reset" },
    { MenuItem::SoundVolume, "Sound Volume  MUTE LOW  MID  HIGH" },
    { MenuItem::SelectSlot,  "Select Slot    #1   #2   #3" },
    { MenuItem::Save, "Save" },
    { MenuItem::Load, "Load" },
    { MenuItem::Licenses, "Licenses" },
};

static std::map<MenuItem, std::string> menuDesc = {
    { MenuItem::Resume, "Nothing to do, back in play." },
    { MenuItem::Reset, "Reset the MSX." },
    { MenuItem::SoundVolume, "Adjusts the volume level." },
    { MenuItem::SelectSlot, "Select slot number to save and load." },
    { MenuItem::Save, "Saves the state of play." },
    { MenuItem::Load, "Loads the state of play." },
    { MenuItem::Licenses, "Displays OSS license information in use." },
};

static const std::vector<MenuItem> menuItems = {
    MenuItem::Resume,
    MenuItem::Reset,
    MenuItem::SoundVolume,
    MenuItem::SelectSlot,
    MenuItem::Save,
    MenuItem::Load,
    MenuItem::Licenses,
};

static int menuCursor = 0;
static int menuDescIndex = 0;
static GameState gameState = GameState::None;

static IRAM_ATTR bool idleTask0()
{
	idle0++;
	return false;
}

static IRAM_ATTR bool idleTask1()
{
	idle1++;
	return false;
}

static DRAM_ATTR MSX1 msx1(TMS9918A::ColorMode::RGB565_Swap, ram, sizeof(ram), &vram, [](void* arg, int frame, int lineNumber, uint16_t* display) {
    canvas.pushImage(0, lineNumber, 256, 1, display);
    if (191 == lineNumber) {
        backdropColor = ((MSX1*)arg)->getBackdropColor(true);
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

static void log(const char* format, ...)
{
    char buf[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    Serial.println(buf); // いちいちscreenコマンドでチェックするのが面倒なので暫定的にLCDにデバッグ表示しておく
}

void IRAM_ATTR ticker(void* arg)
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
        if (pauseRequest) {
            tickerPaused = true;
            vTaskDelay(10);
            continue;
        } else {
            tickerPaused = false;
        }
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

void IRAM_ATTR psgTicker(void* arg)
{
    static uint8_t buf[1024];
    static size_t size;
    static int i;
    while (1) {
        if (pauseRequest) {
            psgTickerPaused = true;
            vTaskDelay(10);
            continue;
        } else {
            psgTickerPaused = false;
        }
        for (i = 0; i < 1024; i++) {
            buf[i] = psg.tick(81);
        }
        i2s_write(I2S_NUM_0, buf, sizeof(buf), &size, portMAX_DELAY);
        vTaskDelay(2);
    }
}

void renderer(void* arg)
{
    static uint16_t backdropPrev = 0;
    static char buf[80];
    static long start;
    static long procTime;
    static int renderFps;
    static int renderFpsCounter;
    static long sec;
    while (1) {
        start = millis();
        if (pauseRequest) {
            rendererPaused = true;
            vTaskDelay(10);
            continue;
        } else {
            rendererPaused = false;
        }
        if (sec != start / 1000) {
            sec = start / 1000;
            renderFps = renderFpsCounter;
            renderFpsCounter = 0;
        }
        gfx.startWrite();
        if (backdropColor != backdropPrev) {
            backdropPrev = backdropColor;
            gfx.fillRect(0, 8, 320, 16, backdropPrev);
            gfx.fillRect(0, 216, 320, 16, backdropPrev);
            gfx.fillRect(0, 24, 32, 192, backdropPrev);
            gfx.fillRect(288, 24, 32, 192, backdropPrev);
        }
        gfx.setCursor(0, 0);
        sprintf(buf, "EMU:%d/60fps  LCD:%d/30fps  C0:%d%%  C1:%d%% ", fps, renderFps, cpu0, cpu1);
        gfx.print(buf);
        xSemaphoreTake(displayMutex, portMAX_DELAY);
        canvas.pushSprite(32, 24);
        xSemaphoreGive(displayMutex);
        gfx.endWrite();
        renderFpsCounter++;
        procTime = millis() - start;
        if (procTime < 33) {
            ets_delay_us((33 - procTime) * 1000);
        }
    }
}

void cpuMonitor(void* arg)
{
	while (1) {
		float f0 = idle0;
		float f1 = idle1;
		idle0 = 0;
		idle1 = 0;
		cpu0 = (int)(100.f - f0 / MaxIdleCalls * 100.f);
		cpu1 = (int)(100.f - f1 / MaxIdleCalls * 100.f);
		vTaskDelay(1000);
	}
}

void pauseAllTasks()
{
    if (pauseRequest) {
        return;
    }
    pauseRequest = true;
    while (!tickerPaused || !psgTickerPaused || !rendererPaused) {
        vTaskDelay(5);
    }
}

void resumeToPlay()
{
    gfx.startWrite();
    gfx.clear();
    gfx.fillRect(0, 8, 320, 16, backdropColor);
    gfx.fillRect(0, 216, 320, 16, backdropColor);
    gfx.fillRect(0, 24, 32, 192, backdropColor);
    gfx.fillRect(288, 24, 32, 192, backdropColor);
    gfx.pushImage(0, 232, 320, 8, rom_guide_normal);
    gfx.endWrite();
    gameState = GameState::Playing;
    pauseRequest = false;
}

void setup() {
    i2s_config_t audioConfig = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_8BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = I2S_COMM_FORMAT_STAND_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 1024,
        .use_apll = false,
        .tx_desc_auto_clear = true
    };
    i2s_driver_install(I2S_NUM_0, &audioConfig, 0, nullptr);
    i2s_set_dac_mode(I2S_DAC_CHANNEL_RIGHT_EN);
    i2s_set_clk(I2S_NUM_0, 44100, I2S_BITS_PER_SAMPLE_8BIT, I2S_CHANNEL_MONO);
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
    msx1.psgDelegate.reset = []() { psg.reset(27); };
    msx1.psgDelegate.setPads = [](unsigned char pad1, unsigned char pad2) { psg.setPads(pad1, pad2); };
    msx1.psgDelegate.read = []() -> unsigned char { return psg.read(); };
    msx1.psgDelegate.getPad1 = []() -> unsigned char { return psg.getPad1(); };
    msx1.psgDelegate.getPad2 = []() -> unsigned char { return psg.getPad2(); };
    msx1.psgDelegate.latch = [](unsigned char value) { return psg.latch(value); };
    msx1.psgDelegate.write = [](unsigned char value) { return psg.write(value); };
    msx1.psgDelegate.getContext = []() -> const void* { return &psg.ctx; };
    msx1.psgDelegate.getContextSize = []() -> int { return (int)sizeof(psg.ctx); };
    msx1.psgDelegate.setContext = [](const void* context, int size) { memcpy(&psg.ctx, context, size); };
    psg.reset(27);
    displayMessage("Setup finished.");
    booted = true;
    usleep(1000000);
    gfx.clear();
    disableCore0WDT();
	esp_register_freertos_idle_hook_for_cpu(idleTask0, 0);
	esp_register_freertos_idle_hook_for_cpu(idleTask1, 1);
    xTaskCreatePinnedToCore(ticker, "ticker", 4096, nullptr, 25, nullptr, 0);
    xTaskCreatePinnedToCore(psgTicker, "psgTicker", 4096, nullptr, 25, nullptr, 1);
    xTaskCreatePinnedToCore(renderer, "renderer", 4096, nullptr, 25, nullptr, 1);
    xTaskCreatePinnedToCore(cpuMonitor, "cpuMonitor", 1024, nullptr, 1, nullptr, 1);
}

inline void renderMenu()
{
    gfx.startWrite();
    gfx.fillRect(0, 0, 320, 8, TFT_BLACK);
    gfx.fillRoundRect(32, 16, 256, 208, 4, TFT_BLACK);
    gfx.drawRoundRect(34, 18, 252, 204, 4, WHITE);
    int y = 32;
    for (MenuItem item : menuItems) {
        gfx.setCursor(64, y);
        gfx.print(menuName[item].c_str());
        y += 16;
    }
    gfx.pushImage(0, 232, 320, 8, rom_guide_menu);
    gfx.endWrite();
}

inline void menuLoop()
{
    if (buttons[ButtonPosition::Center]->wasPressed()) { // TODO: or gamepad B/A/START/SELECT
        switch (menuItems[menuCursor]) {
            case MenuItem::Resume:
                resumeToPlay();
                return;
            case MenuItem::Reset:
                msx1.reset();
                resumeToPlay();
                return;
            case MenuItem::SoundVolume:
                // TODO
                break;
            case MenuItem::SelectSlot:
                // TODO
                break;
            case MenuItem::Save:
                // TODO
                break;
            case MenuItem::Load:
                // TODO
                break;
            case MenuItem::Licenses:
                // TODO
                break;
        }
    } else if (buttons[ButtonPosition::Left]->wasPressed()) { // TODO: or gamepad dpad:down
        menuCursor++;
        menuCursor %= (int)menuItems.size();
    } else if (buttons[ButtonPosition::Right]->wasPressed()) { // TODO: or gamepad dpad:up
        menuCursor--;
        if (menuCursor < 0) {
            menuCursor = (int)menuItems.size() - 1;
        }
    }
    gfx.startWrite();
    for (int i = 0; i < menuItems.size(); i++) {
        if (i == menuCursor) {
            gfx.pushImage(52, 32 + i * 16, 8, 8, rom_menu_cursor);
        } else {
            gfx.fillRect(52, 32 + i * 16, 8, 8, TFT_BLACK);
        }
    }
    if (menuDescIndex != menuCursor) {
        menuDescIndex = menuCursor;
        gfx.fillRect(0, 0, 320, 8, TFT_BLACK);
        gfx.setCursor(0, 0);
        gfx.print(menuDesc[menuItems[menuDescIndex]].c_str());
    }
    gfx.endWrite();
}

inline void playingLoop()
{
    if (buttons[ButtonPosition::Left]->wasPressed()) {
        if (GameState::Playing == gameState) {
            pauseAllTasks();
            gameState = GameState::Menu;
            menuCursor = 0;
            menuDescIndex = -1;
            renderMenu();
        }
    } else if (buttons[ButtonPosition::Center]->wasPressed()) {
        // TODO: Save
    } else if (buttons[ButtonPosition::Right]->wasPressed()) {
        // TODO: Load
    }
}

void loop() {
    M5.update();
    switch (gameState) {
        case GameState::None: resumeToPlay(); break;
        case GameState::Playing: playingLoop(); break;
        case GameState::Menu: menuLoop(); break;
    }
    vTaskDelay(10);
}
