#import "Honggo.h"
#import <QuartzCore/QuartzCore.h>
#import <objc/message.h>
#include <math.h>
#include <string.h>

// Only the version-checked host tab bar is a global hook target. Page/content
// observers below are per-instance subclasses, never UIView/UIScrollView hooks.
static Class HGTabBarClass;
static BOOL HGGlassEnabled;
static char HGStateKey, HGObservedOwnerKey, HGObservedBaseKey;
static void (*HGOriginalControllerLayout)(id, SEL);
static void (*HGOriginalSetAppearance)(id, SEL);
static void (*HGOriginalResetRatio)(id, SEL, double);
static void (*HGOriginalBarLayout)(id, SEL);
static void (*HGOriginalBarMove)(id, SEL);
static void (*HGOriginalBarAdd)(id, SEL, UIView *);
static void (*HGOriginalBarHidden)(id, SEL, BOOL);
static void (*HGOriginalBarAlpha)(id, SEL, CGFloat);
static void (*HGOriginalBarTraits)(id, SEL, id);

static id HGObject(id object, const char *name) {
    SEL selector = sel_registerName(name);
    return [object respondsToSelector:selector] ? ((id (*)(id, SEL))objc_msgSend)(object, selector) : nil;
}

static UIView *HGView(id object, const char *name) {
    id result = HGObject(object, name);
    return [result isKindOfClass:UIView.class] ? result : nil;
}

static BOOL HGNear(CGFloat a, CGFloat b) { return fabs(a - b) <= 0.75; }
static BOOL HGRectNear(CGRect a, CGRect b) {
    return HGNear(a.origin.x, b.origin.x) && HGNear(a.origin.y, b.origin.y) &&
        HGNear(a.size.width, b.size.width) && HGNear(a.size.height, b.size.height);
}
static BOOL HGClear(UIColor *color) { return !color || CGColorGetAlpha(color.CGColor) <= 0.001; }

@interface HGGlassBackdrop : UIVisualEffectView
@property(nonatomic) BOOL nativeGlass;
@property(nonatomic) BOOL changingWindow;
@end
@implementation HGGlassBackdrop
- (void)layoutSubviews {
    [super layoutSubviews];
    BOOL visible = self.window != nil && !self.changingWindow;
    for (UIView *view = self; view && visible; view = view.superview)
        visible = !view.hidden && view.alpha >= 0.999;
    if (!self.effect && visible && CGRectGetWidth(self.bounds) >= 64 && CGRectGetHeight(self.bounds) >= 32) {
        Class glassClass = NSClassFromString(@"UIGlassEffect");
        SEL factory = sel_registerName("effectWithStyle:");
        self.nativeGlass = HGNativeGlassConfigured();
        UIVisualEffect *effect = self.nativeGlass ?
            ((id (*)(id, SEL, NSInteger))objc_msgSend)(glassClass, factory, 0) : nil;
        if (!effect) {
            self.nativeGlass = NO;
            effect = [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemMaterial];
        }
        // UIGlassEffect is created in a real, attached layout pass. Creating it in
        // didMoveToWindow can leave it unrendered when a host view is recycled.
        self.effect = effect;
    }
}
- (void)willMoveToWindow:(UIWindow *)newWindow {
    if (!newWindow || self.window != newWindow) {
        self.changingWindow = YES;
        self.effect = nil;
    }
    [super willMoveToWindow:newWindow];
}
- (void)didMoveToWindow {
    [super didMoveToWindow];
    self.changingWindow = NO;
    if (self.window) [self setNeedsLayout];
}
@end

@class HGTabState;
@interface HGObserverOwner : NSObject
@property(nonatomic, weak) HGTabState *state;
@end
@implementation HGObserverOwner
@end

// Store only plugin-owned mutations, and restore only values still owned by us.
@interface HGSurfaceRecord : NSObject
@property(nonatomic, weak) UIView *view;
@property(nonatomic, strong) UIColor *color;
@property(nonatomic) BOOL opaque;
@property(nonatomic) BOOL cleared;
@property(nonatomic) BOOL hidden;
@property(nonatomic) BOOL hid;
@property(nonatomic, strong) UIVisualEffect *effect;
@property(nonatomic) BOOL removedEffect;
@property(nonatomic, strong) CALayer *mask;
@property(nonatomic) BOOL clips;
@property(nonatomic) BOOL unclipped;
- (void)clearFill;
- (void)hideBackground;
- (void)restore;
@end
@implementation HGSurfaceRecord
- (void)clearFill {
    UIView *view = self.view;
    if (!self.cleared || !HGClear(view.backgroundColor)) self.color = view.backgroundColor;
    if (!self.cleared || view.opaque) self.opaque = view.opaque;
    self.cleared = YES;
    if (!HGClear(view.backgroundColor)) view.backgroundColor = UIColor.clearColor;
    if (view.opaque) view.opaque = NO;
}
- (void)hideBackground {
    UIView *view = self.view;
    if (!self.hid || !view.hidden) self.hidden = view.hidden;
    self.hid = YES;
    if (!view.hidden) view.hidden = YES;
}
- (void)restore {
    UIView *view = self.view;
    if (self.cleared) {
        if (HGClear(view.backgroundColor)) view.backgroundColor = self.color;
        if (!view.opaque) view.opaque = self.opaque;
    }
    if (self.hid && view.hidden) view.hidden = self.hidden;
    if (self.removedEffect && [view isKindOfClass:UIVisualEffectView.class] &&
            !((UIVisualEffectView *)view).effect) ((UIVisualEffectView *)view).effect = self.effect;
    if (self.unclipped) {
        if (!view.layer.mask) view.layer.mask = self.mask;
        if (!view.clipsToBounds) view.clipsToBounds = self.clips;
    }
}
@end

