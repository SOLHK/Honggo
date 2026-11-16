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
void HGRefreshVideoBackdropForFeed(UIScrollView *feed);
void HGOpenAppearance(UIViewController *presenter, NSString *group);
UIVisualEffect *HGAppearanceMaterial(NSString *style);
UIColor *HGAppearanceColor(NSString *hex, UIColor *fallback);
