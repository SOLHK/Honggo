#pragma once
#import "AppearancePreferences.h"
#import <UIKit/UIKit.h>
void HGInstallAppearance(void);
void HGAppearanceRefresh(void);
void HGAppearanceProcessView(UIView *view);
void HGAppearanceProcessController(UIViewController *controller);
BOOL HGAppearanceDockEnabled(void);
void HGAppearanceLayoutDock(UIView *bar, UIViewController *controller);
void HGAppearanceRestoreDock(UIView *bar);
BOOL HGAppearanceSuspendDockForSidebar(UIView *bar);
void HGAppearanceUpdateSidebarLayout(UIWindow *window, BOOL suspended);
void HGScheduleFeedLayoutRefresh(UIScrollView *feed);
void HGRefreshVideoBackdropForFeed(UIScrollView *feed);
void HGRefreshVideoBackdropForCategoryPager(UIView *pager);
void HGRefreshVideoBackdropForPlayer(UIView *view);
static inline BOOL HGIsAlbumDetailControl(UIView *view) {
    // The multi-series base is also subclassed by the outer derivative feed
    // and profile controls; only the inner list and its album variant qualify.
    return [view isMemberOfClass:NSClassFromString(@"FQVShortVideoVPlayerMultiSeriesControlView")] ||
        [view isKindOfClass:NSClassFromString(@"FQVShortVideoVPlayerVideoListControlView")];
}
void HGVideoBackdropPrepareRendererTransfer(UIView *view);
void HGAppearanceReclaimDockChrome(UIView *bar);
NSDictionary *HGAppearanceDockDiagnosticState(UIWindow *window);
BOOL HGAppearancePlayerIsTransferring(UIView *view);
void HGOpenAppearance(UIViewController *presenter, NSString *group);
UIVisualEffect *HGAppearanceMaterial(NSString *style);
