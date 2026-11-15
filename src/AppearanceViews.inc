// Scoped appearance operations. Values are restored only while they are still
// owned by Honggo; a later host/third-party update becomes the new baseline.
@interface HGAAppearanceMaterial : UIVisualEffectView
@property(nonatomic,copy) NSString *style;
@property(nonatomic,copy) NSString *materialSignature;
@property(nonatomic) CGFloat overlayOpacity;
@property(nonatomic,strong) UIColor *overlayColor;
- (void)requestMaterialRefresh;
@end
@implementation HGAAppearanceMaterial
- (void)setOverlayColor:(UIColor *)color {
    if(_overlayColor==color || [_overlayColor isEqual:color])return;
    _overlayColor=color;self.materialSignature=nil;[self setNeedsLayout];
}
- (void)willMoveToWindow:(UIWindow *)window {
    // A live glass effect must never be carried into a detached scene. Its
    // replacement is created only after attachment with final, nonzero bounds.
    if(window!=self.window){self.effect=nil;self.materialSignature=nil;}
    [super willMoveToWindow:window];
}
- (void)didMoveToWindow {
    [super didMoveToWindow];
    if(self.window)[self setNeedsLayout];
}
- (NSString *)currentMaterialSignature {
    return [NSString stringWithFormat:@"%@/%d/%ld/%d/%.3f",self.style,
        HGNativeGlassConfigured(),(long)self.traitCollection.userInterfaceStyle,
        UIAccessibilityIsReduceTransparencyEnabled(),self.overlayOpacity];
}
- (void)requestMaterialRefresh {
    if(![[self currentMaterialSignature] isEqual:self.materialSignature])[self setNeedsLayout];
}
- (void)layoutSubviews {
    [super layoutSubviews];
    if(!self.window || CGRectIsEmpty(self.bounds))return;
    NSString *signature=[self currentMaterialSignature];
    if([signature isEqual:self.materialSignature])return;
    self.materialSignature=signature;
    self.effect=HGAppearanceMaterial(self.style);
    UIColor *color=self.overlayColor ?: [UIColor.secondarySystemBackgroundColor colorWithAlphaComponent:
        self.effect ? self.overlayOpacity : 1];
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
static void HGARemoveGlass(UIView *view) {
    HGAViewState *state=HGAState(view,NO);
    if(!state)return;
    [state.glass removeFromSuperview];state.glass=nil;
    if(state.clearsBackground && [view.backgroundColor isEqual:UIColor.clearColor])view.backgroundColor=state.background;
    state.clearsBackground=NO;state.background=nil;
}
static void HGAInstallGlassInRect(UIView *view,NSString *style,CGRect frame,CGFloat radius,CGFloat opacity) {
    if(!view.window || CGRectIsEmpty(frame)){HGARemoveGlass(view);return;}
    HGAViewState *state=HGAState(view,YES);
    if(!state.clearsBackground || ![view.backgroundColor isEqual:UIColor.clearColor])state.background=view.backgroundColor;
    state.clearsBackground=YES;
    if(![view.backgroundColor isEqual:UIColor.clearColor])view.backgroundColor=UIColor.clearColor;
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
@end
@implementation HGAIconState @end
static char HGAIconKey;
static void HGARestoreIcon(UIView *root) {
    HGAIconState *s=objc_getAssociatedObject(root,&HGAIconKey);
    HGARestoreGeometry(s.imageView);
    if(s.imageView.image==s.applied || [s.imageView.image isEqual:s.applied])s.imageView.image=s.source;
    if(s.appliedTint && [s.imageView.tintColor isEqual:s.appliedTint])s.imageView.tintColor=s.tint;
    objc_setAssociatedObject(root,&HGAIconKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static UIImageView *HGAFindIcon(UIView *root) {
    const char *names[]={"iconImageView","imageView","iconView"};
    for(NSUInteger i=0;i<3;i++) {
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
static void HGAApplyIcon(UIView *root,NSString *role) {
    if(![@[@"like",@"collect",@"comment",@"share"] containsObject:role])return;
    if(!HGAppearanceEnabled() || !HGAppearanceFlag(@"icons.enabled") || !root.window) {HGARestoreIcon(root);return;}
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
    UIImage *image=HGAIcon(role,selected);
    if(!icon || !image){HGARestoreIcon(root);return;}
    if(!state){state=[HGAIconState new];state.imageView=icon;state.source=icon.image;state.tint=icon.tintColor;
        objc_setAssociatedObject(root,&HGAIconKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);}
    else if(icon.image!=state.applied && ![icon.image isEqual:state.applied])state.source=icon.image;
    icon.image=image;
    // UIImageView can normalize a symbol to its own configuration. Track the
    // value actually retained by UIKit, not the pre-assignment image pointer.
    state.applied=icon.image;
    if(selected) {
        if(!state.appliedTint || ![icon.tintColor isEqual:state.appliedTint])state.tint=icon.tintColor;
        state.appliedTint=[role isEqual:@"like"]?UIColor.systemPinkColor:UIColor.systemYellowColor;
        icon.tintColor=state.appliedTint;
    }else if(state.appliedTint) {
        if([icon.tintColor isEqual:state.appliedTint])icon.tintColor=state.tint;
        state.appliedTint=nil;
    }
    // Adjust only the image's rendering scale, not the touch owner or its count.
    CGFloat side=MIN(icon.bounds.size.width,icon.bounds.size.height);
    if(side>0)HGASetGeometry(icon,0,0,MIN(1.5,HGAppearanceNumber(@"icons.size")/side));
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
    return @{@"SSShortVideoCommentPanelViewController":@"comment",
        @"FQCShortVideoCommentPanelViewController":@"comment",
        @"FQVShortVideoCommentPanelViewController":@"comment",
        @"FQVShortVideoSettingsViewController":@"longpress",
        @"FQVShortVideoNewSpeedSelectController":@"speed",
        @"SSUGKmpSharePanelContainerViewController":@"share",
        @"SSVideoSharePanelViewController":@"share",
        @"SSShortVideoSharePanelViewController":@"share",
        @"SSSharePanelViewController":@"share",
        @"UGKmpSharePanelContainerViewController":@"share"};
}
// Concrete comment surfaces are present even when the app changes the
// containing controller. Do not hook generic text, drawing or scroll classes.
static NSArray<NSString *> *HGACommentSurfaces(void) {
    return @[@"FQCShortVideoCommentPanelTitleView", @"FQCShortVideoCommentTitleView",
        @"FQCShortVideoCommentListCollectionView", @"FQCShortVideoBottomToolBar",
        @"FQCShortVideoNewEditorView"];
}
static char HGAPanelCoversKey;
static void HGAApplyPanel(UIView *root,NSString *kind) {
    BOOL enabled=HGAppearanceEnabled() && HGAppearanceFlag([@"panels." stringByAppendingString:kind]) && root.window;
    NSMapTable<UIView *,UIColor *> *covers=objc_getAssociatedObject(root,&HGAPanelCoversKey);
    if(!enabled) {
        HGARemoveGlass(root);
        for(UIView *cover in covers.keyEnumerator) if([cover.backgroundColor isEqual:UIColor.clearColor])cover.backgroundColor=[covers objectForKey:cover];
        objc_setAssociatedObject(root,&HGAPanelCoversKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        return;
    }
    if(!covers){covers=[NSMapTable weakToStrongObjectsMapTable];objc_setAssociatedObject(root,&HGAPanelCoversKey,covers,OBJC_ASSOCIATION_RETAIN_NONATOMIC);}
    HGAInstallGlass(root,HGAppearanceString(@"panels.material"),24,HGAppearanceNumber(@"panels.opacity"));
    // Clear only broad, plain background carriers within this exact panel.
    // Images, drawing surfaces, text fields and their rendering layers stay intact.
    for(UIView *v in HGADescendants(root,160)) {
        if(v==root || HGAOwnView(v) || CGRectGetWidth(v.bounds)<CGRectGetWidth(root.bounds)*0.8)continue;
        BOOL plain=v.class==UIView.class || v.class==UITableView.class || v.class==UICollectionView.class;
        if(!plain || !v.backgroundColor)continue;
        CGFloat r=0,g=0,b=0,a=0;
        if([[v.backgroundColor resolvedColorWithTraitCollection:v.traitCollection] getRed:&r green:&g blue:&b alpha:&a] &&
            a>0.8 && MAX(r,MAX(g,b))-MIN(r,MIN(g,b))<0.08) {
            [covers setObject:v.backgroundColor forKey:v];v.backgroundColor=UIColor.clearColor;
        }
    }
}
void HGAppearanceProcessView(UIView *view) {
    if(!NSThread.isMainThread || HGAApplying || ![view isKindOfClass:UIView.class] || HGAOwnView(view))return;
    HGAApplying=YES;
    @try {
        HGARegisterView(view);
        NSString *name=NSStringFromClass(view.class),*role=HGARole(view);
        if([name isEqual:@"SSTabBar"]){HGARefreshDock(view);return;}
        if([name isEqual:@"FQVCollectionView"] && [view isKindOfClass:UIScrollView.class]) {HGAObserveFeed((id)view);return;}
        if([name isEqual:@"TTPlayerView"]){HGAApplyFullScreen(view);return;}
        if([name isEqual:@"FQVShortVideoPlayerCellView"]){
            for(UIView *child in HGADescendants(view,100))if([NSStringFromClass(child.class) isEqual:@"TTPlayerView"])HGAApplyFullScreen(child);
            HGAApplyTypography(view);return;
        }
        if([HGACommentSurfaces() containsObject:name]) {
            HGAApplyPanel(view,@"comment");HGAApplyTypography(view);return;
        }
        if(!role)return;
        BOOL enabled=HGAppearanceEnabled() && view.window;
        BOOL outer=!HGAHasRoleAncestor(view,role);
        NSString *prefix=[@"layout." stringByAppendingString:role];
        HGASetHidden(view,enabled && outer && HGAppearanceFlag([prefix stringByAppendingString:@".hidden"]));
        HGASetGeometry(view,enabled && outer?HGAppearanceNumber([prefix stringByAppendingString:@".x"]):0,
            enabled && outer?HGAppearanceNumber([prefix stringByAppendingString:@".y"]):0,
            enabled && outer?HGAppearanceNumber([prefix stringByAppendingString:@".scale"]):1);
        BOOL action=[@[@"avatar",@"collect",@"comment",@"like",@"share",@"more"] containsObject:role];
        BOOL glass=enabled && outer && ((action && HGAppearanceFlag(@"player.rightGlass")) ||
            ([role isEqual:@"channels"] && HGAppearanceFlag(@"player.headerGlass")) ||
            ([role isEqual:@"episodes"] && HGAppearanceFlag(@"player.episodeGlass")));
        BOOL episode=[role isEqual:@"episodes"];
        CGFloat episodeRadius=MIN(view.bounds.size.width,view.bounds.size.height)/2;
        if(episode)HGAApplyEpisodeCorners(view,glass && !view.hidden,episodeRadius);
        if(glass && !view.hidden) {
            CGRect glassFrame=view.bounds;CGFloat radius=episode ? episodeRadius : 20;
            if(action) {
                UIImageView *icon=HGAFindIcon(view);
                CGRect rect=icon ? [icon convertRect:icon.bounds toView:view] : CGRectMake(0,0,view.bounds.size.width,MIN(52,view.bounds.size.height));
                CGFloat side=MAX(36,MIN(view.bounds.size.width,MAX(rect.size.width,rect.size.height)+10));
                glassFrame=CGRectMake((view.bounds.size.width-side)/2,MAX(0,CGRectGetMidY(rect)-side/2),side,side);
                radius=side/2;
            }
            HGAInstallGlassInRect(view,HGAppearanceString(@"player.material"),glassFrame,radius,0.08);
        }
        else HGARemoveGlass(view);
        HGAApplyIcon(view,role);HGAApplyTypography(view);
    }@finally {HGAApplying=NO;}
}
void HGAppearanceProcessController(UIViewController *controller) {
    if(!NSThread.isMainThread || HGAApplying || ![controller isKindOfClass:UIViewController.class] || !controller.isViewLoaded)return;
    HGAApplying=YES;
    @try {
        if(!HGAControllers)HGAControllers=[NSHashTable weakObjectsHashTable];[HGAControllers addObject:controller];
        NSString *name=NSStringFromClass(controller.class);
        if([name isEqual:@"SSTabBarController"]) {
            UIView *bar=HGAViewGetter(controller,"customTabBar",controller.view);
            if(bar)HGAppearanceLayoutDock(bar,controller);
            return;
        }
        NSString *kind=HGAPanelControllers()[name];
        if(kind){HGAApplyPanel(controller.view,kind);HGAApplyTypography(controller.view);}
    }@finally {HGAApplying=NO;}
}
