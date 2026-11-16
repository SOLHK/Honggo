// Landscape control surfaces confirmed in Eggplant 7.4.3.33. The native
// buttons, recognizers, title and scrub rail stay in their original views.
static NSDictionary<NSString *,NSArray<NSString *> *> *HGALandscapeTargetGetters(void) {
    static NSDictionary *getters;
    static dispatch_once_t once;
    dispatch_once(&once,^{
        getters=@{
            @"FQVShortVideoPlayerLandscapeBaseInfoView":@[@"moreBtn",@"nextBtn"],
            @"FQVShortVideoPlayerLandscapeBaseControlView":@[@"screenLockBtn"],
            @"FQVShortVideoLandscapeNavBar":@[],
            @"FQVLandscapeVolumeButton":@[],
            @"SSShortVideoLandscapeAutoInflowTipView":@[],
            @"FQVShortVideoLandscapeBottomRightView":@[@"playRateBtn",@"resolutionBtn",@"seriesBtn"],
            @"SSShortVideoLandscapeBizInteractionView":@[@"likeBtnView",@"commentBtnView",@"starBtnView",
                @"danmakuBtnView",@"danmakuInputView"]
        };
    });
    return getters;
}

static NSArray<NSString *> *HGALandscapeChromeClasses(void) {
    return HGALandscapeTargetGetters().allKeys;
}

@interface HGALandscapeSurface : NSObject
@property(nonatomic,strong) HGAAppearanceMaterial *glass;
@property(nonatomic,strong) UIColor *sourceBackground;
@property(nonatomic,weak) UIView *coveredPaint;
@property(nonatomic) BOOL clearsBackground;
@property(nonatomic) BOOL unclipped;
@property(nonatomic) BOOL clearedOpaque;
@end
@implementation HGALandscapeSurface
- (void)dealloc { [_glass removeFromSuperview]; }
@end
static char HGALandscapeSurfacesKey;

static void HGARestoreLandscapeSurface(UIView *target,HGALandscapeSurface *state) {
    if(state.clearsBackground && [target.backgroundColor isEqual:UIColor.clearColor])
        target.backgroundColor=state.sourceBackground;
    if(state.unclipped)target.clipsToBounds=YES;
    if(state.clearedOpaque)target.opaque=YES;
    HGASetHidden(state.coveredPaint,NO);
    [state.glass removeFromSuperview];
}

static void HGAPlaceLandscapeSurface(UIView *target,CGRect frame,BOOL inside,HGALandscapeSurface *state) {
    if(!state.glass) {
        state.glass=[[HGAAppearanceMaterial alloc] initWithEffect:nil];
        state.glass.accessibilityIdentifier=@"honggo.appearance.landscape.chrome";
        state.glass.accessibilityElementsHidden=YES;
        state.glass.userInteractionEnabled=NO;
        state.glass.clipsToBounds=YES;
        state.glass.layer.cornerCurve=kCACornerCurveContinuous;
        state.glass.overrideUserInterfaceStyle=UIUserInterfaceStyleDark;
        state.glass.opaque=NO;
        if(@available(iOS 26.0,*))state.glass.cornerConfiguration=UICornerConfiguration.capsuleConfiguration;
    }
    if(!state.clearsBackground || ![target.backgroundColor isEqual:UIColor.clearColor])
        state.sourceBackground=target.backgroundColor;
    state.clearsBackground=YES;
    if(target.opaque){state.clearedOpaque=YES;target.opaque=NO;}
    if(![target.backgroundColor isEqual:UIColor.clearColor])target.backgroundColor=UIColor.clearColor;
    HGAAppearanceMaterial *glass=state.glass;
    NSString *style=HGAppearanceString(@"player.material");
    if(![glass.style isEqual:style]) {
        glass.style=style;glass.materialSignature=nil;
    }
    if(!CGRectEqualToRect(glass.frame,frame))glass.frame=frame;
    CGFloat cornerRadius=MIN(frame.size.width,frame.size.height)/2;
    if(glass.layer.cornerRadius!=cornerRadius)glass.layer.cornerRadius=cornerRadius;
    BOOL opaque=[style isEqual:@"solid"] || UIAccessibilityIsReduceTransparencyEnabled();
    glass.overlayColor=[UIColor colorWithWhite:0.10 alpha:opaque ? 1 : 0.12];
    if(inside) {
        // Keep the glass with controls that have separate foreground views so
        // it follows their native hide and scale animations.
        if(target.clipsToBounds){state.unclipped=YES;target.clipsToBounds=NO;}
        if(glass.superview!=target)[target insertSubview:glass atIndex:0];
    } else if(glass.superview!=target.superview) {
        // A sibling sits behind UIImageView's own pixels as well as descendants.
        [target.superview insertSubview:glass belowSubview:target];
    }
    BOOL hidden=!inside && (target.hidden || target.alpha<0.01);
    if(glass.hidden!=hidden)glass.hidden=hidden;
    [glass requestMaterialRefresh];
}

