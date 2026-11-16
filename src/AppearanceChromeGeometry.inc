@interface HGAHeaderInsetState : NSObject
@property(nonatomic,weak) UIView *sidebar;
@property(nonatomic,weak) UIView *searchSlot;
@end
@implementation HGAHeaderInsetState @end
static char HGAHeaderInsetKey;

static void HGAInsetHeaderTarget(UIView *target,UIView *header,CGFloat edge,CGRect channels,BOOL leading) {
    if(!target)return;
    HGAViewState *state=HGAState(target,NO);
    BOOL owned=state.geometry && state.geometrySuperview==target.superview &&
        CGAffineTransformEqualToTransform(target.transform,state.appliedTransform);
    CGAffineTransform base=owned ? state.sourceTransform : target.transform;
    if(!HGAVisible(target) || !CGAffineTransformIsIdentity(base)) {HGARestoreGeometry(target);return;}
    for(UIView *parent=target.superview;parent && parent!=header;parent=parent.superview)
        if(!CGAffineTransformIsIdentity(parent.transform)) {HGARestoreGeometry(target);return;}

    CGRect rect=[target convertRect:target.bounds toView:header];
    CGFloat center=CGRectGetMidX(rect)-(owned ? state.appliedTransform.tx : 0);
    CGFloat requested=edge+(leading ? 23 : -23);
    if(!CGRectIsNull(channels)) {
        CGFloat half=CGRectGetWidth(rect)/2;
        requested=leading ? MIN(requested,CGRectGetMinX(channels)-half) :
            MAX(requested,CGRectGetMaxX(channels)+half);
    }
    CGFloat offset=leading ? MAX(0,requested-center) : MIN(0,requested-center);
    HGASetGeometry(target,offset,0,1);
}

// The 7.4.3.33 header's search container spans the entire 440pt row. Its
// searchButtonSlotView and sideBar own the native 24pt tap targets. Move those
// views in place to align their centers, keeping their sizes and actions.
static void HGAApplyHeaderInsets(UIView *header,BOOL enabled,BOOL finder,CGFloat width) {
    HGAHeaderInsetState *state=objc_getAssociatedObject(header,&HGAHeaderInsetKey);
    if(!enabled || finder) {
        HGARestoreGeometry(state.sidebar);HGARestoreGeometry(state.searchSlot);
        if(state)objc_setAssociatedObject(header,&HGAHeaderInsetKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        return;
    }
    UIView *sidebar=HGAViewGetter(header,"sideBar",header);
    UIView *search=HGAViewGetter(header,"searchViewContainer",header);
    UIView *slot=HGAViewGetter(search,"searchButtonSlotView",search);
    if(slot==search)slot=nil;
    if(!state) {
        state=[HGAHeaderInsetState new];
        objc_setAssociatedObject(header,&HGAHeaderInsetKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    if(state.sidebar!=sidebar)HGARestoreGeometry(state.sidebar);
    if(state.searchSlot!=slot)HGARestoreGeometry(state.searchSlot);
    state.sidebar=sidebar;state.searchSlot=slot;
    UIView *tabs=HGAViewGetter(header,"tabContainer",header);
    CGRect channels=HGAVisible(tabs) ? [tabs convertRect:tabs.bounds toView:header] : CGRectNull;
    CGFloat middle=CGRectGetMidX(header.bounds);
    HGAInsetHeaderTarget(sidebar,header,middle-width/2,channels,YES);
    HGAInsetHeaderTarget(slot,header,middle+width/2,channels,NO);
}

static CGRect HGAHeaderGlassFrame(UIView *header,CGFloat width) {
    CGRect frame=CGRectMake(CGRectGetMidX(header.bounds)-width/2,CGRectGetMinY(header.bounds),
        width,CGRectGetHeight(header.bounds));
    BOOL channelsHidden=HGATopChannelHidden(@"关注") && HGATopChannelHidden(@"漫剧") &&
        HGATopChannelHidden(@"真人剧") && HGATopChannelHidden(@"推荐");
    BOOL menuHidden=HGAppearanceFlag(@"layout.channels.hide.sidebar");
    BOOL searchHidden=HGAppearanceFlag(@"layout.channels.hide.search");
    if(channelsHidden && menuHidden!=searchHidden) {
        UIView *target=menuHidden ? HGAViewGetter(HGAViewGetter(header,"searchViewContainer",header),
            "searchButtonSlotView",header) : HGAViewGetter(header,"sideBar",header);
        if(target && !target.hidden && !CGRectIsEmpty(target.bounds)) {
            // Shrink only the decoration. Keep the original search/menu target,
            // gesture and position, including the common inset above.
            CGRect control=[target convertRect:target.bounds toView:header];
            frame.size.width=frame.size.height;
            frame.origin.x=CGRectGetMidX(control)-frame.size.width/2;
        }
    }
    return frame;
}
