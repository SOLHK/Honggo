@interface HGAHeaderInsetState : NSObject
@property(nonatomic,strong) HGAAppearanceMaterial *sidebarGlass;
@property(nonatomic,strong) HGAAppearanceMaterial *searchGlass;
@property(nonatomic,weak) UIView *categories;
@property(nonatomic,copy) NSArray *observedTitles;
@property(nonatomic) CGSize observedSize;
@property(nonatomic,weak) UICollectionView *observedCollection;
@end
@implementation HGAHeaderInsetState @end
static char HGAHeaderInsetKey;

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

static char HGAHeaderLayoutKey;
static BOOL HGAHeaderAdaptersReady;
static IMP HGAHeaderOriginals[5];

static CGSize HGAChannelSize(id owner, SEL cmd, UICollectionView *collection, UICollectionViewLayout *layout, NSIndexPath *path) {
    CGSize size = ((CGSize (*)(id,SEL,id,id,id))HGAHeaderOriginals[0])(owner,cmd,collection,layout,path);
    HGAHeaderChannelLayout *plan = objc_getAssociatedObject(owner,&HGAHeaderLayoutKey);
    if (collection == plan.collection && path.section == 0 && path.item >= 0 && path.item < (NSInteger)plan.frames.count)
        size.width = plan.frames[path.item].CGRectValue.size.width;
    return size;
}

