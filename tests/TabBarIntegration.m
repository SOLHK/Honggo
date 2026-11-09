// Actual UIKit behavior under a controlled SSTabBar host. This is not the Hongguo IPA.
#import "Honggo.h"
#import <CoreFoundation/CoreFoundation.h>
#import <QuartzCore/QuartzCore.h>
#import <dispatch/dispatch.h>
#import <objc/message.h>
#include <math.h>

NSArray<NSDictionary *> *HGRunControlTests(UIView *root);
@interface SSNestedContainerScrollView : UIScrollView
@property(nonatomic, strong) UIView *contentView;
@end
@implementation SSNestedContainerScrollView
@end

// The profile coordinator is a sibling, NOT an SSNestedContainerScrollView
// subclass. This is the hierarchy in the 7.4.3.33 host and the device log.
@interface SSMyUser637NestedContainerScrollView : UIScrollView
@property(nonatomic, strong) UIView *contentView;
@end
@implementation SSMyUser637NestedContainerScrollView
@end

// Instrument only our plugin's effect subclass in this test executable. Keep
// the production setter implementation intact and observe real assignments;
// UIVisualEffectView.effect is not required to preserve getter object identity.
static void (*HGOriginalEffectSetter)(id, SEL, UIVisualEffect *);
static char HGEffectSetterCountKey, HGEffectNonNilSetterCountKey, HGEffectSetterFactsKey;
static BOOL HGEffectSpyInstalled;

static NSDictionary *HGColorFacts(UIColor *color, UITraitCollection *traits) {
    if (!color) return @{@"description": @"nil", @"resolvedAlpha": @0, @"rgba": NSNull.null};
    UIColor *resolved = [color resolvedColorWithTraitCollection:traits];
    CGFloat red = 0, green = 0, blue = 0, alpha = 0;
    BOOL rgb = [resolved getRed:&red green:&green blue:&blue alpha:&alpha];
    return @{
        @"description": color.description ?: @"nil",
        @"resolvedAlpha": @(CGColorGetAlpha(resolved.CGColor)),
        @"rgba": rgb ? @[@(red), @(green), @(blue), @(alpha)] : (id)NSNull.null
    };
}

static NSDictionary *HGSurfaceColorFacts(UIVisualEffectView *view) {
    UIView *content = view.contentView;
    return @{
        @"contentViewAddress": [NSString stringWithFormat:@"%p", (__bridge void *)content],
        @"background": HGColorFacts(view.backgroundColor, view.traitCollection),
        @"contentBackground": HGColorFacts(content.backgroundColor, content.traitCollection),
        @"opaque": @(view.opaque),
        @"contentOpaque": @(content.opaque)
    };
}

static NSUInteger HGEffectSetterCount(UIVisualEffectView *view, BOOL nonNilOnly) {
    const void *key = nonNilOnly ? &HGEffectNonNilSetterCountKey : &HGEffectSetterCountKey;
    return [objc_getAssociatedObject(view, key) unsignedIntegerValue];
}

static void HGSpySetEffect(UIVisualEffectView *view, SEL selector, UIVisualEffect *effect) {
    // Only inspect an attached, fully initialized view. The nil assignment in
    // initWithEffect: is counted too, but must not trigger early UIKit getters.
    NSDictionary *before = effect && view.window ? HGSurfaceColorFacts(view) : nil;
    objc_setAssociatedObject(view, &HGEffectSetterCountKey,
        @(HGEffectSetterCount(view, NO) + 1), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    if (effect) {
        objc_setAssociatedObject(view, &HGEffectNonNilSetterCountKey,
            @(HGEffectSetterCount(view, YES) + 1), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    HGOriginalEffectSetter(view, selector, effect);
    if (before) {
        objc_setAssociatedObject(view, &HGEffectSetterFactsKey, @{
            @"effectClass": NSStringFromClass(effect.class),
            @"before": before,
            @"after": HGSurfaceColorFacts(view)
        }, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
}

static BOOL HGInstallEffectSpy(void) {
    Class target = NSClassFromString(@"HGGlassBackdrop");
    Method method = class_getInstanceMethod(target, @selector(setEffect:));
    if (!target || ![target isSubclassOfClass:UIVisualEffectView.class] || !method) return NO;
    HGOriginalEffectSetter = (void (*)(id, SEL, UIVisualEffect *))method_getImplementation(method);
    class_replaceMethod(target, @selector(setEffect:), (IMP)HGSpySetEffect, method_getTypeEncoding(method));
    HGEffectSpyInstalled = YES;
    return YES;
}

static BOOL HGColorIsClear(UIColor *color, UITraitCollection *traits) {
    return !color || CGColorGetAlpha([color resolvedColorWithTraitCollection:traits].CGColor) < 0.01;
}

static BOOL HGPublicSurfaceIsClear(UIVisualEffectView *view) {
    return view && !view.opaque &&
        HGColorIsClear(view.backgroundColor, view.traitCollection) &&
        HGColorIsClear(view.contentView.backgroundColor, view.contentView.traitCollection);
}

static NSDictionary *HGSerializedBooleanTypes(NSDictionary *values, NSArray<NSString *> *keys) {
    NSData *data = [NSJSONSerialization dataWithJSONObject:values options:0 error:nil];
    NSDictionary *decoded = data ? [NSJSONSerialization JSONObjectWithData:data options:0 error:nil] : nil;
    NSMutableDictionary *types = [NSMutableDictionary dictionary];
    for (NSString *key in keys) {
        id value = decoded[key];
        BOOL boolean = [value isKindOfClass:NSNumber.class] &&
            CFGetTypeID((__bridge CFTypeRef)value) == CFBooleanGetTypeID();
        types[key] = boolean ? @"boolean" : @"not-boolean";
    }
    return types;
}

static UIVisualEffect *HGReferenceEffect(NSInteger style) {
    Class glassClass = NSClassFromString(@"UIGlassEffect");
    SEL factory = sel_registerName("effectWithStyle:");
    return [glassClass respondsToSelector:factory] ?
        ((id (*)(id, SEL, NSInteger))objc_msgSend)(glassClass, factory, style) :
        [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemMaterial];
}

static NSDictionary *HGMaterialFacts(UIVisualEffectView *view) {
    UIVisualEffect *first = view.effect;
    UIVisualEffect *second = view.effect;
    UIColor *background = view.backgroundColor;
    UIColor *contentBackground = view.contentView.backgroundColor;
    return @{
        @"viewClass": view ? NSStringFromClass(object_getClass(view)) : @"none",
        @"viewAddress": [NSString stringWithFormat:@"%p", (__bridge void *)view],
        @"effectClass": first ? NSStringFromClass(first.class) : @"none",
        @"firstGetterAddress": [NSString stringWithFormat:@"%p", (__bridge void *)first],
        @"secondGetterAddress": [NSString stringWithFormat:@"%p", (__bridge void *)second],
        @"consecutiveGetterIdentityEqual": @((BOOL)(first == second)),
        @"effectSetterCalls": @(HGEffectSetterCount(view, NO)),
        @"nonNilEffectSetterCalls": @(HGEffectSetterCount(view, YES)),
        @"backgroundColor": background.description ?: @"nil",
        @"backgroundClear": @(HGColorIsClear(background, view.traitCollection)),
        @"contentBackgroundColor": contentBackground.description ?: @"nil",
        @"contentBackgroundClear": @(HGColorIsClear(contentBackground, view.traitCollection)),
        @"opaque": @(view.opaque),
        @"contentOpaque": @(view.contentView.opaque),
        @"clipsToBounds": @(view.clipsToBounds),
        @"cornerRadius": @(view.layer.cornerRadius),
        @"alpha": @(view.alpha),
        @"frame": NSStringFromCGRect(view.frame),
        @"framePoints": @[@(view.frame.origin.x), @(view.frame.origin.y),
            @(view.frame.size.width), @(view.frame.size.height)],
        @"style": @(view.traitCollection.userInterfaceStyle),
        @"overrideStyle": @(view.overrideUserInterfaceStyle),
        @"windowAddress": [NSString stringWithFormat:@"%p", (__bridge void *)view.window],
        @"surfaceColors": HGSurfaceColorFacts(view),
        @"mostRecentAttachedNonNilAssignment": objc_getAssociatedObject(view, &HGEffectSetterFactsKey) ?: @{},
        @"reduceTransparency": @(UIAccessibilityIsReduceTransparencyEnabled()),
        @"darkerSystemColors": @(UIAccessibilityDarkerSystemColorsEnabled()),
        @"reduceMotion": @(UIAccessibilityIsReduceMotionEnabled())
    };
}

static NSDictionary *HGMaterialFactsInWindow(UIVisualEffectView *view, UIWindow *window) {
    NSMutableDictionary *facts = [HGMaterialFacts(view) mutableCopy];
    CGRect frame = [view convertRect:view.bounds toView:window];
    facts[@"framePoints"] = @[@(frame.origin.x), @(frame.origin.y),
        @(frame.size.width), @(frame.size.height)];
    facts[@"samplingCoordinateSpace"] = @"window";
    return facts;
}

static BOOL HGBackgroundPathIsClear(UIView *view, UIView *last) {
    for (UIView *current = view; current; current = current.superview) {
        if (!HGColorIsClear(current.backgroundColor, current.traitCollection) || current.opaque) {
            return NO;
        }
        if (current == last) {
            return YES;
        }
    }
    return NO;
}

static BOOL HGEffectivelyHidden(UIView *view) {
    if (!view || !view.window) {
        return YES;
    }
    for (UIView *current = view; current; current = current.superview) {
        if (current.hidden || current.alpha < 0.01) {
            return YES;
        }
    }
    return NO;
}

static void HGCollectEffects(UIView *view, NSMutableArray<UIVisualEffectView *> *effects) {
    // Do not inspect UIKit's private backdrop descendants.
    for (UIView *child in view.subviews) {
        if ([child isKindOfClass:UIVisualEffectView.class]) {
            [effects addObject:(UIVisualEffectView *)child];
        } else {
            HGCollectEffects(child, effects);
        }
    }
}

static NSArray<UIVisualEffectView *> *HGEffects(UIView *view) {
    NSMutableArray<UIVisualEffectView *> *effects = [NSMutableArray array];
    HGCollectEffects(view, effects);
    return effects;
}

static BOOL HGIsBelowContent(UIVisualEffectView *effect, UIView *content) {
    if (!effect || !content) {
        return NO;
    }
    for (UIView *ancestor = effect.superview; ancestor; ancestor = ancestor.superview) {
        if (![content isDescendantOfView:ancestor]) {
            continue;
        }
        UIView *effectBranch = effect;
        UIView *contentBranch = content;
        while (effectBranch.superview && effectBranch.superview != ancestor) {
            effectBranch = effectBranch.superview;
        }
        while (contentBranch.superview && contentBranch.superview != ancestor) {
            contentBranch = contentBranch.superview;
        }
        return effectBranch != contentBranch &&
            [ancestor.subviews indexOfObjectIdenticalTo:effectBranch] <
            [ancestor.subviews indexOfObjectIdenticalTo:contentBranch];
    }
    return NO;
}

static BOOL HGFullEffectAlpha(UIVisualEffectView *effect) {
    if (!effect || !effect.window) {
        return NO;
    }
    for (UIView *view = effect; view; view = view.superview) {
        if (fabs(view.alpha - 1.0) > 0.001) {
            return NO;
        }
    }
    return YES;
}

static UIImage *HGWhiteImage(void) {
    UIGraphicsImageRenderer *renderer = [[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(2, 2)];
    return [renderer imageWithActions:^(UIGraphicsImageRendererContext *context) {
        [UIColor.whiteColor setFill];
        UIRectFill(CGRectMake(0, 0, 2, 2));
    }];
}

@interface HGPatternView : UIView
@property(nonatomic) BOOL verticalReferencePattern;
@end

@implementation HGPatternView
- (void)drawRect:(CGRect)rect {
    NSArray<UIColor *> *colors = @[
        [UIColor colorWithRed:0.10 green:0.47 blue:0.73 alpha:1],
        [UIColor colorWithRed:0.98 green:0.67 blue:0.27 alpha:1],
        [UIColor colorWithRed:0.18 green:0.63 blue:0.47 alpha:1],
        [UIColor colorWithRed:0.75 green:0.31 blue:0.42 alpha:1]
    ];
    if (self.verticalReferencePattern) {
        CGFloat width = self.bounds.size.width;
        for (NSUInteger index = 0; index < 8; index++) {
            [colors[index % colors.count] setFill];
            UIRectFill(CGRectMake(index * width / 8, 0, width / 8, self.bounds.size.height));
            [UIColor.whiteColor setFill];
            UIRectFill(CGRectMake(index * width / 8, 0, 3, self.bounds.size.height));
        }
        return;
    }
    CGFloat rowHeight = 96;
    for (NSInteger row = 0; row * rowHeight < self.bounds.size.height; row++) {
        [colors[(NSUInteger)row % colors.count] setFill];
        UIRectFill(CGRectMake(0, row * rowHeight, self.bounds.size.width, rowHeight));
        NSString *text = [NSString stringWithFormat:@"模拟列表内容 %ld", (long)row + 1];
        [text drawAtPoint:CGPointMake(22, row * rowHeight + 30) withAttributes:@{
            NSFontAttributeName: [UIFont systemFontOfSize:22 weight:UIFontWeightSemibold],
            NSForegroundColorAttributeName: UIColor.whiteColor
        }];
    }
}
@end

// Names and signatures model the supported host ABI. Layer topology is deliberately
// nested so a replacement that only hides direct bar children cannot pass.
@interface SSTabBar : UIView
@property(nonatomic, strong) UIView *contentView;
@property(nonatomic, strong) UIImageView *backgroundImageView;
@property(nonatomic, strong) UIView *lightView;
@property(nonatomic, strong) UIView *darkView;
@property(nonatomic, strong) UIView *lineView;
@property(nonatomic, strong) UIView *shell;
@property(nonatomic, strong) NSArray<UIButton *> *items;
@property(nonatomic, strong) UIView *badge;
@property(nonatomic, strong) UIView *gestureItem;
@property(nonatomic, strong) UILabel *gestureTitle;
@property(nonatomic, strong) UIView *gestureBadge;
@property(nonatomic) BOOL mediaAppearance;
@property(nonatomic) NSUInteger originalLayouts;
@property(nonatomic) NSUInteger originalRatios;
@property(nonatomic) NSUInteger originalWindowMoves;
@property(nonatomic) NSUInteger taps;
@property(nonatomic) NSInteger selectedIndex;
- (void)writeHostAppearance;
- (void)resetBlurVisualViewWithRatio:(double)ratio;
@end

@implementation SSTabBar
- (instancetype)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        _shell = [[UIView alloc] initWithFrame:self.bounds];
        _shell.accessibilityIdentifier = @"host.tab.shell";
        [self addSubview:_shell];
        _backgroundImageView = [[UIImageView alloc] initWithImage:HGWhiteImage()];
        _backgroundImageView.accessibilityIdentifier = @"host.tab.image";
        _lightView = [UIView new];
        _darkView = [UIView new];
        _lineView = [UIView new];
        [_shell addSubview:_backgroundImageView];
        [_shell addSubview:_lightView];
        [_shell addSubview:_darkView];
        [_shell addSubview:_lineView];
        _contentView = [UIView new];
        _contentView.accessibilityIdentifier = @"host.tab.content";
        [_shell addSubview:_contentView];
        NSMutableArray<UIButton *> *items = [NSMutableArray array];
        NSArray<NSString *> *titles = @[@"首页", @"剧场", @"商城", @"我的"];
        for (NSUInteger index = 0; index < titles.count; index++) {
            UIButton *button = [UIButton buttonWithType:UIButtonTypeCustom];
            button.tag = (NSInteger)index;
            button.accessibilityIdentifier = [NSString stringWithFormat:@"host.tab.item.%lu", (unsigned long)index];
            [button setTitle:titles[index] forState:UIControlStateNormal];
            button.titleLabel.font = [UIFont systemFontOfSize:17 weight:UIFontWeightSemibold];
            [button addTarget:self action:@selector(selectTab:) forControlEvents:UIControlEventTouchUpInside];
            [_contentView addSubview:button];
            [items addObject:button];
        }
        _items = items;
        _badge = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 7, 7)];
        _badge.backgroundColor = UIColor.systemOrangeColor;
        _badge.layer.cornerRadius = 3.5;
        _badge.userInteractionEnabled = NO;
        _badge.accessibilityIdentifier = @"host.tab.badge";
        [_items.lastObject addSubview:_badge];
        // The real host can implement a tab as UIView + recognizer rather than
        // UIControl. A blanket clear of every plain content descendant erases
        // this item's badge even though the UIButton case survives.
        _items[2].hidden = YES;
        _gestureItem = [UIView new];
        _gestureItem.accessibilityIdentifier = @"host.tab.gesture.item";
        _gestureItem.isAccessibilityElement = YES;
        _gestureItem.accessibilityLabel = @"商城";
        _gestureItem.accessibilityTraits = UIAccessibilityTraitButton;
        [_gestureItem addGestureRecognizer:[[UITapGestureRecognizer alloc]
            initWithTarget:self action:@selector(selectGestureTab:)]];
        _gestureTitle = [UILabel new];
        _gestureTitle.text = @"商城";
        _gestureTitle.textAlignment = NSTextAlignmentCenter;
        _gestureTitle.font = [UIFont systemFontOfSize:17 weight:UIFontWeightSemibold];
        [_gestureItem addSubview:_gestureTitle];
        _gestureBadge = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 7, 7)];
        _gestureBadge.backgroundColor = UIColor.systemOrangeColor;
        _gestureBadge.layer.cornerRadius = 3.5;
        _gestureBadge.userInteractionEnabled = NO;
        _gestureBadge.accessibilityIdentifier = @"host.tab.gesture.badge";
        [_gestureItem addSubview:_gestureBadge];
        [_contentView addSubview:_gestureItem];
        [self updateHostFrames];
        [self writeHostAppearance];
    }
    return self;
}
- (void)updateHostFrames {
    self.shell.frame = self.bounds;
    CGRect contentFrame = CGRectMake(16, 6, MAX(0, self.bounds.size.width - 32), 52);
    self.contentView.frame = contentFrame;
    self.backgroundImageView.frame = contentFrame;
    self.lightView.frame = contentFrame;
    self.darkView.frame = contentFrame;
    self.lineView.frame = CGRectMake(0, 0, self.bounds.size.width, 0.5);
    CGFloat width = self.contentView.bounds.size.width / self.items.count;
    for (NSUInteger index = 0; index < self.items.count; index++) {
        self.items[index].frame = CGRectMake(index * width, 0, width, self.contentView.bounds.size.height);
    }
    self.badge.frame = CGRectMake(MAX(0, width * 0.73), 11, 7, 7);
    self.gestureItem.frame = CGRectMake(2 * width, 0, width, self.contentView.bounds.size.height);
    self.gestureTitle.frame = self.gestureItem.bounds;
    self.gestureBadge.frame = CGRectMake(MAX(0, width * 0.73), 11, 7, 7);
}
- (void)writeHostAppearance {
    UIColor *background = self.mediaAppearance ? UIColor.blackColor : UIColor.whiteColor;
    self.backgroundColor = background;
    self.opaque = YES;
    self.shell.backgroundColor = background;
    self.shell.opaque = YES;
    self.contentView.backgroundColor = background;
    self.contentView.opaque = YES;
    self.backgroundImageView.hidden = NO;
    self.lightView.backgroundColor = UIColor.whiteColor;
    self.darkView.backgroundColor = UIColor.blackColor;
    self.lightView.hidden = self.mediaAppearance;
    self.darkView.hidden = !self.mediaAppearance;
    self.lineView.backgroundColor = UIColor.lightGrayColor;
    self.lineView.hidden = NO;
    for (NSUInteger index = 0; index < self.items.count; index++) {
        UIButton *button = self.items[index];
        button.selected = (NSInteger)index == self.selectedIndex;
        UIColor *color = self.mediaAppearance ? UIColor.whiteColor : UIColor.blackColor;
        if (!button.selected) {
            color = [color colorWithAlphaComponent:0.62];
        }
        [button setTitleColor:color forState:UIControlStateNormal];
        [button setTitleColor:color forState:UIControlStateSelected];
    }
    UIColor *gestureColor = self.mediaAppearance ? UIColor.whiteColor : UIColor.blackColor;
    self.gestureTitle.textColor = self.selectedIndex == 2 ? gestureColor :
        [gestureColor colorWithAlphaComponent:0.62];
}
- (void)selectTab:(UIButton *)sender {
    self.taps++;
    self.selectedIndex = sender.tag;
    [self writeHostAppearance];
}
- (void)selectGestureTab:(UITapGestureRecognizer *)gesture {
    self.taps++;
    self.selectedIndex = 2;
    [self writeHostAppearance];
}
- (void)layoutSubviews {
    [super layoutSubviews];
    self.originalLayouts++;
    [self updateHostFrames];
    [self writeHostAppearance];
}
- (void)didMoveToWindow {
    [super didMoveToWindow];
    self.originalWindowMoves++;
}
- (void)resetBlurVisualViewWithRatio:(double)ratio {
    self.originalRatios++;
    [self writeHostAppearance];
}
@end

