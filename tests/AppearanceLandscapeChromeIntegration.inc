@interface FQVShortVideoPlayerLandscapeBaseInfoView : UIView
@property(nonatomic,strong) UIImageView *navBackImageView;
@property(nonatomic,strong) UILabel *titleL;
@property(nonatomic,strong) UIView *moreBtn;
@property(nonatomic,strong) UIView *nextBtn;
@property(nonatomic,strong) UIView *navBar;
@property(nonatomic,strong) UIView *bottomLeftView;
@property(nonatomic,strong) UIView *bottomRightView;
- (void)setVideoDataModel:(NSString *)title;
@end
@implementation FQVShortVideoPlayerLandscapeBaseInfoView
- (void)setVideoDataModel:(NSString *)title { self.titleL.text=title; }
@end

@interface FQVShortVideoPlayerLandscapeBaseControlView : UIView
@property(nonatomic,strong) UIView *screenLockBtn;
@property(nonatomic,strong) UIView *infoView;
@property(nonatomic,weak) UIView *fixtureVolume;
@property(nonatomic,weak) UIView *fixtureFooter;
- (void)updateScreenLockButtonStyle;
- (void)showControlWithoutScreenLock;
- (void)showControlWithoutAnimation;
@end
@implementation FQVShortVideoPlayerLandscapeBaseControlView
- (void)updateScreenLockButtonStyle { self.screenLockBtn.hidden=YES; }
- (void)showControlWithoutScreenLock { self.fixtureVolume.alpha=1; }
- (void)showControlWithoutAnimation { self.fixtureVolume.alpha=1; self.fixtureFooter.alpha=1; }
@end

@interface SSShortVideoShareLandscapeNavBarItem : NSObject
@property(nonatomic,strong) UIImageView *shareIconV;
@property(nonatomic) BOOL isShow;
@end
@implementation SSShortVideoShareLandscapeNavBarItem @end

@interface FQVShortVideoLandscapeNavBar : UIView
@property(nonatomic,copy) NSArray *itemList;
@end
@implementation FQVShortVideoLandscapeNavBar @end

@interface FQVLandscapeVolumeButton : UIControl @end
@implementation FQVLandscapeVolumeButton @end

@interface FQVTouchIgoringView : UIView @end
@implementation FQVTouchIgoringView @end

@interface SSShortVideoLandscapeAutoInflowTipView : UIControl @end
@implementation SSShortVideoLandscapeAutoInflowTipView @end

