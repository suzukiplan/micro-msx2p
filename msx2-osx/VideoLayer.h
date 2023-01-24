//
//  VideoLayer.h
//  msx2-osx
//
//  Created by Yoji Suzuki on 2023/01/23.
//

#import <QuartzCore/QuartzCore.h>

NS_ASSUME_NONNULL_BEGIN

@interface VideoLayer : CALayer
- (void)drawVRAM;
@end

NS_ASSUME_NONNULL_END
