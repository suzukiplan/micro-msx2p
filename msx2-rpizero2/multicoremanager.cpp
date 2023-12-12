#include "multicoremanager.h"

// micro-msx2p
#include "msx2.hpp"
#include "roms.hpp"

extern int msxPad1;
extern uint16_t* hdmiBuffer;
extern int hdmiPitch;
extern CLogger* clogger;
static MSX2 msx2(MSX2_COLOR_MODE_RGB565);

#define DISPLAY_SIZE 568 * 240 * 2
static unsigned short displayBuffer[2][DISPLAY_SIZE / 2];
static short soundBuffer[2][4096];
static size_t soundBufferSize[2];
static int currentBuffer;
static int previousBuffer;
static bool msxTickEnd;

bool isMsxTickEnd() { return msxTickEnd; }

int16_t* getPreviousSoundBuffer(size_t* size)
{
    *size = soundBufferSize[previousBuffer];
    return soundBuffer[previousBuffer];
}

MultiCoreManager::MultiCoreManager(CMemorySystem* pMemorySystem) : CMultiCoreSupport(pMemorySystem)
{
    for (unsigned nCore = 0; nCore < CORES; nCore++) {
        coreStatus[nCore] = CoreStatus::Init;
    }
}

MultiCoreManager::~MultiCoreManager(void)
{
    for (unsigned nCore = 1; nCore < CORES; nCore++) {
        assert(coreStatus[nCore] == CoreStatus::Idle);
        coreStatus[nCore] = CoreStatus::Exit;
        while (coreStatus[nCore] == CoreStatus::Exit) {}
    }
}

boolean MultiCoreManager::Initialize(void)
{
    clogger->Write("MCM", LogNotice, "init multi-core-support");
    if (!CMultiCoreSupport::Initialize()) {
        return FALSE;
    }
    clogger->Write("MCM", LogNotice, "init MSX2+ core");
    msx2.setupSecondaryExist(false, false, false, true);
    msx2.setup(0, 0, 0, (void*)rom_cbios_main_msx2p, 0x8000, "MAIN");
    msx2.setup(0, 0, 4, (void*)rom_cbios_logo_msx2p, 0x4000, "LOGO");
    msx2.setup(3, 0, 0, (void*)rom_cbios_sub, 0x4000, "SUB");
    msx2.setupRAM(3, 3);
    msx2.setupKeyAssign(0, MSX2_JOY_S1, ' ');                              // start button: SPACE
    msx2.setupKeyAssign(0, MSX2_JOY_S2, 0x1B);                             // select button: ESC
    msx2.loadRom((void*)rom_game, sizeof(rom_game), MSX2_ROM_TYPE_NORMAL); // modify here if use mega rom
    currentBuffer = 0;
    previousBuffer = 1;
    clogger->Write("MCM", LogNotice, "wait for idle...");
    for (unsigned nCore = 1; nCore < 3; nCore++) {
        while (coreStatus[nCore] != CoreStatus::Idle) {
            ; // just wait
        }
    }
    msxTickEnd = true;
    return TRUE;
}

void MultiCoreManager::Run(unsigned nCore)
{
    assert(1 <= nCore && nCore < CORES);
    char buf[80];
    sprintf(buf, "cpu#%u idle", nCore);
    clogger->Write("MCM", LogNotice, buf);
    coreStatus[nCore] = CoreStatus::Idle;
    while (1) {
        ; // just wait interrupt
    }
}

void MultiCoreManager::IPIHandler(unsigned nCore, unsigned nIPI)
{
    if (nIPI == IPI_USER + 0) {
        // execute MSX core
        msxTickEnd = false;
        msx2.tick(msxPad1, 0, 0);
        memcpy(displayBuffer[currentBuffer], msx2.getDisplay(), DISPLAY_SIZE);
        void* pcm = msx2.getSound(&soundBufferSize[currentBuffer]);
        memcpy(soundBuffer[currentBuffer], pcm, soundBufferSize[currentBuffer]);
        previousBuffer = currentBuffer;
        currentBuffer++;
        currentBuffer &= 1;
        msxTickEnd = true;
        CMultiCoreSupport::SendIPI(2, IPI_USER + 1);
    } else if (nIPI == IPI_USER + 1) {
        // execute video buffering
        uint16_t* display = &displayBuffer[previousBuffer][0];
        uint16_t* hdmi = hdmiBuffer;
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
    }
}