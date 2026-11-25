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
@property(nonatomic) CGFloat materialCornerRadius;
@property(nonatomic,strong) CALayer *gaussianBackdrop;
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

static UIUserInterfaceStyle HGAVideoPageStyle(UIViewController *controller,UIView *bar) {
    // Playback's white controls and black video backing stay the same in both app themes.
    static id themeManager;
    if(!themeManager) {
        Protocol *adapter=objc_getProtocol("SSAdaptiveUIColorProtocol");
        id center=adapter ? HGReadObjectGetter(objc_lookUpClass("HTSServiceCenter"),"defaultCenter") : nil;
        SEL getter=sel_registerName("getProtocolService:");
        Method method=center ? class_getInstanceMethod(object_getClass(center),getter) : NULL;
        if(method && !strcmp(method_getTypeEncoding(method),"@24@0:8@16"))
            themeManager=((id (*)(id,SEL,Protocol *))objc_msgSend)(center,getter,adapter);
    }
    BOOL dark;
    if(HGABoolGetter(themeManager,@"isCurrentDarkMode",&dark))
        return dark ? UIUserInterfaceStyleDark : UIUserInterfaceStyleLight;
    UIUserInterfaceStyle appStyle=controller.viewIfLoaded.traitCollection.userInterfaceStyle;
    if(appStyle==UIUserInterfaceStyleUnspecified)appStyle=bar.window.traitCollection.userInterfaceStyle;
    return appStyle!=UIUserInterfaceStyleUnspecified ? appStyle : bar.traitCollection.userInterfaceStyle;
}

