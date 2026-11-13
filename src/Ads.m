#import "Honggo.h"
#include <stdatomic.h>

typedef struct {
    HGFeature feature;
    const char *className;
    const char *selector;
    const char *encoding;
    IMP replacement;
    BOOL classMethod;
} HGGate;

static void HGAdDecision(id object, SEL selector, BOOL result);
static BOOL HGNo(id self, SEL cmd) { HGAdDecision(self, cmd, NO); return NO; }
static BOOL HGNoObject(id self, SEL cmd, id argument) { HGAdDecision(self, cmd, NO); return NO; }
static BOOL HGNoObjects(id self, SEL cmd, id first, id second) { HGAdDecision(self, cmd, NO); return NO; }
static BOOL HGNoTimeObject(id self, SEL cmd, double time, id context) { HGAdDecision(self, cmd, NO); return NO; }
static BOOL HGNoTimesObject(id self, SEL cmd, double time, double duration, id context) { HGAdDecision(self, cmd, NO); return NO; }
static BOOL HGNoObjectClass(id self, SEL cmd, id argument, Class viewClass) { HGAdDecision(self, cmd, NO); return NO; }
static BOOL HGYesObject(id self, SEL cmd, id argument) { HGAdDecision(self, cmd, YES); return YES; }
static id HGNoModel(id self, SEL cmd, id argument) { HGAdDecision(self, cmd, NO); return nil; }
static id HGNoModels(id self, SEL cmd, id data, id context) { HGAdDecision(self, cmd, NO); return nil; }
static void HGSkipSurvey(id self, SEL cmd, NSInteger style, id videoID) { HGAdDecision(self, cmd, NO); }
static void HGSkipPendant(id self, SEL cmd, UIView *parent) { HGAdDecision(self, cmd, NO); }

static void HGSkipPatch(id self, SEL cmd, id context, id seriesID, id videoID,
    double currentTime, void (^ready)(id), void (^remove)(void)) {
    HGAdDecision(self, cmd, NO);
    // Follow the host's declined-ad path so playback callers get their completion.
    if (ready) {
        ready(nil);
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
    {HGPatch, "SSBizVideoPatchAdService", "showPatchAdViewWithContext:seriesId:vId:currentTime:patchAdViewReadyBlock:removeBlock:", "v64@0:8@16@24@32d40@?48@?56", (IMP)HGSkipPatch},
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
        if (!enabled[gate->feature]) {
            continue;
        }

        selected++;
        Class target = objc_lookUpClass(gate->className);
        if (gate->classMethod) {
            target = object_getClass(target);
        }
        if (HGReplaceMethod(target, sel_registerName(gate->selector), gate->encoding, gate->replacement)) {
            installed++;
        } else {
            NSLog(@"[Honggo] 跳过不匹配的入口 %s %s", gate->className, gate->selector);
        }
    }

    NSLog(@"[Honggo] 广告与营销入口 %lu/%lu", (unsigned long)installed, (unsigned long)selected);
}
