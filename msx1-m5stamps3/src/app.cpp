/**
 * micro MSX2+ - Example for M5StampS3
 * -----------------------------------------------------------------------------
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Yoji Suzuki.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -----------------------------------------------------------------------------
 */
// C++ stdlib
#include <map>
#include <string>
#include <vector>

// micro MSX2+
#include "ay8910.hpp"
#include "msx1.hpp"
#include "roms.hpp"
#include "joypad.hpp"

// Arduino
#include <Arduino.h>
#include <SD.h>
#include <SPIFFS.h>
#include <Wire.h>
#include <driver/i2s.h>
#include <esp_freertos_hooks.h>
#include <esp_log.h>

// LGFX
#include "ILI9431.hpp"

class CustomCanvas : public lgfx::LGFX_Sprite
{
  public:
    CustomCanvas() : LGFX_Sprite() {}

    CustomCanvas(LovyanGFX* parent) : LGFX_Sprite(parent)
    {
        _psram = false; // PSRAM is not used to ensure performance (DMA is used)
    }

    void* frameBuffer(uint8_t)
    {
        return getBuffer();
    }
};

class Audio
{
  private:
    static constexpr i2s_port_t i2sNum = I2S_NUM_0;
    static constexpr int sampleRate = 44100;

  public:
    void begin()
    {
        // I2S config
        i2s_config_t config;
        memset(&config, 0, sizeof(i2s_config_t));
        config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
        config.sample_rate = this->sampleRate;
        config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
        config.channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT;
        config.communication_format = I2S_COMM_FORMAT_STAND_MSB;
        config.dma_buf_count = 4;
        config.dma_buf_len = 1024;
        config.tx_desc_auto_clear = true;
        // I2S pin config
        i2s_pin_config_t pinConfig;
        memset(&pinConfig, ~0u, sizeof(i2s_pin_config_t));
        pinConfig.bck_io_num = GPIO_NUM_5;
        pinConfig.ws_io_num = GPIO_NUM_9;
        pinConfig.data_out_num = GPIO_NUM_7;
        pinConfig.data_in_num = I2S_PIN_NO_CHANGE;
        // Setup I2S
        if (ESP_OK != i2s_driver_install(this->i2sNum, &config, 0, nullptr)) {
            i2s_driver_uninstall(this->i2sNum);
            i2s_driver_install(this->i2sNum, &config, 0, nullptr);
        }
        i2s_set_pin(this->i2sNum, &pinConfig);
        i2s_set_sample_rates(this->i2sNum, this->sampleRate);
        i2s_set_clk(this->i2sNum, this->sampleRate, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
        i2s_zero_dma_buffer(this->i2sNum);
        i2s_start(this->i2sNum);
    }

    inline void write(int16_t* buf, size_t bufSize)
    {
        size_t wrote;
        i2s_write(this->i2sNum, buf, bufSize, &wrote, portMAX_DELAY);
        vTaskDelay(2);
    }
};

class KantanUsbKeyboard {
private:
    class MSXKey {
    public:
        int column;
        uint8_t bit;

        MSXKey(int column, uint8_t bit)
        {
            this->column = column;
            this->bit = bit;
        }
    };
    std::map<uint8_t, MSXKey*> mapK;
    std::map<uint8_t, MSXKey*> mapM;

public:
    typedef struct Event_ {
        uint8_t addr;
        uint8_t code;
        uint8_t type;
    } Event;
    uint8_t msxKeyCodeMap[16];

private:
    Event lastEvent;

public:
    KantanUsbKeyboard()
    {
        memset(&this->lastEvent, 0, sizeof(this->lastEvent));
        memset(&this->msxKeyCodeMap, 0, sizeof(this->msxKeyCodeMap));
    }

