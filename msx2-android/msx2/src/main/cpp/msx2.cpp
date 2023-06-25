#include <pthread.h>
#include <jni.h>
#include <unistd.h>
#include <android/bitmap.h>
#include "core/msx2.hpp"
#include "audio.hpp"
#include <chrono>
#include <thread>
#include <map>
#include <string>

class Binary {
public:
    void *data;
    size_t size;

    Binary(void *newData, size_t newSize) {
        this->data = nullptr;
        this->size = 0;
        this->setNewData(newData, newSize);
    }

    void replace(void *newData, size_t newSize) {
        this->setNewData(newData, newSize);
    }

    ~Binary() {
        free(this->data);
    }

private:
    void setNewData(void *newData, size_t newSize) {
        if (this->data) free(this->data);
        this->data = malloc(newSize);
        this->size = newSize;
        memcpy(this->data, newData, newSize);
    }
};

class Context {
public:
    MSX2 *msx2;
    std::map<std::string, Binary *> bios;
    Binary *rom;
    pthread_mutex_t mutex{};
    pthread_mutex_t audioMutex{};
    unsigned char audioBuffer[65536]{};
    size_t audioBufferSize;
    AudioSystem *audio;

    Context() {
        this->msx2 = new MSX2(MSX2_COLOR_MODE_RGB565);
        this->rom = nullptr;
        pthread_mutex_init(&this->mutex, nullptr);
        pthread_mutex_init(&this->audioMutex, nullptr);
        memset(this->audioBuffer, 0, sizeof(this->audioBuffer));
        this->audioBufferSize = 0;
        this->audio = new AudioSystem(this, 44100, 16, 2, [](void *ctx, char *buf, size_t size) {
            auto c = (Context *) ctx;
            if (c->audioBufferSize < AUDIO_BUFFER_SIZE) {
                memset(buf, 0, size);
            } else {
                pthread_mutex_lock(&c->audioMutex);
                if (c->msx2 && AUDIO_BUFFER_SIZE <= c->audioBufferSize) {
                    memcpy(buf, c->audioBuffer, AUDIO_BUFFER_SIZE);
                    c->audioBufferSize -= AUDIO_BUFFER_SIZE;
                    memmove(c->audioBuffer, &c->audioBuffer[AUDIO_BUFFER_SIZE], c->audioBufferSize);
                }
                pthread_mutex_unlock(&c->audioMutex);
            }
        });
    }

    ~Context() {
        delete this->audio;
        delete this->msx2;
        delete this->rom;
        for (std::pair<std::string, Binary *> bin: this->bios) {
            delete bin.second;
        }
        pthread_mutex_destroy(&this->audioMutex);
        pthread_mutex_destroy(&this->mutex);
    }

    void addBios(std::string &label, void *data, size_t size) {
        if (this->bios.find(label) == this->bios.end()) {
            this->bios[label] = new Binary(data, size);
        } else {
            this->bios[label]->replace(data, size);
        }
    }

    void lock() {
        pthread_mutex_lock(&this->mutex);
    }

    void unlock() {
        pthread_mutex_unlock(&this->mutex);
    }
};

