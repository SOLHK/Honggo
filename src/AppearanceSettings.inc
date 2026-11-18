#import "SettingsTransition.h"
#import "Community.h"
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
static NSString *HGAAppearancePageForGroup(NSString *group) {
    if(!group.length)return @"appearance.root";
    if([group isEqual:@"player"])return @"pages.home";
    return group;
}
static NSString *HGARoleTitleForKey(NSString *role) {
    NSUInteger index=[HGAppearanceRoles() indexOfObject:role];
    return index!=NSNotFound ? HGAppearanceRoleTitles()[index] : role;
}
static NSString *HGAAppearancePageTitle(NSString *page) {
    if([page isEqual:@"dock"])return @"悬浮底栏设置";
    if([page isEqual:@"dock.geometry"])return @"底栏尺寸、高度与圆角";
    if([page isEqual:@"dock.colors"])return @"底栏配色定制";
    if([page isEqual:@"dock.behavior"])return @"收缩、独立按钮与标签隐藏";
    if([page isEqual:@"pages.hub"])return @"页面外观与净化";
    if([page isEqual:@"pages.home"])return @"首页与播放页";
    if([page isEqual:@"pages.home.topHide"])return @"首页顶部栏隐藏";
    if([page isEqual:@"pages.layout"])return @"播放页控件布局与隐藏";
    if([page hasPrefix:@"layout."])return [HGARoleTitleForKey([page substringFromIndex:7]) stringByAppendingString:@" · 控件调节"];
    if([page isEqual:@"pages.detail"])return @"播放详情页与全集";
    if([page isEqual:@"pages.finder"])return @"找剧／剧场页";
    if([page isEqual:@"pages.mine"])return @"我的页面";
    if([page isEqual:@"panels"])return @"弹出面板玻璃";
    if([page isEqual:@"icons"])return @"动作图标定制";
    if([page hasPrefix:@"icons."])return [HGARoleTitleForKey([page substringFromIndex:6]) stringByAppendingString:@" · 图标定制"];
    if([page isEqual:@"fonts"])return @"字体与字号";
    return @"外观定制";
}
static NSString *HGAAppearancePageHeader(NSString *page) {
    if([page isEqual:@"dock"])return @"当前调节页面：全局底部导航栏（首页／剧场／商城／我的）";
    if([page isEqual:@"dock.geometry"])return @"当前调节页面：全局悬浮底栏（尺寸、悬浮上移、圆角与图标大小）";
    if([page isEqual:@"dock.colors"])return @"当前调节页面：全局悬浮底栏（底色、选中与未选中颜色）";
    if([page isEqual:@"dock.behavior"])return @"当前调节页面：全局悬浮底栏（触感、收缩手势、独立圆按钮与导航隐藏）";
    if([page isEqual:@"pages.hub"])return @"当前调节页面：按红果页面分类进入对应外观与净化调节";
    if([page isEqual:@"pages.home"])return @"当前调节页面：首页推荐流、竖屏播放页与横屏播放器";
    if([page isEqual:@"pages.home.topHide"])return @"当前调节页面：首页顶部栏（菜单、关注、漫剧、真人剧、推荐、搜索）";
    if([page isEqual:@"pages.layout"])return @"当前调节页面：短剧播放页浮层控件列表（点击进入四级单项调节）";
    if([page hasPrefix:@"layout."])return [NSString stringWithFormat:@"当前调节页面：短剧播放页 · %@（隐藏／位移／缩放）",
        HGARoleTitleForKey([page substringFromIndex:7])];
    if([page isEqual:@"pages.detail"])return @"当前调节页面：短剧播放详情页（选集入口、清屏、剧好物与弹出面板）";
    if([page isEqual:@"pages.finder"])return @"当前调节页面：找剧／剧场页（搜索框、识剧、AI、顶部分类与快捷入口）";
    if([page isEqual:@"pages.mine"])return @"当前调节页面：我的（个人主页控件、作品/历史/收藏/点赞/预约、筛选与编辑）";
    if([page isEqual:@"panels"])return @"当前调节页面：播放页弹出面板（选集／评论／分享／长按／倍速）";
    if([page isEqual:@"icons"])return @"当前调节页面：播放页右侧互动栏图标（点赞／收藏／评论／分享）";
    if([page hasPrefix:@"icons."])return [NSString stringWithFormat:@"当前调节页面：播放页右侧 · %@图标定制",
        HGARoleTitleForKey([page substringFromIndex:6])];
    if([page isEqual:@"fonts"])return @"当前调节页面：已适配播放页、弹出面板与底栏的原生文字";
    return @"当前调节页面：外观定制总览（全插件外观入口与预设）";
}
static NSString *HGAAppearanceKeyLocation(NSString *key) {
    if([key isEqual:@"enabled"])return @"外观定制总览";
    if([key hasPrefix:@"dock.width"] || [key hasPrefix:@"dock.height"] || [key hasPrefix:@"dock.lift"] ||
        [key hasPrefix:@"dock.radius"] || [key hasPrefix:@"dock.iconSize"] || [key isEqual:@"dock.geometry.enabled"])
        return @"悬浮底栏 → 尺寸、高度与圆角";
    if([key isEqual:@"dock.tint"] || [key isEqual:@"dock.selected"] || [key isEqual:@"dock.normal"])
        return @"悬浮底栏 → 配色定制";
    if([key hasPrefix:@"dock.haptics"] || [key hasPrefix:@"dock.collapse"] || [key hasPrefix:@"dock.autoCollapse"] ||
        [key hasPrefix:@"dock.anchor"] || [key hasPrefix:@"dock.extra"] || [key hasPrefix:@"dock.action"] || [key hasPrefix:@"dock.hide."])
        return @"悬浮底栏 → 交互与标签隐藏";
    if([key hasPrefix:@"dock."])return @"悬浮底栏";
    if([key hasPrefix:@"layout.channels.hide."])return @"首页与播放页 → 首页顶部栏隐藏";
    if([key isEqual:@"player.finderGlass"] || [key hasPrefix:@"layout.finder.hide."])return @"找剧／剧场页";
    if([key isEqual:@"player.mineGlass"])return @"我的页面";
    if([key isEqual:@"player.episodeGlass"] || [key isEqual:@"player.controlsGlass"] ||
        [key isEqual:@"player.separateProgress"] || [key isEqual:@"player.goodsPillGlass"] ||
        [key isEqual:@"layout.goodsPill.hidden"])return @"播放详情页与全集";
    if([key hasPrefix:@"panels."])return @"弹出面板玻璃";
    if([key hasPrefix:@"layout."])return @"首页与播放页 → 控件布局与隐藏";
    if([key hasPrefix:@"icons."])return @"动作图标定制";
    if([key hasPrefix:@"fonts."])return @"字体与字号";
    return @"首页与播放页";
}
static NSString *HGAAppearanceKeyNote(NSString *key) {
    if([key isEqual:@"player.fullscreen"])return @"仅横屏播放横向视频时铺满，竖屏保持原比例";
    if([key isEqual:@"dock.geometry.enabled"])return @"打开后在下方展开宽度、高度、上移距离、圆角与图标大小滑块";
    if([key isEqual:@"player.finderGlass"])return @"优化浅色模式液态玻璃通透度，清理搜索／识剧／AI 灰色底板（图四上框）";
    if([key isEqual:@"layout.finder.hide.channels"])return @"隐藏找剧页面顶部「找剧／真人剧／社区…」频道栏（图四下框）";
    if([key isEqual:@"layout.finder.hide.shortcuts"])return @"隐藏找剧页面「筛选／排行榜／新剧／预约」快捷入口栏（图四下框）";
    if([key isEqual:@"player.mineGlass"])return @"覆盖 🌙/≡、编辑资料、功能宫格、作品/历史/收藏/点赞/预约、筛选与编辑（图五）；新版 Compose 上半区需补充宿主日志";
    if([key isEqual:@"player.defaultMaxQuality"])return @"图三需求 · 需上传所提诊断 JSON 日志后接入宿主清晰度切换逻辑，当前仅保存偏好";
    if([key isEqual:@"player.seriesPillGlass"] || [key isEqual:@"layout.seriesPill.hidden"])
        return @"图一需求 · 需上传所提诊断 JSON 日志以定位系列剧标签子控件，当前仅保存偏好";
    if([key isEqual:@"player.goodsPillGlass"] || [key isEqual:@"layout.goodsPill.hidden"])
        return @"图二需求 · 需上传所提诊断 JSON 日志以定位剧好物子控件，当前仅保存偏好";
    return nil;
}

