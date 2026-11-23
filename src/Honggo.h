#pragma once

#import "HonggoVersion.h"
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <objc/runtime.h>
#import "Diagnostics.h"
#import "Preferences.h"
#import "RuntimeSupport.h"
BOOL HGNativeGlassConfigured(void);
void HGPrepareNativeGlass(void);
BOOL HGNativeGlassStartupOverride(void);
IMP HGReplaceMethod(Class target, SEL selector, const char *encoding, IMP replacement);
void HGInstallAds(void);
void HGInstallPlaybackDiagnostics(void);
BOOL HGInstallTabBar(void);
BOOL HGInstallSettings(void);
void HGAttachSettings(UIView *tabBar);
void HGInstallControls(void);
NSDictionary *HGInterfaceSnapshot(void);
void HGStyleControl(UIView *control);
UIUserInterfaceStyle HGGlassForegroundStyle(UIView *content, UITraitCollection *traits);
UIColor *HGOriginalTabTitleColor(UILabel *label);

@interface HGSettingsController : UITableViewController
+ (void)openFromController:(UIViewController *)presenter;
+ (void)openFromTabBar:(UILongPressGestureRecognizer *)gesture;
+ (void)openPath:(NSString *)path fromController:(UIViewController *)presenter;
@end
