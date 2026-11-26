// The 7.4.5.33 quality menu maps the current player's supported engine IDs to
// display labels. IDs need not sort in display-resolution order.
static NSNumber *HGAHighestMenuResolution(NSArray *supported,NSDictionary *labels,NSArray *options) {
    if(![supported isKindOfClass:NSArray.class] || ![labels isKindOfClass:NSDictionary.class] ||
        ![options isKindOfClass:NSArray.class])return nil;
    NSInteger highest=0;
    NSNumber *choice=nil;
    for(NSNumber *identifier in supported) {
        if(![identifier isKindOfClass:NSNumber.class])continue;
        NSString *label=labels[identifier];
        if(![label isKindOfClass:NSString.class] || ![options containsObject:label])continue;
        NSInteger lines=label.integerValue;
        if(lines>highest && [label isEqualToString:[NSString stringWithFormat:@"%ldP",(long)lines]]) {
            highest=lines;
            choice=identifier;
        }
    }
    return choice;
}

@interface HGAPlaybackQualitySelection : NSObject
@property(nonatomic,weak) id model;
@property(nonatomic,weak) id player;
@end
@implementation HGAPlaybackQualitySelection @end
static char HGAQualitySelectionKey;
static NSHashTable *HGAQualityOwners;
static __weak id HGALastPlayingQualityComponent;
static CFAbsoluteTime HGAQualitySwitchActiveUntil;
static NSUInteger HGAQualityScheduleGeneration;

static BOOL HGAPlaybackQualitySwitchActive(void) {
    return HGAQualitySwitchActiveUntil > CFAbsoluteTimeGetCurrent();
}

static BOOL HGAAnyVideoFeedScrolling(void) {
    Class feedClass=objc_lookUpClass("FQVCollectionView");
    if(!feedClass || !HGAManaged.count)return NO;
    for(UIView *view in HGAManaged.allObjects) {
        if(!view.window || ![view isKindOfClass:feedClass] || ![view isKindOfClass:UIScrollView.class])continue;
        UIScrollView *scroll=(UIScrollView *)view;
        if(scroll.tracking || scroll.dragging || scroll.decelerating)return YES;
    }
    return NO;
}

static BOOL HGAQualityMethod(id target,SEL selector,const char *encoding) {
    Method method=class_getInstanceMethod(object_getClass(target),selector);
    return method && strcmp(method_getTypeEncoding(method),encoding)==0;
}

