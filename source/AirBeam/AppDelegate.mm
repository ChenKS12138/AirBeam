// Copyright (c) 2025 ChenKS12138

#import "AppDelegate.h"
#include <AppKit/AppKit.h>
#include <AppKit/NSToolbar.h>

#include "ViewController/AboutViewController.h"
#include "ViewController/InstallViewController.h"

static NSString *const kAboutTabIdentifier = @"About";
static NSString *const kInstallTabIdentifier = @"Install";

@implementation AppDelegate {
  NSDictionary<NSString *, NSViewController *> *_viewControllers;
}

- (instancetype)init {
  _viewControllers = @{
    kInstallTabIdentifier : [[InstallViewController alloc] init],
    kAboutTabIdentifier : [[AboutViewController alloc] init],
  };
  return self;
}

#pragma mark - NSApplicationDelegate

- (void)setupWindowMenu {
  NSMenu *mainMenu = [NSApp mainMenu];
  NSMenu *windowMenu = [[NSMenu alloc] initWithTitle:@"Window"];

  NSMenuItem *closeItem = [[NSMenuItem alloc] initWithTitle:@"Close Window"
                                                     action:@selector(performClose:)
                                              keyEquivalent:@"w"];
  [closeItem setKeyEquivalentModifierMask:NSEventModifierFlagCommand];
  [windowMenu addItem:closeItem];

  NSMenuItem *windowMenuItem = [[NSMenuItem alloc] init];
  [windowMenuItem setSubmenu:windowMenu];

  NSInteger windowMenuIndex = [mainMenu indexOfItemWithTitle:@"Window"];
  if (windowMenuIndex == -1) {
    [mainMenu addItem:windowMenuItem];
  } else {
    [mainMenu setSubmenu:windowMenu forItem:[mainMenu itemAtIndex:windowMenuIndex]];
  }
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
  [self setupWindowMenu];

  self.mainWindow =
      [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 480, 320)
                                  styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                                             NSWindowStyleMaskMiniaturizable)
                                    backing:NSBackingStoreBuffered
                                      defer:NO];
  [self.mainWindow center];
  [self.mainWindow makeKeyAndOrderFront:nil];

  NSToolbar *toolbar = [[NSToolbar alloc] initWithIdentifier:@"AirBeamToolbar"];
  [toolbar setDelegate:self];
  toolbar.allowsUserCustomization = YES;
  toolbar.autosavesConfiguration = NO;
  toolbar.displayMode = NSToolbarDisplayModeIconAndLabel;

  [self.mainWindow setToolbar:toolbar];
  [self.mainWindow center];
  [self.mainWindow makeKeyAndOrderFront:nil];

  [self switchToTab:kInstallTabIdentifier];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
  return YES;
}

#pragma mark - NSToolbarDelegate

- (NSArray<NSToolbarItemIdentifier> *)toolbarAllowedItemIdentifiers:(NSToolbar *)toolbar {
  return @[
    kInstallTabIdentifier,
    kAboutTabIdentifier,
  ];
}

- (NSArray<NSToolbarItemIdentifier> *)toolbarDefaultItemIdentifiers:(NSToolbar *)toolbar {
  return @[ kInstallTabIdentifier, kAboutTabIdentifier ];
}

- (NSToolbarItem *)toolbar:(NSToolbar *)toolbar
        itemForItemIdentifier:(NSToolbarItemIdentifier)itemIdentifier
    willBeInsertedIntoToolbar:(BOOL)flag {
  if ([itemIdentifier isEqualToString:kInstallTabIdentifier]) {
    NSToolbarItem *item = [[NSToolbarItem alloc] initWithItemIdentifier:itemIdentifier];
    item.label = kInstallTabIdentifier;
    item.image = [NSImage imageNamed:NSImageNameBonjour];
    item.action = @selector(toolbarButtonClicked:);
    item.target = self;
    return item;
  }
  if ([itemIdentifier isEqualToString:kAboutTabIdentifier]) {
    NSToolbarItem *item = [[NSToolbarItem alloc] initWithItemIdentifier:itemIdentifier];
    item.label = kAboutTabIdentifier;
    item.image = [NSImage imageNamed:NSImageNameInfo];
    item.action = @selector(toolbarButtonClicked:);
    item.target = self;
    return item;
  }
  return nil;
}

- (void)toolbarButtonClicked:(NSToolbarItem *)sender {
  [self switchToTab:sender.itemIdentifier];
}

- (void)switchToTab:(NSString *)identifier {
  NSViewController *vc = _viewControllers[identifier];
  if (!vc) return;

  self.mainWindow.contentViewController = vc;
  self.mainWindow.title = vc.title ?: @"AirBeam";
}

@end