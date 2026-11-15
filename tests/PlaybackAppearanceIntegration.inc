// Transition regressions for the production appearance adapters. This file is
// included by AppearanceIntegration.m after its controlled host classes and
// Check/Find/Count/Layout helpers. No real host playback or device acceptance is
// claimed: these fixtures verify layout ownership and native action delivery.
#import <objc/runtime.h>

static char HGPlaybackTransformWritesKey;
static IMP HGPlaybackNativeSetTransform;
static void HGPlaybackCountedTransform(id object, SEL selector, CGAffineTransform value) {
    NSNumber *writes=objc_getAssociatedObject(object,&HGPlaybackTransformWritesKey);
    if(writes)objc_setAssociatedObject(object,&HGPlaybackTransformWritesKey,
        @(writes.unsignedIntegerValue+1),OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    ((void(*)(id,SEL,CGAffineTransform))HGPlaybackNativeSetTransform)(object,selector,value);
}
static void HGPlaybackResetTransformWrites(UIView *view) {
    objc_setAssociatedObject(view,&HGPlaybackTransformWritesKey,@0,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static NSUInteger HGPlaybackTransformWrites(UIView *view) {
    return [objc_getAssociatedObject(view,&HGPlaybackTransformWritesKey) unsignedIntegerValue];
}
static BOOL HGPlaybackInstallTransformProbe(void) {
    static BOOL installed;
    if(installed)return YES;
    // Add an override only to this test's exact TTPlayerView fixture. Changing
    // UIView globally or subclassing the fixture would invalidate the scope
    // being exercised by the production adapter.
    Method inherited=class_getInstanceMethod(TTPlayerView.class,@selector(setTransform:));
    if(!inherited)return NO;
    HGPlaybackNativeSetTransform=method_getImplementation(inherited);
    installed=class_addMethod(TTPlayerView.class,@selector(setTransform:),
        (IMP)HGPlaybackCountedTransform,method_getTypeEncoding(inherited));
    return installed;
}

@interface HGPlaybackNativeEpisodeAction : NSObject
@property(nonatomic,strong) TTPlayerView *render;
@property(nonatomic,strong) UIView *detail;
@property(nonatomic) NSUInteger taps;
@property(nonatomic) BOOL keptWindow;
@property(nonatomic) CGAffineTransform adoptedTransform;
- (void)openEpisodes:(UIControl *)sender;
@end
@implementation HGPlaybackNativeEpisodeAction
- (void)openEpisodes:(UIControl *)sender {
    self.taps++;
    UIWindow *sourceWindow=self.render.window;
    self.detail.hidden=NO;
    [self.detail addSubview:self.render];
    // Capture before asking for a layout pass. A destination must never
    // receive a borrowed render with Honggo's source-canvas transform intact.
    self.adoptedTransform=self.render.transform;
    self.keptWindow=sourceWindow!=nil && self.render.window==sourceWindow;
    self.render.frame=CGRectMake(18,26,220,330);
}
@end

static NSString *HGPlaybackCheckName(BOOL fullscreen,BOOL glass,NSString *name) {
    return [NSString stringWithFormat:@"episode transition [fullscreen=%d glass=%d]: %@",
        fullscreen,glass,name];
}
static void HGPlaybackCheckCombination(UIView *root,BOOL fullscreen,BOOL glass) {
    HGAppearanceApply(@{@"enabled":@YES,@"player.fullscreen":@(fullscreen),
        @"player.episodeGlass":@(glass),@"layout.episodes.hidden":@NO,
        @"layout.episodes.x":@0,@"layout.episodes.y":@0,@"layout.episodes.scale":@1},nil);

    UIView *stage=[[UIView alloc] initWithFrame:CGRectMake(10,100,300,470)];
    [root addSubview:stage];
    FQVShortVideoPlayerCellView *canvas=[[FQVShortVideoPlayerCellView alloc]
        initWithFrame:CGRectMake(0,0,300,420)];
    [stage addSubview:canvas];
    CGRect sourceFrame=CGRectMake(50,0,200,350);
    TTPlayerView *render=[[TTPlayerView alloc] initWithFrame:sourceFrame];
    [canvas addSubview:render];
    UIView *hud=[[UIView alloc] initWithFrame:CGRectMake(248,170,44,44)];
    [canvas addSubview:hud];
    CGRect hudFrame=hud.frame;
    SSShortVideoBottomDistributionView *entry=[[SSShortVideoBottomDistributionView alloc]
        initWithFrame:CGRectMake(8,368,284,44)];
    UIControl *nativeButton=[[UIControl alloc] initWithFrame:entry.bounds];
    [entry addSubview:nativeButton];[canvas addSubview:entry];
    UIView *detail=[[UIView alloc] initWithFrame:stage.bounds];
    detail.hidden=YES;[stage addSubview:detail];
    HGPlaybackNativeEpisodeAction *action=[HGPlaybackNativeEpisodeAction new];
    action.render=render;action.detail=detail;
    [nativeButton addTarget:action action:@selector(openEpisodes:)
        forControlEvents:UIControlEventTouchUpInside];

    @try {
        Layout(canvas);Layout(render);Layout(entry);
        CGAffineTransform filled=render.transform;
        Check(fullscreen ? filled.a>1 && fabs(filled.a-filled.d)<0.001 :
            CGAffineTransformIsIdentity(filled),HGPlaybackCheckName(fullscreen,glass,
            @"portrait rendering follows the fullscreen preference"));
        UIView *material=Find(entry,@"honggo.appearance.material");
        Check(glass ? material.superview==entry && !material.userInteractionEnabled : material==nil,
            HGPlaybackCheckName(fullscreen,glass,@"episode material follows its independent preference"));
        CGPoint tap=CGPointMake(CGRectGetMidX(entry.bounds),CGRectGetMidY(entry.bounds));
        Check([entry hitTest:tap withEvent:nil]==nativeButton,
            HGPlaybackCheckName(fullscreen,glass,@"episode tap still reaches the native control"));

        HGPlaybackResetTransformWrites(render);
        for(NSUInteger pass=0;pass<12;pass++){Layout(canvas);Layout(render);Layout(entry);}
        Check(HGPlaybackTransformWrites(render)==0 &&
            CGAffineTransformEqualToTransform(render.transform,filled),
            HGPlaybackCheckName(fullscreen,glass,@"stable layout performs no further render transform writes"));

        if(glass) {
            UIView *nativeBackdrop=[[UIView alloc] initWithFrame:entry.bounds];
            nativeBackdrop.userInteractionEnabled=NO;
            [entry insertSubview:nativeBackdrop atIndex:0];
            for(NSUInteger pass=0;pass<6;pass++)Layout(entry);
            Check(entry.subviews.firstObject==nativeBackdrop && material.superview==entry &&
                Count(entry,@"honggo.appearance.material")==1,
                HGPlaybackCheckName(fullscreen,glass,@"native subview order is not repeatedly overwritten"));
        }

        HGPlaybackResetTransformWrites(render);
        [nativeButton sendActionsForControlEvents:UIControlEventTouchUpInside];
        Check(action.taps==1 && render.superview==detail && action.keptWindow,
            HGPlaybackCheckName(fullscreen,glass,@"native episode action executes once and transfers within the same window"));
        Check(CGAffineTransformIsIdentity(action.adoptedTransform) &&
            HGPlaybackTransformWrites(render)==(fullscreen?1u:0u),
            HGPlaybackCheckName(fullscreen,glass,@"owned transform is restored once before native detail adoption"));
        CGRect detailFrame=render.frame;
        HGPlaybackResetTransformWrites(render);
        for(NSUInteger pass=0;pass<6;pass++){Layout(detail);Layout(render);}
        Check(CGRectEqualToRect(render.frame,detailFrame) && CGAffineTransformIsIdentity(render.transform) &&
            HGPlaybackTransformWrites(render)==0,
            HGPlaybackCheckName(fullscreen,glass,@"detail layout retains its native geometry without repeated writes"));

        CGAffineTransform nativeTransition=CGAffineTransformMakeTranslation(24,-10);
        render.transform=nativeTransition;
        HGPlaybackResetTransformWrites(render);Layout(render);
        Check(CGAffineTransformEqualToTransform(render.transform,nativeTransition) &&
            HGPlaybackTransformWrites(render)==0,
            HGPlaybackCheckName(fullscreen,glass,@"native detail transition transform is preserved"));

        // Model a completed or cancelled native detail transition returning
        // the same renderer to the existing feed canvas.
        render.transform=CGAffineTransformIdentity;render.frame=sourceFrame;
        HGPlaybackResetTransformWrites(render);
        [canvas addSubview:render];detail.hidden=YES;
        Layout(canvas);Layout(render);
        Check(CGAffineTransformEqualToTransform(render.transform,filled) &&
            HGPlaybackTransformWrites(render)==(fullscreen?1u:0u),
            HGPlaybackCheckName(fullscreen,glass,@"returning to the feed applies the desired rendering exactly once"));
        Check(action.taps==1 && CGRectEqualToRect(hud.frame,hudFrame) &&
            CGRectEqualToRect(entry.frame,CGRectMake(8,368,284,44)) &&
            CGRectEqualToRect(nativeButton.frame,entry.bounds),
            HGPlaybackCheckName(fullscreen,glass,@"return preserves native HUD geometry and action count"));

        // A host transform written while the view is still in the feed wins
        // over the previous plugin-owned value, including during cleanup.
        render.transform=nativeTransition;
        HGPlaybackResetTransformWrites(render);Layout(canvas);Layout(render);
        Check(CGAffineTransformEqualToTransform(render.transform,nativeTransition) &&
            HGPlaybackTransformWrites(render)==0,
            HGPlaybackCheckName(fullscreen,glass,@"a later host transform in the feed is not overwritten"));
        [render removeFromSuperview];
        Check(CGAffineTransformEqualToTransform(render.transform,nativeTransition) &&
            HGPlaybackTransformWrites(render)==0,
            HGPlaybackCheckName(fullscreen,glass,@"detach preserves a later host transform"));

        render.transform=CGAffineTransformIdentity;render.frame=sourceFrame;
        [canvas addSubview:render];Layout(canvas);Layout(render);
        HGPlaybackResetTransformWrites(render);[render removeFromSuperview];
        Check(render.window==nil && CGAffineTransformIsIdentity(render.transform) &&
            CGRectEqualToRect(render.frame,sourceFrame) &&
            HGPlaybackTransformWrites(render)==(fullscreen?1u:0u),
            HGPlaybackCheckName(fullscreen,glass,@"detach restores only owned source geometry once"));
    } @finally {
        [nativeButton removeTarget:action action:@selector(openEpisodes:)
            forControlEvents:UIControlEventTouchUpInside];
        [stage removeFromSuperview];
        objc_setAssociatedObject(render,&HGPlaybackTransformWritesKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
}

static char HGPlaybackMaterialFrameWritesKey;
static IMP HGPlaybackNativeSetMaterialFrame;
static void HGPlaybackCountedMaterialFrame(id object,SEL selector,CGRect frame) {
    NSNumber *writes=objc_getAssociatedObject(object,&HGPlaybackMaterialFrameWritesKey);
    if(writes)objc_setAssociatedObject(object,&HGPlaybackMaterialFrameWritesKey,
        @(writes.unsignedIntegerValue+1),OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    ((void(*)(id,SEL,CGRect))HGPlaybackNativeSetMaterialFrame)(object,selector,frame);
}
static void HGPlaybackCheckMaterialGeometry(UIView *root) {
    HGAppearanceApply(@{@"enabled":@YES,@"player.rightGlass":@YES,
        @"layout.comment.hidden":@NO,@"layout.comment.x":@0,
        @"layout.comment.y":@0,@"layout.comment.scale":@1},nil);
    SSShortVideoRightInteractionCommentView *action=[[SSShortVideoRightInteractionCommentView alloc]
        initWithFrame:CGRectMake(20,140,54,65)];
    [root addSubview:action];Layout(action);
    UIView *material=Find(action,@"honggo.appearance.material");
    Layout(material);Layout(action);
    Check(material!=nil && material.bounds.size.height<action.bounds.size.height,
        @"action glass uses its final circular geometry");
    if(!material){[action removeFromSuperview];return;}

    // Derive from the actual production material class, so all its behavior
    // remains present. Only this one test instance gets the setter probe;
    // UIVisualEffectView and UIView implementations are never changed.
    Class nativeClass=object_getClass(material);
    Class probeClass=objc_allocateClassPair(nativeClass,"HGPlaybackMaterialFrameProbe",0);
    Method setter=class_getInstanceMethod(nativeClass,@selector(setFrame:));
    BOOL probeReady=probeClass!=Nil && setter!=NULL;
    if(probeReady) {
        HGPlaybackNativeSetMaterialFrame=method_getImplementation(setter);
        probeReady=class_addMethod(probeClass,@selector(setFrame:),
            (IMP)HGPlaybackCountedMaterialFrame,method_getTypeEncoding(setter));
    }
    Check(probeReady,@"material geometry probe is confined to one controlled instance");
    if(!probeReady) {
        if(probeClass)objc_disposeClassPair(probeClass);
        [action removeFromSuperview];return;
    }
    objc_registerClassPair(probeClass);object_setClass(material,probeClass);
    @try {
        CGRect frame=material.frame;
        objc_setAssociatedObject(material,&HGPlaybackMaterialFrameWritesKey,@0,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        for(NSUInteger pass=0;pass<12;pass++)Layout(action);
        NSUInteger writes=[objc_getAssociatedObject(material,&HGPlaybackMaterialFrameWritesKey) unsignedIntegerValue];
        Check(writes==0 && CGRectEqualToRect(material.frame,frame) &&
            Find(action,@"honggo.appearance.material")==material,
            @"stable action layouts never expand glass to owner bounds and shrink it again");
    } @finally {
        object_setClass(material,nativeClass);
        objc_setAssociatedObject(material,&HGPlaybackMaterialFrameWritesKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        [action removeFromSuperview];
    }
}

static void RunPlaybackAppearanceChecks(UIView *root) {
    NSDictionary *saved=[HGAppearanceValues() copy];
    Check(HGPlaybackInstallTransformProbe(),@"render transform probe is confined to the TTPlayerView test fixture");
    @try {
        for(NSNumber *fullscreen in @[@NO,@YES])
            for(NSNumber *glass in @[@NO,@YES])
                HGPlaybackCheckCombination(root,fullscreen.boolValue,glass.boolValue);
        HGPlaybackCheckMaterialGeometry(root);
    } @finally {
        HGAppearanceApply(saved,nil);
    }
}
