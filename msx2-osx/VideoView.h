//
//  VideoView.h
//  msx2-osx
//
//  Created by Yoji Suzuki on 2023/01/23.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@class VideoView;

@interface VideoView : NSView
@property (nonatomic, readwrite) BOOL isFullScreen;
- (id)initWithFrame:(CGRect)frame;
- (void)releaseDisplayLink;
@end

NS_ASSUME_NONNULL_END
