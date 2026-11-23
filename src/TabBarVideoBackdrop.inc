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
@property(nonatomic, strong) UIView *underlay;
@property(nonatomic, weak) UIView *portraitRenderer;
@property(nonatomic) CATransform3D portraitTransform;
@property(nonatomic, strong) NSMutableSet<UIView *> *portraitClips;
@property(nonatomic, strong) NSMapTable<UIImageView *, NSNumber *> *cornerAlpha;
@property(nonatomic, weak) CAGradientLayer *bottomGradient;
@property(nonatomic, copy) NSArray *sourceGradientColors;
@property(nonatomic, copy) NSArray<NSNumber *> *sourceGradientLocations;
@property(nonatomic, copy) NSArray *appliedGradientColors;
@property(nonatomic, copy) NSArray<NSNumber *> *appliedGradientLocations;
@property(nonatomic) BOOL ownsClipping;
@property(nonatomic) BOOL refreshing;
- (void)refreshFeed;
- (void)prepareRendererTransfer:(UIView *)view;
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

static UICollectionViewCell *HGVideoBackdropCellAtBottom(UICollectionView *feed, CGRect *edgeFrame) {
    CGRect viewport = feed.bounds;
    CGFloat bottom = CGRectGetMaxY(viewport);
    for (UICollectionViewCell *cell in feed.visibleCells) {
        if (cell.superview != feed || !HGVideoBackdropVisible(cell, feed)) continue;
        CGRect frame = [cell convertRect:cell.bounds toView:feed];
        if (CGRectGetMinY(frame) < bottom - 0.75 && CGRectGetMaxY(frame) >= bottom - 0.75 &&
            fabs(CGRectGetMinX(frame) - CGRectGetMinX(viewport)) <= 0.75 &&
            fabs(CGRectGetMaxX(frame) - CGRectGetMaxX(viewport)) <= 0.75) {
            *edgeFrame = frame;
            return cell;
        }
    }
    return nil;
}

static BOOL HGVideoBackdropCrossesBottom(UICollectionView *feed, CGRect cellFrame) {
    CGRect viewport = feed.bounds;
    CGFloat top = CGRectGetMinY(viewport);
    CGFloat bottom = CGRectGetMaxY(viewport);
    UIEdgeInsets insets = feed.adjustedContentInset;
    // Pull-to-refresh can move the first cell across the edge without a page
    // entering. Let the host keep its clip outside the regular scroll range.
    if (top < -insets.top - 0.75 || bottom > feed.contentSize.height + insets.bottom + 0.75)
        return NO;
    return CGRectGetMinY(cellFrame) > top + 0.75 && CGRectGetMaxY(cellFrame) > bottom + 0.75;
}

static UIView *HGVideoBackdropCanvas(UICollectionViewCell *cell, UICollectionView *feed) {
    Class canvasClass = NSClassFromString(@"FQVShortVideoPlayerCellView");
    CGRect viewport = feed.bounds;
    CGFloat bottom = CGRectGetMaxY(viewport);
    for (UIView *canvas in cell.contentView.subviews) {
        if (![canvas isKindOfClass:canvasClass] || !HGVideoBackdropVisible(canvas, cell)) continue;
        CGRect frame = [canvas convertRect:canvas.bounds toView:feed];
        if (CGRectGetMinY(frame) >= bottom - 0.75 || CGRectGetMaxY(frame) < bottom - 0.75 ||
            fabs(CGRectGetMinX(frame) - CGRectGetMinX(viewport)) > 0.75 ||
            fabs(CGRectGetMaxX(frame) - CGRectGetMaxX(viewport)) > 0.75) continue;
        return canvas;
    }
    return nil;
}

static UIColor *HGVideoBackdropMatte(UIView *canvas) {
    // The host's refreshBgColor resolves its per-video palette on this view.
    UIColor *color = [canvas.backgroundColor resolvedColorWithTraitCollection:canvas.traitCollection];
    CGFloat alpha = 0;
    return [color getRed:NULL green:NULL blue:NULL alpha:&alpha] && alpha >= 0.99 ? color : nil;
}

