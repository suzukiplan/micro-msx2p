/**
 * Gamepad API for M5Face II
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
#ifndef INCLUDE_GAMEPAD_HPP
#define INCLUDE_GAMEPAD_HPP

#include <M5Unified.h>

class Gamepad
{
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

    Gamepad()
    {
        this->code = 0;
        this->enabled = false;
        this->mutex = xSemaphoreCreateMutex();
        this->clearPush();
    }

    void begin()
    {
        Wire1.begin(this->sda, this->scl);
        Wire1.beginTransmission(0x08);
        this->enabled = 0 == Wire1.endTransmission();
#ifndef M5StackCoreS3
        if (this->enabled) pinMode(this->pinInt, INPUT_PULLUP);
#endif
    }

#ifdef M5StackCoreS3
    inline bool isReadble()
    {
        return true;
    }

    inline uint8_t read()
    {
        uint8_t result = Wire1.peek();
        Wire1.flush();
        return result;
    }
#else
    inline bool isReadble()
    {
        return digitalRead(this->pinInt) == LOW;
    }
    inline uint8_t read() { return (uint8_t)Wire1.read(); }
#endif
    inline bool isEnabled()
    {
        return this->enabled;
    }

    inline uint8_t get()
    {
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

    inline uint8_t getExcludeHotkey()
    {
        uint8_t result = this->get();
        return result & MSX1_JOY_S2 && result ^ MSX1_JOY_S2 ? 0 : result;
    }

    inline uint8_t getLatest()
    {
        xSemaphoreTake(this->mutex, portMAX_DELAY);
        uint8_t result = this->code;
        xSemaphoreGive(this->mutex);
        return result;
    }

    inline void clearPush()
    {
        this->wasPushUp = false;
        this->wasPushDown = false;
        this->wasPushLeft = false;
        this->wasPushRight = false;
        this->wasPushA = false;
        this->wasPushB = false;
        this->wasPushStart = false;
        this->wasPushSelect = false;
    }

    inline void updatePush()
    {
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

    inline void resetCode()
    {
        this->get();
        xSemaphoreTake(this->mutex, portMAX_DELAY);
        this->code = 0;
        xSemaphoreGive(this->mutex);
    }
};

#endif
