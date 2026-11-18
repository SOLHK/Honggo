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
@property(nonatomic,strong) UIView *extra;
@property(nonatomic,strong) UIView *expand;
@property(nonatomic,strong) UIButton *extraButton;
@property(nonatomic,strong) UIButton *expandButton;
@property(nonatomic,strong) NSMutableDictionary<NSString *,UIButton *> *buttons;
@property(nonatomic,strong) NSArray<UISwipeGestureRecognizer *> *swipes;
@property(nonatomic,strong) UIPanGestureRecognizer *selectionPan;
@property(nonatomic,strong) HGAAppearanceMaterial *dragLens;
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
@end
static char HGADockKey;
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
static NSArray *HGATabIDs(void){return @[@"home",@"theater",@"store",@"mine"];}
static CGFloat HGAExpandedDockWidth(CGFloat width,CGFloat controlHeight,NSUInteger visibleCount) {
    CGFloat reserve=HGAppearanceFlag(@"dock.extra") ?
        MIN(HGAppearanceNumber(@"dock.extraSize"),controlHeight)+HGADockAccessoryGap : 0;
    return MIN(width-8,MAX(visibleCount*44+reserve,HGAppearanceNumber(@"dock.width")*width));
}
static CGFloat HGAExpandedDockWidthForHeader(UIView *header) {
    CGFloat width=CGRectGetWidth(header.bounds);
    if(!HGAppearanceDockEnabled() || width<180)return width;
    CGFloat controlHeight=HGAppearanceNumber(@"dock.extraSize");
    for(UIView *bar in HGABars.allObjects) {
        if(bar.window==header.window && HGAVisible(bar) && HGADockControlHeight(bar)>=44) {
            controlHeight=HGADockControlHeight(bar);
            break;
        }
    }
    NSUInteger visibleCount=0;
    for(NSString *key in HGATabIDs())
        if(!HGAppearanceFlag([@"dock.hide." stringByAppendingString:key]))visibleCount++;
    return HGAExpandedDockWidth(width,controlHeight,visibleCount);
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
    config.title=title;config.image=image;
    config.preferredSymbolConfigurationForImage=[UIImageSymbolConfiguration
        configurationWithPointSize:iconSize weight:UIImageSymbolWeightMedium];
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
static UIView *HGADockMaterialContent(UIView *surface,NSString *style,CGFloat radius,UIColor *fill) {
    HGAInstallGlass(surface,style,radius,0);
    HGAAppearanceMaterial *glass=HGAState(surface,NO).glass;
    // Native regular glass already diffuses its backdrop. A second system
    // material below it made light pages opaque and white in R21.
    glass.userInteractionEnabled=YES;
    glass.accessibilityElementsHidden=NO;
    glass.overlayColor=fill;
    return glass.contentView ?: surface;
}
static CGRect HGADockSelectionFrame(CGFloat center,CGFloat width,CGFloat height,CGFloat preferredWidth) {
    CGFloat halfWidth=MIN(preferredWidth/2,MIN(center,width-center)-4);
    return CGRectMake(center-halfWidth,4,halfWidth*2,height-8);
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
        [self.capsule addSubview:self.selection];
        for(NSString *key in HGATabIDs()) {
            UIButton *b=HGADockButton();
            b.accessibilityIdentifier=[@"honggo.appearance.tab." stringByAppendingString:key];
            b.tag=[HGATabIDs() indexOfObject:key];
            [b addTarget:self action:@selector(tabTapped:) forControlEvents:UIControlEventTouchUpInside];
            [self.capsule addSubview:b];self.buttons[key]=b;
        }
        self.selectionPan=[[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(dragSelection:)];
        self.selectionPan.delegate=self;
        self.selectionPan.maximumNumberOfTouches=1;
        self.extra=[UIView new];
        self.extraButton=HGADockButton();
        self.extraButton.accessibilityIdentifier=@"honggo.appearance.dock.extra";
        [self.extraButton addTarget:self action:@selector(extraTapped:) forControlEvents:UIControlEventTouchUpInside];
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
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gesture shouldReceiveTouch:(UITouch *)touch {
    return gesture!=self.selectionPan ||
        [self acceptsSelectionTouchAtPoint:[touch locationInView:self.capsule] inView:touch.view];
}
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gesture shouldBeRequiredToFailByGestureRecognizer:(UIGestureRecognizer *)other {
    if(gesture!=self.selectionPan || !gesture.enabled || other.view.window!=self.window ||
        ![other.view isDescendantOfView:gesture.view] || ![other isKindOfClass:UIPanGestureRecognizer.class])return NO;
    id player=other.delegate;
    return [player isKindOfClass:NSClassFromString(@"FQVShortVideoVPlayerControlView")] &&
        other==HGReadObjectGetter(player,"progressPanGesture");
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
    return gesture!=self.selectionPan && other!=self.selectionPan;
}
- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
    if(self.userInteractionEnabled && [self pointInside:point withEvent:event] &&
        HGADockContainsPoint(self.capsule,[self convertPoint:point toView:self.capsule])) {
        for(UIButton *button in self.buttons.allValues)
            if(!button.hidden && button.enabled && [button pointInside:[self convertPoint:point toView:button] withEvent:event])
                return button;
        return self.capsule;
    }
    for(UIView *hit=[super hitTest:point withEvent:event];hit && hit!=self;hit=hit.superview)
        if(hit==self.extraButton || hit==self.expandButton)return [(UIButton *)hit isEnabled] ? hit : nil;
    return nil;
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
    if(self.rendering || self.restoring)return;self.rendering=YES;
    @try {
        UIView *bar=self.hostBar;
        if(!HGAppearanceDockEnabled() || !bar.window || !HGAVisible(bar)) {[self restore];return;}
        CGFloat controlHeight=HGADockControlHeight(bar);
        if(bar.bounds.size.width<180 || controlHeight<44 || ![self resolveTargets]) {
            [self restore];
            HGRecordCapability(@"appearance.dock.navigation",@{@"module":@"appearance",@"state":@"unavailable",@"reason":@"native tab actions could not be mapped; original navigation retained"});
            return;
        }
        NSUInteger visibleCount=0;
        for(NSString *key in HGATabIDs()) if(!HGAppearanceFlag([@"dock.hide." stringByAppendingString:key]))visibleCount++;
        CGFloat extraWidth=HGAppearanceFlag(@"dock.extra") ?
            MIN(HGAppearanceNumber(@"dock.extraSize"),controlHeight)+HGADockAccessoryGap : 0;
        if(bar.bounds.size.width-8 < visibleCount*44+extraWidth) {
            [self restore];
            HGRecordCapability(@"appearance.dock.navigation",@{@"module":@"appearance",@"state":@"unavailable",@"reason":@"insufficient space for accessible tab targets; native navigation retained"});
            return;
        }
        if(self.superview!=bar)[bar addSubview:self];
        if(self.selectionPan.view!=bar.superview)[bar.superview addGestureRecognizer:self.selectionPan];
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
        self.hidden=NO;
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
    NSMutableArray *visible=[NSMutableArray array];
    for(NSString *key in HGATabIDs())if(!HGAppearanceFlag([@"dock.hide." stringByAppendingString:key]))[visible addObject:key];
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
    for(UIView *surface in @[self.capsule,self.extra,self.expand]) {
        surface.overrideUserInterfaceStyle=surfaceStyle;
        if(@available(iOS 17.0,*))[surface updateTraitsIfNeeded];
        surface.layer.cornerCurve=kCACornerCurveContinuous;
    }
    UIColor *tint=HGAppearanceColor(HGAppearanceString(@"dock.tint"),nil);
    BOOL reduced=UIAccessibilityIsReduceTransparencyEnabled();
    UIColor *base=tint ?: (translucent ? [UIColor colorWithWhite:light?1:0.10 alpha:1] : UIColor.secondarySystemBackgroundColor);
    UIColor *fill=[base colorWithAlphaComponent:!translucent || reduced ? 1 : (tint ? 0.30 : 0)];
    UIView *capsuleContent=HGADockMaterialContent(self.capsule,style,self.collapsed?22:radius,fill);
    UIView *extraContent=HGADockMaterialContent(self.extra,style,circle/2,fill);
    UIView *expandContent=HGADockMaterialContent(self.expand,style,22,fill);
    // UIKit can adapt dynamic label colors to the material's own background
    // when the foreground belongs to the visual effect's contentView.
    if(self.selection.superview!=capsuleContent)[capsuleContent insertSubview:self.selection atIndex:0];
    if(self.extraButton.superview!=extraContent)[extraContent addSubview:self.extraButton];
    if(self.expandButton.superview!=expandContent)[expandContent addSubview:self.expandButton];
    self.extraButton.frame=self.extra.bounds;
    self.expandButton.frame=self.expand.bounds;
    self.extraButton.hidden=self.extra.hidden;
    self.expandButton.hidden=self.expand.hidden;
    self.capsule.hidden=self.collapsed;
    self.selectionPan.enabled=!self.collapsed && !UIAccessibilityIsVoiceOverRunning();
    self.capsule.layer.cornerRadius=radius;
    self.capsule.layer.borderWidth=translucent && !classicBlur ? 0.5 : 0;
    self.capsule.layer.borderColor=[UIColor colorWithWhite:light?0:1 alpha:0.14].CGColor;
    BOOL darkSurface=self.capsule.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark;
    if(!self.dragLens) {
        self.selection.backgroundColor=classicBlur && !reduced ?
            [UIColor colorWithWhite:light?0:1 alpha:light?0.08:0.12] : (translucent && !reduced ?
            [UIColor colorWithWhite:0 alpha:(light?0.12:0.30)] :
            [UIColor colorWithWhite:darkSurface?1:0 alpha:(darkSurface?0.18:0.08)]);
    }
    CGRect selectedFrame=CGRectNull;
    NSString *content=HGAppearanceString(@"dock.content");
    CGFloat slotInset=MIN(14,MAX(0,(expanded.size.width-54*visible.count)/2));
    CGFloat slotWidth=(expanded.size.width-2*slotInset)/visible.count;
    CGFloat unselectedOpacity=self.traitCollection.accessibilityContrast==UIAccessibilityContrastHigh ? 0.94 : 0.80;
    CGFloat requestedIcon=HGAppearanceNumber(@"dock.iconSize");
    for(NSString *key in self.buttons) {
        UIButton *b=self.buttons[key];NSUInteger index=[HGATabIDs() indexOfObject:key];
        if(b.superview!=capsuleContent)[capsuleContent addSubview:b];
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
        CGFloat iconHeight=title.length ? height-11-font.lineHeight : height*0.48;
        CGFloat iconSize=MAX(12,MIN(requestedIcon,MIN(iconHeight,slotWidth-16)));
        b.enabled=source.enabled;b.selected=active;
        HGADockConfigureButton(b,title,[content isEqual:@"text"]?nil:image,iconSize,font,color);
        if(active) {
            CGFloat titleWidth=title.length ? [title sizeWithAttributes:@{NSFontAttributeName:font}].width : 0;
            CGFloat preferredWidth=MIN(slotWidth+20,MAX(height*1.52,titleWidth+24));
            selectedFrame=HGADockSelectionFrame(CGRectGetMidX(b.frame),expanded.size.width,height,preferredWidth);
        }
        b.accessibilityLabel=HGATabTitles()[index];
        b.accessibilityTraits=UIAccessibilityTraitButton | (active?UIAccessibilityTraitSelected:0);
    }
    if(self.selectionDragging && !self.collapsed)[self positionDragSelection:self.selectionDragX];
    else {
        self.selection.hidden=CGRectIsNull(selectedFrame);
        if(!self.selection.hidden)[self placeSelection:selectedFrame];
    }
    NSString *action=HGAppearanceString(@"dock.action");
    NSString *symbol=[action isEqual:@"settings"]?@"slider.horizontal.3":([action isEqual:@"search"]?@"magnifyingglass":HGATabSymbols()[[HGATabIDs() indexOfObject:action]]);
    UIColor *customColor=HGAppearanceColor(HGAppearanceString(@"dock.normal"),nil);
    UIColor *accessoryColor=customColor ?: UIColor.labelColor;
    UIFont *accessoryFont=[UIFont systemFontOfSize:11];
    HGADockConfigureButton(self.extraButton,nil,[UIImage systemImageNamed:symbol],MIN(requestedIcon,circle*0.48),accessoryFont,accessoryColor);
    HGADockConfigureButton(self.expandButton,nil,[UIImage systemImageNamed:@"ellipsis"],MIN(requestedIcon,22),accessoryFont,accessoryColor);
    if(@available(iOS 17.0,*)) {
        for(UIView *surface in @[self.capsule,self.extra,self.expand])[surface updateTraitsIfNeeded];
    }
    self.extraButton.accessibilityLabel=[action isEqual:@"settings"]?@"Honggo 外观设置":([action isEqual:@"search"]?@"搜索":HGATabTitles()[[HGATabIDs() indexOfObject:action]]);
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
        [self clearSelectionDrag];
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
    if(sender.tag<0 || sender.tag>=4)return;
    UIControl *target=[self.targets objectForKey:HGATabIDs()[sender.tag]];
    if(!target || !target.enabled)return;
    UIControlEvents event=(target.allControlEvents & UIControlEventTouchUpInside)?UIControlEventTouchUpInside:UIControlEventPrimaryActionTriggered;
    [target sendActionsForControlEvents:event];[self feedback];
    dispatch_async(dispatch_get_main_queue(),^{[self refresh];});
}
- (void)placeSelection:(CGRect)frame {
    self.selection.frame=frame;
    CGFloat radius=MIN(frame.size.width,frame.size.height)/2;
    self.selection.layer.cornerRadius=radius;
    self.dragLens.frame=self.selection.bounds;
    self.dragLens.layer.cornerRadius=radius;
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
        [self placeSelection:HGADockSelectionFrame(MIN(last,MAX(first,x)),self.capsule.bounds.size.width,height,preferredWidth)];
    }
    return destination;
}
- (void)clearSelectionDrag {
    self.selectionDragging=NO;
    [self.dragLens removeFromSuperview];
    self.dragLens=nil;
}
- (void)dragSelection:(UIPanGestureRecognizer *)gesture {
    CGFloat position=[gesture locationInView:self.capsule].x;
    if(gesture.state==UIGestureRecognizerStateBegan) {
        self.selectionDragging=YES;
        [self.selection.layer removeAllAnimations];
        [self positionDragSelection:position];
        NSString *style=HGAppearanceString(@"dock.material");
        if(HGNativeGlassConfigured() && !UIAccessibilityIsReduceTransparencyEnabled() &&
            ([style isEqual:@"regular"] || [style isEqual:@"clear"])) {
            HGAAppearanceMaterial *lens=[[HGAAppearanceMaterial alloc] initWithEffect:nil];
            lens.accessibilityIdentifier=@"honggo.appearance.dock.drag-lens";
            lens.style=@"clear";lens.interactiveGlass=YES;lens.overlayColor=UIColor.clearColor;
            lens.frame=self.selection.bounds;lens.layer.cornerRadius=self.selection.layer.cornerRadius;
            lens.layer.cornerCurve=kCACornerCurveContinuous;lens.clipsToBounds=YES;
            lens.userInteractionEnabled=NO;lens.accessibilityElementsHidden=YES;
            self.dragLens=lens;
            self.selection.backgroundColor=UIColor.clearColor;
            [self.selection addSubview:lens];
        }
    }else if(self.selectionDragging) {
        if(gesture.state==UIGestureRecognizerStateChanged){[self positionDragSelection:position];return;}
        if(gesture.state==UIGestureRecognizerStateEnded) {
            UIButton *destination=[self positionDragSelection:position];
            if(destination && !destination.selected)[self tabTapped:destination];
        }
        [self clearSelectionDrag];
        if(!self.superview || self.collapsed)return;
        if(UIAccessibilityIsReduceMotionEnabled())[self renderGeometry];
        else [UIView animateWithDuration:0.22 delay:0
            options:UIViewAnimationOptionBeginFromCurrentState|UIViewAnimationOptionAllowUserInteraction
            animations:^{[self renderGeometry];} completion:nil];
    }
}
- (void)extraTapped:(UIButton *)sender {
    NSString *action=HGAppearanceString(@"dock.action");
    if([action isEqual:@"settings"]) { HGOpenAppearance(self.hostController ?: HGAOwner(self.hostBar),nil);return; }
    NSUInteger index=[HGATabIDs() indexOfObject:action];
    if(index!=NSNotFound){UIButton *proxy=[UIButton new];proxy.tag=index;[self tabTapped:proxy];return;}
    if(![action isEqual:@"search"])return;
    NSMutableArray *queue=[NSMutableArray array];
    UIViewController *root=self.hostBar.window.rootViewController;if(root)[queue addObject:root];
    for(NSUInteger i=0;i<queue.count && i<40;i++) {
        UIViewController *vc=queue[i];
        if([NSStringFromClass(vc.class) isEqual:@"SSVideoSeriesFeedViewController"] && HGAVisible(vc.viewIfLoaded)) {
            SEL sel=NSSelectorFromString(@"simpleSearchViewDidClicked");Method m=class_getInstanceMethod(vc.class,sel);
            if(m && !strcmp(method_getTypeEncoding(m),"v16@0:8")){((void(*)(id,SEL))objc_msgSend)(vc,sel);[self feedback];return;}
        }
        for(UIViewController *child in vc.childViewControllers)if(queue.count<40 && ![queue containsObject:child])[queue addObject:child];
        if(vc.presentedViewController && queue.count<40 && ![queue containsObject:vc.presentedViewController])[queue addObject:vc.presentedViewController];
    }
    UIAccessibilityPostNotification(UIAccessibilityAnnouncementNotification,@"当前页面没有已适配的搜索入口，请返回首页搜索。");
}
- (void)expandTapped:(UIButton *)button {[self setCollapsed:NO animated:YES];}
- (void)swipe:(UISwipeGestureRecognizer *)gesture {
    if(HGAppearanceFlag(@"dock.collapse"))[self setCollapsed:gesture.direction==UISwipeGestureRecognizerDirectionUp animated:YES];
}
- (void)setCollapsed:(BOOL)value animated:(BOOL)animated {
    if(!HGAppearanceFlag(@"dock.collapse") || UIAccessibilityIsVoiceOverRunning())value=NO;
    if(self.collapsed==value)return;
    [self layoutIfNeeded];self.collapsed=value;
    if(value)[self clearSelectionDrag];
    // Public UIKit animation; no private platter/visual-provider dependency.
    if(animated && !UIAccessibilityIsReduceMotionEnabled()) {
        [UIView transitionWithView:self duration:0.22 options:UIViewAnimationOptionTransitionCrossDissolve|UIViewAnimationOptionAllowUserInteraction
            animations:^{[self renderGeometry];} completion:nil];
    }else [self renderGeometry];
}
@end
BOOL HGAppearanceDockIsMutating(UIView *bar) {
    HGAAppearanceDock *dock=objc_getAssociatedObject(bar,&HGADockKey);
    return dock.rendering || dock.restoring;
}
void HGAppearanceLayoutDock(UIView *bar,UIViewController *controller) {
    if(!NSThread.isMainThread || !bar)return;
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
    CGFloat y=[gesture translationInView:gesture.view].y;if(fabs(y)<40)return;
    for(UIView *bar in HGABars.allObjects)if(bar.window==gesture.view.window)
        [(HGAAppearanceDock *)objc_getAssociatedObject(bar,&HGADockKey) setCollapsed:y<0 animated:YES];
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
