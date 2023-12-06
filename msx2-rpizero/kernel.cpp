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
#include <circle/timer.h>

CKernel::CKernel(void) : screen(640, 480)
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
    // draw rectangle on screen
    for (unsigned nPosX = 0; nPosX < screen.GetWidth(); nPosX++) {
        screen.SetPixel(nPosX, 0, NORMAL_COLOR);
        screen.SetPixel(nPosX, screen.GetHeight() - 1, NORMAL_COLOR);
    }
    for (unsigned nPosY = 0; nPosY < screen.GetHeight(); nPosY++) {
        screen.SetPixel(0, nPosY, NORMAL_COLOR);
        screen.SetPixel(screen.GetWidth() - 1, nPosY, NORMAL_COLOR);
    }

    // draw cross on screen
    for (unsigned nPosX = 0; nPosX < screen.GetWidth(); nPosX++) {
        unsigned nPosY = nPosX * screen.GetHeight() / screen.GetWidth();

        screen.SetPixel(nPosX, nPosY, NORMAL_COLOR);
        screen.SetPixel(screen.GetWidth() - nPosX - 1, nPosY, NORMAL_COLOR);
    }

    while (1) {
        led.On();
        CTimer::SimpleMsDelay(100);

        led.Off();
        CTimer::SimpleMsDelay(100);
    }

    return ShutdownHalt;
}
