#import "Honggo.h"
#import "RuntimeSupport.h"
#import <dispatch/dispatch.h>
#include <stdatomic.h>
#include <string.h>

typedef struct {
    HGFeature feature;
    const char *className;
    const char *selector;
    const char *encoding;
    IMP replacement;
    BOOL classMethod;
} HGGate;

typedef UIView *(*HGDirectGameInitializer)(UIView * __attribute__((ns_consumed)), SEL, CGRect)
    __attribute__((ns_returns_retained));

static void HGAdDecision(id object, SEL selector, BOOL result);
static void (*HGOriginalDirectGameDidDisplay)(id, SEL);
typedef void (*HGPatchPresentationIMP)(id, SEL, id, id, id, double,
    void (^)(id), void (^)(void));
typedef BOOL (*HGNaturalAdInsertIMP)(id, SEL, id);
typedef struct {
    _Atomic(IMP) original;
    BOOL installed;
} HGForwardingAdHook;
static HGForwardingAdHook HGPatchPresentationHook;
static HGForwardingAdHook HGDirectGameInitHook;
static HGForwardingAdHook HGNaturalAdInsertHook;
static HGForwardingAdHook HGSidebarLayoutHook;
static Class HGNaturalAdDataClass;
static Class HGDirectGameFeedHost, HGDirectGameSeriesHost;
static char HGDirectGameRemovalPending;

@protocol HGDirectGameHost <NSObject>
- (id)contentModel;
- (BOOL)isCurrentDisplaying;
@end

@protocol HGDirectGameModel <NSObject>
- (id<HGDirectGameHost>)host;
- (BOOL)displaying;
- (BOOL)isInvalidated;
- (void)handleTerminalFailureWithReason:(NSString *)reason;
@end

@protocol HGNaturalAdData <NSObject>
- (NSInteger)forceWatchTime;
- (void)parseGameDict;
- (BOOL)isGameContent;
@end

static BOOL HGNaturalAdInsert(id self, SEL cmd, id<HGNaturalAdData> data) {
    if (data && object_getClass(data) == HGNaturalAdDataClass && data.forceWatchTime < 1) {
        // The host classifies this payload lazily, before preparing its game views.
        [data parseGameDict];
        if (data.isGameContent) {
            HGAdDecision(self, cmd, NO);
            return NO;
        }
    }
    HGNaturalAdInsertIMP original = (HGNaturalAdInsertIMP)atomic_load_explicit(
        &HGNaturalAdInsertHook.original, memory_order_acquire);
    return original(self, cmd, data);
}

__attribute__((ns_returns_retained))
static UIView *HGDirectGameInit(UIView * __attribute__((ns_consumed)) self, SEL cmd, CGRect frame) {
    HGDirectGameInitializer original = (HGDirectGameInitializer)atomic_load_explicit(
        &HGDirectGameInitHook.original, memory_order_acquire);
    UIView *view = original(self, cmd, frame);
    // Both wrappers attach this root only after initialization, even if binding is skipped.
    view.hidden = YES;
    HGAdDecision(view, cmd, NO);
    return view;
}

