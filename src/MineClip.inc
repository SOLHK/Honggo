// Device scene: Mine's outer viewport clips at 872, while its list draws to
// 951.67 and the glass occupies 872.33..922. Extend only the visual clip.
@interface HGMineClipRecord : NSObject
@property(nonatomic) BOOL clips;
@property(nonatomic, strong) CAShapeLayer *mask;
@property(nonatomic, strong) id top;
@property(nonatomic, strong) id bottom;
@property(nonatomic) BOOL topHidden;
@property(nonatomic) BOOL bottomHidden;
@end
@implementation HGMineClipRecord
@end
static char HGMineClipKey;
static void HGApplyMineClip(UIScrollView *scroll) {
    HGMineClipRecord *record = objc_getAssociatedObject(scroll, &HGMineClipKey);
    UIView *root = scroll.superview;
    Class rootClass = objc_lookUpClass("SSMine737KMPRootView");
    BOOL eligible = HGGlassEnabled && scroll.window && rootClass && [root isKindOfClass:rootClass] &&
        CGAffineTransformIsIdentity(scroll.transform) && HGNear(scroll.frame.origin.x, 0) &&
        HGNear(scroll.frame.origin.y, 0) && HGNear(scroll.bounds.size.width, root.bounds.size.width) &&
        root.bounds.size.height >= scroll.bounds.size.height &&
        root.bounds.size.height - scroll.bounds.size.height <= 160 &&
        (!scroll.layer.mask || scroll.layer.mask == record.mask);
    if (!eligible) {
        if (record) {
            if (scroll.layer.mask == record.mask) scroll.layer.mask = nil;
            if (!scroll.clipsToBounds) scroll.clipsToBounds = record.clips;
            if (record.top) ((void (*)(id, SEL, BOOL))objc_msgSend)(record.top, @selector(setHidden:), record.topHidden);
            if (record.bottom) ((void (*)(id, SEL, BOOL))objc_msgSend)(record.bottom, @selector(setHidden:), record.bottomHidden);
            objc_setAssociatedObject(scroll, &HGMineClipKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        return;
    }
    if (!record) {
        record = [HGMineClipRecord new]; record.clips = scroll.clipsToBounds;
        record.mask = [CAShapeLayer layer]; record.mask.fillColor = UIColor.blackColor.CGColor;
        for (NSString *name in @[@"topEdgeEffect", @"bottomEdgeEffect"]) {
            id edge = HGObject(scroll, name.UTF8String);
            if (![edge respondsToSelector:@selector(setHidden:)] || ![edge respondsToSelector:sel_registerName("isHidden")]) continue;
            BOOL hidden = ((BOOL (*)(id, SEL))objc_msgSend)(edge, sel_registerName("isHidden"));
            if ([name isEqual:@"topEdgeEffect"]) { record.top = edge; record.topHidden = hidden; }
            else { record.bottom = edge; record.bottomHidden = hidden; }
        }
        objc_setAssociatedObject(scroll, &HGMineClipKey, record, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    [CATransaction begin]; [CATransaction setDisableActions:YES];
    CGRect clip = scroll.bounds;
    clip.size.height = root.bounds.size.height;
    if (!CGRectEqualToRect(record.mask.frame, clip)) {
        BOOL resize = !CGSizeEqualToSize(record.mask.bounds.size, clip.size) || !record.mask.path;
        record.mask.frame = clip;
        if (resize) {
            CGPathRef path = CGPathCreateWithRect((CGRect){CGPointZero, clip.size}, NULL);
            record.mask.path = path; CGPathRelease(path);
        }
    }
    scroll.clipsToBounds = NO;
    scroll.layer.mask = record.mask;
    if (record.top) ((void (*)(id, SEL, BOOL))objc_msgSend)(record.top, @selector(setHidden:), YES);
    if (record.bottom) ((void (*)(id, SEL, BOOL))objc_msgSend)(record.bottom, @selector(setHidden:), YES);
    [CATransaction commit];
}
static void HGInstallMineClip(void) {
    Class cls = objc_lookUpClass("SSMyUser637NestedContainerScrollView");
    for (NSString *name in @[@"layoutSubviews", @"didMoveToWindow"]) {
        SEL sel = NSSelectorFromString(name);
        Method m = class_getInstanceMethod(cls, sel);
        if (!m || strcmp(method_getTypeEncoding(m), "v16@0:8")) continue;
        void (*original)(id, SEL) = (void *)method_getImplementation(m);
        HGReplaceMethod(cls, sel, "v16@0:8", imp_implementationWithBlock(^(UIScrollView *scroll) {
            original(scroll, sel); HGApplyMineClip(scroll);
        }));
    }
}
