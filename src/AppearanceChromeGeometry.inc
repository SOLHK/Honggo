@interface HGAHeaderInsetState : NSObject
@property(nonatomic,weak) UIView *sidebar;
@property(nonatomic,weak) UIView *searchSlot;
@property(nonatomic,strong) HGAAppearanceMaterial *sidebarGlass;
@property(nonatomic,strong) HGAAppearanceMaterial *searchGlass;
@property(nonatomic,strong) NSHashTable<UIView *> *compactedViews;
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

static CGFloat HGAOwnedTx(UIView *view) {
    HGAViewState *s=HGAState(view,NO);
    if(s.geometry && s.geometrySuperview==view.superview &&
        CGAffineTransformEqualToTransform(view.transform,s.appliedTransform)) {
        return s.appliedTransform.tx;
    }
    return 0.0;
}

static void HGARestoreCompactedHeaderChannels(HGAHeaderInsetState *state) {
    if(!state.compactedViews)return;
    for(UIView *v in state.compactedViews.allObjects)HGARestoreGeometry(v);
    [state.compactedViews removeAllObjects];
}

// When one or more top channels are hidden (e.g. hiding 关注 and 真人剧 leaves a
// 102pt void between 漫剧 and 推荐), compact the visible cells with an even 20pt
// gap centered in their host container and align the active indicator underline.
static void HGACompactHeaderChannels(UIView *header,HGAHeaderInsetState *state,BOOL enabled) {
    if(!header || !state)return;
    UIView *tabs=HGAViewGetter(header,"tabContainer",header);
    Class cellCls=NSClassFromString(@"FQRJXCategoryTitleCell");
    Class catCls=NSClassFromString(@"FQRJXCategoryTitleView");
    if(!enabled || !tabs || tabs.hidden || !cellCls) {
        HGARestoreCompactedHeaderChannels(state);
        return;
    }
    NSMutableArray<UIView *> *visibleCells=[NSMutableArray array];
    NSUInteger hiddenCount=0;
    UIView *categoryView=nil;
    for(UIView *node in HGADescendants(tabs,96)) {
        if(!categoryView && catCls && [node isKindOfClass:catCls])categoryView=node;
        if(![node isKindOfClass:cellCls])continue;
        if(node.hidden || node.alpha<0.01 || CGRectIsEmpty(node.bounds)) {
            hiddenCount++;
        } else {
            [visibleCells addObject:node];
        }
    }
    if(!hiddenCount || !visibleCells.count) {
        HGARestoreCompactedHeaderChannels(state);
        return;
    }
    UIView *container=visibleCells.firstObject.superview;
    if(!container || CGRectIsEmpty(container.bounds)) {
        HGARestoreCompactedHeaderChannels(state);
        return;
    }
    for(UIView *c in visibleCells) {
        if(c.superview!=container) {
            HGARestoreCompactedHeaderChannels(state);
            return;
        }
    }
    [visibleCells sortUsingComparator:^NSComparisonResult(UIView *a,UIView *b) {
        CGFloat ax=CGRectGetMinX(a.frame)-HGAOwnedTx(a);
        CGFloat bx=CGRectGetMinX(b.frame)-HGAOwnedTx(b);
        return ax<bx ? NSOrderedAscending : (ax>bx ? NSOrderedDescending : NSOrderedSame);
    }];
    static const CGFloat kGap=20.0;
    CGFloat totalW=0.0;
    for(UIView *c in visibleCells)totalW+=CGRectGetWidth(c.bounds);
    totalW+=kGap*(visibleCells.count>1 ? (visibleCells.count-1) : 0);
    CGFloat cursorX=round(CGRectGetMidX(container.bounds)-totalW*0.5);
    if(!state.compactedViews)state.compactedViews=[NSHashTable weakObjectsHashTable];
    NSMutableArray<NSNumber *> *naturalMidXs=[NSMutableArray arrayWithCapacity:visibleCells.count];
    NSMutableArray<NSNumber *> *deltas=[NSMutableArray arrayWithCapacity:visibleCells.count];
    for(UIView *c in visibleCells) {
        CGFloat baseMinX=CGRectGetMinX(c.frame)-HGAOwnedTx(c);
        CGFloat w=CGRectGetWidth(c.bounds);
        CGFloat delta=cursorX-baseMinX;
        [naturalMidXs addObject:@(baseMinX+w*0.5)];
        [deltas addObject:@(delta)];
        HGASetGeometry(c,delta,0,1);
        [state.compactedViews addObject:c];
        cursorX+=w+kGap;
    }
    id indicators=categoryView ? HGReadObjectGetter(categoryView,"indicators") : nil;
    if([indicators isKindOfClass:NSArray.class]) {
        for(id item in (NSArray *)indicators) {
            if(![item isKindOfClass:UIView.class])continue;
            UIView *ind=(UIView *)item;
            if(ind.hidden || !ind.superview)continue;
            CGFloat indMidX=CGRectGetMidX(ind.frame)-HGAOwnedTx(ind);
            if(ind.superview!=container) {
                CGRect converted=[ind.superview convertRect:CGRectOffset(ind.frame,-HGAOwnedTx(ind),0) toView:container];
                indMidX=CGRectGetMidX(converted);
            }
            NSUInteger bestIdx=0;
            CGFloat bestDist=CGFLOAT_MAX;
            for(NSUInteger i=0;i<naturalMidXs.count;i++) {
                CGFloat d=fabs(naturalMidXs[i].doubleValue-indMidX);
                if(d<bestDist){bestDist=d;bestIdx=i;}
            }
            HGASetGeometry(ind,deltas[bestIdx].doubleValue,0,1);
            [state.compactedViews addObject:ind];
        }
    }
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
    HGARestoreCompactedHeaderChannels(state);
    HGARestoreGeometry(state.sidebar);
    HGARestoreGeometry(state.searchSlot);
    [state.sidebarGlass removeFromSuperview];
    [state.searchGlass removeFromSuperview];
    objc_setAssociatedObject(header,&HGAHeaderInsetKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

static CGRect HGAHeaderVisibleChannelsRect(UIView *header) {
    BOOL allChannelsHidden=HGATopChannelHidden(@"关注") && HGATopChannelHidden(@"漫剧") &&
        HGATopChannelHidden(@"真人剧") && HGATopChannelHidden(@"推荐");
    if(allChannelsHidden)return CGRectNull;
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
    // Keep host targets strictly at their native positions.
    HGARestoreGeometry(state.sidebar);
    HGARestoreGeometry(state.searchSlot);

    UIView *sidebar=HGAViewGetter(header,"sideBar",header);
    UIView *searchSlot=HGAHeaderSearchSlotView(header);
    state.sidebar=sidebar;
    state.searchSlot=searchSlot;

    // Compact remaining visible tabs when any channel is hidden so there is no
    // empty gap inside the center capsule.
    HGACompactHeaderChannels(header,state,YES);

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
        leftCircle=CGRectMake(x,midY-side*0.5,side,side);
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
        rightCircle=CGRectMake(x,midY-side*0.5,side,side);
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