@interface SSTabBarController : UIViewController
@property(nonatomic, strong) SSTabBar *customTabBar;
@property(nonatomic, strong) UIView *viewport;
@property(nonatomic, strong) UIScrollView *primaryFeed;
@property(nonatomic, strong) UIScrollView *carousel;
@property(nonatomic, strong) HGPatternView *pattern;
@property(nonatomic, strong) UILabel *mediaTitle;
@property(nonatomic) CGFloat fractionalEdge;
@property(nonatomic) BOOL mediaAppearance;
@property(nonatomic) NSUInteger originalLayouts;
@property(nonatomic) NSUInteger originalAppearances;
- (void)setCustomTabBarAppearance;
@end

@implementation SSTabBarController
- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = UIColor.whiteColor;
    self.viewport = [UIView new];
    self.viewport.backgroundColor = UIColor.whiteColor;
    self.viewport.clipsToBounds = YES;
    self.viewport.accessibilityIdentifier = @"host.main.viewport";
    self.primaryFeed = [UIScrollView new];
    self.primaryFeed.accessibilityIdentifier = @"host.main.feed";
    self.primaryFeed.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentNever;
    self.primaryFeed.contentInset = UIEdgeInsetsMake(0, 0, 12, 0);
    self.primaryFeed.verticalScrollIndicatorInsets = UIEdgeInsetsMake(0, 0, 9, 0);
    self.pattern = [HGPatternView new];
    self.pattern.accessibilityIdentifier = @"host.feed.artwork";
    self.pattern.backgroundColor = UIColor.systemTealColor;
    [self.primaryFeed addSubview:self.pattern];
    self.carousel = [[UIScrollView alloc] initWithFrame:CGRectMake(20, 130, 260, 80)];
    self.carousel.contentSize = CGSizeMake(800, 80);
    self.carousel.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentNever;
    self.carousel.contentInset = UIEdgeInsetsMake(0, 3, 4, 5);
    self.carousel.backgroundColor = UIColor.systemIndigoColor;
    self.carousel.accessibilityIdentifier = @"host.feed.horizontal.carousel";
    [self.primaryFeed addSubview:self.carousel];
    [self.viewport addSubview:self.primaryFeed];
    [self.view addSubview:self.viewport];
    self.mediaTitle = [UILabel new];
    self.mediaTitle.text = @"视频页 · 模拟宿主";
    self.mediaTitle.textColor = UIColor.whiteColor;
    self.mediaTitle.textAlignment = NSTextAlignmentCenter;
    self.mediaTitle.font = [UIFont systemFontOfSize:24 weight:UIFontWeightSemibold];
    [self.view addSubview:self.mediaTitle];
    self.customTabBar = [[SSTabBar alloc] initWithFrame:CGRectMake(0, 0, self.view.bounds.size.width, 88)];
    [self.view addSubview:self.customTabBar];
}
- (void)viewDidLayoutSubviews {
    [super viewDidLayoutSubviews];
    self.originalLayouts++;
    CGFloat width = self.view.bounds.size.width;
    CGFloat height = self.view.bounds.size.height;
    CGFloat barTop = height - 88;
    self.customTabBar.frame = CGRectMake(0, barTop, width, 88);
    // The host deliberately reintroduces a short viewport on each layout.
    self.viewport.frame = CGRectMake(0, 0, width, barTop + self.fractionalEdge);
    self.primaryFeed.frame = self.viewport.bounds;
    self.primaryFeed.contentSize = CGSizeMake(width, height * 2.4);
    self.pattern.frame = CGRectMake(0, 0, width, self.primaryFeed.contentSize.height);
    self.mediaTitle.frame = CGRectMake(0, height * 0.4, width, 60);
    self.customTabBar.mediaAppearance = self.mediaAppearance;
    self.view.backgroundColor = self.mediaAppearance ? UIColor.blackColor : UIColor.whiteColor;
    self.viewport.backgroundColor = self.view.backgroundColor;
    self.primaryFeed.backgroundColor = self.view.backgroundColor;
    self.pattern.hidden = self.mediaAppearance && !self.pattern.verticalReferencePattern;
    self.carousel.hidden = self.mediaAppearance || self.pattern.verticalReferencePattern;
    self.mediaTitle.hidden = !self.mediaAppearance || self.pattern.verticalReferencePattern;
    [self.customTabBar setNeedsLayout];
    [self.customTabBar layoutIfNeeded];
}
- (void)setCustomTabBarAppearance {
    self.originalAppearances++;
    self.customTabBar.mediaAppearance = self.mediaAppearance;
    [self.customTabBar writeHostAppearance];
}
@end

