/**
 * micro MSX2+ - LCD driver for ILI9341
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
#ifndef INCLUDE_ILI9431_HPP
#define INCLUDE_ILI9431_HPP

#include <LovyanGFX.hpp>

#define PIN_CS GPIO_NUM_1
#define PIN_DC GPIO_NUM_3
#define PIN_MOSI GPIO_NUM_13
#define PIN_SCLK GPIO_NUM_15

class ILI9431 : public lgfx::LGFX_Device
{
  private:
    lgfx::Panel_ILI9341 panel;
    lgfx::Bus_SPI bus;
    lgfx::Light_PWM light;

    void setupBusConfig()
    {
        auto config = this->bus.config();
        config.spi_host = SPI3_HOST;
        config.spi_mode = 0;
        config.freq_write = 40000000;
        config.freq_read = 16000000;
        config.spi_3wire = true;
        config.use_lock = true;
        config.dma_channel = SPI_DMA_CH_AUTO;
        config.pin_sclk = PIN_SCLK;
        config.pin_mosi = PIN_MOSI;
        config.pin_miso = -1;
        config.pin_dc = PIN_DC;
        this->bus.config(config);
    }

    void setupPanelConfig()
    {
        auto config = this->panel.config();
        config.pin_cs = PIN_CS;
        config.pin_busy = -1;
        config.panel_width = 240;
        config.panel_height = 320;
        config.offset_x = 0;
        config.offset_y = 0;
        config.offset_rotation = 2;
        config.dummy_read_pixel = 8;
        config.dummy_read_bits = 1;
        config.readable = false;
        config.invert = false;
        config.rgb_order = false;
        config.dlen_16bit = false;
        config.bus_shared = true;
        this->panel.config(config);
    }

    void setupLightConfig()
    {
        auto config = this->light.config();
        config.pin_bl = PIN_LED;
        config.invert = false;
        config.freq = 44100;
        config.pwm_channel = 7;
        this->light.config(config);
    }

  public:
    ILI9431(void)
    {
        this->setupBusConfig();
        this->setupPanelConfig();
        this->panel.setBus(&this->bus);
        setPanel(&this->panel);
    }
};

#endif
