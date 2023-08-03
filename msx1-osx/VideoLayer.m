//
//  VideoLayer.m
//  msx2-osx
//
//  Created by Yoji Suzuki on 2023/01/23.
//

#import "constants.h"
#import "emu.h"
#import "VideoLayer.h"

static unsigned short imgbuf[2][VRAM_WIDTH * VRAM_HEIGHT];
static CGContextRef img[2];
static volatile int bno;

@implementation VideoLayer

+ (id)defaultActionForKey:(NSString*)key
{
    return nil;
}

- (id)init
{
    if (self = [super init]) {
        self.opaque = NO;
        img[0] = nil;
        img[1] = nil;
        // create image buffer
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        NSInteger width = VRAM_WIDTH;
        NSInteger height = VRAM_HEIGHT;
        for (int i = 0; i < 2; i++) {
            img[i] = CGBitmapContextCreate(imgbuf[i], width, height, 5, 2 * VRAM_WIDTH, colorSpace, kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder16Little);
        }
        CFRelease(colorSpace);
    }
    return self;
}

- (void)drawVRAM
{
    bno = 1 - bno;
    unsigned short* buf = imgbuf[1 - bno];
    int i = 0;
    for (int y = 0; y < VRAM_HEIGHT; y++) {
        int ptr = y * VRAM_WIDTH;
        for (int x = 0; x < VRAM_WIDTH; x++) {
            buf[i++] = emu_vram[ptr++];
        }
    }
    CGImageRef cgImage = CGBitmapContextCreateImage(img[1 - bno]);
    self.contents = (__bridge id)cgImage;
    CFRelease(cgImage);
}

- (void)dealloc
{
    for (int i = 0; i < 2; i++) {
        if (img[i] != nil) {
            CFRelease(img[i]);
            img[i] = nil;
        }
    }
}

@end
