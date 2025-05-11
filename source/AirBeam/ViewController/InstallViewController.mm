// Copyright (c) 2025 ChenKS12138

#import "InstallViewController.h"
#include <AppKit/AppKit.h>
#include <Foundation/Foundation.h>
#import <Security/Authorization.h>

static NSString *const kAirBeamASPPath = @"/Library/Audio/Plug-Ins/HAL/AirBeamASP.driver";
static NSString *const kAirBeamInstallStatusChangedNotification =
    @"AirBeamInstallStatusChangedNotification";

@implementation InstallViewController

- (void)installStatusChanged:(NSNotification *)notification {
  [self loadASPStatus];
}

- (void)loadASPStatus {
  NSFileManager *fileManager = [NSFileManager defaultManager];
  BOOL isDirectory;
  BOOL fileExists = [fileManager fileExistsAtPath:kAirBeamASPPath isDirectory:&isDirectory];
  self.isInstalled = fileExists && isDirectory;
}

- (void)viewDidLoad {
  [super viewDidLoad];
  [self setUpView];
  [self updateUI];
}

- (void)setUpView {
  NSStackView *mainHorizontalStackView = [[NSStackView alloc]
      initWithFrame:NSMakeRect(20, self.view.bounds.size.height - 80, 300, 60)];
  mainHorizontalStackView.orientation = NSUserInterfaceLayoutOrientationHorizontal;
  mainHorizontalStackView.spacing = 8;
  mainHorizontalStackView.alignment = NSLayoutAttributeTop;
  [self.view addSubview:mainHorizontalStackView];
  mainHorizontalStackView.translatesAutoresizingMaskIntoConstraints = NO;
  [NSLayoutConstraint activateConstraints:@[
    [mainHorizontalStackView.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor
                                                          constant:20],
    [mainHorizontalStackView.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor
                                                           constant:-20],
    [mainHorizontalStackView.topAnchor constraintEqualToAnchor:self.view.topAnchor constant:20]
  ]];

  NSTextField *extensionsLabel = [[NSTextField alloc] initWithFrame:NSZeroRect];
  extensionsLabel.stringValue = @"Status:";
  extensionsLabel.editable = NO;
  extensionsLabel.selectable = NO;
  extensionsLabel.bordered = NO;
  extensionsLabel.backgroundColor = [NSColor clearColor];
  [mainHorizontalStackView addArrangedSubview:extensionsLabel];

  NSStackView *rightVerticalStackView = [[NSStackView alloc] init];
  rightVerticalStackView.orientation = NSUserInterfaceLayoutOrientationVertical;
  rightVerticalStackView.spacing = 8;
  rightVerticalStackView.alignment = NSLayoutAttributeLeading;
  [mainHorizontalStackView addArrangedSubview:rightVerticalStackView];

  self.statusText = [[NSTextField alloc] initWithFrame:NSZeroRect];
  self.statusText.stringValue = @"Loading...";
  self.statusText.editable = NO;
  self.statusText.selectable = NO;
  self.statusText.bordered = NO;
  self.statusText.backgroundColor = [NSColor clearColor];
  [rightVerticalStackView addArrangedSubview:self.statusText];
  [self updateUI];

  self.actionButton = [[NSButton alloc] initWithFrame:NSZeroRect];
  [self.actionButton setEnabled:false];
  [self.actionButton setTitle:@"Reset"];
  [self.actionButton setBezelStyle:NSBezelStyleRounded];
  [rightVerticalStackView addArrangedSubview:self.actionButton];
}

- (void)updateUI {
  [self loadASPStatus];
  if (self.isInstalled) {
    self.statusText.stringValue = @"🟢 Installed";
    [self.actionButton setTitle:@"Uninstall"];
    [self.actionButton setEnabled:true];
    self.actionButton.target = self;
    [self.actionButton setAction:@selector(onActionBtnClicked)];
  } else {
    self.statusText.stringValue = @"🔴 Not Installed";
    [self.actionButton setTitle:@"Install"];
    [self.actionButton setEnabled:true];
    self.actionButton.target = self;
    [self.actionButton setAction:@selector(onActionBtnClicked)];
  }
}

- (void)onActionBtnClicked {
  BOOL current = self.isInstalled;
  [self loadASPStatus];
  if (current != self.isInstalled) {
    [self updateUI];
    return;
  }

  if (self.isInstalled) {
    [self uninstallASP];
  } else {
    [self installASP];
  }
}

- (void)installASP {
  NSError *error = nil;
  if (![self installDriverWithError:&error]) {
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:@"安装失败"];
    [alert setInformativeText:[error localizedDescription]];
    [alert setAlertStyle:NSAlertStyleCritical];
    [alert addButtonWithTitle:@"OK"];
    [alert runModal];
    [alert release];
    return;
  }
}

