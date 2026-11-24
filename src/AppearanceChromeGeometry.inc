@protocol HGAHeaderSelecting <NSObject>
@property(nonatomic) NSInteger selectedSegmentIndex;
@end

// Each glass surface is also its button's hit target. UIKit owns the optical
// response and press animation; the host still commits the channel change.
API_AVAILABLE(ios(26.0))
@interface HGAHeaderGlassChannels : UIControl <HGAHeaderSelecting>
@property(nonatomic,copy) NSArray<NSString *> *titles;
@property(nonatomic,copy) NSArray<UIButton *> *buttons;
@property(nonatomic) BOOL clearGlass;
- (instancetype)initWithTitles:(NSArray<NSString *> *)titles clearGlass:(BOOL)clear;
@end
@implementation HGAHeaderGlassChannels
@synthesize selectedSegmentIndex=_selectedSegmentIndex;
- (instancetype)initWithTitles:(NSArray<NSString *> *)titles clearGlass:(BOOL)clear {
    if ((self=[super initWithFrame:CGRectZero])) {
        _titles=[titles copy];
        _clearGlass=clear;
        _selectedSegmentIndex=UISegmentedControlNoSegment;
        self.isAccessibilityElement=NO;
        UIGlassContainerEffect *material=[UIGlassContainerEffect new];
        material.spacing=8;
        UIVisualEffectView *group=[[UIVisualEffectView alloc] initWithEffect:material];
        group.frame=self.bounds;
        group.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
        [self addSubview:group];
        UIStackView *row=[[UIStackView alloc] initWithFrame:group.bounds];
        row.distribution=UIStackViewDistributionFillEqually;
        row.spacing=2;
        row.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
        [group.contentView addSubview:row];
        NSMutableArray *buttons=[NSMutableArray arrayWithCapacity:titles.count];
        for (NSUInteger index=0;index<titles.count;index++) {
            UIButton *button=[UIButton buttonWithType:UIButtonTypeSystem];
            button.tag=(NSInteger)index;
            [self configureButton:button selected:NO];
            [button addTarget:self action:@selector(choose:) forControlEvents:UIControlEventPrimaryActionTriggered];
            [button addTarget:self action:@selector(finishDrag:event:) forControlEvents:UIControlEventTouchUpOutside];
            [row addArrangedSubview:button];
            [buttons addObject:button];
        }
        _buttons=[buttons copy];
    }
    return self;
}
- (void)configureButton:(UIButton *)button selected:(BOOL)selected {
    UIButtonConfiguration *configuration;
    if (selected) configuration=self.clearGlass ? UIButtonConfiguration.prominentClearGlassButtonConfiguration :
        UIButtonConfiguration.prominentGlassButtonConfiguration;
    else configuration=self.clearGlass ? UIButtonConfiguration.clearGlassButtonConfiguration :
        UIButtonConfiguration.glassButtonConfiguration;
    configuration.cornerStyle=UIButtonConfigurationCornerStyleCapsule;
    configuration.automaticallyUpdateForSelection=NO;
    configuration.contentInsets=NSDirectionalEdgeInsetsZero;
    configuration.baseForegroundColor=UIColor.whiteColor;
    if (selected) configuration.baseBackgroundColor=[UIColor colorWithWhite:0.76 alpha:0.45];
    configuration.attributedTitle=[[NSAttributedString alloc] initWithString:self.titles[button.tag] attributes:@{
        NSFontAttributeName:[UIFont systemFontOfSize:13 weight:selected ? UIFontWeightSemibold : UIFontWeightRegular]}];
    button.configuration=configuration;
    button.selected=selected;
}
- (void)setSelectedSegmentIndex:(NSInteger)index {
    if (_selectedSegmentIndex==index) return;
    NSInteger previous=_selectedSegmentIndex;
    _selectedSegmentIndex=index;
    if (previous>=0 && previous<(NSInteger)self.buttons.count) [self configureButton:self.buttons[previous] selected:NO];
    if (index>=0 && index<(NSInteger)self.buttons.count) [self configureButton:self.buttons[index] selected:YES];
}
- (BOOL)isTracking {
    for (UIButton *button in self.buttons) if (button.isTracking) return YES;
    return [super isTracking];
}
- (void)choose:(UIButton *)button {
    if (button.tag==self.selectedSegmentIndex) return;
    self.selectedSegmentIndex=button.tag;
    [self sendActionsForControlEvents:UIControlEventValueChanged];
}
- (void)finishDrag:(UIButton *)origin event:(UIEvent *)event {
    UITouch *touch=[event touchesForView:origin].anyObject;
    if (!touch) return;
    for (UIButton *button in self.buttons) {
        if (button!=origin && CGRectContainsPoint(button.bounds,[touch locationInView:button])) {
            [self choose:button];
            break;
        }
    }
}
@end

@class HGAHeaderToolButton;
@interface HGAHeaderInsetState : NSObject
@property(nonatomic,strong) HGAAppearanceMaterial *sidebarGlass;
@property(nonatomic,strong) HGAAppearanceMaterial *searchGlass;
@property(nonatomic,strong) HGAHeaderToolButton *sidebarButton;
@property(nonatomic,strong) HGAHeaderToolButton *searchButton;
@property(nonatomic,strong) UIControl<HGAHeaderSelecting> *channelControl;
@property(nonatomic,copy) NSString *channelStyle;
@property(nonatomic,weak) UICollectionView *channelCollection;
@property(nonatomic,copy) NSArray<NSNumber *> *channelIndices;
@property(nonatomic,copy) NSArray<NSString *> *channelTitles;
@property(nonatomic,weak) UIView *categories;
@property(nonatomic,copy) NSArray *observedTitles;
@property(nonatomic) CGSize observedSize;
@property(nonatomic,weak) UICollectionView *observedCollection;
@property(nonatomic,strong) NSHashTable<UIView *> *visibleSideTools;
@property(nonatomic) NSUInteger channelSelectionDepth;
@property(nonatomic) BOOL ownsSideToolTransition;
- (void)activateTool:(HGAHeaderToolButton *)sender;
- (void)changeChannel:(UIControl<HGAHeaderSelecting> *)sender;
@end
static char HGAHeaderInsetKey;

static UIViewController *HGAHeaderToolOwner(UIView *source,SEL selector) {
    UIViewController *owner=HGAOwner(source);
    if (![owner isKindOfClass:objc_lookUpClass("SSVideoSeriesFeedViewController")]) return nil;
    Method method=class_getInstanceMethod(object_getClass(owner),selector);
    return method && !strcmp(method_getTypeEncoding(method),"v16@0:8") ? owner : nil;
}

@interface HGAHeaderToolButton : UIButton
@property(nonatomic,weak) UIView *source;
@property(nonatomic) BOOL previousAccessibilityHidden;
@property(nonatomic) BOOL previousAccessibilityElement;
@property(nonatomic,copy) NSString *glassStyle;
@end
@implementation HGAHeaderToolButton
@synthesize source=_source;
- (void)setSource:(UIView *)source {
    if (_source==source) return;
    if (_source.accessibilityElementsHidden) _source.accessibilityElementsHidden=self.previousAccessibilityHidden;
    if (!_source.isAccessibilityElement) _source.isAccessibilityElement=self.previousAccessibilityElement;
    _source=source;
    self.previousAccessibilityHidden=source.accessibilityElementsHidden;
    self.previousAccessibilityElement=source.isAccessibilityElement;
    source.accessibilityElementsHidden=YES;
    source.isAccessibilityElement=NO;
}
- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent *)event {
    return [super pointInside:point withEvent:event] &&
        [[UIBezierPath bezierPathWithOvalInRect:self.bounds] containsPoint:point];
}
@end

@implementation HGAHeaderInsetState
- (void)activateTool:(HGAHeaderToolButton *)sender {
    BOOL sidebar=sender==self.sidebarButton;
    UIView *source=sender.source;
    if (!sender.window || !HGAVisible(source) || HGFeedSidebarOwnsLayout(sender.window)) return;
    SEL selector=sel_registerName(sidebar ? "simpleSideBarViewDidClicked" : "simpleSearchViewDidClicked");
    UIViewController *owner=HGAHeaderToolOwner(source,selector);
    if (owner) ((void (*)(id,SEL))objc_msgSend)(owner,selector);
}
- (void)changeChannel:(UIControl<HGAHeaderSelecting> *)sender {
    UIView *categories=self.categories;
    NSInteger next=sender.selectedSegmentIndex;
    if (!sender.window || !categories.window || next<0 || next>=(NSInteger)self.channelIndices.count ||
        HGFeedSidebarOwnsLayout(sender.window)) return;
    SEL selected=sel_registerName("selectedIndex"), select=sel_registerName("selectItemAtIndex:needAnimation:");
    NSInteger current=((NSInteger (*)(id,SEL))objc_msgSend)(categories,selected);
    NSInteger index=self.channelIndices[next].integerValue;
    if (current!=index) ((BOOL (*)(id,SEL,NSInteger,BOOL))objc_msgSend)(categories,select,index,NO);
    if (sender!=self.channelControl || categories!=self.categories) return;
    current=((NSInteger (*)(id,SEL))objc_msgSend)(categories,selected);
    NSUInteger settled=[self.channelIndices indexOfObject:@(current)];
    if (settled!=NSNotFound && sender.selectedSegmentIndex!=(NSInteger)settled)
        sender.selectedSegmentIndex=(NSInteger)settled;
}
@end

