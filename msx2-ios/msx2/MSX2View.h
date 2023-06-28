/*
 * micro MSX2+ - MSX2View for iOS
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
#import <UIKit/UIKit.h>
#import <msx2/MSX2Core.h>

NS_ASSUME_NONNULL_BEGIN

@class MSX2View;

@protocol MSX2ViewDelegate <NSObject>
- (NSInteger)didRequirePad1CodeWithView:(MSX2View*)view;
- (void)didStartWithView:(MSX2View*)view;
- (void)didStopWithView:(MSX2View*)view;
@end

@interface MSX2View : UIView
@property (nonatomic, weak) id<MSX2ViewDelegate> delegate;
- (void)setupWithCBiosMain:(NSData*)main
                      logo:(NSData*)logo
                       sub:(NSData*)sub
                       rom:(NSData*)rom
                   romType:(MSX2RomType)romType
                    select:(NSInteger)select
                     start:(NSInteger)start;
- (nullable NSData*)quickSave;
- (void)quickLoadWithSaveData:(NSData*)saveData;
- (void)reset;
- (void)pause;
- (void)resume;
@end

NS_ASSUME_NONNULL_END
