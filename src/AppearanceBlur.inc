// Hardware CABackdropLayer Gaussian blur + iOS 26/27 UIGlassEffect & UICornerConfiguration helpers.
// Adapted cleanly from FanqieX's FQXBlurControl / FQXBackdropBlurView architecture.
#import <objc/message.h>
#include <math.h>

static inline int HGBlurPercent(double value) {
    if (!isfinite(value) || value <= 0.0) return 0;
    if (value >= 100.0) return 100;
    return (int)floor(value + 0.5);
}

static inline double HGBlurFraction(int percent) {
    return (percent <= 0 ? 0 : (percent >= 100 ? 100 : percent)) / 100.0;
}

static inline double HGBlurGaussianRadius(int percent) {
    int p = percent <= 0 ? 0 : (percent >= 100 ? 100 : percent);
    if (p == 0) return 0.0;
    return 0.25 + 29.75 * pow((double)(p - 1) / 99.0, 1.30);
}

static inline double HGBlurTintAlpha(int percent, double baseAlpha, double maxAlpha) {
    if (!isfinite(baseAlpha) || baseAlpha < 0.0) baseAlpha = 0.0;
    if (!isfinite(maxAlpha) || maxAlpha < baseAlpha) maxAlpha = baseAlpha;
    int p = percent <= 0 ? 0 : (percent >= 100 ? 100 : percent);
    if (p == 0) return baseAlpha;
    double span = maxAlpha - baseAlpha;
    return baseAlpha + span * (0.01 + 0.99 * pow((double)(p - 1) / 99.0, 1.15));
}

static inline double HGBlurSaturation(int percent) {
    int p = percent <= 0 ? 0 : (percent >= 100 ? 100 : percent);
    return 1.0 + 0.80 * ((double)p / 100.0);
}

static BOOL HGBackdropFilterAvailable(void) {
    static BOOL available = NO;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        Class bdCls = NSClassFromString(@"CABackdropLayer");
        Class filterCls = NSClassFromString(@"CAFilter");
        SEL filterSel = NSSelectorFromString(@"filterWithType:");
        available = bdCls != Nil && filterCls != Nil && [filterCls respondsToSelector:filterSel];
    });
    return available;
}

static UIVisualEffect *HGMakeNativeGlassEffect(NSInteger style, BOOL interactive, UIColor *tint) {
    if (!HGNativeGlassConfigured() || UIAccessibilityIsReduceTransparencyEnabled()) return nil;
    Class glassCls = NSClassFromString(@"UIGlassEffect");
    SEL factory = NSSelectorFromString(@"effectWithStyle:");
    if (!glassCls || ![glassCls respondsToSelector:factory]) return nil;
    UIVisualEffect *effect = ((id (*)(id, SEL, NSInteger))objc_msgSend)(glassCls, factory, style);
    if (!effect) return nil;
    SEL interactiveSel = NSSelectorFromString(@"setInteractive:");
    if ([effect respondsToSelector:interactiveSel]) {
        ((void (*)(id, SEL, BOOL))objc_msgSend)(effect, interactiveSel, interactive);
    }
    SEL tintSel = NSSelectorFromString(@"setTintColor:");
    if ([effect respondsToSelector:tintSel]) {
        ((void (*)(id, SEL, id))objc_msgSend)(effect, tintSel, tint);
    }
    return effect;
}

