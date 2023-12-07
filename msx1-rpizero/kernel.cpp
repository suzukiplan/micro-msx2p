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
#include "msx1.hpp"
#include "roms.hpp"
#include <circle/timer.h>

static uint16_t* hdmiBuffer;
static int hdmiPitch;

CKernel::CKernel(void) : screen(256, 192)
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
    auto buffer = screen.GetFrameBuffer();
    hdmiPitch = buffer->GetPitch() / sizeof(TScreenColor);
    hdmiBuffer = (uint16_t*)buffer->GetBuffer();

    unsigned char ram[0x4000];
    TMS9918A::Context vram;
    MSX1 msx1(TMS9918A::ColorMode::RGB565, ram, sizeof(ram), &vram, [](void* arg, int frame, int line, unsigned short* display) {
        memcpy(&hdmiBuffer[line * hdmiPitch], display, 512);
    });
    msx1.setup(0, 0, (void*)rom_cbios_main_msx1, 0x8000, "MAIN");
    msx1.setup(0, 4, (void*)rom_cbios_logo_msx1, 0x4000, "LOGO");
    msx1.loadRom((void*)rom_game, sizeof(rom_game), MSX1_ROM_TYPE_NORMAL); // modify here if use mega rom
    int swap = 0;
    while (1) {
        msx1.tick(0, 0, 0);
        size_t pcmSize;
        msx1.getSound(&pcmSize);
        swap = 192 - swap;
		buffer->SetVirtualOffset(0, swap);
		buffer->WaitForVerticalSync();
    }

    while (1) {
        led.On();
        CTimer::SimpleMsDelay(100);

        led.Off();
        CTimer::SimpleMsDelay(100);
    }
    return ShutdownHalt;
}
