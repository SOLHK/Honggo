#import <UIKit/UIGestureRecognizerSubclass.h>
#import "DockSelectionGeometry.h"

@class HGAAppearanceDock;
@interface HGADockPointer : UIGestureRecognizer
@property(nonatomic,weak) HGAAppearanceDock *dock;
@property(nonatomic,weak) UITouch *trackedTouch;
@property(nonatomic) CGPoint dockPoint;
@end

@interface HGAAppearanceDock : UIView <UIGestureRecognizerDelegate, UITabBarDelegate, UITabBarControllerDelegate>
@property(nonatomic,weak) UIView *hostBar;
@property(nonatomic,weak) UIViewController *hostController;
@property(nonatomic,strong) UITabBar *systemTabBar;
@property(nonatomic,strong) UITabBarController *systemTabController;
@property(nonatomic,weak) UIView *systemPlatter;
@property(nonatomic,weak) UIView *systemWrapper;
@property(nonatomic,weak) UIView *customSystemPlatter;
@property(nonatomic,strong) UIVisualEffect *installedSystemGlass;
@property(nonatomic,copy) NSString *systemMaterialStyle;
@property(nonatomic,copy) NSArray *systemItemSignature;
@property(nonatomic,copy) NSArray *failedSystemProbe;
@property(nonatomic,copy) NSArray *systemRendererRecord;
@property(nonatomic) CGRect systemProbeFrame;
@property(nonatomic) CGRect systemCapsuleFrame;
@property(nonatomic) NSInteger systemHostSelection;
@property(nonatomic) BOOL systemDockActive;
@property(nonatomic) BOOL quietSystemTouches;
@property(nonatomic,copy) NSArray<NSValue *> *systemHitSlots;
@property(nonatomic) BOOL systemDockSyncing;
@property(nonatomic) BOOL deliveringTabAction;
@property(nonatomic) BOOL systemWarmupPending;
@property(nonatomic) BOOL systemWarmupComplete;
@property(nonatomic,strong) NSMapTable<NSString *,UIControl *> *targets;
@property(nonatomic,strong) NSMapTable<UIView *,NSNumber *> *hiddenSources;
@property(nonatomic,strong) UIColor *hostBackground;
@property(nonatomic) BOOL backgroundOwned;
@property(nonatomic) BOOL clearingHostFill;
@property(nonatomic) BOOL hostOpaque;
@property(nonatomic,strong) UIView *capsule;
@property(nonatomic,strong) UIView *selection;
@property(nonatomic,strong) UIView *selectionHighlight;
@property(nonatomic,weak) UIButton *motionGlyph;
@property(nonatomic,strong) UIView *extra;
@property(nonatomic,strong) UIView *extraSelection;
@property(nonatomic,strong) UIView *expand;
@property(nonatomic,strong) UIButton *extraButton;
@property(nonatomic,strong) UIButton *expandButton;
@property(nonatomic,strong) NSMutableDictionary<NSString *,UIButton *> *buttons;
@property(nonatomic,strong) NSArray<UISwipeGestureRecognizer *> *swipes;
@property(nonatomic,strong) UIPanGestureRecognizer *selectionPan;
@property(nonatomic,strong) HGADockPointer *dockTouch;
@property(nonatomic,weak) UIButton *dockTouchButton;
@property(nonatomic) CGPoint dockTouchStart;
@property(nonatomic,weak) UISegmentedControl *flatSegments;
@property(nonatomic,strong) UIVisualEffectView *dragLens;
@property(nonatomic,strong) UIVisualEffectView *quietDragLens;
@property(nonatomic,strong) NSMapTable<UIView *,UIView *> *quietSelectionSources;
@property(nonatomic,weak) UIView *quietProvider;
@property(nonatomic,strong) UIVisualEffectView *restingLens;
@property(nonatomic,strong) UIVisualEffectView *extraRestingLens;
@property(nonatomic) BOOL motionMaterialPresented;
@property(nonatomic,weak) UIButton *pressedTab;
@property(nonatomic) NSUInteger selectionGeneration;
@property(nonatomic) BOOL selectionDragging;
@property(nonatomic) BOOL selectionSettling;
@property(nonatomic) CGFloat selectionDragX;
@property(nonatomic) CGRect restingSelectionFrame;
@property(nonatomic) NSInteger restingFlatIndex;
@property(nonatomic) BOOL collapsed;
@property(nonatomic) BOOL sidebarSuspended;
@property(nonatomic) BOOL rendering;
@property(nonatomic) BOOL restoring;
@property(nonatomic) CGRect lastOccupiedRect;
- (void)refresh;
- (void)restore;
- (void)setCollapsed:(BOOL)value animated:(BOOL)animated;
- (void)dragSelection:(UIPanGestureRecognizer *)gesture;
- (void)beginDockTouchAtPoint:(CGPoint)point;
- (void)moveDockTouchToPoint:(CGPoint)point;
- (void)endDockTouchAtPoint:(CGPoint)point cancelled:(BOOL)cancelled;
- (void)removeSystemDock;
@end

@implementation HGADockPointer
- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    if(self.trackedTouch || touches.count!=1) {
        self.state=self.trackedTouch ? UIGestureRecognizerStateCancelled : UIGestureRecognizerStateFailed;
        return;
    }
    UITouch *touch=touches.anyObject;
    self.trackedTouch=touch;
    self.dockPoint=[touch locationInView:self.dock];
    [self.dock beginDockTouchAtPoint:self.dockPoint];
    self.state=UIGestureRecognizerStateBegan;
}
- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    UITouch *touch=self.trackedTouch;
    if(!touch || ![touches containsObject:touch])return;
    self.dockPoint=[touch locationInView:self.dock];
    [self.dock moveDockTouchToPoint:self.dockPoint];
    self.state=UIGestureRecognizerStateChanged;
}
- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    UITouch *touch=self.trackedTouch;
    if(!touch || ![touches containsObject:touch])return;
    self.dockPoint=[touch locationInView:self.dock];
    self.state=UIGestureRecognizerStateEnded;
}
- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    self.state=UIGestureRecognizerStateCancelled;
}
- (void)reset {
    [self.dock endDockTouchAtPoint:self.dockPoint cancelled:YES];
    self.trackedTouch=nil;
    [super reset];
}
@end
static char HGADockKey;
static void (*HGADockOriginalImpact)(id,SEL);
static void HGADockHostImpact(id controller,SEL cmd) {
    if(HGAppearanceDockEnabled() && !HGAppearanceFlag(@"dock.haptics")) {
        UIView *bar=HGReadObjectGetter(controller,"customTabBar");
        HGAAppearanceDock *dock=objc_getAssociatedObject(bar,&HGADockKey);
        if(bar && dock.superview==bar)return;
    }
    HGADockOriginalImpact(controller,cmd);
}
static void (*HGADockOriginalBarColor)(UIView *,SEL,UIColor *);
static void HGADockSetBarColor(UIView *bar,SEL selector,UIColor *color) {
    HGAAppearanceDock *dock=objc_getAssociatedObject(bar,&HGADockKey);
    if(dock.backgroundOwned && !dock.clearingHostFill && !dock.restoring &&
        bar.window && dock.superview==bar) {
        // 7.4.5.33 settles the selected page's light/dark fill directly on
        // SSTabBar between layout passes. While the sidebar moves the bar,
        // keep this same paint ownership without touching its layout.
        dock.hostBackground=color;
        color=UIColor.clearColor;
    }
    HGADockOriginalBarColor(bar,selector,color);
}
static BOOL HGADockInstallHostHitRoute(void);
static const CGFloat HGADockAccessoryGap=12;
static CGFloat HGADockControlHeight(UIView *bar) {
    // Keep the larger dock inside the native hit-test bounds and leave a gap
    // above the home indicator even when the original safe-area inset is tall.
    return bar.bounds.size.height-MIN(bar.safeAreaInsets.bottom,14);
}
static CGRect HGADockRectInView(UIView *coordinateView) {
    if(!HGAppearanceDockEnabled() || !coordinateView.window)return CGRectNull;
    CGRect result=CGRectNull;
    for(UIView *bar in HGABars.allObjects) {
        HGAAppearanceDock *dock=objc_getAssociatedObject(bar,&HGADockKey);
        if(dock.window!=coordinateView.window || !HGAVisible(dock))continue;
        for(UIView *surface in @[dock.capsule,dock.extra,dock.expand]) {
            if(!HGAVisible(surface))continue;
            CGRect rect=[surface convertRect:surface.bounds toView:coordinateView];
            result=CGRectIsNull(result)?rect:CGRectUnion(result,rect);
        }
    }
    return result;
}
static CGRect HGAExpandedCapsuleInWindow(UIWindow *window) {
    if(!window || !HGAppearanceDockEnabled())return CGRectNull;
    for(UIView *bar in HGABars.allObjects) {
        HGAAppearanceDock *dock=objc_getAssociatedObject(bar,&HGADockKey);
        if(dock.window==window && !dock.collapsed && HGAVisible(dock.capsule))
            return [dock.capsule convertRect:dock.capsule.bounds toView:window];
    }
    return CGRectNull;
}
static void HGAInvalidateDockDependentLayout(UIWindow *window) {
    BOOL sync=HGAppearanceFlag(@"player.episodeDockWidth");
    for(UIView *view in HGAManaged.allObjects) {
        if(view.window==window && (HGAIsBottomControl(view) || (sync && [HGARole(view) isEqual:@"episodes"])))
            [view setNeedsLayout];
    }
}
static NSArray *HGATabIDs(void){return @[@"home",@"theater",@"store",@"mine"];}
static NSString *HGADockDetachedTabKey(NSArray<NSString *> *visible) {
    if(!HGAppearanceFlag(@"dock.extra") || visible.count<2)return nil;
    NSString *action=HGAppearanceString(@"dock.action");
    return [HGATabIDs() containsObject:action] && [visible containsObject:action] ? action : nil;
}
static NSMutableArray<NSString *> *HGADockCapsuleTabs(void) {
    NSMutableArray<NSString *> *visible=[NSMutableArray array];
    for(NSString *key in HGATabIDs())
        if(!HGAppearanceFlag([@"dock.hide." stringByAppendingString:key]))[visible addObject:key];
    NSString *detached=HGADockDetachedTabKey(visible);
    if(detached)[visible removeObject:detached];
    return visible;
}
static CGFloat HGAExpandedDockWidth(CGFloat width,CGFloat controlHeight,NSUInteger visibleCount) {
    CGFloat reserve=HGAppearanceFlag(@"dock.extra") ?
        MIN(HGAppearanceNumber(@"dock.extraSize"),controlHeight)+HGADockAccessoryGap : 0;
    CGFloat ratio=HGAppearanceNumber(@"dock.width");
    return HGADockRowWidth(width,ratio,reserve,(unsigned)visibleCount,(unsigned)HGATabIDs().count,
        fabs(ratio-HGADockDefaultWidth)<0.00001);
}
static NSArray *HGATabTitles(void){return @[@"首页",@"剧场",@"商城",@"我的"];}
static NSArray *HGATabSymbols(void){return @[@"house",@"play.rectangle",@"bag",@"person"];}
@interface HGADockTabButton : UIButton
@end
@implementation HGADockTabButton
- (BOOL)accessibilityActivate {
    if(self.hidden || !self.enabled)return NO;
    [self sendActionsForControlEvents:UIControlEventTouchUpInside];
    return YES;
}
- (void)layoutSubviews {
    [super layoutSubviews];
    // The configuration-based button can give its title a zero-height slot
    // in a compact Dock. Reserve space for the caption before fitting the icon.
    CGRect bounds=self.bounds;
    CGFloat width=MAX(0,CGRectGetWidth(bounds)-4),height=MAX(0,CGRectGetHeight(bounds)-4);
    CGFloat titleHeight=self.currentTitle.length ? MIN(height,ceil(self.titleLabel.font.lineHeight)) : 0;
    UIImage *image=self.currentImage;
    CGFloat gap=image && titleHeight ? 2 : 0;
    CGSize glyph=image ? image.size : CGSizeZero;
    if(glyph.width>0 && glyph.height>0) {
        CGFloat scale=MIN(1,MIN(width/glyph.width,MAX(0,height-titleHeight-gap)/glyph.height));
        glyph=CGSizeMake(glyph.width*scale,glyph.height*scale);
    }
    CGFloat x=CGRectGetMidX(bounds),top=CGRectGetMidY(bounds)-(glyph.height+gap+titleHeight)/2;
    self.imageView.hidden=!image;
    self.imageView.bounds=(CGRect){CGPointZero,glyph};
    self.imageView.center=CGPointMake(x,top+glyph.height/2);
    self.titleLabel.hidden=titleHeight==0;
    self.titleLabel.bounds=CGRectMake(0,0,width,titleHeight);
    self.titleLabel.center=CGPointMake(x,top+glyph.height+gap+titleHeight/2);
}
@end
static BOOL HGADockLiquidSelection(void) {
    return [HGAppearanceString(@"dock.selectionStyle") isEqual:@"glass"];
}
static UIVisualEffectView *HGADockRestingLens(UIVisualEffectView *lens,UIView *selection,CGRect frame,BOOL enabled) {
    if(!enabled || CGRectIsNull(frame)) {
        lens.effect=nil;[lens removeFromSuperview];return nil;
    }
    if(@available(iOS 26.0,*)) {
        if(!lens) {
            lens=[[UIVisualEffectView alloc] initWithEffect:nil];
            lens.accessibilityIdentifier=@"honggo.appearance.dock.resting-lens";
            lens.accessibilityElementsHidden=YES;lens.userInteractionEnabled=NO;
            lens.cornerConfiguration=UICornerConfiguration.capsuleConfiguration;
        }
        if(!CGRectEqualToRect(lens.frame,frame))lens.frame=frame;
        if(lens.superview!=selection.superview)[selection.superview insertSubview:lens aboveSubview:selection];
        if(!lens.effect)lens.effect=[UIGlassEffect effectWithStyle:UIGlassEffectStyleClear];
    }
    return lens;
}
static UIButton *HGADockButton(void) {
    UIButton *button=[UIButton buttonWithType:UIButtonTypeCustom];
    // System buttons promote their selected configuration to the app's accent
    // color. Selection belongs to our neutral glass pill, not that blue fill.
    button.automaticallyUpdatesConfiguration=NO;
    button.configurationUpdateHandler=^(UIButton *control) {
        control.alpha=control.enabled ? (control.highlighted ? 0.72 : 1) : 0.4;
    };
    return button;
}
static void HGADockConfigureButton(UIButton *button,NSString *title,UIImage *image,
    CGFloat iconSize,UIFont *font,UIColor *color) {
    UIButtonConfiguration *config=[UIButtonConfiguration plainButtonConfiguration];
    config.automaticallyUpdateForSelection=NO;
    config.background=[UIBackgroundConfiguration clearConfiguration];
    config.background.backgroundColorTransformer=^UIColor *(UIColor *incoming){return UIColor.clearColor;};
    config.baseBackgroundColor=UIColor.clearColor;
    config.baseForegroundColor=color;
    config.imageColorTransformer=^UIColor *(UIColor *incoming){return color;};
    UIImageSymbolConfiguration *symbolConfig=[UIImageSymbolConfiguration
        configurationWithPointSize:iconSize weight:UIImageSymbolWeightMedium];
    if(image.isSymbolImage) {
        image=[image imageWithConfiguration:symbolConfig] ?:
            [image imageByApplyingSymbolConfiguration:symbolConfig] ?: image;
    }
    config.title=title;config.image=image;
    config.preferredSymbolConfigurationForImage=symbolConfig;
    config.imagePlacement=NSDirectionalRectEdgeTop;
    config.imagePadding=title.length && image ? 3 : 0;
    config.contentInsets=NSDirectionalEdgeInsetsMake(4,4,4,4);
    config.titleLineBreakMode=NSLineBreakByClipping;
    config.titleTextAttributesTransformer=^NSDictionary *(NSDictionary *attributes){
        NSMutableDictionary *updated=attributes.mutableCopy;
        updated[NSFontAttributeName]=font;
        updated[NSForegroundColorAttributeName]=color;
        return updated;
    };
    button.tintColor=color;
    button.configuration=config;
    button.alpha=button.enabled ? (button.highlighted ? 0.72 : 1) : 0.4;
}
BOOL HGAppearanceDockEnabled(void){return HGAppearanceEnabled() && HGAppearanceFlag(@"dock.enabled");}
static NSInteger HGASelectedTab(UIViewController *controller) {
    if([controller isKindOfClass:UITabBarController.class])return [(UITabBarController *)controller selectedIndex];
    SEL sel=NSSelectorFromString(@"selectedIndex");
    Method method=controller?class_getInstanceMethod(object_getClass(controller),sel):NULL;
    const char *type=method?method_getTypeEncoding(method):NULL;
    if(type && (!strcmp(type,"q16@0:8") || !strcmp(type,"Q16@0:8"))) {
        NSUInteger index=((NSUInteger(*)(id,SEL))objc_msgSend)(controller,sel);
        if(index<4)return index;
    }return NSNotFound;
}
static UIUserInterfaceStyle HGADockColorStyle(id value,UITraitCollection *traits,BOOL foreground) {
    if(![value isKindOfClass:UIColor.class])return UIUserInterfaceStyleUnspecified;
    UIColor *color=[(UIColor *)value resolvedColorWithTraitCollection:traits];
    CGFloat r,g,b,a;
    if(![color getRed:&r green:&g blue:&b alpha:&a] || a<0.85)return UIUserInterfaceStyleUnspecified;
    CGFloat brightness=0.2126*r+0.7152*g+0.0722*b;
    if(brightness>0.72)return foreground ? UIUserInterfaceStyleDark : UIUserInterfaceStyleLight;
    if(brightness<0.30)return foreground ? UIUserInterfaceStyleLight : UIUserInterfaceStyleDark;
    return UIUserInterfaceStyleUnspecified;
}
static UIUserInterfaceStyle HGADockPageStyle(UIViewController *controller,UIView *bar,NSInteger selected) {
    if(selected==0)return HGAVideoPageStyle(controller,bar);
    id appearance=HGReadObjectGetter(bar,"appearanceStyle");
    UIUserInterfaceStyle style=HGADockColorStyle(HGReadObjectGetter(appearance,"selectedImageColor"),bar.traitCollection,YES);
    if(style==UIUserInterfaceStyleUnspecified)
        style=HGADockColorStyle(HGReadObjectGetter(appearance,"backgroundColor"),bar.traitCollection,NO);
    if(style!=UIUserInterfaceStyleUnspecified)return style;

    UIViewController *page=[controller isKindOfClass:UITabBarController.class] ?
        [(UITabBarController *)controller selectedViewController] : nil;
    if([page isKindOfClass:UINavigationController.class])
        page=[(UINavigationController *)page visibleViewController];
    UIView *content=page.viewIfLoaded;
    if(content.window==bar.window && HGAVisible(content)) {
        style=HGADockColorStyle(content.backgroundColor,content.traitCollection,NO);
        if(style!=UIUserInterfaceStyleUnspecified)return style;
        UIStatusBarStyle status=page.preferredStatusBarStyle;
        if(status==UIStatusBarStyleLightContent)return UIUserInterfaceStyleDark;
        if(status==UIStatusBarStyleDarkContent)return UIUserInterfaceStyleLight;
        return content.traitCollection.userInterfaceStyle;
    }
    return bar.traitCollection.userInterfaceStyle;
}
static UIView *HGADockMaterialContent(UIView *surface,NSString *style,CGFloat radius,UIColor *fill,BOOL interactive) {
    HGAInstallGlass(surface,style,radius,0);
    HGAAppearanceMaterial *glass=HGAState(surface,NO).glass;
    // Native regular glass already diffuses its backdrop. A second system
    // material below it made light pages opaque and white in R21.
    glass.userInteractionEnabled=YES;
    glass.accessibilityElementsHidden=NO;
    glass.overlayColor=fill;
    glass.untintedGlass=YES;
    glass.interactiveGlass=interactive;
    [glass requestMaterialRefresh];
    return glass.contentView ?: surface;
}

