static NSArray<NSString *> *HGAPageChromeViews(void) {
    return @[@"FQRJXCategoryTitleCell",@"FQRJXCategoryTitleView",@"SSBookStoreEntranceView",@"SSBookStoreKingKongBannerView",
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

static NSString *HGAPageChromeLabel(UIView *view,const char *getter) {
    UIView *label=HGAViewGetter(view,getter,view);
    return [label isKindOfClass:UILabel.class] ? ((UILabel *)label).text : nil;
}

static UIView *HGAHistoryFilter(UIView *toolbar) {
    for(UIView *child in toolbar.subviews)
        if([NSStringFromClass(child.class) isEqual:@"SSVideoFilterPanelEntranceView"])return child;
    return nil;
}

static void HGAApplyPageChrome(UIView *target,BOOL enabled) {
    if([target isKindOfClass:UICollectionViewCell.class])target=((UICollectionViewCell *)target).contentView;
    if(enabled && HGAVisible(target)) {
        CGFloat radius=MIN(CGRectGetHeight(target.bounds)/2,24);
        BOOL dark=target.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark;
        BOOL reduced=UIAccessibilityIsReduceTransparencyEnabled();
        HGAInstallGlass(target,HGAppearanceString(@"player.material"),radius,0);
        HGASetGlassOverlay(target,[UIColor colorWithWhite:reduced ? (dark ? 0.25 : 0.91) : (dark ? 0.85 : 0.45)
            alpha:reduced ? 1 : (dark ? 0.12 : 0.16)]);
    } else if(target)HGARemoveGlass(target);
}

static char HGAPageChromeCoversKey;
static void HGASetPageChromeCover(UIView *container,const char *getter,BOOL hide) {
    NSMapTable<NSString *,UIView *> *covers=objc_getAssociatedObject(container,&HGAPageChromeCoversKey);
    if(!hide && !covers)return;
    NSString *key=@(getter);
    UIView *previous=[covers objectForKey:key];
    UIView *cover=hide ? HGAViewGetter(container,getter,container) : nil;
    if(cover==container || ![cover isKindOfClass:UIImageView.class])cover=nil;
    if(previous && previous!=cover)HGASetHidden(previous,NO);
    if(cover) {
        if(!covers) {
            covers=[NSMapTable strongToWeakObjectsMapTable];
            objc_setAssociatedObject(container,&HGAPageChromeCoversKey,covers,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        [covers setObject:cover forKey:key];
        HGASetHidden(cover,YES);
    } else {
        [covers removeObjectForKey:key];
        if(covers && !covers.count)
            objc_setAssociatedObject(container,&HGAPageChromeCoversKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
}

static char HGAPageChromeTargetsKey;
static void HGAApplyPageChromeTargets(UIView *owner,NSArray<UIView *> *targets) {
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
    for(UIView *target in targets) {
        [previous addObject:target];
        HGAApplyPageChrome(target,YES);
    }
}

static BOOL HGAApplyHeaderChrome(UIView *header,BOOL enabled) {
    BOOL finder=HGAHeaderShowsFinder(header);
    UIView *sidebar=HGAViewGetter(header,"sideBar",header);
    UIView *search=HGAViewGetter(header,"searchViewContainer",header);
    // The finder row reuses the native search container. Restore its home
    // visibility before applying the finder material and leave its actions alone.
    if(sidebar)HGASetHidden(sidebar,enabled && !finder && HGAppearanceFlag(@"layout.channels.hide.sidebar"));
    if(search)HGASetHidden(search,enabled && !finder && HGAppearanceFlag(@"layout.channels.hide.search"));
    if(!enabled || !finder || !HGAppearanceFlag(@"player.finderGlass")) {
        HGAApplyPageChromeTargets(header,nil);
        return finder;
    }
    UIView *row=HGAViewGetter(header,"findDramaSearchRowViewV731",header);
    UIView *searchOwner=HGAViewGetter(header,"searchViewContainer",row ?: header) ?: search;
    UIView *searchField=HGAViewGetter(searchOwner,"searchView",searchOwner);
    UIView *field=HGAViewGetter(searchOwner,"searchBgView",searchOwner) ?: searchField;
    UIView *recognition=HGAViewGetter(searchOwner,"imageSearchContainer",searchOwner);
    UIView *assistant=HGAViewGetter(header,"findDramaAIEntranceViewV731",header);
    NSMutableArray *targets=[NSMutableArray array];
    if(field)[targets addObject:field];
    if(recognition)[targets addObject:recognition];
    if(assistant)[targets addObject:assistant];
    HGAApplyPageChromeTargets(header,targets);
    return finder;
}

static BOOL HGAIsFinderController(UIViewController *controller) {
    return [controller isKindOfClass:NSClassFromString(@"FQTheaterViewController")] ||
        [controller isKindOfClass:NSClassFromString(@"SSBookStoreNewViewController")];
}

static BOOL HGAIsFinderView(UIView *view) {
    for(UIViewController *owner=HGAOwner(view);owner;owner=owner.parentViewController)
        if(HGAIsFinderController(owner))return YES;
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

static BOOL HGAIsHomeChannel(UIView *view) {
    UIView *header=HGAAncestorNamed(view,@"SSVideoSeriesFeedHeaderView");
    UIView *channels=HGAViewGetter(header,"tabContainer",header);
    return channels && !HGAHeaderShowsFinder(header) && (view==channels || [view isDescendantOfView:channels]);
}

static void HGAApplyChannelIndicators(UIView *categories,BOOL enabled) {
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
    HGASetHidden(cell,home && HGATopChannelHidden(title));

    BOOL history=NO;
    if(mine && [@[@"全部",@"已看完",@"未看完"] containsObject:title ?: @""]) {
        UIView *categories=HGAAncestorNamed(cell,@"FQRJXCategoryTitleView");
        // The native history filters are in a nested horizontal row with the
        // filter entrance sharing its toolbar. The top Mine tabs lack it.
        history=HGAHistoryFilter(categories.superview.superview)!=nil;
    }
    HGAApplyPageChrome(cell,history);
}

static void HGAApplyNativePageChrome(UIView *view,NSString *name,BOOL enabled) {
    if([name isEqual:@"FQRJXCategoryTitleCell"]) {
        HGAApplyChannelOrHistoryTitle(view,enabled);
    } else if([name isEqual:@"FQRJXCategoryTitleView"]) {
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
        NSString *title=filter ? @"筛选" : HGAPageChromeLabel(view,"nameLabel");
        BOOL finder=enabled && HGAppearanceFlag(@"player.finderGlass") &&
            [@[@"筛选",@"排行榜",@"新剧",@"预约"] containsObject:title ?: @""] && HGAIsFinderView(view);
        HGASetPageChromeCover(view,filter ? "backgroundView" : "bgImgView",finder);
        HGAApplyPageChrome(view,finder);
    }
}

static void HGAApplyFinderChrome(UIViewController *controller) {
    UIView *root=controller.view;
    if(!HGAppearanceEnabled() || !HGAppearanceFlag(@"player.finderGlass") || !root.window) {
        HGAApplyPageChromeTargets(root,nil);
        return;
    }
    BOOL theater=[controller isKindOfClass:NSClassFromString(@"FQTheaterViewController")];
    UIView *search=HGAViewGetter(controller,"searchView",root) ?: HGAViewGetter(controller,"searchButtonView",root);
    // The search background and its cue/gesture view are siblings in the host.
    UIView *field=HGAViewGetter(controller,"searchBgView",root) ?: search;
    UIView *recognition=HGAViewGetter(controller,"imageSearchContainer",root) ?:
        HGAViewGetter(controller,"normalImageSearchView",root);
    UIView *assistant=HGAViewGetter(controller,theater ? "aiSearchEntranceView" : "searchAiButtonView",root);
    NSMutableArray *targets=[NSMutableArray array];
    if(field)[targets addObject:field];
    if(recognition)[targets addObject:recognition];
    if(assistant)[targets addObject:assistant];
    HGAApplyPageChromeTargets(root,targets);
}
