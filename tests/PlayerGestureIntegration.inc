@interface HGAGesturePlayerFixture : FQVShortVideoVPlayerControlView
@property(nonatomic,strong) UITapGestureRecognizer *singleTapGesture;
@property(nonatomic,strong) UIGestureRecognizer *doubleTapGesture;
@property(nonatomic,strong) UIPinchGestureRecognizer *pinch;
@property(nonatomic) BOOL isCleanMode;
@property(nonatomic) BOOL isSeriesType;
@property(nonatomic) NSUInteger cleanChanges;
@end
@implementation HGAGesturePlayerFixture
- (id)videoDataModel {return self;}
- (BOOL)enterCleanMode:(BOOL)enabled {self.cleanChanges++;self.isCleanMode=enabled;return YES;}
@end
@interface HGAGestureCommentFixture : SSShortVideoRightInteractionCommentView
@property(nonatomic) NSUInteger opens;
@end
@implementation HGAGestureCommentFixture
- (id)delegate {return self;}
- (void)commentViewDidClick:(id)view {self.opens++;}
- (void)commentViewDidClicked {[self commentViewDidClick:self];}
@end
@interface HGAGestureTouchFixture : UITouch
@property(nonatomic,weak) UIView *hit;
@end
@implementation HGAGestureTouchFixture
- (UIView *)view {return self.hit;}
@end
@interface HGAGesturePinchFixture : UIPinchGestureRecognizer
@property(nonatomic) UIGestureRecognizerState phase;
@end
@implementation HGAGesturePinchFixture
- (UIGestureRecognizerState)state {return self.phase;}
@end
@interface HGAGestureDoubleTapFixture : UITapGestureRecognizer @end
@implementation HGAGestureDoubleTapFixture
- (UIGestureRecognizerState)state {return UIGestureRecognizerStateEnded;}
@end
@interface FQVShortVideoBaseInfoView : UIView
@property(nonatomic,strong) UIView *leftContainer;
@end
@implementation FQVShortVideoBaseInfoView @end
@protocol HGAGestureFixtureActions <UIGestureRecognizerDelegate>
- (void)pinched:(UIPinchGestureRecognizer *)pinch;
- (void)tapped:(UITapGestureRecognizer *)tap;
@end
static BOOL HGAGestureChromeCovered(UIView *view) {
    CABasicAnimation *opacity=(id)[view.layer animationForKey:@"honggo.clean.opacity"];
    return [opacity.keyPath isEqual:@"opacity"] && [opacity.toValue isEqual:@0] && !view.userInteractionEnabled;
}
@interface HGAAutoCleanContextFixture : NSObject
@property(nonatomic,strong) id currentModel;
@end
@implementation HGAAutoCleanContextFixture @end

@interface FQVShortVideoPlayerCellView (HGAAutoCleanFixture)
- (void)playWithType:(NSInteger)reason;
- (void)resumeWithType:(NSInteger)reason;
- (void)pauseWhenPlayNext;
@end
@implementation FQVShortVideoPlayerCellView (HGAAutoCleanFixture)
- (void)playWithType:(NSInteger)reason {}
- (void)resumeWithType:(NSInteger)reason {}
- (void)pauseWhenPlayNext {}
@end

@interface HGAAutoCleanCellFixture : FQVShortVideoPlayerCellView
@property(nonatomic,strong) HGAAutoCleanContextFixture *listVCContext;
@property(nonatomic,strong) UIView *vPlayerControlView;
@property(nonatomic) BOOL isPlaying;
@property(nonatomic) BOOL isUserPlaying;
@end
@implementation HGAAutoCleanCellFixture @end

@interface FQVShortVideoListVCControlViewComponent : NSObject
@property(nonatomic,strong) id currentVideoDataModel;
@property(nonatomic) NSUInteger taps;
- (void)controlView:(UIView *)hud singleTapPlay:(BOOL)play model:(id)model;
@end
@implementation FQVShortVideoListVCControlViewComponent
- (void)controlView:(UIView *)hud singleTapPlay:(BOOL)play model:(id)model { self.taps++; }
@end