@interface HGViewportRecord : NSObject
@property(nonatomic, weak) UIView *view;
@property(nonatomic, weak) UIView *parent;
@property(nonatomic) CGRect originalFrame;
@property(nonatomic) CGRect appliedFrame;
@property(nonatomic) BOOL resized;
@property(nonatomic) UIEdgeInsets originalInset;
@property(nonatomic) UIEdgeInsets appliedInset;
@property(nonatomic) UIEdgeInsets originalIndicator;
@property(nonatomic) UIEdgeInsets appliedIndicator;
@property(nonatomic) CGFloat originalAutomaticBottom;
@property(nonatomic) BOOL insetOwned;
@property(nonatomic, strong) id edge;
@property(nonatomic) BOOL edgeWasHidden;
- (void)restore;
@end
@implementation HGViewportRecord
- (void)restore {
    UIView *view = self.view;
    if ([view isKindOfClass:UIScrollView.class]) {
        UIScrollView *scroll = (UIScrollView *)view;
        if (self.insetOwned && HGNear(scroll.contentInset.bottom, self.appliedInset.bottom)) {
            UIEdgeInsets inset = scroll.contentInset;
            inset.bottom = self.originalInset.bottom;
            scroll.contentInset = inset;
        }
        if (self.insetOwned && HGNear(scroll.verticalScrollIndicatorInsets.bottom, self.appliedIndicator.bottom)) {
            UIEdgeInsets inset = scroll.verticalScrollIndicatorInsets;
            inset.bottom = self.originalIndicator.bottom;
            scroll.verticalScrollIndicatorInsets = inset;
        }
    }
    if (self.edge && [self.edge respondsToSelector:@selector(setHidden:)]) {
        ((void (*)(id, SEL, BOOL))objc_msgSend)(self.edge, @selector(setHidden:), self.edgeWasHidden);
    }
}
@end

static NSUInteger HGDepth(UIView *view) {
    NSUInteger result = 0;
    while (view.superview && result < 64) { result++; view = view.superview; }
    return result;
}
static void HGRestoreViewports(NSArray<HGViewportRecord *> *records) {
    NSArray<HGViewportRecord *> *ordered = [records sortedArrayUsingComparator:
        ^NSComparisonResult(HGViewportRecord *a, HGViewportRecord *b) {
            NSUInteger first = HGDepth(a.view), second = HGDepth(b.view);
            return first < second ? NSOrderedAscending : (first > second ? NSOrderedDescending : NSOrderedSame);
        }];
    NSHashTable<UIView *> *owned = [NSHashTable weakObjectsHashTable];
    // Capture ownership before resizing a parent can autoresize its child.
    for (HGViewportRecord *record in ordered) {
        if (record.resized && record.view.superview == record.parent &&
            HGRectNear(record.view.frame, record.appliedFrame)) [owned addObject:record.view];
    }
    for (HGViewportRecord *record in ordered) {
        if ([owned containsObject:record.view]) record.view.frame = record.originalFrame;
        [record restore];
    }
}

@interface HGTabState : NSObject
@property(nonatomic, weak) UIView *bar;
@property(nonatomic, weak) UIViewController *controller;
@property(nonatomic, strong) HGGlassBackdrop *glass;
@property(nonatomic, strong) NSMapTable<UIView *, HGSurfaceRecord *> *surfaces;
@property(nonatomic, strong) NSMapTable<UIView *, HGViewportRecord *> *viewports;
@property(nonatomic, strong) NSHashTable<UIView *> *observed;
@property(nonatomic) BOOL refreshing;
@property(nonatomic) BOOL nativeGlass;
@property(nonatomic) BOOL logged;
- (void)refresh;
- (void)restore;
@end

