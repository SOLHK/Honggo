#import <UIKit/UIGestureRecognizerSubclass.h>
#import "DockSelectionGeometry.h"
#import "SiriaiDockMotion.h"

typedef NS_ENUM(NSInteger, HGADockCollapseMode) {
    HGADockCollapseModeOff = 0,
    HGADockCollapseModeLegacy = 1,
    HGADockCollapseModeTikTok = 2,
    HGADockCollapseModeSiriai = 3,
};

static HGADockCollapseMode HGACurrentDockCollapseMode(void) {
    NSString *mode=HGAppearanceString(@"dock.collapseMode");
    if([mode isEqual:@"tiktok"])return HGADockCollapseModeTikTok;
    if([mode isEqual:@"siriai"])return HGADockCollapseModeSiriai;
    if([mode isEqual:@"legacy"] || HGAppearanceFlag(@"dock.collapse"))return HGADockCollapseModeLegacy;
    return HGADockCollapseModeOff;
}

static BOOL HGADockUsesFeedMotionMode(HGADockCollapseMode mode) {
    return mode==HGADockCollapseModeTikTok || mode==HGADockCollapseModeSiriai;
}

@interface HGAFeedDockItemPart : NSObject
@property(nonatomic,weak) CALayer *buttonLayer;
@property(nonatomic,weak) UIImageView *icon;
@property(nonatomic,strong) UILabel *caption;
@end
@implementation HGAFeedDockItemPart @end
static char HGAFeedDockItemPartKey;

@class HGAAppearanceDock;
@interface HGAFeedSpringTarget : NSObject
@property(nonatomic,weak) HGAAppearanceDock *dock;
- (void)advance:(CADisplayLink *)link;
@end
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
@property(nonatomic,copy) NSString *lastSystemPlatterClass;
@property(nonatomic) CGRect lastSystemPlatterMeasured;
@property(nonatomic) CGFloat lastSystemPlatterRenderedWidth;
@property(nonatomic) CGFloat lastSystemPlatterRequestedWidth;
@property(nonatomic) BOOL lastSystemManaged;
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
@property(nonatomic,strong) UIColor *restingSelectionColor;
@property(nonatomic,weak) UIButton *motionGlyph;
@property(nonatomic,strong) UIView *extra;
@property(nonatomic,strong) UIView *extraSelection;
@property(nonatomic,strong) UIView *expand;
@property(nonatomic,strong) UIButton *extraButton;
@property(nonatomic,strong) UIButton *expandButton;
@property(nonatomic,copy) NSArray *extraButtonSignature;
@property(nonatomic,copy) NSArray *expandButtonSignature;
@property(nonatomic,strong) NSMutableDictionary<NSString *,UIButton *> *buttons;
@property(nonatomic,strong) NSArray<UISwipeGestureRecognizer *> *swipes;
@property(nonatomic,strong) UIPanGestureRecognizer *selectionPan;
@property(nonatomic,strong) HGADockPointer *dockTouch;
@property(nonatomic,weak) UIButton *dockTouchButton;
@property(nonatomic) CGPoint dockTouchStart;
@property(nonatomic,weak) UISegmentedControl *flatSegments;
@property(nonatomic,strong) UIVisualEffectView *dragLens;
@property(nonatomic) CGFloat dragLensCornerRadius;
@property(nonatomic) BOOL dragLensIsCapsule;
@property(nonatomic,strong) UIVisualEffectView *quietDragLens;
@property(nonatomic) BOOL quietLensActive;
@property(nonatomic,strong) NSMapTable<UIView *,UIView *> *quietSelectionSources;
@property(nonatomic,weak) UIView *quietProvider;
@property(nonatomic,strong) UISelectionFeedbackGenerator *selectionFeedback;
@property(nonatomic,weak) UIButton *dragHoveredTab;
@property(nonatomic) CGFloat dragLastX;
@property(nonatomic) CFTimeInterval dragLastTimestamp;
@property(nonatomic) CGFloat dragVelocityX;
@property(nonatomic) CGFloat dragPeakVelocity;
@property(nonatomic) CFTimeInterval motionStartTimestamp;
@property(nonatomic) BOOL motionMaterialPresented;
@property(nonatomic,weak) UIButton *pressedTab;
@property(nonatomic) NSUInteger selectionGeneration;
@property(nonatomic) BOOL selectionDragging;
@property(nonatomic) BOOL selectionSettling;
@property(nonatomic) CGFloat selectionDragX;
@property(nonatomic) CGRect restingSelectionFrame;
@property(nonatomic) NSInteger restingFlatIndex;
@property(nonatomic) BOOL collapsed;
@property(nonatomic) HGADockCollapseMode appliedCollapseMode;
@property(nonatomic) DKFeedSpringState feedSpring;
@property(nonatomic,strong) CADisplayLink *feedDisplayLink;
@property(nonatomic,strong) CALayer *siriaiMinimumFill;
@property(nonatomic,copy) NSArray<HGAFeedDockItemPart *> *systemFeedItems;
@property(nonatomic) CGRect feedCapsuleAtRest;
@property(nonatomic) CGRect feedExtraAtRest;
@property(nonatomic) CGPoint feedSystemCarrierAtRest;
@property(nonatomic) CGFloat feedSystemBaseScale;
@property(nonatomic) CGFloat feedCompactCapsuleWidth;
@property(nonatomic) CGFloat feedExtraScale;
@property(nonatomic) NSInteger feedLastSelectedTab;
@property(nonatomic) BOOL feedControlsHidden;
@property(nonatomic) BOOL feedLightSurface;
@property(nonatomic) BOOL sidebarSuspended;
@property(nonatomic) BOOL rendering;
@property(nonatomic) BOOL restoring;
@property(nonatomic) CGRect lastOccupiedRect;
- (void)refresh;
- (void)restore;
- (void)claimHostChrome;
- (void)setCollapsed:(BOOL)value animated:(BOOL)animated;
- (BOOL)usesFeedMotion;
- (BOOL)isFeedCompact;
- (BOOL)beginFeedDrag;
- (void)moveFeedDrag:(CGFloat)delta;
- (void)endFeedDragCompact:(BOOL)compact velocity:(CGFloat)velocity;
- (void)setFeedCompact:(BOOL)compact animated:(BOOL)animated;
- (void)advanceFeedSpring:(CADisplayLink *)link;
- (void)cancelActiveFeedObserverIfNeeded;
- (void)dragSelection:(UIPanGestureRecognizer *)gesture;
- (void)beginDockTouchAtPoint:(CGPoint)point;
- (void)moveDockTouchToPoint:(CGPoint)point;
- (void)endDockTouchAtPoint:(CGPoint)point cancelled:(BOOL)cancelled;
- (void)removeSystemDock;
@end

@interface HGADockScrollObserver : NSObject
@property(nonatomic,weak) UIPanGestureRecognizer *activeGesture;
@property(nonatomic,weak) HGAAppearanceDock *activeDock;
@property(nonatomic) DKFeedPanState panState;
+ (instancetype)shared;
- (void)pan:(UIPanGestureRecognizer *)gesture;
- (void)cancelFeedPan;
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
        if(bar.window && dock.hostBar==bar)return;
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
    }else if(HGTabBarKeepsFallbackFill(bar,color))color=UIColor.clearColor;
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
        if([dock usesFeedMotion] && !CGRectIsEmpty(dock.feedCapsuleAtRest)) {
            CGRect rest=dock.feedCapsuleAtRest;
            if(!dock.extra.hidden && !CGRectIsEmpty(dock.feedExtraAtRest))rest=CGRectUnion(rest,dock.feedExtraAtRest);
            CGRect rect=[dock convertRect:rest toView:coordinateView];
            result=CGRectIsNull(result)?rect:CGRectUnion(result,rect);
            continue;
        }
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
        if(dock.window==window && !dock.collapsed && HGAVisible(dock.capsule)) {
            if([dock usesFeedMotion] && !CGRectIsEmpty(dock.feedCapsuleAtRest))
                return [dock convertRect:dock.feedCapsuleAtRest toView:window];
            return [dock.capsule convertRect:dock.capsule.bounds toView:window];
        }
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
typedef void (^HGADockTextTabAction)(UIControl *);
static HGADockTextTabAction HGATextTabAction(UIControl *control) {
    // 7.4.5.33 binds SSTabBarTextButton to either touch-down or touch-up.
    // Its installed callback takes the original control and resolves its live tag.
    if(![control isKindOfClass:objc_lookUpClass("SSTabBarTextButton")])return nil;
    SEL getter=sel_registerName("tabBarButtonTappedBlock");
    Method method=class_getInstanceMethod(object_getClass(control),getter);
    if(!method || strcmp(method_getTypeEncoding(method),"@?16@0:8"))return nil;
    return ((HGADockTextTabAction (*)(id,SEL))objc_msgSend)(control,getter);
}
static NSArray *HGATabSymbols(void){return @[@"house",@"play.rectangle",@"bag",@"person"];}
@interface HGADockTabButton : UIButton
@property(nonatomic) CGFloat faceOffsetX;
@property(nonatomic,copy) NSArray *visualSignature;
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
    CGFloat width=MAX(0,CGRectGetWidth(bounds)-4-2*fabs(self.faceOffsetX)),height=MAX(0,CGRectGetHeight(bounds)-4);
    CGFloat titleHeight=self.currentTitle.length ? MIN(height,ceil(self.titleLabel.font.lineHeight)) : 0;
    UIImage *image=self.currentImage;
    CGFloat gap=image && titleHeight ? 2 : 0;
    CGSize glyph=image ? image.size : CGSizeZero;
    if(glyph.width>0 && glyph.height>0) {
        CGFloat scale=MIN(1,MIN(width/glyph.width,MAX(0,height-titleHeight-gap)/glyph.height));
        glyph=CGSizeMake(glyph.width*scale,glyph.height*scale);
    }
    CGFloat x=CGRectGetMidX(bounds)+self.faceOffsetX,top=CGRectGetMidY(bounds)-(glyph.height+gap+titleHeight)/2;
    self.imageView.hidden=!image;
    self.imageView.bounds=(CGRect){CGPointZero,glyph};
    self.imageView.center=CGPointMake(x,top+glyph.height/2);
    self.titleLabel.hidden=titleHeight==0;
    self.titleLabel.bounds=CGRectMake(0,0,width,titleHeight);
    self.titleLabel.center=CGPointMake(x,top+glyph.height+gap+titleHeight/2);
}
@end
@interface HGADockTouchLens : UIVisualEffectView
@property(nonatomic,weak) UIView *touchHost;
@end
@implementation HGADockTouchLens
- (UIView *)_flexInteractionGestureView {
    UIView *host=self.touchHost;
    if(host.window && host.window==self.window)return host;
    IMP inherited=class_getMethodImplementation(UIVisualEffectView.class,_cmd);
    return ((UIView *(*)(id,SEL))inherited)(self,_cmd);
}
@end
static UIVisualEffectView *HGADockMakeInteractiveLens(UIView *touchHost) {
    Method flex=touchHost ? class_getInstanceMethod(UIVisualEffectView.class,sel_registerName("_flexInteractionGestureView")) : NULL;
    if(flex && !strcmp(method_getTypeEncoding(flex),"@16@0:8")) {
        HGADockTouchLens *lens=[[HGADockTouchLens alloc] initWithEffect:nil];
        lens.touchHost=touchHost;
        return lens;
    }
    return [[UIVisualEffectView alloc] initWithEffect:nil];
}
static BOOL HGADockLiquidSelection(void) {
    return [HGAppearanceString(@"dock.selectionStyle") isEqual:@"glass"];
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
    // Home video playback always has a dark canvas and white chrome, even when
    // the app's Day theme is active. Light optical Regular glass washes out white.
    if(selected==0)return UIUserInterfaceStyleDark;
    BOOL hostDark=NO;
    if(HGAHostExplicitDarkMode(bar,&hostDark) && hostDark)return UIUserInterfaceStyleDark;
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
        if(content.traitCollection.userInterfaceStyle!=UIUserInterfaceStyleUnspecified)
            return content.traitCollection.userInterfaceStyle;
    }
    return HGAAdaptivePageStyle(controller,bar);
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

static NSString *HGADockColorCacheToken(UIColor *color) {
    if(!color)return @"none";
    CGFloat r=0,g=0,b=0,a=0;
    if([color getRed:&r green:&g blue:&b alpha:&a])
        return [NSString stringWithFormat:@"%.3f,%.3f,%.3f,%.3f",r,g,b,a];
    return [NSString stringWithFormat:@"%p",color];
}

