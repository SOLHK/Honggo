// Mounted action sheets cover the plugin's Dock while the host owns the panel.
// This noninteractive marker follows the sheet through interactive dismissal
// and also handles hosts that remove their content without a controller callback.
static void HGARefreshSheetDockVisibility(UIWindow *window);
static void HGARestoreSheetPresentation(UIView *root);
static NSHashTable<UIView *> *HGAMountedActionSheets;
static BOOL HGAWindowHasActionSheet(UIWindow *window) {
    if(!window)return NO;
    for(UIView *marker in HGAMountedActionSheets) {
        UIView *surface=marker.superview;
        if(surface.window!=window || !HGAVisible(surface))continue;
        CGRect rect=CGRectIntersection([surface convertRect:surface.bounds toView:window],window.bounds);
        CGFloat bottom=CGRectGetMaxY(window.bounds);
        if(rect.size.width>=window.bounds.size.width*0.8 &&
            CGRectGetMinY(rect)<bottom-48 && CGRectGetMaxY(rect)>=bottom-24)return YES;
    }
    return NO;
}
@interface HGAActionSheetMount : UIView
@property(nonatomic,weak) UIWindow *previousWindow;
@property(nonatomic,weak) UIView *sheetRoot;
@end
@implementation HGAActionSheetMount
- (void)didMoveToWindow {
    [super didMoveToWindow];
    UIWindow *old=self.previousWindow;
    self.previousWindow=self.window;
    if(self.window) {
        if(!HGAMountedActionSheets)HGAMountedActionSheets=[NSHashTable weakObjectsHashTable];
        [HGAMountedActionSheets addObject:self];
    } else [HGAMountedActionSheets removeObject:self];
    if(old && old!=self.window) {
        HGARestoreSheetPresentation(self.sheetRoot);
        HGARefreshSheetDockVisibility(old);
    }
    HGARefreshSheetDockVisibility(self.window);
}
@end

@interface HGAActionSheetPlate : NSObject
@property(nonatomic,weak) UIView *view;
@property(nonatomic,strong) UIColor *source;
@property(nonatomic,strong) UIColor *applied;
@property(nonatomic,copy) NSAttributedString *sourceText;
@property(nonatomic,copy) NSAttributedString *appliedText;
@end
@implementation HGAActionSheetPlate @end

// A single material behind the sheet's native controls and content.
@interface HGAActionSheetState : NSObject
@property(nonatomic,weak) UIView *surface;
@property(nonatomic,weak) UITableView *settingsTable;
@property(nonatomic,strong) HGAActionSheetMount *dockMount;
@property(nonatomic,strong) NSMapTable<UIView *,UIColor *> *covers;
@property(nonatomic,strong) NSMapTable<UIVisualEffectView *,UIVisualEffect *> *effects;
@property(nonatomic,strong) NSHashTable<UIView *> *shareCells;
@property(nonatomic,strong) NSMapTable<CALayer *,HGAActionSheetPlate *> *plates;
@property(nonatomic,strong) NSMapTable<UILabel *,HGAActionSheetPlate *> *labels;
@property(nonatomic,weak) UIView *dimmer;
@property(nonatomic,weak) UIView *episodePreview;
@property(nonatomic) CGFloat previewOffset;
@property(nonatomic,strong) UIColor *dimmerPaint;
@property(nonatomic,strong) UIColor *overlay;
@property(nonatomic) CGFloat overlayOpacity;
@property(nonatomic) BOOL nativeClear;
@property(nonatomic) BOOL unclipped;
@property(nonatomic) UIUserInterfaceStyle originalStyle;
@property(nonatomic) UIUserInterfaceStyle appliedStyle;
@property(nonatomic) BOOL rounding;
@property(nonatomic) CGFloat sourceRadius;
@property(nonatomic) CGFloat appliedRadius;
@property(nonatomic,copy) NSString *sourceCurve;
@property(nonatomic) CACornerMask sourceCorners;
@property(nonatomic) CACornerMask appliedCorners;
@property(nonatomic) BOOL sourceClips;
@end
@implementation HGAActionSheetState @end
static char HGAActionSheetKey;

static void HGARestoreSheetDimmer(HGAActionSheetState *state) {
    if([state.dimmer.backgroundColor isEqual:UIColor.clearColor])state.dimmer.backgroundColor=state.dimmerPaint;
    state.dimmer=nil;state.dimmerPaint=nil;
}

static void HGAAlignEpisodePreview(HGAActionSheetState *state,UIView *top) {
    UIView *previous=state.episodePreview;
    CGFloat oldOffset=state.previewOffset;
    CATransform3D applied=CATransform3DMakeTranslation(0,oldOffset,0);
    if(previous!=top) {
        if(previous && CATransform3DEqualToTransform(previous.layer.sublayerTransform,applied))
            previous.layer.sublayerTransform=CATransform3DIdentity;
        state.episodePreview=top;
        state.previewOffset=0;
    }
    if(!top)return;
    UIView *transition=top.superview;
    UIView *player=HGAViewGetter(top,"playerContainerView",top);
    CGFloat offset=0;
    if(player.superview==top && CGRectGetWidth(player.frame)>CGRectGetHeight(player.frame) &&
       fabs(CGRectGetWidth(player.frame)-CGRectGetWidth(top.bounds))<1) {
        // The native preview centers a full-width landscape movie above the
        // sheet. Translate only its visual contents to meet the card edge.
        CGRect sheet=[state.surface convertRect:state.surface.bounds toView:transition];
        CGFloat offscreen=MAX(0,CGRectGetMaxY(sheet)-CGRectGetMaxY(transition.bounds));
        offset=MAX(0,CGRectGetMaxY(top.bounds)-CGRectGetMaxY(player.frame)-offscreen);
    }
    CATransform3D current=top.layer.sublayerTransform;
    if(!CATransform3DIsIdentity(current) &&
       (previous!=top || !CATransform3DEqualToTransform(current,applied)))return;
    CATransform3D next=CATransform3DMakeTranslation(0,offset,0);
    if(!CATransform3DEqualToTransform(current,next))top.layer.sublayerTransform=next;
    state.previewOffset=offset;
}
static void HGARestoreSheetPresentation(UIView *root) {
    HGAActionSheetState *state=objc_getAssociatedObject(root,&HGAActionSheetKey);
    HGARestoreSheetDimmer(state);
    HGAAlignEpisodePreview(state,nil);
}

