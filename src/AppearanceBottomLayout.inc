// Space the measured native rail and episode entry above the visible dock.
// The 7.4.3.33 feed uses VPlayerOutterMultiSeriesControlView, not the legacy
// ShortVideoControlView. Keep the playback canvas, native page stride and all
// control sizes intact; only bounded HUD positions belong to this adapter.
@interface HGABottomPosition : NSObject
@property(nonatomic,weak) UIView *view;
@property(nonatomic,weak) UIView *parent;
@property(nonatomic) CGPoint source;
@property(nonatomic) CGPoint applied;
@property(nonatomic) BOOL owned;
@end
@implementation HGABottomPosition @end
@interface HGABottomLayoutState : NSObject
@property(nonatomic,strong) HGABottomPosition *entry;
@property(nonatomic,strong) HGABottomPosition *info;
@property(nonatomic,strong) HGABottomPosition *progress;
@property(nonatomic,strong) NSArray<HGABottomPosition *> *groups;
@property(nonatomic) CGFloat lastLift;
@property(nonatomic) CGFloat lastProgressLift;
@property(nonatomic) CGFloat measuredDockLift;
@property(nonatomic,weak) UIWindow *resumeWindow;
@property(nonatomic) CGSize resumeSize;
- (CGFloat)dockLiftForOverlap:(CGFloat)overlap resuming:(BOOL)resuming movingSideways:(BOOL)moving;
@end
@implementation HGABottomLayoutState
- (CGFloat)dockLiftForOverlap:(CGFloat)overlap resuming:(BOOL)resuming movingSideways:(BOOL)moving {
    if(!isnan(overlap)) {
        CGFloat lift=isfinite(overlap) && overlap>0 && overlap<=64 ? overlap : 0;
        self.measuredDockLift=lift;
        return lift;
    }
    if(resuming)return self.measuredDockLift;
    return moving ? self.lastProgressLift : 0;
}
@end
static char HGABottomLayoutKey;