static void HGAApplyPlaybackQuality(id component,id model) {
    if(!HGAppearanceFlag(@"player.highestQuality"))return;
    id context=HGReadObjectGetter(component,"listVCContext");
    if(!model || HGReadObjectGetter(context,"currentVideoDataModel")!=model)return;
    id player=HGReadObjectGetter(context,"currentPlayer");
    HGAPlaybackQualitySelection *last=objc_getAssociatedObject(context,&HGAQualitySelectionKey);
    if(last && (last.model!=model || last.player!=player)) {
        objc_setAssociatedObject(context,&HGAQualitySelectionKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        last=nil;
    }
    if(last || !player)return;

    // Asking for the actual menu item preserves scene, account and asset
    // eligibility. This getter builds option data without presenting the sheet.
    id share=HGReadObjectGetter(context,"shareComponent");
    id menu=HGReadObjectGetter(share,"resolutionItem");
    if(!menu)return;
    id playerComponent=HGReadObjectGetter(context,"playerComponent");
    NSNumber *choice=HGAHighestMenuResolution(HGReadObjectGetter(player,"supportedResolutionTypes"),
        HGReadObjectGetter(playerComponent,"videoAllowResolutionDict"),HGReadObjectGetter(menu,"options"));
    if(!choice)return;
    id engine=HGReadObjectGetter(player,"videoEngine");
    SEL current=sel_registerName("currentResolution");
    SEL configure=sel_registerName("configResolution:completion:");
    SEL rate=sel_registerName("playRate");
    SEL update=sel_registerName("updateDropFrameStrategyWithPlayRate:resolutionType:");
    if(!HGAQualityMethod(engine,configure,"v32@0:8Q16@?24") || !HGAQualityMethod(player,current,"Q16@0:8") ||
        !HGAQualityMethod(player,rate,"f16@0:8") || !HGAQualityMethod(player,update,"v28@0:8f16Q20"))return;
    last=[HGAPlaybackQualitySelection new];
    last.player=player;last.model=model;
    objc_setAssociatedObject(context,&HGAQualitySelectionKey,last,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    if(!HGAQualityOwners)HGAQualityOwners=[NSHashTable weakObjectsHashTable];
    [HGAQualityOwners addObject:context];
    NSUInteger selected=choice.unsignedIntegerValue;
    if(((NSUInteger(*)(id,SEL))objc_msgSend)(player,current)==selected)return;

    // Use the same engine operation and success callback as manual selection,
    // without writing the host's per-series/global manual choice history.
    HGAQualitySwitchActiveUntil=CFAbsoluteTimeGetCurrent()+1.25;
    __weak id weakContext=context;
    __weak HGAPlaybackQualitySelection *weakSelection=last;
    void (^completion)(BOOL,NSUInteger)=^(BOOL success,NSUInteger actual) {
        if(HGEnhancedDiagnostics())HGLog(@"playback.default-quality",@{@"requested":@(selected),
            @"applied":@(actual),@"success":@(success)});
        dispatch_async(dispatch_get_main_queue(),^{
            HGAQualitySwitchActiveUntil=0;
            HGAPlaybackQualitySelection *selection=weakSelection;
            id owner=weakContext;
            if(!owner || !selection || objc_getAssociatedObject(owner,&HGAQualitySelectionKey)!=selection)return;
            if(!success) {
                objc_setAssociatedObject(owner,&HGAQualitySelectionKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                return;
            }
            id live=selection.player;
            if(!live || !selection.model || HGReadObjectGetter(owner,"currentPlayer")!=live ||
                HGReadObjectGetter(owner,"currentVideoDataModel")!=selection.model)return;
            float speed=((float(*)(id,SEL))objc_msgSend)(live,rate);
            ((void(*)(id,SEL,float,NSUInteger))objc_msgSend)(live,update,speed,actual);
        });
    };
    ((void(*)(id,SEL,NSUInteger,id))objc_msgSend)(engine,configure,selected,completion);
}

static void HGASchedulePlaybackQuality(id component,id model,id cell) {
    if(!cell && model && NSThread.isMainThread)HGALastPlayingQualityComponent=component;
    if(!HGAppearanceFlag(@"player.highestQuality"))return;
    if(cell)model=HGReadObjectGetter(cell,"model");
    if(!model)return;
    // Commit target resolution synchronously when stationary (cold launch or
    // settled playback start) so TTVideoEngine starts at target quality before
    // first frame. While a feed scroll gesture is in flight (UITrackingRunLoopMode),
    // coalesce onto kCFRunLoopDefaultMode so intermediate swiped cells never
    // stall 120Hz scrolling with stream reconfigurations.
    if(!cell && NSThread.isMainThread && !HGAAnyVideoFeedScrolling()) {
        HGAQualityScheduleGeneration++;
        HGAApplyPlaybackQuality(component,model);
        return;
    }
    NSUInteger generation=++HGAQualityScheduleGeneration;
    __weak id weakComponent=component,weakModel=model;
    CFRunLoopPerformBlock(CFRunLoopGetMain(),kCFRunLoopDefaultMode,^{
        if(HGAQualityScheduleGeneration!=generation)return;
        HGAApplyPlaybackQuality(weakComponent,weakModel);
    });
    CFRunLoopWakeUp(CFRunLoopGetMain());
}
static void HGARefreshPlaybackQualityPreference(void) {
    static BOOL previous;
    BOOL enabled=HGAppearanceFlag(@"player.highestQuality");
    if(enabled==previous)return;
    previous=enabled;
    if(enabled) {
        id component=HGALastPlayingQualityComponent;
        id context=HGReadObjectGetter(component,"listVCContext");
        BOOL playing=NO;
        if(HGABoolGetter(context,@"isVideoPlaying",&playing) && playing)
            HGASchedulePlaybackQuality(component,HGReadObjectGetter(context,"currentVideoDataModel"),nil);
    }else {
        for(id owner in HGAQualityOwners)
            objc_setAssociatedObject(owner,&HGAQualitySelectionKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        [HGAQualityOwners removeAllObjects];
    }
}
