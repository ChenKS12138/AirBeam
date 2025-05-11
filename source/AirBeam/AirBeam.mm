// Copyright (c) 2025 ChenKS12138

#import <Cocoa/Cocoa.h>
#import "AppDelegate.h"

int main(int argc, const char *argv[]) {
  @autoreleasepool {
    AppDelegate *appDelegate = [[AppDelegate alloc] init];
    [NSApplication sharedApplication].delegate = appDelegate;
    return NSApplicationMain(argc, argv);
  }
}