static void HGApplyNativeCornerConfig(UIView *view, CGFloat radius, BOOL capsule) {
    if (!view) return;
    if (!HGNativeGlassConfigured()) return;
    Class cornerCls = NSClassFromString(@"UICornerConfiguration");
    SEL setCornerSel = NSSelectorFromString(@"setCornerConfiguration:");
    if (!cornerCls || ![view respondsToSelector:setCornerSel]) return;
    id config = nil;
    if (capsule) {
        SEL capSel = NSSelectorFromString(@"capsuleConfiguration");
        if ([cornerCls respondsToSelector:capSel]) {
            config = ((id (*)(id, SEL))objc_msgSend)(cornerCls, capSel);
        }
    } else {
        Class radiusCls = NSClassFromString(@"UICornerRadius");
        SEL fixedSel = NSSelectorFromString(@"fixedRadius:");
        SEL uniformSel = NSSelectorFromString(@"configurationWithUniformRadius:");
        if (radiusCls && [radiusCls respondsToSelector:fixedSel] && [cornerCls respondsToSelector:uniformSel]) {
            id rad = ((id (*)(id, SEL, CGFloat))objc_msgSend)(radiusCls, fixedSel, MAX(0.0, radius));
            if (rad) config = ((id (*)(id, SEL, id))objc_msgSend)(cornerCls, uniformSel, rad);
        }
    }
    if (config) ((void (*)(id, SEL, id))objc_msgSend)(view, setCornerSel, config);
}

@interface HGBackdropHostView : UIView
@end
@implementation HGBackdropHostView
+ (Class)layerClass {
    Class cls = NSClassFromString(@"CABackdropLayer");
    return cls ?: CALayer.class;
}
@end

@interface HGBackdropBlurView : UIVisualEffectView
@property(nonatomic, strong) HGBackdropHostView *backdropHost;
@property(nonatomic, strong) UIView *tintOverlay;
@property(nonatomic, strong) UIViewPropertyAnimator *blurAnimator;
@property(nonatomic, weak) UIWindow *animatorWindow;
@property(nonatomic) UIBlurEffectStyle blurStyle;
@property(nonatomic) NSInteger percent;
@property(nonatomic) BOOL isDarkStyle;
@property(nonatomic) CGFloat baseTintAlpha;
@property(nonatomic) CGFloat maxTintAlpha;
@property(nonatomic) BOOL matteChromeBorder;
@property(nonatomic) BOOL commentPureMode;
@property(nonatomic) BOOL usingHardwareFilter;
@property(nonatomic) BOOL applyPending;
- (void)configurePercent:(NSInteger)percent
                    dark:(BOOL)dark
                baseTint:(CGFloat)baseTint
                 maxTint:(CGFloat)maxTint
            chromeBorder:(BOOL)chromeBorder;
- (void)invalidateBlurAnimator;
@end

