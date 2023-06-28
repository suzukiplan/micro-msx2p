/**
 * micro MSX2+ - Example for iOS Objective-c
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
#import "ViewController.h"
#import "MenuView.h"
#import "VirtualPadView.h"
@import MSX2;

@interface ViewController () <MSX2ViewDelegate, MenuViewDelegate>
@property (nonatomic) UIView* container;
@property (nonatomic) MenuView* menuView;
@property (nonatomic) MSX2View* msx2View;
@property (nonatomic) VirtualPadView* virtualPadView;
@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = [UIColor blackColor];

    // Create Views
    _container = [[UIView alloc] init];
    [self.view addSubview:_container];
    _menuView = [[MenuView alloc] init];
    _menuView.delegate = self;
    [_container addSubview:_menuView];
    _msx2View = [[MSX2View alloc] init];
    _msx2View.delegate = self;
    [_container addSubview:_msx2View];
    _virtualPadView = [[VirtualPadView alloc] init];
    [_container addSubview:_virtualPadView];

    // Sub-threaded execution of time-consuming initialization processes such as BIOS file I/O to avoid degrading the app user experience
    __weak ViewController* weakSelf = self;
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        NSData* main = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"cbios_main_msx2+_jp" ofType:@"rom"]];
        if (!main || main.length < 1) {
            NSLog(@"main not found!");
        }
        NSData* logo = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"cbios_logo_msx2+" ofType:@"rom"]];
        NSData* sub = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"cbios_sub" ofType:@"rom"]];
        NSData* rom = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"game" ofType:@"rom"]];
        [weakSelf.msx2View setupWithCBiosMain:main
                                         logo:logo
                                          sub:sub
                                          rom:rom
                                      romType:MSX2RomTypeNormal
                                       select:0x1B
                                        start:0x20];
    });
}

- (void)viewWillAppear:(BOOL)animated
{
    [_msx2View resume];
}

- (void)viewDidAppear:(BOOL)animated
{
    [self resize];
}

- (void)viewWillDisappear:(BOOL)animated
{
    [_msx2View pause];
}

- (UIStatusBarStyle)preferredStatusBarStyle
{
    return UIStatusBarStyleLightContent;
}

- (void)resize
{
    CGFloat x = self.view.frame.origin.x + self.view.safeAreaInsets.left;
    CGFloat y = self.view.frame.origin.y + self.view.safeAreaInsets.top;
    CGFloat width = self.view.frame.size.width - self.view.safeAreaInsets.left - self.view.safeAreaInsets.right;
    CGFloat height = self.view.frame.size.height - self.view.safeAreaInsets.top - self.view.safeAreaInsets.bottom;
    _container.frame = CGRectMake(x, y, width, height);
    y = 0;
    _menuView.frame = CGRectMake(0, y, width, 44);
    y += 44;
    CGFloat h = (height - y) / 2;
    _msx2View.frame = CGRectMake(0, y, width, h);
    y += h;
    _virtualPadView.frame = CGRectMake(0, y, width, h);
}

- (NSInteger)didRequirePad1CodeWithView:(MSX2View*)view
{
    return _virtualPadView.code;
}

- (void)didStartWithView:(MSX2View*)view
{
    NSLog(@"Emulator start");
}

- (void)didStopWithView:(MSX2View*)view
{
    NSLog(@"Emulator stop");
}

- (void)menuView:(MenuView*)view didTapItem:(MenuViewItem)item
{
    switch (item) {
        case MenuviewItemReset:
            [_msx2View reset];
            break;
    }
}

@end
