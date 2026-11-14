// Exercise the production installer/replacements on the Objective-C runtime.
// These controlled host fixtures do not establish real Hongguo device behavior.
#import "Honggo.h"
#import <objc/message.h>
#import <QuartzCore/QuartzCore.h>

static NSUInteger forceRequests, feedRequests, manualEntries, patchRequests;
static NSUInteger gameModelCreations;
static NSUInteger manualGameEntries;

@interface SSGameService : NSObject
- (void)enterGame:(id)game;
@end
@implementation SSGameService
- (void)enterGame:(id)game { if (game) manualGameEntries++; }
@end

@interface SSShortVideoFeedDirectGameFeedPageHost : NSObject
@property(nonatomic, getter=isCurrentDisplaying) BOOL currentDisplaying;
@property(nonatomic, weak) id contentModel;
@end
@implementation SSShortVideoFeedDirectGameFeedPageHost
@end

@interface SSShortVideoFeedDirectGameSeriesEndInsertHost : SSShortVideoFeedDirectGameFeedPageHost
@end
@implementation SSShortVideoFeedDirectGameSeriesEndInsertHost
@end

@interface HGUnknownGameHost : SSShortVideoFeedDirectGameFeedPageHost
@end
@implementation HGUnknownGameHost
@end

@interface SSShortVideoFeedDirectGameContentView : UIView
@property(nonatomic) NSUInteger bindCalls;
@property(nonatomic) BOOL hiddenAtNativeBind;
@property(nonatomic) BOOL detachedAtNativeBind;
@property(nonatomic) BOOL hiddenAfterInitialization;
@property(nonatomic, strong) id boundModel;
@property(nonatomic, strong) UIView *defaultCTA;
- (void)bindContentModel:(id)model;
- (void)resetContent;
@end
@implementation SSShortVideoFeedDirectGameContentView
- (instancetype)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        // Native construction creates default content before any model bind.
        self.defaultCTA = [[UIView alloc] initWithFrame:self.bounds];
        self.defaultCTA.backgroundColor = UIColor.redColor;
        [self addSubview:self.defaultCTA];
    }
    return self;
}
- (void)resetContent {
    self.boundModel = nil;
    self.backgroundColor = UIColor.clearColor;
}
- (void)bindContentModel:(id)model {
    // The original bind must inherit the constructor's visibility, including
    // when a reused view resets its content before accepting another model.
    self.hiddenAtNativeBind = self.hidden;
    self.detachedAtNativeBind = self.superview == nil;
    self.bindCalls++;
    [self resetContent];
    self.boundModel = model;
    self.backgroundColor = UIColor.redColor;
}
@end

@interface SSShortVideoFeedDirectGameContentModel : NSObject
@property(nonatomic, strong) SSShortVideoFeedDirectGameFeedPageHost *host;
@property(nonatomic) BOOL displaying;
@property(nonatomic, getter=isInvalidated) BOOL invalidated;
@property(nonatomic) NSUInteger displayCalls;
@property(nonatomic) NSUInteger removalCalls;
- (void)modelDidDisplay;
- (void)handleTerminalFailureWithReason:(NSString *)reason;
- (UIView *)createInsertView;
@end
@implementation SSShortVideoFeedDirectGameContentModel
- (void)modelDidDisplay {
    self.displayCalls++;
    if (!self.invalidated) self.displaying = YES;
}
- (void)handleTerminalFailureWithReason:(NSString *)reason {
    self.removalCalls++;
    self.invalidated = YES;
}
- (UIView *)createInsertView {
    SSShortVideoFeedDirectGameContentView *view = [[SSShortVideoFeedDirectGameContentView alloc]
        initWithFrame:CGRectMake(0, 0, 32, 32)];
    view.hiddenAfterInitialization = view.hidden;
    [view bindContentModel:self];
    return view;
}
@end

