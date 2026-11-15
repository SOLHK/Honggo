#pragma once
#import <UIKit/UIKit.h>
#import "SettingsTransition.h"

// One fixed destination, supplied by the project owner. No launch-time network
// request, subscription, tracking parameter, or automatic clipboard access.
static inline NSString *HGCommunityURLString(void) {
    return @"https://t.me/Naloiafeed";
}
static inline BOOL HGCommunityMatchesQuery(NSString *query) {
    NSString *text = @"TG Telegram 频道 更新 公告 发布 Naloiafeed";
    for (NSString *word in [query componentsSeparatedByCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet]) {
        if (word.length && [text rangeOfString:word options:NSCaseInsensitiveSearch |
                NSDiacriticInsensitiveSearch | NSWidthInsensitiveSearch].location == NSNotFound) return NO;
    }
    return YES;
}
static inline UIAlertController *HGCommunityMenu(UIViewController *presenter) {
    UIAlertController *menu = [UIAlertController alertControllerWithTitle:@"TG 更新频道"
        message:HGCommunityURLString() preferredStyle:UIAlertControllerStyleAlert];
    menu.view.accessibilityIdentifier = @"honggo.community.menu";
    __weak UIViewController *weakPresenter = presenter;
    [menu addAction:[UIAlertAction actionWithTitle:@"打开频道" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
        // HTTPS lets the operating system choose Telegram or a browser. This
        // does not require probing installed apps or changing the host plist.
        [UIApplication.sharedApplication openURL:[NSURL URLWithString:HGCommunityURLString()]
            options:@{} completionHandler:^(BOOL success) {
                if (success) return;
                HGAfterViewTransition(weakPresenter, ^{
                    UIViewController *owner = weakPresenter;
                    if (!owner.viewIfLoaded.window || owner.presentedViewController || owner.isBeingDismissed) return;
                    UIAlertController *failure = [UIAlertController alertControllerWithTitle:@"无法打开频道"
                        message:@"请检查网络，或复制地址后在 Telegram／浏览器中打开。" preferredStyle:UIAlertControllerStyleAlert];
                    [failure addAction:[UIAlertAction actionWithTitle:@"复制地址" style:UIAlertActionStyleDefault handler:^(UIAlertAction *button) {
                        UIPasteboard.generalPasteboard.string = HGCommunityURLString();
                    }]];
                    [failure addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:nil]];
                    [owner presentViewController:failure animated:YES completion:nil];
                });
            }];
    }]];
    [menu addAction:[UIAlertAction actionWithTitle:@"复制地址" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
        UIPasteboard.generalPasteboard.string = HGCommunityURLString();
        UIAccessibilityPostNotification(UIAccessibilityAnnouncementNotification, @"频道地址已复制");
    }]];
    [menu addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:nil]];
    return menu;
}
static inline void HGShowCommunity(UIViewController *presenter) {
    if (!NSThread.isMainThread || !presenter.viewIfLoaded.window || presenter.presentedViewController ||
            presenter.isBeingDismissed || presenter.navigationController.isBeingDismissed) return;
    [presenter presentViewController:HGCommunityMenu(presenter) animated:YES completion:nil];
}
