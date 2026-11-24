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

static void HGAApplyPageChromeMaterial(UIView *target,CGRect frame,NSString *style) {
    if(HGAVisible(target) && !CGRectIsEmpty(frame)) {
        CGFloat radius=MIN(CGRectGetHeight(frame)/2,24);
        BOOL dark=target.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark;
        BOOL reduced=UIAccessibilityIsReduceTransparencyEnabled();
        HGAInstallGlassInRect(target,style,frame,radius,0);
        HGASetGlassOverlay(target,[UIColor colorWithWhite:reduced ? (dark ? 0.25 : 0.91) : (dark ? 0.85 : 0.45)
            alpha:reduced ? 1 : (dark ? 0.12 : 0.16)]);
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

static void HGAApplyPageChromeTargets(UIView *owner,NSArray<UIView *> *targets) {
    HGAUpdatePageChromeTargets(owner,targets);
    for(UIView *target in targets)HGAApplyPageChrome(target,YES);
}

static NSString *HGAFinderMaterial(void) {
    return [HGAppearanceString(@"player.material") isEqual:@"clear"] ? @"clear" : @"regular";
}

@interface HGAPageChromeIcon : NSObject
@property(nonatomic,strong) UIImage *sourceImage;
@property(nonatomic,strong) UIImage *templateImage;
@property(nonatomic,strong) UIColor *sourceTint;
@end
@implementation HGAPageChromeIcon @end
static char HGAPageChromeIconKey, HGAPageChromeIconsKey;

static void HGAApplyPageChromeIcons(UIView *owner,NSArray<UIView *> *candidates) {
    NSHashTable<UIImageView *> *previous=objc_getAssociatedObject(owner,&HGAPageChromeIconsKey);
    for(UIImageView *icon in previous) {
        if([candidates containsObject:icon])continue;
        HGAPageChromeIcon *state=objc_getAssociatedObject(icon,&HGAPageChromeIconKey);
        if(icon.image==state.templateImage)icon.image=state.sourceImage;
        if([icon.tintColor isEqual:UIColor.labelColor])icon.tintColor=state.sourceTint;
        objc_setAssociatedObject(icon,&HGAPageChromeIconKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    [previous removeAllObjects];
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
        if(![icon.tintColor isEqual:UIColor.labelColor]) {
            state.sourceTint=icon.tintColor;
            icon.tintColor=UIColor.labelColor;
        }
        if(icon.image!=state.templateImage)icon.image=state.templateImage;
    }
}

static void HGAApplyHistoryGlass(UIView *target,BOOL circle,BOOL selected,BOOL enabled) {
    if([target isKindOfClass:UICollectionViewCell.class])target=((UICollectionViewCell *)target).contentView;
    CGRect frame=enabled ? target.bounds : CGRectZero;
    if(enabled && !CGRectIsEmpty(frame)) {
        CGFloat height=MIN(30,frame.size.height);
        CGFloat width=frame.size.width;
        if(circle)width=height=target.clipsToBounds ? MIN(height,width) : 30;
        else if(width>=28 && width<=34 && !target.clipsToBounds)width+=4;
        else if(width>=40)width-=4;
        frame=CGRectInset(frame,(frame.size.width-width)/2,(frame.size.height-height)/2);
    }
    HGAApplyPageChromeMaterial(target,frame,HGAFinderMaterial());
    if(enabled && !UIAccessibilityIsReduceTransparencyEnabled()) {
        BOOL dark=target.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark;
        HGASetGlassOverlay(target,[UIColor colorWithWhite:dark ? 1 : 0.12 alpha:selected ? 0.23 : 0.12]);
    }
}

static char HGAFinderMovedKey;
static BOOL HGAReflowFinderRecognition(UIView *owner,UIView *recognition,UIView *assistant,BOOL hideAI) {
    NSHashTable<UIView *> *moved=objc_getAssociatedObject(owner,&HGAFinderMovedKey);
    UIView *previous=moved.anyObject;
    if(previous && previous!=recognition)HGARestoreGeometry(previous);
    CGFloat offset=0;
    HGAViewState *state=HGAState(recognition,NO);
    if(hideAI && HGAVisible(recognition) && recognition.superview && assistant.superview && assistant.alpha>=0.01) {
        // Measure the host slot before our translation. The native control and
        // its gesture move together inside the same hit-testing parent.
        CGRect button=recognition.frame;
        if(state.geometry && state.geometrySuperview==recognition.superview &&
            CGAffineTransformEqualToTransform(recognition.transform,state.appliedTransform))
            button.origin.x-=state.appliedTransform.tx-state.sourceTransform.tx;
        CGRect slot=[assistant convertRect:assistant.bounds toView:recognition.superview];
        CGFloat gap=CGRectGetMinX(slot)-CGRectGetMaxX(button);
        if(!CGRectIsEmpty(slot) && gap>=-2 && gap<24 && fabs(CGRectGetMidY(slot)-CGRectGetMidY(button))<8) {
            CGFloat edge=MIN(CGRectGetMaxX(slot),CGRectGetMaxX(recognition.superview.bounds));
            offset=MAX(0,edge-CGRectGetMaxX(button));
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

static void HGAApplyFinderSearch(UIView *owner,UIView *field,UIView *recognition,UIView *assistant,UIView *divider,BOOL glass) {
    if(!field && !recognition && !assistant) {
        HGAReflowFinderRecognition(owner,nil,nil,NO);
        HGAHidePageChromeTarget(owner,@"finderAI",nil);
        HGAHidePageChromeTarget(owner,@"finderRecognitionDivider",nil);
        HGAUpdatePageChromeTargets(owner,nil);
        return;
    }
    HGAViewState *assistantState=HGAState(assistant,NO);
    BOOL hostShowsAI=assistantState.visibility && assistant.hidden==assistantState.appliedHidden ?
        !assistantState.sourceHidden : HGAVisible(assistant);
    BOOL hideAI=assistant && !HGAppearanceFlag(@"player.finderAI");
    BOOL reflow=HGAReflowFinderRecognition(owner,recognition,assistant,hideAI && hostShowsAI);
    HGAHidePageChromeTarget(owner,@"finderAI",hideAI ? assistant : nil);
    NSMutableArray<UIView *> *targets=[NSMutableArray array];
    if(glass && field)[targets addObject:field];
    if(glass && recognition && recognition!=field)[targets addObject:recognition];
    if(glass && assistant && !hideAI)[targets addObject:assistant];
    HGAUpdatePageChromeTargets(owner,targets);

    CGRect searchFrame=field ? field.bounds : CGRectZero;
    BOOL separate=glass && HGAVisible(field) && HGAVisible(recognition) && recognition!=field;
    if(separate) {
        CGRect button=[recognition convertRect:recognition.bounds toView:field];
        CGFloat edge=CGRectGetMinX(button)-6;
        if(edge>CGRectGetMidX(searchFrame) &&
            (edge<CGRectGetMaxX(searchFrame) || (reflow && !field.clipsToBounds)))
            searchFrame.size.width=edge-CGRectGetMinX(searchFrame);
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
        BOOL dark=target.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark;
        if(!UIAccessibilityIsReduceTransparencyEnabled())
            HGASetGlassOverlay(target,[UIColor colorWithWhite:dark ? 0.06 : 0.97 alpha:dark ? 0.32 : 0.28]);
    }
}

static char HGAFinderAIInkKey;
static void HGAApplyFinderAIInk(UIView *owner,UIView *assistant,id artOwner,NSArray<NSString *> *artGetters) {
    UIImageView *glyph=objc_getAssociatedObject(owner,&HGAFinderAIInkKey);
    if(!HGAVisible(assistant))assistant=nil;
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
        HGAApplyFinderSearch(header,nil,nil,nil,nil,NO);
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
    HGAApplyFinderSearch(header,field,recognition,assistant,divider,glass);
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

static void HGAApplyChannelIndicators(UIView *categories,BOOL enabled) {
    id indicators=HGReadObjectGetter(categories,"indicators");
    if(![indicators isKindOfClass:NSArray.class] || ![indicators count])return;
    BOOL hide=NO;
    BOOL home=enabled && HGAIsHomeChannel(categories);
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
    if (home) HGAHeaderCategoryGeometryChanged(categories);
}

static void HGAApplyChannelOrHistoryTitle(UIView *cell,BOOL enabled) {
    BOOL home=enabled && HGAIsHomeChannel(cell);
    BOOL mine=enabled && HGAppearanceFlag(@"player.mineGlass") && HGAAncestorNamed(cell,@"SSMyUser637TabsView");
    NSString *title=home || mine ? HGAPageChromeLabel(cell,"titleLabel") : nil;
    if(home && !title.length) {
        id modelTitle=HGReadObjectGetter(HGReadObjectGetter(cell,"cellModel"),"title");
        if([modelTitle isKindOfClass:NSString.class])title=modelTitle;
    }
    HGASetHidden(cell,home && HGATopChannelHidden(title));

    BOOL history=NO;
    if(mine && [@[@"全部",@"已看完",@"未看完"] containsObject:title ?: @""]) {
        UIView *categories=HGAAncestorNamed(cell,@"FQRJXCategoryTitleView");
        // The native history filters are in a nested horizontal row with the
        // filter entrance sharing its toolbar. The top Mine tabs lack it.
        history=HGAHistoryFilter(categories.superview.superview)!=nil;
    }
    BOOL selected=NO;
    if(history && !HGABoolGetter(HGReadObjectGetter(cell,"cellModel"),@"isSelected",&selected))
        selected=[cell isKindOfClass:UICollectionViewCell.class] && ((UICollectionViewCell *)cell).selected;
    HGAApplyHistoryGlass(cell,NO,selected,history);
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
    } else if([view isKindOfClass:NSClassFromString(@"SSMyUser637TabsView")]) {
        UIView *search=HGAViewGetter(view,"searchBtn",view);
        BOOL mine=enabled && HGAppearanceFlag(@"player.mineGlass") && search;
        HGAUpdatePageChromeTargets(view,mine ? @[search] : nil);
        HGAApplyHistoryGlass(search,YES,NO,mine);
        UIImageView *icon=[search isKindOfClass:UIButton.class] ? ((UIButton *)search).imageView : nil;
        HGAApplyPageChromeIcons(view,mine && icon ? @[icon] : nil);
    } else if([view isKindOfClass:NSClassFromString(@"SSVideoFilterPanelEntranceView")]) {
        BOOL mine=enabled && HGAppearanceFlag(@"player.mineGlass") && HGAAncestorNamed(view,@"SSMyUser637TabsView");
        HGASetPageChromeCover(view,"backgroundView",mine);
        NSMutableArray *targets=[NSMutableArray array];
        if(mine) {
            [targets addObject:view];
            for(UIView *sibling in view.superview.subviews) {
                if(![sibling isKindOfClass:NSClassFromString(@"FQReaderSaaSBaseButton")] ||
                    ![sibling isKindOfClass:UIButton.class])continue;
                NSString *title=((UIButton *)sibling).currentTitle;
                if([title isEqual:@"编辑"] || [title isEqual:@"完成"]){[targets addObject:sibling];break;}
            }
        }
        HGAUpdatePageChromeTargets(view,targets);
        for(UIView *target in targets)HGAApplyHistoryGlass(target,NO,NO,YES);
        UIView *icon=mine ? HGAViewGetter(view,"icon",view) : nil;
        HGAApplyPageChromeIcons(view,icon ? @[icon] : nil);
    } else if([name isEqual:@"FQFeedTabFilterPanelEntranceView"] ||
        [name isEqual:@"SSBookStoreEntranceView"] || [name isEqual:@"SSBookStoreKingKongBannerView"]) {
        BOOL filter=[name isEqual:@"FQFeedTabFilterPanelEntranceView"];
        BOOL finder=enabled && HGAppearanceFlag(@"player.finderGlass") && HGAVisible(view) && HGAIsFinderView(view);
        if(finder && !filter) {
            NSString *title=HGAPageChromeLabel(view,"nameLabel");
            if(!title.length) {
                id modelTitle=HGReadObjectGetter(HGReadObjectGetter(view,"model"),"name");
                if([modelTitle isKindOfClass:NSString.class])title=modelTitle;
            }
            finder=[@[@"筛选",@"排行榜",@"新剧",@"预约"] containsObject:title ?: @""];
        }
        HGASetPageChromeCover(view,filter ? "backgroundView" : "bgImgView",finder);
        HGAApplyPageChromeMaterial(view,finder ? CGRectInset(view.bounds,1,0) : CGRectZero,HGAFinderMaterial());
        UIView *icon=finder ? HGAViewGetter(view,filter ? "icon" : "iconImgView",view) : nil;
        HGAApplyPageChromeIcons(view,icon ? @[icon] : nil);
    }
}

static void HGAApplyFinderChrome(UIViewController *controller) {
    UIView *root=controller.view;
    BOOL theater=[controller isKindOfClass:NSClassFromString(@"FQTheaterViewController")];
    NSArray<NSString *> *aiArt=@[@"aiSearchEntrancePlaceholderView",@"aiSearchEntranceImageView",
        @"aiSearchEntranceLottieFirstFrameView",@"aiSearchEntranceLottieView"];
    if(!HGAppearanceEnabled() || !root.window) {
        HGAApplyFinderSearch(root,nil,nil,nil,nil,NO);
        HGAApplyPageChromeIcons(root,nil);
        if(theater)HGAApplyFinderAIInk(root,nil,nil,aiArt);
        return;
    }
    UIView *search=HGAViewGetter(controller,"searchView",root) ?: HGAViewGetter(controller,"searchButtonView",root);
    // The search background and its cue/gesture view are siblings in the host.
    UIView *field=HGAViewGetter(controller,"searchBgView",root) ?: search;
    UIView *recognitionContent=HGAViewGetter(controller,"normalImageSearchView",root);
    UIView *recognition=HGAViewGetter(controller,"imageSearchContainer",root) ?: recognitionContent;
    UIView *assistant=HGAViewGetter(controller,theater ? "aiSearchEntranceView" : "searchAiButtonView",root);
    UIView *divider=recognition ? HGAViewGetter(recognitionContent,"separatorView",recognition) : nil;
    BOOL glass=HGAppearanceFlag(@"player.finderGlass");
    HGAApplyFinderSearch(root,field,recognition,assistant,divider,glass);
    NSMutableArray<UIView *> *icons=[NSMutableArray array];
    if(glass) {
        UIView *searchIcon=[search isKindOfClass:UIButton.class] ? ((UIButton *)search).imageView :
            (HGAViewGetter(search,"searchIcon",search) ?: HGAViewGetter(search,"iconImageView",search));
        if(!searchIcon) {
            // On the theatre host the cue row exposes the leading magnifier as
            // a plain image view, alongside a separate cue content view.
            for(UIView *child in search.subviews) {
                if([child isKindOfClass:UIImageView.class] && HGAVisible(child) &&
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
