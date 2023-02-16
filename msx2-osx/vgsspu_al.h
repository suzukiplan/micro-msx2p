/* (C)2016, SUZUKI PLAN.
 *----------------------------------------------------------------------------
 * Description: VGS - Sound Processing Unit
 *    Platform: Common
 *      Author: Yoji Suzuki (SUZUKI PLAN)
 *----------------------------------------------------------------------------
 */
#ifndef INCLUDE_VGSSPU_H
#define INCLUDE_VGSSPU_H
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif
    
void* vgsspu_start(void (*callback)(void* buffer, size_t size));
void* vgsspu_start2(int sampling, int bit, int ch, size_t size, void (*callback)(void* buffer, size_t size));
int vgsspu_resize(void* context, size_t size);
void vgsspu_end(void* context);
    
#ifdef __cplusplus
};
#endif

#endif
