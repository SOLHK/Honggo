#import "Honggo.h"
#import "Appearance.h"
#import "RuntimeSupport.h"
#import <QuartzCore/QuartzCore.h>
#import <objc/message.h>
#include <math.h>
#include <string.h>

// Shared with the custom dock implementation. Host setter observations must
// not take a new geometry/background baseline halfway through its transaction.
BOOL HGAppearanceDockIsMutating(UIView *bar);

// Lifecycle hooks target version-checked host types. Page/content observers
// below use per-instance subclasses instead of UIKit base class hooks.
static Class HGTabBarClass;
static BOOL HGGlassEnabled;
static char HGStateKey, HGObservedOwnerKey, HGObservedBaseKey;
// The home feed and tab controller both mutate geometry during the same
// native transition. Publish ownership before invoking either original.
@interface HGSidebarTransition : NSObject
@property(nonatomic, weak) UIViewController *page;
@property(nonatomic) BOOL pending;
@end
@implementation HGSidebarTransition @end

static char HGSidebarTransitionKey;
static SEL HGSidebarOpenGetter;
static __weak UIWindow *HGSidebarMotionWindow;

static BOOL HGSidebarIsOpen(UIViewController *page) {
    return page && HGSidebarOpenGetter && ((BOOL (*)(id, SEL))objc_msgSend)(page, HGSidebarOpenGetter);
}

BOOL HGFeedSidebarOwnsLayout(UIWindow *window) {
    if (!window) return NO;
    if (HGSidebarMotionWindow == window) return YES;
    HGSidebarTransition *transition = objc_getAssociatedObject(window, &HGSidebarTransitionKey);
    UIViewController *page = transition.page;
    return page && page.viewIfLoaded.window == window && (transition.pending || HGSidebarIsOpen(page));
}

static void (*HGOriginalControllerLayout)(id, SEL);
static void (*HGOriginalSetAppearance)(id, SEL);
static void (*HGOriginalResetRatio)(id, SEL, double);
static void (*HGOriginalBarLayout)(id, SEL);
static void (*HGOriginalBarMove)(id, SEL);
static void (*HGOriginalBarAdd)(id, SEL, UIView *);
static void (*HGOriginalBarHidden)(id, SEL, BOOL);
static void (*HGOriginalBarAlpha)(id, SEL, CGFloat);
static void (*HGOriginalBarTraits)(id, SEL, id);
static void (*HGOriginalMineLayout)(id, SEL);
static void (*HGOriginalMineAppear)(id, SEL, BOOL);
static void (*HGOriginalMallMount)(id, SEL);
static void (*HGOriginalTheaterListLayout)(id, SEL);
static void (*HGOriginalToggleFeedSidebar)(id, SEL, double);
static char HGMineRefreshPendingKey;

static id HGObject(id object, const char *name) {
    return HGReadObjectGetter(object, name);
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

#import "MineClip.inc"

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
        if (self.nativeGlass && [effect respondsToSelector:sel_registerName("setInteractive:")])
            ((void (*)(id, SEL, BOOL))objc_msgSend)(effect, sel_registerName("setInteractive:"), YES);
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
#import "TabTitles.inc"
static char HGTitleRecordKey;
UIColor *HGOriginalTabTitleColor(UILabel *label) {
    HGTabTitleRecord *record = objc_getAssociatedObject(label, &HGTitleRecordKey);
    return record && [label.textColor isEqual:record.appliedColor] ? record.color : label.textColor;
}
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
@property(nonatomic, strong) id topEdge;
@property(nonatomic) BOOL topEdgeWasHidden;
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
    if (self.topEdge) ((void (*)(id, SEL, BOOL))objc_msgSend)(self.topEdge,
        @selector(setHidden:), self.topEdgeWasHidden);
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
        if (record.resized && (!record.view.superview || record.view.superview == record.parent) &&
            HGRectNear(record.view.frame, record.appliedFrame)) [owned addObject:record.view];
    }
    for (HGViewportRecord *record in ordered) {
        if ([owned containsObject:record.view]) record.view.frame = record.originalFrame;
        [record restore];
    }
}

#import "TabBarVideoBackdrop.inc"

@interface HGTabState : NSObject
@property(nonatomic, weak) UIView *bar;
@property(nonatomic, weak) UIViewController *controller;
@property(nonatomic, strong) HGGlassBackdrop *glass;
@property(nonatomic, strong) NSMapTable<UIView *, HGSurfaceRecord *> *surfaces;
@property(nonatomic, strong) NSMapTable<UIView *, HGViewportRecord *> *viewports;
@property(nonatomic, strong) NSHashTable<UIView *> *observed;
@property(nonatomic, strong) NSMapTable<UILabel *, HGTabTitleRecord *> *titles;
@property(nonatomic, strong) HGVideoBackdrop *videoBackdrop;
@property(nonatomic) BOOL refreshing;
@property(nonatomic) BOOL customDock;
@property(nonatomic) BOOL refreshPending;
@property(nonatomic) BOOL nativeGlass;
@property(nonatomic) BOOL logged;
@property(nonatomic) NSTimeInterval lastDiagnostic;
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
    HGObserverOwner *owner = objc_getAssociatedObject(view, &HGObservedOwnerKey);
    UIView *bar = owner.state.bar;
    if (bar && view != bar && [bar isDescendantOfView:view]) {
        // A drawer or feed can mount a sibling during scrollViewDidScroll: or
        // cell reuse. Do not inspect the live collection from inside that mutation.
        if (!HGFeedSidebarOwnsLayout(bar.window) && ![child.accessibilityIdentifier isEqual:@"honggo.video.matte"])
            [bar setNeedsLayout];
        return;
    }
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

static BOOL HGIsPrivateSystemContainerName(NSString *name) {
    if (!name.length) return YES;
    if ([name hasPrefix:@"NSKVONotifying_"] || [name hasPrefix:@"_UI"] || [name hasPrefix:@"UIKit."])
        return YES;
    return [name isEqualToString:@"UILayoutContainerView"] ||
        [name isEqualToString:@"UINavigationTransitionView"] ||
        [name isEqualToString:@"UIViewControllerWrapperView"] ||
        [name isEqualToString:@"UIDropShadowView"] ||
        [name isEqualToString:@"UITransitionView"];
}