@interface HGAShareInk : NSObject
@property(nonatomic,weak) UIImageView *icon;
@property(nonatomic,weak) UIView *plate;
@property(nonatomic,strong) UIImage *original;
@property(nonatomic,strong) UIImage *applied;
@property(nonatomic,strong) UIColor *tint;
@property(nonatomic,strong) UIColor *appliedTint;
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

static UIColor *HGAActionSheetColor(CGFloat opacity) {
    return [UIColor colorWithDynamicProvider:^UIColor *(UITraitCollection *traits) {
        BOOL dark=traits.userInterfaceStyle==UIUserInterfaceStyleDark;
        // Over a black video surround, directly applying the opacity setting
        // leaves light-mode text on near-black glass. Keep a readable baseline
        // and map the setting over the remaining translucent range.
        CGFloat baseline=dark ? 0.54 : 0.62;
        CGFloat alpha=baseline+(1-baseline)*opacity;
        return [UIColor colorWithWhite:dark ? 0.10 : 1 alpha:alpha];
    }];
}

static BOOL HGASheetNeutralColor(UIColor *color,UITraitCollection *traits,CGFloat *white,CGFloat *alpha) {
    CGFloat red=0,green=0,blue=0;
    if(![[color resolvedColorWithTraitCollection:traits] getRed:&red green:&green blue:&blue alpha:alpha] ||
        MAX(red,MAX(green,blue))-MIN(red,MIN(green,blue))>=0.10)return NO;
    *white=0.2126*red+0.7152*green+0.0722*blue;
    return YES;
}

static void HGARestoreSheetLabel(HGAActionSheetState *state,UILabel *label) {
    HGAActionSheetPlate *ink=[state.labels objectForKey:label];
    if(ink) {
        BOOL ownedRich=ink.appliedText && (label.attributedText==ink.appliedText || [label.attributedText isEqual:ink.appliedText]);
        BOOL ownedColor=ink.applied && [label.textColor isEqual:ink.applied];
        if(ownedRich) {
            label.textColor=ink.source;
            if(ink.sourceText)label.attributedText=ink.sourceText;
        } else if(ownedColor && (!ink.appliedText || !label.attributedText)) {
            label.textColor=ink.source;
        }
    }
    [state.labels removeObjectForKey:label];
}

static BOOL HGASheetUsesConvertedInk(HGAActionSheetState *state) {
    return state && (state.nativeClear ||
        state.surface.overrideUserInterfaceStyle==UIUserInterfaceStyleDark ||
        state.surface.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark);
}

