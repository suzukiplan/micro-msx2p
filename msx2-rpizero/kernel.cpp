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
#include "msx2def.h"

#define TAG "kernel"
int msxPad1 = 0;

CKernel::CKernel(void) :
#ifdef MSX2_DISPLAY_HALF_HORIZONTAL
                         screen(320, 240),
#else
                         screen(640, 480),
#endif
                         timer(&interrupt),
                         logger(options.GetLogLevel(), &timer),
                         usb(&interrupt, &timer, TRUE),
                         vchiq(CMemorySystem::Get(), &interrupt),
                         sound(&vchiq, (TVCHIQSoundDestination)options.GetSoundOption()),
                         gamePad(nullptr)
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
        logger.Write(TAG, LogNotice, "init interrupt");
        bOK = interrupt.Initialize();
    }

    if (bOK) {
        logger.Write(TAG, LogNotice, "init timer");
        bOK = timer.Initialize();
    }

    if (bOK) {
        logger.Write(TAG, LogNotice, "init vchiq");
        bOK = vchiq.Initialize();
    }

    if (bOK) {
        logger.Write(TAG, LogNotice, "init usb");
        bOK = usb.Initialize();
    }

    if (bOK) {
        led.Off();
    }
    return bOK;
}

void CKernel::updateUsbStatus(void)
{
    if (usb.UpdatePlugAndPlay()) {
        if (!gamePad) {
            gamePad = (CUSBGamePadDevice*)deviceNameService.GetDevice("upad1", FALSE);
            if (gamePad) {
                gamePad->RegisterStatusHandler([](unsigned index, const TGamePadState* state) {
                    msxPad1 = 0;
                    msxPad1 |= state->axes[0].value == state->axes[0].minimum ? MSX2_JOY_LE : 0;
                    msxPad1 |= state->axes[0].value == state->axes[0].maximum ? MSX2_JOY_RI : 0;
                    msxPad1 |= state->axes[1].value == state->axes[1].minimum ? MSX2_JOY_UP : 0;
                    msxPad1 |= state->axes[1].value == state->axes[1].maximum ? MSX2_JOY_DW : 0;
                    msxPad1 |= (state->buttons & 0x0001) ? MSX2_JOY_T2 : 0;
                    msxPad1 |= (state->buttons & 0x0002) ? MSX2_JOY_T1 : 0;
                    msxPad1 |= (state->buttons & 0x0100) ? MSX2_JOY_S2 : 0;
                    msxPad1 |= (state->buttons & 0x0200) ? MSX2_JOY_S1 : 0;
                });
            }
        } else {
            if (!(CUSBGamePadDevice*)deviceNameService.GetDevice("upad1", FALSE)) {
                gamePad = nullptr;
                msxPad1 = 0;
            }
        }
    }
}