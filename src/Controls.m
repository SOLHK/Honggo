#import "Honggo.h"
#import <QuartzCore/QuartzCore.h>
#import <objc/message.h>
#include <math.h>
#include <string.h>

static char HGControlKey, HGControlBusyKey;
static NSHashTable<UIView *> *HGStyledControls;

@interface HGControlGlass : UIVisualEffectView
@property(nonatomic, weak) UIView *host;
@property(nonatomic, strong) UIColor *originalColor;
@property(nonatomic) BOOL originalOpaque;
@property(nonatomic, strong) NSArray *backgroundImages;
@property(nonatomic) BOOL attached;
@property(nonatomic) NSTimeInterval lastTouch;
- (void)restore;
@end

@implementation HGControlGlass
- (void)restore {
    UIView *host = self.host;
    if (self.attached) {
        if (!host.backgroundColor || CGColorGetAlpha(host.backgroundColor.CGColor) < 0.001)
            host.backgroundColor = self.originalColor;
        if (!host.opaque) host.opaque = self.originalOpaque;
        if ([host isKindOfClass:UIButton.class]) {
            UIButton *button = (UIButton *)host;
            for (NSUInteger state = 0; state < self.backgroundImages.count; state++) {
                id old = self.backgroundImages[state];
                if (![button backgroundImageForState:state] && old != NSNull.null)
                    [button setBackgroundImage:old forState:state];
            }
        }
    }
    self.attached = NO;
    self.effect = nil;
    [self removeFromSuperview];
}
- (void)didMoveToWindow {
    [super didMoveToWindow];
    if (!self.window) self.effect = nil;
    else [self setNeedsLayout];
}
- (void)layoutSubviews {
    [super layoutSubviews];
    if (!self.window || self.effect || !HGNativeGlassConfigured()) return;
    // Material creation waits for final geometry and attachment.
    if (self.bounds.size.width < 20 || self.bounds.size.height < 20) return;
    Class type = NSClassFromString(@"UIGlassEffect");
    UIVisualEffect *effect = ((id (*)(id, SEL, NSInteger))objc_msgSend)(type, sel_registerName("effectWithStyle:"), 0);
    if ([effect respondsToSelector:sel_registerName("setInteractive:")])
        ((void (*)(id, SEL, BOOL))objc_msgSend)(effect, sel_registerName("setInteractive:"), YES);
    self.effect = effect;
}
- (void)touched:(UIControl *)sender {
    if (!self.attached || !HGEnhancedDiagnostics()) return;
    NSTimeInterval now = NSProcessInfo.processInfo.systemUptime;
    if (now - self.lastTouch < 0.1) return;
    self.lastTouch = now;
    HGLog(@"control.touch", @{@"class": NSStringFromClass(sender.class),
        @"owner": NSStringFromClass(sender.superview.class), @"enabled": @(sender.enabled),
        @"selected": @(sender.selected)});
}
@end

