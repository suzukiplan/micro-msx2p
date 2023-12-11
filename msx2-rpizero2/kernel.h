//
// kernel.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
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
#ifndef _kernel_h
#define _kernel_h

#include "multicoremanager.h"
#include <circle/actled.h>
#include <circle/devicenameservice.h>
#include <circle/exceptionhandler.h>
#include <circle/interrupt.h>
#include <circle/koptions.h>
#include <circle/logger.h>
#include <circle/sched/scheduler.h>
#include <circle/screen.h>
#include <circle/serial.h>
#include <circle/sound/soundbasedevice.h>
#include <circle/timer.h>
#include <circle/types.h>
#include <circle/usb/usbgamepad.h>
#include <circle/usb/usbhcidevice.h>
#include <vc4/sound/vchiqsoundbasedevice.h>
#include <vc4/sound/vchiqsounddevice.h>
#include <vc4/vchiq/vchiqdevice.h>

enum TShutdownMode {
    ShutdownNone,
    ShutdownHalt,
    ShutdownReboot
};

extern int msxPad1;

class CKernel
{
  public:
    CKernel(void);
    ~CKernel(void);

    boolean initialize(void);
    TShutdownMode run(void);

  private:
    // do not change this order
    CActLED led;
    CKernelOptions options;
    CDeviceNameService deviceNameService;
    CScreenDevice screen;
    CSerialDevice serial;
    CExceptionHandler exceptionHandler;
    CInterruptSystem interrupt;
    CTimer timer;
    CLogger logger;
    MultiCoreManager mcm;
    CUSBHCIDevice usb;
    CScheduler scheduler;
    CVCHIQDevice vchiq;
    CVCHIQSoundDevice sound;
    CUSBGamePadDevice* volatile gamePad;
};

#endif