// Use the same UIKit track and selected fill as the header's channel switch.
// The original buttons above it retain their images, actions and VoiceOver traits.
static char HGADockFlatBackingKey;
static UISegmentedControl *HGADockFlatBacking(UIView *surface,NSUInteger count) {
    UISegmentedControl *control=objc_getAssociatedObject(surface,&HGADockFlatBackingKey);
    if(control && control.numberOfSegments!=count) {
        [control removeFromSuperview];
        objc_setAssociatedObject(surface,&HGADockFlatBackingKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        control=nil;
    }
    if(!count)return nil;
    if(!control) {
        control=[UISegmentedControl new];
        for(NSUInteger i=0;i<count;i++)[control insertSegmentWithTitle:@"" atIndex:i animated:NO];
        control.semanticContentAttribute=UISemanticContentAttributeForceLeftToRight;
        control.userInteractionEnabled=NO;
        control.accessibilityElementsHidden=YES;
        control.accessibilityIdentifier=@"honggo.appearance.dock.flat";
        [surface insertSubview:control atIndex:0];
        objc_setAssociatedObject(surface,&HGADockFlatBackingKey,control,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    if(!CGRectEqualToRect(control.frame,surface.bounds))control.frame=surface.bounds;
    return control;
}

static CGRect HGADockSelectionFrame(CGFloat center,CGFloat width,CGFloat height,CGFloat preferredWidth,BOOL pressed) {
    HGADockSelectionBounds frame=HGADockSelectionBoundsForState(center,width,height,preferredWidth,pressed);
    return CGRectMake(frame.x,frame.y,frame.width,frame.height);
}

static BOOL HGADockUIKitPart(UIView *view,NSString *name) {
    NSString *type=NSStringFromClass(view.class);
    return [type isEqual:name] || [type hasSuffix:[@"." stringByAppendingString:name]] ||
        ([type hasPrefix:@"_TtC5UIKit"] && [type hasSuffix:name]);
}

static NSArray<UIView *> *HGADockSystemSubviews(UIView *root) {
    NSMutableArray<UIView *> *views=[root.subviews mutableCopy];
    for(NSUInteger index=0;index<views.count && index<96;index++) {
        for(UIView *child in views[index].subviews) {
            if(views.count>=96)break;
            [views addObject:child];
        }
    }
    return views;
}

static UIView *HGADockSystemPlatter(UITabBar *bar) {
    // iOS 27 may use a child item platter alongside its background effect.
    // Accept either of UIKit's known platter shapes, never a legacy tab bar
    // background that merely happens to be a wide blur.
    Class platterClass=NSClassFromString(@"UIKit._UITabBarPlatterView");
    UIView *platter=nil,*items=nil;
    for(UIView *view in HGADockSystemSubviews(bar)) {
        CGSize size=view.bounds.size;
        if(view.hidden || view.alpha<0.05 || size.height<24 || size.width<size.height*1.30)continue;
        if([view isKindOfClass:platterClass] || HGADockUIKitPart(view,@"_UITabBarPlatterView")) {
            if(!platter || size.width>platter.bounds.size.width)platter=view;
        }else if(HGADockUIKitPart(view,@"_UITabBarItemPlatterView") && (!items || size.width>items.bounds.size.width))items=view;
    }
    return platter ?: items;
}

static NSArray<UIView *> *HGADockSystemItems(UIView *root) {
    NSMutableArray<UIView *> *items=[NSMutableArray array];
    for(UIView *view in HGADockSystemSubviews(root)) {
        if(!view.hidden && view.alpha>0.05 && view.bounds.size.width>20 && view.bounds.size.height>18 &&
            (HGADockUIKitPart(view,@"_UITabButton") || HGADockUIKitPart(view,@"UITabBarButton") ||
                HGADockUIKitPart(view,@"_UITabBarItemView")))[items addObject:view];
    }
    return items;
}

static void HGADockSetInkEdge(UIButton *button,UIColor *edge) {
    CALayer *layer=button.layer;
    if(edge) {
        layer.shadowOffset=CGSizeZero;layer.shadowRadius=1;
        layer.shadowColor=edge.CGColor;layer.shadowOpacity=1;
    }else if(layer.shadowOpacity)layer.shadowOpacity=0;
}

static UIImage *HGADockSystemImage(NSString *symbol,NSString *caption,CGFloat size,UIColor *color,BOOL selected,UIColor *edge) {
    UIImageSymbolConfiguration *config=[UIImageSymbolConfiguration configurationWithPointSize:size weight:UIImageSymbolWeightMedium];
    UIImage *glyph=symbol ? [UIImage systemImageNamed:symbol withConfiguration:config] : nil;
    if(selected && glyph)
        glyph=[UIImage systemImageNamed:[symbol stringByAppendingString:@".fill"] withConfiguration:config] ?: glyph;
    glyph=[glyph imageWithTintColor:color renderingMode:UIImageRenderingModeAlwaysOriginal];
    if(!caption && !edge)return glyph;
    // The floating provider can collapse its title slot independently of the
    // image when the feed scrolls. Put the complete tab face inside the lens.
    UIFont *font=[UIFont systemFontOfSize:glyph ? 11 : 15 weight:selected ? UIFontWeightSemibold : UIFontWeightRegular];
    NSDictionary *ink=@{NSForegroundColorAttributeName:color,NSFontAttributeName:font};
    CGSize text=caption ? [caption sizeWithAttributes:ink] : CGSizeZero;
    CGSize glyphSize=glyph ? glyph.size : CGSizeZero;
    CGFloat titleY=glyph ? ceil(glyphSize.height)+(caption ? 2 : 0) : 0;
    CGFloat pad=edge ? 1 : 0;
    CGSize canvas=CGSizeMake(ceil(MAX(glyphSize.width,text.width))+2*pad,titleY+ceil(text.height)+2*pad);
    UIGraphicsImageRenderer *renderer=[[UIGraphicsImageRenderer alloc] initWithSize:canvas];
    UIImage *face=[renderer imageWithActions:^(UIGraphicsImageRendererContext *context) {
        if(edge)CGContextSetShadowWithColor(context.CGContext,CGSizeZero,1,edge.CGColor);
        [glyph drawAtPoint:CGPointMake((canvas.width-glyphSize.width)/2,pad)];
        [caption drawAtPoint:CGPointMake((canvas.width-text.width)/2,titleY+pad) withAttributes:ink];
    }];
    return [face imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
}
static BOOL HGADockContainsPoint(UIView *capsule,CGPoint point) {
    return HGAVisible(capsule) && CGRectContainsPoint(capsule.bounds,point) &&
        [[UIBezierPath bezierPathWithRoundedRect:capsule.bounds cornerRadius:capsule.layer.cornerRadius]
            containsPoint:point];
}
@implementation HGAAppearanceDock
- (instancetype)initWithFrame:(CGRect)frame {
    if((self=[super initWithFrame:frame])) {
        self.accessibilityIdentifier=@"honggo.appearance.dock";
        self.lastOccupiedRect=CGRectNull;
        self.targets=[NSMapTable strongToWeakObjectsMapTable];
        self.hiddenSources=[NSMapTable weakToStrongObjectsMapTable];
        self.buttons=[NSMutableDictionary dictionary];
        self.capsule=[UIView new];self.capsule.accessibilityIdentifier=@"honggo.appearance.dock.capsule";
        [self addSubview:self.capsule];
        self.selection=[UIView new];
        self.selection.accessibilityIdentifier=@"honggo.appearance.dock.selection";
        self.selection.userInteractionEnabled=NO;self.selection.accessibilityElementsHidden=YES;
        self.selection.layer.cornerCurve=kCACornerCurveContinuous;
        self.selection.layer.borderWidth=0.5;
        self.selectionHighlight=[UIView new];
        self.selectionHighlight.accessibilityIdentifier=@"honggo.appearance.dock.selection-highlight";
        self.selectionHighlight.userInteractionEnabled=NO;
        self.selectionHighlight.backgroundColor=[UIColor colorWithWhite:1 alpha:0.16];
        self.selectionHighlight.layer.cornerCurve=kCACornerCurveContinuous;
        self.selectionHighlight.layer.borderWidth=1;
        self.selectionHighlight.layer.borderColor=[UIColor colorWithWhite:1 alpha:0.66].CGColor;
        self.selectionHighlight.alpha=0;
        [self.selection addSubview:self.selectionHighlight];
        [self.capsule addSubview:self.selection];
        for(NSString *key in HGATabIDs()) {
            UIButton *b=[[HGADockTabButton alloc] initWithFrame:CGRectZero];
            b.titleLabel.textAlignment=NSTextAlignmentCenter;
            b.titleLabel.lineBreakMode=NSLineBreakByClipping;
            b.imageView.contentMode=UIViewContentModeScaleAspectFit;
            b.accessibilityIdentifier=[@"honggo.appearance.tab." stringByAppendingString:key];
            b.tag=[HGATabIDs() indexOfObject:key];
            [b addTarget:self action:@selector(pressTab:) forControlEvents:UIControlEventTouchDown|UIControlEventTouchDragEnter];
            [b addTarget:self action:@selector(cancelTabPress:) forControlEvents:UIControlEventTouchCancel|
                UIControlEventTouchDragExit|UIControlEventTouchUpOutside];
            [b addTarget:self action:@selector(tabTapped:) forControlEvents:UIControlEventTouchUpInside];
            [self.capsule addSubview:b];self.buttons[key]=b;
        }
        self.selectionPan=[[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(dragSelection:)];
        self.selectionPan.delegate=self;
        self.selectionPan.maximumNumberOfTouches=1;
        self.dockTouch=[[HGADockPointer alloc] initWithTarget:self action:@selector(finishDockPointer:)];
        self.dockTouch.dock=self;
        self.dockTouch.name=@"Honggo.DockPointer";
        self.dockTouch.delaysTouchesBegan=YES;
        self.dockTouch.cancelsTouchesInView=YES;
        self.dockTouch.delegate=self;
        self.extra=[UIView new];
        self.extraSelection=[UIView new];
        self.extraSelection.accessibilityIdentifier=@"honggo.appearance.dock.extra.selection";
        self.extraSelection.userInteractionEnabled=NO;self.extraSelection.accessibilityElementsHidden=YES;
        self.extraSelection.layer.cornerCurve=kCACornerCurveContinuous;
        self.extraSelection.layer.borderWidth=0.5;
        self.extraSelection.hidden=YES;
        self.extraButton=HGADockButton();
        self.extraButton.accessibilityIdentifier=@"honggo.appearance.dock.extra";
        [self.extraButton addTarget:self action:@selector(accessoryDown:) forControlEvents:UIControlEventTouchDown|UIControlEventTouchDragEnter];
        [self.extraButton addTarget:self action:@selector(accessoryUp:) forControlEvents:UIControlEventTouchUpInside|UIControlEventTouchDragExit|UIControlEventTouchCancel|UIControlEventTouchUpOutside];
        [self.extraButton addTarget:self action:@selector(extraTapped:) forControlEvents:UIControlEventTouchUpInside];
        [self.extra addSubview:self.extraSelection];
        [self.extra addSubview:self.extraButton];
        [self addSubview:self.extra];
        self.expand=[UIView new];
        self.expandButton=HGADockButton();
        self.expandButton.accessibilityIdentifier=@"honggo.appearance.dock.expand";
        self.expandButton.accessibilityLabel=@"展开导航栏";
        [self.expandButton addTarget:self action:@selector(expandTapped:) forControlEvents:UIControlEventTouchUpInside];
        [self.expand addSubview:self.expandButton];
        [self addSubview:self.expand];
        NSMutableArray *swipes=[NSMutableArray array];
        for(NSNumber *direction in @[@(UISwipeGestureRecognizerDirectionUp),@(UISwipeGestureRecognizerDirectionDown)]) {
            UISwipeGestureRecognizer *g=[[UISwipeGestureRecognizer alloc] initWithTarget:self action:@selector(swipe:)];
            g.direction=direction.unsignedIntegerValue;g.cancelsTouchesInView=NO;g.delegate=self;
            [self addGestureRecognizer:g];[swipes addObject:g];
        }
        self.swipes=swipes;
    }return self;
}
- (BOOL)acceptsSelectionTouchAtPoint:(CGPoint)point inView:(UIView *)hit {
    UIView *container=self.selectionPan.view;
    if(!self.selectionPan.enabled || !container.window || hit.window!=container.window ||
        !HGADockContainsPoint(self.capsule,point))return NO;
    if([hit isDescendantOfView:self.capsule])return YES;

    // A player's seek hotspot can extend below its visible progress rail.
    // Only the recognized player branch may share these capsule touches.
    Class player=NSClassFromString(@"FQVShortVideoVPlayerControlView");
    for(UIView *node=hit;node && node!=container;node=node.superview)
        if([node isKindOfClass:player] && [node isDescendantOfView:container])return YES;
    return NO;
}
- (UIButton *)dockButtonAtPoint:(CGPoint)point event:(UIEvent *)event {
    for(UIButton *button in @[self.extraButton,self.expandButton]) {
        UIView *surface=button==self.extraButton ? self.extra : self.expand;
        if(!button.hidden && button.enabled && HGADockContainsPoint(surface,[self convertPoint:point toView:surface]))return button;
    }
    if(!HGADockContainsPoint(self.capsule,[self convertPoint:point toView:self.capsule]))return nil;
    for(UIButton *button in self.buttons.allValues)
        if(!button.hidden && button.enabled && [button pointInside:[self convertPoint:point toView:button] withEvent:event])return button;
    return nil;
}
- (BOOL)acceptsDockTouchAtPoint:(CGPoint)point fromView:(UIView *)hit {
    UIView *container=self.dockTouch.view;
    UIButton *button=[self dockButtonAtPoint:point event:nil];
    if(!self.dockTouch.enabled || !container.window || hit.window!=container.window || !button)return NO;
    if(self.systemDockActive && !self.quietSystemTouches && button!=self.extraButton && button!=self.expandButton)return NO;
    if([hit isDescendantOfView:self] || hit==self.hostBar)return YES;
    if(button!=self.extraButton && button!=self.expandButton &&
        [self acceptsSelectionTouchAtPoint:[self convertPoint:point toView:self.capsule] inView:hit])return YES;

    // iOS 27 mounts an interactive UIKit wrapper above the actual SSTabBar,
    // even when its CYLTabBar inside has been hidden. Only accept the sibling
    // wrapper with the same visible bar geometry, and leave any real control alone.
    for(UIView *view=hit;view && view!=container;view=view.superview) {
        if([view isKindOfClass:UIControl.class])return NO;
        if(view.superview!=container)continue;
        if(![NSStringFromClass(view.class) isEqual:@"UIKit._UITabBarContainerWrapperView"])return NO;
        return CGRectEqualToRect([view convertRect:view.bounds toView:container],
            [self.hostBar convertRect:self.hostBar.bounds toView:container]);
    }
    return NO;
}
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gesture shouldReceiveTouch:(UITouch *)touch {
    if([self.hostBar.layer animationForKey:HGACleanOpacity])return NO;
    if(gesture==self.dockTouch && (touch.view==self.extraButton || [touch.view isDescendantOfView:self.extraButton]))
        return NO;
    if(gesture==self.dockTouch)
        return [self acceptsDockTouchAtPoint:[touch locationInView:self] fromView:touch.view];
    if(gesture!=self.selectionPan)return YES;
    // A proven player seek hotspot may win the initial hit. Its pan is
    // arbitrated only there, while the active Dock owns all direct presses.
    return ![self acceptsDockTouchAtPoint:[touch locationInView:self] fromView:touch.view] &&
        [self acceptsSelectionTouchAtPoint:[touch locationInView:self.capsule] inView:touch.view];
}
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gesture shouldBeRequiredToFailByGestureRecognizer:(UIGestureRecognizer *)other {
    if((gesture!=self.selectionPan && gesture!=self.dockTouch) || !gesture.enabled || other.view.window!=self.window ||
        ![other.view isDescendantOfView:gesture.view] || ![other isKindOfClass:UIPanGestureRecognizer.class])return NO;
    id player=other.delegate;
    Class controls=objc_lookUpClass("FQVShortVideoVPlayerControlView");
    if(![player isKindOfClass:controls])return NO;
    // The 7.4.5.33 getter constructs a pan and assigns its delegate on first
    // access. Gesture arbitration must only inspect already installed objects.
    Ivar progress=class_getInstanceVariable(controls,"_progressPanGesture");
    const char *type=progress ? ivar_getTypeEncoding(progress) : NULL;
    return type && !strcmp(type,"@\"UIPanGestureRecognizer\"") && object_getIvar(player,progress)==other;
}
- (BOOL)gestureRecognizerShouldBegin:(UIGestureRecognizer *)gesture {
    if(gesture!=self.selectionPan)return YES;
    CGPoint velocity=[self.selectionPan velocityInView:self.capsule];
    if(fabs(velocity.x)<=fabs(velocity.y))return NO;
    NSUInteger count=0;
    for(UIButton *button in self.buttons.allValues)
        if(!button.hidden && button.enabled && ++count==2)return YES;
    return NO;
}
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gesture shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)other {
    if(gesture==self.dockTouch)return [self.swipes containsObject:(id)other];
    if(other==self.dockTouch)return [self.swipes containsObject:(id)gesture];
    return gesture!=self.selectionPan && other!=self.selectionPan;
}
- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
    if(self.hidden || !self.userInteractionEnabled || ![self pointInside:point withEvent:event])return nil;
    if(self.systemDockActive && (!self.quietSystemTouches || self.systemDockSyncing) &&
        HGADockContainsPoint(self.capsule,[self convertPoint:point toView:self.capsule])) {
        CGPoint inBar=[self convertPoint:point toView:self.systemTabBar];
        UIView *hit=[self.systemTabBar hitTest:inBar withEvent:event];
        return hit==self.systemTabBar ? nil : hit;
    }
    UIButton *button=[self dockButtonAtPoint:point event:event];
    if(button)return button;
    return HGADockContainsPoint(self.capsule,[self convertPoint:point toView:self.capsule]) ? self.capsule : nil;
}
- (void)hideSource:(UIView *)view {
    if(!view || view==self.hostBar || [view isDescendantOfView:self])return;
    if(![self.hiddenSources objectForKey:view] || !view.hidden)[self.hiddenSources setObject:@(view.hidden) forKey:view];
    if(!view.hidden)view.hidden=YES;
}
- (BOOL)resolveTargets {
    UIView *content=HGAViewGetter(self.hostBar,"contentView",self.hostBar);
    if(!content || content==self.hostBar)return NO;
    [self.targets removeAllObjects];
    for(UIView *node in HGADescendants(content,180)) {
        if(![node isKindOfClass:UILabel.class])continue;
        NSString *title=[(UILabel *)node text];NSUInteger index=title?[HGATabTitles() indexOfObject:title]:NSNotFound;
        if(index==NSNotFound)continue;
        UIView *owner=node.superview;
        for(NSUInteger i=0;owner && owner!=content && i<8;i++,owner=owner.superview) {
            if([owner isKindOfClass:UIControl.class]) {
                UIControl *control=(id)owner;
                if((control.allControlEvents & UIControlEventTouchUpInside) || (control.allControlEvents & UIControlEventPrimaryActionTriggered))
                    [self.targets setObject:control forKey:HGATabIDs()[index]];
                break;
            }
        }
    }
    for(NSString *key in HGATabIDs())
        if(!HGAppearanceFlag([@"dock.hide." stringByAppendingString:key]) && ![self.targets objectForKey:key])return NO;
    return self.targets.count>0;
}

- (UIView *)systemDockCarrier {
    return self.systemTabController ? self.systemTabController.view : self.systemTabBar;
}

- (void)removeSystemDock {
    [self endQuietLens];
    self.systemDockActive=NO;
    self.quietSystemTouches=NO;self.systemHitSlots=nil;
    UIView *bar=self.hostBar,*wrapper=self.systemWrapper;
    if(wrapper && wrapper.superview==bar.superview) {
        NSArray *siblings=bar.superview.subviews;
        NSUInteger wrapperIndex=[siblings indexOfObjectIdenticalTo:wrapper];
        if(wrapperIndex!=NSNotFound && [siblings indexOfObjectIdenticalTo:bar]==wrapperIndex+1)
            [bar.superview insertSubview:bar belowSubview:wrapper];
    }
    self.systemWrapper=nil;
    if(self.systemTabController) {
        self.systemTabController.delegate=nil;
        [self.systemTabController willMoveToParentViewController:nil];
        [self.systemTabController.view removeFromSuperview];
        [self.systemTabController removeFromParentViewController];
    }else {
        self.systemTabBar.delegate=nil;
        [self.systemTabBar removeFromSuperview];
    }
    self.systemTabController=nil;self.systemTabBar=nil;self.systemPlatter=nil;
    self.customSystemPlatter=nil;self.installedSystemGlass=nil;self.systemMaterialStyle=nil;
    self.systemItemSignature=nil;
    self.systemWarmupPending=NO;self.systemWarmupComplete=NO;
    self.systemProbeFrame=CGRectNull;
    self.systemHostSelection=NSNotFound;
}

- (BOOL)buildSystemDockUsingController:(BOOL)managed {
    if(managed) {
        UIViewController *owner=self.hostController;
        if(!owner)return NO;
        if(@available(iOS 26.0,*)) {
            UITabBarController *controller=[UITabBarController new];
            controller.mode=UITabBarControllerModeTabBar;
            controller.tabBarMinimizeBehavior=UITabBarMinimizeBehaviorNever;
            controller.delegate=self;
            [owner addChildViewController:controller];
            controller.view.backgroundColor=UIColor.clearColor;
            self.systemTabController=controller;
            self.systemTabBar=controller.tabBar;
        }else return NO;
    }else {
        self.systemTabBar=[[UITabBar alloc] initWithFrame:self.bounds];
        self.systemTabBar.delegate=self;
    }
    self.systemTabBar.backgroundColor=UIColor.clearColor;
    self.systemTabBar.barTintColor=nil;
    UIView *carrier=[self systemDockCarrier];
    carrier.frame=self.bounds;
    carrier.accessibilityIdentifier=@"honggo.appearance.dock.system";
    [self insertSubview:carrier aboveSubview:self.capsule];
    [self.systemTabController didMoveToParentViewController:self.hostController];
    self.systemHostSelection=NSNotFound;
    return YES;
}

- (BOOL)syncSystemItems:(NSArray<NSString *> *)keys content:(NSString *)content iconSize:(CGFloat)size {
    UITraitCollection *traits=self.capsule.traitCollection;
    UIColor *selected=[UIColor.labelColor resolvedColorWithTraitCollection:traits];
    UIColor *normal=[UIColor.secondaryLabelColor resolvedColorWithTraitCollection:traits];
    BOOL videoClear=HGASelectedTab(self.hostController)==0 && [HGAppearanceString(@"dock.material") isEqual:@"clear"];
    if(videoClear) {
        selected=UIColor.whiteColor;
        normal=[UIColor.whiteColor colorWithAlphaComponent:traits.accessibilityContrast==UIAccessibilityContrastHigh ? 0.94 : 0.84];
    }
    UIColor *edge=videoClear ? [UIColor colorWithWhite:0 alpha:0.78] : nil;
    NSMutableArray<NSNumber *> *enabled=[NSMutableArray arrayWithCapacity:keys.count];
    for(NSString *key in keys)[enabled addObject:@([(UIControl *)[self.targets objectForKey:key] isEnabled])];
    NSArray *signature=@[keys,content,@(size),selected,normal,enabled,@(videoClear)];
    if([self.systemItemSignature isEqualToArray:signature])return NO;
    self.systemTabBar.tintColor=selected;
    self.systemTabBar.unselectedItemTintColor=normal;
    BOOL replace=![self.systemItemSignature.firstObject isEqual:keys];
    BOOL metricsChanged=replace || ![self.systemItemSignature[1] isEqual:content] || ![self.systemItemSignature[2] isEqual:@(size)];
    NSMutableArray *items=[NSMutableArray arrayWithCapacity:keys.count];
    for(NSUInteger slot=0;slot<keys.count;slot++) {
        NSString *key=keys[slot];
        NSUInteger index=[HGATabIDs() indexOfObject:key];
        NSString *label=HGATabTitles()[index];
        NSString *symbol=[content isEqual:@"text"] ? nil : HGATabSymbols()[index];
        NSString *caption=[content isEqual:@"icon"] ? nil : label;
        UIImage *image=HGADockSystemImage(symbol,caption,size,normal,NO,edge);
        UIImage *selectedImage=HGADockSystemImage(symbol,caption,size,selected,YES,edge);
        if(self.systemTabController) {
            if(@available(iOS 26.0,*)) {
                UITab *tab=replace ? [[UITab alloc] initWithTitle:@"" image:image identifier:key viewControllerProvider:^UIViewController *(UITab *requestedTab) {
                        UIViewController *empty=[UIViewController new];
                        empty.view.backgroundColor=UIColor.clearColor;empty.view.userInteractionEnabled=NO;
                        return empty;
                    }] : [self.systemTabController tabForIdentifier:key];
                if(!replace)tab.image=image;
                if(@available(iOS 26.1,*))tab.selectedImage=selectedImage;
                tab.accessibilityLabel=label;
                tab.enabled=enabled[slot].boolValue;
                tab.userInfo=@(index);
                [items addObject:tab];
            }
        }else {
            UITabBarItem *item=replace ? [[UITabBarItem alloc] initWithTitle:nil image:image selectedImage:selectedImage] :
                self.systemTabBar.items[slot];
            if(!replace){item.image=image;item.selectedImage=selectedImage;}
            item.tag=(NSInteger)index;item.enabled=enabled[slot].boolValue;item.accessibilityLabel=label;
            [items addObject:item];
        }
    }
    if(replace) {
        if(self.systemTabController) {
            if(@available(iOS 26.0,*))[self.systemTabController setTabs:items animated:NO];
        }else [self.systemTabBar setItems:items animated:NO];
    }
    self.systemItemSignature=signature;
    return metricsChanged;
}

- (void)syncSystemSelection:(NSInteger)selected force:(BOOL)force {
    if(!force && self.systemHostSelection==selected)return;
    self.systemHostSelection=selected;
    if(self.systemTabController) {
        if(@available(iOS 26.0,*)) {
            UITab *tab=selected>=0 && selected<(NSInteger)HGATabIDs().count ?
                [self.systemTabController tabForIdentifier:HGATabIDs()[(NSUInteger)selected]] : nil;
            if(self.systemTabController.selectedTab!=tab)self.systemTabController.selectedTab=tab;
        }
    }else {
        UITabBarItem *item=nil;
        for(UITabBarItem *candidate in self.systemTabBar.items)if(candidate.tag==selected){item=candidate;break;}
        if(self.systemTabBar.selectedItem!=item)self.systemTabBar.selectedItem=item;
    }
}

- (NSString *)applySystemMaterial:(NSString *)style {
    if(@available(iOS 26.0,*)) {
        UIView *surface=self.systemPlatter;
        BOOL clear=[style isEqual:@"clear"];
        SEL setter=sel_registerName("setGlassEffect:");
        Method method=class_getInstanceMethod(object_getClass(surface),setter);
        UIVisualEffect *current=HGReadObjectGetter(surface,"glassEffect");
        BOOL hasSetter=method && !strcmp(method_getTypeEncoding(method),"v24@0:8@16");
        if(hasSetter && !current) {
            Method getter=class_getInstanceMethod(object_getClass(surface),sel_registerName("glassEffect"));
            hasSetter=getter && !strcmp(method_getTypeEncoding(getter),"@16@0:8");
        }
        UIVisualEffectView *backing=[surface isKindOfClass:UIVisualEffectView.class] ? (UIVisualEffectView *)surface : nil;
        Ivar slot=NULL;
        if(!backing && !hasSetter) {
            Ivar ivar=class_getInstanceVariable(object_getClass(surface),"_glassEffect");
            const char *type=ivar ? ivar_getTypeEncoding(ivar) : NULL;
            if(type && *type=='@') {
                id effect=object_getIvar(surface,ivar);
                if([effect isKindOfClass:UIVisualEffect.class] || (!effect && !strcmp(type,"@\"UIGlassEffect\""))) {
                    slot=ivar;current=effect;
                }
            }
        }
        if(!backing && !hasSetter && !slot) {
            // The item platter on iOS 27 can have a separate backing view
            // directly under the tab bar. Never replace its moving selection.
            for(UIView *child in self.systemTabBar.subviews) {
                if(![child isKindOfClass:UIVisualEffectView.class] || child.hidden || child.alpha<0.05 ||
                    child.bounds.size.height<=10 || child.bounds.size.width<2*child.bounds.size.height)continue;
                UIVisualEffectView *candidate=(id)child;
                if(candidate.effect && (!backing || child.bounds.size.width>backing.bounds.size.width))backing=candidate;
            }
            if(!backing)return @"factory-glass";
        }
        if(backing) {
            surface=backing;current=backing.effect;
        }
        if(surface!=self.customSystemPlatter || ![style isEqual:self.systemMaterialStyle] ||
            ![current isEqual:self.installedSystemGlass]) {
            UIGlassEffect *glass=[UIGlassEffect effectWithStyle:clear ? UIGlassEffectStyleClear : UIGlassEffectStyleRegular];
            glass.interactive=backing || ![current isKindOfClass:UIGlassEffect.class] || ((UIGlassEffect *)current).interactive;
            if(backing) {
                backing.backgroundColor=UIColor.clearColor;backing.contentView.backgroundColor=UIColor.clearColor;
                backing.effect=glass;
                self.installedSystemGlass=backing.effect;
            }else if(slot) {
                object_setIvar(surface,slot,glass);
                self.installedSystemGlass=object_getIvar(surface,slot);
                [surface setNeedsLayout];[surface setNeedsDisplay];
            }else {
                ((void(*)(id,SEL,id))objc_msgSend)(surface,setter,glass);
                self.installedSystemGlass=HGReadObjectGetter(surface,"glassEffect");
                [surface setNeedsLayout];
            }
            if(![self.installedSystemGlass isKindOfClass:UIGlassEffect.class] ||
                (current && self.installedSystemGlass==current))
                return @"factory-glass";
            self.customSystemPlatter=surface;self.systemMaterialStyle=style;
        }
        if(slot)return clear ? @"clear-glass-slot" : @"regular-glass-slot";
        return clear ? @"clear-glass" : @"regular-glass";
    }
    return @"factory-glass";
}

- (void)recordDockRenderer:(NSString *)mode reason:(NSString *)reason material:(NSString *)material {
    NSString *requested=HGAppearanceString(@"dock.material");
    NSString *provider=self.systemPlatter ? NSStringFromClass(self.systemPlatter.class) : @"";
    UIUserInterfaceStyle style=self.capsule.overrideUserInterfaceStyle;
    if(style==UIUserInterfaceStyleUnspecified)style=self.capsule.traitCollection.userInterfaceStyle;
    NSString *interfaceStyle=style==UIUserInterfaceStyleLight ? @"light" : (style==UIUserInterfaceStyleDark ? @"dark" : @"unspecified");
    NSArray *record=@[mode,reason,material,requested,provider,interfaceStyle];
    if([self.systemRendererRecord isEqualToArray:record])return;
    self.systemRendererRecord=record;
    HGLog(@"appearance.dock-renderer",@{@"mode":mode,@"reason":reason,@"material":material,
        @"requestedMaterial":requested,@"platter":provider,@"interfaceStyle":interfaceStyle});
}

- (BOOL)fitSystemDockInCapsule:(CGRect)requested itemCount:(NSUInteger)count {
    UIView *carrier=[self systemDockCarrier];
    CGRect destination=requested;
    CGSize hostSize=self.bounds.size;
    CGFloat expectedWidth=CGRectGetWidth(destination)+40;
    CGFloat hostHeight=MAX(76,hostSize.height);
    carrier.bounds=CGRectMake(0,0,expectedWidth,hostHeight);
    [carrier setNeedsLayout];[carrier layoutIfNeeded];
    UIView *platter=HGADockSystemPlatter(self.systemTabBar);
    if(!platter)return NO;

    CGRect measured=[platter convertRect:platter.bounds toView:carrier];
    CGFloat verticalScale=CGRectGetHeight(destination)/CGRectGetHeight(measured);
    CGFloat margin=MAX(0,CGRectGetWidth(self.systemTabBar.bounds)-CGRectGetWidth(measured));
    CGFloat fittedWidth=MIN(hostSize.width+44,CGRectGetWidth(destination)/verticalScale+MIN(margin,64));
    if(fabs(fittedWidth-expectedWidth)>0.5) {
        carrier.bounds=CGRectMake(0,0,fittedWidth,hostHeight);
        [carrier setNeedsLayout];[carrier layoutIfNeeded];
        platter=HGADockSystemPlatter(self.systemTabBar);
        if(!platter)return NO;
        measured=[platter convertRect:platter.bounds toView:carrier];
        verticalScale=CGRectGetHeight(destination)/CGRectGetHeight(measured);
    }
    // A native platter has a natural maximum width. Stretching beyond that
    // width also stretches every symbol, corner and refracting lens.
    CGFloat renderedWidth=CGRectGetWidth(measured)*verticalScale;
    CGFloat difference=CGRectGetWidth(destination)-renderedWidth;
    if(renderedWidth<count*44 || difference < -2)return NO;
    if(difference>2 && fabs(HGAppearanceNumber(@"dock.width")-HGADockDefaultWidth)>0.00001)return NO;
    destination=CGRectInset(destination,difference*0.5,0);
    CGRect bounds=carrier.bounds;
    carrier.transform=CGAffineTransformMakeScale(verticalScale,verticalScale);
    carrier.center=CGPointMake(CGRectGetMidX(destination)-(CGRectGetMidX(measured)-CGRectGetMidX(bounds))*verticalScale,
        CGRectGetMidY(destination)-(CGRectGetMidY(measured)-CGRectGetMidY(bounds))*verticalScale);
    self.systemProbeFrame=requested;
    self.systemCapsuleFrame=destination;
    if(!CGRectEqualToRect(self.capsule.frame,destination))self.capsule.frame=destination;
    self.systemPlatter=platter;
    return YES;
}

- (BOOL)systemDockCanReceiveTouches:(NSArray<NSString *> *)keys {
    // UIKit's lens changes size and center during selection and attachment.
    // Measure the stable item hit areas instead of a presentation-time thumb.
    CGFloat width=CGRectGetWidth(self.capsule.bounds);
    CGFloat edgeAllowance=MIN(6,width/keys.count*0.08);
    NSMutableArray<NSValue *> *centers=[NSMutableArray array];
    CGFloat left=CGFLOAT_MAX,right=-CGFLOAT_MAX;
    NSArray<UIView *> *items=HGADockSystemItems(self.systemPlatter);
    if(!items.count)items=HGADockSystemItems(self.systemTabBar);
    for(UIView *item in items) {
        if(!HGAVisible(item))continue;
        CGPoint center=CGPointMake(CGRectGetMidX(item.bounds),CGRectGetMidY(item.bounds));
        CGPoint local=[item convertPoint:center toView:self];
        if(!HGADockContainsPoint(self.capsule,[self convertPoint:local toView:self.capsule]))continue;
        CGRect slot=[item convertRect:item.bounds toView:self.capsule];
        left=MIN(left,CGRectGetMinX(slot));right=MAX(right,CGRectGetMaxX(slot));
        BOOL duplicate=NO;
        for(NSValue *existing in centers)if(fabs(existing.CGPointValue.x-local.x)<2){duplicate=YES;break;}
        if(!duplicate)[centers addObject:[NSValue valueWithCGPoint:local]];
    }
    if(centers.count!=keys.count)return NO;
    if(left>edgeAllowance || right<width-edgeAllowance)return NO;
    BOOL rtl=self.systemTabBar.effectiveUserInterfaceLayoutDirection==UIUserInterfaceLayoutDirectionRightToLeft;
    [centers sortUsingComparator:^NSComparisonResult(NSValue *first,NSValue *second) {
        return (first.CGPointValue.x<second.CGPointValue.x)!=rtl ? NSOrderedAscending : NSOrderedDescending;
    }];
    NSMutableArray<NSValue *> *slots=[NSMutableArray arrayWithCapacity:centers.count];
    CGFloat edge=rtl ? width : 0;
    for(NSUInteger slot=0;slot<centers.count;slot++) {
        CGFloat next=rtl ? 0 : width;
        if(slot+1<centers.count) {
            CGPoint a=[self convertPoint:centers[slot].CGPointValue toView:self.capsule];
            CGPoint b=[self convertPoint:centers[slot+1].CGPointValue toView:self.capsule];
            next=(a.x+b.x)/2;
        }
        [slots addObject:[NSValue valueWithCGRect:CGRectMake(MIN(edge,next),0,fabs(next-edge),self.capsule.bounds.size.height)]];
        edge=next;
    }
    UIView *parent=self.hostBar.superview;
    BOOL checked=NO;
    for(NSUInteger slot=0;slot<keys.count;slot++) {
        if(![(UIControl *)[self.targets objectForKey:keys[slot]] isEnabled])continue;
        checked=YES;
        CGPoint local=centers[slot].CGPointValue;
        UIView *hit=[parent hitTest:[self convertPoint:local toView:parent] withEvent:nil];
        if(hit!=self.systemTabBar && [hit isDescendantOfView:self.systemTabBar])continue;

        // Do not forward touches to UIKit manually. On the affected host the
        // inactive CYL wrapper is an adjacent sibling with identical geometry.
        UIView *wrapper=hit;
        while(wrapper && wrapper.superview!=parent) {
            if([wrapper isKindOfClass:UIControl.class]){wrapper=nil;break;}
            wrapper=wrapper.superview;
        }
        NSArray<UIView *> *siblings=parent.subviews;
        NSUInteger barIndex=[siblings indexOfObjectIdenticalTo:self.hostBar];
        if(!wrapper || barIndex==NSNotFound || [siblings indexOfObjectIdenticalTo:wrapper]!=barIndex+1 ||
            !HGADockUIKitPart(wrapper,@"_UITabBarContainerWrapperView") ||
            !CGRectEqualToRect([wrapper convertRect:wrapper.bounds toView:parent],self.hostBar.frame))return NO;
        BOOL inactiveHostBar=NO;
        for(UIView *view in HGADockSystemSubviews(wrapper))
            if([NSStringFromClass(view.class) isEqual:@"CYLTabBar"] && !HGAVisible(view)){inactiveHostBar=YES;break;}
        if(!inactiveHostBar || !HGADockInstallHostHitRoute())return NO;
        [parent insertSubview:self.hostBar aboveSubview:wrapper];
        self.systemWrapper=wrapper;
        hit=[parent hitTest:[self convertPoint:local toView:parent] withEvent:nil];
        if(hit==self.systemTabBar || ![hit isDescendantOfView:self.systemTabBar])return NO;
    }
    if(checked)self.systemHitSlots=slots;
    return checked;
}

- (BOOL)syncSystemDock:(NSArray<NSString *> *)keys content:(NSString *)content iconSize:(CGFloat)size
    selected:(NSInteger)selected style:(NSString *)style requestedFrame:(CGRect)requested {
    // A failed mount is cached for this geometry and tab configuration, so a
    // legacy UIKit provider is not recreated on every host layout callback.
    BOOL requireClear=selected==0 && [style isEqual:@"clear"];
    NSArray *probe=@[keys,content,style,@(requireClear),[NSValue valueWithCGSize:self.bounds.size],
        [NSValue valueWithCGSize:requested.size],[NSValue valueWithNonretainedObject:self.window],
        [NSValue valueWithNonretainedObject:self.hostBar.superview]];
    if([self.failedSystemProbe isEqualToArray:probe]) {
        if(self.systemTabBar)[self removeSystemDock];
        return NO;
    }
    if(self.systemWarmupPending)return NO;
    if(self.hidden && !self.systemDockActive)return NO;
    self.systemDockSyncing=YES;
    BOOL active=NO;
    NSString *failure=@"UIKit could not attach a tab bar";
    BOOL managed=self.systemTabController!=nil;
    for(;;) {
        if(!self.systemTabBar && ![self buildSystemDockUsingController:managed])break;
        UIView *carrier=[self systemDockCarrier];
        if(carrier.alpha!=1)carrier.alpha=1;
        if(!carrier.userInteractionEnabled)carrier.userInteractionEnabled=YES;
        if(carrier.overrideUserInterfaceStyle!=self.capsule.overrideUserInterfaceStyle)
            carrier.overrideUserInterfaceStyle=self.capsule.overrideUserInterfaceStyle;
        if(self.systemTabBar!=carrier && self.systemTabBar.overrideUserInterfaceStyle!=self.capsule.overrideUserInterfaceStyle)
            self.systemTabBar.overrideUserInterfaceStyle=self.capsule.overrideUserInterfaceStyle;
        BOOL rebuild=[self syncSystemItems:keys content:content iconSize:size];
        [self syncSystemSelection:selected force:rebuild];
        BOOL geometryChanged=!CGRectEqualToRect(self.systemProbeFrame,requested) ||
            ![self.systemPlatter isDescendantOfView:self.systemTabBar];
        if(!geometryChanged && !rebuild && self.systemDockActive && self.systemHitSlots.count==keys.count){active=YES;break;}
        if([self fitSystemDockInCapsule:requested itemCount:keys.count]) {
            self.systemDockActive=YES;
            self.systemHitSlots=nil;
            if(self.hidden || [self systemDockCanReceiveTouches:keys]){active=YES;break;}
            failure=@"Floating tab item geometry or hit route unavailable";
        }else failure=@"Floating platter missing or outside configured dimensions";
        if(!self.systemWarmupComplete) {
            // Give UIKit one turn to populate its initial attachment before
            // concluding this bar is a legacy provider. Keep it nonvisual.
            self.systemDockActive=NO;
            self.systemWarmupPending=YES;
            carrier.alpha=0.001;carrier.userInteractionEnabled=NO;
            __weak HGAAppearanceDock *weakSelf=self;
            __weak UIView *weakCarrier=carrier;
            dispatch_async(dispatch_get_main_queue(),^{
                HGAAppearanceDock *dock=weakSelf;
                if(!dock || [dock systemDockCarrier]!=weakCarrier)return;
                dock.systemWarmupPending=NO;dock.systemWarmupComplete=YES;
                if(dock.window)[dock refresh];
            });
            self.systemDockSyncing=NO;
            return NO;
        }
        [self removeSystemDock];
        if(managed)break;
        managed=YES;
    }
    self.systemDockSyncing=NO;
    NSString *material=active ? [self applySystemMaterial:style] : style;
    // A read-only factory platter supplies its default material. The single-
    // surface Dock below can honor Clear directly, including native refraction.
    if(active && requireClear && [material isEqual:@"factory-glass"]) {
        [self removeSystemDock];
        active=NO;material=style;
        failure=@"Video uses native Clear glass in place of a read-only factory platter";
    }
    if(!active)self.failedSystemProbe=probe;
    NSString *mode=active ? (self.systemTabController?@"uikit-controller":@"uikit-tabbar") : @"compatibility";
    NSString *reason=active ? (HGAppearanceFlag(@"dock.haptics") ? @"UIKit receives the original touch" :
        @"UIKit platter with quiet touch routing") : failure;
    [self recordDockRenderer:mode reason:reason material:material];
    return active;
}

- (void)tabBar:(UITabBar *)tabBar didSelectItem:(UITabBarItem *)item {
    if(!self.systemDockActive || self.quietSystemTouches || self.systemDockSyncing || self.systemTabController || tabBar!=self.systemTabBar ||
        !item.enabled || ![tabBar.items containsObject:item])return;
    [self activateTabAtIndex:item.tag animation:@"uikit-tabbar"];
    self.systemHostSelection=NSIntegerMin;
}

- (void)tabBarController:(UITabBarController *)controller didSelectTab:(UITab *)tab previousTab:(UITab *)previousTab API_AVAILABLE(ios(18.0)) {
    if(!self.systemDockActive || self.quietSystemTouches || self.systemDockSyncing || controller!=self.systemTabController ||
        ![controller.tabs containsObject:tab] || ![tab.userInfo isKindOfClass:NSNumber.class])return;
    [self activateTabAtIndex:[tab.userInfo integerValue] animation:@"uikit-controller"];
    self.systemHostSelection=NSIntegerMin;
}
- (void)claimHostChrome {
    UIView *bar=self.hostBar;
    if(!self.backgroundOwned || ![bar.backgroundColor isEqual:UIColor.clearColor])self.hostBackground=bar.backgroundColor;
    if(!self.backgroundOwned || bar.opaque)self.hostOpaque=bar.opaque;
    self.backgroundOwned=YES;
    if(![bar.backgroundColor isEqual:UIColor.clearColor]) {
        self.clearingHostFill=YES;
        bar.backgroundColor=UIColor.clearColor;
        self.clearingHostFill=NO;
    }
    if(bar.opaque)bar.opaque=NO;
    [self hideSource:HGAViewGetter(bar,"contentView",bar)];
    const char *backdrops[]={"backgroundImageView","lightView","darkView","lineView"};
    for(NSUInteger i=0;i<4;i++)[self hideSource:HGAViewGetter(bar,backdrops[i],bar)];
    for(UIView *v in bar.subviews)if([v.accessibilityIdentifier isEqual:@"honggo.tabbar.glass"])[self hideSource:v];
}
- (void)refresh {
    if(self.rendering || self.restoring || self.deliveringTabAction || HGFeedSidebarOwnsLayout(self.hostBar.window))return;
    self.rendering=YES;
    @try {
        UIView *bar=self.hostBar;
        if(!HGAppearanceDockEnabled() || !bar.window || !HGAVisible(bar)) {[self restore];return;}
        // Preserve the live dock untouched while customTabBar slides horizontally
        // during sidebar presentation or search push navigation.
        if(fabs(bar.frame.origin.x)>2.0) {
            // The Mine drawer moves the host bar left. On return from
            // settings it can reappear already displaced, with native labels
            // visible. Reclaim its paint now; its frame belongs to the host.
            if(self.superview==bar){[self claimHostChrome];return;}
            if(bar.frame.origin.x>0)return;
        }
        if(self.sidebarSuspended){self.sidebarSuspended=NO;self.userInteractionEnabled=YES;}
        if(self.superview==bar && CGRectEqualToRect(self.frame,bar.bounds) &&
            (self.dockTouchButton || self.pressedTab || self.selectionDragging || self.selectionSettling))return;
        CGFloat controlHeight=HGADockControlHeight(bar);
        if(bar.bounds.size.width<180 || controlHeight<44 || ![self resolveTargets]) {
            [self restore];
            HGRecordCapability(@"appearance.dock.navigation",@{@"module":@"appearance",@"state":@"unavailable",@"reason":@"native tab actions could not be mapped; original navigation retained"});
            return;
        }
        NSUInteger visibleCount=HGADockCapsuleTabs().count;
        CGFloat extraWidth=HGAppearanceFlag(@"dock.extra") ?
            MIN(HGAppearanceNumber(@"dock.extraSize"),controlHeight)+HGADockAccessoryGap : 0;
        if(bar.bounds.size.width-8 < visibleCount*44+extraWidth) {
            [self restore];
            HGRecordCapability(@"appearance.dock.navigation",@{@"module":@"appearance",@"state":@"unavailable",@"reason":@"insufficient space for accessible tab targets; native navigation retained"});
            return;
        }
        if(self.superview!=bar)[bar addSubview:self];
        if(self.selectionPan.view!=bar.superview)[bar.superview addGestureRecognizer:self.selectionPan];
        if(self.dockTouch.view!=bar.superview)[bar.superview addGestureRecognizer:self.dockTouch];
        if(!CGRectEqualToRect(self.frame,bar.bounds))self.frame=bar.bounds;
        [self claimHostChrome];
        self.hidden=HGAWindowHasActionSheet(bar.window);
        BOOL gestures=HGAppearanceFlag(@"dock.collapse") && !UIAccessibilityIsVoiceOverRunning();
        for(UISwipeGestureRecognizer *g in self.swipes)g.enabled=gestures;
        if(!gestures)self.collapsed=NO;
        [self renderGeometry];
        HGRecordCapability(@"appearance.dock.navigation",@{@"module":@"appearance",@"state":@"installed",@"reason":@"native control actions mapped; host retains navigation decisions"});
    }@finally{self.rendering=NO;}
}
- (void)renderGeometry {
    CGFloat width=self.bounds.size.width,availableHeight=self.bounds.size.height;
    CGFloat controlHeight=HGADockControlHeight(self.hostBar);
    if(width<180 || controlHeight<44)return;
    NSMutableArray *visible=HGADockCapsuleTabs();
    BOOL extra=HGAppearanceFlag(@"dock.extra");
    CGFloat circle=MIN(HGAppearanceNumber(@"dock.extraSize"),controlHeight);
    CGFloat height=MIN(HGAppearanceNumber(@"dock.height"),controlHeight);
    CGFloat reserve=extra ? circle+HGADockAccessoryGap : 0;
    CGFloat total=HGAExpandedDockWidth(width,controlHeight,visible.count);
    CGFloat rowHeight=self.collapsed ? 44 : (extra?MAX(height,circle):height);
    CGFloat safeBottom=MIN(self.hostBar.safeAreaInsets.bottom,availableHeight-rowHeight);
    CGFloat rowTop=MAX(0,availableHeight-safeBottom-rowHeight-HGAppearanceNumber(@"dock.lift"));
    CGFloat x=(width-total)/2;
    CGRect expanded=CGRectMake(x,rowTop+(rowHeight-height)/2,total-reserve,height);
    CGRect compact=CGRectMake(width-48,rowTop,44,44);
    NSString *anchor=HGAppearanceString(@"dock.anchor");
    if([anchor isEqual:@"left"])compact.origin.x=4;
    if([anchor isEqual:@"center"])compact.origin.x=(width-44)/2;
    self.expand.frame=compact;self.expand.hidden=!self.collapsed;
    self.extra.hidden=!extra || self.collapsed;
    BOOL flat=HGFlatDockStyle();
    NSString *style=HGAppearanceString(@"dock.material");
    BOOL classicBlur=[style isEqual:@"blur"];
    CGFloat radius=MIN(height/2,HGAppearanceNumber(@"dock.radius"));
    BOOL translucent=![style isEqual:@"solid"];
    NSInteger selected=HGASelectedTab(self.hostController);
    UIUserInterfaceStyle surfaceStyle=(selected==0 || flat || translucent) ?
        HGADockPageStyle(self.hostController,self.hostBar,selected) : UIUserInterfaceStyleUnspecified;
    BOOL light=translucent && surfaceStyle==UIUserInterfaceStyleLight;
    BOOL styleChangedAny=NO;
    for(UIView *surface in @[self.capsule,self.extra,self.expand]) {
        BOOL styleChanged=(surface.overrideUserInterfaceStyle!=surfaceStyle);
        if(styleChanged) {
            surface.overrideUserInterfaceStyle=surfaceStyle;
            styleChangedAny=YES;
        }
        surface.layer.cornerCurve=kCACornerCurveContinuous;
    }
    BOOL reduced=UIAccessibilityIsReduceTransparencyEnabled();
    BOOL haptics=HGAppearanceFlag(@"dock.haptics");
    BOOL liquidSelection=HGADockLiquidSelection();
    BOOL nativeGlass=!flat && HGAUsesNativeGlass(style);
    BOOL videoClear=nativeGlass && selected==0 && [style isEqual:@"clear"];
    UIColor *inkEdge=videoClear ? [UIColor colorWithWhite:0 alpha:0.78] : nil;
    UIColor *base=translucent ? [UIColor colorWithWhite:light?1:0.10 alpha:1] : UIColor.secondarySystemBackgroundColor;
    UIColor *fill=[base colorWithAlphaComponent:!translucent || reduced ? 1 : 0];
    NSString *content=HGAppearanceString(@"dock.content");
    CGFloat unselectedOpacity=self.traitCollection.accessibilityContrast==UIAccessibilityContrastHigh ? 0.94 : 0.80;
    CGFloat requestedIcon=HGAppearanceNumber(@"dock.iconSize");
    self.capsule.hidden=self.collapsed;
    self.capsule.layer.cornerRadius=radius;
    BOOL systemEligible=NO;
    if(@available(iOS 26.0,*))
        systemEligible=nativeGlass && liquidSelection && fabs(self.hostBar.frame.origin.x)<=2.0 && !self.collapsed && visible.count>1 &&
            radius>=height/2-0.5 &&
            ([content isEqual:@"icon"] || !HGAppearanceFlag(@"fonts.enabled"));
    BOOL useSystem=NO;
    BOOL tracking=self.pressedTab || self.selectionDragging || self.selectionSettling || self.motionMaterialPresented;
    BOOL nativePreview=tracking && self.systemDockActive && self.quietSystemTouches;
    if(nativePreview && self.quietDragLens) {
        if(self.quietProvider!=self.systemPlatter || !self.quietSelectionSources.count)[self endQuietLens];
        for(UIView *part in self.quietSelectionSources.keyEnumerator.allObjects) {
            if(!part.hidden || part.superview!=[self.quietSelectionSources objectForKey:part] ||
                ![part isDescendantOfView:self.systemTabBar]){[self endQuietLens];break;}
        }
    }
    if(nativePreview && (!systemEligible || haptics || !CGRectEqualToRect(self.systemProbeFrame,expanded) ||
        styleChangedAny ||
        ![self.systemItemSignature.firstObject isEqual:visible] || ![self.systemPlatter isDescendantOfView:self.systemTabBar])) {
        self.dockTouchButton=nil;
        [self clearSelectionDrag];
        [self syncSystemSelection:selected force:YES];
        tracking=NO;nativePreview=NO;
    }
    if(nativePreview) {
        // A host layout does not end the contact. Keep the original platter
        // and its previewed tab until this contact commits or cancels.
        useSystem=YES;
    }else if(systemEligible && !tracking) {
        if(@available(iOS 17.0,*))[self.capsule updateTraitsIfNeeded];
        useSystem=[self syncSystemDock:visible content:content iconSize:MIN(requestedIcon,height*0.36)
            selected:selected style:style requestedFrame:expanded];
    }else if(!systemEligible) {
        if(self.systemTabBar)[self removeSystemDock];
        if(!self.collapsed) {
            NSString *reason=@"Configured material or shape requires compatibility";
            if(!liquidSelection)reason=@"Plain selection";
            if(flat)reason=@"UIKit segmented track";
            [self recordDockRenderer:flat?@"flat":@"compatibility" reason:reason material:flat?@"flat":style];
        }
    }
    self.quietSystemTouches=useSystem && !haptics;
    UIView *systemCarrier=[self systemDockCarrier];
    if(systemCarrier.accessibilityElementsHidden!=self.quietSystemTouches)systemCarrier.accessibilityElementsHidden=self.quietSystemTouches;
    CGRect actual=useSystem ? self.systemCapsuleFrame : expanded;
    CGRect capsuleFrame=self.collapsed ? compact : actual;
    BOOL capsuleChanged=!CGRectEqualToRect(self.capsule.frame,capsuleFrame);
    if(capsuleChanged)self.capsule.frame=capsuleFrame;
    self.extra.frame=CGRectMake(CGRectGetMaxX(actual)+HGADockAccessoryGap,rowTop+(rowHeight-circle)/2,circle,circle);
    if(nativeGlass)fill=UIColor.clearColor;
    if(![self.capsule.backgroundColor isEqual:UIColor.clearColor])self.capsule.backgroundColor=UIColor.clearColor;
    if(flat || useSystem)HGARemoveGlass(self.capsule);
    else HGADockMaterialContent(self.capsule,style,self.collapsed?22:radius,fill,NO);
    self.flatSegments=HGADockFlatBacking(self.capsule,flat ? visible.count : 0);
    UIView *background=HGAState(self.capsule,NO).glass;
    if(self.selection.superview!=self.capsule) {
        if(background)[self.capsule insertSubview:self.selection aboveSubview:background];
        else [self.capsule insertSubview:self.selection atIndex:0];
    }
    if(flat){HGARemoveGlass(self.extra);HGARemoveGlass(self.expand);}
    UISegmentedControl *extraTrack=HGADockFlatBacking(self.extra,flat ? 1 : 0);
    HGADockFlatBacking(self.expand,flat ? 1 : 0);
    UIView *extraContent=flat ? self.extra : HGADockMaterialContent(self.extra,style,circle/2,fill,haptics);
    UIView *expandContent=flat ? self.expand : HGADockMaterialContent(self.expand,style,22,fill,haptics);
    // UIKit can adapt dynamic label colors to the material's own background
    // when the foreground belongs to the visual effect's contentView.
    if(self.extraSelection.superview!=extraContent)[extraContent insertSubview:self.extraSelection atIndex:0];
    if(self.extraButton.superview!=extraContent)[extraContent addSubview:self.extraButton];
    if(self.expandButton.superview!=expandContent)[expandContent addSubview:self.expandButton];
    UIUserInterfaceStyle buttonStyle=videoClear ? surfaceStyle : UIUserInterfaceStyleUnspecified;
    if(self.extraButton.overrideUserInterfaceStyle!=buttonStyle)self.extraButton.overrideUserInterfaceStyle=buttonStyle;
    if(self.expandButton.overrideUserInterfaceStyle!=buttonStyle)self.expandButton.overrideUserInterfaceStyle=buttonStyle;
    if(styleChangedAny) {
        if(@available(iOS 17.0,*)) {
            for(UIView *surface in @[self.capsule,self.extra,self.expand])[surface updateTraitsIfNeeded];
        }
    }
    self.extraButton.frame=self.extra.bounds;
    self.expandButton.frame=self.expand.bounds;
    self.extraButton.hidden=self.extra.hidden;
    self.expandButton.hidden=self.expand.hidden;
    self.capsule.hidden=self.collapsed;
    self.selectionPan.enabled=(!useSystem || self.quietSystemTouches) && !self.hidden && !self.collapsed && !UIAccessibilityIsVoiceOverRunning();
    self.dockTouch.enabled=!self.hidden && !UIAccessibilityIsVoiceOverRunning();
    self.capsule.layer.cornerRadius=radius;
    self.capsule.layer.borderWidth=!flat && !useSystem && translucent && !classicBlur && !nativeGlass ? 0.5 : 0;
    self.capsule.layer.borderColor=[UIColor colorWithWhite:light?0:1 alpha:0.14].CGColor;
    self.extra.layer.cornerRadius=circle/2;
    self.expand.layer.cornerRadius=22;
    self.extra.layer.borderWidth=!flat && translucent && !classicBlur && !nativeGlass ? 0.5 : 0;
    self.extra.layer.borderColor=[UIColor colorWithWhite:light?0:1 alpha:0.14].CGColor;
    BOOL darkSurface=self.capsule.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark;
    BOOL nativeSelection=liquidSelection && !reduced && HGNativeGlassConfigured();
    UIColor *selectionBg=nativeSelection ? UIColor.clearColor : HGADockSelectionColor(self.capsule.traitCollection,flat || !liquidSelection);
    UIColor *selectionEdge=[UIColor colorWithWhite:darkSurface?1:0 alpha:darkSurface?0.23:0.11];
    self.selection.backgroundColor=selectionBg;
    self.selection.layer.borderWidth=liquidSelection && !nativeSelection ? 0.5 : 0;
    self.selectionHighlight.backgroundColor=nativeSelection ? UIColor.clearColor : [UIColor colorWithWhite:1 alpha:0.16];
    self.selectionHighlight.layer.borderWidth=nativeSelection ? 0 : 1;
    self.selection.layer.borderColor=selectionEdge.CGColor;
    CGRect selectedFrame=CGRectNull;
    NSInteger flatIndex=UISegmentedControlNoSegment;
    CGFloat slotWidth=expanded.size.width/visible.count;
    for(NSString *key in self.buttons) {
        UIButton *b=self.buttons[key];NSUInteger index=[HGATabIDs() indexOfObject:key];
        if(b.superview!=self.capsule)[self.capsule addSubview:b];
        NSUInteger position=[visible indexOfObject:key];
        b.hidden=(useSystem && !self.quietSystemTouches) || position==NSNotFound;
        if(b.hidden)continue;
        UIControl *source=[self.targets objectForKey:key];
        BOOL active=selected!=NSNotFound ? selected==(NSInteger)index : source.selected;
        b.enabled=source.enabled;b.selected=active;
        b.isAccessibilityElement=YES;
        b.accessibilityLabel=HGATabTitles()[index];
        b.accessibilityTraits=UIAccessibilityTraitButton | (active?UIAccessibilityTraitSelected:0);
        HGADockSetInkEdge(b,self.quietSystemTouches ? nil : inkEdge);
        if(self.quietSystemTouches) {
            // The UITabBar draws both content and selection. These unpainted
            // controls use its measured slots without triggering touch haptics.
            b.frame=position<self.systemHitSlots.count ? self.systemHitSlots[position].CGRectValue : CGRectZero;
            if(b.currentTitle)[b setTitle:nil forState:UIControlStateNormal];
            if(b.currentImage)[b setImage:nil forState:UIControlStateNormal];
            continue;
        }
        b.frame=CGRectMake(position*slotWidth,0,slotWidth,height);
        UIColor *color=active ? UIColor.labelColor :
            (translucent ? [UIColor.labelColor colorWithAlphaComponent:unselectedOpacity] : UIColor.secondaryLabelColor);
        if(videoClear)color=active ? UIColor.whiteColor : [UIColor.whiteColor colorWithAlphaComponent:unselectedOpacity];
        NSString *symbol=HGATabSymbols()[index];
        UIImage *image=[UIImage systemImageNamed:(flat || active) ? [symbol stringByAppendingString:@".fill"] : symbol] ?: [UIImage systemImageNamed:symbol];
        NSString *title=[content isEqual:@"icon"]?nil:HGATabTitles()[index];
        UIFont *font=[UIFont systemFontOfSize:[content isEqual:@"text"]?15:11 weight:active?UIFontWeightSemibold:UIFontWeightRegular];
        if(HGAppearanceFlag(@"fonts.enabled"))font=HGAConfiguredFont(font);
        CGFloat iconHeight=title.length ? height-11-font.lineHeight : height*0.34;
        CGFloat iconSize=MAX(12,MIN(requestedIcon,MIN(iconHeight,slotWidth-16)));
        // A host layout during tracking must not replace the image view that
        // is being animated and sampled by the moving lens.
        if((!self.pressedTab && !self.selectionDragging) || (!b.currentImage && !b.currentTitle.length)) {
            UIImageSymbolConfiguration *symbolConfig=[UIImageSymbolConfiguration configurationWithPointSize:iconSize weight:UIImageSymbolWeightMedium];
            UIImage *glyph=[content isEqual:@"text"] ? nil : [image imageByApplyingSymbolConfiguration:symbolConfig];
            [b setImage:glyph forState:UIControlStateNormal];
            [b setTitle:title forState:UIControlStateNormal];
            [b setTitleColor:color forState:UIControlStateNormal];
            b.tintColor=color;b.titleLabel.font=font;
            b.alpha=b.enabled ? 1 : 0.4;
            [b setNeedsLayout];
        }
        if(active) {
            flatIndex=(NSInteger)position;
            selectedFrame=HGADockSelectionFrame(CGRectGetMidX(b.frame),expanded.size.width,height,slotWidth,NO);
        }
    }
    self.restingFlatIndex=flatIndex;
    self.restingSelectionFrame=selectedFrame;
    if((self.selectionDragging || self.pressedTab) && !self.collapsed)
        [self positionDragSelection:self.selectionDragX];
    else if(!self.selectionSettling) {
        self.selection.hidden=flat || CGRectIsNull(selectedFrame);
        if(flat && self.flatSegments.selectedSegmentIndex!=flatIndex)self.flatSegments.selectedSegmentIndex=flatIndex;
        if(!CGRectIsNull(selectedFrame))[self placeSelection:selectedFrame];
    }
    if(useSystem) {
        [self.dragLens removeFromSuperview];self.dragLens=nil;
    }else [self prepareSelectionLens];
    BOOL restingGlass=nativeSelection;
    self.restingLens=HGADockRestingLens(self.restingLens,self.selection,selectedFrame,restingGlass && !useSystem);
    self.restingLens.hidden=self.selectionDragging || self.pressedTab || self.selectionSettling;
    NSString *action=HGAppearanceString(@"dock.action");
    NSUInteger detachedIndex=[HGATabIDs() indexOfObject:action];
    UIControl *detachedSource=detachedIndex!=NSNotFound ? [self.targets objectForKey:action] : nil;
    BOOL detachedSelected=detachedIndex!=NSNotFound &&
        (selected!=NSNotFound ? selected==(NSInteger)detachedIndex : detachedSource.selected);
    NSString *baseSymbol=[action isEqual:@"settings"]?@"slider.horizontal.3":([action isEqual:@"search"]?@"magnifyingglass":HGATabSymbols()[detachedIndex]);
    UIImage *extraImage=detachedSelected || (flat && detachedIndex!=NSNotFound) ?
        ([UIImage systemImageNamed:[baseSymbol stringByAppendingString:@".fill"]] ?: [UIImage systemImageNamed:baseSymbol]) : [UIImage systemImageNamed:baseSymbol];
    UIColor *accessoryColor=detachedSelected ? UIColor.labelColor :
        (detachedIndex!=NSNotFound && translucent ? [UIColor.labelColor colorWithAlphaComponent:unselectedOpacity] : UIColor.labelColor);
    if(videoClear)accessoryColor=detachedSelected || detachedIndex==NSNotFound ? UIColor.whiteColor : [UIColor.whiteColor colorWithAlphaComponent:unselectedOpacity];
    UIFont *accessoryFont=[UIFont systemFontOfSize:11];
    CGFloat extraIconSize=MIN(requestedIcon,circle*0.34);
    HGADockConfigureButton(self.extraButton,nil,extraImage,extraIconSize,accessoryFont,accessoryColor);
    HGADockConfigureButton(self.expandButton,nil,[UIImage systemImageNamed:@"ellipsis"],MIN(requestedIcon,22),accessoryFont,accessoryColor);
    HGADockSetInkEdge(self.extraButton,inkEdge);HGADockSetInkEdge(self.expandButton,inkEdge);
    NSInteger detachedSegment=detachedSelected ? 0 : UISegmentedControlNoSegment;
    if(extraTrack && extraTrack.selectedSegmentIndex!=detachedSegment)extraTrack.selectedSegmentIndex=detachedSegment;
    self.extraSelection.hidden=flat || !detachedSelected || self.extra.hidden;
    if(!self.extraSelection.hidden) {
        CGRect inner=self.extra.bounds;
        self.extraSelection.frame=inner;
        self.extraSelection.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
        self.extraSelection.layer.cornerRadius=MIN(inner.size.width,inner.size.height)/2;
        self.extraSelection.backgroundColor=selectionBg;
        self.extraSelection.layer.borderWidth=liquidSelection && !nativeSelection ? 0.5 : 0;
        self.extraSelection.layer.borderColor=selectionEdge.CGColor;
    }
    self.extraRestingLens=HGADockRestingLens(self.extraRestingLens,self.extraSelection,self.extraButton.frame,
        restingGlass && detachedSelected && !self.extra.hidden);
    self.extraButton.selected=detachedSelected;
    self.extraButton.accessibilityLabel=[action isEqual:@"settings"]?@"Honggo 外观设置":([action isEqual:@"search"]?@"搜索":HGATabTitles()[detachedIndex]);
    self.extraButton.accessibilityTraits=UIAccessibilityTraitButton | (detachedSelected?UIAccessibilityTraitSelected:0);
    CGRect occupied=self.collapsed?compact:(extra?CGRectUnion(actual,self.extra.frame):actual);
    occupied=[self convertRect:occupied toView:self.window];
    if(capsuleChanged || !CGRectEqualToRect(self.lastOccupiedRect,occupied)) {
        self.lastOccupiedRect=occupied;
        // Let the native HUD finish its own layout before reconciling the new
        // capsule edge. Never reenter a player layout from a tab-bar setter.
        HGAInvalidateDockDependentLayout(self.window);
    }
}
- (void)restore {
    if(self.restoring)return;
    UIWindow *previousWindow=self.window;
    BOOL wasRendering=self.rendering;
    self.restoring=YES;self.rendering=YES;
    @try {
        // Restoring a source calls host setters. Their per-instance observers
        // can request another dock/tab-bar update synchronously. Consume the
        // complete old ownership before any setter, and keep strong immutable
        // pairs so neither reentry nor a weak-key removal can mutate the walk.
        NSMutableArray<NSArray *> *restorations=[NSMutableArray array];
        for(UIView *view in self.hiddenSources.keyEnumerator.allObjects) {
            NSNumber *hidden=[self.hiddenSources objectForKey:view];
            if(hidden)[restorations addObject:@[view,hidden]];
        }
        [self.hiddenSources removeAllObjects];
        UIView *bar=self.hostBar;
        BOOL backgroundOwned=self.backgroundOwned,opaque=self.hostOpaque;
        UIColor *background=self.hostBackground;
        self.backgroundOwned=NO;self.collapsed=NO;
        [self removeSystemDock];
        self.failedSystemProbe=nil;
        [self clearSelectionDrag];
        self.dockTouchButton=nil;
        [self.dockTouch.view removeGestureRecognizer:self.dockTouch];
        [self.selectionPan.view removeGestureRecognizer:self.selectionPan];
        self.lastOccupiedRect=CGRectNull;
        for(NSArray *entry in restorations) {
            UIView *view=entry[0];
            if(view.hidden)view.hidden=[entry[1] boolValue];
        }
        if(backgroundOwned && [bar.backgroundColor isEqual:UIColor.clearColor])bar.backgroundColor=background;
        if(backgroundOwned && !bar.opaque)bar.opaque=opaque;
        [self removeFromSuperview];
        if(previousWindow)HGAInvalidateDockDependentLayout(previousWindow);
    } @finally {
        self.rendering=wasRendering;self.restoring=NO;
    }
}
- (void)feedback {
    if(HGAppearanceFlag(@"dock.haptics"))[[[UISelectionFeedbackGenerator alloc] init] selectionChanged];
}
- (void)tabTapped:(UIButton *)sender {
    if(self.dockTouchButton || self.selectionDragging || !sender.enabled || sender.hidden ||
        (self.pressedTab && self.pressedTab!=sender))return;
    [self activateTabAtIndex:sender.tag animation:[self dockMotionMode]];
    if(self.pressedTab==sender || self.quietSystemTouches)[self settleSelection];
}
- (void)activateTabAtIndex:(NSInteger)index animation:(NSString *)animation {
    if(self.sidebarSuspended || index<0 || index>=(NSInteger)HGATabIDs().count)return;
    NSString *key=HGATabIDs()[(NSUInteger)index];
    UIControl *target=[self.targets objectForKey:key];
    if(!target || !target.enabled)return;
    UIControlEvents event=(target.allControlEvents & UIControlEventTouchUpInside)?UIControlEventTouchUpInside:UIControlEventPrimaryActionTriggered;
    HGLog(@"navigation.dock", @{@"tab":key, @"phase":@"enter", @"mode":animation});
    // Native selection synchronously attaches the new page and its player. Its
    // layout callbacks must not each rebuild the same dock during that action.
    self.deliveringTabAction=YES;
    @try { [target sendActionsForControlEvents:event]; }
    @finally { self.deliveringTabAction=NO; }
    HGLog(@"navigation.dock", @{@"tab":key, @"phase":@"return", @"mode":animation});
    if(![animation hasPrefix:@"uikit-"] || !HGADockOriginalImpact)[self feedback];
    UIView *bar=self.hostBar;
    // Layout the complete host bar, including the video behind it, for the
    // incoming selection as well as a reselect. UIKit coalesces repeated marks.
    [bar setNeedsLayout];
    if ([key isEqual:@"home"]) {
        UIViewController *host=self.hostController;
        UIViewController *page=[host isKindOfClass:UITabBarController.class] ?
            ((UITabBarController *)host).selectedViewController : nil;
        id<UIViewControllerTransitionCoordinator> transition=page.transitionCoordinator ?: host.transitionCoordinator;
        [transition animateAlongsideTransition:nil completion:^(id<UIViewControllerTransitionCoordinatorContext> context) {
            dispatch_async(dispatch_get_main_queue(), ^{ [bar setNeedsLayout]; });
        }];
    }
}
- (void)placeSelection:(CGRect)frame {
    if(!CGRectEqualToRect(self.selection.frame,frame))self.selection.frame=frame;
    CGFloat radius=MIN(self.capsule.layer.cornerRadius,MIN(frame.size.width,frame.size.height)/2);
    if(self.selection.layer.cornerRadius!=radius)self.selection.layer.cornerRadius=radius;
    if(!CGRectEqualToRect(self.dragLens.frame,frame))self.dragLens.frame=frame;
    if(self.dragLens.layer.cornerRadius!=radius)self.dragLens.layer.cornerRadius=radius;
    if(!CGRectEqualToRect(self.selectionHighlight.frame,self.selection.bounds))self.selectionHighlight.frame=self.selection.bounds;
    if(self.selectionHighlight.layer.cornerRadius!=radius)self.selectionHighlight.layer.cornerRadius=radius;
}
- (void)endQuietLens {
    self.quietDragLens.effect=nil;[self.quietDragLens removeFromSuperview];self.quietDragLens=nil;
    for(UIView *part in self.quietSelectionSources.keyEnumerator.allObjects) {
        if(part.hidden && part.superview==[self.quietSelectionSources objectForKey:part] &&
            self.quietProvider==self.systemPlatter && [part isDescendantOfView:self.systemTabBar])part.hidden=NO;
    }
    self.quietSelectionSources=nil;self.quietProvider=nil;
}
- (BOOL)placeQuietLensAtX:(CGFloat)x button:(UIButton *)button {
    if(!button || UIAccessibilityIsReduceMotionEnabled()) {
        if(self.quietDragLens)[self endQuietLens];
        return NO;
    }
    CGSize size=self.capsule.bounds.size;
    CGRect local=HGADockSelectionFrame(x,size.width,size.height,button.bounds.size.width,YES);
    CGRect frame=[self.capsule convertRect:local toView:self];
    if(!self.quietSelectionSources) {
        // Only suspend a positively identified selection surface inside our
        // own UITabBar. Leave the platter, real tab glyphs and captions live.
        NSMapTable<UIView *,UIView *> *thumbs=[NSMapTable weakToWeakObjectsMapTable];
        NSBundle *uikit=[NSBundle bundleForClass:UITabBar.class];
        for(UIView *part in HGADockSystemSubviews(self.systemTabBar)) {
            NSString *name=NSStringFromClass(part.class);
            if((![name containsString:@"TabSelectionView"] && ![name containsString:@"SelectionPlatter"]) ||
                ![[NSBundle bundleForClass:part.class] isEqual:uikit] || !HGAVisible(part) ||
                [part isKindOfClass:UIControl.class] || HGADockSystemItems(part).count)continue;
            CGRect footprint=[part convertRect:part.bounds toView:self.capsule];
            if(CGRectIntersectsRect(footprint,self.capsule.bounds))[thumbs setObject:part.superview forKey:part];
        }
        self.quietSelectionSources=thumbs;
        if(thumbs.count) {
            if(@available(iOS 26.0,*)) {
                UIVisualEffectView *lens=[[UIVisualEffectView alloc] initWithEffect:nil];
                lens.accessibilityIdentifier=@"honggo.appearance.dock.quiet-lens";
                lens.accessibilityElementsHidden=YES;lens.userInteractionEnabled=NO;
                lens.overrideUserInterfaceStyle=self.capsule.overrideUserInterfaceStyle;
                lens.frame=frame;lens.cornerConfiguration=UICornerConfiguration.capsuleConfiguration;
                [self insertSubview:lens aboveSubview:[self systemDockCarrier]];
                UIGlassEffect *glass=[UIGlassEffect effectWithStyle:UIGlassEffectStyleClear];
                glass.interactive=YES;lens.effect=glass;
                self.quietDragLens=lens;self.quietProvider=self.systemPlatter;
                for(UIView *part in thumbs.keyEnumerator.allObjects)part.hidden=YES;
            }
        }
    }
    if(!self.quietDragLens)return NO;
    if(!CGRectEqualToRect(self.quietDragLens.frame,frame))self.quietDragLens.frame=frame;
    return YES;
}
- (UIButton *)positionDragSelection:(CGFloat)x {
    CGFloat first=CGFLOAT_MAX,last=-CGFLOAT_MAX,distance=CGFLOAT_MAX;
    UIButton *destination=nil;
    for(NSString *key in HGATabIDs()) {
        UIButton *button=self.buttons[key];
        if(button.hidden || !button.enabled)continue;
        CGFloat center=CGRectGetMidX(button.frame);
        first=MIN(first,center);last=MAX(last,center);
        if(fabs(center-x)<distance){destination=button;distance=fabs(center-x);}
    }
    if(self.quietSystemTouches) {
        self.selectionDragX=x;
        if(destination && ![self placeQuietLensAtX:MIN(last,MAX(first,x)) button:destination])
            [self syncSystemSelection:destination.tag force:NO];
        return destination;
    }
    self.selection.hidden=!destination;
    if(destination) {
        self.selectionDragX=x;
        // Move a single selection surface while the finger is down.
        if(self.flatSegments && self.flatSegments.selectedSegmentIndex!=UISegmentedControlNoSegment)
            self.flatSegments.selectedSegmentIndex=UISegmentedControlNoSegment;
        CGFloat height=self.capsule.bounds.size.height;
        CGRect frame=HGADockSelectionFrame(MIN(last,MAX(first,x)),self.capsule.bounds.size.width,height,
            destination.bounds.size.width,HGADockLiquidSelection() && !UIAccessibilityIsReduceMotionEnabled());
        [self placeSelection:frame];
    }
    return destination;
}
- (void)clearSelectionDrag {
    [self endQuietLens];
    self.selectionGeneration++;
    self.pressedTab=nil;
    self.selectionDragging=NO;
    self.selectionSettling=NO;
    self.motionMaterialPresented=NO;
    [self.selection.layer removeAllAnimations];
    [self.dragLens.layer removeAllAnimations];
    [self.selectionHighlight.layer removeAllAnimations];
    self.dragLens.hidden=YES;
    self.restingLens.hidden=NO;
    self.selectionHighlight.alpha=0;
    self.motionGlyph.imageView.transform=CGAffineTransformIdentity;
    self.motionGlyph.titleLabel.transform=CGAffineTransformIdentity;
    self.motionGlyph=nil;
}
- (void)prepareSelectionLens {
    NSString *style=HGAppearanceString(@"dock.material");
    if(!HGADockLiquidSelection() || !HGNativeGlassConfigured() || UIAccessibilityIsReduceTransparencyEnabled() ||
        (!self.flatSegments && ![style isEqual:@"regular"] && ![style isEqual:@"clear"])) {
        [self.dragLens removeFromSuperview];self.dragLens=nil;
        return;
    }
    if(self.dragLens || CGRectIsEmpty(self.selection.bounds))return;
    if(@available(iOS 26.0,*)) {
        UIGlassEffect *effect=[UIGlassEffect effectWithStyle:UIGlassEffectStyleClear];
        effect.interactive=YES;
        UIVisualEffectView *lens=[[UIVisualEffectView alloc] initWithEffect:nil];
        lens.accessibilityIdentifier=@"honggo.appearance.dock.drag-lens";
        lens.frame=self.selection.frame;lens.layer.cornerRadius=self.selection.layer.cornerRadius;
        lens.layer.cornerCurve=kCACornerCurveContinuous;
        lens.cornerConfiguration=[UICornerConfiguration configurationWithUniformRadius:[UICornerRadius fixedRadius:self.selection.layer.cornerRadius]];
        lens.userInteractionEnabled=NO;lens.accessibilityElementsHidden=YES;
        lens.hidden=YES;
        self.dragLens=lens;
        // Sample the assembled tab face during a press, including the symbol
        // and caption beneath the moving optical surface.
        [self.capsule addSubview:lens];
        lens.effect=effect;
    }
}
- (void)showSelectionMotionMaterial {
    if(!HGADockLiquidSelection())return;
    self.restingLens.hidden=YES;
    if(!self.motionMaterialPresented)[self prepareSelectionLens];
    self.motionMaterialPresented=YES;
    if(self.dragLens)[self.capsule bringSubviewToFront:self.dragLens];
    self.dragLens.hidden=NO;
    self.selectionHighlight.alpha=1;
}
- (void)animateMotionGlyph:(UIButton *)glyph {
    if(glyph==self.motionGlyph || (self.dragLens && !self.dragLens.hidden) || !HGADockLiquidSelection() ||
        UIAccessibilityIsReduceMotionEnabled())return;
    UIButton *previous=self.motionGlyph;
    self.motionGlyph=glyph;
    [UIView animateWithDuration:glyph?0.26:0.38 delay:0 usingSpringWithDamping:0.85 initialSpringVelocity:0
        options:UIViewAnimationOptionBeginFromCurrentState|UIViewAnimationOptionAllowUserInteraction animations:^{
            previous.imageView.transform=CGAffineTransformIdentity;
            previous.titleLabel.transform=CGAffineTransformIdentity;
            glyph.imageView.transform=CGAffineTransformMakeScale(1.18,1.18);
            glyph.titleLabel.transform=CGAffineTransformMakeScale(1.08,1.08);
        } completion:nil];
}
- (NSString *)dockMotionMode {
    if(self.quietSystemTouches)return self.quietDragLens ? @"uikit-quiet-lens" : @"uikit-quiet-selection";
    if(UIAccessibilityIsReduceMotionEnabled())return @"reduced-motion";
    if(!HGADockLiquidSelection())return @"plain-selection";
    Class glass=NSClassFromString(@"UIGlassEffect");
    return self.motionMaterialPresented && !self.dragLens.hidden && [self.dragLens.effect isKindOfClass:glass] ?
        @"glass-morph" : @"highlight-spring";
}
- (void)pressTab:(UIButton *)button {
    if(self.selectionDragging || self.pressedTab==button || !button.enabled || button.hidden)return;
    self.selectionGeneration++;
    self.selectionSettling=NO;
    self.pressedTab=button;
    if(self.quietSystemTouches) {
        self.selectionDragX=CGRectGetMidX(button.frame);
        if(![self placeQuietLensAtX:self.selectionDragX button:button])[self syncSystemSelection:button.tag force:NO];
        return;
    }
    self.restingLens.hidden=YES;
    CGFloat position=CGRectGetMidX(button.frame);
    if(UIAccessibilityIsReduceMotionEnabled()) {
        [self positionDragSelection:position];
        return;
    }
    if(!self.flatSegments && (self.selection.hidden || CGRectIsEmpty(self.selection.bounds))) {
        CGSize size=self.capsule.bounds.size;
        [self placeSelection:HGADockSelectionFrame(position,size.width,size.height,button.bounds.size.width,NO)];
        self.selection.hidden=NO;
    }
    if(self.flatSegments && self.selection.hidden) {
        CGFloat start=CGRectIsEmpty(self.selection.bounds) ? position : CGRectGetMidX(self.selection.frame);
        CGSize size=self.capsule.bounds.size;
        [self placeSelection:HGADockSelectionFrame(start,size.width,size.height,button.bounds.size.width,NO)];
    }
    [self showSelectionMotionMaterial];
    [self animateMotionGlyph:button];
    [UIView animateWithDuration:0.33 delay:0 usingSpringWithDamping:0.84 initialSpringVelocity:0
        options:UIViewAnimationOptionBeginFromCurrentState|UIViewAnimationOptionAllowUserInteraction animations:^{
            [self positionDragSelection:position];
        } completion:nil];
}
- (void)settleSelection {
    self.pressedTab=nil;
    self.selectionDragging=NO;
    if(self.quietSystemTouches) {
        [self clearSelectionDrag];
        [self syncSystemSelection:HGASelectedTab(self.hostController) force:YES];
        [self refresh];
        return;
    }
    if(!self.superview || self.collapsed || self.restoring || UIAccessibilityIsReduceMotionEnabled()) {
        [self clearSelectionDrag];
        if(self.superview && !self.restoring && !self.collapsed)[self renderGeometry];
        return;
    }
    NSUInteger generation=++self.selectionGeneration;
    self.selectionSettling=YES;
    // Apply the host's settled tab state outside the animation transaction.
    // The animation below changes only the selection and its highlight.
    [self renderGeometry];
    CGRect resting=self.restingSelectionFrame;
    [self animateMotionGlyph:nil];
    [UIView animateWithDuration:0.42 delay:0 usingSpringWithDamping:0.87 initialSpringVelocity:0
        options:UIViewAnimationOptionBeginFromCurrentState|UIViewAnimationOptionAllowUserInteraction
        animations:^{
            if(!CGRectIsNull(resting))[self placeSelection:resting];
            self.selectionHighlight.alpha=0;
        } completion:^(BOOL finished){
            if(self.selectionGeneration!=generation)return;
            [UIView performWithoutAnimation:^{
                if(self.flatSegments && self.flatSegments.selectedSegmentIndex!=self.restingFlatIndex)
                    self.flatSegments.selectedSegmentIndex=self.restingFlatIndex;
                self.selection.hidden=self.flatSegments || CGRectIsNull(resting);
                [self clearSelectionDrag];
            }];
            if(self.window && !self.collapsed && !self.restoring)[self refresh];
        }];
}
- (void)cancelTabPress:(UIButton *)button {
    if(self.pressedTab==button && !self.selectionDragging &&
        self.dockTouch.state!=UIGestureRecognizerStateBegan && self.dockTouch.state!=UIGestureRecognizerStateChanged)
        [self settleSelection];
}
- (void)beginDockDragAtX:(CGFloat)position {
    self.selectionGeneration++;
    self.selectionSettling=NO;
    self.pressedTab=nil;
    self.selectionDragging=YES;
    if(self.quietSystemTouches){[self positionDragSelection:position];return;}
    self.restingLens.hidden=YES;
    [self.selection.layer removeAllAnimations];
    [self.dragLens.layer removeAllAnimations];
    [self animateMotionGlyph:nil];
    [self positionDragSelection:position];
    if(!UIAccessibilityIsReduceMotionEnabled())[self showSelectionMotionMaterial];
}
- (void)accessoryDown:(UIButton *)button { [self animateDockAccessory:button pressed:YES]; }
- (void)accessoryUp:(UIButton *)button { [self animateDockAccessory:button pressed:NO]; }
- (void)animateDockAccessory:(UIButton *)button pressed:(BOOL)pressed {
    if(UIAccessibilityIsReduceMotionEnabled())return;
    [UIView animateWithDuration:pressed?0.22:0.38 delay:0 usingSpringWithDamping:0.85 initialSpringVelocity:0
        options:UIViewAnimationOptionBeginFromCurrentState|UIViewAnimationOptionAllowUserInteraction animations:^{
            button.imageView.transform=pressed ? CGAffineTransformMakeScale(0.91,0.91) : CGAffineTransformIdentity;
        } completion:nil];
}
- (void)willMoveToWindow:(UIWindow *)window {
    if(!window) {
        [self removeSystemDock];
        self.failedSystemProbe=nil;
        self.dockTouchButton=nil;
        [self clearSelectionDrag];
        self.extraButton.imageView.transform=CGAffineTransformIdentity;
        self.expandButton.imageView.transform=CGAffineTransformIdentity;
    }
    [super willMoveToWindow:window];
}
- (void)releaseDockButton:(UIButton *)button {
    if(button==self.extraButton || button==self.expandButton) {
        [self animateDockAccessory:button pressed:NO];
        if(button==self.extraButton)[self extraTapped:button];
        else [self expandTapped:button];
        return;
    }
    [self activateTabAtIndex:button.tag animation:[self dockMotionMode]];
    [self settleSelection];
}
- (void)beginDockTouchAtPoint:(CGPoint)point {
    self.dockTouchStart=point;
    self.dockTouchButton=[self dockButtonAtPoint:point event:nil];
    UIButton *button=self.dockTouchButton;
    if(!button)return;
    BOOL accessory=button==self.extraButton || button==self.expandButton;
    if(accessory)[self animateDockAccessory:button pressed:YES];
    else [self pressTab:button];
    HGLog(@"appearance.dock-touch",@{@"phase":@"begin",@"action":accessory?@"accessory":@"capsule",
        @"mode":[self dockMotionMode]});
}
- (void)moveDockTouchToPoint:(CGPoint)point {
    UIButton *button=self.dockTouchButton;
    if(!button)return;
    BOOL accessory=button==self.extraButton || button==self.expandButton;
    CGFloat x=[self convertPoint:point toView:self.capsule].x;
    CGFloat dx=point.x-self.dockTouchStart.x,dy=point.y-self.dockTouchStart.y;
    if(fabs(dy)>14 && fabs(dy)>fabs(dx)) {
        [self endDockTouchAtPoint:point cancelled:YES];return;
    }
    if(!accessory && !self.selectionDragging && fabs(dx)>7)[self beginDockDragAtX:x];
    if(self.selectionDragging || (!accessory && self.quietDragLens))[self positionDragSelection:x];
}
- (void)finishDockPointer:(HGADockPointer *)gesture {
    if(gesture.state==UIGestureRecognizerStateEnded || gesture.state==UIGestureRecognizerStateCancelled)
        [self endDockTouchAtPoint:gesture.dockPoint cancelled:gesture.state==UIGestureRecognizerStateCancelled];
}
- (void)endDockTouchAtPoint:(CGPoint)point cancelled:(BOOL)cancelled {
    UIButton *button=self.dockTouchButton;
    if(!button)return;
    BOOL accessory=button==self.extraButton || button==self.expandButton;
    BOOL dragged=self.selectionDragging;
    NSString *mode=[self dockMotionMode];
    UIButton *destination=nil;
    CGPoint local=[self convertPoint:point toView:self.capsule];
    if(!cancelled && (!dragged || (local.y>=0 && local.y<=CGRectGetHeight(self.capsule.bounds))))
        destination=dragged ? [self positionDragSelection:local.x] :
        ([self dockButtonAtPoint:point event:nil]==button ? button : nil);
    self.dockTouchButton=nil;
    HGLog(@"appearance.dock-touch",@{@"phase":destination?@"end":@"cancel",@"mode":mode,
        @"action":accessory?@"accessory":(dragged?@"drag":@"tap")});
    if(destination && (!dragged || !destination.selected)) {
        [self releaseDockButton:destination];
    }else if(accessory) {
        [self animateDockAccessory:button pressed:NO];
    }else {
        [self settleSelection];
    }
}
- (void)dragSelection:(UIPanGestureRecognizer *)gesture {
    CGPoint local=[gesture locationInView:self.capsule];
    CGFloat position=local.x;
    if(gesture.state==UIGestureRecognizerStateBegan) {
        [self beginDockDragAtX:position];
        HGLog(@"appearance.dock-touch",@{@"phase":@"begin",@"action":@"player-overlap",@"mode":[self dockMotionMode]});
    }else if(self.selectionDragging) {
        if(gesture.state==UIGestureRecognizerStateChanged){[self positionDragSelection:position];return;}
        if(gesture.state==UIGestureRecognizerStateEnded && local.y>=0 && local.y<=CGRectGetHeight(self.capsule.bounds)) {
            UIButton *destination=[self positionDragSelection:position];
            if(destination && !destination.selected)
                [self activateTabAtIndex:destination.tag animation:[self dockMotionMode]];
        }
        HGLog(@"appearance.dock-touch",@{@"phase":gesture.state==UIGestureRecognizerStateEnded?@"end":@"cancel",
            @"action":@"player-overlap",@"mode":[self dockMotionMode]});
        [self settleSelection];
    }
}
- (void)extraTapped:(UIButton *)sender {
    if(self.dockTouchButton || self.sidebarSuspended)return;
    NSString *action=HGAppearanceString(@"dock.action");
    if([action isEqual:@"settings"]) { HGOpenAppearance(self.hostController ?: HGAOwner(self.hostBar),nil);return; }
    NSUInteger index=[HGATabIDs() indexOfObject:action];
    if(index!=NSNotFound){[self activateTabAtIndex:(NSInteger)index animation:@"accessory"];return;}
    if(![action isEqual:@"search"])return;
    UIWindow *win=self.hostBar.window;
    if(!win)return;
    // Dispatch search presentation on the next main-loop turn so touch delivery
    // on the circular dock button settles before UINavigationController pushes.
    __weak HGAAppearanceDock *weakSelf=self;
    __weak UIWindow *weakWin=win;
    dispatch_async(dispatch_get_main_queue(),^{
        HGAAppearanceDock *dock=weakSelf;
        UIWindow *targetWin=weakWin;
        if(!dock || !targetWin || dock.sidebarSuspended)return;
        NSMutableArray *queue=[NSMutableArray array];
        UIViewController *root=targetWin.rootViewController;if(root)[queue addObject:root];
        for(NSUInteger i=0;i<queue.count && i<40;i++) {
            UIViewController *vc=queue[i];
            if([NSStringFromClass(vc.class) isEqual:@"SSVideoSeriesFeedViewController"] && HGAVisible(vc.viewIfLoaded)) {
                if(vc.presentedViewController || vc.navigationController.transitionCoordinator)return;
                SEL sel=NSSelectorFromString(@"simpleSearchViewDidClicked");Method m=class_getInstanceMethod(vc.class,sel);
                if(m && !strcmp(method_getTypeEncoding(m),"v16@0:8")){((void(*)(id,SEL))objc_msgSend)(vc,sel);[dock feedback];return;}
            }
            for(UIViewController *child in vc.childViewControllers)if(queue.count<40 && ![queue containsObject:child])[queue addObject:child];
            if(vc.presentedViewController && queue.count<40 && ![queue containsObject:vc.presentedViewController])[queue addObject:vc.presentedViewController];
        }
        UIAccessibilityPostNotification(UIAccessibilityAnnouncementNotification,@"当前页面没有已适配的搜索入口，请返回首页搜索。");
    });
}
- (void)expandTapped:(UIButton *)button {
    if(!self.dockTouchButton)[self setCollapsed:NO animated:YES];
}
- (void)swipe:(UISwipeGestureRecognizer *)gesture {
    if(HGAppearanceFlag(@"dock.collapse"))[self setCollapsed:gesture.direction==UISwipeGestureRecognizerDirectionUp animated:YES];
}
- (void)setCollapsed:(BOOL)value animated:(BOOL)animated {
    if(!HGAppearanceFlag(@"dock.collapse") || UIAccessibilityIsVoiceOverRunning())value=NO;
    if(self.collapsed==value || self.sidebarSuspended || self.rendering || self.restoring || !self.superview)return;
    self.rendering=YES;
    @try {
        self.collapsed=value;
        if(value)[self clearSelectionDrag];
        [self renderGeometry];
    } @finally { self.rendering=NO; }
    if(!animated || UIAccessibilityIsReduceMotionEnabled())return;
    // A layer animation leaves the playing video and live glass out of UIKit's
    // transition snapshot while the native pager hands off the current cell.
    UIView *surface=value ? self.expand : self.capsule;
    CABasicAnimation *animation=[CABasicAnimation animationWithKeyPath:@"transform.scale"];
    animation.fromValue=@0.94;
    animation.toValue=@1;
    animation.duration=0.22;
    [surface.layer addAnimation:animation forKey:@"honggo.dock.collapse"];
}
@end
static UIView *(*HGADockHostOriginalHit)(id,SEL,CGPoint,UIEvent *);
static UIView *HGADockHostHit(id bar,SEL cmd,CGPoint point,UIEvent *event) {
    if([[(UIView *)bar layer] animationForKey:HGACleanOpacity])return nil;
    HGAAppearanceDock *dock=objc_getAssociatedObject(bar,&HGADockKey);
    // Only a host bar moved above its verified inactive sibling needs this
    // pass-through. UIKit owns all touches in the actual tab item subtree.
    if(dock.systemDockActive && dock.systemWrapper && dock.superview==bar)
        return [dock hitTest:[dock convertPoint:point fromView:bar] withEvent:event];
    return HGADockHostOriginalHit(bar,cmd,point,event);
}
static BOOL HGADockInstallHostHitRoute(void) {
    if(HGADockHostOriginalHit)return YES;
    Class target=objc_lookUpClass("SSTabBar");
    SEL method=@selector(hitTest:withEvent:);
    IMP original=HGInstallVerifiedHook(target,method,"@40@0:8{CGPoint=dd}16@32",(IMP)HGADockHostHit,@"appearance");
    HGADockHostOriginalHit=(UIView *(*)(id,SEL,CGPoint,UIEvent *))original;
    return original!=NULL;
}
BOOL HGAppearanceDockIsMutating(UIView *bar) {
    HGAAppearanceDock *dock=objc_getAssociatedObject(bar,&HGADockKey);
    return dock.rendering || dock.restoring || dock.deliveringTabAction;
}
static void HGARefreshSheetDockVisibility(UIWindow *window) {
    if(!window)return;
    BOOL covered=HGAWindowHasActionSheet(window);
    for(UIView *bar in HGABars.allObjects) {
        if(bar.window!=window)continue;
        HGAAppearanceDock *dock=objc_getAssociatedObject(bar,&HGADockKey);
        if(dock.sidebarSuspended || dock.superview!=bar || dock.hidden==covered)continue;
        dock.hidden=covered;
        dock.selectionPan.enabled=!covered && (!dock.systemDockActive || dock.quietSystemTouches) &&
            !dock.collapsed && !UIAccessibilityIsVoiceOverRunning();
        dock.dockTouch.enabled=!covered && !UIAccessibilityIsVoiceOverRunning();
        if(!covered && !dock.systemDockActive) {
            dock.failedSystemProbe=nil;
            [dock refresh];
        }
    }
}
void HGAppearanceLayoutDock(UIView *bar,UIViewController *controller) {
    if(!NSThread.isMainThread || !bar || HGFeedSidebarOwnsLayout(bar.window))return;
    if(!HGABars)HGABars=[NSHashTable weakObjectsHashTable];[HGABars addObject:bar];
    HGAAppearanceDock *dock=objc_getAssociatedObject(bar,&HGADockKey);
    if(!HGAppearanceDockEnabled()){[dock restore];return;}
    static dispatch_once_t impactOnce;
    dispatch_once(&impactOnce,^{
        HGADockOriginalImpact=(void (*)(id,SEL))HGInstallVerifiedHook(objc_lookUpClass("SSTabBarController"),
            sel_registerName("impactFeedback"),"v16@0:8",(IMP)HGADockHostImpact,@"appearance");
    });
    if(!HGADockOriginalBarColor)HGADockOriginalBarColor=(void (*)(UIView *,SEL,UIColor *))
        HGInstallVerifiedHook(objc_lookUpClass("SSTabBar"),@selector(setBackgroundColor:),"v24@0:8@16",
            (IMP)HGADockSetBarColor,@"appearance");
    if(!dock){dock=[[HGAAppearanceDock alloc] initWithFrame:bar.bounds];dock.hostBar=bar;
        objc_setAssociatedObject(bar,&HGADockKey,dock,OBJC_ASSOCIATION_RETAIN_NONATOMIC);}
    if(controller)dock.hostController=controller;
    if(!dock.hostController)dock.hostController=HGAOwner(bar);
    [dock refresh];
}
void HGAppearanceRestoreDock(UIView *bar) {[(HGAAppearanceDock *)objc_getAssociatedObject(bar,&HGADockKey) restore];}
BOOL HGAppearanceSuspendDockForSidebar(UIView *bar) {
    HGAAppearanceDock *dock=objc_getAssociatedObject(bar,&HGADockKey);
    if(!dock || !bar || dock.superview!=bar || dock.hidden)return NO;
    dock.sidebarSuspended=YES;
    BOOL tracking=dock.pressedTab || dock.selectionDragging || dock.selectionSettling;
    dock.dockTouchButton=nil;
    [dock clearSelectionDrag];
    if(tracking && dock.quietSystemTouches)[dock syncSystemSelection:HGASelectedTab(dock.hostController) force:YES];
    dock.dockTouch.enabled=NO;
    dock.selectionPan.enabled=NO;
    dock.userInteractionEnabled=NO;
    dock.extraButton.imageView.transform=CGAffineTransformIdentity;
    dock.expandButton.imageView.transform=CGAffineTransformIdentity;
    if(tracking && !dock.systemDockActive) {
        // A released preview normally settles in renderGeometry. While the
        // drawer owns the layout, restore only the existing selection.
        dock.selection.hidden=YES;
        NSArray<NSString *> *keys=HGADockCapsuleTabs();
        for(NSUInteger index=0;index<keys.count;index++) {
            UIButton *button=dock.buttons[keys[index]];
            if(!button.selected || button.hidden)continue;
            if(dock.flatSegments)dock.flatSegments.selectedSegmentIndex=(NSInteger)index;
            else {
                CGRect bounds=dock.capsule.bounds;
                [dock placeSelection:HGADockSelectionFrame(CGRectGetMidX(button.frame),CGRectGetWidth(bounds),
                    CGRectGetHeight(bounds),CGRectGetWidth(button.bounds),NO)];
                dock.selection.hidden=NO;
            }
            break;
        }
    }
    return YES;
}
static void HGARefreshDock(UIView *bar){HGAppearanceLayoutDock(bar,nil);}
@interface HGADockScrollObserver : NSObject
+ (instancetype)shared;
- (void)pan:(UIPanGestureRecognizer *)gesture;
@end
@implementation HGADockScrollObserver
+ (instancetype)shared{static HGADockScrollObserver *s;static dispatch_once_t once;dispatch_once(&once,^{s=[self new];});return s;}
- (void)pan:(UIPanGestureRecognizer *)gesture {
    if(!HGAppearanceDockEnabled() || !HGAppearanceFlag(@"dock.autoCollapse") || !HGAppearanceFlag(@"dock.collapse") ||
        UIAccessibilityIsVoiceOverRunning() || gesture.state!=UIGestureRecognizerStateEnded)return;
    CGPoint translation=[gesture translationInView:gesture.view];
    if(fabs(translation.y)<40 || fabs(translation.y)<=fabs(translation.x))return;
    BOOL collapse=translation.y<0;
    UIWindow *win=gesture.view.window;
    if(!win || HGFeedSidebarOwnsLayout(win))return;
    __weak UIWindow *weakWin=win;
    dispatch_async(dispatch_get_main_queue(),^{
        UIWindow *targetWin=weakWin;
        if(!targetWin || HGFeedSidebarOwnsLayout(targetWin))return;
        for(UIView *bar in HGABars.allObjects) {
            HGAAppearanceDock *dock=objc_getAssociatedObject(bar,&HGADockKey);
            if(bar.window==targetWin && fabs(bar.frame.origin.x)<=2.0 && dock.superview==bar)
                [dock setCollapsed:collapse animated:YES];
        }
    });
}
@end
static char HGAFollowsFeed;
static void HGAObserveFeed(UIScrollView *feed) {
    BOOL enabled=HGAppearanceDockEnabled() && HGAppearanceFlag(@"dock.autoCollapse") && feed.window;
    BOOL registered=[objc_getAssociatedObject(feed,&HGAFollowsFeed) boolValue];
    if(enabled && !registered)[feed.panGestureRecognizer addTarget:HGADockScrollObserver.shared action:@selector(pan:)];
    if(!enabled && registered)[feed.panGestureRecognizer removeTarget:HGADockScrollObserver.shared action:@selector(pan:)];
    objc_setAssociatedObject(feed,&HGAFollowsFeed,@(enabled),OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
