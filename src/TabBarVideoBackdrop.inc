// Video pages deliberately retain their native paging stride. The R14 scene
// has a 440x866 FQVCollectionView inside a 440x956 controller: resizing that
// collection, its cells, or its player can change the host's episode handoff.
//
// Only plugin-owned views are installed here. One read-only UIKit portal
// reflects the observed TTPlayerView across the feed's bottom edge, clipped to
// the uncovered strip. The portal is composed directly: a background-extension
// effect must not sample another portal, which lost live Metal content in the
// compositor regression. No snapshots, pixel readback, polling timer, native
// reparenting, playback calls, or changes to the portrait video ratio are used.
// Original letterboxing remains letterboxing; only the exposed edge is mirrored.
//
// The portal selectors are used by Telegram's UIKitRuntimeUtils/UIKitUtils.m;
// every selector below is checked against the running Objective-C ABI first.
// https://github.com/TelegramMessenger/Telegram-iOS/blob/master/submodules/UIKitRuntimeUtils/Source/UIKitRuntimeUtils/UIKitUtils.m

@interface HGVideoBackdrop : NSObject
- (void)refreshWithBranches:(NSArray<UIView *> *)branches root:(UIView *)root
                       bar:(UIView *)bar active:(BOOL)active;
- (void)restore;
@end

#if defined(__IPHONE_26_0) && __IPHONE_OS_VERSION_MAX_ALLOWED >= __IPHONE_26_0

@protocol HGVideoPortalAPI <NSObject>
@property(nonatomic, weak) UIView *sourceView;
@property(nonatomic) BOOL hidesSourceView;
@property(nonatomic) BOOL matchesPosition;
@property(nonatomic) BOOL matchesTransform;
@property(nonatomic) BOOL matchesAlpha;
@property(nonatomic) BOOL allowsHitTesting;
@property(nonatomic) BOOL forwardsClientHitTestingToSourceView;
@end

@interface HGVideoBackdropLink : NSObject
@property(nonatomic, weak) HGVideoBackdrop *owner;
@end
@implementation HGVideoBackdropLink @end
static char HGVideoBackdropLinkKey;

@interface HGVideoBackdrop ()
@property(nonatomic, weak) UIScrollView *feed;
@property(nonatomic, weak) UIView *parent;
@property(nonatomic, weak) UIView *root;
@property(nonatomic, weak) UIView *bar;
@property(nonatomic, weak) UIViewController *owner;
@property(nonatomic, weak) UIView *renderer;
@property(nonatomic, strong) UIView *extension;
@property(nonatomic, strong) UIView *canvas;
@property(nonatomic, strong) UIView<HGVideoPortalAPI> *portal;
@property(nonatomic) BOOL mutating;
@property(nonatomic, copy) NSString *lastStatus;
- (void)refreshFeed;
- (void)prepareRendererTransfer:(UIView *)view;
@end

static BOOL HGVideoBackdropSignature(Class type, const char *name, char result, char argument) {
    Method method = class_getInstanceMethod(type, sel_registerName(name));
    if (!method) return NO;
    const char *encoding = method_getTypeEncoding(method);
    if (!encoding) return NO;
    NSMethodSignature *signature = [NSMethodSignature signatureWithObjCTypes:encoding];
    if (!signature || signature.numberOfArguments != (argument ? 3u : 2u) ||
        signature.methodReturnType[0] != result) return NO;
    if (!argument) return YES;
    const char *actual = [signature getArgumentTypeAtIndex:2];
    // Objective-C BOOL is encoded as B on modern iOS; tolerate the older
    // signed-char BOOL ABI too, never an integer/object/forwarding-only setter.
    return actual && (actual[0] == argument || (argument == 'B' && actual[0] == 'c'));
}

static Class HGVideoBackdropPortalClass(void) {
    Class type = NSClassFromString(@"_UIPortalView");
    if (!type || ![type isSubclassOfClass:UIView.class] ||
        !HGVideoBackdropSignature(type, "sourceView", '@', 0) ||
        !HGVideoBackdropSignature(type, "setSourceView:", 'v', '@')) return Nil;
    const char *setters[] = {"setHidesSourceView:", "setMatchesPosition:",
        "setMatchesTransform:", "setMatchesAlpha:", "setAllowsHitTesting:",
        "setForwardsClientHitTestingToSourceView:"};
    for (NSUInteger i = 0; i < sizeof(setters) / sizeof(setters[0]); i++)
        if (!HGVideoBackdropSignature(type, setters[i], 'v', 'B')) return Nil;
    return type;
}