static CGFloat HGAChannelSpacing(id owner, SEL cmd, UICollectionView *collection, UICollectionViewLayout *layout, NSInteger section) {
    HGAHeaderChannelLayout *plan = objc_getAssociatedObject(owner,&HGAHeaderLayoutKey);
    if (plan && collection == plan.collection && section == 0) return 0;
    return ((CGFloat (*)(id,SEL,id,id,NSInteger))HGAHeaderOriginals[1])(owner,cmd,collection,layout,section);
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

static void HGAInstallHeaderLayoutAdapters(void) {
    if (HGAHeaderAdaptersReady) return;
    Class cls = NSClassFromString(@"SSHGBookStoreCategoryTitleView");
    if (!cls) return;
    static const struct { const char *name, *encoding; IMP replacement; } adapters[] = {
        {"collectionView:layout:sizeForItemAtIndexPath:", "{CGSize=dd}40@0:8@16@24@32", (IMP)HGAChannelSize},
        {"collectionView:layout:minimumLineSpacingForSectionAtIndex:", "d40@0:8@16@24q32", (IMP)HGAChannelSpacing},
        {"collectionView:layout:insetForSectionAtIndex:", "{UIEdgeInsets=dddd}40@0:8@16@24q32", (IMP)HGAChannelInsets},
        {"getTargetCellFrame:", "{CGRect={CGPoint=dd}{CGSize=dd}}24@0:8q16", (IMP)HGAChannelFrame},
        {"getTargetSelectedCellFrame:selectedType:", "{CGRect={CGPoint=dd}{CGSize=dd}}32@0:8q16Q24", (IMP)HGAChannelSelectedFrame}
    };
    static const struct { const char *name, *encoding; } properties[] = {
        {"titles", "@16@0:8"}, {"indicators", "@16@0:8"}, {"collectionView", "@16@0:8"},
        {"contentScrollView", "@16@0:8"}, {"selectedIndex", "q16@0:8"}
    };
    for (NSUInteger i = 0; i < sizeof(properties)/sizeof(properties[0]); i++) {
        Method m = class_getInstanceMethod(cls,sel_registerName(properties[i].name));
        if (!m || strcmp(method_getTypeEncoding(m),properties[i].encoding)) return;
    }
    for (NSUInteger i = 0; i < 5; i++) {
        Method m = class_getInstanceMethod(cls,sel_registerName(adapters[i].name));
        if (!m || strcmp(method_getTypeEncoding(m),adapters[i].encoding)) return;
    }
    for (NSUInteger i = 0; i < 5; i++) {
        SEL selector = sel_registerName(adapters[i].name);
        if (!HGAHeaderOriginals[i])
            HGAHeaderOriginals[i] = class_getMethodImplementation(cls,selector);
        IMP original = HGInstallVerifiedHook(cls,selector,adapters[i].encoding,adapters[i].replacement,@"appearance");
        if (!original || class_getMethodImplementation(cls,selector) != adapters[i].replacement) return;
        HGAHeaderOriginals[i] = original;
    }
    HGAHeaderAdaptersReady = YES;
}

static BOOL HGAHeaderScrollInFlight(id categories, UIScrollView *collection) {
    UIScrollView *pager = HGReadObjectGetter(categories,"contentScrollView");
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

static void HGAAlignSettledChannelLine(UIView *categories, UICollectionView *collection, HGAHeaderChannelLayout *plan) {
    SEL selector = sel_registerName("selectedIndex");
    Method method = class_getInstanceMethod(categories.class,selector);
    if (!method || strcmp(method_getTypeEncoding(method),"q16@0:8")) return;
    NSInteger index = ((NSInteger (*)(id,SEL))objc_msgSend)(categories,selector);
    if (index < 0 || index >= [collection numberOfItemsInSection:0]) return;
    CGRect target = HGAChannelFrame(categories,sel_registerName("getTargetCellFrame:"),index);
    if (CGRectIsEmpty(target) || (plan && (index >= (NSInteger)plan.frames.count || (plan.hiddenMask & (1UL << index))))) return;
    NSArray *lines = HGReadObjectGetter(categories,"indicators");
    if (![lines isKindOfClass:NSArray.class]) return;
    for (UIView *line in lines) {
        if (![line isKindOfClass:NSClassFromString(@"FQRJXCategoryIndicatorLineView")] ||
            ![line isDescendantOfView:collection] || line.layer.animationKeys.count) continue;
        CGPoint center = line.center;
        CGFloat x = [collection convertPoint:CGPointMake(CGRectGetMidX(target),CGRectGetMidY(target))
                                      toView:line.superview].x;
        if (isfinite(x) && fabs(x-center.x) > 0.25) line.center = CGPointMake(x,center.y);
    }
}

static void HGASetChannelLayout(UIView *categories, HGAHeaderChannelLayout *next) {
    HGAHeaderChannelLayout *old = objc_getAssociatedObject(categories,&HGAHeaderLayoutKey);
    if (old == next) return;
    objc_setAssociatedObject(categories,&HGAHeaderLayoutKey,next,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [old.collection.collectionViewLayout invalidateLayout];
    if (old.collection != next.collection) [next.collection.collectionViewLayout invalidateLayout];
    UICollectionView *collection = next.collection ?: old.collection;
    if (collection.numberOfSections && !HGAHeaderScrollInFlight(categories,collection))
        HGAAlignSettledChannelLine(categories,collection,next);
}

static void HGACompactHeaderChannels(UIView *header, HGAHeaderInsetState *state) {
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
    BOOL animating = NO;
    if (HGABoolGetter(header,@"isHeaderViewLayoutAnimating",&animating) && animating) return;
    NSUInteger mask = 0;
    for (NSUInteger i = 0; i < titles.count; i++) {
        if ([titles[i] isKindOfClass:NSString.class] && HGATopChannelHidden(titles[i])) mask |= 1UL << i;
    }
    if (!mask || (NSUInteger)__builtin_popcountll(mask) == titles.count) {
        HGASetChannelLayout(categories,nil);
        return;
    }
    if (old.collection == collection && old.hiddenMask == mask && old.width == collection.bounds.size.width &&
        [old.titles isEqualToArray:titles]) return;
    HGASetChannelLayout(categories,HGAMakeChannelLayout(categories,collection,titles,mask));
}

static void HGAConfigureHeaderCircleGlass(HGAAppearanceMaterial *glass,UIView *header,
    CGRect frame,NSString *style) {
    BOOL solid=[style isEqual:@"solid"];
    BOOL reduced=UIAccessibilityIsReduceTransparencyEnabled();
    if(![glass.style isEqual:style]) {
        glass.style=style;
        glass.materialSignature=nil;
        [glass setNeedsLayout];
    }
    UIUserInterfaceStyle desired=solid ? UIUserInterfaceStyleUnspecified : UIUserInterfaceStyleDark;
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
    UIColor *color=solid ? UIColor.secondarySystemBackgroundColor : [UIColor colorWithWhite:0.10 alpha:1];
    CGFloat opacity=(solid || reduced) ? 1.0 : 0.12;
    glass.overlayColor=[color colorWithAlphaComponent:opacity];
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

static void HGARemoveHeaderDetachedGlass(UIView *header) {
    HGAHeaderInsetState *state=objc_getAssociatedObject(header,&HGAHeaderInsetKey);
    if(!state)return;
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

// Separate the home header into three native liquid-glass surfaces (left menu
// circle, center category capsule, right search circle) without moving,
// reparenting or intercepting any host tap target.
static void HGAApplyHeaderSeparatedGlass(UIView *header,BOOL enabled,BOOL finder,NSString *style) {
    if(!header || !enabled || finder || header.hidden || CGRectIsEmpty(header.bounds)) {
        HGARemoveHeaderDetachedGlass(header);
        if(header)HGARemoveGlass(header);
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
    UIView *sidebar=HGAViewGetter(header,"sideBar",header);
    UIView *searchSlot=HGAHeaderSearchSlotView(header);

    // Compact remaining visible tabs when any channel is hidden so there is no
    // empty gap inside the center capsule.
    HGACompactHeaderChannels(header,state);

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
        if(!state.sidebarGlass)state.sidebarGlass=HGAMakeHeaderCircleGlass(@"honggo.appearance.header.sidebar");
        HGAConfigureHeaderCircleGlass(state.sidebarGlass,header,leftCircle,style);
    } else {
        [state.sidebarGlass removeFromSuperview];
        state.sidebarGlass=nil;
    }

    CGRect rightCircle=CGRectNull;
    if(showSearch) {
        CGRect r=[searchSlot convertRect:searchSlot.bounds toView:header];
        CGFloat x=MAX(4.0,MIN(headerW-side-4.0,CGRectGetMidX(r)-side*0.5));
        rightCircle=CGRectMake(x,CGRectGetMidY(r)-side*0.5,side,side);
        if(!state.searchGlass)state.searchGlass=HGAMakeHeaderCircleGlass(@"honggo.appearance.header.search");
        HGAConfigureHeaderCircleGlass(state.searchGlass,header,rightCircle,style);
    } else {
        [state.searchGlass removeFromSuperview];
        state.searchGlass=nil;
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
            HGAInstallPlaybackGlass(header,style,centerFrame,side*0.5,nil);
            return;
        }
    }
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
