/**
 * Raw Streaming Audio API for M5Stack
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
#ifndef INCLUDE_AUDIO_HPP
#define INCLUDE_AUDIO_HPP

#include <driver/i2s.h>
#include <M5Unified.h>

class Audio {
#if defined(M5StackCore2)
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
#elif defined(M5StackCoreS3)
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
#else
#error unsupported
#endif
};

#endif
