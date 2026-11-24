@interface FQVShortVideoVPlayerControlView : UIView <UIGestureRecognizerDelegate>
@property(nonatomic,strong) UIButton *fullscreenBtn;
@property(nonatomic,strong) UIButton *playBtn;
@property(nonatomic,strong,readonly) UIPanGestureRecognizer *progressPanGesture;
@property(nonatomic) NSUInteger nativeClipPasses;
- (void)handleOutterFeedClipBounds:(BOOL)clip;
@end
@implementation FQVShortVideoVPlayerControlView
@synthesize progressPanGesture=_progressPanGesture;
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
@property(nonatomic,strong) UIImageView *iconView;
@property(nonatomic,strong) UIImage *legacyImage;
- (void)refreshIcon;
@end
@implementation FQCShortVideoDanmakuEntranceView
- (void)refreshIcon {self.iconView.image=self.legacyImage;}
@end

static void RunDanmakuChromeChecks(UIView *root) {
    NSDictionary *saved=HGAppearanceValues();
    HGAppearanceApply(@{@"enabled":@YES,@"player.controlsGlass":@YES},nil);

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
    Check(!Find(entrance.capsuleView,@"honggo.appearance.material") &&
        Find(entrance.clickView,@"honggo.appearance.material").superview==entrance.clickView &&
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
    Check(Find(entrance.capsuleView,@"honggo.appearance.material") &&
        !Find(entrance.clickView,@"honggo.appearance.material") && entrance.iconView.image==combined,
        @"combined danmaku moves glass to the capsule and keeps its native state image");

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

static void RunPlayerChromeChecks(SSTabBarController *host) {
    RunDanmakuChromeChecks(host.view);
    NSDictionary *saved=HGAppearanceValues();
    HGAppearanceApply(@{@"enabled":@YES,@"player.controlsGlass":@YES,@"panels.episodes":@YES,@"panels.material":@"blur"},nil);
    RunDetailNavigationChromeChecks(host);
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
    UIButton *play=[UIButton buttonWithType:UIButtonTypeCustom];
    play.frame=CGRectMake(8,4,48,48);
    [play setImage:[UIImage systemImageNamed:@"play.fill"] forState:UIControlStateNormal];
    [play setImage:[UIImage systemImageNamed:@"pause.fill"] forState:UIControlStateSelected];
    [play addTarget:action action:@selector(open:) forControlEvents:UIControlEventTouchUpInside];
    playback.playBtn=play;
    [playback addSubview:play];
    HGAppearanceApply(@{@"player.rightGlass":@YES,@"player.actionStyle":@"glass",@"player.material":@"blur"},nil);
    HGAppearanceProcessView(playback);
    UIVisualEffectView *playGlass=(id)Find(play,@"honggo.appearance.material");
    Layout(playGlass);
    Check(playGlass && !playGlass.userInteractionEnabled && (!HGNativeGlassConfigured() ||
        UIAccessibilityIsReduceTransparencyEnabled() || [playGlass.effect isKindOfClass:NSClassFromString(@"UIGlassEffect")]),
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
    UIVisualEffectView *cleanGlass=(id)Find(insert.cleanSwitchView,@"honggo.appearance.material");
    UIVisualEffectView *episodeGlass=(id)Find(insert.episodeInfoView,@"honggo.appearance.material");
    CGColorRef paint=insert.episodeInfoView.layer.backgroundColor;
    Check(cleanGlass.superview==insert.cleanSwitchView && !Find(insert.cleanScreenView,@"honggo.appearance.material") &&
        CGRectEqualToRect(cleanGlass.frame,CGRectMake(11,12,40,40)) &&
        [insert hitTest:CGPointMake(337,20) withEvent:nil]==cleanButton,
        @"detail clean switch caps oversized glass at the 40pt episode-row height and preserves the hit target");
    Check(episodeGlass.superview==insert.episodeInfoView && episodeGlass.layer.cornerRadius==20 &&
        (!paint || CGColorGetAlpha(paint)==0) && Count(insert,@"honggo.appearance.material")==2 &&
        [insert hitTest:CGPointMake(120,20) withEvent:nil]==insert.episodeInfoView,
        @"detail episode pill decorates the actual native entry and clears its layer paint after a model update");
    if(HGNativeGlassConfigured() && !UIAccessibilityIsReduceTransparencyEnabled()) {
        Layout(cleanGlass);Layout(episodeGlass);
        Class glassClass=NSClassFromString(@"UIGlassEffect");
        Check([cleanGlass.effect isKindOfClass:glassClass] && [episodeGlass.effect isKindOfClass:glassClass],
            @"detail episode and clean controls use native liquid glass when available");
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
    [host.view addSubview:nativeEntry];
    [nativeEntry refreshSubviews];
    UIView *nativeGlass=Find(nativeEntry,@"honggo.appearance.material");
    paint=nativeEntry.layer.backgroundColor;
    Check(nativeGlass && nativeGlass.layer.cornerRadius==20 && (!paint || CGColorGetAlpha(paint)==0),
        @"the native episode entry gets rounded glass after refreshing its layer color");
    HGAppearanceSet(@"player.controlsGlass",@NO,nil);
    paint=nativeEntry.layer.backgroundColor;
    Check(!Find(nativeEntry,@"honggo.appearance.material") && paint && CGColorEqualToColor(paint,entryPaint.CGColor),
        @"the native episode entry restores its own background when glass is disabled");
    [nativeEntry removeFromSuperview];

    SSShortVideoIntroductionTabViewController *panel=[SSShortVideoIntroductionTabViewController new];
    [host addChildViewController:panel];
    panel.view.frame=CGRectMake(12,256,240,300);
    panel.view.overrideUserInterfaceStyle=UIUserInterfaceStyleLight;
    panel.view.backgroundColor=UIColor.whiteColor;
    [host.view addSubview:panel.view];
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
    [collect.containerView sendActionsForControlEvents:UIControlEventTouchUpInside];
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
    HGAppearanceSet(@"panels.episodes",@NO,nil);
    HGAppearanceProcessController(panel);
    Check(action.taps==3 && !Find(panel.view,@"honggo.appearance.material") && !collect.bgView.hidden &&
        collect.shareBgView.hidden && [episode.bgView.backgroundColor isEqual:UIColor.systemOrangeColor] &&
        [collect.textL.textColor isEqual:UIColor.labelColor] &&
        [collect.iconImageView.tintColor isEqual:UIColor.labelColor] &&
        ImageContentEqual(collect.iconImageView.image,newIcon),
        @"disabling episode glass restores native backgrounds gradient visibility and original foreground");
    [panel willMoveToParentViewController:nil];
    [panel.view removeFromSuperview];
    [panel removeFromParentViewController];
    HGAppearanceApply(saved,nil);
}