static BOOL HGVideoBackdropSameRect(CGRect a, CGRect b, CGFloat tolerance) {
    return fabs(CGRectGetMinX(a)-CGRectGetMinX(b)) <= tolerance &&
        fabs(CGRectGetMinY(a)-CGRectGetMinY(b)) <= tolerance &&
        fabs(CGRectGetMaxX(a)-CGRectGetMaxX(b)) <= tolerance &&
        fabs(CGRectGetMaxY(a)-CGRectGetMaxY(b)) <= tolerance;
}

static UIView *HGVideoBackdropPortraitRenderer(UIView *canvas, UICollectionView *feed, CGRect cellFrame) {
    CGRect viewport = feed.bounds;
    if (!canvas || feed.dragging || feed.decelerating ||
        !HGVideoBackdropSameRect(cellFrame, viewport, 0.75)) return nil;
    Class playerClass = NSClassFromString(@"FQVPlayer");
    Class rendererClass = NSClassFromString(@"TTPlayerView");
    Class metalClass = NSClassFromString(@"TTMetalView");
    for (UIView *wrapper in canvas.subviews) {
        UIView *player = wrapper.subviews.firstObject;
        if (wrapper.class != UIView.class || wrapper.subviews.count != 1 ||
            ![player isKindOfClass:playerClass]) continue;
        for (UIView *renderer in player.subviews) {
            if (![renderer isKindOfClass:rendererClass] || !HGVideoBackdropVisible(renderer, feed) ||
                HGAppearancePlayerIsTransferring(renderer) || renderer.subviews.count != 1 ||
                renderer.layer.sublayers.count != 1) continue;
            UIView *metal = renderer.subviews.firstObject;
            if (![metal isKindOfClass:metalClass] || !HGVideoBackdropVisible(metal, renderer) ||
                renderer.layer.sublayers.firstObject != metal.layer ||
                !CGPointEqualToPoint(renderer.layer.anchorPoint, CGPointMake(0.5,0.5))) continue;
            CGFloat pixel = 1 / MAX(1, renderer.traitCollection.displayScale) + 0.01;
            if (!HGVideoBackdropSameRect([renderer convertRect:renderer.bounds toView:feed],viewport,pixel))
                continue;
            // A portrait frame can already be wider than its crop. The real
            // 9:16 surface extends 23.56pt on each side of the 440pt renderer.
            CGRect video = metal.frame, bounds = renderer.bounds;
            if (CGRectGetHeight(video) <= CGRectGetWidth(video) ||
                fabs(CGRectGetMinY(video)-CGRectGetMinY(bounds)) > pixel ||
                fabs(CGRectGetMaxY(video)-CGRectGetMaxY(bounds)) > pixel ||
                fabs(CGRectGetMidX(video)-CGRectGetMidX(bounds)) > pixel ||
                CGRectGetWidth(video) < CGRectGetWidth(bounds)-pixel) continue;
            BOOL clippedByHostMask = NO;
            for (UIView *node = renderer; node != feed; node = node.superview) {
                if (node.layer.mask || (node != renderer && !CATransform3DIsIdentity(node.layer.sublayerTransform))) {
                    clippedByHostMask = YES;
                    break;
                }
            }
            if (!clippedByHostMask) return renderer;
        }
    }
    return nil;
}

static CAGradientLayer *HGVideoBackdropBottomGradient(UIView *canvas, UICollectionView *feed) {
    Class controlsClass = NSClassFromString(@"FQVShortVideoVPlayerControlView");
    Class gradientClass = NSClassFromString(@"FQVShortVideoGradientLayerView");
    CGRect viewport = feed.bounds;
    for (UIView *control in canvas.subviews) {
        if (![control isKindOfClass:controlsClass]) continue;
        for (UIView *view in control.subviews) {
            if (![view isKindOfClass:gradientClass] || !HGVideoBackdropVisible(view,feed)) continue;
            CGRect frame = [view convertRect:view.bounds toView:feed];
            CGRect expected = CGRectMake(CGRectGetMinX(viewport),CGRectGetMaxY(viewport)-frame.size.height,
                viewport.size.width,frame.size.height);
            if (!HGVideoBackdropSameRect(frame,expected,0.75) ||
                view != HGReadObjectGetter(control,"bottomGradientView") ||
                ![view.layer isKindOfClass:CAGradientLayer.class]) continue;
            return (CAGradientLayer *)view.layer;
        }
    }
    return nil;
}

