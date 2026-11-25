#import "Appearance.h"
#import "Honggo.h"
#import <CoreText/CoreText.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <QuartzCore/QuartzCore.h>
#import <CommonCrypto/CommonDigest.h>
#import <objc/message.h>
#import <dispatch/dispatch.h>
#include <mach-o/dyld.h>
#include <math.h>
#include <string.h>

// Original Honggo implementation. Adapter names are interoperability facts;
// no reference-project implementation, graphics or fonts are incorporated.
static BOOL HGAApplying;
static NSHashTable<UIView *> *HGAManaged;
static NSHashTable<UIViewController *> *HGAControllers;
static NSHashTable<UIView *> *HGABars;
static void HGARefreshAssets(void);
static void HGAApplyTypography(UIView *root);
static void HGAObserveFeed(UIScrollView *feed);
static void HGARegisterCategoryFeed(UIScrollView *feed);
static void HGARefreshDock(UIView *bar);
static NSArray<NSString *> *HGABottomControlClasses(void);
static void HGAUpdateBottomLayoutForView(UIView *view);
static void HGARestoreBottomLayoutForView(UIView *view);
static void HGARelinquishBottomLayout(UIView *view);
static BOOL HGAIsBottomControl(UIView *view);

static void HGAResumeSettledHUDs(UIScrollView *feed);

