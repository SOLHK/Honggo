// Pure CoreAnimation selection pill surfaces for "柔性粉雾" (silky matte
// sakura-blush to cool pearl) and "粉雾玻璃" (translucent sakura-mist liquid
// glass). Designed as zero-subview-mutation CALayers for private UIKit
// selection views and thin UIView wrappers for plugin-owned pills.

typedef NS_ENUM(NSInteger, HGASelectionColorMode) {
    HGASelectionColorDefault = 0,
    HGASelectionColorSoftRoseMist = 1,
    HGASelectionColorRoseMistGlass = 2,
};

static HGASelectionColorMode HGASelectionColorModeForKey(NSString *key) {
    NSString *value = HGAppearanceString(key);
    if ([value isEqual:@"softRoseMist"]) return HGASelectionColorSoftRoseMist;
    if ([value isEqual:@"roseMistGlass"]) return HGASelectionColorRoseMistGlass;
    return HGASelectionColorDefault;
}

static UIColor *HGASoftRoseMistInkColor(void) {
    return [UIColor colorWithRed:20.0 / 255.0 green:19.0 / 255.0 blue:22.0 / 255.0 alpha:1.0];
}

@interface HGASelectedPillLayer : CALayer
@property(nonatomic) HGASelectionColorMode colorMode;
@property(nonatomic) BOOL lightBackdrop;
@property(nonatomic) CGFloat pillCornerRadius;
@end

@implementation HGASelectedPillLayer {
    CALayer *_clipLayer;
    CALayer *_baseFillLayer;
    CAGradientLayer *_pinkGlowLayer;
    CAGradientLayer *_topBevelLayer;
    CAGradientLayer *_rimGradientLayer;
    CAShapeLayer *_rimMaskLayer;
}

- (instancetype)init {
    self = [super init];
    if (!self) return nil;
    _colorMode = HGASelectionColorSoftRoseMist;
    _lightBackdrop = YES;
    _pillCornerRadius = 0;
    [self hga_setupLayers];
    [self hga_applyPalette];
    return self;
}

- (instancetype)initWithLayer:(id)layer {
    self = [super initWithLayer:layer];
    if (!self) return nil;
    if ([layer isKindOfClass:HGASelectedPillLayer.class]) {
        HGASelectedPillLayer *other = (HGASelectedPillLayer *)layer;
        _colorMode = other.colorMode;
        _lightBackdrop = other.lightBackdrop;
        _pillCornerRadius = other.pillCornerRadius;
    }
    return self;
}

- (void)hga_setupLayers {
    self.masksToBounds = NO;

    _clipLayer = [CALayer layer];
    _clipLayer.masksToBounds = YES;
    _clipLayer.cornerCurve = kCACornerCurveContinuous;
    [self addSublayer:_clipLayer];

    _baseFillLayer = [CALayer layer];
    [_clipLayer addSublayer:_baseFillLayer];

    _pinkGlowLayer = [CAGradientLayer layer];
    _pinkGlowLayer.type = kCAGradientLayerRadial;
    _pinkGlowLayer.startPoint = CGPointMake(0.10, 0.12);
    _pinkGlowLayer.endPoint = CGPointMake(0.84, 0.96);
    _pinkGlowLayer.locations = @[@0.0, @0.26, @0.58, @0.84, @1.0];
    [_clipLayer addSublayer:_pinkGlowLayer];

    _topBevelLayer = [CAGradientLayer layer];
    [_clipLayer addSublayer:_topBevelLayer];

    _rimGradientLayer = [CAGradientLayer layer];
    _rimGradientLayer.startPoint = CGPointMake(0.0, 0.0);
    _rimGradientLayer.endPoint = CGPointMake(1.0, 1.0);
    _rimGradientLayer.locations = @[@0.0, @0.32, @0.68, @1.0];

    _rimMaskLayer = [CAShapeLayer layer];
    _rimMaskLayer.fillColor = NULL;
    _rimMaskLayer.strokeColor = UIColor.blackColor.CGColor;
    _rimMaskLayer.lineWidth = 0.75;
    _rimMaskLayer.contentsScale = UIScreen.mainScreen.scale;
    _rimGradientLayer.mask = _rimMaskLayer;
    [self addSublayer:_rimGradientLayer];
}

