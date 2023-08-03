/* (C)2016, SUZUKI PLAN.
 *----------------------------------------------------------------------------
 * Description: VGS - Sound Processing Unit for OpenAL
 *    Platform: iOS and Mac OS X
 *      Author: Yoji Suzuki (SUZUKI PLAN)
 *----------------------------------------------------------------------------
 */
#include "vgsspu_al.h"
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#define kOutputBus 0
#define kInputBus 1
#define BUFNUM 2

typedef ALvoid AL_APIENTRY (*alBufferDataStaticProcPtr)(const ALint bid, ALenum format, ALvoid* data, ALsizei size, ALsizei freq);

struct AL {
    ALCdevice* sndDev;
    ALCcontext* sndCtx;
    ALuint sndABuf;
    ALuint sndASrc;
    alBufferDataStaticProcPtr alBufferDataStaticProc;
};

struct SPU {
    struct AL al;
    volatile int alive;
    void* buffer;
    void (*callback)(void* buffer, size_t size);
    pthread_t tid;
    size_t size;
    int sampling;
    int bit;
    int ch;
    int format;
};

static int get_format(int bit, int ch);
static void* sound_thread(void* context);
static int init_al(struct AL* al);
static void term_al(struct AL* al);

void* vgsspu_start(void (*callback)(void* buffer, size_t size))
{
    return vgsspu_start2(22050, 16, 1, 1470, callback);
}

void* vgsspu_start2(int sampling, int bit, int ch, size_t size, void (*callback)(void* buffer, size_t size))
{
    struct SPU* result;
    int format;
    format = get_format(bit, ch);
    if (-1 == format) {
        return NULL;
    }
    result = (struct SPU*)malloc(sizeof(struct SPU));
    if (NULL == result) {
        return NULL;
    }
    memset(result, 0, sizeof(struct SPU));
    result->buffer = malloc(size);
    if (NULL == result->buffer) {
        free(result);
        return NULL;
    }
    result->sampling = sampling;
    result->bit = bit;
    result->ch = ch;
    result->format = format;
    result->callback = callback;
    result->size = size;

    if (init_al(&result->al)) {
        free(result->buffer);
        free(result);
        return NULL;
    }

    result->alive = 1;
    if (pthread_create(&result->tid, NULL, sound_thread, result)) {
        free(result->buffer);
        free(result);
        return NULL;
    }
    return result;
}

void vgsspu_end(void* context)
{
    struct SPU* c = (struct SPU*)context;
    c->alive = 0;
    pthread_join(c->tid, NULL);
    term_al(&c->al);
    free(c->buffer);
    free(c);
}

static int get_format(int bit, int ch)
{
    if (1 == ch) {
        if (8 == bit) {
            return AL_FORMAT_MONO8;
        }
        if (16 == bit) {
            return AL_FORMAT_MONO16;
        }
    } else if (2 == ch) {
        if (8 == bit) {
            return AL_FORMAT_STEREO8;
        }
        if (16 == bit) {
            return AL_FORMAT_STEREO16;
        }
    }
    return -1;
}

static void* sound_thread(void* context)
{
    struct SPU* c = (struct SPU*)context;
    ALint st;

    memset(c->buffer, 0, c->size);

    while (c->alive) {
        alGetSourcei(c->al.sndASrc, AL_BUFFERS_QUEUED, &st);
        if (st < BUFNUM) {
            alGenBuffers(1, &c->al.sndABuf);
        } else {
            alGetSourcei(c->al.sndASrc, AL_SOURCE_STATE, &st);
            if (st != AL_PLAYING) {
                alSourcePlay(c->al.sndASrc);
            }
            while ((void)(alGetSourcei(c->al.sndASrc, AL_BUFFERS_PROCESSED, &st)), st == 0) {
                usleep(1000);
            }
            alSourceUnqueueBuffers(c->al.sndASrc, 1, &c->al.sndABuf);
            alDeleteBuffers(1, &c->al.sndABuf);
            alGenBuffers(1, &c->al.sndABuf);
        }
        c->callback(c->buffer, c->size);
        alBufferData(c->al.sndABuf, c->format, c->buffer, (int)c->size, c->sampling);
        alSourceQueueBuffers(c->al.sndASrc, 1, &c->al.sndABuf);
    }
    do {
        usleep(1000);
        alGetSourcei(c->al.sndASrc, AL_SOURCE_STATE, &st);
    } while (st == AL_PLAYING);
    return NULL;
}

static int init_al(struct AL* al)
{
    al->sndDev = alcOpenDevice(NULL);
    if (NULL == al->sndDev) {
        term_al(al);
        return -1;
    }
    al->sndCtx = alcCreateContext(al->sndDev, NULL);
    if (NULL == al->sndCtx) {
        term_al(al);
        return -1;
    }
    if (!alcMakeContextCurrent(al->sndCtx)) {
        term_al(al);
        return -1;
    }
    al->alBufferDataStaticProc = (alBufferDataStaticProcPtr)alcGetProcAddress(NULL, (const ALCchar*)"alBufferDataStatic");

    alGenSources(1, &(al->sndASrc));
    return 0;
}

static void term_al(struct AL* al)
{
    if (al->sndCtx) {
        alcDestroyContext(al->sndCtx);
        al->sndCtx = NULL;
    }
    if (al->sndDev) {
        alcCloseDevice(al->sndDev);
        al->sndDev = NULL;
    }
}
