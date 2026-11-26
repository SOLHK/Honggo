// Follow only the current list's ancestor chain, never scan cells or resize
// host content. Both expanded and pinned headers must reveal the glass area.
@interface HGMineClipRecord : NSObject
@property(nonatomic) BOOL clips;
@property(nonatomic) BOOL captured;
@property(nonatomic, strong) CAShapeLayer *mask;
@property(nonatomic, strong) id top;
@property(nonatomic, strong) id bottom;
@property(nonatomic) BOOL topHidden;
@property(nonatomic) BOOL bottomHidden;
- (void)restore:(UIView *)view;
- (void)apply:(UIView *)view height:(CGFloat)height movingViewport:(BOOL)movingViewport;
@end
@implementation HGMineClipRecord
- (void)restore:(UIView *)view {
    if (view.layer.mask == self.mask) view.layer.mask = nil;
    if (!view.clipsToBounds) view.clipsToBounds = self.clips;
    if (self.top) ((void (*)(id, SEL, BOOL))objc_msgSend)(self.top, @selector(setHidden:), self.topHidden);
    if (self.bottom) ((void (*)(id, SEL, BOOL))objc_msgSend)(self.bottom, @selector(setHidden:), self.bottomHidden);
}
- (void)apply:(UIView *)view height:(CGFloat)height movingViewport:(BOOL)movingViewport {
    if (!self.captured) {
        self.captured = YES;
        self.clips = view.clipsToBounds;
        for (NSString *name in @[@"topEdgeEffect", @"bottomEdgeEffect"]) {
            id edge = HGObject(view, name.UTF8String);
            if (![edge respondsToSelector:@selector(setHidden:)] || ![edge respondsToSelector:sel_registerName("isHidden")]) continue;
            BOOL hidden = ((BOOL (*)(id, SEL))objc_msgSend)(edge, sel_registerName("isHidden"));
            if ([name isEqual:@"topEdgeEffect"]) { self.top = edge; self.topHidden = hidden; }
            else { self.bottom = edge; self.bottomHidden = hidden; }
        }
    }
    if (movingViewport) {
        // History/Collection taps animate a horizontal pager's bounds without
        // calling the outer Mine scroll delegate. A mask in those moving
        // coordinates stays on the old page and clips the incoming page to
        // nothing (or half its width). The stationary outer/tabs masks already
        // provide the screen boundary. Release only this pager's clipping;
        // never chase its contentOffset or interfere with its animation.
        if (view.layer.mask == self.mask) view.layer.mask = nil;
    } else {
        if (!self.mask) {
            self.mask = [CAShapeLayer layer]; self.mask.fillColor = UIColor.blackColor.CGColor;
        }
        CGRect clip = view.bounds; clip.size.height = height;
        if (!CGRectEqualToRect(self.mask.frame, clip)) {
            BOOL resize = !CGSizeEqualToSize(self.mask.bounds.size, clip.size) || !self.mask.path;
            self.mask.frame = clip;
            if (resize) {
                CGPathRef path = CGPathCreateWithRect((CGRect){CGPointZero, clip.size}, NULL);
                self.mask.path = path; CGPathRelease(path);
            }
        }
        view.layer.mask = self.mask;
    }
    view.clipsToBounds = NO;
    // Preserve the inner list's own top boundary; only the outer coordinator
    // had a redundant top edge effect behind the custom header.
    if (self.top && [view isKindOfClass:objc_lookUpClass("SSMyUser637NestedContainerScrollView")])
        ((void (*)(id, SEL, BOOL))objc_msgSend)(self.top, @selector(setHidden:), YES);
    if (self.bottom) ((void (*)(id, SEL, BOOL))objc_msgSend)(self.bottom, @selector(setHidden:), YES);
}
@end
static char HGMineClipKey;
static BOOL HGMineMovingViewport(UIView *view) {
    if (![view isKindOfClass:UIScrollView.class]) return NO;
    UIScrollView *scroll = (UIScrollView *)view;
    // pagingEnabled also covers lazy/empty pagers before contentSize is known.
    // Keep a known vertical pager's top crop; only its horizontal counterpart
    // has the stale bounds.x problem. Non-paging section scrollers qualify too.
    BOOL horizontal = scroll.contentSize.width > scroll.bounds.size.width + 2;
    BOOL vertical = scroll.contentSize.height > scroll.bounds.size.height + 2;
    return horizontal || (scroll.pagingEnabled && !vertical);
}
static void HGApplyMineClip(UIScrollView *scroll) {
    if (!scroll) return;
    NSMapTable<UIView *, HGMineClipRecord *> *records = objc_getAssociatedObject(scroll, &HGMineClipKey);
    UIView *root = scroll.superview;
    Class rootClass = objc_lookUpClass("SSMine737KMPRootView");
    BOOL eligible = (HGTabBarGlassActive() || HGAppearanceDockEnabled()) &&
        scroll.window && rootClass && [root isKindOfClass:rootClass] &&
        CGAffineTransformIsIdentity(scroll.transform) && HGNear(scroll.frame.origin.x, 0) &&
        HGNear(scroll.frame.origin.y, 0) && HGNear(scroll.bounds.size.width, root.bounds.size.width) &&
        root.bounds.size.height >= scroll.bounds.size.height &&
        root.bounds.size.height - scroll.bounds.size.height <= 160;
    NSMutableArray<UIView *> *path = [NSMutableArray array];
    UIViewController *mineOwner = nil;
    if (eligible) {
        [path addObject:scroll];
        UIResponder *owner = root.nextResponder;
        while (owner && ![owner isKindOfClass:UIViewController.class]) owner = owner.nextResponder;
        mineOwner = (UIViewController *)owner;
        UIView *tabs = HGView(owner, "bottomView");
        UIView *leaf = HGView(tabs, "currentVCScrollView");
        if (leaf && [leaf isDescendantOfView:scroll]) {
            for (UIView *v = leaf; v && v != scroll && path.count < 24; v = v.superview) [path addObject:v];
        }
    }
    if (!records && path.count) {
        records = [NSMapTable weakToStrongObjectsMapTable];
        objc_setAssociatedObject(scroll, &HGMineClipKey, records, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    [CATransaction begin]; [CATransaction setDisableActions:YES];
    NSHashTable *active = [NSHashTable weakObjectsHashTable];
    for (UIView *view in path) {
        HGMineClipRecord *record = [records objectForKey:view];
        CGRect rect = [view convertRect:view.bounds toView:root];
        CGFloat height = CGRectGetMaxY(root.bounds) - CGRectGetMinY(rect);
        if (!CGAffineTransformIsIdentity(view.transform) ||
            (view.layer.mask && view.layer.mask != record.mask) ||
            height < 20 || height > view.bounds.size.height + 160 ||
            CGRectGetWidth(rect) < CGRectGetWidth(root.bounds) * 0.8 ||
            CGRectGetMinX(rect) < -2 || CGRectGetMaxX(rect) > CGRectGetMaxX(root.bounds) + 2) continue;
        if (!record) {
            // A non-clipping wrapper needs no material or geometry mutation.
            if (!view.clipsToBounds) continue;
            record = [HGMineClipRecord new]; [records setObject:record forKey:view];
        }
        [active addObject:view];
        [record apply:view height:height movingViewport:(view != scroll && HGMineMovingViewport(view))];
    }
    for (UIView *view in records.keyEnumerator.allObjects) {
        if (![active containsObject:view]) { [[records objectForKey:view] restore:view]; [records removeObjectForKey:view]; }
    }
    if (mineOwner) {
        UIView *bar = HGView(HGContainingTabController(mineOwner), "customTabBar");
        if ([bar isKindOfClass:HGTabBarClass] && bar.window && !bar.hidden && (bar.layer.mask || bar.clipsToBounds))
            HGRefreshBar(bar);
    }
    [CATransaction commit];
}
static void HGInstallMineClip(void) {
    Class cls = objc_lookUpClass("SSMyUser637NestedContainerScrollView");
    for (NSString *name in @[@"layoutSubviews", @"didMoveToWindow"]) {
        SEL sel = NSSelectorFromString(name); Method m = class_getInstanceMethod(cls, sel);
        if (!m || strcmp(method_getTypeEncoding(m), "v16@0:8")) continue;
        void (*original)(id, SEL) = (void *)method_getImplementation(m);
        HGReplaceMethod(cls, sel, "v16@0:8", imp_implementationWithBlock(^(UIScrollView *scroll) {
            original(scroll, sel); HGApplyMineClip(scroll);
        }));
    }
    // Host coordination moves the pinned header and inner list bounds on each
    // scroll frame without always laying out the outer container. Update masks
    // synchronously inside the same transaction so pull-down never lags a frame
    // or leaves a host mask on SSTabBar.
    Class mine = objc_lookUpClass("SSMine737KMPViewController");
    SEL sel = sel_registerName("scrollViewDidScroll:");
    Method m = class_getInstanceMethod(mine, sel);
    if (m && !strcmp(method_getTypeEncoding(m), "v24@0:8@16")) {
        void (*original)(id, SEL, id) = (void *)method_getImplementation(m);
        HGReplaceMethod(mine, sel, "v24@0:8@16", imp_implementationWithBlock(^(UIViewController *page, id sender) {
            original(page, sel, sender);
            HGApplyMineClip((UIScrollView *)HGView(page, "scrollView"));
        }));
    }
}
