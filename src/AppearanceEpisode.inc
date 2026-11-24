// The episode entry has a native content view inside an insert wrapper. A
// rounded material alone leaves their rectangular fills visible at its corners.
// Decorate only this observed component; preserve native control proportions.
@interface HGAEpisodeInsetState : NSObject
@property(nonatomic) CGRect sourceBounds;
@property(nonatomic) CGRect appliedBounds;
@property(nonatomic,weak) UIView *parent;
@end
@implementation HGAEpisodeInsetState @end
static char HGAEpisodeInsetKey;
static void HGARestoreEpisodeInset(UIView *entry) {
    HGAEpisodeInsetState *state=objc_getAssociatedObject(entry,&HGAEpisodeInsetKey);
    if(!state)return;
    BOOL changed=NO;
    if(CGRectEqualToRect(entry.bounds,state.appliedBounds)){entry.bounds=state.sourceBounds;changed=YES;}
    objc_setAssociatedObject(entry,&HGAEpisodeInsetKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    if(changed)[entry setNeedsLayout];
}
static void HGAApplyEpisodeInset(UIView *entry,BOOL enabled) {
    HGAEpisodeInsetState *state=objc_getAssociatedObject(entry,&HGAEpisodeInsetKey);
    if(state && state.parent!=entry.superview){HGARestoreEpisodeInset(entry);state=nil;}
    CGRect source=state && CGRectEqualToRect(entry.bounds,state.appliedBounds) ? state.sourceBounds : entry.bounds;
    CGFloat width=source.size.width;
    // Only the edge-to-edge distribution entry needs outer breathing room.
    // Change its layout width, not its transform, so native text/icons keep
    // their proportions and the host recalculates truncation and arrow position.
    if(!enabled || !entry.window || !CGAffineTransformIsIdentity(entry.transform) ||
        entry.window.bounds.size.width>=entry.window.bounds.size.height ||
        width<entry.window.bounds.size.width*0.88 || width>entry.window.bounds.size.width+1 || source.size.height>80) {
        HGARestoreEpisodeInset(entry);return;
    }
    if(!state){state=[HGAEpisodeInsetState new];objc_setAssociatedObject(entry,&HGAEpisodeInsetKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);}
    state.sourceBounds=source;state.parent=entry.superview;
    CGFloat inset=round(MAX(12,width*0.09));
    CGRect bounds=source;bounds.size.width-=inset*2;state.appliedBounds=bounds;
    if(!CGRectEqualToRect(entry.bounds,bounds)){entry.bounds=bounds;[entry setNeedsLayout];}
}
static CGRect HGAEpisodeGlassFrame(UIView *entry) {
    CGRect frame=entry.bounds;
    if(objc_getAssociatedObject(entry,&HGAEpisodeInsetKey) && frame.size.height>36)
        frame=CGRectInset(frame,0,(frame.size.height-36)/2);
    return frame;
}
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
static void HGAApplyEpisodeSurface(HGAEpisodeSurfaceState *state,CGFloat radius,BOOL clear,BOOL round) {
    UIView *view=state.view;
    if(clear) {
        if(!state.clearsBackground || ![view.backgroundColor isEqual:UIColor.clearColor])
            state.background=view.backgroundColor;
        state.clearsBackground=YES;
        if(![view.backgroundColor isEqual:UIColor.clearColor])view.backgroundColor=UIColor.clearColor;
    }
    if(!round)return;
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
    HGAEpisodeInsetState *inset=objc_getAssociatedObject(entry,&HGAEpisodeInsetKey);
    if(inset && fabs(rect.size.width-inset.sourceBounds.size.width)<0.5 &&
        fabs(rect.origin.y-entry.bounds.origin.y)<0.5 && fabs(rect.size.height-entry.bounds.size.height)<0.5) {
        CGFloat margin=(inset.sourceBounds.size.width-entry.bounds.size.width)/2;
        // The verified single-child wrapper retains native width. A native
        // content child may still be waiting for its next width layout pass.
        if(fabs(rect.origin.x-entry.bounds.origin.x+margin)<0.5 ||
            fabs(rect.origin.x-entry.bounds.origin.x)<0.5)return YES;
    }
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
    if(!enabled)HGARestoreEpisodeInset(entry);
    NSArray<HGAEpisodeSurfaceState *> *previous=objc_getAssociatedObject(entry,&HGAEpisodeSurfacesKey);
    NSMutableArray<UIView *> *surfaces=[NSMutableArray array];
    UIView *shade=nil;
    if(enabled && entry.window && !CGRectIsEmpty(entry.bounds)) {
        [surfaces addObject:entry];
        UIView *content=HGAViewGetter(entry,"defaultContentView",entry);
        if(content.superview==entry && [content isKindOfClass:NSClassFromString(@"SSShortVideoDistributionContentView")]) {
            // The host inserts this 15%-white paint-only UIView at index zero
            // above the content view's own black background.
            UIView *paint=content.subviews.firstObject;
            CGFloat white=0,alpha=0;
            if(object_getClass(paint)==UIView.class &&
                [paint.backgroundColor getWhite:&white alpha:&alpha] && white==1 && fabs(alpha-0.15)<0.005)
                shade=paint;
            if(!shade && [paint.backgroundColor isEqual:UIColor.clearColor]) {
                for(HGAEpisodeSurfaceState *state in previous)
                    if(state.view==paint && state.clearsBackground && !state.rounded){shade=paint;break;}
            }
            if(shade)[surfaces addObject:shade];
        }
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
        HGAApplyEpisodeSurface(state,radius,view!=entry,view!=shade);
        if(view==entry.superview)
            objc_setAssociatedObject(view,&HGAEpisodeWrapperKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        [current addObject:state];
    }
    objc_setAssociatedObject(entry,&HGAEpisodeSurfacesKey,current,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static void HGAEpisodeWrapperWillMove(UIView *wrapper) {
    if(wrapper.subviews.count!=1)return;
    UIView *entry=wrapper.subviews.firstObject;
    if(!objc_getAssociatedObject(entry,&HGAEpisodeInsetKey))return;
    HGAApplyEpisodeCorners(entry,NO,0);
    HGARemoveGlass(entry);
    [entry setNeedsLayout];
}
