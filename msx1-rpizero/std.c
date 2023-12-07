/**
 * micro MSX2+ for RaspberryPi Baremetal Environment - Standard library Hooks
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
#include <circle/startup.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

void exit(int code)
{
    halt();
}

time_t time(time_t* second)
{
    return 0;
}

struct tm* localtime(const time_t* _timer)
{
    static struct tm result;
    return &result;
}

int printf(const char* format, ...)
{
    return 0;
}

int puts(const char* text)
{
    return 0;
}

int snprintf(char* buffer, size_t size, const char* format, ...)
{
    return 0;
}

int vsnprintf(char* buffer, size_t size, const char* format, va_list arg)
{
    return snprintf(buffer, size, format, arg);
}