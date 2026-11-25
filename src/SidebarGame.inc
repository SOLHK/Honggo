// 7.4.5.33 uses these lists for both sizing and selecting sidebar cells.
// Keep the host's cached lists intact so an already-created drawer can switch
// back without a new response from the server.
typedef struct {
    const char *owner;
    const char *getter;
    NSString * __unsafe_unretained component;
    BOOL installed;
} HGSidebarList;
static HGSidebarList HGSidebarLists[] = {
    {"SSVideoSeriesFeedSideBarViewController","cellList",@"kSSVideoSeriesFeedSideBarGameCenterCell"},
    {"SSVideoSeriesFeedSideBarV725ViewController","cellList",@"kSSVideoSeriesFeedSideBarGameCenterV725Cell"},
    {"SSMyUser621SideBarViewController","componentsLayout",@"SSMyUser621SideBarGameContainerCell"},
    {"SSVideoSeriesFeedSideBarViewController","functionModelList",nil},
};
static NSHashTable<UIViewController *> *HGSidebarControllers;

// 621 sizes its shortcut container with models.count, then configures each
// row from four parallel arrays. Read the original model/title getters so all
// four projections omit precisely the same original indices.
typedef NSArray *(*HGSidebarGetter)(id,SEL);
static HGSidebarGetter HGProfileGetters[4];
static BOOL HGProfileShortcutsReady;

static NSArray *HGVisibleProfileColumn(UIViewController *drawer, NSArray *source, NSUInteger column) {
    if (!HGProfileShortcutsReady || !HGIsEnabled(HGSidebarGameCenter) ||
        ![source isKindOfClass:NSArray.class] || !source.count) return source;
    NSArray *models=column==0 ? source : HGProfileGetters[0](drawer,sel_registerName("models"));
    NSArray *titles=column==1 ? source : HGProfileGetters[1](drawer,sel_registerName("functionEntryTitles"));
    if (![models isKindOfClass:NSArray.class]) return source;
    NSUInteger titleCount=[titles isKindOfClass:NSArray.class] ? titles.count : 0;
    NSMutableIndexSet *hidden=nil;
    for (NSUInteger i=0;i<MIN(models.count,source.count);i++) {
        id title=i<titleCount ? titles[i] : nil;
        if (![title isKindOfClass:NSString.class] || ![title length])
            title=HGReadObjectGetter(models[i],"titleString");
        if (![@"游戏中心" isEqual:title]) continue;
        if (!hidden) hidden=[NSMutableIndexSet indexSet];
        [hidden addIndex:i];
    }
    if (!hidden) return source;
    NSMutableArray *visible=[source mutableCopy];
    [visible removeObjectsAtIndexes:hidden];
    return visible.copy;
}

static void HGInstallProfileShortcutColumns(void) {
    if (HGProfileShortcutsReady) return;
    Class cls=objc_lookUpClass("SSMyUser621SideBarViewController");
    const char *names[]={"models","functionEntryTitles","functionEntryIcons","functionEntryDarkIcons"};
    for (NSUInteger i=0;i<4;i++)
        if (!HGAdMethodMatches(cls,sel_registerName(names[i]),"@16@0:8")) return;
    for (NSUInteger i=0;i<4;i++) {
        if (HGProfileGetters[i]) continue;
        SEL selector=sel_registerName(names[i]);
        __block HGSidebarGetter original=(HGSidebarGetter)class_getMethodImplementation(cls,selector);
        IMP hook=imp_implementationWithBlock(^NSArray *(UIViewController *drawer) {
            NSArray *source=original(drawer,selector);
            if (NSThread.isMainThread) [HGSidebarControllers addObject:drawer];
            return HGVisibleProfileColumn(drawer,source,i);
        });
        IMP previous=HGInstallVerifiedHook(cls,selector,"@16@0:8",hook,@"sidebar");
        if (!previous) { imp_removeBlock(hook); return; }
        HGProfileGetters[i]=original=(HGSidebarGetter)previous;
    }
    HGProfileShortcutsReady=YES;
}

static NSArray *HGSidebarVisibleList(NSArray *source, NSString *component) {
    if (![source isKindOfClass:NSArray.class] || !HGIsEnabled(HGSidebarGameCenter)) return source;
    if (component && ![source containsObject:component]) return source;
    NSMutableArray *visible = nil;
    for (NSUInteger i=0; i<source.count; i++) {
        id item=source[i];
        BOOL game=component ? [component isEqual:item] :
            [item isKindOfClass:objc_lookUpClass("SSVideoSeriesFeedSideBarFunctionItemModel")] &&
            [@"游戏中心" isEqual:HGReadObjectGetter(item,"titleString")];
        if (game && !visible) visible=[[source subarrayWithRange:NSMakeRange(0,i)] mutableCopy];
        if (!game && visible) [visible addObject:item];
    }
    return visible ? visible.copy : source;
}

static void HGInstallSidebarLists(void) {
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        HGSidebarControllers=[NSHashTable weakObjectsHashTable];
        [NSNotificationCenter.defaultCenter addObserverForName:HGSidebarGameChanged object:nil
            queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification *note) {
                for (UIViewController *drawer in HGSidebarControllers.allObjects) {
                    if (!drawer.isViewLoaded) continue;
                    id collection=HGReadObjectGetter(drawer,"collectionView");
                    if ([collection isKindOfClass:UICollectionView.class]) [collection reloadData];
                }
            }];
    });
    HGInstallProfileShortcutColumns();
    for (NSUInteger i=0; i<sizeof(HGSidebarLists)/sizeof(*HGSidebarLists); i++) {
        HGSidebarList *list=&HGSidebarLists[i];
        if (list->installed) continue;
        Class cls=objc_lookUpClass(list->owner);
        SEL sel=sel_registerName(list->getter);
        if (!HGAdMethodMatches(cls,sel,"@16@0:8")) continue;
        NSArray *(*original)(id,SEL)=(void *)class_getMethodImplementation(cls,sel);
        IMP hook=imp_implementationWithBlock(^NSArray *(UIViewController *drawer) {
            NSArray *source=original(drawer,sel);
            if ([NSThread isMainThread]) [HGSidebarControllers addObject:drawer];
            return HGSidebarVisibleList(source,list->component);
        });
        list->installed=HGInstallVerifiedHook(cls,sel,"@16@0:8",hook,@"sidebar")!=NULL;
        if (!list->installed) imp_removeBlock(hook);
    }
}
