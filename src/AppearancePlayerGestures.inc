// The live 7.4.5.33 feed mounts one comment action underneath each player's
// info view. Stay inside the touched player when neighbouring cells are loaded.
static UIView *HGAPlayerCommentAction(UIView *view,Class commentClass) {
    if(view.hidden || view.alpha<0.01)return nil;
    if([view isKindOfClass:commentClass]) {
        Method click=class_getInstanceMethod(object_getClass(view),sel_registerName("commentViewDidClicked"));
        if(click && !strcmp(method_getTypeEncoding(click),"v16@0:8") &&
            [HGReadObjectGetter(view,"delegate") respondsToSelector:sel_registerName("commentViewDidClick:")])return view;
        return nil;
    }
    for(UIView *child in view.subviews) {
        UIView *action=HGAPlayerCommentAction(child,commentClass);
        if(action)return action;
    }
    return nil;
}

static BOOL HGAPlayerIsSeries(UIView *player) {
    // The unqualified inner-series HUD is also a base of PUGC profile
    // controls. Match that concrete class; ask other subclasses for the model.
    if(player.class==objc_lookUpClass("FQVShortVideoVPlayerMultiSeriesControlView") ||
       [player isKindOfClass:objc_lookUpClass("FQVShortVideoVPlayerOutterMultiSeriesControlView")] ||
       [player isKindOfClass:objc_lookUpClass("FQVShortVideoVPlayerSingleSeriesControlView")] ||
       [player isKindOfClass:objc_lookUpClass("FQVShortVideoDerivativeVideoControlView")])return YES;
    id model=HGReadObjectGetter(player,"videoDataModel");
    BOOL series=NO;
    return (HGABoolGetter(model,@"isSeriesType",&series) && series) ||
        (HGABoolGetter(model,@"isEpisodeModel",&series) && series);
}

// The host recycles the gesture HUD on vertical paging. Own this mode on
// its list page, and suppress only playback chrome mounted on that page.
@interface HGACleanTarget : NSObject
@property(nonatomic) BOOL interaction;
@property(nonatomic,strong) CALayer *nativeMask;
@property(nonatomic,strong) CALayer *cleanMask;
@end
@implementation HGACleanTarget @end
@interface HGACleanPlayback : NSObject
@property(nonatomic,weak) UIView *page;
@property(nonatomic,strong) NSMapTable<UIView *,HGACleanTarget *> *targets;
@end
@implementation HGACleanPlayback @end
static NSMapTable<UIWindow *,HGACleanPlayback *> *HGACleanPages;
static NSHashTable *HGAPlayerNavigations;
static UIView *HGAPlayerDetailNavigationTitle(UIView *bar);
static NSString * const HGACleanOpacity = @"honggo.clean.opacity";

