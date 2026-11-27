@class FQVShortVideoProgressView;
@interface FQVShortVideoVPlayerMultiSeriesControlView : UIView
@property(nonatomic,strong) FQVShortVideoProgressView *progressContainer;
@property(nonatomic,strong) FQVShortVideoBaseInsertView *insertAboveProgressView;
@end
@implementation FQVShortVideoVPlayerMultiSeriesControlView
- (void)layoutSubviews {[super layoutSubviews];}
@end

@interface FQVShortVideoDerivativeVideoControlView : FQVShortVideoVPlayerMultiSeriesControlView
@property(nonatomic) BOOL writesNativeFrames;
@property(nonatomic) BOOL observesSuperclassLayout;
@property(nonatomic) CGPoint entryCenterAfterSuperclass;
@property(nonatomic,weak) UIView *nativeEntry;
@property(nonatomic,weak) UIView *nativeInfo;
@property(nonatomic) CGRect nativeEntryFrame;
@property(nonatomic) CGRect nativeInfoFrame;
@end
@implementation FQVShortVideoDerivativeVideoControlView
- (void)layoutSubviews {
    if(self.observesSuperclassLayout) self.nativeEntry.frame=self.nativeEntryFrame;
    [super layoutSubviews];
    if(self.observesSuperclassLayout) self.entryCenterAfterSuperclass=self.nativeEntry.center;
    if(self.writesNativeFrames) {
        self.nativeEntry.frame=self.nativeEntryFrame;
        self.nativeInfo.frame=self.nativeInfoFrame;
    }
}
@end
@interface FQVShortVideoControlView : UIView @end
@implementation FQVShortVideoControlView
- (void)layoutSubviews {[super layoutSubviews];}
@end
@interface FQVShortVideoProgressView : UIControl
@property(nonatomic,strong) UIView *progressBarBackgroundView;
- (void)updateProgressViewWithActivatedState:(NSUInteger)state;
@end
@implementation FQVShortVideoProgressView
- (void)updateProgressViewWithActivatedState:(NSUInteger)state {
    // The native repaint reads the owner's frame width on every activation.
    self.progressBarBackgroundView.frame=CGRectMake(16,state==1 ? 19 : 21,
        CGRectGetWidth(self.frame)-32,state==1 ? 4 : 2);
}
@end
@interface SSShortVideoDerivateInfoView : UIView @end
@implementation SSShortVideoDerivateInfoView @end
// This independent class is the exact HUD identity in the user's R14 scene.
// It must not inherit the legacy test HUD, which concealed the missing adapter.
@interface FQVShortVideoVPlayerOutterMultiSeriesControlView : UIView
@property(nonatomic,strong) FQVShortVideoProgressView *progressContainer;
@property(nonatomic,strong) FQVShortVideoBaseInsertView *insertAboveProgressView;
@end
@implementation FQVShortVideoVPlayerOutterMultiSeriesControlView
- (void)layoutSubviews {[super layoutSubviews];}
@end
@interface SSShortVideoRightInteractionView : UIView @end
@implementation SSShortVideoRightInteractionView @end
@interface HGTestHUDTraversalProbe : UIView
@property(nonatomic) NSUInteger subviewReads;
@end
@implementation HGTestHUDTraversalProbe
- (NSArray<UIView *> *)subviews { self.subviewReads++; return [super subviews]; }
@end
@interface HGTestFontReadLabel : UILabel
@property(nonatomic) NSUInteger fontReads;
@end
@implementation HGTestFontReadLabel
- (UIFont *)font { self.fontReads++; return [super font]; }
@end
@interface SSShortVideoAboveLeftContainerView : UIView @end
@implementation SSShortVideoAboveLeftContainerView @end
@interface SSShortVideoOutterSeriesLeftContainerView : UIView @end
@implementation SSShortVideoOutterSeriesLeftContainerView @end

@interface FQRJXCategoryListContainerView : UIView <UIScrollViewDelegate>
@property(nonatomic) NSUInteger nativeEnds;
@property(nonatomic) NSInteger currentIndex;
@property(nonatomic) NSUInteger nativeSelectionRepairs;
- (void)didClickSelectedItemAtIndex:(NSInteger)index;
@property(nonatomic,strong) UIScrollView *contentScrollView;
@end
@implementation FQRJXCategoryListContainerView
- (void)scrollViewDidEndDecelerating:(UIScrollView *)scroll { self.nativeEnds++; }
- (void)scrollViewDidEndDragging:(UIScrollView *)scroll willDecelerate:(BOOL)decelerate { self.nativeEnds++; }
- (void)listDidAppear:(NSInteger)index { self.currentIndex=index;self.nativeEnds++; }
- (void)didClickSelectedItemAtIndex:(NSInteger)index { self.currentIndex=index;self.nativeSelectionRepairs++; }
@end

