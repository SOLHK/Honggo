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

// The native share and more centers are 44pt apart. A common 36pt
// diameter leaves an 8pt gap within the original navigation hit region.
static const CGFloat HGALandscapeButtonDiameter=36;

@interface HGALandscapeSurface : NSObject
@property(nonatomic,strong) HGAAppearanceMaterial *glass;
@property(nonatomic,strong) UIColor *sourceBackground;
@property(nonatomic,weak) UIView *coveredPaint;
@property(nonatomic,weak) UILabel *counter;
@property(nonatomic,weak) UIView *counterIcon;
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
    HGARestoreGeometry(state.counter);
    HGARestoreGeometry(state.counterIcon);
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

static CGRect HGALandscapeSurfaceFrame(UIView *target,BOOL round,BOOL inside) {
    UIView *parent=inside ? target : target.superview;
    CGRect frame=inside ? target.bounds : [target convertRect:target.bounds toView:parent];
    CGFloat height=round ? HGALandscapeButtonDiameter : 40;
    CGFloat width=round ? height : MAX(height,CGRectGetWidth(frame));
    CGRect padded=CGRectMake(CGRectGetMidX(frame)-width/2,CGRectGetMidY(frame)-height/2,width,height);
    return !inside && parent.clipsToBounds ? CGRectIntersection(padded,parent.bounds) : padded;
}

static void HGAPlaceLandscapeCounter(UIView *target,UILabel *counter,UIView *icon,HGALandscapeSurface *state) {
    CGRect ink=counter ? [counter textRectForBounds:counter.bounds limitedToNumberOfLines:counter.numberOfLines] : CGRectZero;
    if(counter.superview!=target || icon.superview!=target || counter.hidden ||
        CGRectIsEmpty(ink) || CGRectIsEmpty(icon.bounds)) {
        counter=nil;icon=nil;
    }
    if(state.counter!=counter)HGARestoreGeometry(state.counter);
    if(state.counterIcon!=icon)HGARestoreGeometry(state.counterIcon);
    state.counter=counter;state.counterIcon=icon;
    if(!counter)return;
    CGFloat midX=CGRectGetMidX(target.bounds),midY=CGRectGetMidY(target.bounds);
    CGFloat iconScale=MIN(1,20/MAX(CGRectGetWidth(icon.bounds),CGRectGetHeight(icon.bounds)));
    // Center the original glyph above its count. Align the text's ink instead
    // of the original label frame, which is offset over the icon's corner.
    HGASetGeometry(icon,midX-icon.center.x,midY-7.5-icon.center.y,iconScale);
    CGFloat textScale=MIN(1,MIN(24/CGRectGetWidth(ink),10/CGRectGetHeight(ink)));
    CGFloat inkX=(CGRectGetMidX(ink)-CGRectGetMidX(counter.bounds))*textScale;
    CGFloat inkY=(CGRectGetMidY(ink)-CGRectGetMidY(counter.bounds))*textScale;
    HGASetGeometry(counter,midX-counter.center.x-inkX,midY+8.5-counter.center.y-inkY,textScale);
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
    NSMapTable<UIView *,NSArray<UIView *> *> *counters=biz ? [NSMapTable strongToStrongObjectsMapTable] : nil;
    NSDictionary<NSString *,NSArray<NSString *> *> *countGetters=biz ? @{@"likeBtnView":@[@"likeCountLabel",@"likeIcon"],
        @"commentBtnView":@[@"commentCountLabel",@"commentIcon"],@"starBtnView":@[@"starCountLabel",@"starIcon"]} : nil;
    UIView *back=nil,*next=nil;
    UIView *input=nil,*inputPaint=nil;
    for(NSString *getter in getters[kind]) {
        UIView *target=HGAViewGetter(root,getter.UTF8String,root);
        if(!target || CGRectIsEmpty(target.bounds))continue;
        [targets addObject:target];
        NSArray<NSString *> *counterGetters=countGetters[getter];
        if(counterGetters) {
            UIView *label=HGAViewGetter(root,counterGetters[0].UTF8String,target);
            UIView *icon=HGAViewGetter(root,counterGetters[1].UTF8String,target);
            if([label isKindOfClass:UILabel.class] && icon)[counters setObject:@[label,icon] forKey:target];
        } else if(biz && [getter isEqual:@"danmakuInputView"]) {
            input=target;
            inputPaint=HGAViewGetter(root,"danmakuInputBGView",input);
        } else if(info && [getter isEqual:@"nextBtn"])next=target;
    }
    if(info) {
        back=HGAViewGetter(root,"navBackImageView",root);
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
        NSArray<UIView *> *counter=[counters objectForKey:target];
        BOOL inside=adjustment || target==input || counter || (target!=back && !target.clipsToBounds &&
            ![target isKindOfClass:UIImageView.class] && ![target isKindOfClass:UILabel.class]);
        BOOL round=target!=next && target!=input && !inflow;
        CGRect frame=HGALandscapeSurfaceFrame(target,round,inside);
        // Give the switch breathing room without shifting the host's input
        // target, placeholder or draft publish button.
        if(target==input)frame=CGRectInset(frame,MIN(6,MAX(0,(CGRectGetWidth(frame)-44)/2)),0);
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
        if(counter || state.counter)HGAPlaceLandscapeCounter(target,(UILabel *)counter.firstObject,counter.lastObject,state);
        HGAPlaceLandscapeSurface(target,frame,inside,state);
    }
    [CATransaction commit];
}

