@interface HGAAppearanceDock : UIView <UIGestureRecognizerDelegate>
@property(nonatomic,weak) UIView *hostBar;
@property(nonatomic,weak) UIViewController *hostController;
@property(nonatomic,strong) NSMapTable<NSString *,UIControl *> *targets;
@property(nonatomic,strong) NSMapTable<UIView *,NSNumber *> *hiddenSources;
@property(nonatomic,strong) UIColor *hostBackground;
@property(nonatomic) BOOL backgroundOwned;
@property(nonatomic,strong) UIView *capsule;
@property(nonatomic,strong) UIButton *extra;
@property(nonatomic,strong) UIButton *expand;
@property(nonatomic,strong) NSMutableDictionary<NSString *,UIButton *> *buttons;
@property(nonatomic,strong) NSArray<UISwipeGestureRecognizer *> *swipes;
@property(nonatomic) BOOL collapsed;
@property(nonatomic) BOOL rendering;
- (void)refresh;
- (void)restore;
- (void)setCollapsed:(BOOL)value animated:(BOOL)animated;
@end
static char HGADockKey;
static NSArray *HGATabIDs(void){return @[@"home",@"theater",@"store",@"mine"];}
static NSArray *HGATabTitles(void){return @[@"首页",@"剧场",@"商城",@"我的"];}
static NSArray *HGATabSymbols(void){return @[@"house",@"play.rectangle",@"bag",@"person.crop.circle"];}
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
        for(NSString *key in HGATabIDs()) {
            UIButton *b=[UIButton buttonWithType:UIButtonTypeSystem];
            b.accessibilityIdentifier=[@"honggo.appearance.tab." stringByAppendingString:key];
            b.tag=[HGATabIDs() indexOfObject:key];
            [b addTarget:self action:@selector(tabTapped:) forControlEvents:UIControlEventTouchUpInside];
            [self.capsule addSubview:b];self.buttons[key]=b;
        }
        self.extra=[UIButton buttonWithType:UIButtonTypeSystem];
        self.extra.accessibilityIdentifier=@"honggo.appearance.dock.extra";
        [self.extra addTarget:self action:@selector(extraTapped:) forControlEvents:UIControlEventTouchUpInside];
        [self addSubview:self.extra];
        self.expand=[UIButton buttonWithType:UIButtonTypeSystem];
        self.expand.accessibilityIdentifier=@"honggo.appearance.dock.expand";
        self.expand.accessibilityLabel=@"展开导航栏";
        [self.expand setImage:[UIImage systemImageNamed:@"ellipsis"] forState:UIControlStateNormal];
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
    if(self.rendering)return;self.rendering=YES;
    @try {
        UIView *bar=self.hostBar;
        if(!HGAppearanceDockEnabled() || !bar.window || !HGAVisible(bar)) {[self restore];return;}
        if(bar.bounds.size.width<180 || bar.bounds.size.height<44 || ![self resolveTargets]) {
            [self restore];
            HGRecordCapability(@"appearance.dock.navigation",@{@"module":@"appearance",@"state":@"unavailable",@"reason":@"native tab actions could not be mapped; original navigation retained"});
            return;
        }
        if(self.superview!=bar)[bar addSubview:self];
        if(!CGRectEqualToRect(self.frame,bar.bounds))self.frame=bar.bounds;
        if(!self.backgroundOwned || ![bar.backgroundColor isEqual:UIColor.clearColor])self.hostBackground=bar.backgroundColor;
        self.backgroundOwned=YES;bar.backgroundColor=UIColor.clearColor;
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
    HGAInstallGlass(self.capsule,style,self.collapsed?22:radius,0.08);
    HGAInstallGlass(self.extra,style,circle/2,0.08);
    HGAInstallGlass(self.expand,style,22,0.08);
    self.capsule.hidden=self.collapsed;
    UIColor *tint=HGAppearanceColor(HGAppearanceString(@"dock.tint"),nil);
    HGAState(self.capsule,NO).glass.contentView.backgroundColor=tint ? [tint colorWithAlphaComponent:0.30] :
        [UIColor.secondarySystemBackgroundColor colorWithAlphaComponent:[style isEqual:@"solid"] || UIAccessibilityIsReduceTransparencyEnabled()?1:0.08];
    NSInteger selected=HGASelectedTab(self.hostController);
    for(NSString *key in self.buttons) {
        UIButton *b=self.buttons[key];NSUInteger index=[HGATabIDs() indexOfObject:key];
        NSUInteger position=[visible indexOfObject:key];
        b.hidden=position==NSNotFound;
        if(b.hidden)continue;
        b.frame=CGRectMake(position*expanded.size.width/visible.count,0,expanded.size.width/visible.count,height);
        UIControl *source=[self.targets objectForKey:key];
        BOOL active=selected!=NSNotFound ? selected==(NSInteger)index : source.selected;
        NSString *content=HGAppearanceString(@"dock.content");
        UIColor *color=HGAppearanceColor(HGAppearanceString(active?@"dock.selected":@"dock.normal"),
            active?UIColor.labelColor:UIColor.secondaryLabelColor);
        UIButtonConfiguration *config=[UIButtonConfiguration plainButtonConfiguration];
        config.title=[content isEqual:@"icon"]?nil:HGATabTitles()[index];
        NSString *symbol=HGATabSymbols()[index];
        UIImage *image=[UIImage systemImageNamed:active?[symbol stringByAppendingString:@".fill"]:symbol] ?: [UIImage systemImageNamed:symbol];
        config.image=[content isEqual:@"text"]?nil:image;
        config.preferredSymbolConfigurationForImage=[UIImageSymbolConfiguration configurationWithPointSize:HGAppearanceNumber(@"dock.iconSize") weight:active?UIImageSymbolWeightSemibold:UIImageSymbolWeightRegular];
        config.imagePlacement=NSDirectionalRectEdgeTop;config.imagePadding=3;
        config.baseForegroundColor=color;config.contentInsets=NSDirectionalEdgeInsetsMake(3,0,3,0);
        config.titleTextAttributesTransformer=^NSDictionary *(NSDictionary *attributes){
            NSMutableDictionary *updated=attributes.mutableCopy;updated[NSFontAttributeName]=[UIFont systemFontOfSize:[content isEqual:@"text"]?15:11 weight:active?UIFontWeightSemibold:UIFontWeightRegular];return updated;
        };
        b.configuration=config;b.enabled=source.enabled;b.selected=active;
        b.accessibilityLabel=HGATabTitles()[index];
        b.accessibilityTraits=UIAccessibilityTraitButton | (active?UIAccessibilityTraitSelected:0);
    }
    NSString *action=HGAppearanceString(@"dock.action");
    NSString *symbol=[action isEqual:@"settings"]?@"slider.horizontal.3":([action isEqual:@"search"]?@"magnifyingglass":HGATabSymbols()[[HGATabIDs() indexOfObject:action]]);
    [self.extra setImage:[UIImage systemImageNamed:symbol] forState:UIControlStateNormal];
    self.extra.accessibilityLabel=[action isEqual:@"settings"]?@"Honggo 外观设置":([action isEqual:@"search"]?@"搜索":HGATabTitles()[[HGATabIDs() indexOfObject:action]]);
    self.extra.tintColor=UIColor.labelColor;self.expand.tintColor=UIColor.labelColor;
}
- (void)restore {
    for(UIView *view in self.hiddenSources.keyEnumerator)if(view.hidden)view.hidden=[[self.hiddenSources objectForKey:view] boolValue];
    [self.hiddenSources removeAllObjects];
    if(self.backgroundOwned && [self.hostBar.backgroundColor isEqual:UIColor.clearColor])self.hostBar.backgroundColor=self.hostBackground;
    self.backgroundOwned=NO;self.collapsed=NO;
    [self removeFromSuperview];
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
