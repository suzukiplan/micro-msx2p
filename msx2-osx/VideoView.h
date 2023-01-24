//
//  VideoView.h
//  msx2-osx
//
//  Created by Yoji Suzuki on 2023/01/23.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@class VideoView;

@protocol VideoViewDelegate <NSObject>
- (void)videoView:(VideoView*)view
   didChangeScore:(NSInteger)score
       isGameOver:(BOOL)isGameOver;
@end

@interface VideoView : NSView
@property (nonatomic, readwrite) BOOL isFullScreen;
@property (nonatomic, weak, nullable) id<VideoViewDelegate> delegate;
- (id)initWithFrame:(CGRect)frame;
- (void)releaseDisplayLink;
@end

NS_ASSUME_NONNULL_END
