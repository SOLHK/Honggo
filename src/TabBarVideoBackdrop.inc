// The vertical feed's native height is also its paging stride. The full-screen
// window can clip the top, sides and newly exposed bottom while a realized cell
// straddles the feed edge. Let UIKit draw that overflow in place.

@interface HGVideoBackdrop : NSObject
- (void)refreshWithBranches:(NSArray<UIView *> *)branches root:(UIView *)root
                       bar:(UIView *)bar active:(BOOL)active;
- (void)restore;
@end

@interface HGVideoBackdropLink : NSObject
@property(nonatomic, weak) HGVideoBackdrop *owner;
@end
@implementation HGVideoBackdropLink @end
static char HGVideoBackdropLinkKey;

@interface HGVideoBackdrop ()
@property(nonatomic, weak) UICollectionView *feed;
@property(nonatomic, weak) UIView *root;
@property(nonatomic, weak) UIView *bar;
@property(nonatomic, weak) UIViewController *owner;
@property(nonatomic) BOOL ownsClipping;
@property(nonatomic) BOOL refreshing;
- (void)refreshFeed;
@end

static BOOL HGVideoBackdropVisible(UIView *view, UIView *root) {
    if (!view.window) return NO;
    for (UIView *node = view; node; node = node.superview) {
        if (node.hidden || node.alpha < 0.99 ||
            !CGAffineTransformIsIdentity(node.transform) ||
            !CATransform3DIsIdentity(node.layer.transform)) return NO;
        if (node == root) return YES;
    }
    return NO;
}

static UIViewController *HGVideoBackdropOwner(UIView *view) {
    Class expected = NSClassFromString(@"FQVShortVideoListViewController");
    if (!expected) return nil;
    for (UIResponder *responder = view.nextResponder; responder; responder = responder.nextResponder) {
        if ([responder isKindOfClass:UIViewController.class])
            return [responder isKindOfClass:expected] ? (UIViewController *)responder : nil;
    }
    return nil;
}

static BOOL HGVideoBackdropHasTransition(UIViewController *owner) {
    for (UIViewController *controller = owner; controller; controller = controller.parentViewController)
        if (controller.transitionCoordinator || controller.isBeingDismissed || controller.isBeingPresented)
            return YES;
    return NO;
}

static BOOL HGVideoBackdropFullWindow(UIView *view, UIWindow *window) {
    CGRect rect = [view convertRect:view.bounds toView:window];
    CGRect bounds = window.bounds;
    return fabs(CGRectGetMinX(rect) - CGRectGetMinX(bounds)) <= 0.01 &&
        fabs(CGRectGetMaxX(rect) - CGRectGetMaxX(bounds)) <= 0.01 &&
        fabs(CGRectGetMinY(rect) - CGRectGetMinY(bounds)) <= 0.01 &&
        fabs(CGRectGetMaxY(rect) - CGRectGetMaxY(bounds)) <= 0.01;
}

static BOOL HGVideoBackdropFeedGeometry(UICollectionView *feed, UIViewController *owner,
                                        UIView *root, UIView *bar) {
    UIView *parent = feed.superview;
    UIWindow *window = root.window;
    if (!window || !parent || owner.viewIfLoaded != parent ||
        window.bounds.size.height <= window.bounds.size.width || feed.layer.mask ||
        !HGVideoBackdropVisible(feed, window) || !HGVideoBackdropVisible(bar, root) ||
        !HGVideoBackdropFullWindow(parent, window) || !HGVideoBackdropFullWindow(root, window) ||
        HGVideoBackdropHasTransition(owner)) return NO;
    CGRect frame = feed.frame, full = parent.bounds;
    CGRect barInWindow = [bar convertRect:bar.bounds toView:window];
    CGFloat uncovered = CGRectGetMaxY(full) - CGRectGetMaxY(frame);
    return uncovered >= 20 && uncovered <= 160 &&
        fabs(CGRectGetMinX(frame) - CGRectGetMinX(full)) <= 0.01 &&
        fabs(CGRectGetMinY(frame) - CGRectGetMinY(full)) <= 0.01 &&
        fabs(CGRectGetMaxX(frame) - CGRectGetMaxX(full)) <= 0.01 &&
        CGRectGetMinY(barInWindow) >= CGRectGetMaxY(window.bounds) - 180 &&
        CGRectGetMaxY(barInWindow) <= CGRectGetMaxY(window.bounds) + 0.75;
}

static BOOL HGVideoBackdropCrossesBottom(UICollectionView *feed) {
    CGRect viewport = feed.bounds;
    CGFloat top = CGRectGetMinY(viewport);
    CGFloat bottom = CGRectGetMaxY(viewport);
    UIEdgeInsets insets = feed.adjustedContentInset;
    // Pull-to-refresh can move the first cell across the edge without a page
    // entering. Let the host keep its clip outside the regular scroll range.
    if (top < -insets.top - 0.75 || bottom > feed.contentSize.height + insets.bottom + 0.75)
        return NO;
    for (UICollectionViewCell *cell in feed.visibleCells) {
        if (cell.superview != feed || !HGVideoBackdropVisible(cell, feed)) continue;
        CGRect frame = [cell convertRect:cell.bounds toView:feed];
        if (CGRectGetMinY(frame) > top + 0.75 && CGRectGetMinY(frame) < bottom - 0.75 &&
            CGRectGetMaxY(frame) > bottom + 0.75 &&
            fabs(CGRectGetMinX(frame) - CGRectGetMinX(viewport)) <= 0.75 &&
            fabs(CGRectGetMaxX(frame) - CGRectGetMaxX(viewport)) <= 0.75) return YES;
    }
    return NO;
}