- (void)setColorMode:(HGASelectionColorMode)colorMode {
    if (_colorMode == colorMode) return;
    _colorMode = colorMode;
    [self hga_applyPalette];
}

- (void)setLightBackdrop:(BOOL)lightBackdrop {
    if (_lightBackdrop == lightBackdrop) return;
    _lightBackdrop = lightBackdrop;
    [self hga_applyPalette];
}

- (void)setPillCornerRadius:(CGFloat)pillCornerRadius {
    if (fabs(_pillCornerRadius - pillCornerRadius) < 0.01) return;
    _pillCornerRadius = pillCornerRadius;
    [self hga_syncGeometry];
}

- (void)hga_applyPalette {
    if (!_baseFillLayer || !_pinkGlowLayer) return;
    BOOL matte = (_colorMode != HGASelectionColorRoseMistGlass);
    BOOL lightPalette = matte || _lightBackdrop;
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    if (lightPalette) {
        // Silky porcelain-pearl (#EAE9EE) base + upper-left sakura blush glow
        // (#EEB1C2 -> #E7C5CE -> #E7E6E9). RoseMistGlass relaxes opacity on
        // light surfaces so underlying Liquid Glass refraction stays live.
        CGFloat baseAlpha = matte ? 0.96 : 0.44;
        CGFloat peakAlpha = matte ? 0.62 : 0.56;
        CGFloat midAlpha = matte ? 0.38 : 0.34;
        CGFloat softAlpha = matte ? 0.14 : 0.13;
        _baseFillLayer.backgroundColor = [UIColor colorWithRed:234.0 / 255.0
                                                         green:233.0 / 255.0
                                                          blue:238.0 / 255.0
                                                         alpha:baseAlpha].CGColor;
        _pinkGlowLayer.colors = @[
            (__bridge id)[UIColor colorWithRed:242.0 / 255.0 green:142.0 / 255.0 blue:168.0 / 255.0 alpha:peakAlpha].CGColor,
            (__bridge id)[UIColor colorWithRed:241.0 / 255.0 green:165.0 / 255.0 blue:185.0 / 255.0 alpha:midAlpha].CGColor,
            (__bridge id)[UIColor colorWithRed:237.0 / 255.0 green:196.0 / 255.0 blue:207.0 / 255.0 alpha:softAlpha].CGColor,
            (__bridge id)[UIColor colorWithRed:234.0 / 255.0 green:224.0 / 255.0 blue:230.0 / 255.0 alpha:0.04].CGColor,
            (__bridge id)[UIColor colorWithRed:234.0 / 255.0 green:233.0 / 255.0 blue:238.0 / 255.0 alpha:0.00].CGColor,
        ];
        _topBevelLayer.startPoint = CGPointMake(0.5, 0.0);
        _topBevelLayer.endPoint = CGPointMake(0.5, 0.24);
        _topBevelLayer.locations = @[@0.0, @1.0];
        _topBevelLayer.colors = @[
            (__bridge id)[UIColor colorWithRed:178.0 / 255.0 green:138.0 / 255.0 blue:150.0 / 255.0 alpha:0.09].CGColor,
            (__bridge id)[UIColor colorWithRed:234.0 / 255.0 green:233.0 / 255.0 blue:238.0 / 255.0 alpha:0.00].CGColor,
        ];
        _rimGradientLayer.colors = @[
            (__bridge id)[UIColor colorWithRed:228.0 / 255.0 green:158.0 / 255.0 blue:178.0 / 255.0 alpha:0.82].CGColor,
            (__bridge id)[UIColor colorWithRed:222.0 / 255.0 green:186.0 / 255.0 blue:198.0 / 255.0 alpha:0.48].CGColor,
            (__bridge id)[UIColor colorWithRed:210.0 / 255.0 green:208.0 / 255.0 blue:216.0 / 255.0 alpha:0.44].CGColor,
            (__bridge id)[UIColor colorWithRed:224.0 / 255.0 green:222.0 / 255.0 blue:230.0 / 255.0 alpha:0.62].CGColor,
        ];
    } else {
        // Dark-surface RoseMistGlass: luminous translucent sakura-rose mist
        // over dark video or dark-mode Liquid Glass with pearl specular rim.
        _baseFillLayer.backgroundColor = [UIColor colorWithRed:240.0 / 255.0
                                                         green:236.0 / 255.0
                                                          blue:244.0 / 255.0
                                                         alpha:0.16].CGColor;
        _pinkGlowLayer.colors = @[
            (__bridge id)[UIColor colorWithRed:246.0 / 255.0 green:150.0 / 255.0 blue:178.0 / 255.0 alpha:0.56].CGColor,
            (__bridge id)[UIColor colorWithRed:242.0 / 255.0 green:168.0 / 255.0 blue:190.0 / 255.0 alpha:0.36].CGColor,
            (__bridge id)[UIColor colorWithRed:238.0 / 255.0 green:196.0 / 255.0 blue:210.0 / 255.0 alpha:0.15].CGColor,
            (__bridge id)[UIColor colorWithRed:236.0 / 255.0 green:224.0 / 255.0 blue:232.0 / 255.0 alpha:0.04].CGColor,
            (__bridge id)[UIColor colorWithRed:240.0 / 255.0 green:236.0 / 255.0 blue:244.0 / 255.0 alpha:0.00].CGColor,
        ];
        _topBevelLayer.startPoint = CGPointMake(0.0, 0.0);
        _topBevelLayer.endPoint = CGPointMake(1.0, 1.0);
        _topBevelLayer.locations = @[@0.0, @0.45, @1.0];
        _topBevelLayer.colors = @[
            (__bridge id)[UIColor colorWithRed:255.0 / 255.0 green:224.0 / 255.0 blue:236.0 / 255.0 alpha:0.08].CGColor,
            (__bridge id)[UIColor colorWithRed:245.0 / 255.0 green:242.0 / 255.0 blue:248.0 / 255.0 alpha:0.00].CGColor,
            (__bridge id)[UIColor colorWithRed:238.0 / 255.0 green:236.0 / 255.0 blue:246.0 / 255.0 alpha:0.10].CGColor,
        ];
        _rimGradientLayer.colors = @[
            (__bridge id)[UIColor colorWithRed:252.0 / 255.0 green:184.0 / 255.0 blue:204.0 / 255.0 alpha:0.88].CGColor,
            (__bridge id)[UIColor colorWithRed:244.0 / 255.0 green:198.0 / 255.0 blue:214.0 / 255.0 alpha:0.42].CGColor,
            (__bridge id)[UIColor colorWithRed:236.0 / 255.0 green:234.0 / 255.0 blue:244.0 / 255.0 alpha:0.28].CGColor,
            (__bridge id)[UIColor colorWithRed:248.0 / 255.0 green:246.0 / 255.0 blue:252.0 / 255.0 alpha:0.52].CGColor,
        ];
    }
    [CATransaction commit];
}