static NSArray<NSString *> *HGABottomControlClasses(void) {
    // Exact observed host identities; do not hook their common UIKit base.
    // Feed type 3 uses the inner multi-series control, a sibling of the
    // outer control responsible for home playback.
    return @[@"FQVShortVideoDerivativeVideoControlView",@"FQVShortVideoControlView",
        @"FQVShortVideoVPlayerMultiSeriesControlView",@"FQVShortVideoVPlayerOutterMultiSeriesControlView"];
}
static BOOL HGAIsBottomControl(UIView *view) {
    for(Class cls=object_getClass(view);cls && cls!=UIView.class;cls=class_getSuperclass(cls))
        if([HGABottomControlClasses() containsObject:NSStringFromClass(cls)])return YES;
    return NO;
}
static UIView *HGABottomControlForView(UIView *view) {
    for(NSUInteger depth=0;view && depth<12;depth++,view=view.superview)
        if(HGAIsBottomControl(view))return view;
    return nil;
}
static void HGARestoreBottomPosition(HGABottomPosition *position) {
    UIView *view=position.view;
    BOOL owned=position.owned && view.superview==position.parent &&
        CGPointEqualToPoint(view.center,position.applied);
    CGPoint source=position.source;
    position.owned=NO;position.parent=nil;
    if(owned && !CGPointEqualToPoint(view.center,source))view.center=source;
}
static void HGARestoreBottomState(HGABottomLayoutState *state) {
    HGARestoreBottomPosition(state.entry);HGARestoreBottomPosition(state.info);
    HGARestoreBottomPosition(state.progress);
    for(HGABottomPosition *position in state.groups)HGARestoreBottomPosition(position);
    state.groups=nil;state.lastLift=0;state.lastProgressLift=0;
    state.measuredDockLift=0;state.resumeWindow=nil;
}
// Before the drawer starts moving the feed, give back only centers still
// owned by this adapter. The host may already have laid out a reused cell.
static void HGARelinquishBottomLayout(UIView *view) {
    HGABottomLayoutState *state = objc_getAssociatedObject(view, &HGABottomLayoutKey);
    if (state) HGARestoreBottomState(state);
}
static void HGARestoreBottomLayoutForView(UIView *view) {
    UIView *control=HGABottomControlForView(view);
    HGARestoreBottomState(objc_getAssociatedObject(control,&HGABottomLayoutKey));
}
static HGABottomPosition *HGABottomPositionForView(HGABottomPosition *old,UIView *view) {
    if(old.view!=view || (old.owned && old.parent!=view.superview)) {
        HGARestoreBottomPosition(old);old=nil;
    }
    if(!view)return nil;
    if(!old){old=[HGABottomPosition new];old.view=view;}
    if(!old.owned || !CGPointEqualToPoint(view.center,old.applied)) {
        // A later native frame/center update becomes the new layout baseline.
        old.source=view.center;old.owned=NO;
    }
    return old;
}
static CGRect HGABottomNativeRect(UIView *view,UIView *control,HGABottomPosition *position) {
    CGRect rect=[view convertRect:view.bounds toView:control];
    if(position.owned && CGPointEqualToPoint(view.center,position.applied)) {
        rect.origin.x+=position.source.x-position.applied.x;
        rect.origin.y+=position.source.y-position.applied.y;
    }
    return rect;
}
static void HGAApplyBottomPosition(HGABottomPosition *position,CGFloat lift) {
    UIView *view=position.view;
    if(!view)return;
    if(lift<0.01){HGARestoreBottomPosition(position);return;}
    CGPoint next=CGPointMake(position.source.x,position.source.y-lift);
    position.applied=next;position.parent=view.superview;position.owned=YES;
    if(!CGPointEqualToPoint(view.center,next))view.center=next;
}
static BOOL HGABottomHasCustomGeometry(void) {
    for(NSString *role in @[@"episodes",@"progress",@"info"]) {
        NSString *prefix=[@"layout." stringByAppendingString:role];
        if(fabs(HGAppearanceNumber([prefix stringByAppendingString:@".x"]))>0.01 ||
            fabs(HGAppearanceNumber([prefix stringByAppendingString:@".y"]))>0.01 ||
            fabs(HGAppearanceNumber([prefix stringByAppendingString:@".scale"])-1)>0.01)return YES;
    }
    return NO;
}
static BOOL HGABottomViewIsA(UIView *view,NSString *name) {
    Class cls=NSClassFromString(name);
    return cls && [view isKindOfClass:cls];
}
static BOOL HGABottomNativeCoordinates(UIView *view,UIView *control) {
    for(NSUInteger depth=0;view && depth<12;depth++,view=view.superview) {
        if(view==control)return YES;
        if(!CGAffineTransformIsIdentity(view.transform))return NO;
    }
    return NO;
}
static char HGDeferredHUDsKey, HGCategoryFeedsKey;