// Forward to the original runtime class even if a framework adds a subclass
// above ours later. Only selected live instances ever receive this subclass.
static Class HGObservedClass(id object) {
    for (Class cls = object_getClass(object); cls; cls = class_getSuperclass(cls)) {
        if (objc_getAssociatedObject(cls, &HGObservedBaseKey)) return cls;
    }
    return Nil;
}
static Class HGObservedBase(id object) {
    return (Class)objc_getAssociatedObject(HGObservedClass(object), &HGObservedBaseKey);
}
static void HGObservedRefresh(UIView *view) {
    HGObserverOwner *owner = objc_getAssociatedObject(view, &HGObservedOwnerKey);
    [owner.state refresh];
}
static void HGObservedLayout(UIView *view, SEL cmd) {
    struct objc_super parent = {view, HGObservedBase(view)};
    ((void (*)(struct objc_super *, SEL))objc_msgSendSuper)(&parent, cmd);
    HGObservedRefresh(view);
}
static void HGObservedAdd(UIView *view, SEL cmd, UIView *child) {
    struct objc_super parent = {view, HGObservedBase(view)};
    ((void (*)(struct objc_super *, SEL, id))objc_msgSendSuper)(&parent, cmd, child);
    HGObservedRefresh(view);
}
static void HGObservedColor(UIView *view, SEL cmd, UIColor *color) {
    struct objc_super parent = {view, HGObservedBase(view)};
    ((void (*)(struct objc_super *, SEL, id))objc_msgSendSuper)(&parent, cmd, color);
    HGObservedRefresh(view);
}
static void HGObservedHidden(UIView *view, SEL cmd, BOOL hidden) {
    struct objc_super parent = {view, HGObservedBase(view)};
    ((void (*)(struct objc_super *, SEL, BOOL))objc_msgSendSuper)(&parent, cmd, hidden);
    HGObservedRefresh(view);
}
static void HGObservedAlpha(UIView *view, SEL cmd, CGFloat alpha) {
    struct objc_super parent = {view, HGObservedBase(view)};
    ((void (*)(struct objc_super *, SEL, CGFloat))objc_msgSendSuper)(&parent, cmd, alpha);
    HGObservedRefresh(view);
}
static Class HGReportedClass(id object, SEL cmd) { return HGObservedBase(object); }

