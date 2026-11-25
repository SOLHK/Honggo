#import "Preferences.h"
#import "HonggoVersion.h"

static NSUInteger HGChecks, HGFailures;

static void HGCheck(BOOL passed, NSString *name) {
    HGChecks++;
    HGFailures += !passed;
    NSLog(@"%@ %@", passed ? @"PASS" : @"FAIL", name);
}

static NSData *HGEncode(id document) {
    return [NSJSONSerialization dataWithJSONObject:document options:0 error:nil];
}

static NSMutableDictionary *HGDocument(NSDictionary *features) {
    return [@{@"format": @"Honggo.preferences", @"schema": @1,
        @"pluginVersion": HG_PLUGIN_VERSION, @"features": features} mutableCopy];
}

static NSDictionary *HGCurrentFeatures(void) {
    return HGPreferencesParse(HGPreferencesExport(nil), nil);
}

static void HGCheckRejectedData(NSData *data, NSString *name) {
    NSDictionary *before = HGCurrentFeatures();
    NSError *error = nil;
    NSDictionary *parsed = HGPreferencesParse(data, &error);
    HGCheck(parsed == nil && [error.domain isEqual:HGPreferencesErrorDomain] &&
        error.localizedDescription.length > 0 && [HGCurrentFeatures() isEqual:before], name);
}

