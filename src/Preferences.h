#pragma once

#import <Foundation/Foundation.h>

// The order and identifiers are shared with shipped preferences and ad rules.
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
    HGSidebarGameCenter,
    HGScreenshotShopping,
    HGFeatureCount,
};

FOUNDATION_EXPORT NSString *const HGPreferencesErrorDomain;
FOUNDATION_EXPORT const NSUInteger HGPreferencesMaximumBytes;

NSString *HGFeatureIdentifier(HGFeature feature);
NSString *HGFeatureTitle(HGFeature feature);
NSString *HGFeatureSearchText(HGFeature feature);
NSString *HGPreferenceKey(HGFeature feature);
BOOL HGIsEnabled(HGFeature feature);
BOOL HGSetEnabled(HGFeature feature, BOOL enabled);

// Backups contain only the feature switches. Diagnostic consent, account data,
// host settings and logs are never included or changed by these operations.
NSData *HGPreferencesExport(NSError **error);
NSDictionary<NSString *, NSNumber *> *HGPreferencesParse(NSData *data, NSError **error);

// Partial restores preserve omitted switches. Apply validates the entire
// dictionary again before the first write; a caller cannot bypass Parse.
BOOL HGPreferencesApply(NSDictionary<NSString *, NSNumber *> *features, NSError **error);
NSUInteger HGPreferencesChangeCount(NSDictionary<NSString *, NSNumber *> *features);
void HGPreferencesReset(void);