static UIView *HGAHeaderSearchSlotView(UIView *header) {
    UIView *search=HGAViewGetter(header,"searchViewContainer",header);
    if(!search)return nil;
    UIView *slot=HGAViewGetter(search,"searchButtonSlotView",search) ?: HGAViewGetter(search,"searchButtonView",search);
    if(slot && slot!=search)return slot;
    // Fallback for host builds where the right search icon wrapper is an
    // anonymous ~24x24 subview inside SSVideoSeriesFeedHeaderSearchView.
    CGFloat midX=CGRectGetMidX(search.bounds);
    UIView *best=nil;
    for(UIView *sub in search.subviews) {
        if(sub.hidden || sub.alpha<0.01 || HGAOwnView(sub))continue;
        CGFloat w=CGRectGetWidth(sub.frame),h=CGRectGetHeight(sub.frame);
        if(w>=18.0 && w<=64.0 && h>=18.0 && h<=44.0 && CGRectGetMidX(sub.frame)>midX) {
            if(!best || CGRectGetMidX(sub.frame)>CGRectGetMidX(best.frame))best=sub;
        }
    }
    return best;
}

// The host uses UICollectionViewFlowLayout to hit-test channel indices and its
// own two target-frame getters to drive the native underline during selection
// and horizontal paging. Keep all three in collection-content coordinates.
@interface HGAHeaderChannelLayout : NSObject
@property(nonatomic,weak) UICollectionView *collection;
@property(nonatomic,copy) NSArray<NSString *> *titles;
@property(nonatomic,copy) NSArray<NSValue *> *frames;
@property(nonatomic) NSUInteger hiddenMask;
@property(nonatomic) CGFloat width;
@property(nonatomic) CGFloat inset;
@property(nonatomic) CGRect capsule;
@end
@implementation HGAHeaderChannelLayout @end

static char HGAHeaderLayoutKey, HGAHeaderLineRestoreKey, HGAHeaderPagerKey, HGAHeaderSettlementKey;
static BOOL HGAHeaderAdaptersReady;
static IMP HGAHeaderOriginals[6];
static IMP HGAHeaderOriginalSelectCell;
static void HGARefreshHeaderChannelLine(UIView *categories,BOOL completed);

@interface HGAFixedChannelPan : UIPanGestureRecognizer
@property(nonatomic,weak) UIPanGestureRecognizer *nativePan;
@end
@implementation HGAFixedChannelPan
- (BOOL)canPreventGestureRecognizer:(UIGestureRecognizer *)other {
    return other==self.nativePan;
}
@end

@interface HGAFixedChannelPager : NSObject <UIGestureRecognizerDelegate>
@property(nonatomic,weak) UIView *categories;
@property(nonatomic,strong) HGAFixedChannelPan *pan;
@property(nonatomic) BOOL suspendsNativePan;
- (void)updateNativePan;
- (void)detach;
- (BOOL)isFiltering;
@end
@implementation HGAFixedChannelPager
- (void)dealloc { [self detach]; }

- (void)updateNativePan {
    UIScrollView *scroll=(UIScrollView *)self.pan.view;
    if (scroll.scrollEnabled && self.pan.nativePan.enabled) {
        self.suspendsNativePan=YES;
        self.pan.nativePan.enabled=NO;
    }
}

