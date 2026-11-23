#import "SettingsTransition.h"
#import "Community.h"

@interface HGABlurSlider : UISlider
@property(nonatomic,weak) UILabel *valueLabel;
@property(nonatomic,weak) UIStepper *stepper;
@end
@implementation HGABlurSlider
- (void)accessibilityIncrement {
    self.value=HGBlurPercent(self.value+1.0);
    [self sendActionsForControlEvents:UIControlEventValueChanged];
}
- (void)accessibilityDecrement {
    self.value=HGBlurPercent(self.value-1.0);
    [self sendActionsForControlEvents:UIControlEventValueChanged];
}
@end

static char HGABlurSliderAssocKey;

@interface HGAAppearanceController : UITableViewController <UISearchResultsUpdating,UISearchControllerDelegate,PHPickerViewControllerDelegate,UIDocumentPickerDelegate,UIFontPickerViewControllerDelegate,UIColorPickerViewControllerDelegate>
@property(nonatomic,copy) NSString *group;
@property(nonatomic,copy) NSArray<NSDictionary *> *rows;
@property(nonatomic,strong) UISearchController *search;
@property(nonatomic,copy) dispatch_block_t pendingAction;
@property(nonatomic,copy) NSString *assetKey;
@property(nonatomic,copy) NSString *colorKey;
@property(nonatomic,copy) NSString *documentPurpose;
@property(nonatomic) BOOL busy;
@end

static NSDictionary *HGATool(NSString *key,NSString *title,NSString *detail) {
    return detail.length ? @{@"key":key,@"title":title,@"detail":detail,@"kind":@"action"} :
        @{@"key":key,@"title":title,@"kind":@"action"};
}
static NSDictionary *HGAAppearanceItem(NSString *key) {
    for(NSDictionary *def in HGAppearanceCatalog())if([def[@"key"] isEqual:key])return def;
    return nil;
}
static NSString *HGAAppearanceNormalizeSearch(NSString *text) {
    NSString *simple=[text stringByApplyingTransform:@"Traditional-Simplified" reverse:NO];
    NSString *norm=[[(simple ?: text) lowercaseString] stringByFoldingWithOptions:NSDiacriticInsensitiveSearch|NSWidthInsensitiveSearch
        locale:[NSLocale localeWithLocaleIdentifier:@"zh_CN"]];
    for(NSArray<NSString *> *pair in @[
        @[@"dock栏",@"底栏"],@[@"dock",@"底栏"],@[@"底部栏",@"底栏"],@[@"tabbar",@"底栏"],@[@"导航栏",@"底栏"],
        @[@"液态玻璃",@"玻璃"],@[@"毛玻璃",@"模糊"],@[@"磨砂",@"模糊"],
        @[@"黑夜",@"深色"],@[@"暗黑",@"深色"],@[@"白天",@"浅色"],@[@"日间",@"浅色"],
        @[@"剧评",@"评论"],@[@"触感",@"振动"],@[@"震动",@"振动"],
        @[@"折叠",@"收缩"],@[@"收起",@"收缩"],@[@"1080p",@"清晰度"],@[@"画质",@"清晰度"],
        @[@"保存",@"导出"],@[@"下载",@"导出"]]) {
        norm=[norm stringByReplacingOccurrencesOfString:pair[0] withString:pair[1]];
    }
    return norm;
}
static NSString *HGAAppearancePageForGroup(NSString *group) {
    if(!group.length)return @"appearance.root";
    if([group isEqual:@"player"] || [group isEqual:@"pages.hub"] || [group isEqual:@"pages.home"])return @"drama";
    return group;
}
static NSString *HGARoleTitleForKey(NSString *role) {
    NSUInteger index=[HGAppearanceRoles() indexOfObject:role];
    return index!=NSNotFound ? HGAppearanceRoleTitles()[index] : role;
}
static NSString *HGAAppearancePageTitle(NSString *page) {
    if([page isEqual:@"nav"])return @"页面与导航";
    if([page isEqual:@"dock"])return @"底栏与材质";
    if([page isEqual:@"dock.geometry"])return @"底栏尺寸与圆角";
    if([page isEqual:@"dock.colors"])return @"底栏配色";
    if([page isEqual:@"nav.detached"])return @"独立圆形模块";
    if([page isEqual:@"nav.tabs"])return @"底栏入口";
    if([page isEqual:@"dock.behavior"])return @"底栏交互与收缩";
    if([page isEqual:@"drama"])return @"短剧";
    if([page isEqual:@"drama.top"])return @"顶部分类与搜索";
    if([page isEqual:@"pages.home.topHide"])return @"顶部栏元素隐藏";
    if([page isEqual:@"drama.right"])return @"右侧互动按钮";
    if([page isEqual:@"drama.playback"])return @"播放设置与手势";
    if([page isEqual:@"pages.detail"])return @"播放详情与浮层";
    if([page isEqual:@"pages.layout"])return @"播放页控件布局";
    if([page hasPrefix:@"layout."])return HGARoleTitleForKey([page substringFromIndex:7]);
    if([page isEqual:@"comment"])return @"评论区净化";
    if([page isEqual:@"pages.finder"])return @"找剧与剧场";
    if([page isEqual:@"pages.mine"])return @"我的页面净化";
    if([page isEqual:@"panels"])return @"弹出面板玻璃";
    if([page isEqual:@"icons"])return @"自定义图标";
    if([page hasPrefix:@"icons."])return [HGARoleTitleForKey([page substringFromIndex:6]) stringByAppendingString:@"图标"];
    if([page isEqual:@"fonts"])return @"字体与字号";
    return @"外观";
}
static NSString *HGAAppearanceKeyLocation(NSString *key) {
    if([key isEqual:@"enabled"])return @"外观";
    if([key hasPrefix:@"dock.width"] || [key hasPrefix:@"dock.height"] || [key hasPrefix:@"dock.lift"] ||
       [key hasPrefix:@"dock.radius"] || [key hasPrefix:@"dock.iconSize"] || [key isEqual:@"dock.geometry.enabled"])
        return @"页面与导航 › 底栏与材质 › 尺寸与圆角";
    if([key isEqual:@"dock.tint"] || [key isEqual:@"dock.selected"] || [key isEqual:@"dock.normal"])
        return @"页面与导航 › 底栏与材质 › 配色";
    if([key hasPrefix:@"dock.extra"] || [key isEqual:@"dock.action"])
        return @"页面与导航 › 独立圆形模块";
    if([key hasPrefix:@"dock.hide."])return @"页面与导航 › 底栏入口";
    if([key isEqual:@"dock.haptics"] || [key hasPrefix:@"dock.collapse"] || [key isEqual:@"dock.autoCollapse"] || [key isEqual:@"dock.anchor"])
        return @"页面与导航 › 底栏交互与收缩";
    if([key hasPrefix:@"dock."] || [key isEqual:@"glass.darkTint"])return @"页面与导航 › 底栏与材质";
    if([key isEqual:@"player.headerGlass"] || [key isEqual:@"header.blur"])return @"短剧 › 顶部分类与搜索";
    if([key hasPrefix:@"layout.channels.hide."])return @"短剧 › 顶部分类与搜索 › 隐藏元素";
    if([key isEqual:@"player.outlineActions"] || [key isEqual:@"player.rightGlass"])return @"短剧 › 右侧互动按钮";
    if([key hasPrefix:@"icons."])return @"短剧 › 右侧互动按钮 › 自定义图标";
    if([key isEqual:@"player.defaultMaxQuality"] || [key isEqual:@"player.fullscreen"] ||
       [key isEqual:@"player.pinchClean"] || [key isEqual:@"player.pinchHaptic"] ||
       [key isEqual:@"player.doubleTapComment"] || [key isEqual:@"player.hideExitCollectAlert"])
        return @"短剧 › 播放设置与手势";
    if([key hasPrefix:@"comment."] || [key isEqual:@"panels.comment"])return @"评论区净化";
    if([key isEqual:@"player.finderGlass"] || [key hasPrefix:@"layout.finder.hide."])return @"找剧与剧场";
    if([key isEqual:@"player.mineGlass"])return @"我的页面净化";
    if([key hasPrefix:@"panels."])return @"短剧 › 播放详情与浮层 › 弹出面板";
    if([key hasPrefix:@"layout."])return @"短剧 › 播放详情与浮层 › 控件布局";
    if([key hasPrefix:@"fonts."])return @"外观 › 字体与字号";
    return @"短剧 › 播放详情与浮层";
}
static BOOL HGAIsBlurPercentKey(NSString *key) {
    return [key isEqual:@"dock.blur"] || [key isEqual:@"header.blur"] || [key isEqual:@"comment.blur"];
}

