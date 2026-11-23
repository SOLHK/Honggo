// FanqieX-unified comment sheet materials, appearance polarity, backdrop blur,
// composer bar styling, dark dimmer zero-masking, no-video-shift, and banner/search cleanup.

static char HGACommentGlassKey;
static char HGACommentSavedBgKey;
static char HGACommentSavedLayerBgKey;
static char HGACommentSavedClipsKey;
static char HGACommentSavedCornerRadiusKey;
static char HGACommentSavedMaskedCornersKey;
static char HGACommentDimmerMaskKey;
static char HGACommentDimmerSavedMaskKey;
static char HGACommentComposerGlassKey;
static char HGACommentTypographyStampKey;
static char HGACommentNoVideoShiftKey;
static char HGACommentBannerSavedHeightKey;
static char HGACommentSavedEffectKey;
static char HGACommentSavedHiddenKey;

static NSHashTable<UIViewController *> *HGACommentTrackedVCs(void) {
    static NSHashTable<UIViewController *> *table;
    static dispatch_once_t once;
    dispatch_once(&once,^{ table=[NSHashTable weakObjectsHashTable]; });
    return table;
}
static NSHashTable<UIView *> *HGACommentTrackedSheets(void) {
    static NSHashTable<UIView *> *table;
    static dispatch_once_t once;
    dispatch_once(&once,^{ table=[NSHashTable weakObjectsHashTable]; });
    return table;
}
static NSHashTable<UIView *> *HGACommentTrackedBars(void) {
    static NSHashTable<UIView *> *table;
    static dispatch_once_t once;
    dispatch_once(&once,^{ table=[NSHashTable weakObjectsHashTable]; });
    return table;
}
static NSHashTable<UIView *> *HGACommentTrackedDimmers(void) {
    static NSHashTable<UIView *> *table;
    static dispatch_once_t once;
    dispatch_once(&once,^{ table=[NSHashTable weakObjectsHashTable]; });
    return table;
}
static NSHashTable<UIView *> *HGACommentTrackedBanners(void) {
    static NSHashTable<UIView *> *table;
    static dispatch_once_t once;
    dispatch_once(&once,^{ table=[NSHashTable weakObjectsHashTable]; });
    return table;
}
static NSHashTable<UIView *> *HGACommentTrackedSearchEntrances(void) {
    static NSHashTable<UIView *> *table;
    static dispatch_once_t once;
    dispatch_once(&once,^{ table=[NSHashTable weakObjectsHashTable]; });
    return table;
}

static NSString *HGACommentEffectiveMaterial(void) {
    if(!HGAppearanceEnabled() || !HGAppearanceFlag(@"panels.comment"))return nil;
    NSString *mat=HGAppearanceString(@"comment.material");
    if(!mat.length || [mat isEqual:@"native"]) {
        return HGAppearanceString(@"panels.material") ?: @"regular";
    }
    return mat;
}

static BOOL HGACommentStylingEnabled(void) {
    return HGACommentEffectiveMaterial()!=nil;
}

static BOOL HGACommentAppIsDark(UIView *referenceView) {
    Class adaptCls=NSClassFromString(@"SSUIColorAdaptedManager");
    if(adaptCls) {
        SEL sharedSel=sel_registerName("sharedManager");
        Method sm=class_getClassMethod(adaptCls,sharedSel);
        if(sm && !strcmp(method_getTypeEncoding(sm),"@16@0:8")) {
            id mgr=((id (*)(id,SEL))objc_msgSend)(adaptCls,sharedSel);
            BOOL dark=NO;
            if(HGABoolGetter(mgr,@"isDarkMode",&dark) || HGABoolGetter(mgr,@"darkMode",&dark)) {
                return dark;
            }
        }
    }
    if(referenceView) {
        return referenceView.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark;
    }
    return UITraitCollection.currentTraitCollection.userInterfaceStyle==UIUserInterfaceStyleDark;
}

static BOOL HGACommentUseDarkTheme(UIView *referenceView) {
    NSString *appearance=HGAppearanceString(@"comment.appearance");
    if([appearance isEqual:@"light"])return NO;
    if([appearance isEqual:@"dark"])return YES;
    return HGACommentAppIsDark(referenceView);
}

static BOOL HGACommentAppearanceOverridden(void) {
    NSString *appearance=HGAppearanceString(@"comment.appearance");
    return [appearance isEqual:@"light"] || [appearance isEqual:@"dark"];
}

static UIUserInterfaceStyle HGACommentDesiredUIStyle(UIView *referenceView) {
    if(!HGACommentStylingEnabled())return UIUserInterfaceStyleUnspecified;
    return HGACommentUseDarkTheme(referenceView) ? UIUserInterfaceStyleDark : UIUserInterfaceStyleLight;
}

static BOOL HGACommentColorGetRGBA(UIColor *color,CGFloat *r,CGFloat *g,CGFloat *b,CGFloat *a) {
    if(!color)return NO;
    if([color getRed:r green:g blue:b alpha:a])return YES;
    CGFloat w=0;
    if([color getWhite:&w alpha:a]){*r=w;*g=w;*b=w;return YES;}
    return NO;
}

