#import "AppearancePreferences.h"
#import <CoreFoundation/CoreFoundation.h>
#include <math.h>
static NSUInteger checks,failures;
static void Check(BOOL passed,NSString *name){checks++;if(!passed)failures++;NSLog(@"%@ %@",passed?@"PASS":@"FAIL",name);}
static void SeedR14Appearance(NSDictionary *values) {
    NSUserDefaults *defaults=NSUserDefaults.standardUserDefaults;
    [defaults removeObjectForKey:@"Honggo.Appearance.r15GeometryMigrated"];
    if(values)[defaults setObject:values forKey:@"Honggo.Appearance.v1"];
    else [defaults removeObjectForKey:@"Honggo.Appearance.v1"];
    [NSNotificationCenter.defaultCenter postNotificationName:NSUserDefaultsDidChangeNotification object:defaults];
}
static void CheckR15GeometryMigration(void) {
    NSUserDefaults *defaults=NSUserDefaults.standardUserDefaults;
    SeedR14Appearance(@{@"enabled":@YES,@"dock.enabled":@YES,@"dock.height":@52,@"dock.radius":@26,
        @"dock.iconSize":@21,@"dock.extraSize":@48,@"icons.size":@26,@"dock.width":@0.88,
        @"layout.episodes.y":@-7,@"icons.collect.normal.symbol":@"bookmark"});
    NSDictionary *migrated=HGAppearanceValues();
    Check([migrated[@"dock.height"] isEqual:@62] && [migrated[@"dock.radius"] isEqual:@31] &&
        [migrated[@"dock.iconSize"] isEqual:@26] && [migrated[@"dock.extraSize"] isEqual:@62] &&
        migrated[@"icons.size"]==nil,
        @"installing R15 migrates persisted R14 stock geometry to a balanced thicker dock");
    Check([defaults boolForKey:@"Honggo.Appearance.r15GeometryMigrated"] &&
        [[[defaults dictionaryForKey:@"Honggo.Appearance.v1"] objectForKey:@"dock.height"] isEqual:@62] &&
        [migrated[@"dock.width"] isEqual:@0.88] && [migrated[@"layout.episodes.y"] isEqual:@-7] &&
        migrated[@"icons.collect.normal.symbol"]==nil && [migrated[@"enabled"] boolValue],
        @"geometry migration persists once while retaining unrelated custom appearance settings");
    HGAppearanceApply(@{@"dock.height":@52,@"dock.radius":@26,@"dock.iconSize":@21,
        @"dock.extraSize":@48},nil);
    [NSNotificationCenter.defaultCenter postNotificationName:NSUserDefaultsDidChangeNotification object:defaults];
    Check(HGAppearanceNumber(@"dock.height")==52 && HGAppearanceNumber(@"dock.radius")==26 &&
        HGAppearanceNumber(@"dock.iconSize")==21 && HGAppearanceNumber(@"dock.extraSize")==48 &&
        HGAppearanceValue(@"icons.size")==nil,
        @"a user choosing former stock dimensions after migration keeps those choices on reload");

    SeedR14Appearance(@{@"dock.height":@70,@"dock.radius":@26,@"dock.iconSize":@23,
        @"dock.extraSize":@56,@"icons.size":@30});
    Check(HGAppearanceNumber(@"dock.height")==70 && HGAppearanceNumber(@"dock.radius")==26 &&
        HGAppearanceNumber(@"dock.iconSize")==23 && HGAppearanceNumber(@"dock.extraSize")==56 &&
        HGAppearanceValue(@"icons.size")==nil,
        @"nondefault user geometry and a radius paired with custom height remain unchanged");

    SeedR14Appearance(nil);
    Check(HGAppearanceNumber(@"dock.height")==58 && HGAppearanceNumber(@"dock.radius")==29 &&
        HGAppearanceNumber(@"dock.extraSize")==54 &&
        !HGAppearanceEnabled(),@"a fresh install gets the current dimensions with appearance still opt-in");
}
static void CheckR20GeometryMigration(void) {
    NSUserDefaults *defaults=NSUserDefaults.standardUserDefaults;
    NSString *key=@"Honggo.Appearance.r20GeometryMigrated";
    [defaults removeObjectForKey:key];
    NSDictionary *oldStock=@{@"dock.width":@0.905,@"dock.height":@62,@"dock.radius":@31,
        @"dock.iconSize":@26,@"dock.extraSize":@62};
    SeedR14Appearance(oldStock);
    NSDictionary *migrated=HGAppearanceValues();
    Check([migrated[@"dock.width"] isEqual:@0.90] && [migrated[@"dock.height"] isEqual:@58] &&
        [migrated[@"dock.radius"] isEqual:@29] && [migrated[@"dock.iconSize"] isEqual:@24] &&
        [migrated[@"dock.extraSize"] isEqual:@54] && [defaults boolForKey:key],
        @"a complete R19 stock dock moves to current proportions once");
    HGAppearanceApply(oldStock,nil);
    [NSNotificationCenter.defaultCenter postNotificationName:NSUserDefaultsDidChangeNotification object:defaults];
    Check(HGAppearanceNumber(@"dock.width")==0.905 && HGAppearanceNumber(@"dock.height")==62,
        @"choosing the former stock dock after migration survives reload");
    [defaults removeObjectForKey:key];
    SeedR14Appearance(@{@"dock.width":@0.905,@"dock.height":@65,@"dock.radius":@31,
        @"dock.iconSize":@26,@"dock.extraSize":@62});
    Check(HGAppearanceNumber(@"dock.width")==0.905 && HGAppearanceNumber(@"dock.height")==65 &&
        HGAppearanceNumber(@"dock.iconSize")==26 && HGAppearanceNumber(@"dock.extraSize")==62,
        @"a customised geometry keeps the previous width, icon and accessory dimensions");
}
static void CheckBalancedDockMigration(void) {
    NSString *migration=@"Honggo.Appearance.balancedDockMigrated";
    NSUserDefaults *defaults=NSUserDefaults.standardUserDefaults;
    NSDictionary *previous=@{@"dock.width":@0.94,@"dock.height":@70,@"dock.radius":@35,
        @"dock.extraSize":@70,@"dock.iconSize":@30,@"dock.lift":@0,@"dock.action":@"search"};
    [defaults removeObjectForKey:migration];
    SeedR14Appearance(previous);
    NSDictionary *values=HGAppearanceValues(), *preset=HGAppearancePreset(@"glass");
    Check([values[@"dock.height"] isEqual:@58] && [values[@"dock.extraSize"] isEqual:@54] &&
        [values[@"dock.action"] isEqual:@"search"] && [values[@"dock.width"] isEqual:preset[@"dock.width"]] &&
        [defaults boolForKey:migration],@"beta9 stock dock migrates while retaining the user's accessory action");
    [defaults removeObjectForKey:migration];
    NSMutableDictionary *custom=[previous mutableCopy];custom[@"dock.height"]=@66;
    SeedR14Appearance(custom);
    Check(HGAppearanceNumber(@"dock.height")==66 && HGAppearanceNumber(@"dock.extraSize")==70,
        @"custom dock dimensions remain as saved");
    HGAppearanceApply(previous,nil);
    [NSNotificationCenter.defaultCenter postNotificationName:NSUserDefaultsDidChangeNotification object:defaults];
    Check(HGAppearanceNumber(@"dock.height")==70 && HGAppearanceNumber(@"dock.width")==0.94,
        @"choosing beta9 dimensions after migration remains possible");
}
static void CheckHeaderLiquidSelectionMigration(void) {
    NSString *migration=@"Honggo.Appearance.headerLiquidSelectionMigrated";
    NSUserDefaults *defaults=NSUserDefaults.standardUserDefaults;
    [defaults removeObjectForKey:migration];
    SeedR14Appearance(@{@"enabled":@YES,@"player.headerGlass":@YES,@"player.headerStyle":@"glass",
        @"player.headerSelectionStyle":@"plain"});
    Check([HGAppearanceString(@"player.headerSelectionStyle") isEqual:@"glass"] && [defaults boolForKey:migration],
        @"persisted glass top bar upgrades its default plain selection to liquid glass once");
    HGAppearanceSet(@"player.headerSelectionStyle",@"plain",nil);
    [NSNotificationCenter.defaultCenter postNotificationName:NSUserDefaultsDidChangeNotification object:defaults];
    Check([HGAppearanceString(@"player.headerSelectionStyle") isEqual:@"plain"],
        @"choosing plain top channel selection after migration survives reload");
}
int main(void) {@autoreleasepool {
    NSUserDefaults *defaultsStore=NSUserDefaults.standardUserDefaults;
    for (NSString *legacy in @[@"HongguoGlass.glass",@"HongguoGlass.glassControls"]) {
        [defaultsStore setBool:YES forKey:legacy];
        SeedR14Appearance(@{@"enabled":@NO,@"dock.enabled":@NO,@"dock.height":@66});
        BOOL wasDock=[legacy isEqual:@"HongguoGlass.glass"];
        Check(HGAppearanceEnabled() && HGAppearanceFlag(@"dock.enabled")==wasDock &&
            HGAppearanceNumber(@"dock.height")==66 && ![defaultsStore objectForKey:legacy],
            @"either historical glass switch promotes the master and preserves unrelated appearance settings");
        HGAppearanceSet(@"enabled",@NO,nil);
        [NSNotificationCenter.defaultCenter postNotificationName:NSUserDefaultsDidChangeNotification object:defaultsStore];
        Check(!HGAppearanceEnabled(),@"turning the migrated master off survives a fresh preference load");
    }
    CheckR15GeometryMigration();
    CheckR20GeometryMigration();
    CheckBalancedDockMigration();
    CheckHeaderLiquidSelectionMigration();
    HGAppearanceReset();
    [NSUserDefaults.standardUserDefaults setObject:@"preserve" forKey:@"host.fixture"];
    NSMutableSet *keys=[NSMutableSet set];
    NSMutableSet *onWhenEnabled=[NSMutableSet setWithArray:@[@"dock.enabled",@"player.finderGlass",@"player.finderSearch",@"player.finderRecognition",@"player.finderAI",@"player.mineGlass",@"player.episodeGlass",
        @"player.controlsGlass",@"player.commentMovesVideo",@"panels.episodes",@"panels.commentFloatingBar",
        @"panels.commentDetachedInput",@"mine.show.publish",@"mine.show.theme",@"player.homePreviewRepair"]];
    [onWhenEnabled addObjectsFromArray:[HGHostSettingsEntries() valueForKey:@"key"]];
    BOOL unique=YES,defaults=YES;
    for(NSDictionary *entry in HGAppearanceCatalog()) {
        unique &= ![keys containsObject:entry[@"key"]];[keys addObject:entry[@"key"]];
        if([entry[@"kind"] isEqual:@"bool"])
            defaults &= HGAppearanceFlag(entry[@"key"])==[onWhenEnabled containsObject:entry[@"key"]];
    }
    Check(unique && keys.count>100,@"every visual option has one stable key");
    Check(defaults && [onWhenEnabled isSubsetOfSet:keys] && !HGAppearanceEnabled(),
        @"appearance remains opt-in with the new scoped defaults ready under its master switch");
    Check(HGAppearanceFlag(@"player.homePreviewRepair") &&
        [[HGAppearanceSearch(@"首页预览 错集") valueForKey:@"key"] isEqual:@[@"player.homePreviewRepair"]],
        @"the preview episode setting defaults on and can be found by the reported symptom");
    HGAppearanceSet(@"player.homePreviewRepair",@NO,nil);
    Check(!HGAppearanceEnabled() && [HGAppearancePreset(@"glass")[@"player.homePreviewRepair"] isEqual:@NO] &&
        [HGAppearanceParse(HGAppearanceExport(nil),nil)[@"player.homePreviewRepair"] isEqual:@NO],
        @"an explicit preview opt-out survives appearance presets and configuration export");
    HGAppearanceSet(@"player.homePreviewRepair",@YES,nil);
    Check([[HGAppearanceSearch(@" GLAss  conTROLS ") valueForKey:@"key"] isEqual:@[@"enabled"]] &&
        [[HGAppearanceSearch(@"glassControls") valueForKey:@"key"] isEqual:@[@"enabled"]] &&
        [[HGAppearanceSearch(@"player.controlsGlass") valueForKey:@"key"] isEqual:@[@"player.controlsGlass"]],
        @"legacy glass-control names open the unified master and the precise playback key still finds its own setting");
    Check([HGAppearanceSearch(@"双指清屏").firstObject[@"key"] isEqual:@"gestures.pinchClean"] &&
        [HGAppearanceSearch(@"双击评论").firstObject[@"key"] isEqual:@"gestures.doubleTapComments"],
        @"global search recognizes the commonly used gesture names");
    Check([HGAppearanceSearch(@"暂停显示控件").firstObject[@"key"] isEqual:@"gestures.cleanWhilePlaying"] &&
        !HGAppearanceFlag(@"gestures.cleanWhilePlaying"), @"automatic playback clean is searchable and opt-in");
    Check([HGAppearanceSearch(@"评论区上移").firstObject[@"key"] isEqual:@"player.commentMovesVideo"] &&
        HGAppearanceFlag(@"player.commentMovesVideo"), @"the comment video movement switch is searchable and defaults to host behavior");
    Check(HGAppearanceFlag(@"panels.commentFloatingBar") && HGAppearanceFlag(@"panels.commentDetachedInput") &&
        [HGAppearanceString(@"panels.commentInputMaterial") isEqual:@"liquid"] &&
        HGAppearanceSet(@"panels.commentInputMaterial",@"blur",nil) &&
        [HGAppearanceString(@"panels.commentInputMaterial") isEqual:@"blur"] &&
        !HGAppearanceSet(@"panels.commentInputMaterial",@"invalid",nil) &&
        HGAppearanceSet(@"panels.commentInputMaterial",@"liquid",nil) &&
        [HGAppearanceSearch(@"底部悬浮输入框").firstObject[@"key"] isEqual:@"panels.commentFloatingBar"] &&
        [HGAppearanceSearch(@"分离悬浮").firstObject[@"key"] isEqual:@"panels.commentDetachedInput"],
        @"comment floating bar, detached popup input and liquid/blur input material validate and remain searchable");
    __block NSUInteger feedNotifies=0;
    id feedToken=[NSNotificationCenter.defaultCenter addObserverForName:HGVideoBlockedKeywordsDidChangeNotification
        object:nil queue:nil usingBlock:^(__unused NSNotification *n){ feedNotifies++; }];
    Check(!HGAppearanceFlag(@"player.hideFollowRecommendUsers") &&
        [HGAppearanceSearch(@"推荐其他精选用户").firstObject[@"key"] isEqual:@"player.hideFollowRecommendUsers"] &&
        HGAppearanceApply(@{@"enabled":@YES,@"player.hideFollowRecommendUsers":@YES},nil) &&
        feedNotifies==1 && HGAppearanceFlag(@"player.hideFollowRecommendUsers") &&
        HGAppearanceApply(@{@"enabled":@NO,@"player.hideFollowRecommendUsers":@NO},nil) &&
        feedNotifies==2,
        @"follow-tab recommended users card switch is opt-in, searchable and notifies live feed filters");
    [NSNotificationCenter.defaultCenter removeObserver:feedToken];
    Check(HGFlatTopStyle() && !HGFlatDockStyle() && !HGFlatPlayerStyle(),
        @"existing appearance settings start with flat top channels and glass dock and player actions");
    Check(HGAppearanceSet(@"dock.style",@"flat",nil) && HGFlatDockStyle() && HGFlatTopStyle() && !HGFlatPlayerStyle(),
        @"flat dock can be selected independently of the top bar and player");
    Check(HGAppearanceApply(@{@"player.headerStyle":@"glass",@"player.actionStyle":@"flat"},nil) &&
        HGFlatDockStyle() && !HGFlatTopStyle() && HGFlatPlayerStyle(),
        @"each area retains its choice when another area changes");
    NSDictionary *styles=HGAppearanceParse(HGAppearanceExport(nil),nil);
    Check([styles[@"dock.style"] isEqual:@"flat"] && [styles[@"player.headerStyle"] isEqual:@"glass"] &&
        [styles[@"player.actionStyle"] isEqual:@"flat"] && !HGAppearanceSet(@"player.headerStyle",@"unknown",nil),
        @"both styles round trip through the appearance backup and invalid values are rejected");
    HGAppearanceSet(@"player.episodeGlass",@NO,nil);
    Check(!HGAppearanceFlag(@"player.episodeGlass"),@"an explicit disabled episode preference overrides the new default");
    HGAppearanceReset();
    Check(![keys containsObject:@"player.fillButton"] && HGAppearanceValue(@"player.fillButton")==nil,
        @"the removed playback button has no setting or stored effective value");
    Check(!HGAppearanceSet(@"player.fillButton",@"show",nil),
        @"the removed button cannot be restored through the live settings API");
    Check([HGAppearanceString(@"player.separateProgress") isEqual:@"show"],
        @"episode spacing remains available once appearance is enabled");
    NSDictionary *fillDefinition=nil;
    for(NSDictionary *entry in HGAppearanceCatalog())if([entry[@"key"] isEqual:@"player.fullscreen"])fillDefinition=entry;
    Check([fillDefinition[@"title"] isEqual:@"横屏铺满"] && [fillDefinition[@"kind"] isEqual:@"bool"] &&
        !HGAppearanceFlag(@"player.fullscreen"),@"landscape fill is a settings-only switch that defaults off");
    for(NSString *legacyValue in @[@"show",@"hide"]) {
        NSDictionary *legacy=@{@"format":@"Honggo.appearance",@"schema":@1,
            @"values":@{@"player.fillButton":legacyValue,@"player.fullscreen":@YES,@"layout.episodes.y":@0}};
        NSDictionary *migrated=HGAppearanceParse([NSJSONSerialization dataWithJSONObject:legacy options:0 error:nil],nil);
        Check(migrated && !migrated[@"player.fillButton"] && [migrated[@"player.fullscreen"] boolValue] &&
            [migrated[@"layout.episodes.y"] isEqual:@0],@"R13 appearance backups omit the retired button key and preserve supported settings");
    }
    for(id invalid in @[@YES,@"invalid"]) {
        NSDictionary *legacy=@{@"format":@"Honggo.appearance",@"schema":@1,
            @"values":@{@"player.fillButton":invalid,@"enabled":@YES}};
        Check(!HGAppearanceParse([NSJSONSerialization dataWithJSONObject:legacy options:0 error:nil],nil),
            @"invalid retired button values are rejected instead of silently accepted");
    }
    NSMutableDictionary *legacyStored=[HGAppearanceValues() mutableCopy];legacyStored[@"player.fillButton"]=@"show";
    [NSUserDefaults.standardUserDefaults setObject:legacyStored forKey:@"Honggo.Appearance.v1"];
    [NSNotificationCenter.defaultCenter postNotificationName:NSUserDefaultsDidChangeNotification object:NSUserDefaults.standardUserDefaults];
    Check(!HGAppearanceValues()[@"player.fillButton"] && !HGAppearanceFlag(@"player.fullscreen"),
        @"an old persisted button preference cannot reactivate playback-page controls or automatic cropping");
    Check(HGAppearanceNumber(@"layout.like.scale")==1,@"layout default scale is identity");
    Check(HGAppearanceNumber(@"layout.like.x")==0,@"layout default offset is zero");
    Check(!HGAppearanceSet(@"unknown",@YES,nil),@"unknown setting rejected");
    Check(!HGAppearanceSet(@"enabled",@1,nil),@"number cannot masquerade as boolean");
    Check(!HGAppearanceSet(@"dock.height",@YES,nil),@"boolean cannot masquerade as dimension");
    Check(!HGAppearanceSet(@"dock.height",@43,nil),@"undersized touch height rejected");
    Check(!HGAppearanceSet(@"dock.height",@81,nil),@"oversized dimension rejected");
    Check(!HGAppearanceSet(@"layout.like.x",@(NAN),nil),@"NaN rejected");
    Check(!HGAppearanceSet(@"layout.like.scale",@(INFINITY),nil),@"infinity rejected");
    Check(!HGAppearanceSet(@"dock.material",@"unrecognized",nil),@"unknown material rejected");
    HGAppearanceSet(@"dock.material",@"clear",nil);
    Check([HGAppearanceString(@"player.material") isEqual:@"clear"] && [HGALiquidButtonMaterial() isEqual:@"clear"],
        @"selecting Clear on the Dock updates liquid button material");
    HGAppearanceSet(@"player.material",@"regular",nil);
    Check([HGAppearanceString(@"dock.material") isEqual:@"regular"],@"changing liquid buttons updates the Dock");
    HGAppearanceSet(@"player.material",@"blur",nil);
    HGAppearanceSet(@"dock.material",@"clear",nil);
    Check([HGAppearanceString(@"player.material") isEqual:@"blur"],@"an explicit traditional blur remains independent");
    HGAppearanceApply(@{@"player.material":@"regular",@"dock.material":@"regular"},nil);
    Check(!HGAppearanceSearch(@"底栏配色").count && !HGAppearanceSearch(@"自定义动作图标").count &&
        !HGAppearanceSet(@"dock.tint",@"#123ABC",nil) && !HGAppearanceSet(@"icons.enabled",@YES,nil),
        @"retired tint and icon options are not searchable or writable");
    Check(!HGAppearanceSet(@"fonts.asset",@"file:///etc/passwd",nil),@"font URI rejected");
    Check(!HGAppearanceSet(@"fonts.asset",@"a.ttf/../b",nil),@"font path traversal rejected");
    Check(!HGVideoBlockedKeywords().count &&
        [HGAppearanceSearch(@"关键词屏蔽").firstObject[@"key"] isEqual:@"video.blockedKeywords"],
        @"existing settings start with an empty, searchable keyword filter");
    __block NSUInteger keywordChanges=0;
    id observer=[NSNotificationCenter.defaultCenter addObserverForName:HGVideoBlockedKeywordsDidChangeNotification
        object:nil queue:nil usingBlock:^(NSNotification *change){keywordChanges++;}];
    Check(HGAppearanceSet(@"video.blockedKeywords",@[@" 都市 ",@"",@"ＡＩ",@"ai"],nil) &&
        [HGVideoBlockedKeywords() isEqual:@[@"都市",@"ＡＩ"]] &&
        !HGAppearanceSet(@"video.blockedKeywords",@[@1],nil) &&
        HGAppearanceSet(@"video.blockedKeywords",@[@"都市",@"ＡＩ "],nil) && keywordChanges==1 &&
        [HGAppearancePreset(@"glass")[@"video.blockedKeywords"] isEqual:HGVideoBlockedKeywords()],
        @"keyword storage trims, deduplicates case and width, rejects malformed updates and survives visual presets");
    [NSNotificationCenter.defaultCenter removeObserver:observer];
    Check(HGAppearanceSet(@"fonts.name",@"HelveticaNeue-Bold",nil),@"font name accepted");
    NSDictionary *before=HGAppearanceValues();
    Check(!HGAppearanceApply(@{@"enabled":@YES,@"layout.like.x":@999},nil) && [before isEqual:HGAppearanceValues()],@"one invalid field rejects the whole update");
    Check(!HGAppearanceApply(@{@"dock.hide.home":@YES,@"dock.hide.theater":@YES,@"dock.hide.store":@YES,@"dock.hide.mine":@YES},nil),@"at least one navigation item remains");
    Check(HGAppearanceApply(@{@"layout.like.x":@20},nil) && [HGAppearanceString(@"fonts.name") isEqual:@"HelveticaNeue-Bold"],@"partial restore preserves omitted settings");
    NSData *data=HGAppearanceExport(nil);NSDictionary *parsed=HGAppearanceParse(data,nil);
    Check([parsed isEqual:HGAppearanceValues()],@"versioned appearance JSON round trips");
    NSString *text=[[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    Check([text rangeOfString:@"host.fixture"].location==NSNotFound,@"backup excludes host preferences");
    Check(!HGAppearanceParse([@"{}" dataUsingEncoding:NSUTF8StringEncoding],nil),@"missing schema rejected");
    Check(!HGAppearanceParse([@"[]" dataUsingEncoding:NSUTF8StringEncoding],nil),@"array wrapper rejected");
    Check(!HGAppearanceParse([@"{" dataUsingEncoding:NSUTF8StringEncoding],nil),@"malformed JSON rejected");
    Check(!HGAppearanceParse([NSData data],nil),@"empty JSON rejected");
    Check(!HGAppearanceParse([NSMutableData dataWithLength:128*1024+1],nil),@"oversized JSON rejected");
    NSDictionary *root=@{@"format":@"Honggo.appearance",@"schema":@YES,@"values":@{@"enabled":@YES}};
    Check(!HGAppearanceParse([NSJSONSerialization dataWithJSONObject:root options:0 error:nil],nil),@"boolean schema rejected");
    HGAppearanceSet(@"dock.haptics",@NO,nil);
    Check(HGAppearanceApply(HGAppearancePreset(@"glass"),nil) && !HGAppearanceFlag(@"dock.haptics"),
        @"applying the glass preset preserves disabled navigation feedback");
    HGAppearanceSet(@"dock.haptics",@YES,nil);
    Check([HGAppearancePreset(@"focus")[@"dock.haptics"] boolValue],@"the focus preset preserves feedback opted in by the user");
    Check(HGAppearanceEnabled() && HGAppearanceFlag(@"dock.enabled") && HGAppearanceFlag(@"panels.speed"),@"glass preset activates visible modules");
    NSDictionary *legacyIcons=@{@"format":@"Honggo.appearance",@"schema":@1,
        @"values":@{@"dock.height":@54,@"dock.normal":@"#001122",@"icons.preset":@"outline",@"icons.collect.normal.symbol":@"bookmark.fill"}};
    NSDictionary *importedIcons=HGAppearanceParse([NSJSONSerialization dataWithJSONObject:legacyIcons options:0 error:nil],nil);
    Check([importedIcons isEqual:@{@"dock.height":@54}],@"old backups discard the retired entries and retain valid settings");
    Check(HGAppearanceEnabled(),@"startup knows the visual preset requests glass");
    Check(HGAppearanceApply(HGAppearancePreset(@"focus"),nil) && HGAppearanceFlag(@"dock.autoCollapse") && HGAppearanceFlag(@"layout.tags.hidden"),@"focus preset supplies collapse and clean layout");
    HGAppearanceReset();
    Check(!HGAppearanceEnabled() && !HGVideoBlockedKeywords().count,
        @"reset restores the appearance and empty video filter defaults");
    Check([[NSUserDefaults.standardUserDefaults stringForKey:@"host.fixture"] isEqual:@"preserve"],@"reset preserves unrelated host settings");
    NSLog(@"Appearance preferences: %lu checks, %lu failures",(unsigned long)checks,(unsigned long)failures);
    return failures?1:0;
}}
