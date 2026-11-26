// Diagnostic-only: no frame, inset, alpha, mask or gesture mutations.
static __weak UIViewController *HGScenePage;
static char HGSceneIDKey;
static char HGAutomaticSceneTimesKey;
static NSUInteger HGSceneNextID;
static NSNumber *HGSceneID(id object) {
    if (!object) return @0;
    NSNumber *value = objc_getAssociatedObject(object, &HGSceneIDKey);
    if (!value) {
        value = @(++HGSceneNextID);
        objc_setAssociatedObject(object, &HGSceneIDKey, value, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    return value;
}
static NSArray *HGSceneRect(CGRect r) {
    if (!isfinite(r.origin.x) || !isfinite(r.origin.y) || !isfinite(r.size.width) || !isfinite(r.size.height)) return @[];
    return @[@(r.origin.x), @(r.origin.y), @(r.size.width), @(r.size.height)];
}
static void HGCaptureMask(UIViewController *page, NSString *reason, BOOL cancelled) {
    if (!HGEnhancedDiagnostics() || !page.viewIfLoaded.window) return;
    NSTimeInterval started = NSProcessInfo.processInfo.systemUptime;
    BOOL detailed = [reason isEqual:@"user.screenshot"] || cancelled;
    NSUInteger nodeLimit = detailed ? 192 : 80;
    NSTimeInterval timeBudget = detailed ? 0.012 : 0.003;
    if (!detailed) {
        NSMutableDictionary<NSString *, NSNumber *> *last = objc_getAssociatedObject(page, &HGAutomaticSceneTimesKey);
        NSNumber *previous = last[reason];
        if (previous && started - previous.doubleValue < 3) return;
        if (!last) {
            last = [NSMutableDictionary dictionary];
            objc_setAssociatedObject(page, &HGAutomaticSceneTimesKey, last, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        last[reason] = @(started);
    }
    UIView *root = page.viewIfLoaded;
    UIWindow *window = root.window;
    NSMutableArray<UIView *> *queue = [NSMutableArray array];
    NSHashTable *seen = [NSHashTable weakObjectsHashTable];
    void (^add)(UIView *) = ^(UIView *v) {
        if (v && ![seen containsObject:v] && queue.count < nodeLimit) { [seen addObject:v]; [queue addObject:v]; }
    };
    NSMutableDictionary *roles = [NSMutableDictionary dictionary];
    // Seed ownership paths before cells can consume the node budget.
    roles[@"page"] = HGSceneID(root);
    for (NSString *name in @[@"scrollView", @"bottomView", @"bgView", @"maskView"]) {
        UIView *v = HGView(page, name.UTF8String);
        roles[name] = HGSceneID(v);
        for (UIView *a = v; a; a = a.superview) add(a);
    }
    id handle = HGObject(page, "minePageKMPHandle");
    UIView *currentList = HGView(HGView(page, "bottomView"), "currentVCScrollView");
    roles[@"currentList"] = HGSceneID(currentList);
    for (UIView *a = currentList; a; a = a.superview) add(a);
    for (NSString *name in @[@"contentView", @"backgroundView", @"titleBarView"]) {
        UIView *v = HGView(handle, name.UTF8String);
        roles[[@"compose." stringByAppendingString:name]] = HGSceneID(v);
        for (UIView *a = v; a; a = a.superview) add(a);
    }
    for (UIView *a = root; a; a = a.superview) add(a);
    for (UIViewController *owner = page; owner; owner = owner.parentViewController) {
        UIView *bar = HGView(owner, "customTabBar");
        if (bar) { roles[@"tabBar"] = HGSceneID(bar); add(bar); }
    }
    NSMutableArray *nodes = [NSMutableArray array];
    BOOL timeLimited = NO;
    for (NSUInteger i = 0; i < queue.count; i++) {
        if (NSProcessInfo.processInfo.systemUptime - started > timeBudget) { timeLimited = YES; break; }
        UIView *v = queue[i];
        NSMutableDictionary *n = [@{@"id": HGSceneID(v), @"parent": HGSceneID(v.superview),
            @"class": NSStringFromClass(v.class), @"frame": HGSceneRect(v.frame), @"bounds": HGSceneRect(v.bounds),
            @"windowRect": HGSceneRect([v convertRect:v.bounds toView:window]),
            @"hidden": @(v.hidden), @"alpha": @(v.alpha), @"opaque": @(v.opaque),
            @"clips": @(v.clipsToBounds), @"layerClips": @(v.layer.masksToBounds),
            @"backgroundAlpha": @(v.backgroundColor ? CGColorGetAlpha(v.backgroundColor.CGColor) : 0),
            @"layerBackgroundAlpha": @(v.layer.backgroundColor ? CGColorGetAlpha(v.layer.backgroundColor) : 0),
            @"layerOpacity": @(v.layer.opacity), @"transform": NSStringFromCGAffineTransform(v.transform),
            @"mask": HGSceneID(v.layer.mask)} mutableCopy];
        if (v.layer.mask) {
            n[@"maskFrame"] = HGSceneRect(v.layer.mask.frame);
            n[@"maskBounds"] = HGSceneRect(v.layer.mask.bounds);
            if ([v.layer.mask isKindOfClass:CAShapeLayer.class] && ((CAShapeLayer *)v.layer.mask).path)
                n[@"maskPathBounds"] = HGSceneRect(CGPathGetBoundingBox(((CAShapeLayer *)v.layer.mask).path));
        }
        NSMutableArray *layers = [NSMutableArray array];
        for (CALayer *layer in v.layer.sublayers) {
            if (layers.count == 4) break;
            [layers addObject:@{@"id": HGSceneID(layer), @"class": NSStringFromClass(layer.class),
                @"frame": HGSceneRect(layer.frame), @"clips": @(layer.masksToBounds),
                @"hidden": @(layer.hidden), @"opacity": @(layer.opacity), @"mask": HGSceneID(layer.mask),
                @"backgroundAlpha": @(layer.backgroundColor ? CGColorGetAlpha(layer.backgroundColor) : 0)}];
        }
        n[@"layers"] = layers;
        n[@"layerCount"] = @(v.layer.sublayers.count);
        CALayer *presentation = v.layer.presentationLayer, *wp = window.layer.presentationLayer;
        if (presentation && wp) n[@"presentationWindowRect"] = HGSceneRect([presentation convertRect:presentation.bounds toLayer:wp]);
        if ([v isKindOfClass:UIScrollView.class]) {
            UIScrollView *s = (UIScrollView *)v;
            n[@"offset"] = NSStringFromCGPoint(s.contentOffset);
            n[@"contentSize"] = NSStringFromCGSize(s.contentSize);
            n[@"inset"] = NSStringFromUIEdgeInsets(s.contentInset);
            n[@"adjustedInset"] = NSStringFromUIEdgeInsets(s.adjustedContentInset);
            n[@"dragging"] = @(s.dragging); n[@"decelerating"] = @(s.decelerating);
            for (NSString *key in @[@"topEdgeEffect", @"bottomEdgeEffect"]) {
                id edge = HGObject(s, key.UTF8String);
                if ([edge respondsToSelector:sel_registerName("isHidden")])
                    n[key] = @(((BOOL (*)(id, SEL))objc_msgSend)(edge, sel_registerName("isHidden")));
            }
        }
        BOOL effect = [v isKindOfClass:UIVisualEffectView.class];
        if (effect) n[@"effect"] = NSStringFromClass(((UIVisualEffectView *)v).effect.class) ?: @"none";
        [nodes addObject:n];
        // Preserve structural nodes, never read labels, image bytes or cell contents.
        if (!effect && ![v isKindOfClass:UITableViewCell.class] && ![v isKindOfClass:UICollectionViewCell.class] &&
            ![v isKindOfClass:UILabel.class] && ![v isKindOfClass:UIImageView.class])
            for (UIView *child in v.subviews) add(child);
    }
    HGStoreMaskScene(@{@"reason": reason, @"time": @(NSDate.date.timeIntervalSince1970),
        @"uptime": @(started), @"captureMs": @((NSProcessInfo.processInfo.systemUptime - started) * 1000),
        @"cancelled": @(cancelled), @"roles": roles, @"nodes": nodes,
        @"window": HGSceneID(window), @"windowBounds": HGSceneRect(window.bounds),
        @"nodeLimit": @(nodeLimit), @"truncated": @(timeLimited || queue.count >= nodeLimit), @"timeLimited": @(timeLimited)});
}
static void HGSceneSettled(UIViewController *page, NSString *reason) {
    __weak UIViewController *weak = page;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.5 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        HGCaptureMask(weak, reason, NO);
    });
}
@interface HGMaskSceneObserver : NSObject
@end
@implementation HGMaskSceneObserver
- (void)screenshot:(NSNotification *)note { HGCaptureMask(HGScenePage, @"user.screenshot", NO); }
@end
static void HGInstallMaskScenes(void) {
    Class mine = objc_lookUpClass("SSMine737KMPViewController");
    if (!mine) return;
    for (NSString *name in @[@"viewWillAppear:", @"viewDidAppear:", @"viewWillDisappear:"]) {
        SEL sel = NSSelectorFromString(name);
        Method m = class_getInstanceMethod(mine, sel);
        if (!m || strcmp(method_getTypeEncoding(m), "v20@0:8B16")) continue;
        void (*original)(id, SEL, BOOL) = (void *)method_getImplementation(m);
        IMP hook = imp_implementationWithBlock(^(UIViewController *page, BOOL animated) {
            HGScenePage = page;
            HGLog(@"page.lifecycle", @{@"class": NSStringFromClass(page.class), @"selector": name, @"phase": @"enter"});
            if ([name isEqual:@"viewWillDisappear:"]) HGCaptureMask(page, @"before.leave", NO);
            original(page, sel, animated);
            HGLog(@"page.lifecycle", @{@"class": NSStringFromClass(page.class), @"selector": name, @"phase": @"return"});
            if ([name isEqual:@"viewWillAppear:"]) {
                HGCaptureMask(page, @"will.appear", NO);
                id<UIViewControllerTransitionCoordinator> transition = page.transitionCoordinator;
                BOOL registered = [transition animateAlongsideTransition:nil completion:^(id<UIViewControllerTransitionCoordinatorContext> context) {
                    HGCaptureMask(page, @"transition.complete", context.isCancelled);
                    HGSceneSettled(page, @"transition.settled");
                }];
                HGLog(@"mask.transition", @{@"completionRegistered": @(registered)});
            } else if ([name isEqual:@"viewDidAppear:"]) {
                HGCaptureMask(page, @"did.appear", NO);
                HGSceneSettled(page, @"appear.settled");
            }
        });
        HGReplaceMethod(mine, sel, "v20@0:8B16", hook);
    }
    static HGMaskSceneObserver *observer;
    observer = [HGMaskSceneObserver new];
    [NSNotificationCenter.defaultCenter addObserver:observer selector:@selector(screenshot:)
        name:UIApplicationUserDidTakeScreenshotNotification object:nil];
}
