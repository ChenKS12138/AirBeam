#import <Cocoa/Cocoa.h>
#import "AppDelegate.h"

int main(int argc, const char *argv[]) {
  @autoreleasepool {
    // Setup code that might create autoreleased objects goes here.
    AppDelegate *appDelegate = [[AppDelegate alloc] init];
    [NSApplication sharedApplication].delegate = appDelegate;
    return NSApplicationMain(argc, argv);
  }
}
