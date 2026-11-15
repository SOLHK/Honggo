// A single, readable sheet behind the host's own share destinations, grouped
// options and controls. Do not put a magnifying glass slab over the video/dock.
@interface HGAActionSheetState : NSObject
@property(nonatomic,weak) UIView *surface;
@property(nonatomic,strong) NSMapTable<UIView *,UIColor *> *covers;
@property(nonatomic) BOOL rounding;
@property(nonatomic) CGFloat sourceRadius;
@property(nonatomic) CGFloat appliedRadius;
@property(nonatomic,copy) NSString *sourceCurve;
@property(nonatomic) CACornerMask sourceCorners;
@property(nonatomic) BOOL sourceClips;
@end
@implementation HGAActionSheetState @end
static char HGAActionSheetKey;

static void HGARestoreActionSheet(UIView *root) {
    HGAActionSheetState *state=objc_getAssociatedObject(root,&HGAActionSheetKey);
    if(!state)return;
    UIView *surface=state.surface;
    HGARemoveGlass(surface);
    for(UIView *cover in state.covers.keyEnumerator.allObjects) {
        if([cover.backgroundColor isEqual:UIColor.clearColor])cover.backgroundColor=[state.covers objectForKey:cover];
    }
    if(state.rounding) {
        if(fabs(surface.layer.cornerRadius-state.appliedRadius)<0.01)surface.layer.cornerRadius=state.sourceRadius;
        if([surface.layer.cornerCurve isEqual:kCACornerCurveContinuous])surface.layer.cornerCurve=state.sourceCurve;
        if(surface.layer.maskedCorners==(kCALayerMinXMinYCorner|kCALayerMaxXMinYCorner))surface.layer.maskedCorners=state.sourceCorners;
        if(surface.clipsToBounds)surface.clipsToBounds=state.sourceClips;
    }
    objc_setAssociatedObject(root,&HGAActionSheetKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static UIView *HGAActionSheetSurface(UIView *root) {
    UIWindow *window=root.window;
    if(!window || CGRectIsEmpty(root.bounds))return nil;
    CGRect frame=[root convertRect:root.bounds toView:window];
    CGFloat windowHeight=CGRectGetHeight(window.bounds);
    if(frame.size.height<windowHeight*0.94)return root;
    // Some share implementations use a full-screen controller for their dim
    // and dismissal region. Only decorate a validated content getter there.
    UIViewController *owner=HGAOwner(root);
    for(NSString *name in @[@"panelView",@"containerView",@"contentView"]) {
        UIView *candidate=HGAViewGetter(owner,name.UTF8String,root);
        if(!candidate || candidate==root || [candidate isKindOfClass:UIScrollView.class])continue;
        CGRect rect=[candidate convertRect:candidate.bounds toView:window];
        if(rect.size.width>=window.bounds.size.width*0.8 && rect.size.height>=120 &&
            rect.size.height<windowHeight*0.94 && CGRectGetMaxY(rect)>=windowHeight-60 &&
            CGRectGetMaxY(rect)<=windowHeight+2)return candidate;
    }
    return nil;
}
static NSArray<NSString *> *HGAActionPanelSurfaces(void) {
    // Seen in R12's screenshot scene. Its collection owns the colourful app
    // icons, labels and tap actions; only its neutral broad backing is cleared.
    return @[@"SSShortVideoMorePanelShareHeaderView"];
}
static BOOL HGAIsActionPanelSurface(UIView *view) {
    for(Class cls=object_getClass(view);cls && cls!=UIView.class;cls=class_getSuperclass(cls))
        if([HGAActionPanelSurfaces() containsObject:NSStringFromClass(cls)])return YES;
    return NO;
}
static void HGAApplyActionSheet(UIView *root,NSString *kind) {
    BOOL enabled=HGAppearanceEnabled() && HGAppearanceFlag([@"panels." stringByAppendingString:kind]) && root.window;
    UIView *surface=enabled?HGAActionSheetSurface(root):nil;
    HGAActionSheetState *state=objc_getAssociatedObject(root,&HGAActionSheetKey);
    if(state && state.surface!=surface){HGARestoreActionSheet(root);state=nil;}
    if(!surface) {
        if(enabled)HGRecordCapability([@"appearance.panel.surface." stringByAppendingString:kind],
            @{@"module":@"appearance",@"state":@"unavailable",@"reason":@"full-screen dismissal region has no verified bounded sheet; native panel retained"});
        return;
    }
    if(!state) {
        state=[HGAActionSheetState new];state.surface=surface;
        state.covers=[NSMapTable weakToStrongObjectsMapTable];
        objc_setAssociatedObject(root,&HGAActionSheetKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    CGFloat radius=MIN(28,MIN(surface.bounds.size.width,surface.bounds.size.height)/2);
    // Chrome-sized sheets use an even system blur, not UIGlassEffect's lens.
    // The grouped base keeps the dock and moving video from showing through
    // rows while preserving a small amount of depth around the native cards.
    HGAInstallGlass(surface,@"blur",radius,HGAppearanceNumber(@"panels.opacity"));
    CGFloat opacity=UIAccessibilityIsReduceTransparencyEnabled()?1:0.92+0.06*HGAppearanceNumber(@"panels.opacity");
    HGASetGlassOverlay(surface,[UIColor.systemGroupedBackgroundColor colorWithAlphaComponent:opacity]);
    HGAAppearanceMaterial *glass=HGAState(surface,NO).glass;
    if(glass && surface.subviews.firstObject!=glass)[surface sendSubviewToBack:glass];
    // Never replace an existing mask (it may encode the host's transition).
    if(!surface.layer.mask) {
        if(!state.rounding || fabs(surface.layer.cornerRadius-state.appliedRadius)>=0.01)state.sourceRadius=surface.layer.cornerRadius;
        if(!state.rounding || ![surface.layer.cornerCurve isEqual:kCACornerCurveContinuous])state.sourceCurve=surface.layer.cornerCurve;
        CACornerMask corners=kCALayerMinXMinYCorner|kCALayerMaxXMinYCorner;
        if(!state.rounding || surface.layer.maskedCorners!=corners)state.sourceCorners=surface.layer.maskedCorners;
        if(!state.rounding || !surface.clipsToBounds)state.sourceClips=surface.clipsToBounds;
        state.rounding=YES;state.appliedRadius=radius;
        if(fabs(surface.layer.cornerRadius-radius)>=0.01)surface.layer.cornerRadius=radius;
        if(![surface.layer.cornerCurve isEqual:kCACornerCurveContinuous])surface.layer.cornerCurve=kCACornerCurveContinuous;
        if(surface.layer.maskedCorners!=corners)surface.layer.maskedCorners=corners;
        if(!surface.clipsToBounds)surface.clipsToBounds=YES;
    }
    for(UIView *cover in state.covers.keyEnumerator.allObjects) {
        if(![cover isDescendantOfView:surface]) {
            if([cover.backgroundColor isEqual:UIColor.clearColor])cover.backgroundColor=[state.covers objectForKey:cover];
            [state.covers removeObjectForKey:cover];
        }
    }
    for(UIView *view in HGADescendants(surface,240)) {
        if(view==surface || HGAOwnView(view) || view.hidden || CGRectGetHeight(view.bounds)<16 ||
            CGRectGetWidth(view.bounds)<CGRectGetWidth(surface.bounds)*0.8)continue;
        BOOL backing=view.class==UIView.class || view.class==UITableView.class ||
            view.class==UICollectionView.class || HGAIsActionPanelSurface(view);
        if(!backing || !view.backgroundColor)continue;
        CGFloat r=0,g=0,b=0,a=0;
        if([[view.backgroundColor resolvedColorWithTraitCollection:view.traitCollection] getRed:&r green:&g blue:&b alpha:&a] &&
            a>0.8 && MAX(r,MAX(g,b))-MIN(r,MIN(g,b))<0.08) {
            [state.covers setObject:view.backgroundColor forKey:view];view.backgroundColor=UIColor.clearColor;
        }
    }
    HGRecordCapability([@"appearance.panel.surface." stringByAppendingString:kind],
        @{@"module":@"appearance",@"state":@"installed",@"reason":@"bounded system sheet; native share destinations and settings actions retained"});
}
static void HGAUpdateActionPanelSurface(UIView *view) {
    UIViewController *owner=HGAOwner(view);
    NSString *kind=HGAPanelKind(owner);
    if([@[@"share",@"longpress",@"speed"] containsObject:kind] && owner.isViewLoaded) {
        if(!HGAControllers)HGAControllers=[NSHashTable weakObjectsHashTable];
        [HGAControllers addObject:owner];
        HGAApplyActionSheet(owner.view,kind);
    }
}