static NSHashTable *HGADeferredViews(id owner, const void *key) {
    NSHashTable *views = objc_getAssociatedObject(owner, key);
    if (!views) {
        views = [NSHashTable weakObjectsHashTable];
        objc_setAssociatedObject(owner, key, views, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    return views;
}

static void HGARegisterCategoryFeed(UIScrollView *feed) {
    for (UIView *node = feed.superview; node; node = node.superview) {
        if (HGABottomViewIsA(node, @"FQRJXCategoryListContainerView")) {
            [HGADeferredViews(node, &HGCategoryFeedsKey) addObject:feed];
            break;
        }
    }
}

static void HGAResumeCategoryFeeds(UIView *pager) {
    if (!NSThread.isMainThread) return;
    NSHashTable *feeds = objc_getAssociatedObject(pager, &HGCategoryFeedsKey);
    for (UIScrollView *feed in feeds.allObjects) {
        if (![feed isDescendantOfView:pager]) [feeds removeObject:feed];
        else HGScheduleFeedLayoutRefresh(feed);
    }
}

// A horizontal channel pager can carry the entire video feed, so its canvas
// and its vertical collection move together. Compare both coordinate spaces.
static BOOL HGABottomCanvasIsCurrent(UIView *control, UIScrollView **nativeFeed, BOOL *visibleInFeed,
    BOOL *movingSideways) {
    UIView *canvas = HGAPlayerCanvas(control);
    UIWindow *window = control.window;
    if (!canvas || !window) return NO;
    UIScrollView *feed = nil;
    BOOL verticalScroll = NO, outerScroll = NO, horizontalScroll = NO;
    for (UIView *node = canvas.superview; node && node != window; node = node.superview) {
        if (![node isKindOfClass:UIScrollView.class]) continue;
        UIScrollView *scroll = (UIScrollView *)node;
        BOOL scrolling = scroll.tracking || scroll.dragging || scroll.decelerating;
        if (!feed && HGABottomViewIsA(scroll, @"FQVCollectionView")) {
            feed = scroll;
            verticalScroll = scrolling;
        } else if (scrolling) {
            outerScroll = YES;
            horizontalScroll |= scroll.contentSize.width > scroll.bounds.size.width + 1;
        }
    }
    if(nativeFeed)*nativeFeed=feed;
    if (!CGAffineTransformEqualToTransform(canvas.transform, CGAffineTransformMakeTranslation(canvas.transform.tx, 0)) ||
        !HGABottomNativeCoordinates(control, canvas)) return NO;
    CGRect viewport = feed ? [feed convertRect:feed.bounds toView:window] : window.bounds;
    CGRect rect = [canvas convertRect:canvas.bounds toView:window];
    if (CGRectIsEmpty(rect)) return NO;
    BOOL aligned = fabs(CGRectGetMinX(rect) - CGRectGetMinX(viewport)) <= 1 &&
        fabs(CGRectGetMidX(viewport) - CGRectGetMidX(window.bounds)) <= 1;
    BOOL settledVertically = !verticalScroll && fabs(CGRectGetMinY(rect) - CGRectGetMinY(viewport)) <= 1;
    if (movingSideways) *movingSideways = (!aligned || horizontalScroll) && settledVertically &&
        !CGRectIsEmpty(CGRectIntersection(rect, window.bounds));
    if (visibleInFeed) *visibleInFeed = feed && aligned && !outerScroll &&
        !CGRectIsEmpty(CGRectIntersection(rect, viewport));
    return aligned && settledVertically && !outerScroll;
}

static void HGAResumeSettledHUDs(UIScrollView *feed) {
    NSHashTable *pending = objc_getAssociatedObject(feed, &HGDeferredHUDsKey);
    for (UIView *control in pending.allObjects) {
        UIScrollView *currentFeed = nil;
        BOOL current = HGABottomCanvasIsCurrent(control, &currentFeed, NULL, NULL);
        if (currentFeed != feed || current) [pending removeObject:control];
        if (currentFeed == feed && current) [control setNeedsLayout];
    }
}
static void HGAUpdateBottomInfoGroups(HGABottomLayoutState *state,UIView *info,
    UIView *control,CGRect entryRect,CGFloat lift) {
    // OutterSeriesInfoView spans almost the whole player and also owns VIP /
    // payment overlays. Leave that owner in place; move only its known bottom
    // metadata/action children, preserving their spacing with the episode row.
    NSMutableArray<HGABottomPosition *> *next=[NSMutableArray array];
    for(UIView *child in info.subviews) {
        BOOL known=HGABottomViewIsA(child,@"SSShortVideoRightInteractionView") ||
            HGABottomViewIsA(child,@"SSShortVideoAboveLeftContainerView") ||
            HGABottomViewIsA(child,@"SSShortVideoOutterSeriesLeftContainerView") ||
            (HGABottomViewIsA(child,@"FQVShortVideoBaseInsertView") && child.subviews.count==1);
        if(!known || !HGAVisible(child) || !HGABottomNativeCoordinates(child,control))continue;
        HGABottomPosition *position=nil;
        for(HGABottomPosition *old in state.groups)if(old.view==child){position=old;break;}
        position=HGABottomPositionForView(position,child);
        CGRect rect=HGABottomNativeRect(child,control,position);
        if(CGRectIsEmpty(rect) || CGRectGetMinY(rect)<control.bounds.size.height*0.20 ||
            CGRectGetMaxY(rect)>CGRectGetMinY(entryRect)+2 ||
            CGRectGetMaxY(rect)<CGRectGetMinY(entryRect)-180)continue;
        [next addObject:position];
    }
    for(HGABottomPosition *old in state.groups)
        if(![next containsObject:old])HGARestoreBottomPosition(old);
    state.groups=next;
    for(HGABottomPosition *position in next)HGAApplyBottomPosition(position,lift);
}
static BOOL HGABottomTrackCandidate(CGRect rect,CGRect bounds) {
    return !CGRectIsEmpty(rect) && !CGRectIsInfinite(rect) &&
        isfinite(rect.origin.x) && isfinite(rect.origin.y) &&
        rect.size.width>=bounds.size.width*0.65 && rect.size.width<=bounds.size.width+1 &&
        rect.size.height>=0.5 && rect.size.height<=6 &&
        CGRectContainsRect(CGRectInset(bounds,-1,-1),rect);
}
typedef NS_ENUM(NSUInteger,HGABottomRailResult) {
    HGABottomRailUnknown,HGABottomRailMeasured,HGABottomRailHidden,HGABottomRailAmbiguous
};
static HGABottomRailResult HGABottomProgressRail(UIView *progress,UIView *control,CGRect *result) {
    // Scope this geometric reading to the verified native progress owner.
    // UIKit sliders expose a track rect; the host's custom rail is a broad,
    // thin view/layer inside that owner. Never resize or intercept either.
    CGRect rail=CGRectNull;BOOL hiddenRail=NO;
    for(UIView *candidate in HGADescendants(progress,48)) {
        if(!HGABottomNativeCoordinates(candidate,progress))continue;
        BOOL visible=HGAVisible(candidate);
        CGRect rect=CGRectNull;
        if([candidate isKindOfClass:UISlider.class]) {
            UISlider *slider=(id)candidate;
            rect=[slider convertRect:[slider trackRectForBounds:slider.bounds] toView:progress];
        } else if(candidate!=progress)rect=[candidate convertRect:candidate.bounds toView:progress];
        if(HGABottomTrackCandidate(rect,progress.bounds)) {
            if(!visible)hiddenRail=YES;
            else {
                if(!CGRectIsNull(rail) && fabs(CGRectGetMidY(rail)-CGRectGetMidY(rect))>2)return HGABottomRailAmbiguous;
                rail=CGRectIsNull(rail)?rect:CGRectUnion(rail,rect);
            }
        }
        // Some host builds draw their track directly with a CALayer.
        for(CALayer *layer in candidate.layer.sublayers) {
            if(!CATransform3DIsIdentity(layer.transform) ||
                !CATransform3DIsIdentity(candidate.layer.sublayerTransform))continue;
            CGRect layerRect=[layer convertRect:layer.bounds toLayer:progress.layer];
            if(!HGABottomTrackCandidate(layerRect,progress.bounds))continue;
            if(!visible || layer.hidden || layer.opacity<0.01){hiddenRail=YES;continue;}
            if(!CGRectIsNull(rail) && fabs(CGRectGetMidY(rail)-CGRectGetMidY(layerRect))>2)return HGABottomRailAmbiguous;
            rail=CGRectIsNull(rail)?layerRect:CGRectUnion(rail,layerRect);
        }
    }
    if(CGRectIsNull(rail))return hiddenRail?HGABottomRailHidden:HGABottomRailUnknown;
    if(rail.size.height>6)return HGABottomRailAmbiguous;
    *result=[progress convertRect:rail toView:control];
    return HGABottomRailMeasured;
}
static void HGAUpdateBottomLayoutForView(UIView *view) {
    if (HGFeedSidebarOwnsLayout(view.window)) return;
    UIView *control=HGABottomControlForView(view);
    if(!control)return;
    HGABottomLayoutState *state=objc_getAssociatedObject(control,&HGABottomLayoutKey);
    BOOL resuming=state.resumeWindow && state.resumeWindow==control.window &&
        CGSizeEqualToSize(state.resumeSize,control.bounds.size) && HGAppearanceDockEnabled();
    if(state.resumeWindow && !resuming){state.resumeWindow=nil;state.measuredDockLift=0;}
    if(!HGAppearanceEnabled() || ![HGAppearanceString(@"player.separateProgress") isEqual:@"show"] ||
        control.bounds.size.height<=control.bounds.size.width ||
        HGABottomHasCustomGeometry()) {HGARestoreBottomState(state);return;}
    if(!HGAVisible(control)) {if(!resuming)HGARestoreBottomState(state);return;}
    UIScrollView *feed = nil;
    BOOL visibleInFeed = NO;
    BOOL movingSideways = NO;
    BOOL currentCanvas = HGABottomCanvasIsCurrent(control, &feed, &visibleInFeed, &movingSideways);
    if (feed && !currentCanvas) {
        [HGADeferredViews(feed, &HGDeferredHUDsKey) addObject:control];
        HGARegisterCategoryFeed(feed);
        if (!visibleInFeed && !(movingSideways && state.lastLift > 0)) {
            if(!resuming)HGARestoreBottomState(state);
            return;
        }
    }
    UIView *progress=nil,*wrapper=nil,*info=nil;
    BOOL ambiguous=NO,ambiguousInfo=NO;
    // A native layout may add a neutral intermediate container. Find only the
    // recognized components belonging to this HUD, never a nested player HUD.
    for(UIView *child in HGADescendants(control,96)) {
        if(child==control || !HGAVisible(child) || HGABottomControlForView(child)!=control)continue;
        if(HGABottomViewIsA(child,@"FQVShortVideoProgressView")) {
            if(progress){ambiguous=YES;break;}progress=child;
        } else if(HGABottomViewIsA(child,@"FQVShortVideoBaseInsertView") && child.subviews.count==1 &&
            [HGARole(child.subviews.firstObject) isEqual:@"episodes"] && HGAVisible(child.subviews.firstObject)) {
            if(wrapper){ambiguous=YES;break;}wrapper=child;
        } else if(child.superview==control &&
            (HGABottomViewIsA(child,@"SSShortVideoDerivateInfoView") ||
             HGABottomViewIsA(child,@"SSShortVideoOutterSeriesInfoView"))) {
            // Multiple independent HUD groups are left in their native place.
            if(info)ambiguousInfo=YES;else info=child;
        }
    }
    if(ambiguousInfo)info=nil;
    if(ambiguous || !progress || !wrapper || !HGABottomNativeCoordinates(wrapper,control) ||
        !HGABottomNativeCoordinates(progress,control)) {
        if(resuming && !ambiguous && (!wrapper || wrapper==state.entry.view) && (!progress || progress==state.progress.view) &&
            state.entry.view.superview==state.entry.parent && state.progress.view.superview==state.progress.parent)return;
        HGARestoreBottomState(state);return;
    }
    if(!state) {
        state=[HGABottomLayoutState new];
        objc_setAssociatedObject(control,&HGABottomLayoutKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        HGARegisterView(control);
    }
    if((state.entry.view && state.entry.view!=wrapper) || (state.progress.view && state.progress.view!=progress)) {
        state.measuredDockLift=0;state.resumeWindow=nil;resuming=NO;
    }
    state.entry=HGABottomPositionForView(state.entry,wrapper);
    state.info=HGABottomPositionForView(state.info,info);
    state.progress=HGABottomPositionForView(state.progress,progress);
    CGRect entryRect=HGABottomNativeRect(wrapper,control,state.entry);
    CGRect progressRect=HGABottomNativeRect(progress,control,state.progress);
    CGRect railRect=CGRectNull;
    CGFloat width=control.bounds.size.width,height=control.bounds.size.height;
    BOOL ordinary=entryRect.size.height>=32 && entryRect.size.height<=80 &&
        entryRect.size.width>=width*0.65 && progressRect.size.width>=width*0.65 &&
        progressRect.size.height>=6 && progressRect.size.height<=80 &&
        CGRectGetMinY(entryRect)>height*0.55 && CGRectGetMinY(progressRect)>height*0.60;
    if(!ordinary) {HGARestoreBottomState(state);return;}
    HGABottomRailResult railResult=HGABottomProgressRail(progress,control,&railRect);
    BOOL measured=railResult==HGABottomRailMeasured;
    // Measure against the host baseline even while our previous dock offset is
    // live. Repeated child layouts must never consume that offset a second time.
    if(measured && state.progress.owned && CGPointEqualToPoint(progress.center,state.progress.applied))
        railRect.origin.y+=state.progress.source.y-state.progress.applied.y;
    CGFloat needed=0;
    if(measured)needed=MAX(0,CGRectGetMaxY(entryRect)-CGRectGetMinY(railRect)+8);
    else if(railResult==HGABottomRailUnknown && progressRect.size.height>=20 && progressRect.size.height<=40 &&
        CGRectGetMaxY(entryRect)>=CGRectGetMinY(progressRect)+10 &&
        CGRectGetMaxY(entryRect)<=CGRectGetMaxY(progressRect)+4) {
        // The exact native progress owner can draw its rail directly. Only in
        // its observed compact geometry, use a bounded ten-point nudge; do not
        // infer a thin rail from the entire touch strip or reserve extra space.
        needed=10;
    } else {if(!resuming)HGARestoreBottomState(state);return;}
    if(!isfinite(needed) || needed>20) {HGARestoreBottomState(state);return;}
    // The outgoing video and Dock may move in different coordinate spaces.
    // Carry forward the settled clearance until the horizontal transition ends.
    CGFloat overlap=NAN;
    CGRect dock=currentCanvas ? HGADockRectInView(control) : CGRectNull;
    if(measured && currentCanvas && !CGRectIsNull(dock) && !CGRectIsEmpty(dock) &&
        CGRectGetMinY(dock)>height*0.65 && CGRectGetMinY(dock)<height+100 &&
        CGRectGetMaxX(dock)>width*0.25 && CGRectGetMinX(dock)<width*0.75) {
        // Small dock-height/lift changes share one offset across the HUD. A
        // radically different host geometry remains native instead of moving
        // controls far into the video or modifying its paging viewport.
        overlap=CGRectGetMaxY(railRect)+10-CGRectGetMinY(dock);
    }
    CGFloat progressLift=[state dockLiftForOverlap:overlap resuming:resuming movingSideways:movingSideways && HGAppearanceDockEnabled()];
    CGFloat lift=MIN(16,needed)+progressLift;
    BOOL adjacentInfo=NO;
    if(info && HGABottomNativeCoordinates(info,control) && ![wrapper isDescendantOfView:info] &&
        ![progress isDescendantOfView:info]) {
        CGRect infoRect=HGABottomNativeRect(info,control,state.info);
        adjacentInfo=CGRectGetMaxY(infoRect)<=CGRectGetMinY(entryRect)+2 &&
            CGRectGetMaxY(infoRect)>=CGRectGetMinY(entryRect)-80;
    }
    BOOL outterGroups=HGABottomViewIsA(info,@"SSShortVideoOutterSeriesInfoView") &&
        info.bounds.size.height>height*0.5;
    [CATransaction begin];[CATransaction setDisableActions:YES];
    HGAApplyBottomPosition(state.progress,progressLift);
    HGAApplyBottomPosition(state.entry,lift);
    // An overlay HUD can extend to the bottom of the canvas. Its bounds are
    // not the metadata's visible edge; leave it in place instead of abandoning
    // the episode correction or dragging the full-screen HUD upward.
    HGAApplyBottomPosition(state.info,adjacentInfo && !outterGroups?lift:0);
    HGAUpdateBottomInfoGroups(state,outterGroups?info:nil,control,entryRect,lift);
    [CATransaction commit];
    if((fabs(state.lastLift-lift)>0.01 || fabs(state.lastProgressLift-progressLift)>0.01) && HGEnhancedDiagnostics())
        HGLog(@"appearance.episode-spacing",@{@"lift":@(lift),@"targetGap":@8,
            @"measuredGap":measured?@(CGRectGetMinY(railRect)-progressLift-CGRectGetMaxY(entryRect)+lift):NSNull.null,
            @"progressLift":@(progressLift),@"dockGap":measured && !CGRectIsNull(dock)?
                @(CGRectGetMinY(dock)-CGRectGetMaxY(railRect)+progressLift):NSNull.null,
            @"controlClass":NSStringFromClass(control.class),@"infoGroups":@(state.groups.count),
            @"railMeasured":@(measured),@"capped":@(needed>16),
            @"sliderSizePreserved":@YES,@"mode":@"native-hud-centers"});
    state.lastLift=lift;state.lastProgressLift=progressLift;
}

// The multitasking card is drawn from the last settled player. When the app
// becomes live the Dock can be absent for one layout. Its 3pt clearance must
// not disappear from the episode row, action rail and progress rail together.
static NSUInteger HGABottomAppSwitch;
static void HGAPrepareAppSwitchHUDs(void) {
    HGABottomAppSwitch++;
    for(UIView *control in HGAManaged.allObjects) {
        HGABottomLayoutState *state=objc_getAssociatedObject(control,&HGABottomLayoutKey);
        if(state.measuredDockLift<=0 || !HGAVisible(control) || !HGABottomCanvasIsCurrent(control,NULL,NULL,NULL))continue;
        state.resumeWindow=control.window;state.resumeSize=control.bounds.size;
    }
}
static void HGASettleAppSwitchHUDs(BOOL release) {
    [UIView performWithoutAnimation:^{
        for(UIView *control in HGAManaged.allObjects) {
            HGABottomLayoutState *state=objc_getAssociatedObject(control,&HGABottomLayoutKey);
            UIWindow *window=state.resumeWindow;
            if(!window)continue;
            if(release)state.resumeWindow=nil;
            if(control.window!=window){HGARestoreBottomState(state);continue;}
            for(UIView *bar in HGABars.allObjects)
                if(bar.window==window && HGAVisible(bar)){[bar setNeedsLayout];[bar layoutIfNeeded];}
            [control setNeedsLayout];[control layoutIfNeeded];
        }
    }];
}
static void HGAInstallAppSwitchHUDs(void) {
    NSNotificationCenter *notes=NSNotificationCenter.defaultCenter;
    [notes addObserverForName:UIApplicationWillResignActiveNotification object:nil queue:NSOperationQueue.mainQueue
        usingBlock:^(NSNotification *note) { HGAPrepareAppSwitchHUDs(); }];
    [notes addObserverForName:UIApplicationDidBecomeActiveNotification object:nil queue:NSOperationQueue.mainQueue
        usingBlock:^(NSNotification *note) {
            NSUInteger generation=HGABottomAppSwitch;
            [CATransaction begin];
            [CATransaction setCompletionBlock:^{
                dispatch_async(dispatch_get_main_queue(),^{
                    if(generation==HGABottomAppSwitch)HGASettleAppSwitchHUDs(YES);
                });
            }];
            HGASettleAppSwitchHUDs(NO);
            [CATransaction commit];
        }];
}
