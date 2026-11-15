// Keep the host's frame/bounds/center and UIView.transform entirely native.
// A borrowed TTPlayerView is laid out and reparented by the host; its frame is
// undefined while a nonidentity UIView transform is installed. Enlarge only
// the observed TTMetalView rendering subtree through its parent's compositor.
@interface HGAPlayerFillState : NSObject
@property(nonatomic) CATransform3D source;
@property(nonatomic) CATransform3D applied;
@property(nonatomic) BOOL ownsFill;
@property(nonatomic) BOOL releasedClipping;
@property(nonatomic) BOOL sourceClipping;
@property(nonatomic) BOOL yieldedToHost;
@property(nonatomic) NSUInteger transferFlags;
@property(nonatomic) NSUInteger generation;
@property(nonatomic) BOOL ancestorTransferring;
@property(nonatomic,weak) UIView *parent;
@property(nonatomic,weak) id<UIViewControllerTransitionCoordinator> transition;
@property(nonatomic,copy) NSString *lastStatus;
@end
@implementation HGAPlayerFillState @end
static char HGAPlayerFillKey;
static HGAPlayerFillState *HGAPlayerState(UIView *view,BOOL create) {
    HGAPlayerFillState *state=objc_getAssociatedObject(view,&HGAPlayerFillKey);
    if(!state && create) {
        state=[HGAPlayerFillState new];
        objc_setAssociatedObject(view,&HGAPlayerFillKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    return state;
}
static void HGAPlayerFillStatus(UIView *render,NSString *status,CGFloat zoom) {
    HGAPlayerFillState *state=HGAPlayerState(render,YES);
    if([state.lastStatus isEqual:status])return;
    state.lastStatus=status;
    if(HGEnhancedDiagnostics())HGLog(@"appearance.player-fill",@{
        @"mode":@"render-sublayers",@"status":status,@"scale":@(zoom),
        @"nativeViewTransform":@(CGAffineTransformIsIdentity(render.transform)),
        @"width":@(render.bounds.size.width),@"height":@(render.bounds.size.height)});
}
static void HGARestorePlayerFill(UIView *render) {
    HGAPlayerFillState *state=HGAPlayerState(render,NO);
    if(!state)return;
    BOOL owned=state.ownsFill && CATransform3DEqualToTransform(render.layer.sublayerTransform,state.applied);
    BOOL restoreClipping=state.releasedClipping && !render.layer.masksToBounds;
    CATransform3D source=state.source;
    BOOL sourceClipping=state.sourceClipping;
    // Relinquish ownership before any setter can synchronously call back.
    state.ownsFill=NO;state.releasedClipping=NO;state.parent=nil;
    if(owned || restoreClipping) {
        [CATransaction begin];[CATransaction setDisableActions:YES];
        if(owned)render.layer.sublayerTransform=source;
        if(restoreClipping)render.layer.masksToBounds=sourceClipping;
        [CATransaction commit];
    }
}
static BOOL HGAWaitForPlayerTransition(UIView *render,UIView *canvas) {
    id<UIViewControllerTransitionCoordinator> transition=HGAOwner(canvas).transitionCoordinator;
    if(!transition)return NO;
    HGAPlayerFillState *state=HGAPlayerState(render,YES);
    if(state.transition!=transition) {
        state.transition=transition;
        NSUInteger generation=state.generation;
        __weak UIView *weakRender=render;
        __weak UIView *weakParent=render.superview;
        __weak id<UIViewControllerTransitionCoordinator> weakTransition=transition;
        // A completion may be retained even when no animation was queued.
        [transition animateAlongsideTransition:nil completion:^(id<UIViewControllerTransitionCoordinatorContext> context) {
            UIView *view=weakRender;
            HGAPlayerFillState *current=HGAPlayerState(view,NO);
            if(!current || current.transition!=weakTransition || current.generation!=generation)return;
            current.transition=nil;
            if(view.window && view.superview==weakParent)[view setNeedsLayout];
        }];
    }
    return YES;
}
static void HGAPreparePlayerTransfer(UIView *view,SEL selector) {
    HGAPlayerFillState *state=HGAPlayerState(view,YES);
    state.transferFlags|=selector==@selector(willMoveToSuperview:)?1u:2u;
    // A renderer can be borrowed while its old canvas is still moving. The
    // old canvas cannot clear a flag on a renderer it no longer contains.
    // Its own new-parent transfer, and the actual canvas flags below, now gate it.
    if(selector==@selector(willMoveToSuperview:))state.ancestorTransferring=NO;
    state.generation++;state.transition=nil;state.yieldedToHost=NO;
    HGARestorePlayerFill(view);
    if([NSStringFromClass(view.class) isEqual:@"FQVShortVideoPlayerCellView"])
        for(UIView *child in HGADescendants(view,100))
            if([NSStringFromClass(child.class) isEqual:@"TTPlayerView"]) {
                HGAPlayerFillState *childState=HGAPlayerState(child,YES);
                childState.ancestorTransferring=YES;childState.generation++;
                childState.transition=nil;childState.yieldedToHost=NO;
                HGARestorePlayerFill(child);
            }
}
static void HGAFinishPlayerTransfer(UIView *view,SEL selector) {
    HGAPlayerFillState *state=HGAPlayerState(view,NO);
    state.transferFlags&=~(selector==@selector(didMoveToSuperview)?1u:2u);
    if(!state.transferFlags && [NSStringFromClass(view.class) isEqual:@"FQVShortVideoPlayerCellView"])
        for(UIView *child in HGADescendants(view,100))
            if([NSStringFromClass(child.class) isEqual:@"TTPlayerView"])
                HGAPlayerState(child,NO).ancestorTransferring=NO;
}
static BOOL HGAHasKnownPlayerSurface(UIView *render) {
    // This hierarchy is present in the supplied 7.4.3.33 scene. Do not zoom
    // unknown renderers, mixed overlays, a host-owned layer mask or a custom
    // compositor transform. Keeping native size is the safe compatibility path.
    if(render.subviews.count!=1 || render.layer.sublayers.count!=1 || render.layer.mask)return NO;
    UIView *surface=render.subviews.firstObject;
    return [NSStringFromClass(surface.class) isEqual:@"TTMetalView"] &&
        surface.layer==render.layer.sublayers.firstObject && !surface.hidden && surface.alpha>0.01 &&
        CGRectEqualToRect(surface.frame,render.bounds) &&
        CGPointEqualToPoint(render.layer.anchorPoint,CGPointMake(0.5,0.5));
}
static void HGAApplyFullScreen(UIView *render) {
    HGAPlayerFillState *state=HGAPlayerState(render,NO);
    if(!HGAppearanceEnabled() || !HGAppearanceFlag(@"player.fullscreen")) {
        HGARestorePlayerFill(render);state.yieldedToHost=NO;return;
    }
    if(!render.window || state.transferFlags || state.ancestorTransferring) {
        HGARestorePlayerFill(render);return;
    }
    UIView *canvas=render.superview;
    for(NSUInteger i=0;canvas && i<12;i++,canvas=canvas.superview)
        if([NSStringFromClass(canvas.class) isEqual:@"FQVShortVideoPlayerCellView"])break;
    HGAPlayerFillState *canvasState=HGAPlayerState(canvas,NO);
    if(!canvas || ![NSStringFromClass(canvas.class) isEqual:@"FQVShortVideoPlayerCellView"] ||
        canvasState.transferFlags || !CGAffineTransformIsIdentity(render.transform) ||
        render.window.bounds.size.width>=render.window.bounds.size.height) {
        HGARestorePlayerFill(render);return;
    }
    for(UIView *node=render.superview;node;node=node.superview) {
        if(!CGAffineTransformIsIdentity(node.transform)){HGARestorePlayerFill(render);return;}
        if(node==canvas)break;
    }
    if(HGAWaitForPlayerTransition(render,canvas)){HGARestorePlayerFill(render);return;}
    CGSize native=render.bounds.size;
    if(native.width<120 || native.height<160 || native.width>=native.height*0.85) {
        HGARestorePlayerFill(render);return;
    }
    if(!HGAHasKnownPlayerSurface(render)) {
        HGARestorePlayerFill(render);HGAPlayerFillStatus(render,@"native-unknown-surface",1);return;
    }
    state=HGAPlayerState(render,YES);
    if(state.ownsFill && (!CATransform3DEqualToTransform(render.layer.sublayerTransform,state.applied) ||
            render.layer.masksToBounds)) {
        // A host overwrite, including identity, wins for this attachment.
        // Never compete with a native compositor by writing on every layout.
        state.yieldedToHost=YES;HGARestorePlayerFill(render);
    }
    if(state.yieldedToHost) {HGAPlayerFillStatus(render,@"native-host-override",1);return;}
    if(state.ownsFill && state.parent!=render.superview)HGARestorePlayerFill(render);
    CATransform3D source=state.ownsFill?state.source:render.layer.sublayerTransform;
    if(!CATransform3DIsIdentity(source)) {
        HGARestorePlayerFill(render);HGAPlayerFillStatus(render,@"native-host-compositor",1);return;
    }
    CGRect rect=[canvas convertRect:canvas.bounds toView:render.superview];
    CGFloat zoom=MAX(rect.size.width/native.width,rect.size.height/native.height);
    CGFloat x=CGRectGetMidX(rect)-render.center.x,y=CGRectGetMidY(rect)-render.center.y;
    if(!isfinite(zoom)||!isfinite(x)||!isfinite(y)||zoom<1||zoom>1.6) {HGARestorePlayerFill(render);return;}
    if(fabs(zoom-1)<0.001 && fabs(x)<0.001 && fabs(y)<0.001) {HGARestorePlayerFill(render);return;}
    CATransform3D next=CATransform3DMakeScale(zoom,zoom,1);next.m41=x;next.m42=y;
    state.source=source;state.applied=next;state.ownsFill=YES;state.parent=render.superview;
    [CATransaction begin];[CATransaction setDisableActions:YES];
    if(!CATransform3DEqualToTransform(render.layer.sublayerTransform,next))render.layer.sublayerTransform=next;
    // The original view rect stays native; release only this renderer's crop
    // while its known video subtree is enlarged. The canvas still clips pages.
    if(render.layer.masksToBounds) {
        state.sourceClipping=YES;state.releasedClipping=YES;render.layer.masksToBounds=NO;
    }
    [CATransaction commit];
    HGAPlayerFillStatus(render,@"applied",zoom);
}
