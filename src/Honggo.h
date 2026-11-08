#pragma once

#import "HonggoVersion.h"
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <objc/runtime.h>

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
    HGFeatureCount,
};

BOOL HGIsEnabled(HGFeature feature);
IMP HGReplaceMethod(Class target, SEL selector, const char *encoding, IMP replacement);
void HGInstallAds(void);
BOOL HGInstallTabBar(void);
BOOL HGInstallSettings(void);
void HGAttachSettings(UIView *tabBar);

@interface HGSettingsController : UITableViewController
@end
