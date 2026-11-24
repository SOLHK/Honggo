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

static void HGAApplyFinderMaterial(UIView *target,CGRect frame) {
    HGAApplyPageChromeMaterial(target,frame,HGAFinderMaterial());
    if(!UIAccessibilityIsReduceTransparencyEnabled()) {
        BOOL dark=target.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark;
        HGASetGlassOverlay(target,[UIColor colorWithWhite:dark?0.11:0.95 alpha:dark?0.18:0.07]);
    }
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
static BOOL HGAReflowFinderRecognition(UIView *owner,UIView *recognition,UIView *assistant,BOOL hideAI,BOOL glass) {
    NSHashTable<UIView *> *moved=objc_getAssociatedObject(owner,&HGAFinderMovedKey);
    UIView *previous=moved.anyObject;
    if(previous && previous!=recognition)HGARestoreGeometry(previous);
    CGFloat offset=0;
    HGAViewState *state=HGAState(recognition,NO);
    if((hideAI || glass) && HGAVisible(recognition) && recognition.superview && assistant.superview && assistant.alpha>=0.01) {
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

static void HGAApplyFinderSearch(UIView *owner,UIView *field,UIView *recognition,UIView *assistant,
    UIView *recognitionContent,UIView *divider,BOOL glass) {
    if(!field && !recognition && !assistant) {
        HGAReflowFinderRecognition(owner,nil,nil,NO,NO);
        HGAHidePageChromeTarget(owner,@"finderAI",nil);
        HGAHidePageChromeTarget(owner,@"finderRecognitionDivider",nil);
        HGAUpdatePageChromeTargets(owner,nil);
        return;
    }
    HGAViewState *assistantState=HGAState(assistant,NO);
    BOOL hostShowsAI=assistantState.visibility && assistant.hidden==assistantState.appliedHidden ?
        !assistantState.sourceHidden : HGAVisible(assistant);
    BOOL hideAI=assistant && !HGAppearanceFlag(@"player.finderAI");
    BOOL reflow=HGAReflowFinderRecognition(owner,recognition,hostShowsAI?assistant:nil,hideAI,glass);
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
        CGFloat edge=CGRectGetMinX(button)-7;
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
        CGRect ink=CGRectUnion([recognitionLabel convertRect:recognitionLabel.bounds toView:recognitionContent],
            [recognitionIcon convertRect:recognitionIcon.bounds toView:recognitionContent]);
        CGFloat offset=CGRectGetMidX(recognitionContent.bounds)-CGRectGetMidX(ink);
        HGASetGeometry(recognitionContent,separate && CGRectGetWidth(ink)<CGRectGetWidth(recognition.bounds) &&
            fabs(offset)<=12 ? offset : 0,0,1);
    }
    for(UIView *target in targets) {
        CGRect frame=target==field ? searchFrame : target.bounds;
        if(separate && recognition.superview==field) {
            // The nested theatre search uses a 38pt host row. Bring the edge
            // of its glass inside the host and align it with the 36pt AI disc.
            if(target==field)frame=CGRectInset(frame,1,1);
            if(target==recognition)frame=CGRectInset(frame,0,1);
        }
        if(target==assistant) {
            CGFloat side=MIN(frame.size.width,frame.size.height);
            frame=CGRectInset(frame,(frame.size.width-side)/2,(frame.size.height-side)/2);
        }
        HGAApplyFinderMaterial(target,frame);
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
        HGAApplyFinderSearch(header,nil,nil,nil,nil,nil,NO);
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
    HGAApplyFinderSearch(header,field,recognition,assistant,nil,divider,glass);
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

static char HGAFinderShortcutIndexesKey;
static IMP HGAOriginalGoldenPositionSize;

static CGSize HGAFinderShortcutSize(id cell,SEL cmd,UICollectionView *collection,UICollectionViewLayout *layout,NSIndexPath *path) {
    CGSize size=((CGSize (*)(id,SEL,id,id,id))HGAOriginalGoldenPositionSize)(cell,cmd,collection,layout,path);
    NSIndexSet *hidden=objc_getAssociatedObject(cell,&HGAFinderShortcutIndexesKey);
    if(path.section==0 && [hidden containsIndex:path.item] && collection==HGReadObjectGetter(cell,"collectionView"))
        size.width=0.01;
    return size;
}

static void HGAInstallFinderShortcutHook(void) {
    if(HGAOriginalGoldenPositionSize)return;
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
static _Thread_local BOOL HGAMineSnapshotInFlight;
static BOOL HGAMineFilterInstalled;

static NSArray *HGAMineFunctionItems(id owner,SEL cmd) {
    NSArray *items=((id (*)(id,SEL))HGAOriginalMineFunctionItems)(owner,cmd);
    if(!HGAMineSnapshotInFlight || !HGAppearanceEnabled() || ![items isKindOfClass:NSArray.class])return items;
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

static void HGAInstallMineShortcutHooks(void) {
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

static char HGAMineShortcutMaskKey;
static void HGAApplyMineKMPChrome(UIViewController *controller) {
    UIView *root=controller.viewIfLoaded;
    BOOL enabled=HGAppearanceEnabled() && root.window;
    if(HGAMineFilterInstalled) {
        id depend=HGReadObjectGetter(controller,"minePageKMPDepend");
        Class type=NSClassFromString(@"SSMinePageKMPDepend");
        SEL notify=sel_registerName("notifyFunctionDataChanged");
        Method method=class_getInstanceMethod(object_getClass(depend),notify);
        if([depend isKindOfClass:type] && method && !strcmp(method_getTypeEncoding(method),"v16@0:8")) {
            NSUInteger mask=0;
            NSArray *keys=HGAMineShortcuts();
            if(HGAppearanceEnabled())for(NSUInteger index=0;index<keys.count;index++)
                if(HGAppearanceFlag([@"mine.hide.shortcut." stringByAppendingString:keys[index]]))mask|=1UL<<index;
            NSNumber *previous=objc_getAssociatedObject(depend,&HGAMineShortcutMaskKey);
            if(mask!=previous.unsignedIntegerValue) {
                objc_setAssociatedObject(depend,&HGAMineShortcutMaskKey,@(mask),OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                __weak id weakDepend=depend;
                dispatch_async(dispatch_get_main_queue(), ^{
                    id liveDepend=weakDepend;
                    if(liveDepend)((void (*)(id,SEL))objc_msgSend)(liveDepend,notify);
                });
            }
        }
    }
    UIView *titleBar=enabled && HGAppearanceFlag(@"player.mineGlass") ?
        HGAViewGetter(HGReadObjectGetter(controller,"minePageKMPHandle"),"titleBarView",root) : nil;
    NSMutableArray<UIView *> *targets=[NSMutableArray array];
    for(UIView *node in HGADescendants(titleBar,28)) {
        if(![NSStringFromClass(node.class) isEqual:@"DanceUICompose.ComposeRenderingUIViewImpl"] ||
            !HGAVisible(node))continue;
        CGFloat edge=CGRectGetMaxX(node.bounds);
        for(UIView *item in node.subviews) {
            CGRect rect=item.frame;
            if([NSStringFromClass(item.class) isEqual:@"DanceUI._InheritedView"] && HGAVisible(item) &&
                fabs(rect.size.width-24)<1 && fabs(rect.size.height-24)<1 &&
                CGRectGetMinX(rect)>edge-100 && CGRectGetMaxX(rect)<=edge-8)
                [targets addObject:item];
        }
        if(targets.count==2 &&
            fabs(CGRectGetMidY(targets[0].frame)-CGRectGetMidY(targets[1].frame))<1)break;
        [targets removeAllObjects];
    }
    HGAUpdatePageChromeTargets(root,targets);
    for(UIView *target in targets) {
        CGFloat enlargement=target.clipsToBounds?0:6;
        HGAApplyFinderMaterial(target,CGRectInset(target.bounds,-enlargement,-enlargement));
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

static void HGAApplyChannelIndicators(UIView *categories,BOOL enabled) {
    HGAUpdateFinderCategories(categories,enabled);
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
    BOOL finder=enabled && HGAIsFinderChannel(cell);
    BOOL mine=enabled && HGAppearanceFlag(@"player.mineGlass") && HGAAncestorNamed(cell,@"SSMyUser637TabsView");
    NSString *title=home || mine || finder ? HGAPageChromeLabel(cell,"titleLabel") : nil;
    if((home || finder) && !title.length) {
        id modelTitle=HGReadObjectGetter(HGReadObjectGetter(cell,"cellModel"),"title");
        if([modelTitle isKindOfClass:NSString.class])title=modelTitle;
    }
    HGASetHidden(cell,(home && HGATopChannelHidden(title)) || (finder && HGAFinderCategoryHidden(title)));

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
        NSString *key=HGAMineShortcutKey(title);
        BOOL hide=enabled && key && HGAppearanceFlag([@"mine.hide.shortcut." stringByAppendingString:key]);
        HGASetHidden(view,hide);
        HGAApplyPageChrome(view,enabled && !hide && HGAppearanceFlag(@"player.mineGlass") && key);
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
    } else if([name isEqual:@"SSVideoDistributionGoldenPositionCell"]) {
        HGAUpdateFinderShortcuts(view,enabled);
    } else if([name isEqual:@"FQFeedTabFilterPanelEntranceView"] || [name isEqual:@"SSBookStoreEntranceView"] ||
        [name isEqual:@"SSBookStoreKingKongBannerView"] || [name isEqual:@"SSVideoDistributionGoldenPositionSubCell"]) {
        BOOL filter=[name isEqual:@"FQFeedTabFilterPanelEntranceView"];
        BOOL golden=[name isEqual:@"SSVideoDistributionGoldenPositionSubCell"];
        BOOL scoped=enabled && HGAIsFinderView(view);
        NSString *key=filter ? @"filter" : nil;
        if(scoped && !filter) {
            NSString *title=HGAPageChromeLabel(view,golden?"titleLabel":"nameLabel");
            if(!title.length) {
                id modelTitle=HGReadObjectGetter(HGReadObjectGetter(view,golden?"itemData":"model"),golden?"text":"name");
                if([modelTitle isKindOfClass:NSString.class])title=modelTitle;
            }
            key=HGAFinderShortcutKey(title);
        }
        BOOL hide=scoped && key && HGAppearanceFlag([@"finder.hide.shortcut." stringByAppendingString:key]);
        HGASetHidden(view,hide);
        BOOL finder=scoped && key && !hide && HGAppearanceFlag(@"player.finderGlass") && HGAVisible(view);
        HGASetPageChromeCover(view,filter?"backgroundView":golden?"backgroundImageView":"bgImgView",finder);
        UIView *target=golden?HGAViewGetter(view,"innerContentView",view) ?: view:view;
        HGAApplyFinderMaterial(target,finder ? CGRectInset(target.bounds,1,0) : CGRectZero);
        UIView *icon=finder ? HGAViewGetter(view,filter ? "icon" : "iconImgView",view) : nil;
        HGAApplyPageChromeIcons(view,icon ? @[icon] : nil);
    }
}

static void HGAApplyFinderChrome(UIViewController *controller) {
    UIView *root=controller.view;
    BOOL theater=[controller isKindOfClass:NSClassFromString(@"FQTheaterViewController")];
    NSArray<NSString *> *aiArt=@[@"aiSearchEntrancePlaceholderView",@"aiSearchEntranceImageView",
        @"aiSearchEntranceLottieFirstFrameView",@"aiSearchEntranceLottieView"];
    UIView *categories=HGAppearanceEnabled() && root.window && HGAppearanceFlag(@"finder.hide.category.row") ?
        HGAViewGetter(controller,"tabContainer",root) : nil;
    HGAHidePageChromeTarget(root,@"finderCategories",categories);
    if(!HGAppearanceEnabled() || !root.window) {
        UIView *recognitionContent=HGAViewGetter(controller,"normalImageSearchView",root);
        HGARestoreGeometry(recognitionContent);
        HGAApplyFinderSearch(root,nil,nil,nil,nil,nil,NO);
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
    HGAApplyFinderSearch(root,field,recognition,assistant,recognitionContent,divider,glass);
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