- (BOOL)installDriverWithError:(NSError **)error {
  NSBundle *mainBundle = [NSBundle mainBundle];
  NSString *driverPathInBundle = [mainBundle pathForResource:@"AirBeamASP" ofType:@"driver"];

  if (!driverPathInBundle) {
    if (error != NULL) {
      *error = [NSError
          errorWithDomain:NSCocoaErrorDomain
                     code:NSFileNoSuchFileError
                 userInfo:@{NSLocalizedDescriptionKey : @"AirBeamASP.driver not found in bundle."}];
    }
    return NO;
  }

  NSURL *destinationURL = [NSURL fileURLWithPath:kAirBeamASPPath];
  NSFileManager *fileManager = [NSFileManager defaultManager];

  NSURL *destinationDirectoryURL = [destinationURL URLByDeletingLastPathComponent];
  if (![fileManager fileExistsAtPath:[destinationDirectoryURL path] isDirectory:NULL]) {
    NSError *createDirectoryError = nil;
    if (![fileManager createDirectoryAtURL:destinationDirectoryURL
               withIntermediateDirectories:YES
                                attributes:nil
                                     error:&createDirectoryError]) {
      NSLog(@"Error creating destination directory: %@", createDirectoryError);
      if (error != NULL) {
        *error = createDirectoryError;
      }
      return NO;
    }
  }

  NSError *copyError = nil;
  if (![fileManager copyItemAtPath:driverPathInBundle toPath:kAirBeamASPPath error:&copyError]) {
    NSLog(@"Error copying driver: %@", copyError);
    if (error != NULL) {
      *error = copyError;
    }

    OSStatus status;
    AuthorizationRef authorizationRef;

    status = AuthorizationCreate(NULL, kAuthorizationEmptyEnvironment, kAuthorizationFlagDefaults,
                                 &authorizationRef);
    if (status != errAuthorizationSuccess) {
      NSLog(@"Error creating authorization reference: %d", status);
      return NO;
    }

    NSString *command = [NSString
        stringWithFormat:@"/bin/cp -r \"%@\" \"%@\" && killall -u _coreaudiod -9 coreaudiod",
                         driverPathInBundle, kAirBeamASPPath];
    const char *toolPath = "/bin/sh";
    const char *arguments[] = {"-c", [command UTF8String], NULL};
    FILE *pipe = NULL;

    status = AuthorizationExecuteWithPrivileges(
        authorizationRef, toolPath, kAuthorizationFlagDefaults, (char *const *)arguments, &pipe);

    AuthorizationFree(authorizationRef, kAuthorizationFlagDestroyRights);

    if (status != errAuthorizationSuccess) {
      NSLog(@"Error executing privileged copy: %d", status);
      if (error != NULL) {
        *error = [NSError errorWithDomain:NSOSStatusErrorDomain
                                     code:status
                                 userInfo:@{
                                   NSLocalizedDescriptionKey :
                                       @"Failed to copy driver with administrator privileges."
                                 }];
      }
      return NO;
    } else {
      NSLog(@"AirBeamASP.driver copied successfully to %@", kAirBeamASPPath);
      dispatch_async(dispatch_get_main_queue(), ^{
        [self loadASPStatus];
        [self updateUI];
      });
      return YES;
    }
  } else {
    NSLog(@"AirBeamASP.driver copied successfully to %@", kAirBeamASPPath);
    dispatch_async(dispatch_get_main_queue(), ^{
      [self loadASPStatus];
      [self updateUI];
    });
    return YES;
  }
}

- (void)uninstallASP {
  NSError *error = nil;
  if (![self uninstallDriverWithError:&error]) {
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:@"卸载失败"];
    [alert setInformativeText:[error localizedDescription]];
    [alert setAlertStyle:NSAlertStyleCritical];
    [alert addButtonWithTitle:@"OK"];
    [alert runModal];
    [alert release];
    return;
  }
}

- (BOOL)uninstallDriverWithError:(NSError **)error {
  NSFileManager *fileManager = [NSFileManager defaultManager];

  if (![fileManager fileExistsAtPath:kAirBeamASPPath]) {
    if (error != NULL) {
      *error =
          [NSError errorWithDomain:NSCocoaErrorDomain
                              code:NSFileNoSuchFileError
                          userInfo:@{NSLocalizedDescriptionKey : @"AirBeamASP.driver not found."}];
    }
    return NO;
  }

  NSError *removeError = nil;
  if (![fileManager removeItemAtPath:kAirBeamASPPath error:&removeError]) {
    NSLog(@"Error removing driver: %@", removeError);
    if (error != NULL) {
      *error = removeError;
    }

    OSStatus status;
    AuthorizationRef authorizationRef;

    status = AuthorizationCreate(NULL, kAuthorizationEmptyEnvironment, kAuthorizationFlagDefaults,
                                 &authorizationRef);
    if (status != errAuthorizationSuccess) {
      NSLog(@"Error creating authorization reference: %d", status);
      return NO;
    }

    NSString *command =
        [NSString stringWithFormat:@"/bin/rm -rf '%@' && killall -u _coreaudiod -9 coreaudiod",
                                   kAirBeamASPPath];
    const char *toolPath = "/bin/sh";
    const char *arguments[] = {"-c", [command UTF8String], NULL};
    FILE *pipe = NULL;

    status = AuthorizationExecuteWithPrivileges(
        authorizationRef, toolPath, kAuthorizationFlagDefaults, (char *const *)arguments, &pipe);

    AuthorizationFree(authorizationRef, kAuthorizationFlagDestroyRights);

    if (status != errAuthorizationSuccess) {
      NSLog(@"Error executing privileged removal: %d", status);
      if (error != NULL) {
        *error = [NSError errorWithDomain:NSOSStatusErrorDomain
                                     code:status
                                 userInfo:@{
                                   NSLocalizedDescriptionKey :
                                       @"Failed to remove driver with administrator privileges."
                                 }];
      }
      return NO;
    } else {
      NSLog(@"AirBeamASP.driver removed successfully from %@", kAirBeamASPPath);
      dispatch_async(dispatch_get_main_queue(), ^{
        [self loadASPStatus];
        [self updateUI];
      });
      return YES;
    }
  } else {
    NSLog(@"AirBeamASP.driver removed successfully from %@", kAirBeamASPPath);
    dispatch_async(dispatch_get_main_queue(), ^{
      [self loadASPStatus];
      [self updateUI];
    });
    return YES;
  }
}

- (NSString *)title {
  return @"Install";
}

@end