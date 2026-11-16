// Host 7.4.3.33: each named object getter has @16@0:8. Keep the original
// controls, gestures and labels; decorate only their existing bounds.
static NSDictionary<NSString *,NSArray<NSString *> *> *HGAPlayerControlGetters(void) {
    static NSDictionary *getters;static dispatch_once_t once;
    dispatch_once(&once,^{getters=@{
        @"FQVShortVideoVPlayerControlView":@[@"fullscreenBtn"],
        @"SSShortVideoBottomInsertView":@[@"playRateButton",@"cleanScreenView"],
        @"SSShortVideoSingleBottomInsertView":@[@"episodeInfoView"],
        @"SSShortVideoAlbumSeriesBottomInsertView":@[@"containerView"],
        @"SSShortVideoPugcVideoBottomInsertView":@[@"commentContainerView"],
        @"SSShortVideoTagListBottomInsertView":@[@"contentView"],
        @"FQCShortVideoDanmakuEntranceView":@[],
        @"SSShortVideoEpisodePanelCollectView":@[@"containerView",@"shareContainerView"],
        @"SSShortVideoEpisodesRelatedInfoView":@[@"contentView"],
        @"SSShortVideoEpisodesViewItemCell":@[@"bgView"],
        @"SSShortVideoEpisodesViewSwitchView":@[@"bgView"]
    };});return getters;
}
static NSArray<NSString *> *HGAPlayerControlClasses(void) {return HGAPlayerControlGetters().allKeys;}
static BOOL HGAEpisodeControl(UIView *root) {
    return [@[@"SSShortVideoEpisodePanelCollectView",@"SSShortVideoEpisodesRelatedInfoView",
        @"SSShortVideoEpisodesViewItemCell",@"SSShortVideoEpisodesViewSwitchView"] containsObject:NSStringFromClass(root.class)];
}
@interface HGAEpisodeCollectInk : NSObject
@property(nonatomic,weak) UILabel *label;
@property(nonatomic,strong) UIColor *sourceColor;
@property(nonatomic,weak) UIImageView *icon;
@property(nonatomic,strong) UIImage *sourceImage;
@property(nonatomic,strong) UIImage *appliedImage;
@property(nonatomic,strong) UIColor *sourceTint;
@end
@implementation HGAEpisodeCollectInk @end
static char HGAEpisodeCollectInkKey;
static void HGAApplyEpisodeCollectInk(UIView *root,BOOL enabled) {
    HGAEpisodeCollectInk *state=objc_getAssociatedObject(root,&HGAEpisodeCollectInkKey);
    if(!enabled) {
        if([state.label.textColor isEqual:UIColor.labelColor])state.label.textColor=state.sourceColor;
        if([state.icon.tintColor isEqual:UIColor.labelColor])state.icon.tintColor=state.sourceTint;
        if(state.icon.image==state.appliedImage)state.icon.image=state.sourceImage;
        objc_setAssociatedObject(root,&HGAEpisodeCollectInkKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);return;
    }
    UIView *label=HGAViewGetter(root,"textL",root),*icon=HGAViewGetter(root,"iconImageView",root);
    if(![label isKindOfClass:UILabel.class] || ![icon isKindOfClass:UIImageView.class]) {
        HGAApplyEpisodeCollectInk(root,NO);return;
    }
    if(state && (state.label!=label || state.icon!=icon)){HGAApplyEpisodeCollectInk(root,NO);state=nil;}
    if(!state){
        state=[HGAEpisodeCollectInk new];
        state.sourceColor=((UILabel *)label).textColor;state.sourceTint=icon.tintColor;
        objc_setAssociatedObject(root,&HGAEpisodeCollectInkKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    state.label=(id)label;state.icon=(id)icon;
    if(![state.label.textColor isEqual:UIColor.labelColor])state.sourceColor=state.label.textColor;
    if(![state.icon.tintColor isEqual:UIColor.labelColor])state.sourceTint=state.icon.tintColor;
    if(state.icon.image!=state.appliedImage) {
        state.sourceImage=state.icon.image;
        state.appliedImage=[state.sourceImage imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
    }
    state.label.textColor=UIColor.labelColor;state.icon.tintColor=UIColor.labelColor;
    if(state.icon.image!=state.appliedImage)state.icon.image=state.appliedImage;
}
static void HGAPlayerControlGlass(UIView *target,BOOL enabled,BOOL sheet) {
    if(!target)return;
    if(!enabled || !target.window){HGARemoveGlass(target);return;}
    CGFloat radius=MIN(22,MIN(target.bounds.size.width,target.bounds.size.height)/2);
    if(sheet) HGAInstallGlass(target,HGAppearanceString(@"panels.material"),radius,0.04);
    else HGAInstallPlaybackGlass(target,HGAppearanceString(@"player.material"),target.bounds,radius,nil);
}
@interface HGAPlayerChromeState : NSObject
@property(nonatomic,strong) NSHashTable<UIView *> *targets;
@property(nonatomic,strong) NSHashTable<UIView *> *paints;
@end
@implementation HGAPlayerChromeState @end
static char HGAPlayerChromeKey;
static HGAPlayerChromeState *HGAPlayerChromeTargets(id owner,NSArray<UIView *> *targets,NSArray<UIView *> *paints) {
    HGAPlayerChromeState *state=objc_getAssociatedObject(owner,&HGAPlayerChromeKey);
    for(UIView *previous in state.targets)if(![targets containsObject:previous])HGARemoveGlass(previous);
    for(UIView *previous in state.paints)if(![paints containsObject:previous])HGASetHidden(previous,NO);
    if(!targets.count && !paints.count){objc_setAssociatedObject(owner,&HGAPlayerChromeKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);return nil;}
    if(!state) {
        state=[HGAPlayerChromeState new];state.targets=[NSHashTable weakObjectsHashTable];state.paints=[NSHashTable weakObjectsHashTable];
        objc_setAssociatedObject(owner,&HGAPlayerChromeKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    [state.targets removeAllObjects];[state.paints removeAllObjects];
    for(UIView *target in targets)[state.targets addObject:target];
    for(UIView *paint in paints){[state.paints addObject:paint];HGASetHidden(paint,YES);}
    return state;
}
static void HGAApplyPlayerControls(UIView *root) {
    BOOL sheet=HGAEpisodeControl(root);
    BOOL enabled=HGAppearanceEnabled() && HGAppearanceFlag(sheet?@"panels.episodes":@"player.controlsGlass") && root.window;
    if(!enabled) {
        HGAPlayerChromeTargets(root,nil,nil);HGAApplyEpisodeCollectInk(root,NO);return;
    }
    NSMutableArray *targets=[NSMutableArray array],*paints=[NSMutableArray array];
    for(Class cls=object_getClass(root);cls && cls!=UIView.class;cls=class_getSuperclass(cls)) {
        for(NSString *getter in HGAPlayerControlGetters()[NSStringFromClass(cls)]) {
            UIView *target=HGAViewGetter(root,getter.UTF8String,root);
            if(target)[targets addObject:target];
        }
    }
    if([NSStringFromClass(root.class) isEqual:@"FQCShortVideoDanmakuEntranceView"]) {
        UIView *target=HGAViewGetter(root,"capsuleView",root) ?: HGAViewGetter(root,"clickView",root);
        if(target)[targets addObject:target];
    }
    if([NSStringFromClass(root.class) isEqual:@"SSShortVideoDanmakuLeftAboveView"]) {
        UIView *entrance=HGAViewGetter(root,"danmakuEntranceView",root);
        if(entrance)HGAApplyPlayerControls(entrance);
        else {UIView *target=HGAViewGetter(root,"clickView",root);if(target)[targets addObject:target];}
    }
    // The collect button's orange paint is a separate gradient view, not its
    // tappable container. Hide only that named paint carrier, retaining the
    // foreground and the host's selected-state label/icon updates.
    if([NSStringFromClass(root.class) isEqual:@"SSShortVideoEpisodePanelCollectView"]) {
        HGAApplyEpisodeCollectInk(root,enabled);
        for(NSString *getter in @[@"bgView",@"shareBgView"]) {
            UIView *paint=HGAViewGetter(root,getter.UTF8String,root);
            if(paint)[paints addObject:paint];
        }
    }
    HGAPlayerChromeTargets(root,targets,paints);
    for(UIView *target in targets)HGAPlayerControlGlass(target,YES,sheet);
    if([NSStringFromClass(root.class) isEqual:@"SSShortVideoEpisodesViewItemCell"]) {
        // The host's custom setIsSelected: paints bgView independently of
        // UICollectionViewCell.selected. Preserve that authored state as tint.
        for(UIView *target in targets)HGASetGlassOverlay(target,
            [HGAState(target,NO).background colorWithAlphaComponent:0.18] ?: UIColor.clearColor);
    }
}

static NSHashTable *HGAPlayerNavigations;
static void HGAApplyPlayerNavigation(id component) {
    if(!NSThread.isMainThread || HGAApplying)return;
    HGAApplying=YES;
    @try {
        if(!HGAPlayerNavigations)HGAPlayerNavigations=[NSHashTable weakObjectsHashTable];
        [HGAPlayerNavigations addObject:component];
        if(!HGAppearanceEnabled() || !HGAppearanceFlag(@"player.controlsGlass")) {
            HGAPlayerChromeTargets(component,nil,nil);return;
        }
        id candidate=HGReadObjectGetter(component,"navBar");
        if(![candidate isKindOfClass:UIView.class]){HGAPlayerChromeTargets(component,nil,nil);return;}
        UIView *bar=candidate;
        if(!bar.window){HGAPlayerChromeTargets(component,nil,nil);return;}
        NSMutableArray *targets=[NSMutableArray array];
        UIView *back=HGAViewGetter(component,"backBtn",bar);
        UIView *label=HGAViewGetter(component,"navLeftLabel",bar);
        CGRect frame=back && !back.hidden ? [back convertRect:back.bounds toView:bar] : CGRectNull;
        if(label && !label.hidden && !CGRectIsNull(frame))frame=CGRectUnion(frame,[label convertRect:label.bounds toView:bar]);
        if(!CGRectIsNull(frame)) {
            HGAInstallPlaybackGlass(bar,HGAppearanceString(@"player.material"),CGRectInset(frame,-6,0),22,nil);
            [targets addObject:bar];
        }
        for(NSString *getter in @[@"moreBtn",@"playRateView"]) {
            UIView *target=HGAViewGetter(component,getter.UTF8String,bar);
            if(target){HGAPlayerControlGlass(target,YES,NO);[targets addObject:target];}
        }
        HGAPlayerChromeTargets(component,targets,nil);
    } @finally {HGAApplying=NO;}
}

// The episode sheet owns a bounded content root in the exported hierarchy.
// Do not decorate its full-screen modal dismissal window.
static void HGAApplyEpisodeController(UIViewController *controller) {
    UIView *root=controller.view;
    BOOL enabled=HGAppearanceEnabled() && HGAppearanceFlag(@"panels.episodes") && root.window;
    CGRect rect=root.window?[root convertRect:root.bounds toView:root.window]:CGRectZero;
    if(rect.size.height>root.window.bounds.size.height*0.94)enabled=NO;
    HGAApplyPanel(root,enabled?@"episodes":@"episodes-disabled");
    // Concrete surfaces receive their own layout hooks, including reused cells.
    for(UIView *child in HGADescendants(root,420))if(HGAEpisodeControl(child))HGAApplyPlayerControls(child);
}