@implementation HGAAppearanceMaterial
- (void)setMaterialCornerRadius:(CGFloat)radius {
    if(fabs(_materialCornerRadius-radius)<0.001)return;
    _materialCornerRadius=radius;
    [self setNeedsLayout];
}
- (void)setUntintedGlass:(BOOL)untintedGlass {
    if(_untintedGlass==untintedGlass)return;
    _untintedGlass=untintedGlass;self.materialSignature=nil;[self setNeedsLayout];
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
- (NSString *)currentMaterialSignature {
    return [NSString stringWithFormat:@"%@/%d/%d/%ld/%d/%.3f/%d",self.style,
        HGNativeGlassConfigured(),self.interactiveGlass,(long)self.traitCollection.userInterfaceStyle,
        UIAccessibilityIsReduceTransparencyEnabled(),self.overlayOpacity,self.untintedGlass];
}
- (void)requestMaterialRefresh {
    if(![[self currentMaterialSignature] isEqual:self.materialSignature])[self setNeedsLayout];
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
    NSString *signature=[self currentMaterialSignature];
    if([signature isEqual:self.materialSignature])return;
    self.materialSignature=signature;
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
@interface HGAViewState : NSObject
@property(nonatomic) CGAffineTransform sourceTransform;
@property(nonatomic) CGAffineTransform appliedTransform;
@property(nonatomic) BOOL geometry;
@property(nonatomic) BOOL transferring;
@property(nonatomic,weak) UIView *geometrySuperview;
@property(nonatomic,weak) id<UIViewControllerTransitionCoordinator> transition;
@property(nonatomic) BOOL sourceHidden;
@property(nonatomic) BOOL appliedHidden;
@property(nonatomic) BOOL visibility;
@property(nonatomic,strong) UIColor *background;
@property(nonatomic) BOOL clearsBackground;
@property(nonatomic,strong) HGAAppearanceMaterial *glass;
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
static void HGASetGeometry(UIView *view,CGFloat x,CGFloat y,CGFloat scale) {
    if(fabs(x)<0.001 && fabs(y)<0.001 && fabs(scale-1)<0.001){HGARestoreGeometry(view);return;}
    if(!isfinite(x)||!isfinite(y)||!isfinite(scale))return;
    HGAViewState *state=HGAState(view,YES);
    if(state.geometry && state.geometrySuperview!=view.superview)HGARestoreGeometry(view);
    // Reuse the unmodified baseline while we still own the current value.
    // Restoring and reapplying on every layout invalidates native player layout.
    if(!state.geometry || !CGAffineTransformEqualToTransform(view.transform,state.appliedTransform))
        state.sourceTransform=view.transform;
    CGAffineTransform next=CGAffineTransformScale(state.sourceTransform,scale,scale);
    next.tx+=x;next.ty+=y;
    state.appliedTransform=next;state.geometry=YES;state.geometrySuperview=view.superview;
    if(!CGAffineTransformEqualToTransform(view.transform,next))view.transform=next;
}
static void HGASetHidden(UIView *view,BOOL hide) {
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
static void HGAApplyRailAvatarVisibility(UIView *avatar,BOOL enabled) {
    BOOL hide=enabled && HGAppearanceFlag(@"layout.avatar.hidden");
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
static void HGARemoveGlass(UIView *view) {
    HGAViewState *state=HGAState(view,NO);
    if(!state)return;
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
static void HGAConfigureButtonGlass(HGAAppearanceMaterial *glass,UIView *view,NSString *style) {
    BOOL liquid=[style isEqual:@"clear"] || [style isEqual:@"regular"];
    glass.untintedGlass=liquid;
    if(!liquid) glass.clipsToBounds=YES;
    glass.interactiveGlass=liquid;
    UIUserInterfaceStyle appearance=HGAVideoPageStyle(nil,view);
    if(glass.overrideUserInterfaceStyle!=appearance)glass.overrideUserInterfaceStyle=appearance;
    glass.overlayColor=(UIAccessibilityIsReduceTransparencyEnabled() || [style isEqual:@"solid"]) ?
        UIColor.secondarySystemBackgroundColor : UIColor.clearColor;
    [glass requestMaterialRefresh];
}
static void HGAInstallButtonGlass(UIView *view,NSString *style,CGRect frame,CGFloat radius) {
    HGAInstallGlassInRect(view,style,frame,radius,0);
    HGAAppearanceMaterial *glass=HGAState(view,NO).glass;
    if(glass)HGAConfigureButtonGlass(glass,view,style);
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
    for(Class cls=object_getClass(view);cls && cls!=UIView.class;cls=class_getSuperclass(cls)) {
        NSString *role=HGAViewRoles()[NSStringFromClass(cls)];if(role)return role;
    }return nil;
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
@end
@implementation HGAIconState @end
static char HGAIconKey;
static void HGARestoreIcon(UIView *root) {
    HGAIconState *s=objc_getAssociatedObject(root,&HGAIconKey);
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

static void HGAApplyIcon(UIView *root,NSString *role,BOOL flat,BOOL sparkles) {
    if(![@[@"like",@"collect",@"comment",@"share"] containsObject:role])return;
    if(!HGAppearanceEnabled() || (!flat && !sparkles) || !root.window) {HGARestoreIcon(root);return;}
    BOOL selected=NO;
    if([role isEqual:@"like"] || [role isEqual:@"collect"]) {
        NSString *getter=[role isEqual:@"like"]?@"isLiked":@"isCollected";
        if(!HGABoolGetter(root,getter,&selected) &&
            !HGABoolGetter(HGReadObjectGetter(root,"model"),getter,&selected)) {
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
    UIImage *image=sparkles ? [[UIImage systemImageNamed:@"sparkles"] imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate]
        : HGAFlatActionImage(role,selected,heart ? requested : 32);
    if(!image){HGARestoreIcon(root);return;}
    if(!state){state=[HGAIconState new];state.imageView=icon;state.source=icon.image;state.tint=icon.tintColor;
        objc_setAssociatedObject(root,&HGAIconKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);}
    else if(icon.image!=state.applied && ![icon.image isEqual:state.applied])state.source=icon.image;
    UIColor *color=HGAActionIconColor(role,selected);
    if(!state.appliedTint || ![icon.tintColor isEqual:state.appliedTint])state.tint=icon.tintColor;
    state.appliedTint=color;
    if(![icon.tintColor isEqual:color])icon.tintColor=color;
    if(!state.ownsContentMode || icon.contentMode!=state.appliedContentMode)state.contentMode=icon.contentMode;
    state.ownsContentMode=YES;
    state.appliedContentMode=heart ? UIViewContentModeCenter : UIViewContentModeScaleAspectFit;
    if(icon.contentMode!=state.appliedContentMode)icon.contentMode=state.appliedContentMode;
    if(icon.image!=image && ![icon.image isEqual:image])icon.image=image;
    state.applied=icon.image;
    // Bound the longest side, not the shortest one: the host sometimes gives
    // bookmark/share image views a tall rectangle. Scaling their shortest side
    // made those glyphs and their glass discs visibly larger than the heart.
    // The host attaches its one-tap recognizer to likeIcon itself. Center the
    // smaller heart artwork in that original touch surface.
    CGFloat side=MAX(icon.bounds.size.width,icon.bounds.size.height);
    if(side>0)HGASetGeometry(icon,0,0,(flat && !heart) ? MIN(1.5,requested/side) : 1);
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
}
static void HGAApplyTypography(UIView *root) {
    BOOL enabled=HGAppearanceEnabled() && HGAppearanceFlag(@"fonts.enabled") && root.window;
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
            for(UIView *child in HGADescendants(view,100))if([NSStringFromClass(child.class) isEqual:@"TTPlayerView"])HGAApplyFullScreen(child);
            HGAApplyTypography(view);HGRefreshVideoBackdropForPlayer(view);return;
        }
        if([view isKindOfClass:objc_lookUpClass("FQVShortVideoVPlayerSingleSeriesControlView")]) {
            HGRefreshVideoBackdropForPlayer(view);return;
        }
        if(HGAIsCommentRow(view)) {
            HGAUpdateCommentRow(view);return;
        }
        if(HGAIsCommentSurface(view)) {
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
        if(!avatar)HGASetHidden(view,enabled && outer && (HGAppearanceFlag([prefix stringByAppendingString:@".hidden"]) ||
            (header && !headerShowsFinder && HGAAllTopControlsHidden())));
        HGASetGeometry(view,enabled && outer?HGAppearanceNumber([prefix stringByAppendingString:@".x"]):0,
            enabled && outer?HGAppearanceNumber([prefix stringByAppendingString:@".y"]):0,
            enabled && outer?HGAppearanceNumber([prefix stringByAppendingString:@".scale"]):1);
        if(avatar)HGAApplyRailAvatarVisibility(view,enabled && outer);
        BOOL action=[@[@"avatar",@"collect",@"comment",@"like",@"share",@"more"] containsObject:role];
        BOOL styleActions=action && HGAppearanceFlag(@"player.rightGlass");
        BOOL flat=enabled && outer && styleActions && HGFlatPlayerStyle();
        BOOL glass=enabled && outer && ((styleActions && !flat) ||
            ([role isEqual:@"channels"] && !headerShowsFinder && HGAppearanceFlag(@"player.headerGlass")) ||
            ([role isEqual:@"episodes"] && HGAppearanceFlag(@"player.episodeGlass")));
        BOOL episode=[role isEqual:@"episodes"];
        if(episode && outer)HGAApplyEpisodeInset(view,glass && !view.hidden);
        CGRect episodeFrame=episode ? HGAEpisodeGlassFrame(view) : CGRectZero;
        CGFloat episodeRadius=MIN(view.bounds.size.width,view.bounds.size.height)/2;
        if(episode)HGAApplyEpisodeCorners(view,glass && !view.hidden,episodeRadius);
        BOOL sparkles=enabled && outer && [role isEqual:@"collect"] &&
            [HGAppearanceString(@"player.collectGlyph") isEqual:@"sparkles"];
        HGAApplyIcon(view,role,flat,sparkles);
        if(header) {
            HGAApplyHeaderSeparatedGlass(view,glass,enabled && outer && !headerShowsFinder && !view.hidden,
                HGAppearanceString(@"player.material"));
        } else if((glass || (flat && ![role isEqual:@"avatar"] && ![role isEqual:@"more"])) && !view.hidden) {
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
            else HGAInstallButtonGlass(view,style,glassFrame,radius);
        }
        else HGARemoveGlass(view);
        HGAApplyTypography(view);
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
static void HGAPrimeRightCommentCount(id manager) {
    if(HGReadObjectGetter(manager,"countModel"))return;
    id data=HGReadObjectGetter(HGReadObjectGetter(manager,"model"),"videoData");
    SEL getter=sel_registerName("commentCount"),paint=sel_registerName("refreshUIWithCommentCount:");
    Method countMethod=class_getInstanceMethod(object_getClass(data),getter);
    Method paintMethod=class_getInstanceMethod(object_getClass(manager),paint);
    if(!countMethod || strcmp(method_getTypeEncoding(countMethod),"q16@0:8") ||
       !paintMethod || strcmp(method_getTypeEncoding(paintMethod),"v24@0:8q16"))return;
    NSInteger count=((NSInteger(*)(id,SEL))objc_msgSend)(data,getter);
    BOOL supported=NO;
    if(count>0 && HGABoolGetter(manager,@"supportComment",&supported) && supported)
        ((void(*)(id,SEL,NSInteger))objc_msgSend)(manager,paint,count);
}