static void HGDirectGameDidDisplay(id<HGDirectGameModel> self, SEL cmd) {
    HGOriginalDirectGameDidDisplay(self, cmd);
    if (!NSThread.isMainThread || objc_getAssociatedObject(self, &HGDirectGameRemovalPending)) return;

    objc_setAssociatedObject(self, &HGDirectGameRemovalPending, @YES, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    __weak id<HGDirectGameModel> game = self;
    // Let the host finish its display update before it changes the feed again.
    dispatch_async(dispatch_get_main_queue(), ^{
        id<HGDirectGameModel> model = game;
        if (!model) return;
        objc_setAssociatedObject(model, &HGDirectGameRemovalPending, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        if (model.isInvalidated || !model.displaying) return;
        id<HGDirectGameHost> host = model.host;
        Class type = object_getClass(host);
        if (!type || (type != HGDirectGameFeedHost && type != HGDirectGameSeriesHost) ||
            host.contentModel != model || !host.isCurrentDisplaying) return;
        HGAdDecision(model, cmd, NO);
        [model handleTerminalFailureWithReason:@"honggo_feed_filter"];
    });
}

static BOOL HGAdMethodMatches(Class type, SEL selector, const char *expected) {
    Method method = class_getInstanceMethod(type, selector);
    return method && strcmp(method_getTypeEncoding(method), expected) == 0;
}

static BOOL HGNaturalAdInsertSupported(void) {
    Class data = objc_lookUpClass("SSNaturalAdData");
    if (!HGAdMethodMatches(data, @selector(forceWatchTime), "q16@0:8") ||
        !HGAdMethodMatches(data, @selector(parseGameDict), "v16@0:8") ||
        !HGAdMethodMatches(data, @selector(isGameContent), "B16@0:8")) return NO;
    HGNaturalAdDataClass = data;
    return YES;
}

static BOOL HGDirectGameDisplaySupported(Class model) {
    Class feed = objc_lookUpClass("SSShortVideoFeedDirectGameFeedPageHost");
    Class series = objc_lookUpClass("SSShortVideoFeedDirectGameSeriesEndInsertHost");
    HGDirectGameFeedHost = HGAdMethodMatches(feed, @selector(isCurrentDisplaying), "B16@0:8") &&
        HGAdMethodMatches(feed, @selector(contentModel), "@16@0:8") ? feed : Nil;
    HGDirectGameSeriesHost = HGAdMethodMatches(series, @selector(isCurrentDisplaying), "B16@0:8") &&
        HGAdMethodMatches(series, @selector(contentModel), "@16@0:8") ? series : Nil;
    return (HGDirectGameFeedHost || HGDirectGameSeriesHost) &&
        HGAdMethodMatches(model, @selector(host), "@16@0:8") &&
        HGAdMethodMatches(model, @selector(displaying), "B16@0:8") &&
        HGAdMethodMatches(model, @selector(isInvalidated), "B16@0:8") &&
        HGAdMethodMatches(model, @selector(handleTerminalFailureWithReason:), "v24@0:8@16");
}

static BOOL HGNo(id self, SEL cmd) { HGAdDecision(self, cmd, NO); return NO; }
static BOOL HGNoObject(id self, SEL cmd, id argument) { HGAdDecision(self, cmd, NO); return NO; }
static BOOL HGNoObjects(id self, SEL cmd, id first, id second) { HGAdDecision(self, cmd, NO); return NO; }
static BOOL HGNoTimeObject(id self, SEL cmd, double time, id context) { HGAdDecision(self, cmd, NO); return NO; }
static BOOL HGNoTimesObject(id self, SEL cmd, double time, double duration, id context) { HGAdDecision(self, cmd, NO); return NO; }
static BOOL HGNoObjectClass(id self, SEL cmd, id argument, Class viewClass) { HGAdDecision(self, cmd, NO); return NO; }
static BOOL HGYesObject(id self, SEL cmd, id argument) { HGAdDecision(self, cmd, YES); return YES; }
static id HGNoModel(id self, SEL cmd, id argument) { HGAdDecision(self, cmd, NO); return nil; }
static id HGNoModels(id self, SEL cmd, id data, id context) { HGAdDecision(self, cmd, NO); return nil; }
static id HGNoSidebarGameData(id self, SEL cmd) { HGAdDecision(self, cmd, NO); return nil; }
static id HGNoDirectGameModel(id self, SEL cmd, id data, NSInteger source, id host) {
    HGAdDecision(self, cmd, NO);
    return nil;
}
static void HGSkipSurvey(id self, SEL cmd, NSInteger style, id videoID) { HGAdDecision(self, cmd, NO); }
static void HGSkipPendant(id self, SEL cmd, UIView *parent) { HGAdDecision(self, cmd, NO); }

static void HGSetSidebarLayout(id self, SEL cmd, NSArray *layout) {
    // The 621 drawer uses the same component list for counting, sizing and building cells.
    NSString *gameComponent = @"SSMyUser621SideBarGameContainerCell";
    if ([layout isKindOfClass:NSArray.class] && [layout containsObject:gameComponent]) {
        NSMutableArray *filtered = [layout mutableCopy];
        [filtered removeObject:gameComponent];
        layout = filtered.copy;
        HGAdDecision(self, cmd, NO);
    }
    void (*original)(id, SEL, NSArray *) = (void (*)(id, SEL, NSArray *))atomic_load_explicit(
        &HGSidebarLayoutHook.original, memory_order_acquire);
    original(self, cmd, layout);
}

static void HGForwardPatchPresentation(id self, SEL cmd, id context, id seriesID, id videoID,
    double currentTime, void (^ready)(id), void (^remove)(void)) {
    static atomic_ulong calls;
    unsigned long count = atomic_fetch_add_explicit(&calls, 1, memory_order_relaxed) + 1;
    if ((count & (count - 1)) == 0 || HGEnhancedDiagnostics()) {
        HGLog(@"patch.presentation", @{@"rule": @16, @"action": @"forwarded",
            @"hasReady": @(ready != nil), @"hasRemove": @(remove != nil),
            @"mainThread": @(NSThread.isMainThread), @"count": @(count)});
    }
    // The R3 log does not establish the freeze's root cause or the callback
    // contract of every declined presentation. Keep the host's lifecycle,
    // callback identities and scheduling intact instead of synthesizing ready(nil).
    // The separate shouldSkipPatchAdWithContext: decision gate stays installed.
    HGPatchPresentationIMP original = (HGPatchPresentationIMP)atomic_load_explicit(
        &HGPatchPresentationHook.original, memory_order_acquire);
    original(self, cmd, context, seriesID, videoID, currentTime, ready, remove);
}

static IMP HGInstallForwardingAdHook(Class target, SEL selector, const char *encoding,
        IMP replacement, HGForwardingAdHook *hook) {
    @synchronized (target) {
        // Seed a distinct, ABI-checked original before publishing the wrapper:
        // another thread may enter as soon as class_replaceMethod returns.
        // Never reseed from a later interposer, which may itself call our wrapper.
        if (!hook->installed) {
            Method method = class_getInstanceMethod(target, selector);
            const char *actual = method ? method_getTypeEncoding(method) : NULL;
            IMP candidate = method ? method_getImplementation(method) : NULL;
            if (actual && strcmp(actual, encoding) == 0 && candidate &&
                    candidate != replacement) {
                atomic_store_explicit(&hook->original, candidate, memory_order_release);
            }
        }
        IMP original = HGInstallVerifiedHook(target, selector, encoding,
            replacement, @"ads");
        if (original && original != replacement) {
            // The verified installer returns this wrapper's saved original on
            // retries, including when another plugin has installed above it.
            atomic_store_explicit(&hook->original, original, memory_order_release);
            hook->installed = YES;
        }
        return original;
    }
}

static const HGGate HGGates[] = {
    {HGSplash, "SSReadingAdSplashService", "shouldShowSplashAd", "B16@0:8", (IMP)HGNo},
    {HGSplash, "SSReadingAdSplashService", "shouldShowSplashAtHotLaunch", "B16@0:8", (IMP)HGNo},
    {HGSplash, "SSReadingAdSplashService", "shouldShowSplashAdAtHotLaunchByThirdParty", "B16@0:8", (IMP)HGNo},
    {HGTopView, "SSBizVideoAdTopviewService", "topviewEnable", "B16@0:8", (IMP)HGNo},
    {HGTopView, "SSBizVideoAdTopviewService", "canShowTopView", "B16@0:8", (IMP)HGNo},
    {HGFeed, "SSBizVideoAdCommonAdClient", "shouldRequestShortVideoAd:", "B24@0:8@16", (IMP)HGNoObject},
    {HGFeed, "SSBizVideoAdCommonAdClient", "shouldDisableCurrentShortVideoFeedAdByBizRulesWithContext:", "B24@0:8@16", (IMP)HGYesObject},
    {HGFeed, "SSShortVideoMUnionAdService", "shouldDisableCurrentShortVideoFeedAdByBizRulesWithContext:", "B24@0:8@16", (IMP)HGYesObject},
    {HGFeed, "SSShortVideoAdService", "shouldDisableCurrentShortVideoFeedAdByBizRulesWithContext:", "B24@0:8@16", (IMP)HGYesObject},
    {HGComment, "SSShortVideoCommentAdService", "enableCommentAd", "B16@0:8", (IMP)HGNo, YES},
    {HGComment, "SSShortVideoCommentAdService", "enableShortVideoCommentAd:", "B24@0:8@16", (IMP)HGNoObject},
    {HGPause, "SSBizVideoPauseAdServiceImpl", "canShowPauseAd:videoDataModel:", "B32@0:8@16@24", (IMP)HGNoObjects},
    {HGPause, "SSBizVideoPauseAdManager", "shouldShowPauseAd", "B16@0:8", (IMP)HGNo},
    {HGPause, "SSBizVideoPauseAdManager", "shouldRequestPauseAdData", "B16@0:8", (IMP)HGNo},
    {HGPatch, "SSBizVideoInVideoPatchAdServiceImp", "canShowAd", "B16@0:8", (IMP)HGNo},
    {HGPatch, "SSBizVideoPatchAdService", "shouldSkipPatchAdWithContext:", "B24@0:8@16", (IMP)HGYesObject},
    {HGPatch, "SSBizVideoPatchAdService", "showPatchAdViewWithContext:seriesId:vId:currentTime:patchAdViewReadyBlock:removeBlock:", "v64@0:8@16@24@32d40@?48@?56", (IMP)HGForwardPatchPresentation},
    {HGEcommerce, "SSStartUpEcomQCPXPopupOperation", "shouldDisplay:viewClass:", "B32@0:8@16#24", (IMP)HGNoObjectClass},
    {HGMarketing, "SSBizMonetizationAdPopupOperation", "checkCanShowPopup", "B16@0:8", (IMP)HGNo},
    {HGRedPacket, "SSWelfareRedPacketPopupAdapter", "couldShowRedPacket", "B16@0:8", (IMP)HGNo},
    {HGBookstoreAd, "SSBizAdBookShopBannerMaterialService", "canShowBottomView", "B16@0:8", (IMP)HGNo},
    {HGCouponPendant, "SSECommBookCouponPendantManager", "shouldShowPendant", "B16@0:8", (IMP)HGNo},
    {HGMiniGamePendant, "SSBookStorePendantManager", "canShowMiniGamePendantView", "B16@0:8", (IMP)HGNo},
    {HGMiniGamePendant, "SSBookStoreMiniGamePendantView", "addPendantViewInView:", "v24@0:8@16", (IMP)HGSkipPendant},
    {HGWelfareGuide, "SSWelfareTabGuideTipManager", "couldShowBubble", "B16@0:8", (IMP)HGNo},
    {HGSurvey, "SSShortVideoSatisfactionSurveyManager", "showSurveyWithStyle:videoId:", "v32@0:8q16@24", (IMP)HGSkipSurvey},
    {HGSurvey, "SSShortVideoSatisfactionSurveyManager", "tryShowSingleVideoSurveyIfNeededWithVideoId:", "B24@0:8@16", (IMP)HGNoObject},
    {HGSurvey, "SSShortVideoSatisfactionSurveyManager", "tryShowInnerFlowQuickSlideSurveyIfNeededWithVideoId:", "B24@0:8@16", (IMP)HGNoObject},
    {HGSurvey, "SSShortVideoSatisfactionSurveyManager", "tryShowInnerFlowDetailReturnSurveyIfNeededWithVideoId:", "B24@0:8@16", (IMP)HGNoObject},
    {HGSurvey, "SSShortVideoSatisfactionSurveyManager", "tryShowPeriodSkipSurveyIfNeededWithVideoId:", "B24@0:8@16", (IMP)HGNoObject},
    {HGSurvey, "SSShortVideoSatisfactionSurveyManager", "tryShowContinueSkipSurveyIfNeededWithVideoId:", "B24@0:8@16", (IMP)HGNoObject},
    {HGSurvey, "SSShortVideoSatisfactionSurveyManager", "tryRestoreReadingItemSurveyIfNeededWithVideoId:", "B24@0:8@16", (IMP)HGNoObject},
    // 7.4.3.33 has separate commerce/live insertion paths outside the ad client.
    // Both factory callers explicitly accept nil before adding to the feed.
    // Keep rules appended so diagnostic IDs of shipped rules remain stable.
    {HGFeed, "SSBizVideoAdABTestHelper", "enableEcomForceCard", "B16@0:8", (IMP)HGNo, YES},
    {HGFeed, "SSShortVideoLiveService", "createFeedLiveInsertModelWithRoomModel:", "@24@0:8@16", (IMP)HGNoModel},
    {HGFeed, "SSShortVideoRecommendOutterListDataSource", "liveFeedDataModelWithCellViewData:", "@24@0:8@16", (IMP)HGNoModel},
    {HGMarketing, "IESLiveUGBenefitCouponFragment", "shouldShowBalanceCouponPopup", "B16@0:8", (IMP)HGNo},
    {HGMarketing, "IESLiveUGBenefitCouponFragment", "shouldShowExpansionCouponPopup", "B16@0:8", (IMP)HGNo},
    {HGEcommerce, "SSCouponPopupFrequencyHelper", "canShowPopup:", "B24@0:8@16", (IMP)HGNoObject, YES},
    {HGPatch, "BDADBackPatchAdService", "shouldShowAdWithContext:", "B24@0:8@16", (IMP)HGNoObject},
    {HGPatch, "BDADBackPatchAdService", "shouldRequestAdWithPlaybackTime:duration:context:", "B40@0:8d16d24@32", (IMP)HGNoTimesObject},
    {HGPatch, "BDADPlotTouchAdService", "canShowPlotAdWithContext:", "B24@0:8@16", (IMP)HGNoObject},
    {HGPatch, "BDADPlotTouchAdService", "canRequestAdWithPlaybackTime:context:", "B32@0:8d16@24", (IMP)HGNoTimeObject},
    {HGPatch, "BDADShortVideoSideBarPatchSwitchComponent", "isAvailableWithCtx:", "B24@0:8@16", (IMP)HGNoObject},
    // Game feed cards and series-end game inserts are not the bookstore pendant.
    // The feed caller skips nil before append; series-end NO precedes preparation.
    {HGFeed, "SSVideoMiniGameService", "createFeedPageDataModelWithCellViewData:listVCContext:", "@32@0:8@16@24", (IMP)HGNoModels},
    {HGFeed, "SSBizVideoABTestHelper", "enableSeriesEndDirectGame", "B16@0:8", (IMP)HGNo, YES},
    // Both direct-game callers check nil before constructing or enqueueing a page.
    {HGFeed, "SSVideoMiniGameService", "createDirectGameContentModelWithCellViewData:source:host:", "@40@0:8@16q24@32", (IMP)HGNoDirectGameModel},
    {HGFeed, "SSShortVideoFeedDirectGameContentModel", "modelDidDisplay", "v16@0:8", (IMP)HGDirectGameDidDisplay},
    {HGFeed, "SSShortVideoFeedDirectGameContentView", "initWithFrame:", "@48@0:8{CGRect={CGPoint=dd}{CGSize=dd}}16", (IMP)HGDirectGameInit},
    {HGFeed, "SSNaturalAdManager", "tryInsertDataToSaaS:", "B24@0:8@16", (IMP)HGNaturalAdInsert},
    // Both feed drawers already omit the entire game section for these outcomes.
    {HGSidebarGameCenter, "SSVideoSeriesFeedSideBarViewController", "gameEntranceData", "@16@0:8", (IMP)HGNoSidebarGameData},
    {HGSidebarGameCenter, "SSVideoSeriesFeedSideBarV725ViewController", "showsGameSection", "B16@0:8", (IMP)HGNo},
    {HGSidebarGameCenter, "SSMyUser621SideBarViewController", "setComponentsLayout:", "v24@0:8@16", (IMP)HGSetSidebarLayout},
    // Both screenshot flows handle the search action's native declined path.
    // Screenshot sharing and manual image search use independent decisions.
    {HGScreenshotShopping, "SSBizVideoScanShoppingService", "enableVideoSearch", "B16@0:8", (IMP)HGNo},
};

static void HGAdDecision(id object, SEL selector, BOOL result) {
    static atomic_ulong counts[sizeof(HGGates) / sizeof(HGGates[0])];
    for (NSUInteger i = 0; i < sizeof(HGGates) / sizeof(HGGates[0]); i++) {
        const HGGate *gate = &HGGates[i];
        if (selector != sel_registerName(gate->selector)) continue;
        Class type = objc_lookUpClass(gate->className);
        if (!(gate->classMethod ? [object isSubclassOfClass:type] : [object isKindOfClass:type])) continue;
        unsigned long count = atomic_fetch_add_explicit(&counts[i], 1, memory_order_relaxed) + 1;
        // First hit and powers of two preserve useful counts without logging a hot path per call.
        if ((count & (count - 1)) == 0 || HGEnhancedDiagnostics())
            HGLog(@"ad.decision", @{@"rule": @(i), @"class": @(gate->className),
                @"selector": @(gate->selector), @"count": @(count), @"returnOrSkip": @(result)});
        return;
    }
}

void HGInstallAds(void) {
    BOOL enabled[HGFeatureCount] = {NO};
    for (HGFeature feature = HGSplash; feature < HGFeatureCount; feature++) {
        enabled[feature] = HGIsEnabled(feature);
    }

    NSUInteger installed = 0;
    NSUInteger selected = 0;
    for (NSUInteger index = 0; index < sizeof(HGGates) / sizeof(HGGates[0]); index++) {
        const HGGate *gate = &HGGates[index];
        NSString *identifier = [NSString stringWithFormat:@"hook.ads.%@.%s.%s",
            gate->classMethod ? @"class" : @"instance", gate->className, gate->selector];
        if (!enabled[gate->feature]) {
            HGRecordCapability(identifier, @{@"module": @"ads", @"class": @(gate->className),
                @"selector": @(gate->selector), @"state": @"disabled", @"feature": @(gate->feature)});
            continue;
        }

        selected++;
        Class target = objc_lookUpClass(gate->className);
        if (gate->classMethod) {
            target = object_getClass(target);
        }
        if (!target) {
            HGRecordCapability(identifier, @{@"module": @"ads", @"class": @(gate->className),
                @"selector": @(gate->selector), @"state": @"missing-class",
                @"expected": @(gate->encoding), @"actual": @"missing", @"feature": @(gate->feature)});
            continue;
        }
        BOOL directGameDisplay = gate->replacement == (IMP)HGDirectGameDidDisplay;
        BOOL naturalAdInsert = gate->replacement == (IMP)HGNaturalAdInsert;
        SEL selector = sel_registerName(gate->selector);
        if (directGameDisplay && !HGDirectGameDisplaySupported(target)) {
            HGRecordCapability(identifier, @{@"module": @"ads", @"class": @(gate->className),
                @"selector": @(gate->selector), @"state": @"abi-mismatch",
                @"reason": @"direct-game host method ABI mismatch", @"feature": @(gate->feature)});
            HGLog(@"hook.skipped", @{@"class": @(gate->className), @"selector": @(gate->selector),
                @"reason": @"direct-game host method ABI mismatch"});
            continue;
        }
        if (naturalAdInsert && !HGNaturalAdInsertSupported()) {
            HGRecordCapability(identifier, @{@"module": @"ads", @"class": @(gate->className),
                @"selector": @(gate->selector), @"state": @"abi-mismatch",
                @"reason": @"natural ad data method ABI mismatch", @"feature": @(gate->feature)});
            HGLog(@"hook.skipped", @{@"class": @(gate->className), @"selector": @(gate->selector),
                @"reason": @"natural ad data method ABI mismatch"});
            continue;
        }
        HGForwardingAdHook *forwarding = gate->replacement == (IMP)HGForwardPatchPresentation ?
            &HGPatchPresentationHook : gate->replacement == (IMP)HGDirectGameInit ?
            &HGDirectGameInitHook : naturalAdInsert ? &HGNaturalAdInsertHook : NULL;
        if (gate->replacement == (IMP)HGSetSidebarLayout) forwarding = &HGSidebarLayoutHook;
        IMP original = forwarding ?
            HGInstallForwardingAdHook(target, selector, gate->encoding, gate->replacement, forwarding) :
            HGInstallVerifiedHook(target, selector, gate->encoding, gate->replacement, @"ads");
        if (original) {
            if (directGameDisplay) HGOriginalDirectGameDidDisplay = (void (*)(id, SEL))original;
            if (class_getMethodImplementation(target, selector) == gate->replacement) installed++;
        } else {
            NSLog(@"[Honggo] 跳过不匹配的入口 %s %s", gate->className, gate->selector);
        }
    }

    HGRecordCapability(@"module.ads", @{@"module": @"ads",
        @"state": selected == 0 ? @"disabled" : installed == selected ? @"installed" : installed ? @"partial" : @"unavailable",
        @"selected": @(selected), @"installed": @(installed),
        @"reason": @"enabled advertisement rules install independently of appearance modules"});
    NSLog(@"[Honggo] 广告与营销入口 %lu/%lu", (unsigned long)installed, (unsigned long)selected);
}
