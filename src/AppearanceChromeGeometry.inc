@interface HGAHeaderDetachedController : NSObject <UIGestureRecognizerDelegate>
@property(nonatomic,weak) UIView *header;
@property(nonatomic,weak) UIView *tabsView;
@property(nonatomic,strong) HGAAppearanceMaterial *sideGlass;
@property(nonatomic,strong) HGAAppearanceMaterial *centerGlass;
@property(nonatomic,strong) HGAAppearanceMaterial *searchGlass;
@property(nonatomic,strong) HGAAppearanceMaterial *pillGlass;
@property(nonatomic,strong) UILongPressGestureRecognizer *pressGesture;
@property(nonatomic) BOOL touching;
@property(nonatomic) NSInteger hoverIndex;
@property(nonatomic) CGFloat touchAnchorX;
@property(nonatomic,strong) NSMapTable<UIView *,NSNumber *> *dimmedIndicators;
@property(nonatomic,strong) NSHashTable<UIView *> *compactedCells;
- (void)teardown;
@end

static char HGAHeaderDetachedKey;

static HGAAppearanceMaterial *HGAMakeHeaderGlass(NSString *roleLabel) {
    HGAAppearanceMaterial *glass=[[HGAAppearanceMaterial alloc] initWithEffect:nil];
    glass.accessibilityIdentifier=@"honggo.appearance.material";
    glass.accessibilityLabel=roleLabel;
    glass.accessibilityElementsHidden=YES;
    glass.userInteractionEnabled=NO;
    glass.layer.cornerCurve=kCACornerCurveContinuous;
    glass.clipsToBounds=YES;
    return glass;
}

static void HGAConfigureHeaderGlass(HGAAppearanceMaterial *glass,UIView *header,CGRect frame,NSString *style) {
    BOOL solid=[style isEqual:@"solid"];
    BOOL clearGlass=[style isEqual:@"clear"];
    BOOL reduced=UIAccessibilityIsReduceTransparencyEnabled();
    CGFloat radius=MIN(CGRectGetWidth(frame),CGRectGetHeight(frame))*0.5;
    if(![glass.style isEqual:style] || fabs(glass.overlayOpacity)>0.001) {
        glass.style=style;
        glass.overlayOpacity=0;
        glass.materialSignature=nil;
        [glass setNeedsLayout];
    }
    glass.overrideUserInterfaceStyle=solid ? UIUserInterfaceStyleUnspecified : UIUserInterfaceStyleDark;
    glass.interactiveGlass=!solid && ![style isEqual:@"blur"] && !reduced;
    if(!CGRectEqualToRect(glass.frame,frame))glass.frame=frame;
    if(fabs(glass.layer.cornerRadius-radius)>0.001)glass.layer.cornerRadius=radius;
    if(@available(iOS 26.0,*)) {
        HGApplyNativeCornerConfig(glass,radius,YES);
    }
    if(@available(iOS 17.0,*))[glass updateTraitsIfNeeded];
    UIColor *base=solid ? UIColor.secondarySystemBackgroundColor : [UIColor colorWithWhite:0.10 alpha:1.0];
    CGFloat opacity=(solid || reduced) ? 1.0 : (clearGlass ? 0.0 : 0.12);
    glass.overlayColor=[base colorWithAlphaComponent:opacity];
    if(glass.superview!=header)[header insertSubview:glass atIndex:0];
    HGApplySurfaceBackdropBlur(glass,glass,glass,glass.bounds,radius,!solid,style,HGAppearanceNumber(@"header.blur"));
    [glass requestMaterialRefresh];
}

static void HGARemoveHeaderGlass(HGAAppearanceMaterial *glass) {
    if(!glass)return;
    HGClearSurfaceBackdropBlur(glass);
    [glass removeFromSuperview];
}

static UIView *HGAHeaderSearchSlot(UIView *header) {
    UIView *search=HGAViewGetter(header,"searchViewContainer",header);
    if(!search)return nil;
    UIView *slot=HGAViewGetter(search,"searchButtonSlotView",search) ?: HGAViewGetter(search,"searchButtonView",search);
    return (slot && slot!=search) ? slot : nil;
}

