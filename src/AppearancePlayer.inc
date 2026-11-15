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
// A user override belongs to one native content canvas, never to a borrowed
// renderer. 0 follows the portrait preference, 1 keeps native fit, 2 fills.
@property(nonatomic) NSInteger manualMode;
@property(nonatomic,weak) id contentModel;
@property(nonatomic) BOOL tracksModel;
@property(nonatomic) BOOL tracksPortraitPreference;
@property(nonatomic) BOOL portraitPreference;
@property(nonatomic,strong) UIButton *fillButton;
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
static BOOL HGAPlayerFillControlEnabled(void) {
    return HGAppearanceEnabled() && [HGAppearanceString(@"player.fillButton") isEqual:@"show"];
}
static void HGARefreshPlayerFillControls(UIView *canvas);
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
    HGAPlayerState(HGAPlayerCanvas(view),NO).fillButton.hidden=YES;
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
static void HGAApplyFullScreen(UIView *render) {
    HGAPlayerFillState *state=HGAPlayerState(render,NO);
    UIView *canvas=HGAPlayerCanvas(render);
    HGAPlayerFillState *canvasState=HGAPlayerState(canvas,YES);
    BOOL portraitPreference=HGAppearanceFlag(@"player.fullscreen");
    // A later explicit setting change supersedes a per-content fit/fill choice.
    // Unrelated appearance updates leave that local choice untouched.
    if(canvasState.tracksPortraitPreference && canvasState.portraitPreference!=portraitPreference)
        canvasState.manualMode=0;
    canvasState.tracksPortraitPreference=YES;canvasState.portraitPreference=portraitPreference;
    if(!HGAPlayerFillControlEnabled())canvasState.manualMode=0;
    BOOL manual=canvasState.manualMode==2;
    BOOL wantsFill=manual || (canvasState.manualMode==0 && portraitPreference);
    if(!HGAppearanceEnabled() || !wantsFill) {
        HGARestorePlayerFill(render);state.yieldedToHost=NO;return;
    }
    if(!render.window || state.transferFlags) {
        HGARestorePlayerFill(render);return;
    }
    // Resolve the current canvas instead of copying its transition state onto
    // a renderer that the host may borrow together with an intermediate view.
    if(!canvas || ![NSStringFromClass(canvas.class) isEqual:@"FQVShortVideoPlayerCellView"] ||
        canvasState.transferFlags || !CGAffineTransformIsIdentity(render.transform) ||
        (!manual && render.window.bounds.size.width>=render.window.bounds.size.height)) {
        HGARestorePlayerFill(render);return;
    }
    for(UIView *node=render.superview;node;node=node.superview) {
        if(!CGAffineTransformIsIdentity(node.transform)){HGARestorePlayerFill(render);return;}
        if(node==canvas)break;
    }
    if(HGAWaitForPlayerTransition(render,canvas)){HGARestorePlayerFill(render);return;}
    CGSize native=render.bounds.size;
    if(native.width<120 || native.height<(manual?90:160) ||
        (!manual && native.width>=native.height*0.85)) {
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
    if(!isfinite(zoom)||!isfinite(x)||!isfinite(y)||zoom<1||zoom>(manual?6.0:1.6)) {HGARestorePlayerFill(render);return;}
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

static UIView *HGAPlayerFillRenderer(UIView *canvas) {
    for(UIView *view in HGADescendants(canvas,100))
        if([NSStringFromClass(view.class) isEqual:@"TTPlayerView"])return view;
    return nil;
}
static UIView *HGAPlayerFillButtonContainer(UIView *canvas) {
    // These broad native HUD containers hide/fade with clean-screen mode.
    // The new control inherits that behavior without observing or changing
    // the playback state, a context callback or any global UIKit setter.
    for(UIView *view in HGADescendants(canvas,100)) {
        NSString *name=NSStringFromClass(view.class);
        if(( [name isEqual:@"SSShortVideoDerivateInfoView"] ||
             [name isEqual:@"SSShortVideoOutterSeriesInfoView"] ) &&
            view.bounds.size.width>=canvas.bounds.size.width*0.8 &&
            view.bounds.size.height>=canvas.bounds.size.height*0.7)return view;
    }
    for(UIView *view in canvas.subviews)
        if([NSStringFromClass(view.class) isEqual:@"FQVShortVideoDerivativeVideoControlView"] ||
           [NSStringFromClass(view.class) isEqual:@"FQVShortVideoControlView"])return view;
    return canvas;
}
static BOOL HGAPlayerFillIsCurrentCanvas(UIView *canvas) {
    if(!HGAVisible(canvas))return NO;
    for(UIView *node=canvas.superview;node;node=node.superview) {
        if([node isKindOfClass:UICollectionView.class]) {
            CGRect page=[canvas convertRect:canvas.bounds toView:node];
            return CGRectContainsPoint(page,CGPointMake(CGRectGetMidX(node.bounds),CGRectGetMidY(node.bounds)));
        }
    }
    return YES;
}
static void HGAResetPlayerFillControl(UIView *canvas) {
    HGAPlayerFillState *state=HGAPlayerState(canvas,NO);
    state.manualMode=0;state.contentModel=nil;state.tracksModel=NO;
    state.fillButton.hidden=YES;
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
    if(!state.tracksModel || state.contentModel!=model)HGAResetPlayerFillControl(canvas);
    state.contentModel=model;state.tracksModel=YES;
}
@interface HGAPlayerFillButton : UIButton
@property(nonatomic,weak) UIView *canvas;
@property(nonatomic,weak) UIView *renderer;
- (void)toggleFill;
@end
@implementation HGAPlayerFillButton
- (instancetype)initWithFrame:(CGRect)frame {
    if((self=[super initWithFrame:frame])) {
        self.accessibilityIdentifier=@"honggo.appearance.player.fill";
        self.tintColor=UIColor.whiteColor;
        self.automaticallyUpdatesConfiguration=NO;
        self.configurationUpdateHandler=^(UIButton *control) {
            control.alpha=control.highlighted?0.72:1;
        };
        self.layer.cornerCurve=kCACornerCurveContinuous;
        [self addTarget:self action:@selector(toggleFill) forControlEvents:UIControlEventTouchUpInside];
    }
    return self;
}
- (void)toggleFill {
    UIView *canvas=self.canvas,*render=self.renderer;
    if(!HGAPlayerFillControlEnabled() || !HGAVisible(self) ||
        !HGAPlayerFillIsCurrentCanvas(canvas) || HGAPlayerCanvas(render)!=canvas ||
        HGAPlayerState(canvas,NO).transferFlags || HGAPlayerState(render,NO).transferFlags ||
        HGAOwner(canvas).transitionCoordinator || !HGAHasKnownPlayerSurface(render))return;
    HGAPlayerFillState *state=HGAPlayerState(canvas,YES);
    state.manualMode=HGAPlayerState(render,NO).ownsFill?1:2;
    HGAppearanceProcessView(render);
    HGARefreshPlayerFillControls(canvas);
    if(HGEnhancedDiagnostics())HGLog(@"appearance.player-fill-control",@{
        @"requested":state.manualMode==2?@"fill":@"fit",
        @"applied":@(HGAPlayerState(render,NO).ownsFill),@"mode":@"render-sublayers"});
}
@end
static void HGARefreshPlayerFillControls(UIView *canvas) {
    if(!canvas)return;
    HGAPlayerFillState *state=HGAPlayerState(canvas,NO);
    if(!HGAPlayerFillControlEnabled()) {
        state.manualMode=0;
        UIButton *button=state.fillButton;state.fillButton=nil;
        [button removeFromSuperview];return;
    }
    UIView *render=HGAPlayerFillRenderer(canvas);
    HGAPlayerFillState *fill=HGAPlayerState(render,NO);
    BOOL nativeTransforms=render && CGAffineTransformIsIdentity(render.transform);
    for(UIView *node=render.superview;node && nativeTransforms;node=node.superview) {
        nativeTransforms=CGAffineTransformIsIdentity(node.transform);
        if(node==canvas)break;
    }
    CGSize native=render.bounds.size;
    CGFloat manualZoom=MAX(canvas.bounds.size.width/MAX(native.width,1),canvas.bounds.size.height/MAX(native.height,1));
    BOOL ready=render && HGAVisible(render) && HGAPlayerFillIsCurrentCanvas(canvas) && nativeTransforms &&
        isfinite(manualZoom) && manualZoom>=1 && manualZoom<=6 &&
        !state.transferFlags && !fill.transferFlags &&
        !HGAOwner(canvas).transitionCoordinator && HGAHasKnownPlayerSurface(render) &&
        !fill.yieldedToHost && (fill.ownsFill || CATransform3DIsIdentity(render.layer.sublayerTransform)) &&
        render.bounds.size.width>=120 && render.bounds.size.height>=90;
    if(!ready){state.fillButton.hidden=YES;return;}
    state=HGAPlayerState(canvas,YES);
    if(!state.tracksModel) {
        id model=HGReadObjectGetter(canvas,"model");
        if(model){state.contentModel=model;state.tracksModel=YES;}
    }
    UIView *container=HGAPlayerFillButtonContainer(canvas);
    HGAPlayerFillButton *button=(id)state.fillButton;
    if(!button){button=[[HGAPlayerFillButton alloc] initWithFrame:CGRectZero];state.fillButton=button;}
    button.canvas=canvas;button.renderer=render;
    BOOL active=fill.ownsFill;
    if(button.selected!=active || !button.configuration) {
        button.selected=active;
        NSString *title=active?@"还原比例":@"铺满屏幕";
        UIButtonConfiguration *configuration=[UIButtonConfiguration plainButtonConfiguration];
        configuration.automaticallyUpdateForSelection=NO;
        configuration.imageColorTransformer=^UIColor *(UIColor *incoming){return UIColor.whiteColor;};
        configuration.attributedTitle=[[NSAttributedString alloc] initWithString:title attributes:@{
            NSFontAttributeName:[UIFont systemFontOfSize:13 weight:UIFontWeightSemibold],
            NSForegroundColorAttributeName:UIColor.whiteColor}];
        configuration.image=[[UIImage systemImageNamed:active?@"arrow.down.right.and.arrow.up.left":@"arrow.up.left.and.arrow.down.right"
            withConfiguration:[UIImageSymbolConfiguration configurationWithPointSize:13 weight:UIImageSymbolWeightMedium]]
            imageWithTintColor:UIColor.whiteColor renderingMode:UIImageRenderingModeAlwaysOriginal];
        configuration.imagePadding=6;configuration.baseForegroundColor=UIColor.whiteColor;
        configuration.background.backgroundColor=[UIColor colorWithWhite:0 alpha:0.38];
        configuration.background.strokeColor=[UIColor colorWithWhite:1 alpha:0.16];
        configuration.background.strokeWidth=0.5;configuration.background.cornerRadius=22;
        configuration.cornerStyle=UIButtonConfigurationCornerStyleFixed;
        configuration.contentInsets=NSDirectionalEdgeInsetsMake(0,12,0,12);
        button.configuration=configuration;button.accessibilityLabel=title;
        button.accessibilityValue=active?@"画面已等比裁切铺满":@"显示完整画面";
        button.accessibilityHint=active?@"恢复视频原始比例，显示完整画面":@"等比放大并裁切画面边缘，填满播放区域";
    }
    CGRect nativeRect=[render convertRect:render.bounds toView:canvas];
    CGFloat width=MIN(130,canvas.bounds.size.width-32),height=44;
    CGFloat top=MAX(12,canvas.safeAreaInsets.top+12);
    CGFloat bottomSpace=MIN(210,canvas.bounds.size.height*0.24);
    CGFloat y=MAX(top,MIN(CGRectGetMaxY(nativeRect)+12,CGRectGetMaxY(canvas.bounds)-bottomSpace-height));
    CGRect frame=[canvas convertRect:CGRectMake(CGRectGetMinX(canvas.bounds)+16,y,width,height) toView:container];
    if(width<100 || !CGRectContainsRect(container.bounds,frame)){button.hidden=YES;return;}
    if(button.superview!=container)[container addSubview:button];
    if(!CGRectEqualToRect(button.frame,frame))button.frame=frame;
    button.hidden=NO;
}
