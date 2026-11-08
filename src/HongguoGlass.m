#import "HongguoGlass.h"
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

static BOOL HGInstall(void) {
    if (!HGInstallTabBar()) {
        return NO;
    }
    HGInstallSettings();
    HGInstallAds();
    NSLog(@"[HongguoGlass] 已载入，双指长按底栏打开设置");
    return YES;
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
                    NSLog(@"[HongguoGlass] 未找到匹配的底栏入口，跳过插件加载");
                }
            });
        }
    }
}