static void HGACommentSetClearSurface(UIView *view,BOOL clear) {
    if(![view isKindOfClass:UIView.class] || HGAOwnView(view))return;
    id savedBg=objc_getAssociatedObject(view,&HGACommentSavedBgKey);
    id savedLayerBg=objc_getAssociatedObject(view,&HGACommentSavedLayerBgKey);
    if(clear) {
        if(!savedBg && view.backgroundColor && ![view.backgroundColor isEqual:UIColor.clearColor]) {
            objc_setAssociatedObject(view,&HGACommentSavedBgKey,view.backgroundColor,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        if(![view.backgroundColor isEqual:UIColor.clearColor])view.backgroundColor=UIColor.clearColor;
        if(view.opaque)view.opaque=NO;
        CGColorRef cg=view.layer.backgroundColor;
        if(cg && CGColorGetAlpha(cg)>0.01) {
            if(!savedLayerBg) {
                objc_setAssociatedObject(view,&HGACommentSavedLayerBgKey,[UIColor colorWithCGColor:cg],OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            }
            view.layer.backgroundColor=NULL;
        }
    } else {
        if([savedBg isKindOfClass:UIColor.class]) {
            if([view.backgroundColor isEqual:UIColor.clearColor])view.backgroundColor=(UIColor *)savedBg;
            objc_setAssociatedObject(view,&HGACommentSavedBgKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        if([savedLayerBg isKindOfClass:UIColor.class]) {
            if(!view.layer.backgroundColor || CGColorGetAlpha(view.layer.backgroundColor)<0.01) {
                view.layer.backgroundColor=((UIColor *)savedLayerBg).CGColor;
            }
            objc_setAssociatedObject(view,&HGACommentSavedLayerBgKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
    }
}

static void HGACommentSetHiddenTracked(UIView *view,BOOL hide) {
    if(![view isKindOfClass:UIView.class])return;
    NSNumber *saved=objc_getAssociatedObject(view,&HGACommentSavedHiddenKey);
    if(hide) {
        if(!saved)objc_setAssociatedObject(view,&HGACommentSavedHiddenKey,@(view.hidden),OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        if(!view.hidden)view.hidden=YES;
    } else if(saved) {
        view.hidden=saved.boolValue;
        objc_setAssociatedObject(view,&HGACommentSavedHiddenKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
}

static void HGACommentNeutralizeHostEffect(UIVisualEffectView *ev,BOOL active) {
    if(![ev isKindOfClass:UIVisualEffectView.class] || HGAOwnView(ev) ||
       [ev isKindOfClass:HGAAppearanceMaterial.class] || [ev isKindOfClass:HGBackdropBlurView.class])return;
    UIVisualEffect *saved=objc_getAssociatedObject(ev,&HGACommentSavedEffectKey);
    if(active) {
        if(!saved && ev.effect)objc_setAssociatedObject(ev,&HGACommentSavedEffectKey,ev.effect,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        if(ev.effect)ev.effect=nil;
    } else if(saved) {
        if(!ev.effect)ev.effect=saved;
        objc_setAssociatedObject(ev,&HGACommentSavedEffectKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    HGACommentSetClearSurface(ev,active);
    HGACommentSetClearSurface(ev.contentView,active);
    HGACommentSetHiddenTracked(ev,active);
}

static void HGACommentSetDimmerZeroMask(UIView *view,BOOL masked) {
    if(![view isKindOfClass:UIView.class])return;
    CALayer *ownedMask=objc_getAssociatedObject(view,&HGACommentDimmerMaskKey);
    if(masked) {
        [HGACommentTrackedDimmers() addObject:view];
        if(!ownedMask) {
            if(view.layer.mask) {
                objc_setAssociatedObject(view,&HGACommentDimmerSavedMaskKey,view.layer.mask,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            }
            ownedMask=[CALayer layer];
            ownedMask.frame=CGRectZero;
            objc_setAssociatedObject(view,&HGACommentDimmerMaskKey,ownedMask,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        if(view.layer.mask!=ownedMask)view.layer.mask=ownedMask;
    } else if(ownedMask) {
        if(view.layer.mask==ownedMask) {
            CALayer *saved=objc_getAssociatedObject(view,&HGACommentDimmerSavedMaskKey);
            view.layer.mask=saved;
        }
        objc_setAssociatedObject(view,&HGACommentDimmerMaskKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(view,&HGACommentDimmerSavedMaskKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
}

static BOOL HGACommentColorIsDarkScrim(UIColor *color,UITraitCollection *traits,CGFloat viewAlpha) {
    if(!color)return NO;
    UIColor *resolved=traits ? [color resolvedColorWithTraitCollection:traits] : color;
    CGFloat r=0,g=0,b=0,a=0;
    if(!HGACommentColorGetRGBA(resolved,&r,&g,&b,&a))return NO;
    CGFloat eff=a*MAX(0.0,MIN(1.0,viewAlpha));
    CGFloat lum=0.299*r+0.587*g+0.114*b;
    return eff>=0.04 && eff<=0.92 && lum<=0.28;
}

static BOOL HGACommentSubtreeHasContent(UIView *view,NSInteger depth) {
    if(!view || depth>4 || view.hidden || view.alpha<0.01)return NO;
    if([view isKindOfClass:UIScrollView.class] || [view isKindOfClass:UITextView.class] ||
       [view isKindOfClass:UICollectionView.class] || [view isKindOfClass:UITableView.class])return YES;
    if([view isKindOfClass:UILabel.class]) {
        UILabel *l=(UILabel *)view;
        if(l.text.length>0 || l.attributedText.length>0)return YES;
    } else if([view isKindOfClass:UIImageView.class]) {
        if(((UIImageView *)view).image!=nil)return YES;
    } else if([view isKindOfClass:UIButton.class]) {
        UIButton *b=(UIButton *)view;
        if(b.currentTitle.length>0 || b.currentImage!=nil)return YES;
    }
    for(UIView *sub in view.subviews)if(HGACommentSubtreeHasContent(sub,depth+1))return YES;
    return NO;
}

static BOOL HGACommentViewIsDimmerScrim(UIView *view,CGSize winSize) {
    if(![view isKindOfClass:UIView.class] || HGAOwnView(view))return NO;
    if([view.nextResponder isKindOfClass:UIViewController.class])return NO;
    if(objc_getAssociatedObject(view,&HGACommentDimmerMaskKey)!=nil)return YES;
    if(winSize.width<100.0 || winSize.height<100.0)return NO;
    if(CGRectGetWidth(view.bounds)<winSize.width*0.72 || CGRectGetHeight(view.bounds)<winSize.height*0.40)return NO;
    if(HGACommentSubtreeHasContent(view,0))return NO;
    if([view isKindOfClass:UIVisualEffectView.class])return YES;
    if(HGACommentColorIsDarkScrim(view.backgroundColor,view.traitCollection,view.alpha))return YES;
    if(view.layer.backgroundColor) {
        UIColor *lc=[UIColor colorWithCGColor:view.layer.backgroundColor];
        if(HGACommentColorIsDarkScrim(lc,view.traitCollection,view.alpha))return YES;
    }
    NSString *cls=NSStringFromClass(view.class);
    for(NSString *tok in @[@"Dim",@"Mask",@"Overlay",@"Backdrop",@"Scrim",@"Shadow",@"Cover"]) {
        if([cls rangeOfString:tok options:NSCaseInsensitiveSearch].location!=NSNotFound)return YES;
    }
    return NO;
}

static void HGACommentCleanPresentationBackdrops(UIView *sheet,UIView *transition,BOOL active) {
    if(![sheet isKindOfClass:UIView.class])return;
    UIWindow *win=sheet.window;
    CGSize winSize=win ? win.bounds.size : sheet.bounds.size;
    if(transition) {
        HGACommentSetClearSurface(transition,active);
        const char *keys[]={"bgView","backgroundView","dimView","dimmingView","coverView",
            "overlayView","backdropView","maskBgView","maskView","blackMaskView","bgMaskView","tapView"};
        for(size_t i=0;i<sizeof(keys)/sizeof(keys[0]);i++) {
            UIView *v=HGAViewGetter(transition,keys[i],transition);
            if(v && v!=sheet && ![sheet isDescendantOfView:v] && ![v.nextResponder isKindOfClass:UIViewController.class]) {
                HGACommentSetClearSurface(v,active);
                HGACommentSetDimmerZeroMask(v,active);
            }
        }
        for(UIView *sub in transition.subviews) {
            if(sub==sheet || [sheet isDescendantOfView:sub] || [sub isDescendantOfView:sheet])continue;
            if([sub isKindOfClass:UIVisualEffectView.class]) {
                HGACommentNeutralizeHostEffect((UIVisualEffectView *)sub,active);
            } else if(HGACommentViewIsDimmerScrim(sub,winSize)) {
                HGACommentSetClearSurface(sub,active);
                HGACommentSetDimmerZeroMask(sub,active);
            }
        }
    }
    UIView *branch=sheet;
    UIView *anc=sheet.superview;
    for(NSInteger lvl=0;anc && lvl<6;lvl++) {
        NSArray<UIView *> *sibs=anc.subviews;
        NSUInteger idx=[sibs indexOfObjectIdenticalTo:branch];
        if(idx!=NSNotFound) {
            for(NSUInteger i=0;i<idx;i++) {
                UIView *sib=sibs[i];
                if(sib==sheet || [sheet isDescendantOfView:sib] || [sib isDescendantOfView:sheet])continue;
                if(HGACommentViewIsDimmerScrim(sib,winSize)) {
                    if([sib isKindOfClass:UIVisualEffectView.class])HGACommentNeutralizeHostEffect((UIVisualEffectView *)sib,active);
                    else {HGACommentSetClearSurface(sib,active);HGACommentSetDimmerZeroMask(sib,active);}
                }
            }
        }
        if(anc==win)break;
        branch=anc;anc=anc.superview;
    }
}

static UIColor *HGACommentBoostColor(UIColor *color,UITraitCollection *traits,BOOL dark,BOOL *changed) {
    if(changed)*changed=NO;
    if(!color)return nil;
    UIColor *resolved=traits ? [color resolvedColorWithTraitCollection:traits] : color;
    CGFloat r=0,g=0,b=0,a=0;
    if(!HGACommentColorGetRGBA(resolved,&r,&g,&b,&a) || a<0.05)return color;
    CGFloat lum=0.299*r+0.587*g+0.114*b;
    CGFloat sat=MAX(MAX(r,g),b)-MIN(MIN(r,g),b);
    if(dark) {
        if(sat<0.16) {
            if(lum<0.38){if(changed)*changed=YES;return UIColor.whiteColor;}
            if(lum*a>=0.82){if(lum<0.99 || a<0.99){if(changed)*changed=YES;return UIColor.whiteColor;}return color;}
            if(lum<0.98 || a<0.86){if(changed)*changed=YES;return [UIColor colorWithWhite:1.0 alpha:0.88];}
        } else if(b>r+0.12 && b>g+0.03) {
            UIColor *blue=[UIColor colorWithRed:0.24 green:0.64 blue:1.0 alpha:1.0];
            if(changed)*changed=YES;
            return blue;
        }
    } else {
        if(sat<0.16) {
            if(lum>0.65){if(changed)*changed=YES;return [UIColor colorWithWhite:0.08 alpha:0.96];}
            if(lum>0.38 || a<0.80){if(changed)*changed=YES;return [UIColor colorWithWhite:0.12 alpha:0.86];}
        }
    }
    return color;
}

static void HGACommentBoostTypography(UIView *view,BOOL dark,NSInteger depth) {
    if(!HGACommentStylingEnabled() || ![view isKindOfClass:UIView.class] || depth>12 || HGAOwnView(view))return;
    NSString *mat=HGACommentEffectiveMaterial();
    if([mat isEqual:@"regular"] && !HGACommentAppearanceOverridden())return;
    if([view isKindOfClass:UIVisualEffectView.class])return;
    UITraitCollection *traits=view.traitCollection;
    NSUInteger mode=(dark?1u:0u) | ([mat isEqual:@"clear"]?2u:0u);
    if([view isKindOfClass:UILabel.class]) {
        UILabel *label=(UILabel *)view;
        BOOL ch=NO;
        UIColor *boosted=HGACommentBoostColor(label.textColor,traits,dark,&ch);
        if(ch && boosted)label.textColor=boosted;
        if(label.attributedText.length>0) {
            NSUInteger stamp=(mode<<28) ^ (label.attributedText.string.hash & 0x0FFFFFFF);
            NSNumber *prev=objc_getAssociatedObject(label,&HGACommentTypographyStampKey);
            if(!prev || prev.unsignedIntegerValue!=stamp) {
                NSMutableAttributedString *mut=[label.attributedText mutableCopy];
                __block BOOL modified=NO;
                [label.attributedText enumerateAttribute:NSForegroundColorAttributeName inRange:NSMakeRange(0,mut.length) options:0
                    usingBlock:^(id val,NSRange range,BOOL *stop) {
                        (void)stop;
                        if(![val isKindOfClass:UIColor.class])return;
                        BOOL c=NO;
                        UIColor *bc=HGACommentBoostColor((UIColor *)val,traits,dark,&c);
                        if(c && bc) {
                            [mut addAttribute:NSForegroundColorAttributeName value:bc range:range];
                            [mut addAttribute:(__bridge NSAttributedStringKey)kCTForegroundColorAttributeName value:(__bridge id)bc.CGColor range:range];
                            modified=YES;
                        }
                    }];
                if(modified)label.attributedText=mut;
                objc_setAssociatedObject(label,&HGACommentTypographyStampKey,@(stamp),OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            }
        }
        return;
    }
    if([view isKindOfClass:UIButton.class]) {
        UIButton *btn=(UIButton *)view;
        BOOL ch=NO;
        UIColor *bc=HGACommentBoostColor([btn titleColorForState:UIControlStateNormal],traits,dark,&ch);
        if(ch && bc)[btn setTitleColor:bc forState:UIControlStateNormal];
        return;
    }
    if([view isKindOfClass:UITextView.class]) {
        UITextView *tv=(UITextView *)view;
        UIColor *ink=dark ? UIColor.whiteColor : [UIColor colorWithWhite:0.08 alpha:0.96];
        tv.overrideUserInterfaceStyle=dark ? UIUserInterfaceStyleDark : UIUserInterfaceStyleLight;
        tv.textColor=ink;
        NSMutableDictionary *typing=tv.typingAttributes ? tv.typingAttributes.mutableCopy : [NSMutableDictionary dictionary];
        typing[NSForegroundColorAttributeName]=ink;
        tv.typingAttributes=typing;
        return;
    }
    if([view isKindOfClass:UITextField.class]) {
        UITextField *tf=(UITextField *)view;
        tf.overrideUserInterfaceStyle=dark ? UIUserInterfaceStyleDark : UIUserInterfaceStyleLight;
        tf.textColor=dark ? UIColor.whiteColor : [UIColor colorWithWhite:0.08 alpha:0.96];
        return;
    }
    // Custom rich-text YYLabel textLayout boost
    SEL getLayout=sel_registerName("textLayout");
    SEL setLayout=sel_registerName("setTextLayout:");
    if([view respondsToSelector:getLayout] && [view respondsToSelector:setLayout]) {
        id layout=((id (*)(id,SEL))objc_msgSend)(view,getLayout);
        SEL textSel=sel_registerName("text");
        SEL contSel=sel_registerName("container");
        if(layout && [layout respondsToSelector:textSel] && [layout respondsToSelector:contSel]) {
            id lText=((id (*)(id,SEL))objc_msgSend)(layout,textSel);
            id container=((id (*)(id,SEL))objc_msgSend)(layout,contSel);
            if([lText isKindOfClass:NSAttributedString.class] && container) {
                NSAttributedString *as=(NSAttributedString *)lText;
                NSUInteger stamp=(mode<<28) ^ (as.string.hash & 0x0FFFFFFF);
                NSNumber *prev=objc_getAssociatedObject(view,&HGACommentTypographyStampKey);
                if(!prev || prev.unsignedIntegerValue!=stamp) {
                    NSMutableAttributedString *mut=[as mutableCopy];
                    __block BOOL modified=NO;
                    [as enumerateAttribute:NSForegroundColorAttributeName inRange:NSMakeRange(0,mut.length) options:0
                        usingBlock:^(id val,NSRange range,BOOL *stop) {
                            (void)stop;
                            if(![val isKindOfClass:UIColor.class])return;
                            BOOL c=NO;
                            UIColor *bc=HGACommentBoostColor((UIColor *)val,traits,dark,&c);
                            if(c && bc) {
                                [mut addAttribute:NSForegroundColorAttributeName value:bc range:range];
                                [mut addAttribute:(__bridge NSAttributedStringKey)kCTForegroundColorAttributeName value:(__bridge id)bc.CGColor range:range];
                                modified=YES;
                            }
                        }];
                    if(modified) {
                        SEL makeSel=sel_registerName("layoutWithContainer:text:");
                        if([[layout class] respondsToSelector:makeSel]) {
                            id nl=((id (*)(id,SEL,id,id))objc_msgSend)([layout class],makeSel,container,mut);
                            if(nl)((void (*)(id,SEL,id))objc_msgSend)(view,setLayout,nl);
                        }
                    }
                    objc_setAssociatedObject(view,&HGACommentTypographyStampKey,@(stamp),OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                }
            }
        }
    }
    for(UIView *sub in view.subviews)HGACommentBoostTypography(sub,dark,depth+1);
}

static void HGACommentClearCovers(UIView *root,BOOL active) {
    if(![root isKindOfClass:UIView.class])return;
    CGFloat rootW=CGRectGetWidth(root.bounds);
    for(UIView *v in HGADescendants(root,180)) {
        if(v==root || HGAOwnView(v))continue;
        if([v isKindOfClass:UICollectionView.class] || [v isKindOfClass:UITableView.class]) {
            HGACommentSetClearSurface(v,active);
            if([v isKindOfClass:UICollectionView.class]) {
                UICollectionView *cv=(UICollectionView *)v;
                HGACommentSetClearSurface(cv.backgroundView,active);
                for(UICollectionViewCell *cell in cv.visibleCells) {
                    HGACommentSetClearSurface(cell,active);
                    HGACommentSetClearSurface(cell.contentView,active);
                }
            }
            continue;
        }
        if(CGRectGetWidth(v.bounds)<rootW*0.80)continue;
        BOOL plain=v.class==UIView.class || [v isKindOfClass:UIScrollView.class];
        if(!plain)continue;
        if(!active) {
            HGACommentSetClearSurface(v,NO);
            continue;
        }
        CGFloat r=0,g=0,b=0,a=0;
        if(v.backgroundColor && HGACommentColorGetRGBA([v.backgroundColor resolvedColorWithTraitCollection:v.traitCollection],&r,&g,&b,&a) &&
           a>0.80 && (MAX(r,MAX(g,b))-MIN(r,MIN(g,b)))<0.08) {
            HGACommentSetClearSurface(v,YES);
        }
    }
}

static void HGACommentRestoreSheet(UIView *sheet) {
    if(!sheet)return;
    HGClearSurfaceBackdropBlur(sheet);
    HGAAppearanceMaterial *glass=objc_getAssociatedObject(sheet,&HGACommentGlassKey);
    if(glass) {
        [glass removeFromSuperview];
        objc_setAssociatedObject(sheet,&HGACommentGlassKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    NSNumber *savedClips=objc_getAssociatedObject(sheet,&HGACommentSavedClipsKey);
    if(savedClips) {
        sheet.clipsToBounds=savedClips.boolValue;
        objc_setAssociatedObject(sheet,&HGACommentSavedClipsKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    NSNumber *savedRadius=objc_getAssociatedObject(sheet,&HGACommentSavedCornerRadiusKey);
    if(savedRadius) {
        sheet.layer.cornerRadius=savedRadius.doubleValue;
        objc_setAssociatedObject(sheet,&HGACommentSavedCornerRadiusKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    NSNumber *savedMask=objc_getAssociatedObject(sheet,&HGACommentSavedMaskedCornersKey);
    if(savedMask) {
        sheet.layer.maskedCorners=(CACornerMask)savedMask.unsignedIntegerValue;
        objc_setAssociatedObject(sheet,&HGACommentSavedMaskedCornersKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    if(sheet.overrideUserInterfaceStyle!=UIUserInterfaceStyleUnspecified) {
        sheet.overrideUserInterfaceStyle=UIUserInterfaceStyleUnspecified;
    }
    HGACommentSetClearSurface(sheet,NO);
    HGACommentClearCovers(sheet,NO);
    HGARemoveGlass(sheet);
}

static BOOL HGACommentHasVisibleSheetInWindow(UIWindow *window,UIViewController *excludingVC) {
    if(!window)return NO;
    for(UIViewController *vc in HGACommentTrackedVCs().allObjects) {
        if(vc==excludingVC || !vc.isViewLoaded)continue;
        UIView *v=vc.viewIfLoaded;
        if(!v || v.window!=window || v.hidden || v.alpha<0.05)continue;
        if(CGRectGetWidth(v.bounds)<100.0 || CGRectGetHeight(v.bounds)<100.0)continue;
        BOOL ancHidden=NO;
        for(UIView *p=v.superview;p && p!=window;p=p.superview) {
            if(p.hidden || p.alpha<0.05){ancHidden=YES;break;}
        }
        if(!ancHidden)return YES;
    }
    return NO;
}

static void HGACommentFindTabBars(UIView *view,Class cls,NSMutableArray<UIView *> *out,NSInteger depth) {
    if(!view || depth>8)return;
    if(cls && [view isKindOfClass:cls]){[out addObject:view];return;}
    for(UIView *sub in view.subviews)HGACommentFindTabBars(sub,cls,out,depth+1);
}

static void HGACommentSyncDockHidden(UIWindow *window,UIViewController *excludingVC) {
    if(!window)return;
    BOOL hasSheet=HGACommentHasVisibleSheetInWindow(window,excludingVC);
    BOOL shouldHide=HGACommentStylingEnabled() && hasSheet;
    if(!shouldHide) {
        for(UIView *dimmer in HGACommentTrackedDimmers().allObjects) {
            if(!dimmer.window || dimmer.window==window) {
                HGACommentSetDimmerZeroMask(dimmer,NO);
                HGACommentSetClearSurface(dimmer,NO);
            }
        }
    }
    Class tabBarCls=NSClassFromString(@"SSTabBar");
    if(!tabBarCls)return;
    NSMutableArray<UIView *> *bars=[NSMutableArray array];
    HGACommentFindTabBars(window,tabBarCls,bars,0);
    for(UIView *bar in bars) {
        BOOL hiddenByUs=[objc_getAssociatedObject(bar,@selector(hg_dockHiddenByCommentSheet)) boolValue];
        if(shouldHide && !hiddenByUs) {
            objc_setAssociatedObject(bar,@selector(hg_dockHiddenByCommentSheet),@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            bar.userInteractionEnabled=NO;
            [UIView animateWithDuration:0.16 delay:0 options:UIViewAnimationOptionBeginFromCurrentState|UIViewAnimationOptionAllowUserInteraction
                             animations:^{ bar.alpha=0.002; } completion:nil];
        } else if(!shouldHide && hiddenByUs) {
            objc_setAssociatedObject(bar,@selector(hg_dockHiddenByCommentSheet),nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            bar.userInteractionEnabled=YES;
            [UIView animateWithDuration:0.20 delay:0 options:UIViewAnimationOptionBeginFromCurrentState|UIViewAnimationOptionAllowUserInteraction
                             animations:^{ bar.alpha=1.0; } completion:nil];
        }
    }
}

static void HGACommentStyleBar(UIView *toolbar) {
    if(![toolbar isKindOfClass:UIView.class] || !toolbar.window)return;
    [HGACommentTrackedBars() addObject:toolbar];
    NSString *mat=HGACommentEffectiveMaterial();
    NSString *composer=HGAppearanceString(@"comment.composer");
    BOOL active=mat!=nil && ![composer isEqual:@"native"] && CGRectGetWidth(toolbar.bounds)>=44.0 && CGRectGetHeight(toolbar.bounds)>=24.0;
    HGAAppearanceMaterial *barGlass=objc_getAssociatedObject(toolbar,&HGACommentComposerGlassKey);
    if(!active) {
        if(barGlass) {
            HGClearSurfaceBackdropBlur(toolbar);
            [barGlass removeFromSuperview];
            objc_setAssociatedObject(toolbar,&HGACommentComposerGlassKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        HGACommentSetClearSurface(toolbar,NO);
        HGACommentSetClearSurface(HGAViewGetter(toolbar,"bgView",toolbar),NO);
        HGACommentSetHiddenTracked(HGAViewGetter(toolbar,"topLineView",toolbar),NO);
        return;
    }
    BOOL floating=[composer isEqual:@"floating"];
    BOOL dark=HGACommentUseDarkTheme(toolbar);
    CGFloat w=CGRectGetWidth(toolbar.bounds),h=CGRectGetHeight(toolbar.bounds);
    CGFloat insetX=floating ? 12.0 : 0.0;
    CGFloat pillH=floating ? MIN(48.0,MAX(36.0,h-6.0)) : h;
    CGFloat pillY=floating ? MAX(0.0,(h-pillH)*0.5) : 0.0;
    CGRect pillFrame=CGRectMake(insetX,pillY,MAX(44.0,w-insetX*2.0),pillH);
    CGFloat radius=floating ? pillH*0.5 : 0.0;
    NSString *mappedStyle=[mat isEqual:@"gaussian"] ? @"blur" : mat;
    if(!barGlass) {
        barGlass=[[HGAAppearanceMaterial alloc] initWithEffect:nil];
        barGlass.accessibilityIdentifier=@"honggo.appearance.comment.composer";
        barGlass.accessibilityElementsHidden=YES;
        barGlass.userInteractionEnabled=NO;
        barGlass.layer.cornerCurve=kCACornerCurveContinuous;
        barGlass.clipsToBounds=YES;
        objc_setAssociatedObject(toolbar,&HGACommentComposerGlassKey,barGlass,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    barGlass.style=mappedStyle;
    barGlass.interactiveGlass=floating && ![mappedStyle isEqual:@"blur"] && ![mappedStyle isEqual:@"solid"] && !UIAccessibilityIsReduceTransparencyEnabled();
    barGlass.overrideUserInterfaceStyle=dark ? UIUserInterfaceStyleDark : UIUserInterfaceStyleLight;
    barGlass.overlayOpacity=0;
    barGlass.overlayColor=[UIColor colorWithWhite:dark?0.06:0.96 alpha:[mappedStyle isEqual:@"clear"]?0.0:0.12];
    if(!CGRectEqualToRect(barGlass.frame,pillFrame))barGlass.frame=pillFrame;
    barGlass.layer.cornerRadius=radius;
    if(@available(iOS 26.0,*))HGApplyNativeCornerConfig(barGlass,radius,floating);
    if(barGlass.superview!=toolbar)[toolbar insertSubview:barGlass atIndex:0];
    [barGlass requestMaterialRefresh];
    HGApplySurfaceBackdropBlur(toolbar,toolbar,barGlass,pillFrame,radius,dark,mappedStyle,HGAppearanceNumber(@"comment.blur"));
    HGACommentSetClearSurface(toolbar,YES);
    HGACommentSetClearSurface(HGAViewGetter(toolbar,"bgView",toolbar),YES);
    HGACommentSetHiddenTracked(HGAViewGetter(toolbar,"topLineView",toolbar),YES);
    toolbar.overrideUserInterfaceStyle=dark ? UIUserInterfaceStyleDark : UIUserInterfaceStyleLight;
    HGACommentBoostTypography(toolbar,dark,0);
}

static void HGACommentSetBannerCollapsed(UIView *banner,BOOL hide) {
    if(![banner isKindOfClass:UIView.class])return;
    HGACommentSetHiddenTracked(banner,hide);
    if(hide) {
        if(banner.alpha>0.001)banner.alpha=0.0;
        if(CGRectGetHeight(banner.frame)>0.5) {
            if(!objc_getAssociatedObject(banner,&HGACommentBannerSavedHeightKey)) {
                objc_setAssociatedObject(banner,&HGACommentBannerSavedHeightKey,@(CGRectGetHeight(banner.frame)),OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            }
            CGRect f=banner.frame;f.size.height=0.0;banner.frame=f;
        }
    } else {
        if(banner.alpha<0.01)banner.alpha=1.0;
        NSNumber *saved=objc_getAssociatedObject(banner,&HGACommentBannerSavedHeightKey);
        if(saved) {
            objc_setAssociatedObject(banner,&HGACommentBannerSavedHeightKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            if(saved.doubleValue>1.0 && CGRectGetHeight(banner.frame)<1.0) {
                CGRect f=banner.frame;f.size.height=saved.doubleValue;banner.frame=f;
                [banner.superview setNeedsLayout];
            }
        }
    }
}

static void HGAApplyCommentSheetView(UIView *root) {
    if(![root isKindOfClass:UIView.class])return;
    [HGACommentTrackedSheets() addObject:root];
    NSString *mat=HGACommentEffectiveMaterial();
    if(!mat || !root.window || CGRectIsEmpty(root.bounds)) {
        HGACommentRestoreSheet(root);
        HGACommentCleanPresentationBackdrops(root,nil,NO);
        return;
    }
    BOOL dark=HGACommentUseDarkTheme(root);
    UIUserInterfaceStyle uiStyle=HGACommentDesiredUIStyle(root);
    NSString *mappedStyle=[mat isEqual:@"gaussian"] ? @"blur" : mat;
    BOOL clearGlass=[mappedStyle isEqual:@"clear"];
    BOOL reduced=UIAccessibilityIsReduceTransparencyEnabled();
    CGFloat radius=16.0;

    if(!objc_getAssociatedObject(root,&HGACommentSavedClipsKey)) {
        objc_setAssociatedObject(root,&HGACommentSavedClipsKey,@(root.clipsToBounds),OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(root,&HGACommentSavedCornerRadiusKey,@(root.layer.cornerRadius),OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(root,&HGACommentSavedMaskedCornersKey,@(root.layer.maskedCorners),OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    root.clipsToBounds=YES;
    root.layer.cornerCurve=kCACornerCurveContinuous;
    root.layer.cornerRadius=radius;
    root.layer.maskedCorners=kCALayerMinXMinYCorner|kCALayerMaxXMinYCorner;
    if(root.overrideUserInterfaceStyle!=uiStyle)root.overrideUserInterfaceStyle=uiStyle;
    HGACommentSetClearSurface(root,YES);

    HGAAppearanceMaterial *glass=objc_getAssociatedObject(root,&HGACommentGlassKey);
    if(!glass) {
        glass=[[HGAAppearanceMaterial alloc] initWithEffect:nil];
        glass.accessibilityIdentifier=@"honggo.appearance.material";
        glass.accessibilityElementsHidden=YES;
        glass.userInteractionEnabled=NO;
        glass.layer.cornerCurve=kCACornerCurveContinuous;
        glass.clipsToBounds=YES;
        objc_setAssociatedObject(root,&HGACommentGlassKey,glass,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    if(![glass.style isEqual:mappedStyle]) {
        glass.style=mappedStyle;
        glass.materialSignature=nil;
        [glass setNeedsLayout];
    }
    glass.interactiveGlass=NO;
    glass.overrideUserInterfaceStyle=uiStyle;
    glass.overlayOpacity=0;
    CGFloat overlayAlpha=reduced ? 1.0 : (clearGlass ? 0.0 : ([mappedStyle isEqual:@"blur"] ? 0.08 : 0.14));
    glass.overlayColor=[UIColor colorWithWhite:dark?0.08:0.97 alpha:overlayAlpha];
    if(!CGRectEqualToRect(glass.frame,root.bounds))glass.frame=root.bounds;
    glass.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
    glass.layer.cornerRadius=radius;
    glass.layer.maskedCorners=kCALayerMinXMinYCorner|kCALayerMaxXMinYCorner;
    if(glass.superview!=root)[root insertSubview:glass atIndex:0];
    [glass requestMaterialRefresh];
    if(@available(iOS 17.0,*))[glass updateTraitsIfNeeded];

    double blurPct=HGAppearanceNumber(@"comment.blur");
    if([mat isEqual:@"gaussian"] && blurPct<1.0)blurPct=45.0;
    HGApplySurfaceBackdropBlur(root,root,glass,root.bounds,radius,dark,mappedStyle,blurPct);

    UIView *transition=nil;
    Class transCls=NSClassFromString(@"FQVTransitionView");
    for(UIView *p=root.superview;p && ![p isKindOfClass:UIWindow.class];p=p.superview) {
        if(transCls && [p isKindOfClass:transCls]){transition=p;break;}
    }
    HGACommentCleanPresentationBackdrops(root,transition,YES);
    HGACommentClearCovers(root,YES);
    HGACommentBoostTypography(root,dark,0);
    if(root.window)HGACommentSyncDockHidden(root.window,nil);
}

static void HGAApplyCommentController(UIViewController *controller) {
    if(!controller || !controller.isViewLoaded)return;
    [HGACommentTrackedVCs() addObject:controller];
    HGAApplyCommentSheetView(controller.view);
    UIView *bar=HGAViewGetter(controller,"bottomToolBar",controller.view);
    if(bar)HGACommentStyleBar(bar);
}

static BOOL HGACommentIsTopPlayerView(id view) {
    if(!view)return NO;
    Class c1=NSClassFromString(@"FQVTransitionPlayerTopContentView");
    Class c2=NSClassFromString(@"FQVTransitionPlayerLimitScaleTopContentView");
    return (c1 && [view isKindOfClass:c1]) || (c2 && [view isKindOfClass:c2]);
}

static void (*HGOrigTransitionSetTop)(id,SEL,id);
static void HGHookTransitionSetTop(id self,SEL cmd,id topView) {
    if(HGAppearanceEnabled() && HGAppearanceFlag(@"comment.noVideoShift") && HGACommentIsTopPlayerView(topView)) {
        objc_setAssociatedObject(topView,&HGACommentNoVideoShiftKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        HGOrigTransitionSetTop(self,cmd,nil);
        return;
    }
    HGOrigTransitionSetTop(self,cmd,topView);
}
static void (*HGOrigTopBindPlayer)(id,SEL,id,CGRect);
static void HGHookTopBindPlayer(id self,SEL cmd,id playerView,CGRect origin) {
    if(HGAppearanceEnabled() && HGAppearanceFlag(@"comment.noVideoShift")) {
        objc_setAssociatedObject(self,&HGACommentNoVideoShiftKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        return;
    }
    objc_setAssociatedObject(self,&HGACommentNoVideoShiftKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    HGOrigTopBindPlayer(self,cmd,playerView,origin);
}
static void (*HGOrigTopUnbindPlayer)(id,SEL,id);
static void HGHookTopUnbindPlayer(id self,SEL cmd,id playerView) {
    if([objc_getAssociatedObject(self,&HGACommentNoVideoShiftKey) boolValue]) {
        objc_setAssociatedObject(self,&HGACommentNoVideoShiftKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        return;
    }
    HGOrigTopUnbindPlayer(self,cmd,playerView);
}
static void (*HGOrigLimitBindPlayer)(id,SEL,id,CGRect);
static void HGHookLimitBindPlayer(id self,SEL cmd,id playerView,CGRect origin) {
    if(HGAppearanceEnabled() && HGAppearanceFlag(@"comment.noVideoShift")) {
        objc_setAssociatedObject(self,&HGACommentNoVideoShiftKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        return;
    }
    objc_setAssociatedObject(self,&HGACommentNoVideoShiftKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    HGOrigLimitBindPlayer(self,cmd,playerView,origin);
}
static void (*HGOrigLimitUnbindPlayer)(id,SEL,id);
static void HGHookLimitUnbindPlayer(id self,SEL cmd,id playerView) {
    if([objc_getAssociatedObject(self,&HGACommentNoVideoShiftKey) boolValue]) {
        objc_setAssociatedObject(self,&HGACommentNoVideoShiftKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        return;
    }
    HGOrigLimitUnbindPlayer(self,cmd,playerView);
}
static void (*HGOrigTransitionShow)(id,SEL,id,BOOL,id);
static void HGHookTransitionShow(UIView *self,SEL cmd,UIView *parent,BOOL animated,id completion) {
    HGOrigTransitionShow(self,cmd,parent,animated,completion);
    UIWindow *win=parent.window ?: self.window;
    if(win) {
        dispatch_async(dispatch_get_main_queue(),^{ HGACommentSyncDockHidden(win,nil); });
    }
}
static void (*HGOrigTransitionDismiss)(id,SEL,BOOL,id);
static void HGHookTransitionDismiss(UIView *self,SEL cmd,BOOL animated,id completion) {
    UIWindow *win=self.window;
    HGOrigTransitionDismiss(self,cmd,animated,completion);
    if(win) {
        HGACommentSyncDockHidden(win,nil);
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,(int64_t)(0.26*NSEC_PER_SEC)),dispatch_get_main_queue(),^{
            HGACommentSyncDockHidden(win,nil);
        });
    }
}

static double (*HGOrigCommentTitleHeight)(id,SEL);
static double HGHookCommentTitleHeight(id cls,SEL cmd) {
    if(HGAppearanceEnabled() && HGAppearanceFlag(@"comment.hideActivityBanner"))return 0.0;
    return HGOrigCommentTitleHeight ? HGOrigCommentTitleHeight(cls,cmd) : 36.0;
}
static void (*HGOrigCommentTitleUpdate)(id,SEL,id);
static void HGHookCommentTitleUpdate(UIView *self,SEL cmd,id models) {
    HGOrigCommentTitleUpdate(self,cmd,models);
    [HGACommentTrackedBanners() addObject:self];
    HGACommentSetBannerCollapsed(self,HGAppearanceEnabled() && HGAppearanceFlag(@"comment.hideActivityBanner"));
}
static void (*HGOrigCommentSearchLayout)(id,SEL);
static void HGHookCommentSearchLayout(UIView *self,SEL cmd) {
    HGOrigCommentSearchLayout(self,cmd);
    [HGACommentTrackedSearchEntrances() addObject:self];
    BOOL hide=HGAppearanceEnabled() && HGAppearanceFlag(@"comment.hideSearchEntrance");
    HGACommentSetBannerCollapsed(self,hide);
    if(!hide && HGACommentStylingEnabled()) {
        HGACommentSetClearSurface(self,YES);
        HGACommentBoostTypography(self,HGACommentUseDarkTheme(self),0);
    }
}
static void (*HGOrigCommentBarLayout)(id,SEL);
static void HGHookCommentBarLayout(UIView *self,SEL cmd) {
    HGOrigCommentBarLayout(self,cmd);
    HGACommentStyleBar(self);
}
static void (*HGOrigCommentPanelShow)(id,SEL);
static void HGHookCommentPanelShow(UIViewController *self,SEL cmd) {
    HGOrigCommentPanelShow(self,cmd);
    HGAApplyCommentController(self);
}
static void (*HGOrigCommentPanelDismiss)(id,SEL);
static void HGHookCommentPanelDismiss(UIViewController *self,SEL cmd) {
    UIWindow *win=self.viewIfLoaded.window;
    HGOrigCommentPanelDismiss(self,cmd);
    if(win)HGACommentSyncDockHidden(win,self);
}
static void (*HGOrigCommentListDisappear)(id,SEL,BOOL);
static void HGHookCommentListDisappear(UIViewController *self,SEL cmd,BOOL animated) {
    UIWindow *win=self.viewIfLoaded.window;
    HGOrigCommentListDisappear(self,cmd,animated);
    if(win)HGACommentSyncDockHidden(win,self);
}
static void (*HGOrigCommentCellLayout)(id,SEL);
static void HGHookCommentCellLayout(UICollectionViewCell *self,SEL cmd) {
    HGOrigCommentCellLayout(self,cmd);
    BOOL active=HGACommentStylingEnabled();
    HGACommentSetClearSurface(self,active);
    HGACommentSetClearSurface(self.contentView,active);
    if(active)HGACommentBoostTypography(self,HGACommentUseDarkTheme(self),0);
}
static void (*HGOrigCommentFooterLayout)(id,SEL);
static void HGHookCommentFooterLayout(UIView *self,SEL cmd) {
    HGOrigCommentFooterLayout(self,cmd);
    BOOL active=HGACommentStylingEnabled();
    HGACommentSetClearSurface(self,active);
    if(active)HGACommentBoostTypography(self,HGACommentUseDarkTheme(self),0);
}

static void HGACommentRefreshAll(void) {
    for(UIView *sheet in HGACommentTrackedSheets().allObjects)HGAApplyCommentSheetView(sheet);
    for(UIView *bar in HGACommentTrackedBars().allObjects)HGACommentStyleBar(bar);
    BOOL hideBanner=HGAppearanceEnabled() && HGAppearanceFlag(@"comment.hideActivityBanner");
    for(UIView *b in HGACommentTrackedBanners().allObjects)HGACommentSetBannerCollapsed(b,hideBanner);
    BOOL hideSearch=HGAppearanceEnabled() && HGAppearanceFlag(@"comment.hideSearchEntrance");
    for(UIView *s in HGACommentTrackedSearchEntrances().allObjects)HGACommentSetBannerCollapsed(s,hideSearch);
    for(UIScene *scene in UIApplication.sharedApplication.connectedScenes) {
        if(![scene isKindOfClass:UIWindowScene.class])continue;
        for(UIWindow *w in ((UIWindowScene *)scene).windows)HGACommentSyncDockHidden(w,nil);
    }
}

static void HGAInstallCommentHooks(void) {
    static dispatch_once_t once;
    dispatch_once(&once,^{
        NSUInteger installed=0;
#define HGC_HOOK(clsName,selName,enc,origVar,hookFn) do { \
    Class _c=NSClassFromString(@clsName); \
    if(_c) { \
        IMP _o=HGInstallVerifiedHook(_c,sel_registerName(selName),enc,(IMP)(hookFn),@"appearance.comments"); \
        if(_o){ origVar=(__typeof__(origVar))_o; installed++; } \
    } \
} while(0)
        HGC_HOOK("FQVTransitionView","setTopContentView:","v24@0:8@16",HGOrigTransitionSetTop,HGHookTransitionSetTop);
        HGC_HOOK("FQVTransitionPlayerTopContentView","bindPlayerView:originPlayerFrameOnWindow:","v56@0:8@16{CGRect={CGPoint=dd}{CGSize=dd}}24",HGOrigTopBindPlayer,HGHookTopBindPlayer);
        HGC_HOOK("FQVTransitionPlayerTopContentView","unbindPlayerView:","v24@0:8@16",HGOrigTopUnbindPlayer,HGHookTopUnbindPlayer);
        HGC_HOOK("FQVTransitionPlayerLimitScaleTopContentView","bindPlayerView:originPlayerFrameOnWindow:","v56@0:8@16{CGRect={CGPoint=dd}{CGSize=dd}}24",HGOrigLimitBindPlayer,HGHookLimitBindPlayer);
        HGC_HOOK("FQVTransitionPlayerLimitScaleTopContentView","unbindPlayerView:","v24@0:8@16",HGOrigLimitUnbindPlayer,HGHookLimitUnbindPlayer);
        HGC_HOOK("FQVTransitionView","showInView:animated:completion:","v36@0:8@16B24@?28",HGOrigTransitionShow,HGHookTransitionShow);
        HGC_HOOK("FQVTransitionView","dismissAnimated:completion:","v28@0:8B16@?20",HGOrigTransitionDismiss,HGHookTransitionDismiss);
        Class titleCls=NSClassFromString(@"FQCShortVideoCommentTitleView");
        if(titleCls) {
            SEL hSel=sel_registerName("commentTitleViewHeight");
            Class meta=object_getClass(titleCls);
            Method m=class_getInstanceMethod(meta,hSel);
            if(m && !strcmp(method_getTypeEncoding(m),"d16@0:8")) {
                IMP o=HGInstallVerifiedHook(meta,hSel,"d16@0:8",(IMP)HGHookCommentTitleHeight,@"appearance.comments");
                if(o){HGOrigCommentTitleHeight=(double(*)(id,SEL))o;installed++;}
            }
        }
        HGC_HOOK("FQCShortVideoCommentTitleView","updateWithModels:","v24@0:8@16",HGOrigCommentTitleUpdate,HGHookCommentTitleUpdate);
        HGC_HOOK("FQCShortVideoCommentSearchEntranceView","layoutSubviews","v16@0:8",HGOrigCommentSearchLayout,HGHookCommentSearchLayout);
        HGC_HOOK("FQCCommentBottomToolBar","layoutSubviews","v16@0:8",HGOrigCommentBarLayout,HGHookCommentBarLayout);
        HGC_HOOK("FQCShortVideoCommentPanelViewController","onVCWillShow","v16@0:8",HGOrigCommentPanelShow,HGHookCommentPanelShow);
        HGC_HOOK("FQCShortVideoCommentPanelViewController","onVCDismiss","v16@0:8",HGOrigCommentPanelDismiss,HGHookCommentPanelDismiss);
        HGC_HOOK("FQCShortVideoCommentListViewController","viewDidDisappear:","v20@0:8B16",HGOrigCommentListDisappear,HGHookCommentListDisappear);
        HGC_HOOK("FQCShortVideoCommentCommonCell","layoutSubviews","v16@0:8",HGOrigCommentCellLayout,HGHookCommentCellLayout);
        HGC_HOOK("FQCShortVideoCommentSectionFooterView","layoutSubviews","v16@0:8",HGOrigCommentFooterLayout,HGHookCommentFooterLayout);
#undef HGC_HOOK
        HGRecordCapability(@"appearance.comments",@{@"module":@"appearance",
            @"state":installed?@"installed":@"unavailable",@"installedHooks":@(installed)});
    });
}