static void HGACleanCover(HGACleanPlayback *state,UIView *target) {
    if(!state || !target)return;
    if(![state.targets objectForKey:target]) {
        HGACleanTarget *record=[HGACleanTarget new];
        record.interaction=target.userInteractionEnabled;
        [state.targets setObject:record forKey:target];
    }
    if(![target.layer animationForKey:HGACleanOpacity]) {
        // Hold the presentation at zero. Native alpha/hidden changes (including
        // updates made during cell reuse) remain intact when the cover is lifted.
        CABasicAnimation *cover=[CABasicAnimation animationWithKeyPath:@"opacity"];
        cover.fromValue=@0;cover.toValue=@0;cover.duration=0.001;
        cover.fillMode=kCAFillModeBoth;cover.removedOnCompletion=NO;
        [target.layer addAnimation:cover forKey:HGACleanOpacity];
    }
    if(target.userInteractionEnabled)target.userInteractionEnabled=NO;
}
// The host repaints the title column and progress rail during paging, including
// opacity animations. A transparent mask on these two ink surfaces stays in
// effect while the host changes its own visibility or clears layer animations.
static void HGACleanCoverInk(HGACleanPlayback *state,UIView *ink) {
    if(!ink)return;
    HGACleanCover(state,ink);
    HGACleanTarget *record=[state.targets objectForKey:ink];
    if(!record.cleanMask) {
        record.cleanMask=[CALayer layer];
        record.cleanMask.backgroundColor=UIColor.blackColor.CGColor;
        record.cleanMask.opacity=0;
    }
    if(ink.layer.mask!=record.cleanMask || !CGRectEqualToRect(record.cleanMask.frame,ink.bounds)) {
        [CATransaction begin];[CATransaction setDisableActions:YES];
        record.cleanMask.frame=ink.bounds;
        if(ink.layer.mask!=record.cleanMask) {
            record.nativeMask=ink.layer.mask;
            ink.layer.mask=record.cleanMask;
        }
        [CATransaction commit];
    }
}
static void HGACleanRestore(HGACleanPlayback *state,UIView *target) {
    HGACleanTarget *record=[state.targets objectForKey:target];
    if(!record)return;
    if(record.cleanMask && target.layer.mask==record.cleanMask) {
        [CATransaction begin];[CATransaction setDisableActions:YES];
        target.layer.mask=record.nativeMask;
        [CATransaction commit];
    }
    [target.layer removeAnimationForKey:HGACleanOpacity];
    target.userInteractionEnabled=record.interaction;
    [state.targets removeObjectForKey:target];
}
static void HGAEndCleanPlayback(UIWindow *window) {
    HGACleanPlayback *state=[HGACleanPages objectForKey:window];
    for(UIView *target in state.targets.keyEnumerator.allObjects)HGACleanRestore(state,target);
    if(window)[HGACleanPages removeObjectForKey:window];
}
static NSArray<NSString *> *HGACleanControlClasses(void) {
    static NSArray *names;
    static dispatch_once_t once;
    dispatch_once(&once,^{ names=@[@"SSShortVideoRightInteractionView",@"SSShortVideoAboveLeftContainerView",
        @"SSShortVideoSingleSeriesInfoLeftContainerView",@"FQCShortVideoDanmakuContainerView",
        @"FQVShortVideoGradientLayerView",@"FQVShortVideoProgressFloatView"]; });
    return names;
}
static void HGAApplyCleanView(UIView *view) {
    if(!HGACleanPages.count)return;
    HGACleanPlayback *state=view.window ? [HGACleanPages objectForKey:view.window] : nil;
    BOOL inside=state.page && [view isDescendantOfView:state.page];
    BOOL bar=[view isKindOfClass:objc_lookUpClass("SSTabBar")] ||
        [view isKindOfClass:objc_lookUpClass("SSVideoSeriesFeedHeaderView")];
    UIView *barPage=bar ? HGAOwner(view).viewIfLoaded : nil;
    BOOL shared=barPage && state.page && [state.page isDescendantOfView:barPage];
    if(!inside && !shared) {
        for(HGACleanPlayback *old in HGACleanPages.objectEnumerator)HGACleanRestore(old,view);
        return;
    }
    if([view isKindOfClass:objc_lookUpClass("FQVShortVideoVPlayerControlView")]) {
        // Inspect installed controls; several host getters create views on read.
        const char *slots[]={"_fullscreenBtn","_playBtn","_cancelCleanScreenView","_cleanScreenModeRightInteractionView"};
        for(NSUInteger i=0;i<sizeof(slots)/sizeof(slots[0]);i++) {
            Ivar ivar=class_getInstanceVariable(object_getClass(view),slots[i]);
            id child=ivar && ivar_getTypeEncoding(ivar)[0]=='@' ? object_getIvar(view,ivar) : nil;
            if([child isKindOfClass:UIView.class] && [child isDescendantOfView:view])HGACleanCover(state,child);
        }
        for(UIView *child in view.subviews) {
            if([child isKindOfClass:objc_lookUpClass("FQVShortVideoProgressView")])
                HGACleanCoverInk(state,child);
        }
        UIView *info=HGAViewGetter(view,"baseInfoView",view);
        HGACleanCoverInk(state,HGAViewGetter(info,"leftContainer",info));
        return;
    }
    if([view isKindOfClass:objc_lookUpClass("FQVShortVideoBaseInfoView")])
        HGACleanCoverInk(state,HGAViewGetter(view,"leftContainer",view));
    NSString *role=HGARole(view);
    // The full-screen "info" owner also contains loading / payment surfaces.
    // Its metadata column and action rail are independently addressable.
    BOOL metadata=role && (![role isEqual:@"info"] || [NSStringFromClass(view.class) hasSuffix:@"LeftContainerView"]);
    BOOL titleColumn=([role isEqual:@"info"] && [NSStringFromClass(view.class) hasSuffix:@"LeftContainerView"]) ||
        [view isKindOfClass:objc_lookUpClass("SSShortVideoSingleSeriesInfoLeftContainerView")];
    if([role isEqual:@"progress"] || titleColumn)
        HGACleanCoverInk(state,view);
    else if(bar || metadata || [HGACleanControlClasses() containsObject:NSStringFromClass(view.class)] ||
       [view isKindOfClass:objc_lookUpClass("FQVShortVideoPlayerBottomView")] ||
       [view isKindOfClass:objc_lookUpClass("FQVShortVideoControlCleanSwitchView")] ||
       [view isKindOfClass:objc_lookUpClass("FQVShortVideoVPlayerEpisodeInfoView")])
        HGACleanCover(state,view);
}
static void HGAApplyCleanNavigation(id component) {
    if(!HGACleanPages.count)return;
    UIView *bar=HGReadObjectGetter(component,"navBar");
    if(![bar isKindOfClass:UIView.class] || !bar.window)return;
    HGACleanPlayback *state=[HGACleanPages objectForKey:bar.window];
    if(state.page && [bar isDescendantOfView:state.page]) {
        HGACleanCover(state,bar);
        HGACleanCover(state,HGAPlayerDetailNavigationTitle(bar));
    }
}
static void HGABeginCleanPlayback(UIView *player) {
    UIWindow *window=player.window;
    if(!window)return;
    UIViewController *owner=HGAOwner(player);
    for(UIViewController *vc=owner;vc;vc=vc.parentViewController)
        if([vc isKindOfClass:objc_lookUpClass("FQVShortVideoListViewController")]){owner=vc;break;}
    UIView *page=owner.viewIfLoaded;
    if(!page)return;
    if(!HGACleanPages)HGACleanPages=[NSMapTable weakToStrongObjectsMapTable];
    HGAEndCleanPlayback(window);
    HGACleanPlayback *state=[HGACleanPlayback new];
    state.page=page;state.targets=[NSMapTable weakToStrongObjectsMapTable];
    [HGACleanPages setObject:state forKey:window];
    for(UIView *node in HGAManaged.allObjects)if(node.window==window)HGAApplyCleanView(node);
    for(UIView *node in HGADescendants(player,300))HGAApplyCleanView(node);
    for(id component in HGAPlayerNavigations.allObjects)HGAApplyCleanNavigation(component);
}

