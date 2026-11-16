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
@property(nonatomic,weak) UIView *parent;
@property(nonatomic,weak) id<UIViewControllerTransitionCoordinator> transition;
@property(nonatomic,copy) NSString *lastStatus;
// Content identity resets compositor ownership when the native canvas is reused.
@property(nonatomic,weak) id contentModel;
@property(nonatomic) BOOL tracksModel;
@end
@implementation HGAPlayerFillState @end
static char HGAPlayerFillKey;
static HGAPlayerFillState *HGAPlayerState(UIView *view,BOOL create) {
    if(!view)return nil;
    HGAPlayerFillState *state=objc_getAssociatedObject(view,&HGAPlayerFillKey);
    if(!state && create) {
        state=[HGAPlayerFillState new];
        objc_setAssociatedObject(view,&HGAPlayerFillKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    return state;
}
static UIView *HGAPlayerCanvas(UIView *view) {
    for(NSUInteger i=0;view && i<13;i++,view=view.superview)
        if([NSStringFromClass(view.class) isEqual:@"FQVShortVideoPlayerCellView"])return view;
    return nil;
}
BOOL HGAppearancePlayerIsTransferring(UIView *view) {
    UIView *canvas=HGAPlayerCanvas(view);
    return HGAPlayerState(view,NO).transferFlags!=0 ||
        HGAPlayerState(canvas,NO).transferFlags!=0;
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
    state.generation++;state.transition=nil;state.yieldedToHost=NO;
    HGARestorePlayerFill(view);
    if([NSStringFromClass(view.class) isEqual:@"FQVShortVideoPlayerCellView"])
        for(UIView *child in HGADescendants(view,100))
            if([NSStringFromClass(child.class) isEqual:@"TTPlayerView"]) {
                HGAPlayerFillState *childState=HGAPlayerState(child,YES);
                childState.generation++;
                childState.transition=nil;childState.yieldedToHost=NO;
                HGARestorePlayerFill(child);
            }
}
static void HGAFinishPlayerTransfer(UIView *view,SEL selector) {
    HGAPlayerFillState *state=HGAPlayerState(view,NO);
    state.transferFlags&=~(selector==@selector(didMoveToSuperview)?1u:2u);
}
static BOOL HGAHasKnownPlayerSurface(UIView *render) {
    // This hierarchy is present in the supplied 7.4.3.33 scene. Do not zoom
    // unknown renderers, mixed overlays, a host-owned layer mask or a custom
    // compositor transform. Keeping native size is the safe compatibility path.
    if(render.subviews.count!=1 || render.layer.sublayers.count!=1 || render.layer.mask)return NO;
    UIView *surface=render.subviews.firstObject;
    // The supplied R12 scene has a 0.192-point Metal overscan on each edge.
    // Accept pixel alignment only; a genuinely inset/cropped surface remains
    // unknown instead of inferring a video aspect ratio from its container.
    CGFloat pixel=1/MAX(1,render.traitCollection.displayScale)+0.01;
    CGRect a=surface.frame,b=render.bounds;
    BOOL aligned=fabs(CGRectGetMinX(a)-CGRectGetMinX(b))<=pixel &&
        fabs(CGRectGetMaxX(a)-CGRectGetMaxX(b))<=pixel &&
        fabs(CGRectGetMinY(a)-CGRectGetMinY(b))<=pixel &&
        fabs(CGRectGetMaxY(a)-CGRectGetMaxY(b))<=pixel;
    return [NSStringFromClass(surface.class) isEqual:@"TTMetalView"] &&
        surface.layer==render.layer.sublayers.firstObject && !surface.hidden && surface.alpha>0.01 &&
        aligned && CGPointEqualToPoint(render.layer.anchorPoint,CGPointMake(0.5,0.5));
}
static BOOL HGAPlayerLandscapeSize(CGSize size) {
    return isfinite(size.width) && isfinite(size.height) && size.height>0 && size.width>size.height;
}
static void HGAApplyFullScreen(UIView *render) {
    HGAPlayerFillState *state=HGAPlayerState(render,NO);
    UIView *canvas=HGAPlayerCanvas(render);
    HGAPlayerFillState *canvasState=HGAPlayerState(canvas,NO);
    // Keep the existing preference key so an upgrade preserves the setting,
    // but fill is now strictly limited to landscape playback. A wide video
    // embedded in a portrait feed must never be enlarged by this preference.
    if(!HGAppearanceEnabled() || !HGAppearanceFlag(@"player.fullscreen")) {
        HGARestorePlayerFill(render);state.yieldedToHost=NO;return;
    }
    if(!render.window || state.transferFlags) {
        HGARestorePlayerFill(render);return;
    }
    // Window, playback canvas and known native video geometry must all agree.
    // This also releases an owned fill as soon as a rotation returns to portrait.
    if(!canvas || canvasState.transferFlags || !CGAffineTransformIsIdentity(render.transform) ||
        !HGAPlayerLandscapeSize(render.window.bounds.size) ||
        !HGAPlayerLandscapeSize(canvas.bounds.size) ||
        !HGAPlayerLandscapeSize(render.bounds.size)) {
        HGARestorePlayerFill(render);return;
    }
    for(UIView *node=render.superview;node;node=node.superview) {
        if(!CGAffineTransformIsIdentity(node.transform)){HGARestorePlayerFill(render);return;}
        if(node==canvas)break;
    }
    if(HGAWaitForPlayerTransition(render,canvas)){HGARestorePlayerFill(render);return;}
    CGSize native=render.bounds.size;
    CGFloat pixel=1/MAX(1,render.traitCollection.displayScale)+0.01;
    if(native.width<120 || native.height<90 || native.width-native.height<=pixel) {
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
    if(!isfinite(zoom)||!isfinite(x)||!isfinite(y)||zoom<1||zoom>6.0) {HGARestorePlayerFill(render);return;}
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

static void HGAResetPlayerFillState(UIView *canvas) {
    HGAPlayerFillState *state=HGAPlayerState(canvas,NO);
    state.contentModel=nil;state.tracksModel=NO;
    for(UIView *render in HGADescendants(canvas,100))
        if([NSStringFromClass(render.class) isEqual:@"TTPlayerView"]) {
            HGAPlayerFillState *fill=HGAPlayerState(render,NO);
            fill.generation++;fill.transition=nil;fill.yieldedToHost=NO;
            HGARestorePlayerFill(render);
        }
}
static void HGAPlayerModelWillChange(UIView *canvas,id model) {
    HGAPlayerFillState *state=HGAPlayerState(canvas,YES);
    // Model identity is used in memory only. Do not retain or log content.
    if(!state.tracksModel || state.contentModel!=model)HGAResetPlayerFillState(canvas);
    state.contentModel=model;state.tracksModel=YES;
}
