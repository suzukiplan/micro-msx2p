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

extern unsigned short emu_vram[VRAM_WIDTH * VRAM_HEIGHT];
extern unsigned char emu_key;
extern unsigned char emu_keycode;
void emu_init_bios(const void* main, size_t mainSize,
                   const void* ext, size_t extSize,
                   const void* disk, size_t diskSize,
                   const void* fm, size_t fmSize);
void emu_init_cbios(const void* main, size_t mainSize,
                    const void* logo, size_t logoSize,
                    const void* sub, size_t subSize);
void emu_reset(void);
void emu_loadRom(const void* rom, size_t romSize);
void emu_vsync(void);
void emu_destroy(void);
void emu_dumpVideoMemory(void);
void emu_startDebug(void);

#ifdef __cplusplus
};
#endif


#endif /* emu_hpp */
