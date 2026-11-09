// Follow only the current list's ancestor chain, never scan cells or resize
// host content. Both expanded and pinned headers must reveal the glass area.
@interface HGMineClipRecord : NSObject
@property(nonatomic) BOOL clips;
@property(nonatomic, strong) CAShapeLayer *mask;
@property(nonatomic, strong) id top;
@property(nonatomic, strong) id bottom;
@property(nonatomic) BOOL topHidden;
@property(nonatomic) BOOL bottomHidden;
- (void)restore:(UIView *)view;
- (void)apply:(UIView *)view height:(CGFloat)height;
@end
@implementation HGMineClipRecord
- (void)restore:(UIView *)view {
    if (view.layer.mask == self.mask) view.layer.mask = nil;
    if (!view.clipsToBounds) view.clipsToBounds = self.clips;
    if (self.top) ((void (*)(id, SEL, BOOL))objc_msgSend)(self.top, @selector(setHidden:), self.topHidden);
    if (self.bottom) ((void (*)(id, SEL, BOOL))objc_msgSend)(self.bottom, @selector(setHidden:), self.bottomHidden);
}
- (void)apply:(UIView *)view height:(CGFloat)height {
    if (!self.mask) {
        self.clips = view.clipsToBounds;
        self.mask = [CAShapeLayer layer]; self.mask.fillColor = UIColor.blackColor.CGColor;
        for (NSString *name in @[@"topEdgeEffect", @"bottomEdgeEffect"]) {
            id edge = HGObject(view, name.UTF8String);
            if (![edge respondsToSelector:@selector(setHidden:)] || ![edge respondsToSelector:sel_registerName("isHidden")]) continue;
            BOOL hidden = ((BOOL (*)(id, SEL))objc_msgSend)(edge, sel_registerName("isHidden"));
            if ([name isEqual:@"topEdgeEffect"]) { self.top = edge; self.topHidden = hidden; }
            else { self.bottom = edge; self.bottomHidden = hidden; }
        }
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
    view.clipsToBounds = NO; view.layer.mask = self.mask;
    // Preserve the inner list's own top boundary; only the outer coordinator
    // had a redundant top edge effect behind the custom header.
    if (self.top && [view isKindOfClass:objc_lookUpClass("SSMyUser637NestedContainerScrollView")])
        ((void (*)(id, SEL, BOOL))objc_msgSend)(self.top, @selector(setHidden:), YES);
    if (self.bottom) ((void (*)(id, SEL, BOOL))objc_msgSend)(self.bottom, @selector(setHidden:), YES);
}
@end
static char HGMineClipKey, HGMineClipPendingKey;
static void HGApplyMineClip(UIScrollView *scroll) {
    if (!scroll) return;
    NSMapTable<UIView *, HGMineClipRecord *> *records = objc_getAssociatedObject(scroll, &HGMineClipKey);
    UIView *root = scroll.superview;
    Class rootClass = objc_lookUpClass("SSMine737KMPRootView");
    BOOL eligible = HGGlassEnabled && scroll.window && rootClass && [root isKindOfClass:rootClass] &&
        CGAffineTransformIsIdentity(scroll.transform) && HGNear(scroll.frame.origin.x, 0) &&
        HGNear(scroll.frame.origin.y, 0) && HGNear(scroll.bounds.size.width, root.bounds.size.width) &&
        root.bounds.size.height >= scroll.bounds.size.height &&
        root.bounds.size.height - scroll.bounds.size.height <= 160;
    NSMutableArray<UIView *> *path = [NSMutableArray array];
    if (eligible) {
        [path addObject:scroll];
        UIResponder *owner = root.nextResponder;
        while (owner && ![owner isKindOfClass:UIViewController.class]) owner = owner.nextResponder;
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
        [active addObject:view]; [record apply:view height:height];
    }
    for (UIView *view in records.keyEnumerator.allObjects) {
        if (![active containsObject:view]) { [[records objectForKey:view] restore:view]; [records removeObjectForKey:view]; }
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
    // Host coordination may move the pinned header without laying out the
    // outer view. Coalesce a bounded visual-only update after its delegate.
    Class mine = objc_lookUpClass("SSMine737KMPViewController");
    SEL sel = sel_registerName("scrollViewDidScroll:");
    Method m = class_getInstanceMethod(mine, sel);
    if (m && !strcmp(method_getTypeEncoding(m), "v24@0:8@16")) {
        void (*original)(id, SEL, id) = (void *)method_getImplementation(m);
        HGReplaceMethod(mine, sel, "v24@0:8@16", imp_implementationWithBlock(^(UIViewController *page, id sender) {
            original(page, sel, sender);
            if (objc_getAssociatedObject(page, &HGMineClipPendingKey)) return;
            objc_setAssociatedObject(page, &HGMineClipPendingKey, @YES, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            __weak UIViewController *weak = page;
            dispatch_async(dispatch_get_main_queue(), ^{
                UIViewController *current = weak;
                if (!current) return;
                HGApplyMineClip((UIScrollView *)HGView(current, "scrollView"));
                objc_setAssociatedObject(current, &HGMineClipPendingKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            });
        }));
    }
}