- (void)hga_syncGeometry {
    if (!_clipLayer || !_rimMaskLayer) return;
    CGRect b = self.bounds;
    if (!isfinite(b.size.width) || !isfinite(b.size.height) || b.size.width <= 0.5 || b.size.height <= 0.5) return;
    CGFloat maxRadius = MIN(b.size.width, b.size.height) * 0.5;
    CGFloat radius = (_pillCornerRadius > 0.5) ? MIN(_pillCornerRadius, maxRadius) : maxRadius;
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    _clipLayer.frame = b;
    _clipLayer.cornerRadius = radius;
    _baseFillLayer.frame = b;
    _pinkGlowLayer.frame = b;
    _topBevelLayer.frame = b;
    _rimGradientLayer.frame = b;
    CGRect inset = CGRectInset(b, 0.375, 0.375);
    CGFloat ir = MAX(0.0, radius - 0.375);
    _rimMaskLayer.frame = b;
    _rimMaskLayer.path = [UIBezierPath bezierPathWithRoundedRect:inset cornerRadius:ir].CGPath;
    [CATransaction commit];
}

- (void)setBounds:(CGRect)bounds {
    BOOL changed = !CGSizeEqualToSize(self.bounds.size, bounds.size);
    [super setBounds:bounds];
    if (changed) [self hga_syncGeometry];
}

