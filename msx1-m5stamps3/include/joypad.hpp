/**
 * micro MSX2+ - JoyPad driver
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
#ifndef INCLUDE_JOYPAD_HPP
#define INCLUDE_JOYPAD_HPP
#include <Arduino.h>
#include "msx1def.h"

class Joypad {
  private:
    struct Pin {
        gpio_num_t up;
        gpio_num_t down;
        gpio_num_t left;
        gpio_num_t right;
        gpio_num_t a;
        gpio_num_t b;
        gpio_num_t select;
        gpio_num_t start;
    } pin;

  public:
    Joypad()
    {
        memset(&this->pin, 0, sizeof(this->pin));
    }

    void init(gpio_num_t pinUp, gpio_num_t pinDown, gpio_num_t pinLeft, gpio_num_t pinRight, gpio_num_t pinA, gpio_num_t pinB, gpio_num_t pinSelect, gpio_num_t pinStart)
    {
        this->pin.up = pinUp;
        this->pin.down = pinDown;
        this->pin.left = pinLeft;
        this->pin.right = pinRight;
        this->pin.a = pinA;
        this->pin.b = pinB;
        this->pin.select = pinSelect;
        this->pin.start = pinStart;
        if (this->pin.up) pinMode(this->pin.up, INPUT_PULLUP);
        if (this->pin.down) pinMode(this->pin.down, INPUT_PULLUP);
        if (this->pin.left) pinMode(this->pin.left, INPUT_PULLUP);
        if (this->pin.right) pinMode(this->pin.right, INPUT_PULLUP);
        if (this->pin.a) pinMode(this->pin.a, INPUT_PULLUP);
        if (this->pin.b) pinMode(this->pin.b, INPUT_PULLUP);
        if (this->pin.select) pinMode(this->pin.select, INPUT_PULLUP);
        if (this->pin.start) pinMode(this->pin.start, INPUT_PULLUP);
    }

    inline uint8_t scan()
    {
        uint8_t result = 0;
        result |= this->pin.start && digitalRead(this->pin.start) == LOW ? MSX1_JOY_S1 : 0;
        result |= this->pin.select && digitalRead(this->pin.select) == LOW ? MSX1_JOY_S2 : 0;
        result |= this->pin.a && digitalRead(this->pin.a) == LOW ? MSX1_JOY_T1 : 0;
        result |= this->pin.b && digitalRead(this->pin.b) == LOW ? MSX1_JOY_T2 : 0;
        result |= this->pin.left && digitalRead(this->pin.left) == LOW ? MSX1_JOY_LE : 0;
        result |= this->pin.right && digitalRead(this->pin.right) == LOW ? MSX1_JOY_RI : 0;
        result |= this->pin.up && digitalRead(this->pin.up) == LOW ? MSX1_JOY_UP : 0;
        result |= this->pin.down && digitalRead(this->pin.down) == LOW ? MSX1_JOY_DW : 0;
        return result;
    }
};

#endif
