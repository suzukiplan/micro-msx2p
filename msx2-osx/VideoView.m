//
//  VideoView.m
//  msx2-osx
//
//  Created by Yoji Suzuki on 2023/01/23.
//

#import "VideoLayer.h"
#import "VideoView.h"
#import "emu.h"
#import "msx2def.h"
#include <ctype.h>

static CVReturn MyDisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now, const CVTimeStamp* outputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* context);
extern unsigned char emu_key;
extern unsigned char emu_keycode;

@interface VideoView ()
@property (nonatomic) VideoLayer* videoLayer;
@property CVDisplayLinkRef displayLink;
@property NSInteger score;
@property BOOL isGameOver;
@end

@implementation VideoView

+ (Class)layerClass
{
    return [VideoLayer class];
}

- (id)initWithFrame:(CGRect)frame
{
    if ((self = [super initWithFrame:frame]) != nil) {
        [self setWantsLayer:YES];
        _score = -1;
        _isGameOver = -1 ? YES : NO;
        _videoLayer = [VideoLayer layer];
        [self setLayer:_videoLayer];
        CVDisplayLinkCreateWithActiveCGDisplays(&_displayLink);
        CVDisplayLinkSetOutputCallback(_displayLink, MyDisplayLinkCallback, (__bridge void*)self);
        CVDisplayLinkStart(_displayLink);
    }
    return self;
}

- (void)releaseDisplayLink
{
    if (_displayLink) {
        CVDisplayLinkStop(_displayLink);
        CVDisplayLinkRelease(_displayLink);
        _displayLink = nil;
    }
}

- (void)vsync
{
    emu_vsync();
    [self.videoLayer drawVRAM];
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (unichar)keyMapFrom:(unichar)c
{
    switch (c) {
        case '5': return 0x05;
        default: return c;
    }
}

- (void)keyDown:(NSEvent*)event
{
    unichar c = [self keyMapFrom:[event.charactersIgnoringModifiers characterAtIndex:0]];
    //NSLog(@"keyDown: %04X", tolower(c));
    switch (tolower(c)) {
        case 0xF703: emu_key |= MSX2_JOY_RI; break;
        case 0xF702: emu_key |= MSX2_JOY_LE; break;
        case 0xF701: emu_key |= MSX2_JOY_DW; break;
        case 0xF700: emu_key |= MSX2_JOY_UP; break;
        case 0x0078: emu_key |= MSX2_JOY_T2; break;
        case 0x007A: emu_key |= MSX2_JOY_T1; break;
        //case 0x0061: emu_startDebug(); break;
        //case 0x0064: emu_dumpVideoMemory(); break;
        //case 0x0072: emu_reset(); break;
        default: emu_keycode = c;
    }
}

- (void)keyUp:(NSEvent*)event
{
    unichar c = [self keyMapFrom:[event.charactersIgnoringModifiers characterAtIndex:0]];
    switch (tolower(c)) {
        case 0xF703: emu_key &= ~MSX2_JOY_RI; break;
        case 0xF702: emu_key &= ~MSX2_JOY_LE; break;
        case 0xF701: emu_key &= ~MSX2_JOY_DW; break;
        case 0xF700: emu_key &= ~MSX2_JOY_UP; break;
        case 0x0078: emu_key &= ~MSX2_JOY_T2; break;
        case 0x007A: emu_key &= ~MSX2_JOY_T1; break;
    }
    emu_keycode = 0;
}

static CVReturn MyDisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now, const CVTimeStamp* outputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* context)
{
    [(__bridge VideoLayer*)context performSelectorOnMainThread:@selector(vsync) withObject:nil waitUntilDone:NO];
    return kCVReturnSuccess;
}

@end
