#import "Honggo.h"
#import <dispatch/dispatch.h>
#include <string.h>

IMP HGReplaceMethod(Class target, SEL selector, const char *encoding, IMP replacement) {
    Method method = class_getInstanceMethod(target, selector);
    if (method == NULL || strcmp(method_getTypeEncoding(method), encoding) != 0) {
        return NULL;
    }

    IMP original = method_getImplementation(method);
    class_replaceMethod(target, selector, replacement, encoding);
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
            });
        } else {
            NSLog(@"[Honggo] 版本 %@ 已载入，请在 我的 → 设置 → Honggo 管理开关", HG_PLUGIN_VERSION);
        }
    }
}
