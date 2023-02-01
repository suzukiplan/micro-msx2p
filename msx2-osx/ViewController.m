//
//  ViewController.m
//  msx2-osx
//
//  Created by Yoji Suzuki on 2023/01/23.
//

#import "emu.h"
#import "constants.h"
#import "AppDelegate.h"
#import "VideoView.h"
#import "ViewController.h"

@interface ViewController () <NSWindowDelegate, OpenFileDelegate>
@property (nonatomic, weak) AppDelegate* appDelegate;
@property (nonatomic) VideoView* video;
@property (nonatomic) NSData* rom;
@property (nonatomic) BOOL isFullScreen;
@property (nonatomic, nullable) NSData* saveData;
@end

@implementation ViewController

- (void)viewDidLoad
{
    [super viewDidLoad];

#if 0
    NSData* biosMain = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"cbios_main_msx2_jp" ofType:@"rom"]];
    NSData* biosLogo = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"cbios_logo_msx2" ofType:@"rom"]];
    NSData* biosSub = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"cbios_sub" ofType:@"rom"]];
    emu_init_cbios(biosMain.bytes, biosMain.length,
                   biosLogo.bytes, biosLogo.length,
                   biosSub.bytes, biosSub.length);
#elif 1
    NSData* biosMain = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"MSX2" ofType:@"ROM"]];
    NSData* biosExt = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"MSX2EXT" ofType:@"ROM"]];
    NSData* biosDisk = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"DISK" ofType:@"ROM"]];
    NSData* biosFm = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"FMBIOS" ofType:@"ROM"]];
    emu_init_bios(biosMain.bytes, biosMain.length,
                  biosExt.bytes, biosExt.length,
                  biosDisk.bytes, biosDisk.length,
                  biosFm.bytes, biosFm.length);
#else
    NSData* biosMain = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"MSX2P" ofType:@"ROM"]];
    NSData* biosExt = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"MSX2PEXT" ofType:@"ROM"]];
    NSData* biosDisk = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"DISK" ofType:@"ROM"]];
    NSData* biosFm = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"FMBIOS" ofType:@"ROM"]];
    emu_init_bios(biosMain.bytes, biosMain.length,
                  biosExt.bytes, biosExt.length,
                  biosDisk.bytes, biosDisk.length,
                  biosFm.bytes, biosFm.length);
#endif
    
#if 0
    NSString* romFile = [[NSUserDefaults standardUserDefaults] objectForKey:@"previous_rom_file"];
    if (romFile) {
        NSLog(@"previous_rom_file: %@", romFile);
        __weak ViewController* weakSelf = self;
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 0.5 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
            [weakSelf _openURL:[NSURL fileURLWithPath:romFile]];
        });
    }
#endif
    self.view.frame = CGRectMake(0, 0, VRAM_WIDTH * 2, VRAM_HEIGHT * 2);
    CALayer* layer = [CALayer layer];
    [layer setBackgroundColor:CGColorCreateGenericRGB(0.0, 0.0, 0.2525, 1.0)];
    [self.view setWantsLayer:YES];
    [self.view setLayer:layer];
    _video = [[VideoView alloc] initWithFrame:[self calcVramRect]];
    [self.view addSubview:_video];
    _appDelegate = (AppDelegate*)[NSApplication sharedApplication].delegate;
    NSLog(@"menu: %@", _appDelegate.menu);
    _appDelegate.menu.autoenablesItems = NO;
    //_appDelegate.menu.item
    _appDelegate.menuQuickLoadFromMemory.enabled = NO;
    _appDelegate.openFileDelegate = self;
    [self.view.window makeFirstResponder:_video];
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
    // Update the view, if already loaded.
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

- (void)dealloc
{
    emu_destroy();
}

- (void)menuOpenRomFile:(id)sender
{
    NSLog(@"menuOpenRomFile");
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    panel.allowsMultipleSelection = NO;
    panel.canChooseDirectories = NO;
    panel.canCreateDirectories = YES;
    panel.canChooseFiles = YES;
    __weak ViewController* weakSelf = self;
    [panel beginWithCompletionHandler:^(NSModalResponse result) {
        if (!result) return;
        [weakSelf _openURL:panel.URL];
    }];
}

- (void)_openURL:(NSURL*)url
{
    NSError* error;
    NSData* data = [NSData dataWithContentsOfURL:url options:NSDataReadingUncached error:&error];
    if (!data) {
        NSLog(@"File open error: %@", error);
        return;
    }
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
    emu_loadRom(data.bytes, data.length);
    [self.appDelegate.menuQuickLoadFromMemory setEnabled:NO];
    [[NSDocumentController sharedDocumentController] noteNewRecentDocumentURL:url];
    [[NSUserDefaults standardUserDefaults] setObject:url.path forKey:@"previous_rom_file"];
}

- (BOOL)application:(AppDelegate*)app didOpenFile:(NSString*)file
{
    [self _openURL:[NSURL fileURLWithPath:file]];
    return YES;
}

- (void)menuReset:(id)sender
{
    emu_reset();
}

- (void)windowDidEnterFullScreen:(NSNotification*)notification
{
    _isFullScreen = YES;
}

- (void)windowDidExitFullScreen:(NSNotification*)notification
{
    _isFullScreen = NO;
}

- (void)menuViewSize1x:(id)sender
{
    if (_isFullScreen) return;
    [self.view.window setContentSize:NSMakeSize(VRAM_WIDTH * 1, VRAM_HEIGHT * 1)];
}

- (void)menuViewSize2x:(id)sender
{
    if (_isFullScreen) return;
    [self.view.window setContentSize:NSMakeSize(VRAM_WIDTH * 2, VRAM_HEIGHT * 2)];
}

- (void)menuViewSize3x:(id)sender
{
    if (_isFullScreen) return;
    [self.view.window setContentSize:NSMakeSize(VRAM_WIDTH * 3, VRAM_HEIGHT * 3)];
}

- (void)menuViewSize4x:(id)sender
{
    if (_isFullScreen) return;
    [self.view.window setContentSize:NSMakeSize(VRAM_WIDTH * 4, VRAM_HEIGHT * 4)];
}

#if 0
- (void)menuQuickSaveToMemory:(id)sender
{
    [_appDelegate.menuQuickLoadFromMemory setEnabled:YES];
    size_t size;
    const void* data = emu_saveState(&size);
    NSLog(@"saving: %ld bytes", size);
    _saveData = [[NSData alloc] initWithBytes:data length:size];
}

- (void)menuQuickLoadFromMemory:(id)sender
{
    if (_saveData) {
        NSLog(@"loading");
        emu_loadState(_saveData.bytes, _saveData.length);
    }
}
#endif

@end
