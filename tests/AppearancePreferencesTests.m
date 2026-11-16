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
        [migrated[@"icons.size"] isEqual:@28],
        @"installing R15 migrates persisted R14 stock geometry to a balanced thicker dock");
    Check([defaults boolForKey:@"Honggo.Appearance.r15GeometryMigrated"] &&
        [[[defaults dictionaryForKey:@"Honggo.Appearance.v1"] objectForKey:@"dock.height"] isEqual:@62] &&
        [migrated[@"dock.width"] isEqual:@0.88] && [migrated[@"layout.episodes.y"] isEqual:@-7] &&
        [migrated[@"icons.collect.normal.symbol"] isEqual:@"bookmark"] && [migrated[@"enabled"] boolValue],
        @"geometry migration persists once while retaining unrelated custom appearance settings");
    HGAppearanceApply(@{@"dock.height":@52,@"dock.radius":@26,@"dock.iconSize":@21,
        @"dock.extraSize":@48,@"icons.size":@26},nil);
    [NSNotificationCenter.defaultCenter postNotificationName:NSUserDefaultsDidChangeNotification object:defaults];
    Check(HGAppearanceNumber(@"dock.height")==52 && HGAppearanceNumber(@"dock.radius")==26 &&
        HGAppearanceNumber(@"dock.iconSize")==21 && HGAppearanceNumber(@"dock.extraSize")==48 &&
        HGAppearanceNumber(@"icons.size")==26,
        @"a user choosing former stock dimensions after migration keeps those choices on reload");

    SeedR14Appearance(@{@"dock.height":@70,@"dock.radius":@26,@"dock.iconSize":@23,
        @"dock.extraSize":@56,@"icons.size":@30});
    Check(HGAppearanceNumber(@"dock.height")==70 && HGAppearanceNumber(@"dock.radius")==26 &&
        HGAppearanceNumber(@"dock.iconSize")==23 && HGAppearanceNumber(@"dock.extraSize")==56 &&
        HGAppearanceNumber(@"icons.size")==30,
        @"nondefault user geometry and a radius paired with custom height remain unchanged");

    SeedR14Appearance(nil);
    Check(HGAppearanceNumber(@"dock.height")==62 && HGAppearanceNumber(@"dock.radius")==31 &&
        HGAppearanceNumber(@"dock.extraSize")==62 && HGAppearanceNumber(@"icons.size")==28 &&
        !HGAppearanceEnabled(),@"a fresh R15 install gets the new dimensions with appearance still opt-in");
}
int main(void) {@autoreleasepool {
    CheckR15GeometryMigration();
    HGAppearanceReset();
    [NSUserDefaults.standardUserDefaults setObject:@"preserve" forKey:@"host.fixture"];
    NSMutableSet *keys=[NSMutableSet set];
    BOOL unique=YES,defaults=YES;
    for(NSDictionary *entry in HGAppearanceCatalog()) {
        unique &= ![keys containsObject:entry[@"key"]];[keys addObject:entry[@"key"]];
        if([entry[@"kind"] isEqual:@"bool"])defaults &= !HGAppearanceFlag(entry[@"key"]);
    }
    Check(unique && keys.count>100,@"every visual option has one stable key");
    Check(defaults && !HGAppearanceEnabled(),@"all appearance switches default off");
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
    Check(HGAppearanceSet(@"dock.tint",@"#123ABC",nil),@"hex color accepted");
    Check(!HGAppearanceSet(@"dock.tint",@"red",nil),@"nonhex color rejected");
    Check(HGAppearanceSet(@"icons.like.normal.symbol",@"heart.circle.fill",nil),@"SF Symbol syntax accepted");
    Check(!HGAppearanceSet(@"icons.like.normal.asset",@"../../secret.png",nil),@"image path traversal rejected");
    Check(!HGAppearanceSet(@"fonts.asset",@"file:///etc/passwd",nil),@"font URI rejected");
    Check(!HGAppearanceSet(@"fonts.asset",@"a.ttf/../b",nil),@"font path traversal rejected");
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
    Check(HGAppearanceApply(HGAppearancePreset(@"glass"),nil),@"glass preset validates");
    Check(HGAppearanceEnabled() && HGAppearanceFlag(@"dock.enabled") && HGAppearanceFlag(@"icons.enabled") && HGAppearanceFlag(@"panels.speed"),@"glass preset activates visible modules");
    Check(HGAppearanceNeedsGlass(),@"startup knows the visual preset requests glass");
    Check(HGAppearanceApply(HGAppearancePreset(@"focus"),nil) && HGAppearanceFlag(@"dock.autoCollapse") && HGAppearanceFlag(@"layout.tags.hidden"),@"focus preset supplies collapse and clean layout");
    HGAppearanceReset();
    Check(!HGAppearanceEnabled() && !HGAppearanceNeedsGlass(),@"reset stops the new appearance path");
    Check([[NSUserDefaults.standardUserDefaults stringForKey:@"host.fixture"] isEqual:@"preserve"],@"reset preserves unrelated host settings");
    NSLog(@"Appearance preferences: %lu checks, %lu failures",(unsigned long)checks,(unsigned long)failures);
    return failures?1:0;
}}
