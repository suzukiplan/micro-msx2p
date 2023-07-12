/**
 * micro MSX2+ - constants for MSX1
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
#ifndef INCLUDE_MSX1DEF
#define INCLUDE_MSX1DEF

#define MSX1_JOY_UP 0b00000001
#define MSX1_JOY_DW 0b00000010
#define MSX1_JOY_LE 0b00000100
#define MSX1_JOY_RI 0b00001000
#define MSX1_JOY_T1 0b00010000
#define MSX1_JOY_T2 0b00100000
#define MSX1_JOY_S1 0b01000000
#define MSX1_JOY_S2 0b10000000
#define MSX1_ROM_TYPE_NORMAL 0
#define MSX1_ROM_TYPE_ASC8 1
#define MSX1_ROM_TYPE_ASC8_SRAM2 2
#define MSX1_ROM_TYPE_ASC16 3
#define MSX1_ROM_TYPE_ASC16_SRAM2 4
#define MSX1_ROM_TYPE_KONAMI 6

#endif /* INCLUDE_MSX1DEF */
