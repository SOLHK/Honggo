#import "Preferences.h"
#import "HonggoVersion.h"
#import <CoreFoundation/CoreFoundation.h>
#import <dispatch/dispatch.h>

NSString *const HGPreferencesErrorDomain = @"org.honggo.preferences";
const NSUInteger HGPreferencesMaximumBytes = 64 * 1024;

typedef struct {
    NSString *identifier;
    NSString *title;
    NSString *searchAliases;
} HGFeatureDefinition;

static const HGFeatureDefinition HGFeatureDefinitions[HGFeatureCount] = {
    [HGGlass] = {@"glass", @"液态玻璃底栏", @"底部 导航 标签 浮动 模糊 tab bar"},
    [HGSplash] = {@"splash", @"隐藏开屏广告", @"启动 splash"},
    [HGTopView] = {@"topview", @"隐藏首页 TopView 广告", @"顶部 首页 top view"},
    [HGFeed] = {@"feed", @"隐藏短剧广告、直播和小游戏插播", @"信息流 推荐 试玩 游戏 feed live"},
    [HGComment] = {@"comment", @"隐藏短剧评论区广告", @"评论 comment"},
    [HGPause] = {@"pause", @"隐藏暂停广告", @"暂停 pause"},
    [HGPatch] = {@"patch", @"隐藏播放插播广告", @"贴片 中插 退出播放 patch"},
    [HGEcommerce] = {@"commerce", @"隐藏启动电商推广", @"购物 电商 开机 commerce"},
    [HGMarketing] = {@"marketing", @"隐藏商业与直播领券弹窗", @"推广 营销 直播 优惠券 弹窗 popup"},
    [HGRedPacket] = {@"redPacket", @"隐藏福利红包弹窗", @"福利 奖励 红包 popup"},
    [HGBookstoreAd] = {@"bookstoreAd", @"隐藏书城底部广告", @"书城 横幅 banner"},
    [HGCouponPendant] = {@"couponPendant", @"隐藏领券悬浮挂件", @"优惠券 浮窗 pendant coupon"},
    [HGMiniGamePendant] = {@"miniGamePendant", @"隐藏小游戏悬浮挂件", @"游戏 浮窗 pendant game"},
    [HGWelfareGuide] = {@"welfareGuide", @"隐藏福利标签引导气泡", @"提示 福利 气泡 guide"},
    [HGSurvey] = {@"survey", @"隐藏短剧满意度问卷", @"调查 评分 问卷 survey"},
    [HGGlassControls] = {@"glassControls", @"原生液态玻璃按钮", @"按钮 控件 模糊 glass controls"},
};

static NSObject *HGPreferenceLock(void) {
    static NSObject *lock;
    static dispatch_once_t once;
    dispatch_once(&once, ^{ lock = [NSObject new]; });
    return lock;
}

NSString *HGFeatureIdentifier(HGFeature feature) {
    return feature < HGFeatureCount ? HGFeatureDefinitions[feature].identifier : nil;
}

NSString *HGFeatureTitle(HGFeature feature) {
    return feature < HGFeatureCount ? HGFeatureDefinitions[feature].title : nil;
}

NSString *HGFeatureSearchText(HGFeature feature) {
    if (feature >= HGFeatureCount) return nil;
    const HGFeatureDefinition *definition = &HGFeatureDefinitions[feature];
    return [NSString stringWithFormat:@"%@ %@ %@", definition->title,
        definition->identifier, definition->searchAliases];
}

NSString *HGPreferenceKey(HGFeature feature) {
    NSString *identifier = HGFeatureIdentifier(feature);
    // Keep the namespace used by existing installations, including early builds.
    return identifier ? [@"HongguoGlass." stringByAppendingString:identifier] : nil;
}

BOOL HGIsEnabled(HGFeature feature) {
    NSString *key = HGPreferenceKey(feature);
    if (!key) return NO;
    @synchronized (HGPreferenceLock()) {
        return [NSUserDefaults.standardUserDefaults boolForKey:key];
    }
}

BOOL HGSetEnabled(HGFeature feature, BOOL enabled) {
    NSString *key = HGPreferenceKey(feature);
    if (!key) return NO;
    @synchronized (HGPreferenceLock()) {
        [NSUserDefaults.standardUserDefaults setBool:enabled forKey:key];
    }
    return YES;
}

static BOOL HGPreferencesFailure(NSError **error, NSInteger code, NSString *description) {
    if (error) *error = [NSError errorWithDomain:HGPreferencesErrorDomain code:code
        userInfo:@{NSLocalizedDescriptionKey: description}];
    return NO;
}

static BOOL HGIsJSONBoolean(id value) {
    return [value isKindOfClass:NSNumber.class] &&
        CFGetTypeID((__bridge CFTypeRef)value) == CFBooleanGetTypeID();
}

static HGFeature HGFeatureForIdentifier(NSString *identifier) {
    if (![identifier isKindOfClass:NSString.class]) return HGFeatureCount;
    for (HGFeature feature = 0; feature < HGFeatureCount; feature++) {
        if ([identifier isEqualToString:HGFeatureIdentifier(feature)]) return feature;
    }
    return HGFeatureCount;
}