@implementation HGAAppearanceController
- (void)viewDidLoad {
    [super viewDidLoad];
    NSString *page=HGAAppearancePageForGroup(self.group);
    self.title=HGAAppearancePageTitle(page);
    self.tableView.accessibilityIdentifier=@"honggo.appearance.settings";
    self.tableView.keyboardDismissMode=UIScrollViewKeyboardDismissModeOnDrag;
    self.tableView.rowHeight=UITableViewAutomaticDimension;
    self.tableView.estimatedRowHeight=54;
    self.search=[[UISearchController alloc] initWithSearchResultsController:nil];
    self.search.searchResultsUpdater=self;self.search.delegate=self;
    self.search.obscuresBackgroundDuringPresentation=NO;
    self.search.searchBar.placeholder=@"搜索外观、底栏、短剧、评论区或字体";
    self.navigationItem.searchController=self.search;
    self.navigationItem.hidesSearchBarWhenScrolling=NO;
    self.definesPresentationContext=YES;
    if(self.navigationController.viewControllers.firstObject==self)
        self.navigationItem.rightBarButtonItem=[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemDone target:self action:@selector(done)];
    [self reloadRows];
}
- (void)viewWillAppear:(BOOL)animated {[super viewWillAppear:animated];[self reloadRows];}
- (void)done {
    UIViewController *presenter=self.navigationController.presentingViewController;
    [self afterSearch:^{[presenter dismissViewControllerAnimated:YES completion:nil];}];
}
- (void)afterSearch:(dispatch_block_t)action {
    if(self.search.active || self.search.isBeingDismissed || self.presentedViewController==self.search) {
        self.pendingAction=action;
        if(!self.search.isBeingDismissed) HGAfterViewTransition(self.search,^{
            if(self.pendingAction)self.search.active=NO;
        });
    } else if(!self.presentedViewController) HGAfterViewTransition(self,^{
        if(self.viewIfLoaded.window && !self.presentedViewController &&
            !self.isBeingDismissed && !self.navigationController.isBeingDismissed)action();
    });
}
- (void)didDismissSearchController:(UISearchController *)searchController {
    HGAfterViewTransition(self,^{
        dispatch_block_t action=self.pendingAction;self.pendingAction=nil;
        if(action && self.viewIfLoaded.window && !self.presentedViewController &&
            !self.isBeingDismissed && !self.navigationController.isBeingDismissed)action();
    });
}
- (void)updateSearchResultsForSearchController:(UISearchController *)controller {[self reloadRows];}
- (void)addKeys:(NSArray<NSString *> *)keys into:(NSMutableArray *)rows {
    for(NSString *key in keys) {
        NSDictionary *item=HGAAppearanceItem(key);
        if(item)[rows addObject:item];
    }
}
- (void)reloadRows {
    NSString *query=[self.search.searchBar.text ?: @"" stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
    NSString *page=HGAAppearancePageForGroup(self.group);
    NSMutableArray *rows=[NSMutableArray array];
    if(query.length) {
        NSString *normQuery=HGAAppearanceNormalizeSearch(query);
        for(NSDictionary *def in HGAppearanceCatalog()) {
            NSString *key=def[@"key"];
            if([key hasPrefix:@"icons.comment.selected."] || [key hasPrefix:@"icons.share.selected."])continue;
            NSString *haystack=HGAAppearanceNormalizeSearch([NSString stringWithFormat:@"%@ %@ %@",
                def[@"title"],def[@"key"],HGAAppearanceKeyLocation(key)]);
            BOOL match=YES;
            for(NSString *word in [normQuery componentsSeparatedByCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet]) {
                if(word.length && [haystack rangeOfString:word].location==NSNotFound){match=NO;break;}
            }
            if(match)[rows addObject:def];
        }
        if(HGCommunityMatchesQuery(query))
            [rows addObject:HGATool(@"@community",@"TG 更新频道 · @Naloiafeed",@"打开频道或复制地址")];
    } else if([page isEqual:@"nav"]) {
        [rows addObject:HGATool(@"@group.dock",@"底栏与材质",nil)];
        [rows addObject:HGATool(@"@group.nav.detached",@"独立圆形模块",nil)];
        [rows addObject:HGATool(@"@group.nav.tabs",@"底栏入口",nil)];
        [rows addObject:HGATool(@"@group.dock.behavior",@"底栏交互与收缩",nil)];
        [rows addObject:HGATool(@"@settings.controls",@"启动期系统玻璃开关",nil)];
    } else if([page isEqual:@"dock"]) {
        [self addKeys:@[@"dock.enabled",@"dock.material",@"dock.blur",@"glass.darkTint",@"dock.content"] into:rows];
        [rows addObject:HGATool(@"@group.dock.geometry",@"底栏尺寸与圆角",nil)];
        [rows addObject:HGATool(@"@group.dock.colors",@"底栏配色",nil)];
    } else if([page isEqual:@"dock.geometry"]) {
        [self addKeys:@[@"dock.geometry.enabled"] into:rows];
        if(HGAppearanceFlag(@"dock.geometry.enabled"))
            [self addKeys:@[@"dock.width",@"dock.height",@"dock.lift",@"dock.radius",@"dock.iconSize"] into:rows];
    } else if([page isEqual:@"dock.colors"]) {
        [self addKeys:@[@"dock.tint",@"dock.selected",@"dock.normal"] into:rows];
    } else if([page isEqual:@"nav.detached"]) {
        [self addKeys:@[@"dock.extra",@"dock.action",@"dock.extraSize"] into:rows];
    } else if([page isEqual:@"nav.tabs"]) {
        [self addKeys:@[@"dock.hide.home",@"dock.hide.theater",@"dock.hide.store",@"dock.hide.mine"] into:rows];
    } else if([page isEqual:@"dock.behavior"]) {
        [self addKeys:@[@"dock.haptics",@"dock.collapse",@"dock.autoCollapse",@"dock.anchor"] into:rows];
    } else if([page isEqual:@"drama"]) {
        [rows addObject:HGATool(@"@group.drama.top",@"顶部分类与搜索",nil)];
        [rows addObject:HGATool(@"@group.drama.right",@"右侧互动按钮",nil)];
        [rows addObject:HGATool(@"@group.drama.playback",@"播放设置与手势",nil)];
        [rows addObject:HGATool(@"@group.pages.detail",@"播放详情与浮层",nil)];
    } else if([page isEqual:@"drama.top"]) {
        [self addKeys:@[@"player.headerGlass",@"header.blur"] into:rows];
        [rows addObject:HGATool(@"@group.pages.home.topHide",@"顶部栏元素隐藏",nil)];
    } else if([page isEqual:@"pages.home.topHide"]) {
        [self addKeys:@[@"layout.channels.hide.sidebar",@"layout.channels.hide.follow",@"layout.channels.hide.comic",
            @"layout.channels.hide.live",@"layout.channels.hide.recommend",@"layout.channels.hide.search"] into:rows];
    } else if([page isEqual:@"drama.right"]) {
        [self addKeys:@[@"player.outlineActions",@"player.rightGlass"] into:rows];
        [rows addObject:HGATool(@"@group.icons",@"自定义图标与大小",nil)];
    } else if([page isEqual:@"drama.playback"]) {
        [self addKeys:@[@"player.defaultMaxQuality",@"player.fullscreen",
            @"player.pinchClean",@"player.pinchHaptic",
            @"player.doubleTapComment",@"player.hideExitCollectAlert"] into:rows];
    } else if([page isEqual:@"pages.detail"]) {
        [self addKeys:@[@"player.episodeGlass",@"player.controlsGlass",@"player.separateProgress",
            @"player.seriesPillGlass",@"layout.seriesPill.hidden",
            @"player.goodsPillGlass",@"layout.goodsPill.hidden"] into:rows];
        [rows addObject:HGATool(@"@group.pages.layout",@"播放页控件布局与隐藏",nil)];
        [rows addObject:HGATool(@"@group.panels",@"弹出面板玻璃",nil)];
    } else if([page isEqual:@"pages.layout"]) {
        for(NSString *role in HGAppearanceRoles()) {
            [rows addObject:HGATool([@"@group.layout." stringByAppendingString:role],HGARoleTitleForKey(role),nil)];
        }
    } else if([page hasPrefix:@"layout."]) {
        NSString *role=[page substringFromIndex:7];
        NSString *prefix=[@"layout." stringByAppendingString:role];
        [self addKeys:@[[prefix stringByAppendingString:@".hidden"],
            [prefix stringByAppendingString:@".x"],
            [prefix stringByAppendingString:@".y"],
            [prefix stringByAppendingString:@".scale"]] into:rows];
    } else if([page isEqual:@"comment"]) {
        [self addKeys:@[@"panels.comment",@"comment.material",@"comment.appearance",@"comment.blur",
            @"comment.composer",@"comment.noVideoShift",
            @"comment.hideActivityBanner",@"comment.hideSearchEntrance"] into:rows];
    } else if([page isEqual:@"pages.finder"]) {
        [self addKeys:@[@"player.finderGlass",@"layout.finder.hide.channels",@"layout.finder.hide.shortcuts"] into:rows];
    } else if([page isEqual:@"pages.mine"]) {
        [self addKeys:@[@"player.mineGlass"] into:rows];
    } else if([page isEqual:@"panels"]) {
        [self addKeys:@[@"panels.episodes",@"panels.comment",@"panels.share",@"panels.longpress",
            @"panels.speed",@"panels.material",@"panels.opacity"] into:rows];
    } else if([page isEqual:@"icons"]) {
        [self addKeys:@[@"icons.enabled",@"icons.preset",@"icons.size"] into:rows];
        for(NSString *role in @[@"like",@"collect",@"comment",@"share"]) {
            [rows addObject:HGATool([@"@group.icons." stringByAppendingString:role],
                [HGARoleTitleForKey(role) stringByAppendingString:@"图标"],nil)];
        }
    } else if([page hasPrefix:@"icons."]) {
        NSString *role=[page substringFromIndex:6];
        NSMutableArray *keys=[NSMutableArray arrayWithObjects:
            [NSString stringWithFormat:@"icons.%@.normal.symbol",role],
            [NSString stringWithFormat:@"icons.%@.normal.asset",role],nil];
        if([role isEqual:@"like"] || [role isEqual:@"collect"]) {
            [keys addObject:[NSString stringWithFormat:@"icons.%@.selected.symbol",role]];
            [keys addObject:[NSString stringWithFormat:@"icons.%@.selected.asset",role]];
        }
        [self addKeys:keys into:rows];
    } else if([page isEqual:@"fonts"]) {
        [self addKeys:@[@"fonts.enabled",@"fonts.scale",@"fonts.name",@"fonts.asset"] into:rows];
        [rows addObject:HGATool(@"@fonts.reset",@"恢复宿主原字体与字号",nil)];
    } else {
        // appearance.root (外观总览)
        [self addKeys:@[@"enabled",@"player.material",@"glass.darkTint"] into:rows];
        [rows addObject:HGATool(@"@preset.glass",@"应用「液态玻璃」预设",nil)];
        [rows addObject:HGATool(@"@preset.focus",@"应用「专注播放」预设",nil)];
        [rows addObject:HGATool(@"@group.fonts",@"字体与字号",nil)];
        [rows addObject:HGATool(@"@group.dock",@"底栏与材质",nil)];
        [rows addObject:HGATool(@"@group.drama",@"短剧外观与手势",nil)];
        [rows addObject:HGATool(@"@group.comment",@"评论区净化",nil)];
        [rows addObject:HGATool(@"@export",@"导出外观参数",nil)];
        [rows addObject:HGATool(@"@import",@"从文件恢复外观参数",nil)];
        [rows addObject:HGATool(@"@reset",@"恢复全部外观默认值",nil)];
        if(HGCommunityMatchesQuery(@""))
            [rows addObject:HGATool(@"@community",@"TG 更新频道 · @Naloiafeed",nil)];
    }
    self.rows=rows;
    UILabel *empty=nil;
    if(!rows.count) {
        empty=[UILabel new];
        empty.text=@"没有匹配的设置，可尝试功能名称或关键词";
        empty.textAlignment=NSTextAlignmentCenter;
        empty.textColor=UIColor.secondaryLabelColor;
        empty.font=[UIFont systemFontOfSize:14];
    }
    self.tableView.backgroundView=empty;
    [self.tableView reloadData];
}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    (void)tableView;(void)section;
    return self.rows.count;
}
- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)path {
    (void)tableView;
    NSDictionary *row=self.rows[path.row];
    NSString *key=row[@"key"];
    if(HGAIsBlurPercentKey(key)) {
        BOOL searching=[self.search.searchBar.text stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet].length>0;
        return searching ? 106.0 : 86.0;
    }
    return UITableViewAutomaticDimension;
}
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    (void)tableView;(void)section;
    NSString *query=[self.search.searchBar.text ?: @"" stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
    if(query.length)return @"搜索结果";
    return HGAAppearancePageTitle(HGAAppearancePageForGroup(self.group));
}
- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
    (void)tableView;(void)section;
    if(self.search.searchBar.text.length)return nil;
    NSString *page=HGAAppearancePageForGroup(self.group);
    if([page isEqual:@"comment"])return @"评论区材质、深浅外观、模糊程度与发评论输入框可独立搭配使用。";
    if([page isEqual:@"dock"] || [page isEqual:@"nav"])return @"使用 iOS 26 原生液态玻璃与实时高斯模糊；开启“底栏尺寸与圆角”后可精细调节宽、高与圆角。";
    if([page isEqual:@"nav.detached"])return @"将常用底部入口或设置独立到右侧圆形玻璃按钮中，避免与主胶囊重复显示。";
    if([page isEqual:@"drama.top"])return @"顶部「三」菜单、频道胶囊与搜索按钮独立呈现液态玻璃，隐藏频道时自动收紧居中。";
    if([page isEqual:@"drama.right"])return @"开启“透明圆角描边图标”后采用无底盘透明描边图标；关闭时可搭配液态玻璃圆盘或自定义图标。";
    if([page isEqual:@"drama.playback"])return @"双指张开进入纯净清屏，双指捏合恢复；双击视频打开评论区；横屏铺满仅作用于横屏播放横向视频。";
    if([page isEqual:@"fonts"])return @"仅修改已适配界面原生文字，不影响视频字幕、输入框和密码框。字体导入上限 50 MiB。";
    if([page isEqual:@"pages.finder"])return @"可分别隐藏找剧顶部频道栏与筛选／排行榜／新剧／预约快捷入口，不影响首页频道。";
    if([page isEqual:@"appearance.root"])return @"首次开启原生液态玻璃后请完全退出红果再重新打开。参数导出仅含开关与数值，不含本地图片和字体文件。";
    return nil;
}
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)path {
    (void)tableView;
    NSDictionary *row=self.rows[path.row];
    NSString *kind=row[@"kind"],*key=row[@"key"];
    NSString *query=[self.search.searchBar.text ?: @"" stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
    BOOL searching=query.length>0;
    NSString *location=searching && ![key hasPrefix:@"@"] ? HGAAppearanceKeyLocation(key) : nil;

    // FanqieX-unified 0..100% Blur Slider + 1% Stepper cell
    if([kind isEqual:@"number"] && HGAIsBlurPercentKey(key)) {
        UITableViewCell *cell=[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil];
        cell.selectionStyle=UITableViewCellSelectionStyleNone;
        cell.accessibilityIdentifier=[@"honggo.appearance.setting." stringByAppendingString:key];
        UILabel *title=[UILabel new];
        title.text=row[@"title"];
        title.font=[UIFont preferredFontForTextStyle:UIFontTextStyleBody];
        title.adjustsFontSizeToFitWidth=YES;
        title.minimumScaleFactor=0.75;
        UILabel *value=[UILabel new];
        value.font=[UIFont monospacedDigitSystemFontOfSize:14 weight:UIFontWeightRegular];
        value.textColor=UIColor.secondaryLabelColor;
        value.textAlignment=NSTextAlignmentRight;
        HGABlurSlider *slider=[HGABlurSlider new];
        slider.minimumValue=0;slider.maximumValue=100;
        NSInteger pct=HGBlurPercent(HGAppearanceNumber(key));
        slider.value=(float)pct;
        slider.accessibilityIdentifier=key;
        slider.accessibilityLabel=row[@"title"];
        value.text=[NSString stringWithFormat:@"%ld%%",(long)pct];
        slider.accessibilityValue=value.text;
        slider.valueLabel=value;
        UIStepper *stepper=[UIStepper new];
        stepper.minimumValue=0;stepper.maximumValue=100;stepper.stepValue=1;
        stepper.value=pct;stepper.wraps=NO;
        stepper.accessibilityLabel=[row[@"title"] stringByAppendingString:@"，每次调整1%"];
        stepper.accessibilityValue=value.text;
        slider.stepper=stepper;
        objc_setAssociatedObject(stepper,&HGABlurSliderAssocKey,slider,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        [stepper addTarget:self action:@selector(blurStepperChanged:) forControlEvents:UIControlEventValueChanged];
        [slider addTarget:self action:@selector(blurSliderChanged:) forControlEvents:UIControlEventValueChanged];
        [slider addTarget:self action:@selector(blurSliderEnded:) forControlEvents:UIControlEventTouchUpInside|UIControlEventTouchUpOutside|UIControlEventTouchCancel];
        UIView *content=cell.contentView;
        for(UIView *v in @[title,value,slider,stepper]) {
            v.translatesAutoresizingMaskIntoConstraints=NO;
            [content addSubview:v];
        }
        UILayoutGuide *m=content.layoutMarginsGuide;
        [NSLayoutConstraint activateConstraints:@[
            [title.leadingAnchor constraintEqualToAnchor:m.leadingAnchor],
            [title.centerYAnchor constraintEqualToAnchor:stepper.centerYAnchor],
            [title.trailingAnchor constraintLessThanOrEqualToAnchor:value.leadingAnchor constant:-8],
            [value.widthAnchor constraintEqualToConstant:44],
            [value.trailingAnchor constraintEqualToAnchor:stepper.leadingAnchor constant:-8],
            [value.centerYAnchor constraintEqualToAnchor:stepper.centerYAnchor],
            [stepper.trailingAnchor constraintEqualToAnchor:m.trailingAnchor],
            [stepper.topAnchor constraintEqualToAnchor:content.topAnchor constant:6],
            [slider.leadingAnchor constraintEqualToAnchor:m.leadingAnchor],
            [slider.trailingAnchor constraintEqualToAnchor:m.trailingAnchor],
            [slider.topAnchor constraintEqualToAnchor:content.topAnchor constant:36],
            [slider.heightAnchor constraintEqualToConstant:44]
        ]];
        if(location.length) {
            UILabel *pathLbl=[UILabel new];
            pathLbl.translatesAutoresizingMaskIntoConstraints=NO;
            pathLbl.font=[UIFont systemFontOfSize:11];
            pathLbl.textColor=UIColor.secondaryLabelColor;
            pathLbl.text=location;
            [content addSubview:pathLbl];
            [NSLayoutConstraint activateConstraints:@[
                [pathLbl.leadingAnchor constraintEqualToAnchor:m.leadingAnchor],
                [pathLbl.trailingAnchor constraintEqualToAnchor:m.trailingAnchor],
                [pathLbl.topAnchor constraintEqualToAnchor:slider.bottomAnchor constant:2]
            ]];
        }
        return cell;
    }

    // FanqieX-unified clean Value1 cell for choices, colors, assets, fonts
    if([kind isEqual:@"choice"] || [kind isEqual:@"color"] || [kind isEqual:@"image"] ||
       [kind isEqual:@"font"] || [kind isEqual:@"fontfile"] || [kind isEqual:@"symbol"]) {
        UITableViewCell *cell=[[UITableViewCell alloc] initWithStyle:searching ? UITableViewCellStyleSubtitle : UITableViewCellStyleValue1 reuseIdentifier:nil];
        cell.textLabel.text=row[@"title"];
        cell.textLabel.font=[UIFont preferredFontForTextStyle:UIFontTextStyleBody];
        cell.textLabel.numberOfLines=0;
        cell.detailTextLabel.font=[UIFont preferredFontForTextStyle:searching ? UIFontTextStyleCaption1 : UIFontTextStyleBody];
        cell.detailTextLabel.textColor=UIColor.secondaryLabelColor;
        cell.accessoryType=UITableViewCellAccessoryDisclosureIndicator;
        cell.accessibilityIdentifier=[@"honggo.appearance.setting." stringByAppendingString:key];
        NSString *value=HGAppearanceString(key);
        NSString *label=@"默认";
        if([kind isEqual:@"choice"]) {
            NSUInteger i=[row[@"choices"] indexOfObject:value];
            label=i!=NSNotFound ? row[@"labels"][i] : @"默认";
        } else if([kind isEqual:@"image"] || [kind isEqual:@"fontfile"]) {
            label=value.length ? @"已导入" : @"未选择";
        } else {
            label=value.length ? value : @"默认";
        }
        cell.detailTextLabel.text=location.length ? [NSString stringWithFormat:@"%@ · %@",label,location] : label;
        return cell;
    }

    // Action / Folder rows: crisp single-line Default cell when no detail/search
    if([kind isEqual:@"action"]) {
        NSString *detail=row[@"detail"];
        UITableViewCellStyle style=detail.length ? UITableViewCellStyleSubtitle : UITableViewCellStyleDefault;
        UITableViewCell *cell=[[UITableViewCell alloc] initWithStyle:style reuseIdentifier:nil];
        cell.textLabel.text=row[@"title"];
        cell.textLabel.font=[UIFont preferredFontForTextStyle:UIFontTextStyleBody];
        cell.textLabel.numberOfLines=0;
        if(detail.length) {
            cell.detailTextLabel.text=detail;
            cell.detailTextLabel.font=[UIFont preferredFontForTextStyle:UIFontTextStyleCaption1];
            cell.detailTextLabel.textColor=UIColor.secondaryLabelColor;
            cell.detailTextLabel.numberOfLines=0;
        }
        cell.accessoryType=UITableViewCellAccessoryDisclosureIndicator;
        cell.accessibilityIdentifier=[@"honggo.appearance.setting." stringByAppendingString:key];
        if([key isEqual:@"@reset"])cell.textLabel.textColor=UIColor.systemRedColor;
        return cell;
    }

    // Bool switch & standard numeric sliders (dock.geometry / layout.* / icons.size / fonts.scale)
    UITableViewCell *cell=[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:nil];
    cell.textLabel.text=row[@"title"];
    cell.textLabel.numberOfLines=0;
    cell.detailTextLabel.numberOfLines=0;
    cell.textLabel.font=[UIFont preferredFontForTextStyle:UIFontTextStyleBody];
    cell.detailTextLabel.font=[UIFont preferredFontForTextStyle:UIFontTextStyleCaption1];
    cell.detailTextLabel.textColor=UIColor.secondaryLabelColor;
    cell.accessibilityIdentifier=[@"honggo.appearance.setting." stringByAppendingString:key];
    if([kind isEqual:@"bool"]) {
        UISwitch *toggle=[UISwitch new];
        toggle.on=HGAppearanceFlag(key);
        toggle.accessibilityIdentifier=key;
        toggle.accessibilityLabel=row[@"title"];
        [toggle addTarget:self action:@selector(toggle:) forControlEvents:UIControlEventValueChanged];
        cell.accessoryView=toggle;
        cell.selectionStyle=UITableViewCellSelectionStyleNone;
        cell.detailTextLabel.text=location;
    } else if([kind isEqual:@"number"]) {
        UISlider *slider=[[UISlider alloc] initWithFrame:CGRectMake(0,0,125,44)];
        slider.minimumValue=[row[@"min"] floatValue];
        slider.maximumValue=[row[@"max"] floatValue];
        slider.value=HGAppearanceNumber(key);
        slider.accessibilityIdentifier=key;
        slider.accessibilityLabel=row[@"title"];
        [slider addTarget:self action:@selector(slider:) forControlEvents:UIControlEventValueChanged];
        cell.accessoryView=slider;
        NSString *range=[NSString stringWithFormat:@"%.2f（%@～%@）",slider.value,row[@"min"],row[@"max"]];
        cell.detailTextLabel.text=location.length ? [NSString stringWithFormat:@"%@ · %@",range,location] : range;
        cell.selectionStyle=UITableViewCellSelectionStyleNone;
    }
    return cell;
}
- (void)message:(NSString *)title detail:(NSString *)detail {
    if(!self.viewIfLoaded.window || self.presentedViewController)return;
    UIAlertController *alert=[UIAlertController alertControllerWithTitle:title message:detail preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"好" style:UIAlertActionStyleDefault handler:nil]];
    [self presentViewController:alert animated:YES completion:nil];
}
- (void)save:(NSString *)key value:(id)value {
    NSError *error=nil;
    if(!HGAppearanceSet(key,value,&error))[self message:@"设置未更改" detail:error.localizedDescription];
}
- (void)blurStepperChanged:(UIStepper *)stepper {
    HGABlurSlider *slider=objc_getAssociatedObject(stepper,&HGABlurSliderAssocKey);
    slider.value=HGBlurPercent(stepper.value);
    [self blurSliderChanged:slider];
}
- (void)blurSliderEnded:(HGABlurSlider *)slider {
    NSInteger pct=HGBlurPercent(slider.value);
    [slider setValue:(float)pct animated:NO];
    [self blurSliderChanged:slider];
}
- (void)blurSliderChanged:(HGABlurSlider *)slider {
    NSString *key=slider.accessibilityIdentifier;
    if(!key.length)return;
    NSInteger pct=HGBlurPercent(slider.value);
    if(!slider.isTracking)[slider setValue:(float)pct animated:NO];
    slider.stepper.value=pct;
    NSString *text=[NSString stringWithFormat:@"%ld%%",(long)pct];
    slider.valueLabel.text=text;
    slider.accessibilityValue=text;
    slider.stepper.accessibilityValue=text;
    if(HGBlurPercent(HGAppearanceNumber(key))!=pct)[self save:key value:@(pct)];
}
- (void)toggle:(UISwitch *)toggle {
    NSString *key=toggle.accessibilityIdentifier;
    [self save:key value:@(toggle.on)];
    toggle.on=HGAppearanceFlag(key);
    if([key isEqual:@"dock.geometry.enabled"])[self reloadRows];
}
- (void)slider:(UISlider *)slider {
    NSString *key=slider.accessibilityIdentifier;
    double value=slider.maximumValue<=2 ? round(slider.value*100)/100 : round(slider.value);
    [self save:key value:@(value)];
    slider.value=HGAppearanceNumber(key);
    UIView *node=slider;
    while(node && ![node isKindOfClass:UITableViewCell.class])node=node.superview;
    ((UITableViewCell *)node).detailTextLabel.text=[NSString stringWithFormat:@"%.2f",value];
}
- (void)presentSheet:(UIAlertController *)sheet {
    sheet.popoverPresentationController.sourceView=self.view;
    sheet.popoverPresentationController.sourceRect=CGRectMake(CGRectGetMidX(self.view.bounds),CGRectGetMidY(self.view.bounds),1,1);
    [self presentViewController:sheet animated:YES completion:nil];
}
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)path {
    [tableView deselectRowAtIndexPath:path animated:YES];
    NSDictionary *row=self.rows[path.row];
    [self afterSearch:^{[self openRow:row];}];
}
- (void)confirm:(NSString *)title detail:(NSString *)detail action:(dispatch_block_t)action {
    UIAlertController *a=[UIAlertController alertControllerWithTitle:title message:detail preferredStyle:UIAlertControllerStyleAlert];
    [a addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:nil]];
    [a addAction:[UIAlertAction actionWithTitle:@"应用" style:UIAlertActionStyleDestructive handler:^(UIAlertAction *button){(void)button;action();[self reloadRows];}]];
    [self presentViewController:a animated:YES completion:nil];
}
- (void)openRow:(NSDictionary *)row {
    if(self.busy)return;
    NSString *key=row[@"key"],*kind=row[@"kind"];
    if([key isEqual:@"@community"]){HGShowCommunity(self);return;}
    if([key hasPrefix:@"@settings."]){[HGSettingsController openPath:[key substringFromIndex:10] fromController:self];return;}
    if([key hasPrefix:@"@group."]){HGOpenAppearance(self,[key substringFromIndex:7]);return;}
    if([key hasPrefix:@"@preset."]) {
        NSString *preset=[key substringFromIndex:8];
        [self confirm:row[@"title"] detail:@"会应用这套外观预设，不影响去广告开关。首次开启原生玻璃后请重启红果。"
            action:^{HGAppearanceApply(HGAppearancePreset(preset),nil);}];
        return;
    }
    if([key isEqual:@"@reset"]){[self confirm:@"恢复外观默认值？" detail:@"关闭外观定制并恢复宿主原貌，原有去广告设置不变。" action:^{HGAppearanceReset();}];return;}
    if([key isEqual:@"@fonts.reset"]){HGAppearanceApply(@{@"fonts.enabled":@NO,@"fonts.name":@"",@"fonts.asset":@"",@"fonts.scale":@1},nil);[self reloadRows];return;}
    if([key isEqual:@"@export"]){[self exportConfiguration];return;}
    if([key isEqual:@"@import"]){[self pickDocument:@"configuration"];return;}
    if([kind isEqual:@"choice"]) {
        UIAlertController *a=[UIAlertController alertControllerWithTitle:row[@"title"] message:nil preferredStyle:UIAlertControllerStyleActionSheet];
        NSArray *values=row[@"choices"],*labels=row[@"labels"];
        NSString *current=HGAppearanceString(key);
        for(NSUInteger i=0;i<values.count;i++) {
            BOOL selected=[values[i] isEqual:current];
            NSString *title=selected ? [@"✓ " stringByAppendingString:labels[i]] : labels[i];
            [a addAction:[UIAlertAction actionWithTitle:title style:UIAlertActionStyleDefault handler:^(UIAlertAction *button){
                (void)button;
                if([key isEqual:@"comment.material"]) {
                    BOOL isNative=[values[i] isEqual:@"native"];
                    NSMutableDictionary *patch=[NSMutableDictionary dictionaryWithDictionary:@{@"comment.material":values[i],@"panels.comment":@(!isNative)}];
                    if(!isNative)patch[@"enabled"]=@YES;
                    HGAppearanceApply(patch,nil);
                } else {
                    [self save:key value:values[i]];
                }
                [self reloadRows];
            }]];
        }
        [a addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:nil]];
        [self presentSheet:a];
        return;
    }
    if([kind isEqual:@"symbol"]) {
        UIAlertController *a=[UIAlertController alertControllerWithTitle:row[@"title"] message:@"输入系统图标名，如 heart.fill、star.fill。留空清除。" preferredStyle:UIAlertControllerStyleAlert];
        [a addTextFieldWithConfigurationHandler:^(UITextField *text){text.text=HGAppearanceString(key);text.autocorrectionType=UITextAutocorrectionTypeNo;text.autocapitalizationType=UITextAutocapitalizationTypeNone;}];
        [a addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:nil]];
        [a addAction:[UIAlertAction actionWithTitle:@"保存" style:UIAlertActionStyleDefault handler:^(UIAlertAction *button){
            (void)button;
            NSString *value=[a.textFields.firstObject.text ?: @"" stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
            if(value.length && ![UIImage systemImageNamed:value]) {dispatch_async(dispatch_get_main_queue(),^{[self message:@"图标不存在" detail:@"此系统没有该 SF Symbol，请检查名称。"];});return;}
            if(value.length) {
                NSError *error=nil;
                NSString *imageKey=[[key stringByDeletingPathExtension] stringByAppendingString:@".asset"];
                if(!HGAppearanceApply(@{key:value,imageKey:@""},&error))
                    dispatch_async(dispatch_get_main_queue(),^{[self message:@"设置未更改" detail:error.localizedDescription];});
            } else [self save:key value:value];
            [self reloadRows];
        }]];
        [self presentViewController:a animated:YES completion:nil];
        return;
    }
    if([kind isEqual:@"color"] || [kind isEqual:@"image"] || [kind isEqual:@"font"] || [kind isEqual:@"fontfile"]) {
        UIAlertController *a=[UIAlertController alertControllerWithTitle:row[@"title"] message:nil preferredStyle:UIAlertControllerStyleActionSheet];
        [a addAction:[UIAlertAction actionWithTitle:@"选择…" style:UIAlertActionStyleDefault handler:^(UIAlertAction *button){
            (void)button;
            [a dismissViewControllerAnimated:YES completion:^{[self launchPicker:row];}];
        }]];
        [a addAction:[UIAlertAction actionWithTitle:@"恢复默认／清除" style:UIAlertActionStyleDestructive handler:^(UIAlertAction *button){
            (void)button;
            if([kind isEqual:@"fontfile"])HGAppearanceApply(@{@"fonts.asset":@"",@"fonts.name":@""},nil);
            else [self save:key value:@""];
            [self reloadRows];
        }]];
        [a addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:nil]];
        [self presentSheet:a];
    }
}
- (void)launchPicker:(NSDictionary *)row {
    if(self.presentedViewController || !self.viewIfLoaded.window)return;
    NSString *key=row[@"key"],*kind=row[@"kind"];
    if([kind isEqual:@"color"]) {
        self.colorKey=key;UIColorPickerViewController *picker=[UIColorPickerViewController new];
        picker.delegate=self;picker.supportsAlpha=NO;picker.selectedColor=HGAppearanceColor(HGAppearanceString(key),UIColor.labelColor);
        [self presentViewController:picker animated:YES completion:nil];
    } else if([kind isEqual:@"image"]) {
        self.assetKey=key;PHPickerConfiguration *configuration=[PHPickerConfiguration new];
        configuration.filter=PHPickerFilter.imagesFilter;configuration.selectionLimit=1;
        PHPickerViewController *picker=[[PHPickerViewController alloc] initWithConfiguration:configuration];picker.delegate=self;
        [self presentViewController:picker animated:YES completion:nil];
    } else if([kind isEqual:@"font"]) {
        UIFontPickerViewControllerConfiguration *configuration=[UIFontPickerViewControllerConfiguration new];
        configuration.includeFaces=YES;
        UIFontPickerViewController *picker=[[UIFontPickerViewController alloc] initWithConfiguration:configuration];picker.delegate=self;
        [self presentViewController:picker animated:YES completion:nil];
    } else [self pickDocument:@"font"];
}
- (void)colorPickerViewController:(UIColorPickerViewController *)controller didSelectColor:(UIColor *)color continuously:(BOOL)continuously {
    (void)controller;(void)continuously;
    CGFloat r=0,g=0,b=0,a=0;
    if([[color resolvedColorWithTraitCollection:self.traitCollection] getRed:&r green:&g blue:&b alpha:&a]) {
        NSString *hex=[NSString stringWithFormat:@"#%02X%02X%02X",(unsigned)lround(r*255),(unsigned)lround(g*255),(unsigned)lround(b*255)];
        [self save:self.colorKey value:hex];
    }
}
- (void)colorPickerViewControllerDidFinish:(UIColorPickerViewController *)controller {(void)controller;[self reloadRows];}
- (void)fontPickerViewControllerDidPickFont:(UIFontPickerViewController *)controller {
    NSString *name=[controller.selectedFontDescriptor objectForKey:UIFontDescriptorNameAttribute];
    [controller dismissViewControllerAnimated:YES completion:^{
        if(name.length && [UIFont fontWithName:name size:16])HGAppearanceApply(@{@"fonts.name":name,@"fonts.asset":@""},nil);
        [self reloadRows];
    }];
}
- (void)fontPickerViewControllerDidCancel:(UIFontPickerViewController *)controller {[controller dismissViewControllerAnimated:YES completion:nil];}
- (void)picker:(PHPickerViewController *)picker didFinishPicking:(NSArray<PHPickerResult *> *)results {
    NSString *key=self.assetKey;self.assetKey=nil;
    [picker dismissViewControllerAnimated:YES completion:^{
        NSItemProvider *provider=results.firstObject.itemProvider;if(!provider)return;
        self.busy=YES;
        [provider loadFileRepresentationForTypeIdentifier:UTTypeImage.identifier completionHandler:^(NSURL *url,NSError *failure) {
            NSError *error=failure;
            NSData *data=url ? HGAReadFile(url,12*1024*1024,&error) : nil;
            NSString *token=data ? HGAImportImage(data,&error) : nil;
            dispatch_async(dispatch_get_main_queue(),^{
                self.busy=NO;
                if(token){[self save:key value:token];[self reloadRows];}
                else [self message:@"图片未导入" detail:error.localizedDescription ?: @"无法读取所选图片。"];
            });
        }];
    }];
}
- (void)pickDocument:(NSString *)purpose {
    self.documentPurpose=purpose;
    NSMutableArray<UTType *> *types=[NSMutableArray array];
    if([purpose isEqual:@"configuration"])[types addObject:UTTypeJSON];
    else for(NSString *extension in @[@"ttf",@"otf",@"ttc"]) {
        UTType *type=[UTType typeWithFilenameExtension:extension];if(type)[types addObject:type];
    }
    if(!types.count)[types addObject:UTTypeData];
    UIDocumentPickerViewController *picker=[[UIDocumentPickerViewController alloc] initForOpeningContentTypes:types asCopy:YES];
    picker.delegate=self;picker.allowsMultipleSelection=NO;
    [self presentViewController:picker animated:YES completion:nil];
}
- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls {
    NSURL *url=urls.firstObject;NSString *purpose=self.documentPurpose;self.documentPurpose=nil;if(!url)return;
    self.busy=YES;
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED,0),^{
        BOOL scope=[url startAccessingSecurityScopedResource];
        __block NSData *data=nil;__block NSError *readError=nil;NSError *coordinationError=nil;
        NSFileCoordinator *coordinator=[[NSFileCoordinator alloc] initWithFilePresenter:nil];
        [coordinator coordinateReadingItemAtURL:url options:0 error:&coordinationError byAccessor:^(NSURL *local){
            data=HGAReadFile(local,[purpose isEqual:@"font"]?50*1024*1024:128*1024,&readError);
        }];
        if(scope)[url stopAccessingSecurityScopedResource];
        NSError *error=coordinationError ?: readError;NSDictionary *configuration=nil;NSArray *fonts=nil;NSURL *stored=nil;
        if(data && !error && [purpose isEqual:@"font"]) {
            NSString *extension=url.pathExtension.lowercaseString;
            if(![@[@"ttf",@"otf",@"ttc"] containsObject:extension])error=HGAAssetError(@"仅支持 TTF、OTF 或 TTC 字体。");
            else {stored=HGAStoreAsset(data,extension,&error);if(stored)fonts=HGARegisterFont(stored,&error);}
        } else if(data && !error)configuration=HGAppearanceParse(data,&error);
        dispatch_async(dispatch_get_main_queue(),^{
            self.busy=NO;
            void (^finish)(void)=^{
                if(error){[self message:@"导入失败" detail:error.localizedDescription];return;}
                if(fonts.count) {
                    UIAlertController *choice=[UIAlertController alertControllerWithTitle:@"选择字体" message:@"导入完成；选择字形后再开启字体功能。" preferredStyle:UIAlertControllerStyleActionSheet];
                    for(NSString *name in fonts)[choice addAction:[UIAlertAction actionWithTitle:name style:UIAlertActionStyleDefault handler:^(UIAlertAction *a){
                        (void)a;
                        HGAppearanceApply(@{@"fonts.asset":stored.lastPathComponent,@"fonts.name":name},nil);[self reloadRows];
                    }]];
                    [choice addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:nil]];
                    [self presentSheet:choice];
                } else if(configuration) {
                    [self confirm:@"恢复外观参数？" detail:@"只修改文件中列出的 Honggo 外观设置。字体与图片需要在本机存在。" action:^{
                        NSError *applyError=nil;
                        if(!HGAppearanceApply(configuration,&applyError))dispatch_async(dispatch_get_main_queue(),^{[self message:@"未恢复" detail:applyError.localizedDescription];});
                    }];
                }
            };
            if(self.presentedViewController==controller)[controller dismissViewControllerAnimated:YES completion:finish];
            else if(controller.transitionCoordinator)[controller.transitionCoordinator animateAlongsideTransition:nil completion:^(id<UIViewControllerTransitionCoordinatorContext> context){(void)context;finish();}];
            else finish();
        });
    });
}
- (void)documentPickerWasCancelled:(UIDocumentPickerViewController *)controller{(void)controller;self.documentPurpose=nil;}
- (void)exportConfiguration {
    NSError *error=nil;NSData *data=HGAppearanceExport(&error);
    if(!data){[self message:@"导出失败" detail:error.localizedDescription];return;}
    self.busy=YES;
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY,0),^{
        NSURL *directory=[NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingPathComponent:NSUUID.UUID.UUIDString] isDirectory:YES];
        NSError *failure=nil;
        BOOL ok=[NSFileManager.defaultManager createDirectoryAtURL:directory withIntermediateDirectories:YES attributes:nil error:&failure];
        NSURL *url=[directory URLByAppendingPathComponent:@"Honggo-appearance.json"];
        if(ok)ok=[data writeToURL:url options:NSDataWritingAtomic error:&failure];
        dispatch_async(dispatch_get_main_queue(),^{
            self.busy=NO;
            if(!ok){[self message:@"导出失败" detail:failure.localizedDescription];return;}
            UIActivityViewController *share=[[UIActivityViewController alloc] initWithActivityItems:@[url] applicationActivities:nil];
            share.popoverPresentationController.sourceView=self.view;
            share.popoverPresentationController.sourceRect=CGRectMake(CGRectGetMidX(self.view.bounds),CGRectGetMidY(self.view.bounds),1,1);
            share.completionWithItemsHandler=^(UIActivityType type,BOOL completed,NSArray *items,NSError *shareError){
                (void)type;(void)completed;(void)items;(void)shareError;
                dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY,0),^{[NSFileManager.defaultManager removeItemAtURL:directory error:nil];});
            };
            if(self.viewIfLoaded.window && !self.presentedViewController)[self presentViewController:share animated:YES completion:nil];
        });
    });
}
@end

void HGOpenAppearance(UIViewController *presenter,NSString *group) {
    if(!NSThread.isMainThread || !presenter)return;
    UIViewController *current=presenter;
    for(NSUInteger i=0;i<16;i++) {
        if(current.presentedViewController && !current.presentedViewController.isBeingDismissed)current=current.presentedViewController;
        else if([current isKindOfClass:UINavigationController.class])current=[(UINavigationController *)current topViewController];
        else if([current isKindOfClass:UITabBarController.class])current=[(UITabBarController *)current selectedViewController];
        else break;
    }
    if(!current.viewIfLoaded.window || [current isKindOfClass:UIAlertController.class] || current.isBeingDismissed)return;
    if([current isKindOfClass:HGAAppearanceController.class] && !group)return;
    HGAAppearanceController *panel=[[HGAAppearanceController alloc] initWithStyle:UITableViewStyleInsetGrouped];
    panel.group=group;
    if([current isKindOfClass:HGSettingsController.class] || [current isKindOfClass:HGAAppearanceController.class])
        [current.navigationController pushViewController:panel animated:YES];
    else {
        UINavigationController *navigation=[[UINavigationController alloc] initWithRootViewController:panel];
        [current presentViewController:navigation animated:YES completion:nil];
    }
}
