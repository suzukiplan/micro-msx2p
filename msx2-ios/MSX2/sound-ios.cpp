/*
 * micro MSX2+ - Sound Hardware Abstraction Layer for AudioQueue
 * -----------------------------------------------------------------------------
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Yoji Suzuki.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -----------------------------------------------------------------------------
 */
#include "BufferQueue.h"
#include "sound-ios.h"
#include <AudioToolbox/AudioQueue.h>
#include <CoreAudio/CoreAudioTypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NUM_CHANNELS 2
#define NUM_BUFFERS 3
#define BUFFER_SIZE 4096
#define SAMPLE_TYPE short
#define MAX_NUMBER 32767
#define SAMPLE_RATE 44100

struct Context {
    pthread_mutex_t mutex;
    AudioStreamBasicDescription format;
    AudioQueueRef queue;
    AudioQueueBufferRef buffers[NUM_BUFFERS];
    BufferQueue* bq;
};

static void callback(void* context, AudioQueueRef queue, AudioQueueBufferRef buffer)
{
    struct Context* c = (struct Context*)context;
    pthread_mutex_lock(&c->mutex);
    while (c->bq->getCursor() < BUFFER_SIZE) {
        memset(buffer->mAudioData, 0, BUFFER_SIZE);
        pthread_mutex_unlock(&c->mutex);
        AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
        return;
    }
    void* qbuf;
    size_t qsize = BUFFER_SIZE;
    c->bq->dequeue(&qbuf, &qsize, BUFFER_SIZE);
    memcpy(buffer->mAudioData, qbuf, BUFFER_SIZE);
    pthread_mutex_unlock(&c->mutex);
    AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
}

extern "C" void* sound_create(void)
{
    struct Context* result = (struct Context*)malloc(sizeof(struct Context));
    if (!result) return NULL;
    memset(result, 0, sizeof(struct Context));
    result->bq = new BufferQueue(65536);
    pthread_mutex_init(&result->mutex, NULL);
    result->format.mSampleRate = SAMPLE_RATE;
    result->format.mFormatID = kAudioFormatLinearPCM;
    result->format.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    result->format.mBitsPerChannel = 8 * sizeof(SAMPLE_TYPE);
    result->format.mChannelsPerFrame = NUM_CHANNELS;
    result->format.mBytesPerFrame = sizeof(SAMPLE_TYPE) * NUM_CHANNELS;
    result->format.mFramesPerPacket = 1;
    result->format.mBytesPerPacket = result->format.mBytesPerFrame * result->format.mFramesPerPacket;
    result->format.mReserved = 0;
    AudioQueueNewOutput(&result->format, callback, result, CFRunLoopGetCurrent(), kCFRunLoopCommonModes, 0, &result->queue);
    for (int i = 0; i < NUM_BUFFERS; i++) {
        AudioQueueAllocateBuffer(result->queue, BUFFER_SIZE, &result->buffers[i]);
        result->buffers[i]->mAudioDataByteSize = BUFFER_SIZE;
        memset(result->buffers[i]->mAudioData, 0, BUFFER_SIZE);
        AudioQueueEnqueueBuffer(result->queue, result->buffers[i], 0, NULL);
    }
    AudioQueueStart(result->queue, NULL);
    return result;
}

extern "C" void sound_destroy(void* context)
{
    struct Context* c = (struct Context*)context;
    pthread_mutex_lock(&c->mutex);
    AudioQueueStop(c->queue, false);
    AudioQueueDispose(c->queue, false);
    delete c->bq;
    c->bq = NULL;
    pthread_mutex_unlock(&c->mutex);
    pthread_mutex_destroy(&c->mutex);
    free(c);
}

extern "C" void sound_enqueue(void* context, const void* buffer, size_t size)
{
    struct Context* c = (struct Context*)context;
    if (!c->bq) return;
    pthread_mutex_lock(&c->mutex);
    if (c->bq && c->bq->getCursor() < BUFFER_SIZE * 2) c->bq->enqueue(buffer, size);
    pthread_mutex_unlock(&c->mutex);
}
