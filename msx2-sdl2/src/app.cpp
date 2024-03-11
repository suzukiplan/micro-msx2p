/**
 * micro MSX2+ for SDL2
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
#include "SDL.h"
#include "msx2.hpp"
#include <chrono>
#include <map>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <vector>

#define WINDOW_TITLE "micro MSX2+ for SDL2"
#define VRAM_WIDTH 568
#define VRAM_HEIGHT 480
#define USE_CBIOS

static BufferQueue soundQueue(65536);
static pthread_mutex_t soundMutex = PTHREAD_MUTEX_INITIALIZER;
static bool halt = false;

class MSXKeyCode
{
  public:
    int column;
    unsigned char bit;

    MSXKeyCode(int column, unsigned char bit)
    {
        this->column = column;
        this->bit = bit;
    }
};

class MSXDisk
{
  public:
    void* data;
    size_t size;
    std::string name;

    MSXDisk(void* data, size_t size, const char* name)
    {
        this->data = data;
        this->size = size;
        this->name = name;
    }
};

static void log(const char* format, ...)
{
    char buf[256];
    auto now = time(nullptr);
    auto t = localtime(&now);
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    printf("%04d.%02d.%02d %02d:%02d:%02d %s\n",
           t->tm_year + 1900,
           t->tm_mon + 1,
           t->tm_mday,
           t->tm_hour,
           t->tm_min,
           t->tm_sec,
           buf);
}

void* load(const char* path, size_t* size = nullptr)
{
    FILE* fp = fopen(path, "rb");
    if (!fp) {
        log("error: File not found (%s)", path);
        return nullptr;
    }
    fseek(fp, 0, SEEK_END);
    auto fsize = ftell(fp);
    if (fsize < 1) {
        fclose(fp);
        log("error: File is empty (%s)", path);
        return nullptr;
    }
    auto result = malloc(fsize);
    if (!result) {
        fclose(fp);
        log("error: No memory (for load %s)", path);
        return nullptr;
    }
    fseek(fp, 0, SEEK_SET);
    if (fsize != fread(result, 1, fsize, fp)) {
        fclose(fp);
        log("error: File read error");
        return nullptr;
    }
    fclose(fp);
    if (size) *size = fsize;
    log("Loaded %s (%d bytes)", path, (int)fsize);
    return result;
}

static void audioCallback(void* userdata, Uint8* stream, int len)
{
    pthread_mutex_lock(&soundMutex);
    if (halt) {
        pthread_mutex_unlock(&soundMutex);
        return;
    }
    if (soundQueue.getCursor() < len) {
        memset(stream, 0, len);
    } else {
        void* buf;
        size_t bufSize;
        soundQueue.dequeue(&buf, &bufSize, len);
        memcpy(stream, buf, len);
    }
    pthread_mutex_unlock(&soundMutex);
    usleep(1000);
}

static inline unsigned char bit5To8(unsigned char bit5)
{
    bit5 <<= 3;
    bit5 |= (bit5 & 0b11100000) >> 5;
    return bit5;
}

int main(int argc, char* argv[])
{
    const char* romPath = nullptr;
    int romType = MSX2_ROM_TYPE_NORMAL;
    bool cliError = false;
    int fullScreen = 0;
    int gpuType = SDL_WINDOW_OPENGL;
    std::vector<MSXDisk*> disks;

    for (int i = 1; !cliError && i < argc; i++) {
        if ('-' != argv[i][0]) {
            romPath = argv[i];
            continue;
        }
        switch (tolower(argv[i][1])) {
            case 'd':
                i++;
                if (argc <= i) {
                    cliError = true;
                    break;
                }
                while (i < argc && argv[i][0] != '-') {
                    size_t size;
                    void* data = load(argv[i], &size);
                    if (!data) {
                        exit(-1);
                    }
                    const char* name = strrchr(argv[i], '/');
                    if (!name) {
                        name = strrchr(argv[i], '\\');
                        if (!name) {
                            name = argv[i];
                        } else {
                            name++;
                        }
                    } else {
                        name++;
                    }
                    disks.push_back(new MSXDisk(data, size, name));
                    i++;
                }
                if (i < argc && argv[i][0] == '-') {
                    i--;
                }
                break;
            case 'f':
                fullScreen = SDL_WINDOW_FULLSCREEN;
                break;
            case 'g':
                i++;
                if (argc <= i) {
                    cliError = true;
                    break;
                }
                if (0 == strcasecmp(argv[i], "OpenGL")) {
                    gpuType = SDL_WINDOW_OPENGL;
                } else if (0 == strcasecmp(argv[i], "Vulkan")) {
                    gpuType = SDL_WINDOW_VULKAN;
                } else if (0 == strcasecmp(argv[i], "Metal")) {
                    gpuType = SDL_WINDOW_METAL;
                } else if (0 == strcasecmp(argv[i], "None")) {
                    gpuType = 0;
                }
                break;
            case 'h':
                cliError = true;
                break;
            case 't':
                i++;
                if (argc <= i) {
                    cliError = true;
                    break;
                }
                if (0 == strcasecmp(argv[i], "NORMAL")) {
                    romType = MSX2_ROM_TYPE_NORMAL;
                } else if (0 == strcasecmp(argv[i], "ASC8") ||
                           0 == strcasecmp(argv[i], "ASCII8")) {
                    romType = MSX2_ROM_TYPE_ASC8;
                } else if (0 == strcasecmp(argv[i], "ASC8+SRAM2") ||
                           0 == strcasecmp(argv[i], "ASCII8+SRAM2") ||
                           0 == strcasecmp(argv[i], "ASC8_SRAM2") ||
                           0 == strcasecmp(argv[i], "ASCII8_SRAM2")) {
                    romType = MSX2_ROM_TYPE_ASC8_SRAM2;
                } else if (0 == strcasecmp(argv[i], "ASC16") ||
                           0 == strcasecmp(argv[i], "ASCII16")) {
                    romType = MSX2_ROM_TYPE_ASC16;
                } else if (0 == strcasecmp(argv[i], "ASC16+SRAM2") ||
                           0 == strcasecmp(argv[i], "ASCII16+SRAM2") ||
                           0 == strcasecmp(argv[i], "ASC16_SRAM2") ||
                           0 == strcasecmp(argv[i], "ASCII16_SRAM2")) {
                    romType = MSX2_ROM_TYPE_ASC16_SRAM2;
                } else if (0 == strcasecmp(argv[i], "KONAMI")) {
                    romType = MSX2_ROM_TYPE_KONAMI;
                } else if (0 == strcasecmp(argv[i], "KONAMI+SCC") ||
                           0 == strcasecmp(argv[i], "KONAMI_SCC")) {
                    romType = MSX2_ROM_TYPE_KONAMI_SCC;
                } else {
                    cliError = true;
                    break;
                }
                break;
            default:
                cliError = true;
                break;
        }
    }
    if (cliError) {
        puts("usage: app [/path/to/file.rom] ........... Use ROM");
        puts("           [-t { normal .................. 16KB/32KB <default>");
        puts("               | asc8 .................... MegaRom: ASCII-8");
        puts("               | asc16 ................... MegaRom: ASCII-16");
        puts("               | asc8+sram2 .............. MegaRom: ASCII-8 + SRAM2");
        puts("               | asc16+sram2 ............. MegaRom: ASCII-16 + SRAM2");
        puts("               | konami .................. MegaRom: KONAMI");
        puts("               | konami+scc .............. MegaRom: KONAMI+SCC");
        puts("               }]");
        puts("           [-d /path/to/disk*.dsk ...] ... Use Floppy Disk(s) *Max 9 disks");
        puts("           [-g { None .................... GPU: Do not use");
        puts("               | OpenGL .................. GPU: OpenGL <default>");
        puts("               | Vulkan .................. GPU: Vulkan");
        puts("               | Metal ................... GPU: Metal");
        puts("               }]");
        puts("           [-f] .......................... Full Screen Mode");
        return 1;
    }

    log("Booting micro MSX2+ for SDL2.");
    SDL_version sdlVersion;
    SDL_GetVersion(&sdlVersion);
    log("SDL version: %d.%d.%d", sdlVersion.major, sdlVersion.minor, sdlVersion.patch);

    log("Initializing micro-msx2p");
    MSX2 msx2(MSX2_COLOR_MODE_RGB555, true);

#ifdef USE_CBIOS
    // use C-BIOS
    msx2.setupSecondaryExist(false, false, false, true);
    auto biosMain = load("bios/cbios_main_msx2+_jp.rom");
    if (!biosMain) exit(-1);
    auto biosLogo = load("bios/cbios_logo_msx2+.rom");
    if (!biosLogo) exit(-1);
    auto biosSub = load("bios/cbios_sub.rom");
    if (!biosSub) exit(-1);
    msx2.setup(0, 0, 0, biosMain, 0x8000, "MAIN");
    msx2.setup(0, 0, 4, biosLogo, 0x4000, "LOGO");
    msx2.setup(3, 0, 0, biosSub, 0x4000, "SUB");
    msx2.setupRAM(3, 3);
#else
    // use MSX2+ BIOS
    size_t biosMainSize, biosSubSize, biosKnjSize, biosDiskSize, biosFmSize, biosFontSize;
    auto biosMain = load("bios/MSX2P.ROM", &biosMainSize);
    if (!biosMain) exit(-1);
    auto biosSub = load("bios/MSX2PEXT.ROM", &biosSubSize);
    if (!biosSub) exit(-1);
    auto biosKnj = load("bios/KNJDRV.ROM", &biosKnjSize);
    if (!biosKnj) exit(-1);
    auto biosDisk = load("bios/DISK.ROM", &biosDiskSize);
    if (!biosDisk) exit(-1);
    auto biosFm = load("bios/FMBIOS.ROM", &biosFmSize);
    if (!biosFm) exit(-1);
    auto biosFont = load("bios/KNJFNT16.ROM", &biosFontSize);
    unsigned char empty[0x4000];
    msx2.setupSecondaryExist(false, false, false, true);
    msx2.setupRAM(3, 0);
    msx2.setup(0, 0, 0, biosMain, biosMainSize, "MAIN");
    msx2.setup(3, 1, 0, biosSub, biosSubSize, "SUB");
    msx2.setup(3, 1, 2, biosKnj, biosKnjSize, "KNJ");
    msx2.setup(3, 2, 0, empty, sizeof(empty), "DISK");
    msx2.setup(3, 2, 2, biosDisk, biosDiskSize, "DISK");
    msx2.setup(3, 2, 4, empty, sizeof(empty), "DISK");
    msx2.setup(3, 2, 6, empty, sizeof(empty), "DISK");
    msx2.setup(3, 3, 2, biosFm, biosFmSize, "FM");
    msx2.loadFont(biosFont, biosFontSize);
#endif

    void* rom = nullptr;
    size_t romSize;
    if (romPath) {
        rom = load(romPath, &romSize);
        if (!rom) {
            exit(-1);
        }
        msx2.loadRom(rom, romSize, romType);
    }

    if (!disks.empty()) {
        msx2.insertDisk(0, disks[0]->data, disks[0]->size, false);
    }

    log("Initializing SDL");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS)) {
        log("SDL_Init failed: %s", SDL_GetError());
        exit(-1);
    }

    log("Initializing AudioDriver");
    SDL_AudioSpec desired;
    SDL_AudioSpec obtained;
    desired.freq = 44100;
    desired.format = AUDIO_S16LSB;
    desired.channels = 2;
    desired.samples = 735; // desired.freq * 20 / 1000;
    desired.callback = audioCallback;
    desired.userdata = &msx2;
    auto audioDeviceId = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
    if (0 == audioDeviceId) {
        log(" ... SDL_OpenAudioDevice failed: %s", SDL_GetError());
        exit(-1);
    }
    log("- obtained.freq = %d", obtained.freq);
    log("- obtained.format = %X", obtained.format);
    log("- obtained.channels = %d", obtained.channels);
    log("- obtained.samples = %d", obtained.samples);
    SDL_PauseAudioDevice(audioDeviceId, 0);

    SDL_Surface* windowSurface = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    SDL_Window* window = nullptr;
    int frameWidth = 0;
    int frameHeight = 0;
    int framePitch = 0;
    int offsetX = 0;
    int offsetY = 0;
    unsigned int* frameBuffer;
    if (fullScreen) {
        log("create SDL window and renderer");
        SDL_DisplayMode display;
        SDL_GetCurrentDisplayMode(0, &display);
        log("Screen Resolution: width=%d, height=%d", display.w, display.h);
        if (0 != SDL_CreateWindowAndRenderer(display.w, display.h, gpuType | SDL_WINDOW_FULLSCREEN, &window, &renderer)) {
            log("SDL_CreateWindowAndRenderer failed: %s", SDL_GetError());
            exit(-1);
        }
        double sw = display.w / ((double)VRAM_WIDTH);
        double sh = display.h / ((double)VRAM_HEIGHT);
        double scale = sw < sh ? sw : sh;
        frameWidth = (int)(display.w / scale);
        frameHeight = (int)(display.h / scale);
        framePitch = frameWidth * 4;
        offsetX = (frameWidth - VRAM_WIDTH) / 2;
        offsetY = (frameHeight - VRAM_HEIGHT) / 2;
        log("Texture: width=%d, height=%d, offsetX=%d, offsetY=%d", frameWidth, frameHeight, offsetX, offsetY);
        SDL_RenderSetLogicalSize(renderer, frameWidth, frameHeight);
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, frameWidth, frameHeight);
        if (!texture) {
            log("SDL_CreateTexture failed: %s", SDL_GetError());
            exit(-1);
        }
        frameBuffer = (unsigned int*)malloc(framePitch * frameHeight);
        if (!frameBuffer) {
            log("No memory");
            exit(-1);
        }
        memset(frameBuffer, 0, frameWidth * frameHeight * 4);
        SDL_ShowCursor(SDL_DISABLE);
    } else {
        log("create SDL window");
        window = SDL_CreateWindow(
            WINDOW_TITLE,
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            VRAM_WIDTH,
            VRAM_HEIGHT,
            gpuType | fullScreen);
        log("Get SDL window surface");
        windowSurface = SDL_GetWindowSurface(window);
        if (!windowSurface) {
            log("SDL_GetWindowSurface failed: %s", SDL_GetError());
            exit(-1);
        }
        log("PixelFormat: %d bits (%d bytes)", (int)windowSurface->format->BitsPerPixel, (int)windowSurface->format->BytesPerPixel);
        log("Rmask: %08X", (int)windowSurface->format->Rmask);
        log("Gmask: %08X", (int)windowSurface->format->Gmask);
        log("Bmask: %08X", (int)windowSurface->format->Bmask);
        log("Amask: %08X", (int)windowSurface->format->Amask);
        if (4 != windowSurface->format->BytesPerPixel) {
            log("unsupported pixel format (support only 4 bytes / pixel)");
            exit(-1);
        }
        SDL_UpdateWindowSurface(window);
    }

    log("Initializing keyboard map");
    std::map<int, MSXKeyCode*> keyMap;
    keyMap[SDLK_0] = new MSXKeyCode(0, 0b00000001);            // 0
    keyMap[SDLK_1] = new MSXKeyCode(0, 0b00000010);            // 1
    keyMap[SDLK_2] = new MSXKeyCode(0, 0b00000100);            // 2
    keyMap[SDLK_3] = new MSXKeyCode(0, 0b00001000);            // 3
    keyMap[SDLK_4] = new MSXKeyCode(0, 0b00010000);            // 4
    keyMap[SDLK_5] = new MSXKeyCode(0, 0b00100000);            // 5
    keyMap[SDLK_6] = new MSXKeyCode(0, 0b01000000);            // 6
    keyMap[SDLK_7] = new MSXKeyCode(0, 0b10000000);            // 7
    keyMap[SDLK_8] = new MSXKeyCode(1, 0b00000001);            // 8
    keyMap[SDLK_9] = new MSXKeyCode(1, 0b00000010);            // 9
    keyMap[SDLK_MINUS] = new MSXKeyCode(1, 0b00000100);        // -
    keyMap[SDLK_CARET] = new MSXKeyCode(1, 0b00001000);        // ^
    keyMap[SDLK_BACKSLASH] = new MSXKeyCode(1, 0b00010000);    // '\'
    keyMap[SDLK_AT] = new MSXKeyCode(1, 0b00100000);           // @
    keyMap[SDLK_LEFTBRACKET] = new MSXKeyCode(1, 0b01000000);  // [
    keyMap[SDLK_SEMICOLON] = new MSXKeyCode(1, 0b10000000);    // ;
    keyMap[SDLK_COLON] = new MSXKeyCode(2, 0b00000001);        // :
    keyMap[SDLK_RIGHTBRACKET] = new MSXKeyCode(2, 0b00000010); // ]
    keyMap[SDLK_COMMA] = new MSXKeyCode(2, 0b00000100);        // ,
    keyMap[SDLK_PERIOD] = new MSXKeyCode(2, 0b00001000);       // .
    keyMap[SDLK_SLASH] = new MSXKeyCode(2, 0b00010000);        // /
    keyMap[SDLK_UNDERSCORE] = new MSXKeyCode(2, 0b00100000);   // _
    keyMap[SDLK_a] = new MSXKeyCode(2, 0b01000000);            // A
    keyMap[SDLK_b] = new MSXKeyCode(2, 0b10000000);            // B
    keyMap[SDLK_c] = new MSXKeyCode(3, 0b00000001);            // C
    keyMap[SDLK_d] = new MSXKeyCode(3, 0b00000010);            // D
    keyMap[SDLK_e] = new MSXKeyCode(3, 0b00000100);            // E
    keyMap[SDLK_f] = new MSXKeyCode(3, 0b00001000);            // F
    keyMap[SDLK_g] = new MSXKeyCode(3, 0b00010000);            // G
    keyMap[SDLK_h] = new MSXKeyCode(3, 0b00100000);            // H
    keyMap[SDLK_i] = new MSXKeyCode(3, 0b01000000);            // I
    keyMap[SDLK_j] = new MSXKeyCode(3, 0b10000000);            // J
    keyMap[SDLK_k] = new MSXKeyCode(4, 0b00000001);            // K
    keyMap[SDLK_l] = new MSXKeyCode(4, 0b00000010);            // L
    keyMap[SDLK_m] = new MSXKeyCode(4, 0b00000100);            // M
    keyMap[SDLK_n] = new MSXKeyCode(4, 0b00001000);            // N
    keyMap[SDLK_o] = new MSXKeyCode(4, 0b00010000);            // O
    keyMap[SDLK_p] = new MSXKeyCode(4, 0b00100000);            // P
    keyMap[SDLK_q] = new MSXKeyCode(4, 0b01000000);            // Q
    keyMap[SDLK_r] = new MSXKeyCode(4, 0b10000000);            // R
    keyMap[SDLK_s] = new MSXKeyCode(5, 0b00000001);            // S
    keyMap[SDLK_t] = new MSXKeyCode(5, 0b00000010);            // T
    keyMap[SDLK_u] = new MSXKeyCode(5, 0b00000100);            // U
    keyMap[SDLK_v] = new MSXKeyCode(5, 0b00001000);            // V
    keyMap[SDLK_w] = new MSXKeyCode(5, 0b00010000);            // W
    keyMap[SDLK_x] = new MSXKeyCode(5, 0b00100000);            // X
    keyMap[SDLK_y] = new MSXKeyCode(5, 0b01000000);            // Y
    keyMap[SDLK_z] = new MSXKeyCode(5, 0b10000000);            // Z
    keyMap[SDLK_LSHIFT] = new MSXKeyCode(6, 0b00000001);       // left shift
    keyMap[SDLK_RSHIFT] = new MSXKeyCode(6, 0b00000001);       // right shift
    keyMap[SDLK_LCTRL] = new MSXKeyCode(6, 0b00000010);        // control as ctrl
    keyMap[SDLK_RCTRL] = new MSXKeyCode(6, 0b00000010);        // control as ctrl
    keyMap[SDLK_LALT] = new MSXKeyCode(6, 0b00000100);         // option as graph
    keyMap[SDLK_RALT] = new MSXKeyCode(6, 0b00000100);         // option as graph
    keyMap[SDLK_CAPSLOCK] = new MSXKeyCode(6, 0b00001000);     // caps
    keyMap[SDLK_F7] = new MSXKeyCode(6, 0b00010000);           // F7 as kana
    keyMap[SDLK_F1] = new MSXKeyCode(6, 0b00100000);           // F1
    keyMap[SDLK_F2] = new MSXKeyCode(6, 0b01000000);           // F2
    keyMap[SDLK_F3] = new MSXKeyCode(6, 0b10000000);           // F3
    keyMap[SDLK_F4] = new MSXKeyCode(7, 0b00000001);           // F4
    keyMap[SDLK_F5] = new MSXKeyCode(7, 0b00000010);           // F5
    keyMap[SDLK_ESCAPE] = new MSXKeyCode(7, 0b00000100);       // esc
    keyMap[SDLK_TAB] = new MSXKeyCode(7, 0b00001000);          // tab
    keyMap[SDLK_F12] = new MSXKeyCode(7, 0b00010000);          // F12 as stop
    keyMap[SDLK_BACKSPACE] = new MSXKeyCode(7, 0b00100000);    // BS
    keyMap[SDLK_F11] = new MSXKeyCode(7, 0b01000000);          // F11 as select
    keyMap[SDLK_RETURN] = new MSXKeyCode(7, 0b10000000);       // return
    keyMap[SDLK_SPACE] = new MSXKeyCode(8, 0b00000001);        // space
    keyMap[SDLK_F10] = new MSXKeyCode(8, 0b00000010);          // F10 as cls/home
    keyMap[SDLK_F9] = new MSXKeyCode(8, 0b00000100);           // F9 as ins
    keyMap[SDLK_F8] = new MSXKeyCode(8, 0b00001000);           // F8 as del
    keyMap[SDLK_LEFT] = new MSXKeyCode(8, 0b00010000);         // left cursor
    keyMap[SDLK_UP] = new MSXKeyCode(8, 0b00100000);           // up cursor
    keyMap[SDLK_DOWN] = new MSXKeyCode(8, 0b01000000);         // down cursor
    keyMap[SDLK_RIGHT] = new MSXKeyCode(8, 0b10000000);        // right cursor

    log("Start main loop...");
    SDL_Event event;
    unsigned int loopCount = 0;
    const int waitFps60[3] = {17000, 17000, 16000};
    unsigned char msxKeyCodeMap[16];
    memset(msxKeyCodeMap, 0, sizeof(msxKeyCodeMap));
    bool stabled = false;
    bool hotKey = false;
    while (!halt) {
        loopCount++;
        auto start = std::chrono::system_clock::now();
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                halt = true;
                break;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == 0x400000E3) {
                    hotKey = true;
                    memset(msxKeyCodeMap, 0, sizeof(msxKeyCodeMap));
                } else if (hotKey) {
                    switch (event.key.keysym.sym) {
                        case SDLK_q:
                            halt = true;
                            break;
                        case SDLK_1:
                        case SDLK_2:
                        case SDLK_3:
                        case SDLK_4:
                        case SDLK_5:
                        case SDLK_6:
                        case SDLK_7:
                        case SDLK_8:
                        case SDLK_9: {
                            int index = event.key.keysym.sym - SDLK_1;
                            if (index < disks.size()) {
                                log("Insert floppy disk: %s", disks[index]->name.c_str());
                                msx2.insertDisk(0, disks[index]->data, disks[index]->size, false);
                            }
                            break;
                        }
                        case SDLK_0:
                            log("Eject floppy disk");
                            msx2.ejectDisk(0);
                            break;
                        case SDLK_r:
                            log("Reset micro-msx2p");
                            msx2.reset();
                            break;
                    }
                } else {
                    auto it = keyMap.find(event.key.keysym.sym);
                    if (it != keyMap.end()) {
                        msxKeyCodeMap[it->second->column] |= it->second->bit;
                    } else {
                        log("Pressed unassigned key: 0x%02X", event.key.keysym.sym);
                    }
                }
            } else if (event.type == SDL_KEYUP) {
                if (event.key.keysym.sym == 0x400000E3) {
                    hotKey = false;
                } else if (!hotKey) {
                    auto it = keyMap.find(event.key.keysym.sym);
                    if (it != keyMap.end()) {
                        msxKeyCodeMap[it->second->column] ^= it->second->bit;
                    }
                }
            }
        }
        if (halt) {
            break;
        }

        // execute emulator 1 frame
        msx2.tickWithKeyCodeMap(0, 0, msxKeyCodeMap);

        // enqueue sound
        pthread_mutex_lock(&soundMutex);
        size_t pcmSize;
        auto pcm = msx2.getSound(&pcmSize);
        soundQueue.enqueue(pcm, pcmSize);
        pthread_mutex_unlock(&soundMutex);

        // render graphics
        auto msxDisplay = msx2.getDisplay();
        if (fullScreen) {
            auto pcDisplay = (unsigned int*)frameBuffer;
            pcDisplay += offsetY * frameWidth;
            for (int y = 0; y < VRAM_HEIGHT; y += 2) {
                for (int x = 0; x < VRAM_WIDTH; x++) {
                    unsigned int rgb555 = msxDisplay[x];
                    unsigned int rgb888 = 0;
                    rgb888 |= bit5To8((rgb555 & 0b0111110000000000) >> 10);
                    rgb888 <<= 8;
                    rgb888 |= bit5To8((rgb555 & 0b0000001111100000) >> 5);
                    rgb888 <<= 8;
                    rgb888 |= bit5To8(rgb555 & 0b0000000000011111);
                    rgb888 <<= 8;
                    auto offset = offsetX + x;
                    pcDisplay[offset] = rgb888;
                    pcDisplay[offset + frameWidth] = rgb888;
                }
                msxDisplay += VRAM_WIDTH;
                pcDisplay += frameWidth * 2;
            }
            SDL_UpdateTexture(texture, nullptr, frameBuffer, framePitch);
            SDL_SetRenderTarget(renderer, nullptr);
            SDL_RenderCopy(renderer, texture, nullptr, nullptr);
            SDL_RenderPresent(renderer);
        } else {
            auto pcDisplay = (unsigned int*)windowSurface->pixels;
            auto pitch = windowSurface->pitch / windowSurface->format->BytesPerPixel;
            for (int y = 0; y < VRAM_HEIGHT; y += 2) {
                for (int x = 0; x < VRAM_WIDTH; x++) {
                    unsigned int rgb555 = msxDisplay[x];
                    unsigned int rgb888 = 0xFF00;
                    rgb888 |= bit5To8((rgb555 & 0b0111110000000000) >> 10);
                    rgb888 <<= 8;
                    rgb888 |= bit5To8((rgb555 & 0b0000001111100000) >> 5);
                    rgb888 <<= 8;
                    rgb888 |= bit5To8(rgb555 & 0b0000000000011111);
                    pcDisplay[x] = rgb888;
                }
                msxDisplay += VRAM_WIDTH;
                memcpy(&pcDisplay[pitch], &pcDisplay[0], VRAM_WIDTH * 4);
                pcDisplay += pitch * 2;
            }
            SDL_UpdateWindowSurface(window);
        }

        // sync 60fps
        std::chrono::duration<double> diff = std::chrono::system_clock::now() - start;
        int us = (int)(diff.count() * 1000000);
        int wait = waitFps60[loopCount % 3];
        if (us < wait) {
            usleep(wait - us);
            if (!stabled) {
                stabled = true;
                log("Frame rate stabilized at 60 fps (%dus per frame)", us);
            }
        } else if (stabled) {
            stabled = false;
            log("warning: Frame rate is lagging (%dus per frame)", us);
        }
    }

    log("Terminating");
    SDL_CloseAudioDevice(audioDeviceId);
    if (fullScreen) {
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        free(frameBuffer);
    }
    SDL_DestroyWindow(window);
    SDL_Quit();
#ifdef USE_CBIOS
    free(biosMain);
    free(biosLogo);
    free(biosSub);
#else
    free(biosMain);
    free(biosSub);
    free(biosKnj);
    free(biosDisk);
    free(biosFm);
    free(biosFont);
#endif
    if (rom) {
        free(rom);
    }
    for (auto it = keyMap.begin(); it != keyMap.end(); it++) {
        delete it->second;
    }
    for (auto disk : disks) {
        free(disk->data);
        delete (disk);
    }
    return 0;
}
