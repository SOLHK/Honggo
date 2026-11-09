#import "Honggo.h"
#import <dispatch/dispatch.h>
#include <string.h>

static BOOL HGStartupGlassOverride;

void HGPrepareNativeGlass(void) {
    // Called from the version-checked constructor, before any UIKit work.
    // UIKit caches this preference during startup. Keep it process-local so
    // disabling both glass switches (or removing this dylib) restores the
    // host's own configuration on the next launch without preference cleanup.
    if (HGStartupGlassOverride || NSProcessInfo.processInfo.operatingSystemVersion.majorVersion < 26 ||
        (!HGIsEnabled(HGGlass) && !HGIsEnabled(HGGlassControls))) return;
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
    Method method = class_getInstanceMethod(target, selector);
    if (method == NULL || strcmp(method_getTypeEncoding(method), encoding) != 0) {
        HGLog(@"hook.skipped", @{@"class": NSStringFromClass(target) ?: @"missing",
            @"selector": NSStringFromSelector(selector), @"expected": @(encoding),
            @"actual": method ? @(method_getTypeEncoding(method)) : @"missing"});
        return NULL;
    }

    IMP original = method_getImplementation(method);
    class_replaceMethod(target, selector, replacement, encoding);
    HGLog(@"hook.installed", @{@"class": NSStringFromClass(target), @"selector": NSStringFromSelector(selector)});
    return original;
}

static BOOL HGSettingsInstalled;
static BOOL HGTabBarInstalled;
static BOOL HGAdsInstalled;

static BOOL HGInstall(void) {
    // Settings must remain discoverable even when the optional tab-bar hook is unavailable.
    if (!HGSettingsInstalled) {
        HGSettingsInstalled = HGInstallSettings();
    }
    if (!HGTabBarInstalled) {
        HGTabBarInstalled = HGInstallTabBar();
    }
    if (HGTabBarInstalled && !HGAdsInstalled) {
        HGInstallAds();
        HGAdsInstalled = YES;
    }
    return HGSettingsInstalled && HGTabBarInstalled;
}

__attribute__((constructor)) static void HGStart(void) {
    @autoreleasepool {
        NSBundle *bundle = NSBundle.mainBundle;
        id version = [bundle objectForInfoDictionaryKey:@"CFBundleVersion"];
        if (![bundle.executableURL.lastPathComponent isEqualToString:@"Eggplant"] ||
            ![version isEqual:@"7.4.3.33"]) {
            return;
        }

        HGPrepareNativeGlass();
        HGDiagnosticsStart(nil, @{@"hostBuild": version, @"os": NSProcessInfo.processInfo.operatingSystemVersionString,
            @"executable": @"Eggplant"});
        HGLoadStage(@"entry");
        HGLog(@"glass.startup", @{@"processOverride": @(HGNativeGlassStartupOverride()),
            @"compatibility": [bundle objectForInfoDictionaryKey:@"UIDesignRequiresCompatibility"] ?: NSNull.null});
        HGInstallControls();
        HGLoadStage(@"controlsInstalled");

        if (!HGInstall()) {
            dispatch_async(dispatch_get_main_queue(), ^{
                if (!HGInstall()) {
                    NSLog(@"[Honggo] 版本 %@，设置菜单入口：%@；底栏入口：%@。已安装部分继续生效。",
                        HG_PLUGIN_VERSION,
                        HGSettingsInstalled ? @"已安装" : @"未就绪",
                        HGTabBarInstalled ? @"已安装" : @"未就绪");
                } else {
                    NSLog(@"[Honggo] 版本 %@ 已载入，请在 我的 → 设置 → Honggo 管理开关", HG_PLUGIN_VERSION);
                }
                HGLoadStage(@"installationRetried");
            });
        } else {
            NSLog(@"[Honggo] 版本 %@ 已载入，请在 我的 → 设置 → Honggo 管理开关", HG_PLUGIN_VERSION);
        }
        HGLoadStage(@"installationReturned");
    }
}
