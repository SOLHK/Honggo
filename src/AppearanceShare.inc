// A single material behind the sheet's native controls and content.
@interface HGAActionSheetState : NSObject
@property(nonatomic,weak) UIView *surface;
@property(nonatomic,strong) NSMapTable<UIView *,UIColor *> *covers;
@property(nonatomic,strong) NSMapTable<UIVisualEffectView *,UIVisualEffect *> *effects;
@property(nonatomic,strong) NSHashTable<UIView *> *shareCells;
@property(nonatomic) BOOL rounding;
@property(nonatomic) CGFloat sourceRadius;
@property(nonatomic) CGFloat appliedRadius;
@property(nonatomic,copy) NSString *sourceCurve;
@property(nonatomic) CACornerMask sourceCorners;
@property(nonatomic) BOOL sourceClips;
@end
@implementation HGAActionSheetState @end
static char HGAActionSheetKey;

@interface HGAShareInk : NSObject
@property(nonatomic,weak) UIImageView *icon;
@property(nonatomic,weak) UIView *plate;
@property(nonatomic,strong) UIImage *original;
@property(nonatomic,strong) UIImage *applied;
@property(nonatomic,strong) UIColor *tint;
@property(nonatomic,strong) UIColor *paint;
@end
@implementation HGAShareInk @end
static char HGAShareInkKey;

static UIColor *HGAShareForeground(void) {
    static UIColor *color;
    static dispatch_once_t once;
    dispatch_once(&once, ^{color=[UIColor colorWithDynamicProvider:^UIColor *(UITraitCollection *traits) {
        return traits.userInterfaceStyle==UIUserInterfaceStyleDark ? UIColor.whiteColor : UIColor.blackColor;
    }];});
    return color;
}

static UIColor *HGASharePlate(void) {
    static UIColor *color;
    static dispatch_once_t once;
    dispatch_once(&once, ^{color=[UIColor colorWithDynamicProvider:^UIColor *(UITraitCollection *traits) {
        return traits.userInterfaceStyle==UIUserInterfaceStyleDark
            ? [UIColor.whiteColor colorWithAlphaComponent:0.14] : [UIColor.blackColor colorWithAlphaComponent:0.07];
    }];});
    return color;
}

