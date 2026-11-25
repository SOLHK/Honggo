@interface HGAQualityFixture : NSObject
@property(nonatomic,strong) id currentVideoDataModel;
@property(nonatomic,copy) NSArray *supportedResolutionTypes;
@property(nonatomic,copy) NSArray *options;
@property(nonatomic,copy) NSDictionary *videoAllowResolutionDict;
@property(nonatomic) BOOL hasMenu;
@property(nonatomic) BOOL failNext;
@property(nonatomic) NSUInteger currentResolution;
@property(nonatomic) NSUInteger changes;
@property(nonatomic) NSUInteger lastStrategy;
@end
@implementation HGAQualityFixture
- (id)currentPlayer{return self;}
- (id)playerComponent{return self;}
- (id)shareComponent{return self;}
- (id)resolutionItem{return self.hasMenu ? self : nil;}
- (id)videoEngine{return self;}
- (BOOL)isVideoPlaying{return YES;}
- (float)playRate{return 1.5f;}
- (void)updateDropFrameStrategyWithPlayRate:(float)rate resolutionType:(NSUInteger)type {
    if(rate==1.5f)self.lastStrategy=type;
}
- (void)configResolution:(NSUInteger)type completion:(void(^)(BOOL,NSUInteger))finish {
    self.changes++;
    if(self.failNext){self.failNext=NO;finish(NO,self.currentResolution);return;}
    self.currentResolution=type;finish(YES,type);
}
@end
@interface FQVShortVideoListVCPlayComponent : NSObject
@property(nonatomic,strong) HGAQualityFixture *listVCContext;
@property(nonatomic) NSUInteger nativePlays;
@property(nonatomic) NSUInteger nativeDisplays;
- (void)dataModelDidPlay:(id)model;
- (void)cellView:(id)cell vPlayerReadyToDisplay:(id)event;
- (NSInteger)finishActionTypeAtCurrentTimeWithVideoDataModel:(NSNumber *)model;
@end
@implementation FQVShortVideoListVCPlayComponent
- (void)dataModelDidPlay:(id)model {self.nativePlays++;}
- (void)cellView:(id)cell vPlayerReadyToDisplay:(id)event {self.nativeDisplays++;}
- (NSInteger)finishActionTypeAtCurrentTimeWithVideoDataModel:(NSNumber *)model {return model.integerValue;}
@end
@interface SSShortVideoLandscapeSessionDataSource : NSObject
- (BOOL)isAutoInflowBlockedForVideoDataModel:(NSNumber *)model;
@end
@implementation SSShortVideoLandscapeSessionDataSource
- (BOOL)isAutoInflowBlockedForVideoDataModel:(NSNumber *)model {return model.boolValue;}
@end
static void RunAutoEnterSeriesChecks(void) {
    NSString *key=@"player.disableAutoEnterSeries";
    BOOL saved=HGAppearanceFlag(key);
    FQVShortVideoListVCPlayComponent *portrait=[FQVShortVideoListVCPlayComponent new];
    SSShortVideoLandscapeSessionDataSource *landscape=[SSShortVideoLandscapeSessionDataSource new];
    for(NSNumber *enabled in @[@YES,@NO]) {
        HGAppearanceSet(key,enabled,nil);
        BOOL correct=YES;
        for (NSInteger action=0;action<=7;action++) {
            NSInteger expected=enabled.boolValue && (action==4 || action==6) ? 2 : action;
            correct &= [portrait finishActionTypeAtCurrentTimeWithVideoDataModel:@(action)]==expected;
        }
        Check(correct && [landscape isAutoInflowBlockedForVideoDataModel:@NO]==enabled.boolValue &&
            [landscape isAutoInflowBlockedForVideoDataModel:@YES],
            enabled.boolValue ? @"automatic series routes stay on the current video in portrait and landscape" :
            @"disabling the setting restores each native completion decision");
    }
    HGAppearanceSet(key,saved ? @YES : @NO,nil);
}
@interface HGAQualityCellFixture : NSObject
@property(nonatomic,strong) id model;
@end
@implementation HGAQualityCellFixture @end
static void RunPlaybackQualityChecks(void) {
    BOOL saved=HGAppearanceFlag(@"player.highestQuality");
    HGAppearanceSet(@"player.highestQuality",@YES,nil);
    FQVShortVideoListVCPlayComponent *component=[FQVShortVideoListVCPlayComponent new];
    HGAQualityFixture *ctx=[HGAQualityFixture new];component.listVCContext=ctx;
    HGAQualityCellFixture *cell=[HGAQualityCellFixture new];
    ctx.currentVideoDataModel=cell.model=[NSObject new];ctx.hasMenu=YES;
    ctx.videoAllowResolutionDict=@{@9:@"360P",@2:@"720P",@4:@"1080P",@1:@"2160P"};
    ctx.supportedResolutionTypes=@[@9,@2,@4];ctx.options=@[@"360P",@"720P",@"2160P"];
    [component dataModelDidPlay:cell.model];[component cellView:cell vPlayerReadyToDisplay:nil];
    CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.06,false);
    Check(ctx.changes==1 && ctx.currentResolution==2 && ctx.lastStrategy==2 &&
        component.nativePlays==1 && component.nativeDisplays==1,
        @"quality ranks only playable menu choices and keeps native playback callbacks");
    ctx.currentResolution=9;
    [component cellView:cell vPlayerReadyToDisplay:nil];
    CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.03,false);
    Check(ctx.changes==1 && ctx.currentResolution==9,@"a later manual choice survives repeated events for this episode");
    ctx.currentVideoDataModel=cell.model=[NSObject new];ctx.hasMenu=NO;
    [component dataModelDidPlay:cell.model];CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.03,false);
    BOOL denied=ctx.changes==1;
    ctx.hasMenu=YES;ctx.options=@[@"1080P",@"720P"];
    [component cellView:cell vPlayerReadyToDisplay:nil];ctx.currentVideoDataModel=[NSObject new];
    CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.03,false);
    Check(denied && ctx.changes==1,@"a missing menu and a recycled outgoing cell keep their native resolution");
    [component dataModelDidPlay:ctx.currentVideoDataModel];CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.04,false);
    Check(ctx.changes==2 && ctx.currentResolution==4,@"a new playable episode can choose its newly available 1080P");
    HGAppearanceSet(@"player.highestQuality",@NO,nil);ctx.currentVideoDataModel=[NSObject new];
    [component dataModelDidPlay:ctx.currentVideoDataModel];CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.03,false);
    Check(ctx.changes==2,@"disabling the default avoids future engine writes");
    ctx.currentResolution=9;ctx.failNext=YES;
    HGAppearanceSet(@"player.highestQuality",@YES,nil);
    CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.04,false);
    BOOL attempted=ctx.changes==3 && ctx.currentResolution==9;
    [component dataModelDidPlay:ctx.currentVideoDataModel];CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.04,false);
    Check(attempted && ctx.changes==4 && ctx.currentResolution==4,
        @"enabling applies to active playback and a failed switch retries on the next native event");
    HGAppearanceSet(@"player.highestQuality",@(saved),nil);
}
