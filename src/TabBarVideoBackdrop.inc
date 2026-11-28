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

typedef struct {
    CGRect frame;
    CGRect appliedFrame;
    UIViewContentMode contentMode;
} HGVideoPortraitCover;

@interface HGVideoBackdrop ()
@property(nonatomic, weak) UICollectionView *feed;
@property(nonatomic, weak) UIView *root;
@property(nonatomic, weak) UIView *bar;
@property(nonatomic, weak) UIViewController *owner;
@property(nonatomic, strong) UIView *underlay;
@property(nonatomic, strong) NSArray<CALayer *> *pagingMattes;
@property(nonatomic, strong) NSMapTable<UIView *, NSValue *> *portraitRenderers;
@property(nonatomic, strong) NSMapTable<UIImageView *, NSValue *> *portraitCovers;
@property(nonatomic, strong) NSHashTable<UIView *> *portraitClips;
@property(nonatomic, strong) NSMapTable<CAGradientLayer *, NSNumber *> *portraitGradients;
@property(nonatomic, weak) UICollectionViewCell *frontPortraitCell;
@property(nonatomic, strong) NSMapTable<UIImageView *, NSNumber *> *cornerAlpha;
@property(nonatomic) CGRect lastCornerFrame;
@property(nonatomic, weak) UIView *detailTray;
@property(nonatomic, strong) UIColor *detailTrayPaint;
@property(nonatomic, strong) NSMutableArray<HGVideoBackdrop *> *peers;
@property(nonatomic, weak) UIView *horizontalClipPage;
@property(nonatomic) BOOL ownsHorizontalClip;
@property(nonatomic) BOOL ownsClipping;
@property(nonatomic) BOOL refreshing;
@property(nonatomic, strong) NSHashTable<CALayer *> *seamMaskedLayers;
@property(nonatomic, strong) CALayer *topMatteLayer;
@property(nonatomic, weak) UICollectionViewCell *topMatteCell;
- (void)refreshFeed;
- (void)prepareRendererTransfer:(UIView *)view;
- (void)refreshDetail:(UIView *)canvas;
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

static Class HGBackdropResolveClass(const char *name, Class *cache) {
    Class cls = *cache;
    if (!cls) { cls = objc_lookUpClass(name); *cache = cls; }
    return cls;
}

static UIViewController *HGVideoBackdropOwner(UIView *view) {
    static Class expectedCache;
    Class expected = HGBackdropResolveClass("FQVShortVideoListViewController", &expectedCache);
    if (!expected) return nil;
    for (UIResponder *responder = view.nextResponder; responder; responder = responder.nextResponder) {
        if ([responder isKindOfClass:UIViewController.class])
            return [responder isKindOfClass:expected] ? (UIViewController *)responder : nil;
    }
    return nil;
}

static BOOL HGVideoBackdropControllerInSubtree(UIViewController *controller, UIViewController *ancestor) {
    if (!controller || !ancestor) return NO;
    for (UIViewController *node = controller; node; node = node.parentViewController)
        if (node == ancestor) return YES;
    return NO;
}

static BOOL HGVideoBackdropIsNavigationTransition(id<UIViewControllerTransitionCoordinator> tc,
                                                  UIViewController *owner) {
    if (!tc || !owner) return NO;
    UINavigationController *nav = owner.navigationController;
    if (!nav || nav.transitionCoordinator != tc) return NO;
    for (UIViewController *node = owner; node; node = node.parentViewController) {
        if (node.isBeingPresented || node.isBeingDismissed) return NO;
        if (node == nav) break;
    }
    UIViewController *fromVC = [tc viewControllerForKey:UITransitionContextFromViewControllerKey];
    UIViewController *toVC = [tc viewControllerForKey:UITransitionContextToViewControllerKey];
    if (toVC && HGVideoBackdropControllerInSubtree(owner, toVC)) return YES;
    if (fromVC && HGVideoBackdropControllerInSubtree(owner, fromVC)) return YES;
    return !fromVC && !toVC && tc.presentationStyle == UIModalPresentationNone;
}

static BOOL HGVideoBackdropHasTransition(UIViewController *owner) {
    for (UIViewController *controller = owner; controller; controller = controller.parentViewController) {
        if (controller.isBeingDismissed || controller.isBeingPresented) return YES;
        id<UIViewControllerTransitionCoordinator> tc = controller.transitionCoordinator;
        if (tc && tc.presentationStyle != UIModalPresentationNone &&
            !HGVideoBackdropIsNavigationTransition(tc, owner)) return YES;
    }
    return NO;
}

static BOOL HGVideoBackdropFullWindow(UIView *view, UIWindow *window) {
    CGRect rect = [view convertRect:view.bounds toView:window];
    CGRect bounds = window.bounds;
    return fabs(CGRectGetMinX(rect) - CGRectGetMinX(bounds)) <= 0.75 &&
        fabs(CGRectGetMaxX(rect) - CGRectGetMaxX(bounds)) <= 0.75 &&
        fabs(CGRectGetMinY(rect) - CGRectGetMinY(bounds)) <= 0.75 &&
        fabs(CGRectGetMaxY(rect) - CGRectGetMaxY(bounds)) <= 0.75;
}

static BOOL HGVideoBackdropSameRect(CGRect a, CGRect b, CGFloat tolerance) {
    return fabs(CGRectGetMinX(a)-CGRectGetMinX(b)) <= tolerance &&
        fabs(CGRectGetMinY(a)-CGRectGetMinY(b)) <= tolerance &&
        fabs(CGRectGetMaxX(a)-CGRectGetMaxX(b)) <= tolerance &&
        fabs(CGRectGetMaxY(a)-CGRectGetMaxY(b)) <= tolerance;
}

static BOOL HGVideoBackdropHorizontalWindowSlice(UIView *view, UIWindow *window) {
    if (!view || !window) return NO;
    CGRect rect = [view convertRect:view.bounds toView:window];
    CGRect bounds = window.bounds;
    return fabs(CGRectGetMinY(rect) - CGRectGetMinY(bounds)) <= 0.75 &&
        fabs(CGRectGetMaxY(rect) - CGRectGetMaxY(bounds)) <= 0.75 &&
        fabs(CGRectGetWidth(rect) - CGRectGetWidth(bounds)) <= 0.75 &&
        !CGRectIsEmpty(CGRectIntersection(rect, bounds));
}

static UIView *HGVideoBackdropCategoryPager(UIView *view, UIWindow *window) {
    if (!view || !window) return nil;
    static Class containerCache;
    Class containerClass = HGBackdropResolveClass("FQRJXCategoryListContainerView", &containerCache);
    if (!containerClass) return nil;
    UIScrollView *pagerScroll = nil;
    for (UIView *node = view.superview; node && node != window; node = node.superview) {
        if (!pagerScroll && [node isKindOfClass:UIScrollView.class]) pagerScroll = (UIScrollView *)node;
        if ([node isKindOfClass:containerClass]) {
            UIView *contentScroll = HGView(node, "contentScrollView");
            if (pagerScroll && contentScroll == pagerScroll &&
                HGVideoBackdropHorizontalWindowSlice(node, window) &&
                HGVideoBackdropHorizontalWindowSlice(pagerScroll, window))
                return node;
            return nil;
        }
    }
    return nil;
}

static BOOL HGVideoBackdropFeedGeometry(UICollectionView *feed, UIViewController *owner,
                                        UIView *root, UIView *bar) {
    UIView *parent = feed.superview;
    UIWindow *window = root.window;
    UIView *ownerView = owner.viewIfLoaded;
    if (!window || !parent || !ownerView || (parent != ownerView && ![parent isDescendantOfView:ownerView]) ||
        window.bounds.size.height <= window.bounds.size.width || feed.layer.mask ||
        !HGVideoBackdropVisible(feed, window) || bar.window!=window ||
        !HGVideoBackdropHorizontalWindowSlice(root, window) ||
        HGVideoBackdropHasTransition(owner)) return NO;
    BOOL alignedParent = HGVideoBackdropSameRect([parent convertRect:parent.bounds toView:root], root.bounds, 0.75);
    if (!alignedParent && HGVideoBackdropCategoryPager(parent, window)) {
        alignedParent = HGVideoBackdropHorizontalWindowSlice(parent, window);
    }
    if (!alignedParent) return NO;
    CGRect frame = feed.frame, full = parent.bounds;
    CGFloat uncovered = CGRectGetMaxY(full) - CGRectGetMaxY(frame);
    return uncovered >= -0.75 && uncovered <= 160 &&
        fabs(CGRectGetMinX(frame) - CGRectGetMinX(full)) <= 0.75 &&
        fabs(CGRectGetMinY(frame) - CGRectGetMinY(full)) <= 0.75 &&
        fabs(CGRectGetMaxX(frame) - CGRectGetMaxX(full)) <= 0.75;
}

