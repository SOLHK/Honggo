// The host reuses recommendation views for many unrelated links. Only style
// the two named player badges, and let the host keep its text and tap actions.
static NSString *HGAPlayerBadgePreference(id text) {
    if([text isKindOfClass:NSAttributedString.class])text=[text string];
    if(![text isKindOfClass:NSString.class])return nil;
    if([text hasPrefix:@"追更榜"])return @"player.hide.followRank";
    if([text hasPrefix:@"系列剧"])return @"player.hide.seriesBadge";
    return nil;
}

@interface HGAPlayerBadgeGlyph : UIImageView
@property(nonatomic,weak) UIView *source;
@property(nonatomic,strong) CALayer *emptyMask;
@property(nonatomic,strong) CALayer *originalMask;
@property(nonatomic,copy) NSString *symbol;
- (void)detach;
@end
@implementation HGAPlayerBadgeGlyph
- (void)detach {
    if(self.source && self.source.layer.mask==self.emptyMask)self.source.layer.mask=self.originalMask;
    self.source=nil;
    self.originalMask=nil;
    [self removeFromSuperview];
}
@end

@interface HGAPlayerBadgeState : NSObject
@property(nonatomic,weak) UIView *paint;
@property(nonatomic,strong) HGAPlayerBadgeGlyph *leading;
@property(nonatomic,strong) HGAPlayerBadgeGlyph *arrow;
@end
@implementation HGAPlayerBadgeState @end
static char HGAPlayerBadgeKey;

static HGAPlayerBadgeGlyph *HGAUpdateBadgeGlyph(HGAPlayerBadgeGlyph *glyph,UIView *source,
                                               NSString *symbol,UIColor *ink) {
    if(glyph.source!=source)[glyph detach];
    if(!source.superview || source.hidden || CGRectIsEmpty(source.bounds)){[glyph detach];return nil;}
    if(!glyph) {
        glyph=[HGAPlayerBadgeGlyph new];
        glyph.emptyMask=[CALayer layer];
        glyph.accessibilityIdentifier=@"honggo.appearance.playerBadgeGlyph";
        glyph.isAccessibilityElement=NO;
        glyph.userInteractionEnabled=NO;
        glyph.contentMode=UIViewContentModeScaleAspectFit;
    }
    // The host loads its icon asynchronously. Mask just its drawing so a late
    // image can still load and will already be current when the style is removed.
    glyph.source=source;
    if(source.layer.mask!=glyph.emptyMask) {
        glyph.originalMask=source.layer.mask;
        source.layer.mask=glyph.emptyMask;
    }
    if(glyph.superview!=source.superview)[source.superview insertSubview:glyph aboveSubview:source];
    if(![glyph.symbol isEqual:symbol]) {
        glyph.symbol=symbol;
        glyph.image=[UIImage systemImageNamed:symbol] ?: [UIImage systemImageNamed:@"rosette"];
    }
    if(!CGRectEqualToRect(glyph.frame,source.frame))glyph.frame=source.frame;
    glyph.tintColor=ink;
    return glyph;
}

static void HGAApplyPlayerBadge(UIView *badge,BOOL tag) {
    BOOL inPlayer=NO;
    for(UIView *v=badge.superview;v && !inPlayer;v=v.superview)
        inPlayer=[v isKindOfClass:objc_lookUpClass("SSShortVideoAboveLeftContainerView")] ||
            [v isKindOfClass:objc_lookUpClass("FQVShortVideoBaseLeftContainerView")];
    NSString *key=nil;
    if(inPlayer && badge.window && HGAppearanceEnabled()) {
        id model=HGReadObjectGetter(badge,tag?"recTagInfo":"dataModel");
        key=HGAPlayerBadgePreference(HGReadObjectGetter(model,tag?"content":"reasonText"));
        if(!key && !tag)key=HGAPlayerBadgePreference(HGReadObjectGetter(model,"contentText"));
        if(!key) {
            UIView *title=HGAViewGetter(badge,tag?"textLabel":"reasonTextView",badge);
            key=HGAPlayerBadgePreference(HGReadObjectGetter(title,"text"));
        }
    }
    HGAPlayerBadgeState *state=objc_getAssociatedObject(badge,&HGAPlayerBadgeKey);
    BOOL hide=key && HGAppearanceFlag(key);
    HGASetHidden(badge,hide);
    if(!key || hide || CGRectIsEmpty(badge.bounds)) {
        if(state) {
            [state.leading detach];[state.arrow detach];
            HGASetHidden(state.paint,NO);
            HGARemoveGlass(badge);
            objc_setAssociatedObject(badge,&HGAPlayerBadgeKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        return;
    }
    if(!state) {
        state=[HGAPlayerBadgeState new];
        objc_setAssociatedObject(badge,&HGAPlayerBadgeKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    UIView *paint=HGAViewGetter(badge,tag?"bgView":"gradientBackgroundView",badge);
    if(state.paint!=paint)HGASetHidden(state.paint,NO);
    state.paint=paint;
    if(paint)HGASetHidden(paint,YES);
    BOOL series=[key isEqual:@"player.hide.seriesBadge"];
    NSString *material=series ? HGAppearanceString(@"player.material") : HGALiquidButtonMaterial();
    BOOL night=badge.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark;
    HGABoolGetter(badge,@"fqv_isDarkMode",&night);
    HGAInstallButtonGlassWithAppearance(badge,material,badge.bounds,CGRectGetHeight(badge.bounds)/2,
        night ? UIUserInterfaceStyleDark : UIUserInterfaceStyleLight);
    UIColor *ink=night ? UIColor.whiteColor : UIColor.blackColor;
    NSString *symbol=[key isEqual:@"player.hide.followRank"] ? @"trophy.fill" : @"play.rectangle.fill";
    state.leading=HGAUpdateBadgeGlyph(state.leading,HGAViewGetter(badge,tag?"leftIcon":"iconView",badge),symbol,ink);
    state.arrow=HGAUpdateBadgeGlyph(state.arrow,HGAViewGetter(badge,tag?"rightIcon":"arrowImgView",badge),@"chevron.right",ink);
}
