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

typedef NS_ENUM(NSInteger, OpenFileType) {
    OpenFileTypeRom,
    OpenFileTypeDiskA,
    OpenFileTypeDiskB,
    OpenFileTypeQuickSave,
    OpenFileTypeReplay,
};

typedef NS_ENUM(NSInteger, SaveFileType) {
    SaveFileTypeQuick,
    SaveFileTypeRAM,
    SaveFileTypeVRAM,
    SaveFileTypeBitmapVRAM,
    SaveFileTypeBitmapSprite,
    SaveFileTypeBitmapScreen,
    SaveFileTypePlaylog,
};

@interface ViewController () <NSWindowDelegate>
@property (nonatomic, weak) AppDelegate* appDelegate;
@property (nonatomic) VideoView* video;
@property (nonatomic) NSImageView* recIcon;
@property (nonatomic) NSData* rom;
@property (nonatomic) BOOL isFullScreen;
@property (nonatomic, nullable) NSData* saveData;
@property (nonatomic, weak) NSMenuItem* menuOpenROM;
@property (nonatomic, weak) NSMenuItem* menuInsertDisk1;
@property (nonatomic, weak) NSMenuItem* menuEjectDisk1;
@property (nonatomic, weak) NSMenuItem* menuInsertDisk2;
@property (nonatomic, weak) NSMenuItem* menuEjectDisk2;
@property (nonatomic, weak) NSMenuItem* menuQuickLoad;
@property (nonatomic, weak) NSMenuItem* menuStartRecordingPlaylog;
@property (nonatomic, weak) NSMenuItem* menuStopRecordingPlaylog;
@property (nonatomic, weak) NSMenuItem* menuReplayRecordedPlaylog;
@property (nonatomic, weak) NSMenuItem* menuStopReplayPlaylog;
@end

@implementation ViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    
#if 0
    NSData* biosMain = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"cbios_main_msx2+_jp" ofType:@"rom"]];
    NSData* biosLogo = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"cbios_logo_msx2+" ofType:@"rom"]];
    NSData* biosSub = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"cbios_sub" ofType:@"rom"]];
    emu_init_cbios(biosMain.bytes, biosLogo.bytes, biosSub.bytes);
#elif 0
    // MSX2
    NSData* biosMain = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"MSX2" ofType:@"ROM"]];
    NSData* biosExt = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"MSX2EXT" ofType:@"ROM"]];
    NSData* biosDisk = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"DISK" ofType:@"ROM"]];
    NSData* biosFm = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"FMBIOS" ofType:@"ROM"]];
    NSData* biosKnj = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"KNJDRV" ofType:@"ROM"]];
    NSData* biosFont = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"KNJFNT16" ofType:@"ROM"]];
    emu_init_bios(biosMain.bytes,
                  biosExt.bytes,
                  biosDisk.bytes,
                  biosFm.bytes,
                  biosKnj.bytes,
                  biosFont.bytes);
#else
    // MSX2+
    NSData* biosMain = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"MSX2P" ofType:@"ROM"]];
    NSData* biosExt = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"MSX2PEXT" ofType:@"ROM"]];
    NSData* biosDisk = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"DISK" ofType:@"ROM"]];
    NSData* biosFm = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"FMBIOS" ofType:@"ROM"]];
    NSData* biosKnj = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"KNJDRV" ofType:@"ROM"]];
    NSData* biosFont = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"KNJFNT16" ofType:@"ROM"]];
    emu_init_bios(biosMain.bytes,
                  biosExt.bytes,
                  biosDisk.bytes,
                  biosFm.bytes,
                  biosKnj.bytes,
                  biosFont.bytes);
