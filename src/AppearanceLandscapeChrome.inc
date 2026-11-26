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
            @"FQVLandscapeBrightnessButton":@[],
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

static const CGFloat HGALandscapeButtonDiameter = 44;
static const CGFloat HGALandscapeGap = 4;

// The host wins normal hit testing. The HUD can explicitly return this button
// for the extra rim of a nontracking playback option outside its native bounds.
@interface HGALandscapeButton : HGAPageGlassButton
@property(nonatomic,weak) UIButton *option;
- (void)activateOption;
@end
@implementation HGALandscapeButton
- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event { return nil; }
- (void)activateOption {
    if (self.option.enabled) [self.option sendActionsForControlEvents:UIControlEventTouchUpInside];
}
@end

@interface HGALandscapeSurface : NSObject
@property(nonatomic,strong) HGALandscapeButton *glass;
@property(nonatomic,weak) UILabel *nativeOptionTitle;
@property(nonatomic,weak) UILabel *positionedLabel;
@property(nonatomic) CGRect hostLabelFrame;
@property(nonatomic) CGRect appliedLabelFrame;
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
static char HGALandscapeSurfacesKey,HGALandscapeHitTargetsKey;
static UIView *(*HGALandscapeNativeHit)(UIView *,SEL,CGPoint,UIEvent *);

static UIView *HGALandscapeHit(UIView *hud,SEL cmd,CGPoint point,UIEvent *event) {
    UIView *native=HGALandscapeNativeHit(hud,cmd,point,event);
    if (!hud.window || hud.hidden || hud.alpha<=0.01 || !hud.userInteractionEnabled ||
        [native isKindOfClass:UIControl.class])return native;
    NSMapTable *targets=objc_getAssociatedObject(hud,&HGALandscapeHitTargetsKey);
    for(UIView *target in targets) {
        HGALandscapeButton *glass=[targets objectForKey:target];
        if (!glass.superview || ![glass isDescendantOfView:hud] || !HGAVisible(glass) ||
            (native && ![target isDescendantOfView:native]))continue;
        CGPoint p=[glass convertPoint:point fromView:hud];
        CGRect bounds=glass.bounds;
        if(!CGRectContainsPoint(bounds,p))continue;
        CGFloat radius=MIN(bounds.size.width,bounds.size.height)/2;
        CGFloat cx=MAX(CGRectGetMinX(bounds)+radius,MIN(p.x,CGRectGetMaxX(bounds)-radius));
        if(hypot(p.x-cx,p.y-CGRectGetMidY(bounds))>radius)continue;
        // Ask the original implementation who owns the control center. This
        // keeps a native long press, icon tap, and any covering overlay intact.
        CGPoint center=[target convertPoint:CGPointMake(CGRectGetMidX(target.bounds),CGRectGetMidY(target.bounds)) toView:hud];
        UIView *owner=HGALandscapeNativeHit(hud,cmd,center,event);
        if (!owner || ![owner isDescendantOfView:target])continue;
        if (glass.option && owner==glass.option && glass.option.enabled &&
            ![owner pointInside:[owner convertPoint:point fromView:hud] withEvent:event])return glass;
        return owner;
    }
    return native;
}

// A model/title refresh may replace the label or lay it out again while enabled.
static void HGALandscapePositionLabel(HGALandscapeSurface *state, UILabel *label, CGFloat minX, CGFloat maxX) {
    UILabel *previous = state.positionedLabel;
    if (previous && previous != label && CGRectEqualToRect(previous.frame, state.appliedLabelFrame))
        previous.frame = state.hostLabelFrame;
    if (!label) { state.positionedLabel = nil; return; }
    if (previous != label || !CGRectEqualToRect(label.frame, state.appliedLabelFrame))
        state.hostLabelFrame = label.frame;
    CGRect adjusted = state.hostLabelFrame;
    adjusted.origin.x = MAX(CGRectGetMinX(adjusted), minX);
    adjusted.size.width = MIN(adjusted.size.width, MAX(0, maxX - adjusted.origin.x));
    state.positionedLabel = label;
    state.appliedLabelFrame = adjusted;
    if (!CGRectEqualToRect(label.frame, adjusted)) label.frame = adjusted;
}

