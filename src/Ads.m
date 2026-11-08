#import "HongguoGlass.h"

typedef struct {
    HGFeature feature;
    const char *className;
    const char *selector;
    const char *encoding;
    IMP replacement;
    BOOL classMethod;
} HGGate;

static BOOL HGNo(id self, SEL cmd) { return NO; }
static BOOL HGNoObject(id self, SEL cmd, id argument) { return NO; }
static BOOL HGNoObjects(id self, SEL cmd, id first, id second) { return NO; }
static BOOL HGNoObjectClass(id self, SEL cmd, id argument, Class viewClass) { return NO; }
static BOOL HGYesObject(id self, SEL cmd, id argument) { return YES; }
static void HGSkipSurvey(id self, SEL cmd, NSInteger style, id videoID) {}
static void HGSkipPendant(id self, SEL cmd, UIView *parent) {}

static void HGSkipPatch(id self, SEL cmd, id context, id seriesID, id videoID,
    double currentTime, void (^ready)(id), void (^remove)(void)) {
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
};

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
            NSLog(@"[HongguoGlass] 跳过不匹配的入口 %s %s", gate->className, gate->selector);
        }
    }

    NSLog(@"[HongguoGlass] 广告与营销入口 %lu/%lu", (unsigned long)installed, (unsigned long)selected);
}
