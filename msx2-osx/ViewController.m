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
};

typedef NS_ENUM(NSInteger, SaveFileType) {
    SaveFileTypeQuick,
    SaveFileTypeRAM,
    SaveFileTypeVRAM,
    SaveFileTypeBitmapVRAM,
};

@interface ViewController () <NSWindowDelegate>
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
#elif 0
    NSData* biosMain = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"MSX2" ofType:@"ROM"]];
    NSData* biosExt = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"MSX2EXT" ofType:@"ROM"]];
    NSData* biosDisk = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"DISK" ofType:@"ROM"]];
    NSData* biosFm = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"FMBIOS" ofType:@"ROM"]];
    NSData* biosKnj = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"KNJDRV" ofType:@"ROM"]];
    NSData* biosFont = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"KNJFNT16" ofType:@"ROM"]];
    emu_init_bios(biosMain.bytes, biosMain.length,
                  biosExt.bytes, biosExt.length,
                  biosDisk.bytes, biosDisk.length,
                  biosFm.bytes, biosFm.length,
                  biosKnj.bytes, biosKnj.length,
                  biosFont.bytes, biosFont.length);
#elif 0
    /*
     void emu_init_bios_tm1p(const void* tm1pbios,
                             const void* tm1pext,
                             const void* tm1pkdr,
                             const void* tm1pdesk1,
                             const void* tm1pdesk2);
     */
    NSData* tm1pbios = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"tm1pbios" ofType:@"rom"]];
    NSData* tm1pext = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"tm1pext" ofType:@"rom"]];
    NSData* tm1pkdr = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"tm1pkdr" ofType:@"rom"]];
    NSData* tm1pdesk1 = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"tm1pdesk1" ofType:@"rom"]];
    NSData* tm1pdesk2 = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"tm1pdesk2" ofType:@"rom"]];
    NSData* font = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"KNJFNT16" ofType:@"ROM"]];
    emu_init_bios_tm1p(tm1pbios.bytes,
                       tm1pext.bytes,
                       tm1pkdr.bytes,
                       tm1pdesk1.bytes,
                       tm1pdesk2.bytes,
                       font.bytes, font.length);
#elif 0
    NSData* msx2p = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"a1wsbios" ofType:@"rom"]];
    NSData* msx2pext = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"a1wsext" ofType:@"rom"]];
    NSData* msx2pmus = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"a1wsmus" ofType:@"rom"]];
    NSData* disk = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"a1wsdisk" ofType:@"rom"]];
    NSData* msxkanji = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"a1wskdr" ofType:@"rom"]];
    NSData* kanji = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"a1wskfn" ofType:@"rom"]];
    NSData* firm = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"a1wsfirm" ofType:@"rom"]];
    emu_init_bios_fsa1wsx(msx2p.bytes,
                          msx2pext.bytes,
                          msx2pmus.bytes,
                          disk.bytes,
                          msxkanji.bytes,
                          kanji.bytes,
                          firm.bytes);
#else
    NSData* biosMain = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"_MSX2P_a" ofType:@"ROM"]];
    NSData* biosExt = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"_MSX2PEXT" ofType:@"ROM"]];
    NSData* biosDisk = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"FSA1WSX_DISK" ofType:@"ROM"]];
    NSData* biosFm = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"_FMPAC" ofType:@"ROM"]];
    NSData* biosKnj = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"_KNJDRV" ofType:@"ROM"]];
    NSData* biosFont = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"_KNJFNT16" ofType:@"ROM"]];
    emu_init_bios(biosMain.bytes,
                  biosExt.bytes,
                  biosDisk.bytes,
                  biosFm.bytes,
                  biosKnj.bytes,
                  biosFont.bytes);
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

@end
