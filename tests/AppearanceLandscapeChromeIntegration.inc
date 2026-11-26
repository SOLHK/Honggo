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

@interface FQVLandscapeBrightnessButton : UIControl @end
@implementation FQVLandscapeBrightnessButton @end

@interface FQVLandscapeVolumeButton : UIControl @end
@implementation FQVLandscapeVolumeButton @end

@interface FQVTouchIgoringView : UIView @end
@implementation FQVTouchIgoringView
- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
    UIView *result=[super hitTest:point withEvent:event];
    return result==self ? nil : result;
}
@end

@interface SSShortVideoLandscapeAutoInflowTipView : UIControl @end
@implementation SSShortVideoLandscapeAutoInflowTipView @end

@interface FQVShortVideoLandscapeBottomRightView : UIView
@property(nonatomic,strong) UIView *playRateBtn;
@property(nonatomic,strong) UIView *resolutionBtn;
@property(nonatomic,strong) UIView *seriesBtn;
- (void)updateResolutionBtnTextWithString:(NSString *)value;
@end
@implementation FQVShortVideoLandscapeBottomRightView
- (void)updateResolutionBtnTextWithString:(NSString *)value { [(UIButton *)self.resolutionBtn setTitle:value forState:UIControlStateNormal]; }
@end

@interface SSShortVideoLandscapeBizInteractionView : UIView
@property(nonatomic,strong) UIView *likeBtnView;
@property(nonatomic,strong) UIView *commentBtnView;
@property(nonatomic,strong) UIView *starBtnView;
@property(nonatomic,strong) UIImageView *danmakuBtnView;
@property(nonatomic,strong) UIView *danmakuInputView;
@property(nonatomic,strong) UIView *danmakuInputBGView;
@property(nonatomic,readonly) UIImageView *likeIcon;
@property(nonatomic,readonly) UILabel *likeCountLabel;
@property(nonatomic) BOOL fixtureSwitchHidden;
- (void)refreshUI;
@end
@implementation SSShortVideoLandscapeBizInteractionView
- (UIImageView *)likeIcon { return (id)[self.likeBtnView viewWithTag:1]; }
- (UIImageView *)commentIcon { return (id)[self.commentBtnView viewWithTag:1]; }
- (UIImageView *)starIcon { return (id)[self.starBtnView viewWithTag:1]; }
- (UILabel *)likeCountLabel { return (id)[self.likeBtnView viewWithTag:2]; }
- (UILabel *)commentCountLabel { return (id)[self.commentBtnView viewWithTag:2]; }
- (UILabel *)starCountLabel { return (id)[self.starBtnView viewWithTag:2]; }
- (void)refreshUI { self.danmakuBtnView.hidden=self.fixtureSwitchHidden; }
@end

static UIButton *HGALandscapeFixtureButton(UIView *parent,CGRect frame,NSString *title) {
    UIButton *button=[UIButton buttonWithType:UIButtonTypeCustom];
    button.frame=frame;button.backgroundColor=UIColor.darkGrayColor;
    [button setTitle:title forState:UIControlStateNormal];
    [parent addSubview:button];
    return button;
}

static UIView *HGALandscapeFixtureCount(UIView *parent,CGFloat x,NSString *symbol,NSString *count,CGFloat width) {
    UIView *target=[[UIView alloc] initWithFrame:CGRectMake(x,0,32,32)];
    UIImageView *icon=[[UIImageView alloc] initWithImage:[UIImage systemImageNamed:symbol]];
    icon.tag=1;icon.frame=target.bounds;
    [target addSubview:icon];
    UILabel *label=[[UILabel alloc] initWithFrame:CGRectMake(20,16,width,14)];
    label.tag=2;label.font=[UIFont systemFontOfSize:11];label.text=count;
    [target addSubview:label];
    [parent addSubview:target];
    return target;
}

