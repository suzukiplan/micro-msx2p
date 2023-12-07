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
#include <vc4/sound/vchiqsoundbasedevice.h>

static uint16_t* hdmiBuffer;
static int hdmiPitch;

CKernel::CKernel(void) : screen(256, 192),
                         timer(&interrupt),
                         logger(options.GetLogLevel(), &timer),
                         vchiq(CMemorySystem::Get(), &interrupt),
                         sound(&vchiq, (TVCHIQSoundDestination)options.GetSoundOption())
{
    led.Blink(5); // show we are alive
}

CKernel::~CKernel(void)
{
}

boolean CKernel::initialize(void)
{
    boolean bOK = TRUE;
    led.On();

    if (bOK) {
        bOK = screen.Initialize();
    }

    if (bOK) {
        bOK = serial.Initialize(115200);
    }

    if (bOK) {
        CDevice* target = deviceNameService.GetDevice(options.GetLogDevice(), FALSE);
        if (target == 0) {
            target = &screen;
        }
        bOK = logger.Initialize(target);
    }

    if (bOK) {
        bOK = interrupt.Initialize();
    }

    if (bOK) {
        bOK = timer.Initialize();
    }

    if (bOK) {
        bOK = vchiq.Initialize();
    }

    if (bOK) {
        led.Off();
    }
    return bOK;
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
