// Scoped appearance operations. Values are restored only while they are still
// owned by Honggo; a later host/third-party update becomes the new baseline.
static BOOL HGAUsesNativeGlass(NSString *style) {
    return ([style isEqual:@"regular"] || [style isEqual:@"clear"]) && HGNativeGlassConfigured() &&
        !UIAccessibilityIsReduceTransparencyEnabled();
}

@interface HGAAppearanceMaterial : UIVisualEffectView
@property(nonatomic,copy) NSString *style;
@property(nonatomic,copy) NSString *materialSignature;
@property(nonatomic) CGFloat overlayOpacity;
@property(nonatomic,strong) UIColor *overlayColor;
@property(nonatomic) BOOL interactiveGlass;
@property(nonatomic) BOOL untintedGlass;
@property(nonatomic) BOOL lightContourRim;
@property(nonatomic) CGFloat materialCornerRadius;
@property(nonatomic,strong) CALayer *gaussianBackdrop;
@property(nonatomic,strong) CAShapeLayer *contourRimLayer;
@property(nonatomic,weak) UIView *flexTouchHost;
- (void)requestMaterialRefresh;
@end
// Only the backing pixels enter this filter. Comment text, avatars and the
// composer stay in the ordinary view hierarchy above the material.
static CALayer *HGACreateGaussianBackdrop(void) {
    Class backdropType=objc_lookUpClass("CABackdropLayer");
    Class filterType=objc_lookUpClass("CAFilter");
    SEL factory=sel_registerName("filterWithType:");
    Method method=class_getClassMethod(filterType,factory);
    if (![backdropType isSubclassOfClass:CALayer.class] || !method ||
        strcmp(method_getTypeEncoding(method),"@24@0:8@16")) return nil;
    id gaussian=((id (*)(id,SEL,id))objc_msgSend)(filterType,factory,@"gaussianBlur");
    if (!gaussian) return nil;
    [gaussian setValue:@16 forKey:@"inputRadius"];
    CALayer *backdrop=[backdropType layer];
    backdrop.filters=@[gaussian];
    return backdrop;
}

static BOOL HGAHostExplicitDarkMode(UIView *view,BOOL *isDark) {
    Protocol *adapter=objc_getProtocol("SSAdaptiveUIColorProtocol");
    id center=adapter ? HGReadObjectGetter(objc_lookUpClass("HTSServiceCenter"),"defaultCenter") : nil;
    SEL getter=sel_registerName("getProtocolService:");
    Method method=center ? class_getInstanceMethod(object_getClass(center),getter) : NULL;
    id themeManager=(method && !strcmp(method_getTypeEncoding(method),"@24@0:8@16")) ?
        ((id (*)(id,SEL,Protocol *))objc_msgSend)(center,getter,adapter) : nil;
    BOOL dark=NO;
    if(HGABoolGetter(themeManager,@"isCurrentDarkMode",&dark)) {
        if(isDark)*isDark=dark;
        return YES;
    }
    UIWindow *window=view.window;
    if(window && HGABoolGetter(window,@"isDark",&dark)) {
        if(isDark)*isDark=dark;
        return YES;
    }
    return NO;
}

static UIUserInterfaceStyle HGAAdaptivePageStyle(UIViewController *controller,UIView *bar) {
    // Catalogue, profile and modal sheets follow the host's Day/Night theme.
    BOOL dark=NO;
    if(HGAHostExplicitDarkMode(bar ?: controller.viewIfLoaded,&dark))
        return dark ? UIUserInterfaceStyleDark : UIUserInterfaceStyleLight;
    UIUserInterfaceStyle appStyle=controller.viewIfLoaded.traitCollection.userInterfaceStyle;
    if(appStyle==UIUserInterfaceStyleUnspecified)appStyle=bar.window.traitCollection.userInterfaceStyle;
    return appStyle!=UIUserInterfaceStyleUnspecified ? appStyle : bar.traitCollection.userInterfaceStyle;
}

@implementation HGAAppearanceMaterial {
    uint64_t _materialKey;
}
- (void)setMaterialSignature:(NSString *)materialSignature {
    _materialSignature=[materialSignature copy];
    if(!materialSignature)_materialKey=0;
}
- (UIView *)_flexInteractionGestureView {
    UIView *host=self.flexTouchHost;
    if(host.window && host.window==self.window && host.userInteractionEnabled)return host;
    IMP inherited=class_getMethodImplementation(UIVisualEffectView.class,_cmd);
    return ((UIView *(*)(id,SEL))inherited)(self,_cmd);
}
- (void)setMaterialCornerRadius:(CGFloat)radius {
    if(fabs(_materialCornerRadius-radius)<0.001)return;
    _materialCornerRadius=radius;
    [self setNeedsLayout];
}
- (void)setUntintedGlass:(BOOL)untintedGlass {
    if(_untintedGlass==untintedGlass)return;
    _untintedGlass=untintedGlass;self.materialSignature=nil;[self setNeedsLayout];
}
- (void)setLightContourRim:(BOOL)lightContourRim {
    if(_lightContourRim==lightContourRim)return;
    _lightContourRim=lightContourRim;[self setNeedsLayout];
}
- (void)setOverlayColor:(UIColor *)color {
    if(_overlayColor==color || [_overlayColor isEqual:color])return;
    _overlayColor=color;self.materialSignature=nil;[self setNeedsLayout];
}
- (void)willMoveToWindow:(UIWindow *)window {
    // A live glass effect must never be carried into a detached scene. Its
    // replacement is created only after attachment with final, nonzero bounds.
    if(window!=self.window){
        [self.gaussianBackdrop removeFromSuperlayer];
        self.gaussianBackdrop=nil;
        self.effect=nil;self.materialSignature=nil;
    }
    [super willMoveToWindow:window];
}
- (void)didMoveToWindow {
    [super didMoveToWindow];
    if(self.window)[self setNeedsLayout];
}
- (uint64_t)currentMaterialKey {
    uint64_t styleBits=1;
    NSString *s=self.style;
    if([s isEqual:@"regular"])styleBits=2;
    else if([s isEqual:@"clear"])styleBits=3;
    else if([s isEqual:@"blur"])styleBits=4;
    else if([s isEqual:@"gaussian"])styleBits=5;
    else if([s isEqual:@"solid"])styleBits=6;
    else if(s.length)styleBits=7u ^ ((s.hash & 0xFFFFu) << 3);
    uint64_t opacityBits=(uint64_t)((NSInteger)llround(MAX(0.0,MIN(4.0,self.overlayOpacity))*1000.0) & 0xFFFF);
    uint64_t uiStyle=(uint64_t)((self.traitCollection.userInterfaceStyle + 1) & 0xF);
    return (1ULL << 63) | (styleBits << 28) | (opacityBits << 12) | (uiStyle << 8) |
        ((HGNativeGlassConfigured() ? 1ULL : 0ULL) << 3) |
        ((self.interactiveGlass ? 1ULL : 0ULL) << 2) |
        ((UIAccessibilityIsReduceTransparencyEnabled() ? 1ULL : 0ULL) << 1) |
        (self.untintedGlass ? 1ULL : 0ULL);
}
- (NSString *)currentMaterialSignature {
    return [NSString stringWithFormat:@"%@/%d/%d/%ld/%d/%.3f/%d",self.style,
        HGNativeGlassConfigured(),self.interactiveGlass,(long)self.traitCollection.userInterfaceStyle,
        UIAccessibilityIsReduceTransparencyEnabled(),self.overlayOpacity,self.untintedGlass];
}
- (void)requestMaterialRefresh {
    if(!_materialSignature || [self currentMaterialKey]!=_materialKey)[self setNeedsLayout];
}
- (void)layoutSubviews {
    [super layoutSubviews];
    if(!self.window || CGRectIsEmpty(self.bounds))return;
    BOOL native=self.untintedGlass && HGAUsesNativeGlass(self.style);
    if (@available(iOS 26.0,*)) {
        if(self.untintedGlass) {
            CGFloat radius=self.materialCornerRadius;
            UICornerConfiguration *corners=nil;
            if(native)corners=radius<=0 || radius>=MIN(self.bounds.size.width,self.bounds.size.height)*0.5-0.5 ?
                UICornerConfiguration.capsuleConfiguration :
                [UICornerConfiguration configurationWithUniformRadius:[UICornerRadius fixedRadius:radius]];
            if(corners && ![self.cornerConfiguration isEqual:corners])
                self.cornerConfiguration=corners;
            if(self.clipsToBounds==native)self.clipsToBounds=!native;
        }
    }
    if (self.gaussianBackdrop && !CGRectEqualToRect(self.gaussianBackdrop.frame,self.bounds))
        self.gaussianBackdrop.frame=self.bounds;
    BOOL wantsRim=self.lightContourRim && self.traitCollection.userInterfaceStyle!=UIUserInterfaceStyleDark &&
        !UIAccessibilityIsReduceTransparencyEnabled();
    if(wantsRim) {
        if(!self.contourRimLayer) {
            self.contourRimLayer=[CAShapeLayer layer];
            self.contourRimLayer.zPosition=1000;
        }
        if(self.contourRimLayer.superlayer!=self.layer)
            [self.layer addSublayer:self.contourRimLayer];
        CGRect rimBounds=self.bounds;
        CGFloat scale=MAX(1.0,self.traitCollection.displayScale);
        CGFloat strokeW=MAX(0.85,2.0/scale);
        CGRect strokeRect=CGRectInset(rimBounds,strokeW*0.5,strokeW*0.5);
        CGFloat rimRadius=self.materialCornerRadius>0 ?
            MIN(self.materialCornerRadius,MIN(rimBounds.size.width,rimBounds.size.height)*0.5) :
            MIN(rimBounds.size.width,rimBounds.size.height)*0.5;
        CGFloat strokeRadius=MAX(0,rimRadius-strokeW*0.5);
        UIBezierPath *rimPath=[UIBezierPath bezierPathWithRoundedRect:strokeRect cornerRadius:strokeRadius];
        UIBezierPath *shadowPath=[UIBezierPath bezierPathWithRoundedRect:rimBounds cornerRadius:rimRadius];
        [CATransaction begin];[CATransaction setDisableActions:YES];
        self.contourRimLayer.frame=rimBounds;
        self.contourRimLayer.path=rimPath.CGPath;
        self.contourRimLayer.lineWidth=strokeW;
        self.contourRimLayer.strokeColor=[UIColor colorWithWhite:0 alpha:0.22].CGColor;
        self.contourRimLayer.fillColor=[UIColor colorWithWhite:0 alpha:0.035].CGColor;
        self.contourRimLayer.shadowColor=UIColor.blackColor.CGColor;
        self.contourRimLayer.shadowOpacity=self.clipsToBounds ? 0.0f : 0.08f;
        self.contourRimLayer.shadowRadius=1.6;
        self.contourRimLayer.shadowOffset=CGSizeMake(0,0.75);
        self.contourRimLayer.shadowPath=shadowPath.CGPath;
        self.contourRimLayer.hidden=NO;
        [CATransaction commit];
    } else if(self.contourRimLayer) {
        [self.contourRimLayer removeFromSuperlayer];
        self.contourRimLayer=nil;
    }
    uint64_t key=[self currentMaterialKey];
    if(_materialSignature && key==_materialKey)return;
    _materialKey=key;
    _materialSignature=[[self currentMaterialSignature] copy];
    BOOL gaussian=[self.style isEqual:@"gaussian"] && !UIAccessibilityIsReduceTransparencyEnabled();
    if (gaussian && !self.gaussianBackdrop) {
        self.gaussianBackdrop=HGACreateGaussianBackdrop();
        if (self.gaussianBackdrop) {
            self.effect=nil;
            self.gaussianBackdrop.frame=self.bounds;
            [self.layer insertSublayer:self.gaussianBackdrop atIndex:0];
        }
    } else if (!gaussian && self.gaussianBackdrop) {
        [self.gaussianBackdrop removeFromSuperlayer];
        self.gaussianBackdrop=nil;
    }
    UIVisualEffect *effect=self.gaussianBackdrop ? nil : HGAppearanceMaterial(gaussian ? @"blur" : self.style);
    UIColor *color=native ? UIColor.clearColor : self.overlayColor ?: [UIColor.secondarySystemBackgroundColor colorWithAlphaComponent:
        effect ? self.overlayOpacity : 1];
    if(@available(iOS 26.0,*)) {
        if(self.interactiveGlass && [effect isKindOfClass:UIGlassEffect.class])
            ((UIGlassEffect *)effect).interactive=YES;
    }
    self.effect=effect;
    if(![self.contentView.backgroundColor isEqual:color])self.contentView.backgroundColor=color;
}
@end
@interface HGAAdaptiveInkEntry : NSObject
@property(nonatomic,strong) UIColor *sourceColor;
@property(nonatomic,strong) UIColor *appliedColor;
@property(nonatomic,copy) NSAttributedString *sourceText;
@property(nonatomic,copy) NSAttributedString *appliedText;
@end
@implementation HGAAdaptiveInkEntry @end

