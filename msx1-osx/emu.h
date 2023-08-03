//
//  emu.h
//  msx1-osx
//
//  Created by Yoji Suzuki on 2023/08/03.
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
void emu_init_bios(const void* main, const void* logo);
void emu_reset(void);
void emu_loadRom(const void* rom, size_t romSize, const char* fileName);
void emu_vsync(void);
void emu_destroy(void);

#ifdef __cplusplus
};
#endif


#endif /* emu_hpp */