// The native collection can be assembling cells or transferring a renderer
// inside these callbacks. Read its settled hierarchy once after they unwind.
void HGScheduleFeedLayoutRefresh(UIScrollView *feed) {
    static char scheduledKey;
    if (!NSThread.isMainThread || !feed.window || !HGAppearanceEnabled() || HGFeedSidebarOwnsLayout(feed.window) ||
        objc_getAssociatedObject(feed, &scheduledKey)) return;
    objc_setAssociatedObject(feed, &scheduledKey, @YES, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    __weak UIScrollView *weakFeed = feed;
    CFRunLoopPerformBlock(CFRunLoopGetMain(), kCFRunLoopCommonModes, ^{
        UIScrollView *current = weakFeed;
        if (!current) return;
        @try {
            if (current.window && !HGFeedSidebarOwnsLayout(current.window)) {
                HGAResumeSettledHUDs(current);
                HGRefreshVideoBackdropForFeed(current);
            }
        } @finally {
            objc_setAssociatedObject(current, &scheduledKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
    });
    CFRunLoopWakeUp(CFRunLoopGetMain());
}

static void HGARefreshBackdropAfterScroll(id scroll) {
    if ([scroll isKindOfClass:objc_lookUpClass("FQVCollectionView")] && [scroll isKindOfClass:UIScrollView.class])
        HGScheduleFeedLayoutRefresh(scroll);
}
static CGRect HGADockRectInView(UIView *coordinateView);
static CGRect HGAExpandedCapsuleInWindow(UIWindow *window);

UIVisualEffect *HGAppearanceMaterial(NSString *style) {
    if (UIAccessibilityIsReduceTransparencyEnabled() || [style isEqual:@"solid"]) return nil;
    // The blur option uses the traditional iOS 18-era Light/Dark blur pair.
    // It must not be upgraded to Liquid Glass or stack a second material.
    if([style isEqual:@"blur"])return [UIBlurEffect effectWithStyle:UIBlurEffectStyleRegular];
    if (@available(iOS 26.0, *)) {
        if (![style isEqual:@"blur"] && HGNativeGlassConfigured()) {
            UIGlassEffect *glass = [UIGlassEffect effectWithStyle:
                [style isEqual:@"clear"] ? UIGlassEffectStyleClear : UIGlassEffectStyleRegular];
            glass.interactive = NO;
            return glass;
        }
    }
    return [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemMaterial];
}
static BOOL HGAOwnView(UIView *view) {
    return [view.accessibilityIdentifier hasPrefix:@"honggo.appearance."];
}
static NSArray<UIView *> *HGADescendants(UIView *root, NSUInteger budget) {
    if (!root || !budget) return @[];
    NSMutableArray *queue = [NSMutableArray arrayWithObject:root];
    for (NSUInteger i=0; i<queue.count && queue.count<budget; i++) {
        UIView *node = queue[i];
        if (HGAOwnView(node)) continue;
        for (UIView *child in node.subviews) {
            if (queue.count >= budget) break;
            if (!HGAOwnView(child)) [queue addObject:child];
        }
    }
    return queue;
}
static BOOL HGAVisible(UIView *view) {
    if (!view.window || CGRectIsEmpty(view.bounds)) return NO;
    for (UIView *node=view; node; node=node.superview)
        if (node.hidden || node.alpha<0.01) return NO;
    return YES;
}
static BOOL HGABoolGetter(id object, NSString *name, BOOL *value) {
    if (!object) return NO;
    SEL selector=NSSelectorFromString(name);
    Method method=class_getInstanceMethod(object_getClass(object),selector);
    const char *encoding=method ? method_getTypeEncoding(method) : NULL;
    if (!encoding || (strcmp(encoding,"B16@0:8") && strcmp(encoding,"c16@0:8"))) return NO;
    *value=((BOOL (*)(id,SEL))objc_msgSend)(object,selector);
    return YES;
}
static UIView *HGAViewGetter(id object, const char *name, UIView *ancestor) {
    id value=HGReadObjectGetter(object,name);
    return [value isKindOfClass:UIView.class] && (value==ancestor || [value isDescendantOfView:ancestor]) ? value : nil;
}
static void HGARegisterView(UIView *view) {
    if (!HGAManaged) HGAManaged=[NSHashTable weakObjectsHashTable];
    [HGAManaged addObject:view];
}
static UIViewController *HGAOwner(UIView *view) {
    UIResponder *node=view;
    for (NSUInteger i=0; node && i<32; i++,node=node.nextResponder)
        if ([node isKindOfClass:UIViewController.class]) return (id)node;
    return nil;
}
static NSError *HGAAssetError(NSString *message) {
    return [NSError errorWithDomain:@"Honggo.AppearanceAssets" code:1
        userInfo:@{NSLocalizedDescriptionKey:message}];
}
#include "AppearanceAssets.inc"
#include "AppearanceViews.inc"
#include "AppearanceBottomLayout.inc"
#include "AppearanceDock.inc"
#include "AppearanceSettings.inc"
#include "AppearanceCommentMovement.inc"
#include "AppearancePlaybackQuality.inc"
#include "AppearanceNowPlaying.inc"

// Exact adapters only. UIKit base classes and arbitrary third-party classes
// never receive a global appearance hook. Missing adapters remain untouched.
static NSMutableDictionary<NSString *,NSValue *> *HGAHooks;
static BOOL HGAInsideFeedShareTap;
static NSUInteger HGAPlayerTransferInvocation;
static void HGATracePlayerTransfer(id object,SEL selector,id destination,NSUInteger invocation,NSString *phase) {
    if(!invocation)return;
    HGLog(@"appearance.player-transfer",@{@"class":NSStringFromClass([object class]),
        @"method":NSStringFromSelector(selector),@"attached":@(destination!=nil),
        @"fullscreen":@(HGAppearanceFlag(@"player.fullscreen")),@"phase":phase,
        @"invocation":@(invocation),@"mode":@"render-sublayers"});
}
// Only native channel completion callbacks reach this trace; no per-frame
// scroll logging. Pair the markers to distinguish native and deferred work.
static void HGATraceCategoryCallback(SEL selector, NSString *phase) {
    if (HGEnhancedDiagnostics())
        HGLog(@"appearance.channel", @{@"selector": NSStringFromSelector(selector), @"phase": phase});
}
static void HGAHook(Class cls, SEL selector, const char *encoding, BOOL controller) {
    NSString *key=[NSString stringWithFormat:@"%@/%@",NSStringFromClass(cls),NSStringFromSelector(selector)];
    if (HGAHooks[key]) return;
    Method method=class_getInstanceMethod(cls,selector);
    const char *actual=method?method_getTypeEncoding(method):NULL;
    // Publish a checked original before installing the replacement; UIKit can
    // reach a lifecycle observer as soon as class_replaceMethod returns.
    __block IMP original=actual && !strcmp(actual,encoding)?method_getImplementation(method):NULL;
    NSString *className=NSStringFromClass(cls);
    BOOL playerCanvas=[className isEqual:@"FQVShortVideoPlayerCellView"];
    BOOL backdropChange=playerCanvas && (selector==sel_registerName("refreshBgColor") ||
        selector==sel_registerName("refreshCoverImageView"));
    BOOL player=[className isEqual:@"TTPlayerView"] || [className isEqual:@"FQVShortVideoPlayerCellView"];
    BOOL mediaPlayer=[className isEqual:@"FQVPlayer"];
    BOOL metalLayout=[className isEqual:@"TTMetalView"] && selector==@selector(layoutSubviews);
    BOOL scrollComponent=[className isEqual:@"FQVShortVideoListVCScrollComponent"];
    BOOL categoryPager=[className isEqual:@"FQRJXCategoryListContainerView"];
    BOOL feedShareTap=[className isEqual:@"SSShortVideoRightInteractionShareViewManager"] &&
        selector==sel_registerName("shareViewDidClick:");
    BOOL sharePresentation=[className isEqual:@"SSShortVideoSharePanelPresenter"] &&
        selector==sel_registerName("presentationMode");
    BOOL playerNavigation=[className isEqual:@"FQVShortVideoListVCNavBarComponent"];
    BOOL qualityPlayback=[className isEqual:@"FQVShortVideoListVCPlayComponent"];
    BOOL scrubClipping=[className isEqual:@"FQVShortVideoVPlayerControlView"] &&
        selector==sel_registerName("handleOutterFeedClipBounds:");
    BOOL episode=[HGAViewRoles()[className] isEqual:@"episodes"];
    BOOL episodeWrapper=[className isEqual:@"FQVShortVideoBaseInsertView"];
    BOOL bottomParticipant=episodeWrapper || [HGABottomControlClasses() containsObject:className] ||
        [className isEqual:@"FQVShortVideoProgressView"] ||
        [className isEqual:@"SSShortVideoDerivateInfoView"] || [className isEqual:@"SSShortVideoOutterSeriesInfoView"];
    IMP replacement=NULL;
    if (sharePresentation && !strcmp(encoding,"q16@0:8")) {
        replacement=imp_implementationWithBlock(^NSInteger(id presenter) {
            NSInteger mode=((NSInteger(*)(id,SEL))original)(presenter,selector);
            return mode==2 && NSThread.isMainThread && HGAInsideFeedShareTap ? 1 : mode;
        });
    } else if (qualityPlayback && !strcmp(encoding,"v24@0:8@16")) {
        replacement=imp_implementationWithBlock(^(id component,id model) {
            ((void(*)(id,SEL,id))original)(component,selector,model);
            HGASchedulePlaybackQuality(component,model,nil);
        });
    } else if (feedShareTap && !strcmp(encoding,"v24@0:8@16")) {
        replacement=imp_implementationWithBlock(^(id manager,id shareView) {
            if (!NSThread.isMainThread || !HGAPanelWantsGlass(@"share")) {
                ((void(*)(id,SEL,id))original)(manager,selector,shareView);
                return;
            }
            BOOL wasInside=HGAInsideFeedShareTap;
            HGAInsideFeedShareTap=YES;
            @try {((void(*)(id,SEL,id))original)(manager,selector,shareView);}
            @finally {HGAInsideFeedShareTap=wasInside;}
        });
    } else if ([className isEqual:@"FQCShortVideoCommentPanelViewController"] && selector==sel_registerName("setupTabItems")) {
        replacement=imp_implementationWithBlock(^(id panel) {
            ((void(*)(id,SEL))original)(panel,selector);
            HGAPrimeEpisodeCommentCount(panel);
        });
    } else if (selector==sel_registerName("setPlayerViewFrame:") &&
               !strcmp(encoding,"v48@0:8{CGRect={CGPoint=dd}{CGSize=dd}}16")) {
        replacement=imp_implementationWithBlock(^(UIView *hud,CGRect frame) {
            ((void(*)(id,SEL,CGRect))original)(hud,selector,frame);
            if(hud.window && !HGAApplying)HGRefreshVideoBackdropForPlayer(hud);
        });
    } else if (!strcmp(encoding,"v16@0:8")) {
        replacement=imp_implementationWithBlock(^(id object) {
            BOOL recycling=playerCanvas && selector==sel_registerName("resetToRecycle") && NSThread.isMainThread;
            BOOL wasApplying=HGAApplying;
            if(recycling)HGAApplying=YES;
            @try {
                if(recycling) {
                    HGVideoBackdropPrepareRendererTransfer(object);
                    HGAResetPlayerFillState(object);
                }
                ((void(*)(id,SEL))original)(object,selector);
            } @finally {if(recycling)HGAApplying=wasApplying;}
            if(mediaPlayer) {
                if([(UIView *)object window] && !HGAApplying)HGRefreshVideoBackdropForPlayer(object);
                return;
            }
            if(metalLayout) {
                UIView *renderer=[(UIView *)object superview];
                if(renderer.window && [renderer isKindOfClass:objc_lookUpClass("TTPlayerView")] &&
                   (HGAppearanceDockEnabled() || (HGAppearanceEnabled() && HGAppearanceFlag(@"player.controlsGlass"))))
                    HGRefreshVideoBackdropForPlayer(renderer);
                return;
            }
            if(backdropChange) {
                if(!HGAApplying)HGRefreshVideoBackdropForPlayer(object);
                return;
            }
            if(player && !controller && (selector==@selector(didMoveToWindow) || selector==@selector(didMoveToSuperview))) {
                HGAFinishPlayerTransfer(object,selector);
                if ([(UIView *)object window]) {
                    HGRefreshVideoBackdropForPlayer(object);
                    HGScheduleFeedLayoutRefresh((UIScrollView *)HGAAncestorNamed(object,@"FQVCollectionView"));
                }
            }
            if(episodeWrapper)HGAEpisodeWrapperChanged(object);
            if(playerNavigation){HGAApplyPlayerNavigation(object);return;}
            if (controller) {
                HGAppearanceProcessController(object);
                if([@[@"FQVShortVideoListViewController",@"SSShortVideoDetailViewController"] containsObject:className])
                    for(id component in HGAPlayerNavigations.allObjects)HGAApplyPlayerNavigation(component);
            }
            else if ((selector==@selector(didMoveToWindow) || selector==@selector(didMoveToSuperview)) && [(UIView *)object window]) {
                HGAApplyCleanView(object);
                [(UIView *)object setNeedsLayout];
            }
            else HGAppearanceProcessView(object);
        });
    } else if (!strcmp(encoding,"v24@0:8@16")) {
        replacement=imp_implementationWithBlock(^(id object,id argument) {
            BOOL transfer=player && (selector==@selector(willMoveToSuperview:) || selector==@selector(willMoveToWindow:));
            BOOL episodeMove=episode && selector==@selector(willMoveToSuperview:);
            BOOL bottomMove=bottomParticipant && selector==@selector(willMoveToSuperview:);
            BOOL modelChange=playerCanvas && selector==sel_registerName("setModel:");
            BOOL guarded=(transfer || episodeMove || bottomMove || modelChange) && NSThread.isMainThread;
            BOOL wasApplying=HGAApplying,completed=NO;
            NSUInteger invocation=guarded && transfer && HGEnhancedDiagnostics()?++HGAPlayerTransferInvocation:0;
            HGATracePlayerTransfer(object,selector,argument,invocation,@"enter");
            if(guarded)HGAApplying=YES;
            if(categoryPager) HGATraceCategoryCallback(selector, @"enter");
            @try {
                if(guarded && transfer) {
                    HGAPreparePlayerTransfer(object,selector);
                    HGVideoBackdropPrepareRendererTransfer(object);
                    HGATracePlayerTransfer(object,selector,argument,invocation,@"prepared");
                }
                if(guarded && episodeMove)HGAApplyEpisodeCorners(object,NO,0);
                if(guarded && episodeWrapper && bottomMove)HGAEpisodeWrapperWillMove(object);
                if(guarded && (episodeMove || bottomMove))HGARestoreBottomLayoutForView(object);
                if(guarded && modelChange)HGAPlayerModelWillChange(object,argument);
                ((void(*)(id,SEL,id))original)(object,selector,argument);
                completed=YES;
            } @finally {
                if(guarded)HGAApplying=wasApplying;
                HGATracePlayerTransfer(object,selector,argument,invocation,completed?@"return":@"exception");
            }
            if(selector==sel_registerName("setLeftContainer:")) {HGAApplyCleanView(object);return;}
            if(player && selector==@selector(didAddSubview:)) {
                if([argument isKindOfClass:objc_lookUpClass("TTMetalView")])
                    HGRefreshVideoBackdropForPlayer(object);
                return;
            }
            if(episodeWrapper && selector==@selector(didAddSubview:))HGAEpisodeWrapperChanged(object);
            if(playerNavigation)HGAApplyPlayerNavigation(object);
            else if(categoryPager) {
                HGATraceCategoryCallback(selector, @"return");
                HGAResumeCategoryFeeds(object);
                HGAHomeChannelPagerDidSettle(object);
            }
            else if(scrollComponent)HGARefreshBackdropAfterScroll(argument);
            else if(controller)HGAppearanceProcessController(object);
            else if(!transfer && !episodeMove && !bottomMove)HGAppearanceProcessView(object);
        });
    } else if (!strcmp(encoding,"v28@0:8@16B24")) {
        BOOL collectRefresh=[className isEqual:@"SSShortVideoRightInteractionCollectView"] &&
            selector==sel_registerName("refreshUIWithModel:animated:");
        replacement=imp_implementationWithBlock(^(id object,id argument,BOOL animated) {
            if(categoryPager) HGATraceCategoryCallback(selector, @"enter");
            // The host animation draws a five-point star over the image. Use
            // its static redraw when the user has chosen the alternate glyph.
            BOOL nativeAnimation=animated && !(collectRefresh && HGAppearanceEnabled() &&
                [HGAppearanceString(@"player.collectGlyph") isEqual:@"sparkles"]);
            ((void(*)(id,SEL,id,BOOL))original)(object,selector,argument,nativeAnimation);
            if(categoryPager) {
                HGATraceCategoryCallback(selector, @"return");
                if(!animated) {
                    HGAResumeCategoryFeeds(object);
                    HGAHomeChannelPagerDidSettle(object);
                }
            } else if(scrollComponent) {
                if(!animated)HGARefreshBackdropAfterScroll(argument);
            } else HGAppearanceProcessView(object);
        });
    } else if (categoryPager && !strcmp(encoding,"v24@0:8q16")) {
        replacement=imp_implementationWithBlock(^(id object, NSInteger index) {
            HGATraceCategoryCallback(selector, @"enter");
            ((void(*)(id,SEL,NSInteger))original)(object,selector,index);
            HGATraceCategoryCallback(selector, @"return");
            HGAResumeCategoryFeeds(object);
            HGAHomeChannelPagerDidSettle(object);
            HGAFinderCategoryDidAppear(object);
        });
    } else if (!strcmp(encoding,"v32@0:8@16@24")) {
        replacement=imp_implementationWithBlock(^(id object,id first,id second) {
            ((void(*)(id,SEL,id,id))original)(object,selector,first,second);
            if(qualityPlayback)HGASchedulePlaybackQuality(object,nil,first);
            else HGAppearanceProcessView(object);
        });
    } else if (selector==@selector(collectionView:willDisplayCell:forItemAtIndexPath:) &&
        !strcmp(encoding,"v40@0:8@16@24@32")) {
        replacement=imp_implementationWithBlock(^(id object,UICollectionView *collection,UICollectionViewCell *cell,NSIndexPath *path) {
            ((void(*)(id,SEL,id,id,id))original)(object,selector,collection,cell,path);
            HGAppearanceProcessView(cell);
        });
    } else if (!strcmp(encoding,"v36@0:8B16d20B28B32")) {
        replacement=imp_implementationWithBlock(^(id object,BOOL interactive,double ratio,BOOL visible,BOOL animated) {
            BOOL owned=HGABeginNativeHeaderToolTransition(object,animated);
            ((void(*)(id,SEL,BOOL,double,BOOL,BOOL))original)(object,selector,interactive,ratio,visible,animated);
            HGAppearanceProcessView(object);
            if(owned)HGAFinishNativeHeaderToolTransition(object);
        });
    } else if (!strcmp(encoding,"v28@0:8B16d20")) {
        replacement=imp_implementationWithBlock(^(id object,BOOL visible,double top) {
            ((void(*)(id,SEL,BOOL,double))original)(object,selector,visible,top);
            HGAppearanceProcessView(object);
        });
    } else if (!strcmp(encoding,"v28@0:8B16@20")) {
        replacement=imp_implementationWithBlock(^(id object,BOOL value,id title) {
            ((void(*)(id,SEL,BOOL,id))original)(object,selector,value,title);
            HGAppearanceProcessView(object);
        });
    } else if (!strcmp(encoding,"v20@0:8B16")) {
        replacement=imp_implementationWithBlock(^(id object,BOOL value) {
            if(selector==sel_registerName("finishSearchIconVisibilityTransition:")) {
                HGAHeaderInsetState *state=objc_getAssociatedObject(object,&HGAHeaderInsetKey);
                BOOL owned=state.ownsSideToolTransition && HGAHeaderToolTransitionAnimating(object);
                if(owned)state.channelSelectionDepth++;
                @try {
                    ((void(*)(id,SEL,BOOL))original)(object,selector,value);
                    HGAppearanceProcessView(object);
                    if(owned)HGARefreshHeaderChannelLine(state.categories,YES);
                } @finally {
                    if(owned)state.channelSelectionDepth--;
                    HGAFinishNativeHeaderToolTransition(object);
                }
                return;
            }
            ((void(*)(id,SEL,BOOL))original)(object,selector,value);
            if(controller && selector==@selector(viewDidDisappear:)) {
                for(UIWindow *window in HGACleanPages.keyEnumerator.allObjects)
                    if([HGACleanPages objectForKey:window].page==[object viewIfLoaded])HGAEndCleanPlayback(window);
            } else if (scrubClipping) {
                // Ending a seek reclips the control's feed ancestors. Reapply
                // the active dock extension after the original clipping pass.
                if(value)HGRefreshVideoBackdropForPlayer(object);
            } else if (controller && selector==@selector(viewDidAppear:) &&
                       [className isEqual:@"FQVShortVideoListViewController"]) {
                HGAppearanceProcessController(object);
                // The incoming feed is known here even when the old tab is
                // still in the transition tree and a whole-window scan abstains.
                for (UIView *child in ((UIViewController *)object).viewIfLoaded.subviews)
                    HGARefreshBackdropAfterScroll(child);
            } else if(controller)HGAppearanceProcessController(object);
            else HGAppearanceProcessView(object);
        });
    }
    if (!replacement) return;
    original=HGInstallVerifiedHook(cls,selector,encoding,replacement,@"appearance");
    if (original) HGAHooks[key]=[NSValue valueWithPointer:replacement];
    else imp_removeBlock(replacement);
}
static void HGAInstallNativeShareRoute(void) {
    // 7.4.5.33's feed action passes the same params to its mode-1 UIKit
    // presenter and to the UIKit fallback for mode 2. Scope the mode change
    // to the feed action, so other share presenters keep their own route.
    if (![[NSBundle.mainBundle objectForInfoDictionaryKey:@"CFBundleVersion"] isEqual:@"7.4.5.33"]) return;
    Class presenter=objc_lookUpClass("SSShortVideoSharePanelPresenter");
    Class manager=objc_lookUpClass("SSShortVideoRightInteractionShareViewManager");
    Method native=class_getClassMethod(presenter,sel_registerName("presentNativeSharePanelWithParams:"));
    if (!native || strcmp(method_getTypeEncoding(native),"v24@0:8@16") || !manager) return;
    HGAHook(object_getClass(presenter),sel_registerName("presentationMode"),"q16@0:8",NO);
    HGAHook(manager,sel_registerName("shareViewDidClick:"),"v24@0:8@16",NO);
}
void HGAppearanceRefresh(void) {
    if (!NSThread.isMainThread) { dispatch_async(dispatch_get_main_queue(), ^{HGAppearanceRefresh();}); return; }
    HGARefreshPlaybackQualityPreference();
    HGAUpdateEpisodeMedia(UIApplication.sharedApplication.applicationState!=UIApplicationStateBackground);
    if(HGACleanPages.count && !HGAppearanceFlag(@"gestures.pinchClean"))
        for(UIWindow *window in HGACleanPages.keyEnumerator.allObjects)HGAEndCleanPlayback(window);
    HGARefreshAssets();
    HGARefreshMinePublishVisibility();
    // A zero-height shortcut owner may no longer have a visible cell to
    // invalidate itself when a switch is turned back on.
    HGARefreshFinderRowLayout();
    for(id component in HGAPlayerNavigations.allObjects)HGAApplyPlayerNavigation(component);
    HGRecordCapability(@"module.appearance",@{@"module":@"appearance",
        @"state":HGAppearanceEnabled() ? (HGAHooks.count ? @"partial":@"unavailable") : @"disabled",
        @"installedHooks":@(HGAHooks.count),@"fullscreen":@(HGAppearanceFlag(@"player.fullscreen")),
        @"playerFillMode":@"render-sublayers",
        @"fillButton":@NO,@"landscapeOnly":@YES,
        @"episodeSpacing":@([HGAppearanceString(@"player.separateProgress") isEqual:@"show"]),
        @"episodeGlass":@(HGAppearanceFlag(@"player.episodeGlass")),@"rightGlass":@(HGAppearanceFlag(@"player.rightGlass")),
        @"dock":@(HGAppearanceDockEnabled()),@"reason":@"scoped visual adapters; device behavior not verified"});
    if (!HGAppearanceEnabled() || !HGAppearanceFlag(@"fonts.enabled")) for (UIView *text in HGATexts.allObjects) HGARestoreText(text);
    for (UIView *view in HGAManaged.allObjects) HGAppearanceProcessView(view);
    for (UIViewController *controller in HGAControllers.allObjects) HGAppearanceProcessController(controller);
    for (UIView *bar in HGABars.allObjects) { HGARefreshDock(bar); [bar setNeedsLayout]; }
}
void HGInstallAppearance(void) {
    @synchronized(NSClassFromString(@"HGSettingsController")) {
        if (!HGAHooks) HGAHooks=[NSMutableDictionary dictionary];
        HGAInstallEpisodeMedia();
        HGAInstallHeaderLayoutAdapters();
        Class infoView=objc_lookUpClass("FQVShortVideoBaseInfoView");
        if(infoView)HGAHook(infoView,sel_registerName("setLeftContainer:"),"v24@0:8@16",NO);
        Class playComponent=objc_lookUpClass("FQVShortVideoListVCPlayComponent");
        if(playComponent) {
            HGAHook(playComponent,sel_registerName("dataModelDidPlay:"),"v24@0:8@16",NO);
            HGAHook(playComponent,sel_registerName("cellView:vPlayerReadyToDisplay:"),"v32@0:8@16@24",NO);
        }
        HGAInstallMineShortcutHooks();
        HGAInstallFinderShortcutHook();
        HGAInstallHomeEpisodeSwipeGate();
        HGAInstallNativeShareRoute();
        HGAInstallRightHitArea();
        Class commentPanel=objc_lookUpClass("FQCShortVideoCommentPanelViewController");
        if (class_getInstanceMethod(commentPanel,sel_registerName("setupTabItems")))
            HGAHook(commentPanel,sel_registerName("setupTabItems"),"v16@0:8",YES);
        HGAInstallCommentMovement();
        Class metal=objc_lookUpClass("TTMetalView");
        if([metal isSubclassOfClass:UIView.class]) {
            HGAHook(metal,@selector(layoutSubviews),"v16@0:8",NO);
            // The player can attach first, then set its aspect-fill frame without
            // another feed layout. Re-evaluate only the feed owning that surface.
            static IMP originalMetalFrame;
            if (!originalMetalFrame) {
                IMP update=imp_implementationWithBlock(^(UIView *surface,CGRect frame) {
                    CGRect previous=surface.frame;
                    ((void (*)(id,SEL,CGRect))originalMetalFrame)(surface,@selector(setFrame:),frame);
                    if (surface.window && !CGRectEqualToRect(previous,surface.frame))
                        HGScheduleFeedLayoutRefresh((UIScrollView *)HGAAncestorNamed(surface,@"FQVCollectionView"));
                });
                originalMetalFrame=HGInstallVerifiedHook(metal,@selector(setFrame:),"v48@0:8{CGRect={CGPoint=dd}{CGSize=dd}}16",update,@"appearance");
                if (!originalMetalFrame) imp_removeBlock(update);
            }
        }
        NSUInteger found=0;
        NSMutableOrderedSet *classes=[NSMutableOrderedSet orderedSetWithArray:HGAViewRoles().allKeys];
        [classes addObjectsFromArray:HGACommentSurfaces()];
        [classes addObjectsFromArray:HGACommentRows()];
        [classes addObjectsFromArray:HGAPageChromeViews()];
        [classes addObjectsFromArray:HGAActionPanelSurfaces()];
        [classes addObjectsFromArray:HGABottomControlClasses()];
        [classes addObjectsFromArray:HGAPlayerControlClasses()];
        [classes addObjectsFromArray:HGACleanControlClasses()];
        [classes addObjectsFromArray:HGALandscapeChromeClasses()];
        [classes addObject:@"FQVTransitionPlayerTopContentView"];
        [classes addObject:@"FQVShortVideoVPlayerSingleSeriesControlView"];
        [classes addObjectsFromArray:@[@"FQVShortVideoPlayerCellView",@"FQVPlayer",@"TTPlayerView",@"FQVCollectionView",@"SSTabBar",@"FQVShortVideoBaseInsertView"]];
        for (NSString *name in classes) {
            Class cls=NSClassFromString(name);
            if (![cls isSubclassOfClass:UIView.class]) {
                HGRecordCapability([@"appearance.adapter." stringByAppendingString:name],
                    @{ @"module":@"appearance",@"state":@"missing-class",@"class":name });
                continue;
            }
            found++;
            HGAHook(cls,@selector(layoutSubviews),"v16@0:8",NO);
            HGAHook(cls,@selector(didMoveToWindow),"v16@0:8",NO);
            if([name isEqual:@"SSShortVideoRcmdReasonView"] || [name isEqual:@"SSShortVideoRcmdReasonTag"]) {
                BOOL tag=[name isEqual:@"SSShortVideoRcmdReasonTag"];
                HGAHook(cls,sel_registerName(tag?"updateAppearance":"modelDidUpdate"),"v16@0:8",NO);
                HGAHook(cls,@selector(traitCollectionDidChange:),"v24@0:8@16",NO);
                if(tag)HGAHook(cls,sel_registerName("onDarkModeChange:"),"v20@0:8B16",NO);
            }
            if([name isEqual:@"FQCShortVideoCommentSectionFooterView"]) {
                HGAHook(cls,sel_registerName("setupTheme"),"v16@0:8",NO);
                HGAHook(cls,sel_registerName("setModel:animation:"),"v28@0:8@16B24",NO);
            }
            if([name isEqual:@"FQCShortVideoCommentCommonCell"])
                HGAHook(cls,sel_registerName("configTheme:"),"v20@0:8B16",NO);
            if([name isEqual:@"FQCShortVideoCommentReplyCell"])
                HGAHook(cls,sel_registerName("setModel:"),"v24@0:8@16",NO);
            if([name isEqual:@"FQVShortVideoEpisodesNumberView"])
                HGAHook(cls,sel_registerName("refreshGradientLayerColor"),"v16@0:8",NO);
            if([name isEqual:@"FQVShortVideoEpisodesNumberCell"])
                HGAHook(cls,@selector(setSelected:),"v20@0:8B16",NO);
            if([name isEqual:@"FQVShortVideoLandscapeNavBar"])
                HGAHook(cls,sel_registerName("refreshLayout"),"v16@0:8",NO);
            if([name isEqual:@"FQVShortVideoPlayerLandscapeBaseInfoView"])
                HGAHook(cls,sel_registerName("setVideoDataModel:"),"v24@0:8@16",NO);
            if([name isEqual:@"FQVShortVideoPlayerLandscapeBaseControlView"]) {
                HGAHook(cls,sel_registerName("updateScreenLockButtonStyle"),"v16@0:8",NO);
                HGAHook(cls,sel_registerName("showControlWithoutScreenLock"),"v16@0:8",NO);
                HGAHook(cls,sel_registerName("showControlWithoutAnimation"),"v16@0:8",NO);
            }
            if([name isEqual:@"FQVShortVideoVPlayerControlView"]) {
                HGAHook(cls,sel_registerName("setPlayerViewFrame:"),"v48@0:8{CGRect={CGPoint=dd}{CGSize=dd}}16",NO);
                HGAHook(cls,sel_registerName("handleOutterFeedClipBounds:"),"v20@0:8B16",NO);
            }
            if([name isEqual:@"SSShortVideoTagListBottomInsertView"])
                HGAHook(cls,sel_registerName("relayout"),"v16@0:8",NO);
            if([name isEqual:@"SSShortVideoTagListBottomInsertView"] || [name isEqual:@"SSShortVideoSingleBottomInsertView"] ||
                [name isEqual:@"FQVShortVideoPlayerBottomView"])
                HGAHook(cls,sel_registerName("setVideoDataModel:"),"v24@0:8@16",NO);
            if([name isEqual:@"FQVShortVideoControlCleanSwitchView"])
                HGAHook(cls,sel_registerName("refreshLayout"),"v16@0:8",NO);
            if([name isEqual:@"FQVShortVideoVPlayerEpisodeInfoView"])
                HGAHook(cls,sel_registerName("refreshSubviews"),"v16@0:8",NO);
            if([name isEqual:@"SSShortVideoBottomDistributionView"])
                HGAHook(cls,sel_registerName("renderWithModel:"),"v24@0:8@16",NO);
            if([name isEqual:@"SSBookStoreEntranceView"] || [name isEqual:@"SSBookStoreKingKongBannerView"] ||
                [name isEqual:@"SSVideoDistributionGoldenPositionCell"])
                HGAHook(cls,sel_registerName("setModel:"),"v24@0:8@16",NO);
            if([name isEqual:@"SSVideoDistributionGoldenPositionCell"])
                HGAHook(cls,@selector(collectionView:willDisplayCell:forItemAtIndexPath:),"v40@0:8@16@24@32",NO);
            if([name isEqual:@"SSVideoDistributionGoldenPositionSubCell"]) {
                HGAHook(cls,sel_registerName("setData:styleConfig:"),"v32@0:8@16@24",NO);
                HGAHook(cls,sel_registerName("onDarkModeChange:"),"v20@0:8B16",NO);
            }
            if([name isEqual:@"SSShortVideoLandscapeBizInteractionView"])
                HGAHook(cls,sel_registerName("refreshUI"),"v16@0:8",NO);
            if([name isEqual:@"SSHGBookStoreCategoryTitleCell"])
                HGAHook(cls,sel_registerName("reloadData:"),"v24@0:8@16",NO);
            if([name isEqual:@"SSHGBookStoreCategoryTitleView"]) {
                HGAHook(cls,sel_registerName("refreshState"),"v16@0:8",NO);
                HGAHook(cls,sel_registerName("refreshSelectedCellModel:unselectedCellModel:"),"v32@0:8@16@24",NO);
            }
            if([name isEqual:@"SSVideoSeriesFeedHeaderView"]) {
                HGAHook(cls,sel_registerName("setupSideBarViewIfNeeded"),"v16@0:8",NO);
                HGAHook(cls,sel_registerName("relayoutHeaderWithSearchBar"),"v16@0:8",NO);
                HGAHook(cls,sel_registerName("tryShowSearchIconWithInteract:ratio:toShowSearchIcon:animated:"),"v36@0:8B16d20B28B32",NO);
                HGAHook(cls,sel_registerName("applySearchIconVisible:tabContainerTop:"),"v28@0:8B16d20",NO);
                HGAHook(cls,sel_registerName("finishSearchIconVisibilityTransition:"),"v20@0:8B16",NO);
            }
            if([name isEqual:@"SSShortVideoEpisodesViewItemCell"] || [name isEqual:@"FQVShortVideoSettingsOptionView"])
                HGAHook(cls,sel_registerName("setIsSelected:"),"v20@0:8B16",NO);
            if([name isEqual:@"FQVShortVideoSettingsSwitchOptionView"])
                HGAHook(cls,sel_registerName("setOptionSelected:"),"v20@0:8B16",NO);
            if([name isEqual:@"FQVShortVideoSettingsCell"])
                HGAHook(cls,sel_registerName("refreshUIWhenApplayNewStyle"),"v16@0:8",NO);
            if([name isEqual:@"SSShortVideoEpisodePanelCollectView"])
                HGAHook(cls,sel_registerName("updateCollectStatus:collectText:"),"v28@0:8B16@20",NO);
            if([name isEqual:@"TTPlayerView"])
                HGAHook(cls,@selector(didAddSubview:),"v24@0:8@16",NO);
            if([name isEqual:@"TTPlayerView"] || [name isEqual:@"FQVShortVideoPlayerCellView"]) {
                HGAHook(cls,@selector(willMoveToSuperview:),"v24@0:8@16",NO);
                HGAHook(cls,@selector(willMoveToWindow:),"v24@0:8@16",NO);
                HGAHook(cls,@selector(didMoveToSuperview),"v16@0:8",NO);
            }
            if([name isEqual:@"FQVShortVideoPlayerCellView"]) {
                HGAHook(cls,sel_registerName("refreshBgColor"),"v16@0:8",NO);
                HGAHook(cls,sel_registerName("refreshCoverImageView"),"v16@0:8",NO);
            }
            if([name isEqual:@"FQCShortVideoDanmakuEntranceView"])
                HGAHook(cls,sel_registerName("refreshIcon"),"v16@0:8",NO);
            if([name isEqual:@"FQVShortVideoPlayerCellView"]) {
                for(NSString *selectorName in @[@"setModel:",@"resetToRecycle"]) {
                    SEL selector=NSSelectorFromString(selectorName);
                    if(class_getInstanceMethod(cls,selector))
                        HGAHook(cls,selector,[selectorName isEqual:@"setModel:"]?"v24@0:8@16":"v16@0:8",NO);
                }
            }
            if([HGAViewRoles()[name] isEqual:@"episodes"]) {
                HGAHook(cls,@selector(willMoveToSuperview:),"v24@0:8@16",NO);
                HGAHook(cls,@selector(didMoveToSuperview),"v16@0:8",NO);
            }
            if([name isEqual:@"FQVShortVideoBaseInsertView"])
                HGAHook(cls,@selector(didAddSubview:),"v24@0:8@16",NO);
            if([name isEqual:@"FQVShortVideoBaseInsertView"] || [HGABottomControlClasses() containsObject:name] ||
                [name isEqual:@"FQVShortVideoProgressView"] ||
                [name isEqual:@"SSShortVideoDerivateInfoView"] || [name isEqual:@"SSShortVideoOutterSeriesInfoView"]) {
                HGAHook(cls,@selector(willMoveToSuperview:),"v24@0:8@16",NO);
                HGAHook(cls,@selector(didMoveToSuperview),"v16@0:8",NO);
            }
            if ([name hasPrefix:@"SSShortVideoRightInteraction"]) {
                NSArray *names=@[@"setModel:",@"refreshUIWithModel:",@"refreshUIWithModel:animated:",@"setIsCollected:"];
                const char *types[]={"v24@0:8@16","v24@0:8@16","v28@0:8@16B24","v20@0:8B16"};
                for(NSUInteger i=0;i<names.count;i++) {
                    SEL sel=NSSelectorFromString(names[i]);
                    if (class_getInstanceMethod(cls,sel)) HGAHook(cls,sel,types[i],NO);
                }
            }
        }
        // These overrides update their trailing glyph after the base cell's
        // refresh. Reapply only after the last native paint write.
        for(NSString *name in @[@"FQVShortVideoSettingsArrowCell",@"FQVShortVideoSettingsArrowWithInfoCell"]) {
            Class cls=NSClassFromString(name);
            if(cls)HGAHook(cls,sel_registerName("refreshUIWhenApplayNewStyle"),"v16@0:8",NO);
        }
        Class scrollComponent=NSClassFromString(@"FQVShortVideoListVCScrollComponent");
        Class playerNavigation=NSClassFromString(@"FQVShortVideoListVCNavBarComponent");
        if(playerNavigation) {
            for(NSString *name in @[@"setupLayout:",@"updateNavgationBarRightViewWithRightItems:"])
                HGAHook(playerNavigation,NSSelectorFromString(name),"v24@0:8@16",NO);
            for(NSString *name in @[@"updateNavgationBarLeftView",@"updateNavgationBarRightView",@"uiAdaptModeDidChanged"])
                HGAHook(playerNavigation,NSSelectorFromString(name),"v16@0:8",NO);
        }
        Class categoryPager = objc_lookUpClass("FQRJXCategoryListContainerView");
        if ([categoryPager isSubclassOfClass:UIView.class]) {
            HGAHook(categoryPager,@selector(scrollViewDidEndDecelerating:),"v24@0:8@16",NO);
            HGAHook(categoryPager,@selector(scrollViewDidEndDragging:willDecelerate:),"v28@0:8@16B24",NO);
            HGAHook(categoryPager,sel_registerName("listDidAppear:"),"v24@0:8q16",NO);
        }
        if(scrollComponent) {
            HGAHook(scrollComponent,@selector(scrollViewDidScroll:),"v24@0:8@16",NO);
            HGAHook(scrollComponent,@selector(scrollViewDidEndDecelerating:),"v24@0:8@16",NO);
            HGAHook(scrollComponent,@selector(scrollViewDidEndScrollingAnimation:),"v24@0:8@16",NO);
            HGAHook(scrollComponent,@selector(scrollViewDidEndDragging:willDecelerate:),"v28@0:8@16B24",NO);
        }
        NSArray *controllers=[HGAPanelControllers().allKeys arrayByAddingObjectsFromArray:HGAFinderControllers()];
        controllers=[controllers arrayByAddingObject:@"SSMine737KMPViewController"];
        for (NSString *name in controllers) {
            Class cls=NSClassFromString(name);
            if (![cls isSubclassOfClass:UIViewController.class]) continue;
            HGAHook(cls,@selector(viewDidLayoutSubviews),"v16@0:8",YES);
            if([HGAPanelControllers()[name] isEqual:@"episode-page"]) {
                HGAHook(cls,@selector(viewDidAppear:),"v20@0:8B16",YES);
                HGAHook(cls,sel_registerName("onDarkModeChange:"),"v20@0:8B16",YES);
            }
            if([HGAFinderControllers() containsObject:name]) {
                HGAHook(cls,@selector(viewWillAppear:),"v20@0:8B16",YES);
                HGAHook(cls,@selector(viewDidAppear:),"v20@0:8B16",YES);
                SEL traitChanged=sel_registerName("traitCollectionDidChange:");
                if(class_getInstanceMethod(cls,traitChanged))HGAHook(cls,traitChanged,"v24@0:8@16",YES);
                NSArray<NSString *> *updates=[name isEqual:@"FQTheaterViewController"] ?
                    @[@"relayoutHeader",@"setupNewSearchBarNew",@"refreshAISearchEntrance"] :
                    @[@"relayoutHeaderWithSearchBar",@"bookStoreSearchBarComponentNeedsRelayout"];
                for(NSString *update in updates) {
                    SEL selector=NSSelectorFromString(update);
                    if(class_getInstanceMethod(cls,selector))HGAHook(cls,selector,"v16@0:8",YES);
                }
            }
        }
        // The original navigation is still owned by the host controller.
        Class dockController=NSClassFromString(@"SSTabBarController");
        Class playbackController=NSClassFromString(@"FQVShortVideoListViewController");
        if([playbackController isSubclassOfClass:UIViewController.class]) {
            HGAHook(playbackController,@selector(viewDidAppear:),"v20@0:8B16",YES);
            HGAHook(playbackController,@selector(viewDidDisappear:),"v20@0:8B16",YES);
            HGAHook(playbackController,@selector(viewDidLayoutSubviews),"v16@0:8",YES);
        }
        Class detailController=NSClassFromString(@"SSShortVideoDetailViewController");
        if([detailController isSubclassOfClass:UIViewController.class]) {
            HGAHook(detailController,@selector(viewDidDisappear:),"v20@0:8B16",YES);
            HGAHook(detailController,@selector(viewDidLayoutSubviews),"v16@0:8",YES);
        }
        if ([dockController isSubclassOfClass:UIViewController.class])
            HGAHook(dockController,@selector(viewDidLayoutSubviews),"v16@0:8",YES);
        HGAInstallLandscapeLoadFailureDiagnostic();
        static dispatch_once_t once;
        dispatch_once(&once, ^{
            HGAInstallAppSwitchHUDs();
            dispatch_async(dispatch_get_main_queue(), ^{HGAppearanceRefresh();});
            NSArray *notifications=@[HGAppearanceDidChangeNotification,
                UIAccessibilityReduceTransparencyStatusDidChangeNotification,
                UIAccessibilityReduceMotionStatusDidChangeNotification,
                UIAccessibilityVoiceOverStatusDidChangeNotification];
            for (NSString *name in notifications)
                [NSNotificationCenter.defaultCenter addObserverForName:name object:nil
                    queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification *note) { HGAppearanceRefresh(); }];
        });
        HGRecordCapability(@"module.appearance",@{@"module":@"appearance",
            @"state":HGAppearanceEnabled() ? (found ? @"partial":@"unavailable") : @"disabled",
            @"available":@(found),@"fullscreen":@(HGAppearanceFlag(@"player.fullscreen")),
            @"playerFillMode":@"render-sublayers",@"landscapeOnly":@YES,
            @"episodeGlass":@(HGAppearanceFlag(@"player.episodeGlass")),@"rightGlass":@(HGAppearanceFlag(@"player.rightGlass")),
            @"dock":@(HGAppearanceDockEnabled()),@"reason":@"scoped visual adapters; device behavior not verified"});
    }
}