static BOOL HGAPlayerGestureSurface(UIView *player,UIView *hit) {
    if(![hit isDescendantOfView:player] || !HGAVisible(player) ||
       HGFeedSidebarOwnsLayout(player.window) || HGAWindowHasActionSheet(player.window))return NO;
    for(UIViewController *vc=HGAOwner(player);vc;vc=vc.parentViewController)
        if(vc.presentedViewController)return NO;
    for(UIView *node=hit;node && node!=player;node=node.superview) {
        if([node isKindOfClass:UIControl.class] || [node isKindOfClass:UIScrollView.class] ||
            [node isKindOfClass:objc_lookUpClass("SSShortVideoRightInteractionView")] ||
            HGAIsCommentSurface(node) || HGAIsActionPanelSurface(node))return NO;
        NSString *role=HGARole(node);
        if(role && (![role isEqual:@"info"] || [NSStringFromClass(node.class) hasSuffix:@"LeftContainerView"]))return NO;
    }
    return YES;
}

@interface HGAPlayerGestures : NSObject <UIGestureRecognizerDelegate>
@property(nonatomic,weak) UIView *player;
@property(nonatomic,strong) UIPinchGestureRecognizer *pinch;
@property(nonatomic,strong) UITapGestureRecognizer *doubleTap;
@property(nonatomic) BOOL changedCleanMode;
@end
@implementation HGAPlayerGestures
- (UIView *)commentAction {
    Class cls=objc_lookUpClass("SSShortVideoRightInteractionCommentView");
    return cls ? HGAPlayerCommentAction(self.player,cls) : nil;
}
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gesture shouldReceiveTouch:(UITouch *)touch {
    UIView *page=[HGACleanPages objectForKey:self.player.window].page;
    BOOL canRestore=gesture==self.pinch && page && [self.player isDescendantOfView:page];
    return (canRestore || HGAPlayerIsSeries(self.player)) && HGAPlayerGestureSurface(self.player,touch.view);
}
- (BOOL)gestureRecognizerShouldBegin:(UIGestureRecognizer *)gesture {
    if(gesture==self.doubleTap) {
        UIView *page=[HGACleanPages objectForKey:self.player.window].page;
        return HGAppearanceFlag(@"gestures.doubleTapComments") && !(page && [self.player isDescendantOfView:page]) &&
            [self commentAction];
    }
    return HGAppearanceFlag(@"gestures.pinchClean");
}
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gesture shouldBeRequiredToFailByGestureRecognizer:(UIGestureRecognizer *)other {
    // The native getters construct recognizers on access. Inspect the installed
    // ivars instead. Native single-tap playback still fires if the second tap never arrives.
    if(gesture==self.pinch) {
        if(![other isKindOfClass:UIPinchGestureRecognizer.class] || !other.view)return NO;
        if([other.view isDescendantOfView:self.player])return YES;
        for(UIView *node=self.player.superview;node;node=node.superview) {
            if([node isKindOfClass:objc_lookUpClass("FQVShortVideoPlayerCellView")])
                return [self.player isDescendantOfView:other.view] && [other.view isDescendantOfView:node];
        }
        return NO;
    }
    if(other.view!=self.player)return NO;
    const char *slots[2]={"_doubleTapGesture","_singleTapGesture"};
    for(NSUInteger i=0;i<2;i++) {
        Ivar ivar=class_getInstanceVariable(object_getClass(self.player),slots[i]);
        if(ivar && ivar_getTypeEncoding(ivar)[0]=='@' && object_getIvar(self.player,ivar)==other)return YES;
    }
    return NO;
}
- (void)pinched:(UIPinchGestureRecognizer *)gesture {
    if(gesture.state==UIGestureRecognizerStateBegan)self.changedCleanMode=NO;
    if(self.changedCleanMode || (gesture.state!=UIGestureRecognizerStateBegan &&
        gesture.state!=UIGestureRecognizerStateChanged && gesture.state!=UIGestureRecognizerStateEnded))return;
    UIView *page=[HGACleanPages objectForKey:self.player.window].page;
    BOOL clean=page && [self.player isDescendantOfView:page];
    if(!clean && gesture.scale>=1.15) {
        self.changedCleanMode=YES;
        HGABeginCleanPlayback(self.player);
    }else if(gesture.scale<=0.85) {
        // Native clean mode can also have been selected from the playback menu.
        BOOL nativeClean=NO;
        SEL change=sel_registerName("enterCleanMode:");
        Method method=class_getInstanceMethod(object_getClass(self.player),change);
        if(HGABoolGetter(self.player,@"isCleanMode",&nativeClean) && nativeClean && method &&
            !strcmp(method_getTypeEncoding(method),"B20@0:8B16")) {
            ((BOOL(*)(id,SEL,BOOL))objc_msgSend)(self.player,change,NO);
            self.changedCleanMode=YES;
        }
        if(clean) {
            HGAEndCleanPlayback(self.player.window);
            self.changedCleanMode=YES;
        }
    }
}
- (void)tapped:(UITapGestureRecognizer *)gesture {
    if(gesture.state!=UIGestureRecognizerStateEnded)return;
    UIView *action=[self commentAction];
    if(action)((void(*)(id,SEL))objc_msgSend)(action,sel_registerName("commentViewDidClicked"));
}
@end