// Controlled callers replay the two native wrapper orders. The mixed feed's
// content wrapper forwards setDataModel: to the common bind represented here.
@interface FQVShortVideoFeedPageCellView : UIView
@property(nonatomic, strong) SSShortVideoFeedDirectGameContentModel *model;
@property(nonatomic, strong) SSShortVideoFeedDirectGameContentView *gameView;
- (void)refreshUI;
@end
@implementation FQVShortVideoFeedPageCellView
- (void)refreshUI {
    self.gameView = [[SSShortVideoFeedDirectGameContentView alloc] initWithFrame:self.bounds];
    self.gameView.hiddenAfterInitialization = self.gameView.hidden;
    if (self.model.isInvalidated) [self.gameView resetContent];
    else [self.gameView bindContentModel:self.model];
    [self addSubview:self.gameView];
}
@end

@interface FQVShortVideoPageListCellView : UIView
@property(nonatomic, strong) SSShortVideoFeedDirectGameContentModel *model;
@property(nonatomic, strong) SSShortVideoFeedDirectGameContentView *gameView;
- (void)refreshUI;
@end
@implementation FQVShortVideoPageListCellView
- (void)refreshUI {
    self.gameView = (SSShortVideoFeedDirectGameContentView *)[self.model createInsertView];
    [self addSubview:self.gameView];
}
@end