@implementation HGBackdropBlurView
- (instancetype)initWithEffect:(UIVisualEffect *)effect {
    self = [super initWithEffect:effect];
    if (self) {
        self.accessibilityIdentifier = @"honggo.appearance.blur";
        _usingHardwareFilter = HGBackdropFilterAvailable();
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(hgBlurDidBecomeActive:)
                                                     name:UIApplicationDidBecomeActiveNotification
                                                   object:nil];
    }
    return self;
}
- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    if (_blurAnimator.state == UIViewAnimatingStateActive) [_blurAnimator stopAnimation:YES];
}
- (void)layoutSubviews {
    [super layoutSubviews];
    CGRect bounds = self.contentView.bounds;
    if (self.backdropHost && !CGRectEqualToRect(self.backdropHost.frame, bounds)) {
        self.backdropHost.frame = bounds;
    }
    if (self.tintOverlay && !CGRectEqualToRect(self.tintOverlay.frame, bounds)) {
        self.tintOverlay.frame = bounds;
    }
}
- (void)invalidateBlurAnimator {
    if (self.blurAnimator.state == UIViewAnimatingStateActive) [self.blurAnimator stopAnimation:YES];
    self.blurAnimator = nil;
    self.animatorWindow = nil;
    self.effect = nil;
    if (self.backdropHost) {
        @try { [self.backdropHost.layer setValue:nil forKey:@"filters"]; } @catch (__unused NSException *e) {}
        [self.backdropHost removeFromSuperview];
        self.backdropHost = nil;
    }
    if (self.tintOverlay) {
        self.tintOverlay.backgroundColor = UIColor.clearColor;
        [self.tintOverlay removeFromSuperview];
        self.tintOverlay = nil;
    }
    self.layer.borderWidth = 0.0;
    self.layer.borderColor = NULL;
}
- (void)didMoveToWindow {
    [super didMoveToWindow];
    if (self.usingHardwareFilter) {
        if (self.window) [self applyHardwareBlur];
        return;
    }
    [self invalidateBlurAnimator];
    if (self.window) [self scheduleBlurUpdate];
}
- (void)hgBlurDidBecomeActive:(NSNotification *)note {
    (void)note;
    if (self.usingHardwareFilter) {
        if (self.window) [self applyHardwareBlur];
        return;
    }
    [self invalidateBlurAnimator];
    if (self.window) [self scheduleBlurUpdate];
}
- (void)configurePercent:(NSInteger)percent
                    dark:(BOOL)dark
                baseTint:(CGFloat)baseTint
                 maxTint:(CGFloat)maxTint
            chromeBorder:(BOOL)chromeBorder {
    self.isDarkStyle = dark;
    self.baseTintAlpha = baseTint;
    self.maxTintAlpha = maxTint;
    self.matteChromeBorder = chromeBorder;
    self.percent = HGBlurPercent(percent);
    if (self.usingHardwareFilter) {
        [self applyHardwareBlur];
    } else {
        UIBlurEffectStyle targetStyle = dark
            ? (chromeBorder ? UIBlurEffectStyleSystemChromeMaterialDark : UIBlurEffectStyleSystemUltraThinMaterialDark)
            : (chromeBorder ? UIBlurEffectStyleSystemChromeMaterialLight : UIBlurEffectStyleSystemUltraThinMaterialLight);
        if (self.blurStyle != targetStyle) {
            [self invalidateBlurAnimator];
            self.blurStyle = targetStyle;
        }
        [self scheduleBlurUpdate];
    }
}
- (void)applyHardwareBlur {
    if (!self.usingHardwareFilter) return;
    if (!self.window && self.percent <= 0 && !self.matteChromeBorder && self.baseTintAlpha <= 0.0005) return;
    if (self.effect) self.effect = nil;

    int p = (int)self.percent;
    CGFloat radius = HGBlurGaussianRadius(p);
    CGFloat sat = self.commentPureMode ? (1.0 + 0.35 * HGBlurFraction(p)) : HGBlurSaturation(p);
    CGFloat tintAlpha = HGBlurTintAlpha(p, self.baseTintAlpha, self.maxTintAlpha);
    BOOL needsBackdrop = (radius > 0.0);
    BOOL needsTintOverlay = (tintAlpha > 0.0005) &&
                            !(self.commentPureMode && self.maxTintAlpha <= 0.0005 && self.baseTintAlpha <= 0.0005);

    if (needsBackdrop && !self.backdropHost) {
        HGBackdropHostView *host = [[HGBackdropHostView alloc] initWithFrame:self.contentView.bounds];
        host.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        host.userInteractionEnabled = NO;
        host.clipsToBounds = YES;
        [self.contentView insertSubview:host atIndex:0];
        self.backdropHost = host;
    }
    if (needsTintOverlay && !self.tintOverlay) {
        UIView *tint = [[UIView alloc] initWithFrame:self.contentView.bounds];
        tint.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        tint.userInteractionEnabled = NO;
        [self.contentView addSubview:tint];
        self.tintOverlay = tint;
    } else if (!needsTintOverlay && self.tintOverlay) {
        self.tintOverlay.backgroundColor = UIColor.clearColor;
        [self.tintOverlay removeFromSuperview];
        self.tintOverlay = nil;
    }

    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    if (self.backdropHost) self.backdropHost.frame = self.contentView.bounds;
    if (self.tintOverlay) self.tintOverlay.frame = self.contentView.bounds;
    BOOL filterApplied = YES;
    @try {
        if (needsBackdrop && self.backdropHost) {
            CALayer *layer = self.backdropHost.layer;
            CGFloat scale = self.commentPureMode ? 0.5 : ((radius < 4.0) ? 1.0 : 0.5);
            @try { [layer setValue:@(scale) forKey:@"scale"]; } @catch (__unused NSException *ignored) {}
            NSArray *existing = [layer valueForKey:@"filters"];
            if ([existing isKindOfClass:NSArray.class] && existing.count == 2) {
                [layer setValue:@(radius) forKeyPath:@"filters.gaussianBlur.inputRadius"];
                [layer setValue:@(sat) forKeyPath:@"filters.colorSaturate.inputAmount"];
            } else {
                Class filterCls = NSClassFromString(@"CAFilter");
                SEL filterSel = NSSelectorFromString(@"filterWithType:");
                id blurFilter = ((id (*)(id, SEL, id))objc_msgSend)(filterCls, filterSel, @"gaussianBlur");
                id satFilter = ((id (*)(id, SEL, id))objc_msgSend)(filterCls, filterSel, @"colorSaturate");
                if (blurFilter && satFilter) {
                    [blurFilter setValue:@"gaussianBlur" forKey:@"name"];
                    [blurFilter setValue:@YES forKey:@"inputNormalizeEdges"];
                    [blurFilter setValue:@(radius) forKey:@"inputRadius"];
                    [satFilter setValue:@"colorSaturate" forKey:@"name"];
                    [satFilter setValue:@(sat) forKey:@"inputAmount"];
                    [layer setValue:@[blurFilter, satFilter] forKey:@"filters"];
                } else {
                    filterApplied = NO;
                }
            }
            self.backdropHost.hidden = !filterApplied;
        } else if (self.backdropHost) {
            @try { [self.backdropHost.layer setValue:nil forKey:@"filters"]; } @catch (__unused NSException *ignored) {}
            [self.backdropHost removeFromSuperview];
            self.backdropHost = nil;
        }
    } @catch (__unused NSException *e) {
        filterApplied = NO;
    }

    if (self.tintOverlay) {
        UIColor *tintColor = UIColor.clearColor;
        if (tintAlpha > 0.0005) {
            tintColor = self.isDarkStyle
                ? [UIColor colorWithWhite:0.10 alpha:tintAlpha]
                : [UIColor colorWithWhite:0.97 alpha:tintAlpha];
        }
        self.tintOverlay.backgroundColor = tintColor;
    }

    if (self.matteChromeBorder) {
        self.layer.borderWidth = 0.5;
        self.layer.borderColor = (self.isDarkStyle
            ? [UIColor colorWithWhite:1.0 alpha:0.14]
            : [UIColor colorWithWhite:0.0 alpha:0.08]).CGColor;
    } else {
        self.layer.borderWidth = 0.0;
        self.layer.borderColor = NULL;
    }
    [CATransaction commit];

    if (!filterApplied) {
        self.usingHardwareFilter = NO;
        [self invalidateBlurAnimator];
        if (self.percent > 0) [self scheduleBlurUpdate];
    }
}
- (void)scheduleBlurUpdate {
    if (self.usingHardwareFilter) {
        [self applyHardwareBlur];
        return;
    }
    if (self.applyPending) return;
    self.applyPending = YES;
    __weak HGBackdropBlurView *weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
        HGBackdropBlurView *view = weakSelf;
        if (!view || view.usingHardwareFilter) return;
        view.applyPending = NO;
        if (!view.window || view.percent <= 0) {
            [view invalidateBlurAnimator];
            return;
        }
        if (!view.blurAnimator || view.blurAnimator.state != UIViewAnimatingStateActive ||
            view.animatorWindow != view.window) {
            [view invalidateBlurAnimator];
            BOOL animationsEnabled = UIView.areAnimationsEnabled;
            if (!animationsEnabled) [UIView setAnimationsEnabled:YES];
            @try {
                UIBlurEffectStyle style = view.blurStyle ?: (view.isDarkStyle
                    ? UIBlurEffectStyleSystemUltraThinMaterialDark
                    : UIBlurEffectStyleSystemUltraThinMaterialLight);
                __weak HGBackdropBlurView *weakBlur = view;
                UIViewPropertyAnimator *animator = [[UIViewPropertyAnimator alloc]
                    initWithDuration:1.0 curve:UIViewAnimationCurveLinear animations:^{
                        weakBlur.effect = [UIBlurEffect effectWithStyle:style];
                    }];
                animator.pausesOnCompletion = YES;
                animator.scrubsLinearly = YES;
                [animator startAnimation];
                [animator pauseAnimation];
                view.blurAnimator = animator;
                view.animatorWindow = view.window;
            } @finally {
                if (!animationsEnabled) [UIView setAnimationsEnabled:NO];
            }
        }
        CGFloat linear = HGBlurFraction((int)view.percent);
        CGFloat fraction = view.commentPureMode ? linear : (CGFloat)pow(linear, 1.8);
        if (view.blurAnimator.fractionComplete != fraction) view.blurAnimator.fractionComplete = fraction;
    });
}
@end

