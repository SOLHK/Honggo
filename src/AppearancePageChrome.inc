static NSArray<NSString *> *HGAPageChromeViews(void) {
    return @[@"FQRJXCategoryTitleCell",@"FQRJXCategoryTitleView",@"SSHGBookStoreCategoryTitleCell",@"SSHGBookStoreCategoryTitleView",
        @"SSBookStoreEntranceView",@"SSBookStoreKingKongBannerView",
        @"SSVideoDistributionGoldenPositionCell",@"SSVideoDistributionGoldenPositionSubCell",
        @"FQFeedTabFilterPanelEntranceView",@"SSMyUser637TabsView",@"SSVideoFilterPanelEntranceView",
        @"SSMyUser637Bar",@"SSMyUserFollowItemView",@"SSMyUser637JGZoneCell"];
}

static NSArray<NSString *> *HGAFinderControllers(void) {
    return @[@"FQTheaterViewController",@"SSBookStoreNewViewController"];
}

static NSString *HGAFinderCategoryKey(NSString *title) {
    static NSDictionary *keys;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        keys=@{@"找剧":@"dramas",@"真人剧":@"live",@"社区":@"community",@"漫剧":@"comics",
            @"电影":@"films",@"听书":@"audio",@"小说":@"novels",@"漫画":@"manga",
            @"长视频":@"long",@"长剧":@"long"};
    });
    return [title isKindOfClass:NSString.class] ? keys[title] : nil;
}

static NSString *HGAFinderShortcutKey(NSString *title) {
    if(![title isKindOfClass:NSString.class])return nil;
    if([title isEqual:@"筛选"])return @"filter";
    if([title isEqual:@"排行榜"] || [title isEqual:@"热播榜"])return @"ranking";
    if([title isEqual:@"新剧"])return @"new";
    if([title isEqual:@"预约"])return @"reserve";
    return nil;
}

static NSArray<NSString *> *HGAMineShortcuts(void) {
    static NSArray *keys;
    static dispatch_once_t once;
    dispatch_once(&once, ^{keys=@[@"store",@"messages",@"games",@"updates",@"more"];});
    return keys;
}

static NSString *HGAMineShortcutKey(NSString *title) {
    static NSDictionary *keys;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        keys=@{@"商城":@"store",@"订单":@"store",@"消息":@"messages",@"游戏":@"games",@"游戏中心":@"games",
            @"追更":@"updates",@"更多功能":@"more",@"更多\n功能":@"more"};
    });
    return [title isKindOfClass:NSString.class] ? keys[title] : nil;
}

static UIView *HGAAncestorNamed(UIView *view,NSString *name) {
    Class type=NSClassFromString(name);
    for(UIView *parent=view.superview;parent;parent=parent.superview)
        if([parent isKindOfClass:type])return parent;
    return nil;
}

static BOOL HGAHeaderShowsFinder(UIView *header) {
    BOOL finder=NO;
    return HGABoolGetter(header,@"isFindDramaSearchPresentationActiveV731",&finder) && finder;
}

static BOOL HGAIsHeaderView(UIView *view) {
    return [view isKindOfClass:NSClassFromString(@"SSVideoSeriesFeedHeaderView")];
}

static NSString *HGAPageChromeLabel(UIView *view,const char *getter) {
    UIView *label=HGAViewGetter(view,getter,view);
    return [label isKindOfClass:UILabel.class] ? ((UILabel *)label).text : nil;
}

static UIView *HGAHistoryFilter(UIView *toolbar) {
    Class type=NSClassFromString(@"SSVideoFilterPanelEntranceView");
    for(UIView *child in toolbar.subviews)
        if([child isKindOfClass:type])return child;
    return nil;
}

// The landscape controls own their taps. Observe the press without
// recognizing a gesture so UIKit can highlight the backing button.
#import <UIKit/UIGestureRecognizerSubclass.h>
@interface HGAPageButtonPress : UIGestureRecognizer
@property(nonatomic,weak) UIButton *button;
@end
@implementation HGAPageButtonPress
- (BOOL)canPreventGestureRecognizer:(UIGestureRecognizer *)other { return NO; }
- (BOOL)canBePreventedByGestureRecognizer:(UIGestureRecognizer *)other { return NO; }
- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    UITouch *touch=touches.anyObject;
    if (CGRectContainsPoint(self.button.bounds,[touch locationInView:self.button])) self.button.highlighted=YES;
    else self.state=UIGestureRecognizerStateFailed;
}
- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    if (!CGRectContainsPoint(self.button.bounds,[touches.anyObject locationInView:self.button]))
        self.state=UIGestureRecognizerStateFailed;
}
- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event { self.state=UIGestureRecognizerStateFailed; }
- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event { self.state=UIGestureRecognizerStateFailed; }
- (void)reset { self.button.highlighted=NO; [super reset]; }
@end

@interface HGAPageGlassButton : UIButton
@property(nonatomic,strong) HGAPageButtonPress *press;
@end
@implementation HGAPageGlassButton
- (void)dealloc { [_press.view removeGestureRecognizer:_press]; }
@end

static char HGAPageButtonMaterialKey;
static void HGAConfigurePageButton(UIButton *button,NSString *style,UIImage *image) {
    NSString *previous=objc_getAssociatedObject(button,&HGAPageButtonMaterialKey);
    if ([previous isEqual:style] || (!previous && !style)) return;
    UIButtonConfiguration *config=nil;
    if (style) {
        if (@available(iOS 26.0,*)) config=[style isEqual:@"clear"] ?
            UIButtonConfiguration.clearGlassButtonConfiguration : UIButtonConfiguration.glassButtonConfiguration;
        else config=UIButtonConfiguration.grayButtonConfiguration;
        config.cornerStyle=UIButtonConfigurationCornerStyleCapsule;
        config.contentInsets=NSDirectionalEdgeInsetsZero;
        config.image=image;
        if (image) config.baseForegroundColor=UIColor.labelColor;
        else {
            config.buttonSize=UIButtonConfigurationSizeSmall;
            UIBackgroundConfiguration *background=config.background;
            background.backgroundInsets=NSDirectionalEdgeInsetsMake(2,2,2,2);
            config.background=background;
        }
    }
    button.configuration=config;
    objc_setAssociatedObject(button,&HGAPageButtonMaterialKey,style,OBJC_ASSOCIATION_COPY_NONATOMIC);
}

static void HGAApplyPageButtonGlass(UIView *target,CGRect frame,UIView *pressHost) {
    if (!target.window || target.hidden || CGRectIsEmpty(frame)) {
        HGARemoveGlass(target);
        return;
    }
    // Catalogue and profile controls keep their own taps. Lay out their UIKit
    // glass directly at the host size so its compositor can draw the full rim.
    UIUserInterfaceStyle appearance=HGAAdaptivePageStyle(HGAOwner(target),target);
    CGFloat radius=MIN(CGRectGetWidth(frame),CGRectGetHeight(frame))/2;
    HGAInstallButtonGlassWithAppearance(target,HGALiquidButtonMaterial(),frame,radius,appearance);
    HGAState(target,NO).glass.flexTouchHost=pressHost;
    HGAApplyAdaptiveControlInk(target,appearance,YES);
}

static void HGAApplyPageChromeMaterial(UIView *target,CGRect frame,NSString *style) {
    if(HGAVisible(target) && !CGRectIsEmpty(frame)) {
        CGFloat radius=MIN(CGRectGetHeight(frame)/2,24);
        HGAInstallAdaptiveButtonGlass(target,style,frame,radius);
    } else if(target)HGARemoveGlass(target);
}

static void HGAApplyPageChrome(UIView *target,BOOL enabled) {
    if([target isKindOfClass:UICollectionViewCell.class])target=((UICollectionViewCell *)target).contentView;
    if(enabled)HGAApplyPageChromeMaterial(target,target.bounds,HGAppearanceString(@"player.material"));
    else if(target)HGARemoveGlass(target);
}