static UIView *HGAHeaderCategoryView(UIView *tabContainer) {
    if(!tabContainer)return nil;
    Class cls=NSClassFromString(@"FQRJXCategoryTitleView");
    if(!cls)return nil;
    for(UIView *child in HGADescendants(tabContainer,32)) {
        if([child isKindOfClass:cls])return child;
    }
    return nil;
}

static NSString *HGAHeaderCellTitle(UIView *cell) {
    NSString *title=HGAPageChromeLabel(cell,"titleLabel");
    if(!title.length) {
        id modelTitle=HGReadObjectGetter(HGReadObjectGetter(cell,"cellModel"),"title");
        if([modelTitle isKindOfClass:NSString.class])title=modelTitle;
    }
    return title;
}

static NSArray<UIView *> *HGAHeaderChannelCells(UIView *tabContainer,BOOL visibleOnly) {
    if(!tabContainer)return @[];
    Class cellCls=NSClassFromString(@"FQRJXCategoryTitleCell");
    if(!cellCls)return @[];
    NSMutableArray<UIView *> *cells=[NSMutableArray array];
    for(UIView *child in HGADescendants(tabContainer,64)) {
        if(![child isKindOfClass:cellCls])continue;
        if(visibleOnly && (child.hidden || child.alpha<0.01 || CGRectIsEmpty(child.bounds)))continue;
        [cells addObject:child];
    }
    return cells;
}

static CGFloat HGAHeaderBaseMidX(UIView *cell,UIView *header) {
    CGRect rect=[cell convertRect:cell.bounds toView:header];
    HGAViewState *state=HGAState(cell,NO);
    CGFloat appliedTx=(state.geometry && state.geometrySuperview==cell.superview &&
        CGAffineTransformEqualToTransform(cell.transform,state.appliedTransform)) ? state.appliedTransform.tx : 0;
    return CGRectGetMidX(rect)-appliedTx;
}

static NSInteger HGAHeaderSelectedIndex(UIView *categoryView) {
    if(!categoryView)return NSNotFound;
    SEL sel=sel_registerName("selectedIndex");
    Method m=class_getInstanceMethod(categoryView.class,sel);
    if(m && !strcmp(method_getTypeEncoding(m),"q16@0:8")) {
        return ((NSInteger (*)(id,SEL))objc_msgSend)(categoryView,sel);
    }
    return NSNotFound;
}

@implementation HGAHeaderDetachedController

- (instancetype)init {
    if((self=[super init])) {
        _hoverIndex=NSNotFound;
        _dimmedIndicators=[NSMapTable weakToStrongObjectsMapTable];
        _compactedCells=[NSHashTable weakObjectsHashTable];
    }
    return self;
}

- (void)restoreCompactedCells {
    for(UIView *cell in self.compactedCells.allObjects) {
        HGARestoreGeometry(cell);
    }
    [self.compactedCells removeAllObjects];
}

- (void)restoreIndicators {
    for(UIView *indicator in self.dimmedIndicators.keyEnumerator.allObjects) {
        NSNumber *saved=[self.dimmedIndicators objectForKey:indicator];
        if(saved && indicator.alpha<0.02)indicator.alpha=saved.doubleValue;
    }
    [self.dimmedIndicators removeAllObjects];
}

- (void)dimIndicatorsForCategoryView:(UIView *)categoryView active:(BOOL)active {
    if(!active || !categoryView) {
        [self restoreIndicators];
        return;
    }
    id indicators=HGReadObjectGetter(categoryView,"indicators");
    if(![indicators isKindOfClass:NSArray.class])return;
    for(id item in indicators) {
        if(![item isKindOfClass:UIView.class])continue;
        UIView *indicator=(UIView *)item;
        if(![indicator isDescendantOfView:categoryView])continue;
        if(![self.dimmedIndicators objectForKey:indicator] && indicator.alpha>0.02) {
            [self.dimmedIndicators setObject:@(indicator.alpha) forKey:indicator];
        }
        if(indicator.alpha>0.01)indicator.alpha=0.0;
    }
}

