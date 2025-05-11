// Copyright (c) 2025 ChenKS12138

#import "AboutViewController.h"
@implementation AboutViewController
- (void)loadView {
  NSView *view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 480, 300)];
  self.view = view;

  NSStackView *mainStackView = [[NSStackView alloc] initWithFrame:NSMakeRect(20, 60, 440, 80)];
  mainStackView.orientation = NSUserInterfaceLayoutOrientationVertical;
  mainStackView.spacing = 12;
  mainStackView.alignment = NSLayoutAttributeLeading;
  mainStackView.translatesAutoresizingMaskIntoConstraints = NO;
  [self.view addSubview:mainStackView];
  [NSLayoutConstraint activateConstraints:@[
    [mainStackView.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor constant:20],
    [mainStackView.topAnchor constraintEqualToAnchor:self.view.topAnchor constant:20]
  ]];

  NSImageView *appIconImageView = [[NSImageView alloc] initWithFrame:NSMakeRect(20, 20, 64, 64)];
  NSImage *appIcon = [[NSWorkspace sharedWorkspace] iconForFile:[[NSBundle mainBundle] bundlePath]];
  [appIcon setSize:NSMakeSize(64, 64)];
  appIconImageView.image = appIcon;
  [mainStackView addArrangedSubview:appIconImageView];

  NSString *appName = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleName"];
  NSTextField *nameLabel = [[NSTextField alloc] initWithFrame:NSZeroRect];
  nameLabel.stringValue = [NSString stringWithFormat:@"Name: %@", appName ?: @"-"];
  nameLabel.editable = NO;
  nameLabel.selectable = NO;
  nameLabel.bordered = NO;
  nameLabel.backgroundColor = [NSColor clearColor];
  nameLabel.font = [NSFont boldSystemFontOfSize:16];
  [mainStackView addArrangedSubview:nameLabel];

  NSString *version =
      [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
  NSString *build = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleVersion"];
  NSTextField *versionLabel = [[NSTextField alloc] initWithFrame:NSZeroRect];
  versionLabel.stringValue =
      [NSString stringWithFormat:@"Version：%@ (Build %@)", version ?: @"-", build ?: @"-"];
  versionLabel.editable = NO;
  versionLabel.selectable = NO;
  versionLabel.bordered = NO;
  versionLabel.backgroundColor = [NSColor clearColor];
  [mainStackView addArrangedSubview:versionLabel];

  NSTextField *githubLabel = [[NSTextField alloc] initWithFrame:NSZeroRect];
  NSString *githubText = @"Github: ";
  NSString *githubURL = @"https://github.com/chenks12138/AirBeam";
  NSString *githubFullText = [githubText stringByAppendingString:githubURL];

  NSMutableAttributedString *githubAttrString =
      [[NSMutableAttributedString alloc] initWithString:githubFullText];

  [githubAttrString addAttribute:NSForegroundColorAttributeName
                           value:[NSColor labelColor]
                           range:NSMakeRange(0, githubText.length)];

  NSDictionary *linkAttributes = @{
    NSLinkAttributeName : githubURL,
    NSForegroundColorAttributeName : [NSColor systemBlueColor],
    NSCursorAttributeName : [NSCursor pointingHandCursor],
    NSUnderlineStyleAttributeName : @(NSUnderlineStyleSingle)
  };
  [githubAttrString addAttributes:linkAttributes
                            range:NSMakeRange(githubText.length, githubURL.length)];

  githubLabel.attributedStringValue = githubAttrString;
  githubLabel.editable = NO;
  githubLabel.selectable = YES;
  githubLabel.bordered = NO;
  githubLabel.drawsBackground = NO;
  githubLabel.allowsEditingTextAttributes = YES;
  [mainStackView addArrangedSubview:githubLabel];

  NSTextField *buymecoffeeLabel = [[NSTextField alloc] initWithFrame:NSZeroRect];
  NSString *coffeeText = @"Buymecoffee: ";
  NSString *coffeeURL = @"https://buymeacoffee.com/chenks12138";
  NSString *coffeeFullText = [coffeeText stringByAppendingString:coffeeURL];

  NSMutableAttributedString *coffeeAttrString =
      [[NSMutableAttributedString alloc] initWithString:coffeeFullText];

  [coffeeAttrString addAttribute:NSForegroundColorAttributeName
                           value:[NSColor labelColor]
                           range:NSMakeRange(0, coffeeText.length)];

  NSDictionary *coffeeLinkAttributes = @{
    NSLinkAttributeName : coffeeURL,
    NSForegroundColorAttributeName : [NSColor systemBlueColor],
    NSCursorAttributeName : [NSCursor pointingHandCursor],
    NSUnderlineStyleAttributeName : @(NSUnderlineStyleSingle)
  };
  [coffeeAttrString addAttributes:coffeeLinkAttributes
                            range:NSMakeRange(coffeeText.length, coffeeURL.length)];

  buymecoffeeLabel.attributedStringValue = coffeeAttrString;
  buymecoffeeLabel.editable = NO;
  buymecoffeeLabel.selectable = YES;
  buymecoffeeLabel.bordered = NO;
  buymecoffeeLabel.drawsBackground = NO;
  buymecoffeeLabel.allowsEditingTextAttributes = YES;
  [mainStackView addArrangedSubview:buymecoffeeLabel];
}

- (NSString *)title {
  return @"About";
}
@end