static NSDictionary<NSString *, NSNumber *> *HGValidatedFeatureValues(id values, NSError **error) {
    if (![values isKindOfClass:NSDictionary.class] || [values count] == 0 ||
        [values count] > HGFeatureCount) {
        HGPreferencesFailure(error, 5, @"备份必须包含至少一个已支持的 Honggo 功能开关。");
        return nil;
    }
    NSMutableDictionary<NSString *, NSNumber *> *validated = [NSMutableDictionary dictionary];
    for (id identifier in values) {
        if (HGFeatureForIdentifier(identifier) == HGFeatureCount) {
            HGPreferencesFailure(error, 6, @"备份包含当前版本不认识的功能，请使用支持该备份的 Honggo 版本。");
            return nil;
        }
        id value = values[identifier];
        if (!HGIsJSONBoolean(value)) {
            HGPreferencesFailure(error, 7, @"功能开关必须是 JSON 的 true 或 false，不能是数字或文字。");
            return nil;
        }
        validated[identifier] = [value boolValue] ? @YES : @NO;
    }
    return validated.copy;
}

NSData *HGPreferencesExport(NSError **error) {
    if (error) *error = nil;
    NSMutableDictionary<NSString *, NSNumber *> *features = [NSMutableDictionary dictionary];
    @synchronized (HGPreferenceLock()) {
        for (HGFeature feature = 0; feature < HGFeatureCount; feature++) {
            features[HGFeatureIdentifier(feature)] =
                @([NSUserDefaults.standardUserDefaults boolForKey:HGPreferenceKey(feature)]);
        }
    }
    NSDictionary *document = @{@"format": @"Honggo.preferences", @"schema": @1,
        @"pluginVersion": HG_PLUGIN_VERSION, @"features": features};
    return [NSJSONSerialization dataWithJSONObject:document
        options:NSJSONWritingPrettyPrinted | NSJSONWritingSortedKeys error:error];
}

NSDictionary<NSString *, NSNumber *> *HGPreferencesParse(NSData *data, NSError **error) {
    if (error) *error = nil;
    if (![data isKindOfClass:NSData.class] || data.length == 0 || data.length > HGPreferencesMaximumBytes) {
        HGPreferencesFailure(error, 1, @"请选择不超过 64 KB 的 Honggo 配置 JSON 文件。");
        return nil;
    }
    NSError *decodeError = nil;
    id document = [NSJSONSerialization JSONObjectWithData:data options:0 error:&decodeError];
    if (![document isKindOfClass:NSDictionary.class] || decodeError) {
        HGPreferencesFailure(error, 2, @"文件不是有效的 Honggo 配置 JSON。");
        return nil;
    }
    NSSet *keys = [NSSet setWithArray:[document allKeys]];
    NSSet *expected = [NSSet setWithArray:@[@"format", @"schema", @"pluginVersion", @"features"]];
    if (![keys isEqualToSet:expected] || ![document[@"format"] isEqual:@"Honggo.preferences"]) {
        HGPreferencesFailure(error, 3, @"该文件不是支持的 Honggo 配置备份；不会导入其他应用的设置。");
        return nil;
    }
    id schema = document[@"schema"];
    if (![schema isKindOfClass:NSNumber.class] || HGIsJSONBoolean(schema) ||
        CFNumberIsFloatType((__bridge CFNumberRef)schema) || ![schema isEqualToNumber:@1]) {
        HGPreferencesFailure(error, 4, @"当前 Honggo 不支持这个备份格式版本。");
        return nil;
    }
    id version = document[@"pluginVersion"];
    if (![version isKindOfClass:NSString.class] || [version length] == 0 || [version length] > 64) {
        HGPreferencesFailure(error, 3, @"备份中的 Honggo 版本信息无效。");
        return nil;
    }
    return HGValidatedFeatureValues(document[@"features"], error);
}

BOOL HGPreferencesApply(NSDictionary<NSString *, NSNumber *> *features, NSError **error) {
    if (error) *error = nil;
    NSDictionary<NSString *, NSNumber *> *validated = HGValidatedFeatureValues(features, error);
    if (!validated) return NO;
    @synchronized (HGPreferenceLock()) {
        for (NSString *identifier in validated) {
            HGFeature feature = HGFeatureForIdentifier(identifier);
            [NSUserDefaults.standardUserDefaults setBool:validated[identifier].boolValue
                forKey:HGPreferenceKey(feature)];
        }
    }
    return YES;
}

NSUInteger HGPreferencesChangeCount(NSDictionary<NSString *, NSNumber *> *features) {
    NSDictionary<NSString *, NSNumber *> *validated = HGValidatedFeatureValues(features, nil);
    if (!validated) return 0;
    NSUInteger changes = 0;
    @synchronized (HGPreferenceLock()) {
        for (NSString *identifier in validated) {
            HGFeature feature = HGFeatureForIdentifier(identifier);
            BOOL current = [NSUserDefaults.standardUserDefaults boolForKey:HGPreferenceKey(feature)];
            changes += current != validated[identifier].boolValue;
        }
    }
    return changes;
}

void HGPreferencesReset(void) {
    @synchronized (HGPreferenceLock()) {
        for (HGFeature feature = 0; feature < HGFeatureCount; feature++) {
            [NSUserDefaults.standardUserDefaults setBool:NO forKey:HGPreferenceKey(feature)];
        }
    }
}