static void HGAApplySheetLabel(HGAActionSheetState *state,UILabel *label) {
    if(!state.labels || !label)return;
    HGAActionSheetPlate *ink=[state.labels objectForKey:label];
    UIColor *current=label.textColor;
    NSAttributedString *text=label.attributedText;
    BOOL ownedRich=ink.appliedText && (text==ink.appliedText || [text isEqual:ink.appliedText]);
    BOOL ownedColor=ink.applied && [current isEqual:ink.applied];
    BOOL owned=ink && (ink.appliedText ? ownedRich : ownedColor);
    UIColor *original=owned ? ink.source : current;
    if(owned)text=ink.sourceText;
    BOOL settingsRow=[label isDescendantOfView:state.settingsTable];
    // Playback settings and night-mode sheets may pin child rows to light traits.
    // Glass contrast follows the sheet surface whenever it overrides to dark.
    BOOL dark=(settingsRow || state.surface.overrideUserInterfaceStyle==UIUserInterfaceStyleDark ?
        state.surface : label).traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark;
    if(state.surface.overrideUserInterfaceStyle==UIUserInterfaceStyleDark)dark=YES;
    // Keep the text polarity supplied by the host for opaque selection chips.
    for(UIView *parent=label.superview;parent && parent!=state.surface;parent=parent.superview) {
        CGColorRef paint=parent.layer.backgroundColor;
        if(paint && CGColorGetAlpha(paint)>0.82){HGARestoreSheetLabel(state,label);return;}
    }
    if(text.length) {
        __block NSMutableAttributedString *rewritten=nil;
        __block BOOL changedRich=NO;
        __block BOOL hasAnyRunColor=NO;
        [text enumerateAttribute:NSForegroundColorAttributeName inRange:NSMakeRange(0,text.length) options:0
            usingBlock:^(id value,NSRange range,__unused BOOL *stop) {
                if([value isKindOfClass:UIColor.class])hasAnyRunColor=YES;
                UIColor *runColor=[value isKindOfClass:UIColor.class] ? value : original;
                CGFloat rw=0,ra=0;
                if(!HGASheetNeutralColor(runColor,label.traitCollection,&rw,&ra) ||
                    (dark ? rw>=0.68 : rw<=0.40))return;
                CGFloat rLevel=dark ? (rw<0.24 ? 0.98 : 0.78) : (rw>0.80 ? 0.10 : 0.30);
                if(!rewritten)rewritten=[text mutableCopy];
                [rewritten addAttribute:NSForegroundColorAttributeName
                    value:[UIColor colorWithWhite:rLevel alpha:ra] range:range];
                changedRich=YES;
            }];
        if(changedRich) {
            if(!ink){ink=[HGAActionSheetPlate new];[state.labels setObject:ink forKey:label];}
            ink.source=original;ink.sourceText=[text copy];
            CGFloat bw=0,ba=0;
            if(HGASheetNeutralColor(original,label.traitCollection,&bw,&ba) && (dark ? bw<0.68 : bw>0.40)) {
                CGFloat bLevel=dark ? (bw<0.24 ? 0.98 : 0.78) : (bw>0.80 ? 0.10 : 0.30);
                ink.applied=[UIColor colorWithWhite:bLevel alpha:ba];
            } else {
                ink.applied=original;
            }
            if(ink.applied && ![label.textColor isEqual:ink.applied])label.textColor=ink.applied;
            label.attributedText=rewritten;
            ink.appliedText=[label.attributedText copy];
            return;
        }
        if(hasAnyRunColor) {
            HGARestoreSheetLabel(state,label);return;
        }
    }
    CGFloat white=0,alpha=0;
    if(!HGASheetNeutralColor(original,label.traitCollection,&white,&alpha) ||
        (dark ? white>=0.68 : white<=0.40)) {HGARestoreSheetLabel(state,label);return;}
    // Preserve disabled/secondary emphasis and any semantic colored text.
    CGFloat level=dark ? (white<0.24 ? 0.98 : 0.78) : (white>0.80 ? 0.10 : 0.30);
    if(!ink){ink=[HGAActionSheetPlate new];[state.labels setObject:ink forKey:label];}
    ink.source=original;ink.sourceText=text;ink.applied=[UIColor colorWithWhite:level alpha:alpha];
    if(![label.textColor isEqual:ink.applied])label.textColor=ink.applied;
    ink.appliedText=label.attributedText;
}

static void HGAClearSheetBackground(HGAActionSheetState *state,UIView *view) {
    UIColor *source=view.backgroundColor;
    if(source && ![source isEqual:UIColor.clearColor]) {
        [state.covers setObject:source forKey:view];view.backgroundColor=UIColor.clearColor;
    }
}

static void HGARestoreSheetPlate(HGAActionSheetState *state, CALayer *layer) {
    HGAActionSheetPlate *plate=[state.plates objectForKey:layer];
    if(plate && layer.backgroundColor && CGColorEqualToColor(layer.backgroundColor,plate.applied.CGColor))
        layer.backgroundColor=plate.source.CGColor;
    [state.plates removeObjectForKey:layer];
}

static void HGAApplySheetPlate(HGAActionSheetState *state, UIView *view, BOOL selected) {
    if(!view)return;
    CALayer *layer=view.layer;
    if(!selected){HGARestoreSheetPlate(state,layer);return;}
    HGAActionSheetPlate *plate=[state.plates objectForKey:layer];
    CGColorRef current=layer.backgroundColor;
    BOOL owned=plate && current && CGColorEqualToColor(current,plate.applied.CGColor);
    if(!owned && (!current || CGColorGetAlpha(current)<0.01)) {
        [state.plates removeObjectForKey:layer];return;
    }
    if(!plate) {
        plate=[HGAActionSheetPlate new];plate.view=view;
        [state.plates setObject:plate forKey:layer];
    }
    if(!owned)plate.source=[UIColor colorWithCGColor:current];
    BOOL dark=view.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark;
    plate.applied=[UIColor colorWithWhite:dark ? 1 : 0 alpha:dark ? 0.22 : 0.13];
    if(!CGColorEqualToColor(current,plate.applied.CGColor))layer.backgroundColor=plate.applied.CGColor;
}

