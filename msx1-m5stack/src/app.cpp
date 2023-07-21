// C++ stdlib
#include <map>
#include <vector>
#include <string>

// micro MSX2+
#include "msx1.hpp"
#include "ay8910.hpp"

// Arduino
#include <Arduino.h>
#include <Wire.h>
#include <SD.h>
#include <SPIFFS.h>
#include <driver/i2s.h>
#include <esp_freertos_hooks.h>
#include <esp_log.h>

// M5Stack
#include <M5Unified.h>
#include <M5GFX.h>

// ROM data (Graphics, BIOS and Game)
#include "roms.hpp"

#define APP_COPYRIGHT "Copyright (c) 20xx Team HogeHoge"
#define APP_PREFRENCE_FILE "/suzukiplan_micro-msx1.prf"
#define SAVE_SLOT_FORMAT "/game_slot%d.dat"

class CustomCanvas : public lgfx::LGFX_Sprite {
  public:
    CustomCanvas() : LGFX_Sprite() {}
    CustomCanvas(LovyanGFX* parent) : LGFX_Sprite(parent) { _psram = false; }
    void* frameBuffer(uint8_t) { return getBuffer(); }
};

class Audio {
#ifdef M5StackCore2
    // Simple Audio DAC implementation
  private:
    static constexpr i2s_port_t i2sNum = I2S_NUM_0;

  public:
    void begin() {
        i2s_config_t audioConfig = {
            .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
            .sample_rate = 44100,
            .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
            .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
            .communication_format = I2S_COMM_FORMAT_STAND_MSB,
            .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
            .dma_buf_count = 4,
            .dma_buf_len = 1024,
            .use_apll = false,
            .tx_desc_auto_clear = true
        };
        i2s_driver_install(this->i2sNum, &audioConfig, 0, nullptr);
        i2s_set_clk(this->i2sNum, 44100, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
        i2s_zero_dma_buffer(this->i2sNum);
    }

    inline void write(int16_t* buf, size_t bufSize) {
        size_t wrote;
        i2s_write(this->i2sNum, buf, bufSize, &wrote, portMAX_DELAY);
        vTaskDelay(2);
    }
#else
    // AW88298 Audio Amplifier implementation
  private:
    static constexpr uint8_t i2cAddrAw88298 = 0x36;
    static constexpr i2s_port_t i2sNum = I2S_NUM_1;

    inline void writeRegister(uint8_t reg, uint16_t value) {
        value = __builtin_bswap16(value);
        M5.In_I2C.writeRegister(this->i2cAddrAw88298, reg, (const uint8_t*)&value, 2, 400000);
    }

  public:
    void begin() {
        // setup AW88298 regisgter
        M5.In_I2C.bitOn(this->i2cAddrAw88298, 0x02, 0b00000100, 400000);
        this->writeRegister(0x61, 0x0673); // BSTCTL2 (same as M5Unified) 
        this->writeRegister(0x04, 0x4040); // SYSCTL (same as M5Unified)
        this->writeRegister(0x05, 0x0008); // SYSCTL2 (same as M5Unified)
        this->writeRegister(0x06, 0b0001110000000111); // I2SCTL: 44.1kHz, 16bits, monoral (他は全部デフォルト値)
        this->writeRegister(0x0C, 0x0064); // HAGCCFG (same as M5Unified)
        // I2S config
        i2s_config_t config;
        memset(&config, 0, sizeof(i2s_config_t));
        config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
        config.sample_rate = 48000; // dummy setting
        config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
        config.channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT;
        config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
        config.dma_buf_count = 4;
        config.dma_buf_len = 1024;
        config.tx_desc_auto_clear = true;
        // I2S pin config
        i2s_pin_config_t pinConfig;
        memset(&pinConfig, ~0u, sizeof(i2s_pin_config_t));
        pinConfig.bck_io_num = GPIO_NUM_34;
        pinConfig.ws_io_num = GPIO_NUM_33;
        pinConfig.data_out_num = GPIO_NUM_13;
        // Setup I2S
        if (ESP_OK != i2s_driver_install(this->i2sNum, &config, 0, nullptr)) {
            i2s_driver_uninstall(this->i2sNum);
            i2s_driver_install(this->i2sNum, &config, 0, nullptr);
        }
        i2s_set_pin(this->i2sNum, &pinConfig);
        i2s_zero_dma_buffer(this->i2sNum);
        i2s_start(this->i2sNum);
    }

    inline void write(int16_t* buf, size_t bufSize) {
        size_t wrote;
        i2s_write(this->i2sNum, buf, bufSize, &wrote, portMAX_DELAY);
        vTaskDelay(2);
    }
#endif
};

class Gamepad {
  private:
    bool enabled;
    const int i2cAddr = 0x08;
    uint8_t code;
    xSemaphoreHandle mutex;
#ifdef M5StackCoreS3
    const int sda = 12;
    const int scl = 11;
#else
    const int pinInt = 5;
    const int sda = 21;
    const int scl = 22;
#endif

