// R13: reserve a separate touch strip below the episode capsule. The supplied
// scene places the 44-point entry at y=822 and the progress owner at y=841.
// Move the entry's native insert wrapper and its adjacent info group together;
// the host still owns the slider, video canvas, constraints and all actions.
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
@property(nonatomic) CGFloat lastLift;
@end
@implementation HGABottomLayoutState @end
static char HGABottomLayoutKey;

static NSArray<NSString *> *HGABottomControlClasses(void) {
    // Exact control owner captured in the R12 screenshot diagnostic. Unknown
    // host layouts keep their native spacing until their geometry is verified.
    return @[@"FQVShortVideoDerivativeVideoControlView"];
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
    UIView *info=state.info.view,*entry=state.entry.view;
    CGPoint oldInfo=info.center,oldEntry=entry.center;
    HGARestoreBottomPosition(state.entry);HGARestoreBottomPosition(state.info);
    state.lastLift=0;
    if((info && !CGPointEqualToPoint(oldInfo,info.center)) ||
        (entry && !CGPointEqualToPoint(oldEntry,entry.center)))
        HGARefreshPlayerFillControls(HGAPlayerCanvas(info ?: entry));
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
static void HGAUpdateBottomLayoutForView(UIView *view) {
    UIView *control=HGABottomControlForView(view);
    if(!control)return;
    HGABottomLayoutState *state=objc_getAssociatedObject(control,&HGABottomLayoutKey);
    if(!HGAppearanceEnabled() || ![HGAppearanceString(@"player.separateProgress") isEqual:@"show"] ||
        !HGAVisible(control) || control.bounds.size.height<=control.bounds.size.width ||
        HGABottomHasCustomGeometry()) {HGARestoreBottomState(state);return;}
    UIView *progress=nil,*wrapper=nil,*info=nil;
    BOOL ambiguous=NO;
    for(UIView *child in control.subviews) {
        if(!HGAVisible(child))continue;
        NSString *name=NSStringFromClass(child.class);
        if([name isEqual:@"FQVShortVideoProgressView"]) {
            if(progress){ambiguous=YES;break;}progress=child;
        } else if([name isEqual:@"FQVShortVideoBaseInsertView"] && child.subviews.count==1 &&
            [HGARole(child.subviews.firstObject) isEqual:@"episodes"] && HGAVisible(child.subviews.firstObject)) {
            if(wrapper){ambiguous=YES;break;}wrapper=child;
        } else if([name isEqual:@"SSShortVideoDerivateInfoView"] ||
            [name isEqual:@"SSShortVideoOutterSeriesInfoView"]) {
            if(info){ambiguous=YES;break;}info=child;
        }
    }
    if(ambiguous || !progress || !wrapper || !CGAffineTransformIsIdentity(wrapper.transform) ||
        !CGAffineTransformIsIdentity(progress.transform) || (info && !CGAffineTransformIsIdentity(info.transform))) {
        HGARestoreBottomState(state);return;
    }
    if(!state) {
        state=[HGABottomLayoutState new];
        objc_setAssociatedObject(control,&HGABottomLayoutKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        HGARegisterView(control);
    }
    state.entry=HGABottomPositionForView(state.entry,wrapper);
    state.info=HGABottomPositionForView(state.info,info);
    CGRect entryRect=HGABottomNativeRect(wrapper,control,state.entry);
    CGRect progressRect=[progress convertRect:progress.bounds toView:control];
    CGFloat width=control.bounds.size.width,height=control.bounds.size.height;
    BOOL ordinary=entryRect.size.height>=32 && entryRect.size.height<=80 &&
        entryRect.size.width>=width*0.65 && progressRect.size.width>=width*0.65 &&
        progressRect.size.height>=16 && progressRect.size.height<=80 &&
        CGRectGetMinY(entryRect)>height*0.55 && CGRectGetMinY(progressRect)>height*0.60;
    CGFloat lift=MAX(0,CGRectGetMaxY(entryRect)-CGRectGetMinY(progressRect)+8);
    if(!ordinary || !isfinite(lift) || lift>64) {HGARestoreBottomState(state);return;}
    if(info) {
        CGRect infoRect=HGABottomNativeRect(info,control,state.info);
        if(CGRectGetMaxY(infoRect)>CGRectGetMinY(entryRect)+2 ||
            CGRectGetMaxY(infoRect)<CGRectGetMinY(entryRect)-80) {
            HGARestoreBottomState(state);return;
        }
    }
    CGPoint oldInfo=info.center,oldEntry=wrapper.center;
    [CATransaction begin];[CATransaction setDisableActions:YES];
    HGAApplyBottomPosition(state.entry,lift);HGAApplyBottomPosition(state.info,lift);
    [CATransaction commit];
    // The fill button lives in this native HUD. Recompute its canvas-relative
    // frame after moving the HUD, within the same guarded layout pass.
    if((info && !CGPointEqualToPoint(oldInfo,info.center)) ||
        !CGPointEqualToPoint(oldEntry,wrapper.center))
        HGARefreshPlayerFillControls(HGAPlayerCanvas(control));
    if(fabs(state.lastLift-lift)>0.01 && HGEnhancedDiagnostics())
        HGLog(@"appearance.episode-spacing",@{@"lift":@(lift),@"touchGap":@8,
            @"sliderPreserved":@YES,@"mode":@"native-wrapper-centers"});
    state.lastLift=lift;
}
