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
@interface FQVShortVideoControlView : UIView @end
@implementation FQVShortVideoControlView
- (void)layoutSubviews {[super layoutSubviews];}
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
    // The user's 440×956 screenshot has a 440×866 playback canvas above the dock.
    UIView *screen=[[UIView alloc] initWithFrame:CGRectMake(0,0,440,956)];
    [screen addSubview:canvas];[root addSubview:screen];Layout(info);Layout(control);Layout(entry);
    @try {
        CGRect rail=[track convertRect:track.bounds toView:control];
        Check(fabs(CGRectGetMinY(rail)-CGRectGetMaxY(wrapper.frame)-8)<0.01 &&
            fabs(CGRectGetMinY(wrapper.frame)-809)<0.01,
            @"screenshot-size episode capsule lifts only thirteen points to leave eight points above the visible rail");
        Check(fabs(info.center.y-(nativeInfoCenter.y-13))<0.01 &&
            fabs(CGRectGetMaxY(info.frame)-CGRectGetMinY(wrapper.frame))<0.01,
            @"episode spacing moves adjacent metadata and right actions together without a new overlap");
        Check(CGRectEqualToRect(progress.frame,nativeProgress) && CGRectEqualToRect(track.frame,nativeTrack) &&
            CGRectEqualToRect(entry.frame,nativeEntry) && CGAffineTransformIsIdentity(wrapper.transform),
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
    } @finally {
        [screen removeFromSuperview];
        HGAppearanceApply(@{@"enabled":@YES,@"player.separateProgress":@"show",@"layout.episodes.y":@0},nil);
    }
}
