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
#include "msx1.hpp"
#include "roms.hpp"

static uint16_t* hdmiBuffer;
static int hdmiPitch;

TShutdownMode CKernel::run(void)
{
    auto buffer = screen.GetFrameBuffer();
    hdmiPitch = buffer->GetPitch() / sizeof(TScreenColor);
    hdmiBuffer = (uint16_t*)buffer->GetBuffer();

    unsigned char ram[0x4000];
    TMS9918A::Context vram;
    MSX1 msx1(TMS9918A::ColorMode::RGB565, ram, sizeof(ram), &vram, [](void* arg, int frame, int line, unsigned short* display) {
        memcpy(&hdmiBuffer[line * hdmiPitch], display, 512);
    });
    msx1.psg.setVolume(4);
    msx1.setup(0, 0, (void*)rom_cbios_main_msx1, 0x8000, "MAIN");
    msx1.setup(0, 4, (void*)rom_cbios_logo_msx1, 0x4000, "LOGO");
    msx1.loadRom((void*)rom_game, sizeof(rom_game), MSX1_ROM_TYPE_NORMAL); // modify here if use mega rom
    msx1.reset();
    msx1.psg.reset(320);
    sound.SetControl(VCHIQ_SOUND_VOLUME_MAX);

    // main loop
    int swap = 0;
    while (1) {
        // execute MSX tick and rendering
        msx1.tick(0, 0, 0);

        // flip screen and wait V-SYNC
        swap = 192 - swap;
        buffer->SetVirtualOffset(0, swap);
        buffer->WaitForVerticalSync();

        // play sound
        size_t pcmSize;
        int16_t* pcmData = (int16_t*)msx1.getSound(&pcmSize);
        while (sound.PlaybackActive()) {
            scheduler.Sleep(1);
        }
        sound.Playback(pcmData, pcmSize / 2, 1, 16);
    }

    return ShutdownHalt;
}