    void init()
    {
        Serial.begin(9600, SERIAL_8N1);
        mapK[0x27] = new MSXKey(0, 0b00000001); // 0
        mapK[0x1E] = new MSXKey(0, 0b00000010); // 1
        mapK[0x1F] = new MSXKey(0, 0b00000100); // 2
        mapK[0x20] = new MSXKey(0, 0b00001000); // 3
        mapK[0x21] = new MSXKey(0, 0b00010000); // 4
        mapK[0x22] = new MSXKey(0, 0b00100000); // 5
        mapK[0x23] = new MSXKey(0, 0b01000000); // 6
        mapK[0x24] = new MSXKey(0, 0b10000000); // 7
        mapK[0x25] = new MSXKey(1, 0b00000001); // 8
        mapK[0x26] = new MSXKey(1, 0b00000010); // 9
        mapK[0x2D] = new MSXKey(1, 0b00000100); // -
        mapK[0x2E] = new MSXKey(1, 0b00001000); // ^
        mapK[0x89] = new MSXKey(1, 0b00010000); // ¥
        mapK[0x2F] = new MSXKey(1, 0b00100000); // @
        mapK[0x30] = new MSXKey(1, 0b01000000); // [
        mapK[0x33] = new MSXKey(1, 0b10000000); // ;
        mapK[0x34] = new MSXKey(2, 0b00000001); // :
        mapK[0x31] = new MSXKey(2, 0b00000010); // ]
        mapK[0x36] = new MSXKey(2, 0b00000100); // ,
        mapK[0x37] = new MSXKey(2, 0b00001000); // .
        mapK[0x38] = new MSXKey(2, 0b00010000); // /
        mapK[0x87] = new MSXKey(2, 0b00100000); // _
        mapK[0x04] = new MSXKey(2, 0b01000000); // A
        mapK[0x05] = new MSXKey(2, 0b10000000); // B
        mapK[0x06] = new MSXKey(3, 0b00000001); // C
        mapK[0x07] = new MSXKey(3, 0b00000010); // D
        mapK[0x08] = new MSXKey(3, 0b00000100); // E
        mapK[0x09] = new MSXKey(3, 0b00001000); // F
        mapK[0x0A] = new MSXKey(3, 0b00010000); // G
        mapK[0x0B] = new MSXKey(3, 0b00100000); // H
        mapK[0x0C] = new MSXKey(3, 0b01000000); // I
        mapK[0x0D] = new MSXKey(3, 0b10000000); // J
        mapK[0x0E] = new MSXKey(4, 0b00000001); // K
        mapK[0x0F] = new MSXKey(4, 0b00000010); // L
        mapK[0x10] = new MSXKey(4, 0b00000100); // M
        mapK[0x11] = new MSXKey(4, 0b00001000); // N
        mapK[0x12] = new MSXKey(4, 0b00010000); // O
        mapK[0x13] = new MSXKey(4, 0b00100000); // P
        mapK[0x14] = new MSXKey(4, 0b01000000); // Q
        mapK[0x15] = new MSXKey(4, 0b10000000); // R
        mapK[0x16] = new MSXKey(5, 0b00000001); // S
        mapK[0x17] = new MSXKey(5, 0b00000010); // T
        mapK[0x18] = new MSXKey(5, 0b00000100); // U
        mapK[0x19] = new MSXKey(5, 0b00001000); // V
        mapK[0x1A] = new MSXKey(5, 0b00010000); // W
        mapK[0x1B] = new MSXKey(5, 0b00100000); // X
        mapK[0x1C] = new MSXKey(5, 0b01000000); // Y
        mapK[0x1D] = new MSXKey(5, 0b10000000); // Z
        mapK[0x39] = new MSXKey(6, 0b00001000); // caps
        mapK[0x90] = new MSXKey(6, 0b00010000); // kana
        mapK[0x3A] = new MSXKey(6, 0b00100000); // F1
        mapK[0x3B] = new MSXKey(6, 0b01000000); // F2
        mapK[0x3C] = new MSXKey(6, 0b10000000); // F3
        mapK[0x3D] = new MSXKey(7, 0b00000001); // F4
        mapK[0x3E] = new MSXKey(7, 0b00000010); // F5
        mapK[0x29] = new MSXKey(7, 0b00000100); // esc
        mapK[0x2B] = new MSXKey(7, 0b00001000); // tab
        mapK[0x45] = new MSXKey(7, 0b00010000); // F12 as stop
        mapK[0x2A] = new MSXKey(7, 0b00100000); // delete as BS
        mapK[0x44] = new MSXKey(7, 0b01000000); // F11 as select
        mapK[0x28] = new MSXKey(7, 0b10000000); // return
        mapK[0x2C] = new MSXKey(8, 0b00000001); // space
        mapK[0x43] = new MSXKey(8, 0b00000010); // F10 as cls/home
        mapK[0x42] = new MSXKey(8, 0b00000100); // F9 as ins
        mapK[0x41] = new MSXKey(8, 0b00001000); // F8 as del
        mapK[0x50] = new MSXKey(8, 0b00010000); // left cursor
        mapK[0x52] = new MSXKey(8, 0b00100000); // up cursor
        mapK[0x51] = new MSXKey(8, 0b01000000); // down cursor
        mapK[0x4F] = new MSXKey(8, 0b10000000); // right cursor
        mapM[0x02] = new MSXKey(6, 0b00000001); // left shift
        mapM[0x20] = new MSXKey(6, 0b00000001); // right shift
        mapM[0x01] = new MSXKey(6, 0b00000010); // control as ctrl
        mapM[0x04] = new MSXKey(6, 0b00000100); // option as graph
    }