@interface HGTabBarTests : UIResponder <UIWindowSceneDelegate>
@property(nonatomic, strong) UIWindow *window;
@property(nonatomic, strong) SSTabBarController *host;
@property(nonatomic, strong) NSMutableArray<NSDictionary *> *checks;
@property(nonatomic, strong) NSMutableArray<NSDictionary *> *snapshots;
@property(nonatomic, strong) NSMutableArray<NSString *> *captureAcknowledgements;
@property(nonatomic, strong) NSMutableArray<NSDictionary *> *drawCaptureResults;
@property(nonatomic, strong) NSMutableArray<NSDictionary *> *captureEvents;
@property(nonatomic, strong) UIWindow *referenceWindow;
@property(nonatomic, strong) UIVisualEffectView *referenceRegular;
@property(nonatomic, strong) UIVisualEffectView *referenceClear;
@property(nonatomic, strong) UIVisualEffectView *referenceRegularClearAfter;
@property(nonatomic, strong) UIVisualEffectView *referenceNilClear;
@property(nonatomic, strong) NSDictionary *referenceClearAfterAssignment;
@property(nonatomic, strong) UIVisualEffectView *hostNilControl;
@property(nonatomic, strong) UIVisualEffectView *hostBlurReference;
@property(nonatomic) BOOL glassDisabled;
@property(nonatomic) BOOL legacyControl;
@property(nonatomic) BOOL nativeGlassAvailable;
@property(nonatomic) BOOL nativeReferenceOnly;
@property(nonatomic) BOOL compatibilityFallbackOnly;
@property(nonatomic) BOOL backgroundResetOnly;
@property(nonatomic) BOOL nestedOnly;
@property(nonatomic) IMP imageHiddenBeforeInstall;
@end

