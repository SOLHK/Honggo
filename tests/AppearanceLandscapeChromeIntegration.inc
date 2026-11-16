@interface FQVShortVideoPlayerLandscapeBaseInfoView : UIView
@property(nonatomic,strong) UIImageView *navBackImageView;
@property(nonatomic,strong) UILabel *titleL;
@property(nonatomic,strong) UIView *moreBtn;
@property(nonatomic,strong) UIView *nextBtn;
- (void)setVideoDataModel:(NSString *)title;
@end
@implementation FQVShortVideoPlayerLandscapeBaseInfoView
- (void)setVideoDataModel:(NSString *)title { self.titleL.text=title; }
@end

@interface FQVShortVideoPlayerLandscapeBaseControlView : UIView
@property(nonatomic,strong) UIView *screenLockBtn;
- (void)updateScreenLockButtonStyle;
@end
@implementation FQVShortVideoPlayerLandscapeBaseControlView
- (void)updateScreenLockButtonStyle { self.screenLockBtn.hidden=YES; }
@end

@interface SSShortVideoShareLandscapeNavBarItem : NSObject
@property(nonatomic,strong) UIView *view;
@end
@implementation SSShortVideoShareLandscapeNavBarItem @end

@interface FQVShortVideoLandscapeNavBar : UIView
@property(nonatomic,copy) NSArray *itemList;
@end
@implementation FQVShortVideoLandscapeNavBar @end

@interface FQVLandscapeVolumeButton : UIControl @end
@implementation FQVLandscapeVolumeButton @end

@interface FQVShortVideoLandscapeBottomRightView : UIStackView
@property(nonatomic,strong) UIView *playRateBtn;
@property(nonatomic,strong) UIView *resolutionBtn;
@property(nonatomic,strong) UIView *seriesBtn;
@end
@implementation FQVShortVideoLandscapeBottomRightView @end

@interface SSShortVideoLandscapeBizInteractionView : UIView
@property(nonatomic,strong) UIView *likeBtnView;
@property(nonatomic,strong) UIView *commentBtnView;
@property(nonatomic,strong) UIView *starBtnView;
@property(nonatomic,strong) UIImageView *danmakuBtnView;
@property(nonatomic,strong) UIView *danmakuInputBGView;
@end
@implementation SSShortVideoLandscapeBizInteractionView @end

static UIButton *HGALandscapeFixtureButton(UIView *parent,CGRect frame,NSString *title) {
    UIButton *button=[UIButton buttonWithType:UIButtonTypeCustom];
    button.frame=frame;button.backgroundColor=UIColor.darkGrayColor;
    [button setTitle:title forState:UIControlStateNormal];
    [parent addSubview:button];
    return button;
}

