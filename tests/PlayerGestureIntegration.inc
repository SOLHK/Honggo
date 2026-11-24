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
static void RunPlayerGestureChecks(UIView *root) {
    NSDictionary *saved=HGAppearanceValues();
    HGAppearanceApply(@{@"enabled":@NO,@"gestures.pinchClean":@YES,@"gestures.doubleTapComments":@YES},nil);
    HGAGesturePlayerFixture *player=[[HGAGesturePlayerFixture alloc] initWithFrame:CGRectMake(0,0,260,360)];
    player.isSeriesType=YES;
    player.singleTapGesture=[UITapGestureRecognizer new];[player addGestureRecognizer:player.singleTapGesture];
    player.doubleTapGesture=[UITapGestureRecognizer new];[player addGestureRecognizer:player.doubleTapGesture];
    player.pinch=[UIPinchGestureRecognizer new];[player addGestureRecognizer:player.pinch];
    HGAGestureCommentFixture *comment=[[HGAGestureCommentFixture alloc] initWithFrame:CGRectMake(200,200,46,62)];
    [player addSubview:comment];[root addSubview:player];
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
    BOOL once=player.isCleanMode && player.cleanChanges==1;
    sample.phase=UIGestureRecognizerStateBegan;[driver pinched:sample];
    Check(once && !player.isCleanMode && player.cleanChanges==2,@"one spread enters clean mode once and the next pinch restores it");
    [driver tapped:[HGAGestureDoubleTapFixture new]];
    comment.hidden=YES;
    Check(comment.opens==1 && ![driver gestureRecognizerShouldBegin:tap],
        @"double tap invokes the current player's real comment action and yields when the action is unavailable");
    HGAppearanceSet(@"gestures.pinchClean",@NO,nil);
    Check(!pinch.view && tap.view==player,@"disabling clean-screen pinch leaves the comment gesture active");
    HGAppearanceSet(@"gestures.doubleTapComments",@NO,nil);
    Check(!tap.view && player.pinch.view==player && player.doubleTapGesture.view==player,
        @"disabling both removes only plugin recognizers");
    [player removeFromSuperview];HGAppearanceApply(saved,nil);
}
