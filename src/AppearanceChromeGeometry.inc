@interface HGAHeaderInsetState : NSObject
@property(nonatomic,weak) UIView *sidebar;
@property(nonatomic,weak) UIView *searchSlot;
@property(nonatomic,strong) HGAAppearanceMaterial *sidebarGlass;
@property(nonatomic,strong) HGAAppearanceMaterial *searchGlass;
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
    BOOL styleChanged=(glass.overrideUserInterfaceStyle!=desired);
    if(styleChanged) {
        glass.overrideUserInterfaceStyle=desired;
        if(@available(iOS 17.0,*))[glass updateTraitsIfNeeded];
    }
    glass.interactiveGlass=!solid && ![style isEqual:@"blur"] && !reduced;
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