static void HGVideoBackdropEdgeCells(UICollectionView *feed, UICollectionViewCell **topCell, CGRect *topFrame,
    UICollectionViewCell **bottomCell, CGRect *bottomFrame) {
    CGRect viewport = feed.bounds;
    CGFloat topY=CGRectGetMinY(viewport)+0.75;
    CGFloat bottomY=CGRectGetMaxY(viewport)-0.75;
    // Only inspect realized, attached cells. Asking UICollectionView for its
    // visible cells can synchronously trigger a layout during host recycling.
    for (UIView *child in feed.subviews) {
        if (![child isKindOfClass:UICollectionViewCell.class] || !HGVideoBackdropVisible(child, feed)) continue;
        UICollectionViewCell *cell = (UICollectionViewCell *)child;
        CGRect frame = [cell convertRect:cell.bounds toView:feed];
        if (fabs(CGRectGetMinX(frame)-CGRectGetMinX(viewport))>0.75 ||
            fabs(CGRectGetMaxX(frame)-CGRectGetMaxX(viewport))>0.75) continue;
        CGFloat start=CGRectGetMinY(frame), end=CGRectGetMaxY(frame);
        if (!*topCell && start<=topY && end>topY) {
            *topCell=cell;
            *topFrame=frame;
        }
        if (!*bottomCell && start<=bottomY && end>bottomY) {
            *bottomCell=cell;
            *bottomFrame=frame;
        }
        if (*topCell && *bottomCell) break;
    }
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

static UIView *HGVideoBackdropCanvas(UICollectionViewCell *cell, UICollectionView *feed, CGFloat y) {
    static Class canvasCache;
    Class canvasClass = HGBackdropResolveClass("FQVShortVideoPlayerCellView", &canvasCache);
    CGRect viewport = feed.bounds;
    for (UIView *canvas in cell.contentView.subviews) {
        if (![canvas isKindOfClass:canvasClass] || !HGVideoBackdropVisible(canvas, cell)) continue;
        CGRect frame = [canvas convertRect:canvas.bounds toView:feed];
        if (CGRectGetMinY(frame) > y || CGRectGetMaxY(frame) <= y ||
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

static BOOL HGVideoBackdropHasLetterbox(UIView *canvas) {
    static Class playerCache, rendererCache;
    Class playerClass = HGBackdropResolveClass("FQVPlayer", &playerCache);
    Class rendererClass = HGBackdropResolveClass("TTPlayerView", &rendererCache);
    for (UIView *wrapper in canvas.subviews) {
        UIView *player=wrapper.subviews.firstObject;
        if (![player isKindOfClass:playerClass]) continue;
        for (UIView *renderer in player.subviews) {
            if (![renderer isKindOfClass:rendererClass] || renderer.hidden) continue;
            CGRect frame=[renderer convertRect:renderer.bounds toView:canvas];
            return frame.size.width>frame.size.height && frame.size.height>0 &&
                CGRectGetMinY(frame)>CGRectGetMinY(canvas.bounds)+1 &&
                CGRectGetMaxY(frame)<CGRectGetMaxY(canvas.bounds)-1;
        }
    }
    return NO;
}

static BOOL HGVideoBackdropMatchesViewport(CGRect candidate, CGRect viewport) {
    if (HGVideoBackdropSameRect(candidate,viewport,0.75)) return YES;
    // Host refreshCoverImageView can size a portrait poster with aspect-fill
    // overscan centered on the same viewport (matching TTMetalView).
    return candidate.size.width >= viewport.size.width-1.5 &&
        candidate.size.height >= viewport.size.height-1.5 &&
        fabs(CGRectGetMidX(candidate)-CGRectGetMidX(viewport)) <= 1.5 &&
        fabs(CGRectGetMidY(candidate)-CGRectGetMidY(viewport)) <= 1.5;
}

// The outer feed also plays width-fitted portrait clips in a centered,
// shorter TTPlayerView (402 x 714.67 inside the 402 x 784 feed on device).
static BOOL HGVideoBackdropCenteredPortrait(CGRect frame, CGRect viewport) {
    return CGRectGetHeight(frame)>CGRectGetWidth(frame) && CGRectGetHeight(frame)<CGRectGetHeight(viewport) &&
        fabs(CGRectGetMinX(frame)-CGRectGetMinX(viewport))<=0.75 &&
        fabs(CGRectGetMaxX(frame)-CGRectGetMaxX(viewport))<=0.75 &&
        fabs(CGRectGetMidY(frame)-CGRectGetMidY(viewport))<=0.75;
}

// A detail HUD can reserve space for its episode controls below the picture.
// The native width-fit portrait then sits above the screen's vertical center.
static BOOL HGDetailInsetPortrait(CGRect picture, CGRect page) {
    return CGRectGetHeight(picture)>CGRectGetWidth(picture) &&
        fabs(CGRectGetMinX(picture)-CGRectGetMinX(page))<=0.75 &&
        fabs(CGRectGetMaxX(picture)-CGRectGetMaxX(page))<=0.75 &&
        CGRectGetMinY(picture)>=CGRectGetMinY(page)-0.75 &&
        CGRectGetMaxY(picture)<=CGRectGetMaxY(page)+0.75;
}

// Measure the actual picture, which can extend beyond the native renderer
// frame. Scaling from the frame instead can zoom a tall source unnecessarily.
static CATransform3D HGVideoBackdropCoverTransform(CGSize picture, CGRect native, CGRect full) {
    CGFloat scale=MAX(full.size.width/picture.width,full.size.height/picture.height);
    CATransform3D transform=CATransform3DMakeScale(scale,scale,1);
    transform.m42=CGRectGetMidY(full)-CGRectGetMidY(native);
    return transform;
}

static BOOL HGVideoBackdropHasHostMask(UIView *view) {
    CALayer *mask = view.layer.mask;
    return mask && ![mask.name isEqualToString:@"honggo.video.seam-mask"];
}

static CGFloat HGVideoBackdropSeamExtension(CGFloat advance, CGFloat extra) {
    if (extra <= 0.75 || advance <= 0) return MAX(0, extra);
    if (advance >= extra) return 0;
    CGFloat u = advance / extra;
    return extra * (1.0 - u * u);
}

static UIView *HGVideoBackdropMetal(UIView *renderer, UIView *player, CGRect viewport) {
    CGRect frame=[renderer convertRect:renderer.bounds toView:player];
    if (HGAppearancePlayerIsTransferring(renderer) || renderer.subviews.count != 1 ||
        renderer.layer.sublayers.count != 1 ||
        (!HGVideoBackdropSameRect(frame,viewport,0.75) && !HGVideoBackdropCenteredPortrait(frame,viewport)) ||
        !CGPointEqualToPoint(renderer.layer.anchorPoint,CGPointMake(0.5,0.5))) return nil;
    UIView *metal=renderer.subviews.firstObject;
    static Class metalCache;
    if (![metal isKindOfClass:HGBackdropResolveClass("TTMetalView", &metalCache)] || renderer.layer.sublayers.firstObject!=metal.layer ||
        metal.bounds.size.width<=0 || metal.bounds.size.height<=metal.bounds.size.width ||
        fabs(metal.center.x-CGRectGetMidX(renderer.bounds))>0.75 ||
        fabs(metal.center.y-CGRectGetMidY(renderer.bounds))>0.75) return nil;
    for (UIView *node=metal; node; node=node.superview) {
        if (HGVideoBackdropHasHostMask(node) || !CATransform3DIsIdentity(node.layer.transform) ||
            (node!=renderer && !CATransform3DIsIdentity(node.layer.sublayerTransform))) return nil;
        if (node==player) return metal;
    }
    return nil;
}

static BOOL HGVideoBackdropReadDimension(id info, SEL selector, double *outValue) {
    Method m = class_getInstanceMethod(object_getClass(info), selector);
    const char *enc = m ? method_getTypeEncoding(m) : NULL;
    if (!enc) return NO;
    if (!strcmp(enc, "d16@0:8")) {
        *outValue = ((double(*)(id,SEL))objc_msgSend)(info, selector);
        return YES;
    }
    if (!strcmp(enc, "q16@0:8") || !strcmp(enc, "Q16@0:8")) {
        *outValue = (double)((NSInteger(*)(id,SEL))objc_msgSend)(info, selector);
        return YES;
    }
    return NO;
}

static BOOL HGVideoBackdropInfoIsPortrait(id info, BOOL *resolved) {
    if (!info) return NO;
    double x = 0, y = 0;
    if (HGVideoBackdropReadDimension(info, sel_registerName("videoWidth"), &x) &&
        HGVideoBackdropReadDimension(info, sel_registerName("videoHeight"), &y) &&
        x > 0 && y > 0 && isfinite(x) && isfinite(y)) {
        if (resolved) *resolved = YES;
        return y > x;
    }
    SEL verticalSel = sel_registerName("vertical");
    Method vm = class_getInstanceMethod(object_getClass(info), verticalSel);
    const char *venc = vm ? method_getTypeEncoding(vm) : NULL;
    if (venc && !strcmp(venc, "B16@0:8") && ((BOOL(*)(id,SEL))objc_msgSend)(info, verticalSel)) {
        if (resolved) *resolved = YES;
        return YES;
    }
    return NO;
}

// The cell's model is available before its poster or Metal surface. Both
// source sizes are authored by the host, independent of the screen bounds.
static char HGVideoBackdropPortraitModelKey;
static BOOL HGVideoBackdropPortraitModel(id model) {
    if (!model) return NO;
    NSNumber *cached = objc_getAssociatedObject(model, &HGVideoBackdropPortraitModelKey);
    if (cached) return cached.boolValue;
    BOOL resolved = NO;
    BOOL result = HGVideoBackdropInfoIsPortrait(model, &resolved);
    if (resolved) {
        objc_setAssociatedObject(model, &HGVideoBackdropPortraitModelKey, @(result), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        return result;
    }
    const char *keys[] = {"shortVideoModel","seriesShortVideoModel","videoInfo","videoDataModel","getVideoDataModel",
        "videoData","episodeVideoData","seriesVideoData","derivateVideoData",
        "filterableEpisodeVideoData","filterableSeriesVideoData"};
    const char *inner[] = {"shortVideoModel","seriesShortVideoModel","videoInfo","videoData"};
    for (NSUInteger i = 0; i < sizeof(keys)/sizeof(keys[0]); i++) {
        id child = HGReadObjectGetter(model, keys[i]);
        if (!child) continue;
        result = HGVideoBackdropInfoIsPortrait(child, &resolved);
        if (resolved) {
            objc_setAssociatedObject(model, &HGVideoBackdropPortraitModelKey, @(result), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            return result;
        }
        for (NSUInteger j = 0; j < sizeof(inner)/sizeof(inner[0]); j++) {
            result = HGVideoBackdropInfoIsPortrait(HGReadObjectGetter(child, inner[j]), &resolved);
            if (resolved) {
                objc_setAssociatedObject(model, &HGVideoBackdropPortraitModelKey, @(result), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                return result;
            }
        }
    }
    return NO;
}
static BOOL HGVideoBackdropSingleList(UIViewController *owner) {
    id list=HGReadObjectGetter(owner,"listPlayer");
    SEL single=sel_registerName("isSingleSeriesList");
    Method method=class_getInstanceMethod(object_getClass(list),single);
    return method && !strcmp(method_getTypeEncoding(method),"B16@0:8") && ((BOOL(*)(id,SEL))objc_msgSend)(list,single);
}

static UIView *HGDetailVideoBackdropCanvas(UIView *view);

static UIView *HGVideoBackdropPlayer(UIView *canvas) {
    static Class playerCache;
    Class playerClass = HGBackdropResolveClass("FQVPlayer", &playerCache);
    for (UIView *wrapper in canvas.subviews) {
        UIView *player=wrapper.subviews.firstObject;
        if (wrapper.class==UIView.class && wrapper.subviews.count==1 &&
            [player isKindOfClass:playerClass] &&
            HGVideoBackdropSameRect(wrapper.frame,canvas.bounds,0.75) &&
            HGVideoBackdropSameRect(player.frame,wrapper.bounds,0.75) &&
            !HGVideoBackdropHasHostMask(wrapper) && CATransform3DIsIdentity(wrapper.layer.transform) &&
            CATransform3DIsIdentity(wrapper.layer.sublayerTransform)) return player;
    }
    return nil;
}

static UIImageView *HGVideoBackdropCover(UIView *owner) {
    UIImageView *cover=(id)HGView(owner,"coverImageView");
    return [cover isKindOfClass:UIImageView.class] && [cover isDescendantOfView:owner] ? cover : nil;
}

static BOOL HGVideoBackdropPortraitImage(UIImageView *cover) {
    return cover.image.size.height>cover.image.size.width;
}

static BOOL HGVideoBackdropLooksPortrait(UIView *canvas) {
    if (!canvas) return NO;
    UIView *player=HGVideoBackdropPlayer(canvas);
    if (HGVideoBackdropPortraitModel(HGReadObjectGetter(canvas,"model")) ||
        HGVideoBackdropPortraitModel(HGReadObjectGetter(player,"model"))) return YES;
    if (HGVideoBackdropPortraitImage(HGVideoBackdropCover(canvas)) ||
        HGVideoBackdropPortraitImage(HGVideoBackdropCover(player))) return YES;
    static Class rendererCache, metalCache;
    Class rendererClass=HGBackdropResolveClass("TTPlayerView",&rendererCache);
    Class metalClass=HGBackdropResolveClass("TTMetalView",&metalCache);
    for (UIView *renderer in player.subviews) {
        if (![renderer isKindOfClass:rendererClass]) continue;
        UIView *surface=renderer.subviews.firstObject;
        if ([surface isKindOfClass:metalClass] && surface.bounds.size.width>0 &&
            surface.bounds.size.height>surface.bounds.size.width+0.75) return YES;
        if (renderer.bounds.size.width>0 && renderer.bounds.size.height>renderer.bounds.size.width+0.75)
            return YES;
    }
    return NO;
}

static BOOL HGVideoBackdropFrameReady(UIView *player) {
    id model=HGReadObjectGetter(player,"model");
    SEL ready=sel_registerName("readyToDisPlay");
    Method method=class_getInstanceMethod(object_getClass(model),ready);
    return !method || strcmp(method_getTypeEncoding(method),"B16@0:8") ||
        ((BOOL(*)(id,SEL))objc_msgSend)(model,ready);
}

static BOOL HGVideoBackdropCoverVisible(UIImageView *cover, UIView *canvas) {
    for (UIView *node=cover; node!=canvas; node=node.superview)
        if (node.hidden || node.alpha<=0.01) return NO;
    return YES;
}

static CGFloat HGVideoBackdropFirstPictureY(UIView *canvas, UIView *feed) {
    UIView *player=HGVideoBackdropPlayer(canvas);
    CGFloat pictureTop=CGFLOAT_MAX;
    if (player && !player.hidden && player.alpha>0.01 && HGVideoBackdropFrameReady(player)) {
        for (UIView *view in player.subviews) {
            if (![view isKindOfClass:objc_lookUpClass("TTPlayerView")] ||
                view.hidden || view.alpha<=0.01 || HGAppearancePlayerIsTransferring(view)) continue;
            UIView *metal=view.subviews.firstObject;
            if (![metal isKindOfClass:objc_lookUpClass("TTMetalView")] ||
                metal.hidden || metal.alpha<=0.01 || CGRectIsEmpty(metal.bounds)) continue;
            pictureTop=MIN(pictureTop,CGRectGetMinY([metal.layer convertRect:metal.layer.bounds toLayer:feed.layer]));
        }
    }
    UIImageView *covers[]={HGVideoBackdropCover(canvas),HGVideoBackdropCover(player)};
    for (NSUInteger i=0;i<2;i++) {
        UIImageView *cover=covers[i];
        if (!cover.image || (i==1 && cover==covers[0]) || !HGVideoBackdropCoverVisible(cover,canvas)) continue;
        CGRect picture=cover.bounds;
        if (cover.contentMode==UIViewContentModeScaleAspectFit) {
            CGSize size=cover.image.size;
            if (size.width<=0 || size.height<=0) continue;
            CGFloat scale=MIN(picture.size.width / size.width,picture.size.height / size.height);
            picture=CGRectInset(picture,(picture.size.width-size.width*scale)/2,(picture.size.height-size.height*scale)/2);
        }
        if (!CGRectIsEmpty(picture)) pictureTop=MIN(pictureTop,CGRectGetMinY([cover.layer convertRect:picture toLayer:feed.layer]));
    }
    return pictureTop;
}

static CAGradientLayer *HGVideoBackdropBottomGradient(UIView *canvas, CGRect viewport) {
    static Class controlsCache, outterControlsCache, gradientCache;
    Class controlsClass = HGBackdropResolveClass("FQVShortVideoVPlayerControlView", &controlsCache);
    Class outterControlsClass = HGBackdropResolveClass("FQVShortVideoVPlayerOutterMultiSeriesControlView", &outterControlsCache);
    Class gradientClass = HGBackdropResolveClass("FQVShortVideoGradientLayerView", &gradientCache);
    for (UIView *control in canvas.subviews) {
        if (![control isKindOfClass:controlsClass] && ![control isKindOfClass:outterControlsClass]) continue;
        UIView *grad = HGView(control, "bottomGradientView");
        if (grad && [grad isDescendantOfView:control] &&
            (!gradientClass || [grad isKindOfClass:gradientClass]) &&
            [grad.layer isKindOfClass:CAGradientLayer.class]) {
            CGRect frame = [grad convertRect:grad.bounds toView:canvas];
            if (frame.size.width >= viewport.size.width - 1.5 && frame.size.height > 0.5 &&
                CGRectGetMaxY(frame) >= CGRectGetMaxY(viewport) - 24.0 &&
                CGRectGetMinY(frame) >= CGRectGetMidY(viewport)) {
                return (CAGradientLayer *)grad.layer;
            }
        }
        for (UIView *view in control.subviews) {
            if (![view isKindOfClass:gradientClass]) continue;
            CGRect frame = [view convertRect:view.bounds toView:canvas];
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
    for (HGVideoBackdrop *peer in _peers) [peer restore];
    [_peers removeAllObjects];
    for (CALayer *layer in _pagingMattes) [layer removeFromSuperlayer];
    [_topMatteLayer removeFromSuperlayer];
    for (CALayer *layer in _seamMaskedLayers.allObjects)
        if ([layer.mask.name isEqualToString:@"honggo.video.seam-mask"]) layer.mask = nil;
    [self restorePortraitExcept:nil];
    [self bringPortraitForward:nil];
    [self restoreDetailTray];
    if (_ownsHorizontalClip && _horizontalClipPage.clipsToBounds) _horizontalClipPage.clipsToBounds = NO;
    if (_ownsClipping) _feed.clipsToBounds = YES;
    [self restoreCornerImages];
    [_underlay removeFromSuperview];
}

- (void)restoreHorizontalClip {
    if (self.ownsHorizontalClip && self.horizontalClipPage.clipsToBounds)
        self.horizontalClipPage.clipsToBounds = NO;
    self.ownsHorizontalClip = NO;
    self.horizontalClipPage = nil;
}

- (void)applyHorizontalClip:(BOOL)needed {
    UIView *page = needed ? self.feed.superview : nil;
    if (self.horizontalClipPage != page) [self restoreHorizontalClip];
    if (!page) return;
    if (!page.clipsToBounds) {
        self.horizontalClipPage = page;
        self.ownsHorizontalClip = YES;
        page.clipsToBounds = YES;
    }
}

- (void)restoreDetailTray {
    if ([self.detailTray.backgroundColor isEqual:UIColor.clearColor])
        self.detailTray.backgroundColor = self.detailTrayPaint;
    self.detailTray = nil;
    self.detailTrayPaint = nil;
}

- (void)suppressPortraitGradient:(CAGradientLayer *)layer active:(NSMutableSet *)active transaction:(BOOL *)opened {
    // The bottom HUD gradient ends at the old viewport edge. The native
    // picture now covers that edge as well as the formerly reserved strip.
    if (!layer) return;
    [active addObject:layer];
    if (layer.opacity != 0) {
        if (opened && !*opened) { *opened=YES; [CATransaction begin]; [CATransaction setDisableActions:YES]; }
        if (!self.portraitGradients) self.portraitGradients=[NSMapTable weakToStrongObjectsMapTable];
        [self.portraitGradients setObject:@(layer.opacity) forKey:layer];
        layer.opacity=0;
    }
}

- (void)clearTopMatte {
    if (self.topMatteLayer.superlayer) {
        [CATransaction begin];
        [CATransaction setDisableActions:YES];
        [self.topMatteLayer removeFromSuperlayer];
        [CATransaction commit];
    }
    self.topMatteCell = nil;
}

- (void)applyTopMatteToCell:(UICollectionViewCell *)cell color:(UIColor *)color
                     height:(CGFloat)height active:(NSMutableSet *)active {
    if (!cell || !color || height <= 0.25 || HGVideoBackdropHasHostMask(cell)) {
        [self clearTopMatte];
        return;
    }
    CALayer *layer = self.topMatteLayer;
    if (!layer) {
        layer = [CALayer layer];
        layer.name = @"honggo.video.top-matte";
        layer.zPosition = -1;
        self.topMatteLayer = layer;
    }
    CGFloat overlap = 0.5;
    CGRect targetFrame = CGRectMake(0, CGRectGetHeight(cell.bounds) - overlap,
        CGRectGetWidth(cell.bounds), height + overlap);
    CGColorRef cgColor = color.CGColor;
    BOOL needsAttach = layer.superlayer != cell.layer;
    BOOL needsFrame = !CGRectEqualToRect(layer.frame, targetFrame);
    BOOL needsColor = !layer.backgroundColor || !CGColorEqualToColor(layer.backgroundColor, cgColor);
    BOOL needsUnclip = cell.clipsToBounds;
    [active addObject:cell];
    if (needsAttach || needsFrame || needsColor || needsUnclip) {
        [CATransaction begin];
        [CATransaction setDisableActions:YES];
        if (needsUnclip) {
            if (!self.portraitClips) self.portraitClips = [NSHashTable weakObjectsHashTable];
            [self.portraitClips addObject:cell];
            cell.clipsToBounds = NO;
        }
        if (needsAttach) [cell.layer insertSublayer:layer atIndex:0];
        if (needsFrame) layer.frame = targetFrame;
        if (needsColor) layer.backgroundColor = cgColor;
        [CATransaction commit];
    }
    self.topMatteCell = cell;
}

- (void)clearSeamMasksExcept:(NSSet<CALayer *> *)keep {
    if (!self.seamMaskedLayers.count) return;
    BOOL tx = NO;
    for (CALayer *layer in self.seamMaskedLayers.allObjects) {
        if ([keep containsObject:layer]) continue;
        if ([layer.mask.name isEqualToString:@"honggo.video.seam-mask"]) {
            if (!tx) { tx = YES; [CATransaction begin]; [CATransaction setDisableActions:YES]; }
            layer.mask = nil;
        }
        [self.seamMaskedLayers removeObject:layer];
    }
    if (tx) [CATransaction commit];
}

- (void)applySeamMaskToCanvas:(UIView *)canvas clipMinY:(CGFloat)clipMinY
                   clipHeight:(CGFloat)clipHeight keep:(NSMutableSet<CALayer *> *)keep {
    if (!canvas || clipHeight <= clipMinY) return;
    NSMutableArray<UIView *> *targets = [NSMutableArray arrayWithCapacity:2];
    UIView *player = HGVideoBackdropPlayer(canvas);
    if (player.superview && [player.superview isDescendantOfView:canvas] && !HGVideoBackdropHasHostMask(player.superview)) {
        [targets addObject:player.superview];
    }
    UIImageView *cellCover = HGVideoBackdropCover(canvas);
    if (cellCover && (!player.superview || ![cellCover isDescendantOfView:player.superview])) {
        UIView *coverTarget = cellCover;
        if (cellCover.superview && cellCover.superview != canvas &&
            [cellCover.superview isDescendantOfView:canvas] &&
            [self.portraitRenderers objectForKey:cellCover.superview]) {
            coverTarget = cellCover.superview;
        }
        if (!HGVideoBackdropHasHostMask(coverTarget)) {
            [targets addObject:coverTarget];
        }
    }
    CGFloat w = CGRectGetWidth(canvas.bounds);
    CGRect clipInCanvas = CGRectMake(-w, clipMinY, w * 3.0, clipHeight - clipMinY);
    CGColorRef opaque = UIColor.whiteColor.CGColor;
    BOOL tx = NO;
    for (UIView *target in targets) {
        CALayer *layer = target.layer;
        if (!layer) continue;
        [keep addObject:layer];
        CGRect maskFrame = [canvas convertRect:clipInCanvas toView:target];
        CALayer *mask = [layer.mask.name isEqualToString:@"honggo.video.seam-mask"] ? layer.mask : nil;
        if (!mask) {
            mask = [CALayer layer];
            mask.name = @"honggo.video.seam-mask";
            mask.backgroundColor = opaque;
        }
        BOOL needsColor = !mask.backgroundColor || !CGColorEqualToColor(mask.backgroundColor, opaque);
        if (layer.mask != mask || !CGRectEqualToRect(mask.frame, maskFrame) || needsColor) {
            if (!tx) { tx = YES; [CATransaction begin]; [CATransaction setDisableActions:YES]; }
            if (needsColor) mask.backgroundColor = opaque;
            mask.frame = maskFrame;
            if (layer.mask != mask) layer.mask = mask;
        }
        if (!self.seamMaskedLayers) self.seamMaskedLayers = [NSHashTable weakObjectsHashTable];
        [self.seamMaskedLayers addObject:layer];
    }
    if (tx) [CATransaction commit];
}

- (void)applyZeroMaskToView:(UIView *)view keep:(NSMutableSet<CALayer *> *)keep {
    CALayer *layer = view.layer;
    if (!layer || HGVideoBackdropHasHostMask(view)) return;
    [keep addObject:layer];
    CALayer *mask = [layer.mask.name isEqualToString:@"honggo.video.seam-mask"] ? layer.mask : nil;
    CGColorRef transparent = UIColor.clearColor.CGColor;
    if (!mask) {
        mask = [CALayer layer];
        mask.name = @"honggo.video.seam-mask";
        mask.backgroundColor = transparent;
    }
    CGRect hiddenFrame = CGRectMake(0, 0, 1, 1);
    BOOL needsColor = !mask.backgroundColor || !CGColorEqualToColor(mask.backgroundColor, transparent);
    if (layer.mask != mask || !CGRectEqualToRect(mask.frame, hiddenFrame) || needsColor) {
        [CATransaction begin];
        [CATransaction setDisableActions:YES];
        if (needsColor) mask.backgroundColor = transparent;
        mask.frame = hiddenFrame;
        if (layer.mask != mask) layer.mask = mask;
        [CATransaction commit];
    }
    if (!self.seamMaskedLayers) self.seamMaskedLayers = [NSHashTable weakObjectsHashTable];
    [self.seamMaskedLayers addObject:layer];
}

- (void)bringPortraitForward:(UICollectionViewCell *)cell {
    if (cell==self.frontPortraitCell && (!cell || cell.layer.zPosition==1)) return;
    [CATransaction begin]; [CATransaction setDisableActions:YES];
    if (self.frontPortraitCell.layer.zPosition==1) self.frontPortraitCell.layer.zPosition=0;
    self.frontPortraitCell=nil;
    if (cell && cell.layer.zPosition==0) {
        cell.layer.zPosition=1;
        self.frontPortraitCell=cell;
    }
    [CATransaction commit];
}

- (void)restoreCornerImages {
    for (UIImageView *image in self.cornerAlpha)
        if (image.alpha == 0) image.alpha = [[self.cornerAlpha objectForKey:image] doubleValue];
    [self.cornerAlpha removeAllObjects];
    self.lastCornerFrame = CGRectNull;
}

- (void)updateCornerImages:(NSArray<UIView *> *)siblings frame:(CGRect)frame {
    // The host mounts a pair of 12pt corner images as siblings of its feed
    // (FQVShortVideoListVCUIComponent setupCornerRadiusImageView). They stay
    // at the old bottom edge even when a live renderer or matching matte
    // now continues through the dock strip.
    if (self.cornerAlpha.count == 2 && !CGRectIsNull(self.lastCornerFrame) &&
        HGVideoBackdropSameRect(self.lastCornerFrame, frame, 0.25)) {
        UIView *expectedParent = siblings.firstObject.superview;
        BOOL intact = expectedParent != nil;
        for (UIImageView *cached in self.cornerAlpha) {
            if (cached.superview != expectedParent || cached.alpha != 0) { intact = NO; break; }
        }
        if (intact) return;
    }
    CGRect leftFrame = CGRectMake(CGRectGetMinX(frame), CGRectGetMaxY(frame)-12, 12, 12);
    CGRect rightFrame = CGRectOffset(leftFrame, CGRectGetWidth(frame)-12, 0);
    UIImageView *left = nil, *right = nil;
    for (UIView *view in siblings) {
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
    self.lastCornerFrame = frame;
}

- (void)restoreClipping {
    BOOL owned = self.ownsClipping;
    self.ownsClipping = NO;
    if (owned && !self.feed.clipsToBounds) self.feed.clipsToBounds = YES;
}

- (void)restorePortraitExcept:(NSSet *)active {
    if (!self.portraitRenderers.count && !self.portraitCovers.count &&
        !self.portraitClips.count && !self.portraitGradients.count) return;
    BOOL tx=NO;
    for (UIView *renderer in self.portraitRenderers.keyEnumerator.allObjects) {
        if ([active containsObject:renderer]) continue;
        CATransform3D applied=[[self.portraitRenderers objectForKey:renderer] CATransform3DValue];
        if (CATransform3DEqualToTransform(renderer.layer.sublayerTransform,applied)) {
            if (!tx) { tx=YES; [CATransaction begin]; [CATransaction setDisableActions:YES]; }
            renderer.layer.sublayerTransform=CATransform3DIdentity;
        }
        [self.portraitRenderers removeObjectForKey:renderer];
    }
    for (UIImageView *cover in self.portraitCovers.keyEnumerator.allObjects) {
        if ([active containsObject:cover]) continue;
        HGVideoPortraitCover saved;
        [[self.portraitCovers objectForKey:cover] getValue:&saved size:sizeof(saved)];
        if (CGRectEqualToRect(cover.frame,saved.appliedFrame)) {
            if (!tx) { tx=YES; [CATransaction begin]; [CATransaction setDisableActions:YES]; }
            cover.frame=saved.frame;
        }
        if (cover.contentMode==UIViewContentModeScaleAspectFill) {
            if (!tx) { tx=YES; [CATransaction begin]; [CATransaction setDisableActions:YES]; }
            cover.contentMode=saved.contentMode;
        }
        [self.portraitCovers removeObjectForKey:cover];
    }
    for (UIView *view in self.portraitClips.allObjects) {
        if ([active containsObject:view]) continue;
        if (!view.clipsToBounds) {
            if (!tx) { tx=YES; [CATransaction begin]; [CATransaction setDisableActions:YES]; }
            view.clipsToBounds=YES;
        }
        [self.portraitClips removeObject:view];
    }
    for (CAGradientLayer *layer in self.portraitGradients.keyEnumerator.allObjects) {
        if ([active containsObject:layer]) continue;
        if (layer.opacity==0) {
            if (!tx) { tx=YES; [CATransaction begin]; [CATransaction setDisableActions:YES]; }
            layer.opacity=[[self.portraitGradients objectForKey:layer] floatValue];
        }
        [self.portraitGradients removeObjectForKey:layer];
    }
    if (tx) [CATransaction commit];
}

- (void)releasePortraitClippingFrom:(UIView *)view to:(UIView *)boundary active:(NSMutableSet *)active transaction:(BOOL *)opened {
    for (UIView *node=view; node && node!=boundary; node=node.superview) {
        [active addObject:node];
        if (!node.clipsToBounds) continue;
        if (opened && !*opened) { *opened=YES; [CATransaction begin]; [CATransaction setDisableActions:YES]; }
        if (!self.portraitClips) self.portraitClips=[NSHashTable weakObjectsHashTable];
        [self.portraitClips addObject:node];
        node.clipsToBounds=NO;
    }
}

- (BOOL)fillPortraitInCanvas:(UIView *)canvas viewport:(CGRect)viewport full:(CGRect)full
                   boundary:(UIView *)boundary active:(NSMutableSet *)active {
    UIView *player=HGVideoBackdropPlayer(canvas);
    // The series cover belongs to the cell and can be visible before FQVPlayer
    // attaches. Both covers can be present during the native handoff.
    UIImageView *covers[]={HGVideoBackdropCover(canvas),HGVideoBackdropCover(player)};
    BOOL extends=CGRectGetMaxY(full)>CGRectGetMaxY(viewport)+0.75;
    if ((!player && !covers[0]) || CGRectIsEmpty(viewport) || CGRectIsEmpty(full)) {
        if (extends && !CGRectIsEmpty(viewport) && HGVideoBackdropPortraitModel(HGReadObjectGetter(canvas,"model"))) {
            BOOL earlyTx=NO;
            [self suppressPortraitGradient:HGVideoBackdropBottomGradient(canvas,viewport) active:active transaction:&earlyTx];
            if (earlyTx) [CATransaction commit];
        }
        return NO;
    }
    UIView *coordinateView=player ?: canvas;
    for (UIView *node=coordinateView; node && node!=boundary; node=node.superview)
        if (HGVideoBackdropHasHostMask(node) || !CATransform3DIsIdentity(node.layer.transform) ||
            !CATransform3DIsIdentity(node.layer.sublayerTransform)) return NO;
    static Class rendererCache;
    Class rendererClass=HGBackdropResolveClass("TTPlayerView",&rendererCache);
    BOOL transferring=NO;
    for (UIView *renderer in player.subviews) {
        if (![renderer isKindOfClass:rendererClass]) continue;
        if (HGAppearancePlayerIsTransferring(renderer)) {
            transferring=YES;
            continue;
        }
        // A decoded, visible landscape picture still owns its native crop.
        if (!renderer.hidden && renderer.alpha>0.01 && !player.hidden && player.alpha>0.01 &&
            !CGRectIsEmpty(renderer.frame) && renderer.bounds.size.width > renderer.bounds.size.height + 0.75 &&
            HGVideoBackdropFrameReady(player)) {
            static Class metalLandscapeCache;
            UIView *surface = renderer.subviews.firstObject;
            BOOL portraitMetal = [surface isKindOfClass:HGBackdropResolveClass("TTMetalView", &metalLandscapeCache)] &&
                !surface.hidden && surface.alpha > 0.01 &&
                surface.bounds.size.width > 0 && surface.bounds.size.height > surface.bounds.size.width;
            if (!portraitMetal) return NO;
        }
    }
    BOOL filled=NO, tx=NO;
    for (UIView *renderer in player.subviews) {
        if (![renderer isKindOfClass:rendererClass]) continue;
        CGRect native=[renderer convertRect:renderer.bounds toView:player];
        BOOL detailPortrait=boundary==canvas && HGDetailInsetPortrait(native,full);
        UIView *metal=HGVideoBackdropMetal(renderer,player,detailPortrait ? native : viewport);
        if (!metal) continue;
        CATransform3D current=renderer.layer.sublayerTransform;
        NSValue *previous=[self.portraitRenderers objectForKey:renderer];
        if (!CATransform3DIsIdentity(current) &&
            !(previous && CATransform3DEqualToTransform(current,previous.CATransform3DValue))) continue;
        CATransform3D transform=HGVideoBackdropCoverTransform(metal.bounds.size,native,full);
        if (!self.portraitRenderers) self.portraitRenderers=[NSMapTable weakToStrongObjectsMapTable];
        if (!previous || !CATransform3DEqualToTransform(previous.CATransform3DValue,transform))
            [self.portraitRenderers setObject:[NSValue valueWithCATransform3D:transform] forKey:renderer];
        if (!CATransform3DEqualToTransform(current,transform)) {
            if (!tx) { tx=YES; [CATransaction begin]; [CATransaction setDisableActions:YES]; }
            renderer.layer.sublayerTransform=transform;
        }
        [active addObject:renderer];
        if (extends || HGVideoBackdropCenteredPortrait(native,viewport) ||
            (detailPortrait && !HGVideoBackdropSameRect(native,full,0.75)))
            [self releasePortraitClippingFrom:renderer to:boundary active:active transaction:&tx];
        else if ([self.portraitClips containsObject:renderer]) {
            if (!tx) { tx=YES; [CATransaction begin]; [CATransaction setDisableActions:YES]; }
            renderer.clipsToBounds=YES;
            [self.portraitClips removeObject:renderer];
        }
        filled=YES;
    }

    for (NSUInteger i=0;i<2;i++) {
        UIImageView *cover=covers[i];
        // A revisited cell can retain its own poster while UIKit borrows the
        // renderer. Keep that picture continuous until the real frame returns.
        if (!cover || (i==1 && cover==covers[0]) || HGVideoBackdropHasHostMask(cover) ||
            (transferring && (!cover.image || !HGVideoBackdropCoverVisible(cover,canvas))) ||
            !CGAffineTransformIsIdentity(cover.transform) || !(cover.image ? HGVideoBackdropPortraitImage(cover) :
            (filled || HGVideoBackdropPortraitModel(HGReadObjectGetter(canvas,"model")) ||
             HGVideoBackdropPortraitModel(HGReadObjectGetter(player,"model"))))) continue;
        UIView *coverHost = cover.superview;
        BOOL useHostSublayer = coverHost && coverHost != canvas && coverHost != player &&
            coverHost.class == UIView.class && coverHost.subviews.count == 1 &&
            coverHost.layer.sublayers.count == 1 && coverHost.layer.sublayers.firstObject == cover.layer &&
            !HGVideoBackdropHasHostMask(coverHost) &&
            CGPointEqualToPoint(coverHost.layer.anchorPoint, CGPointMake(0.5, 0.5)) &&
            HGVideoBackdropSameRect(cover.frame, coverHost.bounds, 0.75);
        NSValue *old=[self.portraitCovers objectForKey:cover];
        HGVideoPortraitCover saved={cover.frame,CGRectZero,cover.contentMode};
        if (old) {
            [old getValue:&saved size:sizeof(saved)];
            if (!CGRectEqualToRect(cover.frame,saved.appliedFrame)) saved.frame=cover.frame;
            if (cover.contentMode!=UIViewContentModeScaleAspectFill) saved.contentMode=cover.contentMode;
        }
        CGRect nativeCover=[coverHost convertRect:saved.frame toView:coordinateView];
        if ((boundary==canvas && HGDetailInsetPortrait(nativeCover,full)) ||
            HGVideoBackdropMatchesViewport(nativeCover,viewport) || HGVideoBackdropCenteredPortrait(nativeCover,viewport) ||
            HGVideoBackdropMatchesViewport(nativeCover,coordinateView.bounds)) {
            if (useHostSublayer) {
                CATransform3D current = coverHost.layer.sublayerTransform;
                NSValue *previous = [self.portraitRenderers objectForKey:coverHost];
                if (CATransform3DIsIdentity(current) ||
                    (previous && CATransform3DEqualToTransform(current, previous.CATransform3DValue))) {
                    CATransform3D transform = HGVideoBackdropCoverTransform(nativeCover.size, nativeCover, full);
                    saved.appliedFrame = saved.frame;
                    if (!self.portraitCovers) self.portraitCovers = [NSMapTable weakToStrongObjectsMapTable];
                    BOOL updateSaved = !old;
                    if (old) {
                        HGVideoPortraitCover prev;
                        [old getValue:&prev size:sizeof(prev)];
                        updateSaved = !CGRectEqualToRect(prev.frame, saved.frame) ||
                            !CGRectEqualToRect(prev.appliedFrame, saved.appliedFrame) ||
                            prev.contentMode != saved.contentMode;
                    }
                    if (updateSaved)
                        [self.portraitCovers setObject:[NSValue valueWithBytes:&saved objCType:@encode(HGVideoPortraitCover)] forKey:cover];
                    if (cover.contentMode != UIViewContentModeScaleAspectFill) {
                        if (!tx) { tx = YES; [CATransaction begin]; [CATransaction setDisableActions:YES]; }
                        cover.contentMode = UIViewContentModeScaleAspectFill;
                    }
                    if (!self.portraitRenderers) self.portraitRenderers = [NSMapTable weakToStrongObjectsMapTable];
                    if (!previous || !CATransform3DEqualToTransform(previous.CATransform3DValue, transform))
                        [self.portraitRenderers setObject:[NSValue valueWithCATransform3D:transform] forKey:coverHost];
                    if (!CATransform3DEqualToTransform(current, transform)) {
                        if (!tx) { tx = YES; [CATransaction begin]; [CATransaction setDisableActions:YES]; }
                        coverHost.layer.sublayerTransform = transform;
                    }
                    [active addObject:cover];
                    [active addObject:coverHost];
                    if (extends) [self releasePortraitClippingFrom:coverHost to:boundary active:active transaction:&tx];
                    filled = YES;
                    continue;
                }
            }
            saved.appliedFrame=[coordinateView convertRect:full toView:cover.superview];
            if (!self.portraitCovers) self.portraitCovers=[NSMapTable weakToStrongObjectsMapTable];
            BOOL updateSaved = !old;
            if (old) {
                HGVideoPortraitCover prev;
                [old getValue:&prev size:sizeof(prev)];
                updateSaved = !CGRectEqualToRect(prev.frame,saved.frame) ||
                    !CGRectEqualToRect(prev.appliedFrame,saved.appliedFrame) ||
                    prev.contentMode!=saved.contentMode;
            }
            if (updateSaved)
                [self.portraitCovers setObject:[NSValue valueWithBytes:&saved objCType:@encode(HGVideoPortraitCover)] forKey:cover];
            if (cover.contentMode!=UIViewContentModeScaleAspectFill) {
                if (!tx) { tx=YES; [CATransaction begin]; [CATransaction setDisableActions:YES]; }
                cover.contentMode=UIViewContentModeScaleAspectFill;
            }
            if (!CGRectEqualToRect(cover.frame,saved.appliedFrame)) {
                if (!tx) { tx=YES; [CATransaction begin]; [CATransaction setDisableActions:YES]; }
                cover.frame=saved.appliedFrame;
            }
            [active addObject:cover];
            if (extends) [self releasePortraitClippingFrom:cover.superview to:boundary active:active transaction:&tx];
            filled=YES;
        }
    }
    if (filled || (extends && !HGVideoBackdropHasLetterbox(canvas) && HGVideoBackdropLooksPortrait(canvas)))
        [self suppressPortraitGradient:HGVideoBackdropBottomGradient(canvas,viewport) active:active transaction:&tx];
    if (tx) [CATransaction commit];
    return filled;
}

- (void)removeFeed {
    for (CALayer *layer in self.pagingMattes) [layer removeFromSuperlayer];
    self.pagingMattes=nil;
    UICollectionView *feed = self.feed;
    HGVideoBackdropLink *link = objc_getAssociatedObject(feed, &HGVideoBackdropLinkKey);
    if (link.owner == self)
        objc_setAssociatedObject(feed, &HGVideoBackdropLinkKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [self clearTopMatte];
    [self clearSeamMasksExcept:nil];
    [self restorePortraitExcept:nil];
    [self bringPortraitForward:nil];
    [self restoreHorizontalClip];
    [self restoreClipping];
    [self restoreCornerImages];
    [self restoreDetailTray];
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

- (void)restorePeers {
    for (HGVideoBackdrop *peer in self.peers) [peer restore];
    [self.peers removeAllObjects];
}

- (void)restore {
    if (self.refreshing) return;
    self.refreshing = YES;
    [self restorePeers];
    [self removeFeed];
    self.refreshing = NO;
}

// These two native pages carry different opaque palettes. Blend only their
// letterbox mattes as the boundary travels, leaving native media and HUD on top.
- (UIColor *)pagingMatteForCanvas:(UIView *)bottom frame:(CGRect)bottomFrame color:(UIColor *)color
                      topCanvas:(UIView *)top topFrame:(CGRect)topFrame {
    UICollectionView *feed=self.feed;
    if (!color || !HGVideoBackdropCrossesBottom(feed,bottomFrame) || !HGVideoBackdropHasLetterbox(bottom) ||
        fabs(CGRectGetMaxY(topFrame)-CGRectGetMinY(bottomFrame))>0.75) top=nil;
    UIColor *start=HGVideoBackdropMatte(top);
    if (!start || !HGVideoBackdropHasLetterbox(top) || [start isEqual:color]) {
        for (CALayer *layer in self.pagingMattes) [layer removeFromSuperlayer];
        self.pagingMattes=nil;
        return color;
    }
    CGFloat r,g,b,endR,endG,endB;
    [start getRed:&r green:&g blue:&b alpha:NULL];
    [color getRed:&endR green:&endG blue:&endB alpha:NULL];
    CGFloat fraction=MIN(1,MAX(0,(CGRectGetMaxY(feed.bounds)-CGRectGetMinY(bottomFrame))/feed.bounds.size.height));
    UIColor *blend=[UIColor colorWithRed:r+(endR-r)*fraction green:g+(endG-g)*fraction
                                    blue:b+(endB-b)*fraction alpha:1];
    if (!self.pagingMattes) self.pagingMattes=@[[CALayer layer],[CALayer layer]];
    [CATransaction begin]; [CATransaction setDisableActions:YES];
    NSUInteger index=0;
    for (UIView *canvas in @[top,bottom]) {
        CALayer *layer=self.pagingMattes[index++];
        if (layer.superlayer!=canvas.layer) [canvas.layer insertSublayer:layer atIndex:0];
        layer.zPosition=-1;
        layer.frame=canvas.bounds;
        layer.backgroundColor=blend.CGColor;
    }
    [CATransaction commit];
    return blend;
}

- (void)updateFeed {
    UICollectionView *feed = self.feed;
    if (feed.window && feed.window==self.root.window && HGVideoBackdropHasTransition(self.owner)) return;
    if (!HGVideoBackdropFeedGeometry(feed, self.owner, self.root, self.bar)) {
        [self removeFeed];
        return;
    }

    CGRect cellFrame=CGRectNull, topFrame=CGRectNull;
    UICollectionViewCell *cell=nil, *topCell=nil;
    HGVideoBackdropEdgeCells(feed,&topCell,&topFrame,&cell,&cellFrame);
    CGFloat edge=CGRectGetMaxY(feed.bounds)-0.75;
    UIView *canvas = HGVideoBackdropCanvas(cell,feed,edge);
    // Keep the page stride native. Both attached pages use a fixed full
    // picture size before, during and after a swipe.
    CGFloat extra=MAX(0,CGRectGetMaxY(feed.superview.bounds)-CGRectGetMaxY(feed.frame));
    UIView *topCanvas=topCell!=cell ? HGVideoBackdropCanvas(topCell,feed,CGRectGetMinY(feed.bounds)+0.75) : nil;
    NSMutableSet *active=[NSMutableSet set];
    CGRect full=canvas.bounds;
    full.size.height+=extra;
    BOOL portrait=cell && fabs(cellFrame.size.height-feed.bounds.size.height)<=0.75 &&
        [self fillPortraitInCanvas:canvas viewport:canvas.bounds full:full boundary:feed active:active];
    BOOL topPortrait=NO, keepUpperPicture=NO, clipTopSeam=NO, showTopMatte=NO;
    CGFloat topClipHeight=0, topMatteHeight=0;
    UIColor *topMatteColor=nil;
    if (topCell && topCell!=cell && fabs(topFrame.size.height-feed.bounds.size.height)<=0.75) {
        CGRect topFull=topCanvas.bounds;
        topFull.size.height+=extra;
        CGFloat advance=CGRectGetMinY(feed.bounds)-CGRectGetMinY(topFrame);
        topPortrait=[self fillPortraitInCanvas:topCanvas viewport:topCanvas.bounds full:topFull boundary:feed active:active];
        if (advance>0 && extra>0.75) {
            BOOL reachedEnd=!cell && CGRectGetMaxY(topFrame)>=feed.contentSize.height-0.75;
            if (topPortrait) {
                if (reachedEnd) {
                    keepUpperPicture=YES;
                } else if (advance<extra) {
                    CGFloat physicalBottom=CGRectGetMaxY(feed.bounds)+extra;
                    BOOL bottomHasTopPicture=cell && HGVideoBackdropFirstPictureY(canvas,feed)<=physicalBottom-extra+0.75;
                    // Use one symmetric continuous seam across [0, extra] for both
                    // forward and reverse scrolling so neither pops an extra-tall
                    // band of the lower video on drag start nor leaves a lingering
                    // residue behind the Dock during deceleration. Keep unmasked
                    // coverage only while a lower portrait has no first picture yet.
                    keepUpperPicture=YES;
                    if (!portrait || bottomHasTopPicture) {
                        clipTopSeam=YES;
                        topClipHeight=CGRectGetHeight(topCanvas.bounds)+HGVideoBackdropSeamExtension(advance,extra);
                    }
                } else if (cell) {
                    // Past the Dock zone, clip the outgoing portrait strictly at
                    // its own native bottom edge so its overflow can never bleed
                    // over a landscape or recycled lower cell regardless of subview order.
                    clipTopSeam=YES;
                    topClipHeight=CGRectGetHeight(topCanvas.bounds);
                }
            } else if (advance<extra && cell && !HGVideoBackdropLooksPortrait(topCanvas)) {
                // Extend a landscape top page's opaque matte smoothly across the
                // Dock transition zone [0, extra] so the lower video never jumps
                // into the Dock at 1px of drag or lingers during reverse settling.
                // Never apply this matte to a portrait cell whose renderer is mid-transfer.
                UIColor *candidateMatte=HGVideoBackdropMatte(topCanvas);
                if (candidateMatte && !HGVideoBackdropHasHostMask(topCell)) {
                    CGFloat ext=HGVideoBackdropSeamExtension(advance,extra);
                    if (ext>0.25) {
                        keepUpperPicture=YES;
                        showTopMatte=YES;
                        topMatteHeight=ext;
                        topMatteColor=candidateMatte;
                    }
                }
            }
        }
    }
    UIColor *matte=!portrait && extra>0.75 ? HGVideoBackdropMatte(canvas) : nil;
    UIColor *underlayColor=[self pagingMatteForCanvas:canvas frame:cellFrame color:matte topCanvas:topCanvas topFrame:topFrame];
    [self updateUnderlay:underlayColor];
    if (showTopMatte) {
        if (self.pagingMattes.count && underlayColor) topMatteColor=underlayColor;
        [self applyTopMatteToCell:topCell color:topMatteColor height:topMatteHeight active:active];
    } else {
        [self clearTopMatte];
    }
    [self restorePortraitExcept:active];

    BOOL overflows=extra>0.75 && (portrait || keepUpperPicture || (cell && HGVideoBackdropCrossesBottom(feed,cellFrame)));
    NSMutableSet<CALayer *> *keepSeamMasks=[NSMutableSet set];
    if (overflows) {
        if (clipTopSeam) {
            [self applySeamMaskToCanvas:topCanvas clipMinY:-extra clipHeight:topClipHeight keep:keepSeamMasks];
        }
        if (topCell && topCell!=cell && portrait &&
            HGVideoBackdropFirstPictureY(canvas,feed) < CGRectGetMinY(cellFrame)-0.5) {
            [self applySeamMaskToCanvas:canvas clipMinY:0 clipHeight:CGRectGetHeight(canvas.bounds)+extra*2.0 keep:keepSeamMasks];
        }
        // Hide realized off-viewport prefetched cells while feed.clipsToBounds is NO
        // so a third video (e.g. N+2 below a landscape N+1) can never show behind the Dock.
        for (UIView *child in feed.subviews) {
            if (child==topCell || child==cell || ![child isKindOfClass:UICollectionViewCell.class]) continue;
            [self applyZeroMaskToView:child keep:keepSeamMasks];
        }
    }
    [self clearSeamMasksExcept:keepSeamMasks];

    // A prefetched neighbor must not cover live pixels until its own picture
    // reaches the screen. This only orders siblings within the feed.
    [self bringPortraitForward:keepUpperPicture ? topCell : (overflows && portrait ? cell : nil)];
    if (overflows || matte) [self updateCornerImages:feed.superview.subviews frame:feed.frame];
    else [self restoreCornerImages];
    BOOL displacedHorizontally = self.root.window && !HGVideoBackdropFullWindow(feed.superview, self.root.window);
    if (!overflows) {
        [self applyHorizontalClip:NO];
        [self restoreClipping];
        return;
    }

    if (feed.clipsToBounds) {
        self.ownsClipping = YES;
        feed.clipsToBounds = NO;
    }
    [self applyHorizontalClip:displacedHorizontally || self.peers.count > 0];
}

- (void)prepareRendererTransfer:(UIView *)view {
    if (self.topMatteCell && (self.topMatteCell == view ||
        [self.topMatteCell isDescendantOfView:view] || [view isDescendantOfView:self.topMatteCell])) {
        UICollectionViewCell *matteCell = self.topMatteCell;
        [self clearTopMatte];
        if ([self.portraitClips containsObject:matteCell]) {
            if (!matteCell.clipsToBounds) matteCell.clipsToBounds = YES;
            [self.portraitClips removeObject:matteCell];
        }
    }
    if (self.seamMaskedLayers.count) {
        NSMutableSet<CALayer *> *keepDirect = [NSMutableSet set];
        BOOL clearedAny = NO;
        for (CALayer *layer in self.seamMaskedLayers.allObjects) {
            UIView *delegate = (id)layer.delegate;
            BOOL hit = [delegate isKindOfClass:UIView.class] &&
                (delegate == view || [delegate isDescendantOfView:view] || [view isDescendantOfView:delegate]);
            if (hit) clearedAny = YES;
            else [keepDirect addObject:layer];
        }
        if (clearedAny) [self clearSeamMasksExcept:keepDirect];
    }
    NSArray<UIView *> *renderers=self.portraitRenderers.keyEnumerator.allObjects;
    NSArray<UIImageView *> *covers=self.portraitCovers.keyEnumerator.allObjects;
    NSMutableSet *keep=[NSMutableSet set];
    UICollectionViewCell *keptCell=nil;
    BOOL moving=NO;
    for (UIView *surface in [(renderers ?: @[]) arrayByAddingObjectsFromArray:covers ?: @[]]) {
        if (surface==view || [surface isDescendantOfView:view] || [view isDescendantOfView:surface]) {
            moving=YES;
            continue;
        }
        UIView *canvas=HGDetailVideoBackdropCanvas(surface);
        UIView *boundary=self.feed ?: canvas;
        for (UIView *node=surface; node && node!=boundary; node=node.superview) {
            [keep addObject:node];
            if (!keptCell && [node isKindOfClass:UICollectionViewCell.class]) keptCell=(UICollectionViewCell *)node;
        }
        for (CAGradientLayer *layer in self.portraitGradients) {
            UIView *gradient=(id)layer.delegate;
            if ([gradient isKindOfClass:UIView.class] && [gradient isDescendantOfView:canvas]) [keep addObject:layer];
        }
    }
    if (!moving) return;
    if (keep.count && [view isKindOfClass:objc_lookUpClass("TTPlayerView")]) {
        // Mid-scroll player transfer between two portrait cells in the same feed:
        // keep the departing cell's bottom gradient suppressed while feed overflow
        // remains active so the 280pt HUD gradient never flashes on screen.
        UIView *departingCanvas = HGDetailVideoBackdropCanvas(view);
        for (CAGradientLayer *layer in self.portraitGradients) {
            UIView *gradient = (id)layer.delegate;
            if ([gradient isKindOfClass:UIView.class] && [gradient isDescendantOfView:departingCanvas])
                [keep addObject:layer];
        }
    }
    NSMutableSet<CALayer *> *keepMasks = [NSMutableSet set];
    for (CALayer *layer in self.seamMaskedLayers.allObjects) {
        UIView *delegate = (id)layer.delegate;
        if ([delegate isKindOfClass:UIView.class] && [keep containsObject:delegate]) [keepMasks addObject:layer];
    }
    [self clearSeamMasksExcept:keepMasks];
    [self restorePortraitExcept:keep];
    if (!keep.count) {
        [self restoreClipping];
        [self bringPortraitForward:nil];
        [self restoreCornerImages];
        [self restoreDetailTray];
    } else if (!self.frontPortraitCell || ![keep containsObject:self.frontPortraitCell]) {
        [self bringPortraitForward:keptCell];
    }
}

static char HGDetailOwnerTrayRectKey;

static UIView *HGDetailPlaybackHUD(UIView *canvas) {
    Class single=objc_lookUpClass("FQVShortVideoVPlayerSingleSeriesControlView");
    for (UIView *child in canvas.subviews)
        if ([child isKindOfClass:single] || HGIsAlbumDetailControl(child)) return child;
    return nil;
}

- (void)refreshDetail:(UIView *)canvas {
    if (self.refreshing) return;
    UIViewController *owner = HGVideoBackdropOwner(canvas);
    UIWindow *window = canvas.window;
    UIView *root=owner.viewIfLoaded;
    CGRect page=[canvas convertRect:canvas.bounds toView:root];
    // Both visible cells move vertically during a swipe. Validate the page
    // stride and its horizontal alignment, independently of its scroll offset.
    BOOL fullPage=window && root.window==window &&
        fabs(CGRectGetWidth(root.bounds)-CGRectGetWidth(window.bounds))<=0.75 &&
        fabs(CGRectGetHeight(root.bounds)-CGRectGetHeight(window.bounds))<=0.75 &&
        fabs(CGRectGetMinX(page)-CGRectGetMinX(root.bounds))<=0.75 &&
        fabs(CGRectGetWidth(page)-CGRectGetWidth(window.bounds))<=0.75 &&
        fabs(CGRectGetHeight(page)-CGRectGetHeight(window.bounds))<=0.75;
    if (!HGAppearanceEnabled() || !owner ||
        window.bounds.size.width >= window.bounds.size.height || !fullPage) {
        [self restore];
        return;
    }
    self.refreshing=YES;
    UIView *tray=nil, *corners=nil;
    UIView *hud=HGDetailPlaybackHUD(canvas);
    UIView *player=HGVideoBackdropPlayer(canvas);
    Class trayClass = NSClassFromString(@"FQVShortVideoPlayerBottomView");
    for (UIView *candidate in hud.subviews)
        if ([candidate isKindOfClass:trayClass]) { tray = candidate; break; }
    CGRect rawBottom = (tray && !CGRectIsEmpty(tray.bounds)) ? [tray convertRect:tray.bounds toView:canvas] : CGRectZero;
    BOOL alignedTray = rawBottom.size.height >= 44 && rawBottom.size.height <= 110 &&
        fabs(CGRectGetMinX(rawBottom) - CGRectGetMinX(canvas.bounds)) <= 0.75 &&
        fabs(CGRectGetWidth(rawBottom) - CGRectGetWidth(canvas.bounds)) <= 0.75 &&
        fabs(CGRectGetMaxY(rawBottom) - CGRectGetMaxY(canvas.bounds)) <= 0.75;
    CGRect bottom = alignedTray ? rawBottom : CGRectZero;
    // The inner multi-series/album HUD publishes this reservation through
    // bottomHeight without the single-series HUD's separate bottom tray.
    if (!alignedTray && HGIsAlbumDetailControl(hud)) {
        double height=0;
        if (HGVideoBackdropReadDimension(hud,sel_registerName("bottomHeight"),&height) && height>=44 && height<=110)
            bottom=CGRectMake(CGRectGetMinX(canvas.bounds),CGRectGetMaxY(canvas.bounds)-height,
                CGRectGetWidth(canvas.bounds),height);
    }
    BOOL measuredReservation=!CGRectIsEmpty(bottom);
    CGRect viewport = canvas.bounds;
    viewport.size.height -= bottom.size.height;
    id model=HGReadObjectGetter(canvas,"model");
    BOOL portrait=HGVideoBackdropPortraitModel(model);
    if(CGRectIsEmpty(bottom)) {
        // If TTPlayerView is already sized to the reserved shorter detail viewport
        // before the shared tray finishes AutoLayout, adopt its exact frame on frame 0
        // so the video never starts with an 89pt bottom black bar and jumps later.
        CGRect rendererViewport=CGRectZero;
        for (UIView *renderer in player.subviews) {
            if (![renderer isKindOfClass:objc_lookUpClass("TTPlayerView")] || CGRectIsEmpty(renderer.bounds)) continue;
            UIView *metal=renderer.subviews.firstObject;
            BOOL metalPortrait=[metal isKindOfClass:objc_lookUpClass("TTMetalView")] &&
                metal.bounds.size.width>0 && metal.bounds.size.height>metal.bounds.size.width;
            if (!portrait && !metalPortrait) continue;
            CGRect inCanvas=[renderer convertRect:renderer.bounds toView:canvas];
            CGFloat reserved=CGRectGetMaxY(canvas.bounds)-CGRectGetMaxY(inCanvas);
            if (fabs(CGRectGetMinX(inCanvas)-CGRectGetMinX(canvas.bounds))<=0.75 &&
                fabs(CGRectGetMinY(inCanvas)-CGRectGetMinY(canvas.bounds))<=0.75 &&
                fabs(CGRectGetWidth(inCanvas)-CGRectGetWidth(canvas.bounds))<=0.75 &&
                reserved>=44 && reserved<=110) {
                rendererViewport=inCanvas;
                break;
            }
        }
        if(!CGRectIsEmpty(rendererViewport)) {
            viewport=rendererViewport;
            bottom=CGRectMake(CGRectGetMinX(viewport),CGRectGetMaxY(viewport),viewport.size.width,
                CGRectGetMaxY(canvas.bounds)-CGRectGetMaxY(viewport));
        } else if(portrait || HGVideoBackdropPortraitImage(HGVideoBackdropCover(canvas)) ||
                  HGVideoBackdropPortraitImage(HGVideoBackdropCover(player))) {
            // Before the tray is laid out or before the shared single-series HUD
            // transfers to an incoming cell, use the published/cached tray viewport.
            CGRect predicted=CGRectZero;
            SEL frame=sel_registerName("playerViewFrame");
            Method method=class_getInstanceMethod(object_getClass(hud),frame);
            if(method && !strcmp(method_getTypeEncoding(method),"{CGRect={CGPoint=dd}{CGSize=dd}}16@0:8"))
                predicted=((CGRect(*)(id,SEL))objc_msgSend)(hud,frame);
            if(CGRectIsEmpty(predicted)) {
                id component=HGReadObjectGetter(HGReadObjectGetter(owner,"listPlayer"),"playerComponent");
                frame=sel_registerName("videoPlayerViewFrameWithModel:");
                method=class_getInstanceMethod(object_getClass(component),frame);
                if(method && !strcmp(method_getTypeEncoding(method),"{CGRect={CGPoint=dd}{CGSize=dd}}24@0:8@16"))
                    predicted=((CGRect(*)(id,SEL,id))objc_msgSend)(component,frame,model);
            }
            if(!CGRectIsEmpty(predicted) && fabs(CGRectGetMinY(predicted)-CGRectGetMinY(canvas.bounds))<=0.75 &&
               fabs(CGRectGetMinX(predicted)-CGRectGetMinX(canvas.bounds))<=0.75 &&
               fabs(CGRectGetWidth(predicted)-CGRectGetWidth(canvas.bounds))<=0.75) {
                viewport=predicted;
                bottom=CGRectMake(CGRectGetMinX(viewport),CGRectGetMaxY(viewport),viewport.size.width,
                    CGRectGetMaxY(canvas.bounds)-CGRectGetMaxY(viewport));
            } else {
                NSValue *cached=objc_getAssociatedObject(owner,&HGDetailOwnerTrayRectKey);
                CGRect cachedBottom=cached ? cached.CGRectValue : CGRectZero;
                if(cachedBottom.size.height>=44 && cachedBottom.size.height<=110 &&
                   fabs(CGRectGetWidth(cachedBottom)-CGRectGetWidth(canvas.bounds))<=0.75 &&
                   fabs(CGRectGetMaxY(cachedBottom)-CGRectGetMaxY(canvas.bounds))<=0.75) {
                    bottom=CGRectMake(CGRectGetMinX(canvas.bounds),CGRectGetMaxY(canvas.bounds)-cachedBottom.size.height,
                        CGRectGetWidth(canvas.bounds),cachedBottom.size.height);
                    viewport=canvas.bounds;
                    viewport.size.height-=bottom.size.height;
                }
            }
        }
    }
    NSMutableSet *active=[NSMutableSet set];
    BOOL validBottom=bottom.size.height>=44 && bottom.size.height<=110 &&
        HGVideoBackdropSameRect(bottom,CGRectMake(CGRectGetMinX(viewport),CGRectGetMaxY(viewport),
            viewport.size.width,bottom.size.height),0.75);
    if(measuredReservation && validBottom) {
        NSValue *cached=objc_getAssociatedObject(owner,&HGDetailOwnerTrayRectKey);
        if(!cached || !CGRectEqualToRect(cached.CGRectValue,bottom))
            objc_setAssociatedObject(owner,&HGDetailOwnerTrayRectKey,[NSValue valueWithCGRect:bottom],OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    BOOL knownDetail=hud || HGVideoBackdropSingleList(owner) || objc_getAssociatedObject(owner,&HGDetailOwnerTrayRectKey)!=nil;
    BOOL fills=knownDetail && (validBottom || CGRectIsEmpty(bottom)) &&
        [self fillPortraitInCanvas:canvas viewport:viewport full:canvas.bounds boundary:canvas active:active];
    [self restorePortraitExcept:active];
    if (fills) {
        // The portrait detail player places its lower 12pt corner cutouts in
        // a separate holder at the original renderer edge.
        for (UIView *child in player.subviews) {
            if (child.class != UIView.class || child.subviews.count < 4 ||
                !HGVideoBackdropSameRect(child.frame,viewport,0.75)) continue;
            if (corners) { corners = nil; break; }
            corners = child;
        }
        [self updateCornerImages:corners.subviews frame:corners.bounds];
        if (self.detailTray!=tray || !alignedTray) [self restoreDetailTray];
        UIColor *paint = alignedTray ? tray.backgroundColor : nil;
        if (paint && ![paint isEqual:UIColor.clearColor]) {
            self.detailTray = tray;
            self.detailTrayPaint = paint;
            tray.backgroundColor = UIColor.clearColor;
        }
    } else {
        [self restoreCornerImages];
        [self restoreDetailTray];
    }
    self.refreshing = NO;
}

- (void)attachFeed:(UICollectionView *)candidate owner:(UIViewController *)owner
              root:(UIView *)root bar:(UIView *)bar {
    if (self.feed != candidate) {
        [self removeFeed];
        HGVideoBackdropLink *link = objc_getAssociatedObject(candidate, &HGVideoBackdropLinkKey);
        if (link.owner) return;
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
}

- (void)refreshSingleFeed {
    if (self.refreshing) return;
    self.refreshing = YES;
    [self updateFeed];
    self.refreshing = NO;
}

- (void)refreshFeed {
    if (self.refreshing) return;
    self.refreshing = YES;
    [self updateFeed];
    for (HGVideoBackdrop *peer in self.peers.copy) [peer refreshFeed];
    self.refreshing = NO;
}

- (void)refreshWithBranches:(NSArray<UIView *> *)branches root:(UIView *)root
                       bar:(UIView *)bar active:(BOOL)active {
    if (self.refreshing) return;
    if (!active || !root.window || bar.window!=root.window) {
        [self restore];
        return;
    }
    if (self.feed.window==root.window && HGVideoBackdropHasTransition(self.owner)) return;

    Class feedClass = NSClassFromString(@"FQVCollectionView");
    NSMutableArray<UICollectionView *> *candidates = [NSMutableArray array];
    NSMutableArray<UIViewController *> *owners = [NSMutableArray array];
    NSMutableArray<UIView *> *queue = [branches mutableCopy];
    for (NSUInteger i = 0; feedClass && i < queue.count && i < 192; i++) {
        UIView *view = queue[i];
        if (!HGVideoBackdropVisible(view, root) ||
            CGRectIsEmpty(CGRectIntersection([view convertRect:view.bounds toView:root], root.bounds))) continue;
        if ([view isKindOfClass:feedClass] && [view isKindOfClass:UICollectionView.class]) {
            UICollectionView *feed = (UICollectionView *)view;
            UIViewController *controller = HGVideoBackdropOwner(feed);
            if (!HGVideoBackdropFeedGeometry(feed, controller, root, bar)) continue;
            if (![candidates containsObject:feed]) {
                [candidates addObject:feed];
                [owners addObject:controller];
            }
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
    if (!candidates.count) {
        [self restore];
        return;
    }
    if (candidates.count > 1) {
        UIView *commonPager = HGVideoBackdropCategoryPager(candidates[0].superview, root.window);
        if (!commonPager) {
            [self restore];
            return;
        }
        for (NSUInteger idx = 1; idx < candidates.count; idx++) {
            if (HGVideoBackdropCategoryPager(candidates[idx].superview, root.window) != commonPager) {
                [self restore];
                return;
            }
        }
    }

    self.refreshing = YES;
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    @try {
        NSUInteger primaryIndex = self.feed ? [candidates indexOfObjectIdenticalTo:self.feed] : NSNotFound;
        if (primaryIndex == NSNotFound) primaryIndex = 0;
        UICollectionView *primaryFeed = candidates[primaryIndex];
        UIViewController *primaryOwner = owners[primaryIndex];

        // Release any peer whose feed is no longer visible or is being promoted to primary.
        NSMutableArray<HGVideoBackdrop *> *keptPeers = [NSMutableArray array];
        for (HGVideoBackdrop *peer in self.peers) {
            if (peer.feed && peer.feed != primaryFeed && [candidates containsObject:peer.feed]) {
                [keptPeers addObject:peer];
            } else {
                [peer restore];
            }
        }
        self.peers = keptPeers;

        for (NSUInteger idx = 0; idx < candidates.count; idx++) {
            if (idx == primaryIndex) continue;
            UICollectionView *peerFeed = candidates[idx];
            UIViewController *peerOwner = owners[idx];
            HGVideoBackdrop *peer = nil;
            for (HGVideoBackdrop *existing in self.peers) {
                if (existing.feed == peerFeed) { peer = existing; break; }
            }
            if (!peer) {
                peer = [HGVideoBackdrop new];
                [self.peers addObject:peer];
            }
            peer.refreshing = YES;
            @try { [peer attachFeed:peerFeed owner:peerOwner root:root bar:bar]; }
            @finally { peer.refreshing = NO; }
        }
        [self attachFeed:primaryFeed owner:primaryOwner root:root bar:bar];
    } @finally {
        [CATransaction commit];
        self.refreshing = NO;
    }
}
@end

BOOL HGVideoBackdropRefreshForFeed(UIScrollView *feed) {
    if (HGFeedSidebarOwnsLayout(feed.window)) return NO;
    HGVideoBackdropLink *link = objc_getAssociatedObject(feed, &HGVideoBackdropLinkKey);
    HGVideoBackdrop *owner = link.owner;
    if (!owner) return NO;
    if (HGAppearanceEnabled()) [owner refreshSingleFeed];
    else [owner restore];
    return owner.feed == feed;
}

static UIScrollView *HGVideoBackdropPlayerFeed(UIView *view) {
    static Class feedCache;
    Class feedClass = HGBackdropResolveClass("FQVCollectionView", &feedCache);
    for (UIView *node = view.superview; node; node = node.superview)
        if ([node isKindOfClass:feedClass] && [node isKindOfClass:UICollectionView.class])
            return (UIScrollView *)node;
    return nil;
}

static char HGDetailVideoBackdropKey;
static UIView *HGDetailVideoBackdropCanvas(UIView *view) {
    static Class canvasCache;
    Class canvasClass = HGBackdropResolveClass("FQVShortVideoPlayerCellView", &canvasCache);
    for (UIView *node = view; node; node = node.superview)
        if ([node isKindOfClass:canvasClass]) return node;
    return nil;
}

void HGRefreshVideoBackdropForPlayer(UIView *view) {
    if (!NSThread.isMainThread) return;
    UIScrollView *feed = HGVideoBackdropPlayerFeed(view);
    UIView *canvas = HGDetailVideoBackdropCanvas(view);
    HGVideoBackdrop *detail = objc_getAssociatedObject(canvas, &HGDetailVideoBackdropKey);
    HGVideoBackdropLink *feedLink=objc_getAssociatedObject(feed,&HGVideoBackdropLinkKey);
    if (feedLink ||
        (feed.window && CGRectGetMaxY(feed.frame) < CGRectGetMaxY(feed.superview.bounds)-20)) {
        [detail restore];
        static Class rendererCache, playerCache, gradientCache, controlCache, outterControlCache;
        BOOL immediate = (view == canvas && HGVideoBackdropLooksPortrait(canvas)) ||
            [view isKindOfClass:HGBackdropResolveClass("TTPlayerView", &rendererCache)] ||
            [view isKindOfClass:HGBackdropResolveClass("FQVPlayer", &playerCache)] ||
            [view isKindOfClass:HGBackdropResolveClass("FQVShortVideoGradientLayerView", &gradientCache)] ||
            [view isKindOfClass:HGBackdropResolveClass("FQVShortVideoVPlayerControlView", &controlCache)] ||
            [view isKindOfClass:HGBackdropResolveClass("FQVShortVideoVPlayerOutterMultiSeriesControlView", &outterControlCache)];
        if (!HGAppearancePlayerIsTransferring(view) && immediate)
            HGRefreshVideoBackdropForFeed(feed);
        HGScheduleFeedLayoutRefresh(feed);
        return;
    }
    if (!canvas) return;
    if (!detail) {
        UIViewController *owner = HGVideoBackdropOwner(canvas);
        if (!HGAppearanceEnabled() ||
            (!HGDetailPlaybackHUD(canvas) && !HGVideoBackdropSingleList(owner) &&
             !objc_getAssociatedObject(owner,&HGDetailOwnerTrayRectKey))) return;
        detail = [HGVideoBackdrop new];
        objc_setAssociatedObject(canvas, &HGDetailVideoBackdropKey, detail, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    [detail refreshDetail:canvas];
}

void HGVideoBackdropPrepareRendererTransfer(UIView *view) {
    if (!NSThread.isMainThread) return;
    HGVideoBackdrop *detail = objc_getAssociatedObject(HGDetailVideoBackdropCanvas(view), &HGDetailVideoBackdropKey);
    [detail prepareRendererTransfer:view];
    UIScrollView *feed = HGVideoBackdropPlayerFeed(view);
    if (!feed) return;
    HGVideoBackdropLink *link = objc_getAssociatedObject(feed, &HGVideoBackdropLinkKey);
    [link.owner prepareRendererTransfer:view];
}
