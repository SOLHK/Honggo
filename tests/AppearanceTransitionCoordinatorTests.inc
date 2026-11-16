// Completion-only transition registration can return NO while retaining its
// completion. These controlled coordinators exercise that public UIKit contract
// against the production appearance adapter, without emulating video playback.
@interface HGPlaybackTransitionCoordinator : NSObject <UIViewControllerTransitionCoordinator>
@property(nonatomic) BOOL acceptsAnimations;
@property(nonatomic) BOOL transitionCancelled;
@property(nonatomic) NSUInteger registrations;
@property(nonatomic,strong) UIView *containerView;
@property(nonatomic,strong) NSMutableArray *completions;
- (void)finishCancelled:(BOOL)cancelled;
@end
@implementation HGPlaybackTransitionCoordinator
- (instancetype)init {
    if((self=[super init]))_completions=[NSMutableArray array];
    return self;
}
- (BOOL)isAnimated{return YES;}
- (UIModalPresentationStyle)presentationStyle{return UIModalPresentationNone;}
- (BOOL)initiallyInteractive{return YES;}
- (BOOL)isInterruptible{return YES;}
- (BOOL)isInteractive{return YES;}
- (BOOL)isCancelled{return self.transitionCancelled;}
- (NSTimeInterval)transitionDuration{return 0.25;}
- (CGFloat)percentComplete{return 0.5;}
- (CGFloat)completionVelocity{return 1;}
- (UIViewAnimationCurve)completionCurve{return UIViewAnimationCurveEaseInOut;}
- (CGAffineTransform)targetTransform{return CGAffineTransformIdentity;}
- (UIViewController *)viewControllerForKey:(UITransitionContextViewControllerKey)key{return nil;}
- (UIView *)viewForKey:(UITransitionContextViewKey)key{return nil;}
- (BOOL)animateAlongsideTransition:(void (^)(id<UIViewControllerTransitionCoordinatorContext>))animation
        completion:(void (^)(id<UIViewControllerTransitionCoordinatorContext>))completion {
    self.registrations++;
    if(animation && self.acceptsAnimations)animation(self);
    if(completion)[self.completions addObject:[completion copy]];
    return self.acceptsAnimations;
}
- (BOOL)animateAlongsideTransitionInView:(UIView *)view
        animation:(void (^)(id<UIViewControllerTransitionCoordinatorContext>))animation
        completion:(void (^)(id<UIViewControllerTransitionCoordinatorContext>))completion {
    return [self animateAlongsideTransition:animation completion:completion];
}
- (void)notifyWhenInteractionEndsUsingBlock:(void (^)(id<UIViewControllerTransitionCoordinatorContext>))handler {}
- (void)notifyWhenInteractionChangesUsingBlock:(void (^)(id<UIViewControllerTransitionCoordinatorContext>))handler {}
- (void)finishCancelled:(BOOL)cancelled {
    self.transitionCancelled=cancelled;
    NSArray *callbacks=self.completions.copy;
    [self.completions removeAllObjects];
    for(void (^completion)(id<UIViewControllerTransitionCoordinatorContext>) in callbacks)completion(self);
}
@end

@interface HGPlaybackTransitionOwner : UIViewController
@property(nonatomic,strong) id<UIViewControllerTransitionCoordinator> fixtureTransition;
@end
@implementation HGPlaybackTransitionOwner
- (id<UIViewControllerTransitionCoordinator>)transitionCoordinator{return self.fixtureTransition;}
@end