static BOOL HGIsOverlayOrDrawerView(UIView *view) {
    if (!view) return NO;
    NSString *viewName = NSStringFromClass(view.class);
    if ([viewName containsString:@"SideBar"] || [viewName containsString:@"Drawer"]) return YES;
    UIResponder *resp = view.nextResponder;
    if ([resp isKindOfClass:UIViewController.class]) {
        NSString *vcName = NSStringFromClass(resp.class);
        if ([vcName containsString:@"SideBar"] || [vcName containsString:@"Drawer"] ||
            [vcName containsString:@"Popup"] || [vcName containsString:@"Modal"] ||
            [vcName containsString:@"Search"]) {
            return YES;
        }
    }
    return NO;
}

static BOOL HGTabBarBoolGetter(id obj, const char *name) {
    if (!obj || !name) return NO;
    SEL sel = sel_registerName(name);
    Method m = class_getInstanceMethod(object_getClass(obj), sel);
    const char *enc = m ? method_getTypeEncoding(m) : NULL;
    if (!enc || (strcmp(enc, "B16@0:8") != 0 && strcmp(enc, "c16@0:8") != 0)) return NO;
    return ((BOOL (*)(id, SEL))objc_msgSend)(obj, sel);
}

static BOOL HGTabControllerHasOpenSidebar(UIViewController *controller) {
    if (!controller) return NO;
    Class feedCls = NSClassFromString(@"SSVideoSeriesFeedViewController");
    if (!feedCls) return NO;
    NSMutableArray<UIViewController *> *queue = [NSMutableArray arrayWithObject:controller];
    for (NSUInteger i = 0; i < queue.count && i < 16; i++) {
        UIViewController *vc = queue[i];
        if ([vc isKindOfClass:feedCls] && HGTabBarBoolGetter(vc, "isSidebarOpen")) return YES;
        for (UIViewController *child in vc.childViewControllers) {
            if (queue.count < 16) [queue addObject:child];
        }
    }
    return NO;
}

static BOOL HGTabBarIsInTransitionOrDisplaced(UIView *bar, UIViewController *controller) {
    if (!bar) return NO;
    if (fabs(bar.frame.origin.x) > 2.0) return YES;
    UIView *root = controller.viewIfLoaded;
    if (root && bar.window && root.window == bar.window && [bar isDescendantOfView:root]) {
        CGRect rect = [bar convertRect:bar.bounds toView:root];
        if (fabs(CGRectGetMinX(rect)) > 2.0) return YES;
    }
    if (controller.transitionCoordinator || controller.navigationController.transitionCoordinator) return YES;
    return HGTabControllerHasOpenSidebar(controller);
}

