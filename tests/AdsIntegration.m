// Exercise the production installer/replacements on the Objective-C runtime.
// These controlled host fixtures do not establish real Hongguo device behavior.
#import "Honggo.h"
#import <objc/message.h>
#include <stdint.h>
#import <QuartzCore/QuartzCore.h>

static NSUInteger forceRequests, feedRequests, manualEntries;
static NSUInteger gameModelCreations;
static NSUInteger manualGameEntries;
static NSUInteger nativeGameInitializations;

@interface SSVideoSeriesFeedSideBarViewController : UIViewController
@property(nonatomic, strong) id gameEntranceData;
@end
@implementation SSVideoSeriesFeedSideBarViewController
@end

@interface SSVideoSeriesFeedSideBarV725ViewController : UIViewController
- (BOOL)showsGameSection;
@end
@implementation SSVideoSeriesFeedSideBarV725ViewController
- (BOOL)showsGameSection { return YES; }
@end

@interface SSMyUser621SideBarViewController : UIViewController
@property(nonatomic, copy) NSArray *componentsLayout;
@end
@implementation SSMyUser621SideBarViewController
@end

// Configure host classification outcomes without duplicating its JSON parser.
@interface SSNaturalAdData : NSObject
@property(nonatomic) NSInteger forceWatchTime;
@property(nonatomic) BOOL gameAfterParsing;
@property(nonatomic, getter=isGameContent) BOOL gameContent;
@property(nonatomic) NSUInteger parseCalls;
- (void)parseGameDict;
@end
@implementation SSNaturalAdData
- (void)parseGameDict {
    self.parseCalls++;
    self.gameContent = self.gameAfterParsing;
}
@end

@interface HGUnexpectedNaturalAdData : SSNaturalAdData
@end
@implementation HGUnexpectedNaturalAdData
- (void)parseGameDict {
    self.parseCalls++;
    self.gameContent = YES;
}
@end

@interface SSNaturalAdManager : NSObject
@property(nonatomic) NSUInteger originalCalls;
@property(nonatomic) BOOL originalResult;
@property(nonatomic, strong) id lastArgument;
- (BOOL)tryInsertDataToSaaS:(id)data;
@end
@implementation SSNaturalAdManager
- (BOOL)tryInsertDataToSaaS:(id)data {
    self.originalCalls++;
    self.lastArgument = data;
    return self.originalResult;
}
@end

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
        nativeGameInitializations++;
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

// These paths intentionally exercise different callback contracts. They are
// controlled fixtures, not a reconstruction of the host's playback state machine.
typedef NS_ENUM(NSUInteger, HGPatchFixturePath) {
    HGPatchFixtureSynchronous,
    HGPatchFixtureEarlyDecline,
    HGPatchFixtureDeferred,
};

@interface SSBizVideoPatchAdService : NSObject
@property(nonatomic) HGPatchFixturePath fixturePath;
@property(nonatomic) NSUInteger originalCalls;
@property(nonatomic, strong) NSMutableArray<NSString *> *lifecycle;
@property(nonatomic, strong) id receivedContext;
@property(nonatomic, strong) id receivedSeries;
@property(nonatomic, strong) id receivedVideo;
@property(nonatomic) double receivedTime;
@property(nonatomic) SEL receivedSelector;
@property(nonatomic) uintptr_t receivedReady;
@property(nonatomic) uintptr_t receivedRemove;
@property(nonatomic, strong) NSThread *receivedThread;
- (BOOL)shouldSkipPatchAdWithContext:(id)context;
- (void)showPatchAdViewWithContext:(id)context seriesId:(id)series vId:(id)video
    currentTime:(double)time patchAdViewReadyBlock:(void (^)(id))ready removeBlock:(void (^)(void))remove;
