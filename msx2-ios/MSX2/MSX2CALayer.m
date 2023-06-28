/**
 * micro MSX2+ - Core Animation Layer
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
#import <pthread.h>
#import "MSX2CALayer.h"

#define VRAM_WIDTH 568
#define VRAM_HEIGHT 240

@interface MSX2CALayer ()
@property (atomic) pthread_mutex_t mutex;
@property (atomic) CGContextRef img;
@property (assign) unsigned short* imgbuf;
@property (atomic) BOOL destroyed;
@end

@implementation MSX2CALayer

+ (id)defaultActionForKey:(NSString*)key
{
    return nil;
}

- (instancetype)initWithCore:(MSX2Core*)core
{
    if (self = [super init]) {
        _destroyed = NO;
        _core = core;
        pthread_mutex_init(&_mutex, NULL);
        _imgbuf = (unsigned short*)malloc(VRAM_WIDTH * VRAM_HEIGHT * 2);
        memset(_imgbuf, 0, VRAM_WIDTH * VRAM_HEIGHT * 2);
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        _img = CGBitmapContextCreate(core.display,
                                     core.displayWidth,
                                     core.displayHeight,
                                     5,
                                     core.displayWidth * 2,
                                     colorSpace,
                                     kCGImageAlphaNoneSkipFirst |
                                     kCGBitmapByteOrder16Little);
        CFRelease(colorSpace);
    }
    return self;
}

- (void)lockVram
{
    if (!_destroyed) {
        pthread_mutex_lock(&_mutex);
    }
}

- (void)unlockVram
{
    if (!_destroyed) {
        pthread_mutex_unlock(&_mutex);
    }
}

- (void)drawFrameWithCore:(MSX2Core*)core
{
    if (_destroyed) {
        return;
    }
    [self lockVram];
    memcpy(_imgbuf, core.display, VRAM_WIDTH * VRAM_HEIGHT * 2);
    CGImageRef cgImage = CGBitmapContextCreateImage(_img);
    self.contents = (__bridge id)cgImage;
    [self unlockVram];
    CFRelease(cgImage);
}

- (UIImage*)capture
{
    if (_destroyed) {
        return nil;
    }
    CGImageRef cgImage = CGBitmapContextCreateImage(_img);
    if (!cgImage) {
        return nil;
    }
    UIImage* uiImage = [UIImage imageWithCGImage:cgImage];
    CGImageRelease(cgImage);
    return uiImage;
}

- (void)destroy
{
    if (!_destroyed) {
        _destroyed = YES;
        CGContextRelease(_img);
        _img = nil;
        free(_imgbuf);
        _imgbuf = NULL;
        pthread_mutex_destroy(&_mutex);
    }
}

- (void)dealloc
{
    [self destroy];
}

@end
