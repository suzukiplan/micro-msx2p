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
#import "VirtualPadView.h"
@import MSX2;

@interface VirtualPadView()
@property (nonatomic) MSX2JoyPad* joyPad;
@property (nonatomic) UIView* cursorContainer;
@property (nonatomic) UIImageView* cursorUp;
@property (nonatomic) UIImageView* cursorDown;
@property (nonatomic) UIImageView* cursorLeft;
@property (nonatomic) UIImageView* cursorRight;
@property (nonatomic) UIView* buttonContainer;
@property (nonatomic) UILabel* labelA;
@property (nonatomic) UILabel* labelB;
@property (nonatomic) UIImageView* buttonA;
@property (nonatomic) UIImageView* buttonB;
@property (nonatomic) UIView* ctrlContainer;
@property (nonatomic) UILabel* labelSelect;
@property (nonatomic) UILabel* labelStart;
@property (nonatomic) UIImageView* ctrlSelect;
@property (nonatomic) UIImageView* ctrlStart;
@property (nonatomic) NSMutableArray<UITouch*>* cursorTouches;
@property (nonatomic) NSMutableArray<UITouch*>* buttonTouches;
@property (nonatomic) NSMutableArray<UITouch*>* ctrlTouches;
@end

@implementation VirtualPadView

- (instancetype)init
{
    if (self = [super init]) {
        _joyPad = [[MSX2JoyPad alloc] init];
        _cursorTouches = [NSMutableArray array];
        _buttonTouches = [NSMutableArray array];
        _ctrlTouches = [NSMutableArray array];

        _cursorContainer = [[UIView alloc] init];
        _cursorContainer.backgroundColor = [UIColor colorWithWhite:0.1 alpha:1.0];
        _cursorContainer.userInteractionEnabled = NO;
        [self addSubview:_cursorContainer];
        _cursorUp = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"pad_up_off"]];
        _cursorUp.userInteractionEnabled = NO;
        _cursorDown = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"pad_down_off"]];
        _cursorDown.userInteractionEnabled = NO;
        _cursorLeft = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"pad_left_off"]];
        _cursorLeft.userInteractionEnabled = NO;
        _cursorRight = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"pad_right_off"]];
        _cursorRight.userInteractionEnabled = NO;
        [_cursorContainer addSubview:_cursorUp];
        [_cursorContainer addSubview:_cursorDown];
        [_cursorContainer addSubview:_cursorLeft];
        [_cursorContainer addSubview:_cursorRight];
        
        _buttonContainer = [[UIView alloc] init];
        _buttonContainer.backgroundColor = [UIColor colorWithWhite:0.1 alpha:1.0];
        _buttonContainer.userInteractionEnabled = NO;
        [self addSubview:_buttonContainer];
        _buttonA = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"pad_btn_off"]];
        _buttonA.userInteractionEnabled = NO;
        _buttonB = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"pad_btn_off"]];
        _buttonB.userInteractionEnabled = NO;
        _labelA = [self makeButtonLabel:@"A"];
        _labelB = [self makeButtonLabel:@"B"];
        [_buttonContainer addSubview:_buttonA];
        [_buttonContainer addSubview:_buttonB];
        [_buttonContainer addSubview:_labelA];
        [_buttonContainer addSubview:_labelB];

        _ctrlContainer = [[UIView alloc] init];
        _ctrlContainer.userInteractionEnabled = NO;
        [self addSubview:_ctrlContainer];
        _ctrlStart = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"pad_ctrl_off"]];
        _ctrlStart.userInteractionEnabled = NO;
        _ctrlSelect = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"pad_ctrl_off"]];
        _ctrlSelect.userInteractionEnabled = NO;
        _labelStart = [self makeButtonLabel:@"SPACE"];
        _labelSelect = [self makeButtonLabel:@"ESC"];
        [_ctrlContainer addSubview:_ctrlStart];
        [_ctrlContainer addSubview:_ctrlSelect];
        [_ctrlContainer addSubview:_labelStart];
        [_ctrlContainer addSubview:_labelSelect];
    }
    return self;
}

- (UILabel*)makeButtonLabel:(NSString*)label
{
    UILabel* result = [[UILabel alloc] init];
    result.font = [UIFont systemFontOfSize:12];
    result.textColor = [UIColor colorWithRed:0.874509803921569
                                       green:0.443137254901961
                                        blue:0.149019607843137
                                       alpha:1.0];
    result.text = label;
    result.userInteractionEnabled = NO;
    return result;
}

