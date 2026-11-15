@interface FQVShortVideoDerivativeVideoControlView : UIView
@property(nonatomic) BOOL writesNativeFrames;
@property(nonatomic,weak) UIView *nativeEntry;
@property(nonatomic,weak) UIView *nativeInfo;
@property(nonatomic) CGRect nativeEntryFrame;
@property(nonatomic) CGRect nativeInfoFrame;
@end
@implementation FQVShortVideoDerivativeVideoControlView
- (void)layoutSubviews {
    [super layoutSubviews];
    if(self.writesNativeFrames) {
        self.nativeEntry.frame=self.nativeEntryFrame;
        self.nativeInfo.frame=self.nativeInfoFrame;
    }
}
@end
@interface FQVShortVideoProgressView : UIControl @end
@implementation FQVShortVideoProgressView @end
@interface SSShortVideoDerivateInfoView : UIView @end
@implementation SSShortVideoDerivateInfoView @end

static void RunEpisodeSpacingChecks(UIView *root) {
    HGAppearanceApply(@{@"enabled":@YES,@"player.separateProgress":@"show",
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
    [root addSubview:canvas];Layout(info);
    UIButton *fillButton=(id)Find(info,@"honggo.appearance.player.fill");
    CGRect firstFillRect=[fillButton convertRect:fillButton.bounds toView:canvas];
    Layout(control);Layout(entry);
    @try {
        Check(fillButton && !fillButton.hidden && fabs(firstFillRect.origin.y-569)<0.01 &&
            CGRectEqualToRect(firstFillRect,[fillButton convertRect:fillButton.bounds toView:canvas]),
            @"fill button keeps its canvas-relative position on the first combined HUD and episode spacing layout");
        Check(fabs(CGRectGetMinY(progress.frame)-CGRectGetMaxY(wrapper.frame)-8)<0.01 &&
            fabs(CGRectGetMinY(wrapper.frame)-789)<0.01,
            @"episode capsule reserves eight points above the complete native progress touch strip");
        Check(fabs(info.center.y-(nativeInfoCenter.y-33))<0.01 &&
            fabs(CGRectGetMaxY(info.frame)-CGRectGetMinY(wrapper.frame))<0.01,
            @"episode spacing moves adjacent metadata and right actions together without a new overlap");
        Check(CGRectEqualToRect(progress.frame,nativeProgress) && CGRectEqualToRect(track.frame,nativeTrack) &&
            CGRectEqualToRect(entry.frame,nativeEntry) && CGAffineTransformIsIdentity(wrapper.transform),
            @"episode spacing preserves native slider geometry, child frames and wrapper transform");
        Check([control hitTest:CGPointMake(220,811) withEvent:nil]==open &&
            [control hitTest:CGPointMake(220,856) withEvent:nil]==progress,
            @"episode and scrubbing hit targets remain separate after automatic spacing");
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
    } @finally {
        [canvas removeFromSuperview];
        HGAppearanceApply(@{@"enabled":@YES,@"player.separateProgress":@"show",@"layout.episodes.y":@0},nil);
    }
}
