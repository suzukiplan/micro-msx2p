//
//  AppDelegate.m
//  msx2-osx
//
//  Created by Yoji Suzuki on 2023/01/23.
//

#import "AppDelegate.h"

@interface AppDelegate ()

@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)aNotification
{
}

- (void)applicationWillTerminate:(NSNotification*)aNotification
{
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)theApplication
{
    // Windowを閉じたらアプリを終了させる
    return YES;
}

- (BOOL)application:(NSApplication*)sender openFile:(NSString*)filename
{
    if (_openFileDelegate) {
        return [_openFileDelegate application:self didOpenFile:filename];
    } else {
        return NO;
    }
}

@end