- (NSInteger)code
{
    return _joyPad.code;
}

- (void)setFrame:(CGRect)frame
{
    [super setFrame:frame];
    CGFloat x = 0;
    CGFloat h = _labelStart.intrinsicContentSize.height + 44;
    CGFloat y = frame.size.height - h;
    CGFloat w = frame.size.width;
    _ctrlContainer.frame = CGRectMake(x, y, w, h);
    w = 88;
    h = 44;
    x = (_ctrlContainer.frame.size.width - w * 2 - 16) / 2;
    y = _ctrlContainer.frame.size.height - h;
    _ctrlSelect.frame = CGRectMake(x, y, w, h);
    x += w + 16;
    _ctrlStart.frame = CGRectMake(x, y, w, h);
    w = _labelSelect.intrinsicContentSize.width;
    h = _labelSelect.intrinsicContentSize.height;
    x = _ctrlSelect.frame.origin.x + (88 - w) / 2;
    y = 0;
    _labelSelect.frame = CGRectMake(x, y, w, h);
    w = _labelStart.intrinsicContentSize.width;
    h = _labelStart.intrinsicContentSize.height;
    x = _ctrlStart.frame.origin.x + (88 - w) / 2;
    y = 0;
    _labelStart.frame = CGRectMake(x, y, w, h);
    
    CGFloat cs = (frame.size.width - 8 * 3) / 2;
    w = cs;
    h = cs;
    x = 8;
    y = (frame.size.height - _ctrlContainer.frame.size.height - h) / 2;
    _cursorContainer.frame = CGRectMake(x, y, w, h);
    x += w + 8;
    _buttonContainer.frame = CGRectMake(x, y, w, h);
    
    w = 48;
    h = 48;
    x = (cs - w) / 2;
    y = 8;
    _cursorUp.frame = CGRectMake(x, y, w, h);
    y = cs - h - 8;
    _cursorDown.frame = CGRectMake(x, y, w, h);
    y = x;
    x = 8;
    _cursorLeft.frame = CGRectMake(x, y, w, h);
    x = (cs / 2 - w) / 2;
    _buttonB.frame = CGRectMake(x, y, w, h);
    x = cs - w - 8;
    _cursorRight.frame = CGRectMake(x, y, w, h);
    x = cs / 2 + (cs / 2 - w) / 2;
    _buttonA.frame = CGRectMake(x, y, w, h);

    w = _labelB.intrinsicContentSize.width;
    h = _labelB.intrinsicContentSize.height;
    x = _buttonB.frame.origin.x + (48 - w) / 2;
    y = _buttonB.frame.origin.y - h - 8;
    _labelB.frame = CGRectMake(x, y, w, h);

    w = _labelA.intrinsicContentSize.width;
    h = _labelA.intrinsicContentSize.height;
    x = _buttonA.frame.origin.x + (48 - w) / 2;
    y = _buttonA.frame.origin.y - h - 8;
    _labelA.frame = CGRectMake(x, y, w, h);
}

- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
    for (UITouch* touch in touches) {
        CGPoint pos = [touch locationInView:self];
        if (CGRectContainsPoint(_cursorContainer.frame, pos)) {
            [_cursorTouches addObject:touch];
        } else if (CGRectContainsPoint(_buttonContainer.frame, pos)) {
            [_buttonTouches addObject:touch];
        } else if (CGRectContainsPoint(_ctrlContainer.frame, pos)) {
            [_ctrlTouches addObject:touch];
        }
    }
    [self hitTest];
}

- (void)touchesMoved:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
    [self hitTest];
}

- (void)removeTouch:(UITouch*)touch
{
    [_cursorTouches removeObject:touch];
    [_buttonTouches removeObject:touch];
    [_ctrlTouches removeObject:touch];
}

- (void)touchesCancelled:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
    for (UITouch* touch in touches) {
        [self removeTouch:touch];
    }
    [self hitTest];
}

- (void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
    for (UITouch* touch in touches) {
        [self removeTouch:touch];
    }
    [self hitTest];
}

