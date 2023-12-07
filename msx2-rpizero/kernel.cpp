//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2021  R. Stange <rsta2@o2online.de>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include "kernel.h"
#include "msx2.hpp"
#include "roms.hpp"
#include <circle/timer.h>

#ifdef MSX2_DISPLAY_HALF_HORIZONTAL
CKernel::CKernel(void) : screen(320, 240)
#else
CKernel::CKernel(void) : screen(640, 480)
#endif
{
}

CKernel::~CKernel(void)
{
}

boolean CKernel::initialize(void)
{
    return screen.Initialize();
}

TShutdownMode CKernel::run(void)
{
    MSX2 msx2(MSX2_COLOR_MODE_RGB565);
    msx2.setupSecondaryExist(false, false, false, true);
    msx2.setup(0, 0, 0, (void*)rom_cbios_main_msx2p, 0x8000, "MAIN");
    msx2.setup(0, 0, 4, (void*)rom_cbios_logo_msx2p, 0x4000, "LOGO");
    msx2.setup(3, 0, 0, (void*)rom_cbios_sub, 0x4000, "SUB");
    msx2.setupRAM(3, 3);
    msx2.loadRom((void*)rom_game, sizeof(rom_game), MSX2_ROM_TYPE_NORMAL); // modify here if use mega rom
    auto buffer = screen.GetFrameBuffer();
    int pitch = buffer->GetPitch() / sizeof(TScreenColor);
    while (1) {
        msx2.tick(0, 0, 0);
        size_t pcmSize;
        msx2.getSound(&pcmSize);
        // TODO: play sound
        uint16_t* display = msx2.getDisplay();
        uint16_t* hdmi = (uint16_t*)buffer->GetBuffer();
#ifdef MSX2_DISPLAY_HALF_HORIZONTAL
        for (int y = 0; y < 240; y++) {
            memcpy(hdmi + 18, display, 284 * 2);
            for (int x = 0; x < 18; x++) {
                hdmi[x] = display[0];
                hdmi[x + 302] = display[0];
            }
            memcpy(hdmi + pitch, hdmi, 320 * 2);
            display += 284;
            hdmi += pitch;
        }
#else
        for (int y = 0; y < 480; y += 2) {
            memcpy(hdmi + 36, display, 568 * 2);
            for (int x = 0; x < 36; x++) {
                hdmi[x] = display[0];
                hdmi[x + 604] = display[0];
            }
            memcpy(hdmi + pitch, hdmi, 640 * 2);
            display += 568;
            hdmi += pitch * 2;
        }
#endif
    }

    while (1) {
        led.On();
        CTimer::SimpleMsDelay(100);

        led.Off();
        CTimer::SimpleMsDelay(100);
    }
    return ShutdownHalt;
}
