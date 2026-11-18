#import "Honggo.h"
#import "Appearance.h"
#import "Community.h"
#import "SettingsTransition.h"
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <dispatch/dispatch.h>

// This panel uses only Honggo's own feature catalog and public UIKit. Filtered
// row indices are never used as preference identifiers or destructive actions.
static BOOL HGSettingsMatches(NSString *text, NSString *query) {
    for (NSString *word in [query componentsSeparatedByCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet]) {
        if (word.length && [text rangeOfString:word options:NSCaseInsensitiveSearch |
                NSDiacriticInsensitiveSearch | NSWidthInsensitiveSearch].location == NSNotFound) return NO;
    }
    return YES;
}

static NSString *HGSettingsFeatureTitle(HGFeature feature) {
    if (feature == HGGlass && NSProcessInfo.processInfo.operatingSystemVersion.majorVersion < 26)
        return @"系统模糊底栏";
    return HGFeatureTitle(feature);
}

static NSString *HGSettingsFeatureLocation(HGFeature feature) {
    switch (feature) {
        case HGGlass:
        case HGGlassControls:
            return @"基础玻璃与按钮";
        case HGSplash:
        case HGTopView:
        case HGEcommerce:
            return @"广告拦截 → 启动与首页";
        case HGFeed:
        case HGComment:
        case HGPause:
        case HGPatch:
        case HGSurvey:
            return @"广告拦截 → 播放流与插播";
        case HGMarketing:
        case HGRedPacket:
        case HGCouponPendant:
        case HGMiniGamePendant:
            return @"广告拦截 → 弹窗与挂件";
        case HGBookstoreAd:
        case HGWelfareGuide:
        case HGSidebarGameCenter:
        case HGScreenshotShopping:
            return @"广告拦截 → 书城与侧栏";
        default:
            return @"Honggo";
    }
}

@interface HGCompatibilityController : UITableViewController
@property(nonatomic, copy) NSArray<NSDictionary *> *rows;
@end

@implementation HGCompatibilityController
- (void)viewDidLoad {
    [super viewDidLoad];
    self.title = @"本次启动兼容状态";
    self.tableView.accessibilityIdentifier = @"honggo.compatibility";
    self.tableView.rowHeight = UITableViewAutomaticDimension;
    self.tableView.estimatedRowHeight = 68;
    NSMutableArray *rows = [NSMutableArray arrayWithArray:@[
        @{@"title": @"插件", @"detail": @"Honggo " HG_PLUGIN_VERSION},
        @{@"title": @"宿主与系统", @"detail": [NSString stringWithFormat:@"红果构建 %@ · iOS %@",
            [NSBundle.mainBundle objectForInfoDictionaryKey:@"CFBundleVersion"] ?: @"未知",
            UIDevice.currentDevice.systemVersion]},
        @{@"title": @"玻璃效果", @"detail": HGNativeGlassConfigured() ?
            @"系统玻璃配置已就绪，实际显示请以当前设备为准。" :
            @"当前未配置系统玻璃；底栏按系统能力使用模糊材质。"}
    ]];
    NSDictionary *snapshot = HGRuntimeSnapshot();
    NSArray *modules = @[@"settings", @"tabbar", @"controls", @"ads", @"runtime", @"appearance"];
    NSArray *titles = @[@"设置入口", @"底栏", @"按钮外观", @"广告过滤", @"基础适配检查", @"外观定制"];
    for (NSUInteger index = 0; index < modules.count; index++) {
        NSUInteger installed = 0, skipped = 0, disabled = 0;
        BOOL moduleDisabled = NO;
        for (NSDictionary *entry in snapshot[@"entries"]) {
            if (![entry[@"module"] isEqual:modules[index]]) continue;
            if ([entry[@"id"] isEqual:[@"module." stringByAppendingString:modules[index]]])
                moduleDisabled = [entry[@"state"] isEqual:@"disabled"];
            if ([entry[@"state"] isEqual:@"installed"]) installed++;
            else if ([entry[@"state"] isEqual:@"disabled"]) disabled++;
            else skipped++;
        }
        NSString *detail;
        if (moduleDisabled) detail = @"本次启动未开启此功能。更改开关后请完全退出再打开。";
        else if (installed || skipped) detail = [NSString stringWithFormat:@"%lu 项检查通过，%lu 项跳过。%@",
            (unsigned long)installed, (unsigned long)skipped,
            skipped ? @"未匹配的入口保留宿主行为。" : @"实际效果仍需在宿主中确认。"];
        else detail = disabled ? @"本次启动未开启。更改开关后请完全退出再打开。" : @"暂无检查记录。";
        [rows addObject:@{@"title": titles[index], @"detail": detail}];
    }
    self.rows = rows;
}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section { return self.rows.count; }
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    return @"当前调节页面：本次启动兼容状态与模块检查结果";
}
- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
    return @"此页记录本次启动的适配检查，开关调整不会改写已经安装的入口。需要排查时，可返回设置导出日志，查看具体跳过原因。入口检查通过不代表已完成红果真机验收。";
}
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:nil];
    cell.textLabel.text = self.rows[indexPath.row][@"title"];
    cell.detailTextLabel.text = self.rows[indexPath.row][@"detail"];
    cell.textLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
    cell.detailTextLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleFootnote];
    cell.textLabel.adjustsFontForContentSizeCategory = YES;
    cell.detailTextLabel.adjustsFontForContentSizeCategory = YES;
    cell.textLabel.numberOfLines = 0;
    cell.detailTextLabel.numberOfLines = 0;
    cell.selectionStyle = UITableViewCellSelectionStyleNone;
    return cell;
}
@end