#endif
    
    self.view.frame = CGRectMake(0, 0, VRAM_WIDTH, VRAM_HEIGHT * 2);
    CALayer* layer = [CALayer layer];
    [layer setBackgroundColor:CGColorCreateGenericRGB(0.0, 0.0, 0.2525, 1.0)];
    [self.view setWantsLayer:YES];
    [self.view setLayer:layer];
    _video = [[VideoView alloc] initWithFrame:[self calcVramRect]];
    [self.view addSubview:_video];
    _recIcon = [[NSImageView alloc] initWithFrame:CGRectMake(8, 8, 80, 16)];
    _recIcon.image = [NSImage imageNamed:@"rec"];
    [self.view addSubview:_recIcon];
    _appDelegate = (AppDelegate*)[NSApplication sharedApplication].delegate;
    [self.view.window makeFirstResponder:_video];
    
    NSMenu* mainMenu = [NSApplication sharedApplication].mainMenu;
    [mainMenu setAutoenablesItems:NO];
    for (NSMenuItem* item in [mainMenu itemArray]) {
        [item.submenu setAutoenablesItems:NO];
        for (NSMenuItem* sub in [item.submenu itemArray]) {
            NSLog(@"identifer: %@", sub.identifier);
            if ([sub.identifier isEqualToString:@"OpenROM"]) {
                _menuOpenROM = sub;
            } else if ([sub.identifier isEqualToString:@"InsertDisk1"]) {
                _menuInsertDisk1 = sub;
            } else if ([sub.identifier isEqualToString:@"EjectDisk1"]) {
                _menuEjectDisk1 = sub;
            } else if ([sub.identifier isEqualToString:@"InsertDisk2"]) {
                _menuInsertDisk2 = sub;
            } else if ([sub.identifier isEqualToString:@"EjectDisk2"]) {
                _menuEjectDisk2 = sub;
            } else if ([sub.identifier isEqualToString:@"QuickLoad"]) {
                _menuQuickLoad = sub;
            } else if ([sub.identifier isEqualToString:@"StartRecordingPlaylog"]) {
                _menuStartRecordingPlaylog = sub;
            } else if ([sub.identifier isEqualToString:@"StopRecordingPlaylog"]) {
                _menuStopRecordingPlaylog = sub;
            } else if ([sub.identifier isEqualToString:@"ReplayRecordedPlaylog"]) {
                _menuReplayRecordedPlaylog = sub;
            } else if ([sub.identifier isEqualToString:@"StopReplayPlaylog"]) {
                _menuStopReplayPlaylog = sub;
            }
        }
    }
    [self _setMenuItemEnabledForDefault];
}

- (void)_setMenuItemEnabledForDefault
{
    [_menuOpenROM setEnabled:YES];
    [_menuInsertDisk1 setEnabled:YES];
    [_menuEjectDisk1 setEnabled:YES];
    [_menuInsertDisk2 setEnabled:YES];
    [_menuEjectDisk2 setEnabled:YES];
    [_menuQuickLoad setEnabled:YES];
    [_menuStartRecordingPlaylog setEnabled:YES];
    [_menuStopRecordingPlaylog setEnabled:NO];
    [_menuReplayRecordedPlaylog setEnabled:YES];
    [_menuStopReplayPlaylog setEnabled:NO];
    _recIcon.hidden = YES;
}

- (void)_setMenuItemEnabledForStartRecoding
{
    [_menuOpenROM setEnabled:NO];
    [_menuInsertDisk1 setEnabled:NO];
    [_menuEjectDisk1 setEnabled:NO];
    [_menuInsertDisk2 setEnabled:NO];
    [_menuEjectDisk2 setEnabled:NO];
    [_menuQuickLoad setEnabled:NO];
    [_menuStartRecordingPlaylog setEnabled:NO];
    [_menuStopRecordingPlaylog setEnabled:YES];
    [_menuReplayRecordedPlaylog setEnabled:NO];
    [_menuStopReplayPlaylog setEnabled:NO];
    _recIcon.hidden = NO;
}

- (void)_setMenuItemEnabledForStartReplay
{
    [_menuOpenROM setEnabled:NO];
    [_menuInsertDisk1 setEnabled:NO];
    [_menuEjectDisk1 setEnabled:NO];
    [_menuInsertDisk2 setEnabled:NO];
    [_menuEjectDisk2 setEnabled:NO];
    [_menuQuickLoad setEnabled:NO];
    [_menuStartRecordingPlaylog setEnabled:NO];
    [_menuStopRecordingPlaylog setEnabled:NO];
    [_menuReplayRecordedPlaylog setEnabled:NO];
    [_menuStopReplayPlaylog setEnabled:YES];
    _recIcon.hidden = YES;
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
    CGFloat aspectY = VRAM_HEIGHT * 2 / (VRAM_WIDTH / 16.0);
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
    [self _openWithType:OpenFileTypeRom];
}