static UIImage *HGAShareTemplate(UIImage *image) {
    UIImage *templated=[image imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
    CGImageRef source=image.CGImage;
    if(!source || image.isSymbolImage || image.renderingMode==UIImageRenderingModeAlwaysTemplate)return templated;
    size_t width=CGImageGetWidth(source),height=CGImageGetHeight(source);
    if(width<8 || height<8 || width>256 || height>256)return templated;
    CGColorSpaceRef space=CGColorSpaceCreateDeviceRGB();
    CGContextRef context=CGBitmapContextCreate(NULL,width,height,8,width*4,space,
        kCGBitmapByteOrder32Big|kCGImageAlphaPremultipliedLast);
    CGColorSpaceRelease(space);
    if(!context)return templated;
    CGContextDrawImage(context,CGRectMake(0,0,width,height),source);
    UInt8 *pixels=CGBitmapContextGetData(context);
    // Some destinations supply the glyph and a solid brand tile in one
    // asset. Only separate the glyph when the four inset corners agree.
    const UInt8 *samples[]={pixels+4*((height/5)*width+width/5),pixels+4*((height/5)*width+width*4/5),
        pixels+4*((height*4/5)*width+width/5),pixels+4*((height*4/5)*width+width*4/5)};
    int red=samples[0][0],green=samples[0][1],blue=samples[0][2];
    BOOL solid=YES;
    for(NSUInteger index=0;index<4;index++) {
        const UInt8 *pixel=samples[index];
        if(pixel[3]<245 || abs(pixel[0]-red)>20 || abs(pixel[1]-green)>20 || abs(pixel[2]-blue)>20)solid=NO;
    }
    if(solid) {
        NSUInteger marks=0,covered=0;
        for(size_t index=0;index<width*height;index++) {
            UInt8 *pixel=pixels+4*index;
            unsigned alpha=pixel[3];
            if(alpha>128)covered++;
            unsigned delta=MAX(abs(pixel[0]*255-(int)alpha*red),
                MAX(abs(pixel[1]*255-(int)alpha*green),abs(pixel[2]*255-(int)alpha*blue)))/255;
            alpha=delta>24 ? MIN(alpha,(delta-24)*3) : 0;
            if(alpha>64)marks++;
            pixel[0]=pixel[1]=pixel[2]=pixel[3]=alpha;
        }
        if(marks>width*height/80 && marks<covered*0.66) {
            CGImageRef glyph=CGBitmapContextCreateImage(context);
            if(glyph) {
                templated=[[UIImage imageWithCGImage:glyph scale:image.scale orientation:image.imageOrientation]
                    imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
                CGImageRelease(glyph);
            }
        }
    }
    CGContextRelease(context);
    return templated;
}

static void HGARestoreShareInk(UIView *cell) {
    HGAShareInk *ink=objc_getAssociatedObject(cell,&HGAShareInkKey);
    if(!ink)return;
    if(ink.icon.image==ink.applied)ink.icon.image=ink.original;
    if([ink.icon.tintColor isEqual:HGAShareForeground()])ink.icon.tintColor=ink.tint;
    if([ink.plate.backgroundColor isEqual:HGASharePlate()])ink.plate.backgroundColor=ink.paint;
    objc_setAssociatedObject(cell,&HGAShareInkKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

static void HGAApplyShareInk(UIView *cell) {
    UIView *view=HGAViewGetter(cell,"iconView",cell);
    if(!view)view=HGAViewGetter(cell,"iconImageView",cell);
    UIView *plate=HGAViewGetter(cell,"iconBackgroundView",cell);
    if(!plate)plate=HGAViewGetter(cell,"imageBottomMaskView",cell);
    UIImageView *icon=[view isKindOfClass:UIImageView.class] ? (id)view : nil;
    HGAShareInk *ink=objc_getAssociatedObject(cell,&HGAShareInkKey);
    if(ink && (ink.icon!=icon || ink.plate!=plate || !icon.image)) {
        HGARestoreShareInk(cell);ink=nil;
    }
    if(!icon.image || plate==icon)return;
    if(!ink) {
        ink=[HGAShareInk new];ink.icon=icon;ink.plate=plate;
        ink.tint=icon.tintColor;ink.paint=plate.backgroundColor;
        objc_setAssociatedObject(cell,&HGAShareInkKey,ink,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    if(icon.image!=ink.applied && icon.image!=ink.original) {
        ink.original=icon.image;
        ink.applied=HGAShareTemplate(icon.image);
    }
    if(![icon.tintColor isEqual:HGAShareForeground()]){
        ink.tint=icon.tintColor;icon.tintColor=HGAShareForeground();
    }
    if(plate && ![plate.backgroundColor isEqual:HGASharePlate()]) {
        ink.paint=plate.backgroundColor;plate.backgroundColor=HGASharePlate();
    }
    if(icon.image!=ink.applied)icon.image=ink.applied;
}

static void HGARestoreActionSheet(UIView *root) {
    HGAActionSheetState *state=objc_getAssociatedObject(root,&HGAActionSheetKey);
    if(!state)return;
    UIView *surface=state.surface;
    HGARemoveGlass(surface);
    for(UIView *cover in state.covers.keyEnumerator.allObjects) {
        if([cover.backgroundColor isEqual:UIColor.clearColor])cover.backgroundColor=[state.covers objectForKey:cover];
    }
    for(UIVisualEffectView *backing in state.effects.keyEnumerator)
        if(!backing.effect)backing.effect=[state.effects objectForKey:backing];
    for(UIView *cell in state.shareCells)HGARestoreShareInk(cell);
    if(state.rounding) {
        if(fabs(surface.layer.cornerRadius-state.appliedRadius)<0.01)surface.layer.cornerRadius=state.sourceRadius;
        if([surface.layer.cornerCurve isEqual:kCACornerCurveContinuous])surface.layer.cornerCurve=state.sourceCurve;
        if(surface.layer.maskedCorners==(kCALayerMinXMinYCorner|kCALayerMaxXMinYCorner))surface.layer.maskedCorners=state.sourceCorners;
        if(surface.clipsToBounds)surface.clipsToBounds=state.sourceClips;
    }
    objc_setAssociatedObject(root,&HGAActionSheetKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static UIView *HGAActionSheetSurface(UIView *root) {
    UIWindow *window=root.window;
    if(!window || CGRectIsEmpty(root.bounds))return nil;
    CGRect frame=[root convertRect:root.bounds toView:window];
    CGFloat windowHeight=CGRectGetHeight(window.bounds);
    if(frame.size.height<windowHeight*0.94)return root;
    // Some share implementations use a full-screen controller for their dim
    // and dismissal region. Only decorate a validated content getter there.
    UIViewController *owner=HGAOwner(root);
    for(NSString *name in @[@"panelView",@"containerView",@"contentView"]) {
        UIView *candidate=HGAViewGetter(owner,name.UTF8String,root);
        if(!candidate || candidate==root || [candidate isKindOfClass:UIScrollView.class])continue;
        CGRect rect=[candidate convertRect:candidate.bounds toView:window];
        if(rect.size.width>=window.bounds.size.width*0.8 && rect.size.height>=120 &&
            rect.size.height<windowHeight*0.94 && CGRectGetMaxY(rect)>=windowHeight-60 &&
            CGRectGetMaxY(rect)<=windowHeight+2)return candidate;
    }
    return nil;
}
static NSArray<NSString *> *HGAShareCells(void) {
    static NSArray<NSString *> *classes;
    static dispatch_once_t once;
    dispatch_once(&once, ^{classes=@[@"SSShortVideoMorePanelShareHeaderCell",@"SSVideoSharePanelControlCell",
        @"SSShortVideoShareActivityCell",@"SSShortVideoShareItemCell",@"FQVShortVideoSettingsShareCell",
        @"FQVShortVideoSettingsCell",@"FQVShortVideoSettingsFunCell"];});
    return classes;
}
static NSArray<NSString *> *HGAActionPanelSurfaces(void) {
    static NSArray<NSString *> *classes;
    static dispatch_once_t once;
    dispatch_once(&once, ^{classes=[@[@"SSShortVideoMorePanelShareHeaderView",@"SSVideoSharePanelView",@"SSVideoSharePanelControlView"]
        arrayByAddingObjectsFromArray:HGAShareCells()];});
    return classes;
}
static BOOL HGAPanelPart(UIView *view,NSArray<NSString *> *classes) {
    for(Class cls=object_getClass(view);cls && cls!=UIView.class;cls=class_getSuperclass(cls)) {
        if([classes containsObject:NSStringFromClass(cls)])return YES;
    }
    return NO;
}
static BOOL HGAIsShareCell(UIView *view) {
    return HGAPanelPart(view,HGAShareCells());
}
static BOOL HGAIsActionPanelSurface(UIView *view) {
    return HGAPanelPart(view,HGAActionPanelSurfaces());
}
static BOOL HGAIsCommentSurface(UIView *view) {
    return HGAPanelPart(view,HGACommentSurfaces());
}
static BOOL HGAPanelWantsGlass(NSString *kind) {
    return HGAppearanceEnabled() && HGAppearanceFlag([@"panels." stringByAppendingString:kind]);
}
static void HGAApplyActionSheet(UIView *root,NSString *kind) {
    BOOL enabled=HGAPanelWantsGlass(kind) && root.window;
    UIView *surface=enabled?HGAActionSheetSurface(root):nil;
    BOOL comment=[kind isEqual:@"comment"];
    // A nested comment list can have a second controller. Use the outer sheet
    // if it already owns a bounded panel that contains the list.
    if(surface && comment) {
        for(UIViewController *parent=HGAOwner(root).parentViewController;parent;parent=parent.parentViewController) {
            if(![HGAPanelKind(parent) isEqual:@"comment"])continue;
            UIView *outer=HGAActionSheetSurface(parent.viewIfLoaded);
            if(outer && surface!=outer && [surface isDescendantOfView:outer]) {surface=nil;enabled=NO;break;}
        }
    }
    HGAActionSheetState *state=objc_getAssociatedObject(root,&HGAActionSheetKey);
    if(state && state.surface!=surface){HGARestoreActionSheet(root);state=nil;}
    if(!surface) {
        if(enabled)HGRecordCapability([@"appearance.panel.surface." stringByAppendingString:kind],
            @{@"module":@"appearance",@"state":@"unavailable",@"reason":@"full-screen dismissal region has no verified bounded sheet; native panel retained"});
        return;
    }
    if(!state) {
        state=[HGAActionSheetState new];state.surface=surface;
        state.covers=[NSMapTable weakToStrongObjectsMapTable];
        state.effects=[NSMapTable weakToStrongObjectsMapTable];
        state.shareCells=[NSHashTable weakObjectsHashTable];
        objc_setAssociatedObject(root,&HGAActionSheetKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    CGFloat radius=MIN(28,MIN(surface.bounds.size.width,surface.bounds.size.height)/2);
    CGFloat opacity=UIAccessibilityIsReduceTransparencyEnabled()?1:HGAppearanceNumber(@"panels.opacity");
    HGAInstallGlass(surface,HGAppearanceString(@"panels.material"),radius,opacity);
    HGASetGlassOverlay(surface,[UIColor.systemBackgroundColor colorWithAlphaComponent:opacity]);
    // Never replace an existing mask (it may encode the host's transition).
    if(!surface.layer.mask) {
        if(!state.rounding || fabs(surface.layer.cornerRadius-state.appliedRadius)>=0.01)state.sourceRadius=surface.layer.cornerRadius;
        if(!state.rounding || ![surface.layer.cornerCurve isEqual:kCACornerCurveContinuous])state.sourceCurve=surface.layer.cornerCurve;
        CACornerMask corners=kCALayerMinXMinYCorner|kCALayerMaxXMinYCorner;
        if(!state.rounding || surface.layer.maskedCorners!=corners)state.sourceCorners=surface.layer.maskedCorners;
        if(!state.rounding || !surface.clipsToBounds)state.sourceClips=surface.clipsToBounds;
        state.rounding=YES;state.appliedRadius=radius;
        if(fabs(surface.layer.cornerRadius-radius)>=0.01)surface.layer.cornerRadius=radius;
        if(![surface.layer.cornerCurve isEqual:kCACornerCurveContinuous])surface.layer.cornerCurve=kCACornerCurveContinuous;
        if(surface.layer.maskedCorners!=corners)surface.layer.maskedCorners=corners;
        if(!surface.clipsToBounds)surface.clipsToBounds=YES;
    }
    for(UIView *cover in state.covers.keyEnumerator.allObjects) {
        if(![cover isDescendantOfView:surface]) {
            if([cover.backgroundColor isEqual:UIColor.clearColor])cover.backgroundColor=[state.covers objectForKey:cover];
            [state.covers removeObjectForKey:cover];
        }
    }
    for(UIVisualEffectView *backing in state.effects.keyEnumerator.allObjects) {
        if(![backing isDescendantOfView:surface]) {
            if(!backing.effect)backing.effect=[state.effects objectForKey:backing];
            [state.effects removeObjectForKey:backing];
        }
    }
    for(UIView *cell in state.shareCells.allObjects) {
        if(![cell isDescendantOfView:surface]){HGARestoreShareInk(cell);[state.shareCells removeObject:cell];}
    }
    BOOL share=[kind isEqual:@"share"] || [kind isEqual:@"longpress"];
    BOOL compose=NO;
    for(UIView *view in HGADescendants(surface,240)) {
        if(view==surface || HGAOwnView(view) || view.hidden)continue;
        if(share && [NSStringFromClass(view.class) hasPrefix:@"RKMPandroidx.compose.ui.window.ComposeView"])compose=YES;
        if(share && HGAIsShareCell(view)) {
            [state.shareCells addObject:view];HGAApplyShareInk(view);
        }
        if(CGRectGetHeight(view.bounds)<16 || CGRectGetHeight(view.bounds)>CGRectGetHeight(surface.bounds)+2 ||
            CGRectGetWidth(view.bounds)<CGRectGetWidth(surface.bounds)*0.8)continue;
        if(comment && view.class==UIVisualEffectView.class &&
            [view.superview isKindOfClass:NSClassFromString(@"FQCShortVideoBottomToolBar")]) {
            UIVisualEffectView *backing=(id)view;
            if(backing.effect){[state.effects setObject:backing.effect forKey:backing];backing.effect=nil;}
            // Preserve editor controls hosted by the effect's content view.
        }
        BOOL backing=view.class==UIView.class || view.class==UITableView.class ||
            view.class==UICollectionView.class || HGAIsActionPanelSurface(view) ||
            (share && view.class==UITableViewCell.class) ||
            (comment && (HGAIsCommentSurface(view) ||
                [view isKindOfClass:NSClassFromString(@"FQReadingNavigationBar")] ||
                [view isKindOfClass:NSClassFromString(@"FQCShortVideoCommentCell")]));
        if(!backing || !view.backgroundColor)continue;
        CGFloat r=0,g=0,b=0,a=0;
        if([[view.backgroundColor resolvedColorWithTraitCollection:view.traitCollection] getRed:&r green:&g blue:&b alpha:&a] &&
            a>0.8 && MAX(r,MAX(g,b))-MIN(r,MIN(g,b))<0.08) {
            [state.covers setObject:view.backgroundColor forKey:view];view.backgroundColor=UIColor.clearColor;
        }
    }
    HGRecordCapability([@"appearance.panel.surface." stringByAppendingString:kind],
        @{@"module":@"appearance",@"state":compose ? @"partial" : @"installed",
          @"reason":compose ? @"bounded glass behind the host Compose canvas; in-canvas paint and icons are controlled by the host renderer" :
            @"single bounded glass sheet behind native content and actions"});
}
static void HGAUpdateActionPanelSurface(UIView *view) {
    UIViewController *owner=HGAOwner(view);
    NSString *kind=HGAPanelKind(owner);
    if(HGAIsShareCell(view)) {
        HGAActionSheetState *state=objc_getAssociatedObject(owner.viewIfLoaded,&HGAActionSheetKey);
        if(([kind isEqual:@"share"] || [kind isEqual:@"longpress"]) && HGAPanelWantsGlass(kind) &&
            view.window && [view isDescendantOfView:state.surface]) {
            [state.shareCells addObject:view];HGAApplyShareInk(view);
        }else HGARestoreShareInk(view);
        return;
    }
    if([@[@"share",@"longpress",@"speed"] containsObject:kind] && owner.isViewLoaded) {
        if(!HGAControllers)HGAControllers=[NSHashTable weakObjectsHashTable];
        [HGAControllers addObject:owner];
        HGAApplyActionSheet(owner.view,kind);
    }
}