static void RunLandscapeChromeChecks(SSTabBarController *host) {
    NSDictionary *saved=HGAppearanceValues();
    HGAppearanceApply(@{@"enabled":@YES,@"player.controlsGlass":@YES,@"player.material":@"blur"},nil);
    UIWindow *window=HGPlaybackFixtureWindow(host.view,CGSizeMake(820,360));
    UIView *page=window.rootViewController.view;
    NSString *identifier=@"honggo.appearance.landscape.chrome";

    FQVShortVideoPlayerLandscapeBaseInfoView *info=[[FQVShortVideoPlayerLandscapeBaseInfoView alloc] initWithFrame:page.bounds];
    [page addSubview:info];
    UIView *top=[[UIView alloc] initWithFrame:CGRectMake(0,6,820,52)];
    [info addSubview:top];
    UIImage *arrow=[UIImage systemImageNamed:@"chevron.left"];
    info.navBackImageView=[[UIImageView alloc] initWithImage:arrow];
    info.navBackImageView.frame=CGRectMake(24,14,24,24);
    [top addSubview:info.navBackImageView];
    info.titleL=[[UILabel alloc] initWithFrame:CGRectMake(56,13,425,26)];
    info.titleL.text=@"谁让这个文盲修仙的：第3集完整标题";
    [top addSubview:info.titleL];
    info.moreBtn=HGALandscapeFixtureButton(top,CGRectMake(742,10,52,32),@"更多");
    UIButton *next=HGALandscapeFixtureButton(info,CGRectMake(688,191,86,32),@"下一集");
    info.nextBtn=next;
    HGAEpisodeTapFixture *action=[HGAEpisodeTapFixture new];
    [next addTarget:action action:@selector(open:) forControlEvents:UIControlEventTouchUpInside];

    FQVShortVideoLandscapeNavBar *navigation=[[FQVShortVideoLandscapeNavBar alloc] initWithFrame:CGRectMake(652,6,82,52)];
    [page addSubview:navigation];
    SSShortVideoShareLandscapeNavBarItem *shareItem=[SSShortVideoShareLandscapeNavBarItem new];
    shareItem.view=HGALandscapeFixtureButton(navigation,CGRectMake(13,10,56,32),@"分享");
    navigation.itemList=@[shareItem];

    FQVShortVideoPlayerLandscapeBaseControlView *controls=[[FQVShortVideoPlayerLandscapeBaseControlView alloc] initWithFrame:CGRectMake(15,139,55,100)];
    [page addSubview:controls];
    controls.screenLockBtn=HGALandscapeFixtureButton(controls,CGRectMake(7,5,30,30),@"锁");
    controls.screenLockBtn.clipsToBounds=YES;
    FQVLandscapeVolumeButton *volume=[[FQVLandscapeVolumeButton alloc] initWithFrame:CGRectMake(22,193,30,30)];
    volume.clipsToBounds=YES;
    [page addSubview:volume];

    SSShortVideoLandscapeBizInteractionView *interactions=[[SSShortVideoLandscapeBizInteractionView alloc] initWithFrame:CGRectMake(34,308,514,44)];
    [page addSubview:interactions];
    interactions.likeBtnView=HGALandscapeFixtureButton(interactions,CGRectMake(0,6,57,32),@"点赞");
    interactions.commentBtnView=HGALandscapeFixtureButton(interactions,CGRectMake(67,6,57,32),@"评论");
    interactions.starBtnView=HGALandscapeFixtureButton(interactions,CGRectMake(134,6,57,32),@"收藏");
    UIImage *danmaku=[UIImage systemImageNamed:@"text.bubble"];
    interactions.danmakuBtnView=[[UIImageView alloc] initWithImage:danmaku];
    interactions.danmakuBtnView.frame=CGRectMake(211,7,30,30);
    [interactions addSubview:interactions.danmakuBtnView];
    interactions.danmakuInputBGView=HGALandscapeFixtureButton(interactions,CGRectMake(261,6,200,32),@"发送弹幕");

    FQVShortVideoLandscapeBottomRightView *bottom=[[FQVShortVideoLandscapeBottomRightView alloc] initWithFrame:CGRectMake(558,308,228,44)];
    bottom.spacing=8;bottom.distribution=UIStackViewDistributionFillEqually;
    bottom.playRateBtn=HGALandscapeFixtureButton(bottom,CGRectZero,@"倍速");
    bottom.resolutionBtn=HGALandscapeFixtureButton(bottom,CGRectZero,@"清晰度");
    bottom.seriesBtn=HGALandscapeFixtureButton(bottom,CGRectZero,@"选集");
    for(UIView *item in @[bottom.playRateBtn,bottom.resolutionBtn,bottom.seriesBtn]) [bottom addArrangedSubview:item];
    [page addSubview:bottom];
    UISlider *progress=[[UISlider alloc] initWithFrame:CGRectMake(43,267,734,34)];
    [page addSubview:progress];

    CGRect nextFrame=next.frame,progressFrame=progress.frame;
    for(UIView *root in @[info,navigation,controls,volume,interactions,bottom]) {
        Layout(root);
        HGAppearanceProcessView(root);
    }
    UIView *backGlass=Find(top,identifier);
    CGRect titleRect=[info.titleL textRectForBounds:info.titleL.bounds limitedToNumberOfLines:info.titleL.numberOfLines];
    titleRect=[info.titleL convertRect:titleRect toView:top];
    Check(Count(page,identifier)==14 && Count(bottom,identifier)==3 && Count(interactions,identifier)==5 &&
        Count(navigation,identifier)==1 && Count(progress,identifier)==0,
        @"landscape glass covers the original navigation, side and business controls without styling the scrubber");
    Check(backGlass.superview==top && !backGlass.userInteractionEnabled && CGRectGetHeight(backGlass.bounds)==40 &&
        CGRectContainsRect(backGlass.frame,titleRect) &&
        [top.subviews indexOfObjectIdenticalTo:backGlass]<[top.subviews indexOfObjectIdenticalTo:info.navBackImageView] &&
        info.navBackImageView.image==arrow && interactions.danmakuBtnView.image==danmaku,
        @"landscape title and arrow share a centered 40-point backing behind the unmodified native foreground");
    CGFloat previousTitleWidth=CGRectGetWidth(backGlass.frame);
    [info setVideoDataModel:@"谁让这个文盲修仙的：第十二集更长的完整标题"];
    Check(Find(top,identifier)==backGlass && CGRectGetWidth(backGlass.frame)>previousTitleWidth,
        @"landscape title glass updates from the host's episode model without a second surface");
    Check(CGRectEqualToRect(next.frame,nextFrame) && CGRectEqualToRect(progress.frame,progressFrame) &&
        [info hitTest:CGPointMake(CGRectGetMidX(nextFrame),CGRectGetMidY(nextFrame)) withEvent:nil]==next,
        @"landscape glass preserves the original next-episode hit target and scrubber geometry");
    [next sendActionsForControlEvents:UIControlEventTouchUpInside];

    [controls updateScreenLockButtonStyle];
    volume.hidden=YES;
    Check(!Find(controls,identifier) && controls.screenLockBtn.clipsToBounds &&
        [controls.screenLockBtn.backgroundColor isEqual:UIColor.darkGrayColor] &&
        Find(volume,identifier).superview==volume,
        @"landscape lock and volume glass follow native visibility and restore the lock backing");
    HGAppearanceSet(@"player.controlsGlass",@NO,nil);
    for(UIView *root in @[info,navigation,controls,volume,interactions,bottom]) HGAppearanceProcessView(root);
    Check(!Find(page,identifier) && [next.backgroundColor isEqual:UIColor.darkGrayColor] && action.taps==1 &&
        bottom.arrangedSubviews.count==3 && volume.clipsToBounds,
        @"turning off landscape glass restores original paint and retains the native stack and action");
    window.hidden=YES;
    HGAppearanceApply(saved,nil);
}