static char HGSurfaceBackdropBlurKey;

static void HGClearSurfaceBackdropBlur(UIView *owner) {
    if (!owner) return;
    HGBackdropBlurView *blur = objc_getAssociatedObject(owner, &HGSurfaceBackdropBlurKey);
    if (blur) {
        [blur invalidateBlurAnimator];
        [blur removeFromSuperview];
        objc_setAssociatedObject(owner, &HGSurfaceBackdropBlurKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
}

static void HGApplySurfaceBackdropBlur(UIView *owner, UIView *container, UIView *anchor,
                                       CGRect frame, CGFloat radius, BOOL dark,
                                       NSString *materialStyle, double blurPercentValue) {
    if (!owner) return;
    NSInteger strength = HGBlurPercent(blurPercentValue);
    BOOL isGaussian = [materialStyle isEqualToString:@"blur"];
    BOOL needsView = (strength > 0) && !UIAccessibilityIsReduceTransparencyEnabled();
    if (!container || !needsView || CGRectIsEmpty(frame)) {
        HGClearSurfaceBackdropBlur(owner);
        return;
    }
    HGBackdropBlurView *blur = objc_getAssociatedObject(owner, &HGSurfaceBackdropBlurKey);
    if (!blur) {
        blur = [[HGBackdropBlurView alloc] initWithEffect:nil];
        blur.userInteractionEnabled = NO;
        blur.accessibilityElementsHidden = YES;
        blur.layer.cornerCurve = kCACornerCurveContinuous;
        blur.clipsToBounds = YES;
        objc_setAssociatedObject(owner, &HGSurfaceBackdropBlurKey, blur, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    if (blur.superview != container) {
        if (anchor && anchor.superview == container) [container insertSubview:blur belowSubview:anchor];
        else [container insertSubview:blur atIndex:0];
    }
    if (!CGRectEqualToRect(blur.frame, frame)) blur.frame = frame;
    CGFloat half = MIN(CGRectGetWidth(frame), CGRectGetHeight(frame)) * 0.5;
    CGFloat corner = (radius < 0.0) ? half : MIN(radius, half);
    if (fabs(blur.layer.cornerRadius - corner) > 0.25) blur.layer.cornerRadius = corner;

    CGFloat baseTint = 0.0;
    CGFloat maxTint = dark ? 0.26 : 0.24;
    BOOL chromeBorder = NO;
    if ([materialStyle isEqualToString:@"frosted"]) {
        maxTint = dark ? 0.38 : 0.36;
    } else if ([materialStyle isEqualToString:@"regular"]) {
        maxTint = dark ? 0.30 : 0.28;
    } else if (isGaussian) {
        baseTint = dark ? 0.24 : 0.28;
        maxTint = dark ? 0.64 : 0.70;
        chromeBorder = YES;
    }
    [blur configurePercent:strength dark:dark baseTint:baseTint maxTint:maxTint chromeBorder:chromeBorder];
}
