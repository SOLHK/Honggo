#import "Honggo.h"
#import "RuntimeSupport.h"
#import "AppearancePreferences.h"
#import <objc/message.h>
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
@end

static BOOL HGIsSkippableNaturalAd(id<HGNaturalAdData> data) {
    return data && object_getClass(data)==HGNaturalAdDataClass && data.forceWatchTime<1;
}

static BOOL HGNaturalAdInsert(id self, SEL cmd, id<HGNaturalAdData> data) {
    // NaturalAdManager receives sponsored insertions, including non-game campaigns.
    if (HGIsSkippableNaturalAd(data)) {
        HGAdDecision(self, cmd, NO);
        return NO;
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
    if (!HGAdMethodMatches(data, @selector(forceWatchTime), "q16@0:8")) return NO;
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
static BOOL HGNoEpisodeIndex(id self, SEL cmd, NSInteger index) { HGAdDecision(self, cmd, NO); return NO; }
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
    static _Atomic uint64_t nextLog;
    unsigned long count = atomic_fetch_add_explicit(&calls, 1, memory_order_relaxed) + 1;
    if (HGEnhancedDiagnostics() ? HGDiagnosticSample(&nextLog) : (count & (count - 1)) == 0) {
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
    // The homepage declines both cached and fresh sponsored cards here, before
    // the insertion model can allocate a full-height page in the video list.
    {HGFeed, "BDADShortVideoCommonAdManager", "checkHomePageAdBasicReqRulesWithEpisodeIndex:", "B24@0:8q16", (IMP)HGNoEpisodeIndex},
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

typedef struct {
    _Atomic SEL selector;
    _Atomic Class cls;
} HGGateLookup;

static void HGAdDecision(id object, SEL selector, BOOL result) {
    static atomic_ulong counts[sizeof(HGGates) / sizeof(HGGates[0])];
    static _Atomic uint64_t nextLog[sizeof(HGGates) / sizeof(HGGates[0])];
    static HGGateLookup cache[sizeof(HGGates) / sizeof(HGGates[0])];
    for (NSUInteger i = 0; i < sizeof(HGGates) / sizeof(HGGates[0]); i++) {
        const HGGate *gate = &HGGates[i];
        SEL expected = atomic_load_explicit(&cache[i].selector, memory_order_relaxed);
        if (!expected) {
            expected = sel_registerName(gate->selector);
            atomic_store_explicit(&cache[i].selector, expected, memory_order_relaxed);
        }
        if (selector != expected) continue;
        Class type = atomic_load_explicit(&cache[i].cls, memory_order_relaxed);
        if (!type) {
            type = objc_lookUpClass(gate->className);
            if (type) atomic_store_explicit(&cache[i].cls, type, memory_order_relaxed);
        }
        if (!type || !(gate->classMethod ? [object isSubclassOfClass:type] : [object isKindOfClass:type])) continue;
        unsigned long count = atomic_fetch_add_explicit(&counts[i], 1, memory_order_relaxed) + 1;
        // Log cumulative counts at a bounded cadence even in enhanced mode.
        if (HGEnhancedDiagnostics() ? HGDiagnosticSample(&nextLog[i]) : (count & (count - 1)) == 0)
            HGLog(@"ad.decision", @{@"rule": @(i), @"class": @(gate->className),
                @"requestGate": @(strstr(gate->selector, "Request") != NULL),
                @"selector": @(gate->selector), @"count": @(count), @"returnOrSkip": @(result)});
        return;
    }
}

#include "SidebarGame.inc"

// Raw cards from both disk and the network reach this same outer-feed builder
// before it generates mixed-page models and their parallel video indexes.
@protocol HGKeywordFeedResult <NSObject>
- (BOOL)isSuccess;
- (BOOL)hasMore;
- (NSInteger)nextOffset;
- (id)cellViewData;
@end
@protocol HGKeywordFeedSource <NSObject>
- (BOOL)hasMore;
- (NSInteger)nextOffset;
- (NSInteger)nextLoadMoreCellId;
- (BOOL)canHandleVideoFeedCellViewData:(id)cell;
- (BOOL)isRecommendPeopleCellViewData:(id)cell;
- (void)updatePagingWithCellChangeResult:(id)result;
- (void)requestLoadMoreDataWithCellId:(NSInteger)cellId completion:(void (^)(id<HGKeywordFeedResult>))completion;
@end

static BOOL HGFeedTextHasKeyword(id text, NSArray<NSString *> *keywords) {
    if (![text isKindOfClass:NSString.class]) return NO;
    for (NSString *keyword in keywords)
        if ([text rangeOfString:keyword options:NSCaseInsensitiveSearch|NSWidthInsensitiveSearch].location!=NSNotFound) return YES;
    return NO;
}

static BOOL HGFeedTagsHaveKeyword(id tags, NSArray<NSString *> *keywords) {
    if (![tags isKindOfClass:NSArray.class]) return HGFeedTextHasKeyword(tags,keywords);
    for (id tag in tags) {
        if (HGFeedTextHasKeyword(tag,keywords) || HGFeedTextHasKeyword(HGReadObjectGetter(tag,"categoryName"),keywords) ||
            HGFeedTextHasKeyword(HGReadObjectGetter(tag,"text"),keywords) ||
            HGFeedTextHasKeyword(HGReadObjectGetter(tag,"name"),keywords) ||
            HGFeedTextHasKeyword(HGReadObjectGetter(tag,"title"),keywords)) return YES;
    }
    return NO;
}

static BOOL HGFeedValueIsNonEmptyId(id value) {
    if ([value isKindOfClass:NSString.class]) {
        NSString *s=(NSString *)value;
        return s.length>0 && ![s isEqualToString:@"0"];
    }
    if ([value isKindOfClass:NSNumber.class]) return [(NSNumber *)value longLongValue]>0;
    return NO;
}

static BOOL HGFeedBoolGetter(id object, const char *name) {
    if (!object) return NO;
    SEL sel=sel_registerName(name);
    Method m=class_getInstanceMethod(object_getClass(object),sel);
    const char *enc=m ? method_getTypeEncoding(m) : NULL;
    if (!enc || (strcmp(enc,"B16@0:8") && strcmp(enc,"c16@0:8"))) return NO;
    return ((BOOL(*)(id,SEL))objc_msgSend)(object,sel);
}

static BOOL HGFeedIvarContainsAny(const char *ivarName, const char *const *tokens, NSUInteger count) {
    if (!ivarName || !ivarName[0]) return NO;
    NSString *lower=[@(ivarName) lowercaseString];
    for (NSUInteger i=0;i<count;i++)
        if ([lower containsString:@(tokens[i])]) return YES;
    return NO;
}

static BOOL HGFeedIsModelObject(id value) {
    return value && value!=NSNull.null && ![value isKindOfClass:NSString.class] &&
        ![value isKindOfClass:NSNumber.class] && ![value isKindOfClass:NSValue.class] &&
        ![value isKindOfClass:NSData.class] && ![value isKindOfClass:NSDate.class] &&
        ![value isKindOfClass:NSArray.class] && ![value isKindOfClass:NSDictionary.class] &&
        ![value isKindOfClass:NSSet.class] && ![value isKindOfClass:UIResponder.class];
}

static BOOL HGFeedCellMatchesKeywords(id root, NSArray<NSString *> *keywords, BOOL matchesAlbumBadge) {
    if (!HGFeedIsModelObject(root)) return NO;
    static const char *const textGetters[]={"title","subTitle","rawBookName","videoDesc","desc","authorNickname",
        "nickname","bookName","bookAbstract","author","albumName","albumTitle","mixName","seriesName","name",
        "text","content","reasonText","contentText","prefixText","leftText","tagText"};
    static const char *const tagGetters[]={"categoryList","recTags","tags","tagList","labelList"};
    static const char *const albumObjects[]={"albumInfo","albumData","albumModel","albumSeriesInfo","mixInfo",
        "mixStruct","ugcAlbumInfo","pugcAlbumInfo"};
    static const char *const albumIds[]={"albumId","albumName","albumTitle","mixId","mixName"};
    static const char *const albumFlags[]={"isAlbum","isAlbumSeries","isAlbumVideo","isMix","isMixVideo","isPugcAlbum"};
    static const char *const childGetters[]={"filterableSeriesVideoData","filterableEpisodeVideoData",
        "derivateVideoData","filterableDerivateVideoData","pugcVideoData","albumVideoData","videoData",
        "episodeVideoData","seriesVideoData","shortVideoModel","videoDataModel","bookData","seriesInfo",
        "albumInfo","albumData","albumModel","albumSeriesInfo","mixInfo","mixStruct","ugcAlbumInfo",
        "pugcAlbumInfo","distributionInfo","bottomDistributionInfo","bottomBarInfo","tagInfo"};
    static const char *const textIvarTokens[]={"title","name","desc","abstract","author","nick","text","reason","label","prefix"};
    static const char *const tagIvarTokens[]={"tag","category","label"};
    static const char *const albumIvarTokens[]={"album","mix"};
    static const char *const childIvarTokens[]={"video","episode","series","derivate","derivative","pugc","ugc","book","album","mix","distribution","bottom","tag"};

    NSMutableArray *queue=[NSMutableArray arrayWithObject:@[root,@0]];
    NSMutableSet *visited=[NSMutableSet setWithObject:[NSValue valueWithNonretainedObject:root]];
    for (NSUInteger q=0;q<queue.count && q<24;q++) {
        NSArray *entry=queue[q];
        id obj=entry[0];
        NSUInteger depth=[entry[1] unsignedIntegerValue];
        if (matchesAlbumBadge) {
            NSString *clsName=[NSStringFromClass(object_getClass(obj)) lowercaseString];
            if ([clsName containsString:@"album"] || [clsName containsString:@"mix"]) return YES;
            for (NSUInteger i=0;i<sizeof(albumFlags)/sizeof(albumFlags[0]);i++)
                if (HGFeedBoolGetter(obj,albumFlags[i])) return YES;
            for (NSUInteger i=0;i<sizeof(albumIds)/sizeof(albumIds[0]);i++)
                if (HGFeedValueIsNonEmptyId(HGReadObjectGetter(obj,albumIds[i]))) return YES;
            for (NSUInteger i=0;i<sizeof(albumObjects)/sizeof(albumObjects[0]);i++)
                if (HGFeedIsModelObject(HGReadObjectGetter(obj,albumObjects[i]))) return YES;
        }
        for (NSUInteger i=0;i<sizeof(textGetters)/sizeof(textGetters[0]);i++)
            if (HGFeedTextHasKeyword(HGReadObjectGetter(obj,textGetters[i]),keywords)) return YES;
        for (NSUInteger i=0;i<sizeof(tagGetters)/sizeof(tagGetters[0]);i++)
            if (HGFeedTagsHaveKeyword(HGReadObjectGetter(obj,tagGetters[i]),keywords)) return YES;

        void (^enqueue)(id)=^(id child) {
            if (depth>=3 || visited.count>=24 || !HGFeedIsModelObject(child)) return;
            NSValue *key=[NSValue valueWithNonretainedObject:child];
            if ([visited containsObject:key]) return;
            [visited addObject:key];
            [queue addObject:@[child,@(depth+1)]];
        };
        for (NSUInteger i=0;i<sizeof(childGetters)/sizeof(childGetters[0]);i++)
            enqueue(HGReadObjectGetter(obj,childGetters[i]));

        for (Class cls=object_getClass(obj);cls && cls!=NSObject.class;cls=class_getSuperclass(cls)) {
            unsigned int ivarCount=0;
            Ivar *ivars=class_copyIvarList(cls,&ivarCount);
            if (!ivars) continue;
            BOOL matched=NO;
            for (unsigned int i=0;i<ivarCount && i<96;i++) {
                const char *enc=ivar_getTypeEncoding(ivars[i]);
                if (!enc || enc[0]!='@') continue;
                const char *name=ivar_getName(ivars[i]);
                id val=object_getIvar(obj,ivars[i]);
                if (!val || val==NSNull.null) continue;
                if (matchesAlbumBadge && HGFeedIvarContainsAny(name,albumIvarTokens,2) &&
                    (HGFeedValueIsNonEmptyId(val) || HGFeedIsModelObject(val) ||
                     ([val isKindOfClass:NSDictionary.class] && [(NSDictionary *)val count]>0))) {
                    matched=YES;break;
                }
                if ([val isKindOfClass:NSString.class] &&
                    HGFeedIvarContainsAny(name,textIvarTokens,sizeof(textIvarTokens)/sizeof(textIvarTokens[0])) &&
                    HGFeedTextHasKeyword(val,keywords)) {
                    matched=YES;break;
                }
                if ([val isKindOfClass:NSArray.class] &&
                    HGFeedIvarContainsAny(name,tagIvarTokens,sizeof(tagIvarTokens)/sizeof(tagIvarTokens[0])) &&
                    HGFeedTagsHaveKeyword(val,keywords)) {
                    matched=YES;break;
                }
                if (HGFeedIsModelObject(val) &&
                    HGFeedIvarContainsAny(name,childIvarTokens,sizeof(childIvarTokens)/sizeof(childIvarTokens[0])))
                    enqueue(val);
            }
            free(ivars);
            if (matched) return YES;
        }
    }
    return NO;
}

static BOOL HGIsFilteredRecommendPeopleCell(id<HGKeywordFeedSource> source, id cell) {
    if (!cell || !HGAppearanceEnabled() || !HGAppearanceFlag(@"player.hideFollowRecommendUsers")) return NO;
    if (source && [source isRecommendPeopleCellViewData:cell]) return YES;
    NSString *clsName = NSStringFromClass(object_getClass(cell));
    return [clsName containsString:@"RecommendPeople"];
}

static NSArray *HGFilterFeedPageForSource(id<HGKeywordFeedSource> source, NSArray *cells, NSArray<NSString *> *keywords) {
    if (![cells isKindOfClass:NSArray.class] || !cells.count) return cells;
    BOOL hideRecUsers = HGAppearanceEnabled() && HGAppearanceFlag(@"player.hideFollowRecommendUsers");
    if (!keywords.count && !hideRecUsers) return cells;
    BOOL matchesAlbumBadge = keywords.count && HGFeedTextHasKeyword(@"合集",keywords);
    NSMutableIndexSet *hidden = nil;
    for (NSUInteger i = 0; i < cells.count; i++) {
        id cell = cells[i];
        BOOL drop = (hideRecUsers && HGIsFilteredRecommendPeopleCell(source, cell)) ||
            (keywords.count && HGFeedCellMatchesKeywords(cell, keywords, matchesAlbumBadge));
        if (!drop) continue;
        if (!hidden) hidden = [NSMutableIndexSet indexSet];
        [hidden addIndex:i];
    }
    if (!hidden) return cells;
    NSMutableArray *kept = [cells mutableCopy];
    [kept removeObjectsAtIndexes:hidden];
    return kept;
}

typedef void (*HGKeywordFeedBuilder)(id, SEL, NSArray *, id, id);
static HGForwardingAdHook HGKeywordFeedHook;
@protocol HGKeywordFeedPage <NSObject>
- (BOOL)videoFeedRefreshing;
- (id)currentVideoFeedListDataSource;
- (void)commonRefreshPageWithReloadPage:(BOOL)reload clientReqType:(int)type withCallBack:(void (^)(void))callback;
@end
static NSHashTable<UIViewController<HGKeywordFeedPage> *> *HGKeywordFeedPages;
static NSUInteger HGKeywordRevision;
static char HGKeywordPageRevisionKey, HGKeywordPageVisibleKey;
static HGForwardingAdHook HGKeywordAppearHook, HGKeywordDisappearHook, HGKeywordFinishHook;

static void HGRefreshKeywordFeedIfNeeded(UIViewController<HGKeywordFeedPage> *page) {
    if (!page.viewIfLoaded.window || ![objc_getAssociatedObject(page,&HGKeywordPageVisibleKey) boolValue] ||
        [objc_getAssociatedObject(page,&HGKeywordPageRevisionKey) unsignedIntegerValue]==HGKeywordRevision ||
        [page videoFeedRefreshing]) return;
    id source=[page currentVideoFeedListDataSource];
    if (![source isKindOfClass:objc_lookUpClass("SSShortVideoRecommendOutterListDataSource")]) return;
    objc_setAssociatedObject(page,&HGKeywordPageRevisionKey,@(HGKeywordRevision),OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    // The container uses these same arguments to rebuild its existing feed
    // and publishes the new video and mixed-item indexes as one refresh.
    [page commonRefreshPageWithReloadPage:YES clientReqType:1 withCallBack:nil];
}
static void HGKeywordPageAppeared(UIViewController<HGKeywordFeedPage> *page, SEL cmd) {
    ((void(*)(id,SEL))atomic_load(&HGKeywordAppearHook.original))(page,cmd);
    if (!HGKeywordFeedPages) HGKeywordFeedPages=[NSHashTable weakObjectsHashTable];
    [HGKeywordFeedPages addObject:page];
    objc_setAssociatedObject(page,&HGKeywordPageVisibleKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    HGRefreshKeywordFeedIfNeeded(page);
}
static void HGKeywordPageDisappearing(id page, SEL cmd, NSUInteger reason) {
    objc_setAssociatedObject(page,&HGKeywordPageVisibleKey,@NO,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    ((void(*)(id,SEL,NSUInteger))atomic_load(&HGKeywordDisappearHook.original))(page,cmd,reason);
}
static void HGKeywordPageFinished(UIViewController<HGKeywordFeedPage> *page, SEL cmd, id context) {
    ((void(*)(id,SEL,id))atomic_load(&HGKeywordFinishHook.original))(page,cmd,context);
    HGRefreshKeywordFeedIfNeeded(page);
}

static void HGBuildKeywordFeedPage(id<HGKeywordFeedSource> source, SEL cmd, NSArray *cells, id beforeBuild, id completion) {
    HGKeywordFeedBuilder original=(HGKeywordFeedBuilder)atomic_load_explicit(&HGKeywordFeedHook.original,memory_order_acquire);
    NSArray *kept=HGFilterFeedPageForSource(source,cells,HGVideoBlockedKeywords());
    if (kept==cells || kept.count || ![source hasMore]) {
        original(source,cmd,kept,beforeBuild,completion);
        return;
    }
    NSInteger offset=[source nextOffset], cellId=[source nextLoadMoreCellId];
    [source requestLoadMoreDataWithCellId:cellId completion:^(id<HGKeywordFeedResult> result) {
        if (![result isKindOfClass:objc_lookUpClass("SSShortVideoRecommendOutterCellChangeResult")]) {
            // The 7.4.5 host also supplies alternate implementations of its load-more result protocol.
            Protocol *hostProtocol=objc_getProtocol("SSShortVideoRecommendOutterLoadMoreResultProtocol");
            if (!hostProtocol || ![result conformsToProtocol:hostProtocol]) {
                original(source,cmd,kept,beforeBuild,completion);
                return;
            }
        }
        id container=result.cellViewData;
        if (![result isSuccess] || [source nextOffset]!=offset || [source nextLoadMoreCellId]!=cellId ||
            !container || ![source canHandleVideoFeedCellViewData:container]) {
            original(source,cmd,kept,beforeBuild,completion);
            return;
        }
        NSArray *next=[source isRecommendPeopleCellViewData:container] ? @[container] : HGReadObjectGetter(container,"cellData");
        if (![next isKindOfClass:NSArray.class]) {
            original(source,cmd,kept,beforeBuild,completion);
            return;
        }
        NSInteger nextOffset=result.nextOffset;
        BOOL more=result.hasMore;
        if (more && nextOffset<offset) {
            original(source,cmd,kept,beforeBuild,completion);
            return;
        }
        [source updatePagingWithCellChangeResult:result];
        if (!more || nextOffset<=offset || !next.count) {
            original(source,cmd,HGFilterFeedPageForSource(source,next,HGVideoBlockedKeywords()),beforeBuild,completion);
            return;
        }
        // Cached results may call back synchronously. Start the next pass on
        // the host UI queue so consecutive hidden pages never consume the stack.
        dispatch_async(dispatch_get_main_queue(), ^{
            if ([source nextOffset]!=nextOffset) original(source,cmd,kept,beforeBuild,completion);
            else HGBuildKeywordFeedPage(source,cmd,next,beforeBuild,completion);
        });
    }];
}

static void HGInstallKeywordFeedFilter(void) {
    Class source=objc_lookUpClass("SSShortVideoRecommendOutterListDataSource");
    Class result=objc_lookUpClass("SSShortVideoRecommendOutterCellChangeResult");
    if (!HGAdMethodMatches(result,@selector(isSuccess),"B16@0:8") || !HGAdMethodMatches(result,@selector(hasMore),"B16@0:8") ||
        !HGAdMethodMatches(result,@selector(nextOffset),"q16@0:8") || !HGAdMethodMatches(result,@selector(cellViewData),"@16@0:8") ||
        !HGAdMethodMatches(source,@selector(hasMore),"B16@0:8") || !HGAdMethodMatches(source,@selector(nextOffset),"q16@0:8") ||
        !HGAdMethodMatches(source,@selector(nextLoadMoreCellId),"q16@0:8") ||
        !HGAdMethodMatches(source,@selector(canHandleVideoFeedCellViewData:),"B24@0:8@16") ||
        !HGAdMethodMatches(source,@selector(isRecommendPeopleCellViewData:),"B24@0:8@16") ||
        !HGAdMethodMatches(source,@selector(updatePagingWithCellChangeResult:),"v24@0:8@16") ||
        !HGAdMethodMatches(source,@selector(requestLoadMoreDataWithCellId:completion:),"v32@0:8q16@?24")) return;
    HGInstallForwardingAdHook(source,
        sel_registerName("reqVideoDataIfNeedWithCellViewDataArr:beforeBuild:completion:"),"v40@0:8@16@?24@?32",
        (IMP)HGBuildKeywordFeedPage,&HGKeywordFeedHook);
    Class page=objc_lookUpClass("SSVideoFeedContainerViewController");
    if (!HGKeywordFeedHook.installed || !HGAdMethodMatches(page,@selector(videoFeedRefreshing),"B16@0:8") ||
        !HGAdMethodMatches(page,@selector(currentVideoFeedListDataSource),"@16@0:8") ||
        !HGAdMethodMatches(page,@selector(commonRefreshPageWithReloadPage:clientReqType:withCallBack:),"v32@0:8B16i20@?24")) return;
    HGInstallForwardingAdHook(page,sel_registerName("feedHostDidAppear"),"v16@0:8",(IMP)HGKeywordPageAppeared,&HGKeywordAppearHook);
    HGInstallForwardingAdHook(page,sel_registerName("feedHostWillDisappearForReason:"),"v24@0:8Q16",(IMP)HGKeywordPageDisappearing,&HGKeywordDisappearHook);
    HGInstallForwardingAdHook(page,sel_registerName("videoListVCContextFinishRefreshingData:"),"v24@0:8@16",(IMP)HGKeywordPageFinished,&HGKeywordFinishHook);
    static dispatch_once_t once;
    dispatch_once(&once,^{
        [NSNotificationCenter.defaultCenter addObserverForName:HGVideoBlockedKeywordsDidChangeNotification
            object:nil queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification *note) {
            HGKeywordRevision++;
            for (UIViewController<HGKeywordFeedPage> *page in HGKeywordFeedPages.allObjects)
                HGRefreshKeywordFeedIfNeeded(page);
        }];
    });
}

// A previously prepared homepage ad can re-enter the list without another
// homepage request. Both insertion APIs return 2 when canInsertModel is NO.
static void HGInstallHomepageAdModelFilter(void) {
    Class adModel=objc_lookUpClass("SSBizVideoAdInsertMaterielData");
    SEL homepage=sel_registerName("isInShortVideoHomePage");
    if (!HGAdMethodMatches(adModel,homepage,"B16@0:8")) return;
    static BOOL installed[2][2];
    const char *owners[]={"FQVShortVideoListVCInsertComponent","FQVShortVideoListVCLandSpaceInsertComponent"};
    const char *selectors[]={"insertModel:videoDataModelUUID:","insertModel:videoIndex:isAfter:"};
    const char *encodings[]={"q32@0:8@16q24","q36@0:8@16q24B32"};
    for (NSUInteger c=0;c<2;c++) for (NSUInteger i=0;i<2;i++) {
        if (installed[c][i]) continue;
        Class owner=objc_lookUpClass(owners[c]);
        SEL sel=sel_registerName(selectors[i]);
        if (!HGAdMethodMatches(owner,sel,encodings[i])) continue;
        __block IMP original=class_getMethodImplementation(owner,sel);
        BOOL (^blocked)(id)=^BOOL(id model) {
            return HGIsSkippableNaturalAd(model) ||
                (object_getClass(model)==adModel && ((BOOL(*)(id,SEL))objc_msgSend)(model,homepage));
        };
        IMP hook=i ? imp_implementationWithBlock(^NSInteger(id target,id model,NSInteger index,BOOL after) {
            if (blocked(model)) return (NSInteger)2;
            return ((NSInteger(*)(id,SEL,id,NSInteger,BOOL))original)(target,sel,model,index,after);
        }) : imp_implementationWithBlock(^NSInteger(id target,id model,NSInteger uuid) {
            if (blocked(model)) return (NSInteger)2;
            return ((NSInteger(*)(id,SEL,id,NSInteger))original)(target,sel,model,uuid);
        });
        IMP prior=HGInstallVerifiedHook(owner,sel,encodings[i],hook,@"ads");
        if (prior) { original=prior; installed[c][i]=YES; }
        else imp_removeBlock(hook);
    }
}

void HGInstallAds(void) {
    HGInstallKeywordFeedFilter();
    HGInstallSidebarLists();
    BOOL enabled[HGFeatureCount] = {NO};
    for (HGFeature feature = HGSplash; feature < HGFeatureCount; feature++) {
        enabled[feature] = HGIsEnabled(feature);
    }
    if (enabled[HGFeed]) HGInstallHomepageAdModelFilter();

    NSUInteger installed = 0;
    NSUInteger selected = 0;
    for (NSUInteger index = 0; index < sizeof(HGGates) / sizeof(HGGates[0]); index++) {
        const HGGate *gate = &HGGates[index];
        // Keep the older gate on hosts without the corresponding list getter.
        if (gate->feature==HGSidebarGameCenter) {
            BOOL handlesList=NO;
            for (NSUInteger j=0; j<sizeof(HGSidebarLists)/sizeof(*HGSidebarLists); j++) {
                const HGSidebarList *list=&HGSidebarLists[j];
                if (list->component && list->installed && !strcmp(list->owner,gate->className)) {handlesList=YES;break;}
            }
            if (handlesList) continue;
        }
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