- (void)setFrame:(CGRect)frame {
    BOOL changed = !CGSizeEqualToSize(self.frame.size, frame.size);
    [super setFrame:frame];
    if (changed) [self hga_syncGeometry];
}

- (void)layoutSublayers {
    [super layoutSublayers];
    [self hga_syncGeometry];
}

@end

@interface HGASelectedPillView : UIView
@property(nonatomic) HGASelectionColorMode colorMode;
@property(nonatomic) BOOL lightBackdrop;
@property(nonatomic) CGFloat pillCornerRadius;
@end

@implementation HGASelectedPillView

+ (Class)layerClass {
    return HGASelectedPillLayer.class;
}

- (instancetype)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (!self) return nil;
    self.userInteractionEnabled = NO;
    self.accessibilityElementsHidden = YES;
    self.isAccessibilityElement = NO;
    self.backgroundColor = UIColor.clearColor;
    self.opaque = NO;
    self.clipsToBounds = NO;
    self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    return self;
}

- (HGASelectionColorMode)colorMode {
    return ((HGASelectedPillLayer *)self.layer).colorMode;
}

- (void)setColorMode:(HGASelectionColorMode)colorMode {
    ((HGASelectedPillLayer *)self.layer).colorMode = colorMode;
}

- (BOOL)lightBackdrop {
    return ((HGASelectedPillLayer *)self.layer).lightBackdrop;
}

- (void)setLightBackdrop:(BOOL)lightBackdrop {
    ((HGASelectedPillLayer *)self.layer).lightBackdrop = lightBackdrop;
}

- (CGFloat)pillCornerRadius {
    return ((HGASelectedPillLayer *)self.layer).pillCornerRadius;
}

- (void)setPillCornerRadius:(CGFloat)pillCornerRadius {
    ((HGASelectedPillLayer *)self.layer).pillCornerRadius = pillCornerRadius;
}

@end

static char HGASelectedPillViewKey;
static char HGASelectedPillLayerKey;