static void RunInnerEpisodeSpacingChecks(UIView *root) {
    FQVShortVideoVPlayerMultiSeriesControlView *control=
        [[FQVShortVideoVPlayerMultiSeriesControlView alloc] initWithFrame:CGRectMake(0,0,440,956)];
    CGRect entryFrame=CGRectMake(0,826,440,44), progressFrame=CGRectMake(0,849,440,30);
    CGRect commentFrame=CGRectMake(16,892,378,40);
    FQVShortVideoBaseInsertView *wrapper=[[FQVShortVideoBaseInsertView alloc] initWithFrame:entryFrame];
    SSShortVideoBottomDistributionView *entry=[[SSShortVideoBottomDistributionView alloc] initWithFrame:wrapper.bounds];
    [wrapper addSubview:entry];
    FQVShortVideoProgressView *progress=[[FQVShortVideoProgressView alloc] initWithFrame:progressFrame];
    UIView *track=[[UIView alloc] initWithFrame:CGRectMake(16,21,408,2)];
    track.userInteractionEnabled=NO;
    [progress addSubview:track];
    UIView *comment=[[UIView alloc] initWithFrame:commentFrame];
    [control addSubview:progress];[control addSubview:wrapper];[control addSubview:comment];
    [root addSubview:control];Layout(control);Layout(entry);
    CGRect rail=[track convertRect:track.bounds toView:control];
    Check(fabs(CGRectGetMinY(rail)-CGRectGetMaxY(wrapper.frame)-8)<0.01 &&
        CGRectEqualToRect(progress.frame,progressFrame) && CGRectEqualToRect(comment.frame,commentFrame),
        @"detail playback keeps an eight-point gap above the native rail without moving the comment entry");
    control.progressContainer=progress;control.insertAboveProgressView=wrapper;
    progress.progressBarBackgroundView=track;
    for(NSNumber *state in @[@1,@2,@1,@2,@1,@2]) {
        wrapper.frame=entryFrame;progress.frame=progressFrame;
        [progress updateProgressViewWithActivatedState:state.unsignedIntegerValue];
        CGRect ink=[track.layer convertRect:track.layer.bounds toLayer:control.layer];
        CGRect cap=[entry convertRect:entry.bounds toView:control];
        Check(CGRectEqualToRect(progress.frame,progressFrame) && CGRectGetWidth(track.frame)==408 &&
            fabs(CGRectGetMinX(ink)-CGRectGetMinX(cap))<0.01 && fabs(CGRectGetMaxX(ink)-CGRectGetMaxX(cap))<0.01 &&
            fabs(CGRectGetMinY(ink)-CGRectGetMaxY(wrapper.frame)-8)<0.01 &&
            CGRectEqualToRect(comment.frame,commentFrame) &&
            [control hitTest:CGPointMake(CGRectGetMidX(ink),CGRectGetMidY(ink)) withEvent:nil]==progress,
            @"press and release repaints keep the native 440pt touch strip, both capsule endpoints and the eight-point gap");
    }
    [wrapper addSubview:[UIView new]];
    SSShortVideoBottomDistributionView *inactive=[[SSShortVideoBottomDistributionView alloc] initWithFrame:entry.frame];
    [wrapper addSubview:inactive];inactive.hidden=YES;
    id savedX=HGAppearanceValue(@"layout.progress.x"),savedScale=HGAppearanceValue(@"layout.progress.scale");
    HGAppearanceApply(@{@"layout.progress.x":@18,@"layout.progress.scale":@1.2},nil);
    HGAppearanceProcessView(progress);
    CGRect capsule=[entry convertRect:entry.bounds toView:control];
    CGRect fitted=[track.layer convertRect:track.layer.bounds toLayer:control.layer];
    Check(fabs(CGRectGetMinX(fitted)-CGRectGetMinX(capsule))<0.01 &&
        fabs(CGRectGetMaxX(fitted)-CGRectGetMaxX(capsule))<0.01 &&
        fabs(CGRectGetHeight(fitted)-1.2*CGRectGetHeight(track.bounds))<0.01 &&
        [progress pointInside:[progress convertPoint:CGPointMake(CGRectGetMaxX(fitted)-1,CGRectGetMidY(fitted)) fromView:control] withEvent:nil],
        @"capsule controls both seekable endpoints despite saved X/scale, decorative siblings and an inactive second episode; vertical scale is preserved");
    entry.hidden=YES;HGAppearanceProcessView(progress);
    Check(fabs(progress.transform.a-1.2)<0.001 && fabs(progress.transform.tx-18)<0.001 &&
        CATransform3DIsIdentity(progress.layer.sublayerTransform),
        @"with no visible capsule the saved progress width and horizontal offset take effect again");
    HGAppearanceApply(@{@"layout.progress.x":savedX,@"layout.progress.scale":savedScale},nil);
    HGAppearanceProcessView(progress);
    entry.hidden=NO;
    HGAppearanceProcessView(progress);
    [progress removeFromSuperview];
    Check(CATransform3DIsIdentity(progress.layer.sublayerTransform) && CGRectEqualToRect(progress.frame,progressFrame),
        @"reparenting the native progress view clears the old episode's drawing width before reuse");
    [control addSubview:progress];
    [control removeFromSuperview];
    Check(CGRectEqualToRect(wrapper.frame,entryFrame) && CGRectEqualToRect(progress.frame,progressFrame),
        @"leaving the detail HUD restores its original capsule position");
}

