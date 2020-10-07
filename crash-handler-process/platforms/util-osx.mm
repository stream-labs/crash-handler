#include "../util.hpp"
#include "../logger.hpp"

#import <Cocoa/Cocoa.h>

bool restart = false;

@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate> {
}
- (void)applicationDidFinishLaunching:(NSNotification *)notification;
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)app;
@end

void stopApplication(void) {
    [[NSApplication sharedApplication] stop:nil];
}

@implementation AppDelegate
- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    NSAlert *alert = [[NSAlert alloc] init];
    [alert addButtonWithTitle:@"NO"];
    [alert addButtonWithTitle:@"YES"];
    [alert setMessageText:@"An error occurred which has caused Streamlabs OBS to close. Don't worry! If you were streaming or recording, that is still happening in the background.\
    \n\nWhenever you're ready, we can relaunch the application, however this will end your stream / recording session.\
    \n\nClick the Yes button to keep streaming / recording.\
    \n\nClick the No button to stop streaming / recording."];
    [alert setAlertStyle:NSAlertStyleCritical];

    NSModalResponse response = [alert runModal];

    if (response == NSAlertFirstButtonReturn) {
        stopApplication();
    } else if (response == NSAlertSecondButtonReturn) {
        NSAlert *alert2 = [[NSAlert alloc] init];
        [alert2 addButtonWithTitle:@"OK"];
        [alert2 setMessageText:@"Your stream / recording session is still running in the background. Whenever you're ready, click the OK button below to end your stream / recording and relaunch the application."];
        [alert2 setAlertStyle:NSAlertStyleCritical];

        if ([alert2 runModal] == NSAlertFirstButtonReturn) {
            stopApplication();
            restart = true;
        }
    }
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)app {
    return NO;
}
@end

void Util::runTerminateWindow(bool &shouldRestart) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        AppDelegate *del = [[AppDelegate alloc] init];
        app.delegate = del;
        [app run];
        shouldRestart = restart;
    }
}

void Util::check_pid_file(std::string& pid_path) {
    //TODO
}

std::string Util::get_temp_directory() {
    //TODO
}

void Util::restartApp(std::wstring path) {
    [[NSWorkspace sharedWorkspace] launchApplication:@"/Applications/Streamlabs OBS.app"];
}

void Util::setAppStatePath(std::wstring path) {}

void Util::updateAppState(bool unresponsive_detected) {}