static char HGAPlayerGesturesKey;
static void HGAUpdatePlayerGestures(UIView *player) {
    BOOL pinch=player.window && HGAppearanceFlag(@"gestures.pinchClean");
    BOOL tap=player.window && HGAppearanceFlag(@"gestures.doubleTapComments");
    HGAPlayerGestures *state=objc_getAssociatedObject(player,&HGAPlayerGesturesKey);
    if(!state) {
        if(!pinch && !tap)return;
        state=[HGAPlayerGestures new]; state.player=player;
        objc_setAssociatedObject(player,&HGAPlayerGesturesKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    if(pinch && !state.pinch) {
        state.pinch=[[UIPinchGestureRecognizer alloc] initWithTarget:state action:@selector(pinched:)];
        state.pinch.name=@"Honggo.PlayerCleanPinch"; state.pinch.delegate=state;
        [player addGestureRecognizer:state.pinch];
    }else if(!pinch && state.pinch){[player removeGestureRecognizer:state.pinch];state.pinch=nil;}
    if(tap && !state.doubleTap) {
        state.doubleTap=[[UITapGestureRecognizer alloc] initWithTarget:state action:@selector(tapped:)];
        state.doubleTap.numberOfTapsRequired=2;
        state.doubleTap.name=@"Honggo.PlayerCommentDoubleTap"; state.doubleTap.delegate=state;
        [player addGestureRecognizer:state.doubleTap];
    }else if(!tap && state.doubleTap){[player removeGestureRecognizer:state.doubleTap];state.doubleTap=nil;}
    if(!pinch && !tap)objc_setAssociatedObject(player,&HGAPlayerGesturesKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