static void RunOutterEpisodeSpacingChecks(UIView *root) {
    NSDictionary *prior=HGAppearanceValues();
    NSMutableDictionary<NSValue *,NSNumber *> *barVisibility=[NSMutableDictionary dictionary];
    for(UIView *child in root.subviews)if([child isKindOfClass:SSTabBar.class]) {
        barVisibility[[NSValue valueWithNonretainedObject:child]]=@(child.hidden);child.hidden=YES;
    }
    HGAppearanceSet(@"dock.enabled",@NO,nil);
    UIView *screen=[[UIView alloc] initWithFrame:CGRectMake(0,0,440,956)];
    FQVShortVideoPlayerCellView *canvas=[[FQVShortVideoPlayerCellView alloc] initWithFrame:CGRectMake(0,0,440,866)];
    TTPlayerView *render=[[TTPlayerView alloc] initWithFrame:canvas.bounds];
    FQVShortVideoVPlayerOutterMultiSeriesControlView *control=
        [[FQVShortVideoVPlayerOutterMultiSeriesControlView alloc] initWithFrame:canvas.bounds];
    SSShortVideoOutterSeriesInfoView *info=[[SSShortVideoOutterSeriesInfoView alloc]
        initWithFrame:CGRectMake(0,0,440,822)];
    SSShortVideoAboveLeftContainerView *above=[[SSShortVideoAboveLeftContainerView alloc]
        initWithFrame:CGRectMake(16,658,312,32)];
    SSShortVideoRightInteractionView *right=[[SSShortVideoRightInteractionView alloc]
        initWithFrame:CGRectMake(382,522,46,284)];
    SSShortVideoOutterSeriesLeftContainerView *left=[[SSShortVideoOutterSeriesLeftContainerView alloc]
        initWithFrame:CGRectMake(16,702,312,80)];
    FQVShortVideoBaseInsertView *notice=[[FQVShortVideoBaseInsertView alloc]
        initWithFrame:CGRectMake(16,782,312,24)];
    [notice addSubview:[[UILabel alloc] initWithFrame:notice.bounds]];
    UIView *paymentOverlay=[[UIView alloc] initWithFrame:canvas.bounds];
    paymentOverlay.userInteractionEnabled=NO;
    [info addSubview:paymentOverlay];[info addSubview:above];[info addSubview:right];
    [info addSubview:left];[info addSubview:notice];
    FQVShortVideoBaseInsertView *wrapper=[[FQVShortVideoBaseInsertView alloc]
        initWithFrame:CGRectMake(0,822,440,44)];
    SSShortVideoBottomDistributionView *entry=[[SSShortVideoBottomDistributionView alloc] initWithFrame:wrapper.bounds];
    UIControl *open=[[UIControl alloc] initWithFrame:entry.bounds];
    HGAEpisodeTapFixture *action=[HGAEpisodeTapFixture new];
    [open addTarget:action action:@selector(open:) forControlEvents:UIControlEventTouchUpInside];
    [entry addSubview:open];[wrapper addSubview:entry];
    FQVShortVideoProgressView *progress=[[FQVShortVideoProgressView alloc] initWithFrame:CGRectMake(0,841,440,30)];
    UIView *track=[[UIView alloc] initWithFrame:CGRectMake(16,22,408,2)];
    track.userInteractionEnabled=NO;[progress addSubview:track];
    // Match the real scene's sibling order: the native episode wrapper is
    // above the progress owner's tall invisible touch strip. Keep that native
    // order so its complete visible capsule remains tappable after spacing.
    [control addSubview:progress];[control addSubview:info];[control addSubview:wrapper];
    [canvas addSubview:render];[canvas addSubview:control];[screen addSubview:canvas];
    CGRect nativeInfo=info.frame,nativeProgress=progress.frame,nativeTrack=track.frame,
        nativeCanvas=canvas.frame,nativeRender=render.frame,nativeRight=right.frame,
        nativeAbove=above.frame,nativeLeft=left.frame,nativeNotice=notice.frame;
    [root addSubview:screen];Layout(control);Layout(entry);
    @try {
        HGTestHUDTraversalProbe *probe=[HGTestHUDTraversalProbe new];
        [control addSubview:probe];
        UIView *decoration=[UIView new];
        [right addSubview:decoration];
        probe.subviewReads=0;
        HGAppearanceProcessView(decoration);
        Check(probe.subviewReads==0,@"laying out an unrelated action child does not traverse the playback HUD");
        HGAppearanceProcessView(progress);
        Check(probe.subviewReads>0,@"a HUD without native slots finds its progress rail in the fallback hierarchy");
        control.progressContainer=progress;
        control.insertAboveProgressView=wrapper;
        probe.subviewReads=0;
        track.center=CGPointMake(track.center.x,track.center.y-2);
        for(NSUInteger i=0;i<48;i++)HGAppearanceProcessView(progress);
        CGRect fittedRail=[track convertRect:track.bounds toView:control];
        Check(probe.subviewReads==0 && fabs(CGRectGetMinY(fittedRail)-CGRectGetMaxY(wrapper.frame)-8)<0.01,
            @"repeated progress layouts use mounted native slots and still apply a changed rail position");
        track.center=CGPointMake(track.center.x,track.center.y+2);
        HGAppearanceProcessView(progress);
        control.progressContainer=nil;
        control.insertAboveProgressView=nil;
        [probe removeFromSuperview];
        [decoration removeFromSuperview];
        CGRect rail=[track convertRect:track.bounds toView:control];
        Check(fabs(CGRectGetMinY(rail)-CGRectGetMaxY(wrapper.frame)-8)<0.01 &&
            fabs(wrapper.frame.origin.y-811)<0.01,
            @"the real R14 multi-series HUD receives its missing eleven-point episode correction");
        Check(CGRectEqualToRect(info.frame,nativeInfo) && CGRectEqualToRect(paymentOverlay.frame,nativeCanvas) &&
            fabs(right.frame.origin.y-(nativeRight.origin.y-11))<0.01 &&
            fabs(left.frame.origin.y-(nativeLeft.origin.y-11))<0.01 &&
            fabs(above.frame.origin.y-(nativeAbove.origin.y-11))<0.01 &&
            fabs(notice.frame.origin.y-(nativeNotice.origin.y-11))<0.01,
            @"only known bottom metadata and right actions move while the full information and payment HUD stays native");
        SSTabBar *bar=[[SSTabBar alloc] initWithFrame:CGRectMake(0,872,440,84)];
        NSArray *titles=@[@"首页",@"剧场",@"商城",@"我的"];
        for(NSUInteger i=0;i<titles.count;i++) {
            UIControl *button=[[UIControl alloc] initWithFrame:CGRectMake(i*110,0,110,62)];
            UILabel *label=[[UILabel alloc] initWithFrame:button.bounds];label.text=titles[i];
            [button addSubview:label];
            [button addTarget:action action:@selector(open:) forControlEvents:UIControlEventTouchUpInside];
            [bar.contentView addSubview:button];
        }
        [screen addSubview:bar];
        HGAppearanceApply(@{@"dock.enabled":@YES,@"dock.extra":@YES,@"dock.height":@70,
            @"dock.extraSize":@70,@"dock.lift":@0,@"dock.width":@0.94,
            @"dock.hide.home":@NO,@"dock.hide.theater":@NO,@"dock.hide.store":@NO,@"dock.hide.mine":@NO},nil);
        HGAppearanceLayoutDock(bar,nil);Layout(control);
        UIView *capsule=Find(bar,@"honggo.appearance.dock.capsule");
        CGRect dock=[capsule convertRect:capsule.bounds toView:control];
        rail=[track convertRect:track.bounds toView:control];
        Check(capsule && fabs(capsule.bounds.size.height-70)<0.01 &&
            fabs(CGRectGetMinY(dock)-CGRectGetMaxY(rail)-10)<0.01 &&
            fabs(CGRectGetMinY(rail)-CGRectGetMaxY(wrapper.frame)-8)<0.01,
            @"the larger floating dock leaves ten points below the rail and eight above it");
        Check(CGSizeEqualToSize(progress.bounds.size,nativeProgress.size) &&
            CGRectEqualToRect(track.frame,nativeTrack) && CGRectEqualToRect(canvas.frame,nativeCanvas) &&
            CGRectEqualToRect(render.frame,nativeRender) && CGRectEqualToRect(info.frame,nativeInfo),
            @"dock clearance preserves scrubbing size, track drawing, video geometry and full HUD geometry");
        CGPoint episodePoint=[open convertPoint:CGPointMake(220,22) toView:control];
        CGPoint episodeBottomPoint=[open convertPoint:CGPointMake(220,43) toView:control];
        CGPoint railPoint=CGPointMake(CGRectGetMidX(rail),CGRectGetMidY(rail));
        Check([control hitTest:episodePoint withEvent:nil]==open &&
            [control hitTest:episodeBottomPoint withEvent:nil]==open &&
            [control hitTest:railPoint withEvent:nil]==progress,
            @"the whole visible episode capsule including its lower edge wins native taps while the rail still scrubs");
        [open sendActionsForControlEvents:UIControlEventTouchUpInside];
        Check(action.taps==1,@"the real-HUD episode entry retains its single native action");
        CGPoint beforeProgress=progress.center,beforeEntry=wrapper.center;
        bar.frame=CGRectOffset(bar.frame,0,-8);HGAppearanceLayoutDock(bar,nil);Layout(control);
        Check(fabs(progress.center.y-beforeProgress.y+8)<0.01 &&
            fabs(wrapper.center.y-beforeEntry.y+8)<0.01,
            @"moving the dock up moves the rail, entry and metadata as one bounded clearance adjustment");
        CGPoint adjustedProgress=progress.center,adjustedEntry=wrapper.center,adjustedRight=right.center;
        // The host starts its horizontal drawer animation using the original
        // HUD geometry. A last-minute native change must survive the handoff.
        CGRect hostRight = CGRectOffset(nativeRight, 3, 0);
        right.frame = hostRight;
        HGAppearanceUpdateSidebarLayout(screen.window, YES);
        Check(CGRectEqualToRect(progress.frame, nativeProgress) && fabs(wrapper.frame.origin.y-822)<0.01 &&
            CGRectEqualToRect(right.frame, hostRight) && CGRectEqualToRect(above.frame, nativeAbove),
            @"sidebar handoff restores owned HUD centers and preserves the host's last native write");
        right.frame = nativeRight;
        HGAppearanceUpdateSidebarLayout(screen.window, NO);
        HGAppearanceLayoutDock(bar,nil);
        for(NSUInteger i=0;i<8;i++){Layout(control);Layout(info);Layout(entry);Layout(progress);}
        Check(CGPointEqualToPoint(progress.center,adjustedProgress) &&
            CGPointEqualToPoint(wrapper.center,adjustedEntry) && CGPointEqualToPoint(right.center,adjustedRight),
            @"real-HUD and child layouts do not compound dock and episode offsets");
        right.frame=CGRectOffset(nativeRight,0,-4);Layout(info);Layout(control);Layout(info);
        Check(fabs(right.center.y-adjustedRight.y+4)<0.01,
            @"a later absolute native action-group layout receives one fresh offset without retaining the old offset");
        right.frame=nativeRight;Layout(info);
        for(NSNumber *offset in @[@24,@-24,@400,@-400]) {
            canvas.frame=CGRectOffset(nativeCanvas,0,offset.doubleValue);Layout(control);
            Check(CGRectEqualToRect(progress.frame,nativeProgress) && fabs(wrapper.frame.origin.y-811)<0.01,
                @"an entering or leaving native paging cell retains only its local episode gap without dock anchoring");
        }
        canvas.frame=nativeCanvas;Layout(control);
        Check(CGPointEqualToPoint(progress.center,adjustedProgress) &&
            CGPointEqualToPoint(wrapper.center,adjustedEntry) && CGPointEqualToPoint(right.center,adjustedRight),
            @"settling the native paging cell restores one dock clearance adjustment");
        bar.hidden=YES;Layout(control);
        Check(CGRectEqualToRect(progress.frame,nativeProgress) && fabs(wrapper.frame.origin.y-811)<0.01 &&
            fabs(right.frame.origin.y-(nativeRight.origin.y-11))<0.01,
            @"hiding the dock releases its clearance while preserving the small episode-to-rail gap");
        bar.hidden=NO;HGAppearanceLayoutDock(bar,nil);Layout(control);
        // Recreate the host's horizontal channel movement and a neighbouring
        // cell one page away while its HUD is still attached to the window.
        FQRJXCategoryListContainerView *pages = [[FQRJXCategoryListContainerView alloc]
            initWithFrame:[screen convertRect:screen.window.bounds fromView:screen.window]];
        [screen insertSubview:pages atIndex:0];
        UIScrollView *horizontal = [[UIScrollView alloc] initWithFrame:pages.bounds];
        horizontal.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentNever;
        horizontal.contentSize = CGSizeMake(2 * pages.bounds.size.width, pages.bounds.size.height);
        horizontal.delegate = pages;
        [pages addSubview:horizontal];
        FQVCollectionView *feed = [[FQVCollectionView alloc] initWithFrame:pages.bounds
            collectionViewLayout:[UICollectionViewFlowLayout new]];
        feed.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentNever;
        [horizontal addSubview:feed];
        [feed addSubview:canvas];
        Layout(control);
        BOOL adjustedHere = wrapper.frame.origin.y < 822;
        canvas.frame=CGRectOffset(nativeCanvas,0,-24);Layout(control);
        Check(CGRectEqualToRect(progress.frame,nativeProgress) && fabs(wrapper.frame.origin.y-811)<0.01,
            @"a partially visible vertical page keeps local spacing without borrowing dock clearance");
        canvas.frame=nativeCanvas;Layout(control);
        CGPoint settledEntry=wrapper.center,settledProgress=progress.center,settledRight=right.center;
        UIView *liveDock=Find(bar,@"honggo.appearance.dock");
        [NSNotificationCenter.defaultCenter postNotificationName:UIApplicationWillResignActiveNotification object:nil];
        liveDock.alpha=0;
        Layout(control);
        Check(CGPointEqualToPoint(wrapper.center,settledEntry) && CGPointEqualToPoint(right.center,settledRight) &&
            CGPointEqualToPoint(progress.center,settledProgress),
            @"an app-switch layout without its Dock holds the same episode, right-rail and progress positions");
        [NSNotificationCenter.defaultCenter postNotificationName:UIApplicationDidBecomeActiveNotification object:nil];
        CFTimeInterval until=CACurrentMediaTime()+0.45;
        while(CGPointEqualToPoint(wrapper.center,settledEntry) && CACurrentMediaTime()<until)
            CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.02,false);
        Check(wrapper.center.y>settledEntry.y && CGPointEqualToPoint(progress.center,CGPointMake(CGRectGetMidX(nativeProgress),CGRectGetMidY(nativeProgress))),
            @"a genuinely absent Dock relinquishes its temporary clearance after the active layout commit");
        liveDock.alpha=1;Layout(control);
        Check(CGPointEqualToPoint(wrapper.center,settledEntry) && CGPointEqualToPoint(right.center,settledRight) &&
            CGPointEqualToPoint(progress.center,settledProgress),
            @"returning to a measured Dock recovers the original coordinates without adding the lift twice");
        FQVShortVideoPlayerCellView *incomingCanvas=[[FQVShortVideoPlayerCellView alloc] initWithFrame:CGRectMake(0,0,440,866)];
        FQVShortVideoVPlayerOutterMultiSeriesControlView *incomingControl=
            [[FQVShortVideoVPlayerOutterMultiSeriesControlView alloc] initWithFrame:incomingCanvas.bounds];
        FQVShortVideoBaseInsertView *incomingWrapper=[[FQVShortVideoBaseInsertView alloc] initWithFrame:CGRectMake(0,822,440,44)];
        SSShortVideoBottomDistributionView *incomingEntry=[[SSShortVideoBottomDistributionView alloc] initWithFrame:incomingWrapper.bounds];
        [incomingWrapper addSubview:incomingEntry];
        FQVShortVideoProgressView *incomingProgress=[[FQVShortVideoProgressView alloc] initWithFrame:CGRectMake(0,841,440,30)];
        UIView *incomingTrack=[[UIView alloc] initWithFrame:CGRectMake(16,22,408,2)];
        incomingTrack.userInteractionEnabled=NO;[incomingProgress addSubview:incomingTrack];
        [incomingControl addSubview:incomingProgress];[incomingControl addSubview:incomingWrapper];
        [incomingCanvas addSubview:incomingControl];
        FQVCollectionView *incomingFeed=[[FQVCollectionView alloc] initWithFrame:CGRectOffset(pages.bounds,pages.bounds.size.width,0)
            collectionViewLayout:[UICollectionViewFlowLayout new]];
        incomingFeed.contentInsetAdjustmentBehavior=UIScrollViewContentInsetAdjustmentNever;
        [horizontal addSubview:incomingFeed];
        [incomingFeed addSubview:incomingCanvas];
        horizontal.contentOffset = CGPointMake(70, 0);
        Layout(control);Layout(incomingControl);Layout(incomingEntry);
        CGRect swipeRail=[track convertRect:track.bounds toView:control];
        CGRect incomingRail=[incomingTrack convertRect:incomingTrack.bounds toView:incomingControl];
        Check(adjustedHere && CGPointEqualToPoint(wrapper.center,settledEntry) &&
            CGPointEqualToPoint(progress.center,settledProgress) && CGPointEqualToPoint(right.center,settledRight) &&
            CGRectGetMinY(swipeRail)-CGRectGetMaxY(wrapper.frame)>=8 &&
            fabs(incomingWrapper.center.y-settledEntry.y)<0.01 &&
            fabs(incomingProgress.center.y-settledProgress.y)<0.01 &&
            CGRectGetMinY(incomingRail)-CGRectGetMaxY(incomingWrapper.frame)>=8 &&
            Find(incomingEntry,@"honggo.appearance.material")!=nil,
            @"both outgoing and incoming horizontal channels retain episode glass and Dock clearance during a sideways swipe");
        [incomingFeed removeFromSuperview];
        horizontal.contentOffset = CGPointZero;
        canvas.transform=CGAffineTransformMakeTranslation(-70,0);
        Layout(control);
        Check(CGPointEqualToPoint(wrapper.center,settledEntry) && CGPointEqualToPoint(progress.center,settledProgress),
            @"a translated outgoing player keeps the rail and episode separated during an interactive push");
        canvas.transform=CGAffineTransformIdentity;
        canvas.frame = CGRectOffset(nativeCanvas,0,-nativeCanvas.size.height);
        Layout(control);
        Check(fabs(wrapper.frame.origin.y-822)<0.01 && CGRectEqualToRect(right.frame,nativeRight),
            @"the neighbouring native feed page retains its own entry and action positions");
        canvas.frame = nativeCanvas;
        [horizontal.delegate scrollViewDidEndDecelerating:horizontal];
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.04, false);
        [control layoutIfNeeded];
        Check(pages.nativeEnds == 1 && wrapper.frame.origin.y < 822,
            @"outer pager settlement resumes the current HUD without another inner feed scroll");
        [screen insertSubview:canvas atIndex:0]; [pages removeFromSuperview]; Layout(control);
        HGAppearanceSet(@"player.separateProgress",@"hide",nil);
        Check(CGRectEqualToRect(progress.frame,nativeProgress) && fabs(wrapper.frame.origin.y-822)<0.01 &&
            CGRectEqualToRect(right.frame,nativeRight) && CGRectEqualToRect(above.frame,nativeAbove) &&
            CGRectEqualToRect(left.frame,nativeLeft) && CGRectEqualToRect(notice.frame,nativeNotice),
            @"disabling automatic spacing restores every native HUD position including right actions");
        HGAppearanceSet(@"player.separateProgress",@"show",nil);Layout(control);
        [control removeFromSuperview];
        Check(CGRectEqualToRect(progress.frame,nativeProgress) && fabs(wrapper.frame.origin.y-822)<0.01 &&
            CGRectEqualToRect(right.frame,nativeRight),
            @"transferring the actual multi-series HUD restores its positions before native reparenting");
    } @finally {
        [screen removeFromSuperview];HGAppearanceApply(prior,nil);
        for(NSValue *key in barVisibility) {
            UIView *bar=key.nonretainedObjectValue;bar.hidden=[barVisibility[key] boolValue];
        }
    }
}

