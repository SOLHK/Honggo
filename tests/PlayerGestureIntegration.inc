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
@protocol HGAGestureFixtureActions <UIGestureRecognizerDelegate>
- (void)pinched:(UIPinchGestureRecognizer *)pinch;
- (void)tapped:(UITapGestureRecognizer *)tap;
@end
static BOOL HGAGestureChromeCovered(UIView *view) {
    CABasicAnimation *opacity=(id)[view.layer animationForKey:@"honggo.clean.opacity"];
    return [opacity.keyPath isEqual:@"opacity"] && [opacity.toValue isEqual:@0] && !view.userInteractionEnabled;
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
    [header removeFromSuperview];[otherEntry removeFromSuperview];HGAppearanceApply(saved,nil);
}
