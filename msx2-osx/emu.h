//
//  emu.hpp
//  msx2-osx
//
//  Created by Yoji Suzuki on 2023/01/23.
//

#ifndef emu_hpp
#define emu_hpp
#include "constants.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned short emu_vram[VRAM_WIDTH * 2 * VRAM_HEIGHT * 2];
extern unsigned char emu_key;
extern unsigned char emu_keycode;
void emu_init_bios(const void* main,
                   const void* sub,
                   const void* disk,
                   const void* fm,
                   const void* knj,
                   const void* font);
void emu_init_cbios(const void* main,
                    const void* logo,
                    const void* sub);
void emu_reset(void);
void emu_loadRom(const void* rom, size_t romSize, const char* fileName);
void emu_insertDisk(int driveId, const void* data, size_t size);
void emu_ejectDisk(int driveId);
void emu_vsync(void);
void emu_destroy(void);
void emu_dumpVideoMemory(void);
void emu_startDebug(void);
const void* emu_quickSave(size_t* size);
void emu_quickLoad(const void* data, size_t size);

#ifdef __cplusplus
};
#endif


#endif /* emu_hpp */
