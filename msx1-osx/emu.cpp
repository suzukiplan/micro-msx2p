#include "emu.h"
#include "msx1.hpp"
#include "vgsspu_al.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define PLAYLOG_MAX_TICK_COUNT 655360

extern "C" {
unsigned short emu_vram[VRAM_WIDTH * VRAM_HEIGHT];
unsigned char emu_key;
unsigned char emu_keycode;
};

static unsigned char ram[0x4000];
static TMS9918A::Context vram;
static MSX1 msx1(TMS9918A::ColorMode::RGB555, ram, sizeof(ram), &vram, [](void* arg, int frame, int line, unsigned short* display) {
    memcpy(&emu_vram[line * VRAM_WIDTH], display, VRAM_WIDTH * 2);
});
static unsigned char* rom;
static void* spu;
pthread_mutex_t sound_locker;
static short sound_buffer[65536];
static int sound_cursor;

extern "C" void emu_init_bios(const void* main,
                              const void* logo)
{
    static struct BIOS {
        unsigned char main[0x8000];
        unsigned char logo[0x4000];
    } bios;
    memcpy(bios.main, main, sizeof(bios.main));
    msx1.setup(0, 0, bios.main, sizeof(bios.main), "MAIN");
    if (logo) {
        memcpy(bios.logo, logo, sizeof(bios.logo));
        msx1.setup(0, 4, bios.logo, sizeof(bios.logo), "LOGO");
    }
    emu_reset();
}

void emu_loadRom(const void* rom_, size_t romSize, const char* fileName)
{
    if (rom) free(rom);
    rom = (unsigned char*)malloc(romSize);
    memcpy(rom, rom_, romSize);
    int type = MSX1_ROM_TYPE_NORMAL;
    printf("load game: %s (type:%d)\n", fileName, type);
    msx1.loadRom(rom, (int)romSize, type);
    emu_reset();
}

extern "C" void emu_reset()
{
    msx1.reset();
    msx1.psg.setVolume(4);
}

static void sound_callback(void* buffer, size_t size)
{
    pthread_mutex_lock(&sound_locker);
    while (sound_cursor < size / 2) {
        pthread_mutex_unlock(&sound_locker);
        usleep(1000);
        pthread_mutex_lock(&sound_locker);
    }
    memcpy(buffer, sound_buffer, size);
    memmove(sound_buffer, &sound_buffer[size / 2], sizeof(sound_buffer) - size);
    sound_cursor -= size / 2;
    pthread_mutex_unlock(&sound_locker);
}

static void tick(unsigned char pad1, unsigned char pad2, unsigned char key)
{
    msx1.tick(pad1, pad2, key);
}

extern "C" void emu_vsync()
{
    static bool initialized = false;
    if (!initialized) {
        pthread_mutex_init(&sound_locker, NULL);
        spu = vgsspu_start2(44100, 16, 1, 8192, sound_callback);
        initialized = true;
    }
    tick(emu_key, 0, emu_keycode);
    size_t pcmSize;
    auto pcm = msx1.getSound(&pcmSize);
    pthread_mutex_lock(&sound_locker);
    memcpy(&sound_buffer[sound_cursor], pcm, pcmSize);
    sound_cursor += pcmSize / 2;
    pthread_mutex_unlock(&sound_locker);
}

extern "C" void emu_destroy()
{
}