static BOOL HGGameWrapperHasRedPixel(UIView *wrapper) {
    unsigned char pixel[4] = {0};
    CGColorSpaceRef colors = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(pixel, 1, 1, 8, 4, colors,
        kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
    CGContextTranslateCTM(context, -16, -16);
    [wrapper.layer renderInContext:context];
    BOOL red = pixel[0] > 200 && pixel[1] < 30 && pixel[2] < 30;
    CGContextRelease(context);
    CGColorSpaceRelease(colors);
    return red;
}

static SSShortVideoFeedDirectGameContentModel *HGDisplayingGame(Class hostType) {
    SSShortVideoFeedDirectGameContentModel *model = [SSShortVideoFeedDirectGameContentModel new];
    model.host = [hostType new];
    model.host.currentDisplaying = YES;
    model.host.contentModel = model;
    [model modelDidDisplay];
    return model;
}

@interface SSVideoMiniGameService : NSObject
- (id)createFeedPageDataModelWithCellViewData:(id)cell listVCContext:(id)context;
- (id)createDirectGameContentModelWithCellViewData:(id)cell source:(NSInteger)source host:(id)host;
@end
@implementation SSVideoMiniGameService
- (id)createFeedPageDataModelWithCellViewData:(id)cell listVCContext:(id)context {
    gameModelCreations++; return cell;
}
- (id)createDirectGameContentModelWithCellViewData:(id)cell source:(NSInteger)source host:(id)host {
    return cell;
}
@end

@interface SSBizVideoABTestHelper : NSObject
+ (BOOL)enableSeriesEndDirectGame;
@end
@implementation SSBizVideoABTestHelper
+ (BOOL)enableSeriesEndDirectGame { return YES; }
@end

// Replay the observed f599be0 -> f599bfc nil guard before append, keeping
// neighboring drama entries. This is a controlled caller, not the host binary.
static NSArray *HGGameFeedFixture(SSVideoMiniGameService *service) {
    NSMutableArray *models = [NSMutableArray array];
    for (NSString *cell in @[@"drama-1", @"game-legacy", @"game-native", @"drama-2"]) {
        id model = [cell hasPrefix:@"game-"] ?
            [service createFeedPageDataModelWithCellViewData:cell listVCContext:@"context"] : cell;
        if (model) [models addObject:model];
    }
    return models;
}

@interface SSBizVideoAdABTestHelper : NSObject
+ (BOOL)enableEcomForceCard;
@end
@implementation SSBizVideoAdABTestHelper
+ (BOOL)enableEcomForceCard { forceRequests++; return YES; }
@end

@interface SSShortVideoLiveService : NSObject
- (id)createFeedLiveInsertModelWithRoomModel:(id)room;
- (id)createDirectLiveRoomInnerListWithParamsMaker:(id)params;
@end
@implementation SSShortVideoLiveService
- (id)createFeedLiveInsertModelWithRoomModel:(id)room { return room; }
- (id)createDirectLiveRoomInnerListWithParamsMaker:(id)params { return params; }
@end

@interface SSShortVideoRecommendOutterListDataSource : NSObject
- (id)liveFeedDataModelWithCellViewData:(id)cell;
- (id)videoDataModelWithCellViewData:(id)cell;
@end
@implementation SSShortVideoRecommendOutterListDataSource
- (id)liveFeedDataModelWithCellViewData:(id)cell { return cell; }
- (id)videoDataModelWithCellViewData:(id)cell { return cell; }
@end

@interface IESLiveUGBenefitCouponFragment : NSObject
- (BOOL)shouldShowBalanceCouponPopup;
- (BOOL)shouldShowExpansionCouponPopup;
@end
@implementation IESLiveUGBenefitCouponFragment
- (BOOL)shouldShowBalanceCouponPopup { return YES; }
- (BOOL)shouldShowExpansionCouponPopup { return YES; }
@end

@interface SSCouponPopupFrequencyHelper : NSObject
+ (BOOL)canShowPopup:(id)popup;
@end
@implementation SSCouponPopupFrequencyHelper
+ (BOOL)canShowPopup:(id)popup { return popup != nil; }
@end

@interface BDADBackPatchAdService : NSObject
- (BOOL)shouldShowAdWithContext:(id)context;
- (BOOL)shouldRequestAdWithPlaybackTime:(double)time duration:(double)duration context:(id)context;
@end
@implementation BDADBackPatchAdService
- (BOOL)shouldShowAdWithContext:(id)context { return context != nil; }
- (BOOL)shouldRequestAdWithPlaybackTime:(double)time duration:(double)duration context:(id)context {
    return time < duration && context != nil;
}
@end

@interface BDADPlotTouchAdService : NSObject
- (BOOL)canShowPlotAdWithContext:(id)context;
- (BOOL)canRequestAdWithPlaybackTime:(double)time context:(id)context;
@end
@implementation BDADPlotTouchAdService
- (BOOL)canShowPlotAdWithContext:(id)context { return context != nil; }
- (BOOL)canRequestAdWithPlaybackTime:(double)time context:(id)context { return time > 0 && context != nil; }
@end

@interface BDADShortVideoSideBarPatchSwitchComponent : NSObject
- (BOOL)isAvailableWithCtx:(id)context;
- (BOOL)shouldShowWithCtx:(id)context;
@end
@implementation BDADShortVideoSideBarPatchSwitchComponent
- (BOOL)isAvailableWithCtx:(id)context { return context != nil; }
- (BOOL)shouldShowWithCtx:(id)context { return [self isAvailableWithCtx:context]; }
@end

@interface SSShortVideoAdService : NSObject
- (BOOL)shouldDisableCurrentShortVideoFeedAdByBizRulesWithContext:(id)context;
- (void)enterLiveRoom;
@end
@implementation SSShortVideoAdService
- (BOOL)shouldDisableCurrentShortVideoFeedAdByBizRulesWithContext:(id)context {
    feedRequests++; return NO;
}
- (void)enterLiveRoom { manualEntries++; }
@end

// Deliberately incompatible ABI: the installer must preserve this implementation.
@interface SSBizVideoAdCommonAdClient : NSObject
- (NSInteger)shouldRequestShortVideoAd:(id)context;
@end
@implementation SSBizVideoAdCommonAdClient
- (NSInteger)shouldRequestShortVideoAd:(id)context { return 42; }
@end

@interface SSBizVideoPatchAdService : NSObject
- (void)showPatchAdViewWithContext:(id)context seriesId:(id)series vId:(id)video
    currentTime:(double)time patchAdViewReadyBlock:(void (^)(id))ready removeBlock:(void (^)(void))remove;
@end
@implementation SSBizVideoPatchAdService
- (void)showPatchAdViewWithContext:(id)context seriesId:(id)series vId:(id)video
    currentTime:(double)time patchAdViewReadyBlock:(void (^)(id))ready removeBlock:(void (^)(void))remove {
    patchRequests++;
    if (ready) ready(@"host-ad");
}
@end

@interface HGAdsTests : UIResponder <UIWindowSceneDelegate>
@property(nonatomic, strong) UIWindow *window;
@property(nonatomic, strong) NSMutableArray<NSDictionary *> *checks;
@end
@implementation HGAdsTests
- (void)check:(BOOL)passed name:(NSString *)name {
    [self.checks addObject:@{@"name": name, @"passed": @(passed)}];
    NSLog(@"[HonggoAdsTests] %@: %@", passed ? @"PASS" : @"FAIL", name);
}
- (void)checkGameBindingHidden:(BOOL)hidden {
    NSArray<UIView *> *wrappers = @[
        [[FQVShortVideoFeedPageCellView alloc] initWithFrame:CGRectMake(0, 0, 32, 32)],
        [[FQVShortVideoPageListCellView alloc] initWithFrame:CGRectMake(0, 0, 32, 32)]
    ];
    NSArray<NSString *> *names = @[@"mixed feed", @"series-end insert"];
    for (NSUInteger index = 0; index < wrappers.count; index++) {
        UIView *wrapper = wrappers[index];
        wrapper.backgroundColor = UIColor.blackColor;
        [self.window.rootViewController.view addSubview:wrapper];
        SSShortVideoFeedDirectGameContentModel *model = [SSShortVideoFeedDirectGameContentModel new];
        [(id)wrapper setModel:model];
        [(id)wrapper refreshUI];
        SSShortVideoFeedDirectGameContentView *view = [(id)wrapper gameView];
        NSString *state = hidden ? @"enabled" : @"disabled";
        [self check:view.defaultCTA.superview == view && view.hiddenAfterInitialization == hidden &&
            view.bindCalls == 1 && view.detachedAtNativeBind && view.hiddenAtNativeBind == hidden
            name:[NSString stringWithFormat:@"%@ %@ filtering preserves expected visibility from initialization through original binding", names[index], state]];
        [self check:view.superview == wrapper && view.window == self.window &&
            view.hidden == hidden && view.boundModel == model && model.removalCalls == 0
            name:[NSString stringWithFormat:@"%@ %@ filtering preserves native binding and attachment before cleanup", names[index], state]];
        [self check:HGGameWrapperHasRedPixel(wrapper) == !hidden
            name:[NSString stringWithFormat:@"%@ %@ filtering has the expected first synchronous rendered pixel", names[index], state]];
        if (hidden) {
            UIView *sentinel = [[UIView alloc] initWithFrame:view.bounds];
            sentinel.backgroundColor = UIColor.redColor;
            [view addSubview:sentinel];
            [self check:sentinel.superview == view && !sentinel.hidden && view.hidden &&
                !HGGameWrapperHasRedPixel(wrapper) && model.displayCalls == 0 && model.removalCalls == 0
                name:[NSString stringWithFormat:@"%@ children mounted after attachment remain invisible before display or cleanup", names[index]]];
            SSShortVideoFeedDirectGameContentModel *next = [SSShortVideoFeedDirectGameContentModel new];
            [view bindContentModel:next];
            [self check:view.bindCalls == 2 && view.hiddenAtNativeBind && view.hidden &&
                view.boundModel == next && !HGGameWrapperHasRedPixel(wrapper)
                name:[NSString stringWithFormat:@"%@ reuse preserves invisibility while binding the next model", names[index]]];
            [view bindContentModel:nil];
            [self check:view.bindCalls == 3 && view.hiddenAtNativeBind && view.hidden &&
                view.boundModel == nil && !HGGameWrapperHasRedPixel(wrapper)
                name:[NSString stringWithFormat:@"%@ clearing the model binding does not expose the mounted subtree", names[index]]];
        }
        [wrapper removeFromSuperview];
    }
    FQVShortVideoFeedPageCellView *invalidWrapper = [[FQVShortVideoFeedPageCellView alloc]
        initWithFrame:CGRectMake(0, 0, 32, 32)];
    invalidWrapper.backgroundColor = UIColor.blackColor;
    invalidWrapper.model = [SSShortVideoFeedDirectGameContentModel new];
    invalidWrapper.model.invalidated = YES;
    [self.window.rootViewController.view addSubview:invalidWrapper];
    [invalidWrapper refreshUI];
    SSShortVideoFeedDirectGameContentView *unbound = invalidWrapper.gameView;
    NSString *state = hidden ? @"enabled" : @"disabled";
    [self check:unbound.defaultCTA.superview == unbound && !unbound.defaultCTA.hidden &&
        unbound.bindCalls == 0 && unbound.boundModel == nil && unbound.hiddenAfterInitialization == hidden &&
        unbound.hidden == hidden && unbound.superview == invalidWrapper && unbound.window == self.window &&
        invalidWrapper.model.displayCalls == 0 && invalidWrapper.model.removalCalls == 0
        name:[NSString stringWithFormat:@"invalidated mixed card with %@ filtering attaches default content without ever binding a model", state]];
    [self check:HGGameWrapperHasRedPixel(invalidWrapper) == !hidden
        name:[NSString stringWithFormat:@"invalidated mixed card with %@ filtering has the expected default-CTA pixel before any cleanup", state]];
    [invalidWrapper removeFromSuperview];
    if (hidden) {
        UIView *ordinary = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 32, 32)];
        ordinary.backgroundColor = UIColor.redColor;
        [self.window.rootViewController.view addSubview:ordinary];
        [self check:!ordinary.hidden && HGGameWrapperHasRedPixel(ordinary)
            name:@"unrelated ordinary content remains visible with feed filtering enabled"];
        [ordinary removeFromSuperview];
    }
}
- (void)checkDisplayedGamesWithDisabledModel:(SSShortVideoFeedDirectGameContentModel *)disabled {
    SSShortVideoFeedDirectGameContentModel *feed = HGDisplayingGame(SSShortVideoFeedDirectGameFeedPageHost.class);
    [feed modelDidDisplay];
    SSShortVideoFeedDirectGameContentModel *series = HGDisplayingGame(SSShortVideoFeedDirectGameSeriesEndInsertHost.class);
    SSShortVideoFeedDirectGameContentModel *scrolled = HGDisplayingGame(SSShortVideoFeedDirectGameFeedPageHost.class);
    scrolled.host.currentDisplaying = NO;
    SSShortVideoFeedDirectGameContentModel *ended = HGDisplayingGame(SSShortVideoFeedDirectGameSeriesEndInsertHost.class);
    ended.displaying = NO;
    SSShortVideoFeedDirectGameContentModel *rebound = HGDisplayingGame(SSShortVideoFeedDirectGameFeedPageHost.class);
    rebound.host.contentModel = series;
    SSShortVideoFeedDirectGameContentModel *invalidated = HGDisplayingGame(SSShortVideoFeedDirectGameFeedPageHost.class);
    invalidated.invalidated = YES;
    SSShortVideoFeedDirectGameContentModel *unknown = HGDisplayingGame(HGUnknownGameHost.class);
    [self check:feed.displayCalls == 2 && series.displayCalls == 1 && feed.removalCalls == 0 && series.removalCalls == 0
        name:@"game display runs the original and defers list removal"];
    dispatch_async(dispatch_get_main_queue(), ^{
        [self check:feed.removalCalls == 1 && series.removalCalls == 1
            name:@"each supported current card uses the host removal path once"];
        [self check:scrolled.removalCalls == 0 && ended.removalCalls == 0 && rebound.removalCalls == 0 && invalidated.removalCalls == 0
            name:@"scheduled removal skips a transitioned, rebound, or invalidated card"];
        [self check:unknown.removalCalls == 0 && disabled.removalCalls == 0
            name:@"unknown hosts and a display before hook installation keep host behavior"];
        [self finish];
    });
}
- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session
        options:(UISceneConnectionOptions *)options {
    self.window = [[UIWindow alloc] initWithWindowScene:(UIWindowScene *)scene];
    self.window.rootViewController = [UIViewController new];
    [self.window makeKeyAndVisible];
    self.checks = [NSMutableArray array];
    NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
    [defaults removePersistentDomainForName:NSBundle.mainBundle.bundleIdentifier];
    SSShortVideoAdService *feed = [SSShortVideoAdService new];
    SSBizVideoAdCommonAdClient *mismatch = [SSBizVideoAdCommonAdClient new];
    SSBizVideoPatchAdService *patch = [SSBizVideoPatchAdService new];
    SSShortVideoLiveService *live = [SSShortVideoLiveService new];
    SSShortVideoRecommendOutterListDataSource *mixed = [SSShortVideoRecommendOutterListDataSource new];
    IESLiveUGBenefitCouponFragment *coupon = [IESLiveUGBenefitCouponFragment new];
    BDADBackPatchAdService *back = [BDADBackPatchAdService new];
    BDADPlotTouchAdService *plot = [BDADPlotTouchAdService new];
    BDADShortVideoSideBarPatchSwitchComponent *sidebar = [BDADShortVideoSideBarPatchSwitchComponent new];
    SSVideoMiniGameService *games = [SSVideoMiniGameService new];
    Method gameFactory = class_getInstanceMethod(SSVideoMiniGameService.class,
        @selector(createFeedPageDataModelWithCellViewData:listVCContext:));
    IMP originalGameFactory = method_getImplementation(gameFactory);
    Method seriesGame = class_getClassMethod(SSBizVideoABTestHelper.class, @selector(enableSeriesEndDirectGame));
    IMP originalSeriesGame = method_getImplementation(seriesGame);
    Method force = class_getClassMethod(SSBizVideoAdABTestHelper.class, @selector(enableEcomForceCard));
    IMP originalForce = method_getImplementation(force);
    Method wrongABI = class_getInstanceMethod(SSBizVideoAdCommonAdClient.class, @selector(shouldRequestShortVideoAd:));
    IMP originalWrongABI = method_getImplementation(wrongABI);
    HGInstallAds();
    [self checkGameBindingHidden:NO];
    SSShortVideoFeedDirectGameContentModel *disabledGame = HGDisplayingGame(SSShortVideoFeedDirectGameFeedPageHost.class);
    [self check:[[games createDirectGameContentModelWithCellViewData:@"game" source:1 host:nil] isEqual:@"game"] &&
        [[games createDirectGameContentModelWithCellViewData:@"game" source:2 host:nil] isEqual:@"game"]
        name:@"disabled feed filtering preserves direct-game feed and series models"];
    [self check:[HGGameFeedFixture(games) isEqual:@[@"drama-1", @"game-legacy", @"game-native", @"drama-2"]]
        name:@"disabled feed filtering preserves legacy and native game cards in order"];
    [self check:[SSBizVideoABTestHelper enableSeriesEndDirectGame]
        name:@"disabled filtering preserves the series-end game decision"];
    [defaults setBool:YES forKey:@"HongguoGlass.miniGamePendant"];
    HGInstallAds();
    [self check:[[games createFeedPageDataModelWithCellViewData:@"game-legacy" listVCContext:nil] isEqual:@"game-legacy"]
        name:@"the bookstore pendant preference does not implicitly filter feed games"];
    NSUInteger gameCreationsBeforeFiltering = gameModelCreations;
    [self check:[[live createFeedLiveInsertModelWithRoomModel:@"room"] isEqual:@"room"]
        name:@"disabled filtering preserves client inserted live models"];
    [self check:[[mixed liveFeedDataModelWithCellViewData:@"live-cell"] isEqual:@"live-cell"]
        name:@"disabled filtering preserves server mixed live models"];
    [self check:[coupon shouldShowBalanceCouponPopup] && [coupon shouldShowExpansionCouponPopup]
        name:@"disabled marketing filtering preserves automatic coupon decisions"];
    [self check:[SSCouponPopupFrequencyHelper canShowPopup:@"popup"]
        name:@"disabled startup commerce keeps its coupon queue eligibility"];
    [self check:[back shouldShowAdWithContext:@"context"] && [plot canShowPlotAdWithContext:@"context"] &&
        [plot canRequestAdWithPlaybackTime:19.75 context:@"context"] &&
        [sidebar isAvailableWithCtx:@"context"] && [sidebar shouldShowWithCtx:@"context"]
        name:@"disabled patch filtering preserves return, plot and sidebar branches"];
    [self check:[SSBizVideoAdABTestHelper enableEcomForceCard] && forceRequests == 1
        name:@"disabled feed filtering preserves the host commerce decision"];
    [self check:![feed shouldDisableCurrentShortVideoFeedAdByBizRulesWithContext:nil] && feedRequests == 1
        name:@"disabled filtering preserves the host feed path"];
    __block id readyValue = nil;
    [patch showPatchAdViewWithContext:nil seriesId:nil vId:nil currentTime:12.5
        patchAdViewReadyBlock:^(id value) { readyValue = value; } removeBlock:nil];
    [self check:[readyValue isEqual:@"host-ad"] && patchRequests == 1
        name:@"disabled patch filtering preserves the original ready callback"];

    [defaults setBool:YES forKey:@"HongguoGlass.feed"];
    [defaults setBool:YES forKey:@"HongguoGlass.patch"];
    HGInstallAds();
    [self checkGameBindingHidden:YES];
    [self check:[games createFeedPageDataModelWithCellViewData:@"game-legacy" listVCContext:nil] == nil &&
        [games createFeedPageDataModelWithCellViewData:@"game-native" listVCContext:@"context"] == nil &&
        gameModelCreations == gameCreationsBeforeFiltering
        name:@"both game feed variants are declined before host model construction"];
    [self check:[HGGameFeedFixture(games) isEqual:@[@"drama-1", @"drama-2"]]
        name:@"the caller nil guard omits game cards without empty entries or reordering drama"];
    [self check:![SSBizVideoABTestHelper enableSeriesEndDirectGame]
        name:@"series-end game insertion takes its disabled preparation branch"];
    [self check:[games createDirectGameContentModelWithCellViewData:@"game" source:1 host:nil] == nil &&
        [games createDirectGameContentModelWithCellViewData:@"game" source:2 host:nil] == nil
        name:@"both direct-game callers receive the existing nil outcome"];
    [[SSGameService new] enterGame:@"selected-game"];
    [self check:manualGameEntries == 1 name:@"an independently selected game still reaches the launch service"];
    [self check:[live createFeedLiveInsertModelWithRoomModel:@"room"] == nil
        name:@"client inserted live models are declined before queueing"];
    [self check:[mixed liveFeedDataModelWithCellViewData:@"live-cell"] == nil
        name:@"server mixed live conversion uses the caller nil-skip path"];
    [self check:[[mixed videoDataModelWithCellViewData:@"drama-cell"] isEqual:@"drama-cell"]
        name:@"ordinary drama conversion is not hooked"];
    [self check:[[live createDirectLiveRoomInnerListWithParamsMaker:@"manual-room"] isEqual:@"manual-room"]
        name:@"explicit live room creation stays available"];
    [self check:[coupon shouldShowBalanceCouponPopup] && [coupon shouldShowExpansionCouponPopup]
        name:@"feed filtering does not activate the separate coupon preference"];
    [self check:![back shouldShowAdWithContext:@"context"] && ![plot canShowPlotAdWithContext:@"context"]
        name:@"patch filtering declines return and plot display at their business gates"];
    [self check:![back shouldRequestAdWithPlaybackTime:19.75 duration:60.5 context:@"context"]
        name:@"the two doubles plus object ABI declines return-ad preloading"];
    [self check:![plot canRequestAdWithPlaybackTime:19.75 context:@"context"]
        name:@"the double plus object ABI safely declines plot requests"];
    [self check:![sidebar isAvailableWithCtx:@"context"] && ![sidebar shouldShowWithCtx:@"context"]
        name:@"sidebar filtering covers availability and the bypass-capable display gate"];
    [self check:[SSCouponPopupFrequencyHelper canShowPopup:@"popup"]
        name:@"patch filtering keeps the separate startup commerce preference"];
    [defaults setBool:YES forKey:@"HongguoGlass.commerce"];
    [defaults setBool:YES forKey:@"HongguoGlass.marketing"];
    HGInstallAds();
    [self check:![coupon shouldShowBalanceCouponPopup] && ![coupon shouldShowExpansionCouponPopup]
        name:@"marketing filtering declines both automatic coupon requests"];
    [self check:![SSCouponPopupFrequencyHelper canShowPopup:@"popup"]
        name:@"startup commerce filtering uses the coupon queue reject branch"];
    [self check:![SSBizVideoAdABTestHelper enableEcomForceCard] && forceRequests == 1
        name:@"feed filtering disables forced commerce before its request path"];
    [self check:[feed shouldDisableCurrentShortVideoFeedAdByBizRulesWithContext:@"context"] && feedRequests == 1
        name:@"existing feed suppression still returns the host skip decision"];
    [self check:[mismatch shouldRequestShortVideoAd:nil] == 42 &&
        method_getImplementation(wrongABI) == originalWrongABI
        name:@"an incompatible host ABI remains untouched"];
    [feed enterLiveRoom];
    [self check:manualEntries == 1 name:@"manual live entry is not hooked"];
    __block NSUInteger readyCalls = 0, removeCalls = 0;
    readyValue = @"sentinel";
    [patch showPatchAdViewWithContext:@"context" seriesId:@"series" vId:@"video" currentTime:19.75
        patchAdViewReadyBlock:^(id value) { readyCalls++; readyValue = value; }
        removeBlock:^{ removeCalls++; }];
    [self check:readyCalls == 1 && readyValue == nil && patchRequests == 1
        name:@"declined patch completes exactly once with no ad"];
    [self check:removeCalls == 0 name:@"declined patch does not run an unshown-ad removal callback"];
    [patch showPatchAdViewWithContext:nil seriesId:nil vId:nil currentTime:0
        patchAdViewReadyBlock:nil removeBlock:nil];
    [self check:patchRequests == 1 name:@"optional absent completion does not invoke the host ad"];
    // Restore the fixture's host IMP to model a fresh process after turning off.
    method_setImplementation(force, originalForce);
    method_setImplementation(gameFactory, originalGameFactory);
    method_setImplementation(seriesGame, originalSeriesGame);
    [defaults setBool:NO forKey:@"HongguoGlass.feed"];
    HGInstallAds();
    [self check:[SSBizVideoAdABTestHelper enableEcomForceCard] && forceRequests == 2
        name:@"a fresh host implementation stays enabled after turning the option off"];
    [self check:[SSBizVideoABTestHelper enableSeriesEndDirectGame] &&
        [HGGameFeedFixture(games) isEqual:@[@"drama-1", @"game-legacy", @"game-native", @"drama-2"]]
        name:@"fresh host game implementations remain original after disabling feed filtering"];
    [self checkDisplayedGamesWithDisabledModel:disabledGame];
}
- (void)finish {
    NSUInteger failed = 0;
    for (NSDictionary *check in self.checks) failed += ![check[@"passed"] boolValue];
    NSDictionary *report = @{@"plugin": HG_PLUGIN_NAME, @"version": HG_PLUGIN_VERSION,
        @"scope": @"production hooks with controlled Objective-C host fixtures; not device acceptance",
        @"status": failed ? @"failed" : @"passed", @"failed": @(failed),
        @"total": @(self.checks.count), @"checks": self.checks};
    NSURL *directory = [NSFileManager.defaultManager URLsForDirectory:NSDocumentDirectory
        inDomains:NSUserDomainMask].firstObject;
    NSData *data = [NSJSONSerialization dataWithJSONObject:report options:NSJSONWritingPrettyPrinted error:nil];
    [data writeToURL:[directory URLByAppendingPathComponent:@"ads-results.json"] atomically:YES];
}
@end

@interface HGAdsAppDelegate : UIResponder <UIApplicationDelegate>
@end
@implementation HGAdsAppDelegate
- (UISceneConfiguration *)application:(UIApplication *)application
        configurationForConnectingSceneSession:(UISceneSession *)session
        options:(UISceneConnectionOptions *)options {
    UISceneConfiguration *configuration = [[UISceneConfiguration alloc]
        initWithName:@"HonggoAdsTest" sessionRole:session.role];
    configuration.delegateClass = HGAdsTests.class;
    return configuration;
}
@end

int main(int argc, char *argv[]) {
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass(HGAdsAppDelegate.class));
    }
}
