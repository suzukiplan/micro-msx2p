//
//  VideoView.h
//  msx1-osx
//
//  Created by Yoji Suzuki on 2023/08/03.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@class VideoView;

@interface VideoView : NSView
@property (nonatomic, readwrite) BOOL isFullScreen;
@property (nonatomic, readonly) BOOL pausing;
- (id)initWithFrame:(CGRect)frame;
- (void)releaseDisplayLink;
- (void)pauseWithCompletionHandler:(void (^)(void))completionHandler;
- (void)resumeWithCompletionHandler:(void (^)(void))completionHandler;
@end

NS_ASSUME_NONNULL_END
