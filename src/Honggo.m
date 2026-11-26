#import "Honggo.h"
#import "Appearance.h"
#import "AdSceneDiagnostics.h"
#import "RuntimeSupport.h"
#import <dispatch/dispatch.h>
#include <string.h>

static BOOL HGStartupGlassOverride;

void HGPrepareNativeGlass(void) {
    // Called from the version-checked constructor, before any UIKit work.
    // UIKit caches this preference during startup. Keep it process-local so
    // disabling appearance customization (or removing this dylib) restores the
    // host's own configuration on the next launch without preference cleanup.
    if (HGStartupGlassOverride || NSProcessInfo.processInfo.operatingSystemVersion.majorVersion < 26 ||
        !HGAppearanceEnabled()) return;
    NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
    NSMutableDictionary *arguments = [[defaults volatileDomainForName:NSArgumentDomain] mutableCopy];
    if (!arguments) arguments = [NSMutableDictionary dictionary];
    arguments[@"com.apple.SwiftUI.IgnoreSolariumOptOut"] = @YES;
    [defaults setVolatileDomain:arguments forName:NSArgumentDomain];
    HGStartupGlassOverride = [defaults boolForKey:@"com.apple.SwiftUI.IgnoreSolariumOptOut"];
}

BOOL HGNativeGlassStartupOverride(void) { return HGStartupGlassOverride; }

BOOL HGNativeGlassConfigured(void) {
    // A class existing alone is insufficient: compatibility mode can suppress
    // its rendering. Accept either the original plist opt-in or our early
    // process preference; device diagnostics report both independently.
    id compatibility = [NSBundle.mainBundle objectForInfoDictionaryKey:@"UIDesignRequiresCompatibility"];
    if (!HGStartupGlassOverride &&
        (![compatibility isKindOfClass:NSNumber.class] || [compatibility boolValue])) return NO;
    Class glassClass = NSClassFromString(@"UIGlassEffect");
    return [glassClass respondsToSelector:sel_registerName("effectWithStyle:")];
}

IMP HGReplaceMethod(Class target, SEL selector, const char *encoding, IMP replacement) {
    return HGInstallVerifiedHook(target, selector, encoding, replacement, @"runtime");
}

static BOOL HGSettingsInstalled;
static BOOL HGTabBarInstalled;
static NSUInteger HGInstallationAttempts;

BOOL HGInstallRuntimeModules(void) {
    if (HGInstallationAttempts >= 2) return HGSettingsInstalled && HGTabBarInstalled;
    HGInstallationAttempts++;
    HGInstallControls();
    HGLoadStage(@"controlsInstalled");
    // Settings must remain discoverable even when the optional tab-bar hook is unavailable.
    if (!HGSettingsInstalled) {
        HGSettingsInstalled = HGInstallSettings();
    }
    if (!HGTabBarInstalled) {
        HGTabBarInstalled = HGInstallTabBar();
    }
    // Advertisement gates have their own class/ABI checks and must not depend
    // on an optional appearance module. Retrying is safe at the hook boundary.
    HGInstallAds();
    HGInstallPlaybackDiagnostics();
    HGInstallAppearance();
    HGRecordCapability(@"module.settings", @{@"module": @"settings",
        @"state": HGSettingsInstalled ? @"installed" : @"unavailable",
        @"reason": HGSettingsInstalled ? @"settings entry available" : @"settings entry preflight failed"});
    HGRecordCapability(@"module.tabbar", @{@"module": @"tabbar",
        @"state": HGTabBarInstalled ? @"installed" : @"unavailable",
        @"reason": HGTabBarInstalled ? @"tab-bar entry available" : @"tab-bar entry preflight failed"});
    return HGSettingsInstalled && HGTabBarInstalled;
}

__attribute__((constructor)) static void HGStart(void) {
    @autoreleasepool {
        NSBundle *bundle = NSBundle.mainBundle;
        if (![bundle.executableURL.lastPathComponent isEqualToString:@"Eggplant"]) {
            return;
        }

        id build = [bundle objectForInfoDictionaryKey:@"CFBundleVersion"];
        NSString *hostBuild = [build isKindOfClass:NSString.class] ? build : @"unknown";
        HGDiagnosticsStart(nil, @{@"hostBuild": hostBuild, @"os": NSProcessInfo.processInfo.operatingSystemVersionString,
            @"executable": @"Eggplant"});
        NSNotificationCenter *center = NSNotificationCenter.defaultCenter;
        [center addObserverForName:UIApplicationDidFinishLaunchingNotification object:nil queue:nil usingBlock:^(NSNotification *note) {
            HGDiagnosticsSetForeground(UIApplication.sharedApplication.applicationState != UIApplicationStateBackground);
        }];
        [center addObserverForName:UIApplicationWillEnterForegroundNotification object:nil queue:nil usingBlock:^(NSNotification *note) {
            HGDiagnosticsSetForeground(YES);
        }];
        [center addObserverForName:UIApplicationDidEnterBackgroundNotification object:nil queue:nil usingBlock:^(NSNotification *note) {
            HGDiagnosticsSetForeground(NO);
        }];
        [center addObserverForName:UIApplicationWillTerminateNotification object:nil queue:nil usingBlock:^(NSNotification *note) {
            HGDiagnosticsEnd();
        }];
        HGLoadStage(@"entry");
        HGPrepareNativeGlass();
        HGInstallAdSceneDiagnostics();
        HGLog(@"glass.startup", @{@"processOverride": @(HGNativeGlassStartupOverride()),
            @"compatibility": [bundle objectForInfoDictionaryKey:@"UIDesignRequiresCompatibility"] ?: NSNull.null});
        BOOL complete = HGInstallRuntimeModules();
        // One next-main-loop pass also covers independently missing control/ad
        // classes. No polling loop or unbounded late loading hook is installed.
        dispatch_async(dispatch_get_main_queue(), ^{
            if (!HGInstallRuntimeModules()) {
                NSLog(@"[Honggo] 版本 %@，设置菜单入口：%@；底栏入口：%@。已安装部分继续生效。",
                    HG_PLUGIN_VERSION,
                    HGSettingsInstalled ? @"已安装" : @"未就绪",
                    HGTabBarInstalled ? @"已安装" : @"未就绪");
            } else {
                NSLog(@"[Honggo] 版本 %@ 已载入，请在 我的 → 设置 → Honggo 管理开关", HG_PLUGIN_VERSION);
            }
            HGLoadStage(@"installationRetried");
        });
        if (complete) {
            NSLog(@"[Honggo] 版本 %@ 已载入，请在 我的 → 设置 → Honggo 管理开关", HG_PLUGIN_VERSION);
        }
        HGLoadStage(@"installationReturned");
    }
}
