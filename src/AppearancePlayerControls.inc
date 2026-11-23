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
        @"SSShortVideoTagListBottomInsertView":@[@"episodeInfoView"],
        @"FQVShortVideoControlCleanSwitchView":@[],
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
static char HGAEpisodeLayerPaintKey;
static NSString *HGAPlayerDetailEntryMaterial(void) {
    return [HGAppearanceString(@"player.material") isEqual:@"clear"] ? @"clear" : @"regular";
}
static void HGARestorePlayerControlGlass(UIView *target) {
    HGARemoveGlass(target);
    UIColor *paint=objc_getAssociatedObject(target,&HGAEpisodeLayerPaintKey);
    if(!paint)return;
    CGColorRef current=target.layer.backgroundColor;
    if(!current || CGColorGetAlpha(current)==0)target.layer.backgroundColor=paint.CGColor;
    objc_setAssociatedObject(target,&HGAEpisodeLayerPaintKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static void HGAPlayerControlGlass(UIView *target,BOOL sheet,BOOL episodeEntry) {
    if(!target.window || CGRectIsEmpty(target.bounds)){HGARestorePlayerControlGlass(target);return;}
    CGFloat radius=MIN(22,MIN(target.bounds.size.width,target.bounds.size.height)/2);
    if(episodeEntry) {
        // The tag-list episode pill paints its color directly onto CALayer.
        // Preserve that paint separately if UIView doesn't report the color.
        CGColorRef paint=target.layer.backgroundColor;
        UIColor *background=target.backgroundColor;
        if(paint && CGColorGetAlpha(paint)>0 && (!background || !CGColorEqualToColor(paint,background.CGColor)))
            objc_setAssociatedObject(target,&HGAEpisodeLayerPaintKey,[UIColor colorWithCGColor:paint],OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    if(sheet) HGAInstallGlass(target,HGAppearanceString(@"panels.material"),radius,0.04);
    else HGAInstallPlaybackGlass(target,episodeEntry ? HGAPlayerDetailEntryMaterial() : HGAppearanceString(@"player.material"),
        target.bounds,radius,nil);
    if(episodeEntry && target.layer.backgroundColor && CGColorGetAlpha(target.layer.backgroundColor)>0)
        target.layer.backgroundColor=nil;
}

@interface HGADanmakuLegacyInk : NSObject
@property(nonatomic,weak) UIImageView *view;
@property(nonatomic,strong) UIImage *original;
@property(nonatomic,strong) UIImage *foreground;
@end
@implementation HGADanmakuLegacyInk @end
static char HGADanmakuLegacyInkKey;

static UIImage *HGADanmakuForegroundImage(UIImage *image) {
    CGImageRef source=image.CGImage;
    if(!source)return nil;
    size_t width=CGImageGetWidth(source),height=CGImageGetHeight(source);
    if(!width || !height || width>256 || height>256)return nil;
    CGColorSpaceRef space=CGColorSpaceCreateDeviceRGB();
    CGContextRef context=CGBitmapContextCreate(NULL,width,height,8,width*4,space,
        kCGBitmapByteOrder32Big|kCGImageAlphaPremultipliedLast);
    CGColorSpaceRelease(space);
    if(!context)return nil;
    CGContextDrawImage(context,CGRectMake(0,0,width,height),source);
    UInt8 *pixels=CGBitmapContextGetData(context);
    // The host's compact gray-and-alpha artwork contains a dark plate baked
    // into its pixels. Its white mark is the foreground of our new glass.
    for(size_t i=0;i<width*height;i++) {
        UInt8 *pixel=pixels+4*i;
        unsigned value=MAX(pixel[0],MAX(pixel[1],pixel[2]));
        UInt8 alpha=value>48 ? (UInt8)((value-48)*255/207) : 0;
        pixel[0]=pixel[1]=pixel[2]=pixel[3]=alpha;
    }
    CGImageRef foreground=CGBitmapContextCreateImage(context);
    UIImage *result=foreground ? [[UIImage imageWithCGImage:foreground scale:image.scale orientation:image.imageOrientation]
        imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal] : nil;
    if(foreground)CGImageRelease(foreground);
    CGContextRelease(context);
    return result;
}

static void HGAApplyDanmakuLegacyInk(UIView *root,BOOL enabled) {
    HGADanmakuLegacyInk *state=objc_getAssociatedObject(root,&HGADanmakuLegacyInkKey);
    UIImageView *icon=(id)(enabled ? HGAViewGetter(root,"iconView",root) : nil);
    UIImage *legacy=enabled ? HGReadObjectGetter(root,"legacyImage") : nil;
    if(![legacy isKindOfClass:UIImage.class] || ![icon isKindOfClass:UIImageView.class])icon=nil;
    if(state && (state.view!=icon || ![state.original isEqual:legacy])) {
        if(state.view.image==state.foreground)state.view.image=state.original;
        objc_setAssociatedObject(root,&HGADanmakuLegacyInkKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        state=nil;
    }
    if(!icon || ![icon.image isEqual:legacy])return;
    if(!state) {
        UIImage *foreground=HGADanmakuForegroundImage(legacy);
        if(!foreground)return;
        state=[HGADanmakuLegacyInk new];
        state.view=icon;state.original=icon.image;state.foreground=foreground;
        objc_setAssociatedObject(root,&HGADanmakuLegacyInkKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    icon.image=state.foreground;
}
@interface HGAPlayerChromeState : NSObject
@property(nonatomic,strong) NSHashTable<UIView *> *targets;
@property(nonatomic,strong) NSHashTable<UIView *> *paints;
@end
@implementation HGAPlayerChromeState @end
static char HGAPlayerChromeKey;
static HGAPlayerChromeState *HGAPlayerChromeTargets(id owner,NSArray<UIView *> *targets,NSArray<UIView *> *paints) {
    HGAPlayerChromeState *state=objc_getAssociatedObject(owner,&HGAPlayerChromeKey);
    for(UIView *previous in state.targets)if(![targets containsObject:previous])HGARestorePlayerControlGlass(previous);
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
    NSString *name=NSStringFromClass(root.class);
    BOOL danmaku=[name isEqual:@"FQCShortVideoDanmakuEntranceView"];
    BOOL sheet=HGAEpisodeControl(root);
    BOOL enabled=HGAppearanceEnabled() && HGAppearanceFlag(sheet?@"panels.episodes":@"player.controlsGlass") && root.window;
    if([root isKindOfClass:NSClassFromString(@"FQVShortVideoControlCleanSwitchView")]) {
        HGARegisterView(root);
        if(!enabled || root.hidden || CGRectIsEmpty(root.bounds)){HGARemoveGlass(root);return;}
        CGRect frame=root.bounds;
        CGFloat side=MIN(frame.size.width,frame.size.height);
        frame=CGRectMake(CGRectGetMidX(frame)-side/2,CGRectGetMidY(frame)-side/2,side,side);
        HGAInstallPlaybackGlass(root,HGAPlayerDetailEntryMaterial(),frame,side/2,nil);
        return;
    }
    if([root isKindOfClass:NSClassFromString(@"SSShortVideoBottomInsertView")]) {
        UIView *cleanSwitch=HGAViewGetter(root,"cleanSwitchView",root);
        if(cleanSwitch && cleanSwitch!=root)HGAApplyPlayerControls(cleanSwitch);
    }
    if(!enabled) {
        HGAPlayerChromeTargets(root,nil,nil);HGAApplyEpisodeCollectInk(root,NO);
        if(danmaku)HGAApplyDanmakuLegacyInk(root,NO);
        return;
    }
    NSMutableArray *targets=[NSMutableArray array],*paints=[NSMutableArray array];
    UIView *episodeEntry=nil;
    for(Class cls=object_getClass(root);cls && cls!=UIView.class;cls=class_getSuperclass(cls)) {
        for(NSString *getter in HGAPlayerControlGetters()[NSStringFromClass(cls)]) {
            UIView *target=HGAViewGetter(root,getter.UTF8String,root);
            if(!target)continue;
            if([getter isEqual:@"episodeInfoView"])episodeEntry=target;
            [targets addObject:target];
        }
    }
    if(danmaku) {
        UIView *capsule=HGAViewGetter(root,"capsuleView",root);
        // updateStyleVisibility keeps capsuleView mounted and hides it in
        // compact mode. clickView returns the actual native toggle button.
        UIView *target=HGAVisible(capsule) ? capsule : HGAViewGetter(root,"clickView",root);
        BOOL visible=HGAVisible(target);
        if(visible)[targets addObject:target];
        HGAApplyDanmakuLegacyInk(root,visible && target!=capsule);
    }
    if([name isEqual:@"SSShortVideoDanmakuLeftAboveView"]) {
        UIView *entrance=HGAViewGetter(root,"danmakuEntranceView",root);
        if(entrance)HGAApplyPlayerControls(entrance);
        else {UIView *target=HGAViewGetter(root,"clickView",root);if(target)[targets addObject:target];}
    }
    // The collect button's orange paint is a separate gradient view, not its
    // tappable container. Hide only that named paint carrier, retaining the
    // foreground and the host's selected-state label/icon updates.
    if([name isEqual:@"SSShortVideoEpisodePanelCollectView"]) {
        HGAApplyEpisodeCollectInk(root,enabled);
        for(NSString *getter in @[@"bgView",@"shareBgView"]) {
            UIView *paint=HGAViewGetter(root,getter.UTF8String,root);
            if(paint)[paints addObject:paint];
        }
    }
    HGAPlayerChromeTargets(root,targets,paints);
    for(UIView *target in targets)HGAPlayerControlGlass(target,sheet,target==episodeEntry);
    if([name isEqual:@"SSShortVideoEpisodesViewItemCell"]) {
        // The host's custom setIsSelected: paints bgView independently of
        // UICollectionViewCell.selected. Preserve that authored state as tint.
        for(UIView *target in targets)HGASetGlassOverlay(target,
            [HGAState(target,NO).background colorWithAlphaComponent:0.18] ?: UIColor.clearColor);
    }
}

static NSHashTable *HGAPlayerNavigations;
static CGRect HGAPlayerNavigationContent(UIView *view,UIView *bar) {
    if(!HGAVisible(view))return CGRectNull;
    if([view isKindOfClass:UIButton.class]) {
        UIImageView *image=((UIButton *)view).imageView;
        if(image.image && HGAVisible(image))view=image;
    }
    CGRect bounds=view.bounds;
    if([view isKindOfClass:UILabel.class]) {
        UILabel *label=(UILabel *)view;
        if(!label.text.length)return CGRectNull;
        bounds=[label textRectForBounds:bounds limitedToNumberOfLines:label.numberOfLines];
    }
    return CGRectIsEmpty(bounds) ? CGRectNull : [view convertRect:bounds toView:bar];
}

static UIView *HGAPlayerDetailNavigationTitle(UIView *bar) {
    // Detail playback can put the episode label on the enclosing controller's
    // own navigation view. The player component owns the adjacent back arrow.
    Class detail=NSClassFromString(@"SSShortVideoDetailViewController");
    Class album=NSClassFromString(@"SSShortVideoAlbumDetailViewController");
    for(UIViewController *owner=HGAOwner(bar);owner;owner=owner.parentViewController) {
        if([owner isKindOfClass:detail] || [owner isKindOfClass:album]) {
            UIView *label=HGAViewGetter(owner,"navLeftLabel",owner.view);
            return label.window==bar.window ? label : nil;
        }
    }
    return nil;
}

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
        [bar layoutIfNeeded];
        NSMutableArray *targets=[NSMutableArray array];
        UIView *back=HGAViewGetter(component,"backBtn",bar);
        CGRect backInk=HGAPlayerNavigationContent(back,bar);
        CGRect title=HGAPlayerNavigationContent(HGAViewGetter(component,"navLeftLabel",bar),bar);
        if(CGRectIsNull(title))title=HGAPlayerNavigationContent(HGAPlayerDetailNavigationTitle(bar),bar);
        CGRect left=CGRectIsNull(backInk) ? CGRectNull : CGRectUnion(backInk,title);
        UIView *rate=HGAViewGetter(component,"playRateView",bar);
        CGRect rateInk=rate ? CGRectUnion(HGAPlayerNavigationContent(HGAViewGetter(component,"playRateImgView",rate),bar),
            HGAPlayerNavigationContent(HGAViewGetter(component,"playRateLabel",rate),bar)) : CGRectNull;
        if(CGRectIsNull(rateInk))rateInk=HGAPlayerNavigationContent(rate,bar);
        UIView *more=HGAViewGetter(component,"moreBtn",bar);
        CGRect moreInk=HGAPlayerNavigationContent(more,bar);
        CGRect anchor=!CGRectIsNull(backInk) ? [back convertRect:back.bounds toView:bar] :
            (!CGRectIsNull(rateInk) ? rateInk : moreInk);
        if(CGRectIsNull(anchor)){HGAPlayerChromeTargets(component,nil,nil);return;}
        CGFloat height=34,centerY=CGRectGetMidY(anchor);
        UIView *controls[]={bar,rate,more};
        CGRect content[]={left,rateInk,moreInk};
        NSString *style=HGAppearanceString(@"player.material");
        for(NSUInteger i=0;i<3;i++) {
            UIView *target=controls[i];
            if(CGRectIsNull(content[i]))continue;
            CGFloat width=MAX(height,CGRectGetWidth(content[i])+14);
            CGRect pill=CGRectMake(CGRectGetMidX(content[i])-width/2,centerY-height/2,width,height);
            HGAInstallPlaybackGlass(target,style,[bar convertRect:pill toView:target],height/2,nil);
            [targets addObject:target];
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