-(IBAction)menuInsertDiskA:(id)sender
{
    NSLog(@"menuInsertDiskA");
    [self _openWithType:OpenFileTypeDiskA];
}

-(IBAction)menuEjectDiskA:(id)sender
{
    NSLog(@"menuEjectDiskA");
    emu_ejectDisk(0);
}

-(IBAction)menuInsertDiskB:(id)sender
{
    NSLog(@"menuInsertDiskB");
    [self _openWithType:OpenFileTypeDiskB];
}

-(IBAction)menuEjectDiskB:(id)sender
{
    NSLog(@"menuEjectDiskB");
    emu_ejectDisk(1);
}

- (IBAction)menuReset:(id)sender;
{
    emu_reset();
}

- (IBAction)menuQuickSave:(id)sender
{
    NSLog(@"menuQuickSave");
    __weak ViewController* weakSelf = self;
    [_video pauseWithCompletionHandler:^{
        NSLog(@"paused");
        size_t size;
        const void* rawData = emu_quickSave(&size);
        NSData* data = [NSData dataWithBytes:rawData length:size];
        NSLog(@"save data size = %lu", size);
        [weakSelf _saveData:data type:SaveFileTypeQuick];
    }];
}

- (IBAction)menuQuickLoad:(id)sender
{
    NSLog(@"menuQuickLoad");
    [self _openWithType:OpenFileTypeQuickSave];
}

- (IBAction)menuSaveRamDump:(id)sender
{
    NSLog(@"menuSaveRamDump");
    __weak ViewController* weakSelf = self;
    [_video pauseWithCompletionHandler:^{
        NSData* data = [NSData dataWithBytes:emu_getRAM() length:0x10000];
        [weakSelf _saveData:data type:SaveFileTypeRAM];
    }];
}

- (IBAction)menuSaveVramDump:(id)sender
{
    NSLog(@"menuSaveVramDump");
    __weak ViewController* weakSelf = self;
    [_video pauseWithCompletionHandler:^{
        NSData* data = [NSData dataWithBytes:emu_getVRAM() length:0x20000];
        [weakSelf _saveData:data type:SaveFileTypeVRAM];
    }];
}

- (IBAction)menuSaveVramBitmap:(id)sender
{
    NSLog(@"menuSaveVramBitmap");
    __weak ViewController* weakSelf = self;
    [_video pauseWithCompletionHandler:^{
        size_t size;
        const void* raw = emu_getBitmapVRAM(&size);
        if (!raw) {
            [weakSelf.video resumeWithCompletionHandler:^{
                NSLog(@"unsupported");
            }];
        } else {
            NSLog(@"bitmap size: %lu bytes", size);
            NSData* data = [NSData dataWithBytes:raw length:size];
            [weakSelf _saveData:data type:SaveFileTypeBitmapVRAM];
        }
    }];
}

- (IBAction)menuSaveScreenBitmap:(id)sender
{
    NSLog(@"menuSaveScreenBitmap");
    __weak ViewController* weakSelf = self;
    [_video pauseWithCompletionHandler:^{
        size_t size;
        const void* raw = emu_getBitmapScreen(&size);
        if (!raw) {
            [weakSelf.video resumeWithCompletionHandler:^{
                NSLog(@"unsupported");
            }];
        } else {
            NSLog(@"bitmap size: %lu bytes", size);
            NSData* data = [NSData dataWithBytes:raw length:size];
            [weakSelf _saveData:data type:SaveFileTypeBitmapScreen];
        }
    }];
}

-(IBAction)menuSaveSpritePattern:(id)sender
{
    NSLog(@"menuSaveSpritePattern");
    __weak ViewController* weakSelf = self;
    [_video pauseWithCompletionHandler:^{
        size_t size;
        const void* raw = emu_getBitmapSprite(&size);
        if (!raw) {
            [weakSelf.video resumeWithCompletionHandler:^{
                NSLog(@"unsupported");
            }];
        } else {
            NSLog(@"bitmap size: %lu bytes", size);
            NSData* data = [NSData dataWithBytes:raw length:size];
            [weakSelf _saveData:data type:SaveFileTypeBitmapSprite];
        }
    }];
}

