//
//  ViewController.m
//  msx1-osx
//
//  Created by Yoji Suzuki on 2023/08/03.
//

#import "AppDelegate.h"
#import "VideoView.h"
#import "ViewController.h"
#import "constants.h"
#import "emu.h"

typedef NS_ENUM(NSInteger, OpenFileType) {
    OpenFileTypeRom,
};

@interface ViewController() <NSWindowDelegate>
@property (nonatomic) VideoView* video;
@property (nonatomic, weak) AppDelegate* appDelegate;
@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    NSData* biosMain = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"cbios_main_msx1" ofType:@"rom"]];
    NSData* biosLogo = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"cbios_logo_msx1" ofType:@"rom"]];
    emu_init_bios(biosMain.bytes, biosLogo.bytes);

    self.view.frame = CGRectMake(0, 0, VRAM_WIDTH * 2, VRAM_HEIGHT * 2);
    CALayer* layer = [CALayer layer];
    [layer setBackgroundColor:CGColorCreateGenericRGB(0.0, 0.0, 0.2525, 1.0)];
    [self.view setWantsLayer:YES];
    [self.view setLayer:layer];
    _video = [[VideoView alloc] initWithFrame:[self calcVramRect]];
    [self.view addSubview:_video];
    _appDelegate = (AppDelegate*)[NSApplication sharedApplication].delegate;
    [self.view.window makeFirstResponder:_video];
}

- (NSRect)calcVramRect
{
    // 幅を16とした時の高さのアスペクト比を計算
    CGFloat aspectY = VRAM_HEIGHT / (VRAM_WIDTH / 16.0);
    // window中央にVRAMをaspect-fitで描画
    if (self.view.frame.size.height < self.view.frame.size.width) {
        CGFloat height = self.view.frame.size.height;
        CGFloat width = height / aspectY * 16;
        CGFloat x = (self.view.frame.size.width - width) / 2;
        return NSRectFromCGRect(CGRectMake(x, 0, width, height));
    } else {
        CGFloat width = self.view.frame.size.width;
        CGFloat height = width / 16 * aspectY;
        CGFloat y = (self.view.frame.size.height - height) / 2;
        return NSRectFromCGRect(CGRectMake(0, y, width, height));
    }
}

- (void)viewWillAppear
{
    self.view.window.delegate = self;
}

- (void)windowDidResize:(NSNotification*)notification
{
    _video.frame = [self calcVramRect];
}

- (void)setRepresentedObject:(id)representedObject
{
    [super setRepresentedObject:representedObject];
}

- (void)dealloc
{
    emu_destroy();
}

- (IBAction)menuQuit:(id)sender
{
    exit(0);
}

- (IBAction)menuOpenRomFile:(id)sender
{
    NSLog(@"menuOpenRomFile");
    [self _openWithType:OpenFileTypeRom];
}

- (void)_openWithType:(OpenFileType)type
{
    __weak ViewController* weakSelf = self;
    [_video pauseWithCompletionHandler:^{
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        panel.allowsMultipleSelection = NO;
        panel.canChooseDirectories = NO;
        panel.canCreateDirectories = YES;
        panel.canChooseFiles = YES;
        [panel beginWithCompletionHandler:^(NSModalResponse result) {
            if (!result) {
                [weakSelf.video resumeWithCompletionHandler:^{}];
                return;
            }
            [weakSelf _openURL:panel.URL type:type];
        }];
    }];
}

- (void)_openURL:(NSURL*)url type:(OpenFileType)type
{
    NSError* error;
    NSData* data = [NSData dataWithContentsOfURL:url options:NSDataReadingUncached error:&error];
    if (!data) {
        NSLog(@"File open error: %@", error);
        return;
    }
    switch (type) {
        case OpenFileTypeRom: {
            char gameCode[16];
            memset(gameCode, 0, sizeof(gameCode));
            gameCode[0] = '\0';
            const char* fileName = strrchr(url.path.UTF8String, '/');
            if (fileName) {
                while ('/' == *fileName || isdigit(*fileName)) fileName++;
                if ('_' == *fileName) fileName++;
                strncpy(gameCode, fileName, 15);
                char* cp = strchr(gameCode, '.');
                if (cp) *cp = '\0';
            }
            NSLog(@"loading game: %s", gameCode);
            emu_loadRom(data.bytes, data.length, gameCode);
            break;
        }
    }
    [_video resumeWithCompletionHandler:^{}];
}

@end
