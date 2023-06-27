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
#import <Foundation/Foundation.h>
#import "msx2def.h"

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, MSX2RomType) {
    MSX2RomTypeNormal = MSX2_ROM_TYPE_NORMAL,
    MSX2RomTypeAsc8 = MSX2_ROM_TYPE_ASC8,
    MSX2RomTypeAsc8WSram = MSX2_ROM_TYPE_ASC8_SRAM2,
    MSX2RomTypeAsc16 = MSX2_ROM_TYPE_ASC16,
    MSX2RomTypeAsc16Sram = MSX2_ROM_TYPE_ASC16_SRAM2,
    MSX2RomTypeKonami = MSX2_ROM_TYPE_KONAMI,
    MSX2RomTypeKonamiSCC = MSX2_ROM_TYPE_KONAMI_SCC,
};

@interface MSX2Core : NSObject
@property (nonatomic, readonly) unsigned short* display;
@property (nonatomic, readonly) NSInteger displayWidth;
@property (nonatomic, readonly) NSInteger displayHeight;
@property (nonatomic, readonly) NSData* sound;

- (instancetype)init;
- (void)setupSecondaryExistWithPage0:(BOOL)page0
                               page1:(BOOL)page1
                               page2:(BOOL)page2
                               page3:(BOOL)page3;

- (void)setupRamWithPrimary:(NSInteger)primary
                  secondary:(NSInteger)secondary;

- (void)setupWithPrimary:(NSInteger)primary
               secondary:(NSInteger)secondary
                   index:(NSInteger)index
                    bios:(NSData*)bios
                   label:(NSString*)label;

- (void)loadFont:(NSData*)font;

- (void)setupSpecialKeyCodeWithSelect:(NSInteger)select
                                start:(NSInteger)start;

- (void)tickWithPad1:(NSInteger)pad1
                pad2:(NSInteger)pad2
                 key:(NSInteger)key;

- (void)loadRom:(NSData*)rom
        romType:(MSX2RomType)romType;

- (void)ejectRom;

- (void)insertWithDriveId:(NSInteger)driveId
                     disk:(NSData*)disk
                 readOnly:(BOOL)readOnly;

- (void)ejectWithDriveId:(NSInteger)driveId;

- (NSData*)quickSave;

- (void)quickLoadWithSaveData:(NSData*)data;

- (void)reset;
@end

NS_ASSUME_NONNULL_END
