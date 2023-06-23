#include <pthread.h>
#include <jni.h>
#include <unistd.h>
#include <android/bitmap.h>
#include "core/msx2.hpp"
#include "audio.hpp"
#include <chrono>
#include <thread>

static MSX2 *msx2 = nullptr;

static struct BIOS {
    unsigned char main[0x8000];
    unsigned char logo[0x4000];
    unsigned char sub[0x4000];
} bios;

static struct ROM {
    void *data;
    size_t size;
} rom;

static AudioSystem *audioSystem = nullptr;
static unsigned char audioBuffer[65536];
static volatile size_t audioBufferSize = 0;
static pthread_mutex_t audioMutex = PTHREAD_MUTEX_INITIALIZER;

static void clearRomData() {
    if (rom.data) {
        free(rom.data);
        memset(&rom, 0, sizeof(rom));
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_suzukiplan_msx2_1android_JNI_init(JNIEnv *env,
                                           jclass,
                                           jbyteArray main,
                                           jbyteArray logo,
                                           jbyteArray sub) {
    jbyte *mainRaw = env->GetByteArrayElements(main, nullptr);
    jbyte *logoRaw = env->GetByteArrayElements(logo, nullptr);
    jbyte *subRaw = env->GetByteArrayElements(sub, nullptr);
    memcpy(bios.main, mainRaw, 0x8000);
    memcpy(bios.logo, logoRaw, 0x4000);
    memcpy(bios.sub, subRaw, 0x4000);
    env->ReleaseByteArrayElements(sub, subRaw, 0);
    env->ReleaseByteArrayElements(logo, logoRaw, 0);
    env->ReleaseByteArrayElements(main, mainRaw, 0);
    msx2 = new MSX2(MSX2_COLOR_MODE_RGB565);
    msx2->setupSecondaryExist(false, false, false, true);
    msx2->setup(0, 0, 0, bios.main, 0x8000, "MAIN");
    msx2->setup(0, 0, 4, bios.logo, 0x4000, "LOGO");
    msx2->setup(3, 0, 0, bios.sub, 0x4000, "SUB");
    msx2->setupRAM(3, 3);
    msx2->setupKeyAssign(0, MSX2_JOY_S1, ' ');
    msx2->setupKeyAssign(0, MSX2_JOY_S2, 0x1B);
    audioSystem = new AudioSystem(44100, 16, 2, [](char *buf, size_t size) {
        if (audioBufferSize < AUDIO_BUFFER_SIZE) {
            memset(buf, 0, size);
        } else {
            pthread_mutex_lock(&audioMutex);
            if (msx2 && AUDIO_BUFFER_SIZE <= audioBufferSize) {
                memcpy(buf, audioBuffer, AUDIO_BUFFER_SIZE);
                audioBufferSize -= AUDIO_BUFFER_SIZE;
                memmove(audioBuffer, &audioBuffer[AUDIO_BUFFER_SIZE], audioBufferSize);
            }
            pthread_mutex_unlock(&audioMutex);
        }
    });
}

extern "C"
JNIEXPORT void JNICALL
Java_com_suzukiplan_msx2_1android_JNI_term(JNIEnv *, jclass) {
    delete msx2;
    msx2 = nullptr;
    clearRomData();
    delete audioSystem;
    audioSystem = nullptr;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_suzukiplan_msx2_1android_JNI_tick(JNIEnv *env,
                                           jclass clazz,
                                           jint pad,
                                           jobject vram) {
    if (!msx2) return;
    unsigned short *pixels;
    if ((pad & 0xF0) == 0xF0) {
        msx2->reset();
    } else {
        msx2->tick(pad, 0, 0);
    }
    unsigned short *display = msx2->getDisplay();
    if (AndroidBitmap_lockPixels(env, vram, (void **) &pixels) < 0) return;
    int index = 0;
    for (int y = 0; y < 240; y++) {
        for (int x = 0; x < 568; x++) {
            pixels[index] = display[index];
            index++;
        }
    }
    AndroidBitmap_unlockPixels(env, vram);
    size_t soundSize;
    void *soundBuf = msx2->getSound(&soundSize);
    pthread_mutex_lock(&audioMutex);
    if (audioBufferSize + soundSize < sizeof(audioBuffer)) {
        memcpy(&audioBuffer[audioBufferSize], soundBuf, soundSize);
        audioBufferSize += soundSize;
    }
    pthread_mutex_unlock(&audioMutex);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_suzukiplan_msx2_1android_JNI_loadRom(JNIEnv *env, jclass, jbyteArray romSrc) {
    clearRomData();
    jbyte *romRaw = env->GetByteArrayElements(romSrc, nullptr);
    size_t size = env->GetArrayLength(romSrc);
    rom.data = malloc(size);
    if (rom.data) {
        memcpy(rom.data, romRaw, size);
        rom.size = size;
    }
    env->ReleaseByteArrayElements(romSrc, romRaw, 0);
    msx2->loadRom(rom.data, (int) rom.size, MSX2_ROM_TYPE_NORMAL);
}