static void HGObserve(UIView *view, HGTabState *state) {
    if (!view || view == state.bar || [view isKindOfClass:UIVisualEffectView.class] ||
        [view isKindOfClass:UIControl.class] || [view isKindOfClass:UILabel.class] ||
        [view isKindOfClass:UIImageView.class]) return;
    HGObserverOwner *previous = objc_getAssociatedObject(view, &HGObservedOwnerKey);
    if (previous.state == state) return;
    if (previous.state) return; // Another active tab bar owns it.
    Class base = object_getClass(view);
    Class observed = HGObservedClass(view);
    if (!observed) {
        // Do not replace a KVO observation class, or a system material's internals.
        NSString *name = NSStringFromClass(base);
        if ([name hasPrefix:@"NSKVONotifying_"] || [name hasPrefix:@"_UI"])
            return;
        NSString *subclassName = [@"HGObserved_" stringByAppendingString:name];
        observed = NSClassFromString(subclassName);
        if (!observed) {
            observed = objc_allocateClassPair(base, subclassName.UTF8String, 0);
            if (!observed) return;
            class_addMethod(observed, @selector(layoutSubviews), (IMP)HGObservedLayout, "v16@0:8");
            class_addMethod(observed, @selector(didAddSubview:), (IMP)HGObservedAdd, "v24@0:8@16");
            class_addMethod(observed, @selector(setBackgroundColor:), (IMP)HGObservedColor, "v24@0:8@16");
            class_addMethod(observed, @selector(setHidden:), (IMP)HGObservedHidden, "v20@0:8B16");
            class_addMethod(observed, @selector(setOpaque:), (IMP)HGObservedHidden, "v20@0:8B16");
            class_addMethod(observed, @selector(setAlpha:), (IMP)HGObservedAlpha, "v24@0:8d16");
            class_addMethod(observed, @selector(class), (IMP)HGReportedClass, "#16@0:8");
            objc_setAssociatedObject(observed, &HGObservedBaseKey, base, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            objc_registerClassPair(observed);
        }
        if (class_getSuperclass(observed) != base) return;
        object_setClass(view, observed);
    }
    HGObserverOwner *owner = [HGObserverOwner new];
    owner.state = state;
    objc_setAssociatedObject(view, &HGObservedOwnerKey, owner, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [state.observed addObject:view];
}

static BOOL HGContainsControls(UIView *view, NSUInteger depth) {
    if ([view isKindOfClass:UIControl.class] || [view isKindOfClass:UILabel.class]) return YES;
    if (depth >= 6) return view.subviews.count > 0; // Ambiguous: preserve it.
    for (UIView *child in view.subviews) if (HGContainsControls(child, depth + 1)) return YES;
    return NO;
}

static UIUserInterfaceStyle HGForegroundStyle(UIView *content, UITraitCollection *traits) {
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:content];
    NSUInteger white = 0, black = 0;
    for (NSUInteger index = 0; index < queue.count && index < 96; index++) {
        UIView *view = queue[index];
        if (view.hidden || view.alpha < 0.05) continue;
        UIColor *foreground = nil;
        BOOL button = [view isKindOfClass:UIButton.class];
        if (button) {
            UIButton *item = (UIButton *)view;
            if (item.currentTitle.length || item.currentAttributedTitle.length) foreground = item.currentTitleColor;
        } else if ([view isKindOfClass:UILabel.class]) {
            UILabel *label = (UILabel *)view;
            if (label.text.length || label.attributedText.length) foreground = label.textColor;
        }
        if (foreground) {
                UIColor *color = [foreground resolvedColorWithTraitCollection:traits];
                CGFloat r = 0, g = 0, b = 0, a = 0;
                if ([color getRed:&r green:&g blue:&b alpha:&a] && a > 0.2) {
                    CGFloat luminance = r * 0.2126 + g * 0.7152 + b * 0.0722;
                    if (luminance > 0.82) white++;
                    if (luminance < 0.25) black++;
                }
        }
        // UIButton's current state updates before its titleLabel is laid out.
        // Do not count that stale internal label a second time.
        if (!button) [queue addObjectsFromArray:view.subviews];
    }
    if (white > black) return UIUserInterfaceStyleDark;
    if (black > white) return UIUserInterfaceStyleLight;
    return traits.userInterfaceStyle;
}

@implementation HGTabState
- (instancetype)init {
    self = [super init];
    if (self) {
        _surfaces = [NSMapTable weakToStrongObjectsMapTable];
        _viewports = [NSMapTable weakToStrongObjectsMapTable];
        _observed = [NSHashTable weakObjectsHashTable];
    }
    return self;
}
- (HGSurfaceRecord *)surface:(UIView *)view {
    HGSurfaceRecord *record = [self.surfaces objectForKey:view];
    if (!record) {
        record = [HGSurfaceRecord new];
        record.view = view;
        [self.surfaces setObject:record forKey:view];
    }
    return record;
}
- (void)clearBackgrounds:(UIView *)view content:(UIView *)content frame:(CGRect)frame depth:(NSUInteger)depth {
    if (view == self.glass || depth > 10 || [view isKindOfClass:UIScrollView.class]) return;
    BOOL inContent = view == content || [view isDescendantOfView:content];
    BOOL contentPath = view == self.bar || view == content || [content isDescendantOfView:view];
    BOOL control = [view isKindOfClass:UIControl.class];
    BOOL label = [view isKindOfClass:UILabel.class];
    BOOL image = [view isKindOfClass:UIImageView.class];
    CGRect rect = [view convertRect:view.bounds toView:self.bar];
    BOOL broad = CGRectGetWidth(rect) >= CGRectGetWidth(frame) * 0.75 &&
        CGRectGetHeight(rect) >= MIN(24, CGRectGetHeight(frame) * 0.6) &&
        CGRectIntersectsRect(rect, frame);

    if ([view isKindOfClass:UIVisualEffectView.class]) {
        // A host effect may itself contain live controls. Remove its material,
        // not its content, and never inspect private UIKit effect subviews.
        UIVisualEffectView *effectView = (UIVisualEffectView *)view;
        if (broad || contentPath) {
            HGSurfaceRecord *record = [self surface:view];
            if (!record.removedEffect || effectView.effect) record.effect = effectView.effect;
            record.removedEffect = YES;
            effectView.effect = nil;
            [record clearFill];
            [[self surface:effectView.contentView] clearFill];
            [self clearBackgrounds:effectView.contentView content:content frame:frame depth:depth + 1];
        }
        return;
    }
    BOOL itemContainer = inContent && !image && view.subviews.count > 0 &&
        CGRectGetWidth(rect) >= 44 && CGRectGetHeight(rect) >= 24;
    if (!control && !label && (contentPath || itemContainer)) {
        [[self surface:view] clearFill];
        HGObserve(view, self);
    } else if (!control && !label && broad && !HGContainsControls(view, 0)) {
        [[self surface:view] hideBackground];
        HGObserve(view.superview, self);
        return;
    }
    // An item can be a UIControl, but its title/image/red-dot subtree is owned
    // by the host. It must not be hidden, recolored or taken out of hit-testing.
    if (control || label || image) return;
    for (UIView *child in view.subviews.copy)
        [self clearBackgrounds:child content:content frame:frame depth:depth + 1];
}
- (void)prepareGlass:(CGRect)frame content:(UIView *)content {
    BOOL created = !self.glass;
    if (!self.glass) {
        self.glass = [[HGGlassBackdrop alloc] initWithEffect:nil];
        self.glass.accessibilityIdentifier = @"honggo.tabbar.glass";
        self.glass.userInteractionEnabled = NO;
        self.glass.accessibilityElementsHidden = YES;
        self.glass.opaque = NO;
        self.glass.backgroundColor = UIColor.clearColor;
        self.glass.contentView.backgroundColor = UIColor.clearColor;
        self.glass.alpha = 1;
    }
    HGGlassBackdrop *glass = self.glass;
    // Configure final geometry before attachment, in one disabled-actions
    // transaction. A new zero-sized rectangular effect is never displayed.
    BOOL resized = !HGRectNear(glass.frame, frame);
    if (resized) glass.frame = frame;
    UIUserInterfaceStyle style = HGForegroundStyle(content, self.bar.traitCollection);
    BOOL styleChanged = glass.overrideUserInterfaceStyle != style;
    if (styleChanged) glass.overrideUserInterfaceStyle = style;
    Class corners = NSClassFromString(@"UICornerConfiguration");
    SEL capsule = sel_registerName("capsuleConfiguration");
    SEL setCorners = sel_registerName("setCornerConfiguration:");
    if (created && [corners respondsToSelector:capsule] && [glass respondsToSelector:setCorners]) {
        ((void (*)(id, SEL, id))objc_msgSend)(glass, setCorners,
            ((id (*)(id, SEL))objc_msgSend)(corners, capsule));
    } else if (![glass respondsToSelector:setCorners] && (created || resized)) {
        glass.clipsToBounds = YES;
        glass.layer.cornerRadius = CGRectGetHeight(frame) / 2;
        glass.layer.cornerCurve = kCACornerCurveContinuous;
    }
    if (glass.superview != self.bar) [self.bar insertSubview:glass atIndex:0];
    else if (self.bar.subviews.firstObject != glass) [self.bar sendSubviewToBack:glass];
    glass.hidden = NO;
    if (!glass.effect || resized || styleChanged) {
        [glass setNeedsLayout];
        [glass layoutIfNeeded];
    }
    self.nativeGlass = glass.nativeGlass;
}
- (void)collectViewports:(UIView *)view root:(UIView *)root barTop:(CGFloat)barTop
        bottom:(CGFloat)bottom records:(NSMutableArray<HGViewportRecord *> *)records depth:(NSUInteger)depth {
    if (view == self.bar || [self.bar isDescendantOfView:view] || view.hidden || view.alpha < 0.01 ||
        depth > 10 || !CGAffineTransformIsIdentity(view.transform) || view.layer.mask ||
        [view isKindOfClass:UIControl.class] || [view isKindOfClass:UITableViewCell.class] ||
        [view isKindOfClass:UICollectionViewCell.class] || [view isKindOfClass:UIVisualEffectView.class]) return;
    CGRect actual = [view convertRect:view.bounds toView:root];
    CGFloat rootWidth = CGRectGetWidth(root.bounds), rootHeight = CGRectGetHeight(root.bounds);
    if (CGRectGetWidth(actual) < rootWidth * 0.80 || CGRectGetMinX(actual) < -2 ||
        CGRectGetMaxX(actual) > CGRectGetMaxX(root.bounds) + 2 ||
        CGRectGetMinY(actual) > barTop - 100 || CGRectGetHeight(actual) < rootHeight * 0.35) return;
    HGViewportRecord *record = [self.viewports objectForKey:view];
    CGRect baseline = record && record.resized && view.superview == record.parent &&
        HGRectNear(view.frame, record.appliedFrame) ? record.originalFrame : view.frame;
    CGRect original = [view.superview convertRect:baseline toView:root];
    CGFloat end = CGRectGetMaxY(original);
    // A short card/carousel is not a page viewport. Accept subpixel layout
    // differences and safe-area-ending pages, but never stretch arbitrary rows.
    if (end < barTop - 2 || end > bottom + 2) return;
    if (!record) {
        record = [HGViewportRecord new];
        record.view = view;
        record.parent = view.superview;
        [self.viewports setObject:record forKey:view];
    }
    record.originalFrame = baseline;
    record.parent = view.superview;
    CGRect target = baseline;
    target.size.height += MAX(0, bottom - end);
    record.appliedFrame = target;
    [records addObject:record];
    HGObserve(view, self);
    if ([view isKindOfClass:UIScrollView.class]) {
        UIScrollView *scroll = (UIScrollView *)view;
        BOOL horizontalPager = scroll.pagingEnabled &&
            scroll.contentSize.width > scroll.bounds.size.width + 2 &&
            scroll.contentSize.height <= scroll.bounds.size.height + 2;
        if (!horizontalPager) {
            // Snapshot inset baselines before ANY ancestor is resized: growing
            // a parent can already change the child's automatic safe-area inset.
            if (!record.insetOwned || !HGNear(scroll.contentInset.bottom, record.appliedInset.bottom)) {
                record.originalInset = scroll.contentInset;
                record.originalAutomaticBottom = scroll.adjustedContentInset.bottom - scroll.contentInset.bottom;
            }
            if (!record.insetOwned || !HGNear(scroll.verticalScrollIndicatorInsets.bottom, record.appliedIndicator.bottom))
                record.originalIndicator = scroll.verticalScrollIndicatorInsets;
            return; // Never descend into table/collection cells.
        }
    }
    for (UIView *child in view.subviews.copy)
        [self collectViewports:child root:root barTop:barTop bottom:bottom records:records depth:depth + 1];
}
- (void)refreshViewports {
    UIView *bar = self.bar;
    UIView *root = self.controller.viewIfLoaded;
    if (!root || root.window != bar.window || ![bar isDescendantOfView:root]) {
        HGRestoreViewports(self.viewports.objectEnumerator.allObjects);
        [self.viewports removeAllObjects];
        return;
    }
    CGRect windowRect = [bar.window convertRect:bar.window.bounds toView:root];
    CGFloat bottom = MIN(CGRectGetMaxY(root.bounds), CGRectGetMaxY(windowRect));
    CGRect barRect = [bar convertRect:bar.bounds toView:root];
    CGFloat barTop = CGRectGetMinY(barRect);
    if (bottom - barTop < 20 || bottom - barTop > 160) return;
    NSMutableArray<HGViewportRecord *> *records = [NSMutableArray array];
    // Walk only sibling content branches under the known tab controller.
    // Transparent intermediate containers can also enclose the tab bar.
    NSMutableArray<UIView *> *branches = [NSMutableArray array];
    UIView *path = bar;
    while (path.superview && path != root) {
        UIView *parent = path.superview;
        for (UIView *child in parent.subviews) if (child != path) [branches addObject:child];
        path = parent;
    }
    for (UIView *branch in branches)
        [self collectViewports:branch root:root barTop:barTop bottom:bottom records:records depth:0];
    NSHashTable<UIView *> *active = [NSHashTable weakObjectsHashTable];
    for (HGViewportRecord *record in records) [active addObject:record.view];
    NSMutableArray<HGViewportRecord *> *inactive = [NSMutableArray array];
    for (UIView *view in self.viewports.keyEnumerator.allObjects) {
        if (![active containsObject:view]) {
            [inactive addObject:[self.viewports objectForKey:view]];
            [self.viewports removeObjectForKey:view];
        }
    }
    HGRestoreViewports(inactive);
    // All baseline measurements are collected first, before autoresizing an
    // ancestor can change a descendant. Apply fixed targets parent-to-child.
    for (HGViewportRecord *record in records) {
        UIView *view = record.view;
        CGFloat delta = MAX(0, record.appliedFrame.size.height - record.originalFrame.size.height);
        if ([view isKindOfClass:UIScrollView.class]) {
            UIScrollView *scroll = (UIScrollView *)view;
            BOOL horizontalPager = scroll.pagingEnabled &&
                scroll.contentSize.width > scroll.bounds.size.width + 2 &&
                scroll.contentSize.height <= scroll.bounds.size.height + 2;
            if (!horizontalPager) {
                CGFloat automatic = scroll.adjustedContentInset.bottom - scroll.contentInset.bottom;
                CGFloat extra = MAX(0, delta + record.originalAutomaticBottom - automatic);
                // Own only bottom. A host changing top/left/right while our
                // bottom padding is live must not turn that padding into a new baseline.
                UIEdgeInsets inset = scroll.contentInset, indicator = scroll.verticalScrollIndicatorInsets;
                inset.bottom = record.originalInset.bottom + extra;
                indicator.bottom = record.originalIndicator.bottom + extra;
                record.appliedInset = inset;
                record.appliedIndicator = indicator;
                record.insetOwned = YES;
                // Add scrollable padding before increasing the viewport; keep
                // the last row reachable above the floating navigation capsule.
                if (!UIEdgeInsetsEqualToEdgeInsets(scroll.contentInset, inset)) scroll.contentInset = inset;
                if (!UIEdgeInsetsEqualToEdgeInsets(scroll.verticalScrollIndicatorInsets, indicator))
                    scroll.verticalScrollIndicatorInsets = indicator;
                id edge = HGObject(scroll, "bottomEdgeEffect");
                if (edge != record.edge && [edge respondsToSelector:@selector(setHidden:)]) {
                    if (record.edge) ((void (*)(id, SEL, BOOL))objc_msgSend)(record.edge,
                        @selector(setHidden:), record.edgeWasHidden);
                    record.edge = edge;
                    SEL hidden = sel_registerName("isHidden");
                    record.edgeWasHidden = [edge respondsToSelector:hidden] &&
                        ((BOOL (*)(id, SEL))objc_msgSend)(edge, hidden);
                }
                if (record.edge) ((void (*)(id, SEL, BOOL))objc_msgSend)(record.edge, @selector(setHidden:), YES);
            }
        }
        record.resized = delta > 0.75;
        if (!HGRectNear(view.frame, record.appliedFrame)) view.frame = record.appliedFrame;
    }
    // A scroll view's own resize can change automatic safe-area adjustment too.
    // Reconcile after all frames settle, without treating our padding as new input.
    for (HGViewportRecord *record in records) {
        if (!record.insetOwned || ![record.view isKindOfClass:UIScrollView.class]) continue;
        UIScrollView *scroll = (UIScrollView *)record.view;
        CGFloat delta = MAX(0, record.appliedFrame.size.height - record.originalFrame.size.height);
        CGFloat automatic = scroll.adjustedContentInset.bottom - scroll.contentInset.bottom;
        CGFloat extra = MAX(0, delta + record.originalAutomaticBottom - automatic);
        UIEdgeInsets inset = scroll.contentInset, indicator = scroll.verticalScrollIndicatorInsets;
        inset.bottom = record.originalInset.bottom + extra;
        indicator.bottom = record.originalIndicator.bottom + extra;
        record.appliedInset = inset;
        record.appliedIndicator = indicator;
        if (!UIEdgeInsetsEqualToEdgeInsets(scroll.contentInset, inset)) scroll.contentInset = inset;
        if (!UIEdgeInsetsEqualToEdgeInsets(scroll.verticalScrollIndicatorInsets, indicator))
            scroll.verticalScrollIndicatorInsets = indicator;
    }
}
- (void)restore {
    HGRestoreViewports(self.viewports.objectEnumerator.allObjects);
    [self.viewports removeAllObjects];
    for (HGSurfaceRecord *record in self.surfaces.objectEnumerator.allObjects) [record restore];
    [self.surfaces removeAllObjects];
    for (UIView *view in self.observed.allObjects) {
        HGObserverOwner *owner = objc_getAssociatedObject(view, &HGObservedOwnerKey);
        if (owner.state != self) continue;
        Class observed = HGObservedClass(view);
        if (object_getClass(view) == observed) object_setClass(view, HGObservedBase(view));
        objc_setAssociatedObject(view, &HGObservedOwnerKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    [self.observed removeAllObjects];
    self.glass.effect = nil;
    self.glass.hidden = YES;
    [self.glass removeFromSuperview];
}
- (void)refresh {
    if (self.refreshing) return;
    self.refreshing = YES;
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    @try {
        UIView *bar = self.bar;
        if (!HGGlassEnabled || !bar.window || bar.hidden) {
            [self restore];
            return;
        }
        BOOL visible = YES;
        UIView *ancestor = bar;
        for (NSUInteger depth = 0; ancestor && depth < 24; depth++, ancestor = ancestor.superview) {
            visible = visible && !ancestor.hidden && ancestor.alpha >= 0.999;
            if ([ancestor isKindOfClass:UIWindow.class]) break;
            if (ancestor != bar) HGObserve(ancestor, self);
        }
        if (!visible) {
            self.glass.effect = nil;
            self.glass.hidden = YES;
            return; // Visibility setters on this instance's ancestor path retry synchronously.
        }
        UIView *content = HGView(bar, "contentView");
        if (!content || content == bar || ![content isDescendantOfView:bar]) {
            [self restore];
            return;
        }
        CGRect frame = [content convertRect:content.bounds toView:bar];
        if (!isfinite(frame.size.width) || !isfinite(frame.size.height) ||
            frame.size.width < 64 || frame.size.height < 32) {
            self.glass.hidden = YES;
            return;
        }
        // Drop ownership for detached/replaced content before preparing new views.
        for (UIView *view in self.surfaces.keyEnumerator.allObjects) {
            if (view != bar && ![view isDescendantOfView:bar]) {
                [[self.surfaces objectForKey:view] restore];
                [self.surfaces removeObjectForKey:view];
            }
        }
        [self clearBackgrounds:bar content:content frame:frame depth:0];
        const char *names[] = {"backgroundImageView", "lightView", "darkView", "lineView"};
        for (NSUInteger index = 0; index < sizeof(names) / sizeof(names[0]); index++) {
            UIView *background = HGView(bar, names[index]);
            if (!background || background == bar || background == content ||
                ![background isDescendantOfView:bar] || [content isDescendantOfView:background]) continue;
            if (!HGContainsControls(background, 0)) [[self surface:background] hideBackground];
            else [[self surface:background] clearFill];
            HGObserve(background.superview, self);
        }
        // Parent masks invalidate UIVisualEffectView rendering. Only replace the
        // known tab bar's clipping; capsule clipping belongs to our own effect.
        HGSurfaceRecord *barRecord = [self surface:bar];
        if (!barRecord.unclipped || bar.layer.mask) barRecord.mask = bar.layer.mask;
        if (!barRecord.unclipped || bar.clipsToBounds) barRecord.clips = bar.clipsToBounds;
        barRecord.unclipped = YES;
        if (bar.layer.mask) bar.layer.mask = nil;
        if (bar.clipsToBounds) bar.clipsToBounds = NO;
        [self prepareGlass:frame content:content];
        [self refreshViewports];
        if (!self.logged) {
            NSLog(@"[Honggo] 版本 %@；底栏：%@；主包兼容模式=%@；减少透明度=%d；增强对比度=%d；背景已按宿主层级处理",
                HG_PLUGIN_VERSION, self.nativeGlass ? @"UIGlassEffect Regular" : @"UIBlurEffect fallback",
                [NSBundle.mainBundle objectForInfoDictionaryKey:@"UIDesignRequiresCompatibility"] ?: @"未明确配置",
                UIAccessibilityIsReduceTransparencyEnabled(), UIAccessibilityDarkerSystemColorsEnabled());
            self.logged = YES;
        }
    } @finally {
        [CATransaction commit];
        self.refreshing = NO;
    }
}
@end

static HGTabState *HGState(UIView *bar) {
    if (![bar isKindOfClass:HGTabBarClass]) return nil;
    HGTabState *state = objc_getAssociatedObject(bar, &HGStateKey);
    if (!state) {
        state = [HGTabState new];
        state.bar = bar;
        objc_setAssociatedObject(bar, &HGStateKey, state, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    if (!state.controller) {
        UIResponder *responder = bar.nextResponder;
        while (responder && ![responder isKindOfClass:UIViewController.class]) responder = responder.nextResponder;
        state.controller = (UIViewController *)responder;
    }
    return state;
}
static void HGRefreshTabBar(UIViewController *controller) {
    UIView *bar = HGView(controller, "customTabBar");
    if (![bar isKindOfClass:HGTabBarClass]) return;
    HGAttachSettings(bar);
    if (HGGlassEnabled) {
        HGTabState *state = HGState(bar);
        state.controller = controller;
        [state refresh];
    }
}
static void HGControllerLayout(id self, SEL cmd) {
    HGOriginalControllerLayout(self, cmd);
    HGRefreshTabBar(self);
}
static void HGSetAppearance(id self, SEL cmd) {
    HGOriginalSetAppearance(self, cmd);
    HGRefreshTabBar(self);
}
static void HGResetRatio(id self, SEL cmd, double ratio) {
    HGOriginalResetRatio(self, cmd, ratio);
    [HGState(self) refresh];
}
static void HGBarLayout(id self, SEL cmd) {
    HGOriginalBarLayout(self, cmd);
    [HGState(self) refresh];
}
static void HGBarMove(id self, SEL cmd) {
    HGOriginalBarMove(self, cmd);
    [HGState(self) refresh];
}
static void HGBarAdd(id self, SEL cmd, UIView *child) {
    HGOriginalBarAdd(self, cmd, child);
    [HGState(self) refresh];
}
static void HGBarHidden(id self, SEL cmd, BOOL hidden) {
    HGOriginalBarHidden(self, cmd, hidden);
    [HGState(self) refresh];
}
static void HGBarAlpha(id self, SEL cmd, CGFloat alpha) {
    HGOriginalBarAlpha(self, cmd, alpha);
    [HGState(self) refresh];
}
static void HGBarTraits(id self, SEL cmd, id previous) {
    HGOriginalBarTraits(self, cmd, previous);
    [HGState(self) refresh];
}

BOOL HGInstallTabBar(void) {
    @synchronized (HGTabState.class) {
        static BOOL installed;
        if (installed) return YES;
        HGTabBarClass = objc_lookUpClass("SSTabBar");
        Class controller = objc_lookUpClass("SSTabBarController");
        if (![HGTabBarClass isSubclassOfClass:UIView.class] ||
            ![controller isSubclassOfClass:UIViewController.class] ||
            !class_getInstanceMethod(controller, sel_registerName("customTabBar")) ||
            !class_getInstanceMethod(HGTabBarClass, sel_registerName("contentView"))) return NO;
        Class targets[] = {controller, HGTabBarClass, HGTabBarClass, HGTabBarClass};
        SEL selectors[] = {@selector(viewDidLayoutSubviews), @selector(layoutSubviews),
            @selector(didMoveToWindow), @selector(didAddSubview:)};
        const char *encodings[] = {"v16@0:8", "v16@0:8", "v16@0:8", "v24@0:8@16"};
        // Validate the complete required set before mutating, so failed retries
        // cannot install a hook over itself and recurse forever.
        for (NSUInteger i = 0; i < 4; i++) {
            Method method = class_getInstanceMethod(targets[i], selectors[i]);
            if (!method || strcmp(method_getTypeEncoding(method), encodings[i]) != 0) return NO;
        }
        HGGlassEnabled = HGIsEnabled(HGGlass);
        HGOriginalControllerLayout = (void (*)(id, SEL))HGReplaceMethod(controller, selectors[0], encodings[0], (IMP)HGControllerLayout);
        if (HGGlassEnabled) {
            HGOriginalBarLayout = (void (*)(id, SEL))HGReplaceMethod(HGTabBarClass, selectors[1], encodings[1], (IMP)HGBarLayout);
            HGOriginalBarMove = (void (*)(id, SEL))HGReplaceMethod(HGTabBarClass, selectors[2], encodings[2], (IMP)HGBarMove);
            HGOriginalBarAdd = (void (*)(id, SEL, UIView *))HGReplaceMethod(HGTabBarClass, selectors[3], encodings[3], (IMP)HGBarAdd);
            HGOriginalBarHidden = (void (*)(id, SEL, BOOL))HGReplaceMethod(HGTabBarClass, @selector(setHidden:), "v20@0:8B16", (IMP)HGBarHidden);
            HGOriginalBarAlpha = (void (*)(id, SEL, CGFloat))HGReplaceMethod(HGTabBarClass, @selector(setAlpha:), "v24@0:8d16", (IMP)HGBarAlpha);
            HGOriginalBarTraits = (void (*)(id, SEL, id))HGReplaceMethod(HGTabBarClass, sel_registerName("traitCollectionDidChange:"), "v24@0:8@16", (IMP)HGBarTraits);
            HGOriginalSetAppearance = (void (*)(id, SEL))HGReplaceMethod(controller, sel_registerName("setCustomTabBarAppearance"), "v16@0:8", (IMP)HGSetAppearance);
            HGOriginalResetRatio = (void (*)(id, SEL, double))HGReplaceMethod(HGTabBarClass, sel_registerName("resetBlurVisualViewWithRatio:"), "v24@0:8d16", (IMP)HGResetRatio);
        }
        installed = YES;
        return YES;
    }
}