static BOOL HGVideoBackdropVisible(UIView *view, UIView *root) {
    if (!view || view.window != root.window || !view.window) return NO;
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
    UIResponder *responder = view.nextResponder;
    for (NSUInteger i = 0; responder && i < 24; i++, responder = responder.nextResponder)
        if ([responder isKindOfClass:UIViewController.class])
            return [responder isKindOfClass:expected] ? (UIViewController *)responder : nil;
    return nil;
}

static BOOL HGVideoBackdropHasTransition(UIViewController *owner) {
    for (UIViewController *controller = owner; controller; controller = controller.parentViewController)
        if (controller.transitionCoordinator || controller.isBeingDismissed || controller.isBeingPresented)
            return YES;
    return NO;
}

static BOOL HGVideoBackdropFeedGeometry(UIScrollView *feed, UIView *parent, UIView *root, UIView *bar) {
    if (!feed || !parent || !root || !bar || feed.superview != parent ||
        parent.window != root.window || bar.window != root.window ||
        !HGVideoBackdropVisible(feed, root) || !HGVideoBackdropVisible(bar, root) ||
        !CGPointEqualToPoint(parent.bounds.origin, CGPointZero) ||
        root.bounds.size.height <= root.bounds.size.width ||
        parent.bounds.size.height <= parent.bounds.size.width) return NO;
    CGRect frame = feed.frame, full = parent.bounds;
    CGFloat uncovered = CGRectGetMaxY(full) - CGRectGetMaxY(frame);
    CGRect parentInRoot = [parent convertRect:parent.bounds toView:root];
    CGRect barInRoot = [bar convertRect:bar.bounds toView:root];
    return isfinite(uncovered) && uncovered >= 20 && uncovered <= 160 &&
        fabs(frame.origin.x) <= 0.75 && fabs(frame.origin.y) <= 0.75 &&
        fabs(frame.size.width - full.size.width) <= 0.75 &&
        fabs(CGRectGetMinX(parentInRoot) - CGRectGetMinX(root.bounds)) <= 0.75 &&
        fabs(CGRectGetMaxX(parentInRoot) - CGRectGetMaxX(root.bounds)) <= 0.75 &&
        fabs(CGRectGetMaxY(parentInRoot) - CGRectGetMaxY(root.bounds)) <= 0.75 &&
        CGRectGetMinY(barInRoot) >= CGRectGetMaxY(parentInRoot) - 180 &&
        CGRectGetMaxY(barInRoot) <= CGRectGetMaxY(parentInRoot) + 2;
}

static UIView *HGVideoBackdropCanvasForRenderer(UIView *renderer, UIScrollView *feed) {
    Class type = NSClassFromString(@"FQVShortVideoPlayerCellView");
    for (UIView *node = renderer.superview; node && node != feed; node = node.superview)
        if (type && [node isKindOfClass:type]) return node;
    return nil;
}

static BOOL HGVideoBackdropKnownRenderer(UIView *renderer, UIScrollView *feed) {
    Class metal = NSClassFromString(@"TTMetalView");
    if (!metal || HGAppearancePlayerIsTransferring(renderer) ||
        renderer.subviews.count != 1 || renderer.layer.sublayers.count != 1 ||
        renderer.layer.mask || !CATransform3DIsIdentity(renderer.layer.sublayerTransform) ||
        !HGVideoBackdropVisible(renderer, feed)) return NO;
    UIView *surface = renderer.subviews.firstObject;
    CGFloat pixel = 1 / MAX(1, renderer.traitCollection.displayScale) + 0.01;
    return [surface isKindOfClass:metal] && !surface.hidden && surface.alpha >= 0.99 &&
        surface.layer == renderer.layer.sublayers.firstObject &&
        CGRectGetWidth(renderer.bounds) >= 100 && CGRectGetHeight(renderer.bounds) >= 80 &&
        fabs(CGRectGetMinX(surface.frame) - CGRectGetMinX(renderer.bounds)) <= pixel &&
        fabs(CGRectGetMinY(surface.frame) - CGRectGetMinY(renderer.bounds)) <= pixel &&
        fabs(CGRectGetMaxX(surface.frame) - CGRectGetMaxX(renderer.bounds)) <= pixel &&
        fabs(CGRectGetMaxY(surface.frame) - CGRectGetMaxY(renderer.bounds)) <= pixel;
}