static void HGPlaybackCheckCoordinator(UIViewController *host,BOOL accepted,BOOL cancelled,BOOL hostOwnsTransform) {
    HGPlaybackTransitionOwner *owner=[HGPlaybackTransitionOwner new];
    [host addChildViewController:owner];[owner loadViewIfNeeded];
    owner.view.frame=CGRectMake(10,10,600,320);
    [host.view addSubview:owner.view];[owner didMoveToParentViewController:host];
    FQVShortVideoPlayerCellView *canvas=[[FQVShortVideoPlayerCellView alloc] initWithFrame:owner.view.bounds];
    [owner.view addSubview:canvas];
    TTPlayerView *render=[[TTPlayerView alloc] initWithFrame:CGRectMake(60,20,480,280)];
    [canvas addSubview:render];Layout(canvas);Layout(render);
    CATransform3D filled=render.layer.sublayerTransform;
    NSString *(^name)(NSString *)=^NSString *(NSString *check) {
        return [NSString stringWithFormat:@"transition coordinator [queued=%d cancelled=%d hostGeometry=%d]: %@",
            accepted,cancelled,hostOwnsTransform,check];
    };
    HGPlaybackTransitionCoordinator *coordinator=[HGPlaybackTransitionCoordinator new];
    coordinator.acceptsAnimations=accepted;coordinator.containerView=owner.view;
    @try {
        Check(filled.m11>1 && CGAffineTransformIsIdentity(render.transform),name(@"attached landscape renderer starts filled with native view geometry"));
        owner.fixtureTransition=coordinator;
        HGPlaybackResetTransformWrites(render);Layout(render);
        Check(CATransform3DIsIdentity(render.layer.sublayerTransform) &&
            CGAffineTransformIsIdentity(render.transform) && HGPlaybackFillWrites(render)==1 && HGPlaybackTransformWrites(render)==0,
            name(@"active transition releases owned zoom once"));
        HGPlaybackResetTransformWrites(render);
        for(NSUInteger pass=0;pass<8;pass++){Layout(canvas);Layout(render);}
        Check(coordinator.registrations==1 && coordinator.completions.count==1 &&
            HGPlaybackTransformWrites(render)==0 && HGPlaybackFillWrites(render)==0,
            name(@"repeated layouts register only one completion and keep native geometry"));

        CGAffineTransform native=CGAffineTransformMakeTranslation(24,-10);
        if(hostOwnsTransform)render.transform=native;
        HGPlaybackResetTransformWrites(render);
        owner.fixtureTransition=nil;
        [coordinator finishCancelled:cancelled];
        // No explicit setNeedsLayout here: the production completion must
        // schedule the safe follow-up after completion or cancellation.
        [render layoutIfNeeded];
        Check(CGAffineTransformEqualToTransform(render.transform,hostOwnsTransform?native:CGAffineTransformIdentity) &&
            CATransform3DEqualToTransform(render.layer.sublayerTransform,hostOwnsTransform?CATransform3DIdentity:filled) &&
            HGPlaybackTransformWrites(render)==0 && HGPlaybackFillWrites(render)==(hostOwnsTransform?0u:1u),
            name(@"completion schedules one safe update and preserves newer host transforms"));
        HGPlaybackResetTransformWrites(render);
        for(NSUInteger pass=0;pass<4;pass++){Layout(canvas);Layout(render);}
        Check(HGPlaybackTransformWrites(render)==0 && HGPlaybackFillWrites(render)==0 && coordinator.registrations==1,
            name(@"settled navigation does not repeat transform writes or registration"));
    } @finally {
        owner.fixtureTransition=nil;
        [owner willMoveToParentViewController:nil];[owner.view removeFromSuperview];
        [owner removeFromParentViewController];
        objc_setAssociatedObject(render,&HGPlaybackTransformWritesKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
}

static void RunAppearanceTransitionCoordinatorChecks(UIViewController *host) {
    NSDictionary *saved=[HGAppearanceValues() copy];
    UIWindow *window=HGPlaybackFixtureWindow(host.view,CGSizeMake(700,400));
    @try {
        HGAppearanceApply(@{@"enabled":@YES,@"player.fullscreen":@YES},nil);
        for(NSNumber *accepted in @[@YES,@NO])
            for(NSNumber *cancelled in @[@NO,@YES])
                HGPlaybackCheckCoordinator(window.rootViewController,accepted.boolValue,cancelled.boolValue,NO);
        HGPlaybackCheckCoordinator(window.rootViewController,NO,YES,YES);
    } @finally {
        window.hidden=YES;window.rootViewController=nil;
        HGAppearanceApply(saved,nil);
    }
}
