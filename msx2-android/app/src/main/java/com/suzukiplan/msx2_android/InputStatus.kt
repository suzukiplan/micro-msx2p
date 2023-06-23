/**
 * micro MSX2+ - Android
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
package com.suzukiplan.msx2_android

data class InputStatus(
    var up: Boolean,
    var down: Boolean,
    var left: Boolean,
    var right: Boolean,
    var a: Boolean,
    var b: Boolean,
    var select: Boolean,
    var start: Boolean
) {
    val code: Int
        get() {
            var value = 0
            if (up) value = value.or(0b00000001)
            if (down) value = value.or(0b00000010)
            if (left) value = value.or(0b00000100)
            if (right) value = value.or(0b00001000)
            if (a) value = value.or(0b00010000)
            if (b) value = value.or(0b00100000)
            if (start) value = value.or(0b01000000)
            if (select) value = value.or(0b10000000)
            return value
        }
}
