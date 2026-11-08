#import "HongguoGlass.h"
#import <QuartzCore/QuartzCore.h>
#import <objc/message.h>

@protocol HGHostTabBar <NSObject>
- (UIView *)contentView;
- (UIView *)backgroundImageView;
- (UIView *)lightView;
- (UIView *)darkView;
- (UIView *)lineView;
@end

@protocol HGHostTabController <NSObject>
- (UIView<HGHostTabBar> *)customTabBar;
@end

static Class HGTabBarClass;
static BOOL HGGlassEnabled;
static char HGEffectKey;
static void (*HGOriginalLayout)(id, SEL);
static void (*HGOriginalSetAppearance)(id, SEL);
static void (*HGOriginalResetRatio)(id, SEL, double);

static UIVisualEffectView *HGMakeEffectView(void) {
    Class glassClass = NSClassFromString(@"UIGlassEffect");
    SEL effectSelector = sel_registerName("effectWithStyle:");
    UIVisualEffect *effect;
    BOOL nativeGlass = [glassClass respondsToSelector:effectSelector];
    if (nativeGlass) {
        // UIGlassEffectStyleRegular. Resolve at runtime for deployment on iOS 15.
        effect = ((id (*)(id, SEL, NSInteger))objc_msgSend)(glassClass, effectSelector, 0);
    } else {
        effect = [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemMaterial];
    }

    UIVisualEffectView *view = [[UIVisualEffectView alloc] initWithEffect:effect];
    view.userInteractionEnabled = NO;
    view.accessibilityElementsHidden = YES;
    Class cornersClass = NSClassFromString(@"UICornerConfiguration");
    SEL capsule = sel_registerName("capsuleConfiguration");
    SEL setCorners = sel_registerName("setCornerConfiguration:");
    if (nativeGlass && [cornersClass respondsToSelector:capsule] && [view respondsToSelector:setCorners]) {
        id corners = ((id (*)(id, SEL))objc_msgSend)(cornersClass, capsule);
        ((void (*)(id, SEL, id))objc_msgSend)(view, setCorners, corners);
    } else {
        view.clipsToBounds = YES;
    }
    return view;
}

static void HGRefreshGlass(UIView<HGHostTabBar> *bar) {
    UIView *content = bar.contentView;
    if (!bar.window || bar.hidden || ![content isDescendantOfView:bar]) {
        return;
    }

    CGRect frame = [content convertRect:content.bounds toView:bar];
    if (frame.size.width < 64 || frame.size.height < 32) {
        return;
    }

    UIVisualEffectView *glass = objc_getAssociatedObject(bar, &HGEffectKey);
    if (!glass) {
        glass = HGMakeEffectView();
        objc_setAssociatedObject(bar, &HGEffectKey, glass, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    if (!CGRectEqualToRect(glass.frame, frame)) {
        glass.frame = frame;
        if (glass.clipsToBounds) {
            glass.layer.cornerRadius = frame.size.height / 2;
        }
    }
    if (glass.superview != bar) {
        [bar insertSubview:glass atIndex:0];
    }

    // Only replace background siblings; host controls and hit-testing stay in place.
    UIView *backgrounds[] = {bar.backgroundImageView, bar.lightView, bar.darkView, bar.lineView};
    for (NSUInteger index = 0; index < sizeof(backgrounds) / sizeof(backgrounds[0]); index++) {
        UIView *background = backgrounds[index];
        if (background.superview == bar && ![content isDescendantOfView:background]) {
            background.hidden = YES;
        }
    }
    if (bar.backgroundColor != UIColor.clearColor) {
        bar.backgroundColor = UIColor.clearColor;
    }
}

static void HGRefreshTabBar(id<HGHostTabController> controller) {
    UIView<HGHostTabBar> *bar = controller.customTabBar;
    if (![bar isKindOfClass:HGTabBarClass] || !bar.window) {
        return;
    }

    HGAttachSettings(bar);
    if (HGGlassEnabled) {
        HGRefreshGlass(bar);
    }
}

static void HGLayout(id self, SEL cmd) {
    HGOriginalLayout(self, cmd);
    HGRefreshTabBar(self);
}

static void HGSetAppearance(id self, SEL cmd) {
    HGOriginalSetAppearance(self, cmd);
    HGRefreshTabBar(self);
}

static void HGResetRatio(id self, SEL cmd, double ratio) {
    HGOriginalResetRatio(self, cmd, ratio);
    HGRefreshGlass(self);
}

BOOL HGInstallTabBar(void) {
    HGTabBarClass = objc_lookUpClass("SSTabBar");
    Class controller = objc_lookUpClass("SSTabBarController");
    if (!HGTabBarClass || !class_getInstanceMethod(controller, @selector(customTabBar))) {
        return NO;
    }

    HGOriginalLayout = (void (*)(id, SEL))HGReplaceMethod(controller,
        @selector(viewDidLayoutSubviews), "v16@0:8", (IMP)HGLayout);
    if (!HGOriginalLayout) {
        return NO;
    }

    HGGlassEnabled = HGIsEnabled(HGGlass);
    if (HGGlassEnabled) {
        HGOriginalSetAppearance = (void (*)(id, SEL))HGReplaceMethod(controller,
            sel_registerName("setCustomTabBarAppearance"), "v16@0:8", (IMP)HGSetAppearance);
        HGOriginalResetRatio = (void (*)(id, SEL, double))HGReplaceMethod(HGTabBarClass,
            sel_registerName("resetBlurVisualViewWithRatio:"), "v24@0:8d16", (IMP)HGResetRatio);
    }
    return YES;
}
