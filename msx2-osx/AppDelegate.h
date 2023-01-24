//
//  AppDelegate.h
//  msx2-osx
//
//  Created by Yoji Suzuki on 2023/01/23.
//

#import <Cocoa/Cocoa.h>

@class AppDelegate;

@protocol OpenFileDelegate <NSObject>
- (BOOL)application:(AppDelegate*)app didOpenFile:(NSString*)file;
@end

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, weak) id<OpenFileDelegate> openFileDelegate;
@property (nonatomic, weak) IBOutlet NSMenu* menu;
@property (nonatomic, weak) IBOutlet NSMenuItem* menuQuickLoadFromMemory;
@end
