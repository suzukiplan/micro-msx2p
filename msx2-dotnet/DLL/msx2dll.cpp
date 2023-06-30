/**
 * micro MSX2+ - Shared Library
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
#ifndef _WIN32
#include <pthread.h>
#endif

#include <chrono>
#include <thread>
#include <map>
#include <string>
#include "msx2.hpp"
#include "sha1.hpp"
#include "msx2dll.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#define EXPORT extern "C" DLL_EXPORT

class Binary {
public:
    void *data;
    size_t size;

    Binary(const void *newData, size_t newSize) {
        this->data = nullptr;
        this->size = 0;
        this->setNewData(newData, newSize);
    }

    void replace(const void *newData, size_t newSize) {
        this->setNewData(newData, newSize);
    }

    ~Binary() {
        free(this->data);
    }

private:
    void setNewData(const void *newData, size_t newSize) {
        if (this->data) free(this->data);
        this->data = malloc(newSize);
        this->size = newSize;
        memcpy(this->data, newData, newSize);
    }
};

class Context {
public:
    MSX2* msx2;
    std::map<std::string, Binary *> bintray;
#ifdef _WIN32
    CRITICAL_SECTION cs;
#else
    pthread_mutex_t mutex{};
#endif

    Context(int colorCode) {
        msx2 = new MSX2(colorCode);
#ifdef _WIN32
        InitializeCriticalSection(&this->cs);
#else
        pthread_mutex_init(&this->mutex, nullptr);
#endif
    }
    
    ~Context() {
        delete msx2;
        for (std::pair<std::string, Binary *> bin: this->bintray) {
            delete bin.second;
        }
#ifdef _WIN32
        DeleteCriticalSection(&this->cs);
#else
        pthread_mutex_destroy(&this->mutex);
#endif
    }
    
    void addBinary(std::string &label, const void *data, size_t size) {
        if (this->bintray.find(label) == this->bintray.end()) {
            this->bintray[label] = new Binary(data, size);
        } else {
            this->bintray[label]->replace(data, size);
        }
    }

    void lock() {
#ifdef _WIN32
        EnterCriticalSection(&this->cs);
#else
        pthread_mutex_lock(&this->mutex);
#endif
    }

    void unlock() {
#ifdef _WIN32
        LeaveCriticalSection(&this->cs);
#else
        pthread_mutex_unlock(&this->mutex);
#endif
    }
};

EXPORT void* __stdcall msx2_createContext(int colorCode)
{
    return new Context(colorCode);
}

EXPORT void __stdcall msx2_releaseContext(void* context)
{
    delete (Context*)context;
}

EXPORT void __stdcall msx2_setupSecondaryExist(void* context,
                                               bool page0,
                                               bool page1,
                                               bool page2,
                                               bool page3)
{
    Context* c = (Context*)context;
    c->lock();
    c->msx2->setupSecondaryExist(page0, page1, page2, page3);
    c->unlock();
}

EXPORT void __stdcall msx2_setupRAM(void* context, int pri, int sec)
{
    Context* c = (Context*)context;
    c->lock();
    c->msx2->setupRAM(pri, sec);
    c->unlock();
}

EXPORT void __stdcall msx2_setup(void* context,
                                 int pri,
                                 int sec,
                                 int idx,
                                 const void* data,
                                 int size,
                                 const char* label)
{
    Context* c = (Context*)context;
    c->lock();
    std::string labelStr = label;
    c->addBinary(labelStr, data, size);
    c->msx2->setup(pri, sec, idx, c->bintray[labelStr]->data, size, label);
    c->unlock();
}

EXPORT void __stdcall msx2_loadFont(void* context, const void* font, int size)
{
    Context* c = (Context*)context;
    c->lock();
    std::string label = "KNJFNT16";
    c->addBinary(label, font, size);
    c->msx2->loadFont(c->bintray[label]->data, size);
    c->unlock();
}

EXPORT void __stdcall msx2_setupSpecialKeyCode(void* context, int select, int start)
{
    Context* c = (Context*)context;
    c->lock();
    c->msx2->setupKeyAssign(0, MSX2_JOY_S2, select);
    c->msx2->setupKeyAssign(0, MSX2_JOY_S1, start);
    c->unlock();
}

EXPORT void __stdcall msx2_tick(void* context, int pad1, int pad2, int key)
{
    Context* c = (Context*)context;
    c->lock();
    c->msx2->tick(pad1 & 0xFF, pad2 & 0xff, key & 0xFF);
    c->unlock();
}

EXPORT void __stdcall msx2_getDisplay(void* context, void* display)
{
    Context* c = (Context*)context;
    memcpy(display, c->msx2->getDisplay(), c->msx2->getDisplayWidth() * c->msx2->getDisplayHeight() * 2);
}

EXPORT int __stdcall msx2_getDisplayWidth(void* context)
{
    return ((Context*)context)->msx2->getDisplayWidth();
}

EXPORT int __stdcall msx2_getDisplayHeight(void* context)
{
    return ((Context*)context)->msx2->getDisplayHeight();
}

EXPORT int __stdcall msx2_getCurrentSoundSize(void* context)
{
    return (int)((Context*)context)->msx2->getCurrentSoundSize();
}

EXPORT void __stdcall msx2_getSound(void* context, void* sound)
{
    size_t sz;
    const void* result = ((Context*)context)->msx2->getSound(&sz);
    memcpy(sound, result, sz);
}

EXPORT void __stdcall msx2_loadRom(void* context, const void* rom, int size, int romType)
{
    Context* c = (Context*)context;
    c->lock();
    std::string label = "CART";
    c->addBinary(label, rom, size);
    c->msx2->loadRom(c->bintray[label]->data, size, romType);
    c->unlock();
}

EXPORT void __stdcall msx2_ejectRom(void* context)
{
    Context* c = (Context*)context;
    c->lock();
    c->msx2->ejectRom();
    c->unlock();
}

EXPORT void __stdcall msx2_insertDisk(void* context,
                                      int driveId,
                                      const void* disk,
                                      int size,
                                      bool readOnly)
{
    char base64[SHA1_BASE64_SIZE];
    sha1("DiskImage:")
        .add(disk, size)
        .finalize()
        .print_base64(base64);
    std::string label = base64;
    Context* c = (Context*)context;
    c->lock();
    c->addBinary(label, disk, size);
    c->msx2->insertDisk(driveId, c->bintray[label], size, readOnly);
    c->unlock();
}

EXPORT void __stdcall msx2_ejectDisk(void* context, int driveId)
{
    Context* c = (Context*)context;
    c->lock();
    c->msx2->ejectDisk(driveId);
    c->unlock();
}

EXPORT int __stdcall msx2_getQuickSaveSize(void* context)
{
    Context* c = (Context*)context;
    c->lock();
    size_t sz;
    c->msx2->quickSave(&sz);
    c->unlock();
    return (int)sz;
}

EXPORT const void* __stdcall msx2_quickSave(void* context)
{
    Context* c = (Context*)context;
    c->lock();
    size_t sz;
    const void* result = c->msx2->quickSave(&sz);
    c->unlock();
    return result;
}

EXPORT void __stdcall msx2_quickLoad(void* context, const void* save, int size)
{
    Context* c = (Context*)context;
    c->lock();
    c->msx2->quickLoad(save, size);
    c->unlock();
}

EXPORT void __stdcall msx2_reset(void* context)
{
    Context* c = (Context*)context;
    c->lock();
    c->msx2->reset();
    c->unlock();
}
