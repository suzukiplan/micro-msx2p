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
#import "MSX2View.h"
#import "MSX2Core.h"
#import "MSX2CALayer.h"
#import "sound-ios.h"

@interface MSX2View()
@property (nonatomic) MSX2Core* core;
@property (nonatomic) CADisplayLink* displayLink;
@property (assign) void* sound;
@property (atomic) BOOL started;
@property (atomic) BOOL paused;
@end

@implementation MSX2View

+ (Class)layerClass
{
    return [MSX2CALayer class];
}

- (instancetype)init
{
    if (self = [super init]) {
        [self _init];
    }
    return self;
}

- (instancetype)initWithCoder:(NSCoder*)coder
{
    if (self = [super initWithCoder:coder]) {
        [self _init];
    }
    return self;
}

- (instancetype)initWithFrame:(CGRect)frame
{
    if (self = [super initWithFrame:frame]) {
        [self _init];
    }
    return self;
}

- (void)_init
{
    if (!_core) {
        _core = [[MSX2Core alloc] init];
        _sound = sound_create();
        self.opaque = NO;
        self.clearsContextBeforeDrawing = NO;
        self.multipleTouchEnabled = NO;
        self.userInteractionEnabled = NO;
        ((MSX2CALayer*)self.layer).core = _core;
        _displayLink = [CADisplayLink displayLinkWithTarget:self
                                                   selector:@selector(_detectVsync)];
        _displayLink.preferredFramesPerSecond = 60;
        [_displayLink addToRunLoop:[NSRunLoop currentRunLoop]
                           forMode:NSRunLoopCommonModes];
    }
}

- (void)_detectVsync
{
    if (_started && !_paused) {
        [_core tickWithPad1:[_delegate didRequirePad1CodeWithView:self]
                       pad2:0
                        key:0];
        [(MSX2CALayer*)self.layer drawFrameWithCore:_core];
        NSData* sound = _core.sound;
        sound_enqueue(_sound, sound.bytes, sound.length);
    } else {
        static unsigned char empty[2940];
        sound_enqueue(_sound, empty, sizeof(empty));
    }
}

- (void)setupWithCBiosMain:(NSData*)main
                      logo:(NSData*)logo
                       sub:(NSData*)sub
                       rom:(NSData*)rom
                   romType:(MSX2RomType)romType
                    select:(NSInteger)select
                     start:(NSInteger)start
{
    [_core setupSecondaryExistWithPage0:NO page1:NO page2:NO page3:YES];
    [_core setupWithPrimary:0 secondary:0 index:0 bios:main label:@"MAIN"];
    [_core setupWithPrimary:0 secondary:0 index:4 bios:logo label:@"LOGO"];
    [_core setupWithPrimary:3 secondary:0 index:0 bios:sub label:@"SUB"];
    [_core setupRamWithPrimary:3 secondary:3];
    [_core setupSpecialKeyCodeWithSelect:select start:start];
    [_core loadRom:rom romType:romType];
    [_core reset];
    [_delegate didStopWithView:self];
    _started = YES;
    _paused = NO;
}

- (void)stop
{
    if (_started) {
        [_delegate didStopWithView:self];
        _started = NO;
    }
}

- (void)dealloc
{
    [self stop];
    sound_destroy(_sound);
}

- (nullable NSData*)quickSave
{
    return [_core quickSave];
}

- (void)quickLoadWithSaveData:(NSData*)saveData;
{
    [_core quickLoadWithSaveData:saveData];
}

- (void)reset
{
    [_core reset];
}

- (void)pause
{
    _paused = YES;
}

- (void)resume
{
    _paused = NO;
}

@end