  public:
    bool wasPushUp;
    bool wasPushDown;
    bool wasPushLeft;
    bool wasPushRight;
    bool wasPushA;
    bool wasPushB;
    bool wasPushStart;
    bool wasPushSelect;

    Gamepad() {
         this->code = 0;
         this->enabled = false;
         this->mutex = xSemaphoreCreateMutex();
         this->clearPush();
    }

    void begin() {
        Wire1.begin(this->sda, this->scl);
        Wire1.beginTransmission(0x08);
        this->enabled = 0 == Wire1.endTransmission();
#ifndef M5StackCoreS3
        if (this->enabled) pinMode(this->pinInt, INPUT_PULLUP);
#endif
    }

#ifdef M5StackCoreS3
    inline bool isReadble() { return true; }

    inline uint8_t read() {
        uint8_t result = Wire1.peek();
        Wire1.flush();
        return result;
    }
#else
    inline bool isReadble() { return digitalRead(this->pinInt) == LOW; }
    inline uint8_t read() { return (uint8_t)Wire1.read(); }
#endif
    inline bool isEnabled() { return this->enabled; }

    inline uint8_t get() {
        if (!this->enabled) {
            xSemaphoreTake(this->mutex, portMAX_DELAY);
            this->code = 0;
            xSemaphoreGive(this->mutex);
        } else if (this->isReadble()) {
            Wire1.requestFrom(i2cAddr, 1);
            if (Wire1.available()) {
                uint8_t key = this->read() ^ 0xFF;
                xSemaphoreTake(this->mutex, portMAX_DELAY);
                this->code = 0;
                if (key & 0b00000001) this->code |= MSX1_JOY_UP;
                if (key & 0b00000010) this->code |= MSX1_JOY_DW;
                if (key & 0b00000100) this->code |= MSX1_JOY_LE;
                if (key & 0b00001000) this->code |= MSX1_JOY_RI;
                if (key & 0b00010000) this->code |= MSX1_JOY_T1;
                if (key & 0b00100000) this->code |= MSX1_JOY_T2;
                if (key & 0b01000000) this->code |= MSX1_JOY_S2;
                if (key & 0b10000000) this->code |= MSX1_JOY_S1;
                xSemaphoreGive(this->mutex);
            }
        }
        return this->code;
    }

    inline uint8_t getExcludeHotkey() {
        uint8_t result = this->get();
        return result & MSX1_JOY_S2 && result ^ MSX1_JOY_S2 ? 0 : result;
    }

    inline uint8_t getLatest() {
        xSemaphoreTake(this->mutex, portMAX_DELAY);
        uint8_t result = this->code;
        xSemaphoreGive(this->mutex);
        return result;
    }

    inline void clearPush() {
         this->wasPushUp = false;
         this->wasPushDown = false;
         this->wasPushLeft = false;
         this->wasPushRight = false;
         this->wasPushA = false;
         this->wasPushB = false;
         this->wasPushStart = false;
         this->wasPushSelect = false;
    }

