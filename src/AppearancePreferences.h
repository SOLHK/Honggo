#pragma once
#import <Foundation/Foundation.h>

FOUNDATION_EXPORT NSString *const HGAppearanceDidChangeNotification;
FOUNDATION_EXPORT const double HGADockDefaultWidth;
// Typed, independent appearance domain. No host preferences or old switch IDs.
NSArray<NSDictionary *> *HGAppearanceCatalog(void);
NSArray<NSString *> *HGAppearanceGroups(void);
NSArray<NSString *> *HGAppearanceGroupTitles(void);
NSArray<NSString *> *HGAppearanceRoles(void);
NSArray<NSString *> *HGAppearanceRoleTitles(void);
id HGAppearanceValue(NSString *key);
BOOL HGAppearanceFlag(NSString *key);
double HGAppearanceNumber(NSString *key);
NSString *HGAppearanceString(NSString *key);
BOOL HGFlatDockStyle(void);
BOOL HGFlatTopStyle(void);
BOOL HGFlatPlayerStyle(void);
BOOL HGAppearanceSet(NSString *key, id value, NSError **error);
BOOL HGAppearanceApply(NSDictionary *values, NSError **error);
NSDictionary *HGAppearanceValues(void);
NSDictionary *HGAppearancePreset(NSString *name);
void HGAppearanceReset(void);
BOOL HGAppearanceEnabled(void);
BOOL HGAppearanceNeedsGlass(void);
NSData *HGAppearanceExport(NSError **error);
NSDictionary *HGAppearanceParse(NSData *data, NSError **error);