-(IBAction)menuStop:(id)sender
{
    static char stop[2] = { 0x18, 0x00 };
    emu_startTypeWriter(stop);
}

- (IBAction)menuPause:(id)sender
{
    if (_video.pausing) {
        [_video resumeWithCompletionHandler:^{
            NSLog(@"Resumed");
        }];
    } else {
        [_video pauseWithCompletionHandler:^{
            NSLog(@"Paused");
        }];
    }
}

-(IBAction)menuTypeFromClipboard:(id)sender
{
    NSLog(@"menuTypeFromClipboard");
    NSString* str = [[NSPasteboard generalPasteboard] stringForType:NSPasteboardTypeString];
    if (str) {
        emu_startTypeWriter(str.UTF8String);
    }
}

- (void)_saveData:(NSData*)data type:(SaveFileType)type
{
    [_video pauseWithCompletionHandler:^{
        NSSavePanel* panel = [NSSavePanel savePanel];
        panel.canCreateDirectories = YES;
        panel.showsTagField = YES;
        switch (type) {
            case SaveFileTypeQuick:
                panel.nameFieldStringValue = @"save.dat";
                break;
            case SaveFileTypeRAM:
                panel.nameFieldStringValue = @"ram.bin";
                break;
            case SaveFileTypeVRAM:
                panel.nameFieldStringValue = @"vram.bin";
                break;
            case SaveFileTypeBitmapVRAM:
                panel.nameFieldStringValue = @"vram.bmp";
                break;
            case SaveFileTypeBitmapScreen:
                panel.nameFieldStringValue = @"screenshot.bmp";
                break;
            case SaveFileTypeBitmapSprite:
                panel.nameFieldStringValue = @"sprite.bmp";
                break;
            case SaveFileTypePlaylog:
                panel.nameFieldStringValue = @"playlog.m2p";
                break;
        }
        panel.level = NSModalPanelWindowLevel;
        __weak ViewController* weakSelf = self;
        [panel beginWithCompletionHandler:^(NSModalResponse result) {
            if (result) {
                [data writeToURL:panel.URL atomically:YES];
            }
            [weakSelf.video resumeWithCompletionHandler:^{
                NSLog(@"resumed");
            }];
        }];
    }];
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
        case OpenFileTypeDiskA:
            emu_insertDisk(0, data.bytes, data.length);
            break;
        case OpenFileTypeDiskB:
            emu_insertDisk(1, data.bytes, data.length);
            break;
        case OpenFileTypeQuickSave:
            emu_quickLoad(data.bytes, data.length);
            break;
        case OpenFileTypeReplay:
            emu_startReplay(data.bytes, data.length);
            [self _setMenuItemEnabledForStartReplay];
            break;
    }
    [_video resumeWithCompletionHandler:^{}];
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

-(IBAction)menuLoggingOnce:(id)sender
{
    NSLog(@"menuLoggingOnce");
    emu_loggingOnce();
}

-(IBAction)menuStartRcodingPlaylog:(id)sender
{
    [self _setMenuItemEnabledForStartRecoding];
    emu_startRecording();
}

-(IBAction)menuStopRcodingPlaylog:(id)sender
{
    size_t size;
    void* rawData = emu_stopPlaylog(&size);
    if (rawData) {
        NSData* data = [NSData dataWithBytes:rawData length:size];
        free(rawData);
        __weak ViewController* weakSelf = self;
        [_video pauseWithCompletionHandler:^{
            [weakSelf _setMenuItemEnabledForDefault];
            [weakSelf _saveData:data type:SaveFileTypePlaylog];
        }];
    }
}

-(IBAction)menuReplayRecordedPlaylog:(id)sender
{
    __weak ViewController* weakSelf = self;
    [_video pauseWithCompletionHandler:^{
        [weakSelf _openWithType:OpenFileTypeReplay];
    }];
}

-(IBAction)menuStopReplayPlaylog:(id)sender
{
    emu_stopReplay();
    [self _setMenuItemEnabledForDefault];
}

@end