static BOOL HGALandscapeFixtureIsCircle(UIView *glass) {
    UIButton *button=(id)glass;
    return [button isKindOfClass:UIButton.class] && !button.userInteractionEnabled &&
        button.configuration.cornerStyle==UIButtonConfigurationCornerStyleCapsule &&
        CGRectGetWidth(button.bounds)>=40 && CGRectGetWidth(button.bounds)==CGRectGetHeight(button.bounds);
}

@interface FQVShortVideoListVCToastComponent : NSObject
@property(nonatomic) NSUInteger toastCount;
@property(nonatomic,copy) NSString *message;
- (void)toast:(NSString *)message;
@end
@implementation FQVShortVideoListVCToastComponent
- (void)toast:(NSString *)message { self.toastCount++;self.message=message; }
@end

static void RunLandscapeChromeChecks(SSTabBarController *host) {
    FQVShortVideoListVCToastComponent *toast=[FQVShortVideoListVCToastComponent new];
    NSString *failure=@"加载失败，请稍后重试";
    [toast toast:failure];
    Check(toast.toastCount==1 && [toast.message isEqual:failure],
        @"an unrecognized executable retains the native landscape failure toast and its message");
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
    UIButton *next=HGALandscapeFixtureButton(info,CGRectMake(688,191,86,32),@"下一集");
    info.nextBtn=next;
    HGAEpisodeTapFixture *action=[HGAEpisodeTapFixture new];
    [next addTarget:action action:@selector(open:) forControlEvents:UIControlEventTouchUpInside];

    FQVShortVideoLandscapeNavBar *navigation=[[FQVShortVideoLandscapeNavBar alloc] initWithFrame:CGRectMake(718,6,68,36)];
    info.navBar=navigation;
    [top addSubview:navigation];
    SSShortVideoShareLandscapeNavBarItem *shareItem=[SSShortVideoShareLandscapeNavBarItem new];
    shareItem.isShow=YES;
    UIImage *shareImage=[UIImage systemImageNamed:@"arrowshape.turn.up.right"];
    shareItem.shareIconV=[[UIImageView alloc] initWithImage:shareImage];
    shareItem.shareIconV.frame=CGRectMake(0,6,24,24);
    [navigation addSubview:shareItem.shareIconV];
    navigation.itemList=@[shareItem];
    info.moreBtn=HGALandscapeFixtureButton(navigation,CGRectMake(44,6,24,24),@"⋮");

    controls.screenLockBtn=HGALandscapeFixtureButton(controls,CGRectMake(22,145,30,30),@"锁");
    controls.screenLockBtn.clipsToBounds=YES;
    FQVLandscapeVolumeButton *volume=[[FQVLandscapeVolumeButton alloc] initWithFrame:CGRectMake(22,193,30,30)];
    volume.clipsToBounds=YES;
    volume.opaque=YES;
    volume.backgroundColor=UIColor.darkGrayColor;
    volume.alpha=0;
    controls.fixtureVolume=volume;
    [controls addSubview:volume];
    FQVLandscapeBrightnessButton *brightness=[[FQVLandscapeBrightnessButton alloc] initWithFrame:CGRectMake(22,193,32,32)];
    brightness.backgroundColor=UIColor.darkGrayColor;
    UILongPressGestureRecognizer *brightnessDrag=[[UILongPressGestureRecognizer alloc] initWithTarget:nil action:NULL];
    [brightness addGestureRecognizer:brightnessDrag];
    [controls addSubview:brightness];
    volume.frame=CGRectMake(730,193,32,32);
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
    SSShortVideoLandscapeBizInteractionView *interactions=[[SSShortVideoLandscapeBizInteractionView alloc] initWithFrame:CGRectMake(34,312,406,32)];
    info.bottomLeftView=interactions;
    [bottomGroup addSubview:interactions];
    interactions.likeBtnView=HGALandscapeFixtureCount(interactions,0,@"heart.fill",@"4.7万",25);
    interactions.commentBtnView=HGALandscapeFixtureCount(interactions,56,@"bubble.fill",@"768",18);
    interactions.starBtnView=HGALandscapeFixtureCount(interactions,112,@"star.fill",@"61.8万",29);
    UIImage *danmaku=[UIImage systemImageNamed:@"text.bubble"];
    interactions.danmakuBtnView=[[UIImageView alloc] initWithImage:danmaku];
    interactions.danmakuBtnView.frame=CGRectMake(168,0,32,32);
    [interactions addSubview:interactions.danmakuBtnView];
    interactions.danmakuInputView=[[UIView alloc] initWithFrame:CGRectMake(204,0,146,32)];
    UIVisualEffectView *nativeInputPaint=[[UIVisualEffectView alloc] initWithEffect:[UIBlurEffect effectWithStyle:UIBlurEffectStyleRegular]];
    nativeInputPaint.frame=interactions.danmakuInputView.bounds;
    interactions.danmakuInputBGView=nativeInputPaint;
    [interactions.danmakuInputView addSubview:nativeInputPaint];
    UILabel *placeholder=[[UILabel alloc] initWithFrame:CGRectInset(interactions.danmakuInputView.bounds,12,0)];
    placeholder.text=@"发条友善的弹幕吧";
    [interactions.danmakuInputView addSubview:placeholder];
    [interactions addSubview:interactions.danmakuInputView];

    FQVShortVideoLandscapeBottomRightView *bottom=[[FQVShortVideoLandscapeBottomRightView alloc] initWithFrame:CGRectMake(558,308,228,44)];
    UIStackView *stack=[[UIStackView alloc] initWithFrame:bottom.bounds];
    stack.spacing=8;stack.distribution=UIStackViewDistributionFillEqually;
    bottom.playRateBtn=HGALandscapeFixtureButton(stack,CGRectZero,@"倍速");
    bottom.resolutionBtn=HGALandscapeFixtureButton(stack,CGRectZero,@"720P");
    bottom.seriesBtn=HGALandscapeFixtureButton(stack,CGRectZero,@"选集");
    for(UIView *item in @[bottom.playRateBtn,bottom.resolutionBtn,bottom.seriesBtn]) [stack addArrangedSubview:item];
    [bottom addSubview:stack];
    info.bottomRightView=bottom;
    [bottomGroup addSubview:bottom];
    UISlider *progress=[[UISlider alloc] initWithFrame:CGRectMake(43,267,734,34)];
    [page addSubview:progress];

    CGRect nextFrame=next.frame,progressFrame=progress.frame,originalInputFrame=interactions.danmakuInputView.frame;
    CGRect originalCountFrame=interactions.likeCountLabel.frame,originalIconFrame=interactions.likeIcon.frame;
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
    CGRect titleFrame=info.titleL.frame;
    Check(CGRectGetMinX(titleFrame)-CGRectGetMaxX(backGlass.frame)>=12 && HGALandscapeFixtureIsCircle(Find(brightness,identifier)) && [brightness.gestureRecognizers containsObject:brightnessDrag], @"back/title separation and a native brightness glass button preserve the adjustment gesture");
    Check(Count(page,identifier)==16 && Count(bottom,identifier)==3 && Count(interactions,identifier)==5 &&
        Count(navigation,identifier)==2 && Count(progress,identifier)==0 && Count(inflow,identifier)==1,
        @"landscape glass covers the original navigation, side and business controls without styling the scrubber");
    Check(backGlass.superview==top && !backGlass.userInteractionEnabled && HGALandscapeFixtureIsCircle(backGlass) &&
        CGRectContainsRect(backGlass.frame,info.navBackImageView.frame) && !CGRectIntersectsRect(backGlass.frame,titleFrame) &&
        [top.subviews indexOfObjectIdenticalTo:backGlass]<[top.subviews indexOfObjectIdenticalTo:info.navBackImageView] &&
        info.navBackImageView.image==arrow && interactions.danmakuBtnView.image==danmaku && shareItem.shareIconV.image==shareImage,
        @"the landscape back arrow has its own round glass with the title outside the material");
    CGRect previousBackFrame=backGlass.frame;
    NSString *longTitle=@"谁让这个文盲修仙的：第十二集更长的完整标题";
    [info setVideoDataModel:longTitle];
    Check(Find(top,identifier)==backGlass && CGRectEqualToRect(backGlass.frame,previousBackFrame) &&
        CGRectEqualToRect(info.titleL.frame,titleFrame) && [info.titleL.text isEqual:longTitle] && !info.titleL.hidden,
        @"a long native title remains visible without widening the back arrow glass");
    UIView *likeGlass=Find(interactions.likeBtnView,identifier);
    UILabel *count=interactions.likeCountLabel;
    CGRect countInk=[count textRectForBounds:count.bounds limitedToNumberOfLines:count.numberOfLines];
    countInk=[count convertRect:countInk toView:interactions.likeBtnView];
    Check(CGRectContainsRect(likeGlass.frame,countInk) &&
        fabs(CGRectGetMidX(countInk)-CGRectGetMidX(likeGlass.frame))<0.1 &&
        CGRectGetMinY(countInk)>=CGRectGetMaxY(interactions.likeIcon.frame) && [count.text isEqual:@"4.7万"],
        @"the original landscape count fits centered below its glyph in the round backing");
    NSUInteger switchIndex=[interactions.subviews indexOfObjectIdenticalTo:interactions.danmakuBtnView];
    UIView *switchGlass=switchIndex>0 && switchIndex!=NSNotFound ? interactions.subviews[switchIndex-1] : nil;
    UIView *inputGlass=Find(interactions.danmakuInputView,identifier);
    UIView *shareGlass=Find(navigation,identifier),*moreGlass=Find(info.moreBtn,identifier);
    CGRect moreRect=[moreGlass convertRect:moreGlass.bounds toView:navigation];
    CGRect inputRect=[inputGlass convertRect:inputGlass.bounds toView:interactions];
    Check(inputGlass.superview==interactions.danmakuInputView && nativeInputPaint.hidden && !placeholder.hidden &&
        CGRectEqualToRect(interactions.danmakuInputView.frame,CGRectOffset(originalInputFrame,0,-2)) &&
        CGRectGetMinX(inputRect)-CGRectGetMaxX(switchGlass.frame)>=9.9 &&
        CGRectGetMinX(moreRect)-CGRectGetMaxX(shareGlass.frame)>=9.9,
        @"the share and more circles and the danmaku switch and input retain a gap within native hit geometry");
    BOOL equalCircles=HGALandscapeFixtureIsCircle(switchGlass) && HGALandscapeFixtureIsCircle(shareGlass);
    UIView *lockGlass=Find(controls.screenLockBtn,identifier), *lightGlass=Find(brightness,identifier);
    CGRect lockRect=[lockGlass convertRect:lockGlass.bounds toView:controls], lightRect=[lightGlass convertRect:lightGlass.bounds toView:controls];
    Check(CGRectGetMinY(lightRect)-CGRectGetMaxY(lockRect)>=13 && fabs(CGRectGetMidX(lightRect)-CGRectGetMidX(lockRect))<2,
        @"the lock and brightness controls form a spaced column");
    for(UIView *button in @[interactions.likeBtnView,interactions.commentBtnView,interactions.starBtnView,
        info.moreBtn,controls.screenLockBtn,volume,brightness])
        equalCircles &= HGALandscapeFixtureIsCircle(Find(button,identifier));
    Check(equalCircles,@"navigation and side actions use 40–44pt circles with room for counters");
    [bottom updateResolutionBtnTextWithString:@"1080P"];
    NSArray *options=@[bottom.playRateBtn,bottom.resolutionBtn,bottom.seriesBtn];
    for (NSNumber *rowWidth in @[@228,@168,@135]) {
        stack.frame=CGRectMake(0,0,rowWidth.doubleValue,32);[stack setNeedsLayout];[stack layoutIfNeeded];
        HGAppearanceProcessView(bottom);
        CGRect previous=CGRectNull;
        BOOL valid=YES;
        for (UIButton *option in options) {
            UIButton *glass=(id)Find(option,identifier);
            CGRect rect=[glass convertRect:glass.bounds toView:stack];
            valid &= [glass isKindOfClass:UIButton.class] && [glass.configuration.attributedTitle.string isEqual:option.currentTitle] &&
                (CGRectIsNull(previous) || CGRectGetMinX(rect)-CGRectGetMaxX(previous)>=3.9) &&
                glass.configuration.contentInsets.leading>=7 && [glass hitTest:CGPointMake(5,5) withEvent:nil]==nil;
            previous=rect;
        }
        Check(valid,@"long native playback choices retain horizontal padding and never overlap in compact or wide stacks");
    }
    UIButton *rate=(UIButton *)bottom.playRateBtn,*resolution=(UIButton *)bottom.resolutionBtn;
    UIButton *rateGlass=(id)Find(rate,identifier);
    CGPoint rim=[rateGlass convertPoint:CGPointMake(CGRectGetMidX(rateGlass.bounds),2) toView:controls];
    CGPoint center=[rate convertPoint:CGPointMake(CGRectGetMidX(rate.bounds),CGRectGetMidY(rate.bounds)) toView:controls];
    HGAEpisodeTapFixture *rimAction=[HGAEpisodeTapFixture new];
    [rate addTarget:rimAction action:@selector(open:) forControlEvents:UIControlEventTouchUpInside];
    UIView *rimHit=[controls hitTest:rim withEvent:nil];
    if(rimHit==rateGlass)[(UIButton *)rimHit sendActionsForControlEvents:UIControlEventTouchUpInside];
    Check([controls hitTest:center withEvent:nil]==rate && rimHit==rateGlass && rimAction.taps==1,
        @"capsule rim activates the correct original option once while its center hits the host button");
    CGRect first=[rateGlass convertRect:rateGlass.bounds toView:controls];
    UIView *second=Find(resolution,identifier);
    CGRect following=[second convertRect:second.bounds toView:controls];
    CGPoint gap=CGPointMake((CGRectGetMaxX(first)+CGRectGetMinX(following))/2,CGRectGetMidY(first));
    UIView *gapHit=[controls hitTest:gap withEvent:nil];
    UIButton *cover=HGALandscapeFixtureButton(controls,CGRectMake(rim.x-4,rim.y-4,8,8),@"");
    Check(gapHit!=rateGlass && gapHit!=second && [controls hitTest:rim withEvent:nil]==cover,
        @"the margin belongs to neither option and a native covering control retains hit priority");
    [cover removeFromSuperview];
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
    Check(!Find(page,identifier) && fabs(info.titleL.frame.origin.x-56)<0.1 && !((UIButton *)bottom.resolutionBtn).titleLabel.hidden && [next.backgroundColor isEqual:UIColor.darkGrayColor] && action.taps==2 &&
        stack.arrangedSubviews.count==3 && volume.clipsToBounds && volume.opaque && !nativeInputPaint.hidden &&
        [volume.backgroundColor isEqual:UIColor.systemTealColor] &&
        CGRectEqualToRect(interactions.likeCountLabel.frame,originalCountFrame) &&
        CGRectEqualToRect(interactions.likeIcon.frame,originalIconFrame) &&
        controls.screenLockBtn.clipsToBounds && [inflow.backgroundColor isEqual:UIColor.darkGrayColor],
        @"turning off landscape glass restores original foreground geometry, paint, native stack and actions");
    window.hidden=YES;
    HGAppearanceApply(saved,nil);
}