static UIView *HGVideoBackdropRenderer(UIScrollView *feed) {
    Class renderClass = NSClassFromString(@"TTPlayerView");
    if (!renderClass) return nil;
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:feed];
    UIView *best = nil;
    CGFloat bestScore = -1;
    CGRect viewport = feed.bounds;
    for (NSUInteger i = 0; i < queue.count && i < 256; i++) {
        UIView *view = queue[i];
        if (view.hidden || view.alpha < 0.99) continue;
        if ([view isKindOfClass:renderClass]) {
            if (!HGVideoBackdropKnownRenderer(view, feed)) continue;
            UIView *canvas = HGVideoBackdropCanvasForRenderer(view, feed);
            if (!canvas) continue;
            CGRect page = [canvas convertRect:canvas.bounds toView:feed];
            CGRect visible = CGRectIntersection(page, viewport);
            if (CGRectIsNull(visible) || CGRectIsEmpty(visible)) continue;
            // During a native vertical swipe the page touching the lower edge
            // supplies that edge's background. No paging offset is modified.
            BOOL touchesBottom = CGRectGetMinY(page) < CGRectGetMaxY(viewport) - 0.75 &&
                CGRectGetMaxY(page) >= CGRectGetMaxY(viewport) - 0.75;
            CGFloat score = visible.size.width * visible.size.height;
            if (touchesBottom) score += viewport.size.width * viewport.size.height;
            if (score > bestScore) { best = view; bestScore = score; }
            continue;
        }
        if ([view isKindOfClass:UIVisualEffectView.class] || [view isKindOfClass:UILabel.class] ||
            [view isKindOfClass:UIImageView.class]) continue;
        for (UIView *child in view.subviews) {
            if (queue.count >= 256) break;
            [queue addObject:child];
        }
    }
    return best;
}

static UIColor *HGVideoBackdropLetterboxColor(UIView *renderer, UIScrollView *feed) {
    // Read only already-visible UIKit backing colors. Content-model getters,
    // poster images and the host's playback configuration are never inspected.
    for (UIView *node = renderer; node && node != feed; node = node.superview) {
        UIColor *color = node.backgroundColor;
        if (color && CGColorGetAlpha(color.CGColor) >= 0.99) return color;
    }
    return UIColor.blackColor;
}

