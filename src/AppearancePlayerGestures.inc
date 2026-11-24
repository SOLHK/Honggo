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
    if([player isKindOfClass:objc_lookUpClass("FQVShortVideoVPlayerOutterMultiSeriesControlView")] ||
       [player isKindOfClass:objc_lookUpClass("FQVShortVideoVPlayerSingleSeriesControlView")] ||
       [player isKindOfClass:objc_lookUpClass("FQVShortVideoDerivativeVideoControlView")])return YES;
    id model=HGReadObjectGetter(player,"videoDataModel");
    BOOL series=NO;
    return (HGABoolGetter(model,@"isSeriesType",&series) && series) ||
        (HGABoolGetter(model,@"isEpisodeModel",&series) && series);
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
    return HGAPlayerIsSeries(self.player) && HGAPlayerGestureSurface(self.player,touch.view);
}
- (BOOL)gestureRecognizerShouldBegin:(UIGestureRecognizer *)gesture {
    if(gesture==self.doubleTap)return HGAppearanceFlag(@"gestures.doubleTapComments") && [self commentAction];
    BOOL clean=NO;
    Method change=class_getInstanceMethod(object_getClass(self.player),sel_registerName("enterCleanMode:"));
    return HGAppearanceFlag(@"gestures.pinchClean") && change && !strcmp(method_getTypeEncoding(change),"B20@0:8B16") &&
        HGABoolGetter(self.player,@"isCleanMode",&clean);
}
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gesture shouldBeRequiredToFailByGestureRecognizer:(UIGestureRecognizer *)other {
    // The native getters construct recognizers on access. Inspect the installed
    // ivars instead. Native single-tap playback still fires if the second tap never arrives.
    if(other.view!=self.player)return NO;
    const char *slots[2]={gesture==self.doubleTap ? "_doubleTapGesture" : "_pinch","_singleTapGesture"};
    NSUInteger count=gesture==self.doubleTap ? 2 : 1;
    for(NSUInteger i=0;i<count;i++) {
        Ivar ivar=class_getInstanceVariable(object_getClass(self.player),slots[i]);
        if(ivar && ivar_getTypeEncoding(ivar)[0]=='@' && object_getIvar(self.player,ivar)==other)return YES;
    }
    return NO;
}
- (void)pinched:(UIPinchGestureRecognizer *)gesture {
    if(gesture.state==UIGestureRecognizerStateBegan)self.changedCleanMode=NO;
    if(self.changedCleanMode || (gesture.state!=UIGestureRecognizerStateBegan &&
        gesture.state!=UIGestureRecognizerStateChanged && gesture.state!=UIGestureRecognizerStateEnded))return;
    BOOL clean=NO;
    if(!HGABoolGetter(self.player,@"isCleanMode",&clean))return;
    if((!clean && gesture.scale>=1.15) || (clean && gesture.scale<=0.85)) {
        self.changedCleanMode=YES;
        ((BOOL(*)(id,SEL,BOOL))objc_msgSend)(self.player,sel_registerName("enterCleanMode:"),!clean);
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
