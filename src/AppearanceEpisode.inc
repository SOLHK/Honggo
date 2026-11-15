// The episode entry has a native content view inside an insert wrapper. A
// rounded material alone leaves their rectangular fills visible at its corners.
// Decorate only this observed component; never move its controls or its frame.
@interface HGAEpisodeSurfaceState : NSObject
@property(nonatomic,weak) UIView *view;
@property(nonatomic,weak) UIView *owner;
@property(nonatomic,strong) UIColor *background;
@property(nonatomic) BOOL clearsBackground;
@property(nonatomic) CGFloat radius;
@property(nonatomic) CGFloat appliedRadius;
@property(nonatomic,copy) NSString *curve;
@property(nonatomic) CACornerMask corners;
@property(nonatomic) BOOL clips;
@property(nonatomic) BOOL rounded;
@end
@implementation HGAEpisodeSurfaceState @end
static char HGAEpisodeSurfacesKey;
static char HGAEpisodeWrapperKey;
static const CACornerMask HGAEpisodeAllCorners = kCALayerMinXMinYCorner |
    kCALayerMaxXMinYCorner | kCALayerMinXMaxYCorner | kCALayerMaxXMaxYCorner;

static void HGARestoreEpisodeSurface(HGAEpisodeSurfaceState *state) {
    UIView *view=state.view;
    if(!view)return;
    if(state.clearsBackground && [view.backgroundColor isEqual:UIColor.clearColor])
        view.backgroundColor=state.background;
    if(state.rounded) {
        if(fabs(view.layer.cornerRadius-state.appliedRadius)<0.001)view.layer.cornerRadius=state.radius;
        if([view.layer.cornerCurve isEqual:kCACornerCurveContinuous])view.layer.cornerCurve=state.curve;
        if(view.layer.maskedCorners==HGAEpisodeAllCorners)view.layer.maskedCorners=state.corners;
        if(view.clipsToBounds)view.clipsToBounds=state.clips;
    }
    state.clearsBackground=NO;state.rounded=NO;
    if(objc_getAssociatedObject(view,&HGAEpisodeWrapperKey)==state)
        objc_setAssociatedObject(view,&HGAEpisodeWrapperKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static void HGAApplyEpisodeSurface(HGAEpisodeSurfaceState *state,CGFloat radius,BOOL clear) {
    UIView *view=state.view;
    if(clear) {
        if(!state.clearsBackground || ![view.backgroundColor isEqual:UIColor.clearColor])
            state.background=view.backgroundColor;
        state.clearsBackground=YES;
        if(![view.backgroundColor isEqual:UIColor.clearColor])view.backgroundColor=UIColor.clearColor;
    }
    if(!state.rounded || fabs(view.layer.cornerRadius-state.appliedRadius)>0.001)state.radius=view.layer.cornerRadius;
    if(!state.rounded || ![view.layer.cornerCurve isEqual:kCACornerCurveContinuous])state.curve=view.layer.cornerCurve;
    if(!state.rounded || view.layer.maskedCorners!=HGAEpisodeAllCorners)state.corners=view.layer.maskedCorners;
    if(!state.rounded || !view.clipsToBounds)state.clips=view.clipsToBounds;
    state.rounded=YES;state.appliedRadius=radius;
    if(fabs(view.layer.cornerRadius-radius)>0.001)view.layer.cornerRadius=radius;
    if(![view.layer.cornerCurve isEqual:kCACornerCurveContinuous])view.layer.cornerCurve=kCACornerCurveContinuous;
    if(view.layer.maskedCorners!=HGAEpisodeAllCorners)view.layer.maskedCorners=HGAEpisodeAllCorners;
    if(!view.clipsToBounds)view.clipsToBounds=YES;
}
static BOOL HGAEpisodeSameSurface(UIView *candidate,UIView *entry) {
    if(!candidate || !CGAffineTransformIsIdentity(candidate.transform) ||
        !CGAffineTransformIsIdentity(entry.transform))return NO;
    CGRect rect=[candidate convertRect:candidate.bounds toView:entry];
    return fabs(rect.origin.x-entry.bounds.origin.x)<0.5 &&
        fabs(rect.origin.y-entry.bounds.origin.y)<0.5 &&
        fabs(rect.size.width-entry.bounds.size.width)<0.5 &&
        fabs(rect.size.height-entry.bounds.size.height)<0.5;
}
static void HGAEpisodeWrapperChanged(UIView *wrapper) {
    HGAEpisodeSurfaceState *state=objc_getAssociatedObject(wrapper,&HGAEpisodeWrapperKey);
    if(!state)return;
    UIView *entry=state.owner;
    if(!entry || entry.superview!=wrapper || wrapper.subviews.count!=1 ||
        !HGAEpisodeSameSurface(wrapper,entry)) {
        HGARestoreEpisodeSurface(state);
        [entry setNeedsLayout];
    }
}
static void HGAApplyEpisodeCorners(UIView *entry,BOOL enabled,CGFloat radius) {
    NSArray<HGAEpisodeSurfaceState *> *previous=objc_getAssociatedObject(entry,&HGAEpisodeSurfacesKey);
    NSMutableArray<UIView *> *surfaces=[NSMutableArray array];
    if(enabled && entry.window && !CGRectIsEmpty(entry.bounds)) {
        [surfaces addObject:entry];
        for(UIView *child in HGADescendants(entry,32)) {
            NSString *name=NSStringFromClass(child.class);
            if(child!=entry && ([name isEqual:@"SSShortVideoDistributionContentView"] ||
                [name isEqual:@"SSShortVideoBottomDistributionTextView"]) &&
                HGAEpisodeSameSurface(child,entry))[surfaces addObject:child];
        }
        UIView *wrapper=entry.superview;
        // The diagnostic hierarchy shows one entry inside this exact wrapper.
        // A wrapper with any other content must retain all of its own styling.
        if([NSStringFromClass(wrapper.class) isEqual:@"FQVShortVideoBaseInsertView"] &&
            wrapper.subviews.count==1 && HGAEpisodeSameSurface(wrapper,entry))
            [surfaces addObject:wrapper];
    }
    for(HGAEpisodeSurfaceState *state in previous)
        if(!state.view || ![surfaces containsObject:state.view])HGARestoreEpisodeSurface(state);
    if(!surfaces.count) {
        objc_setAssociatedObject(entry,&HGAEpisodeSurfacesKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        return;
    }
    NSMutableArray<HGAEpisodeSurfaceState *> *current=[NSMutableArray array];
    for(UIView *view in surfaces) {
        HGAEpisodeSurfaceState *state=nil;
        for(HGAEpisodeSurfaceState *candidate in previous)if(candidate.view==view){state=candidate;break;}
        if(!state){state=[HGAEpisodeSurfaceState new];state.view=view;state.owner=entry;}
        HGAApplyEpisodeSurface(state,radius,view!=entry);
        if(view==entry.superview)
            objc_setAssociatedObject(view,&HGAEpisodeWrapperKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        [current addObject:state];
    }
    objc_setAssociatedObject(entry,&HGAEpisodeSurfacesKey,current,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