@implementation HGTabBarTests
- (void)check:(BOOL)passed name:(NSString *)name {
    [self.checks addObject:@{@"name": name, @"passed": @(passed)}];
    NSLog(@"[HonggoTabBarTests] %@: %@", passed ? @"PASS" : @"FAIL", name);
}
- (void)snapshot:(NSString *)phase {
    SSTabBar *bar = self.host.customTabBar;
    UIVisualEffectView *glass = HGEffects(bar).firstObject;
    UIScrollView *feed = self.host.primaryFeed;
    NSMutableArray<NSDictionary *> *titles = [NSMutableArray array];
    for (UIButton *item in bar.items) {
        [titles addObject:@{
            @"title": item.currentTitle ?: @"",
            @"currentTitleColor": item.currentTitleColor.description ?: @"nil",
            @"labelTextColor": item.titleLabel.textColor.description ?: @"nil"
        }];
    }
    [self.snapshots addObject:@{
        @"phase": phase,
        @"rootBounds": NSStringFromCGRect(self.host.view.bounds),
        @"barFrame": NSStringFromCGRect(bar.frame),
        @"viewportFrame": NSStringFromCGRect(self.host.viewport.frame),
        @"scrollFrame": NSStringFromCGRect(feed.frame),
        @"contentInset": NSStringFromUIEdgeInsets(feed.contentInset),
        @"adjustedInset": NSStringFromUIEdgeInsets(feed.adjustedContentInset),
        @"indicatorInset": NSStringFromUIEdgeInsets(feed.verticalScrollIndicatorInsets),
        @"glassCount": @(HGEffects(bar).count),
        @"glassFrame": NSStringFromCGRect(glass.frame),
        @"glassStyle": @(glass.traitCollection.userInterfaceStyle),
        @"glassOverrideStyle": @(glass.overrideUserInterfaceStyle),
        @"effectClass": glass.effect ? NSStringFromClass(glass.effect.class) : @"none",
        @"contentBackground": bar.contentView.backgroundColor.description ?: @"nil",
        @"wrapperBackground": bar.shell.backgroundColor.description ?: @"nil",
        @"gestureBadgeColor": bar.gestureBadge.backgroundColor.description ?: @"nil",
        @"nestedImageHidden": @(bar.backgroundImageView.hidden),
        @"materialFacts": HGMaterialFacts(glass),
        @"materialBelowContent": @(HGIsBelowContent(glass, bar.contentView)),
        @"originalContentParent": @((BOOL)(bar.contentView.superview == bar.shell)),
        @"originalButtonParent": @((BOOL)(bar.items.firstObject.superview == bar.contentView)),
        @"titles": titles
    }];
}
- (void)snapshotSurface:(UIVisualEffectView *)surface phase:(NSString *)phase expectedWindow:(UIWindow *)window {
    [self.snapshots addObject:@{
        @"phase": phase,
        @"surfacePresent": @((BOOL)(surface != nil)),
        @"opaque": @(surface.opaque),
        @"ownBackgroundClear": @(HGColorIsClear(surface.backgroundColor, surface.traitCollection)),
        @"contentBackgroundClear": @(HGColorIsClear(surface.contentView.backgroundColor,
            surface.contentView.traitCollection)),
        @"publicSurfaceClear": @(HGPublicSurfaceIsClear(surface)),
        @"effectPresent": @((BOOL)(surface.effect != nil)),
        @"inExpectedWindow": @((BOOL)(surface.window == window)),
        @"materialFacts": HGMaterialFacts(surface)
    }];
}
- (NSURL *)outputURL:(NSString *)name {
    NSURL *directory = [NSFileManager.defaultManager URLsForDirectory:NSDocumentDirectory
        inDomains:NSUserDomainMask].firstObject;
    [NSFileManager.defaultManager createDirectoryAtURL:directory withIntermediateDirectories:YES
        attributes:nil error:nil];
    return [directory URLByAppendingPathComponent:name];
}
- (void)capture:(NSString *)name {
    [self captureWindow:self.window name:name];
}
- (void)captureWindow:(UIWindow *)window name:(NSString *)name {
    [window layoutIfNeeded];
    [CATransaction flush];
    UIGraphicsImageRenderer *renderer = [[UIGraphicsImageRenderer alloc] initWithSize:window.bounds.size];
    __block BOOL complete = NO;
    UIImage *image = [renderer imageWithActions:^(UIGraphicsImageRendererContext *context) {
        complete = [window drawViewHierarchyInRect:window.bounds afterScreenUpdates:YES];
    }];
    [UIImagePNGRepresentation(image) writeToURL:[self outputURL:name] atomically:YES];
    [self.drawCaptureResults addObject:@{
        @"file": name,
        @"drawViewHierarchyComplete": @(complete),
        @"windowAddress": [NSString stringWithFormat:@"%p", (__bridge void *)window]
    }];
}
- (void)layoutHost {
    [self.host.view setNeedsLayout];
    [self.host.view layoutIfNeeded];
    [self.host viewDidLayoutSubviews];
}
- (BOOL)mainFeedReachesBottom {
    CGFloat bottom = CGRectGetMaxY(self.host.view.bounds);
    CGRect viewport = [self.host.viewport convertRect:self.host.viewport.bounds toView:self.host.view];
    CGRect feed = [self.host.primaryFeed convertRect:self.host.primaryFeed.bounds toView:self.host.view];
    return fabs(CGRectGetMaxY(viewport) - bottom) < 0.75 &&
        fabs(CGRectGetMaxY(feed) - bottom) < 0.75;
}
- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session
        options:(UISceneConnectionOptions *)options {
    self.checks = [NSMutableArray array];
    self.snapshots = [NSMutableArray array];
    self.captureAcknowledgements = [NSMutableArray array];
    self.drawCaptureResults = [NSMutableArray array];
    self.captureEvents = [NSMutableArray array];
    self.glassDisabled = [NSProcessInfo.processInfo.arguments containsObject:@"--glass-off"];
    self.legacyControl = [NSProcessInfo.processInfo.arguments containsObject:@"--legacy-control"];
    self.nativeReferenceOnly = [NSProcessInfo.processInfo.arguments containsObject:@"--native-reference"];
    self.compatibilityFallbackOnly = [NSProcessInfo.processInfo.arguments containsObject:@"--compat-fallback"];
    self.backgroundResetOnly = [NSProcessInfo.processInfo.arguments containsObject:@"--background-reset-only"];
    self.nestedOnly = [NSProcessInfo.processInfo.arguments containsObject:@"--nested-only"];
    Class glass = NSClassFromString(@"UIGlassEffect");
    self.nativeGlassAvailable = [glass respondsToSelector:sel_registerName("effectWithStyle:")];
    if ([NSProcessInfo.processInfo.arguments containsObject:@"--startup-override"]) {
        NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
        NSDictionary *persistent = [defaults persistentDomainForName:NSBundle.mainBundle.bundleIdentifier];
        [self check:[[NSBundle.mainBundle objectForInfoDictionaryKey:@"UIDesignRequiresCompatibility"] boolValue] &&
            HGNativeGlassStartupOverride() && HGNativeGlassConfigured()
            name:@"startup enables native configuration while preserving compatibility plist"];
        [self check:![persistent[@"com.apple.SwiftUI.IgnoreSolariumOptOut"] boolValue] &&
            [defaults boolForKey:@"com.apple.SwiftUI.IgnoreSolariumOptOut"] &&
            [defaults integerForKey:@"HonggoStartupFixture"] == 73
            name:@"startup preference is process-local and preserves unrelated arguments"];
    }
    [NSUserDefaults.standardUserDefaults removePersistentDomainForName:NSBundle.mainBundle.bundleIdentifier];
    [NSUserDefaults.standardUserDefaults setBool:!self.glassDisabled forKey:@"HongguoGlass.glass"];
    [UIView setAnimationsEnabled:NO];
    if (self.nativeReferenceOnly) {
        // The compatibility-NO comparison uses only Apple's native effects.
        // Do not install any plugin hooks or run the host regression suite.
        UIWindowScene *windowScene = (UIWindowScene *)scene;
        self.window = [[UIWindow alloc] initWithWindowScene:windowScene];
        self.window.frame = windowScene.coordinateSpace.bounds;
        self.window.rootViewController = [UIViewController new];
        [self.window makeKeyAndVisible];
        dispatch_async(dispatch_get_main_queue(), ^{ [self runSystemCaptureAtIndex:0]; });
        return;
    }
    HGInstallEffectSpy();
    IMP viewLayout = class_getMethodImplementation(UIView.class, @selector(layoutSubviews));
    IMP scrollLayout = class_getMethodImplementation(UIScrollView.class, @selector(layoutSubviews));
    IMP scrollWindow = class_getMethodImplementation(UIScrollView.class, @selector(didMoveToWindow));
    self.imageHiddenBeforeInstall = class_getMethodImplementation(UIImageView.class, @selector(setHidden:));
    [self check:HGInstallTabBar() name:@"supported host tab bar hooks install"];
    [self check:viewLayout == class_getMethodImplementation(UIView.class, @selector(layoutSubviews)) &&
        scrollLayout == class_getMethodImplementation(UIScrollView.class, @selector(layoutSubviews)) &&
        scrollWindow == class_getMethodImplementation(UIScrollView.class, @selector(didMoveToWindow))
        name:@"UIKit base view and scroll lifecycle implementations are not globally replaced"];
    self.host = [SSTabBarController new];
    self.host.overrideUserInterfaceStyle = UIUserInterfaceStyleLight;
    UIWindowScene *windowScene = (UIWindowScene *)scene;
    self.window = [[UIWindow alloc] initWithWindowScene:windowScene];
    self.window.frame = windowScene.coordinateSpace.bounds;
    self.window.rootViewController = self.host;
    [self.window makeKeyAndVisible];
    dispatch_async(dispatch_get_main_queue(), ^{
        @try {
            if (self.nestedOnly) {
                [self runNestedChecks];
            } else if (self.backgroundResetOnly) {
                [self runBackgroundResetChecks];
            } else if (self.glassDisabled) {
                [self runDisabledChecks];
            } else if (self.compatibilityFallbackOnly) {
                [self runFallbackChecks];
            } else {
                [self runGlassChecks];
            }
        } @catch (NSException *exception) {
            [self check:NO name:[@"UIKit exception: " stringByAppendingString:exception.description]];
        }
        // This focused regression checks state synchronously at each setter's
        // return. A capture would force layout and could repair the old defect
        // before it is observed, so this mode does not enter the capture path.
        if (self.backgroundResetOnly || self.nestedOnly) [self finish];
        else [self runSystemCaptureAtIndex:0];
    });
}
- (void)runNestedChecks {
    [self runNestedChecksForClass:SSNestedContainerScrollView.class];
    [self runNestedChecksForClass:SSMyUser637NestedContainerScrollView.class];
}
- (void)runNestedChecksForClass:(Class)coordinatorClass {
    [self layoutHost];
    UIView *root = self.host.view;
    SSTabBar *bar = self.host.customTabBar;
    CGFloat width = root.bounds.size.width, height = root.bounds.size.height;
    CGFloat top = bar.frame.origin.y;
    self.host.viewport.hidden = YES;
    // Both verified host classes expose the same contentView getter/setter.
    SSNestedContainerScrollView *outer = [[coordinatorClass alloc] initWithFrame:CGRectMake(0, 0, width, top)];
    outer.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentNever;
    outer.contentSize = CGSizeMake(width, top + 120);
    outer.contentView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, width, top + 120)];
    [outer addSubview:outer.contentView];
    UIScrollView *feed = [[UIScrollView alloc] initWithFrame:CGRectMake(0, 120, width, top - 120)];
    feed.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentNever;
    feed.contentInset = UIEdgeInsetsMake(0, 0, 12, 0);
    feed.contentSize = CGSizeMake(width, height * 3);
    CGRect contentBefore = outer.contentView.frame;
    [outer.contentView addSubview:feed];
    [root insertSubview:outer belowSubview:bar];
    [bar layoutSubviews];
    BOOL mine = coordinatorClass == SSMyUser637NestedContainerScrollView.class;
    CGRect rect = [feed convertRect:feed.bounds toView:root];
    [self check:fabs(CGRectGetMaxY(rect) - height) < 0.75
        name:mine ? @"profile inner feed extends behind the bar through the collapsing-header wrapper" :
            @"nested inner feed extends behind the bar through the collapsing-header wrapper"];
    [self check:outer.contentInset.bottom == 0 && fabs(feed.contentInset.bottom - 100) < 0.75
        name:mine ? @"profile coordinator keeps zero extra padding and extends the inner list" :
            @"nested scroll reserves bottom padding only on the inner list"];
    [self check:CGRectEqualToRect(outer.contentView.frame, contentBefore)
        name:@"nested scrolling content height remains owned by the host"];
    SEL edgeGetter = sel_registerName("bottomEdgeEffect");
    if ([outer respondsToSelector:edgeGetter] && [feed respondsToSelector:edgeGetter]) {
        id outerEdge = ((id (*)(id, SEL))objc_msgSend)(outer, edgeGetter);
        id innerEdge = ((id (*)(id, SEL))objc_msgSend)(feed, edgeGetter);
        [self check:((BOOL (*)(id, SEL))objc_msgSend)(outerEdge, @selector(isHidden)) &&
            ((BOOL (*)(id, SEL))objc_msgSend)(innerEdge, @selector(isHidden))
            name:@"both nested scroll layers hide their bottom edge veil"];
    }
    outer.contentOffset = CGPointMake(0, 120);
    feed.frame = CGRectMake(0, 120, width, top);
    [bar layoutSubviews];
    feed.contentOffset = CGPointMake(0, feed.contentSize.height - feed.bounds.size.height + feed.contentInset.bottom);
    rect = [feed convertRect:feed.bounds toView:root];
    [self check:fabs(CGRectGetMaxY(rect) - height) < 0.75 && !HGEffectivelyHidden(bar.items.firstObject)
        name:@"collapsed header and list bottom preserve an extended viewport and host tab controls"];
    CGFloat inset = feed.contentInset.bottom;
    for (NSUInteger i = 0; i < 20; i++) [bar layoutSubviews];
    [self check:fabs(feed.contentInset.bottom - inset) < 0.01
        name:@"nested scroll refresh does not accumulate bottom padding"];
    [outer removeFromSuperview];
    [bar layoutSubviews];
    [self check:fabs(feed.contentInset.bottom - 12) < 0.01
        name:@"detaching the nested page restores inner list padding"];
    self.host.viewport.hidden = NO;
    [self layoutHost];
    CGRect originalContent = bar.contentView.frame;
    bar.contentView.frame = CGRectMake(0, originalContent.origin.y, width, originalContent.size.height);
    [bar.contentView layoutSubviews];
    UIVisualEffectView *surface = HGEffects(bar).firstObject;
    [self check:CGRectGetMinX(surface.frame) >= 20 && CGRectGetMaxX(surface.frame) <= width - 20 &&
        bar.contentView.frame.size.width == width && HGFullEffectAlpha(surface)
        name:@"edge-to-edge host content retains symmetric floating material margins without shrinking hit targets"];
    bar.contentView.frame = originalContent;
    [self layoutHost];
}
- (void)runBackgroundResetChecks {
    [self layoutHost];
    SSTabBar *bar = self.host.customTabBar;
    UIImageView *originalImage = bar.backgroundImageView;
    UIView *originalLight = bar.lightView;
    [self check:bar.window == self.window && !HGEffectivelyHidden(bar.items.firstObject) &&
        HGEffects(bar).firstObject.effect != nil && originalImage.hidden && originalLight.hidden &&
        originalImage.window == self.window && originalLight.window == self.window
        name:@"background reset fixture starts with live glass and attached suppressed backgrounds"];

    NSUInteger barLayouts = bar.originalLayouts;
    NSUInteger hostLayouts = self.host.originalLayouts;
    NSUInteger appearances = self.host.originalAppearances;
    // No layout, appearance callback, runloop turn, or screenshot is permitted
    // between these host setters and their immediate assertions.
    originalImage.hidden = NO;
    BOOL imageStayedHidden = HGEffectivelyHidden(originalImage);
    [self check:imageStayedHidden && bar.originalLayouts == barLayouts &&
        self.host.originalLayouts == hostLayouts && self.host.originalAppearances == appearances
        name:@"background image hidden reset is suppressed before its setter returns"];
    originalLight.hidden = NO;
    BOOL lightStayedHidden = HGEffectivelyHidden(originalLight);
    [self check:lightStayedHidden && bar.originalLayouts == barLayouts &&
        self.host.originalLayouts == hostLayouts && self.host.originalAppearances == appearances
        name:@"light background hidden reset is suppressed before its setter returns"];
    [self.snapshots addObject:@{
        @"phase": @"background-reset-setter-returns",
        @"imageEffectivelyHidden": @(imageStayedHidden),
        @"lightEffectivelyHidden": @(lightStayedHidden),
        @"barLayoutDelta": @(bar.originalLayouts - barLayouts),
        @"hostLayoutDelta": @(self.host.originalLayouts - hostLayouts),
        @"appearanceDelta": @(self.host.originalAppearances - appearances)
    }];

    UIImageView *icon = [[UIImageView alloc] initWithImage:HGWhiteImage()];
    icon.frame = CGRectMake(4, 4, 18, 18);
    [bar.items.firstObject addSubview:icon];
    icon.hidden = YES;
    BOOL iconCanHide = icon.hidden;
    icon.hidden = NO;
    icon.alpha = 0.6;
    [self check:iconCanHide && !icon.hidden && !HGEffectivelyHidden(icon) &&
        fabs(icon.alpha - 0.6) < 0.001 && object_getClass(icon) == UIImageView.class &&
        self.imageHiddenBeforeInstall == class_getMethodImplementation(UIImageView.class, @selector(setHidden:))
        name:@"unrelated host image visibility alpha and runtime class remain untouched"];

    [originalImage removeFromSuperview];
    [self check:originalImage.superview == nil && !originalImage.hidden &&
        object_getClass(originalImage) == UIImageView.class
        name:@"detaching a suppressed image restores its visibility and releases observation immediately"];
    [originalLight removeFromSuperview];
    [self check:originalLight.superview == nil && !originalLight.hidden &&
        object_getClass(originalLight) == UIView.class
        name:@"detaching a suppressed light view restores its visibility and releases observation immediately"];

    UIImageView *replacementImage = [[UIImageView alloc] initWithImage:HGWhiteImage()];
    replacementImage.frame = bar.contentView.frame;
    UIView *replacementLight = [[UIView alloc] initWithFrame:bar.contentView.frame];
    replacementLight.backgroundColor = UIColor.whiteColor;
    bar.backgroundImageView = replacementImage;
    [bar.shell addSubview:replacementImage];
    bar.lightView = replacementLight;
    [bar.shell addSubview:replacementLight];
    [self check:replacementImage.window == self.window && replacementLight.window == self.window &&
        replacementImage.hidden && replacementLight.hidden && bar.originalLayouts == barLayouts &&
        self.host.originalLayouts == hostLayouts && self.host.originalAppearances == appearances
        name:@"rebuilt backgrounds are suppressed during insertion without another host layout"];

    replacementImage.hidden = NO;
    [self check:HGEffectivelyHidden(replacementImage) && bar.originalLayouts == barLayouts &&
        self.host.originalLayouts == hostLayouts && self.host.originalAppearances == appearances
        name:@"rebuilt image hidden reset is suppressed before its setter returns"];
    replacementLight.hidden = NO;
    [self check:HGEffectivelyHidden(replacementLight) && bar.originalLayouts == barLayouts &&
        self.host.originalLayouts == hostLayouts && self.host.originalAppearances == appearances
        name:@"rebuilt light hidden reset is suppressed before its setter returns"];

    // Reuse the removed objects outside the bar while its new backgrounds are
    // still active. Later owner refreshes must not restore or hide these views.
    originalImage.frame = CGRectMake(20, 80, 24, 24);
    originalLight.frame = CGRectMake(52, 80, 24, 24);
    [self.host.view addSubview:originalImage];
    [self.host.view addSubview:originalLight];
    originalImage.hidden = YES;
    originalImage.hidden = NO;
    originalLight.hidden = YES;
    originalLight.hidden = NO;
    originalLight.backgroundColor = UIColor.systemPurpleColor;
    originalLight.opaque = YES;
    replacementImage.hidden = NO;
    [self check:!HGEffectivelyHidden(originalImage) && !HGEffectivelyHidden(originalLight) &&
        [originalLight.backgroundColor isEqual:UIColor.systemPurpleColor] && originalLight.opaque &&
        object_getClass(originalImage) == UIImageView.class && object_getClass(originalLight) == UIView.class
        name:@"reused former backgrounds retain host state after their old owner refreshes"];

    bar.hidden = YES;
    [self check:!replacementImage.hidden && !replacementLight.hidden &&
        object_getClass(replacementImage) == UIImageView.class && object_getClass(replacementLight) == UIView.class
        name:@"hiding the tab bar restores replacement backgrounds and their original classes"];
    replacementImage.hidden = YES;
    replacementImage.hidden = NO;
    replacementLight.hidden = YES;
    replacementLight.hidden = NO;
    [self check:!replacementImage.hidden && !replacementLight.hidden &&
        !HGEffectivelyHidden(originalImage) && !HGEffectivelyHidden(originalLight)
        name:@"released background setters preserve host visibility without stale owner intervention"];
}
- (void)runDisabledChecks {
    [self layoutHost];
    [self snapshot:@"disabled"];
    SSTabBar *bar = self.host.customTabBar;
    [self check:HGEffects(bar).count == 0 name:@"disabled glass creates no replacement surface"];
    [self check:!HGBackgroundPathIsClear(bar.contentView, bar) && !bar.backgroundImageView.hidden
        name:@"disabled glass preserves the original host backgrounds"];
    [self check:fabs(CGRectGetMaxY(self.host.viewport.frame) - CGRectGetMinY(bar.frame)) < 0.75 &&
        fabs(self.host.primaryFeed.contentInset.bottom - 12) < 0.01 &&
        fabs(self.host.primaryFeed.verticalScrollIndicatorInsets.bottom - 9) < 0.01
        name:@"disabled glass preserves the host viewport and insets"];
    NSUInteger taps = bar.taps;
    [bar.items[1] sendActionsForControlEvents:UIControlEventTouchUpInside];
    [self check:bar.taps == taps + 1 && bar.selectedIndex == 1
        name:@"disabled glass preserves original tab actions"];
    BOOL settingsGesture = NO;
    for (UIGestureRecognizer *gesture in bar.gestureRecognizers) {
        settingsGesture |= [gesture isKindOfClass:UILongPressGestureRecognizer.class];
    }
    [self check:settingsGesture name:@"settings fallback stays discoverable with glass disabled"];
    [self capture:@"tabbar-disabled-simulator.png"];
}
- (void)runFallbackChecks {
    [self layoutHost];
    SSTabBar *bar = self.host.customTabBar;
    UIVisualEffectView *surface = HGEffects(bar).firstObject;
    [self snapshot:@"compatibility-fallback-initial"];
    [self check:[[NSBundle.mainBundle objectForInfoDictionaryKey:@"UIDesignRequiresCompatibility"] boolValue]
        name:@"the unprepared fallback host retains its compatibility metadata"];
    [self check:HGEffects(bar).count == 1 && [surface.effect isKindOfClass:UIBlurEffect.class] &&
        ![surface.effect isKindOfClass:NSClassFromString(@"UIGlassEffect")] &&
        HGEffectSpyInstalled && HGEffectSetterCount(surface, YES) > 0
        name:@"an unprepared host installs a real UIBlurEffect instead of claiming native glass"];
    [self check:HGBackgroundPathIsClear(bar.contentView, bar) &&
        HGEffectivelyHidden(bar.backgroundImageView) && HGIsBelowContent(surface, bar.contentView)
        name:@"fallback material stays behind clear host wrappers and visible controls"];
    [self check:bar.contentView.superview == bar.shell && bar.items.firstObject.superview == bar.contentView &&
        !HGEffectivelyHidden(bar.items.firstObject) && !HGEffectivelyHidden(bar.gestureTitle) &&
        !HGEffectivelyHidden(bar.badge) && !HGEffectivelyHidden(bar.gestureBadge) &&
        [bar.gestureBadge.backgroundColor isEqual:UIColor.systemOrangeColor]
        name:@"fallback preserves original control parents labels and colored badges"];
    CGFloat extension = self.host.primaryFeed.bounds.size.height - CGRectGetMinY(bar.frame);
    [self check:[self mainFeedReachesBottom] && extension > 40 &&
        fabs(self.host.primaryFeed.contentInset.bottom - (12 + extension)) < 0.75 &&
        fabs(self.host.primaryFeed.verticalScrollIndicatorInsets.bottom - (9 + extension)) < 0.75
        name:@"fallback exposes background content behind the capsule without accumulating insets"];
    NSUInteger taps = bar.taps;
    [bar.items[3] sendActionsForControlEvents:UIControlEventTouchUpInside];
    [self.host setCustomTabBarAppearance];
    [self check:bar.taps == taps + 1 && bar.selectedIndex == 3 && bar.items[3].selected &&
        bar.items[3].enabled && bar.items[3].userInteractionEnabled
        name:@"fallback keeps the original tab selection action usable"];
}
- (void)runGlassChecks {
    [self layoutHost];
    [self snapshot:@"initial"];
    SSTabBar *bar = self.host.customTabBar;
    UIVisualEffectView *glass = HGEffects(bar).firstObject;
    [self check:HGEffects(bar).count == 1 && glass.effect != nil
        name:@"first controller layout installs one final material"];
    [self check:!self.nativeGlassAvailable || [glass.effect isKindOfClass:NSClassFromString(@"UIGlassEffect")]
        name:@"a supported runtime uses the actual UIGlassEffect class"];
    [self check:HGBackgroundPathIsClear(bar.contentView, bar)
        name:@"nested content and wrapper backgrounds no longer cover the glass"];
    [self check:HGEffectivelyHidden(bar.backgroundImageView) && HGEffectivelyHidden(bar.lightView) &&
        HGEffectivelyHidden(bar.darkView) && HGEffectivelyHidden(bar.lineView)
        name:@"known nested host backgrounds are suppressed"];
    [self check:HGIsBelowContent(glass, bar.contentView)
        name:@"the material is ordered behind the host content branch"];
    [self check:bar.contentView.superview == bar.shell
        name:@"the original host content parent is preserved"];
    [self check:bar.items.firstObject.superview == bar.contentView
        name:@"the original host button parent is preserved"];
    [self check:HGColorIsClear(glass.backgroundColor, glass.traitCollection)
        name:@"the glass view itself has no solid background fill"];
    [self check:HGColorIsClear(glass.contentView.backgroundColor, glass.traitCollection)
        name:@"the glass content view has no solid background fill"];
    [self check:HGEffectSpyInstalled && HGEffectSetterCount(glass, YES) > 0
        name:@"the material spy observes an actual nonnil effect installation"];
    [self check:![bar.gestureItem isKindOfClass:UIControl.class] &&
        bar.gestureItem.superview == bar.contentView && bar.gestureItem.gestureRecognizers.count == 1 &&
        !HGEffectivelyHidden(bar.gestureBadge) &&
        [bar.gestureBadge.backgroundColor isEqual:UIColor.systemOrangeColor] &&
        fabs(bar.gestureBadge.bounds.size.width - 7) < 0.01 &&
        fabs(bar.gestureBadge.bounds.size.height - 7) < 0.01 &&
        [bar.gestureTitle.text isEqualToString:@"商城"]
        name:@"a plain UIView gesture item retains its small colored badge and label"];
    [self check:HGFullEffectAlpha(glass) && !glass.opaque && glass.layer.mask == nil && bar.layer.mask == nil
        name:@"glass uses full alpha without an ancestor mask"];
    CGRect contentFrame = [bar.contentView convertRect:bar.contentView.bounds toView:bar];
    [self check:glass.bounds.size.width >= 64 && glass.bounds.size.height >= 44 &&
        fabs(CGRectGetMidX(glass.frame) - CGRectGetMidX(contentFrame)) < 0.75 &&
        fabs(CGRectGetMidY(glass.frame) - CGRectGetMidY(contentFrame)) < 0.75 &&
        CGRectContainsRect(CGRectInset(bar.bounds, -0.75, -0.75), glass.frame)
        name:@"the surface has final finite capsule geometry before display"];
    [self check:[self mainFeedReachesBottom]
        name:@"the main scroll viewport extends behind the entire floating bar"];
    CGFloat extension = self.host.primaryFeed.bounds.size.height - CGRectGetMinY(bar.frame);
    [self check:extension > 40 &&
        fabs(self.host.primaryFeed.contentInset.bottom - (12 + extension)) < 0.75 &&
        fabs(self.host.primaryFeed.verticalScrollIndicatorInsets.bottom - (9 + extension)) < 0.75
        name:@"the viewport extension is balanced by content and indicator insets"];
    CGRect carouselFrame = self.host.carousel.frame;
    UIEdgeInsets carouselInsets = self.host.carousel.contentInset;
    UIColor *artworkColor = self.host.pattern.backgroundColor;
    [self capture:@"tabbar-light-simulator.png"];

    NSUInteger taps = bar.taps;
    [bar.items[3] sendActionsForControlEvents:UIControlEventTouchUpInside];
    [self.host setCustomTabBarAppearance];
    [self check:bar.taps == taps + 1 && bar.selectedIndex == 3 && bar.items[3].selected &&
        bar.items[3].enabled && bar.items[3].userInteractionEnabled &&
        !HGEffectivelyHidden(bar.badge) && bar.badge.superview == bar.items.lastObject &&
        [bar.items[3].currentTitle isEqualToString:@"我的"]
        name:@"original tab events titles selection and red dot remain usable"];
    [self check:HGBackgroundPathIsClear(bar.contentView, bar) &&
        HGEffectivelyHidden(bar.backgroundImageView)
        name:@"appearance updates cannot restore the white overlay"];

    self.host.mediaAppearance = YES;
    [self layoutHost];
    [self.host setCustomTabBarAppearance];
    glass = HGEffects(bar).firstObject;
    [self snapshot:@"dark-media"];
    [self check:glass.traitCollection.userInterfaceStyle == UIUserInterfaceStyleDark &&
        self.host.traitCollection.userInterfaceStyle == UIUserInterfaceStyleLight &&
        [bar.items[3].currentTitleColor isEqual:UIColor.whiteColor]
        name:@"white video page labels select a dark material without changing the host theme"];
    [self check:HGPublicSurfaceIsClear(glass)
        name:@"switching to a dark page cannot restore a solid material background"];
    [self capture:@"tabbar-dark-simulator.png"];
    self.host.mediaAppearance = NO;
    [self layoutHost];
    [self.host setCustomTabBarAppearance];
    glass = HGEffects(bar).firstObject;
    [self check:glass.traitCollection.userInterfaceStyle == UIUserInterfaceStyleLight &&
        [bar.items[3].currentTitleColor isEqual:UIColor.blackColor]
        name:@"returning to a light page restores matching glass contrast"];
    [self check:HGPublicSurfaceIsClear(glass)
        name:@"switching back to a light page keeps both public material backgrounds clear"];

    UIVisualEffectView *stableSurface = glass;
    NSUInteger setterCallsBefore = HGEffectSetterCount(stableSurface, NO);
    NSUInteger nonNilSetterCallsBefore = HGEffectSetterCount(stableSurface, YES);
    CGFloat stableInset = self.host.primaryFeed.contentInset.bottom;
    CGFloat stableIndicator = self.host.primaryFeed.verticalScrollIndicatorInsets.bottom;
    for (NSUInteger index = 0; index < 20; index++) {
        [self.host viewDidLayoutSubviews];
    }
    [self snapshot:@"repeated-layouts"];
    [self.snapshots addObject:@{
        @"phase": @"material-reuse-setter-evidence",
        @"spyInstalled": @(HGEffectSpyInstalled),
        @"setterCallsBefore": @(setterCallsBefore),
        @"setterCallsAfter": @(HGEffectSetterCount(stableSurface, NO)),
        @"nonNilSetterCallsBefore": @(nonNilSetterCallsBefore),
        @"nonNilSetterCallsAfter": @(HGEffectSetterCount(stableSurface, YES)),
        @"sameSurfaceInstance": @((BOOL)(HGEffects(bar).firstObject == stableSurface)),
        @"materialFacts": HGMaterialFacts(stableSurface)
    }];
    [self check:HGEffects(bar).count == 1 && HGEffects(bar).firstObject == stableSurface &&
        HGEffectSpyInstalled && nonNilSetterCallsBefore > 0 && stableSurface.effect != nil &&
        HGEffectSetterCount(stableSurface, NO) == setterCallsBefore &&
        HGEffectSetterCount(stableSurface, YES) == nonNilSetterCallsBefore &&
        (!self.nativeGlassAvailable || [stableSurface.effect isKindOfClass:NSClassFromString(@"UIGlassEffect")])
        name:@"repeated host layouts reuse the same glass and material"];
    [self check:[self mainFeedReachesBottom] &&
        fabs(self.host.primaryFeed.contentInset.bottom - stableInset) < 0.01 &&
        fabs(self.host.primaryFeed.verticalScrollIndicatorInsets.bottom - stableIndicator) < 0.01
        name:@"host geometry resets are repaired without accumulating padding"];
    [self check:CGRectEqualToRect(self.host.carousel.frame, carouselFrame) &&
        UIEdgeInsetsEqualToEdgeInsets(self.host.carousel.contentInset, carouselInsets) &&
        [self.host.pattern.backgroundColor isEqual:artworkColor]
        name:@"horizontal carousels and artwork keep their original geometry and colors"];

    // A real subsequent nonnil UIKit setter may restore its compatibility fill.
    // Do not clear it from the harness: the plugin's next layout must maintain
    // the public surface while retaining the assigned native effect.
    NSUInteger reassignmentCount = HGEffectSetterCount(glass, YES);
    glass.effect = HGReferenceEffect(0);
    NSDictionary *immediatelyAfterReassignment = HGMaterialFacts(glass);
    [glass setNeedsLayout];
    [glass layoutIfNeeded];
    [self.snapshots addObject:@{
        @"phase": @"material-explicit-reassignment",
        @"beforeNonNilSetterCalls": @(reassignmentCount),
        @"immediatelyAfterSetter": immediatelyAfterReassignment,
        @"afterLayout": HGMaterialFacts(glass)
    }];
    [self check:HGEffects(bar).firstObject == glass && glass.effect != nil &&
        HGEffectSetterCount(glass, YES) == reassignmentCount + 1 &&
        (!self.nativeGlassAvailable || [glass.effect isKindOfClass:NSClassFromString(@"UIGlassEffect")]) &&
        HGPublicSurfaceIsClear(glass)
        name:@"a later nonnil material assignment is clear after its next layout"];

    NSUInteger originalLayouts = bar.originalLayouts;
    [bar layoutSubviews];
    [self check:bar.originalLayouts == originalLayouts + 1 &&
        HGBackgroundPathIsClear(bar.contentView, bar) && HGEffectivelyHidden(bar.backgroundImageView)
        name:@"a standalone bar layout removes the overlay before returning"];
    bar.contentView.opaque = YES;
    bar.shell.opaque = YES;
    [self check:!bar.contentView.opaque && !bar.shell.opaque && HGBackgroundPathIsClear(bar.contentView, bar)
        name:@"host opaque setters are repaired without waiting for another layout"];
    NSUInteger originalRatios = bar.originalRatios;
    [bar resetBlurVisualViewWithRatio:0.35];
    [self check:bar.originalRatios == originalRatios + 1 &&
        HGBackgroundPathIsClear(bar.contentView, bar) && HGFullEffectAlpha(HGEffects(bar).firstObject)
        name:@"host ratio callbacks execute once and preserve glass compositing"];

    UIImageView *lateBackground = [[UIImageView alloc] initWithImage:HGWhiteImage()];
    lateBackground.frame = bar.contentView.frame;
    [bar.backgroundImageView removeFromSuperview];
    bar.backgroundImageView = lateBackground;
    [bar addSubview:lateBackground];
    [self check:HGEffectivelyHidden(lateBackground) && HGIsBelowContent(HGEffects(bar).firstObject, bar.contentView)
        name:@"a newly inserted background is suppressed before the next controller layout"];
    [lateBackground removeFromSuperview];
    UIImageView *nestedReplacement = [[UIImageView alloc] initWithImage:HGWhiteImage()];
    nestedReplacement.frame = bar.contentView.frame;
    bar.backgroundImageView = nestedReplacement;
    [bar.shell addSubview:nestedReplacement];
    [bar layoutSubviews];
    [self check:HGEffectivelyHidden(nestedReplacement) && HGBackgroundPathIsClear(bar.contentView, bar)
        name:@"nested replacement backgrounds are repaired by the bar lifecycle"];

    self.host.fractionalEdge = 0.25;
    [self layoutHost];
    [self snapshot:@"fractional-edge"];
    CGFloat fractionalExtension = self.host.primaryFeed.bounds.size.height -
        (CGRectGetMinY(bar.frame) + self.host.fractionalEdge);
    [self check:[self mainFeedReachesBottom] && fractionalExtension > 40 &&
        self.host.primaryFeed.contentInset.bottom >= 12 + fractionalExtension - 0.75 &&
        self.host.primaryFeed.contentInset.bottom <= 12 + fractionalExtension + 0.75
        name:@"fractional layout edges do not leave a white strip or duplicate inset"];
    self.host.fractionalEdge = 0;
    [self layoutHost];
    CGFloat bottomOffset = MAX(0, self.host.primaryFeed.contentSize.height -
        self.host.primaryFeed.bounds.size.height + self.host.primaryFeed.contentInset.bottom);
    self.host.primaryFeed.contentOffset = CGPointMake(0, bottomOffset);
    [self.host.primaryFeed layoutIfNeeded];
    [self.host viewDidLayoutSubviews];
    [self snapshot:@"scrolled-bottom"];
    [self check:[self mainFeedReachesBottom] && HGBackgroundPathIsClear(bar.contentView, bar) &&
        HGEffectivelyHidden(bar.backgroundImageView)
        name:@"scrolling to the list bottom retains content beneath the capsule"];
    SEL edgeSelector = sel_registerName("bottomEdgeEffect");
    BOOL edgeSuppressed = YES;
    if ([self.host.primaryFeed respondsToSelector:edgeSelector]) {
        id edge = ((id (*)(id, SEL))objc_msgSend)(self.host.primaryFeed, edgeSelector);
        edgeSuppressed = edge && [edge respondsToSelector:@selector(isHidden)] &&
            ((BOOL (*)(id, SEL))objc_msgSend)(edge, @selector(isHidden));
    }
    [self check:edgeSuppressed name:@"the supported main scroll bottom edge effect cannot add a mask"];
    [self capture:@"tabbar-scroll-simulator.png"];

    [self runAutoresizingChecks];
    [self runNestedChecks];
    for (NSDictionary *result in HGRunControlTests(self.host.view))
        [self check:[result[@"passed"] boolValue] name:result[@"name"]];
    [self runNonBottomInsetChecks];
    [self runAutomaticInsetChecks];
    [self runWindowChecks];
}
- (BOOL)originalFlexibleGeometryRestored {
    CGFloat originalHeight = self.host.view.bounds.size.height - 88;
    return fabs(self.host.viewport.frame.size.height - originalHeight) < 0.75 &&
        fabs(self.host.primaryFeed.frame.size.height - originalHeight) < 0.75 &&
        fabs(self.host.primaryFeed.contentInset.bottom - 12) < 0.01 &&
        fabs(self.host.primaryFeed.verticalScrollIndicatorInsets.bottom - 9) < 0.01;
}
- (void)runAutoresizingChecks {
    SSTabBar *bar = self.host.customTabBar;
    self.host.viewport.autoresizingMask = UIViewAutoresizingFlexibleHeight;
    self.host.primaryFeed.autoresizingMask = UIViewAutoresizingFlexibleHeight;
    [self layoutHost];
    bar.hidden = YES;
    [self snapshot:@"flexible-parent-hidden"];
    [self check:[self originalFlexibleGeometryRestored]
        name:@"hiding the bar restores flexible parent and child without double shrinking"];
    bar.hidden = NO;
    [self snapshot:@"flexible-parent-visible"];
    [self check:[self mainFeedReachesBottom] && HGEffects(bar).count == 1 &&
        HGEffects(bar).firstObject.effect != nil &&
        fabs(self.host.primaryFeed.contentInset.bottom - 100) < 0.75 &&
        fabs(self.host.primaryFeed.verticalScrollIndicatorInsets.bottom - 97) < 0.75 &&
        [bar.gestureBadge.backgroundColor isEqual:UIColor.systemOrangeColor]
        name:@"showing the bar reapplies one material and exactly one viewport extension"];
}
- (void)runNonBottomInsetChecks {
    UIScrollView *feed = self.host.primaryFeed;
    // The host owns top/left/right. Changing those must not make the plugin
    // mistake its already-extended bottom (100/97) for a fresh baseline.
    UIEdgeInsets inset = feed.contentInset;
    inset.top = 17;
    inset.left = 5;
    inset.right = 9;
    feed.contentInset = inset;
    UIEdgeInsets indicator = feed.verticalScrollIndicatorInsets;
    indicator.top = 23;
    indicator.left = 7;
    indicator.right = 11;
    feed.verticalScrollIndicatorInsets = indicator;
    [self.host viewDidLayoutSubviews];
    [self.host viewDidLayoutSubviews];
    [self snapshot:@"host-changed-nonbottom-insets"];
    [self check:fabs(feed.contentInset.bottom - 100) < 0.75 &&
        fabs(feed.verticalScrollIndicatorInsets.bottom - 97) < 0.75 &&
        fabs(feed.contentInset.top - 17) < 0.01 && fabs(feed.contentInset.left - 5) < 0.01 &&
        fabs(feed.contentInset.right - 9) < 0.01 &&
        fabs(feed.verticalScrollIndicatorInsets.top - 23) < 0.01 &&
        fabs(feed.verticalScrollIndicatorInsets.left - 7) < 0.01 &&
        fabs(feed.verticalScrollIndicatorInsets.right - 11) < 0.01
        name:@"host top and side inset changes never accumulate another bottom extension"];
}
- (void)runAutomaticInsetChecks {
    SSTabBarController *host = [SSTabBarController new];
    [host loadViewIfNeeded];
    host.customTabBar.hidden = YES;
    host.primaryFeed.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentAutomatic;
    // Ensure an actual safe-area change even on an iPhone runtime without a
    // home-indicator inset. The short viewport ends above this added 28 pt.
    host.additionalSafeAreaInsets = UIEdgeInsetsMake(0, 0, 28, 0);
    UIWindow *window = [[UIWindow alloc] initWithWindowScene:self.window.windowScene];
    window.frame = self.window.frame;
    window.rootViewController = host;
    [window makeKeyAndVisible];
    [host.view setNeedsLayout];
    [host.view layoutIfNeeded];
    [host viewDidLayoutSubviews];
    [host.primaryFeed layoutIfNeeded];
    CGFloat originalHeight = host.primaryFeed.bounds.size.height;
    CGFloat originalAdjusted = host.primaryFeed.adjustedContentInset.bottom;
    CGFloat originalAutomatic = originalAdjusted - host.primaryFeed.contentInset.bottom;
    host.customTabBar.hidden = NO;
    [host.view setNeedsLayout];
    [host.view layoutIfNeeded];
    [host viewDidLayoutSubviews];
    [host.primaryFeed layoutIfNeeded];
    CGFloat extension = host.primaryFeed.bounds.size.height - originalHeight;
    CGFloat finalAdjusted = host.primaryFeed.adjustedContentInset.bottom;
    CGFloat finalAutomatic = finalAdjusted - host.primaryFeed.contentInset.bottom;
    [self.snapshots addObject:@{
        @"phase": @"automatic-safe-area-extension",
        @"originalViewportHeight": @(originalHeight),
        @"finalViewportHeight": @(host.primaryFeed.bounds.size.height),
        @"viewportExtension": @(extension),
        @"originalAdjustedBottom": @(originalAdjusted),
        @"finalAdjustedBottom": @(finalAdjusted),
        @"originalAutomaticBottom": @(originalAutomatic),
        @"finalAutomaticBottom": @(finalAutomatic),
        @"finalManualInset": NSStringFromUIEdgeInsets(host.primaryFeed.contentInset)
    }];
    [self check:extension > 40 && finalAutomatic > originalAutomatic + 0.5 &&
        fabs(finalAdjusted - (originalAdjusted + extension)) < 0.75
        name:@"automatic safe-area growth is counted once in the extended adjusted bottom inset"];
    [host.customTabBar removeFromSuperview];
    window.hidden = YES;
    [self.window makeKeyAndVisible];
}
- (void)runAncestorVisibilityChecksInRoot:(UIView *)root {
    for (NSUInteger mode = 0; mode < 2; mode++) {
        UIView *ancestor = [[UIView alloc] initWithFrame:CGRectMake(0,
            root.bounds.size.height - 88, root.bounds.size.width, 88)];
        ancestor.hidden = mode == 0;
        ancestor.alpha = mode == 1 ? 0 : 1;
        [root addSubview:ancestor];
        SSTabBar *bar = [[SSTabBar alloc] initWithFrame:ancestor.bounds];
        [ancestor addSubview:bar];
        UIVisualEffectView *pending = HGEffects(bar).firstObject;
        [self check:!pending || pending.effect == nil
            name:mode == 0 ? @"a bar mounted under a hidden ancestor defers its live material" :
                @"a bar mounted under a zero-alpha ancestor defers its live material"];
        ancestor.hidden = NO;
        ancestor.alpha = 1;
        [ancestor setNeedsLayout];
        [ancestor layoutIfNeeded];
        UIVisualEffectView *visible = HGEffects(bar).firstObject;
        BOOL native = !self.nativeGlassAvailable ||
            [visible.effect isKindOfClass:NSClassFromString(@"UIGlassEffect")];
        [self check:HGEffects(bar).count == 1 && visible.effect != nil && native &&
            HGFullEffectAlpha(visible) && !HGEffectivelyHidden(bar.items.firstObject) &&
            bar.contentView.superview == bar.shell && bar.items.firstObject.superview == bar.contentView &&
            [bar.gestureBadge.backgroundColor isEqual:UIColor.systemOrangeColor]
            name:mode == 0 ? @"unhiding an ancestor installs native material and preserves host controls" :
                @"restoring ancestor alpha installs native material and preserves host controls"];
        [ancestor removeFromSuperview];
    }
}
- (void)runWindowChecks {
    SSTabBar *bar = self.host.customTabBar;
    UIWindow *other = [[UIWindow alloc] initWithWindowScene:self.window.windowScene];
    other.frame = self.window.frame;
    UIViewController *otherRoot = [UIViewController new];
    other.rootViewController = otherRoot;
    [otherRoot loadViewIfNeeded];
    otherRoot.view.frame = other.bounds;
    otherRoot.view.backgroundColor = UIColor.systemIndigoColor;
    UIScrollView *unrelated = [[UIScrollView alloc] initWithFrame:CGRectMake(15, 120, 240, 160)];
    unrelated.contentSize = CGSizeMake(240, 500);
    unrelated.contentInset = UIEdgeInsetsMake(1, 2, 3, 4);
    [otherRoot.view addSubview:unrelated];
    CGRect unrelatedFrame = unrelated.frame;
    UIEdgeInsets unrelatedInsets = unrelated.contentInset;
    SSTabBar *standalone = [[SSTabBar alloc] initWithFrame:CGRectMake(0,
        other.bounds.size.height - 88, other.bounds.size.width, 88)];
    [other makeKeyAndVisible];
    [otherRoot.view addSubview:standalone];
    [self check:HGEffects(standalone).count == 1 && HGEffects(standalone).firstObject.effect != nil &&
        HGBackgroundPathIsClear(standalone.contentView, standalone)
        name:@"a bar mounted without a tab controller is prepared in its window callback"];
    [self check:HGEffects(bar).count == 1 && HGEffects(bar).firstObject.window == self.window &&
        CGRectEqualToRect(unrelated.frame, unrelatedFrame) &&
        UIEdgeInsetsEqualToEdgeInsets(unrelated.contentInset, unrelatedInsets)
        name:@"another window cannot redirect a bar or resize unrelated content"];
    [standalone removeFromSuperview];
    [self runAncestorVisibilityChecksInRoot:otherRoot.view];

    // UIKit's addSubview: itself performs the cross-window move. There is no
    // explicit removal step in which the plugin can assume it saw window=nil.
    [otherRoot.view addSubview:bar];
    UIVisualEffectView *movedSurface = HGEffects(bar).firstObject;
    [self snapshotSurface:movedSurface phase:@"direct-cross-window-immediate" expectedWindow:other];
    [self check:[self originalFlexibleGeometryRestored] && HGEffects(bar).count == 1 &&
        movedSurface.window == other && movedSurface.effect != nil &&
        (!self.nativeGlassAvailable ||
            [movedSurface.effect isKindOfClass:NSClassFromString(@"UIGlassEffect")]) &&
        fabs(self.host.primaryFeed.contentInset.top - 17) < 0.01 &&
        fabs(self.host.primaryFeed.contentInset.left - 5) < 0.01
        name:@"a direct cross-window move restores the old page and installs material in the new window"];
    [self check:HGPublicSurfaceIsClear(movedSurface)
        name:@"a direct cross-window move does not restore a solid material background"];
    [self.host.view addSubview:bar];
    [self.host viewDidLayoutSubviews];

    UIVisualEffectView *oldSurface = HGEffects(bar).firstObject;
    NSUInteger windowMoves = bar.originalWindowMoves;
    [bar removeFromSuperview];
    [self snapshot:@"flexible-parent-detached"];
    [self check:oldSurface == nil || oldSurface.effect == nil || oldSurface.superview == nil
        name:@"detaching a bar invalidates its live backdrop material"];
    [self check:[self originalFlexibleGeometryRestored]
        name:@"detaching the bar restores flexible ancestor geometry and original insets"];
    UIScrollView *feed = self.host.primaryFeed;
    [self check:fabs(feed.contentInset.bottom - 12) < 0.01 &&
        fabs(feed.verticalScrollIndicatorInsets.bottom - 9) < 0.01 &&
        fabs(feed.contentInset.top - 17) < 0.01 && fabs(feed.contentInset.left - 5) < 0.01 &&
        fabs(feed.contentInset.right - 9) < 0.01 &&
        fabs(feed.verticalScrollIndicatorInsets.top - 23) < 0.01 &&
        fabs(feed.verticalScrollIndicatorInsets.left - 7) < 0.01 &&
        fabs(feed.verticalScrollIndicatorInsets.right - 11) < 0.01
        name:@"detaching restores only owned bottom insets and preserves the latest host edges"];
    [otherRoot.view addSubview:bar];
    [self snapshotSurface:HGEffects(bar).firstObject phase:@"detached-remount-immediate" expectedWindow:other];
    [self check:HGEffects(bar).count == 1 && HGEffects(bar).firstObject.effect != nil &&
        HGEffects(bar).firstObject.window == other && HGBackgroundPathIsClear(bar.contentView, bar) &&
        bar.originalWindowMoves >= windowMoves + 2
        name:@"the original bar remounts with a valid material in a different window"];
    [self check:HGPublicSurfaceIsClear(HGEffects(bar).firstObject)
        name:@"remounting after a detach keeps both public material backgrounds clear"];
    [bar removeFromSuperview];
    [self.host.view addSubview:bar];
    [self snapshotSurface:HGEffects(bar).firstObject phase:@"return-to-original-immediate" expectedWindow:self.window];
    other.hidden = YES;
    [self.window makeKeyAndVisible];
    [self layoutHost];
    [self snapshotSurface:HGEffects(bar).firstObject phase:@"return-to-original-after-host-layout" expectedWindow:self.window];
    [self check:HGEffects(bar).count == 1 && HGEffects(bar).firstObject.effect != nil &&
        HGEffects(bar).firstObject.window == self.window &&
        !HGEffectivelyHidden(bar.badge) && bar.contentView.superview == bar.shell
        name:@"returning to the original window preserves one surface and all host content"];
    [self check:HGPublicSurfaceIsClear(HGEffects(bar).firstObject)
        name:@"returning to the original window keeps both public material backgrounds clear"];
    [self capture:@"tabbar-remount-simulator.png"];
}
- (NSArray<NSString *> *)systemCapturePhases {
    if (self.nativeReferenceOnly) return @[@"native-reference-light", @"native-reference-dark"];
    if (self.glassDisabled) return @[@"tabbar-disabled"];
    if (self.compatibilityFallbackOnly) return @[@"tabbar-material-light", @"tabbar-material-dark"];
    if (self.legacyControl) return @[@"tabbar-light", @"tabbar-dark", @"tabbar-scroll",
        @"native-reference-light", @"native-reference-dark"];
    return @[@"tabbar-light", @"tabbar-dark", @"tabbar-scroll",
        @"tabbar-material-light", @"tabbar-material-dark",
        @"native-reference-light", @"native-reference-dark"];
}
- (UIVisualEffectView *)makeNativeReferenceStyle:(NSInteger)style frame:(CGRect)frame
        title:(NSString *)title materialEnabled:(BOOL)materialEnabled {
    UIVisualEffect *effect = materialEnabled ? HGReferenceEffect(style) : nil;
    // Apple's minimal public construction, without Honggo's subclass, lifecycle,
    // observers, background rewrites or content hierarchy adaptation.
    UIVisualEffectView *surface = [[UIVisualEffectView alloc] initWithEffect:effect];
    surface.frame = frame;
    Class corners = NSClassFromString(@"UICornerConfiguration");
    SEL capsule = sel_registerName("capsuleConfiguration");
    SEL setCorners = sel_registerName("setCornerConfiguration:");
    if ([corners respondsToSelector:capsule] && [surface respondsToSelector:setCorners]) {
        id configuration = ((id (*)(id, SEL))objc_msgSend)(corners, capsule);
        ((void (*)(id, SEL, id))objc_msgSend)(surface, setCorners, configuration);
    } else {
        surface.layer.cornerRadius = frame.size.height / 2;
        surface.clipsToBounds = YES;
    }
    UILabel *label = [[UILabel alloc] initWithFrame:surface.contentView.bounds];
    label.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    label.text = title;
    label.textColor = UIColor.labelColor;
    label.font = [UIFont systemFontOfSize:17 weight:UIFontWeightSemibold];
    label.textAlignment = NSTextAlignmentCenter;
    [surface.contentView addSubview:label];
    return surface;
}
- (void)prepareNativeReferenceDark:(BOOL)dark {
    if (!self.referenceWindow) {
        self.referenceWindow = [[UIWindow alloc] initWithWindowScene:self.window.windowScene];
        self.referenceWindow.frame = self.window.frame;
        UIViewController *root = [UIViewController new];
        self.referenceWindow.rootViewController = root;
        [root loadViewIfNeeded];
        root.view.frame = self.referenceWindow.bounds;
        NSArray<UIColor *> *colors = @[
            [UIColor colorWithRed:0.10 green:0.47 blue:0.73 alpha:1],
            [UIColor colorWithRed:0.98 green:0.67 blue:0.27 alpha:1],
            [UIColor colorWithRed:0.18 green:0.63 blue:0.47 alpha:1],
            [UIColor colorWithRed:0.75 green:0.31 blue:0.42 alpha:1]
        ];
        CGFloat width = root.view.bounds.size.width;
        CGFloat height = root.view.bounds.size.height;
        // Identical vertical color strips sit behind all four material variants.
        // A few narrow white strips give refraction/blur a visible edge.
        for (NSUInteger index = 0; index < 8; index++) {
            UIView *stripe = [[UIView alloc] initWithFrame:CGRectMake(index * width / 8, 0, width / 8, height)];
            stripe.backgroundColor = colors[index % colors.count];
            [root.view addSubview:stripe];
            UIView *edge = [[UIView alloc] initWithFrame:CGRectMake(index * width / 8, 0, 3, height)];
            edge.backgroundColor = UIColor.whiteColor;
            [root.view addSubview:edge];
        }
        CGRect regularFrame = CGRectMake(16, height - 82, width - 32, 52);
        CGRect clearFrame = CGRectOffset(regularFrame, 0, -68);
        CGRect clearAfterFrame = CGRectOffset(regularFrame, 0, -136);
        CGRect nilFrame = CGRectOffset(regularFrame, 0, -204);
        self.referenceRegular = [self makeNativeReferenceStyle:0 frame:regularFrame
            title:@"Regular · UIKit default" materialEnabled:YES];
        self.referenceClear = [self makeNativeReferenceStyle:1 frame:clearFrame
            title:@"Clear · UIKit default" materialEnabled:YES];
        self.referenceRegularClearAfter = [self makeNativeReferenceStyle:0 frame:clearAfterFrame
            title:@"Regular · clear after effect" materialEnabled:YES];
        // This must remain an ordinary UIVisualEffectView with nil effect.
        // HGGlassBackdrop would reinstall an effect in its own layout method.
        self.referenceNilClear = [self makeNativeReferenceStyle:0 frame:nilFrame
            title:@"nil effect · transparent" materialEnabled:NO];
        [root.view addSubview:self.referenceRegular];
        [root.view addSubview:self.referenceClear];
        [root.view addSubview:self.referenceRegularClearAfter];
        [root.view addSubview:self.referenceNilClear];
    }
    UIUserInterfaceStyle style = dark ? UIUserInterfaceStyleDark : UIUserInterfaceStyleLight;
    self.referenceWindow.rootViewController.overrideUserInterfaceStyle = style;
    self.referenceRegular.overrideUserInterfaceStyle = style;
    self.referenceClear.overrideUserInterfaceStyle = style;
    self.referenceRegularClearAfter.overrideUserInterfaceStyle = style;
    self.referenceNilClear.overrideUserInterfaceStyle = style;
    [self.referenceWindow makeKeyAndVisible];
    [self.referenceWindow layoutIfNeeded];
    // Isolate the public, local change under investigation. Assign a live
    // material first, then clear only public fills; do this for each theme.
    // The original Regular/Clear views above remain untouched controls.
    self.referenceRegularClearAfter.effect = HGReferenceEffect(0);
    NSDictionary *afterSetter = HGSurfaceColorFacts(self.referenceRegularClearAfter);
    for (UIVisualEffectView *surface in @[self.referenceRegularClearAfter, self.referenceNilClear]) {
        surface.backgroundColor = UIColor.clearColor;
        surface.opaque = NO;
        surface.contentView.backgroundColor = UIColor.clearColor;
        surface.contentView.opaque = NO;
    }
    self.referenceClearAfterAssignment = @{
        @"afterEffectAssignment": afterSetter,
        @"afterPublicBackgroundClear": HGSurfaceColorFacts(self.referenceRegularClearAfter)
    };
}
- (void)prepareHostMaterialControl {
    UIVisualEffectView *surface = HGEffects(self.host.customTabBar).firstObject;
    CGRect frame = [surface convertRect:surface.bounds toView:self.host.view];
    CGRect nilFrame = CGRectOffset(frame, 0, self.compatibilityFallbackOnly ? -272 : -68);
    if (!self.hostNilControl) {
        self.hostNilControl = [self makeNativeReferenceStyle:0 frame:nilFrame
            title:@"nil effect · transparent control" materialEnabled:NO];
        self.hostNilControl.userInteractionEnabled = NO;
        self.hostNilControl.backgroundColor = UIColor.clearColor;
        self.hostNilControl.opaque = NO;
        self.hostNilControl.contentView.backgroundColor = UIColor.clearColor;
        self.hostNilControl.contentView.opaque = NO;
        [self.host.view addSubview:self.hostNilControl];
    }
    self.hostNilControl.frame = nilFrame;
    self.hostNilControl.hidden = NO;
    self.hostNilControl.overrideUserInterfaceStyle = surface.traitCollection.userInterfaceStyle;
    if (self.compatibilityFallbackOnly) {
        CGRect referenceFrame = CGRectOffset(frame, 0, -136);
        if (!self.hostBlurReference) {
            // Independent public UIKit positive control. Use the documented
            // own-layer clipping path, not Honggo's subclass or native glass
            // corner configuration, which compatibility mode does not render.
            self.hostBlurReference = [[UIVisualEffectView alloc]
                initWithEffect:[UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemMaterial]];
            self.hostBlurReference.backgroundColor = UIColor.clearColor;
            self.hostBlurReference.opaque = NO;
            self.hostBlurReference.clipsToBounds = YES;
            self.hostBlurReference.layer.cornerCurve = kCACornerCurveContinuous;
            UILabel *label = [UILabel new];
            label.text = @"UIKit SystemMaterial · reference";
            label.textColor = UIColor.labelColor;
            label.font = [UIFont systemFontOfSize:16 weight:UIFontWeightSemibold];
            label.textAlignment = NSTextAlignmentCenter;
            label.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
            [self.hostBlurReference.contentView addSubview:label];
            [self.host.view addSubview:self.hostBlurReference];
        }
        self.hostBlurReference.frame = referenceFrame;
        self.hostBlurReference.layer.cornerRadius = CGRectGetHeight(referenceFrame) / 2;
        self.hostBlurReference.contentView.subviews.firstObject.frame = self.hostBlurReference.contentView.bounds;
        self.hostBlurReference.overrideUserInterfaceStyle = surface.traitCollection.userInterfaceStyle;
    }
}
- (void)snapshotHostMaterialProbe:(NSString *)phase {
    SSTabBar *bar = self.host.customTabBar;
    UIVisualEffectView *surface = HGEffects(bar).firstObject;
    Class expectedEffectClass = self.compatibilityFallbackOnly ? UIBlurEffect.class : NSClassFromString(@"UIGlassEffect");
    BOOL originalControlsVisible = !HGEffectivelyHidden(bar.items[0]) &&
        !HGEffectivelyHidden(bar.items[1]) && !HGEffectivelyHidden(bar.items[3]) &&
        !HGEffectivelyHidden(bar.gestureTitle) && !HGEffectivelyHidden(bar.badge) &&
        !HGEffectivelyHidden(bar.gestureBadge);
    NSMutableDictionary *snapshot = [@{
        @"phase": phase,
        @"pixelComparisonKind": @"actual-host-tabbar",
        @"expectedEffectClass": self.compatibilityFallbackOnly ? @"UIBlurEffect" : @"UIGlassEffect",
        @"effectMatchesExpectedClass": @([surface.effect isKindOfClass:expectedEffectClass]),
        @"UIDesignRequiresCompatibility": @([[NSBundle.mainBundle
            objectForInfoDictionaryKey:@"UIDesignRequiresCompatibility"] boolValue]),
        @"plugin": HGMaterialFactsInWindow(surface, self.window),
        @"nilClear": HGMaterialFactsInWindow(self.hostNilControl, self.window),
        @"hostInstalledSurface": @((BOOL)(HGEffects(bar).count == 1 &&
            [surface.accessibilityIdentifier isEqualToString:@"honggo.tabbar.glass"] &&
            [surface isDescendantOfView:bar])),
        @"materialBelowHostContent": @(HGIsBelowContent(surface, bar.contentView)),
        @"originalContentParent": @((BOOL)(bar.contentView.superview == bar.shell)),
        @"originalButtonParent": @((BOOL)(bar.items.firstObject.superview == bar.contentView)),
        @"originalControlsVisible": @(originalControlsVisible),
        @"foregroundStyle": @(surface.traitCollection.userInterfaceStyle),
        @"windowSizePoints": @[@(self.window.bounds.size.width), @(self.window.bounds.size.height)],
        // Avoid the host badges at y=11..18 as well as the centered tab titles.
        @"pixelComparisonSampleBandsPoints": @[@[@5, @9], @[@40, @44]]
    } mutableCopy];
    NSMutableArray<NSString *> *booleanKeys = [@[@"effectMatchesExpectedClass", @"hostInstalledSurface",
        @"materialBelowHostContent", @"originalContentParent", @"originalButtonParent",
        @"originalControlsVisible"] mutableCopy];
    if (self.compatibilityFallbackOnly) {
        snapshot[@"blurReference"] = HGMaterialFactsInWindow(self.hostBlurReference, self.window);
        snapshot[@"blurReferenceMatchesExpectedClass"] =
            @([self.hostBlurReference.effect isKindOfClass:UIBlurEffect.class]);
        snapshot[@"blurReferenceMatchesForeground"] = @((BOOL)(
            self.hostBlurReference.traitCollection.userInterfaceStyle == surface.traitCollection.userInterfaceStyle));
        [booleanKeys addObjectsFromArray:@[@"blurReferenceMatchesExpectedClass", @"blurReferenceMatchesForeground"]];
    }
    snapshot[@"semanticBooleanJSONTypes"] = HGSerializedBooleanTypes(snapshot, booleanKeys);
    [self.snapshots addObject:snapshot];
}
- (void)runSystemCaptureAtIndex:(NSUInteger)index {
    NSArray<NSString *> *phases = self.systemCapturePhases;
    if (index >= phases.count) {
        self.referenceWindow.hidden = YES;
        [self.window makeKeyAndVisible];
        [self finish];
        return;
    }
    NSString *phase = phases[index];
    BOOL reference = [phase hasPrefix:@"native-reference-"];
    BOOL hostMaterialProbe = [phase hasPrefix:@"tabbar-material-"];
    @try {
        if (reference) {
            [self prepareNativeReferenceDark:[phase hasSuffix:@"-dark"]];
        } else if (!self.glassDisabled) {
            self.host.pattern.verticalReferencePattern = hostMaterialProbe;
            [self.host.pattern setNeedsDisplay];
            self.hostNilControl.hidden = !hostMaterialProbe;
            self.host.mediaAppearance = [phase hasSuffix:@"-dark"];
            [self layoutHost];
            [self.host setCustomTabBarAppearance];
            CGFloat offset = [phase isEqualToString:@"tabbar-scroll"] ? MAX(0,
                self.host.primaryFeed.contentSize.height - self.host.primaryFeed.bounds.size.height +
                self.host.primaryFeed.contentInset.bottom) : 0;
            self.host.primaryFeed.contentOffset = CGPointMake(0, offset);
            [self.window makeKeyAndVisible];
            if (hostMaterialProbe) [self prepareHostMaterialControl];
        }
    } @catch (NSException *exception) {
        [self check:NO name:[NSString stringWithFormat:@"system capture setup %@: %@", phase, exception.description]];
        [self runSystemCaptureAtIndex:index + 1];
        return;
    }
    // Return to the main runloop and allow an actual Core Animation commit.
    // The shell captures with simctl while this phase remains on screen.
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.7 * NSEC_PER_SEC)),
        dispatch_get_main_queue(), ^{
        @try {
            if (reference) {
                [self.snapshots addObject:@{
                    @"phase": phase,
                    @"UIDesignRequiresCompatibility": @([[NSBundle.mainBundle
                        objectForInfoDictionaryKey:@"UIDesignRequiresCompatibility"] boolValue]),
                    @"regularStyle": @0,
                    @"clearStyle": @1,
                    @"regularClearAfterStyle": @0,
                    @"regular": HGMaterialFacts(self.referenceRegular),
                    @"clear": HGMaterialFacts(self.referenceClear),
                    @"regularClearAfter": HGMaterialFacts(self.referenceRegularClearAfter),
                    @"nilClear": HGMaterialFacts(self.referenceNilClear),
                    @"clearAfterAssignment": self.referenceClearAfterAssignment,
                    @"windowSizePoints": @[@(self.referenceWindow.bounds.size.width),
                        @(self.referenceWindow.bounds.size.height)],
                    @"pixelComparisonSampleBandsPoints": @[@[@8, @12], @[@40, @44]]
                }];
                if (self.nativeReferenceOnly) {
                    BOOL valid = self.referenceRegular.window == self.referenceWindow &&
                        self.referenceClear.window == self.referenceWindow &&
                        self.referenceRegularClearAfter.window == self.referenceWindow &&
                        self.referenceNilClear.window == self.referenceWindow &&
                        self.referenceRegular.effect != nil && self.referenceClear.effect != nil &&
                        self.referenceRegularClearAfter.effect != nil && self.referenceNilClear.effect == nil &&
                        HGPublicSurfaceIsClear(self.referenceRegularClearAfter) &&
                        HGPublicSurfaceIsClear(self.referenceNilClear) &&
                        (!self.nativeGlassAvailable ||
                            ([self.referenceRegular.effect isKindOfClass:NSClassFromString(@"UIGlassEffect")] &&
                             [self.referenceClear.effect isKindOfClass:NSClassFromString(@"UIGlassEffect")] &&
                             [self.referenceRegularClearAfter.effect isKindOfClass:NSClassFromString(@"UIGlassEffect")]));
                    [self check:valid name:[phase stringByAppendingString:@" isolates three public materials and a nil-effect control"]];
                }
            } else {
                [self snapshot:[phase stringByAppendingString:@"-runloop-settled"]];
                if (hostMaterialProbe) [self snapshotHostMaterialProbe:phase];
                if (!self.glassDisabled) {
                    [self check:HGPublicSurfaceIsClear(HGEffects(self.host.customTabBar).firstObject)
                        name:[phase stringByAppendingString:@" keeps public material backgrounds clear after a runloop commit"]];
                }
            }
            [self captureWindow:reference ? self.referenceWindow : self.window
                name:[phase stringByAppendingString:@"-stable-simulator.png"]];
            BOOL ready = [phase writeToURL:[self outputURL:@"tabbar-capture-phase.txt"]
                atomically:YES encoding:NSUTF8StringEncoding error:nil];
            if (!ready) {
                [self check:NO name:[phase stringByAppendingString:@" could not publish screenshot phase"]];
                [self runSystemCaptureAtIndex:index + 1];
                return;
            }
        } @catch (NSException *exception) {
            [self check:NO name:[NSString stringWithFormat:@"system capture %@: %@", phase, exception.description]];
            [self runSystemCaptureAtIndex:index + 1];
            return;
        }
        CFTimeInterval ready = CACurrentMediaTime();
        [self.captureEvents addObject:@{@"phase": phase, @"event": @"ready",
            @"monotonicSeconds": @(ready), @"deadlineSeconds": @60}];
        [self waitForSystemCapture:phase index:index deadline:ready + 60];
    });
}
- (void)waitForSystemCapture:(NSString *)phase index:(NSUInteger)index deadline:(CFTimeInterval)deadline {
    NSString *acknowledged = [NSString stringWithContentsOfURL:[self outputURL:@"tabbar-capture-ack.txt"]
        encoding:NSUTF8StringEncoding error:nil];
    acknowledged = [acknowledged stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
    if ([acknowledged isEqualToString:phase]) {
        [self.captureEvents addObject:@{@"phase": phase, @"event": @"ack",
            @"monotonicSeconds": @(CACurrentMediaTime())}];
        [self.captureAcknowledgements addObject:phase];
        [self runSystemCaptureAtIndex:index + 1];
        return;
    }
    if (CACurrentMediaTime() >= deadline) {
        [self check:NO name:[phase stringByAppendingString:@" system screenshot acknowledgement timed out"]];
        [self.captureEvents addObject:@{@"phase": phase, @"event": @"timeout-stopped",
            @"monotonicSeconds": @(CACurrentMediaTime())}];
        // Keep the failed scene on screen. Advancing here could label a late
        // screenshot with the previous phase while showing the next scene.
        [self finish];
        return;
    }
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.2 * NSEC_PER_SEC)),
        dispatch_get_main_queue(), ^{
        [self waitForSystemCapture:phase index:index deadline:deadline];
    });
}
- (void)finish {
    NSUInteger failed = 0;
    for (NSDictionary *check in self.checks) {
        failed += ![check[@"passed"] boolValue];
    }
    NSDictionary *report = @{
        @"scope": @"real UIKit with a simulated SSTabBar host; not a Hongguo IPA or physical-device test",
        @"status": failed == 0 ? @"passed" : @"failed",
        @"failed": @(failed),
        @"total": @(self.checks.count),
        @"nativeGlassAvailable": @(self.nativeGlassAvailable),
        @"startupGlassOverride": @(HGNativeGlassStartupOverride()),
        @"glassEnabled": @((BOOL)(!self.glassDisabled && !self.nativeReferenceOnly)),
        @"legacyNegativeControl": @(self.legacyControl),
        @"nativeReferenceOnly": @(self.nativeReferenceOnly),
        @"compatibilityFallbackOnly": @(self.compatibilityFallbackOnly),
        @"backgroundResetOnly": @(self.backgroundResetOnly),
        @"UIDesignRequiresCompatibility": @([[NSBundle.mainBundle
            objectForInfoDictionaryKey:@"UIDesignRequiresCompatibility"] boolValue]),
        @"systemScreenshotsAcknowledged": self.captureAcknowledgements,
        @"systemCaptureEvents": self.captureEvents,
        @"drawCaptureResults": self.drawCaptureResults,
        @"effectSpyInstalled": @(HGEffectSpyInstalled),
        @"systemVersion": UIDevice.currentDevice.systemVersion,
        @"checks": self.checks,
        @"snapshots": self.snapshots
    };
    NSData *data = [NSJSONSerialization dataWithJSONObject:report options:NSJSONWritingPrettyPrinted error:nil];
    NSString *name = self.nestedOnly ? @"tabbar-nested-results.json" : self.backgroundResetOnly ? @"tabbar-background-reset-results.json" :
        (self.nativeReferenceOnly ? @"tabbar-reference-results.json" :
        (self.compatibilityFallbackOnly ? @"tabbar-fallback-results.json" :
        (self.glassDisabled ? @"tabbar-disabled-results.json" : @"tabbar-results.json")));
    [data writeToURL:[self outputURL:name] options:NSDataWritingAtomic error:nil];
}
@end