static UIImage *HGADockSystemImage(NSString *symbol,NSString *caption,CGFloat size,UIColor *color,BOOL selected,UIColor *edge) {
    static NSCache<NSString *,UIImage *> *cache;
    static dispatch_once_t once;
    dispatch_once(&once,^{
        cache=[NSCache new];
        cache.countLimit=64;
    });
    NSString *cacheKey=[NSString stringWithFormat:@"%@|%@|%.2f|%@|%d|%@",
        symbol ?: @"",caption ?: @"",size,HGADockColorCacheToken(color),selected,HGADockColorCacheToken(edge)];
    UIImage *cached=[cache objectForKey:cacheKey];
    if(cached)return cached;
    UIImageSymbolConfiguration *config=[UIImageSymbolConfiguration configurationWithPointSize:size weight:UIImageSymbolWeightMedium];
    UIImage *glyph=symbol ? [UIImage systemImageNamed:symbol withConfiguration:config] : nil;
    if(selected && glyph)
        glyph=[UIImage systemImageNamed:[symbol stringByAppendingString:@".fill"] withConfiguration:config] ?: glyph;
    glyph=[glyph imageWithTintColor:color renderingMode:UIImageRenderingModeAlwaysOriginal];
    if(!caption && !edge) {
        if(glyph)[cache setObject:glyph forKey:cacheKey];
        return glyph;
    }
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
    UIImage *result=[face imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
    if(result)[cache setObject:result forKey:cacheKey];
    return result;
}
static BOOL HGADockContainsPoint(UIView *capsule,CGPoint point) {
    CGRect bounds=capsule.bounds;
    if(!HGAVisible(capsule) || !CGRectContainsPoint(bounds,point))return NO;
    CGFloat r=MIN(capsule.layer.cornerRadius,MIN(bounds.size.width,bounds.size.height)*0.5);
    if(r<=0.5)return YES;
    CGFloat dx=MAX(0,MAX(bounds.origin.x+r-point.x,point.x-(CGRectGetMaxX(bounds)-r)));
    CGFloat dy=MAX(0,MAX(bounds.origin.y+r-point.y,point.y-(CGRectGetMaxY(bounds)-r)));
    return dx*dx+dy*dy<=r*r;
}
@implementation HGAFeedSpringTarget
- (void)advance:(CADisplayLink *)link {
    [self.dock advanceFeedSpring:link];
}
@end

static void HGADockSetViewGeometry(UIView *view,CGAffineTransform transform,CGPoint center) {
    if(!CGAffineTransformEqualToTransform(view.transform,transform))view.transform=transform;
    if(!CGPointEqualToPoint(view.center,center))view.center=center;
}

@implementation HGAAppearanceDock
- (void)cancelActiveFeedObserverIfNeeded {
    if(HGADockScrollObserver.shared.activeDock==self)[HGADockScrollObserver.shared cancelFeedPan];
}
- (instancetype)initWithFrame:(CGRect)frame {
    if((self=[super initWithFrame:frame])) {
        self.accessibilityIdentifier=@"honggo.appearance.dock";
        self.lastOccupiedRect=CGRectNull;
        self.lastSystemPlatterMeasured=CGRectNull;
        DKFeedSpringInit(&_feedSpring);
        _feedSystemBaseScale=1.0;
        _feedExtraScale=1.0;
        _feedLastSelectedTab=NSNotFound;
        [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(handleApplicationDidEnterBackground:)
            name:UIApplicationDidEnterBackgroundNotification object:nil];
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
- (void)dealloc {
    [_feedDisplayLink invalidate];
    [NSNotificationCenter.defaultCenter removeObserver:self];
}
- (BOOL)usesFeedMotion {
    return HGADockUsesFeedMotionMode(self.appliedCollapseMode);
}
- (BOOL)isFeedCompact {
    return _feedSpring.compact!=0;
}
- (BOOL)acceptsSelectionTouchAtPoint:(CGPoint)point inView:(UIView *)hit {
    if(self.feedControlsHidden)return NO;
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
    if(self.feedControlsHidden)return nil;
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
    if(self.hidden || self.feedControlsHidden || !self.userInteractionEnabled || ![self pointInside:point withEvent:event])return nil;
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
    if(self.targets.count) {
        BOOL cachedValid=YES;
        for(NSString *key in HGATabIDs()) {
            if(HGAppearanceFlag([@"dock.hide." stringByAppendingString:key]))continue;
            UIControl *existing=[self.targets objectForKey:key];
            if(!existing || ![existing isDescendantOfView:content]){cachedValid=NO;break;}
        }
        if(cachedValid)return YES;
    }
    [self.targets removeAllObjects];
    for(UIView *node in HGADescendants(content,180)) {
        if(![node isKindOfClass:UILabel.class])continue;
        NSString *title=[(UILabel *)node text];NSUInteger index=title?[HGATabTitles() indexOfObject:title]:NSNotFound;
        if(index==NSNotFound)continue;
        UIView *owner=node.superview;
        for(NSUInteger i=0;owner && owner!=content && i<8;i++,owner=owner.superview) {
            if([owner isKindOfClass:UIControl.class]) {
                UIControl *control=(id)owner;
                if((control.allControlEvents & (UIControlEventTouchUpInside|UIControlEventPrimaryActionTriggered)) ||
                    HGATextTabAction(control))
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
    self.systemItemSignature=nil;self.systemFeedItems=nil;
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
    BOOL darkDock=self.capsule.overrideUserInterfaceStyle==UIUserInterfaceStyleDark ||
        traits.userInterfaceStyle==UIUserInterfaceStyleDark;
    UITraitCollection *inkTraits=(darkDock && traits.userInterfaceStyle!=UIUserInterfaceStyleDark) ?
        [UITraitCollection traitCollectionWithUserInterfaceStyle:UIUserInterfaceStyleDark] : traits;
    UIColor *selected=[UIColor.labelColor resolvedColorWithTraitCollection:inkTraits];
    CGFloat normalAlpha=traits.accessibilityContrast==UIAccessibilityContrastHigh ? 0.90 : 0.62;
    UIColor *normal=[selected colorWithAlphaComponent:normalAlpha];
    BOOL videoClear=HGASelectedTab(self.hostController)==0 && [HGAppearanceString(@"dock.material") isEqual:@"clear"];
    if(videoClear) {
        selected=UIColor.whiteColor;
        normal=[UIColor.whiteColor colorWithAlphaComponent:traits.accessibilityContrast==UIAccessibilityContrastHigh ? 0.94 : 0.78];
    }
    UIColor *edge=videoClear ? [UIColor colorWithWhite:0 alpha:0.78] : nil;
    NSMutableArray<NSNumber *> *enabled=[NSMutableArray arrayWithCapacity:keys.count];
    for(NSString *key in keys)[enabled addObject:@([(UIControl *)[self.targets objectForKey:key] isEnabled])];
    BOOL separateFeedCaptions=[self usesFeedMotion] && [content isEqual:@"both"];
    NSArray *signature=@[keys,content,@(size),selected,normal,enabled,@(videoClear),@(separateFeedCaptions)];
    if([self.systemItemSignature isEqualToArray:signature])return NO;
    self.systemTabBar.tintColor=selected;
    self.systemTabBar.unselectedItemTintColor=normal;
    BOOL replace=![self.systemItemSignature.firstObject isEqual:keys];
    BOOL metricsChanged=replace || ![self.systemItemSignature[1] isEqual:content] || ![self.systemItemSignature[2] isEqual:@(size)] ||
        (self.systemItemSignature.count>7 && ![self.systemItemSignature[7] isEqual:@(separateFeedCaptions)]);
    NSMutableArray *items=[NSMutableArray arrayWithCapacity:keys.count];
    for(NSUInteger slot=0;slot<keys.count;slot++) {
        NSString *key=keys[slot];
        NSUInteger index=[HGATabIDs() indexOfObject:key];
        NSString *label=HGATabTitles()[index];
        NSString *symbol=[content isEqual:@"text"] ? nil : HGATabSymbols()[index];
        NSString *caption=([content isEqual:@"icon"] || separateFeedCaptions) ? nil : label;
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

static NSMutableDictionary<NSString *,NSNumber *> *HGASystemPlatterKVCProbe;

- (NSString *)applySystemMaterial:(NSString *)style {
    if(@available(iOS 26.0,*)) {
        UIView *surface=self.systemPlatter;
        if(!surface)return @"factory-glass";
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
            Ivar ivar=class_getInstanceVariable(object_getClass(surface),"_glassEffect") ?:
                class_getInstanceVariable(object_getClass(surface),"glassEffect");
            const char *type=ivar ? ivar_getTypeEncoding(ivar) : NULL;
            if(type && *type=='@') {
                id effect=object_getIvar(surface,ivar);
                if([effect isKindOfClass:UIVisualEffect.class] || (!effect && !strcmp(type,"@\"UIGlassEffect\""))) {
                    slot=ivar;current=effect;
                }
            }
        }
        BOOL hasKVC=NO;
        if(!backing && !hasSetter && !slot) {
            if(!HGASystemPlatterKVCProbe)HGASystemPlatterKVCProbe=[NSMutableDictionary dictionary];
            NSString *className=NSStringFromClass(object_getClass(surface));
            NSNumber *probed=className ? HGASystemPlatterKVCProbe[className] : nil;
            if(!probed || probed.boolValue) {
                @try {
                    id kvcEffect=[surface valueForKey:@"glassEffect"];
                    if([kvcEffect isKindOfClass:UIVisualEffect.class]) {
                        current=kvcEffect;
                        hasKVC=YES;
                        if(className)HGASystemPlatterKVCProbe[className]=@YES;
                    } else if(!probed && className) {
                        HGASystemPlatterKVCProbe[className]=@NO;
                    }
                } @catch (__unused NSException *exception) {
                    if(className)HGASystemPlatterKVCProbe[className]=@NO;
                }
            }
        }
        if(!backing && !hasSetter && !slot && !hasKVC) {
            // The item platter on iOS 27 can have a separate backing view
            // directly under the tab bar or nested inside the platter itself.
            // Match only full-platter effects; never replace the moving selection lens.
            for(UIView *child in self.systemTabBar.subviews) {
                if(![child isKindOfClass:UIVisualEffectView.class] || child.hidden || child.alpha<0.05 ||
                    child.bounds.size.height<=10 || child.bounds.size.width<2*child.bounds.size.height)continue;
                UIVisualEffectView *candidate=(id)child;
                if(candidate.effect && (!backing || child.bounds.size.width>backing.bounds.size.width))backing=candidate;
            }
            if(!backing) {
                CGRect platterBounds=surface.bounds;
                for(UIView *child in HGADockSystemSubviews(surface)) {
                    if(![child isKindOfClass:UIVisualEffectView.class] || child.hidden || child.alpha<0.05)continue;
                    UIVisualEffectView *candidate=(id)child;
                    if(!candidate.effect)continue;
                    CGRect inPlatter=[child convertRect:child.bounds toView:surface];
                    if(fabs(CGRectGetWidth(inPlatter)-CGRectGetWidth(platterBounds))<=4.0 &&
                        fabs(CGRectGetHeight(inPlatter)-CGRectGetHeight(platterBounds))<=4.0) {
                        backing=candidate;
                        break;
                    }
                }
            }
            if(!backing)return @"factory-glass";
        }
        if(backing) {
            surface=backing;current=backing.effect;
        }
        // UIKit's platter glassEffect getter returns a copy whose -isEqual:
        // uses pointer identity. Track platter ownership so ordinary layouts
        // do not rewrite glassEffect and interrupt in-flight lens motion.
        BOOL needsInstall=surface!=self.customSystemPlatter || ![style isEqual:self.systemMaterialStyle];
        if(!needsInstall) {
            if(backing)needsInstall=![backing.effect isKindOfClass:UIGlassEffect.class];
            else if(slot)needsInstall=current!=self.installedSystemGlass;
            else needsInstall=![self.installedSystemGlass isKindOfClass:UIGlassEffect.class];
        }
        if(needsInstall) {
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
            }else if(hasSetter) {
                ((void(*)(id,SEL,id))objc_msgSend)(surface,setter,glass);
                id readback=HGReadObjectGetter(surface,"glassEffect");
                self.installedSystemGlass=[readback isKindOfClass:UIGlassEffect.class] ? readback : glass;
                [surface setNeedsLayout];
            }else {
                @try {
                    [surface setValue:glass forKey:@"glassEffect"];
                    id readback=[surface valueForKey:@"glassEffect"];
                    self.installedSystemGlass=[readback isKindOfClass:UIGlassEffect.class] ? readback : glass;
                    [surface setNeedsLayout];
                } @catch (__unused NSException *exception) {
                    NSString *className=NSStringFromClass(object_getClass(surface));
                    if(className)HGASystemPlatterKVCProbe[className]=@NO;
                    return @"factory-glass";
                }
            }
            if(![self.installedSystemGlass isKindOfClass:UIGlassEffect.class])
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
    NSString *provider=self.systemPlatter ? NSStringFromClass(self.systemPlatter.class) : (self.lastSystemPlatterClass ?: @"");
    UIUserInterfaceStyle style=self.capsule.overrideUserInterfaceStyle;
    if(style==UIUserInterfaceStyleUnspecified)style=self.capsule.traitCollection.userInterfaceStyle;
    NSString *interfaceStyle=style==UIUserInterfaceStyleLight ? @"light" : (style==UIUserInterfaceStyleDark ? @"dark" : @"unspecified");
    NSArray *record=@[mode,reason,material,requested,provider,interfaceStyle];
    if([self.systemRendererRecord isEqualToArray:record])return;
    self.systemRendererRecord=record;
    HGAAppearanceMaterial *capsuleGlass=HGAState(self.capsule,NO).glass;
    HGLog(@"appearance.dock-renderer",@{
        @"mode":mode,@"reason":reason,@"material":material,
        @"requestedMaterial":requested,@"platter":provider,@"interfaceStyle":interfaceStyle,
        @"managed":@(self.lastSystemManaged),
        @"requestedWidth":@(round(self.lastSystemPlatterRequestedWidth*10.0)/10.0),
        @"renderedWidth":@(round(self.lastSystemPlatterRenderedWidth*10.0)/10.0),
        @"measuredPlatter":CGRectIsNull(self.lastSystemPlatterMeasured) ? @"" : NSStringFromCGRect(self.lastSystemPlatterMeasured),
        @"materialSignature":capsuleGlass.materialSignature ?: @"",
        @"materialId":capsuleGlass ? [NSString stringWithFormat:@"%p",capsuleGlass] : @""
    });
}

- (BOOL)fitSystemDockInCapsule:(CGRect)requested itemCount:(NSUInteger)count {
    UIView *carrier=[self systemDockCarrier];
    CGRect destination=requested;
    CGSize hostSize=self.bounds.size;
    self.lastSystemPlatterClass=nil;
    self.lastSystemPlatterMeasured=CGRectNull;
    self.lastSystemPlatterRenderedWidth=0;
    self.lastSystemPlatterRequestedWidth=CGRectGetWidth(destination);
    self.lastSystemManaged=self.systemTabController!=nil;
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
    // Use UIKit's natural maximum for stock geometry. An explicit wider
    // request needs our adjustable capsule when the native platter cannot fit.
    CGFloat renderedWidth=CGRectGetWidth(measured)*verticalScale;
    CGFloat difference=CGRectGetWidth(destination)-renderedWidth;
    self.lastSystemPlatterClass=NSStringFromClass(platter.class);
    self.lastSystemPlatterMeasured=measured;
    self.lastSystemPlatterRenderedWidth=renderedWidth;
    CGFloat tolerance=MAX(1.5,2.0/MAX(1,self.traitCollection.displayScale));
    if(renderedWidth<count*44 || difference<-tolerance)return NO;
    if(difference>tolerance) {
        if(fabs(HGAppearanceNumber(@"dock.width")-HGADockDefaultWidth)>0.00001)return NO;
        destination.origin.x+=difference*0.5;
        destination.size.width=renderedWidth;
    }
    CGRect bounds=carrier.bounds;
    carrier.transform=CGAffineTransformMakeScale(verticalScale,verticalScale);
    carrier.center=CGPointMake(CGRectGetMidX(destination)-(CGRectGetMidX(measured)-CGRectGetMidX(bounds))*verticalScale,
        CGRectGetMidY(destination)-(CGRectGetMidY(measured)-CGRectGetMidY(bounds))*verticalScale);
    self.feedSystemBaseScale=verticalScale;
    self.feedSystemCarrierAtRest=carrier.center;
    self.systemProbeFrame=requested;
    self.systemCapsuleFrame=destination;
    CGRect destBounds=(CGRect){CGPointZero,destination.size};
    CGPoint destCenter=CGPointMake(CGRectGetMidX(destination),CGRectGetMidY(destination));
    if(!CGRectEqualToRect(self.capsule.bounds,destBounds))self.capsule.bounds=destBounds;
    if(CGAffineTransformIsIdentity(self.capsule.transform) && !CGPointEqualToPoint(self.capsule.center,destCenter))
        self.capsule.center=destCenter;
    self.systemPlatter=platter;
    return YES;
}

- (BOOL)systemDockCanReceiveTouches:(NSArray<NSString *> *)keys {
    // UIKit's lens changes size and center during selection and attachment.
    // Measure the stable item hit areas instead of a presentation-time thumb.
    CGFloat width=CGRectGetWidth(self.capsule.bounds);
    CGFloat edgeAllowance=MIN(8,MAX(6,width/keys.count*0.10));
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
    NSArray *probe=@[keys,content,style,[NSValue valueWithCGSize:self.bounds.size],
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
        if(carrier.overrideUserInterfaceStyle!=self.capsule.overrideUserInterfaceStyle) {
            carrier.overrideUserInterfaceStyle=self.capsule.overrideUserInterfaceStyle;
            if(@available(iOS 17.0,*))[carrier updateTraitsIfNeeded];
        }
        if(self.systemTabBar!=carrier && self.systemTabBar.overrideUserInterfaceStyle!=self.capsule.overrideUserInterfaceStyle) {
            self.systemTabBar.overrideUserInterfaceStyle=self.capsule.overrideUserInterfaceStyle;
            if(@available(iOS 17.0,*))[self.systemTabBar updateTraitsIfNeeded];
        }
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
            CFRunLoopPerformBlock(CFRunLoopGetMain(),kCFRunLoopCommonModes,^{
                HGAAppearanceDock *dock=weakSelf;
                if(!dock || [dock systemDockCarrier]!=weakCarrier)return;
                dock.systemWarmupPending=NO;dock.systemWarmupComplete=YES;
                if(dock.window)[dock refresh];
            });
            CFRunLoopWakeUp(CFRunLoopGetMain());
            self.systemDockSyncing=NO;
            return NO;
        }
        [self removeSystemDock];
        if(managed)break;
        managed=YES;
    }
    self.systemDockSyncing=NO;
    NSString *material=active ? [self applySystemMaterial:style] : style;
    // If UIKit owns an inaccessible platter, use the single configurable
    // UIGlassEffect capsule so switching between Regular and Clear is effective.
    if(active && [material isEqual:@"factory-glass"]) {
        [self removeSystemDock];
        active=NO;
        failure=@"Platter does not expose the requested material";
        material=style;
    }
    BOOL transientTransition=self.hostController.transitionCoordinator!=nil ||
        self.hostController.presentedViewController!=nil;
    if(!active && !transientTransition)self.failedSystemProbe=probe;
    if(!active && transientTransition) {
        id<UIViewControllerTransitionCoordinator> transition=self.hostController.transitionCoordinator;
        __weak HGAAppearanceDock *weakSelf=self;
        if(transition) {
            [transition animateAlongsideTransition:nil completion:^(id<UIViewControllerTransitionCoordinatorContext> context) {
                dispatch_async(dispatch_get_main_queue(),^{
                    HGAAppearanceDock *dock=weakSelf;
                    if(dock && dock.window && !dock.systemDockActive){dock.failedSystemProbe=nil;[dock refresh];}
                });
            }];
        }
    }
    NSString *mode=active ? (self.systemTabController?@"uikit-controller":@"uikit-tabbar") : @"compatibility";
    NSString *reason=active ? @"UIKit receives the original touch" : failure;
    [self recordDockRenderer:mode reason:reason material:material];
    return active;
}

- (void)tabBar:(UITabBar *)tabBar didSelectItem:(UITabBarItem *)item {
    if(!self.systemDockActive || self.quietSystemTouches || self.systemDockSyncing || self.systemTabController || tabBar!=self.systemTabBar ||
        !item.enabled || ![tabBar.items containsObject:item])return;
    self.systemHostSelection=item.tag;
    [self activateTabAtIndex:item.tag animation:@"uikit-tabbar"];
}

- (void)tabBarController:(UITabBarController *)controller didSelectTab:(UITab *)tab previousTab:(UITab *)previousTab API_AVAILABLE(ios(18.0)) {
    if(!self.systemDockActive || self.quietSystemTouches || self.systemDockSyncing || controller!=self.systemTabController ||
        ![controller.tabs containsObject:tab] || ![tab.userInfo isKindOfClass:NSNumber.class])return;
    NSInteger index=[tab.userInfo integerValue];
    self.systemHostSelection=index;
    [self activateTabAtIndex:index animation:@"uikit-controller"];
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
    for(UIView *v in bar.subviews) {
        if(v==self || [v isDescendantOfView:self] || [self isDescendantOfView:v])continue;
        if([v.accessibilityIdentifier isEqual:@"honggo.tabbar.glass"] ||
            [v isKindOfClass:UIVisualEffectView.class]) {
            [self hideSource:v];
        }
    }
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
        HGADockCollapseMode collapseMode=HGACurrentDockCollapseMode();
        if(collapseMode!=self.appliedCollapseMode) {
            [self resetFeedMotionState];
            self.appliedCollapseMode=collapseMode;
        }
        BOOL gestures=(collapseMode==HGADockCollapseModeLegacy) && !UIAccessibilityIsVoiceOverRunning();
        for(UISwipeGestureRecognizer *g in self.swipes)g.enabled=gestures;
        if(!gestures)self.collapsed=NO;
        if(HGADockUsesFeedMotionMode(collapseMode)) {
            _feedSpring.siriai=(collapseMode==HGADockCollapseModeSiriai) ? 1 : 0;
            NSInteger selected=HGASelectedTab(self.hostController);
            BOOL landscape=bar.window && CGRectGetWidth(bar.window.bounds)>CGRectGetHeight(bar.window.bounds);
            if(self.hidden || landscape || (self.feedLastSelectedTab!=NSNotFound && selected!=self.feedLastSelectedTab)) {
                [self cancelActiveFeedObserverIfNeeded];
                [self setFeedCompact:NO animated:NO];
            }
            self.feedLastSelectedTab=selected;
        } else {
            self.feedLastSelectedTab=NSNotFound;
        }
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
    if(CGAffineTransformIsIdentity(self.expand.transform)) {
        if(!CGRectEqualToRect(self.expand.frame,compact))self.expand.frame=compact;
    } else {
        self.expand.bounds=(CGRect){CGPointZero,compact.size};
        self.expand.center=CGPointMake(CGRectGetMidX(compact),CGRectGetMidY(compact));
    }
    self.expand.hidden=!self.collapsed;
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
    self.feedLightSurface=light;
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
    if(nativePreview && self.quietLensActive) {
        if(self.quietProvider!=self.systemPlatter || !self.quietSelectionSources.count)[self endQuietLens];
        else {
            for(UIView *part in self.quietSelectionSources.keyEnumerator.allObjects) {
                if(!part.hidden || part.superview!=[self.quietSelectionSources objectForKey:part] ||
                    ![part isDescendantOfView:self.systemTabBar]){[self endQuietLens];break;}
            }
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
    if(!self.quietSystemTouches && self.quietDragLens)[self endQuietLens];
    else if(self.quietSystemTouches && self.quietLensActive && self.quietDragLens) {
        if(self.quietProvider!=self.systemPlatter)[self endQuietLens];
        else {
            if(self.quietDragLens.overrideUserInterfaceStyle!=self.capsule.overrideUserInterfaceStyle)
                self.quietDragLens.overrideUserInterfaceStyle=self.capsule.overrideUserInterfaceStyle;
            [self setQuietSourcesHidden:YES];
        }
    }
    UIView *systemCarrier=[self systemDockCarrier];
    if(systemCarrier.accessibilityElementsHidden!=self.quietSystemTouches)
        systemCarrier.accessibilityElementsHidden=self.quietSystemTouches;
    CGRect actual=useSystem ? self.systemCapsuleFrame : expanded;
    CGRect capsuleFrame=self.collapsed ? compact : actual;
    CGRect targetCapsuleBounds=(CGRect){CGPointZero,capsuleFrame.size};
    CGPoint targetCapsuleCenter=CGPointMake(CGRectGetMidX(capsuleFrame),CGRectGetMidY(capsuleFrame));
    BOOL capsuleChanged=!CGSizeEqualToSize(self.capsule.bounds.size,targetCapsuleBounds.size) ||
        (![self usesFeedMotion] && !CGPointEqualToPoint(self.capsule.center,targetCapsuleCenter)) ||
        ([self usesFeedMotion] && !CGRectEqualToRect(self.feedCapsuleAtRest,actual));
    if(!CGRectEqualToRect(self.capsule.bounds,targetCapsuleBounds))self.capsule.bounds=targetCapsuleBounds;
    if(CGAffineTransformIsIdentity(self.capsule.transform) && !CGPointEqualToPoint(self.capsule.center,targetCapsuleCenter))
        self.capsule.center=targetCapsuleCenter;
    CGRect extraFrame=CGRectMake(CGRectGetMaxX(actual)+HGADockAccessoryGap,rowTop+(rowHeight-circle)/2,circle,circle);
    if(CGAffineTransformIsIdentity(self.extra.transform)) {
        if(!CGRectEqualToRect(self.extra.frame,extraFrame))self.extra.frame=extraFrame;
    } else {
        CGRect extraBounds=(CGRect){CGPointZero,extraFrame.size};
        if(!CGRectEqualToRect(self.extra.bounds,extraBounds))self.extra.bounds=extraBounds;
        if(![self usesFeedMotion])self.extra.center=CGPointMake(CGRectGetMidX(extraFrame),CGRectGetMidY(extraFrame));
    }
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
    UIUserInterfaceStyle buttonStyle=(videoClear || (translucent && surfaceStyle!=UIUserInterfaceStyleUnspecified)) ?
        surfaceStyle : UIUserInterfaceStyleUnspecified;
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
    self.dockTouch.enabled=(!useSystem || self.quietSystemTouches || self.collapsed) && !self.hidden && !UIAccessibilityIsVoiceOverRunning();
    self.capsule.layer.cornerRadius=radius;
    self.capsule.layer.borderWidth=!flat && !useSystem && translucent && !classicBlur && !nativeGlass ? 0.5 : 0;
    self.capsule.layer.borderColor=[UIColor colorWithWhite:light?0:1 alpha:0.14].CGColor;
    self.extra.layer.cornerRadius=circle/2;
    self.expand.layer.cornerRadius=22;
    self.extra.layer.borderWidth=!flat && translucent && !classicBlur && !nativeGlass ? 0.5 : 0;
    self.extra.layer.borderColor=[UIColor colorWithWhite:light?0:1 alpha:0.14].CGColor;
    BOOL darkSurface=surfaceStyle==UIUserInterfaceStyleDark ||
        self.capsule.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark;
    BOOL nativeSelection=liquidSelection && !reduced && HGNativeGlassConfigured();
    // The native resting thumb is recessed into the rail. Refract only while tracking.
    UIColor *selectionBg=nativeSelection ? UIColor.secondarySystemFillColor : HGADockSelectionColor(self.capsule.traitCollection,flat || !liquidSelection);
    UIColor *selectionEdge=[UIColor colorWithWhite:darkSurface?1:0 alpha:darkSurface?0.23:0.11];
    self.restingSelectionColor=selectionBg;
    BOOL opticalLensActive=(self.pressedTab || self.selectionDragging || self.selectionSettling) &&
        self.dragLens && !self.dragLens.hidden && [self.dragLens.effect isKindOfClass:NSClassFromString(@"UIGlassEffect")];
    if(!opticalLensActive)self.selection.backgroundColor=selectionBg;
    self.selection.layer.borderWidth=liquidSelection && !nativeSelection ? 0.5 : 0;
    self.selectionHighlight.backgroundColor=nativeSelection ?
        [UIColor colorWithWhite:darkSurface?1:0 alpha:darkSurface?0.10:0.06] : [UIColor colorWithWhite:1 alpha:0.16];
    self.selectionHighlight.layer.borderWidth=nativeSelection ? 0 : 1;
    self.selection.layer.borderColor=selectionEdge.CGColor;
    CGRect selectedFrame=CGRectNull;
    NSInteger flatIndex=UISegmentedControlNoSegment;
    CGFloat slotWidth=expanded.size.width/visible.count;
    for(NSString *key in self.buttons) {
        HGADockTabButton *b=(id)self.buttons[key];NSUInteger index=[HGATabIDs() indexOfObject:key];
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
            b.visualSignature=nil;
            b.frame=position<self.systemHitSlots.count ? self.systemHitSlots[position].CGRectValue : CGRectZero;
            if(b.currentTitle)[b setTitle:nil forState:UIControlStateNormal];
            if(b.currentImage)[b setImage:nil forState:UIControlStateNormal];
            continue;
        }
        CGRect slotFrame=CGRectMake(position*slotWidth,0,slotWidth,height);
        if(!CGRectEqualToRect(b.frame,slotFrame))b.frame=slotFrame;
        CGRect thumb=HGADockSelectionFrame(CGRectGetMidX(b.frame),expanded.size.width,height,slotWidth,NO);
        CGFloat nextOffset=flat ? 0 : CGRectGetMidX(thumb)-CGRectGetMidX(b.frame);
        if(fabs(b.faceOffsetX-nextOffset)>0.001){b.faceOffsetX=nextOffset;[b setNeedsLayout];}
        UIColor *color=active ? UIColor.labelColor :
            (translucent ? [UIColor.labelColor colorWithAlphaComponent:unselectedOpacity] : UIColor.secondaryLabelColor);
        if(videoClear || (darkSurface && translucent))
            color=active ? UIColor.whiteColor : [UIColor.whiteColor colorWithAlphaComponent:unselectedOpacity];
        NSString *symbol=HGATabSymbols()[index];
        NSString *title=[content isEqual:@"icon"]?nil:HGATabTitles()[index];
        UIFont *font=[UIFont systemFontOfSize:[content isEqual:@"text"]?15:11 weight:active?UIFontWeightSemibold:UIFontWeightRegular];
        if(HGAppearanceFlag(@"fonts.enabled"))font=HGAConfiguredFont(font);
        CGFloat iconHeight=title.length ? height-11-font.lineHeight : height*0.34;
        CGFloat iconSize=MAX(12,MIN(requestedIcon,MIN(iconHeight,slotWidth-16)));
        // A host layout during tracking must not replace the image view that
        // is being animated and sampled by the moving lens.
        if((!self.pressedTab && !self.selectionDragging) || (!b.currentImage && !b.currentTitle.length)) {
            NSArray *btnSig=@[symbol,title ?: @"",@(flat || active),@(iconSize),color,font,@(darkSurface),@(videoClear),@(b.enabled),content];
            if(![b.visualSignature isEqualToArray:btnSig] || (!b.currentImage && !b.currentTitle.length)) {
                UIImage *image=[UIImage systemImageNamed:(flat || active) ? [symbol stringByAppendingString:@".fill"] : symbol] ?: [UIImage systemImageNamed:symbol];
                UIImageSymbolConfiguration *symbolConfig=[UIImageSymbolConfiguration configurationWithPointSize:iconSize weight:UIImageSymbolWeightMedium];
                UIImage *glyph=[content isEqual:@"text"] ? nil : [image imageByApplyingSymbolConfiguration:symbolConfig];
                [b setImage:glyph forState:UIControlStateNormal];
                [b setTitle:title forState:UIControlStateNormal];
                [b setTitleColor:color forState:UIControlStateNormal];
                b.tintColor=color;b.titleLabel.font=font;
                b.alpha=b.enabled ? 1 : 0.4;
                b.visualSignature=btnSig;
                [b setNeedsLayout];
            }
        }
        if(active) {
            flatIndex=(NSInteger)position;
            selectedFrame=thumb;
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
    NSString *action=HGAppearanceString(@"dock.action");
    NSUInteger detachedIndex=[HGATabIDs() indexOfObject:action];
    UIControl *detachedSource=detachedIndex!=NSNotFound ? [self.targets objectForKey:action] : nil;
    BOOL detachedSelected=detachedIndex!=NSNotFound &&
        (selected!=NSNotFound ? selected==(NSInteger)detachedIndex : detachedSource.selected);
    NSString *baseSymbol=[action isEqual:@"settings"]?@"slider.horizontal.3":([action isEqual:@"search"]?@"magnifyingglass":HGATabSymbols()[detachedIndex]);
    UIColor *accessoryColor=detachedSelected ? UIColor.labelColor :
        (detachedIndex!=NSNotFound && translucent ? [UIColor.labelColor colorWithAlphaComponent:unselectedOpacity] : UIColor.labelColor);
    if(videoClear || (darkSurface && translucent))
        accessoryColor=detachedSelected || detachedIndex==NSNotFound ? UIColor.whiteColor : [UIColor.whiteColor colorWithAlphaComponent:unselectedOpacity];
    UIFont *accessoryFont=[UIFont systemFontOfSize:11];
    CGFloat extraIconSize=MIN(requestedIcon,circle*0.34);
    NSArray *extraSig=@[baseSymbol ?: @"",@(detachedSelected || (flat && detachedIndex!=NSNotFound)),@(extraIconSize),accessoryColor,@(darkSurface),@(videoClear),@(self.extraButton.enabled)];
    if(![self.extraButtonSignature isEqualToArray:extraSig] || !self.extraButton.configuration) {
        UIImage *extraImage=detachedSelected || (flat && detachedIndex!=NSNotFound) ?
            ([UIImage systemImageNamed:[baseSymbol stringByAppendingString:@".fill"]] ?: [UIImage systemImageNamed:baseSymbol]) : [UIImage systemImageNamed:baseSymbol];
        HGADockConfigureButton(self.extraButton,nil,extraImage,extraIconSize,accessoryFont,accessoryColor);
        self.extraButtonSignature=extraSig;
    }
    CGFloat expandIconSize=MIN(requestedIcon,22);
    NSArray *expandSig=@[@(expandIconSize),accessoryColor,@(darkSurface),@(videoClear),@(self.expandButton.enabled)];
    if(![self.expandButtonSignature isEqualToArray:expandSig] || !self.expandButton.configuration) {
        HGADockConfigureButton(self.expandButton,nil,[UIImage systemImageNamed:@"ellipsis"],expandIconSize,accessoryFont,accessoryColor);
        self.expandButtonSignature=expandSig;
    }
    HGADockSetInkEdge(self.extraButton,inkEdge);HGADockSetInkEdge(self.expandButton,inkEdge);
    NSInteger detachedSegment=detachedSelected ? 0 : UISegmentedControlNoSegment;
    if(extraTrack && extraTrack.selectedSegmentIndex!=detachedSegment)extraTrack.selectedSegmentIndex=detachedSegment;
    self.extraSelection.hidden=flat || !detachedSelected || self.extra.hidden;
    if(!self.extraSelection.hidden) {
        CGFloat side=CGRectGetHeight(self.extra.bounds);
        CGFloat inset=HGADockRestingInset(side);
        CGRect inner=CGRectInset(self.extra.bounds,inset,inset);
        self.extraSelection.frame=inner;
        self.extraSelection.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
        self.extraSelection.layer.cornerRadius=MIN(inner.size.width,inner.size.height)/2;
        self.extraSelection.backgroundColor=selectionBg;
        self.extraSelection.layer.borderWidth=liquidSelection && !nativeSelection ? 0.5 : 0;
        self.extraSelection.layer.borderColor=selectionEdge.CGColor;
    }
    self.extraButton.selected=detachedSelected;
    self.extraButton.accessibilityLabel=[action isEqual:@"settings"]?@"Honggo 外观设置":([action isEqual:@"search"]?@"搜索":HGATabTitles()[detachedIndex]);
    self.extraButton.accessibilityTraits=UIAccessibilityTraitButton | (detachedSelected?UIAccessibilityTraitSelected:0);
    if([self usesFeedMotion] && !self.collapsed) {
        self.feedCapsuleAtRest=actual;
        self.feedExtraAtRest=extra ? extraFrame : CGRectZero;
        self.feedCompactCapsuleWidth=DKFeedCompactCapsuleWidth(CGRectGetWidth(actual),
            extra ? CGRectGetWidth(extraFrame) : 0.0,extra ? HGADockAccessoryGap : 0.0,(unsigned)MAX(1u,(unsigned)visible.count));
        if(self.appliedCollapseMode==HGADockCollapseModeSiriai) {
            int startLink=0,scheduleFade=0;
            DKFeedSpringUpdateHideLine(&_feedSpring,HGAppearanceFlag(@"dock.siriaiHideLine") ? 1 : 0,
                (self.feedDisplayLink && !self.feedDisplayLink.paused) ? 1 : 0,&startLink,&scheduleFade);
            if(startLink)[self startFeedDisplayLink];
            if(scheduleFade)[self scheduleSiriaiFade];
        } else if(!_feedSpring.following && !self.feedDisplayLink) {
            _feedSpring.progress=DKFeedSpringRestTarget(&_feedSpring,_feedSpring.compact);
        }
        if(useSystem)[self prepareSystemFeedItems:visible content:content selected:selected videoClear:videoClear];
        else self.systemFeedItems=nil;
        [self applyFeedMotion:_feedSpring.progress];
    } else {
        self.feedCapsuleAtRest=CGRectZero;
        self.feedExtraAtRest=CGRectZero;
        self.systemFeedItems=nil;
    }
    CGRect occupied=self.collapsed?compact:(extra?CGRectUnion(actual,extraFrame):actual);
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
        [self cancelActiveFeedObserverIfNeeded];
        [self resetFeedMotionState];
        self.appliedCollapseMode=HGADockCollapseModeOff;
        self.feedLastSelectedTab=NSNotFound;
        [self removeSystemDock];
        self.failedSystemProbe=nil;
        [self clearSelectionDrag];
        self.capsule.transform=CGAffineTransformIdentity;
        self.extra.transform=CGAffineTransformIdentity;
        self.expand.transform=CGAffineTransformIdentity;
        self.extraButton.imageView.transform=CGAffineTransformIdentity;
        self.expandButton.imageView.transform=CGAffineTransformIdentity;
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
- (void)prepareHaptics {
    if(!HGAppearanceFlag(@"dock.haptics"))return;
    if(!self.selectionFeedback)self.selectionFeedback=[UISelectionFeedbackGenerator new];
    [self.selectionFeedback prepare];
}
- (void)feedback {
    if(!HGAppearanceFlag(@"dock.haptics"))return;
    if(!self.selectionFeedback)self.selectionFeedback=[UISelectionFeedbackGenerator new];
    [self.selectionFeedback selectionChanged];
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
    HGADockTextTabAction textAction=HGATextTabAction(target);
    UIControlEvents events=target.allControlEvents & (UIControlEventTouchUpInside|UIControlEventPrimaryActionTriggered);
    if(!textAction && !events)return;
    HGLog(@"navigation.dock", @{@"tab":key, @"phase":@"enter", @"mode":animation});
    // Native selection synchronously attaches the new page and its player. Its
    // layout callbacks must not each rebuild the same dock during that action.
    self.deliveringTabAction=YES;
    @try {
        if(textAction)textAction(target);
        else [target sendActionsForControlEvents:(events & UIControlEventTouchUpInside) ?
            UIControlEventTouchUpInside : UIControlEventPrimaryActionTriggered];
    }
    @finally { self.deliveringTabAction=NO; }
    HGLog(@"navigation.dock", @{@"tab":key, @"phase":@"return", @"mode":animation});
    if(![animation hasPrefix:@"uikit-"] || !HGADockOriginalImpact)[self feedback];
    if(!self.systemDockActive)self.failedSystemProbe=nil;
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
- (void)updateDragLensCornersForRadius:(CGFloat)radius {
    if(!self.dragLens)return;
    if(fabs(self.dragLens.layer.cornerRadius-radius)>0.01)self.dragLens.layer.cornerRadius=radius;
    if(@available(iOS 26.0,*)) {
        CGFloat capsuleHeight=CGRectGetHeight(self.capsule.bounds);
        BOOL pill=capsuleHeight>0 && self.capsule.layer.cornerRadius>=capsuleHeight*0.5-0.5;
        if(pill) {
            if(!self.dragLensIsCapsule) {
                self.dragLens.cornerConfiguration=UICornerConfiguration.capsuleConfiguration;
                self.dragLensIsCapsule=YES;
                self.dragLensCornerRadius=radius;
            }
        } else if(self.dragLensIsCapsule || fabs(self.dragLensCornerRadius-radius)>0.25) {
            self.dragLens.cornerConfiguration=[UICornerConfiguration configurationWithUniformRadius:[UICornerRadius fixedRadius:radius]];
            self.dragLensIsCapsule=NO;
            self.dragLensCornerRadius=radius;
        }
    }
}
- (void)placeSelection:(CGRect)frame {
    CGRect targetBounds=(CGRect){CGPointZero,frame.size};
    CGPoint targetCenter=CGPointMake(CGRectGetMidX(frame),CGRectGetMidY(frame));
    if(!CGRectEqualToRect(self.selection.bounds,targetBounds))self.selection.bounds=targetBounds;
    if(!CGPointEqualToPoint(self.selection.center,targetCenter))self.selection.center=targetCenter;
    CGFloat radius=MIN(self.capsule.layer.cornerRadius,MIN(frame.size.width,frame.size.height)/2);
    if(self.selection.layer.cornerRadius!=radius)self.selection.layer.cornerRadius=radius;
    if(self.dragLens) {
        if(!CGRectEqualToRect(self.dragLens.bounds,targetBounds))self.dragLens.bounds=targetBounds;
        if(!CGPointEqualToPoint(self.dragLens.center,targetCenter))self.dragLens.center=targetCenter;
        [self updateDragLensCornersForRadius:radius];
    }
    if(!CGRectEqualToRect(self.selectionHighlight.bounds,targetBounds))self.selectionHighlight.bounds=targetBounds;
    CGPoint highlightCenter=CGPointMake(targetBounds.size.width*0.5,targetBounds.size.height*0.5);
    if(!CGPointEqualToPoint(self.selectionHighlight.center,highlightCenter))self.selectionHighlight.center=highlightCenter;
    if(self.selectionHighlight.layer.cornerRadius!=radius)self.selectionHighlight.layer.cornerRadius=radius;
}
- (void)placeQuietLensFrame:(CGRect)frame {
    if(!self.quietDragLens)return;
    CGRect targetBounds=(CGRect){CGPointZero,frame.size};
    CGPoint targetCenter=CGPointMake(CGRectGetMidX(frame),CGRectGetMidY(frame));
    if(!CGRectEqualToRect(self.quietDragLens.bounds,targetBounds))self.quietDragLens.bounds=targetBounds;
    if(!CGPointEqualToPoint(self.quietDragLens.center,targetCenter))self.quietDragLens.center=targetCenter;
    CGFloat radius=MIN(frame.size.width,frame.size.height)*0.5;
    if(fabs(self.quietDragLens.layer.cornerRadius-radius)>0.01)self.quietDragLens.layer.cornerRadius=radius;
}
- (void)setQuietSourcesHidden:(BOOL)hidden {
    if(!self.quietSelectionSources || self.quietProvider!=self.systemPlatter)return;
    for(UIView *part in self.quietSelectionSources.keyEnumerator.allObjects) {
        if(part.superview==[self.quietSelectionSources objectForKey:part] &&
            [part isDescendantOfView:self.systemTabBar] && part.hidden!=hidden) {
            part.hidden=hidden;
        }
    }
}
- (void)hideQuietLens {
    self.quietLensActive=NO;
    [self.quietDragLens.layer removeAllAnimations];
    self.quietDragLens.transform=CGAffineTransformIdentity;
    self.quietDragLens.alpha=0;
    self.quietDragLens.hidden=YES;
    [self setQuietSourcesHidden:NO];
}
- (void)endQuietLens {
    [self hideQuietLens];
    self.quietDragLens.effect=nil;[self.quietDragLens removeFromSuperview];self.quietDragLens=nil;
    self.quietSelectionSources=nil;self.quietProvider=nil;
}
- (UIButton *)buttonForTabIndex:(NSInteger)index {
    if(index<0 || index>=(NSInteger)HGATabIDs().count)return nil;
    UIButton *b=self.buttons[HGATabIDs()[(NSUInteger)index]];
    return (b && !b.hidden && b.enabled) ? b : nil;
}
- (CGRect)quietFrameAtX:(CGFloat)x button:(UIButton *)button lifted:(BOOL)lifted {
    CGSize size=self.capsule.bounds.size;
    CGRect local=HGADockSelectionFrame(x,size.width,size.height,button.bounds.size.width,lifted);
    return [self.capsule convertRect:local toView:self];
}
- (BOOL)ensureQuietLensPrepared {
    if(UIAccessibilityIsReduceMotionEnabled() || !self.systemPlatter || !self.systemTabBar) {
        if(self.quietDragLens)[self endQuietLens];
        return NO;
    }
    if(self.quietDragLens && self.quietProvider==self.systemPlatter &&
        self.quietDragLens.superview==self && self.quietSelectionSources.count) {
        BOOL valid=YES;
        for(UIView *part in self.quietSelectionSources.keyEnumerator.allObjects) {
            if(part.superview!=[self.quietSelectionSources objectForKey:part] ||
                ![part isDescendantOfView:self.systemTabBar]){valid=NO;break;}
        }
        if(valid) {
            UIView *carrier=[self systemDockCarrier];
            NSArray<UIView *> *subs=self.subviews;
            NSUInteger carrierIdx=[subs indexOfObjectIdenticalTo:carrier];
            NSUInteger lensIdx=[subs indexOfObjectIdenticalTo:self.quietDragLens];
            if(carrierIdx!=NSNotFound && (lensIdx==NSNotFound || lensIdx<carrierIdx))
                [self insertSubview:self.quietDragLens aboveSubview:carrier];
            return YES;
        }
        [self endQuietLens];
    }
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
            UIVisualEffectView *lens=HGADockMakeInteractiveLens(self);
            lens.accessibilityIdentifier=@"honggo.appearance.dock.quiet-lens";
            lens.accessibilityElementsHidden=YES;lens.userInteractionEnabled=NO;
            lens.overrideUserInterfaceStyle=self.capsule.overrideUserInterfaceStyle;
            lens.cornerConfiguration=UICornerConfiguration.capsuleConfiguration;
            lens.hidden=YES;lens.alpha=0;
            [self insertSubview:lens aboveSubview:[self systemDockCarrier]];
            UIGlassEffect *glass=[UIGlassEffect effectWithStyle:UIGlassEffectStyleClear];
            glass.interactive=NO;lens.effect=glass;
            self.quietDragLens=lens;self.quietProvider=self.systemPlatter;
        }
    }
    return self.quietDragLens!=nil;
}
- (BOOL)placeQuietLensAtX:(CGFloat)x button:(UIButton *)button {
    if(!button || ![self ensureQuietLensPrepared]) {
        if(self.quietLensActive)[self hideQuietLens];
        return NO;
    }
    CGRect frame=[self quietFrameAtX:x button:button lifted:YES];
    if(self.quietDragLens.overrideUserInterfaceStyle!=self.capsule.overrideUserInterfaceStyle)
        self.quietDragLens.overrideUserInterfaceStyle=self.capsule.overrideUserInterfaceStyle;
    if(!self.quietLensActive) {
        self.quietLensActive=YES;
        self.quietDragLens.alpha=1;
        self.quietDragLens.hidden=NO;
        [self setQuietSourcesHidden:YES];
    }
    [self placeQuietLensFrame:frame];
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
        if(destination) {
            CGFloat clampedX=MIN(last,MAX(first,x));
            if(self.selectionDragging) {
                CFTimeInterval now=CACurrentMediaTime();
                CFTimeInterval dt=self.dragLastTimestamp>0 ? now-self.dragLastTimestamp : 0;
                if(dt>0.001 && dt<0.12) {
                    CGFloat vx=(clampedX-self.dragLastX)/dt;
                    self.dragVelocityX=self.dragVelocityX*0.55+vx*0.45;
                    self.dragPeakVelocity=MAX(self.dragPeakVelocity,fabs(self.dragVelocityX));
                } else if(dt>=0.12) {
                    self.dragVelocityX=0;
                }
                self.dragLastX=clampedX;
                self.dragLastTimestamp=now;
                if(destination!=self.dragHoveredTab) {
                    self.dragHoveredTab=destination;
                    [self syncSystemSelection:destination.tag force:NO];
                }
            }
            if([self placeQuietLensAtX:clampedX button:destination]) {
                if(self.selectionDragging && !UIAccessibilityIsReduceMotionEnabled()) {
                    CGFloat norm=MIN(1.0,fabs(self.dragVelocityX)/1400.0);
                    CGAffineTransform stretch=norm>0.02 ?
                        CGAffineTransformMakeScale(1.0+norm*0.038,1.0-norm*0.024) : CGAffineTransformIdentity;
                    if(!CGAffineTransformEqualToTransform(self.quietDragLens.transform,stretch))
                        self.quietDragLens.transform=stretch;
                }
            } else {
                [self syncSystemSelection:destination.tag force:NO];
            }
        }
        return destination;
    }
    self.selection.hidden=!destination;
    if(destination) {
        CGFloat clampedX=MIN(last,MAX(first,x));
        self.selectionDragX=x;
        if(self.selectionDragging) {
            CFTimeInterval now=CACurrentMediaTime();
            CFTimeInterval dt=self.dragLastTimestamp>0 ? now-self.dragLastTimestamp : 0;
            if(dt>0.001 && dt<0.12) {
                CGFloat vx=(clampedX-self.dragLastX)/dt;
                self.dragVelocityX=self.dragVelocityX*0.55+vx*0.45;
                self.dragPeakVelocity=MAX(self.dragPeakVelocity,fabs(self.dragVelocityX));
            } else if(dt>=0.12) {
                self.dragVelocityX=0;
            }
            self.dragLastX=clampedX;
            self.dragLastTimestamp=now;
            if(destination!=self.dragHoveredTab) {
                if(self.dragHoveredTab && HGAppearanceFlag(@"dock.haptics")) {
                    [self feedback];
                    [self prepareHaptics];
                }
                self.dragHoveredTab=destination;
            }
            if(self.dragLens && !self.dragLens.hidden && !UIAccessibilityIsReduceMotionEnabled()) {
                CGFloat norm=MIN(1.0,fabs(self.dragVelocityX)/1400.0);
                CGAffineTransform stretch=norm>0.02 ?
                    CGAffineTransformMakeScale(1.0+norm*0.038,1.0-norm*0.024) : CGAffineTransformIdentity;
                if(!CGAffineTransformEqualToTransform(self.dragLens.transform,stretch))
                    self.dragLens.transform=stretch;
            }
        }
        // Move a single selection surface while the finger is down.
        if(self.flatSegments && self.flatSegments.selectedSegmentIndex!=UISegmentedControlNoSegment)
            self.flatSegments.selectedSegmentIndex=UISegmentedControlNoSegment;
        CGFloat height=self.capsule.bounds.size.height;
        CGRect frame=HGADockSelectionFrame(clampedX,self.capsule.bounds.size.width,height,
            destination.bounds.size.width,HGADockLiquidSelection() && !UIAccessibilityIsReduceMotionEnabled());
        [self placeSelection:frame];
    }
    return destination;
}
- (void)clearSelectionDrag {
    [self hideQuietLens];
    self.selectionGeneration++;
    self.pressedTab=nil;
    self.selectionDragging=NO;
    self.selectionSettling=NO;
    self.motionMaterialPresented=NO;
    self.dragHoveredTab=nil;
    self.dragVelocityX=0;
    self.dragPeakVelocity=0;
    self.dragLastTimestamp=0;
    self.motionStartTimestamp=0;
    [self.selection.layer removeAllAnimations];
    [self.dragLens.layer removeAllAnimations];
    [self.selectionHighlight.layer removeAllAnimations];
    self.dragLens.transform=CGAffineTransformIdentity;
    self.dragLens.alpha=1;
    self.dragLens.hidden=YES;
    if(self.restingSelectionColor)self.selection.backgroundColor=self.restingSelectionColor;
    self.selectionHighlight.alpha=0;
    self.motionGlyph.imageView.transform=CGAffineTransformIdentity;
    self.motionGlyph.titleLabel.transform=CGAffineTransformIdentity;
    self.motionGlyph=nil;
    if([self usesFeedMotion] && (_feedSpring.progress>0.0 || _feedSpring.overtravel!=0.0))
        [self applyFeedMotion:_feedSpring.progress];
}
- (void)prepareSelectionLens {
    // Never stack a second UIGlassEffect lens inside a glass capsule: nested
    // Clear/Regular backdrop layers double-refract background covers and warp glyphs.
    // Reserve the moving optical lens strictly for the non-glass flat segment track.
    if(!self.flatSegments || !HGADockLiquidSelection() || !HGNativeGlassConfigured() ||
        UIAccessibilityIsReduceTransparencyEnabled()) {
        [self.dragLens removeFromSuperview];self.dragLens=nil;
        self.dragLensIsCapsule=NO;self.dragLensCornerRadius=0;
        return;
    }
    if(@available(iOS 26.0,*)) {
        if([self.dragLens.effect isKindOfClass:UIGlassEffect.class] &&
            ((UIGlassEffect *)self.dragLens.effect).interactive!=HGAppearanceFlag(@"dock.haptics")) {
            [self.dragLens removeFromSuperview];self.dragLens=nil;
        }
    }
    if(self.dragLens) {
        if(self.dragLens.overrideUserInterfaceStyle!=self.capsule.overrideUserInterfaceStyle) {
            self.dragLens.overrideUserInterfaceStyle=self.capsule.overrideUserInterfaceStyle;
            if(@available(iOS 17.0,*))[self.dragLens updateTraitsIfNeeded];
        }
        return;
    }
    if(CGRectIsEmpty(self.selection.bounds))return;
    if(@available(iOS 26.0,*)) {
        UIGlassEffect *effect=[UIGlassEffect effectWithStyle:UIGlassEffectStyleClear];
        effect.interactive=HGAppearanceFlag(@"dock.haptics");
        UIVisualEffectView *lens=HGADockMakeInteractiveLens(self.capsule);
        lens.accessibilityIdentifier=@"honggo.appearance.dock.drag-lens";
        lens.overrideUserInterfaceStyle=self.capsule.overrideUserInterfaceStyle;
        lens.bounds=(CGRect){CGPointZero,self.selection.bounds.size};
        lens.center=self.selection.center;
        lens.layer.cornerCurve=kCACornerCurveContinuous;
        lens.userInteractionEnabled=NO;lens.accessibilityElementsHidden=YES;
        lens.hidden=YES;
        self.dragLens=lens;
        self.dragLensIsCapsule=NO;
        self.dragLensCornerRadius=-1;
        [self updateDragLensCornersForRadius:self.selection.layer.cornerRadius];
        // Sample the assembled tab face during a press, including the symbol
        // and caption beneath the moving optical surface.
        [self.capsule addSubview:lens];
        lens.effect=effect;
    }
}
- (void)showSelectionMotionMaterial {
    if(!HGADockLiquidSelection())return;
    [self prepareSelectionLens];
    self.motionMaterialPresented=YES;
    if(self.dragLens) {
        [self.capsule bringSubviewToFront:self.dragLens];
        [self.dragLens.layer removeAnimationForKey:@"opacity"];
        [self.selection.layer removeAnimationForKey:@"backgroundColor"];
        self.dragLens.alpha=1;
        self.dragLens.hidden=NO;
        if([self.dragLens.effect isKindOfClass:NSClassFromString(@"UIGlassEffect")])
            self.selection.backgroundColor=UIColor.clearColor;
    }
    self.selectionHighlight.alpha=1;
}
- (void)animateMotionGlyph:(UIButton *)glyph {
    if(glyph==self.motionGlyph || (self.dragLens && !self.dragLens.hidden) || !HGADockLiquidSelection() ||
        UIAccessibilityIsReduceMotionEnabled())return;
    UIButton *previous=self.motionGlyph;
    self.motionGlyph=glyph;
    if([self usesFeedMotion] && (_feedSpring.progress>0.0 || _feedSpring.overtravel!=0.0)) {
        [self applyFeedMotion:_feedSpring.progress];
        return;
    }
    [UIView animateWithDuration:glyph?0.18:0.24 delay:0 usingSpringWithDamping:0.82 initialSpringVelocity:0
        options:UIViewAnimationOptionBeginFromCurrentState|UIViewAnimationOptionAllowUserInteraction animations:^{
            previous.imageView.transform=CGAffineTransformIdentity;
            previous.titleLabel.transform=CGAffineTransformIdentity;
            glyph.imageView.transform=CGAffineTransformMakeScale(1.14,1.14);
            glyph.titleLabel.transform=CGAffineTransformMakeScale(1.06,1.06);
        } completion:nil];
}
- (NSString *)dockMotionMode {
    if(self.quietSystemTouches)return (self.quietLensActive && self.quietDragLens) ? @"uikit-quiet-lens" : @"uikit-quiet-selection";
    if(UIAccessibilityIsReduceMotionEnabled())return @"reduced-motion";
    if(!HGADockLiquidSelection())return @"plain-selection";
    Class glass=NSClassFromString(@"UIGlassEffect");
    return self.motionMaterialPresented && !self.dragLens.hidden && [self.dragLens.effect isKindOfClass:glass] ?
        @"glass-morph" : @"highlight-spring";
}
- (void)pressTab:(UIButton *)button {
    if(self.selectionDragging || self.pressedTab==button || !button.enabled || button.hidden)return;
    BOOL wasSettling=self.selectionSettling;
    self.selectionGeneration++;
    self.selectionSettling=NO;
    self.pressedTab=button;
    self.dragHoveredTab=button;
    self.dragVelocityX=0;
    self.dragPeakVelocity=0;
    self.dragLastTimestamp=0;
    self.motionStartTimestamp=CACurrentMediaTime();
    [self prepareHaptics];
    if(self.quietSystemTouches) {
        CGFloat targetX=CGRectGetMidX(button.frame);
        self.selectionDragX=targetX;
        if(!UIAccessibilityIsReduceMotionEnabled() && [self ensureQuietLensPrepared]) {
            CGRect startFrame=CGRectNull;
            if(self.quietLensActive && !self.quietDragLens.hidden &&
                self.quietDragLens.layer.presentationLayer && !CGRectIsEmpty(self.quietDragLens.layer.presentationLayer.bounds)) {
                startFrame=self.quietDragLens.layer.presentationLayer.frame;
            } else {
                UIButton *current=[self buttonForTabIndex:HGASelectedTab(self.hostController)] ?: button;
                startFrame=[self quietFrameAtX:CGRectGetMidX(current.frame) button:current lifted:NO];
            }
            CGRect targetFrame=[self quietFrameAtX:targetX button:button lifted:YES];
            [UIView performWithoutAnimation:^{
                [self.quietDragLens.layer removeAllAnimations];
                self.quietDragLens.transform=CGAffineTransformIdentity;
                self.quietDragLens.overrideUserInterfaceStyle=self.capsule.overrideUserInterfaceStyle;
                [self placeQuietLensFrame:startFrame];
                self.quietDragLens.alpha=1;
                self.quietDragLens.hidden=NO;
                self.quietLensActive=YES;
                [self setQuietSourcesHidden:YES];
            }];
            [self syncSystemSelection:button.tag force:NO];
            [UIView animateWithDuration:0.22 delay:0 usingSpringWithDamping:0.80 initialSpringVelocity:0.8
                options:UIViewAnimationOptionBeginFromCurrentState|UIViewAnimationOptionAllowUserInteraction animations:^{
                    self.quietDragLens.transform=CGAffineTransformIdentity;
                    [self placeQuietLensFrame:targetFrame];
                } completion:nil];
        } else {
            [self syncSystemSelection:button.tag force:NO];
        }
        return;
    }
    CGFloat position=CGRectGetMidX(button.frame);
    if(UIAccessibilityIsReduceMotionEnabled()) {
        [self positionDragSelection:position];
        return;
    }
    if(wasSettling && self.selection.layer.presentationLayer && !CGRectIsEmpty(self.selection.layer.presentationLayer.bounds)) {
        [UIView performWithoutAnimation:^{
            [self placeSelection:self.selection.layer.presentationLayer.frame];
        }];
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
    [UIView animateWithDuration:0.22 delay:0 usingSpringWithDamping:0.82 initialSpringVelocity:0.7
        options:UIViewAnimationOptionBeginFromCurrentState|UIViewAnimationOptionAllowUserInteraction animations:^{
            self.dragLens.transform=CGAffineTransformIdentity;
            [self positionDragSelection:position];
        } completion:nil];
}
- (void)settleSelection {
    CGFloat releaseVelocity=self.dragVelocityX;
    CGFloat peakVelocity=self.dragPeakVelocity;
    CFTimeInterval startTs=self.motionStartTimestamp;
    NSString *motionMode=[self dockMotionMode];
    self.pressedTab=nil;
    self.selectionDragging=NO;
    self.dragHoveredTab=nil;
    self.dragVelocityX=0;
    if(self.quietSystemTouches) {
        NSInteger actualSelected=HGASelectedTab(self.hostController);
        [self syncSystemSelection:actualSelected force:YES];
        if(!self.quietLensActive || !self.quietDragLens || self.quietDragLens.hidden ||
            !self.superview || self.collapsed || self.restoring || UIAccessibilityIsReduceMotionEnabled()) {
            [self clearSelectionDrag];
            [self refresh];
            return;
        }
        // Apply light/dark theme, icon ink and feed reset immediately before
        // running the smooth settle glide so there is zero delayed theme flash.
        [self refresh];
        if(!self.quietSystemTouches || !self.quietLensActive || !self.quietDragLens || self.quietDragLens.hidden) {
            return;
        }
        UIButton *targetBtn=[self buttonForTabIndex:actualSelected];
        if(!targetBtn) {
            [self clearSelectionDrag];
            return;
        }
        NSUInteger generation=++self.selectionGeneration;
        self.selectionSettling=YES;
        CGRect resting=[self quietFrameAtX:CGRectGetMidX(targetBtn.frame) button:targetBtn lifted:NO];
        CALayer *pres=self.quietDragLens.layer.presentationLayer;
        CGRect liveFrame=(pres && !CGRectIsEmpty(pres.bounds)) ?
            CGRectMake(pres.position.x-pres.bounds.size.width*0.5,pres.position.y-pres.bounds.size.height*0.5,
                pres.bounds.size.width,pres.bounds.size.height) :
            CGRectMake(self.quietDragLens.center.x-self.quietDragLens.bounds.size.width*0.5,
                self.quietDragLens.center.y-self.quietDragLens.bounds.size.height*0.5,
                self.quietDragLens.bounds.size.width,self.quietDragLens.bounds.size.height);
        CGFloat dx=CGRectGetMidX(resting)-CGRectGetMidX(liveFrame);
        CGFloat springVelocity=fabs(dx)>4.0 ? 0.5 : 0.0;
        if(fabs(releaseVelocity)>10.0 && fabs(dx)>2.0) {
            CGFloat toward=(dx>=0 ? releaseVelocity : -releaseVelocity)/MAX(40.0,fabs(dx));
            springVelocity=MIN(4.0,MAX(-1.5,toward));
        }
        [UIView performWithoutAnimation:^{
            [self.quietDragLens.layer removeAllAnimations];
            [self placeQuietLensFrame:liveFrame];
            self.quietDragLens.overrideUserInterfaceStyle=self.capsule.overrideUserInterfaceStyle;
            [self setQuietSourcesHidden:YES];
        }];
        __weak HGAAppearanceDock *weakSelf=self;
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,(int64_t)(0.10*NSEC_PER_SEC)),dispatch_get_main_queue(),^{
            HGAAppearanceDock *dock=weakSelf;
            if(!dock || dock.selectionGeneration!=generation || !dock.quietLensActive)return;
            [dock setQuietSourcesHidden:NO];
            [UIView animateWithDuration:0.09 delay:0
                options:UIViewAnimationOptionBeginFromCurrentState|UIViewAnimationOptionAllowUserInteraction|UIViewAnimationOptionCurveEaseOut
                animations:^{
                    dock.quietDragLens.alpha=0;
                } completion:nil];
        });
        [UIView animateWithDuration:0.20 delay:0 usingSpringWithDamping:0.85 initialSpringVelocity:springVelocity
            options:UIViewAnimationOptionBeginFromCurrentState|UIViewAnimationOptionAllowUserInteraction
            animations:^{
                self.quietDragLens.transform=CGAffineTransformIdentity;
                [self placeQuietLensFrame:resting];
            } completion:^(BOOL finished){
                if(self.selectionGeneration!=generation)return;
                if(HGEnhancedDiagnostics() && startTs>0) {
                    HGLog(@"appearance.dock-motion",@{
                        @"mode":motionMode ?: @"",
                        @"peakVelocity":@(round(peakVelocity)),
                        @"durationMs":@(round((CACurrentMediaTime()-startTs)*1000.0)),
                        @"materialReused":@YES
                    });
                }
                [UIView performWithoutAnimation:^{
                    [self hideQuietLens];
                }];
                self.selectionSettling=NO;
                self.dragPeakVelocity=0;
                self.dragLastTimestamp=0;
                self.motionStartTimestamp=0;
            }];
        return;
    }
    if(!self.superview || self.collapsed || self.restoring || UIAccessibilityIsReduceMotionEnabled()) {
        [self clearSelectionDrag];
        if(self.superview && !self.restoring && !self.collapsed)[self renderGeometry];
        return;
    }
    if([self usesFeedMotion]) {
        NSInteger selected=HGASelectedTab(self.hostController);
        if(self.feedLastSelectedTab!=NSNotFound && selected!=self.feedLastSelectedTab) {
            [self cancelActiveFeedObserverIfNeeded];
            [self setFeedCompact:NO animated:NO];
        }
        self.feedLastSelectedTab=selected;
    }
    NSUInteger generation=++self.selectionGeneration;
    self.selectionSettling=YES;
    // Apply the host's settled tab state outside the animation transaction.
    // The animation below changes only the selection and its highlight.
    [self renderGeometry];
    CGRect resting=self.restingSelectionFrame;
    [self animateMotionGlyph:nil];
    CGFloat springVelocity=0;
    if(!CGRectIsNull(resting)) {
        CGFloat dx=CGRectGetMidX(resting)-CGRectGetMidX(self.selection.frame);
        if(fabs(releaseVelocity)>10.0 && fabs(dx)>2.0) {
            CGFloat toward=(dx>=0 ? releaseVelocity : -releaseVelocity)/MAX(40.0,fabs(dx));
            springVelocity=MIN(4.0,MAX(-1.5,toward));
        } else if(fabs(dx)>4.0) {
            springVelocity=0.4;
        }
    }
    BOOL fadeLens=self.dragLens && !self.dragLens.hidden &&
        [self.dragLens.effect isKindOfClass:NSClassFromString(@"UIGlassEffect")];
    if(fadeLens) {
        // Keep the pure refractive lens unclouded during travel, then crossfade
        // smoothly into the recessed resting thumb as the spring lands.
        [UIView animateWithDuration:0.12 delay:0.06
            options:UIViewAnimationOptionBeginFromCurrentState|UIViewAnimationOptionAllowUserInteraction|UIViewAnimationOptionCurveEaseOut
            animations:^{
                self.dragLens.alpha=0;
                if(self.restingSelectionColor)self.selection.backgroundColor=self.restingSelectionColor;
            } completion:nil];
    }
    [UIView animateWithDuration:0.22 delay:0 usingSpringWithDamping:0.84 initialSpringVelocity:springVelocity
        options:UIViewAnimationOptionBeginFromCurrentState|UIViewAnimationOptionAllowUserInteraction
        animations:^{
            if(!CGRectIsNull(resting))[self placeSelection:resting];
            self.dragLens.transform=CGAffineTransformIdentity;
            self.selectionHighlight.alpha=0;
            if(!fadeLens && self.restingSelectionColor)self.selection.backgroundColor=self.restingSelectionColor;
        } completion:^(BOOL finished){
            if(self.selectionGeneration!=generation)return;
            if(HGEnhancedDiagnostics() && startTs>0) {
                HGLog(@"appearance.dock-motion",@{
                    @"mode":motionMode ?: @"",
                    @"peakVelocity":@(round(peakVelocity)),
                    @"durationMs":@(round((CACurrentMediaTime()-startTs)*1000.0)),
                    @"materialReused":@YES
                });
            }
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
    self.dragLastX=position;
    self.dragLastTimestamp=CACurrentMediaTime();
    self.dragVelocityX=0;
    if(self.motionStartTimestamp<=0)self.motionStartTimestamp=self.dragLastTimestamp;
    [self prepareHaptics];
    if(self.quietSystemTouches) {
        [UIView performWithoutAnimation:^{
            [self.quietDragLens.layer removeAllAnimations];
            if(self.quietDragLens && self.quietLensActive) {
                self.quietDragLens.alpha=1;
                self.quietDragLens.transform=CGAffineTransformIdentity;
            }
            [self positionDragSelection:position];
        }];
        return;
    }
    [self animateMotionGlyph:nil];
    if(!UIAccessibilityIsReduceMotionEnabled())[self showSelectionMotionMaterial];
    [UIView performWithoutAnimation:^{
        [self.selection.layer removeAllAnimations];
        [self.dragLens.layer removeAllAnimations];
        self.dragLens.alpha=1;
        self.dragLens.transform=CGAffineTransformIdentity;
        [self positionDragSelection:position];
    }];
}
- (void)accessoryDown:(UIButton *)button { [self animateDockAccessory:button pressed:YES]; }
- (void)accessoryUp:(UIButton *)button { [self animateDockAccessory:button pressed:NO]; }
- (void)animateDockAccessory:(UIButton *)button pressed:(BOOL)pressed {
    if(UIAccessibilityIsReduceMotionEnabled())return;
    UIView *surface=button==self.extraButton ? self.extra : self.expand;
    CGFloat base=(surface==self.extra && [self usesFeedMotion] && self.feedExtraScale>0.0) ? self.feedExtraScale : 1.0;
    CGFloat factor=base*(pressed ? 1.06 : 1.0);
    [UIView animateWithDuration:pressed?0.26:0.36 delay:0 usingSpringWithDamping:pressed?0.62:0.64 initialSpringVelocity:pressed?0.3:0
        options:UIViewAnimationOptionBeginFromCurrentState|UIViewAnimationOptionAllowUserInteraction animations:^{
            surface.transform=CGAffineTransformMakeScale(factor,factor);
        } completion:nil];
}
- (void)willMoveToWindow:(UIWindow *)window {
    if(!window) {
        [self cancelActiveFeedObserverIfNeeded];
        [self resetFeedMotionState];
        [self removeSystemDock];
        self.failedSystemProbe=nil;
        self.dockTouchButton=nil;
        [self clearSelectionDrag];
        self.capsule.transform=CGAffineTransformIdentity;
        self.extra.transform=CGAffineTransformIdentity;
        self.expand.transform=CGAffineTransformIdentity;
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
    if(self.quietSystemTouches)[self syncSystemSelection:button.tag force:NO];
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
    CGPoint local=[self convertPoint:point toView:self.capsule];
    CGFloat x=local.x;
    CGFloat dx=point.x-self.dockTouchStart.x,dy=point.y-self.dockTouchStart.y;
    BOOL outsideVertically=local.y<-12 || local.y>CGRectGetHeight(self.capsule.bounds)+12;
    if(fabs(dy)>24 && fabs(dy)>fabs(dx)*1.35 && outsideVertically) {
        [self endDockTouchAtPoint:point cancelled:YES];return;
    }
    if(!accessory && !self.selectionDragging && fabs(dx)>7)[self beginDockDragAtX:x];
    if(self.selectionDragging)[self positionDragSelection:x];
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

- (void)handleApplicationDidEnterBackground:(__unused NSNotification *)note {
    [self cancelActiveFeedObserverIfNeeded];
    if([self usesFeedMotion] && (_feedSpring.following || self.feedDisplayLink))
        [self setFeedCompact:(_feedSpring.compact!=0) animated:NO];
}

- (void)prepareSystemFeedItems:(NSArray<NSString *> *)keys content:(NSString *)content
    selected:(NSInteger)selected videoClear:(BOOL)videoClear {
    UIView *platter=self.systemPlatter ?: self.systemTabBar;
    if(!platter || !keys.count){self.systemFeedItems=nil;return;}
    NSMutableArray<UIView *> *buttons=[HGADockSystemItems(platter) mutableCopy];
    if(!buttons.count && platter!=self.systemTabBar)buttons=[HGADockSystemItems(self.systemTabBar) mutableCopy];
    [buttons sortUsingComparator:^NSComparisonResult(UIView *left,UIView *right) {
        CGFloat ax=[platter convertPoint:left.center fromView:left.superview].x;
        CGFloat bx=[platter convertPoint:right.center fromView:right.superview].x;
        return ax<bx ? NSOrderedAscending : (ax>bx ? NSOrderedDescending : NSOrderedSame);
    }];
    UITraitCollection *traits=self.capsule.traitCollection;
    BOOL darkDock=self.capsule.overrideUserInterfaceStyle==UIUserInterfaceStyleDark ||
        traits.userInterfaceStyle==UIUserInterfaceStyleDark;
    UITraitCollection *inkTraits=(darkDock && traits.userInterfaceStyle!=UIUserInterfaceStyleDark) ?
        [UITraitCollection traitCollectionWithUserInterfaceStyle:UIUserInterfaceStyleDark] : traits;
    UIColor *selectedColor=[UIColor.labelColor resolvedColorWithTraitCollection:inkTraits];
    UIColor *normalColor=[selectedColor colorWithAlphaComponent:traits.accessibilityContrast==UIAccessibilityContrastHigh ? 0.90 : 0.62];
    if(videoClear) {
        selectedColor=UIColor.whiteColor;
        normalColor=[UIColor.whiteColor colorWithAlphaComponent:traits.accessibilityContrast==UIAccessibilityContrastHigh ? 0.94 : 0.78];
    }
    BOOL rtl=self.systemTabBar.effectiveUserInterfaceLayoutDirection==UIUserInterfaceLayoutDirectionRightToLeft;
    BOOL showCaption=[content isEqual:@"both"];
    NSMutableArray<HGAFeedDockItemPart *> *parts=[NSMutableArray array];
    NSInteger column=-1;
    CGFloat lastX=-CGFLOAT_MAX;
    for(UIView *button in buttons) {
        CGFloat x=[platter convertPoint:button.center fromView:button.superview].x;
        if(fabs(x-lastX)>0.5)column++;
        lastX=x;
        if(column<0 || (NSUInteger)column>=keys.count)continue;
        UIImageView *icon=nil;
        for(UIView *sub in HGADockSystemSubviews(button)) {
            if([sub isKindOfClass:UIImageView.class]){icon=(UIImageView *)sub;break;}
        }
        if(!icon)continue;
        NSUInteger slot=rtl ? (keys.count-1-(NSUInteger)column) : (NSUInteger)column;
        NSString *key=keys[slot];
        NSUInteger tabIndex=[HGATabIDs() indexOfObject:key];
        BOOL active=(selected!=NSNotFound && selected==(NSInteger)tabIndex);
        HGAFeedDockItemPart *part=objc_getAssociatedObject(button,&HGAFeedDockItemPartKey);
        if(!part) {
            part=[HGAFeedDockItemPart new];
            part.buttonLayer=button.layer;
            objc_setAssociatedObject(button,&HGAFeedDockItemPartKey,part,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        if(part.icon!=icon)part.icon=icon;
        if(showCaption && !part.caption) {
            UILabel *caption=[[UILabel alloc] initWithFrame:CGRectZero];
            caption.font=[UIFont systemFontOfSize:10.0 weight:UIFontWeightSemibold];
            caption.textAlignment=NSTextAlignmentCenter;
            caption.adjustsFontSizeToFitWidth=YES;
            caption.minimumScaleFactor=0.78;
            caption.userInteractionEnabled=NO;
            caption.isAccessibilityElement=NO;
            [button addSubview:caption];
            part.caption=caption;
        }
        UILabel *caption=part.caption;
        if(caption) {
            if(caption.hidden==showCaption)caption.hidden=!showCaption;
            NSString *labelText=tabIndex!=NSNotFound ? HGATabTitles()[tabIndex] : @"";
            if(![caption.text isEqualToString:labelText])caption.text=labelText;
            UIColor *ink=active ? selectedColor : normalColor;
            if(![caption.textColor isEqual:ink])caption.textColor=ink;
            CGRect bounds=CGRectMake(0,0,MAX(0.0,MIN(48.0,CGRectGetWidth(button.bounds)-4.0)),ceil(caption.font.lineHeight));
            CGPoint center=CGPointMake(CGRectGetMidX(button.bounds),CGRectGetMidY(button.bounds));
            if(!CGRectEqualToRect(caption.bounds,bounds))caption.bounds=bounds;
            if(!CGPointEqualToPoint(caption.center,center))caption.center=center;
        }
        [parts addObject:part];
    }
    self.systemFeedItems=parts;
}

- (void)applyFeedItemsScaleX:(CGFloat)sx scaleY:(CGFloat)sy glyphScale:(CGFloat)glyphScale
    captionAmount:(CGFloat)captionAmount contentAlpha:(CGFloat)contentAlpha {
    if(self.systemDockActive) {
        CATransform3D iconOnly=CATransform3DMakeScale(glyphScale/sx,glyphScale/sy,1.0);
        CATransform3D withCaption=iconOnly;
        withCaption.m42=-7.0*captionAmount/sy;
        CGAffineTransform text=CGAffineTransformIdentity;
        if(captionAmount>0.0) {
            text=CGAffineTransformMake(1.0/glyphScale,0,0,1.0/glyphScale,0,(13.0-withCaption.m42)/withCaption.m22);
        }
        [CATransaction begin];
        [CATransaction setDisableActions:YES];
        for(HGAFeedDockItemPart *part in self.systemFeedItems) {
            UILabel *caption=part.caption;
            BOOL hasCaption=caption && !caption.hidden;
            CATransform3D contents=hasCaption ? withCaption : iconOnly;
            CALayer *layer=part.buttonLayer;
            if(layer && !CATransform3DEqualToTransform(layer.sublayerTransform,contents))layer.sublayerTransform=contents;
            if(part.icon && part.icon.alpha!=contentAlpha)part.icon.alpha=contentAlpha;
            if(hasCaption) {
                CGFloat capAlpha=captionAmount*contentAlpha;
                if(caption.alpha!=capAlpha)caption.alpha=capAlpha;
                if(captionAmount>0.0 && !CGAffineTransformEqualToTransform(caption.transform,text))caption.transform=text;
            }
        }
        [CATransaction commit];
        return;
    }
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    for(NSString *key in self.buttons) {
        HGADockTabButton *b=(HGADockTabButton *)self.buttons[key];
        if(b.hidden)continue;
        BOOL hasImage=b.currentImage!=nil && !b.imageView.hidden;
        BOOL hasTitle=b.currentTitle.length>0 && !b.titleLabel.hidden;
        CGFloat pressBoost=(self.motionGlyph==b && !UIAccessibilityIsReduceMotionEnabled()) ? 1.14 : 1.0;
        if(hasImage && hasTitle) {
            CGFloat titleHeight=MIN(CGRectGetHeight(b.bounds)-4.0,ceil(b.titleLabel.font.lineHeight));
            CGFloat iconShiftY=((titleHeight+2.0)*0.5)*(1.0-captionAmount);
            CGAffineTransform iconTx=CGAffineTransformMake((glyphScale*pressBoost)/sx,0.0,0.0,
                (glyphScale*pressBoost)/sy,0.0,iconShiftY);
            if(!CGAffineTransformEqualToTransform(b.imageView.transform,iconTx))b.imageView.transform=iconTx;
            if(b.imageView.alpha!=contentAlpha)b.imageView.alpha=contentAlpha;

            CGFloat glyphH=CGRectGetHeight(b.imageView.bounds);
            CGFloat titleOffset=(glyphH+2.0)*0.5;
            CGFloat titleShiftY=titleOffset*(captionAmount/sy-1.0);
            CGAffineTransform titleTx=CGAffineTransformMake(1.0/sx,0.0,0.0,1.0/sy,0.0,titleShiftY);
            CGFloat capAlpha=captionAmount*contentAlpha;
            if(b.titleLabel.alpha!=capAlpha)b.titleLabel.alpha=capAlpha;
            if(capAlpha>0.0 && !CGAffineTransformEqualToTransform(b.titleLabel.transform,titleTx))b.titleLabel.transform=titleTx;
        } else if(hasImage) {
            CGAffineTransform iconTx=CGAffineTransformMakeScale((glyphScale*pressBoost)/sx,(glyphScale*pressBoost)/sy);
            if(!CGAffineTransformEqualToTransform(b.imageView.transform,iconTx))b.imageView.transform=iconTx;
            if(b.imageView.alpha!=contentAlpha)b.imageView.alpha=contentAlpha;
        } else if(hasTitle) {
            CGFloat textBoost=(self.motionGlyph==b && !UIAccessibilityIsReduceMotionEnabled()) ? 1.06 : 1.0;
            CGAffineTransform titleTx=CGAffineTransformMakeScale((glyphScale*textBoost)/sx,(glyphScale*textBoost)/sy);
            if(!CGAffineTransformEqualToTransform(b.titleLabel.transform,titleTx))b.titleLabel.transform=titleTx;
            if(b.titleLabel.alpha!=contentAlpha)b.titleLabel.alpha=contentAlpha;
        }
    }
    [CATransaction commit];
}

- (void)applySiriaiSurface:(DKSiriaiDockGeometry)shape targetCenter:(CGPoint)target {
    CGFloat glassAlpha=shape.whiteAmount>=1.0 ? 0.0 : shape.surfaceAlpha;
    if(self.systemDockActive) {
        if(self.systemPlatter && self.systemPlatter.alpha!=glassAlpha)self.systemPlatter.alpha=glassAlpha;
    } else {
        HGAAppearanceMaterial *glass=HGAState(self.capsule,NO).glass;
        if(glass && glass.alpha!=glassAlpha)glass.alpha=glassAlpha;
        if(self.flatSegments && self.flatSegments.alpha!=glassAlpha)self.flatSegments.alpha=glassAlpha;
        CGFloat selAlpha=shape.contentAlpha*(1.0-shape.whiteAmount);
        if(self.selection.alpha!=selAlpha)self.selection.alpha=selAlpha;
        if(self.capsule.layer.borderWidth>0)
            self.capsule.layer.borderColor=[UIColor colorWithWhite:self.feedLightSurface?0:1 alpha:0.14*glassAlpha].CGColor;
    }
    float fillAlpha=(float)(shape.whiteAmount*shape.surfaceAlpha);
    if(fillAlpha==0.0f && (!self.siriaiMinimumFill || self.siriaiMinimumFill.opacity==0.0f))return;
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    if(fillAlpha>0.0f) {
        if(!self.siriaiMinimumFill) {
            self.siriaiMinimumFill=[CALayer layer];
            self.siriaiMinimumFill.backgroundColor=UIColor.whiteColor.CGColor;
            self.siriaiMinimumFill.zPosition=10.0;
            [self.layer addSublayer:self.siriaiMinimumFill];
        }
        self.siriaiMinimumFill.position=target;
        self.siriaiMinimumFill.bounds=CGRectMake(0.0,0.0,shape.width,shape.height);
        self.siriaiMinimumFill.cornerRadius=shape.height*0.5;
    }
    self.siriaiMinimumFill.opacity=fillAlpha;
    [CATransaction commit];
}

- (void)applyFeedMotion:(CGFloat)progress {
    CGFloat width=CGRectGetWidth(self.feedCapsuleAtRest),height=CGRectGetHeight(self.feedCapsuleAtRest);
    if(width<=0.0 || height<=0.0)return;
    BOOL hasExtra=!self.extra.hidden && !CGRectIsEmpty(self.feedExtraAtRest);
    CGFloat extraDiameter=hasExtra ? CGRectGetWidth(self.feedExtraAtRest) : 0.0;
    CGFloat gap=hasExtra ? HGADockAccessoryGap : 0.0;
    CGPoint pivot=CGPointMake(CGRectGetMidX(self.feedCapsuleAtRest),CGRectGetMidY(self.feedCapsuleAtRest));
    if(self.appliedCollapseMode==HGADockCollapseModeSiriai) {
        CGRect group=hasExtra ? CGRectUnion(self.feedCapsuleAtRest,self.feedExtraAtRest) : self.feedCapsuleAtRest;
        CGFloat x=CGRectGetMinX(group),y=CGRectGetMinY(group);
        CGFloat bottomLimit=MAX(height,CGRectGetMaxY(self.bounds)-y);
        DKSiriaiDockGeometry shape=DKSiriaiDockShape(width,height,extraDiameter,gap,bottomLimit,progress);
        DKSiriaiApplyRebound(&shape,_feedSpring.overtravel,CGRectGetWidth(group),extraDiameter,bottomLimit);
        if(_feedSpring.siriaiOpeningMaterial>1.0)DKSiriaiSetDockMaterial(&shape,_feedSpring.siriaiOpeningMaterial);
        CGPoint target=CGPointMake(x+shape.centerX,y+shape.centerY);
        CGFloat sx=MAX(0.001,shape.width/width),sy=MAX(0.001,shape.height/height);
        HGADockSetViewGeometry(self.capsule,CGAffineTransformMakeScale(sx,sy),target);
        if(self.systemDockActive) {
            UIView *carrier=[self systemDockCarrier];
            CGPoint carrierCenter=CGPointMake(target.x+(self.feedSystemCarrierAtRest.x-pivot.x)*sx,
                target.y+(self.feedSystemCarrierAtRest.y-pivot.y)*sy);
            HGADockSetViewGeometry(carrier,CGAffineTransformMakeScale(self.feedSystemBaseScale*sx,self.feedSystemBaseScale*sy),carrierCenter);
        }
        [self applySiriaiSurface:shape targetCenter:target];
        BOOL controlsHidden=shape.contentAlpha<0.10;
        self.feedControlsHidden=controlsHidden;
        if(self.capsule.accessibilityElementsHidden!=controlsHidden)self.capsule.accessibilityElementsHidden=controlsHidden;
        if(self.systemDockActive) {
            UIView *carrier=[self systemDockCarrier];
            BOOL hideCarrierA11y=self.quietSystemTouches || controlsHidden;
            if(carrier.accessibilityElementsHidden!=hideCarrierA11y)carrier.accessibilityElementsHidden=hideCarrierA11y;
        }
        CGFloat glyphScale=MAX(0.001,shape.glyphScale);
        [self applyFeedItemsScaleX:sx scaleY:sy glyphScale:glyphScale captionAmount:shape.captionAmount contentAlpha:shape.contentAlpha];
        if(hasExtra) {
            CGFloat cameraScale=MAX(0.001,shape.cameraScale);
            self.feedExtraScale=cameraScale;
            CGPoint extraCenter=CGPointMake(x+shape.cameraX,y+shape.cameraY);
            HGADockSetViewGeometry(self.extra,CGAffineTransformMakeScale(cameraScale,cameraScale),extraCenter);
            CGAffineTransform extraGlyph=CGAffineTransformMakeScale(glyphScale/cameraScale,glyphScale/cameraScale);
            if(!CGAffineTransformEqualToTransform(self.extraButton.imageView.transform,extraGlyph))
                self.extraButton.imageView.transform=extraGlyph;
            if(self.extra.alpha!=shape.contentAlpha)self.extra.alpha=shape.contentAlpha;
            if(self.extra.accessibilityElementsHidden!=controlsHidden)self.extra.accessibilityElementsHidden=controlsHidden;
        }
        return;
    }
    self.feedControlsHidden=NO;
    if(self.siriaiMinimumFill && self.siriaiMinimumFill.opacity!=0.0f) {
        [CATransaction begin];[CATransaction setDisableActions:YES];
        self.siriaiMinimumFill.opacity=0.0f;
        [CATransaction commit];
    }
    if(self.systemDockActive) {
        if(self.systemPlatter && self.systemPlatter.alpha!=1.0)self.systemPlatter.alpha=1.0;
    } else {
        HGAAppearanceMaterial *glass=HGAState(self.capsule,NO).glass;
        if(glass && glass.alpha!=1.0)glass.alpha=1.0;
        if(self.flatSegments && self.flatSegments.alpha!=1.0)self.flatSegments.alpha=1.0;
        if(self.selection.alpha!=1.0)self.selection.alpha=1.0;
    }
    if(self.capsule.accessibilityElementsHidden)self.capsule.accessibilityElementsHidden=NO;
    if(self.extra.accessibilityElementsHidden)self.extra.accessibilityElementsHidden=NO;

    DKSiriaiDockGeometry shape=DKTikTokDockShape(CGRectGetMinX(self.feedCapsuleAtRest),CGRectGetMinY(self.feedCapsuleAtRest),
        width,height,CGRectGetMidY(self.feedExtraAtRest),extraDiameter,gap,
        (unsigned)MAX(1u,(unsigned)HGADockCapsuleTabs().count),progress+_feedSpring.overtravel);
    CGFloat sx=MAX(0.001,shape.width/width),sy=MAX(0.001,shape.height/height);
    CGPoint target=CGPointMake(shape.centerX,shape.centerY);
    HGADockSetViewGeometry(self.capsule,CGAffineTransformMakeScale(sx,sy),target);
    if(self.systemDockActive) {
        UIView *carrier=[self systemDockCarrier];
        CGPoint carrierCenter=CGPointMake(target.x+(self.feedSystemCarrierAtRest.x-pivot.x)*sx,
            target.y+(self.feedSystemCarrierAtRest.y-pivot.y)*sy);
        HGADockSetViewGeometry(carrier,CGAffineTransformMakeScale(self.feedSystemBaseScale*sx,self.feedSystemBaseScale*sy),carrierCenter);
    }
    [self applyFeedItemsScaleX:sx scaleY:sy glyphScale:shape.glyphScale captionAmount:shape.captionAmount contentAlpha:1.0];
    if(hasExtra) {
        CGFloat cameraScale=MAX(0.001,shape.cameraScale);
        self.feedExtraScale=cameraScale;
        CGPoint extraCenter=CGPointMake(shape.cameraX,shape.cameraY);
        HGADockSetViewGeometry(self.extra,CGAffineTransformMakeScale(cameraScale,cameraScale),extraCenter);
        CGAffineTransform extraGlyph=CGAffineTransformMakeScale(shape.glyphScale/cameraScale,shape.glyphScale/cameraScale);
        if(!CGAffineTransformEqualToTransform(self.extraButton.imageView.transform,extraGlyph))
            self.extraButton.imageView.transform=extraGlyph;
        if(self.extra.alpha!=1.0)self.extra.alpha=1.0;
    }
}

- (void)resetFeedMotionState {
    [self.feedDisplayLink invalidate];
    self.feedDisplayLink=nil;
    DKFeedSpringInit(&_feedSpring);
    self.feedControlsHidden=NO;
    self.feedExtraScale=1.0;
    [self.siriaiMinimumFill removeFromSuperlayer];
    self.siriaiMinimumFill=nil;
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    self.capsule.transform=CGAffineTransformIdentity;
    if(!CGRectIsEmpty(self.feedCapsuleAtRest))
        self.capsule.center=CGPointMake(CGRectGetMidX(self.feedCapsuleAtRest),CGRectGetMidY(self.feedCapsuleAtRest));
    self.capsule.accessibilityElementsHidden=NO;
    HGAAppearanceMaterial *glass=HGAState(self.capsule,NO).glass;
    if(glass)glass.alpha=1.0;
    if(self.flatSegments)self.flatSegments.alpha=1.0;
    self.selection.alpha=1.0;
    if(self.systemDockActive) {
        UIView *carrier=[self systemDockCarrier];
        carrier.transform=CGAffineTransformMakeScale(self.feedSystemBaseScale,self.feedSystemBaseScale);
        if(!CGPointEqualToPoint(self.feedSystemCarrierAtRest,CGPointZero))carrier.center=self.feedSystemCarrierAtRest;
        carrier.accessibilityElementsHidden=self.quietSystemTouches;
        if(self.systemPlatter)self.systemPlatter.alpha=1.0;
    }
    for(HGAFeedDockItemPart *part in self.systemFeedItems) {
        if(part.buttonLayer)part.buttonLayer.sublayerTransform=CATransform3DIdentity;
        if(part.icon)part.icon.alpha=1.0;
        if(part.caption) {
            part.caption.hidden=YES;
            part.caption.alpha=1.0;
            part.caption.transform=CGAffineTransformIdentity;
        }
    }
    self.systemFeedItems=nil;
    for(UIButton *b in self.buttons.allValues) {
        b.imageView.transform=CGAffineTransformIdentity;
        b.imageView.alpha=1.0;
        b.titleLabel.transform=CGAffineTransformIdentity;
        b.titleLabel.alpha=1.0;
    }
    self.extra.transform=CGAffineTransformIdentity;
    if(!CGRectIsEmpty(self.feedExtraAtRest))
        self.extra.center=CGPointMake(CGRectGetMidX(self.feedExtraAtRest),CGRectGetMidY(self.feedExtraAtRest));
    self.extra.alpha=1.0;
    self.extra.accessibilityElementsHidden=NO;
    self.extraButton.imageView.transform=CGAffineTransformIdentity;
    [CATransaction commit];
    self.feedCapsuleAtRest=CGRectZero;
    self.feedExtraAtRest=CGRectZero;
}

- (void)startFeedDisplayLink {
    if(self.feedDisplayLink) {
        if(self.feedDisplayLink.paused)self.feedDisplayLink.paused=NO;
        return;
    }
    HGAFeedSpringTarget *target=[HGAFeedSpringTarget new];
    target.dock=self;
    CADisplayLink *link=[CADisplayLink displayLinkWithTarget:target selector:@selector(advance:)];
    if(@available(iOS 15.0,*)) {
        link.preferredFrameRateRange=CAFrameRateRangeMake(60,120,120);
    }
    [link addToRunLoop:NSRunLoop.mainRunLoop forMode:NSRunLoopCommonModes];
    self.feedDisplayLink=link;
}

- (void)scheduleSiriaiFade {
    uint64_t ticket=++_feedSpring.siriaiFadeGeneration;
    __weak HGAAppearanceDock *weakSelf=self;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,(int64_t)(DKSiriaiFadeDelay*NSEC_PER_SEC)),dispatch_get_main_queue(),^{
        HGAAppearanceDock *dock=weakSelf;
        if(!dock || ![dock usesFeedMotion])return;
        if(DKFeedSpringTriggerSiriaiFade(&dock->_feedSpring,ticket))
            [dock startFeedDisplayLink];
    });
}

- (void)advanceFeedSpring:(CADisplayLink *)link {
    if(![self usesFeedMotion]) {
        [self.feedDisplayLink invalidate];
        self.feedDisplayLink=nil;
        return;
    }
    double applyProgress=0.0;
    int shouldApply=0,scheduleFade=0;
    DKFeedAdvanceResult res=DKFeedSpringAdvance(&_feedSpring,link.timestamp,link.targetTimestamp,
        UIAccessibilityIsReduceMotionEnabled() ? 1 : 0,&applyProgress,&shouldApply,&scheduleFade);
    if(shouldApply)[self applyFeedMotion:applyProgress];
    if(res==DKFeedAdvancePauseLink) {
        link.paused=YES;
    } else if(res==DKFeedAdvanceFinished) {
        [self.feedDisplayLink invalidate];
        self.feedDisplayLink=nil;
        if(scheduleFade)[self scheduleSiriaiFade];
    }
}

- (BOOL)beginFeedDrag {
    if(![self usesFeedMotion] || !self.window || self.hidden || self.sidebarSuspended)return NO;
    [self clearSelectionDrag];
    BOOL running=self.feedDisplayLink && !self.feedDisplayLink.paused;
    int direct=DKFeedSpringBeginDrag(&_feedSpring,running ? 1 : 0);
    if(self.feedDisplayLink)self.feedDisplayLink.paused=YES;
    return direct!=0;
}

- (void)moveFeedDrag:(CGFloat)delta {
    if(![self usesFeedMotion])return;
    double frame=self.feedDisplayLink ? (self.feedDisplayLink.targetTimestamp-self.feedDisplayLink.timestamp) : 0.0;
    BOOL paused=!self.feedDisplayLink || self.feedDisplayLink.paused;
    if(DKFeedSpringMoveDrag(&_feedSpring,delta,CACurrentMediaTime(),frame,paused ? 1 : 0))
        [self startFeedDisplayLink];
}

- (void)endFeedDragCompact:(BOOL)compact velocity:(CGFloat)velocity {
    if(![self usesFeedMotion])return;
    double frame=self.feedDisplayLink ? (self.feedDisplayLink.targetTimestamp-self.feedDisplayLink.timestamp) : 0.0;
    BOOL canAnimate=self.window!=nil && !UIAccessibilityIsReduceMotionEnabled();
    double applyProgress=0.0;
    int finished=0,startLink=0,scheduleFade=0;
    DKFeedSpringEndDrag(&_feedSpring,compact ? 1 : 0,velocity,CACurrentMediaTime(),frame,
        self.feedDisplayLink!=nil ? 1 : 0,canAnimate ? 1 : 0,&applyProgress,&finished,&startLink,&scheduleFade);
    if(finished) {
        [self.feedDisplayLink invalidate];
        self.feedDisplayLink=nil;
        [self applyFeedMotion:applyProgress];
        if(scheduleFade)[self scheduleSiriaiFade];
    } else if(startLink) {
        [self startFeedDisplayLink];
    }
}

- (void)setFeedCompact:(BOOL)compact animated:(BOOL)animated {
    if(![self usesFeedMotion] || (compact && (self.hidden || self.sidebarSuspended)))return;
    BOOL canAnimate=animated && self.window!=nil && !UIAccessibilityIsReduceMotionEnabled();
    double applyProgress=0.0;
    int finished=0,startLink=0,scheduleFade=0;
    DKFeedSpringSetCompact(&_feedSpring,compact ? 1 : 0,animated ? 1 : 0,CACurrentMediaTime(),
        self.feedDisplayLink!=nil ? 1 : 0,canAnimate ? 1 : 0,&applyProgress,&finished,&startLink,&scheduleFade);
    if(finished) {
        [self.feedDisplayLink invalidate];
        self.feedDisplayLink=nil;
        [self applyFeedMotion:applyProgress];
        if(scheduleFade)[self scheduleSiriaiFade];
    } else if(startLink) {
        [self startFeedDisplayLink];
    }
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
        if(covered && [dock usesFeedMotion]) {
            [dock cancelActiveFeedObserverIfNeeded];
            [dock setFeedCompact:NO animated:NO];
        }
        dock.selectionPan.enabled=!covered && (!dock.systemDockActive || dock.quietSystemTouches) &&
            !dock.collapsed && !UIAccessibilityIsVoiceOverRunning();
        dock.dockTouch.enabled=!covered && (!dock.systemDockActive || dock.quietSystemTouches || dock.collapsed) &&
            !UIAccessibilityIsVoiceOverRunning();
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
void HGAppearanceReclaimDockChrome(UIView *bar) {
    if(!NSThread.isMainThread || !bar.window)return;
    HGAAppearanceDock *dock=objc_getAssociatedObject(bar,&HGADockKey);
    if(dock && dock.superview==bar && !dock.restoring && !dock.hidden)[dock claimHostChrome];
}
NSDictionary *HGAppearanceDockDiagnosticState(UIWindow *window) {
    if(!window)return nil;
    for(UIView *bar in HGABars.allObjects) {
        HGAAppearanceDock *dock=objc_getAssociatedObject(bar,&HGADockKey);
        if(dock.window!=window || dock.superview!=bar)continue;
        HGAAppearanceMaterial *capsuleGlass=HGAState(dock.capsule,NO).glass;
        NSMutableArray *hostOverlays=[NSMutableArray array];
        for(UIView *child in bar.subviews) {
            if(child==dock || [child isDescendantOfView:dock] || child.hidden || child.alpha<0.01)continue;
            if([child isKindOfClass:UIVisualEffectView.class] ||
                (child.backgroundColor && ![child.backgroundColor isEqual:UIColor.clearColor])) {
                [hostOverlays addObject:@{
                    @"class":NSStringFromClass(child.class),
                    @"alpha":@(round(child.alpha*100.0)/100.0),
                    @"frame":NSStringFromCGRect(child.frame)
                }];
            }
        }
        NSString *renderer=dock.systemRendererRecord.count ? dock.systemRendererRecord[0] : @"unknown";
        NSString *material=dock.systemRendererRecord.count>2 ? dock.systemRendererRecord[2] : @"";
        return @{
            @"renderer":renderer,
            @"material":material,
            @"materialSignature":capsuleGlass.materialSignature ?: @"",
            @"materialId":capsuleGlass ? [NSString stringWithFormat:@"%p",capsuleGlass] : @"",
            @"capsuleFrame":NSStringFromCGRect([dock.capsule convertRect:dock.capsule.bounds toView:dock]),
            @"hostVisibleOverlays":hostOverlays
        };
    }
    return nil;
}
BOOL HGAppearanceSuspendDockForSidebar(UIView *bar) {
    HGAAppearanceDock *dock=objc_getAssociatedObject(bar,&HGADockKey);
    if(!dock || !bar || dock.superview!=bar || dock.hidden)return NO;
    dock.sidebarSuspended=YES;
    [dock cancelActiveFeedObserverIfNeeded];
    if([dock usesFeedMotion])[dock setFeedCompact:NO animated:NO];
    BOOL tracking=dock.pressedTab || dock.selectionDragging || dock.selectionSettling;
    dock.dockTouchButton=nil;
    [dock clearSelectionDrag];
    if(tracking && dock.quietSystemTouches)[dock syncSystemSelection:HGASelectedTab(dock.hostController) force:YES];
    dock.dockTouch.enabled=NO;
    dock.selectionPan.enabled=NO;
    dock.userInteractionEnabled=NO;
    dock.extra.transform=CGAffineTransformIdentity;
    dock.expand.transform=CGAffineTransformIdentity;
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
static BOOL HGAWindowHasModalTransitionOverlay(UIWindow *window,UIView *feed) {
    Class transitionCls=objc_lookUpClass("FQVTransitionView");
    if(!transitionCls || !window)return NO;
    for(UIView *sub in window.subviews) {
        if(!HGAVisible(sub) || [feed isDescendantOfView:sub])continue;
        if([sub isKindOfClass:transitionCls])return YES;
        for(UIView *child in sub.subviews)
            if([child isKindOfClass:transitionCls] && HGAVisible(child))return YES;
    }
    return NO;
}
@implementation HGADockScrollObserver
+ (instancetype)shared{static HGADockScrollObserver *s;static dispatch_once_t once;dispatch_once(&once,^{s=[self new];});return s;}
- (HGAAppearanceDock *)eligibleDockForFeed:(UIScrollView *)feed {
    UIWindow *win=feed.window;
    if(!win || ![feed isKindOfClass:UIScrollView.class] || !feed.isScrollEnabled)return nil;
    CGRect winBounds=win.bounds;
    if(CGRectGetHeight(winBounds)<=CGRectGetWidth(winBounds))return nil;
    CGRect inWin=[feed convertRect:feed.bounds toView:win];
    if(CGRectGetWidth(inWin)<CGRectGetWidth(winBounds)-4.0 || CGRectGetHeight(inWin)<CGRectGetHeight(winBounds)*0.60)return nil;
    if(CGRectGetMaxX(inWin)<=4.0 || CGRectGetMinX(inWin)>=CGRectGetWidth(winBounds)-4.0)return nil;
    if(HGFeedSidebarOwnsLayout(win) || HGAWindowHasActionSheet(win) || HGAWindowHasModalTransitionOverlay(win,feed))return nil;
    for(UIView *bar in HGABars.allObjects) {
        HGAAppearanceDock *dock=objc_getAssociatedObject(bar,&HGADockKey);
        if(bar.window!=win || dock.superview!=bar || dock.hidden || dock.sidebarSuspended)continue;
        if(fabs(bar.frame.origin.x)>2.0 || bar.alpha<=0.05 || [bar.layer animationForKey:HGACleanOpacity])continue;
        UIViewController *host=dock.hostController;
        if(host.presentedViewController)continue;
        NSInteger selected=HGASelectedTab(host);
        if(selected!=NSNotFound && selected!=0)continue;
        if(host.viewIfLoaded && ![feed isDescendantOfView:host.viewIfLoaded])continue;
        return dock;
    }
    return nil;
}
- (void)cancelFeedPan {
    HGAAppearanceDock *dock=self.activeDock;
    BOOL wasActive=_panState.active!=0;
    DKFeedPanReset(&_panState);
    self.activeGesture=nil;
    self.activeDock=nil;
    if(wasActive && dock)[dock setFeedCompact:[dock isFeedCompact] animated:YES];
}
- (void)pan:(UIPanGestureRecognizer *)gesture {
    if(!HGAppearanceDockEnabled() || UIAccessibilityIsVoiceOverRunning()) {
        [self cancelFeedPan];
        return;
    }
    HGADockCollapseMode mode=HGACurrentDockCollapseMode();
    if(mode==HGADockCollapseModeLegacy) {
        if(!HGAppearanceFlag(@"dock.autoCollapse") || gesture.state!=UIGestureRecognizerStateEnded)return;
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
        return;
    }
    if(!HGADockUsesFeedMotionMode(mode)) {
        [self cancelFeedPan];
        return;
    }
    UIGestureRecognizerState state=gesture.state;
    if(self.activeGesture && self.activeGesture!=gesture) {
        if(state==UIGestureRecognizerStateBegan)[self cancelFeedPan];
        else return;
    }
    if(!_panState.active) {
        if(state!=UIGestureRecognizerStateBegan && state!=UIGestureRecognizerStateChanged)return;
        UIScrollView *feed=[gesture.view isKindOfClass:UIScrollView.class] ? (UIScrollView *)gesture.view : nil;
        HGAAppearanceDock *dock=[self eligibleDockForFeed:feed];
        if(!dock)return;
        BOOL caughtMotion=[dock beginFeedDrag];
        CGPoint initial=[gesture translationInView:feed];
        CGFloat viewportH=CGRectGetHeight(feed.bounds)>0 ? CGRectGetHeight(feed.bounds) : CGRectGetHeight(feed.window.bounds);
        DKFeedPanBegin(&_panState,viewportH,mode==HGADockCollapseModeSiriai ? 1 : 0,[dock isFeedCompact] ? 1 : 0,
            caughtMotion ? 1 : 0,initial.x,initial.y,state==UIGestureRecognizerStateBegan ? 1 : 0);
        self.activeGesture=gesture;
        self.activeDock=dock;
        if(state==UIGestureRecognizerStateBegan)return;
    }
    HGAAppearanceDock *dock=self.activeDock;
    UIView *view=gesture.view;
    if(!dock || !dock.window || !view.window || dock.hidden || dock.sidebarSuspended ||
        HGAWindowHasActionSheet(dock.window) || HGAWindowHasModalTransitionOverlay(dock.window,view)) {
        [self cancelFeedPan];
        return;
    }
    if(state==UIGestureRecognizerStateCancelled || state==UIGestureRecognizerStateFailed) {
        [self cancelFeedPan];
        return;
    }
    BOOL ended=(state==UIGestureRecognizerStateEnded);
    CGPoint trans=[gesture translationInView:view];
    CGFloat velY=ended ? [gesture velocityInView:view].y : 0.0;
    double deltaProgress=0.0,endVelocity=0.0;
    int endCompact=0,locked=0;
    DKFeedPanResult res=DKFeedPanUpdate(&_panState,trans.x,trans.y,velY,ended ? 1 : 0,
        &deltaProgress,&endCompact,&endVelocity,&locked);
    if(res==DKFeedPanIgnored)return;
    if(res==DKFeedPanFinishedUnlocked) {
        self.activeGesture=nil;
        self.activeDock=nil;
        [dock endFeedDragCompact:[dock isFeedCompact] velocity:0.0];
        return;
    }
    if(deltaProgress!=0.0)[dock moveFeedDrag:deltaProgress];
    if(res==DKFeedPanEnded) {
        self.activeGesture=nil;
        self.activeDock=nil;
        [dock endFeedDragCompact:(endCompact!=0) velocity:endVelocity];
    }
}
@end
static char HGAFollowsFeed;
static void HGAObserveFeed(UIScrollView *feed) {
    HGADockCollapseMode mode=HGACurrentDockCollapseMode();
    BOOL enabled=HGAppearanceDockEnabled() && feed.window &&
        (HGADockUsesFeedMotionMode(mode) || (mode==HGADockCollapseModeLegacy && HGAppearanceFlag(@"dock.autoCollapse")));
    BOOL registered=[objc_getAssociatedObject(feed,&HGAFollowsFeed) boolValue];
    if(enabled && !registered)[feed.panGestureRecognizer addTarget:HGADockScrollObserver.shared action:@selector(pan:)];
    if(!enabled && registered) {
        [feed.panGestureRecognizer removeTarget:HGADockScrollObserver.shared action:@selector(pan:)];
        if(HGADockScrollObserver.shared.activeGesture==feed.panGestureRecognizer)
            [HGADockScrollObserver.shared cancelFeedPan];
    }
    objc_setAssociatedObject(feed,&HGAFollowsFeed,@(enabled),OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