int main(void) {
    @autoreleasepool {
        NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
        NSArray<NSString *> *sentinelKeys = @[@"host.account", @"host.theme",
            @"Honggo.enhancedDiagnostics", @"HongguoGlass.futureSetting"];
        NSMutableDictionary *prior = [NSMutableDictionary dictionary];
        for (HGFeature feature = 0; feature < HGFeatureCount; feature++) {
            NSString *key = HGPreferenceKey(feature);
            prior[key] = [defaults objectForKey:key] ?: NSNull.null;
        }
        for (NSString *key in sentinelKeys) prior[key] = [defaults objectForKey:key] ?: NSNull.null;
        [defaults setObject:@{@"sentinel": @"not-for-backup"} forKey:@"host.account"];
        [defaults setObject:@"host-theme-unchanged" forKey:@"host.theme"];
        [defaults setBool:YES forKey:@"Honggo.enhancedDiagnostics"];
        [defaults setObject:@"unrelated-prefix-key" forKey:@"HongguoGlass.futureSetting"];

        for (HGFeature f=0;f<HGFeatureCount;f++) [defaults removeObjectForKey:HGPreferenceKey(f)];
        [defaults setBool:NO forKey:HGPreferenceKey(HGSplash)];
        HGCheck(!HGIsEnabled(HGSplash) && HGIsEnabled(HGSidebarGameCenter) && HGIsEnabled(HGScreenshotShopping),
            @"registering ad defaults respects an explicit off value from an earlier installation");
        HGPreferencesReset();
        NSDictionary *initial=HGCurrentFeatures();
        BOOL matches=YES;
        for (HGFeature f=0;f<HGFeatureCount;f++) {
            BOOL expected=f!=HGGlass && f!=HGGlassControls;
            matches &= HGIsEnabled(f)==expected && [initial[HGFeatureIdentifier(f)] isEqual:@(expected)];
        }
        HGCheck(matches,@"all sixteen ad switches start enabled in runtime and backup while the two glass switches stay off");
        HGSetEnabled(HGGlass, YES);
        HGSetEnabled(HGSplash, NO);
        HGSetEnabled(HGRedPacket, NO);
        HGSetEnabled(HGGlassControls, YES);
        HGCheck(HGIsEnabled(HGGlass) && HGIsEnabled(HGFeed) && !HGIsEnabled(HGSplash),
            @"known feature switches persist independently");

        NSArray<NSString *> *shipped = @[@"glass", @"splash", @"topview", @"feed", @"comment",
            @"pause", @"patch", @"commerce", @"marketing", @"redPacket", @"bookstoreAd",
            @"couponPendant", @"miniGamePendant", @"welfareGuide", @"survey", @"glassControls", @"sidebarGameCenter", @"screenshotShopping"];
        BOOL stable = shipped.count == HGFeatureCount;
        for (HGFeature feature = 0; feature < MIN(shipped.count, HGFeatureCount); feature++) {
            stable &= [HGFeatureIdentifier(feature) isEqual:shipped[feature]] &&
                [HGPreferenceKey(feature) isEqual:[@"HongguoGlass." stringByAppendingString:shipped[feature]]] &&
                HGFeatureTitle(feature).length > 0 && HGFeatureSearchText(feature).length > 0;
        }
        HGCheck(stable, @"all shipped identifiers, order and preference keys stay compatible");
        HGCheck(!HGSetEnabled(HGFeatureCount, YES) && !HGIsEnabled(HGFeatureCount) &&
            !HGSetEnabled((HGFeature)NSUIntegerMax, YES) && !HGIsEnabled((HGFeature)NSUIntegerMax) &&
            HGPreferenceKey(HGFeatureCount) == nil && HGFeatureTitle(HGFeatureCount) == nil &&
            HGFeatureSearchText(HGFeatureCount) == nil,
            @"out-of-range feature values do not read or write preferences");

        NSError *error = nil;
        NSData *backup = HGPreferencesExport(&error);
        NSDictionary *document = [NSJSONSerialization JSONObjectWithData:backup options:0 error:nil];
        NSDictionary *saved = HGPreferencesParse(backup, &error);
        HGCheck(backup.length > 0 && backup.length < HGPreferencesMaximumBytes && error == nil &&
            [document[@"pluginVersion"] isEqual:HG_PLUGIN_VERSION] &&
            [document[@"format"] isEqual:@"Honggo.preferences"] && saved.count == HGFeatureCount,
            @"complete versioned backup round-trips through JSON validation");
        NSString *text = [[NSString alloc] initWithData:backup encoding:NSUTF8StringEncoding];
        HGCheck(![text containsString:@"host.account"] && ![text containsString:@"not-for-backup"] &&
            ![text containsString:@"enhancedDiagnostics"] && ![text containsString:@"futureSetting"],
            @"backup excludes host data, diagnostics consent and unknown namespace keys");

        HGPreferencesReset();
        HGCheck(HGPreferencesChangeCount(saved) == 4,
            @"restore preview counts only switches that will change");
        HGCheck(HGPreferencesApply(saved, &error) && error == nil &&
            [HGCurrentFeatures() isEqual:saved] && HGPreferencesChangeCount(saved) == 0,
            @"validated backup restores feature values and produces zero repeat changes");

        NSDictionary *partial = HGPreferencesParse(HGEncode(HGDocument(@{@"feed": @NO})), &error);
        HGCheck(partial.count == 1 && HGPreferencesChangeCount(partial) == 1 &&
            HGPreferencesApply(partial, &error) && !HGIsEnabled(HGFeed) &&
            HGIsEnabled(HGGlass) && HGIsEnabled(HGMarketing) && HGIsEnabled(HGGlassControls),
            @"partial restore leaves every omitted switch unchanged");

        NSMutableDictionary *invalid = HGDocument(@{@"glass": @NO});
        invalid[@"format"] = @"OtherApp.preferences";
        HGCheckRejectedData(HGEncode(invalid), @"foreign format is rejected without changing any switch");
        invalid = HGDocument(@{@"glass": @NO});
        invalid[@"schema"] = @2;
        HGCheckRejectedData(HGEncode(invalid), @"unsupported schema is rejected");
        invalid[@"schema"] = @YES;
        HGCheckRejectedData(HGEncode(invalid), @"boolean schema cannot masquerade as numeric version one");
        HGCheckRejectedData([@"{\"format\":\"Honggo.preferences\",\"schema\":1.0,\"pluginVersion\":\"1.0\",\"features\":{\"glass\":true}}"
            dataUsingEncoding:NSUTF8StringEncoding], @"fractional schema representation is rejected");
        HGCheckRejectedData([@"{\"format\":\"Honggo.preferences\",\"schema\":18446744073709551617,\"pluginVersion\":\"1.0\",\"features\":{\"glass\":true}}"
            dataUsingEncoding:NSUTF8StringEncoding], @"oversized numeric schema cannot wrap to version one");
        invalid = HGDocument(@{@"glass": @NO});
        invalid[@"extra"] = @"unrecognized";
        HGCheckRejectedData(HGEncode(invalid), @"unknown wrapper fields are rejected");
        [invalid removeObjectForKey:@"extra"];
        [invalid removeObjectForKey:@"pluginVersion"];
        HGCheckRejectedData(HGEncode(invalid), @"missing version metadata is rejected");
        invalid[@"pluginVersion"] = @42;
        HGCheckRejectedData(HGEncode(invalid), @"non-string plugin version is rejected");
        invalid[@"pluginVersion"] = @"";
        HGCheckRejectedData(HGEncode(invalid), @"empty plugin version is rejected");
        invalid[@"pluginVersion"] = [@"x" stringByPaddingToLength:65 withString:@"x" startingAtIndex:0];
        HGCheckRejectedData(HGEncode(invalid), @"oversized plugin version is rejected");
        HGCheckRejectedData(HGEncode(HGDocument(@{})), @"empty restore is rejected");
        HGCheckRejectedData(HGEncode(HGDocument(@{@"glass": @NO, @"newerFeature": @YES})),
            @"unknown newer feature rejects the whole restore");
        HGCheckRejectedData(HGEncode(HGDocument(@{@"host.account": @NO})),
            @"foreign preference keys cannot be restored");
        HGCheckRejectedData(HGEncode(HGDocument(@{@"glass": @1})),
            @"numeric one is not accepted as a JSON boolean");
        HGCheckRejectedData(HGEncode(HGDocument(@{@"glass": @0})),
            @"numeric zero is not accepted as a JSON boolean");
        HGCheckRejectedData(HGEncode(HGDocument((id)@{@"glass": @"true"})),
            @"string booleans are rejected");
        HGCheckRejectedData(HGEncode(HGDocument((id)@{@"glass": NSNull.null})),
            @"null feature values are rejected");
        HGCheckRejectedData(HGEncode(HGDocument((id)@[@YES])), @"feature arrays are rejected");
        HGCheckRejectedData(HGEncode(@[]), @"top-level arrays are rejected");
        HGCheckRejectedData([@"{invalid" dataUsingEncoding:NSUTF8StringEncoding], @"malformed JSON is rejected");
        const unsigned char invalidUTF8[] = {0xFF, 0xFE, 0x00};
        HGCheckRejectedData([NSData dataWithBytes:invalidUTF8 length:sizeof(invalidUTF8)], @"invalid text encoding is rejected");
        HGCheckRejectedData([NSData data], @"empty data is rejected");
        HGCheckRejectedData(nil, @"nil data is rejected");
        HGCheckRejectedData([NSMutableData dataWithLength:HGPreferencesMaximumBytes + 1],
            @"data beyond the size limit is rejected before JSON parsing");

        NSMutableData *limit = backup.mutableCopy;
        NSData *space = [@" " dataUsingEncoding:NSUTF8StringEncoding];
        while (limit.length < HGPreferencesMaximumBytes) [limit appendData:space];
        HGCheck(HGPreferencesParse(limit, &error) != nil && error == nil,
            @"valid JSON exactly at the documented size limit is accepted");

        NSDictionary *before = HGCurrentFeatures();
        HGCheck(!HGPreferencesApply(@{@"glass": @NO, @"unknown": @YES}, &error) && error != nil &&
            [HGCurrentFeatures() isEqual:before],
            @"apply revalidates every key before its first write");
        HGCheck(!HGPreferencesApply(@{@"glass": @NO, @"feed": @1}, &error) && error != nil &&
            [HGCurrentFeatures() isEqual:before],
            @"apply rejects invalid values without partially changing a valid key");
        HGCheck(!HGPreferencesApply((id)@{@1: @YES}, &error) && error != nil &&
            HGPreferencesChangeCount((id)@{@1: @YES}) == 0 && [HGCurrentFeatures() isEqual:before],
            @"non-string direct keys are safely rejected");

        HGPreferencesReset();
        HGCheck([HGCurrentFeatures() isEqual:initial], @"reset removes explicit choices and uses the feature defaults");
        HGCheck([[defaults dictionaryForKey:@"host.account"] isEqual:@{@"sentinel": @"not-for-backup"}] &&
            [[defaults stringForKey:@"host.theme"] isEqual:@"host-theme-unchanged"] &&
            [defaults boolForKey:@"Honggo.enhancedDiagnostics"] &&
            [[defaults stringForKey:@"HongguoGlass.futureSetting"] isEqual:@"unrelated-prefix-key"],
            @"restore and reset preserve host preferences, diagnostic consent and other keys");

        for (NSString *key in prior) {
            id value = prior[key];
            if (value == NSNull.null) [defaults removeObjectForKey:key];
            else [defaults setObject:value forKey:key];
        }
        NSLog(@"Preferences: %lu checks, %lu failures", (unsigned long)HGChecks, (unsigned long)HGFailures);
        return HGFailures == 0 ? 0 : 1;
    }
}
