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
#import "MenuView.h"

@interface MenuView()
@property (nonatomic) UILabel* label;
@property (nonatomic) UIButton* resetButton;
@end

@implementation MenuView

- (instancetype)init
{
    if (self = [super init]) {
        _label = [[UILabel alloc] init];
        _label.font = [UIFont boldSystemFontOfSize:16.0];
        _label.textColor = [UIColor whiteColor];
        _label.textAlignment = NSTextAlignmentLeft;
        _label.text = @"micro MSX2+ - Example (Objective-c)";
        [self addSubview:_label];
        _resetButton = [UIButton buttonWithType:UIButtonTypeRoundedRect];
        [_resetButton setTitle:@"Reset" forState:UIControlStateNormal];
        [_resetButton setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
        [_resetButton addTarget:self action:@selector(pushReset:) forControlEvents:UIControlEventTouchUpInside];
        [self addSubview:_resetButton];
    }
    return self;
}

- (void)setFrame:(CGRect)frame
{
    [super setFrame:frame];
    CGFloat w = _resetButton.intrinsicContentSize.width;
    CGFloat h = _resetButton.intrinsicContentSize.height;
    CGFloat x = frame.size.width - w - 16;
    CGFloat y = (frame.size.height - h) / 2;
    _resetButton.frame = CGRectMake(x, y, w, h);
    w = x - 8;
    x = 8;
    h = _label.intrinsicContentSize.height;
    y = (frame.size.height - h) / 2;
    _label.frame = CGRectMake(x, y, w, h);
}

- (void)pushReset:(id)sender
{
    [_delegate menuView:self didTapItem:MenuviewItemReset];
}

@end
