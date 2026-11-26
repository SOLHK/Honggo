@interface FQVShortVideoBaseLeftContainerView : UIView
@property(nonatomic,strong) UILabel *nameLabel;
@property(nonatomic,strong) UILabel *titleLabel;
@property(nonatomic,strong) UILabel *expandedLabel;
- (CGRect)convertSeriesNameFrameToView:(UIView *)view;
@end
@implementation FQVShortVideoBaseLeftContainerView
- (CGRect)convertSeriesNameFrameToView:(UIView *)view {return CGRectMake(4,4,100,20);}
@end

@interface FQVShortVideoVPlayerControlView : UIView <UIGestureRecognizerDelegate>
@property(nonatomic,strong) UIButton *fullscreenBtn;
@property(nonatomic) CGRect playerViewFrame;
@property(nonatomic,strong) UIButton *playBtn;
@property(nonatomic,strong,readonly) UIPanGestureRecognizer *progressPanGesture;
@property(nonatomic) NSUInteger nativeClipPasses;
- (void)handleOutterFeedClipBounds:(BOOL)clip;
- (CGRect)convertTitleClickableFrameToView:(UIView *)view;
@end
@implementation FQVShortVideoVPlayerControlView
@synthesize progressPanGesture=_progressPanGesture;
- (CGRect)convertTitleClickableFrameToView:(UIView *)view {return CGRectMake(4,24,125,20);}
- (UIPanGestureRecognizer *)progressPanGesture {
    if(!_progressPanGesture) {
        _progressPanGesture=[UIPanGestureRecognizer new];
        _progressPanGesture.delegate=self;
        [self addGestureRecognizer:_progressPanGesture];
    }
    return _progressPanGesture;
}
- (void)handleOutterFeedClipBounds:(BOOL)clip {
    self.nativeClipPasses++;
    for(UIView *view=self.superview;view && ![view.nextResponder isKindOfClass:UIViewController.class];view=view.superview)
        view.clipsToBounds=clip;
}
@end

@interface SSShortVideoBottomInsertView : UIView
@property(nonatomic,strong) UIView *cleanSwitchView;
@property(nonatomic,strong) UIView *cleanScreenView;
@end
@implementation SSShortVideoBottomInsertView @end

@interface FQVShortVideoControlCleanSwitchView : UIView @end
@implementation FQVShortVideoControlCleanSwitchView @end

@interface FQVShortVideoVPlayerEpisodeInfoView : UIView
@property(nonatomic,strong) UIColor *nativePaint;
- (void)refreshSubviews;
@end
@implementation FQVShortVideoVPlayerEpisodeInfoView
- (void)refreshSubviews { self.layer.backgroundColor=self.nativePaint.CGColor; }
@end

@interface FQVShortVideoPlayerBottomView : UIView @end
@implementation FQVShortVideoPlayerBottomView @end

@interface SSShortVideoTagListBottomInsertView : SSShortVideoBottomInsertView
@property(nonatomic,strong) UIView *contentView;
@property(nonatomic,strong) UIButton *episodeInfoView;
- (void)setVideoDataModel:(UIColor *)paint;
@end
@implementation SSShortVideoTagListBottomInsertView
- (void)setVideoDataModel:(UIColor *)paint {self.episodeInfoView.layer.backgroundColor=paint.CGColor;}
@end

@interface SSShortVideoDetailViewController : UIViewController
@property(nonatomic,strong) UILabel *navLeftLabel;
@end
@implementation SSShortVideoDetailViewController @end

@interface SSShortVideoDetailViewController_hmd_subfix : SSShortVideoDetailViewController @end
@implementation SSShortVideoDetailViewController_hmd_subfix @end

@interface FQVShortVideoListVCNavBarComponent : NSObject
@property(nonatomic,strong) UIView *navBar;
@property(nonatomic,strong) UIButton *backBtn;
@property(nonatomic,strong) UILabel *navLeftLabel;
@property(nonatomic,strong) UIView *playRateView;
@property(nonatomic,strong) UIImageView *playRateImgView;
@property(nonatomic,strong) UILabel *playRateLabel;
@property(nonatomic,strong) UIButton *moreBtn;
- (void)updateNavgationBarLeftView;
@end
@implementation FQVShortVideoListVCNavBarComponent
- (void)updateNavgationBarLeftView {}
@end

@interface SSShortVideoIntroductionTabViewController : UIViewController @end
@implementation SSShortVideoIntroductionTabViewController @end

@interface FQVTransitionPlayerTopContentView : UIView
@property(nonatomic,strong) UIView *playerContainerView;
@end
@implementation FQVTransitionPlayerTopContentView @end

@interface FQVShortVideoEpisodesNumberView : UIView
@property(nonatomic,strong) UICollectionView *collectionView;
@property(nonatomic,strong) UIView *leftGradientView;
@property(nonatomic,strong) UIView *rightGradientView;
@end
@implementation FQVShortVideoEpisodesNumberView @end

@interface SSShortVideoEpisodeSubViewController : UIViewController
@property(nonatomic,strong) UICollectionView *collectionView;
@property(nonatomic,strong) UIColor *nativeListBackground;
- (void)onDarkModeChange:(BOOL)dark;
@end
@implementation SSShortVideoEpisodeSubViewController
- (void)onDarkModeChange:(BOOL)dark {
    self.collectionView.backgroundColor=dark ? UIColor.blackColor : self.nativeListBackground;
}
@end

@interface SSShortVideoEpisodesViewItemCell : UICollectionViewCell
@property(nonatomic,strong) UIView *bgView;
@end
@implementation SSShortVideoEpisodesViewItemCell @end

@interface SSShortVideoEpisodePanelCollectView : UIView
@property(nonatomic,strong) UIButton *containerView;
@property(nonatomic,strong) UIView *shareContainerView;
@property(nonatomic,strong) UIView *bgView;
@property(nonatomic,strong) UIView *shareBgView;
@property(nonatomic,strong) UILabel *textL;
@property(nonatomic,strong) UIImageView *iconImageView;
@end
@implementation SSShortVideoEpisodePanelCollectView @end

@interface FQCShortVideoDanmakuEntranceView : UIView
@property(nonatomic,strong) UIView *capsuleView;
@property(nonatomic,strong) UIButton *clickView;
@property(nonatomic,strong) UIButton *publishButton;
@property(nonatomic,strong) UIView *dividerView;
@property(nonatomic,strong) UIImageView *iconView;
@property(nonatomic,strong) UIImage *legacyImage;
- (void)refreshIcon;
@end
@implementation FQCShortVideoDanmakuEntranceView
- (void)refreshIcon {self.iconView.image=self.legacyImage;}
@end