@implementation HGVideoBackdrop
- (void)dealloc {
    if (_ownsClipping) _feed.clipsToBounds = YES;
}

- (void)restoreClipping {
    BOOL owned = self.ownsClipping;
    self.ownsClipping = NO;
    if (owned && !self.feed.clipsToBounds) self.feed.clipsToBounds = YES;
}

- (void)removeFeed {
    UICollectionView *feed = self.feed;
    HGVideoBackdropLink *link = objc_getAssociatedObject(feed, &HGVideoBackdropLinkKey);
    if (link.owner == self)
        objc_setAssociatedObject(feed, &HGVideoBackdropLinkKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [self restoreClipping];
    self.feed = nil; self.root = nil; self.bar = nil; self.owner = nil;
}

- (void)restore {
    if (self.refreshing) return;
    self.refreshing = YES;
    [self removeFeed];
    self.refreshing = NO;
}

- (void)updateFeed {
    UICollectionView *feed = self.feed;
    if (!HGVideoBackdropFeedGeometry(feed, self.owner, self.root, self.bar)) {
        [self removeFeed];
        return;
    }

    // A prefetched next cell can already exist at the exact edge while idle.
    // Only a visible cell with a portion on both sides may draw below the feed.
    if (!HGVideoBackdropCrossesBottom(feed)) {
        [self restoreClipping];
        return;
    }

    if (feed.clipsToBounds) {
        self.ownsClipping = YES;
        feed.clipsToBounds = NO;
    }
}

- (void)refreshFeed {
    if (self.refreshing) return;
    self.refreshing = YES;
    [self updateFeed];
    self.refreshing = NO;
}

- (void)refreshWithBranches:(NSArray<UIView *> *)branches root:(UIView *)root
                       bar:(UIView *)bar active:(BOOL)active {
    if (self.refreshing) return;
    if (!active || !root.window || !HGVideoBackdropVisible(bar, root)) {
        [self restore];
        return;
    }

    Class feedClass = NSClassFromString(@"FQVCollectionView");
    UICollectionView *candidate = nil;
    UIViewController *owner = nil;
    NSMutableArray<UIView *> *queue = [branches mutableCopy];
    for (NSUInteger i = 0; feedClass && i < queue.count && i < 192; i++) {
        UIView *view = queue[i];
        if (!HGVideoBackdropVisible(view, root) ||
            CGRectIsEmpty(CGRectIntersection([view convertRect:view.bounds toView:root], root.bounds))) continue;
        if ([view isKindOfClass:feedClass] && [view isKindOfClass:UICollectionView.class]) {
            UICollectionView *feed = (UICollectionView *)view;
            UIViewController *controller = HGVideoBackdropOwner(feed);
            if (!HGVideoBackdropFeedGeometry(feed, controller, root, bar)) continue;
            if (candidate && candidate != feed) {
                [self restore];
                return;
            }
            candidate = feed; owner = controller;
            continue;
        }
        if ([view isKindOfClass:UIVisualEffectView.class] || [view isKindOfClass:UIControl.class] ||
            [view isKindOfClass:UILabel.class] || [view isKindOfClass:UIImageView.class] ||
            [view isKindOfClass:UITableViewCell.class] || [view isKindOfClass:UICollectionViewCell.class]) continue;
        for (UIView *child in view.subviews) {
            if (queue.count >= 192) break;
            [queue addObject:child];
        }
    }
    if (!candidate) {
        [self restore];
        return;
    }

    self.refreshing = YES;
    if (self.feed != candidate) {
        [self removeFeed];
        HGVideoBackdropLink *link = objc_getAssociatedObject(candidate, &HGVideoBackdropLinkKey);
        if (link.owner) {
            self.refreshing = NO;
            return;
        }
        self.feed = candidate;
        link = [HGVideoBackdropLink new];
        link.owner = self;
        objc_setAssociatedObject(candidate, &HGVideoBackdropLinkKey, link, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        HGRecordCapability(@"appearance.dock.video-backdrop", @{
            @"module": @"appearance", @"state": @"installed", @"reason": @"native-feed-overflow",
            @"nativeGeometryPreserved": @YES, @"nativeHierarchyPreserved": @YES, @"mirrorsFrames": @NO
        });
    }
    self.root = root; self.bar = bar; self.owner = owner;
    [self updateFeed];
    self.refreshing = NO;
}
@end

BOOL HGVideoBackdropRefreshForFeed(UIScrollView *feed) {
    HGVideoBackdropLink *link = objc_getAssociatedObject(feed, &HGVideoBackdropLinkKey);
    HGVideoBackdrop *owner = link.owner;
    if (!owner) return NO;
    if (HGAppearanceDockEnabled()) [owner refreshFeed];
    else [owner restore];
    return YES;
}