- (void)updatePillAppearanceForDark:(BOOL)dark {
    if(!self.pillGlass)return;
    BOOL reduce=UIAccessibilityIsReduceTransparencyEnabled();
    BOOL darkTint=HGAppearanceFlag(@"glass.darkTint");
    BOOL liveLens=self.touching && !reduce && HGNativeGlassConfigured();
    self.pillGlass.overrideUserInterfaceStyle=dark ? UIUserInterfaceStyleDark : UIUserInterfaceStyleLight;
    if(liveLens) {
        if(@available(iOS 26.0,*)) {
            UIColor *tint=darkTint ? [UIColor colorWithWhite:0.0 alpha:(dark?0.45:0.10)] : nil;
            self.pillGlass.effect=HGMakeNativeGlassEffect(YES,YES,tint);
        }
        self.pillGlass.contentView.backgroundColor=UIColor.clearColor;
    } else {
        if(self.pillGlass.effect)self.pillGlass.effect=nil;
        UIColor *fill=dark ? [UIColor colorWithWhite:1.0 alpha:0.16] : [UIColor colorWithWhite:0.0 alpha:0.08];
        if(![self.pillGlass.contentView.backgroundColor isEqual:fill]) {
            self.pillGlass.contentView.backgroundColor=fill;
        }
    }
    CGFloat radius=MIN(CGRectGetWidth(self.pillGlass.bounds),CGRectGetHeight(self.pillGlass.bounds))*0.5;
    if(radius>0.5) {
        self.pillGlass.layer.cornerRadius=radius;
        self.pillGlass.contentView.layer.cornerRadius=radius;
        self.pillGlass.contentView.layer.cornerCurve=kCACornerCurveContinuous;
        self.pillGlass.contentView.clipsToBounds=YES;
        if(@available(iOS 26.0,*))HGApplyNativeCornerConfig(self.pillGlass,radius,YES);
    }
}

- (void)placePillFrame:(CGRect)target animated:(BOOL)animated {
    if(!self.pillGlass)return;
    if(CGRectGetWidth(target)<12.0 || CGRectGetHeight(target)<12.0) {
        self.pillGlass.hidden=YES;
        return;
    }
    self.pillGlass.hidden=NO;
    if(CGRectEqualToRect(self.pillGlass.frame,target))return;
    void (^apply)(void)=^{
        self.pillGlass.frame=target;
        CGFloat radius=MIN(CGRectGetWidth(target),CGRectGetHeight(target))*0.5;
        self.pillGlass.layer.cornerRadius=radius;
        self.pillGlass.contentView.layer.cornerRadius=radius;
        if(@available(iOS 26.0,*))HGApplyNativeCornerConfig(self.pillGlass,radius,YES);
    };
    if(animated && !UIAccessibilityIsReduceMotionEnabled()) {
        [UIView animateWithDuration:0.34 delay:0 usingSpringWithDamping:0.78 initialSpringVelocity:0
                            options:UIViewAnimationOptionBeginFromCurrentState|UIViewAnimationOptionAllowUserInteraction
                         animations:apply completion:nil];
    } else {
        [UIView performWithoutAnimation:apply];
    }
}

- (CGRect)pillFrameForCell:(UIView *)cell {
    if(!cell || !self.centerGlass || cell.hidden)return CGRectZero;
    CGRect inCenter=[cell convertRect:cell.bounds toView:self.centerGlass.contentView];
    CGRect bounds=self.centerGlass.contentView.bounds;
    if(CGRectIsEmpty(bounds) || CGRectIsEmpty(inCenter))return CGRectZero;
    CGFloat pillH=MAX(22.0,CGRectGetHeight(bounds)-6.0);
    CGFloat pillW=MIN(CGRectGetWidth(bounds)-6.0,MAX(pillH,CGRectGetWidth(inCenter)+10.0));
    CGFloat minX=3.0;
    CGFloat maxX=MAX(minX,CGRectGetWidth(bounds)-pillW-3.0);
    CGFloat x=MIN(maxX,MAX(minX,CGRectGetMidX(inCenter)-pillW*0.5));
    return CGRectMake(round(x),round((CGRectGetHeight(bounds)-pillH)*0.5),round(pillW),round(pillH));
}