@interface FQVShortVideoLandscapeBottomRightView : UIView
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
@property(nonatomic,strong) UIView *danmakuInputView;
@property(nonatomic,strong) UIView *danmakuInputBGView;
@property(nonatomic,strong) UILabel *likeCountLabel;
@property(nonatomic) BOOL fixtureSwitchHidden;
- (void)refreshUI;
@end
@implementation SSShortVideoLandscapeBizInteractionView
- (void)refreshUI { self.danmakuBtnView.hidden=self.fixtureSwitchHidden; }
@end

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

    FQVShortVideoPlayerLandscapeBaseControlView *controls=[[FQVShortVideoPlayerLandscapeBaseControlView alloc] initWithFrame:page.bounds];
    [page addSubview:controls];
    FQVShortVideoPlayerLandscapeBaseInfoView *info=[[FQVShortVideoPlayerLandscapeBaseInfoView alloc] initWithFrame:page.bounds];
    controls.infoView=info;
    [controls addSubview:info];
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
    info.navBar=navigation;
    [top addSubview:navigation];
    SSShortVideoShareLandscapeNavBarItem *shareItem=[SSShortVideoShareLandscapeNavBarItem new];
    shareItem.isShow=YES;
    UIImage *shareImage=[UIImage systemImageNamed:@"arrowshape.turn.up.right"];
    shareItem.shareIconV=[[UIImageView alloc] initWithImage:shareImage];
    shareItem.shareIconV.frame=CGRectMake(13,10,24,24);
    [navigation addSubview:shareItem.shareIconV];
    navigation.itemList=@[shareItem];

    controls.screenLockBtn=HGALandscapeFixtureButton(controls,CGRectMake(22,145,30,30),@"锁");
    controls.screenLockBtn.clipsToBounds=YES;
    FQVLandscapeVolumeButton *volume=[[FQVLandscapeVolumeButton alloc] initWithFrame:CGRectMake(22,193,30,30)];
    volume.clipsToBounds=YES;
    volume.opaque=YES;
    volume.backgroundColor=UIColor.darkGrayColor;
    volume.alpha=0;
    controls.fixtureVolume=volume;
    [controls addSubview:volume];
    SSShortVideoLandscapeAutoInflowTipView *inflow=[[SSShortVideoLandscapeAutoInflowTipView alloc] initWithFrame:CGRectMake(22,220,123,40)];
    inflow.clipsToBounds=YES;
    inflow.backgroundColor=UIColor.darkGrayColor;
    [inflow addTarget:action action:@selector(open:) forControlEvents:UIControlEventTouchUpInside];
    FQVTouchIgoringView *overlay=[[FQVTouchIgoringView alloc] initWithFrame:controls.bounds];
    [overlay addSubview:inflow];
    [controls addSubview:overlay];

    UIView *bottomGroup=[[UIView alloc] initWithFrame:info.bounds];
    bottomGroup.alpha=0;
    controls.fixtureFooter=bottomGroup;
    [info insertSubview:bottomGroup belowSubview:top];
    SSShortVideoLandscapeBizInteractionView *interactions=[[SSShortVideoLandscapeBizInteractionView alloc] initWithFrame:CGRectMake(34,308,514,44)];
    info.bottomLeftView=interactions;
    [bottomGroup addSubview:interactions];
    interactions.likeBtnView=HGALandscapeFixtureButton(interactions,CGRectMake(0,6,32,32),@"赞");
    interactions.likeCountLabel=[[UILabel alloc] initWithFrame:CGRectMake(20,14,30,16)];
    interactions.likeCountLabel.text=@"4.7万";
    [interactions.likeBtnView addSubview:interactions.likeCountLabel];
    interactions.commentBtnView=HGALandscapeFixtureButton(interactions,CGRectMake(67,6,57,32),@"评论");
    interactions.starBtnView=HGALandscapeFixtureButton(interactions,CGRectMake(134,6,57,32),@"收藏");
    UIImage *danmaku=[UIImage systemImageNamed:@"text.bubble"];
    interactions.danmakuBtnView=[[UIImageView alloc] initWithImage:danmaku];
    interactions.danmakuBtnView.frame=CGRectMake(211,7,30,30);
    [interactions addSubview:interactions.danmakuBtnView];
    interactions.danmakuInputView=[[UIView alloc] initWithFrame:CGRectMake(261,6,200,32)];
    UIVisualEffectView *nativeInputPaint=[[UIVisualEffectView alloc] initWithEffect:[UIBlurEffect effectWithStyle:UIBlurEffectStyleRegular]];
    nativeInputPaint.frame=interactions.danmakuInputView.bounds;
    interactions.danmakuInputBGView=nativeInputPaint;
    [interactions.danmakuInputView addSubview:nativeInputPaint];
    UILabel *placeholder=[[UILabel alloc] initWithFrame:interactions.danmakuInputView.bounds];
    placeholder.text=@"发条友善的弹幕吧";
    [interactions.danmakuInputView addSubview:placeholder];
    [interactions addSubview:interactions.danmakuInputView];

    FQVShortVideoLandscapeBottomRightView *bottom=[[FQVShortVideoLandscapeBottomRightView alloc] initWithFrame:CGRectMake(558,308,228,44)];
    UIStackView *stack=[[UIStackView alloc] initWithFrame:bottom.bounds];
    stack.spacing=8;stack.distribution=UIStackViewDistributionFillEqually;
    bottom.playRateBtn=HGALandscapeFixtureButton(stack,CGRectZero,@"倍速");
    bottom.resolutionBtn=HGALandscapeFixtureButton(stack,CGRectZero,@"清晰度");
    bottom.seriesBtn=HGALandscapeFixtureButton(stack,CGRectZero,@"选集");
    for(UIView *item in @[bottom.playRateBtn,bottom.resolutionBtn,bottom.seriesBtn]) [stack addArrangedSubview:item];
    [bottom addSubview:stack];
    info.bottomRightView=bottom;
    [bottomGroup addSubview:bottom];
    UISlider *progress=[[UISlider alloc] initWithFrame:CGRectMake(43,267,734,34)];
    [page addSubview:progress];

    CGRect nextFrame=next.frame,progressFrame=progress.frame;
    Layout(stack);
    [controls showControlWithoutScreenLock];
    Check(bottomGroup.alpha==0 && volume.alpha==1 && Count(bottomGroup,identifier)==8 &&
        Count(volume,identifier)==1 && nativeInputPaint.hidden,
        @"the native show hook decorates the visible audio control and prepares the still-hidden footer");
    volume.backgroundColor=UIColor.systemTealColor;
    [controls showControlWithoutAnimation];
    Check(bottomGroup.alpha==1 && Count(bottomGroup,identifier)==8 && [volume.backgroundColor isEqual:UIColor.clearColor],
        @"the native nonanimated show hook reuses the footer glass and refreshes the audio backing");
    UIView *backGlass=Find(top,identifier);
    CGRect titleRect=[info.titleL textRectForBounds:info.titleL.bounds limitedToNumberOfLines:info.titleL.numberOfLines];
    titleRect=[info.titleL convertRect:titleRect toView:top];
    Check(Count(page,identifier)==15 && Count(bottom,identifier)==3 && Count(interactions,identifier)==5 &&
        Count(navigation,identifier)==1 && Count(progress,identifier)==0 && Count(inflow,identifier)==1,
        @"landscape glass covers the original navigation, side and business controls without styling the scrubber");
    Check(backGlass.superview==top && !backGlass.userInteractionEnabled && CGRectGetHeight(backGlass.bounds)==40 &&
        CGRectContainsRect(backGlass.frame,titleRect) &&
        [top.subviews indexOfObjectIdenticalTo:backGlass]<[top.subviews indexOfObjectIdenticalTo:info.navBackImageView] &&
        info.navBackImageView.image==arrow && interactions.danmakuBtnView.image==danmaku && shareItem.shareIconV.image==shareImage,
        @"landscape title and arrow share a centered 40-point backing behind the unmodified native foreground");
    CGFloat previousTitleWidth=CGRectGetWidth(backGlass.frame);
    [info setVideoDataModel:@"谁让这个文盲修仙的：第十二集更长的完整标题"];
    Check(Find(top,identifier)==backGlass && CGRectGetWidth(backGlass.frame)>previousTitleWidth,
        @"landscape title glass updates from the host's episode model without a second surface");
    UIView *likeGlass=Find(interactions.likeBtnView,identifier);
    Check(CGRectContainsRect(likeGlass.frame,interactions.likeCountLabel.frame) &&
        Find(interactions.danmakuInputView,identifier).superview==interactions.danmakuInputView &&
        nativeInputPaint.hidden && !placeholder.hidden,
        @"landscape glass covers the native counter bounds and replaces the input blur under its placeholder");
    NSUInteger switchIndex=[interactions.subviews indexOfObjectIdenticalTo:interactions.danmakuBtnView];
    UIView *switchGlass=switchIndex>0 && switchIndex!=NSNotFound ? interactions.subviews[switchIndex-1] : nil;
    interactions.fixtureSwitchHidden=YES;
    [interactions refreshUI];
    BOOL followedHide=[switchGlass.accessibilityIdentifier isEqual:identifier] && switchGlass.hidden;
    interactions.fixtureSwitchHidden=NO;
    [interactions refreshUI];
    Check(followedHide && !switchGlass.hidden && Count(interactions,identifier)==5,
        @"the native footer refresh hides and restores the danmaku sibling glass with its glyph");
    Check(CGRectEqualToRect(next.frame,nextFrame) && CGRectEqualToRect(progress.frame,progressFrame) &&
        [info hitTest:CGPointMake(CGRectGetMidX(nextFrame),CGRectGetMidY(nextFrame)) withEvent:nil]==next,
        @"landscape glass preserves the original next-episode hit target and scrubber geometry");
    [next sendActionsForControlEvents:UIControlEventTouchUpInside];
    [inflow sendActionsForControlEvents:UIControlEventTouchUpInside];

    [controls updateScreenLockButtonStyle];
    volume.hidden=YES;
    Check(controls.screenLockBtn.hidden && Find(controls.screenLockBtn,identifier).superview==controls.screenLockBtn &&
        Find(volume,identifier).superview==volume,
        @"landscape lock and volume glass follow their native owners when hidden");
    HGAppearanceSet(@"player.controlsGlass",@NO,nil);
    for(UIView *root in @[info,navigation,controls,volume,inflow,interactions,bottom]) HGAppearanceProcessView(root);
    Check(!Find(page,identifier) && [next.backgroundColor isEqual:UIColor.darkGrayColor] && action.taps==2 &&
        stack.arrangedSubviews.count==3 && volume.clipsToBounds && volume.opaque && !nativeInputPaint.hidden &&
        [volume.backgroundColor isEqual:UIColor.systemTealColor] &&
        controls.screenLockBtn.clipsToBounds && [inflow.backgroundColor isEqual:UIColor.darkGrayColor],
        @"turning off landscape glass restores original paint and retains the native stack and action");
    window.hidden=YES;
    HGAppearanceApply(saved,nil);
}
