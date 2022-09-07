#include "../util.hpp"
#include "../logger.hpp"
#include <libintl.h>

#import <Cocoa/Cocoa.h>

bool restart = false;

NSString *translate(const char *input) {
    return [NSString stringWithUTF8String: gettext(input)];
}

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

    [alert addButtonWithTitle:translate("No")];
    [alert addButtonWithTitle:translate("YES")];
    [alert setMessageText:translate(
		"An error occurred which has caused Streamlabs Desktop to close. Don't worry! "
		"If you were streaming or recording, that is still happening in the background."
		"\n\nWhenever you're ready, we can relaunch the application, however this will end "
		"your stream / recording session.\n\n"
		"Click the Yes button to keep streaming / recording. \n\n"
		"Click the No button to stop streaming / recording.")];
    [alert setAlertStyle:NSAlertStyleCritical];

    NSModalResponse response = [alert runModal];

    if (response == NSAlertFirstButtonReturn) {
        stopApplication();
    } else if (response == NSAlertSecondButtonReturn) {
        NSAlert *alert2 = [[NSAlert alloc] init];
        [alert2 addButtonWithTitle:translate("OK")];
        [alert2 setMessageText:translate("Your stream / recording session is still running in the background. Whenever you're ready, click the OK "
			"button below to end your stream / recording and relaunch the application.")];
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

void Util::write_pid_file(std::string& pid_path) {
    //TODO
}

std::string Util::get_temp_directory() {
    //TODO
    return "";
}

void Util::restartApp(std::wstring path) {
    [[NSWorkspace sharedWorkspace] launchApplication:@"/Applications/Streamlabs Desktop.app"];
}

void Util::setCachePath(std::wstring path) {}

void Util::updateAppState(bool unresponsive_detected) {}

bool Util::saveMemoryDump(uint32_t pid, const std::wstring& dumpPath, const std::wstring& dumpFileName) { return false;}
bool Util::archiveFile(const std::wstring& srcFullPath, const std::wstring& dstFullPath, const std::string& nameInsideArchive) { return false;}
void Util::abortUploadAWS() { }

void Util::setupLocale() { }