static void RunEpisodeSpacingChecks(UIView *root) {
    BOOL priorDock=HGAppearanceFlag(@"dock.enabled");
    HGAppearanceApply(@{@"enabled":@YES,@"player.separateProgress":@"show",
        @"dock.enabled":@NO,
        @"player.episodeGlass":@YES,@"layout.episodes.hidden":@NO,
        @"layout.progress.hidden":@NO,@"layout.info.hidden":@NO,@"layout.episodes.x":@0,@"layout.episodes.y":@0,
        @"layout.episodes.scale":@1,@"layout.progress.x":@0,@"layout.progress.y":@0,
        @"layout.progress.scale":@1,@"layout.info.x":@0,@"layout.info.y":@0,
        @"layout.info.scale":@1},nil);
    FQVShortVideoDerivativeVideoControlView *control=[[FQVShortVideoDerivativeVideoControlView alloc]
        initWithFrame:CGRectMake(0,0,440,866)];
    FQVShortVideoPlayerCellView *canvas=[[FQVShortVideoPlayerCellView alloc] initWithFrame:control.frame];
    TTPlayerView *render=[[TTPlayerView alloc] initWithFrame:CGRectMake(0,310,440,247)];
    [canvas addSubview:render];[canvas addSubview:control];
    SSShortVideoDerivateInfoView *info=[[SSShortVideoDerivateInfoView alloc] initWithFrame:CGRectMake(0,0,440,822)];
    FQVShortVideoBaseInsertView *wrapper=[[FQVShortVideoBaseInsertView alloc] initWithFrame:CGRectMake(0,822,440,44)];
    SSShortVideoBottomDistributionView *entry=[[SSShortVideoBottomDistributionView alloc] initWithFrame:wrapper.bounds];
    UIControl *open=[[UIControl alloc] initWithFrame:entry.bounds];
    HGAEpisodeTapFixture *action=[HGAEpisodeTapFixture new];
    [open addTarget:action action:@selector(open:) forControlEvents:UIControlEventTouchUpInside];
    [entry addSubview:open];[wrapper addSubview:entry];
    FQVShortVideoProgressView *progress=[[FQVShortVideoProgressView alloc] initWithFrame:CGRectMake(0,841,440,30)];
    UIView *track=[[UIView alloc] initWithFrame:CGRectMake(16,20,408,4)];
    track.userInteractionEnabled=NO;[progress addSubview:track];
    [control addSubview:info];[control addSubview:wrapper];[control addSubview:progress];
    control.nativeEntry=wrapper;control.nativeInfo=info;
    control.nativeEntryFrame=wrapper.frame;control.nativeInfoFrame=info.frame;
    CGRect nativeProgress=progress.frame,nativeEntry=entry.frame,nativeTrack=track.frame;
    CGPoint nativeWrapperCenter=wrapper.center,nativeInfoCenter=info.center;
    // The user's 440×956 screenshot has a 440×866 playback canvas above the dock.
    UIView *screen=[[UIView alloc] initWithFrame:CGRectMake(0,0,440,956)];
    [screen addSubview:canvas];[root addSubview:screen];Layout(info);Layout(control);Layout(entry);
    @try {
        control.observesSuperclassLayout=YES;
        [control layoutSubviews];
        control.observesSuperclassLayout=NO;
        Check(CGPointEqualToPoint(control.entryCenterAfterSuperclass,nativeWrapperCenter) &&
            fabs(wrapper.center.y-(nativeWrapperCenter.y-13))<0.01,
            @"the hooked superclass leaves native spacing in place until the enclosing layout returns");
        CGRect rail=[track convertRect:track.bounds toView:control];
        Check(fabs(CGRectGetMinY(rail)-CGRectGetMaxY(wrapper.frame)-8)<0.01 &&
            fabs(CGRectGetMinY(wrapper.frame)-809)<0.01,
            @"screenshot-size episode capsule lifts only thirteen points to leave eight points above the visible rail");
        Check(fabs(info.center.y-(nativeInfoCenter.y-13))<0.01 &&
            fabs(CGRectGetMaxY(info.frame)-CGRectGetMinY(wrapper.frame))<0.01,
            @"episode spacing moves adjacent metadata and right actions together without a new overlap");
        Check(CGRectEqualToRect(progress.frame,nativeProgress) && CGRectEqualToRect(track.frame,nativeTrack) &&
            fabs(entry.frame.origin.y-nativeEntry.origin.y)<0.01 &&
            fabs(entry.frame.size.height-nativeEntry.size.height)<0.01 &&
            fabs(CGRectGetMidX(entry.frame)-CGRectGetMidX(nativeEntry))<0.01 &&
            CGAffineTransformIsIdentity(wrapper.transform),
            @"episode spacing preserves native slider geometry, child frames and wrapper transform");
        Check([control hitTest:CGPointMake(220,831) withEvent:nil]==open &&
            [control hitTest:CGPointMake(220,863) withEvent:nil]==progress,
            @"episode-center taps and visible-rail scrubbing still reach their native controls");
        [open sendActionsForControlEvents:UIControlEventTouchUpInside];
        Check(action.taps==1,@"spaced episode entry still calls its native action exactly once");
        Check(wrapper.clipsToBounds && fabs(wrapper.layer.cornerRadius-22)<0.01,
            @"episode spacing retains the rounded native wrapper and removes square corners");
        CGPoint spacedWrapper=wrapper.center,spacedInfo=info.center;
        for(NSUInteger i=0;i<12;i++){Layout(control);Layout(entry);Layout(progress);}
        Check(CGPointEqualToPoint(wrapper.center,spacedWrapper) && CGPointEqualToPoint(info.center,spacedInfo),
            @"repeated bottom layout does not accumulate offsets or fight the progress view");
        control.writesNativeFrames=YES;Layout(control);Layout(entry);
        Check(CGPointEqualToPoint(wrapper.center,spacedWrapper) && CGPointEqualToPoint(info.center,spacedInfo),
            @"native frame-driven relayout reapplies one spacing offset from its fresh baseline");
        control.writesNativeFrames=NO;

        track.frame=CGRectOffset(nativeTrack,0,2);Layout(progress);
        Check(fabs(wrapper.center.y-(nativeWrapperCenter.y-11))<0.01 &&
            CGRectEqualToRect(progress.frame,nativeProgress),
            @"a native rail-position change recomputes one small lift without moving the scrub container");
        track.frame=nativeTrack;Layout(progress);
        track.hidden=YES;Layout(progress);
        Check(CGPointEqualToPoint(wrapper.center,nativeWrapperCenter) && CGPointEqualToPoint(info.center,nativeInfoCenter),
            @"a hidden native rail releases visual spacing instead of reserving its invisible touch strip");
        track.hidden=NO;Layout(progress);

        // A full-canvas info HUD was previously rejected together with its entry.
        info.frame=CGRectMake(0,0,440,866);Layout(control);
        Check(CGPointEqualToPoint(wrapper.center,spacedWrapper) && CGRectGetMinY(info.frame)==0 &&
            CGRectGetHeight(info.frame)==866,
            @"an overlay HUD reaching below the entry stays native while the episode still receives its small gap");
        info.frame=control.nativeInfoFrame;Layout(control);
        Check(CGPointEqualToPoint(info.center,spacedInfo),
            @"an adjacent metadata group resumes the same small shift after a native layout change");

        wrapper.frame=CGRectOffset(control.nativeEntryFrame,0,7);Layout(control);
        Check(fabs(wrapper.center.y-(nativeWrapperCenter.y+7-16))<0.01 &&
            CGRectEqualToRect(progress.frame,nativeProgress),
            @"an unusually low episode is capped at sixteen points of lift instead of climbing far up the page");
        wrapper.frame=control.nativeEntryFrame;Layout(control);

        [track removeFromSuperview];
        CALayer *nativeRail=[CALayer layer];nativeRail.frame=nativeTrack;
        nativeRail.backgroundColor=UIColor.whiteColor.CGColor;[progress.layer addSublayer:nativeRail];Layout(progress);
        Check(CGPointEqualToPoint(wrapper.center,spacedWrapper) && CGRectEqualToRect(nativeRail.frame,nativeTrack),
            @"a layer-backed native rail receives the same spacing without changing its layer geometry");
        [nativeRail removeFromSuperlayer];Layout(progress);
        Check(fabs(wrapper.center.y-(nativeWrapperCenter.y-10))<0.01 &&
            CGRectEqualToRect(progress.frame,nativeProgress),
            @"a directly drawn rail in the verified compact progress owner receives only a ten-point fallback nudge");
        [progress addSubview:track];Layout(progress);
        UIView *secondRail=[[UIView alloc] initWithFrame:CGRectMake(16,5,408,4)];
        [progress addSubview:secondRail];Layout(progress);
        Check(CGPointEqualToPoint(wrapper.center,nativeWrapperCenter),
            @"ambiguous visible rails keep the native layout instead of activating the fallback");
        [secondRail removeFromSuperview];Layout(progress);
        HGAppearanceSet(@"player.separateProgress",@"hide",nil);
        Check(CGPointEqualToPoint(wrapper.center,nativeWrapperCenter) && CGPointEqualToPoint(info.center,nativeInfoCenter),
            @"disabling automatic spacing restores both original centers");

        HGAppearanceSet(@"player.separateProgress",@"show",nil);Layout(control);
        wrapper.center=CGPointMake(nativeWrapperCenter.x,nativeWrapperCenter.y+4);
        HGAppearanceSet(@"player.separateProgress",@"hide",nil);
        Check(fabs(wrapper.center.y-nativeWrapperCenter.y-4)<0.01,
            @"spacing cleanup preserves a later native center update");
        wrapper.center=nativeWrapperCenter;
        HGAppearanceSet(@"player.separateProgress",@"show",nil);Layout(control);
        HGAppearanceSet(@"layout.episodes.y",@-12,nil);
        Check(CGPointEqualToPoint(wrapper.center,nativeWrapperCenter) &&
            fabs(entry.transform.ty+12)<0.01,
            @"explicit user geometry takes precedence over automatic episode spacing");
        HGAppearanceSet(@"layout.episodes.y",@0,nil);Layout(control);
        entry.hidden=YES;Layout(control);
        Check(CGPointEqualToPoint(wrapper.center,nativeWrapperCenter) && CGPointEqualToPoint(info.center,nativeInfoCenter),
            @"hidden episode entries release reserved spacing immediately");
        entry.hidden=NO;Layout(control);

        UIView *destination=[[UIView alloc] initWithFrame:control.bounds];[root addSubview:destination];
        [destination addSubview:wrapper];
        Check(CGPointEqualToPoint(wrapper.center,nativeWrapperCenter) && CGPointEqualToPoint(info.center,nativeInfoCenter),
            @"episode wrapper reparenting restores spacing before entering another native page");
        [control addSubview:wrapper];[control bringSubviewToFront:progress];[destination removeFromSuperview];Layout(control);
        [progress removeFromSuperview];
        Check(CGPointEqualToPoint(wrapper.center,nativeWrapperCenter) && CGPointEqualToPoint(info.center,nativeInfoCenter),
            @"removing only the progress owner immediately releases the old spacing reservation");
        [control addSubview:progress];Layout(control);
        UIView *unrelated=[UIView new];[wrapper addSubview:unrelated];Layout(control);
        Check(CGPointEqualToPoint(wrapper.center,nativeWrapperCenter),
            @"an insert wrapper with unrelated native content keeps its original position");
        [unrelated removeFromSuperview];Layout(control);
        HGAppearanceSet(@"enabled",@NO,nil);
        Check(CGPointEqualToPoint(wrapper.center,nativeWrapperCenter) && CGPointEqualToPoint(info.center,nativeInfoCenter) &&
            CGRectEqualToRect(progress.frame,nativeProgress),
            @"disabling appearance returns the complete episode and progress layout to the host");

        HGAppearanceSet(@"enabled",@YES,nil);
        FQVShortVideoControlView *standard=[[FQVShortVideoControlView alloc] initWithFrame:control.bounds];
        UIView *intermediate=[[UIView alloc] initWithFrame:standard.bounds];
        [standard addSubview:intermediate];[canvas addSubview:standard];
        [intermediate addSubview:wrapper];[intermediate addSubview:progress];Layout(standard);Layout(entry);
        Check(fabs(wrapper.center.y-(nativeWrapperCenter.y-13))<0.01 &&
            CGRectEqualToRect(progress.frame,nativeProgress),
            @"the standard native HUD also spaces recognized episode and progress components through an intermediate container");
        progress.hidden=YES;Layout(standard);
        Check(CGPointEqualToPoint(wrapper.center,nativeWrapperCenter),
            @"hiding the standard HUD progress owner restores the original episode position");
        progress.hidden=NO;Layout(standard);
        [standard removeFromSuperview];Layout(entry);
        Check(CGPointEqualToPoint(wrapper.center,nativeWrapperCenter),
            @"detaching a nested native HUD clears the owned episode lift");
        HGAppearanceSet(@"layout.progress.hidden",@YES,nil);
        HGAppearanceProcessView(progress);
        Check(progress.hidden,@"fast-pathed progress view still honors explicit progress hiding");
        HGAppearanceSet(@"layout.progress.hidden",@NO,nil);
        HGAppearanceProcessView(progress);
        Check(!progress.hidden,@"fast-pathed progress view restores visibility immediately");
        RunInnerEpisodeSpacingChecks(root);
        RunOutterEpisodeSpacingChecks(root);
    } @finally {
        [screen removeFromSuperview];
        HGAppearanceApply(@{@"enabled":@YES,@"player.separateProgress":@"show",@"layout.episodes.y":@0,
            @"dock.enabled":@(priorDock)},nil);
    }
}
