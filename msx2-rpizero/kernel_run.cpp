/**
 * micro MSX2+ for RaspberryPi Baremetal Environment - Main Procedure
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
#include "kernel.h"
#include "msx2.hpp"
#include "roms.hpp"

static uint16_t* hdmiBuffer;
static int hdmiPitch;

TShutdownMode CKernel::run(void)
{
    auto buffer = screen.GetFrameBuffer();
    hdmiPitch = buffer->GetPitch() / sizeof(TScreenColor);
    hdmiBuffer = (uint16_t*)buffer->GetBuffer();
    MSX2 msx2(MSX2_COLOR_MODE_RGB565);
    msx2.setupSecondaryExist(false, false, false, true);
    msx2.setup(0, 0, 0, (void*)rom_cbios_main_msx2p, 0x8000, "MAIN");
    msx2.setup(0, 0, 4, (void*)rom_cbios_logo_msx2p, 0x4000, "LOGO");
    msx2.setup(3, 0, 0, (void*)rom_cbios_sub, 0x4000, "SUB");
    msx2.setupRAM(3, 3);
    msx2.loadRom((void*)rom_game, sizeof(rom_game), MSX2_ROM_TYPE_NORMAL); // modify here if use mega rom
    int swap = 0;
    while (1) {
        msx2.tick(0, 0, 0);
        uint16_t* display = msx2.getDisplay();
        uint16_t* hdmi = hdmiBuffer;
#ifdef MSX2_DISPLAY_HALF_HORIZONTAL
        for (int y = 0; y < 240; y++) {
            memcpy(hdmi + 18, display, 284 * 2);
            for (int x = 0; x < 18; x++) {
                hdmi[x] = display[0];
                hdmi[x + 302] = display[0];
            }
            memcpy(hdmi + pitch, hdmi, 320 * 2);
            display += 284;
            hdmi += hdmiPitch;
        }
        swap = 240 - swap;
#else
        for (int y = 0; y < 480; y += 2) {
            memcpy(hdmi + 36, display, 568 * 2);
            for (int x = 0; x < 36; x++) {
                hdmi[x] = display[0];
                hdmi[x + 604] = display[0];
            }
            memcpy(hdmi + hdmiPitch, hdmi, 640 * 2);
            display += 568;
            hdmi += hdmiPitch * 2;
        }
        swap = 480 - swap;
#endif
        buffer->SetVirtualOffset(0, swap);
        buffer->WaitForVerticalSync();

        // play sound
        size_t pcmSize;
        int16_t* pcmData = (int16_t*)msx2.getSound(&pcmSize);
        while (sound.PlaybackActive()) {
            scheduler.Sleep(1);
        }
        sound.Playback(pcmData, pcmSize / 2, 1, 16);
    }

    return ShutdownHalt;
}
