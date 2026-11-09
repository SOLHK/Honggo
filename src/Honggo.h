#pragma once

#import "HonggoVersion.h"
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <objc/runtime.h>
#import "Diagnostics.h"

typedef NS_ENUM(NSUInteger, HGFeature) {
    HGGlass,
    HGSplash,
    HGTopView,
    HGFeed,
    HGComment,
    HGPause,
    HGPatch,
    HGEcommerce,
    HGMarketing,
    HGRedPacket,
    HGBookstoreAd,
    HGCouponPendant,
    HGMiniGamePendant,
    HGWelfareGuide,
    HGSurvey,
    HGGlassControls,
    HGFeatureCount,
};

BOOL HGIsEnabled(HGFeature feature);
BOOL HGNativeGlassConfigured(void);
void HGPrepareNativeGlass(void);
BOOL HGNativeGlassStartupOverride(void);
IMP HGReplaceMethod(Class target, SEL selector, const char *encoding, IMP replacement);
void HGInstallAds(void);
BOOL HGInstallTabBar(void);
BOOL HGInstallSettings(void);
void HGAttachSettings(UIView *tabBar);
void HGInstallControls(void);
NSDictionary *HGInterfaceSnapshot(void);
void HGStyleControl(UIView *control);
UIUserInterfaceStyle HGGlassForegroundStyle(UIView *content, UITraitCollection *traits);
UIColor *HGOriginalTabTitleColor(UILabel *label);

@interface HGSettingsController : UITableViewController
@end
