/**
 * Simple Mailbox API
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
 * NOTE: This code implementation is based on the following source
 * https://github.com/kumaashi/RaspberryPI/blob/master/RPIZEROW/Sample_Framebuffer/main.c
 * (c) 2022 yasai kumaashi (gyaboyan@gmail.com)
 */
#pragma once
#include "extern.h"
#include <stdint.h>
#include <stdio.h>

#define SUBSYSTEM_BASE 0x20000000
#define WAIT_CNT 0x100000
#define VCADDR_BASE 0x40000000
#define MAILBOX_EMPTY 0x40000000
#define MAILBOX_FULL 0x80000000
#define MAILBOX_BASE (SUBSYSTEM_BASE + 0xB880)
#define MAILBOX_READ ((volatile uint32_t*)(MAILBOX_BASE + 0x00))
#define MAILBOX_STATUS ((volatile uint32_t*)(MAILBOX_BASE + 0x18))
#define MAILBOX_WRITE ((volatile uint32_t*)(MAILBOX_BASE + 0x20))
#define MAILBOX_FRAMEBUFFER 1
#define MAILBOX_VC_TO_ARM 9
#define MAILBOX_ARM_TO_VC 8

class Mailbox
{
  private:
    typedef struct FrameBuffer_ {
        int displayWidth;
        int displayHeight;
        int frameBufferWidth;
        int frameBufferHeight;
        int pitch;
        int depth;
        int x;
        int y;
        uint32_t address;
        uint32_t size;
        uint32_t addressV;
    } FrameBuffer;

    volatile uint8_t data[1024] __attribute__((aligned(256)));
    volatile FrameBuffer fb __attribute__((aligned(256)));

    void write(uint8_t mask, uint32_t value)
    {
        InvalidateData();
        do {
            InvalidateData();
        } while ((*MAILBOX_STATUS) & MAILBOX_FULL);
        InvalidateData();
        *MAILBOX_WRITE = (mask | (value & 0xFFFFFFF0));
    }

    uint32_t read(uint8_t mask)
    {
        uint32_t result = 0;
        do {
            MemoryBarrier();
            if ((*MAILBOX_STATUS) & MAILBOX_EMPTY) {
                continue;
            }
            MemoryBarrier();
            result = *MAILBOX_READ;
        } while ((result & 0xF) != mask);
        return result & 0xFFFFFFF0;
    }

    inline uint32_t armToVideo(void* p) { return ((uint32_t)p) + VCADDR_BASE; }
    inline uint32_t videoToArm(uint32_t p) { return (uint32_t)(p) & ~(VCADDR_BASE); }

  public:
    Mailbox(int width, int height)
    {
        fb.displayWidth = width;
        fb.displayHeight = height;
        fb.frameBufferWidth = width;
        fb.frameBufferHeight = height;
        fb.pitch = 0;
        fb.depth = 32;
        fb.x = 0;
        fb.y = 0;
        fb.address = 0;
        fb.size = 0;
        int count = 0;
        do {
            write(MAILBOX_FRAMEBUFFER, armToVideo((void*)&fb));
            read(MAILBOX_FRAMEBUFFER);
            if (fb.address) {
                break;
            }
            count++;
            SLEEP(0x1000);
        } while (1);
        fb.addressV = fb.address;
        fb.address = videoToArm(fb.address);
    }

    inline void pixel(int x, int y, uint32_t color, bool noCheck = false)
    {
        if (!noCheck) {
            if (x < 0 || y < 0 || fb.frameBufferWidth <= x || fb.frameBufferHeight <= y) {
                return;
            }
        }
        auto ptr = (uint32_t*)fb.address;
        ptr += (fb.pitch >> 2) * y;
        ptr += x;
        *ptr = color;
    }
};