@implementation HGVideoBackdrop
- (void)dealloc {
    _portal.sourceView = nil;
    [_extension removeFromSuperview];
}
- (void)recordStatus:(NSString *)status {
    if ([self.lastStatus isEqual:status]) return;
    self.lastStatus = status;
    HGRecordCapability(@"appearance.dock.video-backdrop", @{@"module":@"appearance",
        @"state":[status isEqual:@"live-reflected-edge"] ? @"installed" : @"partial",
        @"reason":status, @"nativeGeometryPreserved":@YES, @"nativeHierarchyPreserved":@YES,
        @"snapshotPolling":@NO});
}
- (void)removeBackdrop {
    UIScrollView *feed = self.feed;
    UIView<HGVideoPortalAPI> *portal = self.portal;
    UIView *extension = self.extension;
    self.feed = nil; self.parent = nil; self.root = nil; self.bar = nil; self.owner = nil;
    self.renderer = nil; self.portal = nil; self.canvas = nil; self.extension = nil;
    HGVideoBackdropLink *link = objc_getAssociatedObject(feed, &HGVideoBackdropLinkKey);
    if (link.owner == self) objc_setAssociatedObject(feed, &HGVideoBackdropLinkKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    portal.sourceView = nil;
    [extension removeFromSuperview];
}
- (void)restore {
    if (self.mutating) return;
    self.mutating = YES;
    @try { [self removeBackdrop]; }
    @finally { self.mutating = NO; }
}
- (void)updateOwnedBackdrop {
    UIScrollView *feed = self.feed;
    UIView *parent = self.parent;
    if (!HGVideoBackdropFeedGeometry(feed, parent, self.root, self.bar) ||
        self.owner.viewIfLoaded != parent || HGVideoBackdropHasTransition(self.owner) ||
        self.extension.superview != parent || self.canvas.superview != self.extension ||
        self.portal.superview != self.canvas) {
        [self removeBackdrop]; return;
    }
    UIView *renderer = HGVideoBackdropRenderer(feed);
    if (!renderer) {
        self.portal.sourceView = nil; self.renderer = nil; self.extension.hidden = YES;
        [self recordStatus:@"waiting-for-known-video-surface"]; return;
    }
    [CATransaction begin]; [CATransaction setDisableActions:YES];
    CGFloat edge = CGRectGetMaxY(feed.frame);
    CGRect strip = CGRectMake(CGRectGetMinX(feed.frame), edge, CGRectGetWidth(feed.frame),
        CGRectGetMaxY(parent.bounds) - edge);
    if (!CGRectEqualToRect(self.extension.frame, strip)) self.extension.frame = strip;
    if (!CGRectEqualToRect(self.canvas.frame, self.extension.bounds))
        self.canvas.frame = self.extension.bounds;
    CGRect source = [renderer convertRect:renderer.bounds toView:parent];
    // Reflection about y=edge maps source y to 2*edge-y. In the strip's local
    // coordinates its center is edge-midY(source). The source rectangle already
    // includes the native feed offset and preserves horizontal-video letterbox.
    // Never write frame on this transformed portal; only its bounds and center.
    CGPoint center = CGPointMake(CGRectGetMidX(source) - CGRectGetMinX(strip),
        edge - CGRectGetMidY(source));
    if (!CGRectEqualToRect(self.portal.bounds, renderer.bounds)) self.portal.bounds = renderer.bounds;
    if (!CGPointEqualToPoint(self.portal.center, center)) self.portal.center = center;
    CGAffineTransform reflection = CGAffineTransformMakeScale(1, -1);
    if (!CGAffineTransformEqualToTransform(self.portal.transform, reflection))
        self.portal.transform = reflection;
    UIColor *color = HGVideoBackdropLetterboxColor(renderer, feed);
    if (![self.canvas.backgroundColor isEqual:color]) self.canvas.backgroundColor = color;
    if (self.renderer != renderer || self.portal.sourceView != renderer) {
        self.portal.sourceView = renderer; self.renderer = renderer;
    }
    self.extension.hidden = NO;
    [CATransaction commit];
    [self recordStatus:@"live-reflected-edge"];
}
- (void)refreshFeed {
    if (self.mutating) return;
    self.mutating = YES;
    @try { [self updateOwnedBackdrop]; }
    @finally { self.mutating = NO; }
}
- (void)prepareRendererTransfer:(UIView *)view {
    if (self.mutating || !self.renderer ||
        (self.renderer != view && ![self.renderer isDescendantOfView:view])) return;
    self.mutating = YES;
    @try {
        self.portal.sourceView = nil; self.renderer = nil; self.extension.hidden = YES;
    } @finally { self.mutating = NO; }
}
- (void)refreshWithBranches:(NSArray<UIView *> *)branches root:(UIView *)root
                       bar:(UIView *)bar active:(BOOL)active {
    if (self.mutating) return;
    self.mutating = YES;
    @try {
        if (!active || !root.window || bar.window != root.window ||
            root.bounds.size.height <= root.bounds.size.width) {
            [self removeBackdrop]; return;
        }
        if (@available(iOS 26.0, *)) {
            Class portalClass = HGVideoBackdropPortalClass();
            if (!portalClass) {
                [self removeBackdrop]; [self recordStatus:@"portal-runtime-abi-unavailable"]; return;
            }
            Class feedClass = NSClassFromString(@"FQVCollectionView");
            if (!feedClass) { [self removeBackdrop]; return; }
            NSMutableArray<UIView *> *queue = [branches mutableCopy];
            UIScrollView *candidate = nil;
            UIViewController *owner = nil;
            for (NSUInteger i = 0; i < queue.count && i < 192; i++) {
                UIView *view = queue[i];
                if (view == self.extension || !HGVideoBackdropVisible(view, root) ||
                    CGRectIsEmpty(CGRectIntersection([view convertRect:view.bounds toView:root], root.bounds))) continue;
                if ([view isKindOfClass:feedClass] && [view isKindOfClass:UIScrollView.class]) {
                    UIScrollView *feed = (UIScrollView *)view;
                    UIViewController *controller = HGVideoBackdropOwner(feed);
                    if (!controller || controller.viewIfLoaded != feed.superview ||
                        HGVideoBackdropHasTransition(controller) ||
                        !HGVideoBackdropFeedGeometry(feed, feed.superview, root, bar)) continue;
                    if (candidate && candidate != feed) { [self removeBackdrop]; return; }
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
            if (!candidate) { [self removeBackdrop]; return; }
            if (candidate != self.feed) {
                [self removeBackdrop];
                // Attach only at a settled page. Later native scrolling merely
                // updates our mirror position/source through the feed's layout.
                if (candidate.dragging || candidate.decelerating || !HGVideoBackdropRenderer(candidate)) return;
                HGVideoBackdropLink *existing = objc_getAssociatedObject(candidate, &HGVideoBackdropLinkKey);
                if (existing.owner && existing.owner != self) return;
                UIView *parent = candidate.superview;
                UIView<HGVideoPortalAPI> *portal = [[portalClass alloc] initWithFrame:CGRectZero];
                if (!portal) { [self recordStatus:@"portal-initialization-unavailable"]; return; }
                portal.hidesSourceView = NO;
                portal.matchesPosition = NO; portal.matchesTransform = NO; portal.matchesAlpha = YES;
                portal.allowsHitTesting = NO; portal.forwardsClientHitTestingToSourceView = NO;
                portal.userInteractionEnabled = NO; portal.accessibilityElementsHidden = YES;
                portal.accessibilityIdentifier = @"honggo.video.backdrop.portal";
                CGFloat edge = CGRectGetMaxY(candidate.frame);
                CGRect strip = CGRectMake(CGRectGetMinX(candidate.frame), edge, CGRectGetWidth(candidate.frame),
                    CGRectGetMaxY(parent.bounds) - edge);
                UIView *extension = [[UIView alloc] initWithFrame:strip];
                extension.userInteractionEnabled = NO; extension.accessibilityElementsHidden = YES;
                extension.accessibilityIdentifier = @"honggo.video.backdrop";
                extension.backgroundColor = UIColor.clearColor; extension.opaque = NO;
                extension.clipsToBounds = YES;
                UIView *canvas = [[UIView alloc] initWithFrame:extension.bounds];
                canvas.userInteractionEnabled = NO; canvas.accessibilityElementsHidden = YES;
                canvas.accessibilityIdentifier = @"honggo.video.backdrop.canvas";
                canvas.clipsToBounds = YES; canvas.backgroundColor = UIColor.blackColor;
                [canvas addSubview:portal];
                [extension addSubview:canvas];
                self.feed = candidate; self.parent = parent; self.owner = owner;
                self.root = root; self.bar = bar;
                self.portal = portal; self.canvas = canvas; self.extension = extension;
                HGVideoBackdropLink *link = [HGVideoBackdropLink new]; link.owner = self;
                objc_setAssociatedObject(candidate, &HGVideoBackdropLinkKey, link, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                [parent insertSubview:extension belowSubview:candidate];
            } else {
                self.root = root; self.bar = bar; self.owner = owner;
            }
            [self updateOwnedBackdrop];
        } else {
            [self removeBackdrop]; [self recordStatus:@"requires-ios-26-video-backdrop"];
        }
    } @finally { self.mutating = NO; }
}
@end

BOOL HGVideoBackdropRefreshForFeed(UIScrollView *feed) {
    HGVideoBackdropLink *link = objc_getAssociatedObject(feed, &HGVideoBackdropLinkKey);
    HGVideoBackdrop *owner = link.owner;
    if (!owner) return NO;
    if (!HGAppearanceDockEnabled()) [owner restore];
    else [owner refreshFeed];
    return YES;
}
void HGVideoBackdropPrepareRendererTransfer(UIView *view) {
    Class feedClass = NSClassFromString(@"FQVCollectionView");
    for (UIView *node = view; node; node = node.superview) {
        if (feedClass && [node isKindOfClass:feedClass]) {
            HGVideoBackdropLink *link = objc_getAssociatedObject(node, &HGVideoBackdropLinkKey);
            [link.owner prepareRendererTransfer:view];
            return;
        }
    }
}

#else
@implementation HGVideoBackdrop
- (void)refreshWithBranches:(NSArray<UIView *> *)branches root:(UIView *)root
                       bar:(UIView *)bar active:(BOOL)active { }
- (void)restore { }
@end
BOOL HGVideoBackdropRefreshForFeed(UIScrollView *feed) { return NO; }
void HGVideoBackdropPrepareRendererTransfer(UIView *view) { }
#endif
