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
@property(nonatomic, strong) UISearchController *settingsSearch;
@property(nonatomic, copy) NSString *activeQuery;
@property(nonatomic, copy) NSArray<NSDictionary *> *rootCategories;
@property(nonatomic, copy) NSArray<NSNumber *> *visibleFeatures;
@property(nonatomic, copy) NSArray<NSNumber *> *visibleDiagnostics;
@property(nonatomic, copy) NSArray<NSNumber *> *visibleConfiguration;
@property(nonatomic) BOOL preferenceOperationInFlight;
@property(nonatomic) BOOL communityVisible;
@property(nonatomic, copy) dispatch_block_t afterSearchDismissal;
- (void)performAction:(NSUInteger)action inSection:(NSInteger)section presenter:(UIViewController *)presenter;
- (void)toggleChanged:(UISwitch *)toggle;
- (void)enhancedChanged:(UISwitch *)toggle;
@end

// Level 2 & Level 3 subpage controller mirroring FanqieX's clean hierarchy.
@interface HGSettingsSubpageController : UITableViewController
@property(nonatomic, copy) NSString *pageID;
@property(nonatomic, weak) HGSettingsController *rootSettings;
@property(nonatomic, copy) NSArray<NSDictionary *> *rows;
@end

@implementation HGSettingsSubpageController
- (void)viewDidLoad {
    [super viewDidLoad];
    self.tableView.accessibilityIdentifier = [@"honggo.settings.page." stringByAppendingString:self.pageID ?: @""];
    self.tableView.rowHeight = UITableViewAutomaticDimension;
    self.tableView.estimatedRowHeight = 58;
    [self reloadPage];
}
- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    [self reloadPage];
}
- (void)reloadPage {
    NSString *pid = self.pageID ?: @"";
    NSMutableArray *rows = [NSMutableArray array];
    if ([pid isEqual:@"ads"]) {
        self.title = @"广告与推广拦截";
        [rows addObject:@{@"kind": @"subpage", @"page": @"ads.startup", @"title": @"启动与首页广告", @"id": @"honggo.settings.ads.startup"}];
        [rows addObject:@{@"kind": @"subpage", @"page": @"ads.playback", @"title": @"短剧播放广告", @"id": @"honggo.settings.ads.playback"}];
        [rows addObject:@{@"kind": @"subpage", @"page": @"ads.popups", @"title": @"弹窗与悬浮挂件", @"id": @"honggo.settings.ads.popups"}];
        [rows addObject:@{@"kind": @"subpage", @"page": @"ads.misc", @"title": @"书城、侧栏与截图", @"id": @"honggo.settings.ads.misc"}];
    } else if ([pid isEqual:@"ads.startup"]) {
        self.title = @"启动与首页广告";
        for (NSNumber *f in @[@(HGSplash), @(HGTopView), @(HGEcommerce)]) [rows addObject:@{@"kind": @"feature", @"feature": f}];
    } else if ([pid isEqual:@"ads.playback"]) {
        self.title = @"短剧播放广告";
        for (NSNumber *f in @[@(HGFeed), @(HGComment), @(HGPause), @(HGPatch)]) [rows addObject:@{@"kind": @"feature", @"feature": f}];
    } else if ([pid isEqual:@"ads.popups"]) {
        self.title = @"弹窗与悬浮挂件";
        for (NSNumber *f in @[@(HGMarketing), @(HGRedPacket), @(HGCouponPendant), @(HGMiniGamePendant), @(HGWelfareGuide), @(HGSurvey)])
            [rows addObject:@{@"kind": @"feature", @"feature": f}];
    } else if ([pid isEqual:@"ads.misc"]) {
        self.title = @"书城、侧栏与截图";
        for (NSNumber *f in @[@(HGBookstoreAd), @(HGSidebarGameCenter), @(HGScreenshotShopping)]) [rows addObject:@{@"kind": @"feature", @"feature": f}];
    } else if ([pid isEqual:@"base"]) {
        self.title = @"基础与兼容开关";
        for (NSNumber *f in @[@(HGGlass), @(HGGlassControls)]) [rows addObject:@{@"kind": @"feature", @"feature": f}];
    } else if ([pid isEqual:@"backup"]) {
        self.title = @"导出与备份";
        [rows addObject:@{@"kind": @"action", @"section": @2, @"action": @0, @"title": @"导出功能配置", @"id": @"honggo.preferences.export"}];
        [rows addObject:@{@"kind": @"action", @"section": @2, @"action": @1, @"title": @"从文件恢复配置", @"id": @"honggo.preferences.import"}];
        [rows addObject:@{@"kind": @"action", @"section": @2, @"action": @2, @"title": @"关闭全部功能开关", @"id": @"honggo.preferences.reset", @"destructive": @YES}];
        [rows addObject:@{@"kind": @"appearance", @"group": @"backup", @"title": @"外观参数备份与重置", @"id": @"honggo.appearance.backup"}];
    } else if ([pid isEqual:@"diagnostics"]) {
        self.title = @"诊断与其他";
        [rows addObject:@{@"kind": @"enhanced", @"title": @"增强诊断", @"id": @"honggo.diagnostics.enhanced"}];
        [rows addObject:@{@"kind": @"action", @"section": @1, @"action": @1, @"title": @"导出最近一小时日志", @"id": @"honggo.diagnostics.export"}];
        [rows addObject:@{@"kind": @"action", @"section": @1, @"action": @2, @"title": @"清除诊断日志", @"id": @"honggo.diagnostics.clear"}];
        [rows addObject:@{@"kind": @"action", @"section": @1, @"action": @3, @"title": @"本次启动兼容状态", @"id": @"honggo.compatibility.open"}];
    } else if ([pid isEqual:@"community"]) {
        self.title = @"作者与频道";
        [rows addObject:@{@"kind": @"info", @"title": @"当前版本", @"detail": @"Honggo " HG_PLUGIN_VERSION, @"id": @"honggo.about.version"}];
        [rows addObject:@{@"kind": @"community", @"title": @"TG 更新频道", @"detail": @"@Naloiafeed · 打开频道或复制地址", @"id": @"honggo.community.channel"}];
    }
    self.rows = rows;
    [self.tableView reloadData];
}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section { return self.rows.count; }
- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
    NSString *pid = self.pageID ?: @"";
    if ([pid isEqual:@"diagnostics"])
        return @"增强诊断即时生效。复现广告或遮罩后截屏，再回来导出日志；仅记录控件结构，不读取截图像素或页面文字。本次启动保留最近 10 份现场，导出前不要退出应用。";
    if ([pid isEqual:@"backup"])
        return @"配置文件仅保存 Honggo 的功能开关。恢复前会核对格式并显示变更数量；账号、宿主设置和增强诊断选择保持不变。恢复或关闭全部开关后，请完全退出再打开。";
    if ([pid isEqual:@"community"])
        return @"查看插件更新与测试版公告。只有点击打开或复制时才执行操作，不会自动加入频道。";
    if ([pid isEqual:@"base"]) {
        NSString *mode = HGNativeGlassConfigured() ?
            @"本次启动已配置系统玻璃。关闭两个原有玻璃开关及外观定制并重启后，插件停止启用该配置。" :
            @"iOS 26 及以上打开玻璃开关并重启后，会在启动时请求系统玻璃；旧系统底栏使用模糊材质。";
        return [@"功能开关更改后请完全退出红果短剧，再重新打开。" stringByAppendingString:mode];
    }
    if ([pid hasPrefix:@"ads."]) return @"功能开关更改后请完全退出红果短剧，再重新打开。";
    if ([pid isEqual:@"ads"]) return @"按场景进入三级页面管理拦截开关，更改后请完全退出红果短剧再打开。";
    return nil;
}
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    NSDictionary *row = self.rows[indexPath.row];
    NSString *kind = row[@"kind"];
    if ([kind isEqual:@"feature"]) {
        HGFeature feature = [row[@"feature"] unsignedIntegerValue];
        UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:nil];
        cell.selectionStyle = UITableViewCellSelectionStyleNone;
        NSString *title = HGSettingsFeatureTitle(feature);
        cell.textLabel.text = title;
        cell.detailTextLabel.text = feature == HGGlassControls && NSProcessInfo.processInfo.operatingSystemVersion.majorVersion < 26 ?
            @"需要 iOS 26 及以上；当前系统保留原按钮。" : @"更改后完全退出应用再打开";
        cell.textLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
        cell.detailTextLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleFootnote];
        cell.textLabel.adjustsFontForContentSizeCategory = YES;
        cell.detailTextLabel.adjustsFontForContentSizeCategory = YES;
        cell.textLabel.numberOfLines = 0;
        cell.detailTextLabel.numberOfLines = 0;
        UISwitch *toggle = [UISwitch new];
        toggle.tag = (NSInteger)feature;
        toggle.accessibilityIdentifier = [@"hg.option." stringByAppendingString:HGFeatureIdentifier(feature)];
        toggle.accessibilityLabel = title;
        toggle.on = HGIsEnabled(feature);
        [toggle addTarget:self.rootSettings action:@selector(toggleChanged:) forControlEvents:UIControlEventValueChanged];
        cell.accessoryView = toggle;
        return cell;
    }
    UITableViewCellStyle style = row[@"detail"] ? UITableViewCellStyleSubtitle : UITableViewCellStyleDefault;
    UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:style reuseIdentifier:nil];
    cell.textLabel.text = row[@"title"];
    cell.detailTextLabel.text = row[@"detail"];
    cell.textLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
    cell.detailTextLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleFootnote];
    cell.textLabel.adjustsFontForContentSizeCategory = YES;
    cell.detailTextLabel.adjustsFontForContentSizeCategory = YES;
    cell.textLabel.numberOfLines = 0;
    cell.detailTextLabel.numberOfLines = 0;
    cell.accessibilityIdentifier = row[@"id"];
    if ([kind isEqual:@"enhanced"]) {
        UISwitch *toggle = [UISwitch new];
        toggle.on = HGEnhancedDiagnostics();
        toggle.accessibilityLabel = row[@"title"];
        [toggle addTarget:self.rootSettings action:@selector(enhancedChanged:) forControlEvents:UIControlEventValueChanged];
        cell.accessoryView = toggle;
        cell.selectionStyle = UITableViewCellSelectionStyleNone;
    } else if ([kind isEqual:@"info"]) {
        cell.selectionStyle = UITableViewCellSelectionStyleNone;
    } else {
        cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
        if ([row[@"destructive"] boolValue]) cell.textLabel.textColor = UIColor.systemRedColor;
    }
    return cell;
}
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    NSDictionary *row = self.rows[indexPath.row];
    NSString *kind = row[@"kind"];
    if ([kind isEqual:@"subpage"]) {
        HGSettingsSubpageController *next = [[HGSettingsSubpageController alloc] initWithStyle:UITableViewStyleInsetGrouped];
        next.pageID = row[@"page"];
        next.rootSettings = self.rootSettings;
        [self.navigationController pushViewController:next animated:YES];
    } else if ([kind isEqual:@"appearance"]) {
        HGOpenAppearance(self, row[@"group"]);
    } else if ([kind isEqual:@"community"]) {
        HGShowCommunity(self);
    } else if ([kind isEqual:@"action"]) {
        [self.rootSettings performAction:[row[@"action"] unsignedIntegerValue]
            inSection:[row[@"section"] integerValue] presenter:self];
    }
}
@end