@implementation HGVideoBackdrop
- (void)dealloc {
    [self restorePortrait];
    [self restoreBottomGradient];
    if (_ownsClipping) _feed.clipsToBounds = YES;
    [self restoreCornerImages];
    [_underlay removeFromSuperview];
}

- (void)restoreBottomGradient {
    CAGradientLayer *layer = self.bottomGradient;
    if ([layer.colors isEqualToArray:self.appliedGradientColors] &&
        [layer.locations isEqualToArray:self.appliedGradientLocations]) {
        [CATransaction begin]; [CATransaction setDisableActions:YES];
        layer.colors = self.sourceGradientColors;
        layer.locations = self.sourceGradientLocations;
        [CATransaction commit];
    }
    self.bottomGradient = nil;
    self.sourceGradientColors = nil; self.sourceGradientLocations = nil;
    self.appliedGradientColors = nil; self.appliedGradientLocations = nil;
}

- (void)updateBottomGradient:(CAGradientLayer *)layer {
    if (layer && layer == self.bottomGradient &&
        [layer.colors isEqualToArray:self.appliedGradientColors] &&
        [layer.locations isEqualToArray:self.appliedGradientLocations]) return;
    [self restoreBottomGradient];
    // The host uses two black stops across a vertical axial gradient. Keep
    // its authored shading through the HUD and fade only its last 16 points
    // to zero before the otherwise abrupt edge over the live video.
    if (!layer || layer.colors.count != 2 || ![layer.locations isEqualToArray:@[@0,@1]] ||
        ![layer.type isEqual:kCAGradientLayerAxial] ||
        !CGPointEqualToPoint(layer.startPoint,CGPointMake(0.5,0)) ||
        !CGPointEqualToPoint(layer.endPoint,CGPointMake(0.5,1)) ||
        CGRectGetHeight(layer.bounds) <= 32) return;
    CGColorRef start = (__bridge CGColorRef)layer.colors.firstObject;
    CGColorRef end = (__bridge CGColorRef)layer.colors.lastObject;
    if (CGColorGetAlpha(start) != 0 || CGColorGetAlpha(end) <= 0) return;
    CGFloat fadeAt = 1 - 16 / CGRectGetHeight(layer.bounds);
    CGColorRef fade = CGColorCreateCopyWithAlpha(end,CGColorGetAlpha(end)*fadeAt);
    if (!fade) return;
    self.bottomGradient = layer;
    self.sourceGradientColors = layer.colors;
    self.sourceGradientLocations = layer.locations;
    self.appliedGradientColors = @[(__bridge id)start,CFBridgingRelease(fade),(__bridge id)start];
    self.appliedGradientLocations = @[@0,@(fadeAt),@1];
    [CATransaction begin]; [CATransaction setDisableActions:YES];
    layer.colors = self.appliedGradientColors;
    layer.locations = self.appliedGradientLocations;
    [CATransaction commit];
}

- (void)restoreCornerImages {
    for (UIImageView *image in self.cornerAlpha)
        if (image.alpha == 0) image.alpha = [[self.cornerAlpha objectForKey:image] doubleValue];
    [self.cornerAlpha removeAllObjects];
}