static char HGAPageChromeHiddenKey;
static void HGAHidePageChromeTarget(UIView *owner,NSString *key,UIView *target) {
    NSMapTable<NSString *,UIView *> *hidden=objc_getAssociatedObject(owner,&HGAPageChromeHiddenKey);
    if(!target && !hidden)return;
    UIView *previous=[hidden objectForKey:key];
    if(previous && previous!=target)HGASetHidden(previous,NO);
    if(target) {
        if(!hidden) {
            hidden=[NSMapTable strongToWeakObjectsMapTable];
            objc_setAssociatedObject(owner,&HGAPageChromeHiddenKey,hidden,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        [hidden setObject:target forKey:key];
        HGASetHidden(target,YES);
    } else {
        [hidden removeObjectForKey:key];
        if(!hidden.count)objc_setAssociatedObject(owner,&HGAPageChromeHiddenKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
}

static void HGASetPageChromeCover(UIView *container,const char *getter,BOOL hide) {
    UIView *cover=hide ? HGAViewGetter(container,getter,container) : nil;
    if(cover==container || ![cover isKindOfClass:UIImageView.class])cover=nil;
    HGAHidePageChromeTarget(container,@(getter),cover);
}

static char HGAPageChromeTargetsKey;
static void HGAUpdatePageChromeTargets(UIView *owner,NSArray<UIView *> *targets) {
    NSHashTable *previous=objc_getAssociatedObject(owner,&HGAPageChromeTargetsKey);
    for(UIView *target in previous)
        if(![targets containsObject:target])HGAApplyPageChrome(target,NO);
    if(!targets.count) {
        if(previous)objc_setAssociatedObject(owner,&HGAPageChromeTargetsKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        return;
    }
    if(!previous) {
        previous=[NSHashTable weakObjectsHashTable];
        objc_setAssociatedObject(owner,&HGAPageChromeTargetsKey,previous,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    [previous removeAllObjects];
    for(UIView *target in targets)[previous addObject:target];
}

static void HGASetPageChromeUnclipped(UIView *owner,NSArray<UIView *> *candidates);
static void HGAApplyPageChromeTargets(UIView *owner,NSArray<UIView *> *targets) {
    HGAUpdatePageChromeTargets(owner,targets);
    HGASetPageChromeUnclipped(owner,targets);
    for(UIView *target in targets)HGAApplyPageButtonGlass(target,target.bounds,target);
}

static BOOL HGAFinderControlReady(UIView *view) {
    // The native page transition can hide or fade a parent while its search
    // row is already laid out. Keep its glass ready for the incoming page.
    return view.window && !view.hidden && !CGRectIsEmpty(view.bounds);
}

static void HGAApplyFinderMaterial(UIView *target,CGRect frame,BOOL shortcut) {
    if(!HGAFinderControlReady(target) || CGRectIsEmpty(frame)) {
        HGARemoveGlass(target);
        return;
    }
    CGFloat radius=CGRectGetHeight(frame)/2;
    HGAInstallAdaptiveButtonGlass(target,HGALiquidButtonMaterial(),frame,shortcut ? MIN(radius,24) : radius);
}

@interface HGAPageChromeIcon : NSObject
@property(nonatomic,strong) UIImage *sourceImage;
@property(nonatomic,strong) UIImage *templateImage;
@property(nonatomic,strong) UIColor *sourceTint;
@property(nonatomic,strong) UIColor *appliedTint;
@end
@implementation HGAPageChromeIcon @end
static char HGAPageChromeIconKey, HGAPageChromeIconsKey;

static void HGAApplyPageChromeIcons(UIView *owner,NSArray<UIView *> *candidates) {
    NSHashTable<UIImageView *> *previous=objc_getAssociatedObject(owner,&HGAPageChromeIconsKey);
    for(UIImageView *icon in previous) {
        if([candidates containsObject:icon])continue;
        HGAPageChromeIcon *state=objc_getAssociatedObject(icon,&HGAPageChromeIconKey);
        if(icon.image==state.templateImage)icon.image=state.sourceImage;
        if(state.appliedTint && [icon.tintColor isEqual:state.appliedTint])icon.tintColor=state.sourceTint;
        objc_setAssociatedObject(icon,&HGAPageChromeIconKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    [previous removeAllObjects];
    UIColor *desired=HGAAdaptivePageStyle(HGAOwner(owner),owner)==UIUserInterfaceStyleDark ?
        UIColor.whiteColor : UIColor.labelColor;
    for(UIView *candidate in candidates) {
        if(![candidate isKindOfClass:UIImageView.class])continue;
        UIImageView *icon=(UIImageView *)candidate;
        if(!previous) {
            previous=[NSHashTable weakObjectsHashTable];
            objc_setAssociatedObject(owner,&HGAPageChromeIconsKey,previous,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        [previous addObject:icon];
        HGAPageChromeIcon *state=objc_getAssociatedObject(icon,&HGAPageChromeIconKey);
        if(!state) {
            state=[HGAPageChromeIcon new];
            state.sourceTint=icon.tintColor;
            objc_setAssociatedObject(icon,&HGAPageChromeIconKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        if(icon.image!=state.templateImage) {
            state.sourceImage=icon.image;
            state.templateImage=[icon.image imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
        }
        if(!state.appliedTint || ![icon.tintColor isEqual:state.appliedTint])state.sourceTint=icon.tintColor;
        state.appliedTint=desired;
        if(![icon.tintColor isEqual:desired])icon.tintColor=desired;
        if(icon.image!=state.templateImage)icon.image=state.templateImage;
    }
}

static char HGAHistoryCellLayersKey;
static void HGASetHistoryCellBackingHidden(UIView *cell,BOOL hide) {
    if(![cell isKindOfClass:UICollectionViewCell.class])return;
    UICollectionViewCell *item=(UICollectionViewCell *)cell;
    HGASetGlassHostBackground(item,hide);
    NSMapTable<CALayer *,NSNumber *> *saved=objc_getAssociatedObject(item,&HGAHistoryCellLayersKey);
    if(!hide) {
        for(CALayer *layer in saved.keyEnumerator.allObjects)
            layer.hidden=[[saved objectForKey:layer] boolValue];
        if(saved)objc_setAssociatedObject(item,&HGAHistoryCellLayersKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        HGAHidePageChromeTarget(item,@"historyCellBgView",nil);
        HGAHidePageChromeTarget(item,@"historyCellBorderView",nil);
        return;
    }
    if(!saved) {
        saved=[NSMapTable weakToStrongObjectsMapTable];
        objc_setAssociatedObject(item,&HGAHistoryCellLayersKey,saved,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    NSMutableArray<CALayer *> *candidates=[NSMutableArray array];
    const char *layerGetters[]={"bgLayer","backgroundLayer","borderLayer"};
    for(NSUInteger i=0;i<3;i++) {
        id candidate=HGReadObjectGetter(item,layerGetters[i]);
        if([candidate isKindOfClass:CALayer.class])[candidates addObject:candidate];
    }
    for(CALayer *sublayer in [item.contentView.layer.sublayers copy]) {
        if(sublayer.delegate)continue;
        BOOL painted=sublayer.cornerRadius>0 || sublayer.borderWidth>0 ||
            (sublayer.backgroundColor && CGColorGetAlpha(sublayer.backgroundColor)>0.001);
        if(painted && ![candidates containsObject:sublayer])[candidates addObject:sublayer];
    }
    for(CALayer *layer in candidates) {
        if(![saved objectForKey:layer])[saved setObject:@(layer.hidden) forKey:layer];
        if(!layer.hidden)layer.hidden=YES;
    }
    UIView *bgView=HGAViewGetter(item,"bgView",item) ?: HGAViewGetter(item,"backgroundView",item);
    if(bgView==item || bgView==item.contentView || [bgView isKindOfClass:UILabel.class])bgView=nil;
    UIView *borderView=HGAViewGetter(item,"borderView",item);
    if(borderView==item || borderView==item.contentView || [borderView isKindOfClass:UILabel.class])borderView=nil;
    HGAHidePageChromeTarget(item,@"historyCellBgView",bgView);
    HGAHidePageChromeTarget(item,@"historyCellBorderView",borderView);
}

static char HGAPageChromeUnclippedKey;
static void HGASetPageChromeUnclipped(UIView *owner,NSArray<UIView *> *candidates) {
    NSHashTable<UIView *> *saved=objc_getAssociatedObject(owner,&HGAPageChromeUnclippedKey);
    for(UIView *view in saved.allObjects) {
        if([candidates containsObject:view])continue;
        view.clipsToBounds=YES;
        [saved removeObject:view];
    }
    for(UIView *view in candidates) {
        if(!view.clipsToBounds)continue;
        if(!saved) {
            saved=NSHashTable.weakObjectsHashTable;
            objc_setAssociatedObject(owner,&HGAPageChromeUnclippedKey,saved,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        [saved addObject:view];
        view.clipsToBounds=NO;
    }
    if(saved && !saved.count)objc_setAssociatedObject(owner,&HGAPageChromeUnclippedKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

static CGFloat HGAHistoryActionCapsuleWidth(UIView *target) {
    CGFloat width=CGRectGetWidth(target.bounds);
    if(width<=0)return 0;
    return (!target.clipsToBounds && width<44) ? 44 : width;
}

static void HGAApplyHistoryGlass(UIView *target,BOOL circle,BOOL enabled) {
    UIView *pressHost=target;
    BOOL isCell=[target isKindOfClass:UICollectionViewCell.class];
    if(isCell) {
        UIView *content=((UICollectionViewCell *)target).contentView;
        HGASetHistoryCellBackingHidden(target,enabled);
        HGASetPageChromeUnclipped(target,enabled ? @[target,content] : nil);
        target=content;
    } else if(target) {
        HGASetPageChromeUnclipped(target,enabled ? @[target] : nil);
    }
    CGRect frame=enabled ? target.bounds : CGRectZero;
    if(enabled && !CGRectIsEmpty(frame)) {
        if(circle) {
            CGFloat side=target.clipsToBounds ? MIN(30,MIN(frame.size.width,frame.size.height)) : 30;
            frame=CGRectInset(frame,(frame.size.width-side)/2,(frame.size.height-side)/2);
        } else {
            // Keep 2pt vertical breathing room inside the 32pt clipped history
            // strip so UIGlassEffect's outer specular rim is never sliced flat,
            // and give right-side 30pt text controls matching 28pt capsules.
            CGFloat height=isCell ? MIN(28,frame.size.height) : 28;
            CGFloat width=isCell ? (frame.size.width>=44 ? frame.size.width-2 : frame.size.width) :
                HGAHistoryActionCapsuleWidth(target);
            frame=CGRectInset(frame,(frame.size.width-width)/2,(frame.size.height-height)/2);
        }
    }
    HGAApplyPageButtonGlass(target,frame,pressHost);
}

static char HGAFinderMovedKey;
static BOOL HGAReflowFinderRecognition(UIView *owner,UIView *recognition,UIView *assistant,BOOL hideAI,BOOL glass) {
    NSHashTable<UIView *> *moved=objc_getAssociatedObject(owner,&HGAFinderMovedKey);
    UIView *previous=moved.anyObject;
    if(previous && previous!=recognition)HGARestoreGeometry(previous);
    CGFloat offset=0;
    HGAViewState *state=HGAState(recognition,NO);
    if((hideAI || glass) && HGAFinderControlReady(recognition) && recognition.superview && assistant.superview && assistant.alpha>=0.01) {
        // Measure the host slot before our translation. The native control and
        // its gesture move together inside the same hit-testing parent.
        CGRect button=recognition.frame;
        if(state.geometry && state.geometrySuperview==recognition.superview &&
            CGAffineTransformEqualToTransform(recognition.transform,state.appliedTransform))
            button.origin.x-=state.appliedTransform.tx-state.sourceTransform.tx;
        CGRect slot=[assistant convertRect:assistant.bounds toView:recognition.superview];
        CGFloat gap=CGRectGetMinX(slot)-CGRectGetMaxX(button);
        if(!CGRectIsEmpty(slot) && gap>=-2 && gap<=36 && fabs(CGRectGetMidY(slot)-CGRectGetMidY(button))<8) {
            CGFloat edge=hideAI ? CGRectGetMaxX(slot) : CGRectGetMinX(slot)-10;
            offset=MAX(0,MIN(edge,CGRectGetMaxX(recognition.superview.bounds))-CGRectGetMaxX(button));
        }
    }
    if(offset>0) {
        if(!moved) {
            moved=[NSHashTable weakObjectsHashTable];
            objc_setAssociatedObject(owner,&HGAFinderMovedKey,moved,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        [moved removeAllObjects];[moved addObject:recognition];
        HGASetGeometry(recognition,offset,0,1);
    } else if(previous==recognition && previous) {
        [moved removeAllObjects];
        HGARestoreGeometry(recognition);
    }
    return offset>0;
}

@interface HGAFinderSearchVisibility : NSObject
@property(nonatomic,weak) UIView *sharedField;
@property(nonatomic,strong) NSHashTable<UIView *> *hidden;
@property(nonatomic,strong) NSHashTable<UIGestureRecognizer *> *disabledTaps;
@property(nonatomic) BOOL wasAccessibilityElement;
@end
@implementation HGAFinderSearchVisibility @end

static void HGASetFinderSearchHidden(UIView *owner,UIView *field,UIView *cue,UIView *alternate,
    UIView *recognition,BOOL hide) {
    static char key;
    HGAFinderSearchVisibility *state=objc_getAssociatedObject(owner,&key);
    if(!hide && !state)return;
    if(!state) {
        state=[HGAFinderSearchVisibility new];
        state.hidden=NSHashTable.weakObjectsHashTable;
        state.disabledTaps=NSHashTable.weakObjectsHashTable;
        objc_setAssociatedObject(owner,&key,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    // The 7.4.5 theatre mounts the recognition tap inside the search pill.
    // Keep that ancestor alive and suppress only its own search paint and taps.
    UIView *shared=hide && field && recognition!=field && [recognition isDescendantOfView:field] ? field : nil;
    if(state.sharedField!=shared) {
        if(state.sharedField) {
            HGASetGlassHostBackground(state.sharedField,NO);
            state.sharedField.isAccessibilityElement=state.wasAccessibilityElement;
        }
        for(UIGestureRecognizer *tap in state.disabledTaps)tap.enabled=YES;
        [state.disabledTaps removeAllObjects];
        state.sharedField=shared;
        state.wasAccessibilityElement=shared.isAccessibilityElement;
    }
    NSMutableSet<UIView *> *parts=[NSMutableSet set];
    if(shared) {
        HGASetGlassHostBackground(shared,YES);
        shared.isAccessibilityElement=NO;
        for(UIGestureRecognizer *tap in shared.gestureRecognizers)
            if([tap isKindOfClass:UITapGestureRecognizer.class] && tap.enabled) {
                [state.disabledTaps addObject:tap];
                tap.enabled=NO;
            }
        for(UIView *child in shared.subviews)
            if(!HGAOwnView(child) && child!=recognition && ![recognition isDescendantOfView:child])
                [parts addObject:child];
    } else if(hide && field)[parts addObject:field];
    UIView *controls[]={cue,alternate};
    for(NSUInteger i=0;hide && i<2;i++) {
        UIView *part=controls[i];
        if(!part || part==owner || part==shared || part==recognition ||
            [recognition isDescendantOfView:part] || (recognition && [part isDescendantOfView:recognition]))continue;
        [parts addObject:part];
    }
    for(UIView *part in state.hidden)if(![parts containsObject:part])HGASetHidden(part,NO);
    [state.hidden removeAllObjects];
    for(UIView *part in parts){HGASetHidden(part,YES);[state.hidden addObject:part];}
    if(!hide)objc_setAssociatedObject(owner,&key,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

static void HGAApplyFinderSearch(UIView *owner,UIView *field,UIView *cue,UIView *alternate,UIView *recognition,
    UIView *assistant,UIView *recognitionContent,UIView *divider,BOOL glass) {
    if(!field && !recognition && !assistant) {
        HGAReflowFinderRecognition(owner,nil,nil,NO,NO);
        HGAHidePageChromeTarget(owner,@"finderAI",nil);
        HGAHidePageChromeTarget(owner,@"finderRecognition",nil);
        HGAHidePageChromeTarget(owner,@"finderRecognitionDivider",nil);
        HGAUpdatePageChromeTargets(owner,nil);
        HGASetFinderSearchHidden(owner,nil,nil,nil,nil,NO);
        HGASetPageChromeUnclipped(owner,nil);
        HGAApplyAdaptiveControlInk(owner,UIUserInterfaceStyleLight,NO);
        return;
    }
    HGAViewState *assistantState=HGAState(assistant,NO);
    BOOL hostShowsAI=assistantState.visibility && assistant.hidden==assistantState.appliedHidden ?
        !assistantState.sourceHidden : HGAFinderControlReady(assistant);
    BOOL hideAI=assistant && !HGAppearanceFlag(@"player.finderAI");
    BOOL hideSearch=!HGAppearanceFlag(@"player.finderSearch");
    BOOL hideRecognition=!HGAppearanceFlag(@"player.finderRecognition");
    HGAHidePageChromeTarget(owner,@"finderRecognition",hideRecognition ? recognition : nil);
    BOOL reflow=HGAReflowFinderRecognition(owner,hideRecognition ? nil : recognition,hostShowsAI?assistant:nil,hideAI,glass);
    HGAHidePageChromeTarget(owner,@"finderAI",hideAI ? assistant : nil);
    NSMutableArray<UIView *> *targets=[NSMutableArray array];
    if(glass && field && !hideSearch)[targets addObject:field];
    if(glass && recognition && recognition!=field && !hideRecognition)[targets addObject:recognition];
    if(glass && assistant && !hideAI)[targets addObject:assistant];
    HGAUpdatePageChromeTargets(owner,targets);
    HGASetFinderSearchHidden(owner,field,cue,alternate,hideRecognition ? nil : recognition,hideSearch);

    CGRect searchFrame=field ? field.bounds : CGRectZero;
    BOOL separate=glass && HGAFinderControlReady(field) && HGAFinderControlReady(recognition) && recognition!=field;
    if(separate) {
        CGRect button=[recognition convertRect:recognition.bounds toView:field];
        CGFloat gap=7;
        if(recognition.superview==field && !hideAI && HGAFinderControlReady(assistant)) {
            CGRect ai=[assistant convertRect:assistant.bounds toView:field];
            CGFloat remaining=CGRectGetMinX(ai)-CGRectGetMaxX(button);
            // Match the gap between recognition and the AI disc.
            if(remaining>=6 && remaining<=16)gap=remaining;
        }
        CGFloat edge=CGRectGetMinX(button)-gap;
        if(edge>CGRectGetMidX(searchFrame) &&
            (edge<CGRectGetMaxX(searchFrame) || (reflow && !field.clipsToBounds)))
            searchFrame.size.width=edge-CGRectGetMinX(searchFrame);
    }
    HGAHidePageChromeTarget(owner,@"finderRecognitionDivider",separate ? divider : nil);
    UIView *recognitionLabel=HGAViewGetter(recognitionContent,"textLabel",recognitionContent);
    UIView *recognitionIcon=HGAViewGetter(recognitionContent,"cameraIconView",recognitionContent);
    if(recognitionLabel && recognitionIcon && recognitionContent!=recognition) {
        // The native icon and label consume the right edge. Center their group
        // inside the measured capsule while preserving the native hit owner.
        UIView *parent=recognitionContent.superview;
        CGRect ink=CGRectUnion([recognitionLabel convertRect:recognitionLabel.bounds toView:parent],
            [recognitionIcon convertRect:recognitionIcon.bounds toView:parent]);
        CGRect surface=[recognition convertRect:recognition.bounds toView:parent];
        HGAViewState *state=HGAState(recognitionContent,NO);
        CGFloat applied=state.geometry && state.geometrySuperview==parent &&
            CGAffineTransformEqualToTransform(recognitionContent.transform,state.appliedTransform) ?
            state.appliedTransform.tx-state.sourceTransform.tx : 0;
        CGFloat offset=CGRectGetMidX(surface)-CGRectGetMidX(ink)+applied;
        HGASetGeometry(recognitionContent,separate && CGRectGetWidth(ink)<CGRectGetWidth(recognition.bounds) &&
            fabs(offset)<=12 ? offset : 0,0,1);
    }
    NSMutableArray<UIView *> *unclipped=[NSMutableArray array];
    for(UIView *target in targets) {
        if(!HGAFinderControlReady(target)) {
            HGARemoveGlass(target);
            continue;
        }
        CGRect frame=target==field ? searchFrame : target.bounds;
        if(target==assistant) {
            CGFloat side=MIN(frame.size.width,frame.size.height);
            frame=CGRectInset(frame,(frame.size.width-side)/2,(frame.size.height-side)/2);
        }
        CGRect rim=CGRectInset(frame,-2,-2);
        for(UIView *parent=target;parent && parent!=owner && ![parent isKindOfClass:UIScrollView.class];parent=parent.superview)
            if(!CGRectContainsRect(parent.bounds,[target convertRect:rim toView:parent]) &&
                ![unclipped containsObject:parent])[unclipped addObject:parent];
        HGAApplyPageButtonGlass(target,frame,target.superview);
    }
    HGASetPageChromeUnclipped(owner,unclipped);
    if(recognitionContent && recognitionContent!=recognition)
        HGAApplyAdaptiveControlInk(recognitionContent,HGAAdaptivePageStyle(HGAOwner(owner),owner),glass);
    if(field && field.superview && field.superview!=owner)
        HGAApplyAdaptiveControlInk(field.superview,HGAAdaptivePageStyle(HGAOwner(owner),owner),glass);
}

static char HGAFinderAIInkKey;
static void HGAApplyFinderAIInk(UIView *owner,UIView *assistant,id artOwner,NSArray<NSString *> *artGetters) {
    UIImageView *glyph=objc_getAssociatedObject(owner,&HGAFinderAIInkKey);
    if(!HGAFinderControlReady(assistant))assistant=nil;
    BOOL replaced=NO;
    for(NSString *name in artGetters) {
        UIView *art=assistant ? HGAViewGetter(artOwner,name.UTF8String,assistant) : nil;
        if(art==assistant)art=nil;
        HGAHidePageChromeTarget(owner,name,art);
        replaced|=art!=nil;
    }
    if(!replaced) {
        [glyph removeFromSuperview];
        return;
    }
    if(!glyph) {
        UIImageSymbolConfiguration *config=[UIImageSymbolConfiguration configurationWithPointSize:17 weight:UIImageSymbolWeightSemibold];
        glyph=[[UIImageView alloc] initWithImage:[UIImage systemImageNamed:@"sparkles" withConfiguration:config]];
        glyph.tintColor=UIColor.labelColor;
        glyph.contentMode=UIViewContentModeCenter;
        glyph.userInteractionEnabled=NO;
        glyph.accessibilityElementsHidden=YES;
        glyph.accessibilityIdentifier=@"honggo.appearance.finder.ai";
        glyph.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
        objc_setAssociatedObject(owner,&HGAFinderAIInkKey,glyph,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    UIColor *aiTint=HGAAdaptivePageStyle(HGAOwner(owner),assistant ?: owner)==UIUserInterfaceStyleDark ?
        UIColor.whiteColor : UIColor.labelColor;
    if(![glyph.tintColor isEqual:aiTint])glyph.tintColor=aiTint;
    if(!CGRectEqualToRect(glyph.frame,assistant.bounds))glyph.frame=assistant.bounds;
    if(glyph.superview!=assistant)[assistant addSubview:glyph];
}

static BOOL HGAApplyHeaderChrome(UIView *header,BOOL enabled) {
    BOOL finder=HGAHeaderShowsFinder(header);
    UIView *search=HGAViewGetter(header,"searchViewContainer",header);
    BOOL home=enabled && !finder;
    UIView *sidebar=home && HGAppearanceFlag(@"layout.channels.hide.sidebar") ? HGAViewGetter(header,"sideBar",header) : nil;
    UIView *searchSlot=nil;
    if(home && HGAppearanceFlag(@"layout.channels.hide.search")) {
        // The container spans the whole header and is reused for the full search row.
        searchSlot=HGAViewGetter(search,"searchButtonSlotView",search) ?: HGAViewGetter(search,"searchButtonView",search);
        if(searchSlot==search)searchSlot=nil;
    }
    HGAHidePageChromeTarget(header,@"sidebar",sidebar);
    HGAHidePageChromeTarget(header,@"searchSlot",searchSlot);
    if(!enabled || !finder) {
        HGAApplyFinderSearch(header,nil,nil,nil,nil,nil,nil,nil,NO);
        HGAApplyPageChromeIcons(header,nil);
        if(objc_getAssociatedObject(header,&HGAFinderAIInkKey))
            HGAApplyFinderAIInk(header,nil,nil,@[@"lottieView",@"loadingLottieView"]);
        return finder;
    }
    UIView *row=HGAViewGetter(header,"findDramaSearchRowViewV731",header);
    UIView *searchOwner=HGAViewGetter(header,"searchViewContainer",row ?: header) ?: search;
    UIView *searchField=HGAViewGetter(searchOwner,"searchView",searchOwner);
    UIView *field=HGAViewGetter(searchOwner,"searchBgView",searchOwner) ?: searchField;
    UIView *recognition=HGAViewGetter(searchOwner,"imageSearchContainer",searchOwner);
    UIView *assistant=HGAViewGetter(header,"findDramaAIEntranceViewV731",header);
    UIView *divider=recognition ? HGAViewGetter(searchOwner,"imageSearchSeparatorView",recognition) : nil;
    BOOL glass=HGAppearanceFlag(@"player.finderGlass");
    UIView *alternate=HGAViewGetter(searchOwner,"searchButtonSlotView",searchOwner);
    HGAApplyFinderSearch(header,field,searchField,alternate,recognition,assistant,nil,divider,glass);
    NSMutableArray<UIView *> *icons=[NSMutableArray array];
    if(glass) {
        UIView *searchIcon=HGAViewGetter(searchOwner,"searchIcon",searchOwner);
        UIView *camera=recognition ? HGAViewGetter(searchOwner,"cameraIcon",recognition) : nil;
        if(searchIcon)[icons addObject:searchIcon];
        if(camera)[icons addObject:camera];
    }
    HGAApplyPageChromeIcons(header,icons);
    HGAApplyFinderAIInk(header,glass ? assistant : nil,assistant,@[@"lottieView",@"loadingLottieView"]);
    return finder;
}

static BOOL HGAIsFinderController(UIViewController *controller) {
    return [controller isKindOfClass:NSClassFromString(@"FQTheaterViewController")] ||
        [controller isKindOfClass:NSClassFromString(@"SSBookStoreNewViewController")];
}

static BOOL HGAIsFinderView(UIView *view) {
    for(UIViewController *owner=HGAOwner(view);owner;owner=owner.parentViewController)
        if(HGAIsFinderController(owner))return YES;
    // Recycled catalogue content can have a nearer detached list controller.
    // Walk its actual view ancestry as well as the controller containment.
    for(UIView *parent=view.superview;parent;parent=parent.superview) {
        UIResponder *responder=parent.nextResponder;
        if([responder isKindOfClass:UIViewController.class] && HGAIsFinderController((id)responder))return YES;
    }
    return NO;
}

static void HGAFinderCategoryDidAppear(UIView *view) {
    for(UIViewController *owner=HGAOwner(view);owner;owner=owner.parentViewController) {
        if(!HGAIsFinderController(owner))continue;
        HGAppearanceProcessController(owner);
        break;
    }
}

static char HGAFinderShortcutIndexesKey;
static IMP HGAOriginalGoldenPositionSize;

static CGSize HGAFinderShortcutSize(id cell,SEL cmd,UICollectionView *collection,UICollectionViewLayout *layout,NSIndexPath *path) {
    CGSize size=((CGSize (*)(id,SEL,id,id,id))HGAOriginalGoldenPositionSize)(cell,cmd,collection,layout,path);
    NSIndexSet *hidden=objc_getAssociatedObject(cell,&HGAFinderShortcutIndexesKey);
    if(path.section==0 && [hidden containsIndex:path.item] && collection==HGReadObjectGetter(cell,"collectionView"))
        size.width=0.01;
    return size;
}

static IMP HGAOriginalFinderRowHeight,HGAOriginalFinderSectionInset,HGAOriginalFinderHeaderHeight,HGAOriginalHideFinderSearchBar;
static IMP HGAOriginalTheaterHeaderBgLayout;
static NSHashTable<UICollectionView *> *HGAFinderLists;
static NSUInteger HGAFinderLayoutMask;
static void HGAReconcileTheaterHeaderBackdropForView(UIView *view);

static void HGARefreshFinderRowLayout(void) {
    NSUInteger mask=0;
    if (HGAppearanceEnabled()) {
        NSArray *keys=@[@"filter",@"ranking",@"new",@"reserve"];
        for (NSUInteger i=0;i<keys.count;i++)
            if (HGAppearanceFlag([@"finder.hide.shortcut." stringByAppendingString:keys[i]])) mask|=1u<<i;
        if (HGAppearanceFlag(@"finder.hide.category.row")) mask|=1u<<4;
    }
    if (mask==HGAFinderLayoutMask) return;
    HGAFinderLayoutMask=mask;
    for (UICollectionView *list in HGAFinderLists) [list.collectionViewLayout invalidateLayout];
}

static BOOL HGAFinderRowFullyHidden(id model) {
    if (![model isKindOfClass:objc_lookUpClass("SSVideoDistributionGoldenPositionModel")]) return NO;
    NSArray *items=HGReadObjectGetter(model,"goldenLineItemArr");
    if (![items isKindOfClass:NSArray.class] || !items.count) return NO;
    for (id item in items) {
        NSString *key=HGAFinderShortcutKey(HGReadObjectGetter(item,"text"));
        if (!key || !HGAppearanceFlag([@"finder.hide.shortcut." stringByAppendingString:key])) return NO;
    }
    return YES;
}

static NSArray *HGAFinderSectionCells(UIView *list,NSInteger section) {
    id source=HGReadObjectGetter(list,"dataSource");
    NSArray *sections=HGReadObjectGetter(source,"sectionModels");
    if (![sections isKindOfClass:NSArray.class] || (NSUInteger)section>=sections.count) return nil;
    NSArray *cells=HGReadObjectGetter(sections[section],"cellModels");
    return [cells isKindOfClass:NSArray.class] ? cells : nil;
}

static CGFloat HGAFinderRowHeight(UIView *list,SEL cmd,UICollectionView *collection,id layout,NSIndexPath *path,CGFloat width) {
    CGFloat native=((CGFloat (*)(id,SEL,id,id,id,CGFloat))HGAOriginalFinderRowHeight)(list,cmd,collection,layout,path,width);
    if (!(HGAFinderLayoutMask & 15)) return native;
    NSArray *models=HGAFinderSectionCells(list,path.section);
    if ((NSUInteger)path.item>=models.count ||
        ![models[path.item] isKindOfClass:objc_lookUpClass("SSVideoDistributionGoldenPositionModel")] ||
        !HGAIsFinderView(list)) return native;
    [HGAFinderLists addObject:collection];
    return HGAFinderRowFullyHidden(models[path.item]) ? 0 : native;
}

static UIEdgeInsets HGAFinderSectionInset(UIView *list,SEL cmd,UICollectionView *collection,id layout,NSInteger section) {
    UIEdgeInsets inset=((UIEdgeInsets (*)(id,SEL,id,id,NSInteger))HGAOriginalFinderSectionInset)(list,cmd,collection,layout,section);
    if (!(HGAFinderLayoutMask & 15) || (!inset.top && !inset.bottom)) return inset;
    NSArray *models=HGAFinderSectionCells(list,section);
    if (!models.count) return inset;
    for (id model in models) if (!HGAFinderRowFullyHidden(model)) return inset;
    if (HGAIsFinderView(list)) { inset.top=0; inset.bottom=0; }
    return inset;
}

static CGFloat HGAFinderHeaderHeight(id coordinator,SEL cmd,BOOL visible) {
    if (visible && HGAppearanceEnabled() && HGAppearanceFlag(@"finder.hide.category.row")) visible=NO;
    return ((CGFloat (*)(id,SEL,BOOL))HGAOriginalFinderHeaderHeight)(coordinator,cmd,visible);
}

static BOOL HGAFinderSearchRowHidden(void) {
    return HGAppearanceEnabled() && !HGAppearanceFlag(@"player.finderSearch") &&
        !HGAppearanceFlag(@"player.finderRecognition") && !HGAppearanceFlag(@"player.finderAI");
}

static BOOL HGAHideFinderSearchBar(id coordinator,SEL cmd) {
    return HGAFinderSearchRowHidden() || ((BOOL (*)(id,SEL))HGAOriginalHideFinderSearchBar)(coordinator,cmd);
}

static void HGATheaterHeaderBgLayout(UIView *bgView,SEL cmd) {
    ((void (*)(id,SEL))HGAOriginalTheaterHeaderBgLayout)(bgView,cmd);
    if(bgView.window)HGAReconcileTheaterHeaderBackdropForView(bgView);
}

static void HGAInstallFinderShortcutHook(void) {
    if(HGAOriginalGoldenPositionSize)return;
    HGAFinderLists=[NSHashTable weakObjectsHashTable];
    HGARefreshFinderRowLayout();
    Class list=objc_lookUpClass("FQFeedListView");
    HGAOriginalFinderRowHeight=HGInstallVerifiedHook(list,sel_registerName("heightForRowAtIndexPathWithCollectionView:layout:indexPath:itemWidth:"),
        "d48@0:8@16@24@32d40",(IMP)HGAFinderRowHeight,@"appearance");
    HGAOriginalFinderSectionInset=HGInstallVerifiedHook(list,sel_registerName("insetForSectionWithCollectionView:layout:section:"),
        "{UIEdgeInsets=dddd}40@0:8@16@24q32",(IMP)HGAFinderSectionInset,@"appearance");
    HGAOriginalFinderHeaderHeight=HGInstallVerifiedHook(objc_lookUpClass("FQTheaterLayoutCoordinator"),
        sel_registerName("_calculateHeaderHeightWithTabContainerVisible:"),"d20@0:8B16",(IMP)HGAFinderHeaderHeight,@"appearance");
    HGAOriginalHideFinderSearchBar=HGInstallVerifiedHook(objc_lookUpClass("FQTheaterLayoutCoordinator"),
        sel_registerName("shouldHideSearchBar"),"B16@0:8",(IMP)HGAHideFinderSearchBar,@"appearance");
    Class headerBg=objc_lookUpClass("FQTheaterHeaderBgView");
    if([headerBg isSubclassOfClass:UIView.class] && !HGAOriginalTheaterHeaderBgLayout)
        HGAOriginalTheaterHeaderBgLayout=HGInstallVerifiedHook(headerBg,@selector(layoutSubviews),"v16@0:8",(IMP)HGATheaterHeaderBgLayout,@"appearance");
    Class type=NSClassFromString(@"SSVideoDistributionGoldenPositionCell");
    SEL selector=@selector(collectionView:layout:sizeForItemAtIndexPath:);
    Method method=class_getInstanceMethod(type,selector);
    const char *encoding="{CGSize=dd}40@0:8@16@24@32";
    if(!method || strcmp(method_getTypeEncoding(method),encoding))return;
    HGAOriginalGoldenPositionSize=method_getImplementation(method);
    HGAOriginalGoldenPositionSize=HGInstallVerifiedHook(type,selector,encoding,(IMP)HGAFinderShortcutSize,@"appearance");
}

static void HGAUpdateFinderShortcuts(UIView *row,BOOL enabled) {
    UICollectionView *collection=(id)HGAViewGetter(row,"collectionView",row);
    if(![collection isKindOfClass:UICollectionView.class])return;
    NSMutableIndexSet *hidden=[NSMutableIndexSet indexSet];
    if(enabled && HGAIsFinderView(row)) {
        UICollectionView *outer=(id)HGAAncestorNamed(row,@"FQFeedListCollectionView");
        if (outer && ![HGAFinderLists containsObject:outer]) {
            [HGAFinderLists addObject:outer];
            if (HGAFinderLayoutMask & 15) [outer.collectionViewLayout invalidateLayout];
        }
        NSArray *items=HGReadObjectGetter(HGReadObjectGetter(row,"model"),"goldenLineItemArr");
        if([items isKindOfClass:NSArray.class] && items.count<=64) {
            for(NSUInteger index=0;index<items.count;index++) {
                NSString *key=HGAFinderShortcutKey(HGReadObjectGetter(items[index],"text"));
                if(key && HGAppearanceFlag([@"finder.hide.shortcut." stringByAppendingString:key]))[hidden addIndex:index];
            }
        }
    }
    NSIndexSet *previous=objc_getAssociatedObject(row,&HGAFinderShortcutIndexesKey);
    if((previous.count || hidden.count) && ![previous isEqual:hidden]) {
        objc_setAssociatedObject(row,&HGAFinderShortcutIndexesKey,hidden.count?hidden.copy:nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        [collection.collectionViewLayout invalidateLayout];
    }
}

static void HGACenterFinderShortcut(UIView *cell,UIView *inner,UIView *content,BOOL enabled) {
    if(!inner || inner==content || inner==cell)return;
    UIView *icon=enabled ? HGAViewGetter(cell,"iconImgView",inner) : nil;
    UIView *label=enabled ? HGAViewGetter(cell,"titleLabel",inner) : nil;
    if(!HGAFinderControlReady(icon) || !HGAFinderControlReady(label)) {
        HGARestoreGeometry(inner);
        return;
    }
    CGRect ink=CGRectUnion([icon convertRect:icon.bounds toView:content],[label convertRect:label.bounds toView:content]);
    HGAViewState *state=HGAState(inner,NO);
    if(state.geometry && state.geometrySuperview==inner.superview &&
        CGAffineTransformEqualToTransform(inner.transform,state.appliedTransform))
        ink.origin.x-=state.appliedTransform.tx-state.sourceTransform.tx;
    CGFloat space=CGRectGetWidth(content.bounds)-CGRectGetWidth(ink);
    HGASetGeometry(inner,space>0 ? CGRectGetMidX(content.bounds)-CGRectGetMidX(ink) : 0,0,1);
}

static BOOL HGAIsFinderChannel(UIView *view) {
    if(HGAAncestorNamed(view,@"FQTheaterTabContainerView"))return YES;
    for(UIViewController *owner=HGAOwner(view);owner;owner=owner.parentViewController) {
        if(!HGAIsFinderController(owner))continue;
        UIView *bar=HGAViewGetter(owner,"tabContainer",owner.viewIfLoaded);
        return view==bar || [view isDescendantOfView:bar];
    }
    return NO;
}

static BOOL HGAFinderCategoryHidden(NSString *title) {
    NSString *key=HGAFinderCategoryKey(title);
    return key && HGAppearanceFlag([@"finder.hide.category." stringByAppendingString:key]);
}

static NSUInteger HGAFinderHiddenMask(NSArray *titles) {
    NSUInteger mask=0;
    if(!HGAppearanceEnabled() || ![titles isKindOfClass:NSArray.class] || titles.count>32)return mask;
    for(NSUInteger index=0;index<titles.count;index++)
        if(HGAFinderCategoryHidden(titles[index]))mask|=1UL<<index;
    return mask;
}

static IMP HGAOriginalMineFunctionItems,HGAOriginalMineFunctionSnapshot;
static IMP HGAOriginalMineSidebarSnapshot;
static IMP HGAOriginalMineSkinSwitch,HGAOriginalMinePublishUpdate;
static char HGAMineLaunchSkinKey;
static NSHashTable *HGAMinePublishOwners;
static BOOL HGAMinePublishHidden;
static char HGAMineEntryStateKey,HGAMineMenuButtonKey;
static _Thread_local BOOL HGAMineSnapshotInFlight;
static BOOL HGAMineFilterInstalled;

static BOOL HGAMineHidesMore(void) {
    return HGAppearanceFlag(@"mine.hide.shortcut.row") || HGAppearanceFlag(@"mine.hide.shortcut.more");
}

// 7.4.5.33 copies this into the Compose launch parameters when creating My.
static BOOL HGAMineShowsSkinSwitch(id page,SEL cmd) {
    BOOL visible=((BOOL (*)(id,SEL))HGAOriginalMineSkinSwitch)(page,cmd) && HGAppearanceFlag(@"mine.show.theme");
    objc_setAssociatedObject(page,&HGAMineLaunchSkinKey,@(visible),OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    return visible;
}

static void HGAMineUpdatePublish(id owner,SEL cmd) {
    ((void (*)(id,SEL))HGAOriginalMinePublishUpdate)(owner,cmd);
    if(!HGAMinePublishOwners)HGAMinePublishOwners=[NSHashTable weakObjectsHashTable];
    [HGAMinePublishOwners addObject:owner];
    if(!HGAppearanceFlag(@"mine.show.publish")) {
        UIView *host=HGReadObjectGetter(owner,"hostView");
        UIView *button=HGAViewGetter(owner,"publishButton",host);
        button.hidden=YES;
    }
}

static void HGARefreshMinePublishVisibility(void) {
    BOOL hide=!HGAppearanceFlag(@"mine.show.publish");
    if(HGAMinePublishHidden==hide)return;
    HGAMinePublishHidden=hide;
    for(id owner in HGAMinePublishOwners.allObjects) {
        BOOL invalidated=NO;
        HGABoolGetter(owner,@"invalidated",&invalidated);
        if(!invalidated)HGAMineUpdatePublish(owner,sel_registerName("updateVisibility"));
    }
}

// MoreSelectionItem is appended by Compose, outside functionItems. The same
// immutable Kotlin state also supplies the profile title bar's sidebar entry.
static id HGAMineSidebarSnapshot(UIViewController *owner,SEL cmd,id depend) {
    id original=((id (*)(id,SEL,id))HGAOriginalMineSidebarSnapshot)(owner,cmd,depend);
    if(!HGAMineHidesMore() || depend!=HGReadObjectGetter(owner,"minePageKMPDepend"))return original;
    SEL copy=sel_registerName("doCopyIsVisible:showRedDot:");
    SEL visible=sel_registerName("isVisible"), dot=sel_registerName("showRedDot");
    Class cls=object_getClass(original);
    Method clone=class_getInstanceMethod(cls,copy), v=class_getInstanceMethod(cls,visible), d=class_getInstanceMethod(cls,dot);
    if(!clone || !v || !d || strcmp(method_getTypeEncoding(clone),"@18@0:8c16c17") ||
       strcmp(method_getTypeEncoding(v),"c16@0:8") || strcmp(method_getTypeEncoding(d),"c16@0:8"))return original;
    BOOL shown=((signed char (*)(id,SEL))objc_msgSend)(original,visible);
    BOOL red=shown && ((signed char (*)(id,SEL))objc_msgSend)(original,dot);
    id filtered=shown ? ((id (*)(id,SEL,signed char,signed char))objc_msgSend)(original,copy,0,0) : original;
    if(!filtered)return original;
    NSNumber *value=@(shown ? (red ? 2 : 1) : 0);
    NSNumber *previous=objc_getAssociatedObject(owner,&HGAMineEntryStateKey);
    if(![previous isEqual:value]) {
        objc_setAssociatedObject(owner,&HGAMineEntryStateKey,value,OBJC_ASSOCIATION_RETAIN);
        dispatch_async(dispatch_get_main_queue(),^{ [owner.viewIfLoaded setNeedsLayout]; });
    }
    return filtered;
}

static NSArray *HGAMineFunctionItems(id owner,SEL cmd) {
    NSArray *items=((id (*)(id,SEL))HGAOriginalMineFunctionItems)(owner,cmd);
    if(!HGAMineSnapshotInFlight || ![items isKindOfClass:NSArray.class])return items;
    if(HGAppearanceFlag(@"mine.hide.shortcut.row"))return @[];
    NSMutableArray *visible=[NSMutableArray arrayWithCapacity:items.count];
    for(id item in items) {
        NSString *key=HGAMineShortcutKey(HGReadObjectGetter(item,"title"));
        if(!key || !HGAppearanceFlag([@"mine.hide.shortcut." stringByAppendingString:key]))[visible addObject:item];
    }
    return visible.count==items.count ? items : visible;
}

static id HGAMineFunctionSnapshot(id owner,SEL cmd) {
    BOOL previous=HGAMineSnapshotInFlight;
    HGAMineSnapshotInFlight=YES;
    @try {
        return ((id (*)(id,SEL))HGAOriginalMineFunctionSnapshot)(owner,cmd);
    } @finally {
        HGAMineSnapshotInFlight=previous;
    }
}

// Compose owns the rows and their actions. Its visibility bridge accepts the
// same stable cell types used for navigation, independent of display order.
typedef void (*HGAMineDrawerVisibility)(id,SEL,NSInteger,BOOL);
static HGAMineDrawerVisibility HGAMineDrawerSetVisible;
static IMP HGAMineDrawerAttach;
static NSHashTable *HGAMineDrawerOwners;
static char HGAMineDrawerApplied, HGAMineDrawerHostFlags;

static void HGAApplyMineDrawerPreferences(id depend) {
    NSMutableSet *wanted=[NSMutableSet set];
    NSDictionary *prefs=HGAppearanceValues();
    for(NSDictionary *entry in HGMineDrawerEntries())
        if([prefs[entry[@"key"]] boolValue])[wanted addObjectsFromArray:entry[@"types"]];
    NSSet *applied=objc_getAssociatedObject(depend,&HGAMineDrawerApplied);
    if(applied ? [wanted isEqualToSet:applied] : !wanted.count)return;
    objc_setAssociatedObject(depend,&HGAMineDrawerApplied,wanted,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    NSDictionary *hostFlags=objc_getAssociatedObject(depend,&HGAMineDrawerHostFlags);
    SEL setVisible=sel_registerName("setSidebarItemVisibleForCellType:visible:");
    for(NSNumber *type in applied)
        if(![wanted containsObject:type])
            HGAMineDrawerSetVisible(depend,setVisible,type.integerValue,
                hostFlags[type] ? [hostFlags[type] boolValue] : YES);
    for(NSNumber *type in wanted)
        if(![applied containsObject:type])HGAMineDrawerSetVisible(depend,setVisible,type.integerValue,NO);
}

static void HGAMineDrawerVisibilityRequested(id depend,SEL cmd,NSInteger type,BOOL visible) {
    // Preserve the latest native decision, e.g. the scanner can be disabled
    // by host configuration even while its Honggo switch is off.
    NSMutableDictionary *host=objc_getAssociatedObject(depend,&HGAMineDrawerHostFlags);
    if(!host) {
        host=[NSMutableDictionary dictionary];
        objc_setAssociatedObject(depend,&HGAMineDrawerHostFlags,host,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    host[@(type)]=@(visible);
    NSSet *hidden=objc_getAssociatedObject(depend,&HGAMineDrawerApplied);
    HGAMineDrawerSetVisible(depend,cmd,type,visible && ![hidden containsObject:@(type)]);
}

static void HGAMineDrawerHandleAttached(id depend,SEL cmd,id handle) {
    id previous=HGReadObjectGetter(depend,"controllerHandle");
    ((void (*)(id,SEL,id))HGAMineDrawerAttach)(depend,cmd,handle);
    if(previous==handle)return;
    objc_setAssociatedObject(depend,&HGAMineDrawerApplied,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    objc_setAssociatedObject(depend,&HGAMineDrawerHostFlags,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    if(!handle){[HGAMineDrawerOwners removeObject:depend];return;}
    [HGAMineDrawerOwners addObject:depend];
    HGAApplyMineDrawerPreferences(depend);
}

static void HGAInstallMineShortcutHooks(void) {
    Class depend=objc_lookUpClass("SSMinePageKMPDepend");
    if(!HGAMineDrawerSetVisible)
        HGAMineDrawerSetVisible=(HGAMineDrawerVisibility)HGInstallVerifiedHook(depend,
            sel_registerName("setSidebarItemVisibleForCellType:visible:"),"v28@0:8q16B24",
            (IMP)HGAMineDrawerVisibilityRequested,@"appearance");
    if(HGAMineDrawerSetVisible && !HGAMineDrawerAttach) {
        HGAMineDrawerOwners=[NSHashTable weakObjectsHashTable];
        HGAMineDrawerAttach=HGInstallVerifiedHook(depend,sel_registerName("attachControllerHandle:"),
            "v24@0:8@16",(IMP)HGAMineDrawerHandleAttached,@"appearance");
    }
    if(!HGAOriginalMineSkinSwitch)
        HGAOriginalMineSkinSwitch=HGInstallVerifiedHook(objc_lookUpClass("SSMine737KMPViewController"),
            sel_registerName("shouldShowSkinSwitch"),"B16@0:8",(IMP)HGAMineShowsSkinSwitch,@"appearance");
    if(!HGAOriginalMinePublishUpdate) {
        HGAMinePublishHidden=!HGAppearanceFlag(@"mine.show.publish");
        HGAOriginalMinePublishUpdate=HGInstallVerifiedHook(objc_lookUpClass("SSMineWorksKMPPublishEntranceController"),
            sel_registerName("updateVisibility"),"v16@0:8",(IMP)HGAMineUpdatePublish,@"appearance");
    }
    if(!HGAOriginalMineSidebarSnapshot) {
        Class page=objc_lookUpClass("SSMine737KMPViewController");
        Method open=class_getInstanceMethod(page,sel_registerName("minePageKMPDepend:setSidebarOpen:"));
        if(open && !strcmp(method_getTypeEncoding(open),"v28@0:8@16B24"))
            HGAOriginalMineSidebarSnapshot=HGInstallVerifiedHook(page,sel_registerName("minePageKMPSidebarEntrySnapshot:"),
                "@24@0:8@16",(IMP)HGAMineSidebarSnapshot,@"appearance");
    }
    if(HGAMineFilterInstalled)return;
    Class type=NSClassFromString(@"SSMinePageKMPFunctionDepend");
    SEL source=sel_registerName("functionItems"), snapshot=sel_registerName("getFunctionSnapshot");
    Method items=class_getInstanceMethod(type,source), publisher=class_getInstanceMethod(type,snapshot);
    if(!items || !publisher || strcmp(method_getTypeEncoding(items),"@16@0:8") ||
        strcmp(method_getTypeEncoding(publisher),"@16@0:8"))return;
    if(!HGAOriginalMineFunctionItems)HGAOriginalMineFunctionItems=method_getImplementation(items);
    if(!HGAOriginalMineFunctionSnapshot)HGAOriginalMineFunctionSnapshot=method_getImplementation(publisher);
    IMP original=HGInstallVerifiedHook(type,source,"@16@0:8",(IMP)HGAMineFunctionItems,@"appearance");
    if(!original)return;
    HGAOriginalMineFunctionItems=original;
    original=HGInstallVerifiedHook(type,snapshot,"@16@0:8",(IMP)HGAMineFunctionSnapshot,@"appearance");
    if(!original)return;
    HGAOriginalMineFunctionSnapshot=original;
    HGAMineFilterInstalled=YES;
}

static BOOL HGAIsMineKMPController(UIViewController *controller) {
    return [controller isKindOfClass:NSClassFromString(@"SSMine737KMPViewController")];
}

// Host 7.4.5.33 DanceUICompose.ComposeRenderingUIViewImpl keeps its 120Hz
// DisplayLinkScheduler unpaused whenever wantsContinuousFrames or
// isNeedHighFrequencyPolling is true and isApplicationActive == YES, even
// after Mine's view detaches from UIWindow (didMoveToWindow returns early on
// window == nil). Quiesce only Mine-owned Compose renderers while Mine is
// offscreen, and restore them immediately on re-attachment or viewWillAppear:.
static char HGAMineDisappearedKey,HGAMineQuiescedRenderersKey,HGAMineRendererOwnerKey;
static char HGAMineQuiescedMaskKey,HGAMineHostActiveKey;
static IMP HGAMineComposeDidMoveToWindow,HGAMineComposeSetActive;
static _Thread_local BOOL HGAMineRestoringComposeActive;

static Class HGAMineComposeRendererClass(void) {
    return objc_lookUpClass("_TtC14DanceUICompose26ComposeRenderingUIViewImpl") ?:
        objc_lookUpClass("DanceUICompose.ComposeRenderingUIViewImpl");
}

static BOOL HGAMineMethodMatches(id target,SEL selector,const char *encoding) {
    if(!target || !selector || !encoding)return NO;
    Method m=class_getInstanceMethod(object_getClass(target),selector);
    const char *actual=m ? method_getTypeEncoding(m) : NULL;
    return actual && !strcmp(actual,encoding);
}

@interface HGAMineComposeOwnerBox : NSObject
@property(nonatomic,weak) UIViewController *owner;
@end
@implementation HGAMineComposeOwnerBox @end

static UIViewController *HGAMineOwnerForComposeRenderer(UIView *renderer) {
    HGAMineComposeOwnerBox *box=objc_getAssociatedObject(renderer,&HGAMineRendererOwnerKey);
    if(HGAIsMineKMPController(box.owner))return box.owner;
    Class mineCls=NSClassFromString(@"SSMine737KMPViewController");
    if(!mineCls)return nil;
    for(UIView *node=renderer.superview;node;node=node.superview) {
        UIResponder *resp=node.nextResponder;
        if([resp isKindOfClass:mineCls]) {
            if(!box) {
                box=[HGAMineComposeOwnerBox new];
                objc_setAssociatedObject(renderer,&HGAMineRendererOwnerKey,box,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            }
            box.owner=(UIViewController *)resp;
            return box.owner;
        }
    }
    return nil;
}

static NSArray<UIView *> *HGAMineCollectComposeRenderers(UIViewController *controller) {
    Class composeCls=HGAMineComposeRendererClass();
    if(!composeCls || !HGAIsMineKMPController(controller) || !controller.isViewLoaded)return @[];
    UIView *root=controller.viewIfLoaded;
    NSMutableArray<UIView *> *roots=[NSMutableArray array];
    if(root)[roots addObject:root];
    id kmpHandle=HGReadObjectGetter(controller,"minePageKMPHandle");
    const char *getters[]={"contentView","titleBarView","migrationGuideView","gameCenterPromptView"};
    for(NSUInteger i=0;i<sizeof(getters)/sizeof(getters[0]);i++) {
        UIView *surface=HGAViewGetter(kmpHandle,getters[i],nil);
        if([surface isKindOfClass:UIView.class] && ![roots containsObject:surface])[roots addObject:surface];
    }
    NSMutableArray<UIView *> *found=[NSMutableArray array];
    for(UIView *branch in roots) {
        if([branch isKindOfClass:composeCls] && ![found containsObject:branch])[found addObject:branch];
        for(UIView *node in HGADescendants(branch,64))
            if([node isKindOfClass:composeCls] && ![found containsObject:node])[found addObject:node];
    }
    for(UIView *renderer in found) {
        HGAMineComposeOwnerBox *box=objc_getAssociatedObject(renderer,&HGAMineRendererOwnerKey);
        if(!box) {
            box=[HGAMineComposeOwnerBox new];
            objc_setAssociatedObject(renderer,&HGAMineRendererOwnerKey,box,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        box.owner=controller;
    }
    return found;
}

static void HGAUpdateMineComposeQuiescence(UIViewController *controller) {
    if(!NSThread.isMainThread || !HGAIsMineKMPController(controller) || !controller.isViewLoaded)return;
    UIView *root=controller.viewIfLoaded;
    BOOL disappeared=[objc_getAssociatedObject(controller,&HGAMineDisappearedKey) boolValue];
    BOOL shouldQuiesce=disappeared || !root.window || root.hidden;
    SEL getDisabled=sel_registerName("disableRendering"),setDisabled=sel_registerName("setDisableRendering:");
    SEL getActive=sel_registerName("isApplicationActive"),setActive=sel_registerName("setIsApplicationActive:");
    SEL redraw=sel_registerName("needRedraw");
    NSHashTable<UIView *> *quiesced=objc_getAssociatedObject(controller,&HGAMineQuiescedRenderersKey);
    NSUInteger changed=0;
    if(shouldQuiesce) {
        NSArray<UIView *> *renderers=HGAMineCollectComposeRenderers(controller);
        for(UIView *renderer in renderers) {
            NSUInteger mask=[objc_getAssociatedObject(renderer,&HGAMineQuiescedMaskKey) unsignedIntegerValue];
            NSUInteger priorMask=mask;
            if(!(mask & 1u) && HGAMineMethodMatches(renderer,getDisabled,"B16@0:8") &&
               HGAMineMethodMatches(renderer,setDisabled,"v20@0:8B16") &&
               !((BOOL (*)(id,SEL))objc_msgSend)(renderer,getDisabled)) {
                ((void (*)(id,SEL,BOOL))objc_msgSend)(renderer,setDisabled,YES);
                mask|=1u;
            }
            if(!(mask & 2u) && HGAMineMethodMatches(renderer,getActive,"B16@0:8") &&
               HGAMineMethodMatches(renderer,setActive,"v20@0:8B16") &&
               ((BOOL (*)(id,SEL))objc_msgSend)(renderer,getActive)) {
                objc_setAssociatedObject(renderer,&HGAMineHostActiveKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                mask|=2u;
                objc_setAssociatedObject(renderer,&HGAMineQuiescedMaskKey,@(mask),OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                BOOL prevRestore=HGAMineRestoringComposeActive;
                HGAMineRestoringComposeActive=YES;
                @try {
                    if(HGAMineComposeSetActive)((void (*)(id,SEL,BOOL))HGAMineComposeSetActive)(renderer,setActive,NO);
                    else ((void (*)(id,SEL,BOOL))objc_msgSend)(renderer,setActive,NO);
                } @finally {
                    HGAMineRestoringComposeActive=prevRestore;
                }
            } else if(mask!=priorMask) {
                objc_setAssociatedObject(renderer,&HGAMineQuiescedMaskKey,@(mask),OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            }
            if(mask) {
                if(!quiesced) {
                    quiesced=[NSHashTable weakObjectsHashTable];
                    objc_setAssociatedObject(controller,&HGAMineQuiescedRenderersKey,quiesced,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                }
                [quiesced addObject:renderer];
                if(mask!=priorMask)changed++;
            }
        }
    } else if(quiesced.count) {
        BOOL appActive=UIApplication.sharedApplication.applicationState!=UIApplicationStateBackground;
        for(UIView *renderer in quiesced.allObjects) {
            NSUInteger mask=[objc_getAssociatedObject(renderer,&HGAMineQuiescedMaskKey) unsignedIntegerValue];
            if(!mask)continue;
            NSNumber *hostWanted=objc_getAssociatedObject(renderer,&HGAMineHostActiveKey);
            objc_setAssociatedObject(renderer,&HGAMineQuiescedMaskKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            objc_setAssociatedObject(renderer,&HGAMineHostActiveKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            if((mask & 1u) && HGAMineMethodMatches(renderer,setDisabled,"v20@0:8B16"))
                ((void (*)(id,SEL,BOOL))objc_msgSend)(renderer,setDisabled,NO);
            if((mask & 2u) && appActive && (!hostWanted || hostWanted.boolValue) &&
               HGAMineMethodMatches(renderer,setActive,"v20@0:8B16")) {
                BOOL prevRestore=HGAMineRestoringComposeActive;
                HGAMineRestoringComposeActive=YES;
                @try {
                    if(HGAMineComposeSetActive)((void (*)(id,SEL,BOOL))HGAMineComposeSetActive)(renderer,setActive,YES);
                    else ((void (*)(id,SEL,BOOL))objc_msgSend)(renderer,setActive,YES);
                } @finally {
                    HGAMineRestoringComposeActive=prevRestore;
                }
            }
            if(appActive && renderer.window && HGAMineMethodMatches(renderer,redraw,"v16@0:8"))
                ((void (*)(id,SEL))objc_msgSend)(renderer,redraw);
            changed++;
        }
        [quiesced removeAllObjects];
    } else {
        HGAMineCollectComposeRenderers(controller);
    }
    static _Atomic uint64_t quiesceSample;
    if(changed && HGEnhancedDiagnostics() && HGDiagnosticSample(&quiesceSample))
        HGLog(@"appearance.mine-quiesce",@{@"quiesced":@(shouldQuiesce),@"renderers":@(changed)});
}

static void HGAMineSetControllerDisappeared(UIViewController *controller,BOOL disappeared) {
    if(!HGAIsMineKMPController(controller))return;
    objc_setAssociatedObject(controller,&HGAMineDisappearedKey,@(disappeared),OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    if(!disappeared) {
        // Resume immediately on viewWillAppear: even before root.window attaches,
        // so the incoming Mine tab renders frame 0 without waiting for layout.
        NSHashTable<UIView *> *quiesced=objc_getAssociatedObject(controller,&HGAMineQuiescedRenderersKey);
        if(quiesced.count) {
            BOOL appActive=UIApplication.sharedApplication.applicationState!=UIApplicationStateBackground;
            SEL setDisabled=sel_registerName("setDisableRendering:");
            SEL setActive=sel_registerName("setIsApplicationActive:");
            SEL redraw=sel_registerName("needRedraw");
            for(UIView *renderer in quiesced.allObjects) {
                NSUInteger mask=[objc_getAssociatedObject(renderer,&HGAMineQuiescedMaskKey) unsignedIntegerValue];
                if(!mask)continue;
                NSNumber *hostWanted=objc_getAssociatedObject(renderer,&HGAMineHostActiveKey);
                objc_setAssociatedObject(renderer,&HGAMineQuiescedMaskKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                objc_setAssociatedObject(renderer,&HGAMineHostActiveKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                if((mask & 1u) && HGAMineMethodMatches(renderer,setDisabled,"v20@0:8B16"))
                    ((void (*)(id,SEL,BOOL))objc_msgSend)(renderer,setDisabled,NO);
                if((mask & 2u) && appActive && (!hostWanted || hostWanted.boolValue) &&
                   HGAMineMethodMatches(renderer,setActive,"v20@0:8B16")) {
                    BOOL prevRestore=HGAMineRestoringComposeActive;
                    HGAMineRestoringComposeActive=YES;
                    @try {
                        if(HGAMineComposeSetActive)((void (*)(id,SEL,BOOL))HGAMineComposeSetActive)(renderer,setActive,YES);
                        else ((void (*)(id,SEL,BOOL))objc_msgSend)(renderer,setActive,YES);
                    } @finally {
                        HGAMineRestoringComposeActive=prevRestore;
                    }
                }
                if(appActive && HGAMineMethodMatches(renderer,redraw,"v16@0:8"))
                    ((void (*)(id,SEL))objc_msgSend)(renderer,redraw);
            }
            [quiesced removeAllObjects];
        }
        return;
    }
    HGAUpdateMineComposeQuiescence(controller);
}

static void HGAMineComposeWindowChanged(UIView *renderer,SEL cmd) {
    ((void (*)(id,SEL))HGAMineComposeDidMoveToWindow)(renderer,cmd);
    if(!NSThread.isMainThread)return;
    UIViewController *owner=HGAMineOwnerForComposeRenderer(renderer);
    if(owner)HGAUpdateMineComposeQuiescence(owner);
}

static void HGAMineComposeActiveChanged(UIView *renderer,SEL cmd,BOOL active) {
    if(!HGAMineRestoringComposeActive && NSThread.isMainThread) {
        NSUInteger mask=[objc_getAssociatedObject(renderer,&HGAMineQuiescedMaskKey) unsignedIntegerValue];
        UIViewController *owner=HGAMineOwnerForComposeRenderer(renderer);
        if((mask & 2u) && owner) {
            UIView *root=owner.viewIfLoaded;
            BOOL offscreen=[objc_getAssociatedObject(owner,&HGAMineDisappearedKey) boolValue] || !root.window || root.hidden;
            if(offscreen) {
                objc_setAssociatedObject(renderer,&HGAMineHostActiveKey,@(active),OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                ((void (*)(id,SEL,BOOL))HGAMineComposeSetActive)(renderer,cmd,NO);
                return;
            }
        }
    }
    ((void (*)(id,SEL,BOOL))HGAMineComposeSetActive)(renderer,cmd,active);
}

static void HGAInstallMineOffscreenQuiescence(void) {
    Class composeCls=HGAMineComposeRendererClass();
    if(!composeCls)return;
    if(!HGAMineComposeDidMoveToWindow && class_getInstanceMethod(composeCls,@selector(didMoveToWindow)))
        HGAMineComposeDidMoveToWindow=HGInstallVerifiedHook(composeCls,@selector(didMoveToWindow),"v16@0:8",
            (IMP)HGAMineComposeWindowChanged,@"appearance");
    SEL setActive=sel_registerName("setIsApplicationActive:");
    if(!HGAMineComposeSetActive && class_getInstanceMethod(composeCls,setActive))
        HGAMineComposeSetActive=HGInstallVerifiedHook(composeCls,setActive,"v20@0:8B16",
            (IMP)HGAMineComposeActiveChanged,@"appearance");
}

@interface HGAMineTitleWatch : NSObject
@property(nonatomic,weak) UIViewController *page;
@property(nonatomic,weak) UIView *renderer;
@property(nonatomic) BOOL queued;
@end
@implementation HGAMineTitleWatch @end
static char HGAMineTitleWatchKey,HGAMineProfileWatchKey;
static IMP HGAMineTitleDidAdd,HGAMineTitleWillRemove;
static void HGAMineTitleGlyphsChanged(UIView *renderer,SEL cmd,UIView *item) {
    IMP original=cmd==@selector(didAddSubview:) ? HGAMineTitleDidAdd : HGAMineTitleWillRemove;
    ((void (*)(id,SEL,id))original)(renderer,cmd,item);
    HGAMineTitleWatch *watch=objc_getAssociatedObject(renderer,&HGAMineTitleWatchKey) ?:
        objc_getAssociatedObject(renderer,&HGAMineProfileWatchKey);
    if(!watch || watch.queued || ![NSStringFromClass(item.class) isEqual:@"DanceUI._InheritedView"])return;
    watch.queued=YES;
    dispatch_async(dispatch_get_main_queue(),^{
        watch.queued=NO;
        if(watch.renderer.window && watch.page.viewIfLoaded.window)HGAppearanceProcessController(watch.page);
    });
}
static void HGAWatchMineComposeRenderer(UIViewController *page,UIView *renderer,const void *watchKey) {
    HGAMineTitleWatch *watch=objc_getAssociatedObject(page,watchKey);
    if(watch.renderer==renderer)return;
    if(watch.renderer)objc_setAssociatedObject(watch.renderer,watchKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    watch.renderer=nil;
    if(!renderer) {
        objc_setAssociatedObject(page,watchKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        return;
    }
    Class cls=HGAMineComposeRendererClass();
    if(!HGAMineTitleDidAdd)HGAMineTitleDidAdd=HGInstallVerifiedHook(cls,@selector(didAddSubview:),"v24@0:8@16",(IMP)HGAMineTitleGlyphsChanged,@"appearance");
    if(!HGAMineTitleWillRemove)HGAMineTitleWillRemove=HGInstallVerifiedHook(cls,@selector(willRemoveSubview:),"v24@0:8@16",(IMP)HGAMineTitleGlyphsChanged,@"appearance");
    if(!HGAMineTitleDidAdd || !HGAMineTitleWillRemove)return;
    if(!watch)watch=[HGAMineTitleWatch new];
    watch.page=page;watch.renderer=renderer;
    objc_setAssociatedObject(page,watchKey,watch,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    objc_setAssociatedObject(renderer,watchKey,watch,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static void HGAWatchMineTitleRenderer(UIViewController *page,UIView *renderer) {
    HGAWatchMineComposeRenderer(page,renderer,&HGAMineTitleWatchKey);
}

static char HGAMineShortcutMaskKey,HGAMineTitleGlassKey,HGAMineEditProfileGlassKey,HGAMineMenuImageKey;
static void HGAUpdateMineMenu(UIViewController *controller,UIView *titleBar,NSMutableArray<UIView *> *glyphs,BOOL glass) {
    UIButton *button=objc_getAssociatedObject(controller,&HGAMineMenuButtonKey);
    NSNumber *source=objc_getAssociatedObject(controller,&HGAMineEntryStateKey);
    BOOL hasMoon=![(NSNumber *)objc_getAssociatedObject(controller,&HGAMineLaunchSkinKey) isEqual:@NO];
    if(!HGAMineHidesMore() || !source.integerValue || !titleBar.window || glyphs.count>(hasMoon ? 1u : 0u)) {
        [button removeFromSuperview];
        if(button)objc_setAssociatedObject(controller,&HGAMineMenuButtonKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        return;
    }
    if(!button) {
        __weak UIViewController *weakPage=controller;
        UIAction *open=[UIAction actionWithHandler:^(UIAction *action) {
            UIViewController *page=weakPage;
            id depend=HGReadObjectGetter(page,"minePageKMPDepend");
            if(depend)((void (*)(id,SEL,id,BOOL))objc_msgSend)(page,sel_registerName("minePageKMPDepend:setSidebarOpen:"),depend,YES);
        }];
        button=[UIButton buttonWithType:UIButtonTypeCustom primaryAction:open];
        button.accessibilityIdentifier=@"honggo.appearance.mine.menu";
        button.accessibilityLabel=@"菜单";
        UIImageSymbolConfiguration *icon=[UIImageSymbolConfiguration configurationWithPointSize:15.0 weight:UIImageSymbolWeightMedium];
        UIImage *menuImage=[[UIImage systemImageNamed:@"line.3.horizontal" withConfiguration:icon]
            imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
        objc_setAssociatedObject(button,&HGAMineMenuImageKey,menuImage,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        [button setImage:menuImage forState:UIControlStateNormal];
        button.imageView.contentMode=UIViewContentModeCenter;
        button.tintColor=UIColor.labelColor;
        UIView *dot=[[UIView alloc] initWithFrame:CGRectMake(28,8,6,6)];
        dot.layer.cornerRadius=3; dot.userInteractionEnabled=NO; dot.accessibilityElementsHidden=YES;
        dot.backgroundColor=UIColor.systemOrangeColor;
        dot.accessibilityIdentifier=@"honggo.appearance.mine.menu.badge";
        [button addSubview:dot];
        objc_setAssociatedObject(controller,&HGAMineMenuButtonKey,button,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    BOOL darkStyle=HGAAdaptivePageStyle(controller,titleBar)==UIUserInterfaceStyleDark;
    UIColor *menuTint=darkStyle ? UIColor.whiteColor : UIColor.labelColor;
    if(![button.tintColor isEqual:menuTint])button.tintColor=menuTint;
    button.overrideUserInterfaceStyle=darkStyle ? UIUserInterfaceStyleDark : UIUserInterfaceStyleLight;
    UIImage *glyph=objc_getAssociatedObject(button,&HGAMineMenuImageKey);
    HGAConfigurePageButton(button,glass ? HGALiquidButtonMaterial() : nil,glyph);
    if(!glass && [button imageForState:UIControlStateNormal]!=glyph)[button setImage:glyph forState:UIControlStateNormal];
    CGFloat x=CGRectGetMaxX(titleBar.bounds)-28,y=CGRectGetMaxY(titleBar.bounds)-22;
    UIView *neighbour=glyphs.firstObject;
    if(neighbour) {
        CGRect occupied=[neighbour convertRect:neighbour.bounds toView:titleBar];
        y=CGRectGetMidY(occupied);
        if(fabs(x-CGRectGetMidX(occupied))<32)x=CGRectGetMidX(occupied)-40;
    }
    CGRect frame=CGRectMake(x-22,y-22,44,44);
    if(!CGRectEqualToRect(button.frame,frame))button.frame=frame;
    for(UIView *child in button.subviews)if([child.accessibilityIdentifier isEqual:@"honggo.appearance.mine.menu.badge"]) {
        child.hidden=source.integerValue!=2;
        child.layer.borderWidth=1.0/MAX(1.0,button.traitCollection.displayScale);
        child.layer.borderColor=(darkStyle ? [UIColor.blackColor colorWithAlphaComponent:0.45] :
            [UIColor.whiteColor colorWithAlphaComponent:0.9]).CGColor;
    }
    button.accessibilityValue=source.integerValue==2 ? @"有新消息" : nil;
    if(button.superview!=titleBar)[titleBar addSubview:button];
    [button layoutIfNeeded];
}

static void HGAApplyMineKMPChrome(UIViewController *controller) {
    HGAInstallMineOffscreenQuiescence();
    HGAUpdateMineComposeQuiescence(controller);
    UIView *root=controller.viewIfLoaded;
    BOOL enabled=HGAppearanceEnabled() && root.window;
    if(HGAMineFilterInstalled) {
        id depend=HGReadObjectGetter(controller,"minePageKMPDepend");
        Class type=NSClassFromString(@"SSMinePageKMPDepend");
        SEL notify=sel_registerName("notifyFunctionDataChanged");
        Method method=class_getInstanceMethod(object_getClass(depend),notify);
        if([depend isKindOfClass:type] && method && !strcmp(method_getTypeEncoding(method),"v16@0:8")) {
            NSUInteger mask=HGAppearanceFlag(@"mine.hide.shortcut.row") ? 1UL<<HGAMineShortcuts().count : 0;
            NSArray *keys=HGAMineShortcuts();
            for(NSUInteger index=0;index<keys.count;index++)
                if(HGAppearanceFlag([@"mine.hide.shortcut." stringByAppendingString:keys[index]]))mask|=1UL<<index;
            NSNumber *previous=objc_getAssociatedObject(depend,&HGAMineShortcutMaskKey);
            if(mask!=previous.unsignedIntegerValue) {
                NSUInteger moreBits=(1UL<<(keys.count-1)) | (1UL<<keys.count);
                BOOL refreshEntry=((mask & moreBits)!=0)!=((previous.unsignedIntegerValue & moreBits)!=0);
                if(refreshEntry)objc_setAssociatedObject(controller,&HGAMineEntryStateKey,nil,OBJC_ASSOCIATION_RETAIN);
                objc_setAssociatedObject(depend,&HGAMineShortcutMaskKey,@(mask),OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                __weak id weakDepend=depend;
                dispatch_async(dispatch_get_main_queue(), ^{
                    id liveDepend=weakDepend;
                    if(liveDepend) {
                        ((void (*)(id,SEL))objc_msgSend)(liveDepend,notify);
                        SEL change=sel_registerName("notifySidebarEntryStateChanged");
                        Method m=class_getInstanceMethod(object_getClass(liveDepend),change);
                        if(refreshEntry && HGAOriginalMineSidebarSnapshot && m && !strcmp(method_getTypeEncoding(m),"v16@0:8"))
                            ((void (*)(id,SEL))objc_msgSend)(liveDepend,change);
                    }
                });
            }
        }
    }
    BOOL glass=enabled && HGAppearanceFlag(@"player.mineGlass");
    id kmpHandle=HGReadObjectGetter(controller,"minePageKMPHandle");
    UIView *profileContent=glass ? HGAViewGetter(kmpHandle,"contentView",root) : nil;
    UIView *profileRenderer=nil,*editTarget=nil,*editGraphicsBg=nil;
    for(UIView *node in HGADescendants(profileContent,32)) {
        if(![NSStringFromClass(node.class) isEqual:@"DanceUICompose.ComposeRenderingUIViewImpl"] ||
            !HGAVisible(node))continue;
        profileRenderer=node;
        CGFloat edge=CGRectGetMaxX(node.bounds);
        for(UIView *item in node.subviews) {
            CGRect rect=item.frame;
            if(![NSStringFromClass(item.class) isEqual:@"DanceUI._InheritedView"] || !HGAVisible(item) ||
               rect.size.width<60 || rect.size.width>108 || rect.size.height<24 || rect.size.height>36 ||
               CGRectGetMaxX(rect)<edge-32 || CGRectGetMinY(rect)<100)continue;
            UIView *graphicsBg=nil,*drawingText=nil;
            for(UIView *sub in item.subviews) {
                NSString *subCls=NSStringFromClass(sub.class);
                if([subCls isEqual:@"DanceUI._UIGraphicsView"] &&
                   fabs(CGRectGetWidth(sub.bounds)-CGRectGetWidth(item.bounds))<2 &&
                   fabs(CGRectGetHeight(sub.bounds)-CGRectGetHeight(item.bounds))<2)graphicsBg=sub;
                else if([sub isKindOfClass:objc_lookUpClass("_TtCOCV7DanceUI11DisplayList11ViewUpdater8Platform13CGDrawingView")] ||
                        [subCls isEqual:@"CGDrawingView"] ||
                        [sub isKindOfClass:UILabel.class])drawingText=sub;
            }
            if(graphicsBg && drawingText){editTarget=item;editGraphicsBg=graphicsBg;break;}
        }
        if(editTarget)break;
    }
    HGAWatchMineComposeRenderer(controller,glass ? profileRenderer : nil,&HGAMineProfileWatchKey);
    HGAHidePageChromeTarget(root,@"mineProfileEditBg",glass ? editGraphicsBg : nil);
    HGASetPageChromeUnclipped(root,glass && editTarget ? @[editTarget] : nil);
    UIView *editBacking=objc_getAssociatedObject(root,&HGAMineEditProfileGlassKey);
    if(glass && editTarget) {
        if(!editBacking) {
            editBacking=[UIView new];
            editBacking.accessibilityIdentifier=@"honggo.appearance.mine.editProfile";
            editBacking.userInteractionEnabled=NO;
            editBacking.accessibilityElementsHidden=YES;
            objc_setAssociatedObject(root,&HGAMineEditProfileGlassKey,editBacking,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        if(!CGRectEqualToRect(editBacking.frame,editTarget.bounds))editBacking.frame=editTarget.bounds;
        if(editBacking.superview!=editTarget)[editTarget insertSubview:editBacking atIndex:0];
        HGAApplyPageButtonGlass(editBacking,editBacking.bounds,profileContent);
    } else if(editBacking) {
        HGARemoveGlass(editBacking);
        [editBacking removeFromSuperview];
        objc_setAssociatedObject(root,&HGAMineEditProfileGlassKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }

    UIView *titleBar=glass || HGAMineHidesMore() ?
        HGAViewGetter(kmpHandle,"titleBarView",root) : nil;
    NSMutableArray<UIView *> *targets=[NSMutableArray array];
    UIView *renderer=nil;
    for(UIView *node in HGADescendants(titleBar,28)) {
        if(![NSStringFromClass(node.class) isEqual:@"DanceUICompose.ComposeRenderingUIViewImpl"] ||
            !HGAVisible(node))continue;
        renderer=node;
        CGFloat edge=CGRectGetMaxX(node.bounds);
        for(UIView *item in node.subviews) {
            CGRect rect=item.frame;
            if([NSStringFromClass(item.class) isEqual:@"DanceUI._InheritedView"] && HGAVisible(item) &&
                fabs(rect.size.width-24)<1 && fabs(rect.size.height-24)<1 &&
                CGRectGetMinX(rect)>edge-100 && CGRectGetMaxX(rect)<=edge-8)
                [targets addObject:item];
        }
        if(targets.count==1 || (targets.count==2 &&
            fabs(CGRectGetMidY(targets[0].frame)-CGRectGetMidY(targets[1].frame))<1))break;
        [targets removeAllObjects];
    }
    HGAWatchMineTitleRenderer(controller,HGAMineHidesMore() ? renderer : nil);
    HGAUpdateMineMenu(controller,renderer ? titleBar : nil,targets,glass);
    NSArray<UIView *> *backings=objc_getAssociatedObject(root,&HGAMineTitleGlassKey);
    if(!glass || !targets.count || targets.count>2 || backings.count!=targets.count) {
        for(UIView *backing in backings) {
            HGARemoveGlass(backing);
            [backing removeFromSuperview];
        }
        if(backings)objc_setAssociatedObject(root,&HGAMineTitleGlassKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        if(!glass || !targets.count || targets.count>2)return;
        backings=nil;
    }
    if(!backings) {
        backings=targets.count==1 ? @[[UIView new]] : @[[UIView new],[UIView new]];
        for(UIView *backing in backings) {
            backing.accessibilityIdentifier=@"honggo.appearance.mine.title";
            backing.userInteractionEnabled=NO;
            backing.accessibilityElementsHidden=YES;
        }
        objc_setAssociatedObject(root,&HGAMineTitleGlassKey,backings,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    for(NSUInteger index=0;index<targets.count;index++) {
        UIView *glyph=targets[index],*backing=backings[index];
        CGRect glyphRect=[glyph convertRect:glyph.bounds toView:titleBar];
        CGPoint center=CGPointMake(CGRectGetMidX(glyphRect),CGRectGetMidY(glyphRect));
        CGRect frame=CGRectMake(center.x-18,center.y-18,36,36);
        if(!CGRectEqualToRect(backing.frame,frame))backing.frame=frame;
        // The Compose glyphs are clipped to 24pt. Use their title bar for the
        // glass so the original touch views, clipping and unread dot stay intact.
        if(backing.superview!=titleBar)[titleBar insertSubview:backing atIndex:0];
        HGAApplyPageButtonGlass(backing,backing.bounds,titleBar);
    }
}

static BOOL HGATopChannelHidden(NSString *title) {
    static NSDictionary<NSString *,NSString *> *options;
    static dispatch_once_t once;
    dispatch_once(&once,^{options=@{@"关注":@"layout.channels.hide.follow",@"漫剧":@"layout.channels.hide.comic",
        @"真人剧":@"layout.channels.hide.live",@"推荐":@"layout.channels.hide.recommend"};});
    NSString *key=title ? options[title] : nil;
    return key && HGAppearanceFlag(key);
}

static BOOL HGAAllTopControlsHidden(void) {
    return HGAppearanceFlag(@"layout.channels.hide.sidebar") && HGAppearanceFlag(@"layout.channels.hide.search") &&
        HGATopChannelHidden(@"关注") && HGATopChannelHidden(@"漫剧") && HGATopChannelHidden(@"真人剧") && HGATopChannelHidden(@"推荐");
}

static BOOL HGAIsHomeChannel(UIView *view) {
    for(UIView *parent=view.superview;parent;parent=parent.superview) {
        if(!HGAIsHeaderView(parent))continue;
        UIView *channels=HGAViewGetter(parent,"tabContainer",parent);
        return channels && !HGAHeaderShowsFinder(parent) && (view==channels || [view isDescendantOfView:channels]);
    }
    return NO;
}

static void HGAHeaderCategoryGeometryChanged(UIView *categories);
static void HGASyncHeaderChannelLine(UIView *categories);
static char HGAFinderCategoryMaskKey,HGAFinderSelectionPendingKey;

static void HGAUpdateFinderCategories(UIView *categories,BOOL enabled) {
    if(!HGAIsFinderChannel(categories))return;
    UICollectionView *collection=(id)HGAViewGetter(categories,"collectionView",categories);
    if(![collection isKindOfClass:UICollectionView.class])return;
    NSArray *titles=HGReadObjectGetter(categories,"titles");
    NSUInteger mask=enabled?HGAFinderHiddenMask(titles):0;
    NSNumber *previous=objc_getAssociatedObject(categories,&HGAFinderCategoryMaskKey);
    if(mask!=previous.unsignedIntegerValue) {
        objc_setAssociatedObject(categories,&HGAFinderCategoryMaskKey,@(mask),OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        [collection.collectionViewLayout invalidateLayout];
    }
    if(!mask || !HGAVisible(categories) || objc_getAssociatedObject(categories,&HGAFinderSelectionPendingKey))return;
    UIScrollView *pager=HGReadObjectGetter(categories,"contentScrollView");
    if([pager isKindOfClass:UIScrollView.class] && (pager.isTracking || pager.isDragging || pager.isDecelerating))return;
    SEL selected=sel_registerName("selectedIndex"), choose=sel_registerName("selectItemAtIndex:needAnimation:");
    Method selectedMethod=class_getInstanceMethod(categories.class,selected);
    Method chooseMethod=class_getInstanceMethod(categories.class,choose);
    if(!selectedMethod || strcmp(method_getTypeEncoding(selectedMethod),"q16@0:8") ||
        !chooseMethod || strcmp(method_getTypeEncoding(chooseMethod),"B28@0:8q16B24"))return;
    NSInteger index=((NSInteger (*)(id,SEL))objc_msgSend)(categories,selected);
    if(index<0 || (NSUInteger)index>=titles.count || !(mask&(1UL<<index)))return;
    for(NSUInteger candidate=0;candidate<titles.count;candidate++) {
        if(mask&(1UL<<candidate))continue;
        objc_setAssociatedObject(categories,&HGAFinderSelectionPendingKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        __weak UIView *weakCategories=categories;
        dispatch_async(dispatch_get_main_queue(), ^{
            UIView *current=weakCategories;
            if(!current)return;
            objc_setAssociatedObject(current,&HGAFinderSelectionPendingKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            if(!HGAVisible(current) || !HGAIsFinderChannel(current) ||
                [objc_getAssociatedObject(current,&HGAFinderCategoryMaskKey) unsignedIntegerValue]!=mask ||
                ((NSInteger (*)(id,SEL))objc_msgSend)(current,selected)!=index)return;
            ((BOOL (*)(id,SEL,NSInteger,BOOL))objc_msgSend)(current,choose,(NSInteger)candidate,NO);
        });
        break;
    }
}

static void HGAApplyHistoryCategoryClipping(UIView *categories,BOOL enabled) {
    if(!categories)return;
    UIView *wrapper=categories.superview;
    BOOL mine=enabled && HGAppearanceFlag(@"player.mineGlass") && HGAAncestorNamed(categories,@"SSMyUser637TabsView");
    BOOL unclip=mine && wrapper.clipsToBounds && CGRectGetHeight(wrapper.bounds)>=30 &&
        HGAHistoryFilter(wrapper.superview)!=nil;
    UIView *collection=HGAViewGetter(categories,"collectionView",categories);
    NSMutableArray<UIView *> *views=[NSMutableArray arrayWithObject:categories];
    if(collection && collection!=categories)[views addObject:collection];
    HGASetPageChromeUnclipped(categories,unclip ? views : nil);
}

static void HGAApplyChannelIndicators(UIView *categories,BOOL enabled) {
    HGAApplyHistoryCategoryClipping(categories,enabled);
    HGAUpdateFinderCategories(categories,enabled);
    BOOL home=enabled && HGAIsHomeChannel(categories);
    if (home) HGAHeaderCategoryGeometryChanged(categories);
    id indicators=HGReadObjectGetter(categories,"indicators");
    if(![indicators isKindOfClass:NSArray.class] || ![indicators count])return;
    BOOL hide=NO;
    if(home) {
        id titles=HGReadObjectGetter(categories,"titles");
        SEL selector=sel_registerName("selectedIndex");
        Method method=class_getInstanceMethod(categories.class,selector);
        if([titles isKindOfClass:NSArray.class] && method && !strcmp(method_getTypeEncoding(method),"q16@0:8")) {
            NSInteger index=((NSInteger (*)(id,SEL))objc_msgSend)(categories,selector);
            if(index>=0 && (NSUInteger)index<[titles count]) {
                id title=titles[index];
                hide=[title isKindOfClass:NSString.class] && HGATopChannelHidden(title);
            }
        }
    }
    for(id indicator in indicators)
        if([indicator isKindOfClass:UIView.class] && [indicator isDescendantOfView:categories])HGASetHidden(indicator,hide);
    HGASyncHeaderChannelLine(categories);
}

static void HGAApplyChannelOrHistoryTitle(UIView *cell,BOOL enabled) {
    BOOL home=enabled && HGAIsHomeChannel(cell);
    BOOL finder=enabled && HGAIsFinderChannel(cell);
    BOOL mine=enabled && HGAppearanceFlag(@"player.mineGlass") && HGAAncestorNamed(cell,@"SSMyUser637TabsView");
    NSString *title=home || mine || finder ? HGAPageChromeLabel(cell,"titleLabel") : nil;
    if((home || finder) && !title.length) {
        id modelTitle=HGReadObjectGetter(HGReadObjectGetter(cell,"cellModel"),"title");
        if([modelTitle isKindOfClass:NSString.class])title=modelTitle;
    }
    HGASetHidden(cell,(home && HGATopChannelHidden(title)) || (finder && HGAFinderCategoryHidden(title)));

    BOOL history=NO;
    UIView *categories=HGAAncestorNamed(cell,@"FQRJXCategoryTitleView");
    if(categories)HGAApplyHistoryCategoryClipping(categories,enabled);
    if(mine && [@[@"全部",@"已看完",@"未看完"] containsObject:title ?: @""]) {
        // The native history filters are in a nested horizontal row with the
        // filter entrance sharing its toolbar. The top Mine tabs lack it.
        history=HGAHistoryFilter(categories.superview.superview)!=nil;
    }
    HGAApplyHistoryGlass(cell,NO,history);
}

static void HGAApplyNativePageChrome(UIView *view,NSString *name,BOOL enabled) {
    if([view isKindOfClass:NSClassFromString(@"FQRJXCategoryTitleCell")]) {
        HGAApplyChannelOrHistoryTitle(view,enabled);
    } else if([view isKindOfClass:NSClassFromString(@"FQRJXCategoryTitleView")]) {
        HGAApplyChannelIndicators(view,enabled);
    } else if([name isEqual:@"SSMyUser637Bar"]) {
        NSMutableArray *targets=[NSMutableArray array];
        UIView *moon=HGAViewGetter(view,"darkModeMaskView",view);
        HGASetHidden(moon,!HGAppearanceFlag(@"mine.show.theme"));
        if(enabled && HGAppearanceFlag(@"player.mineGlass")) {
            UIView *menu=HGAViewGetter(view,"sideBarButtonView",view) ?: HGAViewGetter(view,"settingsMaskView",view);
            if(moon && !moon.hidden)[targets addObject:moon];
            if(menu)[targets addObject:menu];
        }
        HGAApplyPageChromeTargets(view,targets);
    } else if([name isEqual:@"SSMyUserFollowItemView"]) {
        UIView *edit=HGAViewGetter(view,"editButton",view);
        HGAApplyPageChromeTargets(view,enabled && HGAppearanceFlag(@"player.mineGlass") && edit ? @[edit] : nil);
    } else if([name isEqual:@"SSMyUser637JGZoneCell"]) {
        NSString *title=HGAPageChromeLabel(view,"titleLabel");
        NSString *key=HGAMineShortcutKey(title);
        BOOL hide=HGAppearanceFlag(@"mine.hide.shortcut.row") ||
            (key && HGAppearanceFlag([@"mine.hide.shortcut." stringByAppendingString:key]));
        HGASetHidden(view,hide);
        HGAApplyPageChrome(view,enabled && !hide && HGAppearanceFlag(@"player.mineGlass") && key);
    } else if([view isKindOfClass:NSClassFromString(@"SSMyUser637TabsView")]) {
        UIView *search=HGAViewGetter(view,"searchBtn",view);
        BOOL mine=enabled && HGAppearanceFlag(@"player.mineGlass") && search;
        HGAUpdatePageChromeTargets(view,mine ? @[search] : nil);
        HGAApplyHistoryGlass(search,YES,mine);
        UIImageView *icon=[search isKindOfClass:UIButton.class] ? ((UIButton *)search).imageView : nil;
        HGAApplyPageChromeIcons(view,mine && icon ? @[icon] : nil);
    } else if([view isKindOfClass:NSClassFromString(@"SSVideoFilterPanelEntranceView")]) {
        BOOL mine=enabled && HGAppearanceFlag(@"player.mineGlass") && HGAAncestorNamed(view,@"SSMyUser637TabsView");
        HGASetPageChromeCover(view,"backgroundView",mine);
        UIView *toolbar=view.superview;
        UIView *editButton=nil;
        for(UIView *sibling in toolbar.subviews) {
            if(![sibling isKindOfClass:NSClassFromString(@"FQReaderSaaSBaseButton")] ||
                ![sibling isKindOfClass:UIButton.class])continue;
            NSString *title=((UIButton *)sibling).currentTitle;
            if([title isEqual:@"编辑"] || [title isEqual:@"完成"]){editButton=sibling;break;}
        }
        // Never apply CGAffineTransform shifts to Filter or Edit: when the host
        // sets item.frame while a non-identity transform is active, UIKit shifts
        // item.center and invalidates parent layout inside CA::Transaction::commit(),
        // causing a synchronous layout loop. Centered 44pt capsules inside the
        // native 30x32 unclipped controls already stay clear of the scroll strip
        // and symmetric around the 0.5pt divider.
        HGARestoreGeometry(view);
        if(editButton)HGARestoreGeometry(editButton);
        CGRect filterBase=view.frame;
        CGRect editBase=editButton ? editButton.frame : CGRectNull;
        UIView *divider=nil;
        if(mine && editButton && CGRectGetMinX(editBase)>CGRectGetMaxX(filterBase)) {
            for(UIView *sibling in toolbar.subviews) {
                if(sibling==view || sibling==editButton || [sibling isKindOfClass:UIControl.class])continue;
                CGRect rect=sibling.frame;
                if(CGRectGetWidth(rect)<=2.0 && CGRectGetHeight(rect)>=10.0 &&
                    CGRectGetMidX(rect)>=CGRectGetMaxX(filterBase)-2.0 &&
                    CGRectGetMidX(rect)<=CGRectGetMinX(editBase)+2.0) {
                    divider=sibling;break;
                }
            }
        }
        HGAHidePageChromeTarget(view,@"historyDivider",mine ? divider : nil);
        if(toolbar)HGASetPageChromeUnclipped(toolbar,mine ? @[toolbar] : nil);
        NSMutableArray *targets=[NSMutableArray array];
        if(mine) {
            [targets addObject:view];
            if(editButton)[targets addObject:editButton];
        }
        HGAUpdatePageChromeTargets(view,targets);
        for(UIView *target in targets)HGAApplyHistoryGlass(target,NO,YES);
        UIView *icon=mine ? HGAViewGetter(view,"icon",view) : nil;
        HGAApplyPageChromeIcons(view,icon ? @[icon] : nil);
    } else if([view isKindOfClass:NSClassFromString(@"SSVideoDistributionGoldenPositionCell")]) {
        HGAUpdateFinderShortcuts(view,enabled);
    } else if([name isEqual:@"FQFeedTabFilterPanelEntranceView"] || [name isEqual:@"SSBookStoreEntranceView"] ||
        [name isEqual:@"SSBookStoreKingKongBannerView"] || [view isKindOfClass:NSClassFromString(@"SSVideoDistributionGoldenPositionSubCell")]) {
        BOOL filter=[name isEqual:@"FQFeedTabFilterPanelEntranceView"];
        BOOL golden=[view isKindOfClass:NSClassFromString(@"SSVideoDistributionGoldenPositionSubCell")];
        BOOL scoped=enabled && HGAIsFinderView(view);
        NSString *key=filter ? @"filter" : nil;
        if(scoped && !filter) {
            id modelTitle=HGReadObjectGetter(HGReadObjectGetter(view,golden?"itemData":"model"),golden?"text":"name");
            NSString *title=[modelTitle isKindOfClass:NSString.class] ? modelTitle : HGAPageChromeLabel(view,golden?"titleLabel":"nameLabel");
            key=HGAFinderShortcutKey(title);
        }
        BOOL hide=scoped && key && HGAppearanceFlag([@"finder.hide.shortcut." stringByAppendingString:key]);
        HGASetHidden(view,hide);
        BOOL finder=scoped && key && !hide && HGAppearanceFlag(@"player.finderGlass") && HGAFinderControlReady(view);
        HGASetPageChromeCover(view,filter?"backgroundView":golden?"backgroundImageView":"bgImgView",finder);
        UIView *target=view;
        if(golden) {
            // The inner content fits the icon and label. Use the cell's full
            // content area so the glass surrounds both, with the native hit area intact.
            target=((UICollectionViewCell *)view).contentView;
            UIView *inner=HGAViewGetter(view,"innerContentView",view);
            HGAHidePageChromeTarget(view,@"finderShortcutBorder",finder ? HGAViewGetter(view,"borderView",view) : nil);
            HGASetGlassHostBackground(view,finder);
            if(inner && inner!=target && inner!=view)HGASetGlassHostBackground(inner,finder);
            HGACenterFinderShortcut(view,inner,target,finder);
        }
        HGAApplyFinderMaterial(target,finder ? CGRectInset(target.bounds,1,0) : CGRectZero,YES);
    }
}

static char HGAFinderHeaderBackdropKey;
static void HGAUpdateTheaterHeaderBackdrop(UIViewController *controller,UIView *root,UIView *field,BOOL active) {
    UIView *backdrop=objc_getAssociatedObject(root,&HGAFinderHeaderBackdropKey);
    if(!active || !root.window || !field || ![field isDescendantOfView:root]) {
        [backdrop removeFromSuperview];
        if(backdrop)objc_setAssociatedObject(root,&HGAFinderHeaderBackdropKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        return;
    }
    Class pagerCls=objc_lookUpClass("FQRJXCategoryListContainerView");
    Class feedCls=objc_lookUpClass("FQFeedListView");
    Class headerBgCls=objc_lookUpClass("FQTheaterHeaderBgView");
    UIView *headerBar=nil,*hostContainer=nil,*listBranch=nil,*headerBg=nil;
    for(UIView *cursor=field;cursor && cursor!=root;cursor=cursor.superview) {
        UIView *parent=cursor.superview;
        if(!parent)break;
        NSUInteger headerIdx=[parent.subviews indexOfObjectIdenticalTo:cursor];
        if(headerIdx==NSNotFound)continue;
        UIView *foundList=nil,*foundBg=nil;
        for(NSUInteger i=0;i<headerIdx;i++) {
            UIView *sibling=parent.subviews[i];
            if((pagerCls && [sibling isKindOfClass:pagerCls]) || (feedCls && [sibling isKindOfClass:feedCls]))
                foundList=sibling;
            else if(headerBgCls && [sibling isKindOfClass:headerBgCls])
                foundBg=sibling;
        }
        if(foundList) {
            headerBar=cursor;hostContainer=parent;listBranch=foundList;headerBg=foundBg;
            break;
        }
    }
    if(!hostContainer || !listBranch || !headerBar) {
        [backdrop removeFromSuperview];
        if(backdrop)objc_setAssociatedObject(root,&HGAFinderHeaderBackdropKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        return;
    }
    CGRect searchInHost=[field convertRect:field.bounds toView:hostContainer];
    CGRect barInHost=[headerBar convertRect:headerBar.bounds toView:hostContainer];
    CGFloat bottomY=CGRectGetMaxY(searchInHost);
    UIView *tabContainer=HGAViewGetter(controller,"tabContainer",root);
    if(tabContainer && !tabContainer.hidden && tabContainer.alpha>0.01 && [tabContainer isDescendantOfView:hostContainer]) {
        CGRect tabInHost=[tabContainer convertRect:tabContainer.bounds toView:hostContainer];
        CGFloat visibleMaxY=headerBar.clipsToBounds && [tabContainer isDescendantOfView:headerBar] ?
            MIN(CGRectGetMaxY(barInHost),CGRectGetMaxY(tabInHost)) : CGRectGetMaxY(tabInHost);
        if(visibleMaxY>bottomY && visibleMaxY<=bottomY+56)bottomY=visibleMaxY;
    }
    if(headerBar.clipsToBounds && CGRectGetMaxY(barInHost)>=CGRectGetMinY(searchInHost)+24)
        bottomY=MIN(bottomY,CGRectGetMaxY(barInHost));
    if(bottomY<=1 || CGRectGetWidth(hostContainer.bounds)<=1) {
        [backdrop removeFromSuperview];
        return;
    }
    if(!backdrop) {
        backdrop=[UIView new];
        backdrop.accessibilityIdentifier=@"honggo.appearance.finder.headerBackdrop";
        backdrop.userInteractionEnabled=NO;
        backdrop.accessibilityElementsHidden=YES;
        objc_setAssociatedObject(root,&HGAFinderHeaderBackdropKey,backdrop,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    BOOL dark=HGAAdaptivePageStyle(controller,root)==UIUserInterfaceStyleDark;
    UIColor *fill=nil;
    if(headerBg.backgroundColor) {
        UIColor *resolved=[headerBg.backgroundColor resolvedColorWithTraitCollection:
            [UITraitCollection traitCollectionWithUserInterfaceStyle:dark ? UIUserInterfaceStyleDark : UIUserInterfaceStyleLight]];
        if(CGColorGetAlpha(resolved.CGColor)>=0.95)fill=resolved;
    }
    if(!fill)fill=dark ? [UIColor colorWithRed:0.07 green:0.07 blue:0.08 alpha:1.0] : UIColor.whiteColor;
    if(![backdrop.backgroundColor isEqual:fill])backdrop.backgroundColor=fill;
    CGRect targetFrame=CGRectMake(0,0,CGRectGetWidth(hostContainer.bounds),bottomY);
    if(!CGRectEqualToRect(backdrop.frame,targetFrame))backdrop.frame=targetFrame;
    NSUInteger listIdx=[hostContainer.subviews indexOfObjectIdenticalTo:listBranch];
    NSUInteger backIdx=[hostContainer.subviews indexOfObjectIdenticalTo:backdrop];
    NSUInteger barIdx=[hostContainer.subviews indexOfObjectIdenticalTo:headerBar];
    if(backdrop.superview!=hostContainer || backIdx==NSNotFound || backIdx<listIdx || backIdx>barIdx)
        [hostContainer insertSubview:backdrop aboveSubview:listBranch];
}

static void HGAReconcileTheaterHeaderBackdropForView(UIView *view) {
    UIViewController *theater=nil;
    Class theaterCls=NSClassFromString(@"FQTheaterViewController");
    for(UIView *node=view;node;node=node.superview) {
        UIResponder *resp=node.nextResponder;
        if([resp isKindOfClass:theaterCls]){theater=(UIViewController *)resp;break;}
    }
    if(!theater || !theater.isViewLoaded)return;
    UIView *root=theater.view;
    BOOL active=HGAppearanceEnabled() && HGAppearanceFlag(@"player.finderGlass") && !HGAFinderSearchRowHidden() && root.window!=nil;
    UIView *search=HGAViewGetter(theater,"searchView",root) ?: HGAViewGetter(theater,"searchButtonView",root);
    UIView *field=HGAViewGetter(theater,"searchBgView",root) ?: search;
    HGAUpdateTheaterHeaderBackdrop(theater,root,field,active);
}

static void HGAApplyFinderChrome(UIViewController *controller) {
    UIView *root=controller.view;
    BOOL enabled=HGAppearanceEnabled(),glass=enabled && HGAppearanceFlag(@"player.finderGlass");
    // UIKit can detach the controller's view while switching tabs or moving
    // PiP. Leave its surfaces attached to their local targets so their effect
    // views can restore themselves as soon as the same view rejoins a window.
    if(glass && !root.window)return;
    BOOL theater=[controller isKindOfClass:NSClassFromString(@"FQTheaterViewController")];
    NSArray<NSString *> *aiArt=@[@"aiSearchEntrancePlaceholderView",@"aiSearchEntranceImageView",
        @"aiSearchEntranceLottieFirstFrameView",@"aiSearchEntranceLottieView"];
    UIView *categories=enabled && root.window && HGAppearanceFlag(@"finder.hide.category.row") ?
        HGAViewGetter(controller,"tabContainer",root) : nil;
    static char collapsedRows;
    BOOL hideRow=HGAFinderSearchRowHidden();
    NSUInteger collapse=(categories ? 1u : 0u) | (hideRow ? 2u : 0u);
    BOOL changed=collapse!=[objc_getAssociatedObject(root,&collapsedRows) unsignedIntegerValue];
    if (changed) objc_setAssociatedObject(root,&collapsedRows,collapse ? @(collapse) : nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    if (changed && theater && root.window) {
        SEL relayout=sel_registerName("relayoutHeader");
        Method method=class_getInstanceMethod(object_getClass(controller),relayout);
        if (method && !strcmp(method_getTypeEncoding(method),"v16@0:8"))
            ((void (*)(id,SEL))objc_msgSend)(controller,relayout);
    }
    HGAHidePageChromeTarget(root,@"finderCategories",categories);
    if(!enabled || !root.window) {
        UIView *recognitionContent=HGAViewGetter(controller,"normalImageSearchView",root);
        HGARestoreGeometry(recognitionContent);
        HGAApplyFinderSearch(root,nil,nil,nil,nil,nil,nil,nil,NO);
        HGAApplyPageChromeIcons(root,nil);
        if(theater) {
            HGAApplyFinderAIInk(root,nil,nil,aiArt);
            HGAUpdateTheaterHeaderBackdrop(controller,root,nil,NO);
        }
        return;
    }
    UIView *search=HGAViewGetter(controller,"searchView",root) ?: HGAViewGetter(controller,"searchButtonView",root);
    // The search background and its cue/gesture view are siblings in the host.
    UIView *field=HGAViewGetter(controller,"searchBgView",root) ?: search;
    UIView *recognitionContent=HGAViewGetter(controller,"normalImageSearchView",root);
    UIView *recognition=HGAViewGetter(controller,"imageSearchContainer",root) ?: recognitionContent;
    UIView *assistant=HGAViewGetter(controller,theater ? "aiSearchEntranceView" : "searchAiButtonView",root);
    UIView *divider=recognition ? HGAViewGetter(recognitionContent,"separatorView",recognition) : nil;
    UIView *alternate=HGAViewGetter(controller,"searchButtonView",root);
    HGAApplyFinderSearch(root,field,search,alternate,recognition,assistant,recognitionContent,divider,glass);
    if(theater)HGAUpdateTheaterHeaderBackdrop(controller,root,field,glass && !hideRow);
    NSMutableArray<UIView *> *icons=[NSMutableArray array];
    if(glass) {
        UIView *searchIcon=[search isKindOfClass:UIButton.class] ? ((UIButton *)search).imageView :
            (HGAViewGetter(search,"searchIcon",search) ?: HGAViewGetter(search,"iconImageView",search));
        if(!searchIcon) {
            // On the theatre host the cue row exposes the leading magnifier as
            // a plain image view, alongside a separate cue content view.
            for(UIView *child in search.subviews) {
                if([child isKindOfClass:UIImageView.class] && HGAFinderControlReady(child) &&
                    CGRectGetMinX(child.frame)<20 && CGRectGetWidth(child.frame)<=24) {
                    searchIcon=child;break;
                }
            }
        }
        UIView *camera=HGAViewGetter(recognitionContent,"cameraIconView",recognitionContent);
        UIImageView *aiIcon=!theater && [assistant isKindOfClass:UIButton.class] ? ((UIButton *)assistant).imageView : nil;
        if(searchIcon)[icons addObject:searchIcon];
        if(camera)[icons addObject:camera];
        if(aiIcon)[icons addObject:aiIcon];
    }
    HGAApplyPageChromeIcons(root,icons);
    if(theater)HGAApplyFinderAIInk(root,glass ? assistant : nil,controller,aiArt);
}
