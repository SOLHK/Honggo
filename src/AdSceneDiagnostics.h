#pragma once

#import <UIKit/UIKit.h>

// Read-only, main-thread-only capture. The result contains JSON values, never views.
NSDictionary * _Nullable HGAdSceneSnapshot(NSArray<UIWindow *> * _Nonnull windows);
void HGInstallAdSceneDiagnostics(void);