- (void)updateCornerImages {
    // The host mounts a pair of 12pt corner images as siblings of its feed
    // (FQVShortVideoListVCUIComponent setupCornerRadiusImageView). They stay
    // at the old bottom edge even when a live renderer or matching matte
    // now continues through the dock strip.
    CGRect frame = self.feed.frame;
    CGRect leftFrame = CGRectMake(CGRectGetMinX(frame), CGRectGetMaxY(frame)-12, 12, 12);
    CGRect rightFrame = CGRectOffset(leftFrame, CGRectGetWidth(frame)-12, 0);
    UIImageView *left = nil, *right = nil;
    for (UIView *view in self.feed.superview.subviews) {
        if (![view isKindOfClass:UIImageView.class] || view.userInteractionEnabled) continue;
        if (HGVideoBackdropSameRect(view.frame,leftFrame,0.5)) {
            if (left) { [self restoreCornerImages]; return; }
            left = (UIImageView *)view;
        } else if (HGVideoBackdropSameRect(view.frame,rightFrame,0.5)) {
            if (right) { [self restoreCornerImages]; return; }
            right = (UIImageView *)view;
        }
    }
    if (!left || !right) { [self restoreCornerImages]; return; }
    for (UIImageView *image in self.cornerAlpha.keyEnumerator.allObjects) {
        if (image == left || image == right) continue;
        if (image.alpha == 0) image.alpha = [[self.cornerAlpha objectForKey:image] doubleValue];
        [self.cornerAlpha removeObjectForKey:image];
    }
    if (!self.cornerAlpha) self.cornerAlpha = [NSMapTable weakToStrongObjectsMapTable];
    for (UIImageView *image in @[left,right]) {
        if (image.alpha == 0) continue;
        [self.cornerAlpha setObject:@(image.alpha) forKey:image];
        image.alpha = 0;
    }
}

- (void)restoreClipping {
    BOOL owned = self.ownsClipping;
    self.ownsClipping = NO;
    if (owned && !self.feed.clipsToBounds) self.feed.clipsToBounds = YES;
}

- (void)restorePortrait {
    UIView *renderer = self.portraitRenderer;
    NSMutableSet<UIView *> *clips = self.portraitClips;
    self.portraitRenderer = nil;
    self.portraitClips = nil;
    if (!renderer && !clips.count) return;
    [CATransaction begin]; [CATransaction setDisableActions:YES];
    if (renderer && CATransform3DEqualToTransform(renderer.layer.sublayerTransform,self.portraitTransform))
        renderer.layer.sublayerTransform = CATransform3DIdentity;
    for (UIView *view in clips) if (!view.clipsToBounds) view.clipsToBounds = YES;
    [CATransaction commit];
}

- (BOOL)extendPortrait:(UIView *)renderer {
    if (self.portraitRenderer != renderer) [self restorePortrait];
    if (!renderer) return NO;
    CATransform3D current = renderer.layer.sublayerTransform;
    if (!CATransform3DIsIdentity(current) &&
        (self.portraitRenderer != renderer || !CATransform3DEqualToTransform(current,self.portraitTransform))) {
        [self restorePortrait];
        return NO;
    }
    CGFloat visibleHeight = CGRectGetHeight(self.feed.bounds);
    CGFloat extra = CGRectGetMaxY(self.feed.superview.bounds) - CGRectGetMaxY(self.feed.frame);
    CGFloat zoom = (visibleHeight + extra) / visibleHeight;
    CATransform3D fill = CATransform3DMakeScale(zoom,zoom,1);
    // Keep the existing top as the anchor and extend downward to the window.
    fill.m42 = extra / 2;
    if (!self.portraitClips) self.portraitClips = [NSMutableSet set];
    self.portraitRenderer = renderer;
    self.portraitTransform = fill;
    [CATransaction begin]; [CATransaction setDisableActions:YES];
    if (!CATransform3DEqualToTransform(current,fill)) renderer.layer.sublayerTransform = fill;
    for (UIView *view = renderer; view && view != self.feed; view = view.superview) {
        if (view.clipsToBounds) {
            [self.portraitClips addObject:view];
            view.clipsToBounds = NO;
        }
    }
    [CATransaction commit];
    return YES;
}