static CGRect HGALandscapeSurfaceFrame(UIView *target,UIView *labelView,CGFloat horizontalPadding,BOOL inside) {
    UIView *parent=inside ? target : target.superview;
    CGRect frame=inside ? target.bounds : [target convertRect:target.bounds toView:parent];
    if(labelView && !labelView.hidden && !CGRectIsEmpty(labelView.bounds)) {
        CGRect text=[labelView convertRect:labelView.bounds toView:parent];
        if([labelView isKindOfClass:UILabel.class]) {
            UILabel *label=(UILabel *)labelView;
            CGRect used=[label textRectForBounds:label.bounds limitedToNumberOfLines:label.numberOfLines];
            text=[label convertRect:used toView:parent];
        }
        frame=CGRectInset(CGRectUnion(frame,text),-horizontalPadding,0);
    }
    CGFloat width=MAX(40,CGRectGetWidth(frame));
    CGRect padded=CGRectMake(CGRectGetMidX(frame)-width/2,CGRectGetMidY(frame)-20,width,40);
    return !inside && parent.clipsToBounds ? CGRectIntersection(padded,parent.bounds) : padded;
}

static void HGALandscapeApplyView(UIView *root) {
    NSString *kind=nil;
    NSDictionary<NSString *,NSArray<NSString *> *> *getters=HGALandscapeTargetGetters();
    for(Class cls=object_getClass(root);cls && cls!=UIView.class;cls=class_getSuperclass(cls)) {
        NSString *name=NSStringFromClass(cls);
        if(getters[name]) {kind=name;break;}
    }
    if(!kind)return;
    NSMapTable<UIView *,HGALandscapeSurface *> *states=objc_getAssociatedObject(root,&HGALandscapeSurfacesKey);
    // Native landscape builds its HUD before revealing it through alpha-only
    // animations. Its original controls and groups own visibility.
    BOOL enabled=HGAppearanceEnabled() && HGAppearanceFlag(@"player.controlsGlass") && root.window;
    if(!enabled) {
        for(UIView *target in states)HGARestoreLandscapeSurface(target,[states objectForKey:target]);
        if(states)objc_setAssociatedObject(root,&HGALandscapeSurfacesKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        return;
    }

    HGARegisterView(root);
    BOOL control=[kind isEqual:@"FQVShortVideoPlayerLandscapeBaseControlView"];
    BOOL info=[kind isEqual:@"FQVShortVideoPlayerLandscapeBaseInfoView"];
    BOOL biz=[kind isEqual:@"SSShortVideoLandscapeBizInteractionView"];
    BOOL volume=[kind isEqual:@"FQVLandscapeVolumeButton"];
    BOOL inflow=[kind isEqual:@"SSShortVideoLandscapeAutoInflowTipView"];
    if(control) {
        HGALandscapeApplyView(HGAViewGetter(root,"infoView",root));
        Class volumeClass=NSClassFromString(@"FQVLandscapeVolumeButton");
        Class overlayClass=NSClassFromString(@"FQVTouchIgoringView");
        Class inflowClass=NSClassFromString(@"SSShortVideoLandscapeAutoInflowTipView");
        for(UIView *child in root.subviews) {
            if([child isKindOfClass:volumeClass])HGALandscapeApplyView(child);
            if([child isKindOfClass:overlayClass]) {
                for(UIView *entry in child.subviews)
                    if([entry isKindOfClass:inflowClass])HGALandscapeApplyView(entry);
            }
        }
    } else if(info) {
        HGALandscapeApplyView(HGAViewGetter(root,"navBar",root));
        HGALandscapeApplyView(HGAViewGetter(root,"bottomLeftView",root));
        HGALandscapeApplyView(HGAViewGetter(root,"bottomRightView",root));
    }

    NSMutableArray<UIView *> *targets=[NSMutableArray array];
    NSMapTable<UIView *,UIView *> *countLabels=biz ? [NSMapTable strongToStrongObjectsMapTable] : nil;
    NSDictionary<NSString *,NSString *> *labelGetters=biz ? @{@"likeBtnView":@"likeCountLabel",
        @"commentBtnView":@"commentCountLabel",@"starBtnView":@"starCountLabel"} : nil;
    UIView *back=nil,*title=nil;
    UIView *input=nil,*inputPaint=nil;
    for(NSString *getter in getters[kind]) {
        UIView *target=HGAViewGetter(root,getter.UTF8String,root);
        if(!target || CGRectIsEmpty(target.bounds))continue;
        [targets addObject:target];
        NSString *labelGetter=labelGetters[getter];
        if(labelGetter) {
            UIView *label=HGAViewGetter(root,labelGetter.UTF8String,target);
            if(label)[countLabels setObject:label forKey:target];
        } else if(biz && [getter isEqual:@"danmakuInputView"]) {
            input=target;
            inputPaint=HGAViewGetter(root,"danmakuInputBGView",input);
        }
    }
    if(info) {
        back=HGAViewGetter(root,"navBackImageView",root);
        title=HGAViewGetter(root,"titleL",root);
        if(back && !CGRectIsEmpty(back.bounds))[targets addObject:back];
    } else if([kind isEqual:@"FQVShortVideoLandscapeNavBar"]) {
        id items=HGReadObjectGetter(root,"itemList");
        Class shareClass=NSClassFromString(@"SSShortVideoShareLandscapeNavBarItem");
        if([items isKindOfClass:NSArray.class]) {
            for(id item in items) {
                if(![item isKindOfClass:shareClass])continue;
                BOOL shown=NO;
                if(!HGABoolGetter(item,@"isShow",&shown) || !shown)continue;
                UIView *share=HGAViewGetter(item,"shareIconV",root);
                if(share && !CGRectIsEmpty(share.bounds))[targets addObject:share];
            }
        }
    } else if((volume || inflow) && !CGRectIsEmpty(root.bounds)) {
        [targets addObject:root];
    }

    for(UIView *target in states.keyEnumerator.allObjects) {
        if([targets containsObject:target])continue;
        HGARestoreLandscapeSurface(target,[states objectForKey:target]);
        [states removeObjectForKey:target];
    }
    if(!targets.count)return;
    if(!states) {
        states=[NSMapTable weakToStrongObjectsMapTable];
        objc_setAssociatedObject(root,&HGALandscapeSurfacesKey,states,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    BOOL adjustment=control || volume || inflow;
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    for(UIView *target in targets) {
        if(!target.superview)continue;
        BOOL inside=adjustment || (target!=back && !target.clipsToBounds &&
            ![target isKindOfClass:UIImageView.class] && ![target isKindOfClass:UILabel.class]);
        UIView *label=target==back ? title : [countLabels objectForKey:target];
        CGRect frame=HGALandscapeSurfaceFrame(target,label,target==back ? 10 : 2,inside);
        HGALandscapeSurface *state=[states objectForKey:target];
        if(CGRectIsEmpty(frame)) {
            HGARestoreLandscapeSurface(target,state);
            [states removeObjectForKey:target];
            continue;
        }
        if(!state){state=[HGALandscapeSurface new];[states setObject:state forKey:target];}
        if(target==input) {
            if(state.coveredPaint!=inputPaint)HGASetHidden(state.coveredPaint,NO);
            state.coveredPaint=inputPaint;
            if(inputPaint)HGASetHidden(inputPaint,YES);
        }
        HGAPlaceLandscapeSurface(target,frame,inside,state);
    }
    [CATransaction commit];
}