@interface HGTabBarTestAppDelegate : UIResponder <UIApplicationDelegate>
@end

@implementation HGTabBarTestAppDelegate
- (UISceneConfiguration *)application:(UIApplication *)application
        configurationForConnectingSceneSession:(UISceneSession *)session
        options:(UISceneConnectionOptions *)options {
    UISceneConfiguration *configuration = [[UISceneConfiguration alloc]
        initWithName:@"HonggoTabBarTest" sessionRole:session.role];
    configuration.delegateClass = HGTabBarTests.class;
    return configuration;
}
@end

int main(int argc, char *argv[]) {
    @autoreleasepool {
        if ([NSProcessInfo.processInfo.arguments containsObject:@"--startup-override"]) {
            NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
            [defaults setBool:YES forKey:@"HongguoGlass.glass"];
            [defaults setBool:NO forKey:@"com.apple.SwiftUI.IgnoreSolariumOptOut"];
            NSMutableDictionary *arguments = [[defaults volatileDomainForName:NSArgumentDomain] mutableCopy];
            if (!arguments) arguments = [NSMutableDictionary dictionary];
            arguments[@"HonggoStartupFixture"] = @73;
            [defaults setVolatileDomain:arguments forName:NSArgumentDomain];
            HGPrepareNativeGlass();
        }
        return UIApplicationMain(argc, argv, nil, NSStringFromClass(HGTabBarTestAppDelegate.class));
    }
}