static void RunDanmakuChromeChecks(UIView *root) {
    NSDictionary *saved=HGAppearanceValues();
    HGAppearanceApply(@{@"enabled":@YES,@"player.controlsGlass":@YES,@"player.material":@"clear"},nil);

    FQCShortVideoDanmakuEntranceView *entrance=[[FQCShortVideoDanmakuEntranceView alloc] initWithFrame:CGRectMake(20,180,90,32)];
    entrance.capsuleView=[[UIView alloc] initWithFrame:entrance.bounds];
    entrance.capsuleView.hidden=YES;
    [entrance addSubview:entrance.capsuleView];
    entrance.clickView=[UIButton buttonWithType:UIButtonTypeCustom];
    entrance.clickView.frame=CGRectMake(0,0,32,32);
    HGAEpisodeTapFixture *action=[HGAEpisodeTapFixture new];
    [entrance.clickView addTarget:action action:@selector(open:) forControlEvents:UIControlEventTouchUpInside];
    [entrance addSubview:entrance.clickView];
    UIGraphicsImageRenderer *renderer=[[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(32,32)];
    entrance.legacyImage=[renderer imageWithActions:^(UIGraphicsImageRendererContext *context){
        [[UIColor colorWithWhite:23.0/255 alpha:1] setFill];
        [[UIBezierPath bezierPathWithRoundedRect:CGRectMake(0,0,32,32) cornerRadius:8] fill];
        [UIColor.whiteColor setFill];UIRectFill(CGRectMake(12,10,7,12));
    }];
    entrance.iconView=[[UIImageView alloc] initWithImage:entrance.legacyImage];
    [entrance.clickView addSubview:entrance.iconView];
    [root addSubview:entrance];
    HGAppearanceProcessView(entrance);
    UIImage *foreground=entrance.iconView.image;
    UIView *compactGlass=Find(entrance.clickView,@"honggo.appearance.material");
    Layout(compactGlass);
    Check(!Find(entrance.capsuleView,@"honggo.appearance.material") &&
        compactGlass.superview==entrance.clickView && (!HGTestOpenGlass(entrance) || HGTestButtonGlass(compactGlass)) &&
        HGAActionPixelAlpha(entrance.legacyImage,5,10)>240 &&
        HGAActionPixelAlpha(foreground,5,10)==0 && HGAActionPixelAlpha(foreground,16,16)>240,
        @"compact danmaku targets the visible button and replaces only its known baked dark plate");
    [entrance refreshIcon];
    Check(entrance.iconView.image==foreground && Count(entrance,@"honggo.appearance.material")==1,
        @"danmaku refresh reuses the foreground extraction and one glass surface");

    entrance.capsuleView.hidden=NO;
    UIImage *combined=[UIImage systemImageNamed:@"checkmark.circle.fill"];
    entrance.iconView.image=combined;
    HGAppearanceProcessView(entrance);
    UIView *combinedGlass=Find(entrance.capsuleView,@"honggo.appearance.material");
    Layout(combinedGlass);
    Check(combinedGlass && (!HGTestOpenGlass(entrance) || HGTestButtonGlass(combinedGlass)) &&
        !Find(entrance.clickView,@"honggo.appearance.material") && entrance.iconView.image==combined,
        @"combined danmaku moves glass to the capsule and keeps its native state image");

    entrance.publishButton=[UIButton buttonWithType:UIButtonTypeCustom];
    entrance.publishButton.frame=CGRectMake(34,0,56,32);
    [entrance addSubview:entrance.publishButton];
    entrance.dividerView=[[UIView alloc] initWithFrame:CGRectMake(32,8,1,16)];
    [entrance addSubview:entrance.dividerView];
    for(NSUInteger mask=0;mask<4;mask++) {
        BOOL hideToggle=(mask&1)!=0, hidePublish=(mask&2)!=0;
        HGAppearanceApply(@{@"player.hideFeedDanmakuToggle":@(hideToggle),@"player.hideFeedDanmakuPublish":@(hidePublish)},nil);
        HGAppearanceProcessView(entrance);
        Check((entrance.clickView.alpha==0)==hideToggle && (entrance.publishButton.alpha==0)==hidePublish &&
            (entrance.dividerView.alpha==0)==(mask!=0) && (!hideToggle || [entrance hitTest:CGPointMake(16,16) withEvent:nil]!=entrance.clickView) &&
            (hidePublish || [entrance hitTest:CGPointMake(60,16) withEvent:nil]==entrance.publishButton),
            @"the danmaku switch and composer can each be disabled without taking away the other's tap");
    }
    entrance.publishButton.hidden=YES;
    HGAppearanceApply(@{@"player.hideFeedDanmakuToggle":@NO,@"player.hideFeedDanmakuPublish":@NO},nil);
    HGAppearanceProcessView(entrance);
    Check(entrance.publishButton.hidden && !entrance.capsuleView.layer.mask && entrance.dividerView.alpha==1,
        @"restoring both actions preserves the host's own hidden state and restores the full native capsule");

    entrance.capsuleView.hidden=YES;
    entrance.iconView.image=entrance.legacyImage;
    HGAppearanceProcessView(entrance);
    [entrance.clickView sendActionsForControlEvents:UIControlEventTouchUpInside];
    HGAppearanceSet(@"player.controlsGlass",@NO,nil);
    HGAppearanceProcessView(entrance);
    Check(!Find(entrance,@"honggo.appearance.material") && entrance.iconView.image==entrance.legacyImage &&
        action.taps==1 && CGRectEqualToRect(entrance.clickView.frame,CGRectMake(0,0,32,32)),
        @"disabling danmaku glass restores the original art and preserves its native action and frame");
    [entrance removeFromSuperview];
    HGAppearanceApply(saved,nil);
}

static void RunDetailNavigationChromeChecks(UIViewController *host) {
    SSShortVideoDetailViewController *detail=[SSShortVideoDetailViewController_hmd_subfix new];
    [host addChildViewController:detail];
    detail.view.frame=CGRectMake(0,72,360,140);
    [host.view addSubview:detail.view];
    [detail didMoveToParentViewController:host];

    UIViewController *playback=[UIViewController new];
    [detail addChildViewController:playback];
    playback.view.frame=detail.view.bounds;
    [detail.view addSubview:playback.view];
    [playback didMoveToParentViewController:detail];

    FQVShortVideoListVCNavBarComponent *navigation=[FQVShortVideoListVCNavBarComponent new];
    navigation.navBar=[[UIView alloc] initWithFrame:CGRectMake(0,0,360,44)];
    [playback.view addSubview:navigation.navBar];
    navigation.backBtn=[UIButton buttonWithType:UIButtonTypeCustom];
    navigation.backBtn.frame=CGRectMake(18,10,24,24);
    [navigation.backBtn setImage:[UIImage systemImageNamed:@"chevron.left"] forState:UIControlStateNormal];
    [navigation.navBar addSubview:navigation.backBtn];
    navigation.navLeftLabel=[[UILabel alloc] initWithFrame:CGRectMake(42,10,100,24)];
    [navigation.navBar addSubview:navigation.navLeftLabel];

    UIView *detailNavigation=[[UIView alloc] initWithFrame:navigation.navBar.frame];
    detailNavigation.userInteractionEnabled=NO;
    [detail.view addSubview:detailNavigation];
    detail.navLeftLabel=[[UILabel alloc] initWithFrame:CGRectMake(44,10,100,24)];
    detail.navLeftLabel.text=@"第2集";
    [detailNavigation addSubview:detail.navLeftLabel];

    navigation.playRateView=[[UIView alloc] initWithFrame:CGRectMake(228,5,62,30)];
    [navigation.navBar addSubview:navigation.playRateView];
    navigation.playRateImgView=[[UIImageView alloc] initWithImage:[UIImage systemImageNamed:@"speedometer"]];
    navigation.playRateImgView.frame=CGRectMake(2,7,16,16);
    [navigation.playRateView addSubview:navigation.playRateImgView];
    navigation.playRateLabel=[[UILabel alloc] initWithFrame:CGRectMake(20,5,42,20)];
    navigation.playRateLabel.text=@"1.0x";
    [navigation.playRateView addSubview:navigation.playRateLabel];
    navigation.moreBtn=[UIButton buttonWithType:UIButtonTypeCustom];
    navigation.moreBtn.frame=CGRectMake(316,12,24,24);
    [navigation.moreBtn setImage:[UIImage systemImageNamed:@"ellipsis"] forState:UIControlStateNormal];
    [navigation.navBar addSubview:navigation.moreBtn];

    [navigation updateNavgationBarLeftView];
    UIView *backGlass=Find(navigation.navBar,@"honggo.appearance.material");
    UIView *rateGlass=Find(navigation.playRateView,@"honggo.appearance.material");
    UIView *moreGlass=Find(navigation.moreBtn,@"honggo.appearance.material");
    CGRect titleInk=[detail.navLeftLabel textRectForBounds:detail.navLeftLabel.bounds limitedToNumberOfLines:1];
    titleInk=[detail.navLeftLabel convertRect:titleInk toView:navigation.navBar];
    CGRect ratePlate=[rateGlass.superview convertRect:rateGlass.frame toView:navigation.navBar];
    CGRect morePlate=[moreGlass.superview convertRect:moreGlass.frame toView:navigation.navBar];
    Check(backGlass.superview==navigation.navBar && rateGlass && moreGlass &&
        Count(navigation.navBar,@"honggo.appearance.material")==3 &&
        CGRectContainsRect(backGlass.frame,titleInk) &&
        CGRectGetWidth(backGlass.frame)>CGRectGetWidth(morePlate) && CGRectGetWidth(ratePlate)>CGRectGetWidth(morePlate) &&
        CGRectGetHeight(backGlass.frame)==34 && CGRectGetHeight(ratePlate)==34 && CGRectGetHeight(morePlate)==34 &&
        CGRectGetMidY(backGlass.frame)==CGRectGetMidY(ratePlate) && CGRectGetMidY(ratePlate)==CGRectGetMidY(morePlate),
        @"detail navigation includes the controller-owned episode title with the arrow and aligns three content-sized surfaces");
    Check([navigation.navBar hitTest:CGPointMake(30,22) withEvent:nil]==navigation.backBtn &&
        [navigation.navBar hitTest:CGPointMake(328,24) withEvent:nil]==navigation.moreBtn &&
        CGRectEqualToRect(navigation.playRateView.frame,CGRectMake(228,5,62,30)),
        @"detail navigation keeps native button hit areas and the playback speed frame");

    HGAppearanceSet(@"player.controlsGlass",@NO,nil);
    [navigation updateNavgationBarLeftView];
    Check(!Find(navigation.navBar,@"honggo.appearance.material") && [detail.navLeftLabel.text isEqual:@"第2集"],
        @"disabling detail navigation glass preserves the native episode title");
    HGAppearanceSet(@"player.controlsGlass",@YES,nil);
    [detail willMoveToParentViewController:nil];
    [detail.view removeFromSuperview];
    [detail removeFromParentViewController];
}

@interface HGABadgeTestModel : NSObject
@property(nonatomic,copy) NSString *reasonText;
@end
@implementation HGABadgeTestModel @end

@interface SSShortVideoRcmdReasonView : UIControl
@property(nonatomic,strong) HGABadgeTestModel *dataModel;
@property(nonatomic,strong) UIView *gradientBackgroundView;
@property(nonatomic,strong) UIImageView *iconView;
@property(nonatomic) BOOL night;
@end
@implementation SSShortVideoRcmdReasonView
- (BOOL)fqv_isDarkMode {return self.night;}
- (void)modelDidUpdate {}
@end

static void RunPlayerBadgeChecks(UIView *host) {
    SSShortVideoAboveLeftContainerView *stack=[[SSShortVideoAboveLeftContainerView alloc] initWithFrame:CGRectMake(0,0,240,64)];
    [host addSubview:stack];
    SSShortVideoRcmdReasonView *badge=[[SSShortVideoRcmdReasonView alloc] initWithFrame:CGRectMake(0,0,190,28)];
    badge.backgroundColor=UIColor.blackColor;
    badge.gradientBackgroundView=[[UIView alloc] initWithFrame:badge.bounds];
    [badge addSubview:badge.gradientBackgroundView];
    badge.iconView=[[UIImageView alloc] initWithFrame:CGRectMake(6,7,14,14)];
    [badge addSubview:badge.iconView];
    badge.dataModel=[HGABadgeTestModel new];
    badge.dataModel.reasonText=@"追更榜 No.1";
    [stack addSubview:badge];
    [badge modelDidUpdate];
    UIView *glyph=Find(badge,@"honggo.appearance.playerBadgeGlyph");
    UIView *glass=Find(badge,@"honggo.appearance.material");
    Check(glass && !glass.userInteractionEnabled && badge.gradientBackgroundView.hidden &&
          [glyph.tintColor isEqual:UIColor.blackColor] && [badge hitTest:CGPointMake(90,14) withEvent:nil]==badge,
          @"rank badge uses glass on its original hit area and black ink with the day theme");
    HGAppearanceSet(@"player.hide.followRank",@YES,nil);
    [badge modelDidUpdate];
    Check(badge.hidden,@"rank visibility setting hides the bound rank link");
    badge.dataModel.reasonText=@"系列剧·已上线5季";
    badge.night=YES;
    [badge modelDidUpdate];
    glass=Find(badge,@"honggo.appearance.material");
    Layout(glass);
    NSString *savedMaterial=HGAppearanceString(@"player.material");
    if(HGTestOpenGlass(badge) && ![savedMaterial isEqual:@"blur"])Check(HGTestButtonGlass(glass) && badge.gradientBackgroundView.hidden,
        @"the series badge replaces its original gradient with a native glass button");
    HGAppearanceSet(@"player.material",@"blur",nil);[badge modelDidUpdate];
    UIVisualEffectView *blur=(id)Find(badge,@"honggo.appearance.material");Layout(blur);
    Check([blur isKindOfClass:UIVisualEffectView.class] && !blur.maskView &&
        (UIAccessibilityIsReduceTransparencyEnabled() ? !blur.effect : [blur.effect isKindOfClass:UIBlurEffect.class]),
        @"the series badge respects an explicit classic-blur preference");
    HGAppearanceSet(@"player.material",savedMaterial,nil);[badge modelDidUpdate];
    Check(!badge.hidden && [Find(badge,@"honggo.appearance.playerBadgeGlyph").tintColor isEqual:UIColor.whiteColor],
          @"the same reused control shows a series link independently with night-theme ink");
    HGAppearanceSet(@"player.hide.seriesBadge",@YES,nil);
    [badge modelDidUpdate];
    Check(badge.hidden,@"series visibility can be disabled separately");
    badge.dataModel.reasonText=@"大家都在看";
    [badge modelDidUpdate];
    Check(!badge.hidden && !badge.gradientBackgroundView.hidden && !badge.iconView.layer.mask &&
          !Find(badge,@"honggo.appearance.material") && [badge.backgroundColor isEqual:UIColor.blackColor],
          @"reusing either badge for an unrelated recommendation restores its native paint and icon");
    [stack removeFromSuperview];
    HGAppearanceSet(@"player.hide.followRank",@NO,nil);
    HGAppearanceSet(@"player.hide.seriesBadge",@NO,nil);
}

@interface SSShortVideoRightInteractionAvatarView : UIControl
@property(nonatomic,strong) UIView *avatarView;
@property(nonatomic,strong) UIView *followButtonBgView;
@end
@implementation SSShortVideoRightInteractionAvatarView @end
static void RunOffscreenAvatarChecks(UIView *root) {
    NSDictionary *saved=HGAppearanceValues();
    HGAppearanceApply(@{@"enabled":@YES,@"layout.avatar.hidden":@NO,@"layout.avatar.x":@0,
        @"layout.avatar.y":@0,@"layout.avatar.scale":@1},nil);
    CGRect slot=CGRectMake(CGRectGetMaxX(root.window.bounds)-58,372,46,338);
    SSShortVideoRightInteractionView *rail=[[SSShortVideoRightInteractionView alloc]
        initWithFrame:[root.window convertRect:slot toView:root]];
    SSShortVideoRightInteractionAvatarView *avatar=[[SSShortVideoRightInteractionAvatarView alloc]
        initWithFrame:CGRectMake(53,0,46,46)];
    UIView *star=[[UIView alloc] initWithFrame:CGRectMake(0,54,46,62)];
    [rail addSubview:star];[rail addSubview:avatar];[root addSubview:rail];
    Layout(rail);
    Check(avatar.hidden && !star.hidden && !rail.clipsToBounds,
        @"the captured off-rail avatar cannot leak its 5pt sliver over the video or clip its neighboring star");
    avatar.frame=CGRectMake(0,0,46,46);Layout(rail);
    Check(!avatar.hidden,@"reusing the same rail for an on-screen author restores the native avatar");
    HGAppearanceSet(@"layout.avatar.hidden",@YES,nil);Layout(rail);
    Check(avatar.hidden,@"the explicit avatar switch remains authoritative on a visible slot");
    avatar.avatarView=[[UIControl alloc] initWithFrame:CGRectMake(2,0,40,32)];
    avatar.followButtonBgView=[[UIControl alloc] initWithFrame:CGRectMake(13,32,20,14)];
    [avatar addSubview:avatar.avatarView]; [avatar addSubview:avatar.followButtonBgView];
    HGAppearanceSet(@"layout.avatar.hidden",@NO,nil);
    for(NSUInteger mask=0;mask<4;mask++) {
        BOOL hidePhoto=(mask&1)!=0,hideFollow=(mask&2)!=0;
        HGAppearanceApply(@{@"player.hideFeedAvatarPhoto":@(hidePhoto),@"player.hideFeedFollow":@(hideFollow)},nil);
        HGAppearanceProcessView(avatar);
        Check(avatar.hidden==(hidePhoto && hideFollow) && (avatar.avatarView.alpha==0)==hidePhoto &&
            (avatar.followButtonBgView.alpha==0)==hideFollow &&
            (hideFollow || [avatar hitTest:CGPointMake(23,39) withEvent:nil]==avatar.followButtonBgView),
            @"profile image and follow action independently hide, with the remaining control retaining its hit");
    }
    avatar.followButtonBgView.hidden=YES;
    HGAppearanceApply(@{@"player.hideFeedAvatarPhoto":@NO,@"player.hideFeedFollow":@NO},nil);
    HGAppearanceProcessView(avatar);
    Check(avatar.avatarView.alpha==1 && avatar.followButtonBgView.hidden,
        @"reusing the avatar while the host hides the follow button preserves the new host state");
    [rail removeFromSuperview];HGAppearanceApply(saved,nil);
}
@interface HGALazyPlayControlFixture : FQVShortVideoVPlayerControlView
@property(nonatomic) NSUInteger lazyPlayReads;
@property(nonatomic) NSUInteger lazyFullscreenReads;
- (UIButton *)mountLazyPlay;
@end
@implementation HGALazyPlayControlFixture
- (UIButton *)playBtn {
    self.lazyPlayReads++;
    if (![super playBtn]) {
        UIButton *btn=[UIButton buttonWithType:UIButtonTypeCustom];
        btn.frame=CGRectMake(40,6,48,48);
        [btn setImage:[UIImage systemImageNamed:@"play.fill"] forState:UIControlStateNormal];
        [super setPlayBtn:btn];
        [self addSubview:btn];
    }
    return [super playBtn];
}
- (UIButton *)fullscreenBtn {
    self.lazyFullscreenReads++;
    if (![super fullscreenBtn]) {
        UIButton *btn=[UIButton buttonWithType:UIButtonTypeCustom];
        btn.frame=CGRectMake(12,8,108,40);
        [super setFullscreenBtn:btn];
        [self addSubview:btn];
    }
    return [super fullscreenBtn];
}
- (UIButton *)mountLazyPlay { return [self playBtn]; }
@end

static void RunPlayerChromeChecks(SSTabBarController *host) {
    RunOffscreenAvatarChecks(host.view);
    RunDanmakuChromeChecks(host.view);
    NSDictionary *savedLazy=HGAppearanceValues();
    HGAppearanceApply(@{@"enabled":@YES,@"player.rightGlass":@YES,@"player.controlsGlass":@YES,@"player.actionStyle":@"glass"},nil);
    HGALazyPlayControlFixture *coldHud=[[HGALazyPlayControlFixture alloc] initWithFrame:CGRectMake(10,100,144,60)];
    [host.view addSubview:coldHud];
    HGAppearanceProcessView(coldHud);
    Check(coldHud.lazyPlayReads==0 && coldHud.lazyFullscreenReads==0 && coldHud.subviews.count==0,
        @"cold-launch HUD layout does not invoke lazy playBtn or fullscreenBtn getters while buffering");
    UIButton *mountedPlay=[coldHud mountLazyPlay];
    HGAppearanceProcessView(coldHud);
    Check(coldHud.lazyPlayReads==1 && Find(mountedPlay,@"honggo.appearance.material")!=nil,
        @"once the host mounts playBtn on pause, its liquid surface attaches without extra lazy reads");
    [coldHud removeFromSuperview];
    HGAppearanceApply(savedLazy,nil);
    NSDictionary *saved=HGAppearanceValues();
    HGAppearanceApply(@{@"enabled":@YES,@"player.controlsGlass":@YES,@"panels.episodes":@YES,@"panels.material":@"blur"},nil);
    RunDetailNavigationChromeChecks(host);
    RunPlayerBadgeChecks(host.view);
    HGAEpisodeTapFixture *action=[HGAEpisodeTapFixture new];
    FQVShortVideoVPlayerControlView *playback=[[FQVShortVideoVPlayerControlView alloc] initWithFrame:CGRectMake(10,100,144,60)];
    UIButton *fullscreen=[UIButton buttonWithType:UIButtonTypeCustom];
    fullscreen.frame=CGRectMake(12,8,108,40);
    UIColor *nativeFullscreenColor=UIColor.darkGrayColor;
    fullscreen.backgroundColor=nativeFullscreenColor;
    [fullscreen setTitle:@"全屏观看" forState:UIControlStateNormal];
    [fullscreen addTarget:action action:@selector(open:) forControlEvents:UIControlEventTouchUpInside];
    playback.fullscreenBtn=fullscreen;
    [playback addSubview:fullscreen];
    [host.view addSubview:playback];
    CGRect fullscreenFrame=fullscreen.frame;
    HGAppearanceProcessView(playback);
    UIVisualEffectView *fullscreenGlass=(id)Find(fullscreen,@"honggo.appearance.material");
    Check(fullscreenGlass && !fullscreenGlass.userInteractionEnabled && CGRectEqualToRect(fullscreenGlass.frame,fullscreen.bounds) &&
        CGRectEqualToRect(fullscreen.frame,fullscreenFrame) && [fullscreen.currentTitle isEqual:@"全屏观看"] &&
        [playback hitTest:CGPointMake(CGRectGetMidX(fullscreenFrame),CGRectGetMidY(fullscreenFrame)) withEvent:nil]==fullscreen,
        @"fullscreen glass belongs to the existing button and preserves hit target title and geometry");
    [fullscreen sendActionsForControlEvents:UIControlEventTouchUpInside];
    HGAppearanceSet(@"player.controlsGlass",@NO,nil);
    HGAppearanceProcessView(playback);
    [fullscreen sendActionsForControlEvents:UIControlEventTouchUpInside];
    Check(action.taps==2 && !Find(fullscreen,@"honggo.appearance.material") &&
        [fullscreen.backgroundColor isEqual:nativeFullscreenColor],
        @"fullscreen button forwards its native action once in both states and restores its backing");
    FQVShortVideoBaseLeftContainerView *metadata=[[FQVShortVideoBaseLeftContainerView alloc] initWithFrame:CGRectMake(0,0,220,60)];
    metadata.nameLabel=[UILabel new];metadata.titleLabel=[UILabel new];metadata.expandedLabel=[UILabel new];
    for(UILabel *label in @[metadata.nameLabel,metadata.titleLabel,metadata.expandedLabel])[metadata addSubview:label];
    [host.view addSubview:metadata];
    HGAppearanceApply(@{@"player.hideFeedAuthor":@YES,@"player.hideFeedDescription":@NO},nil);
    HGAppearanceProcessView(metadata);
    Check(metadata.nameLabel.hidden && !metadata.titleLabel.hidden && !metadata.expandedLabel.hidden &&
        CGRectIsEmpty([metadata convertSeriesNameFrameToView:playback]) && !CGRectIsEmpty([playback convertTitleClickableFrameToView:playback]),
        @"hiding the feed author leaves the episode description and expansion affordance available");
    HGAppearanceApply(@{@"player.hideFeedAuthor":@NO,@"player.hideFeedDescription":@YES},nil);
    HGAppearanceProcessView(metadata);
    Check(!metadata.nameLabel.hidden && metadata.titleLabel.hidden && metadata.expandedLabel.hidden &&
        !CGRectIsEmpty([metadata convertSeriesNameFrameToView:playback]) && CGRectIsEmpty([playback convertTitleClickableFrameToView:playback]),
        @"hiding the episode description restores the author and hides the associated expansion affordance");
    [metadata removeFromSuperview];
    CGPoint fullscreenCenter=[fullscreen convertPoint:CGPointMake(CGRectGetMidX(fullscreen.bounds),
        CGRectGetMidY(fullscreen.bounds)) toView:playback];
    HGAppearanceSet(@"player.fullscreenEntry",@"invisible",nil);
    HGAppearanceProcessView(playback);
    Check(!fullscreen.hidden && [fullscreen.layer animationForKey:@"honggo.fullscreenEntry.opacity"] &&
        [playback hitTest:fullscreenCenter withEvent:nil]==fullscreen,
        @"invisible fullscreen keeps the mounted native target at its existing coordinates");
    HGAppearanceSet(@"player.fullscreenEntry",@"hide",nil);
    HGAppearanceProcessView(playback);
    Check(fullscreen.hidden && [playback hitTest:fullscreenCenter withEvent:nil]!=fullscreen,
        @"fully hidden fullscreen removes the original target from UIKit hit testing");
    HGAppearanceSet(@"player.fullscreenEntry",@"show",nil);
    HGAppearanceProcessView(playback);
    Check(!fullscreen.hidden && ![fullscreen.layer animationForKey:@"honggo.fullscreenEntry.opacity"],
        @"show restores fullscreen without a lingering presentation cover");
    UIButton *play=[UIButton buttonWithType:UIButtonTypeCustom];
    play.frame=CGRectMake(8,4,48,48);
    [play setImage:[UIImage systemImageNamed:@"play.fill"] forState:UIControlStateNormal];
    [play setImage:[UIImage systemImageNamed:@"pause.fill"] forState:UIControlStateSelected];
    [play addTarget:action action:@selector(open:) forControlEvents:UIControlEventTouchUpInside];
    playback.playBtn=play;
    [playback addSubview:play];
    HGAppearanceApply(@{@"player.rightGlass":@YES,@"player.actionStyle":@"glass",@"player.material":@"blur"},nil);
    HGAppearanceProcessView(playback);
    UIView *playGlass=Find(play,@"honggo.appearance.material");
    Layout(playGlass);
    Check(playGlass && !playGlass.userInteractionEnabled && (!HGTestOpenGlass(play) ||
        HGTestButtonGlass(playGlass)),
        @"play receives its own liquid surface while other player controls use blur");
    HGAppearanceSet(@"player.actionStyle",@"flat",nil);
    play.selected=YES;
    [play sendActionsForControlEvents:UIControlEventTouchUpInside];
    UIVisualEffectView *flatPlay=(id)Find(play,@"honggo.appearance.material");
    Layout(flatPlay);
    Check(flatPlay && !flatPlay.effect && play.currentImage==[play imageForState:UIControlStateSelected] &&
        action.taps==3,@"flat play keeps the host's current playback glyph and its original action");
    [playback removeFromSuperview];

    HGAppearanceSet(@"player.controlsGlass",@YES,nil);
    HGAppearanceSet(@"player.material",@"blur",nil);
    SSShortVideoTagListBottomInsertView *insert=[[SSShortVideoTagListBottomInsertView alloc]
        initWithFrame:CGRectMake(12,160,360,40)];
    insert.contentView=[[UIView alloc] initWithFrame:CGRectMake(0,0,304,40)];
    insert.episodeInfoView=[UIButton buttonWithType:UIButtonTypeCustom];
    insert.episodeInfoView.frame=insert.contentView.bounds;
    [insert.episodeInfoView setTitle:@"选集 · 全114集 · 免费观看" forState:UIControlStateNormal];
    HGAEpisodeTapFixture *episodeAction=[HGAEpisodeTapFixture new];
    [insert.episodeInfoView addTarget:episodeAction action:@selector(open:) forControlEvents:UIControlEventTouchUpInside];
    [insert.contentView addSubview:insert.episodeInfoView];
    [insert addSubview:insert.contentView];
    insert.cleanSwitchView=[[FQVShortVideoControlCleanSwitchView alloc] initWithFrame:CGRectMake(303,-12,62,64)];
    insert.cleanScreenView=[[UIView alloc] initWithFrame:insert.cleanSwitchView.bounds];
    UIButton *cleanButton=[UIButton buttonWithType:UIButtonTypeCustom];
    cleanButton.frame=insert.cleanSwitchView.bounds;
    HGAEpisodeTapFixture *cleanAction=[HGAEpisodeTapFixture new];
    [cleanButton addTarget:cleanAction action:@selector(open:) forControlEvents:UIControlEventTouchUpInside];
    [insert.cleanSwitchView addSubview:cleanButton];
    [insert addSubview:insert.cleanSwitchView];
    UIColor *entryPaint=[UIColor colorWithWhite:1 alpha:0.10];
    [insert setVideoDataModel:entryPaint];
    [host.view addSubview:insert];
    [insert setVideoDataModel:entryPaint];
    UIView *cleanGlass=Find(insert.cleanSwitchView,@"honggo.appearance.material");
    UIView *episodeGlass=Find(insert.episodeInfoView,@"honggo.appearance.material");
    CGColorRef paint=insert.episodeInfoView.layer.backgroundColor;
    Check(cleanGlass.superview==insert.cleanSwitchView && !Find(insert.cleanScreenView,@"honggo.appearance.material") &&
        CGRectEqualToRect(cleanGlass.frame,CGRectMake(11,12,40,40)) &&
        [insert hitTest:CGPointMake(337,20) withEvent:nil]==cleanButton,
        @"detail clean switch caps oversized glass at the 40pt episode-row height and preserves the hit target");
    Check(episodeGlass.superview==insert.episodeInfoView && HGTestMaterialRadius(episodeGlass,20) &&
        (!paint || CGColorGetAlpha(paint)==0) && Count(insert,@"honggo.appearance.material")==2 &&
        [insert hitTest:CGPointMake(120,20) withEvent:nil]==insert.episodeInfoView,
        @"detail episode pill decorates the actual native entry and clears its layer paint after a model update");
    if(HGNativeGlassConfigured() && !UIAccessibilityIsReduceTransparencyEnabled()) {
        Layout(cleanGlass);Layout(episodeGlass);
        Check(HGTestButtonGlass(cleanGlass) && [episodeGlass isKindOfClass:UIVisualEffectView.class] &&
            [[(UIVisualEffectView *)episodeGlass effect] isKindOfClass:UIBlurEffect.class],
            @"the clean control uses native glass and the episode entry respects selected classic blur");
    }
    [insert.episodeInfoView sendActionsForControlEvents:UIControlEventTouchUpInside];
    [cleanButton sendActionsForControlEvents:UIControlEventTouchUpInside];
    HGAppearanceSet(@"player.controlsGlass",@NO,nil);
    HGAppearanceProcessView(insert);
    paint=insert.episodeInfoView.layer.backgroundColor;
    Check(cleanAction.taps==1 && episodeAction.taps==1 && !Find(insert,@"honggo.appearance.material") &&
        paint && CGColorEqualToColor(paint,entryPaint.CGColor),
        @"disabling detail bottom glass restores the native paint and preserves both actions");
    [insert removeFromSuperview];

    HGAppearanceSet(@"player.controlsGlass",@YES,nil);
    FQVShortVideoVPlayerEpisodeInfoView *nativeEntry=[[FQVShortVideoVPlayerEpisodeInfoView alloc]
        initWithFrame:CGRectMake(12,160,304,40)];
    nativeEntry.nativePaint=entryPaint;
    [nativeEntry refreshSubviews];
    FQVShortVideoPlayerBottomView *tray=[[FQVShortVideoPlayerBottomView alloc] initWithFrame:host.view.bounds];
    tray.alpha=0;
    FQVShortVideoControlCleanSwitchView *detailClean=[[FQVShortVideoControlCleanSwitchView alloc]
        initWithFrame:CGRectMake(320,150,64,64)];
    [tray addSubview:nativeEntry];[tray addSubview:detailClean];[host.view addSubview:tray];
    [nativeEntry refreshSubviews];
    Layout(tray);
    UIView *detailDisc=Find(detailClean,@"honggo.appearance.material");
    Check(Count(tray,@"honggo.appearance.material")==2 && CGRectEqualToRect(detailDisc.frame,CGRectMake(12,12,40,40)),
        @"the detail tray prepares both glass controls before its first alpha-only reveal");
    tray.alpha=1;
    UIView *nativeGlass=Find(nativeEntry,@"honggo.appearance.material");
    paint=nativeEntry.layer.backgroundColor;
    Check(HGTestMaterialRadius(nativeGlass,20) && (!paint || CGColorGetAlpha(paint)==0),
        @"the native episode entry gets rounded glass after refreshing its layer color");
    HGAppearanceSet(@"player.controlsGlass",@NO,nil);
    paint=nativeEntry.layer.backgroundColor;
    Check(!Find(nativeEntry,@"honggo.appearance.material") && paint && CGColorEqualToColor(paint,entryPaint.CGColor),
        @"the native episode entry restores its own background when glass is disabled");
    [tray removeFromSuperview];

    SSShortVideoIntroductionTabViewController *panel=[SSShortVideoIntroductionTabViewController new];
    [host addChildViewController:panel];
    FQVTransitionView *transition=[[FQVTransitionView alloc] initWithFrame:CGRectMake(0,0,440,956)];
    FQVTransitionPlayerTopContentView *preview=[[FQVTransitionPlayerTopContentView alloc] initWithFrame:CGRectMake(0,0,440,383)];
    transition.topContentView=preview;
    CGRect nativeFrame=CGRectMake(0,98.75,440,247.5);
    preview.playerContainerView=[[UIView alloc] initWithFrame:nativeFrame];
    [preview addSubview:preview.playerContainerView];[transition addSubview:preview];
    [host.view addSubview:transition];
    panel.view.frame=CGRectMake(0,383,440,573);
    panel.view.overrideUserInterfaceStyle=UIUserInterfaceStyleLight;
    panel.view.backgroundColor=UIColor.whiteColor;
    [transition addSubview:panel.view];
    [panel didMoveToParentViewController:host];

    SSShortVideoEpisodesViewItemCell *episode=[[SSShortVideoEpisodesViewItemCell alloc] initWithFrame:CGRectMake(12,36,46,44)];
    episode.bgView=[[UIView alloc] initWithFrame:episode.contentView.bounds];
    episode.bgView.backgroundColor=UIColor.systemOrangeColor;
    [episode.contentView addSubview:episode.bgView];
    [panel.view addSubview:episode];

    SSShortVideoEpisodePanelCollectView *collect=[[SSShortVideoEpisodePanelCollectView alloc] initWithFrame:CGRectMake(12,100,200,45)];
    collect.containerView=[UIButton buttonWithType:UIButtonTypeCustom];
    collect.containerView.frame=CGRectMake(0,0,126,45);
    [collect.containerView addTarget:action action:@selector(open:) forControlEvents:UIControlEventTouchUpInside];
    collect.bgView=[[UIView alloc] initWithFrame:collect.containerView.bounds];
    CAGradientLayer *gradient=[CAGradientLayer layer];
    gradient.frame=collect.bgView.bounds;
    gradient.colors=@[(id)UIColor.systemYellowColor.CGColor,(id)UIColor.systemOrangeColor.CGColor];
    [collect.bgView.layer addSublayer:gradient];
    collect.textL=[[UILabel alloc] initWithFrame:CGRectMake(40,0,64,45)];
    collect.textL.text=@"收藏";
    collect.textL.textColor=UIColor.whiteColor;
    UIImage *initialIcon=[[UIImage systemImageNamed:@"star.fill"] imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
    collect.iconImageView=[[UIImageView alloc] initWithImage:initialIcon];
    collect.iconImageView.frame=CGRectMake(10,10,25,25);
    collect.iconImageView.tintColor=UIColor.whiteColor;
    [collect.containerView addSubview:collect.bgView];
    [collect.containerView addSubview:collect.textL];
    [collect.containerView addSubview:collect.iconImageView];
    [collect addSubview:collect.containerView];
    collect.shareContainerView=[[UIView alloc] initWithFrame:CGRectMake(138,0,44,45)];
    collect.shareBgView=[[UIView alloc] initWithFrame:collect.shareContainerView.bounds];
    collect.shareBgView.hidden=YES;
    [collect.shareContainerView addSubview:collect.shareBgView];
    [collect addSubview:collect.shareContainerView];
    [panel.view addSubview:collect];

    HGAppearanceProcessController(panel);
    SSShortVideoEpisodeSubViewController *sub=[SSShortVideoEpisodeSubViewController new];
    [panel addChildViewController:sub];
    sub.view.frame=CGRectMake(0,148,240,120);
    sub.view.backgroundColor=UIColor.whiteColor;
    sub.collectionView=[[UICollectionView alloc] initWithFrame:sub.view.bounds collectionViewLayout:[UICollectionViewFlowLayout new]];
    sub.nativeListBackground=sub.collectionView.backgroundColor ?: UIColor.systemBackgroundColor;
    sub.collectionView.backgroundView=[[UIView alloc] initWithFrame:sub.view.bounds];
    sub.collectionView.backgroundView.backgroundColor=UIColor.whiteColor;
    [sub.view addSubview:sub.collectionView];[panel.view addSubview:sub.view];[sub didMoveToParentViewController:panel];
    [sub onDarkModeChange:NO];
    Check([sub.collectionView.backgroundColor isEqual:UIColor.clearColor] &&
        [sub.collectionView.backgroundView.backgroundColor isEqual:UIColor.clearColor] &&
        [sub.view.backgroundColor isEqual:UIColor.clearColor] && !Find(sub.view,@"honggo.appearance.material"),
        @"the late-mounted episode page drops white content and background-view paint under the single card");
    UIView *cellGlass=Find(episode.bgView,@"honggo.appearance.material");
    UIView *collectGlass=Find(collect.containerView,@"honggo.appearance.material");
    Check(Find(panel.view,@"honggo.appearance.material").superview==panel.view &&
        cellGlass.superview==episode.bgView && collectGlass.superview==collect.containerView &&
        Find(collect.shareContainerView,@"honggo.appearance.material")!=nil,
        @"episode sheet decorates the panel, cell backgrounds and both action surfaces");
    UITraitCollection *traits=collect.textL.traitCollection;
    UIColor *ink=[collect.textL.textColor resolvedColorWithTraitCollection:traits];
    CGFloat red=1,green=1,blue=1,alpha=0;
    Check(collect.bgView.hidden && !collect.containerView.hidden &&
        [ink getRed:&red green:&green blue:&blue alpha:&alpha] && MAX(red,MAX(green,blue))<0.15 && alpha>0.9 &&
        [collect.iconImageView.tintColor isEqual:UIColor.labelColor] &&
        collect.iconImageView.image.renderingMode==UIImageRenderingModeAlwaysTemplate,
        @"light episode sheet hides the native gradient and uses readable label and icon foreground");
    NSUInteger tapsBeforeCollect=action.taps;
    [collect.containerView sendActionsForControlEvents:UIControlEventTouchUpInside];
    Check(action.taps==tapsBeforeCollect+1,@"the episode sheet's collect control dispatches one native action");
    UILabel *previousLabel=collect.textL;
    UIImageView *previousIcon=collect.iconImageView;
    [previousLabel removeFromSuperview];
    [previousIcon removeFromSuperview];
    collect.textL=[[UILabel alloc] initWithFrame:previousLabel.frame];
    collect.textL.text=@"已收藏";
    collect.textL.textColor=UIColor.labelColor;
    UIImage *newIcon=[[UIImage systemImageNamed:@"star.circle.fill"] imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
    collect.iconImageView=[[UIImageView alloc] initWithImage:newIcon];
    collect.iconImageView.frame=previousIcon.frame;
    collect.iconImageView.tintColor=UIColor.labelColor;
    [collect.containerView addSubview:collect.textL];
    [collect.containerView addSubview:collect.iconImageView];
    HGAppearanceProcessView(collect);
    Check([previousLabel.textColor isEqual:UIColor.whiteColor] && [previousIcon.tintColor isEqual:UIColor.whiteColor] &&
        ImageContentEqual(previousIcon.image,initialIcon) && collect.iconImageView.image.renderingMode==UIImageRenderingModeAlwaysTemplate,
        @"replacing the host collect label and image restores old views and styles the new ones");

    HGAppearanceProcessView(preview);
    Check(fabs(preview.layer.sublayerTransform.m42-36.75)<0.01 &&
        CGRectEqualToRect(preview.playerContainerView.frame,nativeFrame),
        @"episode preview closes the measured gap without cropping or changing the borrowed player geometry");

    UICollectionReusableView *section=[[UICollectionReusableView alloc] initWithFrame:CGRectMake(0,0,240,46)];
    FQVShortVideoEpisodesNumberView *ranges=[[FQVShortVideoEpisodesNumberView alloc] initWithFrame:section.bounds];
    ranges.backgroundColor=UIColor.whiteColor;section.backgroundColor=UIColor.whiteColor;
    ranges.collectionView=[[UICollectionView alloc] initWithFrame:ranges.bounds collectionViewLayout:[UICollectionViewFlowLayout new]];
    ranges.collectionView.backgroundColor=UIColor.whiteColor;[ranges addSubview:ranges.collectionView];
    ranges.leftGradientView=[UIView new];ranges.rightGradientView=[UIView new];
    [ranges addSubview:ranges.leftGradientView];[ranges addSubview:ranges.rightGradientView];
    [section addSubview:ranges];[sub.collectionView addSubview:section];
    HGAppearanceProcessView(ranges);
    Check([section.backgroundColor isEqual:UIColor.clearColor] && [ranges.backgroundColor isEqual:UIColor.clearColor] &&
        [ranges.collectionView.backgroundColor isEqual:UIColor.clearColor] &&
        ranges.leftGradientView.hidden && ranges.rightGradientView.hidden && ranges.collectionView.userInteractionEnabled,
        @"a late-mounted episode range header retains its controls and removes both white edge fades");
    HGAppearanceSet(@"panels.episodes",@NO,nil);
    HGAppearanceProcessController(panel);
    Check([sub.view.backgroundColor isEqual:UIColor.whiteColor],
        [NSString stringWithFormat:@"disabling episode glass restores the page background (%@)",sub.view.backgroundColor]);
    Check([sub.collectionView.backgroundColor isEqual:sub.nativeListBackground],
        [NSString stringWithFormat:@"disabling episode glass restores the list background (%@)",sub.collectionView.backgroundColor]);
    Check([sub.collectionView.backgroundView.backgroundColor isEqual:UIColor.whiteColor],
        [NSString stringWithFormat:@"disabling episode glass restores the list background view (%@)",sub.collectionView.backgroundView.backgroundColor]);
    [sub onDarkModeChange:YES];
    Check([sub.collectionView.backgroundColor isEqual:UIColor.blackColor],
        @"after glass is disabled the host can repaint the episode list for night mode");
    Check(CATransform3DIsIdentity(preview.layer.sublayerTransform) &&
        [ranges.backgroundColor isEqual:UIColor.whiteColor] && !ranges.leftGradientView.hidden &&
        !ranges.rightGradientView.hidden,@"turning off episode glass restores the preview and range header");
    Check(action.taps==tapsBeforeCollect+1 && !Find(panel.view,@"honggo.appearance.material") && !collect.bgView.hidden &&
        collect.shareBgView.hidden && [episode.bgView.backgroundColor isEqual:UIColor.systemOrangeColor] &&
        [collect.textL.textColor isEqual:UIColor.labelColor] &&
        [collect.iconImageView.tintColor isEqual:UIColor.labelColor] &&
        ImageContentEqual(collect.iconImageView.image,newIcon),
        @"disabling episode glass restores native backgrounds gradient visibility and original foreground");
    HGAppearanceSet(@"panels.episodes",@YES,nil);
    [sub onDarkModeChange:YES];
    Check([sub.collectionView.backgroundColor isEqual:UIColor.clearColor],
        @"episode glass clears an explicit host night-mode repaint");
    HGAppearanceSet(@"panels.episodes",@NO,nil);
    Check([sub.collectionView.backgroundColor isEqual:UIColor.blackColor],
        [NSString stringWithFormat:@"disabling episode glass restores the explicit host night color (%@)",sub.collectionView.backgroundColor]);
    [panel willMoveToParentViewController:nil];
    [panel.view removeFromSuperview];
    [panel removeFromParentViewController];
    [transition removeFromSuperview];
    HGAppearanceApply(saved,nil);
}
