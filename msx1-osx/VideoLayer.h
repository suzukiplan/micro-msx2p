//
//  VideoLayer.h
//  msx1-osx
//
//  Created by Yoji Suzuki on 2023/08/03.
//

#import <QuartzCore/QuartzCore.h>

NS_ASSUME_NONNULL_BEGIN

@interface VideoLayer : CALayer
- (void)drawVRAM;
@end

NS_ASSUME_NONNULL_END