@implementation HGAAppearanceController
- (void)viewDidLoad {
    [super viewDidLoad];
    NSString *page=HGAAppearancePageForGroup(self.group);
    self.title=HGAAppearancePageTitle(page);
    self.tableView.accessibilityIdentifier=@"honggo.appearance.settings";
    self.tableView.rowHeight=UITableViewAutomaticDimension;self.tableView.estimatedRowHeight=68;
    self.search=[[UISearchController alloc] initWithSearchResultsController:nil];
    self.search.searchResultsUpdater=self;self.search.delegate=self;
    self.search.obscuresBackgroundDuringPresentation=NO;
    self.search.searchBar.placeholder=@"搜索底栏、页面、图标、字体或控件名";
    self.navigationItem.searchController=self.search;self.navigationItem.hidesSearchBarWhenScrolling=NO;
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
        if(!self.search.isBeingDismissed) HGAfterViewTransition(self.search, ^{
            if(self.pendingAction)self.search.active=NO;
        });
    }else if(!self.presentedViewController) HGAfterViewTransition(self, ^{
        if(self.viewIfLoaded.window && !self.presentedViewController &&
            !self.isBeingDismissed && !self.navigationController.isBeingDismissed)action();
    });
}
- (void)didDismissSearchController:(UISearchController *)searchController {
    HGAfterViewTransition(self, ^{
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
        for(NSDictionary *def in HGAppearanceCatalog()) {
            NSString *key=def[@"key"];
            if([key hasPrefix:@"icons.comment.selected."] || [key hasPrefix:@"icons.share.selected."])continue;
            NSString *haystack=[NSString stringWithFormat:@"%@ %@",def[@"title"],def[@"key"]];
            BOOL match=YES;
            for(NSString *word in [query componentsSeparatedByCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet])
                if(word.length && [haystack rangeOfString:word options:NSCaseInsensitiveSearch|NSDiacriticInsensitiveSearch].location==NSNotFound){match=NO;break;}
            if(match)[rows addObject:def];
        }
        if(HGCommunityMatchesQuery(query))
            [rows addObject:HGATool(@"@community",@"TG 更新频道 · @Naloiafeed",@"打开频道或复制地址")];
    } else if([page isEqual:@"appearance.root"]) {
        [rows addObject:HGAppearanceCatalog().firstObject];
        [rows addObject:HGATool(@"@preset.glass",@"应用「玻璃外观」预设",@"一键开启底栏、播放页、详情与面板液态玻璃")];
        [rows addObject:HGATool(@"@preset.focus",@"应用「专注播放」预设",@"开启玻璃外观并自动收起干扰浮层与上滑收缩底栏")];
        [rows addObject:HGATool(@"@group.dock",@"悬浮底栏设置",@"当前调节页面：全局底部导航栏（材质／三级尺寸／配色／收缩）")];
        [rows addObject:HGATool(@"@group.pages.home",@"首页与播放页",@"当前调节页面：首页顶部栏、推荐流、竖屏／横屏画质与系列剧标签")];
        [rows addObject:HGATool(@"@group.pages.detail",@"播放详情页与全集",@"当前调节页面：全集入口、清屏倍速、剧好物与弹出面板")];
        [rows addObject:HGATool(@"@group.pages.finder",@"找剧／剧场页",@"当前调节页面：搜索／识剧／AI 玻璃、顶部频道栏与快捷入口关闭")];
        [rows addObject:HGATool(@"@group.pages.mine",@"我的页面",@"当前调节页面：个人页按钮、功能宫格、作品/历史/收藏/点赞/预约与筛选编辑")];
        [rows addObject:HGATool(@"@group.icons",@"动作图标定制",@"当前调节页面：播放页右侧点赞、收藏、评论与分享图标")];
        [rows addObject:HGATool(@"@group.fonts",@"字体与字号",@"当前调节页面：播放页、弹出面板与底栏的原生字体和缩放")];
        [rows addObject:HGATool(@"@export",@"导出外观参数",@"导出 JSON 配置文件（不含图片与字体）")];
        [rows addObject:HGATool(@"@import",@"从文件恢复外观参数",@"从 JSON 文件恢复已保存的外观开关与参数")];
        [rows addObject:HGATool(@"@reset",@"恢复全部外观默认值",@"关闭外观定制并恢复宿主原貌")];
        if(HGCommunityMatchesQuery(@""))
            [rows addObject:HGATool(@"@community",@"TG 更新频道 · @Naloiafeed",@"查看更新说明或复制频道地址")];
    } else if([page isEqual:@"dock"]) {
        [self addKeys:@[@"dock.enabled",@"dock.material",@"dock.content"] into:rows];
        [rows addObject:HGATool(@"@group.dock.geometry",@"底栏尺寸、高度与圆角（三级调节）",
            HGAppearanceFlag(@"dock.geometry.enabled") ? @"已开启调节 · 点击展开宽度、高度、上移、圆角与图标大小" : @"已折叠 · 点击进入并打开开关后显示滑块")];
        [rows addObject:HGATool(@"@group.dock.colors",@"底栏配色定制",@"自定义底栏底色、选中色与未选中色")];
        [rows addObject:HGATool(@"@group.dock.behavior",@"交互手势、独立按钮与标签隐藏",@"上滑收缩、触感、独立圆按钮与 4 个底栏标签隐藏")];
    } else if([page isEqual:@"dock.geometry"]) {
        [self addKeys:@[@"dock.geometry.enabled"] into:rows];
        if(HGAppearanceFlag(@"dock.geometry.enabled"))
            [self addKeys:@[@"dock.width",@"dock.height",@"dock.lift",@"dock.radius",@"dock.iconSize"] into:rows];
    } else if([page isEqual:@"dock.colors"]) {
        [self addKeys:@[@"dock.tint",@"dock.selected",@"dock.normal"] into:rows];
    } else if([page isEqual:@"dock.behavior"]) {
        [self addKeys:@[@"dock.haptics",@"dock.collapse",@"dock.autoCollapse",@"dock.anchor",
            @"dock.extra",@"dock.extraSize",@"dock.action",
            @"dock.hide.home",@"dock.hide.theater",@"dock.hide.store",@"dock.hide.mine"] into:rows];
    } else if([page isEqual:@"pages.hub"]) {
        [rows addObject:HGATool(@"@group.pages.home",@"首页与播放页",@"顶部栏元素、动作玻璃、系列剧标签、横屏铺满与控件位移")];
        [rows addObject:HGATool(@"@group.pages.detail",@"播放详情页与全集",@"全集入口、返回/倍速/弹幕/清屏、剧好物与弹出面板")];
        [rows addObject:HGATool(@"@group.pages.finder",@"找剧／剧场页",@"搜索/识剧/AI 液态玻璃与顶部频道/快捷入口独立开关")];
        [rows addObject:HGATool(@"@group.pages.mine",@"我的页面",@"个人主页按钮、宫格、作品/历史/收藏/点赞/预约与筛选编辑")];
        [rows addObject:HGATool(@"@group.panels",@"弹出面板玻璃",@"选集、评论、分享、长按与倍速弹出面板材质和透明度")];
        [rows addObject:HGATool(@"@group.icons",@"动作图标定制",@"右侧动作图标风格预设、SF Symbol 与自定义图片")];
        [rows addObject:HGATool(@"@group.fonts",@"字体与字号",@"系统字体、导入字体文件与字号比例缩放")];
    } else if([page isEqual:@"pages.home"]) {
        [self addKeys:@[@"player.material",@"player.rightGlass",@"player.headerGlass",@"player.fullscreen",
            @"player.defaultMaxQuality",@"player.seriesPillGlass",@"layout.seriesPill.hidden"] into:rows];
        [rows addObject:HGATool(@"@group.pages.home.topHide",@"首页顶部栏元素隐藏",@"菜单、关注、漫剧、真人剧、推荐、搜索独立隐藏")];
        [rows addObject:HGATool(@"@group.pages.layout",@"播放页控件隐藏、位移与缩放（四级菜单）",@"分别调节 13 类播放浮层的位置、大小与显示")];
    } else if([page isEqual:@"pages.home.topHide"]) {
        [self addKeys:@[@"layout.channels.hide.sidebar",@"layout.channels.hide.follow",@"layout.channels.hide.comic",
            @"layout.channels.hide.live",@"layout.channels.hide.recommend",@"layout.channels.hide.search"] into:rows];
    } else if([page isEqual:@"pages.layout"]) {
        for(NSString *role in HGAppearanceRoles()) {
            [rows addObject:HGATool([@"@group.layout." stringByAppendingString:role],
                HGARoleTitleForKey(role),@"进入调节隐藏、水平／垂直偏移与缩放比例")];
        }
    } else if([page hasPrefix:@"layout."]) {
        NSString *role=[page substringFromIndex:7];
        NSString *prefix=[@"layout." stringByAppendingString:role];
        [self addKeys:@[[prefix stringByAppendingString:@".hidden"],
            [prefix stringByAppendingString:@".x"],
            [prefix stringByAppendingString:@".y"],
            [prefix stringByAppendingString:@".scale"]] into:rows];
    } else if([page isEqual:@"pages.detail"]) {
        [self addKeys:@[@"player.episodeGlass",@"player.controlsGlass",@"player.separateProgress",
            @"player.goodsPillGlass",@"layout.goodsPill.hidden"] into:rows];
        [rows addObject:HGATool(@"@group.panels",@"选集、评论、分享与倍速面板",@"调节弹出面板液态玻璃、材质与底色透明度")];
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
                [HGARoleTitleForKey(role) stringByAppendingString:@"图标定制"],
                ([role isEqual:@"like"] || [role isEqual:@"collect"]) ? @"自定义普通与选中状态 SF Symbol 或图片" : @"自定义 SF Symbol 或相册图片")];
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
        [rows addObject:HGATool(@"@fonts.reset",@"恢复宿主原字体与字号",@"清除已导入的字体并关闭文字缩放")];
    }
    self.rows=rows;
    UILabel *empty=nil;
    if(!rows.count){empty=[UILabel new];empty.text=@"没有匹配的外观设置";empty.textAlignment=NSTextAlignmentCenter;empty.textColor=UIColor.secondaryLabelColor;}
    self.tableView.backgroundView=empty;[self.tableView reloadData];
}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section{return self.rows.count;}
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    NSString *query=[self.search.searchBar.text ?: @"" stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
    if(query.length)return @"搜索结果（含所属菜单路径）";
    return HGAAppearancePageHeader(HGAAppearancePageForGroup(self.group));
}
- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
    NSString *page=HGAAppearancePageForGroup(self.group);
    if([page isEqual:@"fonts"])return @"只改变已适配播放页、面板及底栏的原生文字，不修改视频内字幕、网页或自绘文字。编辑中的输入框、密码、图标字体保持原样。字体导入上限 50 MiB，仅本机使用。";
    if([page isEqual:@"pages.home"] || [page hasPrefix:@"layout."] || [page isEqual:@"pages.layout"])
        return @"开启「横屏铺满」后，仅横屏播放横向视频时等比放大并裁切画面边缘。返回竖屏或关闭开关后恢复原比例，竖屏视频保持原样。偏移单位为点，比例 1 表示原始大小。";
    if([page hasPrefix:@"dock"])return @"底栏优先保留至少 44 点的点击区域，尺寸会受宿主可用空间限制。上滑收缩，下滑或点击圆钮展开；语音朗读开启时保持展开。开启“底栏尺寸与圆角调节”后才会展开滑块。";
    if([page hasPrefix:@"icons"])return @"每个动作可使用预设、系统图标或相册图片。收藏和点赞可单独设置已选中的图标。";
    if([page isEqual:@"panels"] || [page isEqual:@"pages.detail"])return @"仅处理已识别面板与详情控件，不隐藏绘制内容、输入框或点击遮罩。未匹配的界面保留原样，适配状态随日志导出。";
    if([page isEqual:@"pages.finder"])return @"找剧页面的两个关闭开关分别控制顶部频道栏与下方 筛选／排行榜／新剧／预约 快捷入口，不影响首页底部与顶部频道。";
    if([page isEqual:@"pages.mine"])return @"针对 legacy UIKit 个人页控件、作品/历史/收藏/点赞/预约标签及筛选/编辑按钮进行液态玻璃与灰底清理。";
    return @"先开启「启用外观定制」，或应用预设。多数调整即时生效；首次开启原生玻璃后请完全退出红果再打开，启动前未启用玻璃时先使用磨砂。外观参数备份不包含图片与字体文件；恢复时需在同一设备保留这些资源。";
}
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)path {
    NSDictionary *row=self.rows[path.row];NSString *kind=row[@"kind"],*key=row[@"key"];
    NSString *query=[self.search.searchBar.text ?: @"" stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
    UITableViewCell *cell=[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:nil];
    cell.textLabel.text=row[@"title"];cell.textLabel.numberOfLines=0;cell.detailTextLabel.numberOfLines=0;
    cell.textLabel.font=[UIFont preferredFontForTextStyle:UIFontTextStyleBody];
    cell.detailTextLabel.font=[UIFont preferredFontForTextStyle:UIFontTextStyleCaption1];
    cell.textLabel.adjustsFontForContentSizeCategory=YES;cell.detailTextLabel.adjustsFontForContentSizeCategory=YES;
    cell.accessibilityIdentifier=[@"honggo.appearance.setting." stringByAppendingString:key];
    NSString *note=HGAAppearanceKeyNote(key);
    NSString *location=query.length && ![key hasPrefix:@"@"] ? [@"位置：" stringByAppendingString:HGAAppearanceKeyLocation(key)] : nil;
    if([kind isEqual:@"bool"]) {
        UISwitch *toggle=[UISwitch new];toggle.on=HGAppearanceFlag(key);toggle.accessibilityIdentifier=key;
        toggle.accessibilityLabel=row[@"title"];
        [toggle addTarget:self action:@selector(toggle:) forControlEvents:UIControlEventValueChanged];
        cell.accessoryView=toggle;cell.selectionStyle=UITableViewCellSelectionStyleNone;
        if(note && location)cell.detailTextLabel.text=[NSString stringWithFormat:@"%@ · %@",location,note];
        else cell.detailTextLabel.text=note ?: location;
    }else if([kind isEqual:@"number"]) {
        UISlider *slider=[[UISlider alloc] initWithFrame:CGRectMake(0,0,125,44)];
        slider.minimumValue=[row[@"min"] floatValue];slider.maximumValue=[row[@"max"] floatValue];
        slider.value=HGAppearanceNumber(key);slider.accessibilityIdentifier=key;slider.accessibilityLabel=row[@"title"];
        [slider addTarget:self action:@selector(slider:) forControlEvents:UIControlEventValueChanged];
        cell.accessoryView=slider;
        NSString *range=[NSString stringWithFormat:@"%.2f（%@～%@）",slider.value,row[@"min"],row[@"max"]];
        cell.detailTextLabel.text=location ? [NSString stringWithFormat:@"%@ · %@",range,location] : range;
        cell.selectionStyle=UITableViewCellSelectionStyleNone;
    }else {
        cell.accessoryType=UITableViewCellAccessoryDisclosureIndicator;
        NSString *value=HGAppearanceString(key);
        if([kind isEqual:@"choice"]) {
            NSUInteger i=[row[@"choices"] indexOfObject:value];
            NSString *label=i!=NSNotFound?row[@"labels"][i]:@"默认";
            cell.detailTextLabel.text=location ? [NSString stringWithFormat:@"%@ · %@",label,location] : label;
        }else if([kind isEqual:@"image"] || [kind isEqual:@"fontfile"]) {
            NSString *status=value.length?@"已选择本地资源，点击可更换或清除":@"未选择";
            cell.detailTextLabel.text=location ? [NSString stringWithFormat:@"%@ · %@",status,location] : status;
        }else if([kind isEqual:@"action"]) {
            cell.detailTextLabel.text=row[@"detail"];
        }else {
            NSString *status=value.length?value:@"跟随系统／预设";
            cell.detailTextLabel.text=location ? [NSString stringWithFormat:@"%@ · %@",status,location] : status;
        }
        if([key isEqual:@"@reset"])cell.textLabel.textColor=UIColor.systemRedColor;
    }return cell;
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
- (void)toggle:(UISwitch *)toggle {
    NSString *key=toggle.accessibilityIdentifier;[self save:key value:@(toggle.on)];toggle.on=HGAppearanceFlag(key);
    if([key isEqual:@"dock.geometry.enabled"])[self reloadRows];
}
- (void)slider:(UISlider *)slider {
    NSString *key=slider.accessibilityIdentifier;
    double value=slider.maximumValue<=2?round(slider.value*100)/100:round(slider.value);
    [self save:key value:@(value)];slider.value=HGAppearanceNumber(key);
    UIView *node=slider;while(node && ![node isKindOfClass:UITableViewCell.class])node=node.superview;
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
    [a addAction:[UIAlertAction actionWithTitle:@"应用" style:UIAlertActionStyleDestructive handler:^(UIAlertAction *button){action();[self reloadRows];}]];
    [self presentViewController:a animated:YES completion:nil];
}
- (void)openRow:(NSDictionary *)row {
    if(self.busy)return;
    NSString *key=row[@"key"],*kind=row[@"kind"];
    if([key isEqual:@"@community"]){HGShowCommunity(self);return;}
    if([key hasPrefix:@"@group."]){HGOpenAppearance(self,[key substringFromIndex:7]);return;}
    if([key hasPrefix:@"@preset."]) {
        NSString *preset=[key substringFromIndex:8];
        [self confirm:row[@"title"] detail:@"会重置本页外观参数并开启这套预设，不改变去广告开关。首次开启原生玻璃后请重启红果。"
            action:^{HGAppearanceApply(HGAppearancePreset(preset),nil);}];return;
    }
    if([key isEqual:@"@reset"]){[self confirm:@"恢复外观默认值？" detail:@"关闭新外观定制并恢复宿主控件。原有去广告设置不变。" action:^{HGAppearanceReset();}];return;}
    if([key isEqual:@"@fonts.reset"]){HGAppearanceApply(@{@"fonts.enabled":@NO,@"fonts.name":@"",@"fonts.asset":@"",@"fonts.scale":@1},nil);[self reloadRows];return;}
    if([key isEqual:@"@export"]){[self exportConfiguration];return;}
    if([key isEqual:@"@import"]){[self pickDocument:@"configuration"];return;}
    if([kind isEqual:@"choice"]) {
        UIAlertController *a=[UIAlertController alertControllerWithTitle:row[@"title"] message:nil preferredStyle:UIAlertControllerStyleActionSheet];
        NSArray *values=row[@"choices"],*labels=row[@"labels"];
        for(NSUInteger i=0;i<values.count;i++)[a addAction:[UIAlertAction actionWithTitle:labels[i] style:UIAlertActionStyleDefault handler:^(UIAlertAction *button){[self save:key value:values[i]];[self reloadRows];}]];
        [a addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:nil]];[self presentSheet:a];return;
    }
    if([kind isEqual:@"symbol"]) {
        UIAlertController *a=[UIAlertController alertControllerWithTitle:row[@"title"] message:@"输入系统图标名，如 heart.fill、star.fill。保存会替换该项图片，留空清除系统图标。" preferredStyle:UIAlertControllerStyleAlert];
        [a addTextFieldWithConfigurationHandler:^(UITextField *text){text.text=HGAppearanceString(key);text.autocorrectionType=UITextAutocorrectionTypeNo;text.autocapitalizationType=UITextAutocapitalizationTypeNone;}];
        [a addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:nil]];
        [a addAction:[UIAlertAction actionWithTitle:@"保存" style:UIAlertActionStyleDefault handler:^(UIAlertAction *button){
            NSString *value=[a.textFields.firstObject.text ?: @"" stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
            if(value.length && ![UIImage systemImageNamed:value]) {dispatch_async(dispatch_get_main_queue(),^{[self message:@"图标不存在" detail:@"此系统没有该 SF Symbol，请检查名称。"];});return;}
            if(value.length) {
                NSError *error=nil;
                NSString *imageKey=[[key stringByDeletingPathExtension] stringByAppendingString:@".asset"];
                if(!HGAppearanceApply(@{key:value,imageKey:@""},&error))
                    dispatch_async(dispatch_get_main_queue(),^{[self message:@"设置未更改" detail:error.localizedDescription];});
            } else [self save:key value:value];
            [self reloadRows];
        }]];[self presentViewController:a animated:YES completion:nil];return;
    }
    if([kind isEqual:@"color"] || [kind isEqual:@"image"] || [kind isEqual:@"font"] || [kind isEqual:@"fontfile"]) {
        UIAlertController *a=[UIAlertController alertControllerWithTitle:row[@"title"] message:nil preferredStyle:UIAlertControllerStyleActionSheet];
        [a addAction:[UIAlertAction actionWithTitle:@"选择…" style:UIAlertActionStyleDefault handler:^(UIAlertAction *button){
            // Wait for the action sheet's dismissal; presenting on the next
            // run-loop tick is too early on devices with animated transitions.
            [a dismissViewControllerAnimated:YES completion:^{[self launchPicker:row];}];
        }]];
        [a addAction:[UIAlertAction actionWithTitle:@"恢复默认／清除" style:UIAlertActionStyleDestructive handler:^(UIAlertAction *button){
            if([kind isEqual:@"fontfile"])HGAppearanceApply(@{@"fonts.asset":@"",@"fonts.name":@""},nil);
            else [self save:key value:@""];[self reloadRows];
        }]];
        [a addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:nil]];[self presentSheet:a];
    }
}
- (void)launchPicker:(NSDictionary *)row {
    if(self.presentedViewController || !self.viewIfLoaded.window)return;
    NSString *key=row[@"key"],*kind=row[@"kind"];
    if([kind isEqual:@"color"]) {
        self.colorKey=key;UIColorPickerViewController *picker=[UIColorPickerViewController new];
        picker.delegate=self;picker.supportsAlpha=NO;picker.selectedColor=HGAppearanceColor(HGAppearanceString(key),UIColor.labelColor);
        [self presentViewController:picker animated:YES completion:nil];
    }else if([kind isEqual:@"image"]) {
        self.assetKey=key;PHPickerConfiguration *configuration=[PHPickerConfiguration new];
        configuration.filter=PHPickerFilter.imagesFilter;configuration.selectionLimit=1;
        PHPickerViewController *picker=[[PHPickerViewController alloc] initWithConfiguration:configuration];picker.delegate=self;
        [self presentViewController:picker animated:YES completion:nil];
    }else if([kind isEqual:@"font"]) {
        UIFontPickerViewControllerConfiguration *configuration=[UIFontPickerViewControllerConfiguration new];
        configuration.includeFaces=YES;
        UIFontPickerViewController *picker=[[UIFontPickerViewController alloc] initWithConfiguration:configuration];picker.delegate=self;
        [self presentViewController:picker animated:YES completion:nil];
    }else [self pickDocument:@"font"];
}
- (void)colorPickerViewController:(UIColorPickerViewController *)controller didSelectColor:(UIColor *)color continuously:(BOOL)continuously {
    CGFloat r=0,g=0,b=0,a=0;
    if([[color resolvedColorWithTraitCollection:self.traitCollection] getRed:&r green:&g blue:&b alpha:&a]) {
        NSString *hex=[NSString stringWithFormat:@"#%02X%02X%02X",(unsigned)lround(r*255),(unsigned)lround(g*255),(unsigned)lround(b*255)];
        [self save:self.colorKey value:hex];
    }
}
- (void)colorPickerViewControllerDidFinish:(UIColorPickerViewController *)controller {[self reloadRows];}
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
            NSData *data=url?HGAReadFile(url,12*1024*1024,&error):nil;
            NSString *token=data?HGAImportImage(data,&error):nil;
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
    picker.delegate=self;picker.allowsMultipleSelection=NO;[self presentViewController:picker animated:YES completion:nil];
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
        }else if(data && !error)configuration=HGAppearanceParse(data,&error);
        dispatch_async(dispatch_get_main_queue(),^{
            self.busy=NO;
            // The native picker dismisses itself; defer UI until that transition finishes.
            void (^finish)(void)=^{
                if(error){[self message:@"导入失败" detail:error.localizedDescription];return;}
                if(fonts.count){
                    UIAlertController *choice=[UIAlertController alertControllerWithTitle:@"选择字体" message:@"导入完成；选择字形后再开启字体功能。" preferredStyle:UIAlertControllerStyleActionSheet];
                    for(NSString *name in fonts)[choice addAction:[UIAlertAction actionWithTitle:name style:UIAlertActionStyleDefault handler:^(UIAlertAction *a){
                        HGAppearanceApply(@{@"fonts.asset":stored.lastPathComponent,@"fonts.name":name},nil);[self reloadRows];
                    }]];
                    [choice addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:nil]];[self presentSheet:choice];
                }else if(configuration){[self confirm:@"恢复外观参数？" detail:@"只修改文件中列出的 Honggo 外观设置。字体与图片需要在本机存在。" action:^{
                    NSError *applyError=nil;
                    if(!HGAppearanceApply(configuration,&applyError))dispatch_async(dispatch_get_main_queue(),^{[self message:@"未恢复" detail:applyError.localizedDescription];});
                }];}
            };
            if(self.presentedViewController==controller)[controller dismissViewControllerAnimated:YES completion:finish];
            else if(controller.transitionCoordinator)[controller.transitionCoordinator animateAlongsideTransition:nil completion:^(id<UIViewControllerTransitionCoordinatorContext> context){finish();}];
            else finish();
        });
    });
}
- (void)documentPickerWasCancelled:(UIDocumentPickerViewController *)controller{self.documentPurpose=nil;}
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
    HGAAppearanceController *panel=[[HGAAppearanceController alloc] initWithStyle:UITableViewStyleInsetGrouped];panel.group=group;
    if([current isKindOfClass:HGSettingsController.class] || [current isKindOfClass:HGAAppearanceController.class])
        [current.navigationController pushViewController:panel animated:YES];
    else {
        UINavigationController *navigation=[[UINavigationController alloc] initWithRootViewController:panel];
        [current presentViewController:navigation animated:YES completion:nil];
    }
}
