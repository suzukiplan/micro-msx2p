/**
 * micro MSX2+ - Core Module for Objective-c
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
#import "MSX2Core.h"
#import "msx2gw.h"
#import "msx2def.h"

@interface MSX2Core()
@property (nonatomic, readwrite) void* msx2;
@end

@implementation MSX2Core

- (instancetype)init
{
    if (self = [super init]) {
        _msx2 = msx2_createContext(MSX2_COLOR_MODE_RGB555);
    }
    return self;
}

- (void)dealloc
{
    msx2_releaseContext(_msx2);
}

- (void)setupSecondaryExistWithPage0:(BOOL)page0
                               page1:(BOOL)page1
                               page2:(BOOL)page2
                               page3:(BOOL)page3
{
    msx2_setupSecondaryExist(_msx2, page0, page1, page2, page3);
}

- (void)setupRamWithPrimary:(NSInteger)primary
                  secondary:(NSInteger)secondary
{
    msx2_setupRAM(_msx2, (int)primary, (int)secondary);
}

- (void)setupWithPrimary:(NSInteger)primary
               secondary:(NSInteger)secondary
                   index:(NSInteger)index
                    bios:(NSData*)bios
                   label:(NSString*)label
{
    msx2_setup(_msx2,
               (int)primary,
               (int)secondary,
               (int)index,
               bios.bytes,
               bios.length,
               label.UTF8String);
}

- (void)loadFont:(NSData*)font
{
    msx2_loadFont(_msx2, font.bytes, font.length);
}

- (void)setupSpecialKeyCodeWithSelect:(NSInteger)select
                                start:(NSInteger)start
{
    msx2_setupSpecialKeyCode(_msx2, (int)select, (int)start);
}

- (void)tickWithPad1:(NSInteger)pad1
                pad2:(NSInteger)pad2
                 key:(NSInteger)key
{
    msx2_tick(_msx2, (int)pad1, (int)pad2, (int)key);
}

- (unsigned short *)display
{
    return msx2_getDisplay(_msx2);
}

- (NSInteger)displayWidth
{
    return msx2_getDisplayWidth(_msx2);
}

- (NSInteger)displayHeight
{
    return msx2_getDisplayHeight(_msx2);
}

- (NSData*)sound
{
    size_t size;
    const void* data = msx2_getSound(_msx2, &size);
    return [NSData dataWithBytes:data length:size];
}

- (void)loadRom:(NSData*)rom romType:(MSX2RomType)romType
{
    msx2_loadRom(_msx2, rom.bytes, rom.length, (int)romType);
}

- (void)ejectRom
{
    msx2_ejectRom(_msx2);
}

- (void)insertWithDriveId:(NSInteger)driveId
                     disk:(NSData*)disk
                 readOnly:(BOOL)readOnly
{
    msx2_insertDisk(_msx2, (int)driveId, disk.bytes, disk.length, readOnly);
}

- (void)ejectWithDriveId:(NSInteger)driveId
{
    msx2_ejectDisk(_msx2, (int)driveId);
}

- (NSData*)quickSave
{
    size_t size;
    const void* data = msx2_quickSave(_msx2, &size);
    return [NSData dataWithBytes:data length:size];
}

- (void)quickLoadWithSaveData:(NSData*)data
{
    msx2_quickLoad(_msx2, data.bytes, data.length);
}

- (void)reset
{
    msx2_reset(_msx2);
}

@end
