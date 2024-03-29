/**
 * micro MSX2+ - Wrapper Library for C
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
#include <pthread.h>
#include <chrono>
#include <thread>
#include <map>
#include <string>
#include "msx2.hpp"
#include "sha1.hpp"
#include "msx2gw.h"

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
    pthread_mutex_t mutex{};

    Context(int colorCode) {
        msx2 = new MSX2(colorCode);
        pthread_mutex_init(&this->mutex, nullptr);
    }
    
    ~Context() {
        delete msx2;
        for (std::pair<std::string, Binary *> bin: this->bintray) {
            delete bin.second;
        }
    }
    
    void addBinary(std::string &label, const void *data, size_t size) {
        if (this->bintray.find(label) == this->bintray.end()) {
            this->bintray[label] = new Binary(data, size);
        } else {
            this->bintray[label]->replace(data, size);
        }
    }

    void lock() {
        pthread_mutex_lock(&this->mutex);
    }

    void unlock() {
        pthread_mutex_unlock(&this->mutex);
    }
};

extern "C" void* msx2_createContext(int colorCode)
{
    return new Context(colorCode);
}

extern "C" void msx2_releaseContext(void* context)
{
    delete (Context*)context;
}

extern "C" void msx2_setupSecondaryExist(void* context,
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

extern "C" void msx2_setupRAM(void* context, int pri, int sec)
{
    Context* c = (Context*)context;
    c->lock();
    c->msx2->setupRAM(pri, sec);
    c->unlock();
}

extern "C" void msx2_setup(void* context,
                           int pri,
                           int sec,
                           int idx,
                           const void* data,
                           size_t size,
                           const char* label)
{
    Context* c = (Context*)context;
    c->lock();
    std::string labelStr = label;
    c->addBinary(labelStr, data, size);
    c->msx2->setup(pri, sec, idx, c->bintray[labelStr]->data, (int)size, label);
    c->unlock();
}

extern "C" void msx2_loadFont(void* context, const void* font, size_t size)
{
    Context* c = (Context*)context;
    c->lock();
    std::string label = "KNJFNT16";
    c->addBinary(label, font, size);
    c->msx2->loadFont(c->bintray[label]->data, size);
    c->unlock();
}

extern "C" void msx2_setupSpecialKeyCode(void* context, int select, int start)
{
    Context* c = (Context*)context;
    c->lock();
    c->msx2->setupKeyAssign(0, MSX2_JOY_S2, select);
    c->msx2->setupKeyAssign(0, MSX2_JOY_S1, start);
    c->unlock();
}

extern "C" void msx2_tick(void* context, int pad1, int pad2, int key)
{
    Context* c = (Context*)context;
    c->lock();
    c->msx2->tick(pad1 & 0xFF, pad2 & 0xff, key & 0xFF);
    c->unlock();
}

extern "C" unsigned short* msx2_getDisplay(void* context)
{
    return ((Context*)context)->msx2->getDisplay();
}

extern "C" int msx2_getDisplayWidth(void* context)
{
    return ((Context*)context)->msx2->getDisplayWidth();
}

extern "C" int msx2_getDisplayHeight(void* context)
{
    return ((Context*)context)->msx2->getDisplayHeight();
}

extern "C" const void* msx2_getSound(void* context, size_t* size)
{
    return ((Context*)context)->msx2->getSound(size);
}

extern "C" void msx2_loadRom(void* context, const void* rom, size_t size, int romType)
{
    Context* c = (Context*)context;
    c->lock();
    std::string label = "CART";
    c->addBinary(label, rom, size);
    c->msx2->loadRom(c->bintray[label]->data, (int)size, romType);
    c->unlock();
}

extern "C" void msx2_ejectRom(void* context)
{
    Context* c = (Context*)context;
    c->lock();
    c->msx2->ejectRom();
    c->unlock();
}

extern "C" void msx2_insertDisk(void* context,
                                int driveId,
                                const void* disk,
                                size_t size,
                                bool readOnly)
{
    char base64[SHA1_BASE64_SIZE];
    sha1("DiskImage:")
        .add(disk, (int)size)
        .finalize()
        .print_base64(base64);
    std::string label = base64;
    Context* c = (Context*)context;
    c->lock();
    c->addBinary(label, disk, size);
    c->msx2->insertDisk(driveId, c->bintray[label], size, readOnly);
    c->unlock();
}

extern "C" void msx2_ejectDisk(void* context, int driveId)
{
    Context* c = (Context*)context;
    c->lock();
    c->msx2->ejectDisk(driveId);
    c->unlock();
}

extern "C" const void* msx2_quickSave(void* context, size_t* size)
{
    Context* c = (Context*)context;
    c->lock();
    const void* result = c->msx2->quickSave(size);
    c->unlock();
    return result;
}

extern "C" void msx2_quickLoad(void* context, const void* save, size_t size)
{
    Context* c = (Context*)context;
    c->lock();
    c->msx2->quickLoad(save, size);
    c->unlock();
}

extern "C" void msx2_reset(void* context)
{
    Context* c = (Context*)context;
    c->lock();
    c->msx2->reset();
    c->unlock();
}
