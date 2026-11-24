#import <UIKit/UIGestureRecognizerSubclass.h>
#import "DockSelectionGeometry.h"

@class HGAAppearanceDock;
@interface HGADockPointer : UIGestureRecognizer
@property(nonatomic,weak) HGAAppearanceDock *dock;
@property(nonatomic,weak) UITouch *trackedTouch;
@property(nonatomic) CGPoint dockPoint;
@end

@interface HGAAppearanceDock : UIView <UIGestureRecognizerDelegate>
@property(nonatomic,weak) UIView *hostBar;
@property(nonatomic,weak) UIViewController *hostController;
@property(nonatomic,strong) NSMapTable<NSString *,UIControl *> *targets;
@property(nonatomic,strong) NSMapTable<UIView *,NSNumber *> *hiddenSources;
@property(nonatomic,strong) UIColor *hostBackground;
@property(nonatomic) BOOL backgroundOwned;
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
@property(nonatomic) CFTimeInterval dockTouchStartedAt;
@property(nonatomic,strong) CADisplayLink *dockReleaseLink;
@property(nonatomic,weak) UIButton *pendingDockTap;
@property(nonatomic) BOOL dockReleaseObservedFrame;
@property(nonatomic,strong) UIVisualEffectView *dragLens;
@property(nonatomic,strong) UIVisualEffect *pressedGlass;
@property(nonatomic) BOOL motionMaterialPresented;
@property(nonatomic,weak) UIButton *pressedTab;
@property(nonatomic) NSUInteger selectionGeneration;
@property(nonatomic) BOOL selectionDragging;
@property(nonatomic) CGFloat selectionDragX;
@property(nonatomic) BOOL collapsed;
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
static const CGFloat HGADockAccessoryGap=12;
static const CFTimeInterval HGADockMinimumPressDuration=0.10;
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
    return MIN(width-8,MAX(visibleCount*44+reserve,HGAppearanceNumber(@"dock.width")*width));
}
static NSArray *HGATabTitles(void){return @[@"首页",@"剧场",@"商城",@"我的"];}
static NSArray *HGATabSymbols(void){return @[@"house",@"play.rectangle",@"bag",@"person"];}
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
    // The host can retain catalogue foreground colors while returning to playback.
    if(selected==0)return UIUserInterfaceStyleDark;
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
    glass.interactiveGlass=interactive;
    [glass requestMaterialRefresh];
    return glass.contentView ?: surface;
}