- (UIView *)selectedVisibleCellInTabContainer:(UIView *)tabContainer categoryView:(UIView *)categoryView {
    NSArray<UIView *> *visible=HGAHeaderChannelCells(tabContainer,YES);
    if(!visible.count)return nil;
    NSInteger selectedIndex=HGAHeaderSelectedIndex(categoryView);
    id titles=HGReadObjectGetter(categoryView,"titles");
    if([titles isKindOfClass:NSArray.class] && selectedIndex>=0 && (NSUInteger)selectedIndex<[titles count]) {
        id targetTitle=titles[(NSUInteger)selectedIndex];
        if([targetTitle isKindOfClass:NSString.class]) {
            for(UIView *cell in visible) {
                if([HGAHeaderCellTitle(cell) isEqual:targetTitle])return cell;
            }
        }
    }
    return nil;
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer {
    (void)gestureRecognizer; (void)otherGestureRecognizer;
    return YES;
}

- (void)handlePress:(UILongPressGestureRecognizer *)gesture {
    UIView *tabs=self.tabsView;
    if(!tabs || !self.centerGlass || self.centerGlass.hidden)return;
    NSArray<UIView *> *visible=HGAHeaderChannelCells(tabs,YES);
    if(!visible.count)return;
    CGFloat x=[gesture locationInView:self.centerGlass.contentView].x;
    if(gesture.state==UIGestureRecognizerStateBegan || gesture.state==UIGestureRecognizerStateChanged) {
        self.touching=YES;
        [self updatePillAppearanceForDark:YES];
        UIView *nearest=nil;
        NSInteger nearestIdx=NSNotFound;
        CGFloat bestDist=CGFLOAT_MAX;
        for(NSUInteger i=0;i<visible.count;i++) {
            CGRect r=[visible[i] convertRect:visible[i].bounds toView:self.centerGlass.contentView];
            CGFloat d=fabs(CGRectGetMidX(r)-x);
            if(d<bestDist){bestDist=d;nearest=visible[i];nearestIdx=(NSInteger)i;}
        }
        if(nearest) {
            if(gesture.state==UIGestureRecognizerStateChanged && nearestIdx!=self.hoverIndex) {
                self.hoverIndex=nearestIdx;
                [[[UISelectionFeedbackGenerator alloc] init] selectionChanged];
            } else if(gesture.state==UIGestureRecognizerStateBegan) {
                self.hoverIndex=nearestIdx;
            }
            [self placePillFrame:[self pillFrameForCell:nearest] animated:(gesture.state==UIGestureRecognizerStateBegan)];
        }
    } else {
        self.touching=NO;
        self.hoverIndex=NSNotFound;
        [self updatePillAppearanceForDark:YES];
        UIView *cat=HGAHeaderCategoryView(tabs);
        UIView *selectedCell=[self selectedVisibleCellInTabContainer:tabs categoryView:cat];
        [self placePillFrame:[self pillFrameForCell:selectedCell] animated:YES];
    }
}

- (void)syncWithHeader:(UIView *)header style:(NSString *)style {
    self.header=header;
    CGFloat headerW=CGRectGetWidth(header.bounds);
    CGFloat headerH=CGRectGetHeight(header.bounds);
    if(headerW<44.0 || headerH<24.0){[self teardown];return;}
    CGFloat circleSize=MIN(42.0,MAX(36.0,headerH));
    CGFloat midY=CGRectGetMidY(header.bounds);

    UIView *sideBar=HGAViewGetter(header,"sideBar",header);
    UIView *tabContainer=HGAViewGetter(header,"tabContainer",header);
    UIView *searchSlot=HGAHeaderSearchSlot(header);
    self.tabsView=tabContainer;

    // 1. Left detached circular glass directly behind sideBar (zero transform on host control)
    CGRect leftFrame=CGRectNull;
    BOOL showSide=sideBar && !sideBar.hidden && sideBar.alpha>=0.01 && !CGRectIsEmpty(sideBar.bounds);
    if(showSide) {
        CGRect r=[sideBar convertRect:sideBar.bounds toView:header];
        CGFloat x=MIN(MAX(4.0,round(CGRectGetMidX(r)-circleSize*0.5)),MAX(4.0,headerW-circleSize-4.0));
        leftFrame=CGRectMake(x,round(midY-circleSize*0.5),circleSize,circleSize);
        if(!self.sideGlass)self.sideGlass=HGAMakeHeaderGlass(@"header.sidebar");
        HGAConfigureHeaderGlass(self.sideGlass,header,leftFrame,style);
    } else {
        HGARemoveHeaderGlass(self.sideGlass);
    }

    // 2. Right detached circular glass directly behind searchButtonSlotView (zero transform on host control)
    CGRect rightFrame=CGRectNull;
    BOOL showSearch=searchSlot && !searchSlot.hidden && searchSlot.alpha>=0.01 && !CGRectIsEmpty(searchSlot.bounds);
    if(showSearch) {
        CGRect r=[searchSlot convertRect:searchSlot.bounds toView:header];
        CGFloat x=MIN(MAX(4.0,round(CGRectGetMidX(r)-circleSize*0.5)),MAX(4.0,headerW-circleSize-4.0));
        rightFrame=CGRectMake(x,round(midY-circleSize*0.5),circleSize,circleSize);
        if(!self.searchGlass)self.searchGlass=HGAMakeHeaderGlass(@"header.search");
        HGAConfigureHeaderGlass(self.searchGlass,header,rightFrame,style);
    } else {
        HGARemoveHeaderGlass(self.searchGlass);
    }

    // 3. Center compact channel capsule (collapses empty gaps when any home channel is hidden)
    BOOL anyChannelHidden=HGATopChannelHidden(@"关注") || HGATopChannelHidden(@"漫剧") ||
        HGATopChannelHidden(@"真人剧") || HGATopChannelHidden(@"推荐");
    BOOL allChannelsHidden=HGATopChannelHidden(@"关注") && HGATopChannelHidden(@"漫剧") &&
        HGATopChannelHidden(@"真人剧") && HGATopChannelHidden(@"推荐");
    NSArray<UIView *> *visibleCells=HGAHeaderChannelCells(tabContainer,YES);
    if(anyChannelHidden && visibleCells.count>0) {
        NSArray<UIView *> *sorted=[visibleCells sortedArrayUsingComparator:^NSComparisonResult(UIView *a,UIView *b) {
            CGFloat xa=HGAHeaderBaseMidX(a,header),xb=HGAHeaderBaseMidX(b,header);
            return xa<xb ? NSOrderedAscending : (xa>xb ? NSOrderedDescending : NSOrderedSame);
        }];
        CGFloat totalW=0;
        for(UIView *cell in sorted)totalW+=CGRectGetWidth(cell.bounds);
        CGFloat gap=sorted.count>1 ? 4.0 : 0.0;
        totalW+=gap*(CGFloat)(sorted.count-1);
        CGRect tabInHeader=[tabContainer convertRect:tabContainer.bounds toView:header];
        CGFloat cursor=CGRectGetMidX(tabInHeader)-totalW*0.5;
        for(UIView *cell in sorted) {
            CGFloat w=CGRectGetWidth(cell.bounds);
            CGFloat targetMidX=cursor+w*0.5;
            CGFloat baseMidX=HGAHeaderBaseMidX(cell,header);
            CGFloat dx=round(targetMidX-baseMidX);
            HGASetGeometry(cell,dx,0,1.0);
            [self.compactedCells addObject:cell];
            cursor+=w+gap;
        }
    } else {
        [self restoreCompactedCells];
    }

    BOOL showCenter=tabContainer && !tabContainer.hidden && tabContainer.alpha>=0.01 &&
        !CGRectIsEmpty(tabContainer.bounds) && !allChannelsHidden &&
        (visibleCells.count>0 || HGAHeaderChannelCells(tabContainer,NO).count==0);
    UIView *categoryView=HGAHeaderCategoryView(tabContainer);
    if(showCenter) {
        CGRect contentRect=CGRectNull;
        if(visibleCells.count>0) {
            for(UIView *cell in visibleCells) {
                CGRect r=[cell convertRect:cell.bounds toView:header];
                contentRect=CGRectIsNull(contentRect) ? r : CGRectUnion(contentRect,r);
            }
            contentRect=CGRectInset(contentRect,-12.0,0);
        } else {
            contentRect=[tabContainer convertRect:tabContainer.bounds toView:header];
        }
        CGFloat minLeft=CGRectIsNull(leftFrame) ? 8.0 : (CGRectGetMaxX(leftFrame)+8.0);
        CGFloat maxRight=CGRectIsNull(rightFrame) ? (headerW-8.0) : (CGRectGetMinX(rightFrame)-8.0);
        CGFloat availW=MAX(circleSize,maxRight-minLeft);
        CGFloat capsuleW=MIN(availW,MAX(circleSize,CGRectGetWidth(contentRect)));
        CGFloat desiredX=round(CGRectGetMidX(contentRect)-capsuleW*0.5);
        CGFloat clampedX=MIN(MAX(minLeft,desiredX),MAX(minLeft,maxRight-capsuleW));
        CGRect centerFrame=CGRectMake(clampedX,round(midY-circleSize*0.5),round(capsuleW),circleSize);
        if(!self.centerGlass)self.centerGlass=HGAMakeHeaderGlass(@"header.channels");
        HGAConfigureHeaderGlass(self.centerGlass,header,centerFrame,style);

        if(!self.pillGlass) {
            self.pillGlass=[[HGAAppearanceMaterial alloc] initWithEffect:nil];
            self.pillGlass.accessibilityIdentifier=@"honggo.appearance.header.pill";
            self.pillGlass.accessibilityElementsHidden=YES;
            self.pillGlass.userInteractionEnabled=NO;
            self.pillGlass.layer.cornerCurve=kCACornerCurveContinuous;
            self.pillGlass.clipsToBounds=YES;
        }
        if(self.pillGlass.superview!=self.centerGlass.contentView) {
            [self.centerGlass.contentView insertSubview:self.pillGlass atIndex:0];
        }
        [self updatePillAppearanceForDark:![style isEqual:@"solid"]];
        if(!self.touching) {
            UIView *selectedCell=[self selectedVisibleCellInTabContainer:tabContainer categoryView:categoryView];
            CGRect pillRect=[self pillFrameForCell:selectedCell];
            [self placePillFrame:pillRect animated:NO];
            [self dimIndicatorsForCategoryView:categoryView active:!CGRectIsEmpty(pillRect)];
        }
        if(!self.pressGesture) {
            self.pressGesture=[[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(handlePress:)];
            self.pressGesture.minimumPressDuration=0.0;
            self.pressGesture.cancelsTouchesInView=NO;
            self.pressGesture.delegate=self;
        }
        if(self.pressGesture.view!=tabContainer) {
            [self.pressGesture.view removeGestureRecognizer:self.pressGesture];
            [tabContainer addGestureRecognizer:self.pressGesture];
        }
    } else {
        HGARemoveHeaderGlass(self.centerGlass);
        [self.pillGlass removeFromSuperview];
        [self.pressGesture.view removeGestureRecognizer:self.pressGesture];
        [self restoreIndicators];
    }
}

- (void)teardown {
    HGARemoveHeaderGlass(self.sideGlass);
    HGARemoveHeaderGlass(self.centerGlass);
    HGARemoveHeaderGlass(self.searchGlass);
    [self.pillGlass removeFromSuperview];
    [self.pressGesture.view removeGestureRecognizer:self.pressGesture];
    [self restoreCompactedCells];
    [self restoreIndicators];
    self.touching=NO;
    self.hoverIndex=NSNotFound;
}

@end

static BOOL HGAApplyDetachedHeaderGlass(UIView *header,BOOL enabled,BOOL finder,NSString *style) {
    UIView *sideBar=HGAViewGetter(header,"sideBar",header);
    UIView *tabContainer=HGAViewGetter(header,"tabContainer",header);
    UIView *searchSlot=HGAHeaderSearchSlot(header);
    HGAHeaderDetachedController *controller=objc_getAssociatedObject(header,&HGAHeaderDetachedKey);
    if(!sideBar && !tabContainer && !searchSlot) {
        if(controller) {
            [controller teardown];
            objc_setAssociatedObject(header,&HGAHeaderDetachedKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        return NO;
    }
    if(!enabled || finder || header.hidden) {
        if(controller) {
            [controller teardown];
            objc_setAssociatedObject(header,&HGAHeaderDetachedKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        HGARemoveGlass(header);
        return YES;
    }
    HGARemoveGlass(header);
    if(!controller) {
        controller=[HGAHeaderDetachedController new];
        objc_setAssociatedObject(header,&HGAHeaderDetachedKey,controller,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    [controller syncWithHeader:header style:style];
    return YES;
}