@end
@implementation SSBizVideoPatchAdService
- (instancetype)init {
    if ((self = [super init])) _lifecycle = [NSMutableArray array];
    return self;
}
- (BOOL)shouldSkipPatchAdWithContext:(id)context { return NO; }
- (void)showPatchAdViewWithContext:(id)context seriesId:(id)series vId:(id)video
    currentTime:(double)time patchAdViewReadyBlock:(void (^)(id))ready removeBlock:(void (^)(void))remove {
    self.originalCalls++;
    self.receivedContext = context;
    self.receivedSeries = series;
    self.receivedVideo = video;
    self.receivedTime = time;
    self.receivedSelector = _cmd;
    self.receivedReady = (uintptr_t)(__bridge void *)ready;
    self.receivedRemove = (uintptr_t)(__bridge void *)remove;
    self.receivedThread = NSThread.currentThread;
    [self.lifecycle addObject:@"entry"];
    if (self.fixturePath == HGPatchFixtureEarlyDecline) {
        [self.lifecycle addObject:@"decline"];
        [self.lifecycle addObject:@"return"];
        return;
    }
    if (self.fixturePath == HGPatchFixtureDeferred) {
        dispatch_async(dispatch_get_main_queue(), ^{
            [self.lifecycle addObject:@"ready"];
            if (ready) ready(@"host-ad");
            [self.lifecycle addObject:@"remove"];
            if (remove) remove();
        });
    } else {
        [self.lifecycle addObject:@"ready"];
        if (ready) ready(@"host-ad");
    }
    [self.lifecycle addObject:@"return"];
}
@end

typedef UIView *(*HGFixtureGameInitializer)(UIView * __attribute__((ns_consumed)), SEL, CGRect)
    __attribute__((ns_returns_retained));
static HGFixtureGameInitializer HGNextGameInitializer;
static BOOL (*HGNextNaturalInsertion)(id, SEL, id);
static void (*HGNextPatchPresentation)(id, SEL, id, id, id, double,
    void (^)(id), void (^)(void));
static NSUInteger foreignGameInitializations, foreignNaturalInsertions, foreignPatchPresentations;

__attribute__((ns_returns_retained))
static UIView *HGForeignGameInitializer(UIView * __attribute__((ns_consumed)) self, SEL cmd, CGRect frame) {
    foreignGameInitializations++;
    return HGNextGameInitializer(self, cmd, frame);
}

static BOOL HGForeignNaturalInsertion(id self, SEL cmd, id data) {
    foreignNaturalInsertions++;
    return HGNextNaturalInsertion(self, cmd, data);
}

static void HGForeignPatchPresentation(id self, SEL cmd, id context, id series, id video,
        double time, void (^ready)(id), void (^remove)(void)) {
    foreignPatchPresentations++;
    HGNextPatchPresentation(self, cmd, context, series, video, time, ready, remove);
}

