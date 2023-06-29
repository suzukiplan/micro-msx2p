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
#ifndef INCLUDE_MSX2DLL_H
#define INCLUDE_MSX2DLL_H

#include <stdio.h>

#ifdef _MSC_VER
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

DLL_EXPORT void* msx2_createContext(int colorCode);
DLL_EXPORT void msx2_releaseContext(void* context);
DLL_EXPORT void msx2_setupSecondaryExist(void* context, bool page0, bool page1, bool page2, bool page3);
DLL_EXPORT void msx2_setupRAM(void* context, int pri, int sec);
DLL_EXPORT void msx2_setup(void* context, int pri, int sec, int idx, const void* data, int size, const char* label);
DLL_EXPORT void msx2_loadFont(void* context, const void* font, int size);
DLL_EXPORT void msx2_setupSpecialKeyCode(void* context, int select, int start);
DLL_EXPORT void msx2_tick(void* context, int pad1, int pad2, int key);
DLL_EXPORT void msx2_getDisplay16(void* context, void* display);
DLL_EXPORT int msx2_getDisplayWidth(void* context);
DLL_EXPORT int msx2_getDisplayHeight(void* context);
DLL_EXPORT int msx2_getMaxSoundSize(void* context);
DLL_EXPORT void msx2_getSound(void* context, void* sound, int* size);
DLL_EXPORT void msx2_loadRom(void* context, const void* rom, int size, int romType);
DLL_EXPORT void msx2_ejectRom(void* context);
DLL_EXPORT void msx2_insertDisk(void* context, int driveId, const void* disk, int size, bool readOnly);
DLL_EXPORT void msx2_ejectDisk(void* context, int driveId);
DLL_EXPORT int msx2_getQuickSaveSize(void* context);
DLL_EXPORT const void* msx2_quickSave(void* context);
DLL_EXPORT void msx2_quickLoad(void* context, const void* save, int size);
DLL_EXPORT void msx2_reset(void* context);

#ifdef __cplusplus
};
#endif

#endif /* INCLUDE_MSX2DLL_H */
