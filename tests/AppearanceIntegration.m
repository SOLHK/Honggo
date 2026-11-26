#import "Honggo.h"
#import "Appearance.h"
#import <QuartzCore/QuartzCore.h>
#import <dispatch/dispatch.h>
#include <math.h>

// Controlled host identities exercise production adapters, not the Hongguo IPA.
@interface HGAActionFixture : UIControl
@property(nonatomic,strong) UIImageView *iconImageView;
@property(nonatomic,strong) UILabel *countLabel;
@property(nonatomic) NSUInteger taps;
@end
@implementation HGAActionFixture
- (instancetype)initWithFrame:(CGRect)frame {
    if((self=[super initWithFrame:frame])) {
        _iconImageView=[[UIImageView alloc] initWithImage:[UIImage systemImageNamed:@"bolt"]];
        _iconImageView.frame=CGRectMake(12,0,30,30);[self addSubview:_iconImageView];
        _countLabel=[[UILabel alloc] initWithFrame:CGRectMake(0,35,54,20)];_countLabel.text=@"123";
        _countLabel.font=[UIFont systemFontOfSize:18];[self addSubview:_countLabel];
        [self addTarget:self action:@selector(hit:) forControlEvents:UIControlEventTouchUpInside];
    }return self;
}
- (void)hit:(id)sender{self.taps++;}
- (void)layoutSubviews{[super layoutSubviews];}
@end
@interface SSShortVideoRightInteractionLikeModel : NSObject
@property(nonatomic) BOOL isLiked;
@end
@implementation SSShortVideoRightInteractionLikeModel @end
@interface SSShortVideoRightInteractionLikeView : HGAActionFixture
@property(nonatomic,strong) SSShortVideoRightInteractionLikeModel *model;
@property(nonatomic,readonly) UIImageView *likeIcon;
@end
@implementation SSShortVideoRightInteractionLikeView
- (instancetype)initWithFrame:(CGRect)frame {
    if ((self=[super initWithFrame:frame])) {
        self.iconImageView.userInteractionEnabled=YES;
        [self.iconImageView addGestureRecognizer:[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(likeBtnClicked)]];
    }
    return self;
}
- (UIImageView *)likeIcon {return self.iconImageView;}
- (void)likeBtnClicked {self.taps++;}
@end
@interface HGACollectFixtureModel : NSObject
@property(nonatomic,copy) NSString *vid;
@property(nonatomic) BOOL isCollected;
@end
@implementation HGACollectFixtureModel
@end
@interface SSShortVideoRightInteractionCollectView : HGAActionFixture
@property(nonatomic) BOOL isCollected;
@property(nonatomic) BOOL playedStockStar;
@property(nonatomic,strong) HGACollectFixtureModel *model;
- (void)starBtnClicked;
- (void)refreshUIWithModel:(HGACollectFixtureModel *)model animated:(BOOL)animated;
@end
@implementation SSShortVideoRightInteractionCollectView
- (void)starBtnClicked {self.taps++;}
- (void)refreshUIWithModel:(HGACollectFixtureModel *)model animated:(BOOL)animated {
    _model=model;
    self.playedStockStar=animated;
}
@end
@interface SSShortVideoRightInteractionCommentView : HGAActionFixture
@property(nonatomic,weak) id delegate;
- (void)refreshUIWithModel:(id)model;
@end
@implementation SSShortVideoRightInteractionCommentView
- (void)refreshUIWithModel:(id)model { self.countLabel.text=@"抢首评"; }
@end
@interface SSShortVideoRightInteractionShareView : HGAActionFixture @end
@implementation SSShortVideoRightInteractionShareView @end
@interface SSVideoSeriesFeedHeaderView : UIView
@property(nonatomic,strong) UIView *sideBar;
@property(nonatomic,strong) UIView *tabContainer;
@property(nonatomic,strong) UIView *searchViewContainer;
@property(nonatomic,strong) UIView *findDramaSearchRowViewV731;
@property(nonatomic,strong) UIView *findDramaAIEntranceViewV731;
@property(nonatomic) BOOL isFindDramaSearchPresentationActiveV731;
@end
@implementation SSVideoSeriesFeedHeaderView @end
@interface SSShortVideoBottomDistributionView : UIView
@property(nonatomic,strong) UIView *defaultContentView;
@property(nonatomic,strong) UIView *subBackgroundView;
- (void)renderWithModel:(NSDictionary *)model;
@end
@implementation SSShortVideoBottomDistributionView
- (void)renderWithModel:(NSDictionary *)model {
    NSNumber *entryWidth=model[@"entryWidth"];
    if(entryWidth) {
        CGRect frame=self.frame;
        frame.size.width=entryWidth.doubleValue;
        self.frame=frame;
    }
    self.defaultContentView.backgroundColor=model[@"background"];
    UIView *paint=self.subBackgroundView ?: self.defaultContentView.subviews.firstObject;
    paint.backgroundColor=model[@"shade"];
}
@end
@interface SSShortVideoOutterSeriesInfoView : UIView @end
@implementation SSShortVideoOutterSeriesInfoView @end
@interface FQVShortVideoPlayerCellView : UIView @end
@implementation FQVShortVideoPlayerCellView @end
@interface TTMetalView : UIView @end
@implementation TTMetalView @end
@interface HGPlaybackRenderLayer : CALayer
@property(nonatomic) NSUInteger fillWrites;
@end
@implementation HGPlaybackRenderLayer
- (void)setSublayerTransform:(CATransform3D)value {self.fillWrites++;[super setSublayerTransform:value];}
@end
@interface TTPlayerView : UIView
@property(nonatomic) BOOL nativeFrameWriterEnabled;
@property(nonatomic) CGRect nativeFrame;
@property(nonatomic) NSUInteger nativeLayoutPasses;
@property(nonatomic,copy) dispatch_block_t nativeWillMoveObserver;
@end
@implementation TTPlayerView
+ (Class)layerClass{return HGPlaybackRenderLayer.class;}
- (instancetype)initWithFrame:(CGRect)frame {
    if((self=[super initWithFrame:frame])) {
        TTMetalView *surface=[[TTMetalView alloc] initWithFrame:self.bounds];
        surface.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
        surface.backgroundColor=UIColor.greenColor;[self addSubview:surface];
    }
    return self;
}
- (void)layoutSubviews {
    [super layoutSubviews];self.nativeLayoutPasses++;
    if(self.nativeFrameWriterEnabled)self.frame=self.nativeFrame;
    self.subviews.firstObject.frame=self.bounds;
}
- (void)willMoveToSuperview:(UIView *)superview {
    if(self.nativeWillMoveObserver)self.nativeWillMoveObserver();
    [super willMoveToSuperview:superview];
}
@end
@interface FQVCollectionView : UICollectionView @end
@implementation FQVCollectionView @end
@interface HGTestCommentOpenParam : NSObject
@property(nonatomic) NSInteger commentCount;
@end
@implementation HGTestCommentOpenParam @end
@interface HGTestRailCommentModel : NSObject
@property(nonatomic,strong) HGTestCommentOpenParam *videoData;
@property(nonatomic,strong) HGTestCommentOpenParam *episodeVideoData;
@end
@implementation HGTestRailCommentModel @end
@interface SSShortVideoRightInteractionCommentViewManager : NSObject
@property(nonatomic,strong) HGTestRailCommentModel *model;
@property(nonatomic,strong) NSNumber *countModel;
@property(nonatomic,weak) SSShortVideoRightInteractionCommentView *commentView;
@property(nonatomic) BOOL supportComment;
@property(nonatomic) NSInteger displayedCount;
- (void)updateModelWithVideoDataModel:(HGTestRailCommentModel *)model;
- (void)refreshUIWithCommentCount:(NSInteger)count;
- (void)updateCommentData;
- (void)updateCommentDataByCommonRequest;
- (void)commentViewDidClick:(id)view;
@end
@implementation SSShortVideoRightInteractionCommentViewManager
- (void)updateModelWithVideoDataModel:(HGTestRailCommentModel *)model { self.model=model; }
- (void)refreshUIWithCommentCount:(NSInteger)count {
    self.displayedCount=count;
    if(self.commentView)self.commentView.countLabel.text=count>0 ? [@(count) stringValue] : @"抢首评";
}
- (void)updateCommentData { [self refreshUIWithCommentCount:self.countModel ? self.countModel.integerValue : 0]; }
- (void)updateCommentDataByCommonRequest { [self refreshUIWithCommentCount:self.countModel ? self.countModel.integerValue : 0]; }
- (void)commentViewDidClick:(id)view {}
@end
@interface HGTestCommentTab : NSObject
@property(nonatomic) NSUInteger type;
@property(nonatomic,copy) NSString *countText;
- (void)updateCount:(NSInteger)count;
@end
@implementation HGTestCommentTab
- (void)updateCount:(NSInteger)count {self.countText=[@(count) stringValue];}
@end
@interface FQCShortVideoCommentPanelViewController : UIViewController
@property(nonatomic,strong) HGTestCommentOpenParam *param;
@property(nonatomic,copy) NSArray<HGTestCommentTab *> *tabItems;
- (void)setupTabItems;
@end
@implementation FQCShortVideoCommentPanelViewController
- (void)setupTabItems {}
@end
@interface SSVideoSharePanelViewController : UIViewController
@property(nonatomic,strong) UIView *panelView;
@end
@implementation SSVideoSharePanelViewController @end
@interface FQVShortVideoSettingsViewController : UIViewController
@property(nonatomic) BOOL useBlurBackground;
@property(nonatomic,strong) UITableView *tableView;
@property(nonatomic,strong) UIVisualEffectView *blurView;
@property(nonatomic,strong) UIView *effectViewContentView;
@end
@implementation FQVShortVideoSettingsViewController @end
@interface FQVShortVideoNewSpeedSelectController : UIViewController @end
@implementation FQVShortVideoNewSpeedSelectController @end
@interface FQCShortVideoCommentPanelTitleView : UIView @end
@implementation FQCShortVideoCommentPanelTitleView @end
@interface SSTabBar : UIView
@property(nonatomic,strong) UIView *contentView;
@end
@implementation SSTabBar
- (instancetype)initWithFrame:(CGRect)frame{if((self=[super initWithFrame:frame])){_contentView=[[UIView alloc] initWithFrame:self.bounds];[self addSubview:_contentView];}return self;}
@end
@interface SSTabBarController : UIViewController
@property(nonatomic,strong) SSTabBar *customTabBar;
@property(nonatomic) NSUInteger selectedIndex;
@property(nonatomic) NSUInteger nativeTaps;
@end
@implementation SSTabBarController
- (void)viewDidLoad {
    [super viewDidLoad];self.view.backgroundColor=UIColor.systemBackgroundColor;
    self.customTabBar=[[SSTabBar alloc] initWithFrame:CGRectMake(0,self.view.bounds.size.height-90,self.view.bounds.size.width,60)];
    self.customTabBar.backgroundColor=UIColor.systemGrayColor;[self.view addSubview:self.customTabBar];
    NSArray *titles=@[@"首页",@"剧场",@"商城",@"我的"];
    for(NSUInteger i=0;i<4;i++) {
        UIControl *button=[[UIControl alloc] initWithFrame:CGRectMake(i*self.view.bounds.size.width/4,0,self.view.bounds.size.width/4,60)];
        button.tag=i;button.selected=i==0;
        UILabel *label=[[UILabel alloc] initWithFrame:button.bounds];label.text=titles[i];[button addSubview:label];
        [button addTarget:self action:@selector(nativeTap:) forControlEvents:UIControlEventTouchUpInside];
        [self.customTabBar.contentView addSubview:button];
    }
}
- (void)nativeTap:(UIControl *)button {
    self.nativeTaps++;self.selectedIndex=button.tag;
    for(UIControl *control in self.customTabBar.contentView.subviews)control.selected=control==button;
}
@end
@interface HGAAppearanceDock : UIView
- (void)setCollapsed:(BOOL)value animated:(BOOL)animated;
- (void)dragSelection:(UIPanGestureRecognizer *)gesture;
@end
static NSMutableArray *checks;
static void Check(BOOL passed,NSString *name){[checks addObject:@{@"name":name,@"passed":@(passed)}];NSLog(@"%@ %@",passed?@"PASS":@"FAIL",name);}
static UIView *Find(UIView *root,NSString *identifier) {
    if([root.accessibilityIdentifier isEqual:identifier])return root;
    for(UIView *child in root.subviews){UIView *match=Find(child,identifier);if(match)return match;}return nil;
}
static NSUInteger Count(UIView *root,NSString *identifier) {
    NSUInteger n=[root.accessibilityIdentifier isEqual:identifier];for(UIView *v in root.subviews)n+=Count(v,identifier);return n;
}
static BOOL ImageContentEqual(UIImage *actual,UIImage *expected) {
    if(actual==expected || [actual isEqual:expected])return YES;
    NSData *a=UIImagePNGRepresentation(actual),*b=UIImagePNGRepresentation(expected);
    BOOL equal=a.length && b.length && [a isEqual:b];
    if(!equal)NSLog(@"IMAGE_RESTORE expected=%@ actual=%@ bytes=%lu/%lu",expected,actual,(unsigned long)b.length,(unsigned long)a.length);
    return equal;
}
static void Layout(UIView *view){[view setNeedsLayout];[view layoutIfNeeded];}
static BOOL HGTestOpenGlass(UIView *view) {
    return HGNativeGlassConfigured() && !UIAccessibilityIsReduceTransparencyEnabled();
}
static BOOL HGTestFullGlassEffect(UIVisualEffectView *surface) {
    if (@available(iOS 26.0,*)) {
        if(![surface isKindOfClass:UIVisualEffectView.class])return NO;
        UIGlassEffect *effect=[surface.effect isKindOfClass:UIGlassEffect.class] ? (id)surface.effect : nil;
        return effect && !effect.tintColor && surface.cornerConfiguration && surface.alpha==1 &&
            !surface.maskView && !surface.layer.mask && !surface.clipsToBounds &&
            (!surface.contentView.backgroundColor || CGColorGetAlpha(surface.contentView.backgroundColor.CGColor)==0);
    }
    return NO;
}
static BOOL HGTestButtonGlass(UIView *surface) {
    return HGTestFullGlassEffect((id)surface) && !surface.userInteractionEnabled &&
        surface.accessibilityElementsHidden && !surface.isAccessibilityElement &&
        [[surface valueForKey:@"style"] isEqual:HGALiquidButtonMaterial()];
}
static BOOL HGTestMaterialRadius(UIView *view, CGFloat radius) {
    if (@available(iOS 26.0,*))
        if([view isKindOfClass:UIButton.class])
            return [view.cornerConfiguration isEqual:UICornerConfiguration.capsuleConfiguration] &&
                fabs(MIN(view.bounds.size.width,view.bounds.size.height)/2-radius)<0.01;
    return view && fabs(view.layer.cornerRadius-radius)<0.01;
}
static void (*HGOriginalMaterialSetEffect)(id,SEL,UIVisualEffect *);
static char HGMaterialEffectCountKey;
static NSUInteger HGMaterialEffectSetterCount(UIView *view) {
    return [objc_getAssociatedObject(view,&HGMaterialEffectCountKey) unsignedIntegerValue];
}
static void HGSpyMaterialSetEffect(UIVisualEffectView *view,SEL selector,UIVisualEffect *effect) {
    objc_setAssociatedObject(view,&HGMaterialEffectCountKey,
        @(HGMaterialEffectSetterCount(view)+1),OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    HGOriginalMaterialSetEffect(view,selector,effect);
}
static BOOL HGInstallMaterialEffectSpy(void) {
    static BOOL installed=NO;
    if(installed)return YES;
    Class target=NSClassFromString(@"HGAAppearanceMaterial");
    Method method=class_getInstanceMethod(target,@selector(setEffect:));
    if(!target || ![target isSubclassOfClass:UIVisualEffectView.class] || !method)return NO;
    HGOriginalMaterialSetEffect=(void (*)(id,SEL,UIVisualEffect *))method_getImplementation(method);
    class_replaceMethod(target,@selector(setEffect:),(IMP)HGSpyMaterialSetEffect,method_getTypeEncoding(method));
    installed=YES;
    return YES;
}
#include "PlaybackAppearanceIntegration.inc"
#include "AppearanceTransitionCoordinatorTests.inc"
#include "DockAppearanceIntegration.inc"
#include "NowPlayingIntegration.inc"
#include "EpisodeGlassIntegration.inc"
#include "EpisodeSpacingIntegration.inc"
#include "ActionShareAppearanceIntegration.inc"
#include "PlayerChromeIntegration.inc"
#include "PlayerGestureIntegration.inc"
#include "PlaybackQualityIntegration.inc"
#include "CommentMovementIntegration.inc"
#include "AppearanceLandscapeChromeIntegration.inc"
#include "DockVideoBackdropIntegration.inc"
#include "AppearancePageChromeIntegration.inc"
// Use UIKit's normal stack and table delegate, with synchronous pushes so
// every catalog destination can be exercised independently of animation timing.
@interface HGMenuTestNavigation : UINavigationController
@end
@implementation HGMenuTestNavigation
- (void)pushViewController:(UIViewController *)controller animated:(BOOL)animated {
    [super pushViewController:controller animated:NO];
}
- (UIViewController *)popViewControllerAnimated:(BOOL)animated {
    return [super popViewControllerAnimated:NO];
}
@end

static NSIndexPath *HGMenuPath(UITableViewController *menu, NSString *key) {
    NSString *identifier = [@"honggo.appearance.setting." stringByAppendingString:key];
    UITableView *table = menu.tableView;
    for (NSInteger section = 0; section < table.numberOfSections; section++) {
        for (NSInteger row = 0; row < [table numberOfRowsInSection:section]; row++) {
            NSIndexPath *path = [NSIndexPath indexPathForRow:row inSection:section];
            UITableViewCell *cell = [menu tableView:table cellForRowAtIndexPath:path];
            if ([cell.accessibilityIdentifier isEqual:identifier]) return path;
        }
    }
    return nil;
}

static void HGRunRightButtonSizeChecks(UIWindow *window, Class settings) {
    UIViewController *previousRoot=window.rootViewController;
    UITableViewController *editor=[(UITableViewController *)[settings alloc] initWithStyle:UITableViewStyleInsetGrouped];
    [editor setValue:@"rightButtons" forKey:@"group"];
    UINavigationController *navigation=[[UINavigationController alloc] initWithRootViewController:editor];
    window.rootViewController=navigation;
    [window makeKeyAndVisible];
    [editor loadViewIfNeeded];
    [window layoutIfNeeded];
    NSIndexPath *scalePath=HGMenuPath(editor,@"layout.collect.scale");
    UITableViewCell *cell=scalePath ? [editor tableView:editor.tableView cellForRowAtIndexPath:scalePath] : nil;
    UISlider *slider=(id)Find(cell,@"layout.collect.scale");
    double previous=HGAppearanceNumber(@"layout.collect.scale");
    [slider sendActionsForControlEvents:UIControlEventTouchDown];
    slider.value=41;
    [slider sendActionsForControlEvents:UIControlEventValueChanged];
    Check([slider.accessibilityValue isEqual:@"41%"] && HGAppearanceNumber(@"layout.collect.scale")==previous,
        @"dragging previews the exact percentage without refreshing playback");
    [slider sendActionsForControlEvents:UIControlEventTouchUpInside];
    UIStepper *stepper=[cell valueForKey:@"stepper"];
    stepper.value++;
    [stepper sendActionsForControlEvents:UIControlEventValueChanged];
    Check(slider.value==42 && [slider.accessibilityValue isEqual:@"42%"],@"one step advances from 41% to 42%");
    double chosen=HGAppearanceNumber(@"layout.collect.scale");
    NSIndexPath *copy=HGMenuPath(editor,@"@role.copyRightSize.collect");
    if (copy) [editor tableView:editor.tableView didSelectRowAtIndexPath:copy];
    NSDate *applied=[NSDate dateWithTimeIntervalSinceNow:1.5];
    while ((!editor.presentedViewController || editor.presentedViewController.isBeingPresented) && applied.timeIntervalSinceNow>0)
        [NSRunLoop.mainRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
    NSDictionary *stored=[NSUserDefaults.standardUserDefaults dictionaryForKey:@"Honggo.Appearance.v1"];
    BOOL shared=editor.view.window==window && copy && fabs(chosen-(0.65+41.0/49*0.35))<1e-6;
    for (NSString *r in @[@"avatar",@"collect",@"comment",@"like",@"share",@"more"]) {
        NSString *key=[NSString stringWithFormat:@"layout.%@.scale",r];
        shared &= [stored[key] isEqual:@(chosen)];
    }
    if (!shared) NSLog(@"SIZE_COPY mounted=%d row=%@ source=%f stored=%@ sheet=%@",
        editor.view.window==window,copy,chosen,[stored dictionaryWithValuesForKeys:@[@"layout.avatar.scale",
            @"layout.collect.scale",@"layout.comment.scale",@"layout.like.scale",@"layout.share.scale",@"layout.more.scale"]],
        editor.presentedViewController.title);
    Check(shared,@"applying a single size updates every right-hand control");
    __block BOOL dismissed=NO;
    UIAlertController *confirmation=(id)editor.presentedViewController;
    Check([confirmation.title isEqual:@"已统一右侧按钮大小"],@"copying the size confirms the committed edit");
    [confirmation dismissViewControllerAnimated:NO completion:^{dismissed=YES;}];
    NSDate *end=[NSDate dateWithTimeIntervalSinceNow:1.5];
    while (confirmation && !dismissed && end.timeIntervalSinceNow>0)
        [NSRunLoop.mainRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
    window.rootViewController=previousRoot;
    [window makeKeyAndVisible];
}

static void HGRunAppearanceMenuNavigationChecks(UIWindow *window, Class settings) {
    UIViewController *previousRoot = window.rootViewController;
    HGSettingsController *parent = [[HGSettingsController alloc] initWithStyle:UITableViewStyleInsetGrouped];
    HGMenuTestNavigation *navigation = [[HGMenuTestNavigation alloc] initWithRootViewController:parent];
    window.rootViewController = navigation;
    [window makeKeyAndVisible];
    [parent loadViewIfNeeded];
    [window layoutIfNeeded];
    // The global search result opens from the settings controller. Exercise
    // that route before a later category check presents a choice sheet.
    NSDictionary *match=nil;
    for(NSDictionary *entry in HGAppearanceSearch(@"双指清屏")) {
        if([entry[@"key"] isEqual:@"gestures.pinchClean"]){match=entry;break;}
    }
    BOOL wasEnabled=HGAppearanceFlag(@"gestures.pinchClean"),searchMaster=HGAppearanceEnabled();
    if(match)HGOpenAppearance(parent,[@"setting." stringByAppendingString:match[@"key"]]);
    UITableViewController *result=(id)navigation.topViewController;
    [result loadViewIfNeeded];
    [window layoutIfNeeded];
    NSIndexPath *gesturePath=result!=parent ? HGMenuPath(result,@"gestures.pinchClean") : nil;
    if(gesturePath)[result tableView:result.tableView didSelectRowAtIndexPath:gesturePath];
    Check(result!=parent && gesturePath && HGAppearanceFlag(@"gestures.pinchClean")!=wasEnabled && HGAppearanceEnabled()==searchMaster,
        @"the global search destination edits the requested gesture without changing the appearance master");
    HGAppearanceSet(@"gestures.pinchClean",@(wasEnabled),nil);
    [navigation popToViewController:parent animated:NO];
    [window layoutIfNeeded];
    HGOpenAppearance(parent,@"setting.player.fullscreenEntry");
    UIViewController *choiceDestination=navigation.topViewController;
    NSDate *choiceDeadline=[NSDate dateWithTimeIntervalSinceNow:1.5];
    while (!choiceDestination.presentedViewController && choiceDeadline.timeIntervalSinceNow>0)
        [NSRunLoop.mainRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
    UIAlertController *searchChoice=(id)choiceDestination.presentedViewController;
    Check([searchChoice isKindOfClass:UIAlertController.class] && [searchChoice.title isEqual:@"全屏观看入口"] &&
        searchChoice.actions.count==4,@"selecting an enum search result opens its options with one tap");
    [choiceDestination dismissViewControllerAnimated:NO completion:nil];
    [navigation popToViewController:parent animated:NO];
    [window layoutIfNeeded];
    BOOL savedMasterBeforeSubpage=HGAppearanceEnabled();
    BOOL savedHideLive=HGAppearanceFlag(@"layout.channels.hide.live");
    HGAppearanceApply(@{@"enabled":@NO,@"layout.channels.hide.live":@NO},nil);
    HGOpenAppearance(parent,@"chrome.hide");
    UITableViewController *chromeSubpage=(id)navigation.topViewController;
    [chromeSubpage loadViewIfNeeded];
    [window layoutIfNeeded];
    NSIndexPath *hideLivePath=chromeSubpage!=parent ? HGMenuPath(chromeSubpage,@"layout.channels.hide.live") : nil;
    UITableViewCell *showLiveCell=hideLivePath ? [chromeSubpage tableView:chromeSubpage.tableView cellForRowAtIndexPath:hideLivePath] : nil;
    Check([showLiveCell.textLabel.text isEqual:@"显示顶部真人剧"] && [(UISwitch *)showLiveCell.accessoryView isOn],
        @"a visible channel displays its visibility switch as on");
    if(hideLivePath)[chromeSubpage tableView:chromeSubpage.tableView didSelectRowAtIndexPath:hideLivePath];
    Check(chromeSubpage!=parent && hideLivePath && HGAppearanceFlag(@"layout.channels.hide.live") && HGAppearanceEnabled(),
        @"toggling a category subpage option while appearance master is off automatically enables appearance so changes take effect immediately");
    if (hideLivePath) {
        UISwitch *showLive=(UISwitch *)[chromeSubpage tableView:chromeSubpage.tableView cellForRowAtIndexPath:hideLivePath].accessoryView;
        Check(!showLive.on,@"tapping the visibility row off hides the channel using the old preference key");
        showLive.on=YES;
        [showLive sendActionsForControlEvents:UIControlEventValueChanged];
        Check(showLive.on && !HGAppearanceFlag(@"layout.channels.hide.live"),@"switching visibility on restores the channel");
    }
    HGAppearanceApply(@{@"enabled":@(savedMasterBeforeSubpage),@"layout.channels.hide.live":@(savedHideLive)},nil);
    [navigation popToViewController:parent animated:NO];
    [window layoutIfNeeded];
    HGOpenAppearance(parent, nil);
    UITableViewController *home = (id)navigation.topViewController;
    [home loadViewIfNeeded];
    [window layoutIfNeeded];
    BOOL routed = [home isKindOfClass:settings] && home.view.window == window && !home.navigationItem.searchController;
    NSIndexPath *enable = HGMenuPath(home, @"enabled");
    Check(routed && enable, @"the real settings entry mounts the appearance menu in the visible navigation stack");

    // Observe the two writes while home is visible. No action sheet has been
    // presented, so a modal here is a preference error from the actual row.
    BOOL original = HGAppearanceEnabled();
    if (enable) [home tableView:home.tableView didSelectRowAtIndexPath:enable];
    BOOL first = HGAppearanceEnabled();
    NSNumber *firstStored = [NSUserDefaults.standardUserDefaults dictionaryForKey:@"Honggo.Appearance.v1"][@"enabled"];
    UIViewController *firstModal = home.presentedViewController;
    if (enable && !firstModal) [home tableView:home.tableView didSelectRowAtIndexPath:enable];
    BOOL restored = HGAppearanceEnabled();
    NSNumber *lastStored = [NSUserDefaults.standardUserDefaults dictionaryForKey:@"Honggo.Appearance.v1"][@"enabled"];
    UIViewController *lastModal = home.presentedViewController;
    NSLog(@"MASTER_ROW path=%@ original=%d first=%d storedFirst=%@ restored=%d storedLast=%@ firstModal=%@/%@ lastModal=%@/%@",
        enable, original, first, firstStored, restored, lastStored, NSStringFromClass(firstModal.class),
        firstModal.title, NSStringFromClass(lastModal.class), lastModal.title);
    Check(enable && first != original && [firstStored isEqual:@(first)],
        @"tapping the appearance master row changes its persisted preference");
    Check(enable && first != original && restored == original && [lastStored isEqual:@(original)],
        @"tapping the appearance master row again restores its persisted preference");
    Check(!firstModal && !lastModal, @"both valid appearance switch edits complete without a save error sheet");
    BOOL rolesRouted = YES, choiceOpened = NO;
    NSUInteger visitedRoles = 0, styleRows = 0;
    for (NSString *group in @[@"dock", @"chrome", @"panelMaterial", @"fonts", @"player", @"comments", @"drama"]) {
        NSIndexPath *path = HGMenuPath(home, [@"@group." stringByAppendingString:group]);
        if (path) [home tableView:home.tableView didSelectRowAtIndexPath:path];
        UITableViewController *detail = (id)navigation.topViewController;
        if (!path || detail == home || ![[detail valueForKey:@"group"] isEqual:group] || detail.navigationItem.searchController) {
            routed = NO;
            continue;
        }
        [window layoutIfNeeded];
        NSString *styleKey=@{@"dock":@"dock.style",@"chrome":@"player.headerStyle",@"player":@"player.actionStyle"}[group];
        if (styleKey && HGMenuPath(detail,styleKey)) styleRows++;
        if ([group isEqual:@"dock"] || [group isEqual:@"chrome"]) {
            NSString *selection=[group isEqual:@"dock"] ? @"dock.selectionStyle" : @"player.headerSelectionStyle";
            Check(HGMenuPath(detail,selection)!=nil,@"the selection treatment is reachable beside its own rail style");
        }
        if ([group isEqual:@"comments"])
            Check(HGMenuPath(detail,@"panels.comment") && HGMenuPath(detail,@"player.commentMovesVideo") &&
                HGMenuPath(detail,@"gestures.doubleTapComments") &&
                HGMenuPath(detail,@"panels.commentFloatingBar") &&
                HGMenuPath(detail,@"panels.commentDetachedInput") &&
                HGMenuPath(detail,@"panels.commentInputMaterial"),
                @"comments groups its panel, video layout, opening gesture, and floating/detached composer controls");
        if ([group isEqual:@"player"]) {
            Check(HGMenuPath(detail,@"gestures.cleanWhilePlaying")!=nil,@"the playback page exposes automatic clean mode");
            NSIndexPath *shortcut=HGMenuPath(detail,@"@group.player.visibility");
            if(shortcut)[detail tableView:detail.tableView didSelectRowAtIndexPath:shortcut];
            UITableViewController *visibility=(id)navigation.topViewController;
            [visibility loadViewIfNeeded];
            NSArray *keys=@[@"layout.channels.hide.sidebar",@"layout.channels.hide.follow",@"layout.channels.hide.comic",
                @"layout.channels.hide.live",@"layout.channels.hide.recommend",@"layout.channels.hide.search",
                @"player.hideFeedAvatarPhoto",@"player.hideFeedFollow",@"layout.collect.hidden",@"player.hideFeedCollectCount",
                @"layout.comment.hidden",@"player.hideFeedCommentCount",@"layout.like.hidden",@"player.hideFeedLikeCount",
                @"layout.share.hidden",@"player.hideFeedShareCaption",@"player.fullscreenEntry",@"player.hideFeedDanmakuToggle",
                @"player.hideFeedDanmakuPublish",@"player.hideFeedAuthor",@"player.hideFeedDescription",@"layout.episodes.hidden",
                @"layout.progress.hidden"];
            BOOL allReachable=visibility!=detail;
            for(NSString *key in keys)allReachable &= HGMenuPath(visibility,key)!=nil;
            Check(shortcut && allReachable,@"all marked feed controls are reachable from playback visibility");
            if(visibility!=detail)[navigation popViewControllerAnimated:NO];
        }
        if ([group isEqual:@"player"] || [group isEqual:@"drama"]) {
            BOOL player=[group isEqual:@"player"];
            NSArray *roles=player ? @[@"channels",@"info",@"tags",@"progress",@"danmaku",@"notice",
                @"avatar",@"collect",@"comment",@"like",@"share",@"more"] : @[@"episodes"];
            UITableViewController *roleList=detail;
            for (NSString *role in roles) {
                if ([role isEqual:@"avatar"]) {
                    NSIndexPath *right=HGMenuPath(detail,@"@group.rightButtons");
                    if (right) [detail tableView:detail.tableView didSelectRowAtIndexPath:right];
                    roleList=(id)navigation.topViewController;
                    [window layoutIfNeeded];
                    rolesRouted &= right && [[roleList valueForKey:@"group"] isEqual:@"rightButtons"] &&
                        roleList.tableView.numberOfSections==6;
                }
                if (roleList!=detail) {
                    NSString *prefix=[NSString stringWithFormat:@"layout.%@.",role];
                    NSIndexPath *hidden=HGMenuPath(roleList,[prefix stringByAppendingString:@"hidden"]);
                    BOOL inlineRole=hidden && [roleList.tableView numberOfRowsInSection:hidden.section]==6;
                    for (NSString *suffix in @[@"x",@"y",@"scale"]) {
                        NSIndexPath *p=HGMenuPath(roleList,[prefix stringByAppendingString:suffix]);
                        inlineRole &= p && p.section==hidden.section;
                    }
                    inlineRole &= HGMenuPath(roleList,[@"@role.copyRightSize." stringByAppendingString:role]) &&
                        HGMenuPath(roleList,[@"@role.reset." stringByAppendingString:role]);
                    rolesRouted &= inlineRole;
                    if (inlineRole) visitedRoles++;
                    continue;
                }
                NSString *destination = [@"role." stringByAppendingString:role];
                NSIndexPath *entry = HGMenuPath(roleList, [@"@group." stringByAppendingString:destination]);
                if (entry) [roleList tableView:roleList.tableView didSelectRowAtIndexPath:entry];
                if (entry && navigation.topViewController != roleList &&
                        [[navigation.topViewController valueForKey:@"group"] isEqual:destination]) {
                    visitedRoles++;
                    [navigation popViewControllerAnimated:NO];
                    [window layoutIfNeeded];
                } else rolesRouted = NO;
            }
        }
        if ([group isEqual:@"drama"]) {
            NSIndexPath *keywordPath=HGMenuPath(detail,@"video.blockedKeywords");
            if(keywordPath)[detail tableView:detail.tableView didSelectRowAtIndexPath:keywordPath];
            UIViewController *editor=navigation.topViewController;
            [window layoutIfNeeded];
            UITextView *input=(id)Find(editor.view,@"honggo.video.keywordEditor");
            NSArray *oldKeywords=HGVideoBlockedKeywords();
            BOOL sent=NO;
            if(input.window==window) {
                input.text=@"  校园  \n\n校园\n都市";
                UIBarButtonItem *save=editor.navigationItem.rightBarButtonItem;
                sent=[UIApplication.sharedApplication sendAction:save.action to:save.target from:save forEvent:nil];
            }
            [window layoutIfNeeded];
            BOOL saved=sent && navigation.topViewController==detail && [HGVideoBlockedKeywords() isEqual:@[@"校园",@"都市"]];
            if(!saved)NSLog(@"KEYWORD_EDITOR input=%@ sent=%d top=%@ values=%@ alert=%@",
                input.class,sent,navigation.topViewController.class,HGVideoBlockedKeywords(),editor.presentedViewController.title);
            Check(saved,
                @"saving the multiline editor from the drama page updates the active keyword list");
            HGAppearanceSet(@"video.blockedKeywords",oldKeywords,nil);
            NSIndexPath *choice = HGMenuPath(detail, @"player.separateProgress");
            if (choice) [detail tableView:detail.tableView didSelectRowAtIndexPath:choice];
            NSDate *deadline=[NSDate dateWithTimeIntervalSinceNow:1.5];
            while(choice && !detail.presentedViewController && deadline.timeIntervalSinceNow>0)
                [NSRunLoop.mainRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
            choiceOpened = [detail.presentedViewController isKindOfClass:UIAlertController.class] &&
                [detail.presentedViewController.title isEqual:@"合集与进度条分开布局"];
            if (choiceOpened) {
                UIAlertController *sheet=(id)detail.presentedViewController;
                NSString *current=HGAppearanceString(@"player.separateProgress");
                NSString *checked=[current isEqual:@"show"] ? @"✓ 自动留出间距" : @"✓ 原始布局";
                choiceOpened=sheet.actions.count==3 && [[sheet.actions valueForKey:@"title"] containsObject:checked] &&
                    sheet.actions.lastObject.style==UIAlertActionStyleCancel;
            }
            [detail dismissViewControllerAnimated:NO completion:nil];
        }
        [navigation popToViewController:home animated:NO];
        [window layoutIfNeeded];
    }
    Check(routed, @"every category opens from a pushed appearance menu without a search controller");
    Check(styleRows == 3, @"the dock, header and player style selectors are reachable from the appearance menu");
    Check(rolesRouted && visitedRoles == HGAppearanceRoles().count,
        @"every playback control opens its own editor through the table delegate");
    Check(choiceOpened, @"a pushed playback menu presents its layout choice sheet");
    window.rootViewController = previousRoot;
    [window makeKeyAndVisible];
}
@interface HGAAppearanceTests : UIResponder <UIWindowSceneDelegate>
@property(nonatomic,strong) UIWindow *window;
@property(nonatomic,strong) SSTabBarController *host;
@end
@implementation HGAAppearanceTests
- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)options {
    checks=[NSMutableArray array];HGAppearanceReset();
    HGSetEnabled(HGGlass,YES);HGPrepareNativeGlass();
    Check(HGInstallTabBar(),@"legacy tab-bar adapter installs alongside appearance");
    HGInstallAppearance();HGInstallAppearance();
    Check(HGInstallMaterialEffectSpy(),@"material effect setter spy installs");
    self.host=[SSTabBarController new];
    self.window=[[UIWindow alloc] initWithWindowScene:(UIWindowScene *)scene];self.window.rootViewController=self.host;[self.window makeKeyAndVisible];
    // The interactive checks pump the run loop while waiting for host callbacks.
    // Start outside a main-queue callback so those turns can drain main-queue work.
    CFRunLoopRef runLoop=CFRunLoopGetMain();
    CFRunLoopPerformBlock(runLoop,kCFRunLoopDefaultMode,^{
        @try {[self runChecks];} @catch(NSException *e){Check(NO,[@"exception: " stringByAppendingString:e.description]);}
        [self finish];
    });
    CFRunLoopWakeUp(runLoop);
}
- (void)runChecks {
    UIView *root=self.host.view;
    FQCShortVideoCommentPanelViewController *countPanel=[FQCShortVideoCommentPanelViewController new];
    countPanel.param=[HGTestCommentOpenParam new];
    HGTestCommentTab *episodeComments=[HGTestCommentTab new],*seriesReviews=[HGTestCommentTab new];
    seriesReviews.type=1;
    countPanel.tabItems=@[seriesReviews,episodeComments];
    [countPanel setupTabItems];
    Check(!episodeComments.countText && !seriesReviews.countText,@"a missing initial count leaves the loading tabs empty");
    countPanel.param.commentCount=74;
    [countPanel setupTabItems];
    Check([episodeComments.countText isEqual:@"74"] && !seriesReviews.countText,
        @"the current episode opening count seeds only its own first-render tab");
    [episodeComments updateCount:76];[countPanel setupTabItems];
    Check([episodeComments.countText isEqual:@"76"],@"a newer native count retains priority over the opening parameter");
    SSShortVideoRightInteractionCommentViewManager *commentManager=[SSShortVideoRightInteractionCommentViewManager new];
    HGTestRailCommentModel *railModel=[HGTestRailCommentModel new];
    railModel.videoData=[HGTestCommentOpenParam new];
    railModel.videoData.commentCount=74;commentManager.supportComment=YES;
    [commentManager updateModelWithVideoDataModel:railModel];BOOL primed=commentManager.displayedCount==74;
    [commentManager updateCommentData];primed &= commentManager.displayedCount==74;
    [commentManager updateCommentDataByCommonRequest];primed &= commentManager.displayedCount==74;
    [commentManager refreshUIWithCommentCount:0];primed &= commentManager.displayedCount==74;
    Check(primed,@"the right comment button displays this episode's known count at binding and resists placeholder zero resets before countModel arrives");
    commentManager.countModel=@0;[commentManager updateCommentData];
    Check(commentManager.displayedCount==0,@"a real response of zero supersedes a positive feed count");
    commentManager.countModel=nil;railModel.videoData.commentCount=0;[commentManager updateCommentData];
    Check(commentManager.displayedCount==0,@"a missing feed count preserves the native pending state");
    railModel.videoData=nil;railModel.episodeVideoData=[HGTestCommentOpenParam new];
    railModel.episodeVideoData.commentCount=19;[commentManager updateModelWithVideoDataModel:railModel];
    Check(commentManager.displayedCount==19,@"outer episode models without videoData still prime their known comment count");
    railModel.videoData=railModel.episodeVideoData;railModel.episodeVideoData=nil;
    commentManager.displayedCount=-1;railModel.videoData.commentCount=33;commentManager.supportComment=NO;[commentManager updateCommentDataByCommonRequest];
    Check(commentManager.displayedCount<=0,@"unavailable comments are not populated");
    SSShortVideoRightInteractionCommentView *commentAction=[[SSShortVideoRightInteractionCommentView alloc] initWithFrame:CGRectMake(45,210,46,62)];
    [root addSubview:commentAction];
    commentManager.supportComment=YES;commentManager.commentView=commentAction;commentAction.delegate=commentManager;
    railModel.videoData.commentCount=101;[commentManager updateModelWithVideoDataModel:railModel];
    [commentAction refreshUIWithModel:railModel];
    Check([commentAction.countLabel.text isEqual:@"101"],@"a view-level refresh while countModel is pending cannot flash 抢首评 over a primed episode count");
    commentAction.delegate=nil;commentManager.commentView=nil;
    commentAction.countLabel.text=@"74";
    BOOL countVisible=YES;
    for (NSString *style in @[@"stock",@"flat",@"glass"]) {
        BOOL stock=[style isEqual:@"stock"];
        countVisible &= HGAppearanceApply(@{@"enabled":@YES,@"player.rightGlass":stock ? @NO : @YES,
            @"player.actionStyle":stock ? @"glass" : style},nil);
        Layout(commentAction);
        UIView *decoration=Find(commentAction,@"honggo.appearance.material");
        countVisible &= (stock ? decoration==nil : decoration!=nil) && [commentAction.countLabel.text isEqual:@"74"] && !commentAction.countLabel.hidden &&
            commentAction.countLabel.alpha>0.99 && (!decoration || (!decoration.userInteractionEnabled &&
            [commentAction.subviews indexOfObject:decoration]<[commentAction.subviews indexOfObject:commentAction.countLabel]));
    }
    Check(countVisible,@"stock, flat and glass preserve the host comment count above the decorative surface");
    [commentAction removeFromSuperview];HGAppearanceReset();
    SSShortVideoRightInteractionLikeView *like=[[SSShortVideoRightInteractionLikeView alloc] initWithFrame:CGRectMake(20,140,54,65)];
    like.model=[SSShortVideoRightInteractionLikeModel new];like.backgroundColor=UIColor.darkGrayColor;[root addSubview:like];Layout(like);
    UIImage *nativeImage=like.iconImageView.image;UIFont *nativeFont=like.countLabel.font;
    CGRect original=like.frame;
    Check(Count(like,@"honggo.appearance.material")==0,@"disabled appearance adds no playback glass");
    Check(CGAffineTransformIsIdentity(like.transform),@"disabled appearance preserves native geometry");
    SSShortVideoRightInteractionView *rail=[[SSShortVideoRightInteractionView alloc] initWithFrame:CGRectMake(180,210,46,62)];
    [root addSubview:rail];
    SSShortVideoRightInteractionLikeView *touchLike=[[SSShortVideoRightInteractionLikeView alloc] initWithFrame:rail.bounds];
    touchLike.model=[SSShortVideoRightInteractionLikeModel new];
    touchLike.likeIcon.frame=CGRectMake(0,0,46,46);
    touchLike.countLabel.frame=CGRectMake(2,46,42,16);
    [rail addSubview:touchLike];
    UIGestureRecognizer *hostTap=touchLike.likeIcon.gestureRecognizers.firstObject;
    BOOL nativeTargets=YES;
    for (NSString *style in @[@"glass",@"flat"]) {
        HGAppearanceApply(@{@"enabled":@YES,@"player.rightGlass":@YES,@"player.actionStyle":style,@"layout.like.x":@0},nil);
        Layout(touchLike);
        nativeTargets &= [rail hitTest:CGPointMake(4,23) withEvent:nil]==touchLike.likeIcon &&
            [rail hitTest:CGPointMake(23,23) withEvent:nil]==touchLike.likeIcon &&
            touchLike.likeIcon.gestureRecognizers.count==1 && touchLike.likeIcon.gestureRecognizers.firstObject==hostTap;
    }
    HGAppearanceSet(@"layout.like.x",@(-30),nil);Layout(touchLike);
    Check(nativeTargets && [rail hitTest:CGPointMake(-7,23) withEvent:nil]==touchLike.likeIcon &&
        ![rail hitTest:CGPointMake(-55,23) withEvent:nil],
        @"glass, flat and offset hearts route taps to the same host recognizer across the visible disc");
    HGAppearanceSet(@"layout.like.hidden",@YES,nil);
    Check(![rail hitTest:CGPointMake(-7,23) withEvent:nil],@"hiding the offset heart also removes its extended hit area");
    [rail removeFromSuperview];HGAppearanceReset();
    Check(HGAppearanceApply(HGAppearancePreset(@"glass"),nil),@"visible preset can be applied");Layout(like);
    UIView *glass=Find(like,@"honggo.appearance.material");
    Check(glass!=nil && !glass.userInteractionEnabled,@"playback glass is attached and does not intercept touches");
    Check(like.iconImageView.image==nativeImage,@"glass buttons retain the host glyph");
    HGAppearanceSet(@"player.actionStyle",@"flat",nil);Layout(like);
    Check(like.iconImageView.image!=nativeImage,@"flat styling draws the filled glyph");
    Check([like.countLabel.text isEqual:@"123"],@"counts remain host-owned");
    for(NSUInteger i=0;i<5;i++)Layout(like);
    Check(Count(like,@"honggo.appearance.material")==1,@"repeated layout does not stack material views");
    for(NSUInteger cycle=0;cycle<3;cycle++) {
        HGAppearanceSet(@"player.actionStyle",@"glass",nil);
        Check(like.iconImageView.image==nativeImage,@"repeated icon toggles restore the same native image");
        if(like.iconImageView.image!=nativeImage)NSLog(@"ICON RESTORE native=%@ actual=%@",nativeImage,like.iconImageView.image);
        HGAppearanceSet(@"player.actionStyle",@"flat",nil);Layout(like);
    }
    HGAppearanceSet(@"player.actionStyle",@"glass",nil);
    like.iconImageView.image=[UIImage systemImageNamed:@"bolt.fill"];
    UIImage *updatedHostImage=like.iconImageView.image;
    HGAppearanceSet(@"player.actionStyle",@"flat",nil);Layout(like);
    HGAppearanceSet(@"player.actionStyle",@"glass",nil);
    Check(like.iconImageView.image==updatedHostImage,@"new host image becomes the next restoration baseline");
    like.iconImageView.image=nativeImage;
    HGAppearanceSet(@"player.actionStyle",@"flat",nil);Layout(like);
    [like sendActionsForControlEvents:UIControlEventTouchUpInside];Check(like.taps==1,@"native playback action executes exactly once");
    UIImage *unliked=like.iconImageView.image;
    like.model.isLiked=YES;Layout(like);
    Check(![like.iconImageView.image isEqual:unliked] && [like.iconImageView.tintColor isEqual:UIColor.systemPinkColor],@"like icon follows actual model selection state");
    like.model=[SSShortVideoRightInteractionLikeModel new];
    Check(![like.iconImageView.tintColor isEqual:UIColor.systemPinkColor],@"model replacement restores the unselected appearance");
    HGAppearanceApply(@{@"layout.like.x":@12,@"layout.like.y":@8,@"layout.like.scale":@0.9},nil);Layout(like);
    CGAffineTransform expected=like.transform;
    Check(fabs(expected.tx-12)<0.01 && fabs(expected.ty-8)<0.01 && fabs(expected.a-0.9)<0.01,@"per-control offsets and scale affect the actual view");
    for(NSUInteger i=0;i<6;i++)Layout(like);
    Check(CGAffineTransformEqualToTransform(expected,like.transform),@"repeated layout does not compound scaling or offsets");
    HGAppearanceSet(@"layout.like.hidden",@YES,nil);Check(like.hidden,@"per-control hide takes effect");
    HGAppearanceSet(@"layout.like.hidden",@NO,nil);Check(!like.hidden,@"unhide restores original visibility");
    FQVShortVideoPlayerCellView *fontCell=[[FQVShortVideoPlayerCellView alloc] initWithFrame:root.bounds];
    HGTestFontReadLabel *fontProbe=[HGTestFontReadLabel new];
    [fontCell addSubview:fontProbe];[root addSubview:fontCell];
    HGAppearanceApply(@{@"fonts.enabled":@YES,@"fonts.scale":@0.8},nil);Layout(like);
    HGAppearanceProcessView(fontCell);
    Check(fabs(like.countLabel.font.pointSize-nativeFont.pointSize*0.8)<0.01,@"font scaling changes native labels");
    Layout(like);Layout(like);Check(fabs(like.countLabel.font.pointSize-nativeFont.pointSize*0.8)<0.01,@"font scaling is not multiplied on reuse");
    HGAppearanceSet(@"fonts.enabled",@NO,nil);Check([like.countLabel.font isEqual:nativeFont],@"disabling font overrides restores the native font");
    fontProbe.fontReads=0;
    HGAppearanceProcessView(fontCell);
    Check(fontProbe.fontReads==0,@"after font restoration, ordinary player layouts stop inspecting label fonts");
    [fontCell removeFromSuperview];

    SSShortVideoRightInteractionCollectView *collect=[[SSShortVideoRightInteractionCollectView alloc] initWithFrame:CGRectMake(90,140,54,65)];[root addSubview:collect];Layout(collect);
    collect.isCollected=YES;Check([collect.iconImageView.tintColor isEqual:UIColor.systemYellowColor],@"collection setter refreshes actual selected icon");
    NSString *actionStyle=HGAppearanceString(@"player.actionStyle");
    HGAppearanceSet(@"player.collectGlyph",@"sparkles",nil);
    [collect refreshUIWithModel:nil animated:YES];
    Check(!collect.playedStockStar,@"the alternate symbol avoids a transient stock star animation");
    collect.isCollected=NO;
    for (NSString *style in @[@"glass",@"flat"]) {
        HGAppearanceSet(@"player.actionStyle",style,nil);
        HGACollectFixtureModel *initial=[HGACollectFixtureModel new];
        initial.vid=style;
        [collect refreshUIWithModel:initial animated:NO];
        UIImage *outline=collect.iconImageView.image;
        Check(HGAActionPixelAlpha(outline,19,20)==0 && [collect.iconImageView.tintColor isEqual:UIColor.whiteColor],
            @"the alternate favorite has transparent centers when unselected");
        Check(!Find(collect,@"honggo.appearance.material"),@"the alternate favorite has no glass backing");
        NSUInteger taps=collect.taps;
        [collect starBtnClicked];
        Check(collect.taps==taps+1 && collect.iconImageView.image==outline &&
            ![collect.iconImageView.layer animationForKey:@"honggo.collect.pulse"],
            @"the original tap waits for the host model to change");
        [collect refreshUIWithModel:initial animated:YES];
        Check(collect.iconImageView.image==outline && ![collect.iconImageView.layer animationForKey:@"honggo.collect.pulse"],
            @"an unchanged host result never plays the fill animation");
        [collect starBtnClicked];
        HGACollectFixtureModel *confirmed=[HGACollectFixtureModel new];
        confirmed.vid=initial.vid;
        confirmed.isCollected=YES;
        [collect refreshUIWithModel:confirmed animated:YES];
        Check(!collect.isCollected && HGAActionPixelAlpha(collect.iconImageView.image,19,20)>240 &&
            [collect.iconImageView.tintColor isEqual:UIColor.systemBlueColor] &&
            (UIAccessibilityIsReduceMotionEnabled() || [collect.iconImageView.layer animationForKey:@"honggo.collect.pulse"]),
            @"the host model's selection fills the constellation blue and animates despite the stale view property");
        [collect refreshUIWithModel:initial animated:YES];
        Check(collect.iconImageView.image==outline && ![collect.iconImageView.layer animationForKey:@"honggo.collect.pulse"],
            @"removing a favorite restores its outline and stops the previous pulse");
        confirmed.vid=[style stringByAppendingString:@"-reused"];
        [collect refreshUIWithModel:confirmed animated:YES];
        Check([collect.iconImageView.tintColor isEqual:UIColor.systemBlueColor] &&
            ![collect.iconImageView.layer animationForKey:@"honggo.collect.pulse"],
            @"a reused view adopts the new video's selection without replaying the animation");
    }
    HGAppearanceApply(@{@"player.actionStyle":actionStyle,@"player.collectGlyph":@"star"},nil);
    [collect refreshUIWithModel:nil animated:YES];
    Check(collect.playedStockStar,@"the stock favorite restores the host animation");
    for(Class cls in @[SSShortVideoRightInteractionCommentView.class,SSShortVideoRightInteractionShareView.class]) {
        HGAActionFixture *view=[[cls alloc] initWithFrame:CGRectMake(160,140,54,65)];[root addSubview:view];Layout(view);
        Check(Find(view,@"honggo.appearance.material")!=nil,@"comment and share actions receive scoped glass");
        [view sendActionsForControlEvents:UIControlEventTouchUpInside];Check(view.taps==1,@"comment and share preserve original actions");
    }
    SSVideoSeriesFeedHeaderView *header=[[SSVideoSeriesFeedHeaderView alloc] initWithFrame:CGRectMake(20,60,280,44)];[root addSubview:header];Layout(header);
    Check(Find(header,@"honggo.appearance.material")!=nil,@"top channel glass is active");
    VerifyPageChrome(root,self.host.customTabBar,self.host);
    SSShortVideoBottomDistributionView *episodes=[[SSShortVideoBottomDistributionView alloc] initWithFrame:CGRectMake(20,220,280,44)];[root addSubview:episodes];Layout(episodes);
    Check(Find(episodes,@"honggo.appearance.material")!=nil,@"episode entry glass is active");
    BOOL reducedTransparency=UIAccessibilityIsReduceTransparencyEnabled();
    Class expectedEffect=HGNativeGlassConfigured() ? NSClassFromString(@"UIGlassEffect") : UIBlurEffect.class;
    // Exercise the custom-radius glass path independently of UIKit's capsule.
    HGAppearanceSet(@"dock.radius",@21,nil);
    HGAppearanceApply(@{@"player.headerStyle":@"glass",@"player.actionStyle":@"glass",@"player.material":@"regular",
        @"dock.material":@"regular"},nil);
    HGAppearanceLayoutDock(self.host.customTabBar,self.host);
    UIView *capsule=Find(self.host.customTabBar,@"honggo.appearance.dock.capsule");
    BOOL matchingGlass=YES;
    for(UIView *chrome in @[like,header,episodes]) {
        UIView *material=Find(chrome,@"honggo.appearance.material");
        Layout(material);
        if(HGTestOpenGlass(chrome))
            matchingGlass &= chrome==header ? HGTestFullGlassEffect((id)material) : HGTestButtonGlass(material);
        else {
            UIVisualEffectView *fallback=(id)material;
            matchingGlass &= [fallback isKindOfClass:UIVisualEffectView.class] &&
                (reducedTransparency ? !fallback.effect : [fallback.effect isKindOfClass:expectedEffect]);
        }
    }
    Check(matchingGlass,@"the channel rail uses full native glass and the action and episode entry use native glass buttons");
    UIView *episodeMaterial=Find(episodes,@"honggo.appearance.material");
    HGAppearanceSet(@"player.material",@"clear",nil);Layout(episodes);
    if(HGTestOpenGlass(episodes))Check(Find(episodes,@"honggo.appearance.material")==episodeMaterial &&
        HGTestButtonGlass(episodeMaterial),@"changing playback glass material updates one native button in place");
    HGAppearanceSet(@"player.material",@"regular",nil);
    HGAppearanceApply(@{@"player.headerStyle":@"flat",@"player.actionStyle":actionStyle},nil);
    UIVisualEffectView *dockMaterial=(id)Find(capsule,@"honggo.appearance.material");
    Layout(dockMaterial);
    Check(dockMaterial.alpha==1 && (HGTestOpenGlass(capsule) ? HGTestFullGlassEffect(dockMaterial) : !dockMaterial.maskView) &&
        (reducedTransparency ? !dockMaterial.effect : [dockMaterial.effect isKindOfClass:expectedEffect]),
        @"a custom-size playback Dock uses the full native glass surface");
    Check(!Find(capsule,@"honggo.appearance.dock.diffuser") && Count(capsule,@"honggo.appearance.material")==1,
        @"dock uses one native glass layer instead of stacking tinted system materials");
    UIVisualEffect *existingEffect=dockMaterial.effect;
    NSUInteger effectWrites=HGMaterialEffectSetterCount(dockMaterial);
    for(NSUInteger pass=0;pass<3;pass++){HGAppearanceLayoutDock(self.host.customTabBar,self.host);Layout(dockMaterial);}
    Check(HGMaterialEffectSetterCount(dockMaterial)==effectWrites &&
        (!existingEffect ? !dockMaterial.effect : [dockMaterial.effect isEqual:existingEffect]),
        @"stable dock layouts keep the same native glass effect");
    RunEpisodeGlassChecks(root);
    RunEpisodeSpacingChecks(root);
    RunPlayerChromeChecks(self.host);
    RunPlayerGestureChecks(self.host.view);
    RunPlaybackQualityChecks();
    RunAutoEnterSeriesChecks();
    RunEpisodeMediaChecks();
    RunCommentMovementChecks();
    RunLandscapeChromeChecks(self.host);

    SSShortVideoOutterSeriesInfoView *info=[[SSShortVideoOutterSeriesInfoView alloc] initWithFrame:CGRectMake(20,280,280,60)];[root addSubview:info];
    UILabel *rich=[[UILabel alloc] initWithFrame:CGRectMake(0,0,180,24)];
    NSDictionary *attributes=@{NSFontAttributeName:[UIFont systemFontOfSize:20],NSForegroundColorAttributeName:UIColor.systemRedColor};
    rich.attributedText=[[NSAttributedString alloc] initWithString:@"Rich text" attributes:attributes];[info addSubview:rich];
    UITextField *secure=[[UITextField alloc] initWithFrame:CGRectMake(0,30,180,24)];secure.secureTextEntry=YES;secure.font=[UIFont systemFontOfSize:18];[info addSubview:secure];
    HGAppearanceApply(@{@"fonts.enabled":@YES,@"fonts.scale":@0.75},nil);Layout(info);
    Check(fabs([(UIFont *)[rich.attributedText attribute:NSFontAttributeName atIndex:0 effectiveRange:nil] pointSize]-15)<0.01,@"rich text font runs scale");
    Check([[rich.attributedText attribute:NSForegroundColorAttributeName atIndex:0 effectiveRange:nil] isEqual:UIColor.systemRedColor],@"rich text colors survive typography customization");
    Check(secure.font.pointSize==18,@"secure text fields remain untouched");
    HGAppearanceSet(@"fonts.enabled",@NO,nil);
    Check(fabs([(UIFont *)[rich.attributedText attribute:NSFontAttributeName atIndex:0 effectiveRange:nil] pointSize]-20)<0.01,@"rich text restores its original font runs");

    FQVShortVideoPlayerCellView *canvas=[[FQVShortVideoPlayerCellView alloc] initWithFrame:CGRectMake(10,350,300,320)];[root addSubview:canvas];
    UILabel *fiction=[[UILabel alloc] initWithFrame:CGRectMake(-9,20,16,180)];
    fiction.text=@"剧情内容纯属\nAI\n虚构生成";[canvas addSubview:fiction];
    UILabel *filing=[[UILabel alloc] initWithFrame:CGRectMake(10,0,200,20)];
    filing.text=@"Ai（番茄）微短剧备字";[canvas addSubview:filing];
    HGAppearanceSet(@"player.hideAIFictionNotice",@YES,nil);Layout(canvas);
    Check(fiction.hidden && !filing.hidden,@"only the separate left edge fiction notice is hidden");
    fiction.text=@"其他文字";Layout(canvas);
    Check(!fiction.hidden,@"a reused label with ordinary text regains native visibility");
    fiction.text=@"剧情内容纯属AI虚构生成";Layout(canvas);
    HGAppearanceSet(@"player.hideAIFictionNotice",@NO,nil);Layout(canvas);
    Check(!fiction.hidden && CGRectEqualToRect(fiction.frame,CGRectMake(-9,20,16,180)),
        @"disabling the fiction notice preference restores the label without changing video framing");
    [fiction removeFromSuperview];[filing removeFromSuperview];
    TTPlayerView *render=[[TTPlayerView alloc] initWithFrame:CGRectMake(50,0,200,300)];[canvas addSubview:render];
    UIView *hud=[[UIView alloc] initWithFrame:CGRectMake(250,100,44,44)];[canvas addSubview:hud];CGRect hudFrame=hud.frame;
    HGAppearanceSet(@"player.fullscreen",@YES,nil);Layout(render);
    Check(CATransform3DIsIdentity(render.layer.sublayerTransform) &&
        CGAffineTransformIsIdentity(render.transform),@"landscape fill never enlarges portrait playback");
    Check(CGRectEqualToRect(hud.frame,hudFrame),@"the landscape setting preserves native HUD frame and hit target");
    HGAppearanceSet(@"player.fullscreen",@NO,nil);Check(CATransform3DIsIdentity(render.layer.sublayerTransform) && CGAffineTransformIsIdentity(render.transform),@"disabling landscape fill also leaves portrait playback native sized");
    render.frame=CGRectMake(0,70,300,170);CGRect wide=render.frame;
    HGAppearanceSet(@"player.fullscreen",@YES,nil);Layout(render);
    Check(CATransform3DIsIdentity(render.layer.sublayerTransform) && CGAffineTransformIsIdentity(render.transform) && CGRectEqualToRect(wide,render.frame),@"a wide video in the portrait feed preserves size and placement");
    render.frame=CGRectMake(0,0,300,300);Layout(render);
    Check(CATransform3DIsIdentity(render.layer.sublayerTransform) && CGAffineTransformIsIdentity(render.transform),@"square video remains native sized");
    RunPlaybackAppearanceChecks(root);
    RunAppearanceTransitionCoordinatorChecks(self.host);

    for(Class cls in @[FQCShortVideoCommentPanelViewController.class,SSVideoSharePanelViewController.class,FQVShortVideoSettingsViewController.class,FQVShortVideoNewSpeedSelectController.class]) {
        UIViewController *panel=[cls new];[self.host addChildViewController:panel];[panel loadViewIfNeeded];
        panel.view.frame=CGRectMake(0,300,root.bounds.size.width,250);panel.view.backgroundColor=UIColor.whiteColor;
        UIView *backdrop=[[UIView alloc] initWithFrame:panel.view.bounds];backdrop.backgroundColor=UIColor.whiteColor;[panel.view addSubview:backdrop];
        UILabel *label=[[UILabel alloc] initWithFrame:CGRectMake(10,10,100,30)];label.text=@"original";[backdrop addSubview:label];
        [root addSubview:panel.view];[panel didMoveToParentViewController:self.host];Layout(panel.view);[panel viewDidLayoutSubviews];
        Check(Find(panel.view,@"honggo.appearance.material")!=nil,@"each panel category receives its own native material");
        Check([label.text isEqual:@"original"] && label.superview==backdrop,@"panel decoration preserves content and hierarchy");
        HGAppearanceSet(@"enabled",@NO,nil);
        Check(Find(panel.view,@"honggo.appearance.material")==nil && [backdrop.backgroundColor isEqual:UIColor.whiteColor],@"panel reset restores all owned backgrounds");
        [panel.view removeFromSuperview];[panel removeFromParentViewController];HGAppearanceSet(@"enabled",@YES,nil);
    }
    RunActionShareAppearanceChecks(self.host);
    RunFeedShareRouteChecks();

    RunCommentSheetAppearanceChecks(self.host);

    SSTabBar *bar=self.host.customTabBar;HGAppearanceLayoutDock(bar,self.host);Layout(bar);
    HGAAppearanceDock *dock=(id)Find(bar,@"honggo.appearance.dock");
    Check(dock!=nil && bar.contentView.hidden,@"custom floating dock replaces only the verified native content");
    Check(Find(dock,@"honggo.appearance.dock.extra")!=nil,@"independent circular button is available");
    UIButton *theater=(id)Find(dock,@"honggo.appearance.tab.theater");
    [theater sendActionsForControlEvents:UIControlEventTouchUpInside];
    Check(self.host.nativeTaps==1 && self.host.selectedIndex==1,@"custom tab forwards exactly one native navigation action");
    HGAppearanceSet(@"dock.content",@"icon",nil);
    Check(theater.currentTitle.length==0 && theater.currentImage!=nil,@"icon-only mode supplies the tab icon");
    HGAppearanceSet(@"dock.content",@"text",nil);
    Check(theater.currentImage==nil && theater.currentTitle.length>0,@"text-only mode supplies the tab caption");
    HGAppearanceSet(@"dock.content",@"both",nil);
    Check(theater.currentImage!=nil && theater.currentTitle.length>0,@"mixed mode supplies both icon and caption");
    HGAppearanceSet(@"dock.hide.store",@YES,nil);
    Check(Find(dock,@"honggo.appearance.tab.store").hidden,@"individual tab visibility works");
    HGAppearanceSet(@"dock.height",@56,nil);
    Check(fabs(Find(dock,@"honggo.appearance.dock.capsule").bounds.size.height-56)<0.01,@"custom dock height respects available host space");
    HGAppearanceSet(@"dock.collapse",@YES,nil);[dock setCollapsed:YES animated:NO];
    UIButton *expand=(id)Find(dock,@"honggo.appearance.dock.expand");
    Check(!expand.hidden && expand.bounds.size.width>=44,@"collapsed dock exposes an accessible expansion button");
    [expand sendActionsForControlEvents:UIControlEventTouchUpInside];
    Check(expand.hidden && !Find(dock,@"honggo.appearance.dock.capsule").hidden,@"tapping collapsed dock expands it");
    for(NSUInteger i=0;i<5;i++)Layout(bar);
    Check(Count(bar,@"honggo.appearance.dock")==1,@"dock does not duplicate across repeated host layouts");
    HGRunDockAppearanceChecks(self.host);
    HGRunExpandedDockGeometryChecks(self.host);
    RunDockVideoBackdropChecks(self.host);
    HGAppearanceReset();Layout(like);Layout(bar);
    Check(CGRectEqualToRect(like.frame,original) && !like.hidden,@"reset restores original playback geometry and visibility");
    Check(like.iconImageView.image==nativeImage,@"reset restores original host image");
    if(like.iconImageView.image!=nativeImage)NSLog(@"FINAL ICON RESTORE native=%@ actual=%@",nativeImage,like.iconImageView.image);
    Check(Count(like,@"honggo.appearance.material")==0,@"reset removes playback materials");
    Check(!bar.contentView.hidden && !Find(bar,@"honggo.appearance.dock"),@"reset restores native tab content");
    HGAppearanceApply(HGAppearancePreset(@"glass"),nil);
    like.iconImageView.image=[UIImage systemImageNamed:@"leaf"];
    UIImage *hostUpdate=like.iconImageView.image;Layout(like);HGAppearanceReset();
    Check(ImageContentEqual(like.iconImageView.image,hostUpdate),@"a later native image becomes the restoration baseline");
    HGAppearanceApply(HGAppearancePreset(@"glass"),nil);like.model=nil;Layout(like);
    Check(ImageContentEqual(like.iconImageView.image,hostUpdate),@"missing host selection state retains the native icon");
    HGAppearanceReset();
    UIView *unrelated=[[UIView alloc] initWithFrame:CGRectMake(0,0,50,50)];unrelated.backgroundColor=UIColor.purpleColor;[root addSubview:unrelated];
    HGAppearanceApply(HGAppearancePreset(@"glass"),nil);HGAppearanceProcessView(unrelated);
    Check(unrelated.subviews.count==0 && [unrelated.backgroundColor isEqual:UIColor.purpleColor],@"unknown views remain untouched");
    Class settings=NSClassFromString(@"HGAAppearanceController");UITableViewController *page=[(UITableViewController *)[settings alloc] initWithStyle:UITableViewStyleInsetGrouped];
    [page loadViewIfNeeded];
    NSIndexPath *masterRow=HGMenuPath(page,@"enabled"),*playbackRow=HGMenuPath(page,@"@group.player");
    NSIndexPath *iconsRow=HGMenuPath(page,@"@group.icons"),*fontsRow=HGMenuPath(page,@"@group.fonts");
    Check(page.tableView.numberOfSections==3 && masterRow && masterRow.section==0 &&
        playbackRow.section==1 && !iconsRow && fontsRow.section==2 && !HGMenuPath(page,@"@group.backup"),
        @"appearance home groups the master switch, interface and playback, and icon and font settings");
    // Level 3 general page removes presets and pushed subpages never attach a nested UISearchController.
    UITableViewController *generalPage=[(UITableViewController *)[settings alloc] initWithStyle:UITableViewStyleInsetGrouped];
    [generalPage setValue:@"general" forKey:@"group"];[generalPage loadViewIfNeeded];
    Check([generalPage.tableView.dataSource tableView:generalPage.tableView numberOfRowsInSection:0]==1 &&
        generalPage.navigationItem.searchController==nil,
        @"appearance general page removes both presets and avoids nested UISearchController on pushed pages");
    // Level 3 playback layout shows clean per-role rows without inline offset/scale sliders.
    UITableViewController *playerPage=[(UITableViewController *)[settings alloc] initWithStyle:UITableViewStyleInsetGrouped];
    [playerPage setValue:@"player" forKey:@"group"];[playerPage loadViewIfNeeded];
    BOOL noInlineRoleSliders=YES;
    for (NSInteger section=0; section<playerPage.tableView.numberOfSections; section++) {
        NSInteger count=[playerPage.tableView numberOfRowsInSection:section];
        for (NSInteger r=0; r<count; r++) {
            UITableViewCell *cell=[playerPage tableView:playerPage.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:r inSection:section]];
            if ([cell isKindOfClass:NSClassFromString(@"HGAPercentCell")]) noInlineRoleSliders=NO;
        }
    }
    NSIndexPath *swipeSetting=HGMenuPath(playerPage,@"player.disableEpisodeSwipe");
    UITableViewCell *swipeCell=swipeSetting ? [playerPage tableView:playerPage.tableView cellForRowAtIndexPath:swipeSetting] : nil;
    UITableViewController *dramaPage=[(UITableViewController *)[settings alloc] initWithStyle:UITableViewStyleInsetGrouped];
    [dramaPage setValue:@"drama" forKey:@"group"];[dramaPage loadViewIfNeeded];
    Check(HGMenuPath(dramaPage,@"player.hide.followRank") && HGMenuPath(dramaPage,@"player.hide.seriesBadge") &&
        HGMenuPath(dramaPage,@"player.hideAIFictionNotice") && HGMenuPath(playerPage,@"player.highestQuality") &&
        HGMenuPath(playerPage,@"player.hideNowPlayingIsland") &&
        HGMenuPath(dramaPage,@"player.episodeDockWidth") && HGMenuPath(dramaPage,@"player.episodeGlass") &&
        HGMenuPath(playerPage,@"@group.rightButtons") && !HGMenuPath(playerPage,@"@group.role.collect") &&
        noInlineRoleSliders && swipeSetting && swipeSetting.section==0 && [swipeCell.accessoryView isKindOfClass:UISwitch.class],
        @"playback groups the six right-hand buttons while drama groups episode entry and badge settings");
    UITableViewController *rolePage=[(UITableViewController *)[settings alloc] initWithStyle:UITableViewStyleInsetGrouped];
    [rolePage setValue:@"role.avatar" forKey:@"group"];[rolePage loadViewIfNeeded];
    NSUInteger pctSliders=0;
    for (NSString *key in @[@"layout.avatar.x", @"layout.avatar.y", @"layout.avatar.scale"]) {
        NSIndexPath *path = HGMenuPath(rolePage,key);
        if (!path) continue;
        UITableViewCell *cell = [rolePage tableView:rolePage.tableView cellForRowAtIndexPath:path];
        cell.frame=CGRectMake(0,0,340,110);
        [cell layoutIfNeeded];
        UISlider *slider = (id)Find(cell,key);
        if ([slider isKindOfClass:UISlider.class] && slider.minimumValue==1 && slider.maximumValue==100 &&
            slider.continuous && CGRectGetWidth(slider.bounds)>250 && [slider.accessibilityValue containsString:@"%"]) pctSliders++;
    }
    Check(rolePage.tableView.numberOfSections==3 && pctSliders==3 && HGMenuPath(rolePage,@"@role.reset.avatar"),
        @"each control editor provides full-width sliders, visible percentages and reset");
    UISearchController *search=page.navigationItem.searchController;search.searchBar.text=@"layout.like";
    [search.searchResultsUpdater updateSearchResultsForSearchController:search];
    Check([page.tableView.dataSource tableView:page.tableView numberOfRowsInSection:0]==4,@"appearance search resolves stable per-control keys");
    search.searchBar.text=@"横屏铺满";
    [search.searchResultsUpdater updateSearchResultsForSearchController:search];
    Check([page.tableView.dataSource tableView:page.tableView numberOfRowsInSection:0]==1,
        @"the plugin settings expose one landscape fill switch");
    UITableViewCell *fillSetting=[page.tableView.dataSource tableView:page.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]];
    Check([fillSetting.accessibilityIdentifier isEqual:@"honggo.appearance.setting.player.fullscreen"] &&
        [fillSetting.accessoryView isKindOfClass:UISwitch.class],
        @"landscape fill is a settings switch instead of a playback-page button");
    search.searchBar.text=@"player.fillButton";
    [search.searchResultsUpdater updateSearchResultsForSearchController:search];
    Check([page.tableView.dataSource tableView:page.tableView numberOfRowsInSection:0]==0,
        @"removed fill button visibility is no longer offered in plugin settings");
    search.searchBar.text=@"TG Naloiafeed";
    [search.searchResultsUpdater updateSearchResultsForSearchController:search];
    Check([page.tableView.dataSource tableView:page.tableView numberOfRowsInSection:0]==1,@"appearance search finds the Telegram channel");
    UITableViewCell *channel=[page.tableView.dataSource tableView:page.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]];
    Check([channel.accessibilityIdentifier isEqual:@"honggo.appearance.setting.@community"],@"appearance channel keeps its stable action identity");
    BOOL wasAnimating = UIView.areAnimationsEnabled;
    [UIView setAnimationsEnabled:NO];
    HGRunAppearanceMenuNavigationChecks(self.window,settings);
    HGRunRightButtonSizeChecks(self.window,settings);
    [UIView setAnimationsEnabled:wasAnimating];
    HGAppearanceReset();
}
- (void)finish {
    NSUInteger failed=0;for(NSDictionary *c in checks)failed+=![c[@"passed"] boolValue];
    NSDictionary *report=@{@"plugin":@"Honggo",@"simulatorOS":UIDevice.currentDevice.systemVersion,@"version":HG_PLUGIN_VERSION,@"status":failed?@"failed":@"passed",@"total":@(checks.count),@"failed":@(failed),@"checks":checks,@"scope":@"production visual code with controlled UIKit hosts; not Hongguo device acceptance"};
    NSURL *directory=[NSFileManager.defaultManager URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask].firstObject;
    [NSFileManager.defaultManager createDirectoryAtURL:directory withIntermediateDirectories:YES attributes:nil error:nil];
    [[NSJSONSerialization dataWithJSONObject:report options:NSJSONWritingPrettyPrinted error:nil] writeToURL:[directory URLByAppendingPathComponent:@"appearance-results.json"] atomically:YES];
}
@end
@interface HGAAppearanceAppDelegate : UIResponder <UIApplicationDelegate> @end
@implementation HGAAppearanceAppDelegate
- (UISceneConfiguration *)application:(UIApplication *)app configurationForConnectingSceneSession:(UISceneSession *)session options:(UISceneConnectionOptions *)options {
    UISceneConfiguration *config=[[UISceneConfiguration alloc] initWithName:@"Appearance" sessionRole:session.role];config.delegateClass=HGAAppearanceTests.class;return config;
}
@end
int main(int argc,char *argv[]){@autoreleasepool{return UIApplicationMain(argc,argv,nil,NSStringFromClass(HGAAppearanceAppDelegate.class));}}