- (void)removeFeed {
    UICollectionView *feed = self.feed;
    HGVideoBackdropLink *link = objc_getAssociatedObject(feed, &HGVideoBackdropLinkKey);
    if (link.owner == self)
        objc_setAssociatedObject(feed, &HGVideoBackdropLinkKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [self restorePortrait];
    [self restoreClipping];
    [self restoreBottomGradient];
    [self restoreCornerImages];
    [self.underlay removeFromSuperview];
    self.underlay = nil;
    self.feed = nil; self.root = nil; self.bar = nil; self.owner = nil;
}

- (void)updateUnderlay:(UIColor *)color {
    UIView *parent = self.feed.superview;
    UIView *underlay = self.underlay;
    if (!color) {
        underlay.hidden = YES;
        return;
    }
    if (!underlay) {
        underlay = [UIView new];
        underlay.accessibilityIdentifier = @"honggo.video.matte";
        underlay.accessibilityElementsHidden = YES;
        underlay.userInteractionEnabled = NO;
        self.underlay = underlay;
    }
    CGRect frame = self.feed.frame;
    CGFloat edge = CGRectGetMaxY(frame);
    CGRect strip = CGRectMake(CGRectGetMinX(frame), edge, CGRectGetWidth(frame),
        CGRectGetMaxY(parent.bounds) - edge);
    if (!CGRectEqualToRect(underlay.frame, strip)) underlay.frame = strip;
    if (![underlay.backgroundColor isEqual:color]) underlay.backgroundColor = color;
    NSUInteger index = [parent.subviews indexOfObjectIdenticalTo:underlay];
    NSUInteger feedIndex = [parent.subviews indexOfObjectIdenticalTo:self.feed];
    if (index == NSNotFound || index >= feedIndex) [parent insertSubview:underlay belowSubview:self.feed];
    underlay.hidden = NO;
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

    CGRect cellFrame = CGRectNull;
    UICollectionViewCell *cell = HGVideoBackdropCellAtBottom(feed, &cellFrame);
    UIView *canvas = HGVideoBackdropCanvas(cell, feed);
    UIView *renderer = HGVideoBackdropPortraitRenderer(canvas, feed, cellFrame);
    BOOL fillsPortrait = [self extendPortrait:renderer];
    UIColor *matte = fillsPortrait ? nil : HGVideoBackdropMatte(canvas);
    [self updateUnderlay:matte];
    [self updateBottomGradient:fillsPortrait ? HGVideoBackdropBottomGradient(canvas,feed) : nil];

    // A prefetched next cell can already exist at the exact edge while idle.
    // Only a visible cell with a portion on both sides may draw below the feed.
    BOOL overflows = fillsPortrait || (cell && HGVideoBackdropCrossesBottom(feed, cellFrame));
    if (overflows || matte) [self updateCornerImages];
    else [self restoreCornerImages];
    if (!overflows) {
        [self restoreClipping];
        return;
    }

    if (feed.clipsToBounds) {
        self.ownsClipping = YES;
        feed.clipsToBounds = NO;
    }
}

- (void)prepareRendererTransfer:(UIView *)view {
    UIView *renderer = self.portraitRenderer;
    if (!renderer || (renderer != view && ![renderer isDescendantOfView:view])) return;
    [self restorePortrait];
    [self restoreClipping];
    [self restoreBottomGradient];
    [self restoreCornerImages];
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
    if (HGAppearanceDockEnabled() || HGGlassEnabled) [owner refreshFeed];
    else [owner restore];
    return YES;
}

static UIScrollView *HGVideoBackdropPlayerFeed(UIView *view) {
    Class feedClass = NSClassFromString(@"FQVCollectionView");
    for (UIView *node = view.superview; node; node = node.superview)
        if ([node isKindOfClass:feedClass] && [node isKindOfClass:UICollectionView.class])
            return (UIScrollView *)node;
    return nil;
}

void HGRefreshVideoBackdropForPlayer(UIView *view) {
    if (!NSThread.isMainThread) return;
    UIScrollView *feed = HGVideoBackdropPlayerFeed(view);
    if (feed) HGVideoBackdropRefreshForFeed(feed);
}

void HGVideoBackdropPrepareRendererTransfer(UIView *view) {
    if (!NSThread.isMainThread) return;
    UIScrollView *feed = HGVideoBackdropPlayerFeed(view);
    if (!feed) return;
    HGVideoBackdropLink *link = objc_getAssociatedObject(feed, &HGVideoBackdropLinkKey);
    [link.owner prepareRendererTransfer:view];
}
