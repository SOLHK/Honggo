#import "Honggo.h"
#import <dispatch/dispatch.h>
#include <string.h>

BOOL HGNativeGlassConfigured(void) {
    // Require an explicit pre-launch opt-in for this version-checked host.
    // Finding UIGlassEffect also succeeds in compatibility mode, where the
    // system renders an opaque fill (or nothing if that fill is cleared).
    // Never intercept bundle reads or change the process-wide mode at runtime.
    id compatibility = [NSBundle.mainBundle objectForInfoDictionaryKey:@"UIDesignRequiresCompatibility"];
    if (![compatibility isKindOfClass:NSNumber.class] || [compatibility boolValue]) return NO;
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

        HGDiagnosticsStart(nil, @{@"hostBuild": version, @"os": NSProcessInfo.processInfo.operatingSystemVersionString,
            @"executable": @"Eggplant"});
        HGLoadStage(@"entry");
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