@interface HGAViewState : NSObject
@property(nonatomic) CGAffineTransform sourceTransform;
@property(nonatomic) CGAffineTransform appliedTransform;
@property(nonatomic) BOOL geometry;
@property(nonatomic) BOOL transferring;
@property(nonatomic) BOOL matchesEpisodeRail;
@property(nonatomic) CGAffineTransform railDrawingTransform;
@property(nonatomic,weak) UIView *geometrySuperview;
@property(nonatomic,weak) id<UIViewControllerTransitionCoordinator> transition;
@property(nonatomic) BOOL sourceHidden;
@property(nonatomic) BOOL appliedHidden;
@property(nonatomic) BOOL visibility;
@property(nonatomic,strong) UIColor *background;
@property(nonatomic) BOOL clearsBackground;
@property(nonatomic,strong) HGAAppearanceMaterial *glass;
@property(nonatomic,strong) NSMapTable<UIView *,HGAAdaptiveInkEntry *> *adaptiveInk;
@end
@implementation HGAViewState @end
static char HGAViewStateKey;
static HGAViewState *HGAState(UIView *view,BOOL create) {
    HGAViewState *state=objc_getAssociatedObject(view,&HGAViewStateKey);
    if(!state && create){state=[HGAViewState new];objc_setAssociatedObject(view,&HGAViewStateKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);}
    return state;
}
static void HGARestoreGeometry(UIView *view) {
    HGAViewState *state=HGAState(view,NO);
    if(!state.geometry)return;
    BOOL owned=CGAffineTransformEqualToTransform(view.transform,state.appliedTransform);
    CGAffineTransform source=state.sourceTransform;
    // Relinquish ownership before invoking a host setter, which may itself
    // synchronously request layout while a player is being transferred.
    state.geometry=NO;state.geometrySuperview=nil;
    if(owned && !CGAffineTransformEqualToTransform(view.transform,source))view.transform=source;
}
static void HGASetGeometryXY(UIView *view,CGFloat x,CGFloat y,CGFloat scaleX,CGFloat scaleY) {
    if(fabs(x)<0.001 && fabs(y)<0.001 && fabs(scaleX-1)<0.001 && fabs(scaleY-1)<0.001){HGARestoreGeometry(view);return;}
    if(!isfinite(x)||!isfinite(y)||!isfinite(scaleX)||!isfinite(scaleY))return;
    HGAViewState *state=HGAState(view,YES);
    if(state.geometry && state.geometrySuperview!=view.superview)HGARestoreGeometry(view);
    // Reuse the unmodified baseline while we still own the current value.
    // Restoring and reapplying on every layout invalidates native player layout.
    if(!state.geometry || !CGAffineTransformEqualToTransform(view.transform,state.appliedTransform))
        state.sourceTransform=view.transform;
    CGAffineTransform next=CGAffineTransformScale(state.sourceTransform,scaleX,scaleY);
    next.tx+=x;next.ty+=y;
    state.appliedTransform=next;state.geometry=YES;state.geometrySuperview=view.superview;
    if(!CGAffineTransformEqualToTransform(view.transform,next))view.transform=next;
}
static void HGASetGeometry(UIView *view,CGFloat x,CGFloat y,CGFloat scale) {
    HGASetGeometryXY(view,x,y,scale,scale);
}
static void HGASetHidden(UIView *view,BOOL hide) {
    if(!view)return;
    HGAViewState *state=HGAState(view,hide);
    if(!state)return;
    if(!hide && state.visibility) {
        if(view.hidden==state.appliedHidden)view.hidden=state.sourceHidden;
        state.visibility=NO;
    }
    if(hide){
        if(!state.visibility || view.hidden!=state.appliedHidden)state.sourceHidden=view.hidden;
        state.appliedHidden=YES;state.visibility=YES;
        if(!view.hidden)view.hidden=YES;
    }
}
// The host owns hidden for these controls (for example, after following an author).
// Suppress alpha so native hidden changes still take effect when a switch is restored.
static char HGAFeedPartsKey;
static void HGASetSuppressedFeedParts(UIView *owner, NSArray<UIView *> *parts) {
    NSMapTable<UIView *,NSNumber *> *alphas=objc_getAssociatedObject(owner,&HGAFeedPartsKey);
    for (UIView *view in alphas.keyEnumerator.allObjects) {
        if ([parts containsObject:view]) continue;
        if (view.alpha==0) view.alpha=[alphas objectForKey:view].doubleValue;
        [alphas removeObjectForKey:view];
    }
    if (!parts.count) {
        if (alphas) objc_setAssociatedObject(owner,&HGAFeedPartsKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        return;
    }
    if (!alphas) {
        alphas=[NSMapTable weakToStrongObjectsMapTable];
        objc_setAssociatedObject(owner,&HGAFeedPartsKey,alphas,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    for (UIView *view in parts) {
        if (view.alpha!=0 || ![alphas objectForKey:view]) [alphas setObject:@(view.alpha) forKey:view];
        if (view.alpha!=0) view.alpha=0;
    }
}

static UIView *HGALazyControlSubview(UIView *owner,const char *ivarName,const char *getterName);
static void HGAApplyFeedActionCaption(UIView *action,NSString *role,BOOL enabled) {
    static NSDictionary *options;
    static dispatch_once_t once;
    dispatch_once(&once,^{options=@{
        @"collect":@[@"player.hideFeedCollectCount",@"_starCountLabel",@"_collectCountLabel"],
        @"comment":@[@"player.hideFeedCommentCount",@"_commentCountLabel"],
        @"like":@[@"player.hideFeedLikeCount",@"_likeCountLabel"],
        @"share":@[@"player.hideFeedShareCaption",@"_shareCountLabel"]
    };});
    NSArray *fields=options[role];
    if (!fields) return;
    NSMutableArray *parts=nil;
    if (enabled && HGAppearanceFlag(fields.firstObject)) {
        parts=[NSMutableArray array];
        for (NSUInteger i=1;i<fields.count;i++) {
            UIView *label=HGALazyControlSubview(action,[fields[i] UTF8String],NULL);
            if (label) [parts addObject:label];
        }
    }
    HGASetSuppressedFeedParts(action,parts);
}
static void HGAApplyRailAvatarVisibility(UIView *avatar,BOOL enabled) {
    BOOL hidePhoto=enabled && HGAppearanceFlag(@"player.hideFeedAvatarPhoto");
    BOOL hideFollow=enabled && HGAppearanceFlag(@"player.hideFeedFollow");
    BOOL hide=(hidePhoto && hideFollow) || (enabled && HGAppearanceFlag(@"layout.avatar.hidden"));
    UIView *rail=avatar.superview;UIWindow *window=avatar.window;
    if(enabled && !hide && window && [rail isKindOfClass:objc_lookUpClass("SSShortVideoRightInteractionView")] &&
        !CGRectIsEmpty(rail.bounds) && CGRectGetMinX(avatar.frame)>=CGRectGetMaxX(rail.bounds)) {
        // The host parks this 46pt avatar at x=53 in a 46pt rail. Its follow
        // border then leaks a few pixels into the screen from the right.
        CGRect frame=[avatar convertRect:avatar.bounds toView:window];
        CGFloat edge=CGRectGetMaxX(window.bounds);
        hide=CGRectGetMinX(frame)<edge && CGRectGetMidX(frame)>=edge;
    }
    HGASetHidden(avatar,hide);
    NSMutableArray *parts=nil;
    if (hidePhoto || hideFollow) {
        parts=[NSMutableArray array];
        for (NSString *ivar in hidePhoto ? @[@"_avatarView",@"_circleBorder",@"_liveDecoration",@"_decorationView"] : @[]) {
            UIView *part=HGALazyControlSubview(avatar,ivar.UTF8String,NULL);
            if (part) [parts addObject:part];
        }
        for (NSString *ivar in hideFollow ? @[@"_followButtonBgView",@"_followImageView",@"_followedImageView",@"_followGuideBubbleView"] : @[]) {
            UIView *part=HGALazyControlSubview(avatar,ivar.UTF8String,NULL);
            if (part) [parts addObject:part];
        }
    }
    HGASetSuppressedFeedParts(avatar,parts);
}
static void HGASetGlassHostBackground(UIView *view,BOOL clear) {
    HGAViewState *state=HGAState(view,clear);
    if(clear) {
        if(!state.clearsBackground || ![view.backgroundColor isEqual:UIColor.clearColor])state.background=view.backgroundColor;
        state.clearsBackground=YES;
        if(![view.backgroundColor isEqual:UIColor.clearColor])view.backgroundColor=UIColor.clearColor;
    } else if(state.clearsBackground) {
        if([view.backgroundColor isEqual:UIColor.clearColor])view.backgroundColor=state.background;
        state.clearsBackground=NO;state.background=nil;
    }
}
static BOOL HGANeutralColorComponents(UIColor *color,UITraitCollection *traits,CGFloat *white,CGFloat *alpha) {
    CGFloat r=0,g=0,b=0;
    if(!color || ![[color resolvedColorWithTraitCollection:traits] getRed:&r green:&g blue:&b alpha:alpha] ||
        MAX(r,MAX(g,b))-MIN(r,MIN(g,b))>=0.10)return NO;
    *white=0.2126*r+0.7152*g+0.0722*b;
    return YES;
}
static void HGARestoreAdaptiveInkEntry(UIView *node,HGAAdaptiveInkEntry *entry) {
    if(!node || !entry)return;
    if([node isKindOfClass:UILabel.class]) {
        UILabel *label=(UILabel *)node;
        if(entry.appliedText && (label.attributedText==entry.appliedText || [label.attributedText isEqual:entry.appliedText])) {
            label.textColor=entry.sourceColor;
            if(entry.sourceText)label.attributedText=entry.sourceText;
        } else if(entry.appliedColor && [label.textColor isEqual:entry.appliedColor]) {
            label.textColor=entry.sourceColor;
        }
    } else if([node isKindOfClass:UIButton.class]) {
        UIButton *button=(UIButton *)node;
        if(entry.appliedColor && [[button titleColorForState:UIControlStateNormal] isEqual:entry.appliedColor])
            [button setTitleColor:entry.sourceColor forState:UIControlStateNormal];
    }
}
static void HGAApplyAdaptiveControlInk(UIView *view,UIUserInterfaceStyle appearance,BOOL enabled) {
    if(!view)return;
    HGAViewState *state=HGAState(view,enabled && appearance==UIUserInterfaceStyleDark);
    if(!enabled || appearance!=UIUserInterfaceStyleDark) {
        for(UIView *node in state.adaptiveInk.keyEnumerator.allObjects)
            HGARestoreAdaptiveInkEntry(node,[state.adaptiveInk objectForKey:node]);
        state.adaptiveInk=nil;
        return;
    }
    if(!state.adaptiveInk)state.adaptiveInk=[NSMapTable weakToStrongObjectsMapTable];
    NSHashTable<UIView *> *seen=[NSHashTable weakObjectsHashTable];
    for(UIView *node in HGADescendants(view,28)) {
        if([node isKindOfClass:UILabel.class]) {
            UILabel *label=(UILabel *)node;
            HGAAdaptiveInkEntry *entry=[state.adaptiveInk objectForKey:label];
            BOOL ownedColor=entry.appliedColor && [label.textColor isEqual:entry.appliedColor];
            BOOL ownedRich=entry.appliedText && (label.attributedText==entry.appliedText || [label.attributedText isEqual:entry.appliedText]);
            UIColor *baseColor=(ownedColor || ownedRich) ? entry.sourceColor : label.textColor;
            NSAttributedString *baseText=(ownedColor || ownedRich) ? entry.sourceText : label.attributedText;
            CGFloat white=0,alpha=0;
            __block BOOL changedRich=NO;
            __block NSMutableAttributedString *rewritten=nil;
            if(baseText.length) {
                [baseText enumerateAttribute:NSForegroundColorAttributeName inRange:NSMakeRange(0,baseText.length) options:0
                    usingBlock:^(id value,NSRange range,__unused BOOL *stop) {
                        UIColor *run=[value isKindOfClass:UIColor.class] ? value : baseColor;
                        CGFloat rw=0,ra=0;
                        if(!HGANeutralColorComponents(run,label.traitCollection,&rw,&ra) || rw>=0.66)return;
                        if(!rewritten)rewritten=[baseText mutableCopy];
                        [rewritten addAttribute:NSForegroundColorAttributeName
                            value:[UIColor colorWithWhite:(rw<0.24 ? 0.98 : 0.80) alpha:ra] range:range];
                        changedRich=YES;
                    }];
            }
            if(changedRich) {
                if(!entry){entry=[HGAAdaptiveInkEntry new];[state.adaptiveInk setObject:entry forKey:label];}
                entry.sourceColor=baseColor;entry.sourceText=[baseText copy];
                entry.appliedColor=(HGANeutralColorComponents(baseColor,label.traitCollection,&white,&alpha) && white<0.66) ?
                    [UIColor colorWithWhite:(white<0.24 ? 0.98 : 0.80) alpha:alpha] : baseColor;
                if(entry.appliedColor && ![label.textColor isEqual:entry.appliedColor])label.textColor=entry.appliedColor;
                label.attributedText=rewritten;
                entry.appliedText=[label.attributedText copy];
                [seen addObject:label];
            } else if(HGANeutralColorComponents(baseColor,label.traitCollection,&white,&alpha) && white<0.66) {
                if(!entry){entry=[HGAAdaptiveInkEntry new];[state.adaptiveInk setObject:entry forKey:label];}
                entry.sourceColor=baseColor;entry.sourceText=baseText;
                entry.appliedColor=[UIColor colorWithWhite:(white<0.24 ? 0.98 : 0.80) alpha:alpha];
                if(![label.textColor isEqual:entry.appliedColor])label.textColor=entry.appliedColor;
                entry.appliedText=label.attributedText;
                [seen addObject:label];
            }
        } else if([node isKindOfClass:UIButton.class]) {
            UIButton *button=(UIButton *)node;
            HGAAdaptiveInkEntry *entry=[state.adaptiveInk objectForKey:button];
            UIColor *current=[button titleColorForState:UIControlStateNormal];
            BOOL owned=entry.appliedColor && [current isEqual:entry.appliedColor];
            UIColor *baseColor=owned ? entry.sourceColor : current;
            CGFloat white=0,alpha=0;
            if(HGANeutralColorComponents(baseColor,button.traitCollection,&white,&alpha) && white<0.66) {
                if(!entry){entry=[HGAAdaptiveInkEntry new];[state.adaptiveInk setObject:entry forKey:button];}
                entry.sourceColor=baseColor;
                entry.appliedColor=[UIColor colorWithWhite:(white<0.24 ? 0.98 : 0.80) alpha:alpha];
                if(![current isEqual:entry.appliedColor])[button setTitleColor:entry.appliedColor forState:UIControlStateNormal];
                [seen addObject:button];
            }
        }
    }
    for(UIView *previous in state.adaptiveInk.keyEnumerator.allObjects) {
        if(![seen containsObject:previous]) {
            HGARestoreAdaptiveInkEntry(previous,[state.adaptiveInk objectForKey:previous]);
            [state.adaptiveInk removeObjectForKey:previous];
        }
    }
    if(!state.adaptiveInk.count)state.adaptiveInk=nil;
}
static void HGARemoveGlass(UIView *view) {
    HGAViewState *state=HGAState(view,NO);
    if(!state)return;
    if(state.adaptiveInk)HGAApplyAdaptiveControlInk(view,UIUserInterfaceStyleLight,NO);
    [state.glass removeFromSuperview];state.glass=nil;
    HGASetGlassHostBackground(view,NO);
}
static void HGAInstallGlassInRect(UIView *view,NSString *style,CGRect frame,CGFloat radius,CGFloat opacity) {
    if(!view.window || CGRectIsEmpty(frame)){HGARemoveGlass(view);return;}
    HGAViewState *state=HGAState(view,YES);
    HGASetGlassHostBackground(view,YES);
    if(!state.glass) {
        state.glass=[[HGAAppearanceMaterial alloc] initWithEffect:nil];
        state.glass.accessibilityIdentifier=@"honggo.appearance.material";
        state.glass.accessibilityElementsHidden=YES;
        state.glass.userInteractionEnabled=NO;
        state.glass.layer.cornerCurve=kCACornerCurveContinuous;
        state.glass.clipsToBounds=YES;
    }
    HGAAppearanceMaterial *glass=state.glass;
    glass.flexTouchHost=view;
    if(![glass.style isEqual:style] || fabs(glass.overlayOpacity-opacity)>0.001) {
        glass.style=style;glass.overlayOpacity=opacity;glass.materialSignature=nil;[glass setNeedsLayout];
    }
    if(!CGRectEqualToRect(glass.frame,frame))glass.frame=frame;
    CGFloat corner=MIN(radius,MIN(frame.size.width,frame.size.height)/2);
    glass.materialCornerRadius=corner;
    if(fabs(glass.layer.cornerRadius-corner)>0.001)glass.layer.cornerRadius=corner;
    UIViewAutoresizing mask=CGRectEqualToRect(frame,view.bounds) ?
        UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight : UIViewAutoresizingNone;
    if(glass.autoresizingMask!=mask)glass.autoresizingMask=mask;
    // Insert once. Reordering on every native layout can fight the host's own
    // background ordering indefinitely while opening the episode detail page.
    if(glass.superview!=view)[view insertSubview:glass atIndex:0];
    [glass requestMaterialRefresh];
}
static void HGAInstallGlass(UIView *view,NSString *style,CGFloat radius,CGFloat opacity) {
    HGAInstallGlassInRect(view,style,view.bounds,radius,opacity);
}
static void HGASetGlassOverlay(UIView *view,UIColor *color) {
    HGAState(view,NO).glass.overlayColor=color;
}
static void HGAInstallFlatPlaybackButton(UIView *view,CGRect frame) {
    CGFloat radius=MIN(CGRectGetWidth(frame),CGRectGetHeight(frame))/2;
    HGAInstallGlassInRect(view,@"solid",frame,radius,0);
    // The solid glyph keeps its contrast as the video beneath it changes.
    HGASetGlassOverlay(view,[UIColor colorWithWhite:0.16 alpha:UIAccessibilityIsReduceTransparencyEnabled() ? 1 : 0.74]);
}
static void HGAConfigureButtonGlassWithAppearance(HGAAppearanceMaterial *glass,NSString *style,UIUserInterfaceStyle appearance) {
    BOOL liquid=[style isEqual:@"clear"] || [style isEqual:@"regular"];
    glass.untintedGlass=liquid;
    if(!liquid) glass.clipsToBounds=YES;
    glass.interactiveGlass=liquid;
    if(glass.overrideUserInterfaceStyle!=appearance) {
        glass.overrideUserInterfaceStyle=appearance;
        if(@available(iOS 17.0,*))[glass updateTraitsIfNeeded];
    }
    glass.overlayColor=(UIAccessibilityIsReduceTransparencyEnabled() || [style isEqual:@"solid"]) ?
        UIColor.secondarySystemBackgroundColor : UIColor.clearColor;
    [glass requestMaterialRefresh];
}
static void HGAInstallButtonGlassWithAppearance(UIView *view,NSString *style,CGRect frame,CGFloat radius,UIUserInterfaceStyle appearance) {
    HGAInstallGlassInRect(view,style,frame,radius,0);
    HGAAppearanceMaterial *glass=HGAState(view,NO).glass;
    if(glass)HGAConfigureButtonGlassWithAppearance(glass,style,appearance);
}
static void HGAInstallButtonGlass(UIView *view,NSString *style,CGRect frame,CGFloat radius) {
    HGAInstallButtonGlassWithAppearance(view,style,frame,radius,UIUserInterfaceStyleDark);
}
static void HGAInstallAdaptiveButtonGlass(UIView *view,NSString *style,CGRect frame,CGFloat radius) {
    UIUserInterfaceStyle appearance=HGAAdaptivePageStyle(HGAOwner(view),view);
    HGAInstallButtonGlassWithAppearance(view,style,frame,radius,appearance);
    HGAAppearanceMaterial *glass=HGAState(view,NO).glass;
    if(glass)glass.lightContourRim=YES;
    HGAApplyAdaptiveControlInk(view,appearance,glass!=nil);
}
static NSDictionary<NSString *,NSString *> *HGAViewRoles(void) {
    static NSDictionary *roles;static dispatch_once_t once;
    dispatch_once(&once,^{roles=@{
        @"SSShortVideoRightInteractionAvatarView":@"avatar",
        @"SSShortVideoRightInteractionCollectView":@"collect",
        @"SSShortVideoRightInteractionCommentView":@"comment",
        @"SSShortVideoRightInteractionLikeView":@"like",
        @"SSShortVideoRightInteractionShareView":@"share",
        @"SSShortVideoRightInteractionMoreView":@"more",
        @"SSVideoSeriesFeedHeaderView":@"channels",
        @"SSShortVideoOutterSeriesInfoView":@"info",
        @"SSShortVideoDerivateInfoView":@"info",
        @"SSShortVideoDerivateLeftContainerView":@"info",
        @"SSShortVideoOutterSeriesLeftContainerView":@"info",
        @"FQVTagListView":@"tags",
        @"FQVShortVideoProgressView":@"progress",
        @"SSShortVideoDanmakuLeftAboveView":@"danmaku",
        @"FQCShortVideoDanmakuEntranceView":@"danmaku",
        @"SSShortVideoBottomDistributionTextView":@"episodes",
        @"SSShortVideoBottomDistributionView":@"episodes",
        @"SSShortVideoAiUsageView":@"notice"
    };});return roles;
}
static NSString *HGARole(UIView *view) {
    if(!view)return nil;
    Class viewCls=object_getClass(view);
    static NSMapTable *roleCache;
    if(NSThread.isMainThread) {
        if(!roleCache)roleCache=[NSMapTable mapTableWithKeyOptions:NSPointerFunctionsOpaqueMemory|NSPointerFunctionsObjectPointerPersonality
            valueOptions:NSPointerFunctionsStrongMemory];
        id cached=[roleCache objectForKey:viewCls];
        if(cached)return cached==NSNull.null ? nil : cached;
    }
    NSString *resolved=nil;
    NSDictionary<NSString *,NSString *> *roles=HGAViewRoles();
    for(Class cls=viewCls;cls && cls!=UIView.class;cls=class_getSuperclass(cls)) {
        NSString *role=roles[NSStringFromClass(cls)];
        if(role){resolved=role;break;}
    }
    if(NSThread.isMainThread)[roleCache setObject:resolved ?: NSNull.null forKey:viewCls];
    return resolved;
}
static BOOL HGAHasRoleAncestor(UIView *view,NSString *role) {
    UIView *parent=view.superview;
    for(NSUInteger i=0;parent && i<12;i++,parent=parent.superview)
        if([HGARole(parent) isEqual:role])return YES;
    return NO;
}
#include "AppearanceEpisode.inc"

@interface HGAIconState : NSObject
@property(nonatomic,weak) UIImageView *imageView;
@property(nonatomic,strong) UIImage *source;
@property(nonatomic,strong) UIImage *applied;
@property(nonatomic,strong) UIColor *tint;
@property(nonatomic,strong) UIColor *appliedTint;
@property(nonatomic) UIViewContentMode contentMode;
@property(nonatomic) UIViewContentMode appliedContentMode;
@property(nonatomic) BOOL ownsContentMode;
@property(nonatomic,strong) id sparkleIdentity;
@property(nonatomic,strong) id sparkleTapIdentity;
@property(nonatomic,strong) id sparkleAnimationIdentity;
@property(nonatomic) BOOL sparkleSelected;
@end
@implementation HGAIconState @end
static char HGAIconKey;
static NSString *const HGASparklePulse=@"honggo.collect.pulse";
static NSString *const HGASparkleFill=@"honggo.collect.fill";
static void HGARestoreIcon(UIView *root) {
    HGAIconState *s=objc_getAssociatedObject(root,&HGAIconKey);
    if(s.sparkleAnimationIdentity) {
        [s.imageView.layer removeAnimationForKey:HGASparklePulse];
        [s.imageView.layer removeAnimationForKey:HGASparkleFill];
    }
    HGARestoreGeometry(s.imageView);
    BOOL ownedImage=s.imageView.image==s.applied || [s.imageView.image isEqual:s.applied];
    if(s.appliedTint && [s.imageView.tintColor isEqual:s.appliedTint])s.imageView.tintColor=s.tint;
    if(s.ownsContentMode && s.imageView.contentMode==s.appliedContentMode)s.imageView.contentMode=s.contentMode;
    if(ownedImage)s.imageView.image=s.source;
    objc_setAssociatedObject(root,&HGAIconKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static UIImageView *HGAFindIcon(UIView *root) {
    if ([root isKindOfClass:objc_lookUpClass("SSShortVideoRightInteractionLikeView")]) {
        UIImageView *heart=(id)HGAViewGetter(root,"likeIcon",root);
        if ([heart isKindOfClass:UIImageView.class]) return heart.isAnimating ? nil : heart;
    }
    if ([root isKindOfClass:objc_lookUpClass("SSShortVideoRightInteractionCollectView")]) {
        UIImageView *star=(id)HGAViewGetter(root,"starIcon",root);
        if ([star isKindOfClass:UIImageView.class] && !star.isAnimating) return star;
    }
    const char *names[]={"iconImageView","imageView","iconView"};
    for(NSUInteger i=0;i<sizeof(names)/sizeof(names[0]);i++) {
        UIView *view=HGAViewGetter(root,names[i],root);
        if([view isKindOfClass:UIImageView.class] && ![(UIImageView *)view isAnimating])return (id)view;
    }
    // Restrict fallback to one unambiguous icon in this known action owner.
    UIImageView *result=nil;
    for(UIView *view in HGADescendants(root,36)) {
        if(![view isKindOfClass:UIImageView.class] || [(UIImageView *)view isAnimating])continue;
        CGFloat w=view.bounds.size.width,h=view.bounds.size.height;
        if(w<16 || h<16 || w>80 || h>80 || w/h<0.65 || w/h>1.6)continue;
        if(result)return nil;result=(id)view;
    }return result;
}
static CGFloat HGAActionGlassDiameter(UIView *root) {
    return MIN(44,MIN(CGRectGetWidth(root.bounds),CGRectGetHeight(root.bounds)));
}

static BOOL (*HGARightNativePointInside)(UIView *,SEL,CGPoint,UIEvent *);
static BOOL HGARightPointInside(UIView *rail,SEL cmd,CGPoint point,UIEvent *event) {
    if (HGARightNativePointInside(rail,cmd,point,event)) return YES;
    // A customized heart can be visible outside the host's 46pt rail. Let
    // UIKit descend into the original likeIcon and arbitrate its recognizer.
    if (rail.clipsToBounds || !rail.window) return NO;
    for (UIView *child in rail.subviews) {
        if (![child isKindOfClass:objc_lookUpClass("SSShortVideoRightInteractionLikeView")]) continue;
        HGAViewState *state=HGAState(child,NO);
        if (!state.geometry || child.hidden || child.alpha<=0.01 || !child.userInteractionEnabled) continue;
        UIView *icon=HGAViewGetter(child,"likeIcon",child);
        if (icon && [icon hitTest:[rail convertPoint:point toView:icon] withEvent:event]) return YES;
    }
    return NO;
}
static void HGAInstallRightHitArea(void) {
    if (HGARightNativePointInside) return;
    Class rail=objc_lookUpClass("SSShortVideoRightInteractionView");
    Method getter=class_getInstanceMethod(objc_lookUpClass("SSShortVideoRightInteractionLikeView"),sel_registerName("likeIcon"));
    if (rail && getter && !strcmp(method_getTypeEncoding(getter),"@16@0:8"))
        HGARightNativePointInside=(void *)HGInstallVerifiedHook(rail,@selector(pointInside:withEvent:),
            "B40@0:8{CGPoint=dd}16@32",(IMP)HGARightPointInside,@"appearance");
}

// Both states share the exact three outlines; only the ink and fill change.
static UIImage *HGASparkleImage(BOOL filled) {
    static UIImage *images[2];
    static dispatch_once_t once;
    dispatch_once(&once,^{
        UIGraphicsImageRenderer *renderer=[[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(32,32)];
        for (int variant=0;variant<2;variant++) {
            images[variant]=[[renderer imageWithActions:^(UIGraphicsImageRendererContext *context) {
                [UIColor.whiteColor setFill]; [UIColor.whiteColor setStroke];
                const CGFloat stars[][3]={{18.9,19.7,9.4},{7.2,10.4,4.6},{17.8,4.8,2.7}};
                for (NSUInteger i=0;i<3;i++) {
                    CGFloat x=stars[i][0],y=stars[i][1],s=stars[i][2],k=s*.18;
                    UIBezierPath *p=[UIBezierPath bezierPath];
                    [p moveToPoint:CGPointMake(x,y-s)];
                    [p addQuadCurveToPoint:CGPointMake(x+s,y) controlPoint:CGPointMake(x+k,y-k)];
                    [p addQuadCurveToPoint:CGPointMake(x,y+s) controlPoint:CGPointMake(x+k,y+k)];
                    [p addQuadCurveToPoint:CGPointMake(x-s,y) controlPoint:CGPointMake(x-k,y+k)];
                    [p addQuadCurveToPoint:CGPointMake(x,y-s) controlPoint:CGPointMake(x-k,y-k)];
                    [p closePath]; p.lineWidth=1.4; p.lineJoinStyle=kCGLineJoinRound;
                    if (variant) [p fill];
                    else [p stroke];
                }
            }] imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
        }
    });
    return images[filled ? 1 : 0];
}

static id HGACollectIdentity(id model) {
    id vid=HGReadObjectGetter(model,"vid");
    return [vid isKindOfClass:NSString.class] && [vid length] ? vid : model;
}

static void HGAApplyIcon(UIView *root,NSString *role,BOOL flat,BOOL sparkles,BOOL animateSparkles) {
    if(![@[@"like",@"collect",@"comment",@"share"] containsObject:role])return;
    if(!HGAppearanceEnabled() || (!flat && !sparkles) || !root.window) {HGARestoreIcon(root);return;}
    BOOL selected=NO;
    BOOL collect=[role isEqual:@"collect"];
    id model=collect ? HGReadObjectGetter(root,"model") : nil;
    if([role isEqual:@"like"] || collect) {
        // The host's refresh assigns _model directly. Its separate Boolean
        // on the view is not updated by the success callback.
        BOOL known=collect ? (HGABoolGetter(model,@"isCollected",&selected) ||
            HGABoolGetter(root,@"isCollected",&selected)) :
            (HGABoolGetter(root,@"isLiked",&selected) ||
             HGABoolGetter(HGReadObjectGetter(root,"model"),@"isLiked",&selected));
        if(!known) {
            HGARestoreIcon(root);
            HGRecordCapability([@"appearance.icon." stringByAppendingString:role],
                @{@"module":@"appearance",@"state":@"unavailable",@"reason":@"verified host selection getter unavailable; native image retained"});
            return;
        }
    }
    UIImageView *icon=HGAFindIcon(root);
    HGAIconState *state=objc_getAssociatedObject(root,&HGAIconKey);
    if(state.imageView!=icon){HGARestoreIcon(root);state=nil;}
    CGFloat requested=MIN(28 * 0.94,HGAActionGlassDiameter(root)*0.64);
    BOOL heart=flat && [role isEqual:@"like"];
    if (!icon || (heart && requested<=0)) { HGARestoreIcon(root); return; }
    UIImage *image=sparkles ? HGASparkleImage(selected)
        : HGAFlatActionImage(role,selected,heart ? requested : 32);
    if(!image){HGARestoreIcon(root);return;}
    if(!state){state=[HGAIconState new];state.imageView=icon;state.source=icon.image;state.tint=icon.tintColor;
        objc_setAssociatedObject(root,&HGAIconKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);}
    else if(icon.image!=state.applied && ![icon.image isEqual:state.applied])state.source=icon.image;
    id identity=sparkles ? HGACollectIdentity(model) : nil;
    if (state.sparkleTapIdentity && ![state.sparkleTapIdentity isEqual:identity]) state.sparkleTapIdentity=nil;
    if (state.sparkleAnimationIdentity && (!selected || ![state.sparkleAnimationIdentity isEqual:identity])) {
        [icon.layer removeAnimationForKey:HGASparklePulse];
        [icon.layer removeAnimationForKey:HGASparkleFill];
        state.sparkleAnimationIdentity=nil;
    }
    state.sparkleIdentity=identity;
    state.sparkleSelected=selected;
    UIColor *color=sparkles && selected ? UIColor.systemBlueColor : HGAActionIconColor(role,selected);
    CGFloat side=MAX(icon.bounds.size.width,icon.bounds.size.height);
    if (side>0) {
        CGFloat scale=sparkles ? MIN(1,requested/side) : (flat && !heart ? MIN(1.5,requested/side) : 1);
        HGASetGeometry(icon,0,0,scale);
    }
    if (animateSparkles && identity && selected && !UIAccessibilityIsReduceMotionEnabled()) {
        state.sparkleAnimationIdentity=identity;
        CGFloat scale=hypot(icon.transform.a,icon.transform.b);
        CATransition *fill=[CATransition animation]; fill.type=kCATransitionFade; fill.duration=.16;
        [icon.layer addAnimation:fill forKey:HGASparkleFill];
        CAKeyframeAnimation *pop=[CAKeyframeAnimation animationWithKeyPath:@"transform.scale"];
        pop.values=@[@(scale*.86),@(scale*1.15),@(scale)];
        pop.keyTimes=@[@0,@.45,@1]; pop.duration=.32;
        [icon.layer addAnimation:pop forKey:HGASparklePulse];
    }
    if(!state.appliedTint || ![icon.tintColor isEqual:state.appliedTint])state.tint=icon.tintColor;
    state.appliedTint=color;
    if(![icon.tintColor isEqual:color])icon.tintColor=color;
    if(!state.ownsContentMode || icon.contentMode!=state.appliedContentMode)state.contentMode=icon.contentMode;
    state.ownsContentMode=YES;
    state.appliedContentMode=heart ? UIViewContentModeCenter : UIViewContentModeScaleAspectFit;
    if(icon.contentMode!=state.appliedContentMode)icon.contentMode=state.appliedContentMode;
    if(icon.image!=image && ![icon.image isEqual:image])icon.image=image;
    state.applied=icon.image;
}

@interface HGATextState : NSObject
@property(nonatomic,strong) UIFont *source;
@property(nonatomic,strong) UIFont *applied;
@property(nonatomic,copy) NSAttributedString *richSource;
@property(nonatomic,copy) NSAttributedString *richApplied;
@end
@implementation HGATextState @end
static char HGATextKey;
static NSHashTable<UIView *> *HGATexts;
static UIFont *HGAGetFont(UIView *view) {
    if([view isKindOfClass:UILabel.class])return [(UILabel *)view font];
    if([view isKindOfClass:UITextView.class])return [(UITextView *)view font];
    if([view isKindOfClass:UITextField.class])return [(UITextField *)view font];
    return nil;
}
static NSAttributedString *HGAGetRichText(UIView *view) {
    if([view isKindOfClass:UILabel.class])return [(UILabel *)view attributedText];
    if([view isKindOfClass:UITextView.class])return [(UITextView *)view attributedText];
    if([view isKindOfClass:UITextField.class])return [(UITextField *)view attributedText];
    return nil;
}
static void HGAWriteFont(UIView *view,UIFont *font) {
    if([view isKindOfClass:UILabel.class])[(UILabel *)view setFont:font];
    else if([view isKindOfClass:UITextView.class])[(UITextView *)view setFont:font];
    else if([view isKindOfClass:UITextField.class])[(UITextField *)view setFont:font];
}
static void HGAWriteRichText(UIView *view,NSAttributedString *text) {
    if([view isKindOfClass:UILabel.class])[(UILabel *)view setAttributedText:text];
    else if([view isKindOfClass:UITextView.class])[(UITextView *)view setAttributedText:text];
    else if([view isKindOfClass:UITextField.class])[(UITextField *)view setAttributedText:text];
}
static UIFont *HGAConfiguredFont(UIFont *source) {
    if(!source)return nil;
    NSString *name=source.fontName.lowercaseString;
    if([name containsString:@"icon"] || [name containsString:@"symbol"] || [name containsString:@"emoji"] || [name containsString:@"awesome"])return source;
    CGFloat size=MAX(8,MIN(72,source.pointSize*HGAppearanceNumber(@"fonts.scale")));
    NSString *selected=HGAppearanceString(@"fonts.name");
    UIFont *font=selected.length ? [UIFont fontWithName:selected size:size] : nil;
    return font ?: [source fontWithSize:size];
}
static void HGARestoreText(UIView *view) {
    HGATextState *s=objc_getAssociatedObject(view,&HGATextKey);if(!s)return;
    if(s.richApplied && [HGAGetRichText(view) isEqual:s.richApplied]) HGAWriteRichText(view,s.richSource);
    else if(!s.richApplied && [HGAGetFont(view) isEqual:s.applied])HGAWriteFont(view,s.source);
    objc_setAssociatedObject(view,&HGATextKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [HGATexts removeObject:view];
}
static void HGAApplyTypography(UIView *root) {
    BOOL enabled=HGAppearanceEnabled() && HGAppearanceFlag(@"fonts.enabled") && root.window;
    if(!enabled && !HGATexts.count)return;
    for(UIView *view in HGADescendants(root,180)) {
        UIFont *font=HGAGetFont(view);if(!font)continue;
        if([view isKindOfClass:UITextField.class] && ([(UITextField *)view isSecureTextEntry] || view.isFirstResponder))continue;
        if([view isKindOfClass:UITextView.class] && ([(UITextView *)view isEditable] || view.isFirstResponder))continue;
        if(!enabled){HGARestoreText(view);continue;}
        HGATextState *s=objc_getAssociatedObject(view,&HGATextKey);
        NSAttributedString *rich=HGAGetRichText(view);
        if(s && rich && ![rich isEqual:s.richApplied]) {s.richSource=rich;s.source=font;}
        else if(s && !rich && ![font isEqual:s.applied])s.source=font;
        if(!s){s=[HGATextState new];s.source=font;s.richSource=rich;
            objc_setAssociatedObject(view,&HGATextKey,s,OBJC_ASSOCIATION_RETAIN_NONATOMIC);}
        if(!HGATexts)HGATexts=[NSHashTable weakObjectsHashTable];[HGATexts addObject:view];
        if(rich.length && s.richSource.length) {
            NSMutableAttributedString *copy=[s.richSource mutableCopy];
            [s.richSource enumerateAttribute:NSFontAttributeName inRange:NSMakeRange(0,s.richSource.length) options:0
                usingBlock:^(id value,NSRange range,BOOL *stop) {
                    if([value isKindOfClass:UIFont.class]) [copy addAttribute:NSFontAttributeName value:HGAConfiguredFont(value) range:range];
                }];
            s.richApplied=copy;
            if(![rich isEqual:copy])HGAWriteRichText(view,copy);
        }else {
            s.richSource=nil;s.richApplied=nil;s.applied=HGAConfiguredFont(s.source);
            if(![font isEqual:s.applied])HGAWriteFont(view,s.applied);
        }
    }
}

#include "AppearancePlayer.inc"
static NSDictionary<NSString *,NSString *> *HGAPanelControllers(void) {
    static NSDictionary<NSString *,NSString *> *controllers;
    static dispatch_once_t once;
    dispatch_once(&once, ^{controllers=@{@"SSShortVideoCommentPanelViewController":@"comment",
        @"FQCShortVideoCommentListViewController":@"comment",
        @"FQCShortVideoCommentPanelViewController":@"comment",
        @"FQVShortVideoCommentPanelViewController":@"comment",
        @"FQVShortVideoSettingsViewController":@"longpress",
        @"FQVShortVideoNewSpeedSelectController":@"speed",
        @"SSUGKmpSharePanelContainerViewController":@"share",
        @"SSVideoSharePanelViewController":@"share",
        @"SSVideoSharePanelControllerViewController":@"share",
        @"SSShortVideoSharePanelViewController":@"share",
        @"SSSharePanelViewController":@"share",
        @"UGKmpSharePanelContainerViewController":@"share",
        @"SSShortVideoIntroductionTabViewController":@"episodes",
        @"SSShortVideoIntroductionSubViewController":@"episode-page",
        @"SSShortVideoEpisodeSubViewController":@"episode-page"};});
    return controllers;
}
static NSString *HGAPanelKind(UIViewController *controller) {
    for(Class cls=object_getClass(controller);cls && cls!=UIViewController.class;cls=class_getSuperclass(cls)) {
        NSString *kind=HGAPanelControllers()[NSStringFromClass(cls)];
        if(kind)return kind;
    }
    return nil;
}
static NSArray<NSString *> *HGACommentSurfaces(void) {
    return @[@"FQCShortVideoCommentPanelTitleView", @"FQCShortVideoCommentTitleView",
        @"FQCShortVideoCommentListCollectionView", @"FQCShortVideoBottomToolBar",
        @"FQCShortVideoNewEditorView"];
}
static NSArray<NSString *> *HGACommentRows(void) {
    return @[@"FQCShortVideoCommentCommonCell",@"FQCShortVideoCommentReplyCell",
        @"FQCShortVideoCommentSectionFooterView"];
}
#include "AppearanceShare.inc"
#include "AppearancePlayerControls.inc"
#include "AppearancePageChrome.inc"
#include "AppearanceChromeGeometry.inc"
#include "AppearanceLandscapeChrome.inc"

// Limit the preference to the separate left-edge disclosure inside a playback
// canvas. Episode titles, subtitles and the top filing number can also say AI.
static void HGAUpdateFictionNotice(UIView *canvas) {
    static char hiddenLabelsKey;
    NSHashTable<UILabel *> *previous=objc_getAssociatedObject(canvas,&hiddenLabelsKey);
    BOOL enabled=canvas.window && HGAppearanceEnabled() && HGAppearanceFlag(@"player.hideAIFictionNotice");
    if(!previous && !enabled)return;
    NSHashTable<UILabel *> *matched=nil;
    if(enabled) {
        for(UIView *node in HGADescendants(canvas,512)) {
            if(![node isKindOfClass:UILabel.class])continue;
            if(CGRectGetHeight(node.bounds)<=CGRectGetWidth(node.bounds))continue;
            UILabel *label=(UILabel *)node;
            NSString *text=label.attributedText.string ?: label.text;
            if(![text containsString:@"虚"] && ![text containsString:@"演"])continue;
            text=[[text.lowercaseString componentsSeparatedByCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet] componentsJoinedByString:@""];
            BOOL fiction=[text containsString:@"虚构"] || [text containsString:@"剧情演绎"];
            if(!fiction || (![text containsString:@"剧情"] && ![text containsString:@"ai"]))continue;
            CGRect rect=[label convertRect:label.bounds toView:canvas];
            if(CGRectGetMaxX(rect)>CGRectGetMinX(canvas.bounds)+CGRectGetWidth(canvas.bounds)*0.20 ||
               CGRectGetHeight(rect)<=CGRectGetWidth(rect))continue;
            if(!matched)matched=[NSHashTable weakObjectsHashTable];
            [matched addObject:label];
            HGASetHidden(label,YES);
        }
    }
    for(UILabel *label in previous)
        if(![matched containsObject:label])HGASetHidden(label,NO);
    if(matched || previous)objc_setAssociatedObject(canvas,&hiddenLabelsKey,matched,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

void HGAppearanceProcessView(UIView *view) {
    if(!NSThread.isMainThread || HGAApplying || ![view isKindOfClass:UIView.class] ||
       HGAOwnView(view) || HGFeedSidebarOwnsLayout(view.window))return;
    HGAApplying=YES;
    @try {
        HGARegisterView(view);
        if([view isKindOfClass:objc_lookUpClass("FQVShortVideoProgressView")]) {
            BOOL enabled=HGAppearanceEnabled() && view.window;
            HGASetHidden(view,enabled && !HGAHasRoleAncestor(view,@"progress") &&
                HGAppearanceFlag(@"layout.progress.hidden"));
            HGAUpdateFeedProgressWidth(view);
            if(HGAState(view,NO).glass)HGARemoveGlass(view);
            return;
        }
        HGAApplyPlayerControls(view);
        HGALandscapeApplyView(view);
        NSString *name=NSStringFromClass(view.class),*role=HGARole(view);
        if([name isEqual:@"SSTabBar"]){HGARefreshDock(view);return;}
        if ([view isKindOfClass:objc_lookUpClass("FQVCollectionView")] && [view isKindOfClass:UIScrollView.class]) {
            UIScrollView *feed = (UIScrollView *)view;
            HGAObserveFeed(feed);
            if (feed.window) {
                HGARegisterCategoryFeed(feed);
                HGScheduleFeedLayoutRefresh(feed);
            }
            else HGRefreshVideoBackdropForFeed(feed);
            return;
        }
        if([name isEqual:@"TTPlayerView"]){
            HGAApplyFullScreen(view);HGRefreshVideoBackdropForPlayer(view);return;
        }
        if([name isEqual:@"FQVShortVideoPlayerCellView"]){
            HGAUpdateFictionNotice(view);
            if((HGAppearanceFlag(@"player.fullscreen") && view.window && HGAPlayerLandscapeSize(view.window.bounds.size)) ||
               HGAPlayerState(view,NO).hasChildFill) {
                for(UIView *child in HGADescendants(view,100))
                    if([NSStringFromClass(child.class) isEqual:@"TTPlayerView"])HGAApplyFullScreen(child);
            }
            HGAApplyTypography(view);HGRefreshVideoBackdropForPlayer(view);return;
        }
        if([name isEqual:@"FQVShortVideoLoadingView"]) {
            HGAApplyLoadingSpinner(view);return;
        }
        if([view isKindOfClass:objc_lookUpClass("SSShortVideoDistributionContentBaseView")] ||
           [view isKindOfClass:objc_lookUpClass("SSShortVideoDistributionContentView")]) {
            for(UIView *parent=view.superview;parent;parent=parent.superview) {
                if([HGARole(parent) isEqual:@"episodes"] && !HGAHasRoleAncestor(parent,@"episodes")) {
                    BOOL epStyled=HGAppearanceEnabled() && parent.window && HGAppearanceFlag(@"player.episodeGlass") &&
                        (!parent.hidden || (!CGRectIsEmpty(parent.bounds) && !HGAppearanceFlag(@"layout.episodes.hidden")));
                    HGAApplyEpisodeLabels(parent,HGAEpisodeGlassFrame(parent),epStyled);
                    break;
                }
            }
            return;
        }
        if([view isKindOfClass:objc_lookUpClass("FQVShortVideoGradientLayerView")]) {
            if(view.window)HGRefreshVideoBackdropForPlayer(view);
            return;
        }
        if([view isKindOfClass:objc_lookUpClass("FQVShortVideoVPlayerSingleSeriesControlView")]) {
            HGRefreshVideoBackdropForPlayer(view);return;
        }
        if(HGIsAlbumDetailControl(view)) HGRefreshVideoBackdropForPlayer(view);
        if([view isKindOfClass:objc_lookUpClass("SSShortVideoRecommendPeopleCardView")]) {
            HGASetHidden(view,HGAppearanceEnabled() && HGAppearanceFlag(@"player.hideFollowRecommendUsers"));
            return;
        }
        if(HGAIsCommentRow(view)) {
            HGAUpdateCommentRow(view);return;
        }
        if(HGAIsCommentSurface(view)) {
            HGAUpdateCommentSurface(view);
            HGAApplyTypography(view);return;
        }
        if(HGAIsActionPanelSurface(view)) {HGAUpdateActionPanelSurface(view);return;}
        BOOL enabled=HGAppearanceEnabled() && view.window;
        BOOL header=HGAIsHeaderView(view);
        BOOL headerShowsFinder=header && HGAApplyHeaderChrome(view,enabled);
        if([view isKindOfClass:objc_lookUpClass("SSShortVideoRightInteractionView")]) {
            // A reused rail may reposition a hidden avatar without resizing
            // it. Reconcile its visibility after the parent's native layout.
            for(UIView *child in view.subviews)
                if([child isKindOfClass:objc_lookUpClass("SSShortVideoRightInteractionAvatarView")])
                    HGAApplyRailAvatarVisibility(child,enabled);
        }
        if(!role){HGAApplyNativePageChrome(view,name,enabled);return;}
        BOOL outer=!HGAHasRoleAncestor(view,role);
        NSString *prefix=[@"layout." stringByAppendingString:role];
        BOOL avatar=[role isEqual:@"avatar"];
        BOOL hideOwner=[role isEqual:@"info"] ? [NSStringFromClass(view.class) hasSuffix:@"LeftContainerView"] : outer;
        if(!avatar)HGASetHidden(view,enabled && hideOwner && (HGAppearanceFlag([prefix stringByAppendingString:@".hidden"]) ||
            (header && !headerShowsFinder && HGAAllTopControlsHidden())));
        if([role isEqual:@"progress"])HGAUpdateFeedProgressWidth(view);
        else HGASetGeometry(view,enabled && outer?HGAppearanceNumber([prefix stringByAppendingString:@".x"]):0,
            enabled && outer?HGAppearanceNumber([prefix stringByAppendingString:@".y"]):0,
            enabled && outer?HGAppearanceNumber([prefix stringByAppendingString:@".scale"]):1);
        if(avatar)HGAApplyRailAvatarVisibility(view,enabled && outer);
        BOOL action=[@[@"avatar",@"collect",@"comment",@"like",@"share",@"more"] containsObject:role];
        if(action && !avatar)HGAApplyFeedActionCaption(view,role,enabled && outer);
        BOOL styleActions=action && HGAppearanceFlag(@"player.rightGlass") &&
            !(avatar && HGAppearanceFlag(@"player.hideFeedAvatarPhoto"));
        BOOL flat=enabled && outer && styleActions && HGFlatPlayerStyle();
        BOOL glass=enabled && outer && ((styleActions && !flat) ||
            ([role isEqual:@"channels"] && !headerShowsFinder && HGAppearanceFlag(@"player.headerGlass")) ||
            ([role isEqual:@"episodes"] && HGAppearanceFlag(@"player.episodeGlass")));
        BOOL episode=[role isEqual:@"episodes"];
        // Keep the episode glass and inset alive across transient host hidden toggles
        // (such as channel swipes or cell rebinding) while attached to a live window.
        BOOL episodeStyled=episode && glass && (!view.hidden ||
            (outer && view.window && !CGRectIsEmpty(view.bounds) && !HGAppearanceFlag(@"layout.episodes.hidden")));
        if(episode && outer) {
            HGAApplyEpisodeInset(view,episodeStyled);
            HGAUpdateFeedProgressWidth(view);
        }
        CGRect episodeFrame=episode ? HGAEpisodeGlassFrame(view) : CGRectZero;
        CGFloat episodeRadius=MIN(view.bounds.size.width,view.bounds.size.height)/2;
        if(episode)HGAApplyEpisodeCorners(view,episodeStyled,episodeRadius);
        BOOL sparkles=enabled && outer && [role isEqual:@"collect"] &&
            [HGAppearanceString(@"player.collectGlyph") isEqual:@"sparkles"];
        HGAApplyIcon(view,role,flat,sparkles,NO);
        if(header) {
            HGAApplyHeaderSeparatedGlass(view,glass,enabled && outer && !headerShowsFinder && !view.hidden,
                HGAppearanceString(@"player.material"));
        } else if(episodeStyled || (!sparkles && (glass || (flat && ![role isEqual:@"avatar"] && ![role isEqual:@"more"])) && !view.hidden)) {
            CGRect glassFrame=episode ? episodeFrame : view.bounds;
            CGFloat radius=episode ? MIN(episodeFrame.size.width,episodeFrame.size.height)/2 : 20;
            if(action) {
                UIImageView *icon=HGAFindIcon(view);
                CGRect rect=icon ? [icon convertRect:icon.bounds toView:view] : CGRectMake(0,0,view.bounds.size.width,MIN(52,view.bounds.size.height));
                CGFloat width=CGRectGetWidth(view.bounds),height=CGRectGetHeight(view.bounds);
                CGFloat side=HGAActionGlassDiameter(view);
                if([role isEqual:@"avatar"])side=MIN(side,MAX(34,MIN(40,MAX(rect.size.width,rect.size.height)+8)));
                else if([role isEqual:@"more"])side=MIN(side,38);
                CGFloat x=MIN(CGRectGetMinX(view.bounds)+width-side,
                    MAX(CGRectGetMinX(view.bounds),CGRectGetMidX(rect)-side/2));
                CGFloat y=MIN(CGRectGetMinY(view.bounds)+height-side,
                    MAX(CGRectGetMinY(view.bounds),CGRectGetMidY(rect)-side/2));
                glassFrame=CGRectMake(x,y,side,side);
                radius=side/2;
            }
            NSString *style=action ? HGALiquidButtonMaterial() : HGAppearanceString(@"player.material");
            if(episode && HGAUsesNativeGlass(style))style=HGALiquidButtonMaterial();
            if(flat)HGAInstallFlatPlaybackButton(view,glassFrame);
            else {
                HGAInstallButtonGlass(view,style,glassFrame,radius);
                if(episode) {
                    HGAAppearanceMaterial *g=HGAState(view,NO).glass;
                    if(g.window && !g.effect && !CGRectIsEmpty(g.bounds))[g layoutIfNeeded];
                    HGALogEpisodeGlassState(view,g,view.hidden ? @"kept-hidden" : @"active");
                }
            }
        }
        else {
            HGARemoveGlass(view);
            if(episode && outer)HGALogEpisodeGlassState(view,nil,!view.window ? @"detached" : (!glass ? @"disabled" : @"hidden"));
        }
        HGAApplyTypography(view);
        if(episode && outer)HGAApplyEpisodeLabels(view,episodeFrame,episodeStyled);
    }@finally {
        @try {
            HGAUpdateBottomLayoutForView(view);
            HGAApplyCleanView(view);
        }
        @finally {HGAApplying=NO;}
    }
}
void HGAppearanceProcessController(UIViewController *controller) {
    if(!NSThread.isMainThread || HGAApplying || ![controller isKindOfClass:UIViewController.class] || !controller.isViewLoaded)return;
    HGAApplying=YES;
    @try {
        if(!HGAControllers)HGAControllers=[NSHashTable weakObjectsHashTable];[HGAControllers addObject:controller];
        NSString *name=NSStringFromClass(controller.class);
        if([name isEqual:@"SSTabBarController"]) {
            if(HGFeedSidebarOwnsLayout(controller.viewIfLoaded.window))return;
            UIView *bar=HGAViewGetter(controller,"customTabBar",controller.view);
            if(bar)HGAppearanceLayoutDock(bar,controller);
            return;
        }
        if(HGAIsFinderController(controller)){HGAApplyFinderChrome(controller);return;}
        if(HGAIsMineKMPController(controller)){HGAApplyMineKMPChrome(controller);return;}
        NSString *kind=HGAPanelKind(controller);
        if([kind isEqual:@"episodes"])HGAApplyEpisodeController(controller);
        else if([kind isEqual:@"episode-page"])HGARefreshEpisodePage(controller);
        else if(kind){HGAApplyActionSheet(controller.view,kind);HGAApplyTypography(controller.view);}
    }@finally {HGAApplying=NO;}
}
// The feed already supplies this episode's count before the separate comment
// request completes. Both native request strategies can display a placeholder
// in the meantime. Use the owner's formatter without changing its request/model.
static BOOL HGAReadCommentCountSelector(id object,const char *selectorName,NSInteger *outCount) {
    if(!object || !selectorName)return NO;
    SEL getter=sel_registerName(selectorName);
    Method method=class_getInstanceMethod(object_getClass(object),getter);
    const char *enc=method ? method_getTypeEncoding(method) : NULL;
    if(!enc)return NO;
    if(!strcmp(enc,"q16@0:8") || !strcmp(enc,"Q16@0:8")) {
        *outCount=((NSInteger(*)(id,SEL))objc_msgSend)(object,getter);
        return YES;
    }
    if(!strcmp(enc,"i16@0:8") || !strcmp(enc,"I16@0:8")) {
        *outCount=(NSInteger)((int(*)(id,SEL))objc_msgSend)(object,getter);
        return YES;
    }
    if(!strcmp(enc,"@16@0:8")) {
        id val=((id(*)(id,SEL))objc_msgSend)(object,getter);
        if([val isKindOfClass:NSNumber.class]) { *outCount=[val integerValue]; return YES; }
        if([val isKindOfClass:NSString.class] && [val length]) {
            NSInteger parsed=[val integerValue];
            if(parsed>0 || [val isEqualToString:@"0"]) { *outCount=parsed; return YES; }
        }
    }
    return NO;
}
static BOOL HGAReadCommentCountValue(id object,NSInteger *outCount) {
    if(!object)return NO;
    const char *selectors[]={"commentCount","episodeCommentCount","totalCommentCount","commentNum"};
    for(NSUInteger i=0;i<sizeof(selectors)/sizeof(selectors[0]);i++) {
        NSInteger val=0;
        if(HGAReadCommentCountSelector(object,selectors[i],&val)) {
            *outCount=val;
            if(val>0)return YES;
        }
    }
    return NO;
}
static NSInteger HGAPrimedCommentCountFromModel(id model) {
    if(!model)return 0;
    const char *paths[]={"videoData","episodeVideoData","seriesVideoData","derivateVideoData",
        "filterableEpisodeVideoData","filterableSeriesVideoData","albumVideoData","pugcVideoData",
        "shortVideoModel","videoInfo","statistics","statInfo","commentInfo"};
    const char *nested[]={"shortVideoModel","videoInfo","statistics","statInfo","commentInfo"};
    for(NSUInteger i=0;i<sizeof(paths)/sizeof(paths[0]);i++) {
        id sub=HGReadObjectGetter(model,paths[i]);
        if(!sub)continue;
        NSInteger count=0;
        if(HGAReadCommentCountValue(sub,&count) && count>0)return count;
        for(NSUInteger j=0;j<sizeof(nested)/sizeof(nested[0]);j++) {
            NSInteger innerCount=0;
            if(HGAReadCommentCountValue(HGReadObjectGetter(sub,nested[j]),&innerCount) && innerCount>0)
                return innerCount;
        }
    }
    NSInteger direct=0;
    if(HGAReadCommentCountValue(model,&direct) && direct>0)return direct;
    return 0;
}
static NSInteger HGAPrimedCommentCount(id manager) {
    if(![manager isKindOfClass:objc_lookUpClass("SSShortVideoRightInteractionCommentViewManager")] ||
       HGReadObjectGetter(manager,"countModel"))return 0;
    BOOL supported=NO;
    if(!HGABoolGetter(manager,@"supportComment",&supported) || !supported)return 0;
    id model=HGReadObjectGetter(manager,"model");
    id active=HGReadObjectGetter(model,"videoDataModel");
    NSInteger count=0;
    // The interaction wrapper can still be using its initial episode data.
    // The bound feed model already owns this item's short-video metadata.
    id feedData=HGReadObjectGetter(HGReadObjectGetter(active,"shortVideoModel"),"videoData");
    if(HGAReadCommentCountValue(feedData,&count))return count;
    count=HGAPrimedCommentCountFromModel(model);
    if(count>0)return count;

    id preload=HGReadObjectGetter(HGReadObjectGetter(active,"preloadShortVideoModel"),"videoData");
    id currentID=HGReadObjectGetter(feedData,"vid") ?: HGReadObjectGetter(active,"videoId");
    if(currentID && [currentID isEqual:HGReadObjectGetter(preload,"vid")] &&
       HGAReadCommentCountValue(preload,&count))return count;
    return 0;
}
static void HGAPrimeRightCommentCount(id manager,UIView *mountedView) {
    if(!mountedView) {
        // The native commentView getter creates its view on access. Initial
        // binding can precede that; the mounted view gets its own first pass.
        Ivar ivar=class_getInstanceVariable(object_getClass(manager),"_commentView");
        if(ivar && ivar_getTypeEncoding(ivar)[0]=='@')mountedView=object_getIvar(manager,ivar);
    }
    if(![mountedView isKindOfClass:objc_lookUpClass("SSShortVideoRightInteractionCommentView")])return;
    NSInteger count=HGAPrimedCommentCount(manager);
    if(count<=0)return;
    SEL shown=sel_registerName("commentCount"),paint=sel_registerName("updateCommentLabelWithCount:");
    Class viewClass=object_getClass(mountedView);
    Method countMethod=class_getInstanceMethod(viewClass,shown);
    if(countMethod && !strcmp(method_getTypeEncoding(countMethod),"d16@0:8") &&
       ((double(*)(id,SEL))objc_msgSend)(mountedView,shown)==(double)count)return;
    Method paintMethod=class_getInstanceMethod(viewClass,paint);
    if(paintMethod && !strcmp(method_getTypeEncoding(paintMethod),"v24@0:8q16"))
        ((void(*)(id,SEL,NSInteger))objc_msgSend)(mountedView,paint,count);
}
