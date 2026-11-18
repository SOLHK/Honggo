static NSArray<NSString *> *HGAPageChromeViews(void) {
    return @[@"FQRJXCategoryTitleCell",@"FQRJXCategoryTitleView",@"SSHGBookStoreCategoryTitleCell",@"SSHGBookStoreCategoryTitleView",
        @"SSBookStoreEntranceView",@"SSBookStoreKingKongBannerView",
        @"FQFeedTabFilterPanelEntranceView",@"SSMyUser637TabsView",@"SSVideoFilterPanelEntranceView",
        @"SSMyUser637Bar",@"SSMyUserFollowItemView",@"SSMyUser637JGZoneCell"];
}

static NSArray<NSString *> *HGAFinderControllers(void) {
    return @[@"FQTheaterViewController",@"SSBookStoreNewViewController"];
}

static UIView *HGAAncestorNamed(UIView *view,NSString *name) {
    for(UIView *parent=view.superview;parent;parent=parent.superview)
        if([NSStringFromClass(parent.class) isEqual:name])return parent;
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

@interface HGAPageChromeBackingState : NSObject
@property(nonatomic) BOOL modifiedOpaque;
@property(nonatomic) BOOL sourceOpaque;
@property(nonatomic,strong) UIColor *layerBackground;
@property(nonatomic) BOOL modifiedLayerBackground;
@property(nonatomic,strong) NSMutableDictionary<NSNumber *,UIImage *> *buttonImages;
@property(nonatomic,strong) NSMapTable<UIView *,UIColor *> *childBackgrounds;
@property(nonatomic,strong) NSMapTable<UIView *,UIColor *> *childLayerBackgrounds;
@end
@implementation HGAPageChromeBackingState @end
static char HGAPageChromeBackingKey;

static BOOL HGAIsNeutralBackingColor(UIColor *color,UITraitCollection *traits) {
    if(!color)return NO;
    CGFloat r=0,g=0,b=0,a=0;
    if(![[color resolvedColorWithTraitCollection:traits] getRed:&r green:&g blue:&b alpha:&a])return NO;
    return a>0.02 && (MAX(r,MAX(g,b))-MIN(r,MIN(g,b)))<0.10;
}

static void HGACleanPageChromeBacking(UIView *target,BOOL clean) {
    if(!target)return;
    HGAPageChromeBackingState *state=objc_getAssociatedObject(target,&HGAPageChromeBackingKey);
    if(!clean) {
        if(!state)return;
        if(state.modifiedOpaque && !target.opaque)target.opaque=state.sourceOpaque;
        if(state.modifiedLayerBackground) {
            CGColorRef cur=target.layer.backgroundColor;
            if(!cur || CGColorGetAlpha(cur)==0)target.layer.backgroundColor=state.layerBackground.CGColor;
        }
        if([target isKindOfClass:UIButton.class] && state.buttonImages.count) {
            UIButton *button=(UIButton *)target;
            for(NSNumber *stateNum in state.buttonImages) {
                UIControlState st=stateNum.unsignedIntegerValue;
                if(![button backgroundImageForState:st])
                    [button setBackgroundImage:state.buttonImages[stateNum] forState:st];
            }
        }
        for(UIView *child in state.childBackgrounds.keyEnumerator) {
            if([child.backgroundColor isEqual:UIColor.clearColor])
                child.backgroundColor=[state.childBackgrounds objectForKey:child];
        }
        for(UIView *child in state.childLayerBackgrounds.keyEnumerator) {
            CGColorRef cur=child.layer.backgroundColor;
            if(!cur || CGColorGetAlpha(cur)==0)
                child.layer.backgroundColor=[state.childLayerBackgrounds objectForKey:child].CGColor;
        }
        objc_setAssociatedObject(target,&HGAPageChromeBackingKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        return;
    }
    if(!state) {
        state=[HGAPageChromeBackingState new];
        state.childBackgrounds=[NSMapTable weakToStrongObjectsMapTable];
        state.childLayerBackgrounds=[NSMapTable weakToStrongObjectsMapTable];
        objc_setAssociatedObject(target,&HGAPageChromeBackingKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    if(target.opaque) {
        state.sourceOpaque=YES;
        state.modifiedOpaque=YES;
        target.opaque=NO;
    }
    CGColorRef targetLayerCG=target.layer.backgroundColor;
    if(targetLayerCG && CGColorGetAlpha(targetLayerCG)>0.01) {
        UIColor *targetBG=target.backgroundColor;
        if(!targetBG || !CGColorEqualToColor(targetLayerCG,targetBG.CGColor)) {
            state.layerBackground=[UIColor colorWithCGColor:targetLayerCG];
            state.modifiedLayerBackground=YES;
            target.layer.backgroundColor=nil;
        }
    }
    if([target isKindOfClass:UIButton.class]) {
        UIButton *button=(UIButton *)target;
        UIControlState states[]={UIControlStateNormal,UIControlStateHighlighted,UIControlStateSelected};
        for(NSUInteger i=0;i<3;i++) {
            UIImage *bg=[button backgroundImageForState:states[i]];
            if(bg) {
                if(!state.buttonImages)state.buttonImages=[NSMutableDictionary dictionary];
                state.buttonImages[@(states[i])]=bg;
                [button setBackgroundImage:nil forState:states[i]];
            }
        }
    }
    CGFloat w=CGRectGetWidth(target.bounds),h=CGRectGetHeight(target.bounds);
    if(w<=0 || h<=0)return;
    for(UIView *child in HGADescendants(target,24)) {
        if(child==target || HGAOwnView(child))continue;
        if([child isKindOfClass:UILabel.class] || [child isKindOfClass:UIImageView.class] ||
            [child isKindOfClass:UITextField.class] || [child isKindOfClass:UITextView.class] ||
            [child isKindOfClass:UIControl.class] || [child isKindOfClass:UIScrollView.class])continue;
        if(CGRectGetWidth(child.bounds)<w*0.5 || CGRectGetHeight(child.bounds)<h*0.5)continue;
        if(![child.backgroundColor isEqual:UIColor.clearColor] &&
            HGAIsNeutralBackingColor(child.backgroundColor,child.traitCollection)) {
            [state.childBackgrounds setObject:child.backgroundColor forKey:child];
            child.backgroundColor=UIColor.clearColor;
        }
        CGColorRef layerCG=child.layer.backgroundColor;
        if(layerCG && CGColorGetAlpha(layerCG)>0.02) {
            UIColor *layerColor=[UIColor colorWithCGColor:layerCG];
            if((!child.backgroundColor || !CGColorEqualToColor(layerCG,child.backgroundColor.CGColor)) &&
                HGAIsNeutralBackingColor(layerColor,child.traitCollection)) {
                [state.childLayerBackgrounds setObject:layerColor forKey:child];
                child.layer.backgroundColor=nil;
            }
        }
    }
}

static void HGAApplyPageChromeMaterial(UIView *target,CGRect frame,NSString *style) {
    if(HGAVisible(target) && !CGRectIsEmpty(frame)) {
        CGFloat radius=MIN(CGRectGetHeight(frame)/2,24);
        BOOL dark=target.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark;
        BOOL reduced=UIAccessibilityIsReduceTransparencyEnabled();
        BOOL classicBlur=[style isEqual:@"blur"];
        BOOL solid=[style isEqual:@"solid"];
        HGACleanPageChromeBacking(target,YES);
        HGAInstallGlassInRect(target,style,frame,radius,0);
        HGAAppearanceMaterial *glass=HGAState(target,NO).glass;
        if(glass) {
            glass.interactiveGlass=!classicBlur && !solid && !reduced;
            if(!classicBlur && !solid && !reduced && HGNativeGlassConfigured()) {
                glass.layer.borderWidth=0.5;
                glass.layer.borderColor=[UIColor colorWithWhite:dark?1:0 alpha:dark?0.18:0.10].CGColor;
            } else {
                glass.layer.borderWidth=0;
                glass.layer.borderColor=nil;
            }
            if(@available(iOS 17.0,*))[glass updateTraitsIfNeeded];
        }
        UIColor *overlay;
        if(reduced || solid)overlay=[UIColor colorWithWhite:dark?0.25:0.91 alpha:1];
        else if(classicBlur)overlay=[UIColor colorWithWhite:dark?0.85:1.0 alpha:dark?0.12:0.18];
        else overlay=dark ? [UIColor colorWithWhite:0.85 alpha:0.12] : [UIColor colorWithWhite:1 alpha:0];
        HGASetGlassOverlay(target,overlay);
    } else if(target) {
        HGARemoveGlass(target);
        HGACleanPageChromeBacking(target,NO);
    }
}

static void HGAApplyPageChrome(UIView *target,BOOL enabled) {
    if([target isKindOfClass:UICollectionViewCell.class])target=((UICollectionViewCell *)target).contentView;
    if(enabled)HGAApplyPageChromeMaterial(target,target.bounds,HGAppearanceString(@"player.material"));
    else if(target){HGARemoveGlass(target);HGACleanPageChromeBacking(target,NO);}
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

static void HGAApplyPageChromeTargets(UIView *owner,NSArray<UIView *> *targets) {
    HGAUpdatePageChromeTargets(owner,targets);
    for(UIView *target in targets)HGAApplyPageChrome(target,YES);
}

static NSString *HGAFinderMaterial(void) {
    return [HGAppearanceString(@"player.material") isEqual:@"clear"] ? @"clear" : @"regular";
}

static void HGAApplyFinderSearch(UIView *owner,UIView *field,UIView *recognition,UIView *assistant,UIView *divider) {
    NSMutableArray<UIView *> *targets=[NSMutableArray array];
    if(field)[targets addObject:field];
    if(recognition && recognition!=field)[targets addObject:recognition];
    if(assistant)[targets addObject:assistant];
    HGAUpdatePageChromeTargets(owner,targets);

    CGRect searchFrame=field ? field.bounds : CGRectZero;
    BOOL separate=HGAVisible(field) && HGAVisible(recognition) && recognition!=field;
    if(separate) {
        CGRect button=[recognition convertRect:recognition.bounds toView:field];
        // Some host layouts paint the search backing beneath the recognition
        // tool. Shorten only the backing so the native touch regions can stay.
        if(CGRectGetMinX(button)>CGRectGetMidX(searchFrame) &&
            CGRectIntersectsRect(searchFrame,CGRectInset(button,-6,0)))
            searchFrame.size.width=CGRectGetMinX(button)-6-CGRectGetMinX(searchFrame);
    }
    HGAHidePageChromeTarget(owner,@"finderRecognitionDivider",separate ? divider : nil);
    NSString *style=HGAFinderMaterial();
    for(UIView *target in targets) {
        CGRect frame=target==field ? searchFrame : target.bounds;
        if(target==assistant) {
            CGFloat side=MIN(frame.size.width,frame.size.height);
            frame=CGRectInset(frame,(frame.size.width-side)/2,(frame.size.height-side)/2);
        }
        HGAApplyPageChromeMaterial(target,frame,style);
    }
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
    if(!enabled || !finder || !HGAppearanceFlag(@"player.finderGlass")) {
        HGAApplyFinderSearch(header,nil,nil,nil,nil);
        return finder;
    }
    UIView *row=HGAViewGetter(header,"findDramaSearchRowViewV731",header);
    UIView *searchOwner=HGAViewGetter(header,"searchViewContainer",row ?: header) ?: search;
    UIView *searchField=HGAViewGetter(searchOwner,"searchView",searchOwner);
    UIView *field=HGAViewGetter(searchOwner,"searchBgView",searchOwner) ?: searchField;
    UIView *recognition=HGAViewGetter(searchOwner,"imageSearchContainer",searchOwner);
    UIView *assistant=HGAViewGetter(header,"findDramaAIEntranceViewV731",header);
    UIView *divider=recognition ? HGAViewGetter(searchOwner,"imageSearchSeparatorView",recognition) : nil;
    HGAApplyFinderSearch(header,field,recognition,assistant,divider);
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

static BOOL HGAIsFinderChannelCategoryView(UIView *view) {
    if(!([view isKindOfClass:NSClassFromString(@"FQRJXCategoryTitleView")] ||
        [view isKindOfClass:NSClassFromString(@"SSHGBookStoreCategoryTitleView")]))return NO;
    for(UIView *parent=view.superview;parent;parent=parent.superview) {
        if(HGAIsHeaderView(parent) && HGAHeaderShowsFinder(parent))return YES;
    }
    return HGAIsFinderView(view);
}

static void HGAApplyChannelIndicators(UIView *categories,BOOL enabled) {
    HGASetHidden(categories,enabled && HGAppearanceFlag(@"layout.finder.hide.channels") &&
        HGAIsFinderChannelCategoryView(categories));
    id indicators=HGReadObjectGetter(categories,"indicators");
    if(![indicators isKindOfClass:NSArray.class] || ![indicators count])return;
    BOOL hide=NO;
    if(enabled && HGAIsHomeChannel(categories)) {
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
}

static void HGAApplyChannelOrHistoryTitle(UIView *cell,BOOL enabled) {
    BOOL home=enabled && HGAIsHomeChannel(cell);
    BOOL mine=enabled && HGAppearanceFlag(@"player.mineGlass") && HGAAncestorNamed(cell,@"SSMyUser637TabsView");
    NSString *title=home || mine ? HGAPageChromeLabel(cell,"titleLabel") : nil;
    if((home || mine) && !title.length) {
        id modelTitle=HGReadObjectGetter(HGReadObjectGetter(cell,"cellModel"),"title");
        if([modelTitle isKindOfClass:NSString.class])title=modelTitle;
    }
    HGASetHidden(cell,home && HGATopChannelHidden(title));

    BOOL applyMineTab=mine && title.length>0;
    if([cell isKindOfClass:UICollectionViewCell.class]) {
        UIView *content=((UICollectionViewCell *)cell).contentView;
        if(applyMineTab) {
            CGRect frame=CGRectInset(content.bounds,CGRectGetWidth(content.bounds)>24?1:0,
                CGRectGetHeight(content.bounds)>28?2:0);
            HGAApplyPageChromeMaterial(content,frame,HGAppearanceString(@"player.material"));
        } else {
            HGARemoveGlass(content);
            HGACleanPageChromeBacking(content,NO);
        }
    } else {
        HGAApplyPageChrome(cell,applyMineTab);
    }
}

static void HGAApplyNativePageChrome(UIView *view,NSString *name,BOOL enabled) {
    if([view isKindOfClass:NSClassFromString(@"FQRJXCategoryTitleCell")]) {
        HGAApplyChannelOrHistoryTitle(view,enabled);
    } else if([view isKindOfClass:NSClassFromString(@"FQRJXCategoryTitleView")]) {
        HGAApplyChannelIndicators(view,enabled);
    } else if([name isEqual:@"SSMyUser637Bar"]) {
        NSMutableArray *targets=[NSMutableArray array];
        if(enabled && HGAppearanceFlag(@"player.mineGlass")) {
            UIView *moon=HGAViewGetter(view,"darkModeMaskView",view);
            UIView *menu=HGAViewGetter(view,"sideBarButtonView",view) ?: HGAViewGetter(view,"settingsMaskView",view);
            if(moon)[targets addObject:moon];
            if(menu)[targets addObject:menu];
        }
        HGAApplyPageChromeTargets(view,targets);
    } else if([name isEqual:@"SSMyUserFollowItemView"]) {
        UIView *edit=HGAViewGetter(view,"editButton",view);
        HGAApplyPageChromeTargets(view,enabled && HGAppearanceFlag(@"player.mineGlass") && edit ? @[edit] : nil);
    } else if([name isEqual:@"SSMyUser637JGZoneCell"]) {
        NSString *title=HGAPageChromeLabel(view,"titleLabel");
        BOOL requested=[@[@"订单",@"消息",@"游戏",@"追更",@"更多功能",@"更多\n功能"] containsObject:title ?: @""];
        HGAApplyPageChrome(view,enabled && HGAppearanceFlag(@"player.mineGlass") && requested);
    } else if([name isEqual:@"SSMyUser637TabsView"]) {
        UIView *search=HGAViewGetter(view,"searchBtn",view);
        HGAApplyPageChromeTargets(view,enabled && HGAppearanceFlag(@"player.mineGlass") && search ? @[search] : nil);
    } else if([name isEqual:@"SSVideoFilterPanelEntranceView"]) {
        BOOL mine=enabled && HGAppearanceFlag(@"player.mineGlass") && HGAAncestorNamed(view,@"SSMyUser637TabsView");
        HGASetPageChromeCover(view,"backgroundView",mine);
        NSMutableArray *targets=[NSMutableArray array];
        if(mine) {
            [targets addObject:view];
            for(UIView *sibling in view.superview.subviews) {
                if(![NSStringFromClass(sibling.class) isEqual:@"FQReaderSaaSBaseButton"] ||
                    ![sibling isKindOfClass:UIButton.class])continue;
                NSString *title=((UIButton *)sibling).currentTitle;
                if([title isEqual:@"编辑"] || [title isEqual:@"完成"]){[targets addObject:sibling];break;}
            }
        }
        HGAApplyPageChromeTargets(view,targets);
    } else if([name isEqual:@"FQFeedTabFilterPanelEntranceView"] ||
        [name isEqual:@"SSBookStoreEntranceView"] || [name isEqual:@"SSBookStoreKingKongBannerView"]) {
        BOOL filter=[name isEqual:@"FQFeedTabFilterPanelEntranceView"];
        BOOL isFinder=HGAIsFinderView(view);
        BOOL isShortcutTitle=filter;
        if(isFinder && !filter) {
            NSString *title=HGAPageChromeLabel(view,"nameLabel");
            if(!title.length) {
                id modelTitle=HGReadObjectGetter(HGReadObjectGetter(view,"model"),"name");
                if([modelTitle isKindOfClass:NSString.class])title=modelTitle;
            }
            isShortcutTitle=[@[@"筛选",@"排行榜",@"新剧",@"预约"] containsObject:title ?: @""];
        }
        BOOL hideShortcut=enabled && isFinder && isShortcutTitle && HGAppearanceFlag(@"layout.finder.hide.shortcuts");
        HGASetHidden(view,hideShortcut);
        BOOL finderGlass=enabled && !hideShortcut && HGAppearanceFlag(@"player.finderGlass") &&
            HGAVisible(view) && isFinder && isShortcutTitle;
        HGASetPageChromeCover(view,filter ? "backgroundView" : "bgImgView",finderGlass);
        HGAApplyPageChromeMaterial(view,finderGlass ? CGRectInset(view.bounds,1,0) : CGRectZero,HGAFinderMaterial());
    }
}

static void HGAApplyFinderChrome(UIViewController *controller) {
    UIView *root=controller.view;
    BOOL enabled=HGAppearanceEnabled() && root.window;
    for(UIView *child in HGADescendants(root,160)) {
        if([child isKindOfClass:NSClassFromString(@"FQRJXCategoryTitleView")]) {
            HGAApplyChannelIndicators(child,enabled);
        } else {
            NSString *childName=NSStringFromClass(child.class);
            if([childName isEqual:@"FQFeedTabFilterPanelEntranceView"] ||
                [childName isEqual:@"SSBookStoreEntranceView"] || [childName isEqual:@"SSBookStoreKingKongBannerView"])
                HGAApplyNativePageChrome(child,childName,enabled);
        }
    }
    if(!enabled || !HGAppearanceFlag(@"player.finderGlass")) {
        HGAApplyFinderSearch(root,nil,nil,nil,nil);
        return;
    }
    BOOL theater=[controller isKindOfClass:NSClassFromString(@"FQTheaterViewController")];
    UIView *search=HGAViewGetter(controller,"searchView",root) ?: HGAViewGetter(controller,"searchButtonView",root);
    // The search background and its cue/gesture view are siblings in the host.
    UIView *field=HGAViewGetter(controller,"searchBgView",root) ?: search;
    UIView *recognitionContent=HGAViewGetter(controller,"normalImageSearchView",root);
    UIView *recognition=HGAViewGetter(controller,"imageSearchContainer",root) ?: recognitionContent;
    UIView *assistant=HGAViewGetter(controller,theater ? "aiSearchEntranceView" : "searchAiButtonView",root);
    UIView *divider=recognition ? HGAViewGetter(recognitionContent,"separatorView",recognition) : nil;
    HGAApplyFinderSearch(root,field,recognition,assistant,divider);
}