- (void)hitTest
{
    BOOL up = _joyPad.up;
    BOOL down = _joyPad.down;
    BOOL left = _joyPad.left;
    BOOL right = _joyPad.right;
    BOOL a = _joyPad.a;
    BOOL b = _joyPad.b;
    BOOL start = _joyPad.start;
    BOOL select = _joyPad.select;
    NSInteger previousCode = _joyPad.code;

    _joyPad.up = NO;
    _joyPad.down = NO;
    _joyPad.left = NO;
    _joyPad.right = NO;
    int cx = _cursorContainer.frame.size.width / 2;
    int cy = _cursorContainer.frame.size.height / 2;
    for (UITouch* touch in _cursorTouches) {
        CGPoint ptr = [touch locationInView:_cursorContainer];
        if (abs((int)ptr.x - cx) < 8 && abs((int)ptr.y - cy) < 8) {
            continue;
        }
        float r = atan2f(ptr.x - cx, ptr.y - cy) + M_PI / 2;
        while (M_PI * 2 <= r) r -= M_PI * 2;
        while (r < 0) r += M_PI * 2;
        _joyPad.down = 0.5235987755983 <= r && r <= 2.6179938779915;
        _joyPad.right = 2.2689280275926 <= r && r <= 4.1887902047864;
        _joyPad.up = 3.6651914291881 <= r && r <= 5.7595865315813;
        _joyPad.left = r <= 1.0471975511966 || 5.235987755983 <= r;
    }
    
    _joyPad.a = NO;
    _joyPad.b = NO;
    int bE = _buttonB.frame.origin.x + _buttonB.frame.size.width + 4;
    int aS = _buttonA.frame.origin.x - 4;
    for (UITouch* touch in _buttonTouches) {
        CGFloat x = [touch locationInView:_buttonContainer].x;
        if (x < bE) {
            _joyPad.b = YES;
        } else if (aS < x) {
            _joyPad.a = YES;
        } else {
            _joyPad.b = YES;
            _joyPad.a = YES;
        }
    }
    
    _joyPad.select = NO;
    _joyPad.start = NO;
    for (UITouch* touch in _ctrlTouches) {
        CGPoint pos = [touch locationInView:_ctrlContainer];
        if (CGRectContainsPoint(_ctrlStart.frame, pos)) {
            _joyPad.start = YES;
        } else if (CGRectContainsPoint(_ctrlSelect.frame, pos)) {
            _joyPad.select = YES;
        }
    }
    
    if (_joyPad.code == previousCode) {
        return;
    }
    
    if (up != _joyPad.up) {
        if (_joyPad.up) {
            [_cursorUp setImage:[UIImage imageNamed:@"pad_up_on"]];
        } else {
            [_cursorUp setImage:[UIImage imageNamed:@"pad_up_off"]];
        }
    }
    if (down != _joyPad.down) {
        if (_joyPad.down) {
            [_cursorDown setImage:[UIImage imageNamed:@"pad_down_on"]];
        } else {
            [_cursorDown setImage:[UIImage imageNamed:@"pad_down_off"]];
        }
    }
    if (left != _joyPad.left) {
        if (_joyPad.left) {
            [_cursorLeft setImage:[UIImage imageNamed:@"pad_left_on"]];
        } else {
            [_cursorLeft setImage:[UIImage imageNamed:@"pad_left_off"]];
        }
    }
    if (right != _joyPad.right) {
        if (_joyPad.right) {
            [_cursorRight setImage:[UIImage imageNamed:@"pad_right_on"]];
        } else {
            [_cursorRight setImage:[UIImage imageNamed:@"pad_right_off"]];
        }
    }

    if (a != _joyPad.a) {
        if (_joyPad.a) {
            [_buttonA setImage:[UIImage imageNamed:@"pad_btn_on"]];
        } else {
            [_buttonA setImage:[UIImage imageNamed:@"pad_btn_off"]];
        }
    }
    if (b != _joyPad.b) {
        if (_joyPad.b) {
            [_buttonB setImage:[UIImage imageNamed:@"pad_btn_on"]];
        } else {
            [_buttonB setImage:[UIImage imageNamed:@"pad_btn_off"]];
        }
    }

    if (start != _joyPad.start) {
        if (_joyPad.start) {
            [_ctrlStart setImage:[UIImage imageNamed:@"pad_ctrl_on"]];
        } else {
            [_ctrlStart setImage:[UIImage imageNamed:@"pad_ctrl_off"]];
        }
    }
    if (select != _joyPad.select) {
        if (_joyPad.select) {
            [_ctrlSelect setImage:[UIImage imageNamed:@"pad_ctrl_on"]];
        } else {
            [_ctrlSelect setImage:[UIImage imageNamed:@"pad_ctrl_off"]];
        }
    }
}

@end