@interface HGAdsTests : UIResponder <UIWindowSceneDelegate>
@property(nonatomic, strong) UIWindow *window;
@property(nonatomic, strong) NSMutableArray<NSDictionary *> *checks;
@end
@implementation HGAdsTests
- (void)check:(BOOL)passed name:(NSString *)name {
    [self.checks addObject:@{@"name": name, @"passed": @(passed)}];
    NSLog(@"[HonggoAdsTests] %@: %@", passed ? @"PASS" : @"FAIL", name);
}
- (void)checkSidebarGameCenterWithGames:(SSVideoMiniGameService *)games {
    SSVideoSeriesFeedSideBarViewController *legacy = [SSVideoSeriesFeedSideBarViewController new];
    SSVideoSeriesFeedSideBarV725ViewController *modern = [SSVideoSeriesFeedSideBarV725ViewController new];
    SSMyUser621SideBarViewController *profile = [SSMyUser621SideBarViewController new];
    NSString *game = @"SSMyUser621SideBarGameContainerCell";
    NSArray *layout = @[@"messages", @"history", game, @"functions"];
    legacy.gameEntranceData = game;
    profile.componentsLayout = layout;
    [self check:legacy.gameEntranceData == game && [modern showsGameSection] &&
        [profile.componentsLayout isEqualToArray:layout]
        name:@"the default sidebar preference preserves all host Game Center sections"];

    HGSetEnabled(HGSidebarGameCenter, YES);
    HGInstallAds();
    HGInstallAds();
    legacy.gameEntranceData = game;
    profile.componentsLayout = layout;
    [self check:legacy.gameEntranceData == nil && ![modern showsGameSection]
        name:@"both video drawers use native omission when the sidebar preference is enabled"];
    [self check:[profile.componentsLayout isEqualToArray:@[@"messages", @"history", @"functions"]] &&
        [layout containsObject:game]
        name:@"the profile drawer removes the exact game component without reordering other sections or changing the source"];
    [self check:[[games createFeedPageDataModelWithCellViewData:@"game-legacy" listVCContext:nil] isEqual:@"game-legacy"]
        name:@"the sidebar preference does not change feed game models"];
}
- (void)checkPatchLifecycleWithCompletion:(void (^)(void))completion {
    SEL selector = @selector(showPatchAdViewWithContext:seriesId:vId:currentTime:patchAdViewReadyBlock:removeBlock:);
    IMP installed = class_getMethodImplementation(SSBizVideoPatchAdService.class, selector);
    HGInstallAds();
    HGInstallAds();
    [self check:class_getMethodImplementation(SSBizVideoPatchAdService.class, selector) == installed
        name:@"repeated installation keeps one patch presentation implementation"];

    SSBizVideoPatchAdService *patch = [SSBizVideoPatchAdService new];
    id context = [NSObject new], series = [NSObject new], video = [NSObject new];
    __block NSUInteger readyCalls = 0, removeCalls = 0;
    __block id readyValue = nil;
    __block BOOL insideCall = NO, readyWasSynchronous = NO;
    NSMutableArray<NSString *> *events = patch.lifecycle;
    void (^ready)(id) = ^(id value) {
        readyCalls++;
        readyValue = value;
        readyWasSynchronous = insideCall;
        [events addObject:@"caller-ready"];
    };
    void (^remove)(void) = ^{
        removeCalls++;
        [events addObject:@"caller-remove"];
    };
    [self check:[patch shouldSkipPatchAdWithContext:context]
        name:@"patch presentation forwarding retains the separate advertisement skip gate"];
    insideCall = YES;
    [patch showPatchAdViewWithContext:context seriesId:series vId:video currentTime:19.75
        patchAdViewReadyBlock:ready removeBlock:remove];
    insideCall = NO;
    [self check:patch.originalCalls == 1
        name:@"enabled patch presentation reaches the original exactly once after reinstall"];
    [self check:patch.receivedContext == context && patch.receivedSeries == series && patch.receivedVideo == video &&
        patch.receivedTime == 19.75 && patch.receivedSelector == selector && patch.receivedThread == NSThread.currentThread
        name:@"patch forwarding preserves receiver parameters, selector and calling thread"];
    [self check:patch.receivedReady == (uintptr_t)(__bridge void *)ready &&
        patch.receivedRemove == (uintptr_t)(__bridge void *)remove
        name:@"patch forwarding passes the original callback objects without wrapping them"];
    [self check:readyCalls == 1 && [readyValue isEqual:@"host-ad"] && readyWasSynchronous && removeCalls == 0
        name:@"the original synchronous callback keeps its value, timing and invocation count"];
    [self check:[events isEqual:@[@"entry", @"ready", @"caller-ready", @"return"]]
        name:@"a synchronous ready path does not gain a synthetic removal or completion"];

    [events removeAllObjects];
    patch.fixturePath = HGPatchFixtureEarlyDecline;
    readyCalls = removeCalls = 0;
    [patch showPatchAdViewWithContext:context seriesId:series vId:video currentTime:29.5
        patchAdViewReadyBlock:ready removeBlock:remove];
    [self check:patch.originalCalls == 2 && readyCalls == 0 && removeCalls == 0 &&
        [events isEqual:@[@"entry", @"decline", @"return"]]
        name:@"a host early return stays callback-free instead of synthesizing ready(nil)"];

    [events removeAllObjects];
    patch.fixturePath = HGPatchFixtureSynchronous;
    [patch showPatchAdViewWithContext:nil seriesId:nil vId:nil currentTime:0
        patchAdViewReadyBlock:nil removeBlock:nil];
    [self check:patch.originalCalls == 3 && patch.receivedContext == nil && patch.receivedSeries == nil &&
        patch.receivedVideo == nil && patch.receivedTime == 0 && patch.receivedReady == 0 && patch.receivedRemove == 0 &&
        [events isEqual:@[@"entry", @"ready", @"return"]]
        name:@"nil arguments and absent callbacks still reach the original unchanged"];

    SSBizVideoPatchAdService *nested = [SSBizVideoPatchAdService new];
    NSMutableArray<NSString *> *nestedEvents = nested.lifecycle;
    __block NSUInteger outerCalls = 0, innerCalls = 0, nestedRemoveCalls = 0;
    [nested showPatchAdViewWithContext:context seriesId:series vId:video currentTime:59.75
        patchAdViewReadyBlock:^(id outerValue) {
            outerCalls++;
            [nestedEvents addObject:@"outer-callback-enter"];
            [nested showPatchAdViewWithContext:video seriesId:series vId:context currentTime:0.25
                patchAdViewReadyBlock:^(id innerValue) {
                    innerCalls++;
                    [nestedEvents addObject:@"inner-callback"];
                } removeBlock:^{ nestedRemoveCalls++; }];
            [nestedEvents addObject:@"outer-callback-exit"];
        } removeBlock:^{ nestedRemoveCalls++; }];
    [self check:nested.originalCalls == 2 && outerCalls == 1 && innerCalls == 1 && nestedRemoveCalls == 0
        name:@"a callback-triggered presentation reaches each original call once without recursion"];
    [self check:[nestedEvents isEqual:@[@"entry", @"ready", @"outer-callback-enter", @"entry", @"ready",
        @"inner-callback", @"return", @"outer-callback-exit", @"return"]] &&
        nested.receivedContext == video && nested.receivedVideo == context && nested.receivedTime == 0.25
        name:@"nested presentation preserves caller ordering and the inner call's arguments"];

    SSBizVideoPatchAdService *deferred = [SSBizVideoPatchAdService new];
    deferred.fixturePath = HGPatchFixtureDeferred;
    NSMutableArray<NSString *> *deferredEvents = deferred.lifecycle;
    __block NSUInteger deferredReadyCalls = 0, deferredRemoveCalls = 0;
    __block BOOL returned = NO, readyAfterReturn = NO, removeAfterReady = NO;
    __block id deferredValue = nil;
    void (^deferredReady)(id) = ^(id value) {
        deferredReadyCalls++;
        deferredValue = value;
        readyAfterReturn = returned && NSThread.isMainThread;
        [deferredEvents addObject:@"caller-ready"];
    };
    void (^deferredRemove)(void) = ^{
        deferredRemoveCalls++;
        removeAfterReady = returned && deferredReadyCalls == 1 && NSThread.isMainThread;
        [deferredEvents addObject:@"caller-remove"];
    };
    [deferred showPatchAdViewWithContext:context seriesId:series vId:video currentTime:89.125
        patchAdViewReadyBlock:deferredReady removeBlock:deferredRemove];
    returned = YES;
    [self check:deferred.originalCalls == 1 && deferredReadyCalls == 0 && deferredRemoveCalls == 0 &&
        [deferredEvents isEqual:@[@"entry", @"return"]]
        name:@"the original deferred path returns before either callback is invoked"];
    [self check:deferred.receivedContext == context && deferred.receivedSeries == series &&
        deferred.receivedVideo == video && deferred.receivedTime == 89.125 &&
        deferred.receivedReady == (uintptr_t)(__bridge void *)deferredReady &&
        deferred.receivedRemove == (uintptr_t)(__bridge void *)deferredRemove
        name:@"deferred presentation preserves every argument and both callback identities"];
    dispatch_async(dispatch_get_main_queue(), ^{
        [self check:deferredReadyCalls == 1 && deferredRemoveCalls == 1 &&
            [deferredValue isEqual:@"host-ad"] && readyAfterReturn && removeAfterReady
            name:@"deferred ready and removal are invoked once by the original on its chosen queue"];
        [self check:[deferredEvents isEqual:@[@"entry", @"return", @"ready", @"caller-ready", @"remove", @"caller-remove"]]
            name:@"deferred ready and removal retain their full lifecycle order"];
        dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
            SSBizVideoPatchAdService *background = [SSBizVideoPatchAdService new];
            NSThread *caller = NSThread.currentThread;
            __block BOOL callbackOnCaller = NO;
            [background showPatchAdViewWithContext:nil seriesId:nil vId:nil currentTime:0
                patchAdViewReadyBlock:^(id value) {
                    callbackOnCaller = NSThread.currentThread == caller && !NSThread.isMainThread;
                } removeBlock:nil];
            BOOL preserved = background.originalCalls == 1 && background.receivedThread == caller && callbackOnCaller;
            dispatch_async(dispatch_get_main_queue(), ^{
                [self check:preserved name:@"a background caller and its synchronous callback are not dispatched elsewhere"];
                completion();
            });
        });
    });
}
- (void)checkNaturalAdInsertionEnabled:(BOOL)enabled original:(IMP)original {
    SSNaturalAdManager *manager = [SSNaturalAdManager new];
    manager.originalResult = YES;
    SSNaturalAdData *native = [SSNaturalAdData new];
    native.gameAfterParsing = YES;
    SSNaturalAdData *video = [SSNaturalAdData new];
    video.gameAfterParsing = YES;
    video.forceWatchTime = -1;
    NSArray<SSNaturalAdData *> *games = @[native, video];
    NSArray<NSString *> *channels = @[@"native game", @"video game"];
    [self check:!native.isGameContent && !video.isGameContent && native.parseCalls == 0 && video.parseCalls == 0
        name:[NSString stringWithFormat:@"%@ natural insertion starts with unclassified native and video payloads", enabled ? @"enabled" : @"disabled"]];
    Method method = class_getInstanceMethod(SSNaturalAdManager.class, @selector(tryInsertDataToSaaS:));
    IMP installed = method_getImplementation(method);
    [self check:enabled ? installed != original : installed == original
        name:[NSString stringWithFormat:@"%@ natural insertion has the expected original or replacement IMP", enabled ? @"enabled" : @"disabled"]];
    for (NSUInteger index = 0; index < games.count; index++) {
        SSNaturalAdData *data = games[index];
        BOOL result = [manager tryInsertDataToSaaS:data];
        BOOL correct = enabled ?
            (!result && data.parseCalls == 1 && data.isGameContent && manager.originalCalls == 0 && manager.lastArgument == nil) :
            (result && data.parseCalls == 0 && !data.isGameContent && manager.originalCalls == index + 1 && manager.lastArgument == data);
        [self check:correct name:[NSString stringWithFormat:@"%@ %@ insertion %@", enabled ? @"enabled" : @"disabled", channels[index],
            enabled ? @"classifies lazily and declines before the original" : @"preserves the exact original argument without classification"]];
    }
    if (!enabled) return;

    SSNaturalAdData *mandatory = [SSNaturalAdData new];
    mandatory.forceWatchTime = 1;
    mandatory.gameAfterParsing = YES;
    [self check:[manager tryInsertDataToSaaS:mandatory] && manager.originalCalls == 1 &&
        manager.lastArgument == mandatory && mandatory.parseCalls == 0 && !mandatory.isGameContent
        name:@"positive force-watch time preserves the original argument and bypasses optional-game parsing"];
    SSNaturalAdData *ordinary = [SSNaturalAdData new];
    [self check:[manager tryInsertDataToSaaS:ordinary] && manager.originalCalls == 2 &&
        manager.lastArgument == ordinary && ordinary.parseCalls == 1 && !ordinary.isGameContent
        name:@"a classified non-game payload reaches the original unchanged"];
    NSObject *unknown = [NSObject new];
    [self check:[manager tryInsertDataToSaaS:unknown] && manager.originalCalls == 3 && manager.lastArgument == unknown
        name:@"an unknown payload without classification methods reaches the original unchanged"];
    HGUnexpectedNaturalAdData *subclass = [HGUnexpectedNaturalAdData new];
    [self check:[manager tryInsertDataToSaaS:subclass] && manager.originalCalls == 4 &&
        manager.lastArgument == subclass && subclass.parseCalls == 0 && !subclass.isGameContent
        name:@"an unexpected subclass bypasses its parser and retains the original path"];
    SSNaturalAdData *refused = [SSNaturalAdData new];
    manager.originalResult = NO;
    [self check:![manager tryInsertDataToSaaS:refused] && manager.originalCalls == 5 &&
        manager.lastArgument == refused && refused.parseCalls == 1
        name:@"the original insertion refusal is preserved for ordinary content"];

    HGInstallAds();
    HGInstallAds();
    [self check:method_getImplementation(method) == installed && installed != original
        name:@"repeated installation retains the same natural insertion hook"];
    SSNaturalAdData *repeatGame = [SSNaturalAdData new];
    repeatGame.gameAfterParsing = YES;
    [self check:![manager tryInsertDataToSaaS:repeatGame] && repeatGame.parseCalls == 1 &&
        repeatGame.isGameContent && manager.originalCalls == 5 && manager.lastArgument == refused
        name:@"optional-game classification still declines after repeated installation"];
    manager.originalResult = YES;
    SSNaturalAdData *repeatOrdinary = [SSNaturalAdData new];
    [self check:[manager tryInsertDataToSaaS:repeatOrdinary] && manager.originalCalls == 6 &&
        manager.lastArgument == repeatOrdinary && repeatOrdinary.parseCalls == 1
        name:@"ordinary insertion reaches its original exactly once after reinstallation without recursion"];
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
- (void)checkForwardingInterposers {
    Method naturalMethod = class_getInstanceMethod(SSNaturalAdManager.class, @selector(tryInsertDataToSaaS:));
    Method initializerMethod = class_getInstanceMethod(SSShortVideoFeedDirectGameContentView.class, @selector(initWithFrame:));
    Method patchMethod = class_getInstanceMethod(SSBizVideoPatchAdService.class,
        @selector(showPatchAdViewWithContext:seriesId:vId:currentTime:patchAdViewReadyBlock:removeBlock:));
    IMP natural = method_getImplementation(naturalMethod);
    IMP initializer = method_getImplementation(initializerMethod);
    IMP patch = method_getImplementation(patchMethod);
    HGInstallAds();
    HGInstallAds();
    [self check:method_getImplementation(naturalMethod) == natural &&
        method_getImplementation(initializerMethod) == initializer && method_getImplementation(patchMethod) == patch
        name:@"repeated installation preserves all three forwarding wrappers"];

    HGNextNaturalInsertion = (BOOL (*)(id, SEL, id))natural;
    HGNextGameInitializer = (HGFixtureGameInitializer)initializer;
    HGNextPatchPresentation = (void (*)(id, SEL, id, id, id, double, void (^)(id), void (^)(void)))patch;
    foreignGameInitializations = foreignNaturalInsertions = foreignPatchPresentations = 0;
    method_setImplementation(naturalMethod, (IMP)HGForeignNaturalInsertion);
    method_setImplementation(initializerMethod, (IMP)HGForeignGameInitializer);
    method_setImplementation(patchMethod, (IMP)HGForeignPatchPresentation);
    HGInstallAds();
    HGInstallAds();
    [self check:method_getImplementation(naturalMethod) == (IMP)HGForeignNaturalInsertion
        name:@"natural insertion reinstallation leaves a later interposer in place"];
    [self check:method_getImplementation(initializerMethod) == (IMP)HGForeignGameInitializer
        name:@"game initializer reinstallation leaves a later interposer in place"];
    [self check:method_getImplementation(patchMethod) == (IMP)HGForeignPatchPresentation
        name:@"patch presentation reinstallation leaves a later interposer in place"];

    SSNaturalAdManager *manager = [SSNaturalAdManager new];
    manager.originalResult = YES;
    SSNaturalAdData *ordinary = [SSNaturalAdData new];
    [self check:[manager tryInsertDataToSaaS:ordinary] && manager.originalCalls == 1 &&
        manager.lastArgument == ordinary && ordinary.parseCalls == 1 && foreignNaturalInsertions == 1
        name:@"a later natural interposer reaches the wrapper and host once without recursion"];
    SSNaturalAdData *game = [SSNaturalAdData new];
    game.gameAfterParsing = YES;
    [self check:![manager tryInsertDataToSaaS:game] && game.parseCalls == 1 &&
        manager.originalCalls == 1 && manager.lastArgument == ordinary && foreignNaturalInsertions == 2
        name:@"natural promotion rejection survives a later forwarding interposer"];

    CGRect frame = CGRectMake(3, 7, 32, 48);
    NSUInteger originalInitializations = nativeGameInitializations;
    SSShortVideoFeedDirectGameContentView *view = [[SSShortVideoFeedDirectGameContentView alloc] initWithFrame:frame];
    [self check:view.hidden && CGRectEqualToRect(view.frame, frame) && view.defaultCTA.superview == view &&
        nativeGameInitializations == originalInitializations + 1 && foreignGameInitializations == 1
        name:@"a later initializer interposer preserves construction, CGRect and pre-attachment hiding"];

    SSBizVideoPatchAdService *service = [SSBizVideoPatchAdService new];
    __block NSUInteger readyCalls = 0, removeCalls = 0;
    __block id readyValue = nil;
    [service showPatchAdViewWithContext:ordinary seriesId:@"series" vId:@"video" currentTime:3.25
        patchAdViewReadyBlock:^(id value) { readyCalls++; readyValue = value; }
        removeBlock:^{ removeCalls++; }];
    [self check:service.originalCalls == 1 && foreignPatchPresentations == 1 && readyCalls == 1 &&
        [readyValue isEqual:@"host-ad"] && removeCalls == 0 && service.receivedContext == ordinary &&
        service.receivedTime == 3.25
        name:@"a later patch interposer retains the original lifecycle without recursion"];

    method_setImplementation(naturalMethod, natural);
    method_setImplementation(initializerMethod, initializer);
    method_setImplementation(patchMethod, patch);
    HGInstallAds();
    HGInstallAds();
    [self check:method_getImplementation(naturalMethod) == natural &&
        method_getImplementation(initializerMethod) == initializer && method_getImplementation(patchMethod) == patch
        name:@"removing the later interposers retains the original forwarding wrappers"];
    SSNaturalAdData *restored = [SSNaturalAdData new];
    [self check:[manager tryInsertDataToSaaS:restored] && manager.originalCalls == 2 &&
        manager.lastArgument == restored && restored.parseCalls == 1 && foreignNaturalInsertions == 2
        name:@"natural forwarding still reaches the host once after interposer removal"];
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
    Method naturalInsert = class_getInstanceMethod(SSNaturalAdManager.class, @selector(tryInsertDataToSaaS:));
    IMP originalNaturalInsert = method_getImplementation(naturalInsert);
    HGInstallAds();
    [self checkSidebarGameCenterWithGames:games];
    [self checkNaturalAdInsertionEnabled:NO original:originalNaturalInsert];
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
    [self check:[readyValue isEqual:@"host-ad"] && patch.originalCalls == 1
        name:@"disabled patch filtering preserves the original ready callback"];

    [defaults setBool:YES forKey:@"HongguoGlass.feed"];
    [defaults setBool:YES forKey:@"HongguoGlass.patch"];
    HGInstallAds();
    [self checkNaturalAdInsertionEnabled:YES original:originalNaturalInsert];
    [self checkGameBindingHidden:YES];
    [self checkForwardingInterposers];
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
    [self checkPatchLifecycleWithCompletion:^{
        [self checkDisplayedGamesWithDisabledModel:disabledGame];
    }];
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
