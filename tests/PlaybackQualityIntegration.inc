@interface FQVShortVideoListVCPlayComponent : NSObject
@property(nonatomic,strong) id listVCContext;
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

@interface TTVideoEngineModel : NSObject
@property(nonatomic,copy) NSArray *supportedResolutionTypes;
@end
@implementation TTVideoEngineModel @end

@interface FQVPlayerEngineDataResolutionItem : NSObject
@property(nonatomic,strong) TTVideoEngineModel *videoModel;
@property(nonatomic,strong) NSNumber *forceResolutionTypeNumber;
@property(nonatomic,strong) NSNumber *resolutionTypeNumberWithCache;
- (NSUInteger)finalResolutionTypeWithSupportResolutions:(NSArray *)supported videoModel:(id)model;
@end
@implementation FQVPlayerEngineDataResolutionItem
- (NSUInteger)finalResolutionTypeWithSupportResolutions:(NSArray *)supported videoModel:(id)model {
    return (self.forceResolutionTypeNumber ?: self.resolutionTypeNumberWithCache ?: @1).unsignedIntegerValue;
}
@end

static NSUInteger HGAQualityMenuExperiment;
@interface FQVShortVideoABTestHelper : NSObject @end
@implementation FQVShortVideoABTestHelper
+ (NSUInteger)shortVideoResolutionControl { return HGAQualityMenuExperiment; }
@end

@interface FQVShortVideoListVCPlayerComponent : NSObject
@property(nonatomic) BOOL enableResolutionEntrance;
@property(nonatomic,copy) NSDictionary *videoAllowResolutionDict;
@property(nonatomic,strong) TTVideoEngineModel *currentVideoDataModel;
@property(nonatomic) NSUInteger engineSwitches;
- (FQVPlayerEngineDataResolutionItem *)resolutionItemWithModel:(TTVideoEngineModel *)model;
@end
@implementation FQVShortVideoListVCPlayerComponent
- (id)listVCContext { return self; }
- (id)config { return self; }
- (FQVPlayerEngineDataResolutionItem *)resolutionItemWithModel:(TTVideoEngineModel *)model {
    FQVPlayerEngineDataResolutionItem *item=[FQVPlayerEngineDataResolutionItem new];
    item.videoModel=model;
    return item;
}
// The old play/display path would find a valid menu, player and switchable
// engine here. Counting the operation detects asynchronous writes after swiping.
- (id)currentPlayer { return self; }
- (id)shareComponent { return self; }
- (id)playerComponent { return self; }
- (id)videoEngine { return self; }
- (id)resolutionItem { return self; }
- (NSArray *)options { return self.videoAllowResolutionDict.allValues; }
- (NSArray *)supportedResolutionTypes { return self.currentVideoDataModel.supportedResolutionTypes; }
- (NSUInteger)currentResolution { return 1; }
- (float)playRate { return 1.0f; }
- (void)updateDropFrameStrategyWithPlayRate:(float)rate resolutionType:(NSUInteger)type {}
- (void)configResolution:(NSUInteger)type completion:(void(^)(BOOL,NSUInteger))completion {
    self.engineSwitches++;
    completion(YES,type);
}
@end

@interface HGAQualityCellFixture : NSObject
@property(nonatomic,strong) id model;
@end
@implementation HGAQualityCellFixture @end

