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
-(IBAction)menuInsertDiskA:(id)sender;
-(IBAction)menuEjectDiskA:(id)sender;
-(IBAction)menuInsertDiskB:(id)sender;
-(IBAction)menuEjectDiskB:(id)sender;
-(IBAction)menuQuickSave:(id)sender;
-(IBAction)menuQuickLoad:(id)sender;
-(IBAction)menuSaveRamDump:(id)sender;
-(IBAction)menuSaveVramDump:(id)sender;
-(IBAction)menuSaveVramBitmap:(id)sender;
-(IBAction)menuSaveSpritePattern:(id)sender;
-(IBAction)menuStop:(id)sender;
-(IBAction)menuPause:(id)sender;
-(IBAction)menuTypeFromClipboard:(id)sender;
@end