@implementation HGSettingsController
- (void)viewDidLoad {
    [super viewDidLoad];
    self.title = HG_PLUGIN_NAME;
    HGDiagnosticsStart(nil, @{@"os": NSProcessInfo.processInfo.operatingSystemVersionString});
    HGLog(@"settings.open", @{@"nativeGlass": @(HGNativeGlassConfigured())});
    self.tableView.accessibilityIdentifier = @"honggo.settings.options";
    self.tableView.rowHeight = UITableViewAutomaticDimension;
    self.tableView.estimatedRowHeight = 56;
    self.navigationItem.rightBarButtonItem = [[UIBarButtonItem alloc]
        initWithBarButtonSystemItem:UIBarButtonSystemItemDone target:self action:@selector(closeSettings)];
    // Level 1 mirrors FanqieX: clean category entries only, zero inline switches.
    self.rootCategories = @[
        @{@"kind": @"appearance", @"group": @"", @"title": @"外观", @"id": @"honggo.settings.category.appearance"},
        @{@"kind": @"appearance", @"group": @"dock", @"title": @"悬浮底栏", @"id": @"honggo.settings.category.dock"},
        @{@"kind": @"appearance", @"group": @"player", @"title": @"播放布局与玻璃", @"id": @"honggo.settings.category.player"},
        @{@"kind": @"subpage", @"page": @"ads", @"title": @"广告与推广拦截", @"id": @"honggo.settings.category.ads"},
        @{@"kind": @"subpage", @"page": @"base", @"title": @"基础与兼容开关", @"id": @"honggo.settings.category.base"},
        @{@"kind": @"subpage", @"page": @"backup", @"title": @"导出与备份", @"id": @"honggo.settings.category.backup"},
        @{@"kind": @"subpage", @"page": @"diagnostics", @"title": @"诊断与其他", @"id": @"honggo.settings.category.diagnostics"},
        @{@"kind": @"subpage", @"page": @"community", @"title": @"作者与频道", @"id": @"honggo.settings.category.community"}
    ];
    self.settingsSearch = [[UISearchController alloc] initWithSearchResultsController:nil];
    self.settingsSearch.searchResultsUpdater = self;
    self.settingsSearch.delegate = self;
    self.settingsSearch.obscuresBackgroundDuringPresentation = NO;
    self.settingsSearch.searchBar.placeholder = @"搜索所有设置";
    self.settingsSearch.searchBar.accessibilityIdentifier = @"honggo.settings.search";
    self.navigationItem.searchController = self.settingsSearch;
    self.navigationItem.hidesSearchBarWhenScrolling = NO;
    self.definesPresentationContext = YES;
    [self updateSearchResultsForSearchController:self.settingsSearch];
}
- (BOOL)isFiltering {
    return self.activeQuery.length > 0;
}
- (NSArray<NSString *> *)diagnosticTitles {
    return @[@"增强诊断", @"导出最近一小时日志", @"清除诊断日志", @"本次启动兼容状态"];
}
- (NSArray<NSString *> *)configurationTitles { return @[@"导出功能配置", @"从文件恢复配置", @"关闭全部功能开关"]; }
- (void)updateSearchResultsForSearchController:(UISearchController *)searchController {
    NSString *trimmed = [searchController.searchBar.text ?: @""
        stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
    self.activeQuery = trimmed;
    if (!trimmed.length) {
        self.visibleFeatures = @[];
        self.visibleDiagnostics = @[];
        self.visibleConfiguration = @[];
        self.communityVisible = NO;
        self.tableView.backgroundView = nil;
        [self.tableView reloadData];
        return;
    }
    NSMutableArray *features = [NSMutableArray array], *diagnostics = [NSMutableArray array], *configuration = [NSMutableArray array];
    for (HGFeature feature = 0; feature < HGFeatureCount; feature++) {
        NSString *text = [HGFeatureSearchText(feature) stringByAppendingFormat:@" %@", HGSettingsFeatureTitle(feature)];
        if (HGSettingsMatches(text, trimmed)) [features addObject:@(feature)];
    }
    NSArray *diagnosticAliases = @[@"诊断 截屏 现场 排错 debug", @"导出 分享 日志 log", @"清除 删除 日志 log", @"兼容 状态 适配 版本 capability"];
    NSArray *configurationAliases = @[@"备份 导出 配置 backup export", @"备份 导入 恢复 配置 restore import", @"恢复 默认 重置 关闭 全部 reset"];
    for (NSUInteger index = 0; index < self.diagnosticTitles.count; index++) {
        if (HGSettingsMatches([self.diagnosticTitles[index] stringByAppendingFormat:@" %@", diagnosticAliases[index]], trimmed))
            [diagnostics addObject:@(index)];
    }
    for (NSUInteger index = 0; index < self.configurationTitles.count; index++) {
        if (HGSettingsMatches([self.configurationTitles[index] stringByAppendingFormat:@" %@", configurationAliases[index]], trimmed))
            [configuration addObject:@(index)];
    }
    self.visibleFeatures = features;
    self.visibleDiagnostics = diagnostics;
    self.visibleConfiguration = configuration;
    self.communityVisible = HGCommunityMatchesQuery(trimmed);
    UILabel *empty = nil;
    if (features.count + diagnostics.count + configuration.count == 0 && !self.communityVisible) {
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
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return self.isFiltering ? 4 : 1;
}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    if (!self.isFiltering) return section == 0 ? self.rootCategories.count : 0;
    if (section == 3) return self.communityVisible ? 1 : 0;
    return section == 0 ? self.visibleFeatures.count : section == 1 ? self.visibleDiagnostics.count :
        section == 2 ? self.visibleConfiguration.count : 0;
}
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    if (![self tableView:tableView numberOfRowsInSection:section]) return nil;
    if (!self.isFiltering) return @"版本 " HG_PLUGIN_VERSION;
    return @[@"版本 " HG_PLUGIN_VERSION, @"诊断与兼容", @"配置管理", @"更新与频道"][section];
}
- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
    if (![self tableView:tableView numberOfRowsInSection:section]) return nil;
    if (!self.isFiltering) return nil;
    if (section == 3) return @"查看插件更新与测试版公告。只有点击打开或复制时才执行操作，不会自动加入频道。";
    if (section == 1) return @"增强诊断即时生效。复现广告或遮罩后截屏，再回来导出日志；仅记录控件结构，不读取截图像素或页面文字。";
    if (section == 2) return @"配置文件仅保存 Honggo 的功能开关。恢复或关闭全部开关后，请完全退出再打开。";
    return @"功能开关更改后请完全退出红果短剧，再重新打开。";
}
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    if (!self.isFiltering) {
        NSDictionary *category = self.rootCategories[indexPath.row];
        UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil];
        cell.textLabel.text = category[@"title"];
        cell.textLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
        cell.textLabel.adjustsFontForContentSizeCategory = YES;
        cell.textLabel.numberOfLines = 0;
        cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
        cell.accessibilityIdentifier = category[@"id"];
        return cell;
    }
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
        HGFeature feature = self.visibleFeatures[indexPath.row].unsignedIntegerValue;
        NSString *title = HGSettingsFeatureTitle(feature);
        cell.textLabel.text = title;
        cell.detailTextLabel.text = feature == HGGlassControls && NSProcessInfo.processInfo.operatingSystemVersion.majorVersion < 26 ?
            @"需要 iOS 26 及以上；当前系统保留原按钮。" : @"更改后完全退出应用再打开";
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
- (UIViewController *)activePresenter:(UIViewController *)preferred {
    UIViewController *top = preferred ?: self.navigationController.topViewController ?: self;
    return top.viewIfLoaded.window ? top : self;
}
- (void)showMessage:(NSString *)title detail:(NSString *)detail on:(UIViewController *)presenter {
    UIViewController *host = [self activePresenter:presenter];
    if (!host.viewIfLoaded.window || host.presentedViewController) return;
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:title message:detail preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"好" style:UIAlertActionStyleDefault handler:nil]];
    [host presentViewController:alert animated:YES completion:nil];
}
- (void)showDiagnosticError:(NSError *)error on:(UIViewController *)presenter {
    [self showMessage:@"日志操作失败" detail:[NSString stringWithFormat:@"%@ (%ld)", error.domain, (long)error.code] on:presenter];
}
- (void)shareURL:(NSURL *)url on:(UIViewController *)presenter {
    UIViewController *host = [self activePresenter:presenter];
    if (!url || !host.viewIfLoaded.window || host.presentedViewController) return;
    UIActivityViewController *share = [[UIActivityViewController alloc] initWithActivityItems:@[url] applicationActivities:nil];
    share.popoverPresentationController.sourceView = host.view;
    share.popoverPresentationController.sourceRect = CGRectMake(CGRectGetMidX(host.view.bounds), CGRectGetMidY(host.view.bounds), 1, 1);
    [host presentViewController:share animated:YES completion:nil];
}
- (void)setPreferenceBusy:(BOOL)busy {
    self.preferenceOperationInFlight = busy;
    self.tableView.userInteractionEnabled = !busy;
}
- (void)exportPreferencesOn:(UIViewController *)presenter {
    if (self.preferenceOperationInFlight) return;
    [self setPreferenceBusy:YES];
    NSError *encodingError = nil;
    NSData *data = HGPreferencesExport(&encodingError);
    if (!data) {
        [self setPreferenceBusy:NO];
        [self showMessage:@"配置导出失败" detail:encodingError.localizedDescription on:presenter];
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
            if (saved) [self shareURL:url on:presenter];
            else [self showMessage:@"配置导出失败" detail:error.localizedDescription on:presenter];
        });
    });
}
- (void)confirmPreferenceRestore:(NSDictionary<NSString *, NSNumber *> *)features on:(UIViewController *)presenter {
    UIViewController *host = [self activePresenter:presenter];
    if (!host.viewIfLoaded.window || host.presentedViewController) return;
    NSUInteger changed = HGPreferencesChangeCount(features);
    if (!changed) { [self showMessage:@"无需恢复" detail:@"文件中的功能开关与当前设置一致。" on:host]; return; }
    NSString *message = [NSString stringWithFormat:@"将更新 %lu 项 Honggo 功能开关。文件未包含的开关保持当前值，账号和其他设置不受影响。恢复后请完全退出再打开红果。", (unsigned long)changed];
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"恢复功能配置？" message:message preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:nil]];
    [alert addAction:[UIAlertAction actionWithTitle:@"恢复" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
        NSError *error = nil;
        if (HGPreferencesApply(features, &error)) {
            HGLog(@"settings.restored", @{@"changed": @(changed)});
            [self.tableView reloadData];
        } else {
            dispatch_async(dispatch_get_main_queue(), ^{ [self showMessage:@"配置恢复失败" detail:error.localizedDescription on:host]; });
        }
    }]];
    [host presentViewController:alert animated:YES completion:nil];
}
- (void)readPreferenceURL:(NSURL *)url {
    if (self.preferenceOperationInFlight) return;
    if (!url.isFileURL) { [self showMessage:@"无法恢复配置" detail:@"请选择本地或文件提供方中的 JSON 文件。" on:nil]; return; }
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
            if (features) [self confirmPreferenceRestore:features on:nil];
            else [self showMessage:@"无法恢复配置" detail:error.localizedDescription ?: @"无法读取所选文件。" on:nil];
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
- (void)confirmResetOn:(UIViewController *)presenter {
    UIViewController *host = [self activePresenter:presenter];
    if (!host.viewIfLoaded.window || host.presentedViewController) return;
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
    [host presentViewController:alert animated:YES completion:nil];
}
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    if (self.preferenceOperationInFlight) return;
    if (!self.isFiltering) {
        NSDictionary *category = self.rootCategories[indexPath.row];
        NSString *kind = category[@"kind"];
        if ([kind isEqual:@"appearance"]) {
            NSString *group = [category[@"group"] length] ? category[@"group"] : nil;
            [self performAfterSearchDismissal:^{ HGOpenAppearance(self, group); }];
        } else if ([kind isEqual:@"subpage"]) {
            NSString *page = category[@"page"];
            [self performAfterSearchDismissal:^{
                HGSettingsSubpageController *sub = [[HGSettingsSubpageController alloc] initWithStyle:UITableViewStyleInsetGrouped];
                sub.pageID = page;
                sub.rootSettings = self;
                [self.navigationController pushViewController:sub animated:YES];
            }];
        }
        return;
    }
    if (indexPath.section == 0) return;
    if (indexPath.section == 3) {
        [self performAfterSearchDismissal:^{ HGShowCommunity(self); }];
        return;
    }
    NSInteger section = indexPath.section;
    NSArray<NSNumber *> *actions = section == 2 ? self.visibleConfiguration : self.visibleDiagnostics;
    NSUInteger action = actions[indexPath.row].unsignedIntegerValue;
    if (section == 1 && action == 0) return;
    [self performAfterSearchDismissal:^{ [self performAction:action inSection:section presenter:self]; }];
}
- (void)performAction:(NSUInteger)action inSection:(NSInteger)section presenter:(UIViewController *)presenter {
    UIViewController *host = [self activePresenter:presenter];
    if (section == 2) {
        if (action == 0) [self exportPreferencesOn:host];
        else if (action == 1) {
            UIDocumentPickerViewController *picker = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:@[UTTypeJSON] asCopy:YES];
            picker.delegate = self;
            picker.allowsMultipleSelection = NO;
            [host presentViewController:picker animated:YES completion:nil];
        } else [self confirmResetOn:host];
        return;
    }
    if (action == 3) {
        [host.navigationController pushViewController:[[HGCompatibilityController alloc] initWithStyle:UITableViewStyleInsetGrouped] animated:YES];
        return;
    }
    UITableView *tableView = [host isKindOfClass:UITableViewController.class] ? ((UITableViewController *)host).tableView : self.tableView;
    tableView.userInteractionEnabled = NO;
    if (action == 2) {
        HGClearDiagnostics(^(NSError *error) {
            tableView.userInteractionEnabled = YES;
            if (error) [self showDiagnosticError:error on:host];
        });
    } else {
        HGExportDiagnostics(HGInterfaceSnapshot(), ^(NSURL *url, NSError *error) {
            tableView.userInteractionEnabled = YES;
            if (error) [self showDiagnosticError:error on:host];
            else [self shareURL:url on:host];
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
