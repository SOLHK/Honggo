#import "AppearancePreferences.h"
#import <CoreFoundation/CoreFoundation.h>
#include <math.h>
static NSUInteger checks,failures;
static void Check(BOOL passed,NSString *name){checks++;if(!passed)failures++;NSLog(@"%@ %@",passed?@"PASS":@"FAIL",name);}
int main(void) {@autoreleasepool {
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
