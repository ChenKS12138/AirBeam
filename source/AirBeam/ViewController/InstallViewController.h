// Copyright (c) 2025 ChenKS12138

#import <Cocoa/Cocoa.h>

@interface InstallViewController : NSViewController

@property(nonatomic, assign) BOOL isInstalled;
@property(nonatomic, strong) NSTextField *statusText;
@property(nonatomic, strong) NSButton *actionButton;

@end