    Event* check()
    {
        if (Serial.available() < 1) {
            return nullptr;
        }
        char buf[16];
        int ptr =0;
        do {
            while (Serial.available() < 1) {
                vTaskDelay(1);
            }
            buf[ptr++] = Serial.read();
        } while (ptr != 8 && buf[ptr - 1] != ';');
        this->lastEvent.addr = isdigit(buf[0]) ? buf[0] - '0' : buf[0] - 'a' + 10;
        this->lastEvent.addr <<= 4;
        this->lastEvent.addr |= isdigit(buf[1]) ? buf[1] - '0' : buf[1] - 'a' + 10;
        this->lastEvent.type = buf[2];
        this->lastEvent.code = isdigit(buf[3]) ? buf[3] - '0' : buf[3] - 'a' + 10;
        this->lastEvent.code <<= 4;
        this->lastEvent.code |= isdigit(buf[4]) ? buf[4] - '0' : buf[4] - 'a' + 10;
        switch (this->lastEvent.type) {
            case 'k': { // released key
                auto it = mapK.find(this->lastEvent.code);
                if (it != mapK.end()) {
                    this->msxKeyCodeMap[it->second->column] ^= it->second->bit;
                }
                break;
            }
            case 'K': { // pressed key
                auto it = mapK.find(this->lastEvent.code);
                if (it != mapK.end()) {
                    this->msxKeyCodeMap[it->second->column] |= it->second->bit;
                }
                break;
            }
            case 'm': { // released subkey
                auto it = mapM.find(this->lastEvent.code);
                if (it != mapM.end()) {
                    this->msxKeyCodeMap[it->second->column] ^= it->second->bit;
                }
                break;
            }
            case 'M': { // pressed subkey
                auto it = mapM.find(this->lastEvent.code);
                if (it != mapM.end()) {
                    this->msxKeyCodeMap[it->second->column] |= it->second->bit;
                }
                break;
            }
            default: // ignore other events
                return &this->lastEvent;
        }
        return &this->lastEvent;
    }
};

static uint8_t ram[0x4000];
static TMS9918A::Context vram;
static ILI9431 gfx;
static CustomCanvas canvas(&gfx);
static Audio audio;
static uint16_t displayBuffer[256];
static xSemaphoreHandle displayMutex;
static unsigned short backdropColor;
static int fps;
static int cpu0;
static int cpu1;
static uint64_t idle0 = 0;
static uint64_t idle1 = 0;
static AY8910 psg;
static Joypad joypad;
static KantanUsbKeyboard keyboard;

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
    vTaskDelay(100);
}

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

void IRAM_ATTR ticker(void* arg)
{
    static long start;
    static long procTime = 0;
    static const long interval[3] = {16, 17, 17};
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
        uint8_t key = joypad.scan();
        xSemaphoreTake(displayMutex, portMAX_DELAY);
        msx1.tickWithKeyCodeMap(key, 0, keyboard.msxKeyCodeMap);
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
        msx1.tickWithKeyCodeMap(key, 0, keyboard.msxKeyCodeMap);
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
    static uint16_t work;
    static size_t size;
    static int i;
    while (1) {
        for (i = 0; i < 512; i++) {
            buf[i] = psg.tick16(81);
        }
        audio.write(buf, sizeof(buf));
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
        gfx.setCursor(0, 232);
        sprintf(buf, "EMU:%d/60fps  LCD:%d/30fps  C0:%d%%  C1:%d%% ", fps, renderFps, cpu0, cpu1);
        gfx.print(buf);

        auto evt = keyboard.check();
        while (evt) {
            gfx.fillRect(320 - 32, 232, 32, 8, 0x0000);
            gfx.setCursor(320 - 32, 232);
            sprintf(buf, "%02X%c%02X", evt->addr, evt->type, evt->code);
            gfx.print(buf);
            evt = keyboard.check();
        }

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

void setup()
{
    joypad.init(
        GPIO_NUM_41, // up
        GPIO_NUM_42, // down
        GPIO_NUM_39, // left
        GPIO_NUM_40, // right
        GPIO_NUM_12, // A
        GPIO_NUM_14, // B
        GPIO_NUM_11, // Select
        GPIO_NUM_10  // Start
    );
    keyboard.init();
    gfx.init();
    gfx.setRotation(3);
    gfx.setColorDepth(16);
    gfx.fillScreen(TFT_BLACK);
    canvas.setColorDepth(16);
    canvas.createSprite(256, 192);
    displayMutex = xSemaphoreCreateMutex();
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
    displayMessage("Initializing audio system (I2S)");
    audio.begin();
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
    psg.setVolume(2);
    displayMessage("MSX1-core setup finished.");
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

void loop() {}
