//
//  ViewController.h
//  msx2-osx
//
//  Created by Yoji Suzuki on 2023/01/23.
//

#import <Cocoa/Cocoa.h>

@interface ViewController : NSViewController
-(IBAction)menuReset:(id)sender;
-(IBAction)menuOpenRomFile:(id)sender;
-(IBAction)menuViewSize1x:(id)sender;
-(IBAction)menuViewSize2x:(id)sender;
-(IBAction)menuViewSize3x:(id)sender;
-(IBAction)menuViewSize4x:(id)sender;
//-(IBAction)menuQuickSaveToMemory:(id)sender;
//-(IBAction)menuQuickLoadFromMemory:(id)sender;
@end