@interface FQCShortVideoDanmakuContainerView : UIView @end
@implementation FQCShortVideoDanmakuContainerView @end
@interface FQVShortVideoLoadingView : UIView @end
@implementation FQVShortVideoLoadingView @end

static void RunAutomaticCleanChecks(UIView *root) {
    HGAppearanceApply(@{@"enabled":@NO,@"gestures.cleanWhilePlaying":@YES,@"gestures.pinchClean":@YES,
        @"player.highestQuality":@NO},nil);
    SSTabBarController *host=(id)root.nextResponder;
    UIViewController *list=[NSClassFromString(@"FQVShortVideoListViewController") new];
    [host addChildViewController:list];list.view.frame=root.bounds;[root addSubview:list.view];
    [list didMoveToParentViewController:host];
    HGAAutoCleanCellFixture *cell=[[HGAAutoCleanCellFixture alloc] initWithFrame:list.view.bounds];
    HGAGesturePlayerFixture *hud=[[HGAGesturePlayerFixture alloc] initWithFrame:cell.bounds];
    hud.isSeriesType=YES;
    cell.model=hud;cell.vPlayerControlView=hud;
    cell.listVCContext=[HGAAutoCleanContextFixture new];
    cell.listVCContext.currentModel=[NSObject new];
    [cell addSubview:hud];[list.view addSubview:cell];
    TTMetalView *picture=[[TTMetalView alloc] initWithFrame:cell.bounds];[cell insertSubview:picture belowSubview:hud];
    FQVShortVideoProgressView *progress=[[FQVShortVideoProgressView alloc] initWithFrame:CGRectMake(10,310,240,24)];
    FQCShortVideoDanmakuContainerView *comments=[[FQCShortVideoDanmakuContainerView alloc] initWithFrame:hud.bounds];
    FQVShortVideoLoadingView *loading=[[FQVShortVideoLoadingView alloc] initWithFrame:hud.bounds];
    UIButton *play=[UIButton buttonWithType:UIButtonTypeCustom];
    SSShortVideoBottomDistributionView *entry=[SSShortVideoBottomDistributionView new];
    for(UIView *control in @[progress,comments,loading,play,entry])[hud addSubview:control];
    UIView *dock=host.customTabBar;
    FQVShortVideoListVCControlViewComponent *tap=[FQVShortVideoListVCControlViewComponent new];
    tap.currentVideoDataModel=hud;
    [cell playWithType:10];
    Check(!HGAGestureChromeCovered(progress),@"an adjacent prepared cell does not enter playback clean mode");
    cell.listVCContext.currentModel=cell.model;
    [cell playWithType:10];
    Check(HGAGestureChromeCovered(dock) && HGAGestureChromeCovered(progress) && HGAGestureChromeCovered(entry) &&
        HGAGestureChromeCovered(comments) && HGAGestureChromeCovered(loading) && HGAGestureChromeCovered(play) &&
        play.accessibilityElementsHidden && !picture.layer.mask && !hud.layer.mask && hud.userInteractionEnabled,
        @"the current episode starts clean with playback controls, danmaku and loading ink hidden while original taps and video stay available");
    [entry.layer removeAllAnimations];
    Check(entry.layer.mask.opacity==0 && entry.layer.mask && !entry.isHidden && entry.alpha==1,
        @"host animation resets cannot flash the episode entry during clean playback");
    [tap controlView:hud singleTapPlay:NO model:[NSObject new]];
    Check(HGAGestureChromeCovered(progress),@"a stale tap from another episode cannot expose the active HUD");
    [tap controlView:hud singleTapPlay:NO model:hud];
    Check(!HGAGestureChromeCovered(dock) && !entry.layer.mask && !progress.layer.mask && play.userInteractionEnabled &&
        !play.accessibilityElementsHidden && tap.taps==2,@"the native tap pause restores controls and accessibility immediately");
    cell.isUserPlaying=YES;
    id ready=[NSClassFromString(@"FQVShortVideoListVCPlayComponent") new];
    SEL shown=NSSelectorFromString(@"cellView:vPlayerReadyToDisplay:");
    ((void(*)(id,SEL,id,id))[ready methodForSelector:shown])(ready,shown,cell,nil);
    Check(!HGAGestureChromeCovered(dock),@"a late first-frame callback cannot re-hide a user-paused episode");
    [tap controlView:hud singleTapPlay:YES model:hud];
    Check(HGAGestureChromeCovered(dock) && HGAGestureChromeCovered(play) && tap.taps==3,
        @"the native continue action re-enters automatic clean mode");
    [list viewDidDisappear:NO];
    Check(!HGAGestureChromeCovered(dock) && !entry.layer.mask,@"leaving the feed releases all shared and episode controls");

    SSShortVideoDetailViewController *detail=[SSShortVideoDetailViewController new];
    [host addChildViewController:detail];detail.view.frame=root.bounds;[root addSubview:detail.view];
    [detail didMoveToParentViewController:host];
    UIViewController *episodeList=[NSClassFromString(@"FQVShortVideoListViewController") new];
    [detail addChildViewController:episodeList];episodeList.view.frame=detail.view.bounds;
    [detail.view addSubview:episodeList.view];[episodeList didMoveToParentViewController:detail];
    [episodeList.view addSubview:cell];
    FQVShortVideoListVCNavBarComponent *navigation=[FQVShortVideoListVCNavBarComponent new];
    navigation.navBar=[[UIView alloc] initWithFrame:CGRectMake(0,0,360,44)];
    [detail.view addSubview:navigation.navBar];[navigation updateNavgationBarLeftView];
    [cell resumeWithType:2];
    Check(HGAGestureChromeCovered(navigation.navBar) && HGAGestureChromeCovered(progress),
        @"detail resume covers its local navigation and the next episode's controls");
    HGAppearanceSet(@"gestures.cleanWhilePlaying",@NO,nil);
    Check(!HGAGestureChromeCovered(navigation.navBar) && !HGAGestureChromeCovered(dock) && !progress.layer.mask,
        @"disabling auto clean restores detail chrome even if the manual gesture remains enabled");
    [detail willMoveToParentViewController:nil];[detail.view removeFromSuperview];[detail removeFromParentViewController];
    [list willMoveToParentViewController:nil];[list.view removeFromSuperview];[list removeFromParentViewController];
}
static void RunTransientPauseFlashChecks(UIView *root) {
    NSDictionary *saved=HGAppearanceValues();
    HGAppearanceApply(@{@"enabled":@YES,@"gestures.cleanWhilePlaying":@NO,@"player.rightGlass":@YES,@"player.actionStyle":@"glass"},nil);
    HGAAutoCleanCellFixture *cell=[[HGAAutoCleanCellFixture alloc] initWithFrame:CGRectMake(0,0,360,640)];
    cell.isUserPlaying=YES;
    cell.isPlaying=NO;
    cell.model=[NSObject new];
    FQVShortVideoVPlayerControlView *hud=[[FQVShortVideoVPlayerControlView alloc] initWithFrame:cell.bounds];
    cell.vPlayerControlView=hud;
    UIButton *play=[UIButton buttonWithType:UIButtonTypeCustom];
    play.frame=CGRectMake(156,296,48,48);
    [play setImage:[UIImage systemImageNamed:@"play.fill"] forState:UIControlStateNormal];
    hud.playBtn=play;
    [hud addSubview:play];
    [cell addSubview:hud];
    [root addSubview:cell];
    [cell playWithType:0];
    HGAppearanceProcessView(hud);
    CABasicAnimation *graceCover=(id)[play.layer animationForKey:@"honggo.playback.grace.opacity"];
    BOOL suppressedTransientFlash=([graceCover.keyPath isEqual:@"opacity"] && [graceCover.toValue isEqual:@0] &&
        fabs(play.alpha-1.0)<0.001);
    // Simulate host hiding _playBtn via alpha=0 once playback starts, then grace expiring:
    // releasing the presentation cover must NEVER overwrite host alpha=0 back to 1.0!
    play.alpha=0.0;
    cell.isUserPlaying=NO;
    HGAppearanceProcessView(hud);
    BOOL preservesHostZeroAlphaAfterGrace=(![play.layer animationForKey:@"honggo.playback.grace.opacity"] &&
        fabs(play.alpha)<0.001);
    // Simulate host calling pauseWhenPlayNext (alpha=1) while cell is displaced in a vertical feed swipe:
    FQVCollectionView *feed=[[FQVCollectionView alloc] initWithFrame:CGRectMake(0,0,360,640)
        collectionViewLayout:[UICollectionViewFlowLayout new]];
    [root addSubview:feed];
    UICollectionViewCell *wrapperCell=[[UICollectionViewCell alloc] initWithFrame:CGRectMake(0,-120,360,640)];
    [wrapperCell.contentView addSubview:cell];
    [feed addSubview:wrapperCell];
    play.alpha=1.0;
    [cell pauseWhenPlayNext];
    BOOL suppressedOutgoingSwipePause=([play.layer animationForKey:@"honggo.playback.grace.opacity"]!=nil &&
        fabs(play.alpha-1.0)<0.001);
    wrapperCell.frame=CGRectMake(0,0,360,640);
    FQVShortVideoListVCControlViewComponent *tap=[FQVShortVideoListVCControlViewComponent new];
    tap.currentVideoDataModel=cell.model;
    [tap controlView:hud singleTapPlay:NO model:cell.model];
    BOOL revealedOnUserPause=(![play.layer animationForKey:@"honggo.playback.grace.opacity"] &&
        fabs(play.alpha-1.0)<0.001 && Find(play,@"honggo.appearance.material")!=nil);
    Check(suppressedTransientFlash && preservesHostZeroAlphaAfterGrace &&
        suppressedOutgoingSwipePause && revealedOnUserPause,
        @"playback grace covers transient/scroll playBtn flashes via presentation opacity, never overwrites host alpha=0 to 1, and reveals immediately on manual pause");
    [cell removeFromSuperview];
    [feed removeFromSuperview];
    HGAppearanceApply(saved,nil);
}
static void RunPlayerGestureChecks(UIView *root) {
    NSDictionary *saved=HGAppearanceValues();
    HGAppearanceApply(@{@"enabled":@NO,@"gestures.pinchClean":@YES,@"gestures.doubleTapComments":@YES},nil);
    UIViewController *host=(id)root.nextResponder;
    UIViewController *list=[NSClassFromString(@"FQVShortVideoListViewController") new];
    [host addChildViewController:list];list.view.frame=root.bounds;[root addSubview:list.view];
    [list didMoveToParentViewController:host];
    FQVShortVideoPlayerCellView *canvas=[[FQVShortVideoPlayerCellView alloc] initWithFrame:list.view.bounds];
    [list.view addSubview:canvas];
    HGAGesturePlayerFixture *player=[[HGAGesturePlayerFixture alloc] initWithFrame:CGRectMake(0,0,260,360)];
    player.isSeriesType=YES;
    player.singleTapGesture=[UITapGestureRecognizer new];[player addGestureRecognizer:player.singleTapGesture];
    player.doubleTapGesture=[UITapGestureRecognizer new];[player addGestureRecognizer:player.doubleTapGesture];
    player.pinch=[UIPinchGestureRecognizer new];[player addGestureRecognizer:player.pinch];
    HGAGestureCommentFixture *comment=[[HGAGestureCommentFixture alloc] initWithFrame:CGRectMake(200,200,46,62)];
    [player addSubview:comment];[canvas addSubview:player];
    player.fullscreenBtn=[UIButton buttonWithType:UIButtonTypeSystem];[player addSubview:player.fullscreenBtn];
    SSShortVideoBottomDistributionView *entry=[SSShortVideoBottomDistributionView new];[player addSubview:entry];
    FQVShortVideoBaseInfoView *info=[[FQVShortVideoBaseInfoView alloc] initWithFrame:player.bounds];[player addSubview:info];
    UIView *titleColumn=[UIView new];[info addSubview:titleColumn];info.leftContainer=titleColumn;
    FQVShortVideoProgressView *progress=[FQVShortVideoProgressView new];[player addSubview:progress];
    CALayer *nativeMask=[CALayer layer];progress.layer.mask=nativeMask;
    SSShortVideoBottomDistributionView *otherEntry=[SSShortVideoBottomDistributionView new];[root addSubview:otherEntry];
    SSVideoSeriesFeedHeaderView *header=[SSVideoSeriesFeedHeaderView new];[root addSubview:header];
    HGAppearanceProcessView(header);HGAppearanceProcessView(otherEntry);
    UIView *dock=[(SSTabBarController *)host customTabBar];
    HGAppearanceProcessView(player);HGAppearanceProcessView(player);
    NSPredicate *plugin=[NSPredicate predicateWithBlock:^BOOL(UIGestureRecognizer *g,NSDictionary *bindings){
        return [g.name hasPrefix:@"Honggo.Player"];
    }];
    NSArray *gestures=[player.gestureRecognizers filteredArrayUsingPredicate:plugin];
    UIPinchGestureRecognizer *pinch=nil;UITapGestureRecognizer *tap=nil;
    for(UIGestureRecognizer *g in gestures) {
        if([g isKindOfClass:UIPinchGestureRecognizer.class])pinch=(id)g;
        if([g isKindOfClass:UITapGestureRecognizer.class])tap=(id)g;
    }
    id<HGAGestureFixtureActions> driver=(id)pinch.delegate;
    Check(gestures.count==2 && tap.numberOfTapsRequired==2 && [driver gestureRecognizerShouldBegin:tap] &&
        [driver gestureRecognizerShouldBegin:pinch],@"both gestures attach once and run without the appearance master");
    Check([driver gestureRecognizer:tap shouldBeRequiredToFailByGestureRecognizer:player.doubleTapGesture] &&
        [driver gestureRecognizer:tap shouldBeRequiredToFailByGestureRecognizer:player.singleTapGesture] &&
        [driver gestureRecognizer:pinch shouldBeRequiredToFailByGestureRecognizer:player.pinch],
        @"enabled player gestures have priority over their original playback recognizers");
    UIPinchGestureRecognizer *canvasPinch=[UIPinchGestureRecognizer new];[canvas addGestureRecognizer:canvasPinch];
    UIPinchGestureRecognizer *pagePinch=[UIPinchGestureRecognizer new];[list.view addGestureRecognizer:pagePinch];
    Check([driver gestureRecognizer:pinch shouldBeRequiredToFailByGestureRecognizer:canvasPinch] &&
        ![driver gestureRecognizer:pinch shouldBeRequiredToFailByGestureRecognizer:pagePinch],
        @"a native pinch on the cell yields to its HUD; unrelated ancestor gestures keep their priority");
    HGAGestureTouchFixture *touch=[HGAGestureTouchFixture new];touch.hit=player;
    BOOL surface=[driver gestureRecognizer:tap shouldReceiveTouch:touch];
    player.isSeriesType=NO;
    BOOL other=[driver gestureRecognizer:tap shouldReceiveTouch:touch];
    player.isSeriesType=YES;touch.hit=comment;
    Check(surface && !other && ![driver gestureRecognizer:tap shouldReceiveTouch:touch],
        @"non-drama surfaces and the native action rail retain their touches");
    HGAGesturePinchFixture *sample=[HGAGesturePinchFixture new];
    sample.phase=UIGestureRecognizerStateBegan;sample.scale=1.05;[driver pinched:sample];
    sample.phase=UIGestureRecognizerStateChanged;sample.scale=1.3;[driver pinched:sample];
    sample.scale=0.7;[driver pinched:sample];
    Check(HGAGestureChromeCovered(comment) && HGAGestureChromeCovered(player.fullscreenBtn) &&
        HGAGestureChromeCovered(entry) && HGAGestureChromeCovered(header) && HGAGestureChromeCovered(dock) &&
        !HGAGestureChromeCovered(otherEntry) && player.userInteractionEnabled && ![driver gestureRecognizerShouldBegin:tap],
        @"spread covers only this list's HUD and its enclosing header and dock; the paging surface stays interactive");
    [progress.layer removeAllAnimations];[titleColumn.layer removeAllAnimations];
    Check(titleColumn.layer.mask && progress.layer.mask && progress.layer.mask!=nativeMask && !info.layer.mask,
        @"a host fade reset cannot reveal the real progress rail or title column, and their full-screen owner stays interactive");
    [player removeFromSuperview];
    HGAGesturePlayerFixture *nextPlayer=[[HGAGesturePlayerFixture alloc] initWithFrame:player.frame];
    nextPlayer.isSeriesType=YES;[list.view addSubview:nextPlayer];HGAppearanceProcessView(nextPlayer);
    FQVShortVideoPlayerBottomView *bottom=[FQVShortVideoPlayerBottomView new];
    [nextPlayer addSubview:bottom];HGAppearanceProcessView(bottom);
    UIPinchGestureRecognizer *nextPinch=nil;
    for(UIGestureRecognizer *g in nextPlayer.gestureRecognizers)
        if([g.name isEqual:@"Honggo.PlayerCleanPinch"])nextPinch=(id)g;
    id<HGAGestureFixtureActions> nextDriver=(id)nextPinch.delegate;
    Check(HGAGestureChromeCovered(bottom) && HGAGestureChromeCovered(dock) && nextPlayer.userInteractionEnabled &&
        [nextDriver gestureRecognizerShouldBegin:nextPinch],@"the replacement episode acquires clean mode and can restore it");
    bottom.hidden=YES;bottom.alpha=0.6;
    sample.phase=UIGestureRecognizerStateBegan;[nextDriver pinched:sample];
    Check(!HGAGestureChromeCovered(dock) && !HGAGestureChromeCovered(bottom) && bottom.hidden &&
        !titleColumn.layer.mask && progress.layer.mask==nativeMask &&
        fabs(bottom.alpha-0.6)<0.01 && bottom.userInteractionEnabled && nextPlayer.cleanChanges==0 && player.cleanChanges==0,
        @"contracting on the next episode restores current native visibility without changing either model's clean state");
    nextPlayer.isCleanMode=YES;[nextDriver pinched:sample];
    Check(!nextPlayer.isCleanMode && nextPlayer.cleanChanges==1,@"contracting also exits clean mode selected in the host menu");
    sample.scale=1.3;[nextDriver pinched:sample];
    [list viewDidDisappear:NO];
    Check(!HGAGestureChromeCovered(dock) && !HGAGestureChromeCovered(header),
        @"leaving the list releases shared navigation even when its last episode was clean");
    [list.view addSubview:player];HGAppearanceProcessView(player);
    for(UIGestureRecognizer *g in player.gestureRecognizers) {
        if([g.name isEqual:@"Honggo.PlayerCommentDoubleTap"])tap=(id)g;
        if([g.name isEqual:@"Honggo.PlayerCleanPinch"])pinch=(id)g;
    }
    driver=(id)pinch.delegate;
    [driver tapped:[HGAGestureDoubleTapFixture new]];
    comment.hidden=YES;
    Check(comment.opens==1 && ![driver gestureRecognizerShouldBegin:tap],
        @"double tap invokes the current player's real comment action and yields when the action is unavailable");
    sample.scale=1.3;[driver pinched:sample];
    HGAppearanceSet(@"gestures.pinchClean",@NO,nil);
    Check(!pinch.view && tap.view==player && !HGAGestureChromeCovered(dock) &&
        comment.hidden && comment.userInteractionEnabled,@"disabling clean mode restores chrome and leaves the comment gesture active");
    HGAppearanceSet(@"gestures.doubleTapComments",@NO,nil);
    Check(!tap.view && player.pinch.view==player && player.doubleTapGesture.view==player,
        @"disabling both removes only plugin recognizers");
    [list willMoveToParentViewController:nil];[list.view removeFromSuperview];[list removeFromParentViewController];
    [header removeFromSuperview];[otherEntry removeFromSuperview];
    RunAutomaticCleanChecks(root);
    RunTransientPauseFlashChecks(root);
    HGAppearanceApply(saved,nil);
}
