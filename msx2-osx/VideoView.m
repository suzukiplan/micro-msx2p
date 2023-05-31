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

// Comment out the following #define if you want to assign cursor, Z and X on the keyboard as the original key, not the joypad.
#define ASSIGN_KEYBOARD_TO_JOYPAD

static CVReturn MyDisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now, const CVTimeStamp* outputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* context);
extern unsigned char emu_key;
extern unsigned char emu_keycode;

@interface VideoView ()
@property (nonatomic) VideoLayer* videoLayer;
@property CVDisplayLinkRef displayLink;
@property (nonatomic, readwrite) BOOL pausing;
@property (nonatomic, readwrite) BOOL paused;
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
    if (!_pausing) {
        _paused = NO;
        emu_vsync();
        [self.videoLayer drawVRAM];
    } else {
        _paused = YES;
    }
}

- (void)pauseWithCompletionHandler:(void (^)(void))completionHandler
{
    if (_pausing) {
        completionHandler();
        return;
    }
    __weak VideoView* wealSelf = self;
    _paused = NO;
    _pausing = YES;
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        while (NO == wealSelf.paused) usleep(100);
        dispatch_async(dispatch_get_main_queue(), ^{
            completionHandler();
        });
    });
}

- (void)resumeWithCompletionHandler:(void (^)(void))completionHandler
{
    if (!_pausing) {
        completionHandler();
        return;
    }
    __weak VideoView* wealSelf = self;
    _paused = YES;
    _pausing = NO;
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        while (YES == wealSelf.paused) usleep(100);
        dispatch_async(dispatch_get_main_queue(), ^{
            completionHandler();
        });
    });
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
#ifdef ASSIGN_KEYBOARD_TO_JOYPAD
        case 0xF703: emu_key |= MSX2_JOY_RI; break;
        case 0xF702: emu_key |= MSX2_JOY_LE; break;
        case 0xF701: emu_key |= MSX2_JOY_DW; break;
        case 0xF700: emu_key |= MSX2_JOY_UP; break;
        case 0x0078: emu_key |= MSX2_JOY_T2; break;
        case 0x007A: emu_key |= MSX2_JOY_T1; break;
#else
        case 0xF700: emu_keycode = 0xC0; break;
        case 0xF701: emu_keycode = 0xC1; break;
        case 0xF702: emu_keycode = 0xC2; break;
        case 0xF703: emu_keycode = 0xC3; break;
#endif
        case 0xF704: emu_keycode = 0xF1; break;
        case 0xF705: emu_keycode = 0xF2; break;
        case 0xF706: emu_keycode = 0xF3; break;
        case 0xF707: emu_keycode = 0xF4; break;
        case 0xF708: emu_keycode = 0xF5; break;
        case 0xF709: emu_keycode = 0xF6; break;
        case 0xF70A: emu_keycode = 0xF7; break;
        case 0xF70B: emu_keycode = 0xF8; break;
        case 0xF70C: emu_keycode = 0xF9; break;
        case 0xF70D: emu_keycode = 0xFA; break;
        default: emu_keycode = c;
    }
}

- (void)keyUp:(NSEvent*)event
{
#ifdef ASSIGN_KEYBOARD_TO_JOYPAD
    unichar c = [self keyMapFrom:[event.charactersIgnoringModifiers characterAtIndex:0]];
    switch (tolower(c)) {
        case 0xF703: emu_key &= ~MSX2_JOY_RI; break;
        case 0xF702: emu_key &= ~MSX2_JOY_LE; break;
        case 0xF701: emu_key &= ~MSX2_JOY_DW; break;
        case 0xF700: emu_key &= ~MSX2_JOY_UP; break;
        case 0x0078: emu_key &= ~MSX2_JOY_T2; break;
        case 0x007A: emu_key &= ~MSX2_JOY_T1; break;
    }
#endif
    emu_keycode = 0;
}

static CVReturn MyDisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now, const CVTimeStamp* outputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* context)
{
    [(__bridge VideoLayer*)context performSelectorOnMainThread:@selector(vsync) withObject:nil waitUntilDone:NO];
    return kCVReturnSuccess;
}

@end