extern "C"
JNIEXPORT jlong JNICALL
Java_com_suzukiplan_msx2_Core_init(JNIEnv *, jclass) {
    auto context = new Context();
    return (jlong) context;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_suzukiplan_msx2_Core_term(JNIEnv *, jclass, jlong context) {
    delete (Context *) context;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_suzukiplan_msx2_Core_setupSecondaryExist(JNIEnv *,
                                                  jclass,
                                                  jlong context,
                                                  jboolean page0,
                                                  jboolean page1,
                                                  jboolean page2,
                                                  jboolean page3) {
    ((Context *) context)->lock();
    ((Context *) context)->msx2->setupSecondaryExist(page0, page1, page2, page3);
    ((Context *) context)->unlock();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_suzukiplan_msx2_Core_setupRAM(JNIEnv *,
                                       jclass,
                                       jlong context,
                                       jint pri,
                                       jint sec) {
    ((Context *) context)->lock();
    ((Context *) context)->msx2->setupRAM(pri, sec);
    ((Context *) context)->unlock();
}
extern "C"
JNIEXPORT void JNICALL
Java_com_suzukiplan_msx2_Core_setup(JNIEnv *env,
                                    jclass,
                                    jlong context,
                                    jint pri,
                                    jint sec,
                                    jint idx,
                                    jbyteArray data,
                                    jstring label) {
    ((Context *) context)->lock();
    jbyte *dataRaw = env->GetByteArrayElements(data, nullptr);
    size_t dataSize = env->GetArrayLength(data);
    const char *labelRaw = env->GetStringUTFChars(label, nullptr);
    std::string labelString = labelRaw;
    auto ctx = (Context *) context;
    ctx->addBios(labelString, dataRaw, dataSize);
    ctx->msx2->setup(pri, sec, idx,
                     ctx->bios[labelString]->data,
                     (int) ctx->bios[labelString]->size,
                     labelRaw);
    env->ReleaseStringUTFChars(label, labelRaw);
    env->ReleaseByteArrayElements(data, dataRaw, 0);
    ((Context *) context)->unlock();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_suzukiplan_msx2_Core_loadFont(JNIEnv *env,
                                       jclass,
                                       jlong context,
                                       jbyteArray font) {
    ((Context *) context)->lock();
    jbyte *fontRaw = env->GetByteArrayElements(font, nullptr);
    size_t fontSize = env->GetArrayLength(font);
    std::string label = "KNJFNT16";
    auto ctx = (Context *) context;
    ctx->addBios(label, fontRaw, fontSize);
    ctx->msx2->loadFont(ctx->bios[label]->data, ctx->bios[label]->size);
    env->ReleaseByteArrayElements(font, fontRaw, 0);
    ((Context *) context)->unlock();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_suzukiplan_msx2_Core_setupSpecialKeyCode(JNIEnv *,
                                                  jclass,
                                                  jlong context,
                                                  jint select,
                                                  jint start) {
    ((Context *) context)->lock();
    ((Context *) context)->msx2->setupKeyAssign(0, MSX2_JOY_S2, select);
    ((Context *) context)->msx2->setupKeyAssign(0, MSX2_JOY_S1, start);
    ((Context *) context)->unlock();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_suzukiplan_msx2_Core_tick(JNIEnv *env,
                                   jclass,
                                   jlong context,
                                   jint pad1,
                                   jint pad2,
                                   jint key,
                                   jobject vram) {
    ((Context *) context)->lock();
    auto ctx = (Context *) context;
    ctx->msx2->tick(pad1, pad2, key);
    unsigned short *pixels;
    unsigned short *display = ctx->msx2->getDisplay();
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
    void *soundBuf = ctx->msx2->getSound(&soundSize);
    ((Context *) context)->unlock();

    pthread_mutex_lock(&ctx->audioMutex);
    if (ctx->audioBufferSize + soundSize < sizeof(ctx->audioBuffer)) {
        memcpy(&ctx->audioBuffer[ctx->audioBufferSize], soundBuf, soundSize);
        ctx->audioBufferSize += soundSize;
    }
    pthread_mutex_unlock(&ctx->audioMutex);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_suzukiplan_msx2_Core_loadRom(JNIEnv *env,
                                      jclass,
                                      jlong context,
                                      jbyteArray rom,
                                      jint rom_type) {
    ((Context *) context)->lock();
    jbyte *romRaw = env->GetByteArrayElements(rom, nullptr);
    size_t romSize = env->GetArrayLength(rom);
    auto ctx = (Context *) context;
    std::string label = "CART";
    ctx->addBios(label, romRaw, romSize);
    ((Context *) context)->msx2->loadRom(ctx->bios[label]->data, (int) romSize, rom_type);
    env->ReleaseByteArrayElements(rom, romRaw, 0);
    ((Context *) context)->unlock();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_suzukiplan_msx2_Core_insertDisk(JNIEnv *env,
                                         jclass,
                                         jlong context,
                                         jint drive_id,
                                         jbyteArray disk,
                                         jstring sha256,
                                         jboolean read_only) {
    ((Context *) context)->lock();
    jbyte *diskRaw = env->GetByteArrayElements(disk, nullptr);
    size_t diskSize = env->GetArrayLength(disk);
    auto ctx = (Context *) context;
    const char *sha256Raw = env->GetStringUTFChars(sha256, nullptr);
    std::string label = sha256Raw;
    ctx->addBios(label, diskRaw, diskSize);
    ((Context *) context)->msx2->insertDisk(drive_id,
                                            ctx->bios[label]->data,
                                            (int) diskSize,
                                            read_only);
    env->ReleaseByteArrayElements(disk, diskRaw, 0);
    env->ReleaseStringUTFChars(sha256, sha256Raw);
    ((Context *) context)->unlock();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_suzukiplan_msx2_Core_ejectDisk(JNIEnv *, jclass, jlong context, jint drive_id) {
    ((Context *) context)->lock();
    ((Context *) context)->msx2->ejectDisk(drive_id);
    ((Context *) context)->unlock();
}

extern "C"
JNIEXPORT jbyteArray JNICALL
Java_com_suzukiplan_msx2_Core_quickSave(JNIEnv *env, jclass, jlong context) {
    ((Context *) context)->lock();
    size_t size;
    auto save = ((Context *) context)->msx2->quickSave(&size);
    jbyteArray result = env->NewByteArray((int) size);
    jbyte *ptr = env->GetByteArrayElements(result, nullptr);
    memcpy(ptr, save, size);
    ((Context *) context)->unlock();
    return result;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_suzukiplan_msx2_Core_quickLoad(JNIEnv *env, jclass, jlong context, jbyteArray save) {
    ((Context *) context)->lock();
    auto saveRaw = env->GetByteArrayElements(save, nullptr);
    size_t saveSize = env->GetArrayLength(save);
    ((Context *) context)->msx2->quickLoad(saveRaw, saveSize);
    env->ReleaseByteArrayElements(save, saveRaw, 0);
    ((Context *) context)->unlock();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_suzukiplan_msx2_Core_reset(JNIEnv *, jclass, jlong context) {
    ((Context *) context)->lock();
    ((Context *) context)->msx2->reset();
    ((Context *) context)->unlock();
}