static HGASelectedPillView *HGASyncSelectedPillViewInHost(UIView *host,
                                                          HGASelectionColorMode mode,
                                                          BOOL lightBackdrop,
                                                          CGFloat cornerRadius,
                                                          CGFloat alpha,
                                                          NSString *identifier) {
    if (!host) return nil;
    UIView *container = [host isKindOfClass:UIVisualEffectView.class]
        ? ((UIVisualEffectView *)host).contentView : host;
    HGASelectedPillView *pill = objc_getAssociatedObject(host, &HGASelectedPillViewKey);
    if (mode == HGASelectionColorDefault) {
        if (pill) {
            [pill removeFromSuperview];
            objc_setAssociatedObject(host, &HGASelectedPillViewKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        return nil;
    }
    if (!pill) {
        pill = [[HGASelectedPillView alloc] initWithFrame:container.bounds];
        pill.accessibilityIdentifier = identifier ?: @"honggo.appearance.selection.pill";
        objc_setAssociatedObject(host, &HGASelectedPillViewKey, pill, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    if (identifier && ![pill.accessibilityIdentifier isEqual:identifier]) {
        pill.accessibilityIdentifier = identifier;
    }
    pill.colorMode = mode;
    pill.lightBackdrop = lightBackdrop;
    pill.pillCornerRadius = cornerRadius;
    if (fabs(pill.alpha - alpha) > 0.005) pill.alpha = alpha;
    if (pill.superview != container) [container insertSubview:pill atIndex:0];
    if (!CGRectEqualToRect(pill.frame, container.bounds)) pill.frame = container.bounds;
    if (pill.hidden) pill.hidden = NO;
    return pill;
}

static void HGASyncSelectionLayerBounds(UIView *selView) {
    if (!selView) return;
    HGASelectedPillLayer *pillLayer = objc_getAssociatedObject(selView, &HGASelectedPillLayerKey);
    if (!pillLayer) return;
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    if (!CGRectEqualToRect(pillLayer.frame, selView.bounds)) {
        pillLayer.frame = selView.bounds;
    }
    [CATransaction commit];
}

static void HGAEnsureTabSelectionBoundsHook(Class targetClass) {
    if (![targetClass isSubclassOfClass:UIView.class]) return;
    static NSMutableSet<NSString *> *hooked;
    static dispatch_once_t once;
    dispatch_once(&once, ^{ hooked = [NSMutableSet set]; });
    NSString *name = NSStringFromClass(targetClass);
    if (!name.length || [hooked containsObject:name]) return;
    // Scope strictly to UIKit's private floating tab selection surfaces.
    if (![name containsString:@"TabSelectionView"] && ![name containsString:@"SelectionPlatter"]) return;
    Method method = class_getInstanceMethod(targetClass, @selector(layoutSubviews));
    const char *enc = method ? method_getTypeEncoding(method) : NULL;
    if (!enc || strcmp(enc, "v16@0:8") != 0) return;
    __block IMP original = method_getImplementation(method);
    IMP replacement = imp_implementationWithBlock(^(UIView *selfView) {
        ((void (*)(id, SEL))original)(selfView, @selector(layoutSubviews));
        HGASyncSelectionLayerBounds(selfView);
    });
    IMP installed = HGInstallVerifiedHook(targetClass, @selector(layoutSubviews), "v16@0:8", replacement, @"appearance");
    if (installed) {
        original = installed;
        [hooked addObject:name];
    } else {
        imp_removeBlock(replacement);
    }
}

static HGASelectedPillLayer *HGASyncSelectedPillLayerInView(UIView *selView,
                                                            HGASelectionColorMode mode,
                                                            BOOL lightBackdrop) {
    if (!selView) return nil;
    HGASelectedPillLayer *pillLayer = objc_getAssociatedObject(selView, &HGASelectedPillLayerKey);
    if (mode == HGASelectionColorDefault) {
        if (pillLayer) {
            [pillLayer removeFromSuperlayer];
            objc_setAssociatedObject(selView, &HGASelectedPillLayerKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        return nil;
    }
    HGAEnsureTabSelectionBoundsHook(selView.class);
    if (!pillLayer) {
        pillLayer = [HGASelectedPillLayer layer];
        pillLayer.name = @"honggo.appearance.dock.selection-pill";
        objc_setAssociatedObject(selView, &HGASelectedPillLayerKey, pillLayer, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    pillLayer.colorMode = mode;
    pillLayer.lightBackdrop = lightBackdrop;
    pillLayer.pillCornerRadius = 0;
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    if (pillLayer.superlayer != selView.layer) {
        [selView.layer addSublayer:pillLayer];
    }
    if (!CGRectEqualToRect(pillLayer.frame, selView.bounds)) {
        pillLayer.frame = selView.bounds;
    }
    [CATransaction commit];
    return pillLayer;
}