@interface HGSettingsController () <UISearchResultsUpdating, UISearchControllerDelegate, UIDocumentPickerDelegate>
@property(nonatomic, copy) NSString *menuPath;
@property(nonatomic, strong) UISearchController *settingsSearch;
@property(nonatomic, copy) NSArray<NSDictionary *> *visibleFolders;
@property(nonatomic, copy) NSArray<NSNumber *> *visibleFeatures;
@property(nonatomic, copy) NSArray<NSNumber *> *visibleDiagnostics;
@property(nonatomic, copy) NSArray<NSNumber *> *visibleConfiguration;
@property(nonatomic) BOOL preferenceOperationInFlight;
@property(nonatomic) BOOL communityVisible;
@property(nonatomic, copy) dispatch_block_t afterSearchDismissal;
@end

@implementation HGSettingsController
- (void)viewDidLoad {
    [super viewDidLoad];
    self.title = [self pageNavigationTitle];
    if (!self.menuPath.length) {
        HGDiagnosticsStart(nil, @{@"os": NSProcessInfo.processInfo.operatingSystemVersionString});
        HGLog(@"settings.open", @{@"nativeGlass": @(HGNativeGlassConfigured())});
    }
    self.tableView.accessibilityIdentifier = @"honggo.settings.options";
    self.tableView.rowHeight = UITableViewAutomaticDimension;
    self.tableView.estimatedRowHeight = 62;
    if (self.navigationController.viewControllers.firstObject == self) {
        self.navigationItem.rightBarButtonItem = [[UIBarButtonItem alloc]
            initWithBarButtonSystemItem:UIBarButtonSystemItemDone target:self action:@selector(closeSettings)];
        self.navigationItem.leftBarButtonItem = [[UIBarButtonItem alloc] initWithTitle:@"外观"
            style:UIBarButtonItemStylePlain target:self action:@selector(openAppearance)];
        self.navigationItem.leftBarButtonItem.accessibilityIdentifier = @"honggo.appearance.open";
    }
    self.settingsSearch = [[UISearchController alloc] initWithSearchResultsController:nil];
    self.settingsSearch.searchResultsUpdater = self;
    self.settingsSearch.delegate = self;
    self.settingsSearch.obscuresBackgroundDuringPresentation = NO;
    self.settingsSearch.searchBar.placeholder = @"搜索功能、页面、广告位置或工具";
    self.settingsSearch.searchBar.accessibilityIdentifier = @"honggo.settings.search";
    self.navigationItem.searchController = self.settingsSearch;
    self.navigationItem.hidesSearchBarWhenScrolling = NO;
    self.definesPresentationContext = YES;
    [self updateSearchResultsForSearchController:self.settingsSearch];
}
- (NSString *)pageNavigationTitle {
    if ([self.menuPath isEqual:@"ads"]) return @"广告与推广拦截";
    if ([self.menuPath isEqual:@"ads.startup"]) return @"启动与首页广告";
    if ([self.menuPath isEqual:@"ads.playback"]) return @"播放流与插播";
    if ([self.menuPath isEqual:@"ads.popup"]) return @"弹窗与挂件";
    if ([self.menuPath isEqual:@"ads.discovery"]) return @"书城与侧栏";
    if ([self.menuPath isEqual:@"controls"]) return @"基础玻璃与按钮";
    return HG_PLUGIN_NAME;
}
- (NSString *)pageHeaderNote {
    if ([self.menuPath isEqual:@"ads"]) return @"当前调节页面：广告与推广拦截分类（点击进入对应页面调节）";
    if ([self.menuPath isEqual:@"ads.startup"]) return @"当前调节页面：冷启动流程与首页推荐页广告拦截";
    if ([self.menuPath isEqual:@"ads.playback"]) return @"当前调节页面：短剧播放器、信息流混排、评论与暂停插播";
    if ([self.menuPath isEqual:@"ads.popup"]) return @"当前调节页面：直播领券弹窗、福利红包与屏幕悬浮挂件";
    if ([self.menuPath isEqual:@"ads.discovery"]) return @"当前调节页面：书城底部、福利引导、首页侧栏与全局截图卡片";
    if ([self.menuPath isEqual:@"controls"]) return @"当前调节页面：全局启动期系统玻璃与原生液态玻璃按钮开关";
    return @"当前调节页面：Honggo 主分类（版本 " HG_PLUGIN_VERSION @"）";
}
- (NSArray<NSDictionary *> *)foldersForCurrentPath {
    if (!self.menuPath.length) {
        return @[
            @{@"id": @"appearance.root", @"title": @"外观总开关与快捷预设",
              @"detail": @"当前调节页面：全插件外观总览、玻璃／专注预设与参数备份", @"target": @"appearance", @"group": @""},
            @{@"id": @"appearance.dock", @"title": @"悬浮底栏设置（含三级尺寸调节）",
              @"detail": @"当前调节页面：全局底部导航栏（材质、尺寸圆角开关、配色与行为）", @"target": @"appearance", @"group": @"dock"},
            @{@"id": @"appearance.pages", @"title": @"页面外观与控件净化",
              @"detail": @"当前调节页面：首页、详情全集、找剧、我的页面、图标与字体", @"target": @"appearance", @"group": @"pages.hub"},
            @{@"id": @"ads", @"title": @"广告与推广拦截",
              @"detail": @"当前调节页面：启动开屏、短剧流、商业弹窗、书城、侧栏与截图卡片", @"target": @"settings", @"path": @"ads"},
            @{@"id": @"controls", @"title": @"基础玻璃底栏与按钮开关",
              @"detail": @"当前调节页面：启动期系统玻璃底栏与原生液态玻璃按钮总开关", @"target": @"settings", @"path": @"controls"},
        ];
    }
    if ([self.menuPath isEqual:@"ads"]) {
        return @[
            @{@"id": @"ads.startup", @"title": @"启动与首页广告",
              @"detail": @"当前调节页面：开屏广告、首页 TopView 广告、启动电商推广", @"target": @"settings", @"path": @"ads.startup"},
            @{@"id": @"ads.playback", @"title": @"播放流、暂停与插播",
              @"detail": @"当前调节页面：信息流直播/小游戏、评论区、暂停、插播与问卷", @"target": @"settings", @"path": @"ads.playback"},
            @{@"id": @"ads.popup", @"title": @"弹窗与悬浮挂件",
              @"detail": @"当前调节页面：商业直播领券弹窗、福利红包、领券与小游戏挂件", @"target": @"settings", @"path": @"ads.popup"},
            @{@"id": @"ads.discovery", @"title": @"书城、侧栏与截图卡片",
              @"detail": @"当前调节页面：书城底部、福利引导气泡、侧栏游戏中心与截图找同款", @"target": @"settings", @"path": @"ads.discovery"},
        ];
    }
    return @[];
}
- (NSArray<NSNumber *> *)featuresForCurrentPath {
    if ([self.menuPath isEqual:@"controls"]) return @[@(HGGlass), @(HGGlassControls)];
    if ([self.menuPath isEqual:@"ads.startup"]) return @[@(HGSplash), @(HGTopView), @(HGEcommerce)];
    if ([self.menuPath isEqual:@"ads.playback"]) return @[@(HGFeed), @(HGComment), @(HGPause), @(HGPatch), @(HGSurvey)];
    if ([self.menuPath isEqual:@"ads.popup"]) return @[@(HGMarketing), @(HGRedPacket), @(HGCouponPendant), @(HGMiniGamePendant)];
    if ([self.menuPath isEqual:@"ads.discovery"]) return @[@(HGBookstoreAd), @(HGWelfareGuide), @(HGSidebarGameCenter), @(HGScreenshotShopping)];
    return @[];
}
- (NSArray<NSString *> *)diagnosticTitles {
    return @[@"增强诊断", @"导出最近一小时日志", @"清除诊断日志", @"本次启动兼容状态"];
}
- (NSArray<NSString *> *)configurationTitles { return @[@"导出功能配置", @"从文件恢复配置", @"关闭全部功能开关"]; }
- (void)updateSearchResultsForSearchController:(UISearchController *)searchController {
    NSString *rawQuery = searchController.searchBar.text ?: @"";
    NSString *trimmed = [rawQuery stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
    BOOL searching = trimmed.length > 0;
    NSMutableArray *folders = [NSMutableArray array];
    NSMutableArray *features = [NSMutableArray array], *diagnostics = [NSMutableArray array], *configuration = [NSMutableArray array];
    if (searching) {
        for (HGFeature feature = 0; feature < HGFeatureCount; feature++) {
            NSString *text = [NSString stringWithFormat:@"%@ %@ %@",
                HGFeatureSearchText(feature), HGSettingsFeatureTitle(feature), HGSettingsFeatureLocation(feature)];
            if (HGSettingsMatches(text, rawQuery)) [features addObject:@(feature)];
        }
        NSArray *diagnosticAliases = @[@"诊断 截屏 现场 排错 debug", @"导出 分享 日志 log", @"清除 删除 日志 log", @"兼容 状态 适配 版本 capability"];
        NSArray *configurationAliases = @[@"备份 导出 配置 backup export", @"备份 导入 恢复 配置 restore import", @"恢复 默认 重置 关闭 全部 reset"];
        for (NSUInteger index = 0; index < self.diagnosticTitles.count; index++) {
            if (HGSettingsMatches([self.diagnosticTitles[index] stringByAppendingFormat:@" %@", diagnosticAliases[index]], rawQuery))
                [diagnostics addObject:@(index)];
        }
        for (NSUInteger index = 0; index < self.configurationTitles.count; index++) {
            if (HGSettingsMatches([self.configurationTitles[index] stringByAppendingFormat:@" %@", configurationAliases[index]], rawQuery))
                [configuration addObject:@(index)];
        }
        self.communityVisible = HGCommunityMatchesQuery(rawQuery);
    } else {
        [folders addObjectsFromArray:[self foldersForCurrentPath]];
        [features addObjectsFromArray:[self featuresForCurrentPath]];
        if (!self.menuPath.length) {
            for (NSUInteger index = 0; index < self.diagnosticTitles.count; index++) [diagnostics addObject:@(index)];
            for (NSUInteger index = 0; index < self.configurationTitles.count; index++) [configuration addObject:@(index)];
            self.communityVisible = YES;
        } else {
            self.communityVisible = NO;
        }
    }
    self.visibleFolders = folders;
    self.visibleFeatures = features;
    self.visibleDiagnostics = diagnostics;
    self.visibleConfiguration = configuration;
    UILabel *empty = nil;
    if (folders.count + features.count + diagnostics.count + configuration.count == 0 && !self.communityVisible) {
        empty = [[UILabel alloc] initWithFrame:self.tableView.bounds];
        empty.text = @"未找到匹配的设置\n试试“玻璃”“直播”或“日志”";
        empty.numberOfLines = 0;
        empty.textAlignment = NSTextAlignmentCenter;
        empty.textColor = UIColor.secondaryLabelColor;
        empty.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
        empty.adjustsFontForContentSizeCategory = YES;
        empty.accessibilityIdentifier = @"honggo.settings.search.empty";
    }
    self.tableView.backgroundView = empty;
    [self.tableView reloadData];
}
- (void)openAppearance {
    [self performAfterSearchDismissal:^{ HGOpenAppearance(self, nil); }];
}
- (void)closeSettings {
    self.afterSearchDismissal = nil;
    UIViewController *presenter = self.navigationController.presentingViewController ?: self.presentingViewController;
    [self performAfterSearchDismissal:^{ [presenter dismissViewControllerAnimated:YES completion:nil]; }];
}
- (void)performAfterSearchDismissal:(dispatch_block_t)action {
    if (self.settingsSearch.active || self.settingsSearch.isBeingDismissed ||
            self.presentedViewController == self.settingsSearch) {
        // Search owns a presentation until its dismissal completes. Keep the
        // resolved action, not its old row index, while the result list resets.
        self.afterSearchDismissal = action;
        if (!self.settingsSearch.isBeingDismissed) {
            HGAfterViewTransition(self.settingsSearch, ^{
                if (self.afterSearchDismissal) self.settingsSearch.active = NO;
            });
        }
    } else if (!self.presentedViewController) {
        HGAfterViewTransition(self, ^{
            if (self.viewIfLoaded.window && !self.presentedViewController &&
                    !self.isBeingDismissed && !self.navigationController.isBeingDismissed) action();
        });
    }
}
- (void)didDismissSearchController:(UISearchController *)searchController {
    // UIKit can call didDismiss before its presentation bookkeeping unwinds.
    // Starting the parent sheet dismissal in that callback can be discarded.
    HGAfterViewTransition(self, ^{
        dispatch_block_t action = self.afterSearchDismissal;
        self.afterSearchDismissal = nil;
        if (action && self.viewIfLoaded.window && !self.presentedViewController &&
                !self.isBeingDismissed && !self.navigationController.isBeingDismissed) action();
    });
}
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView { return 4; }
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    if (section == 3) return self.communityVisible ? 1 : 0;
    return section == 0 ? (NSInteger)(self.visibleFolders.count + self.visibleFeatures.count) :
        section == 1 ? (NSInteger)self.visibleDiagnostics.count :
        section == 2 ? (NSInteger)self.visibleConfiguration.count : 0;
}
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    if (![self tableView:tableView numberOfRowsInSection:section]) return nil;
    if (section == 0) {
        NSString *trimmed = [self.settingsSearch.searchBar.text ?: @"" stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
        return trimmed.length ? @"匹配的功能开关（含所属页面路径）" : [self pageHeaderNote];
    }
    return @[@"", @"诊断与兼容", @"配置管理", @"更新与频道"][section];
}
- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
    if (![self tableView:tableView numberOfRowsInSection:section]) return nil;
    if (section == 3) return @"查看插件更新与测试版公告。只有点击打开或复制时才执行操作，不会自动加入频道。";
    if (section == 1) return @"增强诊断即时生效。复现广告或遮罩后截屏，再回来导出日志；仅记录控件结构，不读取截图像素或页面文字。本次启动保留最近 10 份现场，导出前不要退出应用。";
    if (section == 2) return @"配置文件仅保存 Honggo 的功能开关。恢复前会核对格式并显示变更数量；账号、宿主设置和增强诊断选择保持不变。恢复或关闭全部开关后，请完全退出再打开。";
    if (!self.menuPath.length && self.visibleFolders.count > 0) {
        return @"按类别进入二／三／四级页面，每个子菜单顶部注明当前调节的宿主界面。顶部搜索框可直接跨级检索任意功能、工具与频道。";
    }
    NSString *mode = HGNativeGlassConfigured() ?
        @"本次启动已配置系统玻璃。关闭两个原有玻璃开关及外观定制并重启后，插件停止启用该配置。" :
        @"iOS 26 及以上打开玻璃开关并重启后，会在启动时请求系统玻璃；旧系统底栏使用模糊材质。";
    return [@"功能开关更改后请完全退出红果短剧，再重新打开。" stringByAppendingString:mode];
}
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    if (indexPath.section == 3) {
        UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:nil];
        cell.textLabel.text = @"TG 更新频道";
        cell.detailTextLabel.text = @"@Naloiafeed · 打开频道或复制地址";
        cell.textLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
        cell.detailTextLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleFootnote];
        cell.textLabel.adjustsFontForContentSizeCategory = YES;
        cell.detailTextLabel.adjustsFontForContentSizeCategory = YES;
        cell.textLabel.numberOfLines = 0;
        cell.detailTextLabel.numberOfLines = 0;
        cell.accessibilityIdentifier = @"honggo.community.channel";
        cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
        return cell;
    }
    if (indexPath.section == 0) {
        if ((NSUInteger)indexPath.row < self.visibleFolders.count) {
            NSDictionary *folder = self.visibleFolders[indexPath.row];
            UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:nil];
            cell.textLabel.text = folder[@"title"];
            cell.detailTextLabel.text = folder[@"detail"];
            cell.textLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
            cell.detailTextLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleFootnote];
            cell.textLabel.adjustsFontForContentSizeCategory = YES;
            cell.detailTextLabel.adjustsFontForContentSizeCategory = YES;
            cell.textLabel.numberOfLines = 0;
            cell.detailTextLabel.numberOfLines = 0;
            cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
            cell.accessibilityIdentifier = [@"honggo.folder." stringByAppendingString:folder[@"id"]];
            return cell;
        }
        UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"HonggoOption"];
        if (!cell) {
            cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:@"HonggoOption"];
            cell.selectionStyle = UITableViewCellSelectionStyleNone;
            cell.textLabel.numberOfLines = 0;
            cell.detailTextLabel.numberOfLines = 0;
            cell.textLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
            cell.detailTextLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleFootnote];
            cell.textLabel.adjustsFontForContentSizeCategory = YES;
            cell.detailTextLabel.adjustsFontForContentSizeCategory = YES;
            UISwitch *toggle = [UISwitch new];
            [toggle addTarget:self action:@selector(toggleChanged:) forControlEvents:UIControlEventValueChanged];
            cell.accessoryView = toggle;
        }
        NSUInteger featureIndex = (NSUInteger)indexPath.row - self.visibleFolders.count;
        HGFeature feature = self.visibleFeatures[featureIndex].unsignedIntegerValue;
        NSString *title = HGSettingsFeatureTitle(feature);
        cell.textLabel.text = title;
        NSString *baseDetail = feature == HGGlassControls && NSProcessInfo.processInfo.operatingSystemVersion.majorVersion < 26 ?
            @"需要 iOS 26 及以上；当前系统保留原按钮。" : @"更改后完全退出应用再打开";
        NSString *trimmed = [self.settingsSearch.searchBar.text ?: @"" stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
        cell.detailTextLabel.text = trimmed.length ?
            [NSString stringWithFormat:@"位置：%@ · %@", HGSettingsFeatureLocation(feature), baseDetail] : baseDetail;
        UISwitch *toggle = (UISwitch *)cell.accessoryView;
        toggle.tag = (NSInteger)feature;
        toggle.accessibilityIdentifier = [@"hg.option." stringByAppendingString:HGFeatureIdentifier(feature)];
        toggle.accessibilityLabel = title;
        toggle.on = HGIsEnabled(feature);
        return cell;
    }
    UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil];
    cell.textLabel.numberOfLines = 0;
    cell.textLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
    cell.textLabel.adjustsFontForContentSizeCategory = YES;
    if (indexPath.section == 1) {
        NSUInteger action = self.visibleDiagnostics[indexPath.row].unsignedIntegerValue;
        cell.textLabel.text = self.diagnosticTitles[action];
        cell.accessibilityIdentifier = @[@"honggo.diagnostics.enhanced", @"honggo.diagnostics.export", @"honggo.diagnostics.clear", @"honggo.compatibility.open"][action];
        if (action == 0) {
            UISwitch *toggle = [UISwitch new];
            toggle.on = HGEnhancedDiagnostics();
            toggle.accessibilityLabel = cell.textLabel.text;
            [toggle addTarget:self action:@selector(enhancedChanged:) forControlEvents:UIControlEventValueChanged];
            cell.accessoryView = toggle;
            cell.selectionStyle = UITableViewCellSelectionStyleNone;
        } else cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
    } else {
        NSUInteger action = self.visibleConfiguration[indexPath.row].unsignedIntegerValue;
        cell.textLabel.text = self.configurationTitles[action];
        cell.accessibilityIdentifier = @[@"honggo.preferences.export", @"honggo.preferences.import", @"honggo.preferences.reset"][action];
        cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
        if (action == 2) cell.textLabel.textColor = UIColor.systemRedColor;
    }
    return cell;
}
- (void)toggleChanged:(UISwitch *)toggle {
    HGFeature feature = (HGFeature)toggle.tag;
    if (HGSetEnabled(feature, toggle.on))
        HGLog(@"settings.toggle", @{@"feature": HGFeatureIdentifier(feature), @"enabled": @(toggle.on)});
}
- (void)enhancedChanged:(UISwitch *)toggle { HGSetEnhancedDiagnostics(toggle.on); }
- (void)showMessage:(NSString *)title detail:(NSString *)detail {
    if (!self.viewIfLoaded.window || self.presentedViewController) return;
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:title message:detail preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"好" style:UIAlertActionStyleDefault handler:nil]];
    [self presentViewController:alert animated:YES completion:nil];
}
- (void)showDiagnosticError:(NSError *)error {
    [self showMessage:@"日志操作失败" detail:[NSString stringWithFormat:@"%@ (%ld)", error.domain, (long)error.code]];
}
- (void)shareURL:(NSURL *)url {
    if (!url || !self.viewIfLoaded.window || self.presentedViewController) return;
    UIActivityViewController *share = [[UIActivityViewController alloc] initWithActivityItems:@[url] applicationActivities:nil];
    share.popoverPresentationController.sourceView = self.view;
    share.popoverPresentationController.sourceRect = CGRectMake(CGRectGetMidX(self.view.bounds), CGRectGetMidY(self.view.bounds), 1, 1);
    [self presentViewController:share animated:YES completion:nil];
}
- (void)setPreferenceBusy:(BOOL)busy {
    self.preferenceOperationInFlight = busy;
    self.tableView.userInteractionEnabled = !busy;
}
- (void)exportPreferences {
    if (self.preferenceOperationInFlight) return;
    [self setPreferenceBusy:YES];
    // Snapshot feature flags on the UI thread. File I/O runs off the UI thread.
    NSError *encodingError = nil;
    NSData *data = HGPreferencesExport(&encodingError);
    if (!data) {
        [self setPreferenceBusy:NO];
        [self showMessage:@"配置导出失败" detail:encodingError.localizedDescription];
        return;
    }
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY, 0), ^{
        NSError *error = nil;
        NSURL *directory = [NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingPathComponent:@"HonggoPreferences"] isDirectory:YES];
        NSURL *url = [directory URLByAppendingPathComponent:@"Honggo-settings.json"];
        BOOL saved = [NSFileManager.defaultManager createDirectoryAtURL:directory withIntermediateDirectories:YES attributes:nil error:&error] &&
            [data writeToURL:url options:NSDataWritingAtomic error:&error];
        dispatch_async(dispatch_get_main_queue(), ^{
            [self setPreferenceBusy:NO];
            if (saved) [self shareURL:url];
            else [self showMessage:@"配置导出失败" detail:error.localizedDescription];
        });
    });
}
- (void)confirmPreferenceRestore:(NSDictionary<NSString *, NSNumber *> *)features {
    if (!self.viewIfLoaded.window || self.presentedViewController) return;
    NSUInteger changed = HGPreferencesChangeCount(features);
    if (!changed) { [self showMessage:@"无需恢复" detail:@"文件中的功能开关与当前设置一致。"]; return; }
    NSString *message = [NSString stringWithFormat:@"将更新 %lu 项 Honggo 功能开关。文件未包含的开关保持当前值，账号和其他设置不受影响。恢复后请完全退出再打开红果。", (unsigned long)changed];
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"恢复功能配置？" message:message preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:nil]];
    [alert addAction:[UIAlertAction actionWithTitle:@"恢复" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
        NSError *error = nil;
        if (HGPreferencesApply(features, &error)) {
            HGLog(@"settings.restored", @{@"changed": @(changed)});
            [self.tableView reloadData];
        } else {
            dispatch_async(dispatch_get_main_queue(), ^{ [self showMessage:@"配置恢复失败" detail:error.localizedDescription]; });
        }
    }]];
    [self presentViewController:alert animated:YES completion:nil];
}
- (void)readPreferenceURL:(NSURL *)url {
    if (self.preferenceOperationInFlight) return;
    if (!url.isFileURL) { [self showMessage:@"无法恢复配置" detail:@"请选择本地或文件提供方中的 JSON 文件。"]; return; }
    [self setPreferenceBusy:YES];
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY, 0), ^{
        BOOL scoped = [url startAccessingSecurityScopedResource];
        NSFileCoordinator *coordinator = [[NSFileCoordinator alloc] initWithFilePresenter:nil];
        __block NSData *data = nil;
        __block NSError *readError = nil;
        NSError *coordinationError = nil;
        [coordinator coordinateReadingItemAtURL:url options:NSFileCoordinatorReadingWithoutChanges error:&coordinationError byAccessor:^(NSURL *readURL) {
            NSFileHandle *handle = [NSFileHandle fileHandleForReadingFromURL:readURL error:&readError];
            if (!handle) return;
            // Bound the actual read too; the document can change after a size query.
            NSMutableData *buffer = [NSMutableData data];
            NSUInteger limit = HGPreferencesMaximumBytes + 1;
            while (buffer.length < limit) {
                NSData *chunk = [handle readDataUpToLength:MIN((NSUInteger)8192, limit - buffer.length) error:&readError];
                if (!chunk.length || readError) break;
                [buffer appendData:chunk];
            }
            data = readError ? nil : buffer.copy;
            [handle closeAndReturnError:nil];
        }];
        if (scoped) [url stopAccessingSecurityScopedResource];
        NSError *error = coordinationError ?: readError;
        NSDictionary *features = error ? nil : HGPreferencesParse(data, &error);
        dispatch_async(dispatch_get_main_queue(), ^{
            [self setPreferenceBusy:NO];
            if (features) [self confirmPreferenceRestore:features];
            else [self showMessage:@"无法恢复配置" detail:error.localizedDescription ?: @"无法读取所选文件。"];
        });
    });
}
- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls {
    NSURL *url = urls.firstObject;
    if (!url) return;
    if (controller.presentingViewController) {
        [controller dismissViewControllerAnimated:YES completion:^{ [self readPreferenceURL:url]; }];
    } else [self readPreferenceURL:url];
}
- (void)confirmReset {
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"关闭全部功能开关？"
        message:@"关闭 Honggo 的全部功能，保留外观参数、账号、宿主设置和增强诊断选择。完全退出并重新打开红果后生效。"
        preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:nil]];
    [alert addAction:[UIAlertAction actionWithTitle:@"全部关闭" style:UIAlertActionStyleDestructive handler:^(UIAlertAction *action) {
        HGPreferencesReset();
        HGAppearanceSet(@"enabled", @NO, nil);
        HGLog(@"settings.reset", @{@"features": @(HGFeatureCount)});
        [self.tableView reloadData];
    }]];
    [self presentViewController:alert animated:YES completion:nil];
}
- (void)openFolder:(NSDictionary *)folder {
    NSString *target = folder[@"target"];
    if ([target isEqual:@"appearance"]) {
        NSString *group = folder[@"group"];
        HGOpenAppearance(self, group.length ? group : nil);
        return;
    }
    HGSettingsController *child = [[HGSettingsController alloc] initWithStyle:UITableViewStyleInsetGrouped];
    child.menuPath = folder[@"path"];
    [self.navigationController pushViewController:child animated:YES];
}
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    if (self.preferenceOperationInFlight) return;
    if (indexPath.section == 0) {
        if ((NSUInteger)indexPath.row >= self.visibleFolders.count) return;
        NSDictionary *folder = self.visibleFolders[indexPath.row];
        [self performAfterSearchDismissal:^{ [self openFolder:folder]; }];
        return;
    }
    if (indexPath.section == 3) {
        [self performAfterSearchDismissal:^{ HGShowCommunity(self); }];
        return;
    }
    // A result remains identified by its stable action even when only one row matches.
    NSInteger section = indexPath.section;
    NSArray<NSNumber *> *actions = section == 2 ? self.visibleConfiguration : self.visibleDiagnostics;
    NSUInteger action = actions[indexPath.row].unsignedIntegerValue;
    if (section == 1 && action == 0) return;
    [self performAfterSearchDismissal:^{ [self performAction:action inSection:section]; }];
}
- (void)performAction:(NSUInteger)action inSection:(NSInteger)section {
    if (section == 2) {
        if (action == 0) [self exportPreferences];
        else if (action == 1) {
            UIDocumentPickerViewController *picker = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:@[UTTypeJSON] asCopy:YES];
            picker.delegate = self;
            picker.allowsMultipleSelection = NO;
            [self presentViewController:picker animated:YES completion:nil];
        } else [self confirmReset];
        return;
    }
    if (action == 3) {
        [self.navigationController pushViewController:[[HGCompatibilityController alloc] initWithStyle:UITableViewStyleInsetGrouped] animated:YES];
        return;
    }
    UITableView *tableView = self.tableView;
    tableView.userInteractionEnabled = NO;
    if (action == 2) {
        HGClearDiagnostics(^(NSError *error) {
            tableView.userInteractionEnabled = YES;
            if (error) [self showDiagnosticError:error];
        });
    } else {
        HGExportDiagnostics(HGInterfaceSnapshot(), ^(NSURL *url, NSError *error) {
            tableView.userInteractionEnabled = YES;
            if (error) [self showDiagnosticError:error];
            else [self shareURL:url];
        });
    }
}
+ (void)openFromController:(UIViewController *)presenter {
    if (!presenter || presenter.presentedViewController) return;
    HGSettingsController *settings = [[self alloc] initWithStyle:UITableViewStyleInsetGrouped];
    UINavigationController *navigation = [[UINavigationController alloc] initWithRootViewController:settings];
    [presenter presentViewController:navigation animated:YES completion:nil];
}
+ (void)openFromTabBar:(UILongPressGestureRecognizer *)gesture {
    if (gesture.state != UIGestureRecognizerStateBegan) return;
    UIResponder *responder = gesture.view;
    while (responder && ![responder isKindOfClass:UIViewController.class]) responder = responder.nextResponder;
    [self openFromController:(UIViewController *)responder];
}
@end