static void HGARestoreLandscapeSurface(UIView *target,HGALandscapeSurface *state) {
    if(state.clearsBackground && [target.backgroundColor isEqual:UIColor.clearColor])
        target.backgroundColor=state.sourceBackground;
    if(state.unclipped)target.clipsToBounds=YES;
    if(state.clearedOpaque)target.opaque=YES;
    HGASetHidden(state.coveredPaint,NO);
    HGARestoreGeometry(state.counter);
    HGARestoreGeometry(state.counterIcon);
    HGALandscapePositionLabel(state,nil,0,0);
    HGASetHidden(state.nativeOptionTitle,NO);
    [state.glass.press.view removeGestureRecognizer:state.glass.press];
    [state.glass removeFromSuperview];
}

static void HGAPlaceLandscapeSurface(UIView *target,CGRect frame,BOOL inside,HGALandscapeSurface *state) {
    if(!state.glass) {
        state.glass=[[HGALandscapeButton alloc] initWithFrame:frame];
        state.glass.accessibilityIdentifier=@"honggo.appearance.landscape.chrome";
        state.glass.accessibilityElementsHidden=YES;
        state.glass.isAccessibilityElement=NO;
        state.glass.userInteractionEnabled=NO;
        state.glass.overrideUserInterfaceStyle=UIUserInterfaceStyleDark;
        state.glass.press=[[HGAPageButtonPress alloc] initWithTarget:nil action:NULL];
        state.glass.press.button=state.glass;
        state.glass.press.cancelsTouchesInView=NO;
        state.glass.press.delaysTouchesEnded=NO;
    }
    if(!state.clearsBackground || ![target.backgroundColor isEqual:UIColor.clearColor])
        state.sourceBackground=target.backgroundColor;
    state.clearsBackground=YES;
    if(target.opaque){state.clearedOpaque=YES;target.opaque=NO;}
    if(![target.backgroundColor isEqual:UIColor.clearColor])target.backgroundColor=UIColor.clearColor;
    HGAPageGlassButton *glass=state.glass;
    HGAConfigurePageButton(glass,HGALiquidButtonMaterial(),nil);
    UIButtonConfiguration *config=glass.configuration;
    UIBackgroundConfiguration *background=config.background;
    if (!NSDirectionalEdgeInsetsEqualToDirectionalEdgeInsets(background.backgroundInsets,NSDirectionalEdgeInsetsZero)) {
        background.backgroundInsets=NSDirectionalEdgeInsetsZero;
        config.background=background;
        glass.configuration=config;
    }
    if(!CGRectEqualToRect(glass.frame,frame))glass.frame=frame;
    UIView *pressHost=target.userInteractionEnabled ? target : target.superview;
    if(pressHost && glass.press.view!=pressHost)[pressHost addGestureRecognizer:glass.press];
    if(inside) {
        // Keep the glass with controls that have separate foreground views so
        // it follows their native hide and scale animations.
        if(target.clipsToBounds){state.unclipped=YES;target.clipsToBounds=NO;}
        if(glass.superview!=target)[target insertSubview:glass atIndex:0];
    } else if(glass.superview!=target.superview) {
        // A sibling sits behind UIImageView's own pixels as well as descendants.
        [target.superview insertSubview:glass belowSubview:target];
    }
    UIView *hud=HGAAncestorNamed(target,@"FQVShortVideoPlayerLandscapeBaseControlView");
    if(hud) {
        NSMapTable *targets=objc_getAssociatedObject(hud,&HGALandscapeHitTargetsKey);
        if(!targets) {
            targets=[NSMapTable weakToWeakObjectsMapTable];
            objc_setAssociatedObject(hud,&HGALandscapeHitTargetsKey,targets,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        if([targets objectForKey:target]!=glass)[targets setObject:glass forKey:target];
    }
    BOOL hidden=!inside && (target.hidden || target.alpha<0.01);
    if(glass.hidden!=hidden)glass.hidden=hidden;

}

// Take half the distance to either neighbouring center. Both buttons may grow
// without crossing this midpoint, even with a fourth arranged item or a compact row.
static CGFloat HGALandscapeSlotWidth(UIView *target, NSArray<UIView *> *peers, CGFloat preferred) {
    UIView *parent=target.superview;
    CGFloat center=CGRectGetMidX([target convertRect:target.bounds toView:parent]);
    CGFloat width=preferred;
    for (UIView *other in peers) {
        if (other==target || other.hidden || CGRectIsEmpty(other.bounds)) continue;
        CGFloat distance=fabs(CGRectGetMidX([other convertRect:other.bounds toView:parent])-center);
        width=MIN(width,MAX(0,distance-HGALandscapeGap));
    }
    return width;
}

// Narrow layouts let the compact input and the first playback option approach
// from different containers. Reserve a shared boundary for both sets of glass.
static CGFloat HGALandscapeFooterDivider(UIView *root,BOOL options) {
    UIView *left=options ? nil : root,*right=options ? root : nil;
    for(UIView *other in root.superview.subviews) {
        if(other.hidden)continue;
        if(options && [other isKindOfClass:objc_lookUpClass("SSShortVideoLandscapeBizInteractionView")])left=other;
        if(!options && [other isKindOfClass:objc_lookUpClass("FQVShortVideoLandscapeBottomRightView")])right=other;
    }
    if(!left || !right)return NAN;
    CGFloat end=-CGFLOAT_MAX,start=CGFLOAT_MAX;
    for(NSString *name in @[@"likeBtnView",@"commentBtnView",@"starBtnView",@"danmakuBtnView",@"danmakuInputView",@"danmakuPublishBtn"]) {
        UIView *v=HGAViewGetter(left,name.UTF8String,left);
        if(v && !v.hidden && !CGRectIsEmpty(v.bounds))end=MAX(end,CGRectGetMaxX([v convertRect:v.bounds toView:root.superview]));
    }
    UIView *rate=HGAViewGetter(right,"playRateBtn",right);
    UIStackView *stack=[rate.superview isKindOfClass:UIStackView.class] ? (UIStackView *)rate.superview : nil;
    for(UIView *v in stack.arrangedSubviews) {
        if(!v.hidden && !CGRectIsEmpty(v.bounds))start=MIN(start,CGRectGetMinX([v convertRect:v.bounds toView:root.superview]));
    }
    return (end==-CGFLOAT_MAX || start==CGFLOAT_MAX) ? NAN : (end+start)/2;
}

static CGRect HGALandscapeSurfaceFrame(UIView *target,CGSize size,BOOL inside) {
    UIView *parent=inside ? target : target.superview;
    CGRect frame=[target convertRect:target.bounds toView:parent];
    CGRect padded=CGRectMake(CGRectGetMidX(frame)-size.width/2,CGRectGetMidY(frame)-size.height/2,size.width,size.height);
    return !inside && parent.clipsToBounds ? CGRectIntersection(padded,parent.bounds) : padded;
}

static void HGALandscapeOptionTitle(UIButton *button,HGALandscapeSurface *state) {
    // The host fixes some button frames at 34pt. Keep its title for accessibility
    // and actions, and draw the complete current title inside the wider native capsule.
    UILabel *host=button.titleLabel;
    if(state.glass.option!=button) {
        state.glass.option=button;
        state.glass.userInteractionEnabled=YES;
        [state.glass addTarget:state.glass action:@selector(activateOption) forControlEvents:UIControlEventTouchUpInside];
    }
    if (state.nativeOptionTitle != host) HGASetHidden(state.nativeOptionTitle,NO);
    state.nativeOptionTitle=host;
    HGASetHidden(host,YES);
    NSAttributedString *title=button.currentAttributedTitle;
    if (!title.length) title=[[NSAttributedString alloc] initWithString:button.currentTitle ?: @""
        attributes:@{NSFontAttributeName:host.font, NSForegroundColorAttributeName:[button titleColorForState:button.state] ?: UIColor.whiteColor}];
    UIButtonConfiguration *config=state.glass.configuration;
    if (![config.attributedTitle isEqual:title]) {
        config.attributedTitle=title;
        config.contentInsets=NSDirectionalEdgeInsetsMake(0,7,0,7);
        config.titleLineBreakMode=NSLineBreakByClipping;
        state.glass.configuration=config;
    }
    state.glass.titleLabel.adjustsFontSizeToFitWidth=YES;
    state.glass.titleLabel.minimumScaleFactor=0.55;
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
    CGFloat iconScale=MIN(1,22/MAX(CGRectGetWidth(icon.bounds),CGRectGetHeight(icon.bounds)));
    // Center the original glyph above its count. Align the text's ink instead
    // of the original label frame, which is offset over the icon's corner.
    HGASetGeometry(icon,midX-icon.center.x,midY-7-icon.center.y,iconScale);
    CGFloat textScale=MIN(1,MIN(32/CGRectGetWidth(ink),12/CGRectGetHeight(ink)));
    CGFloat inkX=(CGRectGetMidX(ink)-CGRectGetMidX(counter.bounds))*textScale;
    CGFloat inkY=(CGRectGetMidY(ink)-CGRectGetMidY(counter.bounds))*textScale;
    HGASetGeometry(counter,midX-counter.center.x-inkX,midY+11-counter.center.y-inkY,textScale);
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
    BOOL adjustmentButton=[kind isEqual:@"FQVLandscapeVolumeButton"] || [kind isEqual:@"FQVLandscapeBrightnessButton"];
    BOOL options=[kind isEqual:@"FQVShortVideoLandscapeBottomRightView"];
    BOOL inflow=[kind isEqual:@"SSShortVideoLandscapeAutoInflowTipView"];
    if(control) {
        HGALandscapeApplyView(HGAViewGetter(root,"infoView",root));
        Class volumeClass=NSClassFromString(@"FQVLandscapeVolumeButton");
        Class brightnessClass=NSClassFromString(@"FQVLandscapeBrightnessButton");
        Class overlayClass=NSClassFromString(@"FQVTouchIgoringView");
        Class inflowClass=NSClassFromString(@"SSShortVideoLandscapeAutoInflowTipView");
        for(UIView *child in root.subviews) {
            if([child isKindOfClass:volumeClass] || [child isKindOfClass:brightnessClass])HGALandscapeApplyView(child);
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
    } else if((adjustmentButton || inflow) && !CGRectIsEmpty(root.bounds)) {
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
    BOOL adjustment=control || adjustmentButton || inflow;
    CGFloat footerDivider=(biz || options) ? HGALandscapeFooterDivider(root,options) : NAN;
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    for(UIView *target in targets) {
        if(!target.superview)continue;
        NSArray<UIView *> *counter=[counters objectForKey:target];
        BOOL inside=adjustment || target==input || counter || (target!=back && !target.clipsToBounds &&
            ![target isKindOfClass:UIImageView.class] && ![target isKindOfClass:UILabel.class]);
        CGFloat diameter=HGALandscapeButtonDiameter;
        CGSize size=CGSizeMake(diameter,diameter);
        if (options && [target isKindOfClass:UIButton.class]) {
            UIButton *option=(UIButton *)target;
            UILabel *label=option.titleLabel;
            CGFloat text=option.currentAttributedTitle.length ? option.currentAttributedTitle.size.width :
                [option.currentTitle sizeWithAttributes:@{NSFontAttributeName:label.font}].width;
            UIStackView *stack=[target.superview isKindOfClass:UIStackView.class] ? (UIStackView *)target.superview : nil;
            size.width=HGALandscapeSlotWidth(target,stack.arrangedSubviews ?: targets,MAX(48,ceil(text)+20));
        } else if (target==input || inflow || target==next) {
            size.width=MAX(diameter,CGRectGetWidth(target.bounds)+(target==input ? 0 : 8));
        } else if (biz) {
            NSMutableArray *icons=targets.mutableCopy;
            if(input)[icons removeObject:input];
            size.width=size.height=HGALandscapeSlotWidth(target,icons,diameter);
        } else if ([kind isEqual:@"FQVShortVideoLandscapeNavBar"] || (info && target!=back)) {
            // On 7.4.5.33 share and more are 44pt apart in the same navigation row.
            size.width=size.height=40;
        }
        CGRect frame=HGALandscapeSurfaceFrame(target,size,inside);
        if (target==input) {
            UIView *toggle=HGAViewGetter(root,"danmakuBtnView",root);
            if (toggle && !toggle.hidden) {
                CGRect slot=[toggle convertRect:toggle.bounds toView:input];
                CGFloat edge=CGRectGetMidX(slot)+diameter/2+HGALandscapeGap;
                if (CGRectGetMidX(slot)<CGRectGetMidX(frame) && edge>CGRectGetMinX(frame)) {
                    CGFloat x=MIN(edge,CGRectGetMaxX(frame));
                    frame=CGRectMake(x,CGRectGetMinY(frame),CGRectGetMaxX(frame)-x,frame.size.height);
                }
            }
        }
        if (isfinite(footerDivider)) {
            UIView *coordinateView=inside ? target : target.superview;
            CGFloat split=[coordinateView convertPoint:CGPointMake(footerDivider,0) fromView:root.superview].x;
            CGFloat edge=options ? MAX(CGRectGetMinX(frame),split+HGALandscapeGap/2) : CGRectGetMinX(frame);
            CGFloat end=options ? CGRectGetMaxX(frame) : MIN(CGRectGetMaxX(frame),split-HGALandscapeGap/2);
            frame=CGRectMake(edge,CGRectGetMinY(frame),MAX(0,end-edge),frame.size.height);
            if (biz && target!=input && frame.size.width<frame.size.height)
                frame=CGRectInset(frame,0,(frame.size.height-frame.size.width)/2);
        }
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
        if (options && [target isKindOfClass:UIButton.class]) HGALandscapeOptionTitle((UIButton *)target,state);
        if (target==back) {
            UIView *title=HGAViewGetter(root,"titleL",root);
            UIView *nav=HGAViewGetter(root,"navBar",root);
            if ([title isKindOfClass:UILabel.class] && title.superview) {
                CGRect arrow=[state.glass convertRect:state.glass.bounds toView:title.superview];
                CGFloat edge=CGRectGetMaxX(title.superview.bounds);
                if (nav && !nav.hidden) {
                    CGRect right=[nav convertRect:nav.bounds toView:title.superview];
                    if (CGRectGetMinX(right)>CGRectGetMaxX(arrow))edge=MIN(edge,CGRectGetMinX(right)-12);
                }
                HGALandscapePositionLabel(state,(UILabel *)title,CGRectGetMaxX(arrow)+12,edge);
            }
        } else if (target==input) {
            UILabel *placeholder=(id)HGAViewGetter(root,"danmakuInputLabel",input);
            if ([placeholder isKindOfClass:UILabel.class] && placeholder.superview==input)
                HGALandscapePositionLabel(state,placeholder,CGRectGetMinX(frame)+10,CGRectGetMaxX(frame)-10);
        }
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
