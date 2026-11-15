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
@property(nonatomic,strong) UIButton *extra;
@property(nonatomic,strong) UIButton *expand;
@property(nonatomic,strong) NSMutableDictionary<NSString *,UIButton *> *buttons;
@property(nonatomic,strong) NSArray<UISwipeGestureRecognizer *> *swipes;
@property(nonatomic) BOOL collapsed;
@property(nonatomic) BOOL rendering;
@property(nonatomic) BOOL restoring;
- (void)refresh;
- (void)restore;
- (void)setCollapsed:(BOOL)value animated:(BOOL)animated;
@end
static char HGADockKey;
static NSArray *HGATabIDs(void){return @[@"home",@"theater",@"store",@"mine"];}
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
@implementation HGAAppearanceDock
- (instancetype)initWithFrame:(CGRect)frame {
    if((self=[super initWithFrame:frame])) {
        self.accessibilityIdentifier=@"honggo.appearance.dock";
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
        self.extra=HGADockButton();
        self.extra.accessibilityIdentifier=@"honggo.appearance.dock.extra";
        [self.extra addTarget:self action:@selector(extraTapped:) forControlEvents:UIControlEventTouchUpInside];
        [self addSubview:self.extra];
        self.expand=HGADockButton();
        self.expand.accessibilityIdentifier=@"honggo.appearance.dock.expand";
        self.expand.accessibilityLabel=@"展开导航栏";
        [self.expand addTarget:self action:@selector(expandTapped:) forControlEvents:UIControlEventTouchUpInside];
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
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)g shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)other{return YES;}
- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
    UIView *hit=[super hitTest:point withEvent:event];
    return hit==self ? nil : hit;
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
        if(bar.bounds.size.width<180 || bar.bounds.size.height<44 || ![self resolveTargets]) {
            [self restore];
            HGRecordCapability(@"appearance.dock.navigation",@{@"module":@"appearance",@"state":@"unavailable",@"reason":@"native tab actions could not be mapped; original navigation retained"});
            return;
        }
        NSUInteger visibleCount=0;
        for(NSString *key in HGATabIDs()) if(!HGAppearanceFlag([@"dock.hide." stringByAppendingString:key]))visibleCount++;
        CGFloat extraWidth=HGAppearanceFlag(@"dock.extra") ? MIN(HGAppearanceNumber(@"dock.extraSize"),bar.bounds.size.height)+8 : 0;
        if(bar.bounds.size.width-8 < visibleCount*44+extraWidth) {
            [self restore];
            HGRecordCapability(@"appearance.dock.navigation",@{@"module":@"appearance",@"state":@"unavailable",@"reason":@"insufficient space for accessible tab targets; native navigation retained"});
            return;
        }
        if(self.superview!=bar)[bar addSubview:self];
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
    if(width<180 || availableHeight<44)return;
    NSMutableArray *visible=[NSMutableArray array];
    for(NSString *key in HGATabIDs())if(!HGAppearanceFlag([@"dock.hide." stringByAppendingString:key]))[visible addObject:key];
    BOOL extra=HGAppearanceFlag(@"dock.extra");
    CGFloat circle=MIN(HGAppearanceNumber(@"dock.extraSize"),availableHeight);
    CGFloat height=MIN(HGAppearanceNumber(@"dock.height"),availableHeight);
    CGFloat reserve=extra ? circle+8 : 0;
    CGFloat total=MIN(width-8,MAX(visible.count*44+reserve,HGAppearanceNumber(@"dock.width")*width));
    CGFloat safeBottom=MIN(self.hostBar.safeAreaInsets.bottom,MAX(0,availableHeight-height));
    CGFloat y=MAX(0,availableHeight-safeBottom-height-HGAppearanceNumber(@"dock.lift"));
    CGFloat x=(width-total)/2;
    CGRect expanded=CGRectMake(x,y,total-reserve,height);
    CGRect compact=CGRectMake(width-48,y,44,44);
    NSString *anchor=HGAppearanceString(@"dock.anchor");
    if([anchor isEqual:@"left"])compact.origin.x=4;
    if([anchor isEqual:@"center"])compact.origin.x=(width-44)/2;
    self.capsule.frame=self.collapsed?compact:expanded;
    self.expand.frame=compact;self.expand.hidden=!self.collapsed;
    self.extra.frame=CGRectMake(x+total-circle,y+(height-circle)/2,circle,circle);
    self.extra.hidden=!extra || self.collapsed;
    NSString *style=HGAppearanceString(@"dock.material");
    CGFloat radius=MIN(height/2,HGAppearanceNumber(@"dock.radius"));
    // A translucent dock can sit over a video even while the host uses its
    // light theme. Keep this small, plugin-owned surface dark with white glyphs;
    // solid material continues to follow the host's light/dark appearance.
    BOOL translucent=![style isEqual:@"solid"];
    UIUserInterfaceStyle surfaceStyle=translucent ? UIUserInterfaceStyleDark : UIUserInterfaceStyleUnspecified;
    for(UIView *surface in @[self.capsule,self.extra,self.expand]) {
        surface.overrideUserInterfaceStyle=surfaceStyle;
        surface.layer.cornerCurve=kCACornerCurveContinuous;
    }
    HGAInstallGlass(self.capsule,style,self.collapsed?22:radius,0.08);
    HGAInstallGlass(self.extra,style,circle/2,0.08);
    HGAInstallGlass(self.expand,style,22,0.08);
    // UIKit may reorder a UIButton's configuration content after our background
    // was inserted. These two buttons belong entirely to Honggo: keep their
    // material behind every normal content layer so it cannot blur the glyph.
    // Do not reorder or change z positions on any host-owned view.
    for(UIButton *accessory in @[self.extra,self.expand]) {
        CALayer *materialLayer=HGAState(accessory,NO).glass.layer;
        if(materialLayer.zPosition!=-1)materialLayer.zPosition=-1;
    }
    self.capsule.hidden=self.collapsed;
    UIColor *tint=HGAppearanceColor(HGAppearanceString(@"dock.tint"),nil);
    UIColor *surfaceColor=translucent ? [UIColor colorWithWhite:0.10 alpha:1] : UIColor.secondarySystemBackgroundColor;
    CGFloat surfaceAlpha=[style isEqual:@"solid"] || UIAccessibilityIsReduceTransparencyEnabled()?1:0.12;
    HGASetGlassOverlay(self.capsule,tint ? [tint colorWithAlphaComponent:0.30] :
        [surfaceColor colorWithAlphaComponent:surfaceAlpha]);
    HGASetGlassOverlay(self.extra,[surfaceColor colorWithAlphaComponent:surfaceAlpha]);
    HGASetGlassOverlay(self.expand,[surfaceColor colorWithAlphaComponent:surfaceAlpha]);
    self.capsule.layer.cornerRadius=radius;
    self.capsule.layer.borderWidth=translucent ? 0.5 : 0;
    self.capsule.layer.borderColor=[UIColor colorWithWhite:1 alpha:0.14].CGColor;
    BOOL darkSurface=translucent || self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark;
    self.selection.backgroundColor=[UIColor colorWithWhite:darkSurface?1:0 alpha:darkSurface?0.18:0.08];
    self.selection.hidden=YES;
    NSInteger selected=HGASelectedTab(self.hostController);
    NSString *content=HGAppearanceString(@"dock.content");
    CGFloat slotWidth=expanded.size.width/visible.count;
    CGFloat requestedIcon=HGAppearanceNumber(@"dock.iconSize");
    for(NSString *key in self.buttons) {
        UIButton *b=self.buttons[key];NSUInteger index=[HGATabIDs() indexOfObject:key];
        NSUInteger position=[visible indexOfObject:key];
        b.hidden=position==NSNotFound;
        if(b.hidden)continue;
        b.frame=CGRectMake(position*slotWidth,0,slotWidth,height);
        UIControl *source=[self.targets objectForKey:key];
        BOOL active=selected!=NSNotFound ? selected==(NSInteger)index : source.selected;
        UIColor *color=HGAppearanceColor(HGAppearanceString(active?@"dock.selected":@"dock.normal"),
            translucent ? [UIColor colorWithWhite:1 alpha:active?1:0.88] : (active?UIColor.labelColor:UIColor.secondaryLabelColor));
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
            CGFloat pillWidth=MIN(slotWidth-8,MAX(height*1.25,titleWidth+24));
            self.selection.frame=CGRectMake(CGRectGetMidX(b.frame)-pillWidth/2,4,pillWidth,height-8);
            self.selection.layer.cornerRadius=(height-8)/2;
            self.selection.hidden=NO;
        }
        b.accessibilityLabel=HGATabTitles()[index];
        b.accessibilityTraits=UIAccessibilityTraitButton | (active?UIAccessibilityTraitSelected:0);
    }
    NSString *action=HGAppearanceString(@"dock.action");
    NSString *symbol=[action isEqual:@"settings"]?@"slider.horizontal.3":([action isEqual:@"search"]?@"magnifyingglass":HGATabSymbols()[[HGATabIDs() indexOfObject:action]]);
    UIColor *accessoryColor=HGAppearanceColor(HGAppearanceString(@"dock.normal"),translucent?UIColor.whiteColor:UIColor.labelColor);
    UIFont *accessoryFont=[UIFont systemFontOfSize:11];
    HGADockConfigureButton(self.extra,nil,[UIImage systemImageNamed:symbol],MIN(requestedIcon,circle*0.48),accessoryFont,accessoryColor);
    HGADockConfigureButton(self.expand,nil,[UIImage systemImageNamed:@"ellipsis"],MIN(requestedIcon,22),accessoryFont,accessoryColor);
    self.extra.accessibilityLabel=[action isEqual:@"settings"]?@"Honggo 外观设置":([action isEqual:@"search"]?@"搜索":HGATabTitles()[[HGATabIDs() indexOfObject:action]]);
}
- (void)restore {
    if(self.restoring)return;
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
        for(NSArray *entry in restorations) {
            UIView *view=entry[0];
            if(view.hidden)view.hidden=[entry[1] boolValue];
        }
        if(backgroundOwned && [bar.backgroundColor isEqual:UIColor.clearColor])bar.backgroundColor=background;
        if(backgroundOwned && !bar.opaque)bar.opaque=opaque;
        [self removeFromSuperview];
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
