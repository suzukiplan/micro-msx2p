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
void emu_init_bios(const void* main, size_t mainSize,
                   const void* ext, size_t extSize,
                   const void* disk, size_t diskSize,
                   const void* fm, size_t fmSize,
                   const void* knj, size_t knjSize,
                   const void* font, size_t fontSize);
void emu_init_cbios(const void* main, size_t mainSize,
                    const void* logo, size_t logoSize,
                    const void* sub, size_t subSize);
void emu_init_bios_tm1p(const void* tm1pbios,
                        const void* tm1pext,
                        const void* tm1pkdr,
                        const void* tm1pdesk1,
                        const void* tm1pdesk2,
                        const void* font,
                        size_t fontSize);
void emu_init_bios_fsa1wsx(const void* msx2p,
                           const void* msx2pext,
                           const void* msx2pmus,
                           const void* disk,
                           const void* msxkanji,
                           const void* kanji,
                           const void* firm);
void emu_reset(void);
void emu_loadRom(const void* rom, size_t romSize, const char* fileName);
void emu_insertDisk(int driveId, const void* data, size_t size);
void emu_ejectDisk(int driveId);
void emu_vsync(void);
void emu_destroy(void);
void emu_dumpVideoMemory(void);
void emu_startDebug(void);
const void* emu_quickSave(size_t* size);

#ifdef __cplusplus
};
#endif


#endif /* emu_hpp */