static CGRect HGADockSelectionFrame(CGFloat center,CGFloat width,CGFloat height,CGFloat preferredWidth,BOOL pressed) {
    HGADockSelectionBounds frame=HGADockSelectionBoundsForState(center,width,height,preferredWidth,pressed);
    return CGRectMake(frame.x,frame.y,frame.width,frame.height);
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
            UIButton *b=HGADockButton();
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
    if(!self.dockTouch.enabled || !container.window || hit.window!=container.window || ![self dockButtonAtPoint:point event:nil])return NO;
    if([hit isDescendantOfView:self] || hit==self.hostBar)return YES;

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
    if(gesture==self.dockTouch)
        return [self acceptsDockTouchAtPoint:[touch locationInView:self] fromView:touch.view];
    if(gesture!=self.selectionPan)return YES;
    // A proven player seek hotspot may win the initial hit. Its pan is
    // arbitrated only there, while the active Dock owns all direct presses.
    return ![touch.view isDescendantOfView:self] &&
        [self acceptsSelectionTouchAtPoint:[touch locationInView:self.capsule] inView:touch.view];
}
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gesture shouldBeRequiredToFailByGestureRecognizer:(UIGestureRecognizer *)other {
    if(gesture!=self.selectionPan || !gesture.enabled || other.view.window!=self.window ||
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
- (void)refresh {
    if(self.rendering || self.restoring || HGFeedSidebarOwnsLayout(self.hostBar.window))return;
    self.rendering=YES;
    @try {
        UIView *bar=self.hostBar;
        if(!HGAppearanceDockEnabled() || !bar.window || !HGAVisible(bar)) {[self restore];return;}
        // Preserve the live dock untouched while customTabBar slides horizontally
        // during sidebar presentation or search push navigation.
        if(fabs(bar.frame.origin.x)>2.0)return;
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
        if(!self.backgroundOwned || ![bar.backgroundColor isEqual:UIColor.clearColor])self.hostBackground=bar.backgroundColor;
        if(!self.backgroundOwned || bar.opaque)self.hostOpaque=bar.opaque;
        self.backgroundOwned=YES;
        if(![bar.backgroundColor isEqual:UIColor.clearColor])bar.backgroundColor=UIColor.clearColor;
        if(bar.opaque)bar.opaque=NO;
        [self hideSource:HGAViewGetter(bar,"contentView",bar)];
        const char *backdrops[]={"backgroundImageView","lightView","darkView","lineView"};
        for(NSUInteger i=0;i<4;i++)[self hideSource:HGAViewGetter(bar,backdrops[i],bar)];
        for(UIView *v in bar.subviews)if([v.accessibilityIdentifier isEqual:@"honggo.tabbar.glass"])[self hideSource:v];
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
    self.capsule.frame=self.collapsed?compact:expanded;
    self.expand.frame=compact;self.expand.hidden=!self.collapsed;
    self.extra.frame=CGRectMake(x+total-circle,rowTop+(rowHeight-circle)/2,circle,circle);
    self.extra.hidden=!extra || self.collapsed;
    NSString *style=HGAppearanceString(@"dock.material");
    BOOL classicBlur=[style isEqual:@"blur"];
    CGFloat radius=MIN(height/2,HGAppearanceNumber(@"dock.radius"));
    BOOL translucent=![style isEqual:@"solid"];
    NSInteger selected=HGASelectedTab(self.hostController);
    UIUserInterfaceStyle surfaceStyle=translucent ? HGADockPageStyle(self.hostController,self.hostBar,selected) :
        UIUserInterfaceStyleUnspecified;
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
    UIColor *tint=HGAppearanceColor(HGAppearanceString(@"dock.tint"),nil);
    BOOL reduced=UIAccessibilityIsReduceTransparencyEnabled();
    UIColor *base=tint ?: (translucent ? [UIColor colorWithWhite:light?1:0.10 alpha:1] : UIColor.secondarySystemBackgroundColor);
    UIColor *fill=[base colorWithAlphaComponent:!translucent || reduced ? 1 : (tint ? 0.30 : 0)];
    NSString *content=HGAppearanceString(@"dock.content");
    CGFloat unselectedOpacity=self.traitCollection.accessibilityContrast==UIAccessibilityContrastHigh ? 0.94 : 0.80;
    CGFloat requestedIcon=HGAppearanceNumber(@"dock.iconSize");
    HGADockMaterialContent(self.capsule,style,self.collapsed?22:radius,fill,NO);
    // The moving material sits above the capsule background and beneath the
    // actual controls, outside the background effect's content tree.
    UIView *background=HGAState(self.capsule,NO).glass;
    if(self.selection.superview!=self.capsule) {
        if(background)[self.capsule insertSubview:self.selection aboveSubview:background];
        else [self.capsule insertSubview:self.selection atIndex:0];
    }
    UIView *extraContent=HGADockMaterialContent(self.extra,style,circle/2,fill,YES);
    UIView *expandContent=HGADockMaterialContent(self.expand,style,22,fill,YES);
    // UIKit can adapt dynamic label colors to the material's own background
    // when the foreground belongs to the visual effect's contentView.
    if(self.extraSelection.superview!=extraContent)[extraContent insertSubview:self.extraSelection atIndex:0];
    if(self.extraButton.superview!=extraContent)[extraContent addSubview:self.extraButton];
    if(self.expandButton.superview!=expandContent)[expandContent addSubview:self.expandButton];
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
    self.selectionPan.enabled=!self.hidden && !self.collapsed && !UIAccessibilityIsVoiceOverRunning();
    self.dockTouch.enabled=!self.hidden && !UIAccessibilityIsVoiceOverRunning();
    self.capsule.layer.cornerRadius=radius;
    self.capsule.layer.borderWidth=translucent && !classicBlur ? 0.5 : 0;
    self.capsule.layer.borderColor=[UIColor colorWithWhite:light?0:1 alpha:0.14].CGColor;
    self.extra.layer.cornerRadius=circle/2;
    self.expand.layer.cornerRadius=22;
    self.extra.layer.borderWidth=translucent && !classicBlur ? 0.5 : 0;
    self.extra.layer.borderColor=[UIColor colorWithWhite:light?0:1 alpha:0.14].CGColor;
    BOOL darkSurface=self.capsule.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark;
    BOOL strongSelection=reduced || self.traitCollection.accessibilityContrast==UIAccessibilityContrastHigh;
    CGFloat selectionOpacity=strongSelection ? (darkSurface?0.18:0.10) : (darkSurface?0.09:0.065);
    UIColor *selectionBg=[UIColor colorWithWhite:darkSurface?1:0 alpha:selectionOpacity];
    UIColor *selectionEdge=[UIColor colorWithWhite:darkSurface?1:0 alpha:darkSurface?0.23:0.11];
    self.selection.backgroundColor=selectionBg;
    self.selection.layer.borderColor=selectionEdge.CGColor;
    CGRect selectedFrame=CGRectNull;
    CGFloat slotInset=MIN(14,MAX(0,(expanded.size.width-54*visible.count)/2));
    CGFloat slotWidth=(expanded.size.width-2*slotInset)/visible.count;
    for(NSString *key in self.buttons) {
        UIButton *b=self.buttons[key];NSUInteger index=[HGATabIDs() indexOfObject:key];
        if(b.superview!=self.capsule)[self.capsule addSubview:b];
        NSUInteger position=[visible indexOfObject:key];
        b.hidden=position==NSNotFound;
        if(b.hidden)continue;
        b.frame=CGRectMake(slotInset+position*slotWidth,0,slotWidth,height);
        UIControl *source=[self.targets objectForKey:key];
        BOOL active=selected!=NSNotFound ? selected==(NSInteger)index : source.selected;
        UIColor *customColor=HGAppearanceColor(HGAppearanceString(active?@"dock.selected":@"dock.normal"),nil);
        UIColor *color=customColor ?: (active ? UIColor.labelColor :
            (translucent ? [UIColor.labelColor colorWithAlphaComponent:unselectedOpacity] : UIColor.secondaryLabelColor));
        NSString *symbol=HGATabSymbols()[index];
        UIImage *image=[UIImage systemImageNamed:active?[symbol stringByAppendingString:@".fill"]:symbol] ?: [UIImage systemImageNamed:symbol];
        NSString *title=[content isEqual:@"icon"]?nil:HGATabTitles()[index];
        UIFont *font=[UIFont systemFontOfSize:[content isEqual:@"text"]?15:11 weight:active?UIFontWeightSemibold:UIFontWeightRegular];
        if(HGAppearanceFlag(@"fonts.enabled"))font=HGAConfiguredFont(font);
        CGFloat iconHeight=title.length ? height-11-font.lineHeight : height*0.34;
        CGFloat iconSize=MAX(12,MIN(requestedIcon,MIN(iconHeight,slotWidth-16)));
        b.enabled=source.enabled;b.selected=active;
        HGADockConfigureButton(b,title,[content isEqual:@"text"]?nil:image,iconSize,font,color);
        if(active) {
            CGFloat titleWidth=title.length ? [title sizeWithAttributes:@{NSFontAttributeName:font}].width : 0;
            CGFloat preferredWidth=MIN(slotWidth+20,MAX(height*1.52,titleWidth+24));
            selectedFrame=HGADockSelectionFrame(CGRectGetMidX(b.frame),expanded.size.width,height,preferredWidth,NO);
        }
        b.accessibilityLabel=HGATabTitles()[index];
        b.accessibilityTraits=UIAccessibilityTraitButton | (active?UIAccessibilityTraitSelected:0);
    }
    if((self.selectionDragging || self.pressedTab) && !self.collapsed)
        [self positionDragSelection:self.selectionDragX];
    else {
        self.selection.hidden=CGRectIsNull(selectedFrame);
        if(!self.selection.hidden)[self placeSelection:selectedFrame];
    }
    [self prepareSelectionLens];
    NSString *action=HGAppearanceString(@"dock.action");
    NSUInteger detachedIndex=[HGATabIDs() indexOfObject:action];
    UIControl *detachedSource=detachedIndex!=NSNotFound ? [self.targets objectForKey:action] : nil;
    BOOL detachedSelected=detachedIndex!=NSNotFound &&
        (selected!=NSNotFound ? selected==(NSInteger)detachedIndex : detachedSource.selected);
    NSString *baseSymbol=[action isEqual:@"settings"]?@"slider.horizontal.3":([action isEqual:@"search"]?@"magnifyingglass":HGATabSymbols()[detachedIndex]);
    UIImage *extraImage=detachedSelected ? ([UIImage systemImageNamed:[baseSymbol stringByAppendingString:@".fill"]] ?: [UIImage systemImageNamed:baseSymbol]) : [UIImage systemImageNamed:baseSymbol];
    UIColor *customColor=HGAppearanceColor(HGAppearanceString(detachedSelected?@"dock.selected":@"dock.normal"),nil);
    UIColor *accessoryColor=customColor ?: (detachedSelected ? UIColor.labelColor :
        (detachedIndex!=NSNotFound && translucent ? [UIColor.labelColor colorWithAlphaComponent:unselectedOpacity] : UIColor.labelColor));
    UIFont *accessoryFont=[UIFont systemFontOfSize:11];
    CGFloat extraIconSize=MIN(requestedIcon,circle*0.34);
    HGADockConfigureButton(self.extraButton,nil,extraImage,extraIconSize,accessoryFont,accessoryColor);
    HGADockConfigureButton(self.expandButton,nil,[UIImage systemImageNamed:@"ellipsis"],MIN(requestedIcon,22),accessoryFont,accessoryColor);
    self.extraSelection.hidden=!detachedSelected || self.extra.hidden;
    if(!self.extraSelection.hidden) {
        CGRect inner=CGRectInset(self.extra.bounds,7,7);
        self.extraSelection.frame=inner;
        self.extraSelection.layer.cornerRadius=MIN(inner.size.width,inner.size.height)/2;
        self.extraSelection.backgroundColor=selectionBg;
        self.extraSelection.layer.borderColor=selectionEdge.CGColor;
    }
    self.extraButton.selected=detachedSelected;
    self.extraButton.accessibilityLabel=[action isEqual:@"settings"]?@"Honggo 外观设置":([action isEqual:@"search"]?@"搜索":HGATabTitles()[detachedIndex]);
    self.extraButton.accessibilityTraits=UIAccessibilityTraitButton | (detachedSelected?UIAccessibilityTraitSelected:0);
    CGRect occupied=self.collapsed?compact:(extra?CGRectUnion(expanded,self.extra.frame):expanded);
    occupied=[self convertRect:occupied toView:self.window];
    if(!CGRectEqualToRect(self.lastOccupiedRect,occupied)) {
        self.lastOccupiedRect=occupied;
        // Let the native HUD finish its own layout before reconciling the new
        // capsule edge. Never reenter a player layout from a tab-bar setter.
        for(UIView *view in HGAManaged.allObjects)
            if(view.window==self.window && HGAIsBottomControl(view))[view setNeedsLayout];
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
        [self stopDockReleaseLink];
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
        if(previousWindow)for(UIView *view in HGAManaged.allObjects)
            if(view.window==previousWindow && HGAIsBottomControl(view))[view setNeedsLayout];
    } @finally {
        self.rendering=wasRendering;self.restoring=NO;
    }
}
- (void)feedback {
    if(HGAppearanceFlag(@"dock.haptics"))[[[UISelectionFeedbackGenerator alloc] init] selectionChanged];
}
- (void)tabTapped:(UIButton *)sender {
    if(self.dockTouchButton || self.pendingDockTap || self.selectionDragging || !sender.enabled || sender.hidden ||
        (self.pressedTab && self.pressedTab!=sender))return;
    if(!self.pressedTab)[self pressTab:sender];
    [self activateTabAtIndex:sender.tag animation:[self dockMotionMode]];
    if(self.pressedTab==sender)[self settleSelection];
}
- (void)activateTabAtIndex:(NSInteger)index animation:(NSString *)animation {
    if(index<0 || index>=(NSInteger)HGATabIDs().count)return;
    NSString *key=HGATabIDs()[(NSUInteger)index];
    UIControl *target=[self.targets objectForKey:key];
    if(!target || !target.enabled)return;
    UIControlEvents event=(target.allControlEvents & UIControlEventTouchUpInside)?UIControlEventTouchUpInside:UIControlEventPrimaryActionTriggered;
    HGLog(@"navigation.dock", @{@"tab":key, @"phase":@"enter", @"mode":animation});
    [target sendActionsForControlEvents:event];
    HGLog(@"navigation.dock", @{@"tab":key, @"phase":@"return", @"mode":animation});
    [self feedback];
    dispatch_async(dispatch_get_main_queue(),^{[self refresh];});
}
- (void)placeSelection:(CGRect)frame {
    if(!CGRectEqualToRect(self.selection.frame,frame))self.selection.frame=frame;
    CGFloat radius=MIN(frame.size.width,frame.size.height)/2;
    if(self.selection.layer.cornerRadius!=radius)self.selection.layer.cornerRadius=radius;
    if(!CGRectEqualToRect(self.dragLens.frame,self.selection.bounds))self.dragLens.frame=self.selection.bounds;
    if(self.dragLens.layer.cornerRadius!=radius)self.dragLens.layer.cornerRadius=radius;
    if(!CGRectEqualToRect(self.selectionHighlight.frame,self.selection.bounds))self.selectionHighlight.frame=self.selection.bounds;
    if(self.selectionHighlight.layer.cornerRadius!=radius)self.selectionHighlight.layer.cornerRadius=radius;
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
    self.selection.hidden=!destination;
    if(destination) {
        self.selectionDragX=x;
        CGFloat height=self.capsule.bounds.size.height;
        CGFloat preferredWidth=MIN(destination.bounds.size.width+20,height*1.52);
        CGRect frame=HGADockSelectionFrame(MIN(last,MAX(first,x)),self.capsule.bounds.size.width,height,
            preferredWidth,!UIAccessibilityIsReduceMotionEnabled());
        [self placeSelection:frame];
        if(self.selectionDragging)[self animateMotionGlyph:destination];
    }
    return destination;
}
- (void)clearSelectionDrag {
    self.selectionGeneration++;
    self.pressedTab=nil;
    self.selectionDragging=NO;
    self.motionMaterialPresented=NO;
    if(self.dragLens.effect)self.dragLens.effect=nil;
    self.dragLens.hidden=YES;
    self.selectionHighlight.alpha=0;
    self.motionGlyph.imageView.transform=CGAffineTransformIdentity;
    self.motionGlyph.titleLabel.transform=CGAffineTransformIdentity;
    self.motionGlyph=nil;
}
- (void)prepareSelectionLens {
    NSString *style=HGAppearanceString(@"dock.material");
    if(!HGNativeGlassConfigured() || UIAccessibilityIsReduceTransparencyEnabled() ||
        (![style isEqual:@"regular"] && ![style isEqual:@"clear"])) {
        [self.dragLens removeFromSuperview];self.dragLens=nil;
        self.pressedGlass=nil;
        return;
    }
    if(self.dragLens || CGRectIsEmpty(self.selection.bounds))return;
    if(@available(iOS 26.0,*)) {
        UIGlassEffect *effect=[UIGlassEffect effectWithStyle:UIGlassEffectStyleClear];
        effect.interactive=YES;
        effect.tintColor=[UIColor colorWithWhite:1 alpha:0.10];
        self.pressedGlass=effect;
        UIVisualEffectView *lens=[[UIVisualEffectView alloc] initWithEffect:nil];
        lens.accessibilityIdentifier=@"honggo.appearance.dock.drag-lens";
        lens.frame=self.selection.bounds;lens.layer.cornerRadius=self.selection.layer.cornerRadius;
        lens.layer.cornerCurve=kCACornerCurveContinuous;lens.clipsToBounds=YES;
        lens.userInteractionEnabled=NO;lens.accessibilityElementsHidden=YES;
        lens.hidden=YES;
        self.dragLens=lens;
        [self.selection insertSubview:lens belowSubview:self.selectionHighlight];
    }
}
- (void)setMotionMaterialVisible:(BOOL)visible {
    if(self.motionMaterialPresented==visible)return;
    self.motionMaterialPresented=visible;
    if(visible)[self prepareSelectionLens];
    UIVisualEffect *effect=visible ? self.pressedGlass : nil;
    CGFloat alpha=visible ? 1 : 0;
    if(visible)self.dragLens.hidden=NO;
    // UIKit animates UIGlassEffect appearance and disappearance through the
    // effect property itself. Keep the effect view and its ancestors opaque.
    [UIView animateWithDuration:visible?0.17:0.23 delay:0
        options:UIViewAnimationOptionBeginFromCurrentState|UIViewAnimationOptionAllowUserInteraction animations:^{
            self.dragLens.effect=effect;
            self.selectionHighlight.alpha=alpha;
        } completion:nil];
}
- (void)animateMotionGlyph:(UIButton *)glyph {
    if(glyph==self.motionGlyph || UIAccessibilityIsReduceMotionEnabled())return;
    UIButton *previous=self.motionGlyph;
    self.motionGlyph=glyph;
    [UIView animateWithDuration:glyph?0.20:0.27 delay:0 usingSpringWithDamping:0.68 initialSpringVelocity:0
        options:UIViewAnimationOptionBeginFromCurrentState|UIViewAnimationOptionAllowUserInteraction animations:^{
            previous.imageView.transform=CGAffineTransformIdentity;
            previous.titleLabel.transform=CGAffineTransformIdentity;
            glyph.imageView.transform=CGAffineTransformMakeScale(1.18,1.18);
            glyph.titleLabel.transform=CGAffineTransformMakeScale(1.08,1.08);
        } completion:nil];
}
- (NSString *)dockMotionMode {
    if(UIAccessibilityIsReduceMotionEnabled())return @"reduced-motion";
    Class glass=NSClassFromString(@"UIGlassEffect");
    return [self.dragLens.effect isKindOfClass:glass] ? @"glass-morph" : @"highlight-spring";
}
- (void)pressTab:(UIButton *)button {
    if(self.selectionDragging || self.pressedTab==button || !button.enabled || button.hidden)return;
    self.selectionGeneration++;
    self.pressedTab=button;
    CGFloat position=CGRectGetMidX(button.frame);
    if(UIAccessibilityIsReduceMotionEnabled()) {
        [self positionDragSelection:position];
        return;
    }
    if(self.selection.hidden || CGRectIsEmpty(self.selection.bounds)) {
        CGSize size=self.capsule.bounds.size;
        CGFloat preferredWidth=MIN(button.bounds.size.width+20,size.height*1.52);
        [self placeSelection:HGADockSelectionFrame(position,size.width,size.height,preferredWidth,NO)];
        self.selection.hidden=NO;
    }
    [self setMotionMaterialVisible:YES];
    [self animateMotionGlyph:button];
    [UIView animateWithDuration:0.28 delay:0 usingSpringWithDamping:0.76 initialSpringVelocity:0
        options:UIViewAnimationOptionBeginFromCurrentState|UIViewAnimationOptionAllowUserInteraction animations:^{
            [self positionDragSelection:position];
        } completion:nil];
}
- (void)settleSelection {
    self.pressedTab=nil;
    self.selectionDragging=NO;
    if(!self.superview || self.collapsed || self.restoring || UIAccessibilityIsReduceMotionEnabled()) {
        [self clearSelectionDrag];
        if(self.superview && !self.restoring && !self.collapsed)[self renderGeometry];
        return;
    }
    NSUInteger generation=++self.selectionGeneration;
    [self setMotionMaterialVisible:NO];
    [self animateMotionGlyph:nil];
    [UIView animateWithDuration:0.27 delay:0 usingSpringWithDamping:0.76 initialSpringVelocity:0
        options:UIViewAnimationOptionBeginFromCurrentState|UIViewAnimationOptionAllowUserInteraction
        animations:^{[self renderGeometry];} completion:^(BOOL finished){
            if(self.selectionGeneration!=generation)return;
            [self clearSelectionDrag];
            if(self.window && !self.collapsed && !self.restoring)[self renderGeometry];
        }];
}
- (void)cancelTabPress:(UIButton *)button {
    if(self.pressedTab==button && !self.selectionDragging &&
        self.dockTouch.state!=UIGestureRecognizerStateBegan && self.dockTouch.state!=UIGestureRecognizerStateChanged)
        [self settleSelection];
}
- (void)beginDockDragAtX:(CGFloat)position {
    self.selectionGeneration++;
    self.pressedTab=nil;
    self.selectionDragging=YES;
    [self.selection.layer removeAnimationForKey:@"position"];
    [self positionDragSelection:position];
    if(!UIAccessibilityIsReduceMotionEnabled())[self setMotionMaterialVisible:YES];
}
- (void)animateDockAccessory:(UIButton *)button pressed:(BOOL)pressed {
    if(UIAccessibilityIsReduceMotionEnabled())return;
    [UIView animateWithDuration:0.24 delay:0 usingSpringWithDamping:0.74 initialSpringVelocity:0
        options:UIViewAnimationOptionBeginFromCurrentState|UIViewAnimationOptionAllowUserInteraction animations:^{
            button.imageView.transform=pressed ? CGAffineTransformMakeScale(0.83,0.83) : CGAffineTransformIdentity;
        } completion:nil];
}
- (void)stopDockReleaseLink {
    [self.dockReleaseLink invalidate];
    self.dockReleaseLink=nil;
    self.pendingDockTap=nil;
}
- (void)willMoveToWindow:(UIWindow *)window {
    if(!window) {
        [self stopDockReleaseLink];
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
- (void)commitPendingDockTap {
    UIButton *button=self.pendingDockTap;
    [self stopDockReleaseLink];
    if(button && self.window && !self.hidden && button.enabled && !button.hidden) {
        [self releaseDockButton:button];
    }else if(button==self.extraButton || button==self.expandButton) {
        [self animateDockAccessory:button pressed:NO];
    }else if(button) {
        [self settleSelection];
    }
}
- (void)observeDockReleaseFrame:(CADisplayLink *)link {
    if(!self.dockReleaseObservedFrame) {
        self.dockReleaseObservedFrame=YES;
        return;
    }
    if(link.timestamp<self.dockTouchStartedAt+HGADockMinimumPressDuration)return;
    // A tap can begin and end in one event turn. Let the pressed state reach
    // the display before re-entering the host's synchronous tab switch.
    UIButton *button=self.pendingDockTap;
    UIView *surface=(button==self.extraButton || button==self.expandButton) ? button.imageView : self.selection;
    CALayer *presentation=surface.layer.presentationLayer;
    HGLog(@"appearance.dock-touch",@{@"phase":@"frame",@"action":@"tap",@"mode":[self dockMotionMode],
        @"height":@(presentation ? CGRectGetHeight(presentation.bounds) : 0),
        @"width":@(presentation ? CGRectGetWidth(presentation.bounds) : 0),
        @"opacity":@(self.selectionHighlight.layer.presentationLayer.opacity)});
    [self commitPendingDockTap];
}
- (void)beginDockTouchAtPoint:(CGPoint)point {
    if(self.pendingDockTap)[self commitPendingDockTap];
    self.dockTouchStart=point;
    self.dockTouchStartedAt=CACurrentMediaTime();
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
    if(!cancelled)destination=dragged ? [self positionDragSelection:[self convertPoint:point toView:self.capsule].x] :
        ([self dockButtonAtPoint:point event:nil]==button ? button : nil);
    self.dockTouchButton=nil;
    HGLog(@"appearance.dock-touch",@{@"phase":destination?@"end":@"cancel",@"mode":mode,
        @"action":accessory?@"accessory":(dragged?@"drag":@"tap")});
    if(destination && (!dragged || !destination.selected)) {
        if(!dragged && !UIAccessibilityIsReduceMotionEnabled() &&
            CACurrentMediaTime()-self.dockTouchStartedAt<HGADockMinimumPressDuration) {
            self.pendingDockTap=destination;
            self.dockReleaseObservedFrame=NO;
            self.dockReleaseLink=[CADisplayLink displayLinkWithTarget:self selector:@selector(observeDockReleaseFrame:)];
            [self.dockReleaseLink addToRunLoop:NSRunLoop.mainRunLoop forMode:NSRunLoopCommonModes];
        }else [self releaseDockButton:destination];
    }else if(accessory) {
        [self animateDockAccessory:button pressed:NO];
    }else {
        [self settleSelection];
    }
}
- (void)dragSelection:(UIPanGestureRecognizer *)gesture {
    CGFloat position=[gesture locationInView:self.capsule].x;
    if(gesture.state==UIGestureRecognizerStateBegan) {
        [self beginDockDragAtX:position];
        HGLog(@"appearance.dock-touch",@{@"phase":@"begin",@"action":@"player-overlap",@"mode":[self dockMotionMode]});
    }else if(self.selectionDragging) {
        if(gesture.state==UIGestureRecognizerStateChanged){[self positionDragSelection:position];return;}
        if(gesture.state==UIGestureRecognizerStateEnded) {
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
    if(self.dockTouchButton || self.pendingDockTap)return;
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
        if(!dock || !targetWin)return;
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
    if(!self.dockTouchButton && !self.pendingDockTap)[self setCollapsed:NO animated:YES];
}
- (void)swipe:(UISwipeGestureRecognizer *)gesture {
    if(HGAppearanceFlag(@"dock.collapse"))[self setCollapsed:gesture.direction==UISwipeGestureRecognizerDirectionUp animated:YES];
}
- (void)setCollapsed:(BOOL)value animated:(BOOL)animated {
    if(!HGAppearanceFlag(@"dock.collapse") || UIAccessibilityIsVoiceOverRunning())value=NO;
    if(self.collapsed==value || self.rendering || self.restoring || !self.superview)return;
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
BOOL HGAppearanceDockIsMutating(UIView *bar) {
    HGAAppearanceDock *dock=objc_getAssociatedObject(bar,&HGADockKey);
    return dock.rendering || dock.restoring;
}
static void HGARefreshSheetDockVisibility(UIWindow *window) {
    if(!window)return;
    BOOL covered=HGAWindowHasActionSheet(window);
    for(UIView *bar in HGABars.allObjects) {
        if(bar.window!=window)continue;
        HGAAppearanceDock *dock=objc_getAssociatedObject(bar,&HGADockKey);
        if(dock.superview!=bar || dock.hidden==covered)continue;
        dock.hidden=covered;
        dock.selectionPan.enabled=!covered &&
            !dock.collapsed && !UIAccessibilityIsVoiceOverRunning();
        dock.dockTouch.enabled=!covered && !UIAccessibilityIsVoiceOverRunning();
    }
}
void HGAppearanceLayoutDock(UIView *bar,UIViewController *controller) {
    if(!NSThread.isMainThread || !bar || HGFeedSidebarOwnsLayout(bar.window))return;
    if(!HGABars)HGABars=[NSHashTable weakObjectsHashTable];[HGABars addObject:bar];
    HGAAppearanceDock *dock=objc_getAssociatedObject(bar,&HGADockKey);
    if(!HGAppearanceDockEnabled()){[dock restore];return;}
    if(!dock){dock=[[HGAAppearanceDock alloc] initWithFrame:bar.bounds];dock.hostBar=bar;
        objc_setAssociatedObject(bar,&HGADockKey,dock,OBJC_ASSOCIATION_RETAIN_NONATOMIC);}
    if(controller)dock.hostController=controller;
    if(!dock.hostController)dock.hostController=HGAOwner(bar);
    [dock refresh];
}
void HGAppearanceRestoreDock(UIView *bar) {[(HGAAppearanceDock *)objc_getAssociatedObject(bar,&HGADockKey) restore];}
void HGAppearanceSuspendDockForSidebar(UIView *bar) {
    HGAAppearanceDock *dock=objc_getAssociatedObject(bar,&HGADockKey);
    if(dock.superview!=bar)return;
    BOOL collapsed=dock.collapsed;
    [dock restore];
    dock.collapsed=collapsed;
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