static void HGAClearSheetCover(HGAActionSheetState *state,UIView *view) {
    UIColor *source=view.backgroundColor;
    CGFloat r=0,g=0,b=0,a=0;
    if([[source resolvedColorWithTraitCollection:view.traitCollection] getRed:&r green:&g blue:&b alpha:&a] &&
        a>0.8 && MAX(r,MAX(g,b))-MIN(r,MIN(g,b))<0.08) {
        HGAClearSheetBackground(state,view);
    }
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
    if([ink.icon.tintColor isEqual:ink.appliedTint])ink.icon.tintColor=ink.tint;
    if([ink.plate.backgroundColor isEqual:HGASharePlate()])ink.plate.backgroundColor=ink.paint;
    objc_setAssociatedObject(cell,&HGAShareInkKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

static void HGAApplyShareGlyph(UIView *cell,UIView *view,UIView *plate,UIColor *foreground) {
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
    if(![icon.tintColor isEqual:ink.appliedTint])ink.tint=icon.tintColor;
    ink.appliedTint=foreground;
    if(![icon.tintColor isEqual:foreground])icon.tintColor=foreground;
    if(plate && ![plate.backgroundColor isEqual:HGASharePlate()]) {
        ink.paint=plate.backgroundColor;plate.backgroundColor=HGASharePlate();
    }
    if(icon.image!=ink.applied)icon.image=ink.applied;
    ink.applied=icon.image;
}

static void HGAApplyShareInk(HGAActionSheetState *state,UIView *cell) {
    UIView *icon=HGAViewGetter(cell,"iconView",cell) ?: HGAViewGetter(cell,"iconImageView",cell);
    UIView *plate=HGAViewGetter(cell,"iconBackgroundView",cell) ?: HGAViewGetter(cell,"imageBottomMaskView",cell);
    BOOL settingsRow=HGASheetUsesConvertedInk(state) && [cell isKindOfClass:objc_lookUpClass("FQVShortVideoSettingsCell")] &&
        [cell isDescendantOfView:state.settingsTable];
    BOOL sheetDark=state.surface.overrideUserInterfaceStyle==UIUserInterfaceStyleDark ||
        state.surface.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark;
    UIColor *foreground=settingsRow ? (sheetDark ? UIColor.whiteColor : UIColor.blackColor) : HGAShareForeground();
    HGAApplyShareGlyph(cell,icon,plate,foreground);
    if(settingsRow) {
        UIView *arrow=HGAViewGetter(cell,"nextImageView",cell);
        if([arrow isKindOfClass:UIImageView.class]) {
            [state.shareCells addObject:arrow];
            HGAApplyShareGlyph(arrow,arrow,nil,[foreground colorWithAlphaComponent:0.64]);
        }
    }
}

static void HGARestoreSheetRounding(HGAActionSheetState *state) {
    if(!state.rounding)return;
    UIView *surface=state.surface;
    if(fabs(surface.layer.cornerRadius-state.appliedRadius)<0.01)surface.layer.cornerRadius=state.sourceRadius;
    if([surface.layer.cornerCurve isEqual:kCACornerCurveContinuous])surface.layer.cornerCurve=state.sourceCurve;
    if(surface.layer.maskedCorners==state.appliedCorners)surface.layer.maskedCorners=state.sourceCorners;
    if(surface.clipsToBounds)surface.clipsToBounds=state.sourceClips;
    state.rounding=NO;
}

static void HGARestoreSheetStyle(HGAActionSheetState *state) {
    UIView *surface=state.surface;
    if(state.appliedStyle!=UIUserInterfaceStyleUnspecified && surface.overrideUserInterfaceStyle==state.appliedStyle)
        surface.overrideUserInterfaceStyle=state.originalStyle;
    state.appliedStyle=UIUserInterfaceStyleUnspecified;
}

static void HGARestoreSheetOptics(HGAActionSheetState *state) {
    UIView *surface=state.surface;
    if(state.unclipped)surface.clipsToBounds=YES;
    state.unclipped=NO;
    HGARestoreSheetDimmer(state);
}

static void HGAApplySheetAppearanceStyle(HGAActionSheetState *state,BOOL clear) {
    UIView *surface=state.surface;
    UIView *transition=surface.superview;
    Class type=objc_lookUpClass("FQVTransitionView");
    while(transition && ![transition isKindOfClass:type])transition=transition.superview;

    BOOL hostDark=NO;
    BOOL hasHostTheme=HGAHostExplicitDarkMode(surface,&hostDark);
    UIUserInterfaceStyle desired=UIUserInterfaceStyleUnspecified;
    if(hasHostTheme && hostDark) {
        desired=UIUserInterfaceStyleDark;
    } else if(clear) {
        if(transition) {
            CGFloat red=0,green=0,blue=0,alpha=0;
            UIColor *surround=[transition.backgroundColor resolvedColorWithTraitCollection:transition.traitCollection];
            BOOL light=[surround getRed:&red green:&green blue:&blue alpha:&alpha] &&
                alpha>0.82 && 0.2126*red+0.7152*green+0.0722*blue>0.50;
            desired=light ? UIUserInterfaceStyleLight : UIUserInterfaceStyleDark;
        } else if(hasHostTheme) {
            desired=UIUserInterfaceStyleLight;
        } else if(surface.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark ||
                  surface.window.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) {
            desired=UIUserInterfaceStyleDark;
        }
    } else if(!hasHostTheme) {
        UIUserInterfaceStyle baseline=state.appliedStyle!=UIUserInterfaceStyleUnspecified ?
            state.originalStyle : surface.overrideUserInterfaceStyle;
        if(baseline!=UIUserInterfaceStyleLight &&
           (baseline==UIUserInterfaceStyleDark || surface.window.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark))
            desired=UIUserInterfaceStyleDark;
    }

    if(desired==UIUserInterfaceStyleUnspecified) {
        HGARestoreSheetStyle(state);
        return;
    }
    if(surface.overrideUserInterfaceStyle!=desired) {
        if(state.appliedStyle==UIUserInterfaceStyleUnspecified || surface.overrideUserInterfaceStyle!=state.appliedStyle)
            state.originalStyle=surface.overrideUserInterfaceStyle;
        state.appliedStyle=desired;
        surface.overrideUserInterfaceStyle=desired;
        if(@available(iOS 17.0,*))[surface updateTraitsIfNeeded];
    }
}

static void HGAApplySheetOptics(HGAActionSheetState *state) {
    UIView *surface=state.surface;
    HGARestoreSheetRounding(state);
    // The glass owns its optical shape. A host mask can belong to a transition.
    if(!surface.layer.mask && surface.clipsToBounds){state.unclipped=YES;surface.clipsToBounds=NO;}
    UIView *transition=surface.superview;
    Class type=objc_lookUpClass("FQVTransitionView");
    while(transition && ![transition isKindOfClass:type])transition=transition.superview;
    if(!transition)return;

    // The verified transition getter identifies the visual scrim. Keep its
    // UIView and gestures in place so touches outside the card still dismiss.
    UIView *dimmer=HGAViewGetter(transition,"bgView",transition);
    if(state.dimmer!=dimmer)HGARestoreSheetDimmer(state);
    CGFloat white=0,alpha=0;
    if(dimmer.superview==transition && dimmer.subviews.count==0 &&
        CGRectContainsRect(dimmer.frame,[surface convertRect:surface.bounds toView:transition]) &&
        HGASheetNeutralColor(dimmer.backgroundColor,dimmer.traitCollection,&white,&alpha) &&
        white<0.20 && alpha*dimmer.alpha>0.02 && alpha*dimmer.alpha<0.95) {
        state.dimmer=dimmer;state.dimmerPaint=dimmer.backgroundColor;dimmer.backgroundColor=UIColor.clearColor;
    }
}

static void HGARestoreActionSheet(UIView *root) {
    HGAActionSheetState *state=objc_getAssociatedObject(root,&HGAActionSheetKey);
    if(!state)return;
    UIView *surface=state.surface;
    [state.dockMount removeFromSuperview];
    HGAAlignEpisodePreview(state,nil);
    HGARemoveGlass(surface);
    HGARestoreSheetOptics(state);
    HGARestoreSheetStyle(state);
    for(UILabel *label in state.labels.keyEnumerator.allObjects)HGARestoreSheetLabel(state,label);
    for(UIView *cover in state.covers.keyEnumerator.allObjects) {
        if(HGAState(cover,NO).adaptiveInk)HGAApplyAdaptiveControlInk(cover,UIUserInterfaceStyleLight,NO);
        if([cover.backgroundColor isEqual:UIColor.clearColor])cover.backgroundColor=[state.covers objectForKey:cover];
    }
    for(UIVisualEffectView *backing in state.effects.keyEnumerator)
        if(!backing.effect)backing.effect=[state.effects objectForKey:backing];
    for(UIView *cell in state.shareCells)HGARestoreShareInk(cell);
    for(CALayer *layer in state.plates.keyEnumerator.allObjects)HGARestoreSheetPlate(state,layer);
    HGARestoreSheetRounding(state);
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
    dispatch_once(&once, ^{classes=[@[@"SSShortVideoMorePanelShareHeaderView",@"SSVideoSharePanelView",@"SSVideoSharePanelControlView",
        @"FQVShortVideoSettingsOptionView",@"FQVShortVideoSettingsSwitchOptionView",
        @"FQVShortVideoSettingsSwitchOptionCell",@"FQVShortVideoSettingsBottomItemsView"]
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
// setupTabItems runs before the header takes its first countText snapshot.
// Seed only the current video's comment tab from the same opening parameter
// that the host passed for this panel. Later server counts still take priority.
static void HGAPrimeEpisodeCommentCount(id panel) {
    id param=HGReadObjectGetter(panel,"param");
    SEL countSel=sel_registerName("commentCount");
    Method countGetter=class_getInstanceMethod(object_getClass(param),countSel);
    if (!countGetter || strcmp(method_getTypeEncoding(countGetter),"q16@0:8")) return;
    NSInteger count=((NSInteger(*)(id,SEL))objc_msgSend)(param,countSel);
    if (count<=0) return;
    NSArray *items=HGReadObjectGetter(panel,"tabItems");
    if (![items isKindOfClass:NSArray.class]) return;
    SEL type=sel_registerName("type"),update=sel_registerName("updateCount:");
    for (id item in items) {
        Class cls=object_getClass(item);
        Method getType=class_getInstanceMethod(cls,type),setCount=class_getInstanceMethod(cls,update);
        if (!getType || strcmp(method_getTypeEncoding(getType),"Q16@0:8") ||
            !setCount || strcmp(method_getTypeEncoding(setCount),"v24@0:8q16") ||
            ((NSUInteger(*)(id,SEL))objc_msgSend)(item,type)!=0) continue;
        NSString *formatted=HGReadObjectGetter(item,"countText");
        if (!formatted || ([formatted isKindOfClass:NSString.class] && !formatted.length))
            ((void(*)(id,SEL,NSInteger))objc_msgSend)(item,update,count);
        return;
    }
}
static BOOL HGAIsCommentSurface(UIView *view) {
    return HGAPanelPart(view,HGACommentSurfaces());
}
static BOOL HGAIsCommentRow(UIView *view) {
    static Class cellClass,footerClass;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        cellClass=objc_lookUpClass("FQCShortVideoCommentCommonCell");
        footerClass=objc_lookUpClass("FQCShortVideoCommentSectionFooterView");
    });
    return [view isKindOfClass:cellClass] || [view isKindOfClass:footerClass];
}
static BOOL HGAPanelWantsGlass(NSString *kind) {
    return HGAppearanceEnabled() && HGAppearanceFlag([@"panels." stringByAppendingString:kind]);
}

static void HGAApplyCommentRow(HGAActionSheetState *state,UIView *row) {
    // Hosts paint the supplementary expand/collapse footer itself and reply
    // cells reuse a painted content view. Preserve their controls and separators.
    HGAClearSheetBackground(state,row);
    if([row isKindOfClass:UICollectionViewCell.class]) {
        UICollectionViewCell *cell=(id)row;
        HGAClearSheetBackground(state,cell.contentView);
        HGAClearSheetBackground(state,cell.backgroundView);
    }
}

static void HGAUpdateCommentRow(UIView *row) {
    if(!row.window || !HGAPanelWantsGlass(@"comment"))return;
    for(UIView *ancestor=row.superview;ancestor;ancestor=ancestor.superview) {
        HGAActionSheetState *state=objc_getAssociatedObject(ancestor,&HGAActionSheetKey);
        if(!state || ![row isDescendantOfView:state.surface])continue;
        HGAApplyCommentRow(state,row);
        if(HGASheetUsesConvertedInk(state)) {
            for(UIView *child in HGADescendants(row,48))
                if([child isKindOfClass:UILabel.class] && HGAVisible(child))HGAApplySheetLabel(state,(UILabel *)child);
        }
        if(state.surface.overrideUserInterfaceStyle==UIUserInterfaceStyleDark)
            HGAApplyAdaptiveControlInk(row,UIUserInterfaceStyleDark,YES);
        return;
    }
}
static void HGAUpdateCommentSurface(UIView *surfaceView) {
    if(!surfaceView.window || !HGAPanelWantsGlass(@"comment"))return;
    for(UIView *ancestor=surfaceView.superview;ancestor;ancestor=ancestor.superview) {
        HGAActionSheetState *state=objc_getAssociatedObject(ancestor,&HGAActionSheetKey);
        if(!state || ![surfaceView isDescendantOfView:state.surface])continue;
        if(state.nativeClear)HGAClearSheetBackground(state,surfaceView);
        else HGAClearSheetCover(state,surfaceView);
        if(HGASheetUsesConvertedInk(state)) {
            for(UIView *child in HGADescendants(surfaceView,36))
                if([child isKindOfClass:UILabel.class] && HGAVisible(child))HGAApplySheetLabel(state,(UILabel *)child);
        }
        return;
    }
}
static BOOL HGAApplySettingsPanelPart(HGAActionSheetState *state, UIView *view) {
    if([view isKindOfClass:objc_lookUpClass("FQVShortVideoSettingsOptionView")]) {
        BOOL selected=NO;
        if(HGABoolGetter(view,@"isSelected",&selected))HGAApplySheetPlate(state,view,selected);
        return YES;
    }
    if([view isKindOfClass:objc_lookUpClass("FQVShortVideoSettingsSwitchOptionView")]) {
        BOOL selected=NO;
        if(HGABoolGetter(view,@"optionSelected",&selected) && selected) {
            UIView *plate=HGAViewGetter(view,"selectionBackgroundView",view);
            if(!plate.hidden)HGAApplySheetPlate(state,plate,YES);
        }
        return YES;
    }
    if([view isKindOfClass:objc_lookUpClass("FQVShortVideoSettingsCell")]) {
        UITableViewCell *cell=(id)view;
        if(HGASheetUsesConvertedInk(state) && [cell isDescendantOfView:state.settingsTable]) {
            // The host repaints the entire row from its settings style model,
            // including translucent backgrounds below the old opacity cutoff.
            HGAClearSheetBackground(state,cell);
            HGAClearSheetBackground(state,cell.contentView);
            HGAClearSheetBackground(state,cell.backgroundView);
        } else {
            HGAClearSheetCover(state,cell);
            HGAClearSheetCover(state,cell.contentView);
            HGAClearSheetCover(state,cell.backgroundView);
        }
        if([view isKindOfClass:objc_lookUpClass("FQVShortVideoSettingsSwitchOptionCell")]) {
            // Its getter lazily creates the native drag view. Paint only the
            // slider the host has already mounted.
            Ivar field=class_getInstanceVariable(view.class,"_dragView");
            const char *type=field ? ivar_getTypeEncoding(field) : NULL;
            UIView *slider=type && !strcmp(type,"@\"UIView\"") ? object_getIvar(view,field) : nil;
            if([slider isKindOfClass:UIView.class] && [slider isDescendantOfView:view] && !slider.hidden)
                HGAApplySheetPlate(state,slider,YES);
        }
        return YES;
    }
    if([view isKindOfClass:objc_lookUpClass("FQVShortVideoSettingsBottomItemsView")]) {
        HGAClearSheetCover(state,view);
        HGAClearSheetCover(state,HGAViewGetter(view,"containerView",view));
        return YES;
    }
    return NO;
}

static void HGAApplyActionSheet(UIView *root,NSString *kind) {
    BOOL enabled=HGAPanelWantsGlass(kind) && root.window;
    UIView *surface=enabled?HGAActionSheetSurface(root):nil;
    BOOL comment=[kind isEqual:@"comment"];
    BOOL episodes=[kind isEqual:@"episodes"];
    BOOL longPress=[kind isEqual:@"longpress"];
    BOOL share=[kind isEqual:@"share"] || longPress;
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
        state.plates=[NSMapTable weakToStrongObjectsMapTable];
        state.labels=[NSMapTable weakToStrongObjectsMapTable];
        objc_setAssociatedObject(root,&HGAActionSheetKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    CGFloat radius=MIN(28,MIN(surface.bounds.size.width,surface.bounds.size.height)/2);
    BOOL reduceTransparency=UIAccessibilityIsReduceTransparencyEnabled();
    CGFloat opacity=reduceTransparency?1:HGAppearanceNumber(@"panels.opacity");
    NSString *material=HGAppearanceString(@"panels.material");
    NSString *commentMaterial=comment ? HGAppearanceString(@"panels.commentMaterial") : nil;
    BOOL frosted=[commentMaterial isEqual:@"frosted"];
    if (comment && ![commentMaterial isEqual:@"inherit"])
        material=frosted ? @"regular" : commentMaterial;
    BOOL clear=(share || comment || episodes) && !reduceTransparency && HGNativeGlassConfigured() &&
        ([material isEqual:@"regular"] || [material isEqual:@"clear"]);
    if(state.nativeClear && !clear) {
        HGARestoreSheetOptics(state);
    }
    state.nativeClear=clear;
    HGAApplySheetAppearanceStyle(state,clear);
    BOOL convertsInk=HGASheetUsesConvertedInk(state);
    if(!convertsInk) {
        for(UILabel *label in state.labels.keyEnumerator.allObjects)HGARestoreSheetLabel(state,label);
        for(UIView *entry in state.shareCells.allObjects) {
            if(![entry isKindOfClass:UIImageView.class])continue;
            HGARestoreShareInk(entry);[state.shareCells removeObject:entry];
        }
    }
    NSString *renderStyle=clear && !frosted ? @"clear" : material;
    HGAInstallGlass(surface,renderStyle,clear?0:radius,opacity);
    HGAAppearanceMaterial *glass=HGAState(surface,NO).glass;
    if(!state.overlay || state.overlayOpacity!=opacity) {
        state.overlayOpacity=opacity;
        state.overlay=(comment || episodes) ? [UIColor colorWithDynamicProvider:^UIColor *(UITraitCollection *traits) {
            return [UIColor colorWithWhite:(traits.userInterfaceStyle==UIUserInterfaceStyleDark ? 0.08 : 1.0) alpha:opacity];
        }] : HGAActionSheetColor(opacity);
    }
    glass.overlayColor=clear?UIColor.clearColor:state.overlay;
    glass.clipsToBounds=!clear;
    [glass requestMaterialRefresh];
    if(clear) {
        glass.opaque=NO;
        HGAApplySheetOptics(state);
        if(@available(iOS 26.0,*)) {
            if(fabs([glass effectiveRadiusForCorner:UIRectCornerTopLeft]-radius)>0.01)
                glass.cornerConfiguration=[UICornerConfiguration configurationWithUniformRadius:[UICornerRadius fixedRadius:radius]];
        }
    } else if(!surface.layer.mask) {
        if(!state.rounding || fabs(surface.layer.cornerRadius-state.appliedRadius)>=0.01)state.sourceRadius=surface.layer.cornerRadius;
        if(!state.rounding || ![surface.layer.cornerCurve isEqual:kCACornerCurveContinuous])state.sourceCurve=surface.layer.cornerCurve;
        CACornerMask corners=kCALayerMinXMinYCorner|kCALayerMaxXMinYCorner;
        if(share)corners|=kCALayerMinXMaxYCorner|kCALayerMaxXMaxYCorner;
        if(!state.rounding || surface.layer.maskedCorners!=state.appliedCorners)state.sourceCorners=surface.layer.maskedCorners;
        if(!state.rounding || !surface.clipsToBounds)state.sourceClips=surface.clipsToBounds;
        state.rounding=YES;state.appliedRadius=radius;state.appliedCorners=corners;
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
    if(longPress) {
        UIViewController *owner=HGAOwner(root);
        UIView *tableView=HGAViewGetter(owner,"tableView",surface);
        state.settingsTable=[tableView isKindOfClass:UITableView.class] ? (UITableView *)tableView : nil;
        if(convertsInk && state.settingsTable) {
            UITableView *table=state.settingsTable;
            HGAClearSheetBackground(state,table);
            HGAClearSheetBackground(state,table.backgroundView);
            // This inner UIKit wrapper is bounded to the observed settings
            // section. It is not a generally hooked UIKit class.
            for(UIView *wrapper in table.subviews)
                if([NSStringFromClass(wrapper.class) isEqual:@"UITableViewWrapperView"])
                    HGAClearSheetBackground(state,wrapper);
        }
        BOOL usesBlur=NO;
        if(HGABoolGetter(owner,@"useBlurBackground",&usesBlur) && usesBlur) {
            UIView *backdrop=HGAViewGetter(owner,"blurView",surface);
            if([backdrop isKindOfClass:UIVisualEffectView.class]) {
                UIVisualEffectView *effectView=(id)backdrop;
                if(effectView.effect){[state.effects setObject:effectView.effect forKey:effectView];effectView.effect=nil;}
                HGAClearSheetBackground(state,HGAViewGetter(owner,"effectViewContentView",surface));
            }
        }
    }
    if(share || comment || episodes) {
        if(!state.dockMount) {
            state.dockMount=[HGAActionSheetMount new];
            state.dockMount.sheetRoot=root;
            state.dockMount.accessibilityIdentifier=@"honggo.appearance.sheet.mount";
            state.dockMount.accessibilityElementsHidden=YES;state.dockMount.userInteractionEnabled=NO;
            [surface addSubview:state.dockMount];
        } else HGARefreshSheetDockVisibility(surface.window);
    }
    if(episodes) {
        UIView *transition=surface.superview;
        while(transition && ![transition isKindOfClass:objc_lookUpClass("FQVTransitionView")])
            transition=transition.superview;
        UIView *top=HGAViewGetter(transition,"topContentView",transition);
        HGAAlignEpisodePreview(state,[top isKindOfClass:objc_lookUpClass("FQVTransitionPlayerTopContentView")] ? top : nil);
    }
    for(CALayer *layer in state.plates.keyEnumerator.allObjects) {
        if(![[state.plates objectForKey:layer].view isDescendantOfView:surface])HGARestoreSheetPlate(state,layer);
    }
    BOOL compose=NO;
    NSArray<UIView *> *views=HGADescendants(surface,240);
    for(UIView *view in views) {
        if(view==surface || HGAOwnView(view) || view.hidden)continue;
        if(share && [NSStringFromClass(view.class) hasPrefix:@"RKMPandroidx.compose.ui.window.ComposeView"])compose=YES;
        if(share && HGAIsShareCell(view)) {
            [state.shareCells addObject:view];HGAApplyShareInk(state,view);
        }
        if(share)HGAApplySettingsPanelPart(state,view);
        if(comment && HGAIsCommentRow(view))HGAApplyCommentRow(state,view);
        if(CGRectGetHeight(view.bounds)<16 || CGRectGetHeight(view.bounds)>CGRectGetHeight(surface.bounds)+2 ||
            CGRectGetWidth(view.bounds)<CGRectGetWidth(surface.bounds)*0.8)continue;
        if(comment && view.class==UIVisualEffectView.class &&
            [view.superview isKindOfClass:NSClassFromString(@"FQCShortVideoBottomToolBar")]) {
            UIVisualEffectView *backing=(id)view;
            if(backing.effect){[state.effects setObject:backing.effect forKey:backing];backing.effect=nil;}
            HGAClearSheetBackground(state,backing.contentView);
            // Preserve editor controls hosted by the effect's content view.
        }
        BOOL episodeBacking=episodes && ([view isKindOfClass:UIScrollView.class] ||
            [view isKindOfClass:objc_lookUpClass("FQRJXCategoryTitleView")] ||
            [view isKindOfClass:objc_lookUpClass("FQRJXCategoryListContainerView")] ||
            [view isKindOfClass:objc_lookUpClass("SSShortVideoEpisodesViewHeaderView")] ||
            [view isKindOfClass:objc_lookUpClass("SSShortVideoEpisodePanelCollectView")]);
        BOOL backing=episodeBacking || view.class==UIView.class || view.class==UITableView.class ||
            view.class==UICollectionView.class || HGAIsActionPanelSurface(view) ||
            (share && view.class==UITableViewCell.class) ||
            (comment && (HGAIsCommentSurface(view) ||
                [view isKindOfClass:NSClassFromString(@"FQReadingNavigationBar")] ||
                [view isKindOfClass:NSClassFromString(@"FQCShortVideoCommentCell")]));
        if(backing) {
            if(comment && convertsInk && HGAIsCommentSurface(view))
                HGAClearSheetBackground(state,view);
            else HGAClearSheetCover(state,view);
        }
    }
    if(convertsInk) {
        for(UILabel *label in state.labels.keyEnumerator.allObjects)
            if(![label isDescendantOfView:surface])HGARestoreSheetLabel(state,label);
        for(UIView *view in views) {
            // The episode heading uses YYLabel, with the same textColor and
            // attributedText getter/setter ABIs as UILabel in this host.
            BOOL episodeTitle=episodes && [view isKindOfClass:objc_lookUpClass("YYLabel")] &&
                [view.superview isKindOfClass:objc_lookUpClass("SSShortVideoEpisodesViewHeaderView")] &&
                HGAViewGetter(view.superview,"titleLabel",view.superview)==view;
            if(([view isKindOfClass:UILabel.class] || episodeTitle) && HGAVisible(view))
                HGAApplySheetLabel(state,(UILabel *)view);
        }
    }
    HGRecordCapability([@"appearance.panel.surface." stringByAppendingString:kind],
        @{@"module":@"appearance",@"state":compose ? @"partial" : @"installed",
          @"material":reduceTransparency ? @"solid" : renderStyle,
          @"reason":compose ? @"bounded glass behind the host Compose canvas; in-canvas paint and icons are controlled by the host renderer" :
            @"single bounded glass sheet behind native content and actions"});
}
static void HGAUpdateActionPanelSurface(UIView *view) {
    UIViewController *owner=HGAOwner(view);
    NSString *kind=HGAPanelKind(owner);
    HGAActionSheetState *state=objc_getAssociatedObject(owner.viewIfLoaded,&HGAActionSheetKey);
    BOOL inActionSheet=([kind isEqual:@"share"] || [kind isEqual:@"longpress"]) &&
        HGAPanelWantsGlass(kind) && view.window && [view isDescendantOfView:state.surface];
    BOOL settingsPart=inActionSheet && HGAApplySettingsPanelPart(state,view);
    if(inActionSheet && HGASheetUsesConvertedInk(state) && (settingsPart || HGAIsShareCell(view)))
        for(UIView *child in HGADescendants(view,32))
            if([child isKindOfClass:UILabel.class] && HGAVisible(child))HGAApplySheetLabel(state,(UILabel *)child);
    if(HGAIsShareCell(view)) {
        if(inActionSheet) {
            [state.shareCells addObject:view];HGAApplyShareInk(state,view);
        }else HGARestoreShareInk(view);
        return;
    }
    if(settingsPart)return;
    if([@[@"share",@"longpress",@"speed"] containsObject:kind] && owner.isViewLoaded) {
        if(!HGAControllers)HGAControllers=[NSHashTable weakObjectsHashTable];
        [HGAControllers addObject:owner];
        HGAApplyActionSheet(owner.view,kind);
    }
}