    inline void updatePush() {
        uint8_t prev = this->code;
        this->get();
        xSemaphoreTake(this->mutex, portMAX_DELAY);
        this->clearPush();
        if (0 != prev && 0 == this->code) {
            this->wasPushUp = prev & MSX1_JOY_UP ? true : false;
            this->wasPushDown = prev & MSX1_JOY_DW ? true : false;
            this->wasPushLeft = prev & MSX1_JOY_LE ? true : false;
            this->wasPushRight = prev & MSX1_JOY_RI ? true : false;
            this->wasPushA = prev & MSX1_JOY_T1 ? true : false;
            this->wasPushB = prev & MSX1_JOY_T2 ? true : false;
            this->wasPushStart = prev & MSX1_JOY_S1 ? true : false;
            this->wasPushSelect = prev & MSX1_JOY_S2 ? true : false;
        }
        xSemaphoreGive(this->mutex);
    }

    inline void resetCode() {
        this->get();
        xSemaphoreTake(this->mutex, portMAX_DELAY);
        this->code = 0;
        xSemaphoreGive(this->mutex);
    }
};

class Preferences {
  private:
    const int DEFAULT_SOUND = 2;
    const int DEFAULT_SLOT = 0;
    const int DEFAULT_ROTATE = 0;
    const int DEFAULT_SLOT_LOCATION = 0;

  public:
    int sound;
    int slot;
    int rotate;
    int slotLocation;

    Preferences() {
        this->factoryReset();
    }

    void factoryReset() {
        this->sound = DEFAULT_SOUND;
        this->slot = DEFAULT_SLOT;
        this->rotate = DEFAULT_ROTATE;
        this->slotLocation = DEFAULT_SLOT_LOCATION;
    }

    void load() {
        this->factoryReset();
        SPIFFS.begin();
        File file = SPIFFS.open(APP_PREFRENCE_FILE, "r");
        if (!file) {
            ESP_LOGI("MSX1", "SPIFFS: preference not found");
            SPIFFS.end();
            return;
        }
        if (file.available()) {
            this->sound = file.read();
            if (this->sound < 0 || 3 < this->sound) {
                this->sound = DEFAULT_SOUND;
            }
            ESP_LOGI("MSX1", "SPIFFS: Loaded sound=%d", this->sound);
        }
        if (file.available()) {
            this->slot = file.read();
            if (this->slot < 0 || 2 < this->slot) {
                this->slot = DEFAULT_SLOT;
            }
            ESP_LOGI("MSX1", "SPIFFS: Loaded slot=%d", this->sound);
        }
        if (file.available()) {
            this->rotate = file.read();
            if (this->rotate < 0 || 1 < this->rotate) {
                this->rotate = DEFAULT_ROTATE;
            }
            ESP_LOGI("MSX1", "SPIFFS: Loaded rotate=%d", this->rotate);
        }
        if (file.available()) {
            this->slotLocation = file.read();
            if (this->slotLocation < 0 || 1 < this->slotLocation) {
                this->slotLocation = DEFAULT_SLOT_LOCATION;
            }
            ESP_LOGI("MSX1", "SPIFFS: Loaded slotLocation=%d", this->slotLocation);
        }
        file.close();
        SPIFFS.end();
    }

    void save() {
        SPIFFS.begin();
        File file = SPIFFS.open(APP_PREFRENCE_FILE, "w");
        if (!file) {
            ESP_LOGI("MSX1", "SPIFFS: preference cannot write");
            SPIFFS.end();
            return;
        }
        file.write((uint8_t)this->sound);
        file.write((uint8_t)this->slot);
        file.write((uint8_t)this->rotate);
        file.write((uint8_t)this->slotLocation);
        file.close();
        SPIFFS.end();
    }
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
static Preferences pref;
static Gamepad gamepad;
static Audio audio;

typedef struct OssInfo_ {
    std::string name;
    std::string license;
    std::string copyright;
} OssInfo;

static std::vector<OssInfo> ossLicensesList = {
    { "C-BIOS", "2-clause BSD", "Copyright (c) 2002 C-BIOS Association"},
    { "M5GFX", "MIT", "Copyright (c) 2021 M5Stack" },
    { "M5Unified", "MIT", "Copyright (c) 2021 M5Stack" },
    { "micro MSX2+", "MIT", "Copyright (c) 2023 Yoji Suzuki"},
    { "SUZUKIPLAN - Z80 Emulator", "MIT", "Copyright (c) 2019 Yoji Suzuki"},
};

enum class GameState {
    None,
    Playing,
    Menu,
    OssLicenses,
};

enum class ButtonPosition {
    Left,
    Center,
    Right
};

class ScreenButton {
  public:
    int x;
    int width;
    bool pressing;
    bool pressed;

