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

CKernel::CKernel(void) :
#ifdef MSX2_DISPLAY_HALF_HORIZONTAL
                         screen(320, 240),
#else
                         screen(640, 480),
#endif
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