static void RunPlaybackQualityChecks(void) {
    BOOL saved=HGAppearanceFlag(@"player.highestQuality");
    HGAppearanceSet(@"player.highestQuality",@YES,nil);
    HGAQualityMenuExperiment=1;
    FQVShortVideoListVCPlayerComponent *builder=[FQVShortVideoListVCPlayerComponent new];
    builder.enableResolutionEntrance=YES;
    builder.videoAllowResolutionDict=@{@0:@"360P",@1:@"480P",@8:@"540P",@2:@"720P",@3:@"1080P",@9:@"2160P"};
    TTVideoEngineModel *model=[TTVideoEngineModel new];
    model.supportedResolutionTypes=@[@8,@2,@9];
    FQVPlayerEngineDataResolutionItem *preload=[builder resolutionItemWithModel:model];
    NSUInteger selected=[preload finalResolutionTypeWithSupportResolutions:model.supportedResolutionTypes videoModel:model];
    Check(selected==2 && !preload.forceResolutionTypeNumber,
        @"preload chooses the highest supported host menu entry, without creating a live-switch target");
    FQVPlayerEngineDataResolutionItem *start=[builder resolutionItemWithModel:model];
    start.forceResolutionTypeNumber=@(selected);
    model.supportedResolutionTypes=@[@1,@2,@3];
    Check([start finalResolutionTypeWithSupportResolutions:model.supportedResolutionTypes videoModel:model]==selected,
        @"starting the preloaded episode retains its stream even when a higher quality becomes available");
    FQVPlayerEngineDataResolutionItem *fresh=[builder resolutionItemWithModel:model];
    Check([fresh finalResolutionTypeWithSupportResolutions:model.supportedResolutionTypes videoModel:model]==3 &&
          [fresh finalResolutionTypeWithSupportResolutions:@[@9] videoModel:model]==1 &&
          [fresh finalResolutionTypeWithSupportResolutions:model.supportedResolutionTypes videoModel:[TTVideoEngineModel new]]==1,
        @"fresh playback uses available 1080P and leaves unknown options or mismatched model decisions native");
    fresh.resolutionTypeNumberWithCache=@2;
    BOOL cached=[fresh finalResolutionTypeWithSupportResolutions:model.supportedResolutionTypes videoModel:model]==2;
    fresh.forceResolutionTypeNumber=@1;
    Check(cached && [fresh finalResolutionTypeWithSupportResolutions:model.supportedResolutionTypes videoModel:model]==1,
        @"a cached previous episode and an explicit manual/download resolution retain the native choice");
    for(NSUInteger gate=0;gate<2;gate++) {
        HGAQualityMenuExperiment=gate;
        builder.enableResolutionEntrance=!gate;
        FQVPlayerEngineDataResolutionItem *denied=[builder resolutionItemWithModel:model];
        Check([denied finalResolutionTypeWithSupportResolutions:model.supportedResolutionTypes videoModel:model]==1,
            @"both native menu eligibility gates must allow quality selection");
    }
    builder.enableResolutionEntrance=YES;
    FQVPlayerEngineDataResolutionItem *unrelated=[FQVPlayerEngineDataResolutionItem new];
    unrelated.videoModel=model;
    Check([unrelated finalResolutionTypeWithSupportResolutions:model.supportedResolutionTypes videoModel:model]==1,
        @"resolution requests from other host components keep their native choice");

    FQVShortVideoListVCPlayComponent *playing=[FQVShortVideoListVCPlayComponent new];
    playing.listVCContext=builder;
    HGAQualityCellFixture *cell=[HGAQualityCellFixture new];
    for(NSUInteger i=0;i<3;i++) {
        TTVideoEngineModel *episode=[TTVideoEngineModel new];
        episode.supportedResolutionTypes=model.supportedResolutionTypes;
        cell.model=builder.currentVideoDataModel=episode;
        [playing dataModelDidPlay:episode];
        [playing cellView:cell vPlayerReadyToDisplay:nil];
    }
    CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.8,false);
    Check(playing.nativePlays==3 && playing.nativeDisplays==3 && builder.engineSwitches==0,
        @"consecutive episode starts and ready-to-display callbacks never switch the active engine, including after the old delay");

    FQVPlayerEngineDataResolutionItem *pending=[builder resolutionItemWithModel:model];
    HGAppearanceSet(@"player.highestQuality",@NO,nil);
    Check([pending finalResolutionTypeWithSupportResolutions:model.supportedResolutionTypes videoModel:model]==1,
        @"turning off highest quality before a pending preload chooses its stream restores the native decision");
    HGAppearanceSet(@"player.highestQuality",@(saved),nil);
}
