#include "msx2.hpp"
#include "multicoremanager.h"
#include "roms.hpp"

extern int msxPad1;
extern uint16_t* hdmiBuffer;
extern int hdmiPitch;
extern CVCHIQSoundDevice* hdmiSoundDevice;
static MSX2 msx2(MSX2_COLOR_MODE_RGB565);

#define DISPLAY_SIZE 568 * 240 * 2
static unsigned short displayBuffer[2][DISPLAY_SIZE / 2];
static short soundBuffer[2][65536];
static size_t soundBufferSize[2];
static int pcmData32[65536];
static int currentBuffer;
static int previousBuffer;

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
    if (!CMultiCoreSupport::Initialize()) {
        return FALSE;
    }
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
    for (unsigned nCore = 1; nCore < CORES; nCore++) {
        while (coreStatus[nCore] != CoreStatus::Idle) {
            ; // just wait
        }
    }
    return TRUE;
}

void MultiCoreManager::Run(unsigned nCore)
{
    assert(1 <= nCore && nCore < CORES);
    coreStatus[nCore] = CoreStatus::Idle;
    while (1) {
        ; // just wait interrupt
    }
}

void MultiCoreManager::IPIHandler(unsigned nCore, unsigned nIPI)
{
    if (nIPI == IPI_USER + 0) {
        // execute MSX core
        msx2.tick(msxPad1, 0, 0);
        memcpy(displayBuffer[currentBuffer], msx2.getDisplay(), DISPLAY_SIZE);
        void* pcm = msx2.getSound(&soundBufferSize[currentBuffer]);
        memcpy(soundBuffer[currentBuffer], pcm, soundBufferSize[currentBuffer]);
        previousBuffer = currentBuffer;
        currentBuffer++;
        currentBuffer &= 1;
        CMultiCoreSupport::SendIPI(2, IPI_USER + 1); // execute video hal at core2
        CMultiCoreSupport::SendIPI(3, IPI_USER + 2); // execute sound hal at core3
    } else if (nIPI == IPI_USER + 1) {
        // execute video hal
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
    } else if (nIPI == IPI_USER + 2) {
        // execute sound hal
        const int ptr = previousBuffer;
        int16_t* pcmData = &soundBuffer[ptr][0];
        size_t pcmSize = soundBufferSize[ptr];
        for (size_t i = 0; i < pcmSize / 2; i++) {
            pcmData32[i] = (int)pcmData[i] * 256;
        }
        if (hdmiSoundDevice) {
            while (hdmiSoundDevice->PlaybackActive()) {
                ;
            }
            hdmiSoundDevice->Playback(pcmData32, pcmSize / 2, 2, 24);
        }
    }
}