UIUserInterfaceStyle HGGlassForegroundStyle(UIView *content, UITraitCollection *traits) {
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
            else if (item.currentImage.renderingMode == UIImageRenderingModeAlwaysTemplate) foreground = item.tintColor;
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

void HGStyleControl(UIView *control) {
    if (!control) return;
    HGControlGlass *glass = objc_getAssociatedObject(control, &HGControlKey);
    if (objc_getAssociatedObject(control, &HGControlBusyKey)) return;
    objc_setAssociatedObject(control, &HGControlBusyKey, @YES, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    @try {
        BOOL eligible = HGIsEnabled(HGGlassControls) && HGNativeGlassConfigured() && control.window &&
            !control.hidden && control.alpha >= 0.999 &&
            control.bounds.size.width >= 20 && control.bounds.size.height >= 20 &&
            control.bounds.size.height <= 100 && control.bounds.size.width <= 500 &&
            !control.layer.mask && CGAffineTransformIsIdentity(control.transform) &&
            !([control isKindOfClass:UIButton.class] && ((UIButton *)control).configuration);
        if (!eligible) { [glass restore]; return; }
        if (!glass) {
            glass = [[HGControlGlass alloc] initWithEffect:nil];
            glass.host = control;
            glass.userInteractionEnabled = NO;
            glass.accessibilityElementsHidden = YES;
            glass.accessibilityIdentifier = @"honggo.control.glass";
            glass.backgroundColor = UIColor.clearColor;
            glass.contentView.backgroundColor = UIColor.clearColor;
            glass.opaque = NO;
            Class corners = NSClassFromString(@"UICornerConfiguration");
            SEL factory = sel_registerName("capsuleConfiguration");
            SEL setter = sel_registerName("setCornerConfiguration:");
            if ([corners respondsToSelector:factory] && [glass respondsToSelector:setter])
                ((void (*)(id, SEL, id))objc_msgSend)(glass, setter, ((id (*)(id, SEL))objc_msgSend)(corners, factory));
            objc_setAssociatedObject(control, &HGControlKey, glass, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            if (!HGStyledControls) HGStyledControls = [NSHashTable weakObjectsHashTable];
            [HGStyledControls addObject:control];
            if ([control isKindOfClass:UIControl.class])
                [(UIControl *)control addTarget:glass action:@selector(touched:) forControlEvents:UIControlEventTouchDown];
            HGLog(@"control.glass", @{@"class": NSStringFromClass(control.class),
                @"owner": NSStringFromClass(control.superview.class)});
        }
        [CATransaction begin];
        [CATransaction setDisableActions:YES];
        if (!glass.attached || (control.backgroundColor && CGColorGetAlpha(control.backgroundColor.CGColor) > 0.001))
            glass.originalColor = control.backgroundColor;
        if (!glass.attached || control.opaque) glass.originalOpaque = control.opaque;
        control.backgroundColor = UIColor.clearColor;
        control.opaque = NO;
        if ([control isKindOfClass:UIButton.class]) {
            UIButton *button = (UIButton *)control;
            // Configured buttons own their background through UIKit; do not override that system.
            if (button.configuration) { [glass restore]; [CATransaction commit]; return; }
            NSMutableArray *images = glass.backgroundImages ? glass.backgroundImages.mutableCopy : [NSMutableArray array];
            for (NSUInteger state = 0; state < 8; state++) {
                UIImage *image = [button backgroundImageForState:state];
                if (images.count <= state) [images addObject:image ?: (id)NSNull.null];
                else if (image) images[state] = image;
                if (image) [button setBackgroundImage:nil forState:state];
            }
            glass.backgroundImages = images;
        }
        glass.attached = YES;
        glass.frame = control.bounds;
        glass.overrideUserInterfaceStyle = HGGlassForegroundStyle(control, control.traitCollection);
        glass.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        if (glass.superview != control) [control insertSubview:glass atIndex:0];
        else [control sendSubviewToBack:glass];
        [glass setNeedsLayout];
        [glass layoutIfNeeded];
        [CATransaction commit];
    } @finally {
        objc_setAssociatedObject(control, &HGControlBusyKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
}

// Verified getter names in Eggplant 7.4.3.33. Empty string means the view itself
// is a single gesture-backed action, not an arbitrary content container.
static const struct { const char *className; const char *getters; } HGControlTargets[] = {
    {"SS703MineHistoryRightView", "searchButton moreButton"},
    {"SSMineHistoryActionControlsView", "sortButton managementButton"},
    {"SSModalDefaultNavigationBar", "leftButton rightButton"},
    {"SSCharacterProfileNavBarView", "backButton shareButton publishButton followButton"},
    {"SSCategoryContainerNavigationBar", "backView"},
    {"SSSearchCategoryNavigationBar", "backView"},
    {"SS511ProfileEditButtonView", ""},
    {"HGToolBarView", "moreButton closeButton gameCenterButton"},
    {"SSMyUserCustomHeaderBar", "leftBtn followButton moreBtn shareBtn voteEntranceBtn"},
    {"SSShortVideoRightInteractionMoreView", "moreButton"},
    {"FQVGVideoGameTopBar", "backButton expandButton replayButton"},
    {"SSShortVideoDetailSeriesViewController", "backButton moreButton shareButton"},
    {"SSMyUser621SideBarViewController", "settingButton"},
    {"SSMyUserFollowUpdateTabViewController", "editButton"},
    {"SSMyUser637VideoSubViewController", "editButton"},
    {"SSMyUserSubscribeSubViewController", "editButton"},
    {"SSVideoTabViewController", "editButton"},
    {"SSVideoSeries649RecentViewController", "editButton"},
    {"SSVideoCollectHeaderCell", "editButton"},
    {"SSVideoSeriesFeedRecentHeader", "editButton"},
};

static void HGRefreshControls(id owner, NSArray<NSString *> *getters) {
    UIView *root = [owner isKindOfClass:UIViewController.class] ? [(UIViewController *)owner viewIfLoaded] : owner;
    if (!root) return;
    if (getters.count == 0) { HGStyleControl(owner); return; }
    for (NSString *name in getters) {
        SEL selector = NSSelectorFromString(name);
        Method method = class_getInstanceMethod([owner class], selector);
        if (!method || strcmp(method_getTypeEncoding(method), "@16@0:8")) continue;
        id value = ((id (*)(id, SEL))objc_msgSend)(owner, selector);
        if (![value isKindOfClass:UIView.class] || ![value isDescendantOfView:root]) continue;
        HGStyleControl(value);
    }
}
void HGInstallControls(void) {
    static BOOL installed;
    if (installed) return;
    installed = YES;
    for (NSUInteger i = 0; i < sizeof(HGControlTargets) / sizeof(HGControlTargets[0]); i++) {
        Class cls = objc_lookUpClass(HGControlTargets[i].className);
        BOOL controller = [cls isSubclassOfClass:UIViewController.class];
        if (!controller && ![cls isSubclassOfClass:UIView.class]) continue;
        NSString *names = @(HGControlTargets[i].getters);
        NSArray *getters = names.length ? [names componentsSeparatedByString:@" "] : @[];
        NSArray *lifecycle = controller ? @[@"viewDidLayoutSubviews"] : @[@"layoutSubviews", @"didMoveToWindow"];
        for (NSString *name in lifecycle) {
            SEL selector = NSSelectorFromString(name);
            Method method = class_getInstanceMethod(cls, selector);
            if (!method || strcmp(method_getTypeEncoding(method), "v16@0:8")) continue;
            void (*original)(id, SEL) = (void (*)(id, SEL))method_getImplementation(method);
            IMP hook = imp_implementationWithBlock(^(id view) {
                original(view, selector);
                HGRefreshControls(view, getters);
            });
            class_replaceMethod(cls, selector, hook, "v16@0:8");
        }
        HGLog(@"controls.hook", @{@"class": NSStringFromClass(cls), @"getters": getters});
    }
}

NSDictionary *HGInterfaceSnapshot(void) {
    // Main-thread, bounded structural snapshot. No titles, images or accessibility text.
    NSMutableArray *nodes = [NSMutableArray array];
    NSMutableArray<UIView *> *queue = [NSMutableArray array];
    for (UIScene *scene in UIApplication.sharedApplication.connectedScenes) {
        if (![scene isKindOfClass:UIWindowScene.class] || scene.activationState != UISceneActivationStateForegroundActive) continue;
        for (UIWindow *window in ((UIWindowScene *)scene).windows) if (!window.hidden) [queue addObject:window];
    }
    for (NSUInteger i = 0; i < queue.count && i < 200; i++) {
        UIView *view = queue[i];
        NSMutableDictionary *node = [@{@"class": NSStringFromClass(view.class),
            @"frame": NSStringFromCGRect(view.frame), @"bounds": NSStringFromCGRect(view.bounds),
            @"hidden": @(view.hidden), @"alpha": @(view.alpha), @"clips": @(view.clipsToBounds),
            @"mask": @(view.layer.mask != nil), @"opaque": @(view.opaque)} mutableCopy];
        if ([view isKindOfClass:UIVisualEffectView.class])
            node[@"effect"] = NSStringFromClass(((UIVisualEffectView *)view).effect.class) ?: @"none";
        if ([view isKindOfClass:UIScrollView.class]) {
            UIScrollView *scroll = (UIScrollView *)view;
            node[@"offset"] = NSStringFromCGPoint(scroll.contentOffset);
            node[@"contentSize"] = NSStringFromCGSize(scroll.contentSize);
            node[@"inset"] = NSStringFromUIEdgeInsets(scroll.contentInset);
            node[@"adjustedInset"] = NSStringFromUIEdgeInsets(scroll.adjustedContentInset);
        }
        [nodes addObject:node];
        // Avoid reading content cells and system material internals.
        if (![view isKindOfClass:UITableViewCell.class] && ![view isKindOfClass:UICollectionViewCell.class] &&
            ![view isKindOfClass:UIVisualEffectView.class] && queue.count < 200) {
            for (UIView *child in view.subviews) { if (queue.count >= 200) break; [queue addObject:child]; }
        }
    }
    NSMutableArray *surfaces = [NSMutableArray array];
    for (UIView *view in HGStyledControls.allObjects) {
        if (surfaces.count >= 100) break;
        HGControlGlass *glass = objc_getAssociatedObject(view, &HGControlKey);
        [surfaces addObject:@{@"class": NSStringFromClass(view.class), @"attached": @(glass.attached),
            @"effect": NSStringFromClass(glass.effect.class) ?: @"none"}];
    }
    return @{@"nativeGlassConfigured": @(HGNativeGlassConfigured()),
        @"startupGlassOverride": @(HGNativeGlassStartupOverride()),
        @"compatibility": [NSBundle.mainBundle objectForInfoDictionaryKey:@"UIDesignRequiresCompatibility"] ?: NSNull.null,
        @"reduceTransparency": @(UIAccessibilityIsReduceTransparencyEnabled()),
        @"increaseContrast": @(UIAccessibilityDarkerSystemColorsEnabled()),
        @"nodes": nodes, @"truncated": @(queue.count >= 200), @"controls": surfaces};
}