- (void)detach {
    UIScrollView *scroll=(UIScrollView *)self.pan.view;
    BOOL suspended=self.suspendsNativePan;
    self.suspendsNativePan=NO;
    if (self.pan.view) [scroll removeGestureRecognizer:self.pan];
    if (suspended && scroll.scrollEnabled) self.pan.nativePan.enabled=YES;
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gesture shouldBeRequiredToFailByGestureRecognizer:(UIGestureRecognizer *)other {
    return other==self.pan.nativePan;
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gesture shouldReceiveTouch:(UITouch *)touch {
    Class progress = objc_lookUpClass("FQVShortVideoProgressView");
    BOOL control=NO, episode=NO;
    for (UIView *view=touch.view; view && view!=gesture.view; view=view.superview) {
        if ([view isKindOfClass:progress]) return NO;
        control |= [view isKindOfClass:UIControl.class];
        episode |= [HGARole(view) isEqual:@"episodes"];
    }
    // A stationary touch still triggers the native episode button. A sideways
    // drag over that button belongs to the filtered home pager.
    return !control || episode;
}

- (BOOL)isFiltering {
    UIView *categories = self.categories;
    UIScrollView *scroll = (UIScrollView *)self.pan.view;
    return HGAppearanceEnabled() && categories.window && categories.window==scroll.window && scroll.scrollEnabled &&
        self.suspendsNativePan && !HGFeedSidebarOwnsLayout(scroll.window) &&
        HGAIsHomeChannel(categories);
}

- (BOOL)gestureRecognizerShouldBegin:(UIPanGestureRecognizer *)gesture {
    if (![self isFiltering] || ((UIScrollView *)self.pan.view).isDecelerating) return NO;
    CGPoint velocity = [gesture velocityInView:self.pan.view];
    return fabs(velocity.x)>fabs(velocity.y);
}

- (void)page:(UIPanGestureRecognizer *)gesture {
    if (gesture.state!=UIGestureRecognizerStateEnded) return;
    UIView *categories = self.categories;
    HGAHeaderChannelLayout *layout = objc_getAssociatedObject(categories,&HGAHeaderLayoutKey);
    if (!layout || ![self isFiltering] || ((UIScrollView *)self.pan.view).isDecelerating) return;
    CGPoint travel = [gesture translationInView:self.pan.view];
    CGPoint velocity = [gesture velocityInView:self.pan.view];
    if (fabs(travel.x)<=fabs(travel.y) || (fabs(travel.x)<28 && fabs(velocity.x)<300)) return;

    NSInteger index = ((NSInteger (*)(id,SEL))objc_msgSend)(categories,sel_registerName("selectedIndex"));
    NSInteger step = travel.x<0 ? 1 : -1;
    if (index<0 || index>=(NSInteger)layout.titles.count) return;
    for (NSInteger next=index+step; next>=0 && next<(NSInteger)layout.titles.count; next+=step) {
        if (layout.hiddenMask & (1UL << next)) continue;
        // Commit via the host's selection and list lifecycle with its native
        // index. Animated offsets expose the intervening hidden pages.
        if (((BOOL (*)(id,SEL,NSInteger,BOOL))objc_msgSend)(categories,sel_registerName("selectItemAtIndex:needAnimation:"),next,NO))
            HGARefreshHeaderChannelLine(categories,YES);
        break;
    }
}
@end

static BOOL HGAFilteredHomePagerContainsView(UIView *view) {
    for (; view && view!=view.window; view=view.superview) {
        if (![view isKindOfClass:UIScrollView.class]) continue;
        for (UIGestureRecognizer *recognizer in view.gestureRecognizers)
            if ([recognizer isKindOfClass:HGAFixedChannelPan.class] &&
                [(HGAFixedChannelPager *)recognizer.delegate isFiltering]) return YES;
    }
    return NO;
}

typedef BOOL (^HGALeftEpisodeDecision)(UIPanGestureRecognizer *gesture);
static char HGAEpisodeSwipeDecisionKey;

static void HGAWrapEpisodeSwipeDecision(UIViewController *owner) {
    if (![owner isKindOfClass:UIViewController.class]) return;
    SEL get=sel_registerName("ss_shouldBeginPushInteractiveTransitionDirectionHorizontalBlock");
    SEL set=sel_registerName("setSs_shouldBeginPushInteractiveTransitionDirectionHorizontalBlock:");
    Method getter=class_getInstanceMethod(object_getClass(owner),get);
    Method setter=class_getInstanceMethod(object_getClass(owner),set);
    if (!getter || !setter || strcmp(method_getTypeEncoding(getter),"@?16@0:8") ||
        strcmp(method_getTypeEncoding(setter),"v24@0:8@?16")) return;

    HGALeftEpisodeDecision original=((HGALeftEpisodeDecision(*)(id,SEL))objc_msgSend)(owner,get);
    if (!original || original==objc_getAssociatedObject(owner,&HGAEpisodeSwipeDecisionKey)) return;
    HGALeftEpisodeDecision decision=[^BOOL(UIPanGestureRecognizer *gesture) {
        if (HGAppearanceEnabled() && HGAppearanceFlag(@"player.disableEpisodeSwipe")) return NO;
        return original(gesture);
    } copy];
    objc_setAssociatedObject(owner,&HGAEpisodeSwipeDecisionKey,decision,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    ((void(*)(id,SEL,HGALeftEpisodeDecision))objc_msgSend)(owner,set,decision);
}

static void HGAInstallEpisodeSwipeSetup(Class target,SEL selector,BOOL argumentIsContext,IMP *installed) {
    if (*installed) return;
    Method method=class_getInstanceMethod(target,selector);
    if (!method || strcmp(method_getTypeEncoding(method),"v24@0:8@16")) return;
    __block IMP original=method_getImplementation(method);
    IMP replacement=imp_implementationWithBlock(^void(id receiver,id argument) {
        ((void(*)(id,SEL,id))original)(receiver,selector,argument);
        HGAWrapEpisodeSwipeDecision(argumentIsContext ? HGReadObjectGetter(argument,"viewController") : argument);
    });
    IMP previous=HGInstallVerifiedHook(target,selector,"v24@0:8@16",replacement,@"appearance");
    if (previous) {
        original=previous;
        *installed=previous;
    } else imp_removeBlock(replacement);
}

static void HGAInstallHomeEpisodeSwipeGate(void) {
    static IMP listSetup,homeSetup;
    // The home controller registers a separate interactive push that resolves
    // its destination through SSBizShortVideo; it bypasses the list delegate.
    HGAInstallEpisodeSwipeSetup(object_getClass(objc_lookUpClass("SSBizVideoImpl")),
        sel_registerName("setLeftSlideToShortsListVCWithParentVC:"),NO,&homeSetup);
    HGAInstallEpisodeSwipeSetup(objc_lookUpClass("SSShortVideoGestureComponent"),
        sel_registerName("setupPanGestureTowardsLeftEnterOtherPage:"),YES,&listSetup);

    static IMP original;
    if (original) return;
    Class delegateClass=objc_lookUpClass("SSShortVideoPanGestureLeftTowardDelegate");
    SEL selector=sel_registerName("shouldLeftSlideEnterOtherPage:context:");
    Method method=class_getInstanceMethod(delegateClass,selector);
    if (!method || strcmp(method_getTypeEncoding(method),"B32@0:8@16@24")) return;

    original=method_getImplementation(method);
    IMP replacement=imp_implementationWithBlock(^BOOL(id delegate,id navigation,id context) {
        // The host's interactive push consults this gate before it opens
        // episode detail. Explicit taps use their original action instead.
        if (NSThread.isMainThread && HGAppearanceEnabled()) {
            if (HGAppearanceFlag(@"player.disableEpisodeSwipe")) return NO;
            UIViewController *owner=HGReadObjectGetter(context,"viewController");
            if ([owner isKindOfClass:UIViewController.class]) {
                UIView *view=owner.viewIfLoaded;
                if (view.window && [owner isKindOfClass:objc_lookUpClass("SSVideoSeriesFeedViewController")]) {
                    UIView *container=HGAViewGetter(owner,"containerView",view);
                    view=HGAViewGetter(container,"contentScrollView",container);
                }
                if (view.window && HGAFilteredHomePagerContainsView(view)) {
                    if (HGEnhancedDiagnostics()) HGLog(@"appearance.channel",
                        @{@"selector":NSStringFromSelector(selector),@"phase":@"deferred-to-channel"});
                    return NO;
                }
            }
        }
        return ((BOOL(*)(id,SEL,id,id))original)(delegate,selector,navigation,context);
    });
    IMP previous=HGInstallVerifiedHook(delegateClass,selector,"B32@0:8@16@24",replacement,@"appearance");
    if (previous) original=previous;
    else {
        imp_removeBlock(replacement);
        original=NULL;
    }
}

static UIScrollView *HGAHomeChannelScroll(id categories) {
    Class cls = objc_lookUpClass("FQRJXCategoryListContainerView");
    UIView *container = HGReadObjectGetter(categories,"listContainer");
    UIScrollView *scroll = [container isKindOfClass:cls] ? HGReadObjectGetter(container,"contentScrollView") : nil;
    if (![scroll isKindOfClass:UIScrollView.class] || ![scroll isDescendantOfView:container]) return nil;
    return scroll;
}

static void HGAUpdateChannelPager(UIView *categories,HGAHeaderChannelLayout *layout) {
    HGAFixedChannelPager *state = objc_getAssociatedObject(categories,&HGAHeaderPagerKey);
    UIScrollView *scroll = layout ? HGAHomeChannelScroll(categories) : nil;
    if (state.pan.view==scroll) {
        [state updateNativePan];
        return;
    }
    [state detach];
    objc_setAssociatedObject(categories,&HGAHeaderPagerKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    if (!scroll) return;
    Method select = class_getInstanceMethod(categories.class,sel_registerName("selectItemAtIndex:needAnimation:"));
    if (!select || strcmp(method_getTypeEncoding(select),"B28@0:8q16B24")) return;
    state = [HGAFixedChannelPager new];
    state.categories = categories;
    state.pan = [[HGAFixedChannelPan alloc] initWithTarget:state action:@selector(page:)];
    state.pan.delegate = state;
    state.pan.nativePan = scroll.panGestureRecognizer;
    state.pan.maximumNumberOfTouches = 1;
    [scroll addGestureRecognizer:state.pan];
    objc_setAssociatedObject(categories,&HGAHeaderPagerKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    // A native pan settling on a 0.01pt-hidden title still exposes that page.
    // The mounted filtered pager uses the host's selection API on release.
    [state updateNativePan];
}

static CGSize HGAChannelSize(id owner, SEL cmd, UICollectionView *collection, UICollectionViewLayout *layout, NSIndexPath *path) {
    CGSize size = ((CGSize (*)(id,SEL,id,id,id))HGAHeaderOriginals[0])(owner,cmd,collection,layout,path);
    HGAHeaderChannelLayout *plan = objc_getAssociatedObject(owner,&HGAHeaderLayoutKey);
    if (collection == plan.collection && path.section == 0 && path.item >= 0 && path.item < (NSInteger)plan.frames.count)
        size.width = plan.frames[path.item].CGRectValue.size.width;
    else if(path.section==0 && path.item>=0 && path.item<32 &&
        ([objc_getAssociatedObject(owner,&HGAFinderCategoryMaskKey) unsignedIntegerValue] & (1UL<<path.item)))
        size.width=0.01;
    return size;
}

static CGFloat HGAChannelSpacing(id owner, SEL cmd, UICollectionView *collection, UICollectionViewLayout *layout, NSInteger section) {
    HGAHeaderChannelLayout *plan = objc_getAssociatedObject(owner,&HGAHeaderLayoutKey);
    if (plan && collection == plan.collection && section == 0) return 0;
    NSUInteger slot = cmd == @selector(collectionView:layout:minimumInteritemSpacingForSectionAtIndex:) ? 5 : 1;
    return ((CGFloat (*)(id,SEL,id,id,NSInteger))HGAHeaderOriginals[slot])(owner,cmd,collection,layout,section);
}

static UIEdgeInsets HGAChannelInsets(id owner, SEL cmd, UICollectionView *collection, UICollectionViewLayout *layout, NSInteger section) {
    UIEdgeInsets insets = ((UIEdgeInsets (*)(id,SEL,id,id,NSInteger))HGAHeaderOriginals[2])(owner,cmd,collection,layout,section);
    HGAHeaderChannelLayout *plan = objc_getAssociatedObject(owner,&HGAHeaderLayoutKey);
    if (plan && collection == plan.collection && section == 0)
        insets.left = insets.right = plan.inset;
    return insets;
}

static CGRect HGACompactTargetFrame(id owner, NSInteger index, CGRect frame) {
    HGAHeaderChannelLayout *plan = objc_getAssociatedObject(owner,&HGAHeaderLayoutKey);
    if (index >= 0 && index < (NSInteger)plan.frames.count && !CGRectIsEmpty(frame)) {
        CGRect item = plan.frames[index].CGRectValue;
        if (plan.hiddenMask & (1UL << index)) frame.size.width = item.size.width;
        frame.origin.x = CGRectGetMidX(item) - frame.size.width / 2;
    }
    return frame;
}
static CGRect HGAChannelFrame(id owner, SEL cmd, NSInteger index) {
    CGRect native = ((CGRect (*)(id,SEL,NSInteger))HGAHeaderOriginals[3])(owner,cmd,index);
    return HGACompactTargetFrame(owner,index,native);
}
static CGRect HGAChannelSelectedFrame(id owner, SEL cmd, NSInteger index, NSUInteger type) {
    CGRect native = ((CGRect (*)(id,SEL,NSInteger,NSUInteger))HGAHeaderOriginals[4])(owner,cmd,index,type);
    return HGACompactTargetFrame(owner,index,native);
}

static BOOL HGAChannelSelectCell(UIView *categories, SEL cmd, NSInteger index, NSUInteger type, BOOL animated) {
    HGAHeaderChannelLayout *plan=objc_getAssociatedObject(categories,&HGAHeaderLayoutKey);
    BOOL compact=plan && categories.window && HGAppearanceEnabled() && HGAIsHomeChannel(categories);
    if (compact && index>=0 && index<(NSInteger)plan.frames.count && (plan.hiddenMask & (1UL<<index))) return NO;
    UIView *header=compact ? categories.superview : nil;
    while (header && !HGAIsHeaderView(header)) header=header.superview;
    HGAHeaderInsetState *state=compact ? objc_getAssociatedObject(header,&HGAHeaderInsetKey) : nil;
    BOOL selected=NO;
    if (state) state.channelSelectionDepth++;
    @try {
        selected=((BOOL (*)(id,SEL,NSInteger,NSUInteger,BOOL))HGAHeaderOriginalSelectCell)(categories,cmd,index,type,animated);
        if (state) HGAppearanceProcessView(header);
    } @finally {
        if (state) state.channelSelectionDepth--;
    }
    if (compact && selected && !animated) HGARefreshHeaderChannelLine(categories,YES);
    return selected;
}

static void HGAInstallHeaderLayoutAdapters(void) {
    if (HGAHeaderAdaptersReady) return;
    Class cls = NSClassFromString(@"SSHGBookStoreCategoryTitleView");
    if (!cls) return;
    static const struct { const char *name, *encoding; IMP replacement; } adapters[] = {
        {"collectionView:layout:sizeForItemAtIndexPath:", "{CGSize=dd}40@0:8@16@24@32", (IMP)HGAChannelSize},
        {"collectionView:layout:minimumLineSpacingForSectionAtIndex:", "d40@0:8@16@24q32", (IMP)HGAChannelSpacing},
        {"collectionView:layout:insetForSectionAtIndex:", "{UIEdgeInsets=dddd}40@0:8@16@24q32", (IMP)HGAChannelInsets},
        {"getTargetCellFrame:", "{CGRect={CGPoint=dd}{CGSize=dd}}24@0:8q16", (IMP)HGAChannelFrame},
        {"getTargetSelectedCellFrame:selectedType:", "{CGRect={CGPoint=dd}{CGSize=dd}}32@0:8q16Q24", (IMP)HGAChannelSelectedFrame},
        // 7.4.5.33 forwards both delegate spacing queries to innerCellSpacing.
        {"collectionView:layout:minimumInteritemSpacingForSectionAtIndex:", "d40@0:8@16@24q32", (IMP)HGAChannelSpacing}
    };
    static const struct { const char *name, *encoding; } properties[] = {
        {"titles", "@16@0:8"}, {"indicators", "@16@0:8"}, {"collectionView", "@16@0:8"},
        {"contentScrollView", "@16@0:8"}, {"selectedIndex", "q16@0:8"}
    };
    for (NSUInteger i = 0; i < sizeof(properties)/sizeof(properties[0]); i++) {
        Method m = class_getInstanceMethod(cls,sel_registerName(properties[i].name));
        if (!m || strcmp(method_getTypeEncoding(m),properties[i].encoding)) return;
    }
    for (NSUInteger i = 0; i < sizeof(adapters)/sizeof(adapters[0]); i++) {
        Method m = class_getInstanceMethod(cls,sel_registerName(adapters[i].name));
        if (!m || strcmp(method_getTypeEncoding(m),adapters[i].encoding)) return;
    }
    for (NSUInteger i = 0; i < sizeof(adapters)/sizeof(adapters[0]); i++) {
        SEL selector = sel_registerName(adapters[i].name);
        if (!HGAHeaderOriginals[i])
            HGAHeaderOriginals[i] = class_getMethodImplementation(cls,selector);
        IMP original = HGInstallVerifiedHook(cls,selector,adapters[i].encoding,adapters[i].replacement,@"appearance");
        if (!original || class_getMethodImplementation(cls,selector) != adapters[i].replacement) return;
        HGAHeaderOriginals[i] = original;
    }
    SEL select=sel_registerName("selectCellAtIndex:selectedType:needAnimation:");
    const char *selectEncoding="B36@0:8q16Q24B32";
    Method selection=class_getInstanceMethod(cls,select);
    if (selection && !strcmp(method_getTypeEncoding(selection),selectEncoding)) {
        HGAHeaderOriginalSelectCell=method_getImplementation(selection);
        HGAHeaderOriginalSelectCell=HGInstallVerifiedHook(cls,select,selectEncoding,(IMP)HGAChannelSelectCell,@"appearance");
    }
    HGAHeaderAdaptersReady = YES;
}

static BOOL HGAHeaderScrollInFlight(id categories, UIScrollView *collection) {
    UIScrollView *pager = HGAHomeChannelScroll(categories) ?: HGReadObjectGetter(categories,"contentScrollView");
    return collection.isTracking || collection.isDragging || collection.isDecelerating ||
        ([pager isKindOfClass:UIScrollView.class] && (pager.isTracking || pager.isDragging || pager.isDecelerating));
}

// A host can populate its titles after header layout. Notify the parent once
// for an actual change, not on each cell refresh or each scroll offset.
static void HGAHeaderCategoryGeometryChanged(UIView *categories) {
    UIView *header = categories.superview;
    while (header && !HGAIsHeaderView(header)) header = header.superview;
    HGAHeaderInsetState *state = objc_getAssociatedObject(header,&HGAHeaderInsetKey);
    if (!state || !HGAHeaderAdaptersReady) return;
    UICollectionView *collection = HGReadObjectGetter(categories,"collectionView");
    if (![collection isKindOfClass:UICollectionView.class] || HGAHeaderScrollInFlight(categories,collection)) return;
    NSArray *titles = HGReadObjectGetter(categories,"titles");
    if (![titles isKindOfClass:NSArray.class] || titles.count > 32) return;
    CGSize size = collection.bounds.size;
    if (state.observedCollection == collection && CGSizeEqualToSize(state.observedSize,size) &&
        [state.observedTitles isEqualToArray:titles]) return;
    state.observedCollection = collection;
    state.observedTitles = titles;
    state.observedSize = size;
    [header setNeedsLayout];
}

// Capture original sizes through the verified flow delegate, never by moving or
// measuring reused cells. A nearly zero extent retains hidden entries' indices.
static HGAHeaderChannelLayout *HGAMakeChannelLayout(UIView *categories, UICollectionView *collection,
    NSArray *titles, NSUInteger mask) {
    CGFloat width = collection.bounds.size.width;
    NSUInteger visibleCount = titles.count - (NSUInteger)__builtin_popcountll(mask);
    CGFloat halfGap = visibleCount > 1 ? 10 : 0;
    NSMutableArray *frames = [NSMutableArray arrayWithCapacity:titles.count];
    CGFloat total = 0;
    for (NSUInteger i = 0; i < titles.count; i++) {
        CGFloat extent = 0.01;
        if (!(mask & (1UL << i))) {
            NSIndexPath *path = [NSIndexPath indexPathForItem:i inSection:0];
            CGSize size = ((CGSize (*)(id,SEL,id,id,id))HGAHeaderOriginals[0])(categories,
                @selector(collectionView:layout:sizeForItemAtIndexPath:), collection, collection.collectionViewLayout, path);
            if (!isfinite(size.width) || size.width <= 0 || size.height <= 0) return nil;
            extent = size.width + 2 * halfGap;
        }
        [frames addObject:[NSValue valueWithCGRect:CGRectMake(total,0,extent,collection.bounds.size.height)]];
        total += extent;
    }
    if (!visibleCount || !isfinite(width) || total > width) return nil;
    CGFloat inset = (width - total) / 2;
    for (NSUInteger i = 0; i < frames.count; i++)
        frames[i] = [NSValue valueWithCGRect:CGRectOffset([frames[i] CGRectValue],inset,0)];
    HGAHeaderChannelLayout *plan = [HGAHeaderChannelLayout new];
    plan.collection = collection;
    plan.frames = frames;
    plan.titles = titles;
    plan.hiddenMask = mask;
    plan.width = width;
    plan.inset = inset;
    plan.capsule = CGRectInset(CGRectMake(inset,0,total,collection.bounds.size.height),halfGap-16,0);
    return plan;
}

static void HGARefreshHeaderChannelLine(UIView *categories,BOOL completed) {
    HGAHeaderChannelLayout *plan = objc_getAssociatedObject(categories,&HGAHeaderLayoutKey);
    BOOL restoring = objc_getAssociatedObject(categories,&HGAHeaderLineRestoreKey) != nil;
    if ((!plan && !restoring) || !categories.window || HGFeedSidebarOwnsLayout(categories.window)) return;
    UICollectionView *collection = plan.collection ?: HGReadObjectGetter(categories,"collectionView");
    if (![collection isKindOfClass:UICollectionView.class] || !collection.numberOfSections ||
        HGAHeaderScrollInFlight(categories,collection)) return;
    UIView *header = categories.superview;
    while (header && !HGAIsHeaderView(header)) header = header.superview;
    BOOL animating = NO;
    if (HGABoolGetter(header,@"isHeaderViewLayoutAnimating",&animating) && animating) return;

    NSInteger index = ((NSInteger (*)(id,SEL))objc_msgSend)(categories,sel_registerName("selectedIndex"));
    if (index < 0 || index >= [collection numberOfItemsInSection:0] ||
        (plan && (index >= (NSInteger)plan.frames.count || (plan.hiddenMask & (1UL << index))))) return;
    CGRect target = plan ? plan.frames[index].CGRectValue :
        HGAChannelFrame(categories,sel_registerName("getTargetCellFrame:"),index);
    if (CGRectIsEmpty(target)) return;
    NSArray *lines = HGReadObjectGetter(categories,"indicators");
    if (![lines isKindOfClass:NSArray.class]) return;
    BOOL waitingForAnimation = NO;
    for (UIView *line in lines) {
        if (![line isKindOfClass:NSClassFromString(@"FQRJXCategoryIndicatorLineView")] ||
            ![line isDescendantOfView:collection]) continue;
        if (line.layer.animationKeys.count && !(completed && plan)) {
            waitingForAnimation = YES;
            continue;
        }
        CGPoint center = line.center;
        CGFloat x = [collection convertPoint:CGPointMake(CGRectGetMidX(target),CGRectGetMidY(target))
                                      toView:line.superview].x;
        if (!isfinite(x)) continue;
        if (completed && plan) [line.layer removeAllAnimations];
        if (fabs(x-center.x) > 0.25)
            [UIView performWithoutAnimation:^{line.center=CGPointMake(x,center.y);}];
    }
    if (restoring && !waitingForAnimation)
        objc_setAssociatedObject(categories,&HGAHeaderLineRestoreKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

static void HGASyncHeaderChannelLine(UIView *categories) {
    HGARefreshHeaderChannelLine(categories,NO);
    for (UIView *header=categories.superview;header;header=header.superview) {
        if (!HGAIsHeaderView(header)) continue;
        HGAHeaderInsetState *state=objc_getAssociatedObject(header,&HGAHeaderInsetKey);
        UIControl<HGAHeaderSelecting> *control=state.channelControl;
        if (state.categories==categories && control && !control.isTracking) {
            NSInteger selected=((NSInteger (*)(id,SEL))objc_msgSend)(categories,sel_registerName("selectedIndex"));
            NSUInteger segment=[state.channelIndices indexOfObject:@(selected)];
            if (segment!=NSNotFound && control.selectedSegmentIndex!=(NSInteger)segment)
                control.selectedSegmentIndex=(NSInteger)segment;
        }
        break;
    }
}

static BOOL HGAHiddenChannelIndex(HGAHeaderChannelLayout *plan,NSInteger index) {
    return index>=0 && index<(NSInteger)plan.frames.count && (plan.hiddenMask & (1UL<<index));
}

static void HGASettleHomeChannel(UIView *categories) {
    HGAHeaderChannelLayout *plan=objc_getAssociatedObject(categories,&HGAHeaderLayoutKey);
    if (!plan || !categories.window || !HGAppearanceEnabled() ||
        HGFeedSidebarOwnsLayout(categories.window) || !HGAIsHomeChannel(categories) ||
        HGAHeaderScrollInFlight(categories,plan.collection)) return;
    HGAFixedChannelPager *state=objc_getAssociatedObject(categories,&HGAHeaderPagerKey);
    [state updateNativePan];
    NSInteger selected=((NSInteger (*)(id,SEL))objc_msgSend)(categories,sel_registerName("selectedIndex"));
    if (HGAHiddenChannelIndex(plan,selected)) {
        NSInteger nearest=NSNotFound;
        for (NSInteger index=0;index<(NSInteger)plan.frames.count;index++) {
            if (!(plan.hiddenMask & (1UL<<index)) &&
                (nearest==NSNotFound || labs(index-selected)<labs(nearest-selected))) nearest=index;
        }
        SEL select=sel_registerName("selectItemAtIndex:needAnimation:");
        Method method=class_getInstanceMethod(categories.class,select);
        if (nearest!=NSNotFound && method && !strcmp(method_getTypeEncoding(method),"B28@0:8q16B24") &&
            ((BOOL (*)(id,SEL,NSInteger,BOOL))objc_msgSend)(categories,select,nearest,NO)) selected=nearest;
    }
    UIScrollView *scroll=HGAHomeChannelScroll(categories);
    CGFloat stride=scroll ? CGRectGetWidth(scroll.bounds) : 0;
    if (selected>=0 && selected<(NSInteger)plan.frames.count && !HGAHiddenChannelIndex(plan,selected) && stride>0) {
        CGFloat page=scroll.contentOffset.x/stride;
        UIView *container=HGReadObjectGetter(categories,"listContainer");
        SEL current=sel_registerName("currentIndex"), click=sel_registerName("didClickSelectedItemAtIndex:");
        Method currentMethod=class_getInstanceMethod(object_getClass(container),current);
        BOOL hasCurrent=currentMethod && !strcmp(method_getTypeEncoding(currentMethod),"q16@0:8");
        NSInteger nativeIndex=hasCurrent ? ((NSInteger (*)(id,SEL))objc_msgSend)(container,current) : NSNotFound;
        BOOL hiddenOffset=isfinite(page) && page>=0 && page<(CGFloat)plan.frames.count &&
            HGAHiddenChannelIndex(plan,lround(page));
        if (hiddenOffset || HGAHiddenChannelIndex(plan,nativeIndex)) {
            // The selection veto can preserve the previous visible title
            // while the native scroll view has already reached a hidden page.
            // Restore its verified width × index position, then let the list
            // container finalize the transition if scrolling did not do so.
            [scroll setContentOffset:CGPointMake(stride*selected,scroll.contentOffset.y) animated:NO];
            Method clickMethod=class_getInstanceMethod(object_getClass(container),click);
            if (hasCurrent && ((NSInteger (*)(id,SEL))objc_msgSend)(container,current)!=selected &&
                clickMethod && !strcmp(method_getTypeEncoding(clickMethod),"v24@0:8q16"))
                ((void (*)(id,SEL,NSInteger))objc_msgSend)(container,click,selected);
        }
    }
    HGAApplyChannelIndicators(categories,YES);
    HGARefreshHeaderChannelLine(categories,YES);
}

static void HGAScheduleHomeChannelSettlement(UIView *categories) {
    if (!categories.window || objc_getAssociatedObject(categories,&HGAHeaderSettlementKey)) return;
    objc_setAssociatedObject(categories,&HGAHeaderSettlementKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    __weak UIView *weakCategories=categories;
    dispatch_async(dispatch_get_main_queue(),^{
        UIView *live=weakCategories;
        if (!live) return;
        HGASettleHomeChannel(live);
        objc_setAssociatedObject(live,&HGAHeaderSettlementKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    });
}

static void HGAHomeChannelPagerDidSettle(UIView *container) {
    UIScrollView *scroll=(id)HGAViewGetter(container,"contentScrollView",container);
    if (![scroll isKindOfClass:UIScrollView.class]) return;
    for (UIGestureRecognizer *recognizer in scroll.gestureRecognizers) {
        if (![recognizer isKindOfClass:HGAFixedChannelPan.class]) continue;
        HGAFixedChannelPager *state=(id)recognizer.delegate;
        if (state.categories) HGAScheduleHomeChannelSettlement(state.categories);
    }
}

static void HGASetChannelLayout(UIView *categories, HGAHeaderChannelLayout *next) {
    HGAHeaderChannelLayout *old = objc_getAssociatedObject(categories,&HGAHeaderLayoutKey);
    if (old != next) {
        objc_setAssociatedObject(categories,&HGAHeaderLayoutKey,next,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(categories,&HGAHeaderLineRestoreKey,next ? nil : @YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        [old.collection.collectionViewLayout invalidateLayout];
        if (old.collection != next.collection) [next.collection.collectionViewLayout invalidateLayout];
    }
    HGAUpdateChannelPager(categories,next);
    HGASyncHeaderChannelLine(categories);
    if (next && old != next) HGAScheduleHomeChannelSettlement(categories);
}

static void HGACompactHeaderChannels(UIView *header, HGAHeaderInsetState *state) {
    BOOL animating=NO;
    if (HGABoolGetter(header,@"isHeaderViewLayoutAnimating",&animating) && animating) return;
    UIView *tabs = HGAViewGetter(header,"tabContainer",header);
    UIView *categories = nil;
    Class cls = NSClassFromString(@"SSHGBookStoreCategoryTitleView");
    if (HGAHeaderAdaptersReady && tabs && !tabs.hidden) {
        for (UIView *node in HGADescendants(tabs,64)) if ([node isKindOfClass:cls]) { categories=node; break; }
    }
    if (state.categories != categories) {
        HGASetChannelLayout(state.categories,nil);
        state.categories = categories;
    }
    if (!categories) return;
    HGAHeaderChannelLayout *old = objc_getAssociatedObject(categories,&HGAHeaderLayoutKey);
    NSArray *titles = HGReadObjectGetter(categories,"titles");
    UICollectionView *collection = (id)HGAViewGetter(categories,"collectionView",categories);
    UICollectionViewFlowLayout *flow = [collection isKindOfClass:UICollectionView.class] ? (id)collection.collectionViewLayout : nil;
    if (![flow isKindOfClass:UICollectionViewFlowLayout.class] ||
        flow.scrollDirection != UICollectionViewScrollDirectionHorizontal || !CGSizeEqualToSize(flow.estimatedItemSize,CGSizeZero) ||
        collection.delegate != (id)categories ||
        collection.numberOfSections != 1 || ![titles isKindOfClass:NSArray.class] || !titles.count || titles.count > 32 ||
        [collection numberOfItemsInSection:0] != (NSInteger)titles.count) {
        HGASetChannelLayout(categories,nil);
        return;
    }
    if (HGAHeaderScrollInFlight(categories,collection)) return;
    state.observedCollection = collection;
    state.observedTitles = titles;
    state.observedSize = collection.bounds.size;
    NSUInteger mask = 0;
    for (NSUInteger i = 0; i < titles.count; i++) {
        if ([titles[i] isKindOfClass:NSString.class] && HGATopChannelHidden(titles[i])) mask |= 1UL << i;
    }
    if (!mask || (NSUInteger)__builtin_popcountll(mask) == titles.count) {
        HGASetChannelLayout(categories,nil);
        return;
    }
    if (old.collection == collection && old.hiddenMask == mask && old.width == collection.bounds.size.width &&
        [old.titles isEqualToArray:titles]) {
        HGASetChannelLayout(categories,old);
        return;
    }
    HGASetChannelLayout(categories,HGAMakeChannelLayout(categories,collection,titles,mask));
}

static void HGAConfigureHeaderCircleGlass(HGAAppearanceMaterial *glass,UIView *header,
    CGRect frame,NSString *style) {
    BOOL flat=[style isEqual:@"flat"];
    NSString *material=flat ? @"solid" : style;
    BOOL reduced=UIAccessibilityIsReduceTransparencyEnabled();
    if(![glass.style isEqual:material]) {
        glass.style=material;
        glass.materialSignature=nil;
        [glass setNeedsLayout];
    }
    UIUserInterfaceStyle desired=UIUserInterfaceStyleDark;
    if(glass.overrideUserInterfaceStyle!=desired) {
        glass.overrideUserInterfaceStyle=desired;
        if(@available(iOS 17.0,*))[glass updateTraitsIfNeeded];
    }
    // Keep interactiveGlass NO on non-interactive backdrop surfaces so iOS 26/27
    // does not attach pointer/touch glass gesture handling beneath host controls.
    glass.interactiveGlass=NO;
    if(!CGRectEqualToRect(glass.frame,frame))glass.frame=frame;
    CGFloat radius=MIN(CGRectGetWidth(frame),CGRectGetHeight(frame))*0.5;
    if(fabs(glass.layer.cornerRadius-radius)>0.01)glass.layer.cornerRadius=radius;
    glass.layer.cornerCurve=kCACornerCurveContinuous;
    glass.clipsToBounds=YES;
    glass.userInteractionEnabled=NO;
    glass.accessibilityElementsHidden=YES;
    glass.overlayColor=flat && !reduced ? UIColor.tertiarySystemFillColor :
        [UIColor colorWithWhite:0.10 alpha:reduced ? 1 : 0.12];
    if(glass.superview!=header)[header insertSubview:glass atIndex:0];
    [glass requestMaterialRefresh];
}

static HGAAppearanceMaterial *HGAMakeHeaderCircleGlass(NSString *identifier) {
    HGAAppearanceMaterial *glass=[[HGAAppearanceMaterial alloc] initWithEffect:nil];
    glass.accessibilityIdentifier=identifier;
    glass.userInteractionEnabled=NO;
    glass.accessibilityElementsHidden=YES;
    glass.layer.cornerCurve=kCACornerCurveContinuous;
    glass.clipsToBounds=YES;
    return glass;
}

static void HGARemoveHeaderToolButton(HGAHeaderToolButton *button) {
    button.source=nil;
    [button removeFromSuperview];
}

static void HGAApplyHeaderTool(UIView *header,HGAHeaderInsetState *state,UIView *source,CGRect frame,
    NSString *style,BOOL sidebar) {
    NSString *identifier=sidebar ? @"honggo.appearance.header.sidebar" : @"honggo.appearance.header.search";
    SEL selector=sel_registerName(sidebar ? "simpleSideBarViewDidClicked" : "simpleSearchViewDidClicked");
    HGAAppearanceMaterial *glass=sidebar ? state.sidebarGlass : state.searchGlass;
    HGAHeaderToolButton *button=sidebar ? state.sidebarButton : state.searchButton;
    BOOL active=!CGRectIsNull(frame);
    BOOL flat=[style isEqual:@"flat"];
    BOOL native=active && (flat || HGNativeGlassConfigured()) && HGAHeaderToolOwner(source,selector);
    if (native) {
        if(!button) {
            button=[[HGAHeaderToolButton alloc] initWithFrame:frame];
            button.accessibilityIdentifier=identifier;
            button.overrideUserInterfaceStyle=UIUserInterfaceStyleDark;
            // Keep the original glyph and badge visible above the button.
            button.layer.zPosition=-1;
            [button addTarget:state action:@selector(activateTool:) forControlEvents:UIControlEventPrimaryActionTriggered];
        }
        if (![button.glassStyle isEqual:style]) {
            button.glassStyle=style;
            UIButtonConfiguration *configuration=UIButtonConfiguration.filledButtonConfiguration;
            configuration.baseBackgroundColor=UIColor.tertiarySystemFillColor;
            if (@available(iOS 26.0,*)) {
                if (!flat) configuration=[style isEqual:@"clear"] ?
                    UIButtonConfiguration.clearGlassButtonConfiguration : UIButtonConfiguration.glassButtonConfiguration;
            }
            configuration.cornerStyle=UIButtonConfigurationCornerStyleCapsule;
            button.configuration=configuration;
        }
        button.source=source;
        button.accessibilityLabel=source.accessibilityLabel ?: (sidebar ? @"菜单" : @"搜索");
        if (!CGRectEqualToRect(button.frame,frame)) button.frame=frame;
        if (button.superview!=header) [header addSubview:button];
        else {
            for (UIView *sibling in header.subviews.reverseObjectEnumerator) {
                if (sibling==button) break;
                if (HGAOwnView(sibling)) continue;
                [header bringSubviewToFront:button];
                break;
            }
        }
    } else {
        HGARemoveHeaderToolButton(button);
        button=nil;
    }
    if (active && !button) {
        if(!glass)glass=HGAMakeHeaderCircleGlass(identifier);
        HGAConfigureHeaderCircleGlass(glass,header,frame,style);
    } else {
        [glass removeFromSuperview];
        glass=nil;
    }
    if (sidebar) { state.sidebarButton=button; state.sidebarGlass=glass; }
    else { state.searchButton=button; state.searchGlass=glass; }
}

static void HGARemoveHeaderChannelControl(HGAHeaderInsetState *state) {
    [state.channelControl removeFromSuperview];
    state.channelControl=nil;
    state.channelStyle=nil;
    HGASetHidden(state.channelCollection,NO);
    state.channelCollection=nil;
    state.channelIndices=nil;
    state.channelTitles=nil;
}

static BOOL HGAApplyHeaderChannelControl(UIView *header,HGAHeaderInsetState *state,CGRect frame,NSString *style) {
    UIView *categories=state.categories;
    HGAHeaderChannelLayout *layout=objc_getAssociatedObject(categories,&HGAHeaderLayoutKey);
    NSArray *titles=layout.titles ?: HGReadObjectGetter(categories,"titles");
    UICollectionView *collection=layout.collection ?: (id)HGAViewGetter(categories,"collectionView",categories);
    Method selected=class_getInstanceMethod(object_getClass(categories),sel_registerName("selectedIndex"));
    Method select=class_getInstanceMethod(object_getClass(categories),sel_registerName("selectItemAtIndex:needAnimation:"));
    if (!selected || strcmp(method_getTypeEncoding(selected),"q16@0:8") || !select ||
        strcmp(method_getTypeEncoding(select),"B28@0:8q16B24") || ![titles isKindOfClass:NSArray.class] ||
        ![collection isKindOfClass:UICollectionView.class] ||
        (collection!=state.channelCollection && !HGAVisible(collection)) ||
        (!layout && collection.contentSize.width>CGRectGetWidth(collection.bounds)+0.5)) {
        HGARemoveHeaderChannelControl(state);
        return NO;
    }
    NSUInteger hiddenMask=layout.hiddenMask;
    NSUInteger visibleCount=titles.count-(NSUInteger)__builtin_popcountll(hiddenMask);
    if (!visibleCount || titles.count>32 || CGRectGetWidth(frame)<44*visibleCount) {
        HGARemoveHeaderChannelControl(state);
        return NO;
    }
    NSMutableArray<NSString *> *items=[NSMutableArray arrayWithCapacity:visibleCount];
    NSMutableArray<NSNumber *> *indices=[NSMutableArray arrayWithCapacity:visibleCount];
    for (NSUInteger index=0;index<titles.count;index++) {
        if (hiddenMask & (1UL<<index)) continue;
        NSString *title=titles[index];
        if (![title isKindOfClass:NSString.class] || !title.length || HGATopChannelHidden(title)) break;
        [items addObject:title];
        [indices addObject:@(index)];
    }
    if (items.count!=visibleCount) {
        HGARemoveHeaderChannelControl(state);
        return NO;
    }
    BOOL useGlass=![style isEqual:@"flat"] && HGNativeGlassConfigured() && !UIAccessibilityIsReduceTransparencyEnabled();
    NSString *controlStyle=useGlass ? style : @"flat";
    UIControl<HGAHeaderSelecting> *control=state.channelControl;
    if (collection!=state.channelCollection || ![indices isEqualToArray:state.channelIndices] ||
        ![items isEqualToArray:state.channelTitles] || ![controlStyle isEqual:state.channelStyle]) {
        HGARemoveHeaderChannelControl(state);
        control=nil;
        if (@available(iOS 26.0,*)) {
            if (useGlass) control=[[HGAHeaderGlassChannels alloc] initWithTitles:items clearGlass:[style isEqual:@"clear"]];
        }
        if (!control) control=(UIControl<HGAHeaderSelecting> *)[[UISegmentedControl alloc] initWithItems:items];
        control.accessibilityIdentifier=@"honggo.appearance.material";
        control.overrideUserInterfaceStyle=UIUserInterfaceStyleDark;
        [control addTarget:state action:@selector(changeChannel:) forControlEvents:UIControlEventValueChanged];
        state.channelControl=control;
        state.channelIndices=indices;
        state.channelStyle=controlStyle;
        state.channelTitles=items;
        state.channelCollection=collection;
    }
    if (control.superview!=header) [header addSubview:control];
    if (!CGRectEqualToRect(control.frame,frame)) control.frame=frame;
    HGASyncHeaderChannelLine(categories);
    HGASetHidden(collection,YES);
    return YES;
}

static void HGARemoveHeaderDetachedGlass(UIView *header) {
    HGAHeaderInsetState *state=objc_getAssociatedObject(header,&HGAHeaderInsetKey);
    if(!state)return;
    HGARemoveHeaderChannelControl(state);
    HGARemoveHeaderToolButton(state.sidebarButton);
    HGARemoveHeaderToolButton(state.searchButton);
    HGASetChannelLayout(state.categories,nil);
    [state.sidebarGlass removeFromSuperview];
    [state.searchGlass removeFromSuperview];
    objc_setAssociatedObject(header,&HGAHeaderInsetKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

static CGRect HGAHeaderVisibleChannelsRect(UIView *header) {
    BOOL allChannelsHidden=HGATopChannelHidden(@"关注") && HGATopChannelHidden(@"漫剧") &&
        HGATopChannelHidden(@"真人剧") && HGATopChannelHidden(@"推荐");
    if(allChannelsHidden)return CGRectNull;
    HGAHeaderInsetState *state = objc_getAssociatedObject(header,&HGAHeaderInsetKey);
    HGAHeaderChannelLayout *plan = objc_getAssociatedObject(state.categories,&HGAHeaderLayoutKey);
    if (plan.collection) return [plan.collection convertRect:plan.capsule toView:header];
    UIView *tabs=HGAViewGetter(header,"tabContainer",header);
    if(!tabs)return CGRectInset(header.bounds,8.0,0);
    if(tabs.hidden || tabs.alpha<0.01 || CGRectIsEmpty(tabs.bounds))return CGRectNull;
    Class cellCls=NSClassFromString(@"FQRJXCategoryTitleCell");
    CGRect unionRect=CGRectNull;
    if(cellCls) {
        for(UIView *node in HGADescendants(tabs,96)) {
            if(![node isKindOfClass:cellCls] || node.hidden || node.alpha<0.01 || CGRectIsEmpty(node.bounds))continue;
            CGRect r=[node convertRect:node.bounds toView:header];
            if(!isfinite(r.origin.x) || !isfinite(r.size.width) || r.size.width<8.0)continue;
            unionRect=CGRectIsNull(unionRect) ? r : CGRectUnion(unionRect,r);
        }
    }
    if(CGRectIsNull(unionRect)) {
        unionRect=[tabs convertRect:tabs.bounds toView:header];
        if(CGRectIsEmpty(unionRect))return CGRectNull;
        return unionRect;
    }
    return CGRectInset(unionRect,-16.0,0);
}

static BOOL HGAHeaderToolTransitionAnimating(UIView *header) {
    BOOL layout=NO,dismissing=NO;
    HGABoolGetter(header,@"isHeaderViewLayoutAnimating",&layout);
    HGABoolGetter(header,@"isSearchBarDismissAnimating",&dismissing);
    return layout || dismissing;
}

static BOOL HGABeginNativeHeaderToolTransition(UIView *header,BOOL animated) {
    HGAHeaderInsetState *state=objc_getAssociatedObject(header,&HGAHeaderInsetKey);
    if (!animated || !state.channelSelectionDepth || !state.visibleSideTools.count) return NO;
    state.ownsSideToolTransition=YES;
    return YES;
}

static void HGAFinishNativeHeaderToolTransition(UIView *header) {
    HGAHeaderInsetState *state=objc_getAssociatedObject(header,&HGAHeaderInsetKey);
    if (!HGAHeaderToolTransitionAnimating(header)) state.ownsSideToolTransition=NO;
}

static void HGAKeepHomeHeaderToolVisible(HGAHeaderInsetState *state,UIView *view,BOOL keep,BOOL repairing) {
    if (!view) return;
    if (!keep) { [state.visibleSideTools removeObject:view]; return; }
    if (CGRectIsEmpty(view.bounds)) return;
    if (HGAVisible(view) && view.alpha>=0.999) {
        if (!state.visibleSideTools) state.visibleSideTools=[NSHashTable weakObjectsHashTable];
        [state.visibleSideTools addObject:view];
    }
    if (!repairing || ![state.visibleSideTools containsObject:view]) return;
    BOOL fading=[view.layer animationForKey:@"opacity"]!=nil;
    if (!view.hidden && view.alpha>=0.999 && !fading) return;

    // The compact video channels use the same mounted native side tools. A
    // stock transition can briefly fade or hide those tools before settling.
    // Retain known visible instances rather than manufacturing any missing one.
    [UIView performWithoutAnimation:^{
        if (view.hidden) view.hidden=NO;
        if (view.alpha<0.999) view.alpha=1;
        if (fading) [view.layer removeAnimationForKey:@"opacity"];
    }];
}

static void HGAKeepCompactHeaderSideTools(UIView *header,HGAHeaderInsetState *state,UIView *sidebar,UIView *searchSlot) {
    HGAHeaderChannelLayout *plan=objc_getAssociatedObject(state.categories,&HGAHeaderLayoutKey);
    NSUInteger visibleCount=plan.titles.count-(NSUInteger)__builtin_popcountll(plan.hiddenMask);
    NSUInteger comic=[plan.titles indexOfObject:@"漫剧"],recommend=[plan.titles indexOfObject:@"推荐"];
    BOOL keepsTools=visibleCount==2 && comic<plan.titles.count && recommend<plan.titles.count &&
        !HGAHiddenChannelIndex(plan,(NSInteger)comic) && !HGAHiddenChannelIndex(plan,(NSInteger)recommend) &&
        HGAVisible(header) && !HGFeedSidebarOwnsLayout(header.window);
    if (!keepsTools) {
        [state.visibleSideTools removeAllObjects];
        state.ownsSideToolTransition=NO;
        return;
    }

    BOOL repairing=state.channelSelectionDepth || (state.ownsSideToolTransition && HGAHeaderToolTransitionAnimating(header));
    HGAKeepHomeHeaderToolVisible(state,sidebar,!HGAppearanceFlag(@"layout.channels.hide.sidebar"),repairing);
    UIView *search=HGAViewGetter(header,"searchViewContainer",header);
    BOOL keepSearch=!HGAppearanceFlag(@"layout.channels.hide.search") && searchSlot && searchSlot!=search;
    HGAKeepHomeHeaderToolVisible(state,search,keepSearch,repairing);
    HGAKeepHomeHeaderToolVisible(state,searchSlot,keepSearch,repairing);
    HGAKeepHomeHeaderToolVisible(state,HGAViewGetter(search,"searchButtonView",searchSlot),keepSearch,repairing);
}

// The verified home header routes native glass controls to the existing
// controller actions. Other header layouts retain their host hit targets.
static void HGAApplyHeaderSeparatedGlass(UIView *header,BOOL enabled,BOOL layoutEnabled,NSString *style) {
    if(!layoutEnabled || CGRectIsEmpty(header.bounds)) {
        HGARemoveHeaderDetachedGlass(header);
        HGARemoveGlass(header);
        return;
    }
    // Never churn header glass while the feed controller is sliding horizontally
    // for the sidebar drawer or pushing the search controller.
    if(header.window && fabs(CGRectGetMinX([header convertRect:header.bounds toView:header.window]))>24.0) {
        return;
    }
    HGAHeaderInsetState *state=objc_getAssociatedObject(header,&HGAHeaderInsetKey);
    if(!state) {
        state=[HGAHeaderInsetState new];
        objc_setAssociatedObject(header,&HGAHeaderInsetKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    // Compact remaining visible tabs when any channel is hidden so there is no
    // empty gap inside the center capsule.
    HGACompactHeaderChannels(header,state);
    if(!enabled) {
        [state.visibleSideTools removeAllObjects];
        HGARemoveHeaderChannelControl(state);
        HGARemoveHeaderToolButton(state.sidebarButton);
        HGARemoveHeaderToolButton(state.searchButton);
        state.sidebarButton=nil;
        state.searchButton=nil;
        [state.sidebarGlass removeFromSuperview];
        [state.searchGlass removeFromSuperview];
        HGARemoveGlass(header);
        return;
    }
    style=HGFlatTopStyle() ? @"flat" : ([style isEqual:@"clear"] ? @"clear" : @"regular");
    UIView *sidebar=HGAViewGetter(header,"sideBar",header);
    UIView *searchSlot=HGAHeaderSearchSlotView(header);
    HGAKeepCompactHeaderSideTools(header,state,sidebar,searchSlot);

    CGFloat headerW=CGRectGetWidth(header.bounds);
    CGFloat headerH=CGRectGetHeight(header.bounds);
    CGFloat side=MIN(42.0,MAX(36.0,headerH));
    CGFloat midY=CGRectGetMidY(header.bounds);

    BOOL showSidebar=!HGAppearanceFlag(@"layout.channels.hide.sidebar") &&
        sidebar && !sidebar.hidden && sidebar.alpha>=0.01 && !CGRectIsEmpty(sidebar.bounds);
    BOOL showSearch=!HGAppearanceFlag(@"layout.channels.hide.search") &&
        searchSlot && !searchSlot.hidden && searchSlot.alpha>=0.01 && !CGRectIsEmpty(searchSlot.bounds);

    CGRect leftCircle=CGRectNull;
    if(showSidebar) {
        CGRect r=[sidebar convertRect:sidebar.bounds toView:header];
        CGFloat x=MAX(4.0,MIN(headerW-side-4.0,CGRectGetMidX(r)-side*0.5));
        leftCircle=CGRectMake(x,CGRectGetMidY(r)-side*0.5,side,side);
    }
    HGAApplyHeaderTool(header,state,sidebar,leftCircle,style,YES);

    CGRect rightCircle=CGRectNull;
    if(showSearch) {
        CGRect r=[searchSlot convertRect:searchSlot.bounds toView:header];
        CGFloat x=MAX(4.0,MIN(headerW-side-4.0,CGRectGetMidX(r)-side*0.5));
        rightCircle=CGRectMake(x,CGRectGetMidY(r)-side*0.5,side,side);
    }
    HGAApplyHeaderTool(header,state,searchSlot,rightCircle,style,NO);

    if (state.channelControl && HGAHeaderToolTransitionAnimating(header)) {
        HGASyncHeaderChannelLine(state.categories);
        return;
    }
    CGRect channels=HGAHeaderVisibleChannelsRect(header);
    if(!CGRectIsNull(channels)) {
        CGFloat minAllowed=!CGRectIsNull(leftCircle) ? CGRectGetMaxX(leftCircle)+8.0 : 8.0;
        CGFloat maxAllowed=!CGRectIsNull(rightCircle) ? CGRectGetMinX(rightCircle)-8.0 : headerW-8.0;
        CGFloat minX=MAX(minAllowed,CGRectGetMinX(channels));
        CGFloat maxX=MIN(maxAllowed,CGRectGetMaxX(channels));
        if(maxX-minX<side && maxAllowed-minAllowed>=side) {
            CGFloat cx=MAX(minAllowed+side*0.5,MIN(maxAllowed-side*0.5,CGRectGetMidX(channels)));
            minX=cx-side*0.5;
            maxX=cx+side*0.5;
        }
        if(maxX-minX>=24.0) {
            CGRect centerFrame=CGRectMake(minX,midY-side*0.5,maxX-minX,side);
            if (HGAApplyHeaderChannelControl(header,state,centerFrame,style)) {
                HGAViewState *material=HGAState(header,YES);
                [material.glass removeFromSuperview];
                material.glass=nil;
                HGASetGlassHostBackground(header,YES);
            } else if ([style isEqual:@"flat"]) {
                // A scrollable or unrecognized host channel strip keeps its own
                // cells and gestures. Put only the neutral track beneath it.
                HGAInstallGlassInRect(header,@"solid",centerFrame,side*0.5,0);
                HGAState(header,NO).glass.overrideUserInterfaceStyle=UIUserInterfaceStyleDark;
                HGASetGlassOverlay(header,UIAccessibilityIsReduceTransparencyEnabled() ?
                    UIColor.darkGrayColor : UIColor.tertiarySystemFillColor);
            } else HGAInstallPlaybackGlass(header,style,centerFrame,side*0.5,nil);
            return;
        }
    }
    HGARemoveHeaderChannelControl(state);
    HGARemoveGlass(header);
}

void HGAppearanceUpdateSidebarLayout(UIWindow *window, BOOL suspended) {
    if (!window) return;
    for (UIView *view in HGAManaged.allObjects) {
        if (view.window != window) continue;
        if (HGAIsHeaderView(view)) {
            if (suspended) {
                HGARemoveHeaderDetachedGlass(view);
                HGARemoveGlass(view);
                HGARestoreGeometry(view);
            } else {
                [view setNeedsLayout];
            }
        } else if (HGAIsBottomControl(view)) {
            if (suspended) HGARelinquishBottomLayout(view);
            else [view setNeedsLayout];
        }
    }
}