static void HGObserveView(UIView *view, HGTabState *state, BOOL knownBackground) {
    if (!view || view == state.bar || [view isKindOfClass:UIVisualEffectView.class] ||
        [view isKindOfClass:UIControl.class] || [view isKindOfClass:UILabel.class] ||
        ([view isKindOfClass:UIImageView.class] && !knownBackground)) return;
    HGObserverOwner *previous = objc_getAssociatedObject(view, &HGObservedOwnerKey);
    if (previous.state == state) return;
    if (previous.state) return; // Another active tab bar owns it.
    Class base = object_getClass(view);
    Class observed = HGObservedClass(view);
    if (!observed) {
        // Never isa-swizzle KVO classes, private UIKit views, or system
        // navigation/tab transition containers (causes freezes on iOS 26/27).
        NSString *name = NSStringFromClass(base);
        if (HGIsPrivateSystemContainerName(name))
            return;
        NSString *subclassName = [@"HGObserved_" stringByAppendingString:name];
        observed = NSClassFromString(subclassName);
        if (!observed) {
            observed = objc_allocateClassPair(base, subclassName.UTF8String, 0);
            if (!observed) return;
            class_addMethod(observed, @selector(layoutSubviews), (IMP)HGObservedLayout, "v16@0:8");
            class_addMethod(observed, @selector(didMoveToSuperview), (IMP)HGObservedLayout, "v16@0:8");
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

static void HGObserve(UIView *view, HGTabState *state) {
    HGObserveView(view, state, NO);
}

static void HGUnobserve(UIView *view, HGTabState *state) {
    HGObserverOwner *owner = objc_getAssociatedObject(view, &HGObservedOwnerKey);
    if (owner.state != state) return;
    Class observed = HGObservedClass(view);
    if (observed && object_getClass(view) == observed)
        object_setClass(view, HGObservedBase(view));
    objc_setAssociatedObject(view, &HGObservedOwnerKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [state.observed removeObject:view];
}

static BOOL HGContainsControls(UIView *view, NSUInteger depth) {
    if ([view isKindOfClass:UIControl.class] || [view isKindOfClass:UILabel.class]) return YES;
    if (depth >= 6) return view.subviews.count > 0; // Ambiguous: preserve it.
    for (UIView *child in view.subviews) if (HGContainsControls(child, depth + 1)) return YES;
    return NO;
}

static BOOL HGNestedScroll(UIView *view) {
    Class type = NSClassFromString(@"SSNestedContainerScrollView");
    Class mine = NSClassFromString(@"SSMyUser637NestedContainerScrollView");
    // Eggplant 7.4.3.33 declares these as separate UIScrollView subclasses.
    // Treating Mine as a leaf adds padding to the outer coordinator and skips
    // the clipped inner list once the profile header has scrolled away.
    return (type && [view isKindOfClass:type]) || (mine && [view isKindOfClass:mine]);
}

static BOOL HGContainerScroll(UIView *view) {
    if (HGNestedScroll(view)) return YES;
    // The device log shows this Compose interop bridge has contentSize={0,0}.
    // Its UIScrollView superclass does not make it a list: adding the viewport
    // extension to its contentInset changes the bridge after navigation.
    // Kotlin appends a generated numeric suffix, so match the verified type
    // prefix and leave unrelated UIScrollViews on the ordinary list path.
    return [NSStringFromClass(view.class)
        hasPrefix:@"RKMPandroidx.compose.ui.scene.UIKitTransparentContainerView"];
}

static UIViewController *HGTheaterPageForView(UIView *view) {
    Class theater = objc_lookUpClass("FQTheaterViewController");
    for (UIView *node = view; node; node = node.superview) {
        UIResponder *owner = node.nextResponder;
        if ([owner isKindOfClass:theater]) return (id)owner;
    }
    return nil;
}

static BOOL HGTheaterCategoryPager(UIScrollView *scroll) {
    Class container = objc_lookUpClass("FQRJXCategoryListContainerView");
    for (UIView *node = scroll.superview; node; node = node.superview) {
        if (![node isKindOfClass:container]) continue;
        return HGView(node, "contentScrollView") == scroll && HGTheaterPageForView(node) != nil;
    }
    return NO;
}

static BOOL HGContainsVisibleVerticalPager(UIView *branch, UIView *root) {
    // A video feed's page height is also its paging stride. Extending either
    // the feed or an autoresizing ancestor can reveal the following episode
    // and change the host's scroll-end/player-transfer decisions. Only ordinary
    // lists may extend beneath the floating bar; leave video branches intact.
    Class videoFeed = objc_lookUpClass("FQVCollectionView");
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:branch];
    for (NSUInteger i = 0; i < queue.count && i < 192; i++) {
        UIView *view = queue[i];
        if (view.hidden || view.alpha < 0.01 || CGRectIsEmpty(view.bounds) ||
            !CGRectIntersectsRect([view convertRect:view.bounds toView:root], root.bounds)) continue;
        if (videoFeed && [view isKindOfClass:videoFeed]) return YES;
        if ([view isKindOfClass:UIScrollView.class]) {
            UIScrollView *scroll = (UIScrollView *)view;
            BOOL verticalPages = scroll.pagingEnabled &&
                scroll.contentSize.height > scroll.bounds.size.height + 2 &&
                scroll.contentSize.width <= scroll.bounds.size.width + 2;
            if (verticalPages) return YES;
            // Ordinary list cells and carousels do not decide the containing
            // page's viewport. Continue through host container/horizontal pagers.
            if (!HGContainerScroll(scroll) && !scroll.pagingEnabled && !HGTheaterCategoryPager(scroll)) continue;
        }
        if ([view isKindOfClass:UIControl.class] || [view isKindOfClass:UILabel.class] ||
            [view isKindOfClass:UIImageView.class] || [view isKindOfClass:UIVisualEffectView.class] ||
            [view isKindOfClass:UITableViewCell.class] || [view isKindOfClass:UICollectionViewCell.class]) continue;
        for (UIView *child in view.subviews) {
            if (queue.count >= 192) return YES; // Ambiguous tree: preserve host geometry.
            [queue addObject:child];
        }
    }
    return NO;
}

@implementation HGTabState
- (instancetype)init {
    self = [super init];
    if (self) {
        _surfaces = [NSMapTable weakToStrongObjectsMapTable];
        _viewports = [NSMapTable weakToStrongObjectsMapTable];
        _observed = [NSHashTable weakObjectsHashTable];
        _titles = [NSMapTable weakToStrongObjectsMapTable];
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
    if (view == self.glass || [view.accessibilityIdentifier hasPrefix:@"honggo.appearance."] ||
        depth > 10 || [view isKindOfClass:UIScrollView.class]) return;
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
        // The custom dock owns the host bar's fill. A second saved color here
        // would overwrite a newer host theme when switching dock modes.
        if (view != self.bar || !self.customDock) [[self surface:view] clearFill];
        HGObserve(view, self);
    } else if (!control && !label && broad && !HGContainsControls(view, 0)) {
        [[self surface:view] hideBackground];
        HGObserve(view, self);
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
    UIUserInterfaceStyle style = HGGlassForegroundStyle(content, self.bar.traitCollection);
    BOOL styleChanged = glass.overrideUserInterfaceStyle != style;
    if (styleChanged) glass.overrideUserInterfaceStyle = style;
    Class corners = NSClassFromString(@"UICornerConfiguration");
    SEL capsule = sel_registerName("capsuleConfiguration");
    SEL setCorners = sel_registerName("setCornerConfiguration:");
    BOOL nativeCorners = HGNativeGlassConfigured() &&
        [corners respondsToSelector:capsule] && [glass respondsToSelector:setCorners];
    if (created && nativeCorners) {
        ((void (*)(id, SEL, id))objc_msgSend)(glass, setCorners,
            ((id (*)(id, SEL))objc_msgSend)(corners, capsule));
    } else if (!nativeCorners && (created || resized)) {
        // Compatibility mode exposes cornerConfiguration but ignores it for
        // the blur fallback. Clip this effect's own layer, never its parent.
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
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:content];
    for (NSUInteger i = 0; i < queue.count && i < 64; i++) {
        UIView *item = queue[i];
        BOOL tab = [item isKindOfClass:objc_lookUpClass("SSTabBarTextButton")] ||
            [item isKindOfClass:objc_lookUpClass("SSTabBarButton")];
        if (tab) {
            UILabel *label = (UILabel *)HGView(item, "tabLabel");
            if (![label isKindOfClass:UILabel.class] || ![label isDescendantOfView:item]) continue;
            HGTabTitleRecord *record = [self.titles objectForKey:label];
            if (!record) {
                record = [HGTabTitleRecord new]; record.label = label;
                [self.titles setObject:record forKey:label];
                objc_setAssociatedObject(label, &HGTitleRecordKey, record, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            }
            if (![label.textColor isEqual:record.appliedColor]) record.color = label.textColor;
            if (![label.font isEqual:record.appliedFont]) record.font = label.font;
            BOOL selected = [(UIControl *)item isSelected];
            BOOL dark = glass.overrideUserInterfaceStyle == UIUserInterfaceStyleDark;
            record.appliedColor = [UIColor colorWithWhite:dark ? (selected ? 1 : 0.92) : (selected ? 0 : 0.12) alpha:1];
            UIFontDescriptor *descriptor = [record.font.fontDescriptor fontDescriptorByAddingAttributes:
                @{UIFontDescriptorTraitsAttribute: @{UIFontWeightTrait: @(selected ? UIFontWeightBold : UIFontWeightSemibold)}}];
            record.appliedFont = [UIFont fontWithDescriptor:descriptor size:record.font.pointSize];
            if (![label.textColor isEqual:record.appliedColor]) label.textColor = record.appliedColor;
            if (![label.font isEqual:record.appliedFont]) label.font = record.appliedFont;
            continue; // Never visit its badge/title descendants as other controls.
        }
        if (![item isKindOfClass:UIVisualEffectView.class] && queue.count < 64)
            for (UIView *child in item.subviews) { if (queue.count >= 64) break; [queue addObject:child]; }
    }
}
- (void)collectViewports:(UIView *)view root:(UIView *)root barTop:(CGFloat)barTop
        bottom:(CGFloat)bottom records:(NSMutableArray<HGViewportRecord *> *)records depth:(NSUInteger)depth {
    if (view == self.bar || [self.bar isDescendantOfView:view] || view.hidden || view.alpha < 0.01 ||
        depth > 10 || !CGAffineTransformIsIdentity(view.transform) || view.layer.mask ||
        HGIsOverlayOrDrawerView(view) ||
        [view isKindOfClass:UIControl.class] || [view isKindOfClass:UITableViewCell.class] ||
        [view isKindOfClass:UICollectionViewCell.class] || [view isKindOfClass:UIVisualEffectView.class]) return;
    Class mineRoot = objc_lookUpClass("SSMine737KMPRootView");
    if (mineRoot && [view isKindOfClass:mineRoot]) return; // Dedicated visual clip; host owns all layout.
    // This is scrollable content (including a collapsing header), not a viewport.
    // Its size belongs to the host even when scrolling brings its edge into view.
    if (HGNestedScroll(view.superview) && HGView(view.superview, "contentView") == view) {
        for (UIView *child in view.subviews.copy)
            [self collectViewports:child root:root barTop:barTop bottom:bottom records:records depth:depth + 1];
        return;
    }
    CGRect actual = [view convertRect:view.bounds toView:root];
    CGFloat rootWidth = CGRectGetWidth(root.bounds), rootHeight = CGRectGetHeight(root.bounds);
    if (CGRectGetWidth(actual) < rootWidth * 0.80 || CGRectGetMinX(actual) < -2 ||
        CGRectGetMaxX(actual) > CGRectGetMaxX(root.bounds) + 2 ||
        CGRectGetMinY(actual) > barTop - 100 || CGRectGetHeight(actual) < rootHeight * 0.35) return;
    if ([view isKindOfClass:UIScrollView.class] && HGTheaterCategoryPager((UIScrollView *)view)) {
        // The Theatre's category coordinator can have pagingEnabled == NO.
        // Its native offset and insets still belong to horizontal navigation.
        if (CGRectGetMaxY(actual) < bottom - 0.75) return;
        for (UIView *child in view.subviews.copy)
            [self collectViewports:child root:root barTop:barTop bottom:bottom records:records depth:depth + 1];
        return;
    }
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
    // Viewport state is reconciled by the tab controller/bar lifecycle, never
    // by scroll offsets or a scroll view's per-frame layout callbacks.
    if ([view isKindOfClass:UIScrollView.class]) {
        UIScrollView *scroll = (UIScrollView *)view;
        // The host coordinator owns its collapsing header, pager and lists.
        // Traversing their moving coordinates causes resize/contentSize/KVO
        // feedback and prevents the host from scrolling back to its header.
        if (HGNestedScroll(scroll)) return;
        BOOL horizontalPager = scroll.pagingEnabled &&
            scroll.contentSize.width > scroll.bounds.size.width + 2;
        if (!horizontalPager && !HGContainerScroll(scroll)) {
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
        [self.videoBackdrop restore];
        HGRestoreViewports(self.viewports.objectEnumerator.allObjects);
        [self.viewports removeAllObjects];
        return;
    }
    CGRect windowRect = [bar.window convertRect:bar.window.bounds toView:root];
    CGFloat bottom = MIN(CGRectGetMaxY(root.bounds), CGRectGetMaxY(windowRect));
    CGRect barRect = [bar convertRect:bar.bounds toView:root];
    CGFloat barTop = CGRectGetMinY(barRect);
    if (HGTabBarIsInTransitionOrDisplaced(bar, self.controller) ||
        bottom - barTop < 20 || bottom - barTop > 160) {
        [self.videoBackdrop restore]; return;
    }
    NSMutableArray<HGViewportRecord *> *records = [NSMutableArray array];
    // Walk only sibling content branches below the tab bar in z-order; ignore
    // overlay/drawer siblings mounted above the tab bar.
    NSMutableArray<UIView *> *branches = [NSMutableArray array];
    UIView *path = bar;
    while (path.superview && path != root) {
        UIView *parent = path.superview;
        NSUInteger pathIndex = [parent.subviews indexOfObjectIdenticalTo:path];
        for (NSUInteger idx = 0; idx < parent.subviews.count; idx++) {
            if (idx >= pathIndex) break;
            UIView *child = parent.subviews[idx];
            if (!HGIsOverlayOrDrawerView(child)) [branches addObject:child];
        }
        path = parent;
    }
    if (self.customDock && !self.videoBackdrop) self.videoBackdrop = [HGVideoBackdrop new];
    [self.videoBackdrop refreshWithBranches:branches root:root bar:bar active:self.customDock];
    for (UIView *branch in branches) {
        if (!HGContainsVisibleVerticalPager(branch, root))
            [self collectViewports:branch root:root barTop:barTop bottom:bottom records:records depth:0];
    }
    NSHashTable<UIView *> *active = [NSHashTable weakObjectsHashTable];
    for (HGViewportRecord *record in records) [active addObject:record.view];
    NSMutableArray<HGViewportRecord *> *inactive = [NSMutableArray array];
    for (UIView *view in self.viewports.keyEnumerator.allObjects) {
        if (![active containsObject:view]) {
            [inactive addObject:[self.viewports objectForKey:view]];
            [self.viewports removeObjectForKey:view];
            HGUnobserve(view, self);
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
                scroll.contentSize.width > scroll.bounds.size.width + 2;
            if (!horizontalPager && !HGContainerScroll(scroll)) {
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
            }
            // Both the nested coordinator and its inner list can supply a
            // system bottom-edge veil. Padding still belongs only to the leaf.
            if (!horizontalPager) {
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
                if (HGContainerScroll(scroll)) {
                    id top = HGObject(scroll, "topEdgeEffect");
                    if (top != record.topEdge && [top respondsToSelector:@selector(setHidden:)] &&
                        [top respondsToSelector:sel_registerName("isHidden")]) {
                        if (record.topEdge) ((void (*)(id, SEL, BOOL))objc_msgSend)(record.topEdge,
                            @selector(setHidden:), record.topEdgeWasHidden);
                        record.topEdge = top;
                        record.topEdgeWasHidden = ((BOOL (*)(id, SEL))objc_msgSend)(top, sel_registerName("isHidden"));
                    }
                    if (record.topEdge) ((void (*)(id, SEL, BOOL))objc_msgSend)(record.topEdge, @selector(setHidden:), YES);
                }
            }
        }
        record.resized = delta > 0.75;
        if (!HGRectNear(view.frame, record.appliedFrame)) {
            view.frame = record.appliedFrame;
            if (HGNestedScroll(view)) {
                // Let the host calculate its children from the final outer
                // viewport once; do not overwrite its result afterwards.
                [view setNeedsLayout];
                [view layoutIfNeeded];
            }
        }
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
    [self.videoBackdrop restore];
    HGRestoreViewports(self.viewports.objectEnumerator.allObjects);
    for (HGTabTitleRecord *record in self.titles.objectEnumerator.allObjects) {
        [record restore]; objc_setAssociatedObject(record.label, &HGTitleRecordKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    [self.titles removeAllObjects];
    [self.viewports removeAllObjects];
    for (HGSurfaceRecord *record in self.surfaces.objectEnumerator.allObjects) [record restore];
    [self.surfaces removeAllObjects];
    for (UIView *view in self.observed.allObjects) HGUnobserve(view, self);
    [self.observed removeAllObjects];
    self.glass.effect = nil;
    self.glass.hidden = YES;
    [self.glass removeFromSuperview];
}
- (void)refresh {
    if (self.refreshing || HGFeedSidebarOwnsLayout(self.bar.window)) return;
    UIView *currentBar = self.bar;
    // Never open a disabled-actions CATransaction or mutate viewports/dock while
    // customTabBar is displaced horizontally (sidebar slide or nav push to search).
    if (HGTabBarIsInTransitionOrDisplaced(currentBar, self.controller)) {
        [self.videoBackdrop restore];
        return;
    }
    if (HGAppearanceDockIsMutating(currentBar)) {
        if (!self.refreshPending) {
            self.refreshPending = YES;
            __weak HGTabState *weak = self;
            dispatch_async(dispatch_get_main_queue(), ^{
                HGTabState *state = weak;
                state.refreshPending = NO;
                [state refresh];
            });
        }
        return;
    }
    self.refreshing = YES;
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    @try {
        UIView *bar = self.bar;
        BOOL customDock = HGAppearanceDockEnabled();
        if (customDock != self.customDock) {
            // Release the old mode once before taking new baselines. Do not
            // restore/reapply live list geometry on every custom-dock layout.
            HGAppearanceRestoreDock(bar);
            [self restore];
            self.customDock = customDock;
        }
        if (!customDock) HGAppearanceRestoreDock(bar);
        if ((!HGGlassEnabled && !customDock) || !bar.window || bar.hidden) {
            HGAppearanceRestoreDock(bar);
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
            [self.videoBackdrop restore];
            if (customDock) HGAppearanceRestoreDock(bar);
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
            [self.videoBackdrop restore];
            self.glass.hidden = YES;
            return;
        }
        // The real host can supply an edge-to-edge content container. Keep the
        // material floating even when the list's last row exposes its plain
        // background; original controls and their hit targets stay untouched.
        CGFloat left = self.bar.safeAreaInsets.left;
        CGFloat right = CGRectGetWidth(self.bar.bounds) - self.bar.safeAreaInsets.right;
        if (CGRectGetMinX(frame) < left + 8 || CGRectGetMaxX(frame) > right - 8) {
            CGFloat insetLeft = MAX(CGRectGetMinX(frame), left + 20);
            CGFloat insetRight = MIN(CGRectGetMaxX(frame), right - 20);
            if (insetRight - insetLeft >= 176) {
                frame.origin.x = insetLeft;
                frame.size.width = insetRight - insetLeft;
            }
        }
        // Drop ownership for detached/replaced content before preparing new views.
        for (UIView *view in self.surfaces.keyEnumerator.allObjects) {
            if (view != bar && ![view isDescendantOfView:bar]) {
                [[self.surfaces objectForKey:view] restore];
                [self.surfaces removeObjectForKey:view];
                HGUnobserve(view, self);
            }
        }
        [self clearBackgrounds:bar content:content frame:frame depth:0];
        const char *names[] = {"backgroundImageView", "lightView", "darkView", "lineView"};
        for (NSUInteger index = 0; index < sizeof(names) / sizeof(names[0]); index++) {
            UIView *background = HGView(bar, names[index]);
            if (!background || background == bar || background == content ||
                ![background isDescendantOfView:bar] || [content isDescendantOfView:background]) continue;
            if (!HGContainsControls(background, 0)) {
                [[self surface:background] hideBackground];
                // A host can show its old backdrop without laying out the bar.
                // Observe only these confirmed background instances so the
                // setter is corrected before the next screen transaction.
                // System effects and their private subviews remain excluded.
                HGObserveView(background, self, YES);
            } else [[self surface:background] clearFill];
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
        if (customDock) {
            // The custom capsule needs the same host-surface cleanup as the
            // original glass bar. The old early return restored a full-screen
            // host mask and left list content ending abruptly above the dock.
            self.glass.effect = nil;
            self.glass.hidden = YES;
            [self.glass removeFromSuperview];
            HGAppearanceLayoutDock(bar, self.controller);
            BOOL attached = NO;
            for (UIView *child in bar.subviews)
                if ([child.accessibilityIdentifier isEqual:@"honggo.appearance.dock"] && !child.hidden) attached = YES;
            if (attached) [self refreshViewports];
            else [self restore]; // Unmapped/too-small dock: keep native navigation visible.
            return;
        }
        [self prepareGlass:frame content:content];
        [self refreshViewports];
        NSTimeInterval now = NSProcessInfo.processInfo.systemUptime;
        if (!self.logged || (HGEnhancedDiagnostics() && now - self.lastDiagnostic >= 1)) {
            NSMutableArray *viewports = [NSMutableArray array];
            for (HGViewportRecord *record in self.viewports.objectEnumerator.allObjects) {
                if (viewports.count >= 24) break;
                NSMutableDictionary *item = [@{@"class": NSStringFromClass(record.view.class),
                    @"originalFrame": NSStringFromCGRect(record.originalFrame),
                    @"appliedFrame": NSStringFromCGRect(record.appliedFrame), @"insetOwned": @(record.insetOwned)} mutableCopy];
                if ([record.view isKindOfClass:UIScrollView.class]) {
                    UIScrollView *scroll = (UIScrollView *)record.view;
                    item[@"offset"] = NSStringFromCGPoint(scroll.contentOffset);
                    item[@"inset"] = NSStringFromUIEdgeInsets(scroll.contentInset);
                    item[@"contentSize"] = NSStringFromCGSize(scroll.contentSize);
                    item[@"nestedCoordinator"] = @(HGNestedScroll(scroll));
                    item[@"containerScroll"] = @(HGContainerScroll(scroll));
                    item[@"bottomEdgeHidden"] = @([record.edge respondsToSelector:sel_registerName("isHidden")] &&
                        ((BOOL (*)(id, SEL))objc_msgSend)(record.edge, sel_registerName("isHidden")));
                }
                item[@"frameInRoot"] = NSStringFromCGRect([record.view convertRect:record.view.bounds
                    toView:self.controller.viewIfLoaded]);
                [viewports addObject:item];
            }
            HGLog(@"tabbar.layout", @{@"material": self.nativeGlass ? @"UIGlassEffect" : @"UIBlurEffect",
                @"frame": NSStringFromCGRect(self.glass.frame), @"viewports": viewports,
                @"surfaces": @(self.surfaces.count), @"nativeConfigured": @(HGNativeGlassConfigured()),
                @"startupOverride": @(HGNativeGlassStartupOverride())});
            self.lastDiagnostic = now;
        }
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
    if (HGFeedSidebarOwnsLayout(controller.viewIfLoaded.window)) return;
    UIView *bar = HGView(controller, "customTabBar");
    if (![bar isKindOfClass:HGTabBarClass] || HGFeedSidebarOwnsLayout(bar.window)) return;
    HGAttachSettings(bar);
    if (HGGlassEnabled || HGAppearanceDockEnabled()) {
        HGTabState *state = HGState(bar);
        state.controller = controller;
        [state refresh];
    }
}
static char HGVideoBackdropDiscoveryTimeKey;

void HGRefreshVideoBackdropForFeed(UIScrollView *feed) {
    if (!NSThread.isMainThread || HGFeedSidebarOwnsLayout(feed.window)) return;
    if (HGVideoBackdropRefreshForFeed(feed) || !HGAppearanceDockEnabled() || !feed.window) return;
    if (feed.dragging || feed.decelerating ||
        !HGVideoBackdropFullWindow(feed.superview, feed.window)) return;
    NSTimeInterval now = NSProcessInfo.processInfo.systemUptime;
    NSNumber *lastAttempt = objc_getAssociatedObject(feed, &HGVideoBackdropDiscoveryTimeKey);
    if (lastAttempt && (now - lastAttempt.doubleValue) < 0.5) return;
    objc_setAssociatedObject(feed, &HGVideoBackdropDiscoveryTimeKey, @(now), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    // The feed can attach after the tab controller's first layout. Find the
    // owning bar once and use the feed's weak link on later layouts.
    UIResponder *responder = feed.nextResponder;
    while (responder && ![responder isKindOfClass:UIViewController.class]) responder = responder.nextResponder;
    for (UIViewController *controller = (UIViewController *)responder; controller; controller = controller.parentViewController) {
        UIView *bar = HGView(controller, "customTabBar");
        if ([bar isKindOfClass:HGTabBarClass]) { HGRefreshTabBar(controller); break; }
    }
}
static void HGControllerLayout(id self, SEL cmd) {
    HGOriginalControllerLayout(self, cmd);
    HGRefreshTabBar(self);
}

static UIViewController *HGContainingTabController(UIViewController *page) {
    for (UIViewController *owner = page; owner; owner = owner.parentViewController) {
        if ([HGView(owner, "customTabBar") isKindOfClass:HGTabBarClass]) return owner;
    }
    return page.tabBarController;
}
static void HGRefreshContainingTabBar(UIViewController *page) {
    if (HGFeedSidebarOwnsLayout(page.viewIfLoaded.window)) return;
    HGRefreshTabBar(HGContainingTabController(page));
}
static void HGRefreshMine(UIViewController *page) {
    if (!page.viewIfLoaded.window || HGFeedSidebarOwnsLayout(page.viewIfLoaded.window)) return;
    UIScrollView *mineScroll = (UIScrollView *)HGView(page, "scrollView");
    if ([mineScroll isKindOfClass:UIScrollView.class]) HGApplyMineClip(mineScroll);
    HGRefreshContainingTabBar(page);
}
static void HGMallMount(UIViewController *page, SEL cmd) {
    HGOriginalMallMount(page, cmd);
    if (!NSThread.isMainThread || (!HGGlassEnabled && !HGAppearanceDockEnabled())) return;
    UIView *root = page.viewIfLoaded;
    if (!root.window || HGView(page, "mallView").superview != root) return;
    // The host attaches the mall after reserving the entire native tab bar.
    // Run the existing reversible viewport pass after the initial mount/reset.
    HGRefreshContainingTabBar(page);
}
static void HGTheaterListLayout(UIView *list, SEL cmd) {
    HGOriginalTheaterListLayout(list, cmd);
    UIWindow *window = list.window;
    if (!window || list.hidden || (!HGGlassEnabled && !HGAppearanceDockEnabled())) return;
    CGRect frame = [list convertRect:list.bounds toView:window];
    if (fabs(CGRectGetMinX(frame) - CGRectGetMinX(window.bounds)) > 2 ||
        fabs(CGRectGetMaxX(frame) - CGRectGetMaxX(window.bounds)) > 2 ||
        CGRectGetMaxY(frame) >= CGRectGetMaxY(window.bounds) - 0.75) return;
    UIViewController *theater = HGTheaterPageForView(list);
    if (!theater) return;
    UIViewController *owner = HGContainingTabController(theater);
    UIView *bar = HGView(owner, "customTabBar");
    if (bar.window == window && CGRectGetMaxY(frame) >= CGRectGetMinY([bar convertRect:bar.bounds toView:window]) - 2)
        HGRefreshTabBar(owner);
}
static void HGMineLayout(UIViewController *page, SEL cmd) {
    HGOriginalMineLayout(page, cmd);
    // Compose can attach native views after this callback. Coalesce layouts
    // and reconcile on the next main-loop turn, after the host has settled.
    if (!page.viewIfLoaded.window || objc_getAssociatedObject(page, &HGMineRefreshPendingKey)) return;
    objc_setAssociatedObject(page, &HGMineRefreshPendingKey, @YES, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    __weak UIViewController *weakPage = page;
    dispatch_async(dispatch_get_main_queue(), ^{
        UIViewController *current = weakPage;
        if (!current) return;
        HGRefreshMine(current);
        objc_setAssociatedObject(current, &HGMineRefreshPendingKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    });
}
static void HGMineAppear(UIViewController *page, SEL cmd, BOOL animated) {
    HGOriginalMineAppear(page, cmd, animated);
    HGRefreshMine(page);
    HGLog(@"mine.appear", @{@"attached": @((BOOL)(page.viewIfLoaded.window != nil)),
        @"frame": NSStringFromCGRect(page.viewIfLoaded.frame)});
}
static void HGSetAppearance(id self, SEL cmd) {
    HGOriginalSetAppearance(self, cmd);
    HGRefreshTabBar(self);
}
static void HGRefreshBar(UIView *bar) {
    if (!HGFeedSidebarOwnsLayout(bar.window)) [HGState(bar) refresh];
}
static void HGResetRatio(id self, SEL cmd, double ratio) {
    HGOriginalResetRatio(self, cmd, ratio);
    HGRefreshBar(self);
}
static void HGBarLayout(id self, SEL cmd) {
    HGOriginalBarLayout(self, cmd);
    HGRefreshBar(self);
}
static void HGBarMove(id self, SEL cmd) {
    HGOriginalBarMove(self, cmd);
    HGRefreshBar(self);
}
static void HGBarAdd(id self, SEL cmd, UIView *child) {
    HGOriginalBarAdd(self, cmd, child);
    HGRefreshBar(self);
}
static void HGBarHidden(id self, SEL cmd, BOOL hidden) {
    HGOriginalBarHidden(self, cmd, hidden);
    HGRefreshBar(self);
}
static void HGBarAlpha(id self, SEL cmd, CGFloat alpha) {
    HGOriginalBarAlpha(self, cmd, alpha);
    HGRefreshBar(self);
}
static void HGBarTraits(id self, SEL cmd, id previous) {
    HGOriginalBarTraits(self, cmd, previous);
    HGRefreshBar(self);
}
static HGSidebarTransition *HGSuspendSidebarAppearance(UIViewController *page, UIWindow *window) {
    BOOL wasSuspended = HGFeedSidebarOwnsLayout(window);
    HGSidebarTransition *transition = [HGSidebarTransition new];
    transition.page = page;
    transition.pending = YES;
    objc_setAssociatedObject(window, &HGSidebarTransitionKey, transition, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    if (!wasSuspended) {
        [UIView performWithoutAnimation:^{
            UIView *bar = HGView(HGContainingTabController(page), "customTabBar");
            HGTabState *state = objc_getAssociatedObject(bar, &HGStateKey);
            HGAppearanceSuspendDockForSidebar(bar);
            [state restore];
            HGAppearanceUpdateSidebarLayout(window, YES);
        }];
    }
    return transition;
}

static void HGSettleSidebarAppearance(HGSidebarTransition *transition, UIWindow *window, double duration) {
    __weak UIWindow *weakWindow = window;
    double delay = isfinite(duration) ? MAX(0, MIN(duration, 2.0)) : 0;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(delay * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        UIWindow *window = weakWindow;
        if (!window || objc_getAssociatedObject(window, &HGSidebarTransitionKey) != transition) return;
        transition.pending = NO;
        UIViewController *page = transition.page;
        if (page.viewIfLoaded.window == window && HGSidebarIsOpen(page)) return;
        objc_setAssociatedObject(window, &HGSidebarTransitionKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        [HGView(HGContainingTabController(page), "customTabBar") setNeedsLayout];
        HGAppearanceUpdateSidebarLayout(window, NO);
    });
}

static void (*HGOriginalSidebarTap)(UIViewController *, SEL);
static void HGSidebarTap(UIViewController *page, SEL cmd) {
    UIWindow *window = NSThread.isMainThread ? page.viewIfLoaded.window : nil;
    if (!window || (!HGGlassEnabled && !HGAppearanceEnabled())) {
        HGOriginalSidebarTap(page, cmd);
        return;
    }
    HGLog(@"appearance.sidebar", @{@"phase": @"tap.enter"});
    HGSidebarTransition *transition = HGSuspendSidebarAppearance(page, window);
    HGLog(@"appearance.sidebar", @{@"phase": @"tap.native-ready"});
    @try {
        HGOriginalSidebarTap(page, cmd);
        HGLog(@"appearance.sidebar", @{@"phase": @"tap.return"});
    } @finally {
        // A nested toggle owns its actual animation duration. If the native
        // tap did not toggle (e.g. it was declined), release this handoff.
        if (objc_getAssociatedObject(window, &HGSidebarTransitionKey) == transition)
            HGSettleSidebarAppearance(transition, window, 0);
    }
}

static void (*HGOriginalToggleSidebar)(UIViewController *, SEL, BOOL, id, double);
static void HGToggleSidebar(UIViewController *page, SEL cmd, BOOL opening, id type, double duration) {
    UIWindow *window = NSThread.isMainThread ? page.viewIfLoaded.window : nil;
    if (!window || (!HGGlassEnabled && !HGAppearanceEnabled())) {
        HGOriginalToggleSidebar(page, cmd, opening, type, duration);
        return;
    }
    HGLog(@"appearance.sidebar", @{@"phase": @"toggle.enter", @"opening": @(opening)});
    HGSidebarTransition *transition = HGSuspendSidebarAppearance(page, window);
    @try {
        HGOriginalToggleSidebar(page, cmd, opening, type, duration);
        HGLog(@"appearance.sidebar", @{@"phase": @"toggle.return", @"open": @(HGSidebarIsOpen(page))});
    } @finally {
        HGSettleSidebarAppearance(transition, window, duration);
    }
}

static void HGToggleFeedSidebar(UIViewController *controller, SEL cmd, double offset) {
    if (!NSThread.isMainThread) {
        HGOriginalToggleFeedSidebar(controller, cmd, offset);
        return;
    }
    UIWindow *previous = HGSidebarMotionWindow;
    HGSidebarMotionWindow = controller.viewIfLoaded.window;
    @try {
        HGOriginalToggleFeedSidebar(controller, cmd, offset);
    } @finally {
        HGSidebarMotionWindow = previous;
    }
    // A motion without the three-argument feed entry still needs a fresh
    // layout when the host restores the tab bar to its original position.
    if (fabs(offset) <= 2.0 && !HGFeedSidebarOwnsLayout(controller.viewIfLoaded.window))
        [HGView(controller, "customTabBar") setNeedsLayout];
}

#import "MaskScene.inc"

BOOL HGInstallTabBar(void) {
    @synchronized (HGTabState.class) {
        static BOOL installed;
        if (installed) return YES;
        HGTabBarClass = objc_lookUpClass("SSTabBar");
        Class controller = objc_lookUpClass("SSTabBarController");
        if (![HGTabBarClass isSubclassOfClass:UIView.class] ||
            ![controller isSubclassOfClass:UIViewController.class]) return NO;
        Class getterTargets[] = {controller, HGTabBarClass};
        const char *getterNames[] = {"customTabBar", "contentView"};
        for (NSUInteger i = 0; i < 2; i++) {
            Method method = class_getInstanceMethod(getterTargets[i], sel_registerName(getterNames[i]));
            const char *encoding = method ? method_getTypeEncoding(method) : NULL;
            if (!encoding || strcmp(encoding, "@16@0:8") != 0) {
                HGRecordCapability([NSString stringWithFormat:@"getter.tabbar.%s", getterNames[i]],
                    @{@"module": @"tabbar", @"class": NSStringFromClass(getterTargets[i]),
                      @"selector": @(getterNames[i]), @"state": method ? @"abi-mismatch" : @"missing-method",
                      @"expected": @"@16@0:8", @"actual": encoding ? @(encoding) : @"missing"});
                return NO;
            }
        }
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
        Class feed = objc_lookUpClass("SSVideoSeriesFeedViewController");
        SEL getter = sel_registerName("isSidebarOpen");
        Method method = class_getInstanceMethod(feed, getter);
        if ([feed isSubclassOfClass:UIViewController.class] && method &&
                !strcmp(method_getTypeEncoding(method), "B16@0:8")) {
            HGSidebarOpenGetter = getter;
            HGOriginalToggleSidebar = (void (*)(UIViewController *, SEL, BOOL, id, double))HGReplaceMethod(feed,
                sel_registerName("toggleSidebar:toggleType:duration:"), "v36@0:8B16@20d28", (IMP)HGToggleSidebar);
            if (HGOriginalToggleSidebar)
                HGOriginalSidebarTap = (void (*)(UIViewController *, SEL))HGReplaceMethod(feed,
                    sel_registerName("simpleSideBarViewDidClicked"), "v16@0:8", (IMP)HGSidebarTap);
        }
        HGOriginalToggleFeedSidebar = (void (*)(id, SEL, double))HGReplaceMethod(controller,
            sel_registerName("toggleVideoSeriesFeedSideBarInAnimation:"), "v24@0:8d16", (IMP)HGToggleFeedSidebar);
        Class mall = objc_lookUpClass("FQMSEcomXTabMallViewController");
        if ([mall isSubclassOfClass:UIViewController.class])
            HGOriginalMallMount = (void (*)(id, SEL))HGReplaceMethod(mall,
                sel_registerName("mountMallViewIfNeeded"), "v16@0:8", (IMP)HGMallMount);
        Class theaterList = objc_lookUpClass("FQFeedListView");
        if ([theaterList isSubclassOfClass:UIView.class])
            HGOriginalTheaterListLayout = (void (*)(id, SEL))HGReplaceMethod(theaterList,
                @selector(layoutSubviews), "v16@0:8", (IMP)HGTheaterListLayout);
        if (HGGlassEnabled) {
            Class mine = objc_lookUpClass("SSMine737KMPViewController");
            HGOriginalMineLayout = (void (*)(id, SEL))HGReplaceMethod(mine,
                @selector(viewDidLayoutSubviews), "v16@0:8", (IMP)HGMineLayout);
            HGOriginalMineAppear = (void (*)(id, SEL, BOOL))HGReplaceMethod(mine,
                @selector(viewDidAppear:), "v20@0:8B16", (IMP)HGMineAppear);
            HGOriginalBarLayout = (void (*)(id, SEL))HGReplaceMethod(HGTabBarClass, selectors[1], encodings[1], (IMP)HGBarLayout);
            HGOriginalBarMove = (void (*)(id, SEL))HGReplaceMethod(HGTabBarClass, selectors[2], encodings[2], (IMP)HGBarMove);
            HGOriginalBarAdd = (void (*)(id, SEL, UIView *))HGReplaceMethod(HGTabBarClass, selectors[3], encodings[3], (IMP)HGBarAdd);
            HGOriginalBarHidden = (void (*)(id, SEL, BOOL))HGReplaceMethod(HGTabBarClass, @selector(setHidden:), "v20@0:8B16", (IMP)HGBarHidden);
            HGOriginalBarAlpha = (void (*)(id, SEL, CGFloat))HGReplaceMethod(HGTabBarClass, @selector(setAlpha:), "v24@0:8d16", (IMP)HGBarAlpha);
            HGOriginalBarTraits = (void (*)(id, SEL, id))HGReplaceMethod(HGTabBarClass, sel_registerName("traitCollectionDidChange:"), "v24@0:8@16", (IMP)HGBarTraits);
            HGOriginalSetAppearance = (void (*)(id, SEL))HGReplaceMethod(controller, sel_registerName("setCustomTabBarAppearance"), "v16@0:8", (IMP)HGSetAppearance);
            HGOriginalResetRatio = (void (*)(id, SEL, double))HGReplaceMethod(HGTabBarClass, sel_registerName("resetBlurVisualViewWithRatio:"), "v24@0:8d16", (IMP)HGResetRatio);
        }
        if (HGGlassEnabled) HGInstallMineClip();
        HGInstallMaskScenes();
        installed = YES;
        return YES;
    }
}