static IMP HGALandscapeToastOriginal;
static const uintptr_t *HGALandscapeToastSites;

static const uintptr_t *HGALandscapeToastSitesForExecutable(void) {
    // These locations belong to the inspected executables, even if the IPA
    // has been repackaged with different version metadata.
    static const struct {
        uint8_t uuid[16];
        uintptr_t sites[6];
    } known[] = {
        {{0x43,0x0a,0xc0,0x1a,0x1a,0x01,0x3c,0x39,0xa3,0xfc,0xcc,0xb2,0x7f,0x7a,0x7b,0x0a},
            {0xf5c5b54,0xf5c5da4,0xf5c6454,0xf5c6730,0xf6491ac,0xf649364}},
        {{0xb0,0x67,0x06,0x6f,0xd7,0xf2,0x3d,0xf8,0x8e,0x5d,0xc1,0x81,0xf8,0xc7,0xce,0x1f},
            {0xfa06cb0,0xfa06f04,0xfa075b0,0xfa07890,0xfa8ccb0,0xfa8ce64}},
    };
    const struct mach_header_64 *header=(const struct mach_header_64 *)_dyld_get_image_header(0);
    if(!header || header->magic!=MH_MAGIC_64)return NULL;
    const uint8_t *cursor=(const uint8_t *)(header+1);
    for(uint32_t i=0;i<header->ncmds;i++) {
        const struct load_command *command=(const struct load_command *)cursor;
        if(command->cmd==LC_UUID && command->cmdsize==sizeof(struct uuid_command)) {
            const struct uuid_command *uuid=(const struct uuid_command *)cursor;
            for(NSUInteger index=0;index<sizeof(known)/sizeof(known[0]);index++)
                if(!memcmp(uuid->uuid,known[index].uuid,sizeof(uuid->uuid)))return known[index].sites;
            break;
        }
        cursor+=command->cmdsize;
    }
    return NULL;
}

static void HGALandscapeToast(id object,SEL command,id message) {
    if([message isKindOfClass:NSString.class] && [message isEqualToString:@"加载失败，请稍后重试"]) {
        static NSString *const stages[]={@"prepare-switch",@"select-target",@"complete-switch",
            @"prepare-direct",@"season-delegate-declined",@"season-route-failed"};
        uintptr_t executableBase=(uintptr_t)_dyld_get_image_header(0);
        NSArray<NSNumber *> *frames=NSThread.callStackReturnAddresses;
        NSString *stage=nil;
        for(NSUInteger i=0;i<MIN(frames.count,8) && !stage;i++) {
            uintptr_t offset=frames[i].unsignedLongLongValue-executableBase;
            for(NSUInteger index=0;index<sizeof(stages)/sizeof(stages[0]);index++) {
                if(offset==HGALandscapeToastSites[index]){stage=stages[index];break;}
            }
        }
        if(stage)HGLogCritical(@"playback.landscape-load-failed",@{@"stage":stage});
    }
    ((void(*)(id,SEL,id))HGALandscapeToastOriginal)(object,command,message);
}

static void HGAInstallLandscapeLoadFailureDiagnostic(void) {
    HGALandscapeToastSites=HGALandscapeToastSitesForExecutable();
    if(!HGALandscapeToastSites)return;
    Class toast=NSClassFromString(@"FQVShortVideoListVCToastComponent");
    if(toast)HGALandscapeToastOriginal=HGReplaceMethod(toast,sel_registerName("toast:"),"v24@0:8@16",(IMP)HGALandscapeToast);
}
