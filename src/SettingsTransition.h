#pragma once
#import <UIKit/UIKit.h>
#import <dispatch/dispatch.h>

// Do not start a new presentation from inside UIKit's presentation callbacks.
// Use the public transition completion and a subsequent main-queue turn.
static inline void HGAfterViewTransition(UIViewController *controller, dispatch_block_t action) {
    if (!controller || !action) return;
    dispatch_async(dispatch_get_main_queue(), ^{
        __block BOOL scheduled = NO;
        dispatch_block_t enqueue = ^{
            if (scheduled) return;
            scheduled = YES;
            dispatch_async(dispatch_get_main_queue(), action);
        };
        id<UIViewControllerTransitionCoordinator> coordinator = controller.transitionCoordinator;
        BOOL accepted = coordinator && coordinator.isAnimated &&
            [coordinator animateAlongsideTransition:nil completion:^(id<UIViewControllerTransitionCoordinatorContext> context) {
                enqueue();
            }];
        if (!accepted) enqueue();
    });
}