    ScreenButton(int x, int width) {
        this->x = x;
        this->width = width;
        this->pressing = false;
        this->pressed = false;
    }

    inline bool wasPressed() {
        return this->pressed;
    }
};

static std::map<ButtonPosition, ScreenButton*> buttons = {
    { ButtonPosition::Left, new ScreenButton(0, 106) },
    { ButtonPosition::Center, new ScreenButton(106, 108) },
    { ButtonPosition::Right, new ScreenButton(214, 106) },
};

static void updateButtons()
{
    bool pressingLeft = false;
    bool pressingCenter = false;
    bool pressingRight = false;
    auto left = buttons[ButtonPosition::Left];
    auto center = buttons[ButtonPosition::Center];
    auto right = buttons[ButtonPosition::Right]; 
    for (int i = 0; i < M5.Touch.getCount(); i++) {
        auto raw = M5.Touch.getTouchPointRaw(i);
        auto& detail = M5.Touch.getDetail(i);
        if (detail.state & m5::touch_state_t::touch) {
            if (left->x <= raw.x && raw.x < left->x + left->width) {
                pressingLeft = true;
            } else if (center->x <= raw.x && raw.x < center->x + center->width) {
                pressingCenter = true;
            } else {
                pressingRight = true;
            }
        }
    }
    left->pressed = left->pressing && !pressingLeft;
    center->pressed = center->pressing && !pressingCenter;
    right->pressed = right->pressing && !pressingRight;
    left->pressing = pressingLeft;
    center->pressing = pressingCenter;
    right->pressing = pressingRight;
}

enum class MenuItem {
    Resume,
    Reset,
    SoundVolume,
    SelectSlot,
    ScreenRotate,
    Save,
    Load,
    Licenses,
    SlotLocation,
};

static std::map<MenuItem, std::string> menuName = {
    { MenuItem::Resume, "Resume" },
    { MenuItem::Reset, "Reset" },
    { MenuItem::SoundVolume, "Sound Volume" },
    { MenuItem::SelectSlot,  "Select Slot" },
    { MenuItem::ScreenRotate,  "Screen Rotate" },
    { MenuItem::Save, "Save" },
    { MenuItem::Load, "Load" },
    { MenuItem::Licenses, "Using OSS Licenses" },
    { MenuItem::SlotLocation, "Slot Location" },
};

static std::map<MenuItem, std::string> menuDesc = {
    { MenuItem::Resume, "Nothing to do, back in play." },
    { MenuItem::Reset, "Reset the MSX." },
    { MenuItem::SoundVolume, "Adjusts the volume level." },
    { MenuItem::SelectSlot, "Select slot number to save and load." },
    { MenuItem::ScreenRotate,  "Flip the screen up and down setting." },
    { MenuItem::Save, "Saves the state of play." },
    { MenuItem::Load, "Loads the state of play." },
    { MenuItem::Licenses, "Displays OSS license information in use." },
    { MenuItem::SlotLocation, "Choice of storage for savedata slots." },
};

static const std::vector<MenuItem> menuItems = {
    MenuItem::Resume,
    MenuItem::Reset,
    MenuItem::SoundVolume,
    MenuItem::SelectSlot,
    MenuItem::SlotLocation,
    MenuItem::ScreenRotate,
    MenuItem::Save,
    MenuItem::Load,
    MenuItem::Licenses,
};

static int menuCursor = 0;
static int menuSoundY = 0;
static int menuSlotY = 0;
static int menuRotateY = 0;
static int menuSlotLocationY = 0;
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
    gfx.println(buf);
    gfx.endWrite();
    ESP_LOGI("MSX1", "%s", buf);
    vTaskDelay(100);
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
        msx1.tick(gamepad.getExcludeHotkey(), 0, 0);
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
        msx1.tick(gamepad.getExcludeHotkey(), 0, 0); 
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
    static int16_t buf[512];
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
        if (0 < pref.sound) {
            for (i = 0; i < 512; i++) {
                buf[i] = psg.tick16(81);
            }
            audio.write(buf, sizeof(buf));
        } else {
            vTaskDelay(10);
        }
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
        gfx.setCursor(0, pref.rotate * 232);
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
		cpu0 = (int)(100.f - f0 / 1855000.f * 100.f);
		cpu1 = (int)(100.f - f1 / 1855000.f * 100.f);
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
    gfx.pushImage(0, pref.rotate ? 0 : 232, 320, 8, gamepad.isEnabled() ? rom_guide_gameboy : rom_guide_normal);
    gfx.endWrite();
    gamepad.resetCode();
    gameState = GameState::Playing;
    pauseRequest = false;
}

void resetRotation()
{
    if (pref.rotate) {
        gfx.setRotation(3);
        buttons[ButtonPosition::Left]->x = 214;
        buttons[ButtonPosition::Right]->x = 0;
    } else {
        gfx.setRotation(1);
        buttons[ButtonPosition::Left]->x = 0;
        buttons[ButtonPosition::Right]->x = 214;
    }
}

void printCenter(int y, const char* format, ...)
{
    char buf[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    gfx.setCursor((M5.Lcd.width() - strlen(buf) * 7) / 2, y);
    gfx.print(buf);
}

void quickSave()
{
    pauseAllTasks();
    gfx.startWrite();
    gfx.fillRect(0, 96, 320, 48, TFT_BLACK);
    gfx.drawRect(0, 98, 320, 2, WHITE);
    gfx.drawRect(0, 140, 320, 2, WHITE);
    printCenter(108, "SAVING TO SLOT #%d", pref.slot + 1);
    printCenter(124, "Please wait...");
    gfx.endWrite();
    char path[256];
    sprintf(path, SAVE_SLOT_FORMAT, pref.slot + 1);
    File fd;
    if (pref.slotLocation) {
        SD.begin();
        fd = SD.open(path, FILE_WRITE);
     } else {
        SPIFFS.begin();
        fd = SPIFFS.open(path, "w");
    }
    if (!fd) {
        vTaskDelay(2000);
        gfx.startWrite();
        gfx.fillRect(0, 96, 320, 48, TFT_BLACK);
        gfx.drawRect(0, 98, 320, 2, WHITE);
        gfx.drawRect(0, 140, 320, 2, WHITE);
        printCenter(116, "Save failed!");
        gfx.endWrite();
        vTaskDelay(2000);
    } else {
        size_t size;
        const uint8_t* save = (const uint8_t*)msx1.quickSave(&size);
        for (int i = 0; i < (int)size; i++) {
            fd.write(save[i]);
        }
        fd.close();
        vTaskDelay(1500);
    }
    if (pref.slotLocation) {
        SD.end();
    } else {
        SPIFFS.end();
    }
    resumeToPlay();
}

void quickLoad()
{
    pauseAllTasks();
    gfx.startWrite();
    gfx.fillRect(0, 96, 320, 48, TFT_BLACK);
    gfx.drawRect(0, 98, 320, 2, WHITE);
    gfx.drawRect(0, 140, 320, 2, WHITE);
    printCenter(108, "LOADING FROM SLOT #%d", pref.slot + 1);
    printCenter(124, "Please wait...");
    gfx.endWrite();
    char path[256];
    sprintf(path, SAVE_SLOT_FORMAT, pref.slot + 1);
    File fd;
    if (pref.slotLocation) {
        SD.begin();
        fd = SD.open(path, FILE_READ);
     } else {
        SPIFFS.begin();
        fd = SPIFFS.open(path, "r");
    }
    if (!fd) {
        vTaskDelay(2000);
        gfx.startWrite();
        gfx.fillRect(0, 96, 320, 48, TFT_BLACK);
        gfx.drawRect(0, 98, 320, 2, WHITE);
        gfx.drawRect(0, 140, 320, 2, WHITE);
        printCenter(116, "NO DATA!");
        gfx.endWrite();
        vTaskDelay(2000);
    } else {
        if (fd.size() < 1) {
            vTaskDelay(2000);
            gfx.startWrite();
            gfx.fillRect(0, 96, 320, 48, TFT_BLACK);
            gfx.drawRect(0, 98, 320, 2, WHITE);
            gfx.drawRect(0, 140, 320, 2, WHITE);
            printCenter(116, "NO DATA!");
            gfx.endWrite();
            vTaskDelay(2000);
        } else {
            uint8_t* buffer = (uint8_t*)malloc(fd.size());
            if (!buffer) {
                vTaskDelay(2000);
                gfx.startWrite();
                gfx.fillRect(0, 96, 320, 48, TFT_BLACK);
                gfx.drawRect(0, 98, 320, 2, WHITE);
                gfx.drawRect(0, 140, 320, 2, WHITE);
                printCenter(116, "MEMORY ALLOCATION ERROR!");
                gfx.endWrite();
                vTaskDelay(2000);
            } else {
                for (int i = 0; i < fd.size(); i++) {
                    buffer[i] = fd.read();
                }
                msx1.quickLoad(buffer, fd.size());
                free(buffer);
                vTaskDelay(1500);
            }
        }
        fd.close();
    }
    if (pref.slotLocation) {
        SD.end();
    } else {
        SPIFFS.end();
    }
    resumeToPlay();
}

void setup() {
    M5.begin();
    Serial.begin(115200);
    gfx.begin();
    gfx.setColorDepth(16);
    gfx.fillScreen(TFT_BLACK);
    canvas.setColorDepth(16);
    canvas.createSprite(256, 192);
    displayMutex = xSemaphoreCreateMutex();
    pref.load();
    resetRotation();
    displayMessage("Initializing...");
    displayMessage("Checking memory usage before launch MSX...");
    displayMessage("- HEAP: %d", esp_get_free_heap_size());
    displayMessage("- MALLOC_CAP_EXEC: %d", heap_caps_get_free_size(MALLOC_CAP_EXEC));
    displayMessage("- MALLOC_CAP_DMA-L: %d", heap_caps_get_largest_free_block(MALLOC_CAP_DMA));
    displayMessage("- MALLOC_CAP_INTERNAL: %d", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    displayMessage("Loading micro MSX2+ (MSX1-core) for M5Stack...");
    msx1.vdp.useOwnDisplayBuffer(displayBuffer, sizeof(displayBuffer));
    msx1.setup(0, 0, (void*)rom_cbios_main_msx1, sizeof(rom_cbios_main_msx1), "MAIN");
    msx1.setup(0, 4, (void*)rom_cbios_logo_msx1, sizeof(rom_cbios_logo_msx1), "LOGO");
    msx1.setupKeyAssign(0, MSX1_JOY_S1, 0x20); // START = SPACE
    msx1.setupKeyAssign(0, MSX1_JOY_S2, 0x1B); // SELECT = ESC
    msx1.loadRom((void*)rom_game, sizeof(rom_game), MSX1_ROM_TYPE_NORMAL);
    displayMessage("Initializing I2S (Audio)");
    audio.begin();
    displayMessage("Initializing I2C (GamePad)");
    gamepad.begin();
    displayMessage("Initializing PSG emulator");
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
    psg.setVolume(pref.sound);
    displayMessage("MSX1-core setup finished.");
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

static bool renderMenuFlag = false;
inline void renderMenu()
{
    renderMenuFlag = true;
    gfx.startWrite();
    gfx.fillRect(0, 0, 320, 8, TFT_BLACK);
    gfx.fillRoundRect(32, 16, 256, 208, 4, TFT_BLACK);
    gfx.drawRoundRect(34, 18, 252, 204, 4, WHITE);
    int y = 32;
    for (MenuItem item : menuItems) {
        gfx.setCursor(64, y);
        gfx.print(menuName[item].c_str());
        if (item == MenuItem::SoundVolume) {
            menuSoundY = y;
            const unsigned short* roms[4] = { rom_sound_mute, rom_sound_low, rom_sound_mid, rom_sound_high };
            for (int i = 0; i < 4; i++) {
                gfx.pushImage(152 + i * 32, y, 32, 8, roms[i]);
            }
        } else if (item == MenuItem::SelectSlot) {
            menuSlotY = y;
            const unsigned short* roms[3] = { rom_slot1, rom_slot2, rom_slot3 };
            for (int i = 0; i < 3; i++) {
                gfx.pushImage(152 + i * 32, y, 32, 8, roms[i]);
            }
        } else if (item == MenuItem::ScreenRotate) {
            menuRotateY = y;
            const unsigned short* roms[2] = { rom_normal, rom_reverse };
            for (int i = 0; i < 2; i++) {
                gfx.pushImage(152 + i * 32, y, 32, 8, roms[i]);
            }
        } else if (item == MenuItem::SlotLocation) {
            menuSlotLocationY = y;
            const unsigned short* roms[2] = { rom_spiffs, rom_sdcard };
            for (int i = 0; i < 2; i++) {
                gfx.pushImage(152 + i * 32, y, 32, 8, roms[i]);
            }
        }
        y += 16;
    }
    gfx.setCursor(64, 200);
    gfx.print(APP_COPYRIGHT);
    gfx.endWrite();
}

inline void renderOssLicenses()
{
    gfx.startWrite();
    gfx.fillRect(0, 0, 320, 8, TFT_BLACK);
    gfx.fillRoundRect(32, 16, 256, 208, 4, TFT_BLACK);
    gfx.drawRoundRect(34, 18, 252, 204, 4, WHITE);
    int y = 28;
    for (OssInfo oss : ossLicensesList) {
        gfx.setCursor(44, y);
        gfx.print(oss.name.c_str());
        y += 10;
        gfx.setCursor(52, y);
        gfx.print(("License: "  + oss.license).c_str());
        y += 8;
        gfx.setCursor(52, y);
        gfx.print(oss.copyright.c_str());
        y += 16;
    }
    printCenter(200, "Press Any Button");
    gfx.endWrite();
}

void modifySoundVolume(int d)
{
    pref.sound += d;
    if (pref.sound < 0) {
        pref.sound = 3;
    } else if (3 < pref.sound) {
        pref.sound = 0;
    }
    psg.setVolume(pref.sound);
}

void modifySlotSelection(int d)
{
    pref.slot += d;
    if (pref.slot < 0) {
        pref.slot = 2;
    } else if (2 < pref.slot) {
        pref.slot = 0;
    }
}

void modifySlotLocation(int d)
{
    pref.slotLocation += d;
    if (pref.slotLocation < 0) {
        pref.slotLocation = 1;
    } else if (1 < pref.slotLocation) {
        pref.slotLocation = 0;
    }
}

void modifyScreenRotation(int d)
{
    pref.rotate += d;
    if (pref.rotate < 0) {
        pref.rotate = 1;
    } else if (1 < pref.rotate) {
        pref.rotate = 0;
    }
    menuDescIndex = -1;
    resetRotation();
    gfx.clear();
    renderMenu();
}

inline void menuLoop()
{
    uint8_t pad = 0;
    if (renderMenuFlag) {
        gamepad.clearPush();
        if (0 == gamepad.get()) {
            renderMenuFlag = false;
        }
    } else {
        gamepad.updatePush();
    }    
    
    if (buttons[ButtonPosition::Center]->wasPressed() || gamepad.wasPushStart || gamepad.wasPushA || gamepad.wasPushB || gamepad.wasPushSelect) {
        switch (menuItems[menuCursor]) {
            case MenuItem::Resume:
                pref.save();
                resumeToPlay();
                return;
            case MenuItem::Reset:
                pref.save();
                msx1.reset();
                resumeToPlay();
                return;
            case MenuItem::SoundVolume:
                modifySoundVolume(gamepad.wasPushB || gamepad.wasPushSelect ? -1 : 1);
                break;
            case MenuItem::SelectSlot:
                modifySlotSelection(gamepad.wasPushB || gamepad.wasPushSelect ? -1 : 1);
                break;
            case MenuItem::SlotLocation:
                modifySlotLocation(gamepad.wasPushB || gamepad.wasPushSelect ? -1 : 1);
                break;
            case MenuItem::ScreenRotate:
                modifyScreenRotation(gamepad.wasPushB || gamepad.wasPushSelect ? -1 : 1);
                break;
            case MenuItem::Save:
                pref.save();
                quickSave();
                break;
            case MenuItem::Load:
                pref.save();
                quickLoad();
                break;
            case MenuItem::Licenses:
                gameState = GameState::OssLicenses;
                renderOssLicenses();
                return;
        }
    } else if (gamepad.wasPushLeft || gamepad.wasPushRight) {
        int d = gamepad.wasPushLeft ? -1 : 1;
        switch (menuItems[menuCursor]) {
            case MenuItem::SoundVolume:
                modifySoundVolume(d);
                break;
            case MenuItem::SelectSlot:
                modifySlotSelection(d);
                break;
            case MenuItem::SlotLocation:
                modifySlotLocation(d);
                break;
            case MenuItem::ScreenRotate:
                modifyScreenRotation(d);
                break;
            default:
                ; // nothing to do
        }
    } else if (buttons[ButtonPosition::Left]->wasPressed() || gamepad.wasPushDown) {
        menuCursor++;
        menuCursor %= (int)menuItems.size();
    } else if (buttons[ButtonPosition::Right]->wasPressed() || gamepad.wasPushUp) {
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
    for (int i = 0; i < 4; i++) {
        gfx.drawRect(152 + i * 32, menuSoundY - 2, 32, 12, i == pref.sound ? WHITE : TFT_BLACK);
    }
    for (int i = 0; i < 3; i++) {
        gfx.drawRect(152 + i * 32, menuSlotY - 2, 32, 12, i == pref.slot ? WHITE : TFT_BLACK);
    }
    for (int i = 0; i < 2; i++) {
        gfx.drawRect(152 + i * 32, menuRotateY - 2, 32, 12, i == pref.rotate ? WHITE : TFT_BLACK);
    }
    for (int i = 0; i < 2; i++) {
        gfx.drawRect(152 + i * 32, menuSlotLocationY - 2, 32, 12, i == pref.slotLocation ? WHITE : TFT_BLACK);
    }
    if (menuDescIndex != menuCursor) {
        menuDescIndex = menuCursor;
        gfx.fillRect(0, pref.rotate ? 232 : 0, 320, 8, TFT_BLACK);
        gfx.setCursor(0, pref.rotate ? 232 : 0);
        gfx.print(menuDesc[menuItems[menuDescIndex]].c_str());
        if (!gamepad.isEnabled()) {
            gfx.pushImage(0, pref.rotate ? 0 : 232, 320, 8, rom_guide_menu);
        }
    }
    gfx.endWrite();
}

bool checkPressedAnyButton() 
{
    if (gamepad.get()) return true;
    if (buttons[ButtonPosition::Left]->wasPressed()) return true;
    if (buttons[ButtonPosition::Center]->wasPressed()) return true;
    if (buttons[ButtonPosition::Right]->wasPressed()) return true;
    return false;
}

inline void ossLicensesLoop()
{
    if (checkPressedAnyButton()) {
        gameState = GameState::Menu;
        menuDescIndex = -1;
        renderMenu();
    }
}

inline void playingLoop()
{
    gfx.pushImage(0, pref.rotate ? 0 : 232, 320, 8, gamepad.isEnabled() ? rom_guide_gameboy : rom_guide_normal);
    uint8_t pad = gamepad.getLatest();
    if (buttons[ButtonPosition::Left]->wasPressed() || (pad & MSX1_JOY_S2 && pad & MSX1_JOY_S1)) {
        pauseAllTasks();
        gameState = GameState::Menu;
        menuCursor = 0;
        menuDescIndex = -1;
        renderMenu();
    } else if (buttons[ButtonPosition::Center]->wasPressed() || (pad & MSX1_JOY_S2 && pad & MSX1_JOY_T2)) {
        quickSave();
    } else if (buttons[ButtonPosition::Right]->wasPressed() || (pad & MSX1_JOY_S2 && pad & MSX1_JOY_T1)) {
        quickLoad();
    }
}

void loop() {
    if (!gamepad.isEnabled()) {
        M5.update();
    }
    updateButtons();
    switch (gameState) {
        case GameState::None: resumeToPlay(); break;
        case GameState::Playing: playingLoop(); break;
        case GameState::Menu: menuLoop(); break;
        case GameState::OssLicenses: ossLicensesLoop(); break;
    }
    vTaskDelay(10);
}
