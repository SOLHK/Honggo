#import "Community.h"
// Real UIKit integration checks with a controlled host, not the Hongguo IPA.
#import "Honggo.h"
#import "AppearancePreferences.h"
#import "ReleaseNotes.h"
#import <dispatch/dispatch.h>
#import <math.h>

@interface SSSettingViewController : UIViewController <UITableViewDataSource, UITableViewDelegate>
@property(nonatomic, strong) UITableView *tableView;
@property(nonatomic) NSUInteger appearances;
@property(nonatomic, copy) NSArray<NSArray<NSNumber *> *> *cellItemsArrs;
- (void)updatePrivacyPolicyFooterView;
@end

@implementation SSSettingViewController
- (void)viewDidLoad {
    [super viewDidLoad];
    self.title = @"设置 · 模拟宿主";
    self.view.backgroundColor = UIColor.systemGroupedBackgroundColor;
    self.tableView = [[UITableView alloc] initWithFrame:self.view.bounds style:UITableViewStyleInsetGrouped];
    self.tableView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    self.tableView.dataSource = self;
    self.tableView.delegate = self;
    self.cellItemsArrs = @[@[@2, @3, @255], @[@0]];
    [self updatePrivacyPolicyFooterView];
    [self.view addSubview:self.tableView];
}
- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    self.appearances++;
}
- (void)updatePrivacyPolicyFooterView {
    self.tableView.tableFooterView = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, 320, 48)];
}
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return self.cellItemsArrs.count;
}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return self.cellItemsArrs[section].count;
}
- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)path {
    return 50;
}
- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section {
    return 12;
}
- (CGFloat)tableView:(UITableView *)tableView heightForFooterInSection:(NSInteger)section {
    return 8;
}
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"host"];
    if (!cell) cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"host"];
    cell.textLabel.text = @{@2:@"账号与安全", @3:@"隐私设置", @255:@"其他项目", @0:@"清理缓存"}[self.cellItemsArrs[indexPath.section][indexPath.row]];
    return cell;
}
@end

static UIButton *HGNavigationButton(UIView *view, NSString *identifier) {
    if ([view isKindOfClass:UIButton.class] && [view.accessibilityIdentifier isEqual:identifier]) return (UIButton *)view;
    for (UIView *child in view.subviews) {
        UIButton *button=HGNavigationButton(child,identifier);
        if (button) return button;
    }
    return nil;
}
static NSArray<UIView *> *HGFindMenus(UIView *view) {
    NSMutableArray<UIView *> *matches = [NSMutableArray array];
    if ([view.accessibilityIdentifier isEqualToString:@"honggo.settings.menu"]) {
        [matches addObject:view];
    }
    for (UIView *child in view.subviews) {
        [matches addObjectsFromArray:HGFindMenus(child)];
    }
    return matches;
}

// Observe public search lifecycle events without changing the plugin's handler.
// In particular, didDismiss must still reach HGSettingsController so a queued
// action is tested through the production path, never invoked by the fixture.
@interface HGSearchTransitionRecorder : NSObject <UISearchControllerDelegate>
@property(nonatomic, weak) id<UISearchControllerDelegate> downstream;
@property(nonatomic) NSUInteger presentations;
@property(nonatomic) NSUInteger dismissals;
@property(nonatomic, copy) void (^record)(NSString *);
@end

@implementation HGSearchTransitionRecorder
- (void)willPresentSearchController:(UISearchController *)searchController {
    if (self.record) self.record(@"delegate.willPresent");
    if ([self.downstream respondsToSelector:_cmd]) [self.downstream willPresentSearchController:searchController];
}
- (void)didPresentSearchController:(UISearchController *)searchController {
    self.presentations++;
    if (self.record) self.record(@"delegate.didPresent");
    if ([self.downstream respondsToSelector:_cmd]) [self.downstream didPresentSearchController:searchController];
}
- (void)willDismissSearchController:(UISearchController *)searchController {
    if (self.record) self.record(@"delegate.willDismiss");
    if ([self.downstream respondsToSelector:_cmd]) [self.downstream willDismissSearchController:searchController];
}
- (void)didDismissSearchController:(UISearchController *)searchController {
    self.dismissals++;
    if (self.record) self.record(@"delegate.didDismiss.beforeForward");
    if ([self.downstream respondsToSelector:_cmd]) [self.downstream didDismissSearchController:searchController];
    if (self.record) self.record(@"delegate.didDismiss.afterForward");
}
@end

@interface HGSettingsTests : UIResponder <UIWindowSceneDelegate>
@property(nonatomic, strong) UIWindow *window;
@property(nonatomic, strong) SSSettingViewController *host;
@property(nonatomic, strong) NSMutableArray<NSDictionary *> *checks;
@property(nonatomic, strong) HGSearchTransitionRecorder *searchEvents;
@property(nonatomic, strong) NSMutableArray<NSDictionary *> *searchStateTrace;
@end

@implementation HGSettingsTests
- (void)check:(BOOL)passed name:(NSString *)name {
    [self.checks addObject:@{@"name": name, @"passed": @(passed)}];
    NSLog(@"[HonggoSettingsTests] %@: %@", passed ? @"PASS" : @"FAIL", name);
}
- (void)layoutHost {
    [self.host.view setNeedsLayout];
    [self.host.view layoutIfNeeded];
    // Explicitly exercise the hooked lifecycle as well as UIKit's own callbacks.
    [self.host viewDidLayoutSubviews];
    [self.host.tableView layoutIfNeeded];
    [self.host.tableView.tableHeaderView layoutIfNeeded];
}
- (NSURL *)outputURL:(NSString *)name {
    NSURL *directory = [NSFileManager.defaultManager URLsForDirectory:NSDocumentDirectory
        inDomains:NSUserDomainMask].firstObject;
    [NSFileManager.defaultManager createDirectoryAtURL:directory withIntermediateDirectories:YES
        attributes:nil error:nil];
    return [directory URLByAppendingPathComponent:name];
}
- (void)capture:(NSString *)name {
    [self.window layoutIfNeeded];
    UIGraphicsImageRenderer *renderer = [[UIGraphicsImageRenderer alloc] initWithSize:self.window.bounds.size];
    UIImage *image = [renderer imageWithActions:^(UIGraphicsImageRendererContext *context) {
        [self.window drawViewHierarchyInRect:self.window.bounds afterScreenUpdates:YES];
    }];
    [UIImagePNGRepresentation(image) writeToURL:[self outputURL:name] atomically:YES];
}
- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session
        options:(UISceneConnectionOptions *)options {
    self.checks = [NSMutableArray array];
    self.searchStateTrace = [NSMutableArray array];
    [NSUserDefaults.standardUserDefaults removePersistentDomainForName:NSBundle.mainBundle.bundleIdentifier];
    [NSUserDefaults.standardUserDefaults setObject:@"preserved" forKey:@"host.setting"];
    [UIView setAnimationsEnabled:NO];
    [self check:HGInstallSettings() name:@"settings hook installs without a tab bar class"];
    [self check:HGInstallSettings() name:@"settings hook installation is idempotent"];
    self.host = [SSSettingViewController new];
    UIWindowScene *windowScene = (UIWindowScene *)scene;
    self.window = [[UIWindow alloc] initWithWindowScene:windowScene];
    self.window.frame = windowScene.coordinateSpace.bounds;
    self.window.rootViewController = [[UINavigationController alloc] initWithRootViewController:self.host];
    [self.window makeKeyAndVisible];
    dispatch_async(dispatch_get_main_queue(), ^{
        @try {
            [self runLayoutChecks];
            [self capture:@"settings-menu-simulator.png"];
            UIControl *menu = (UIControl *)HGFindMenus(self.host.tableView.tableHeaderView).firstObject;
            [self check:[menu isKindOfClass:UIControl.class] name:@"menu exposes a tappable UIKit control"];
            if ([menu isKindOfClass:UIControl.class]) {
                [menu sendActionsForControlEvents:UIControlEventTouchUpInside];
            }
            dispatch_async(dispatch_get_main_queue(), ^{
                @try {
                    [self runOptionsChecks];
                    [self capture:@"settings-options-simulator.png"];
                    [self runReleaseNotesWithCompletion:^{
                        [self runDiagnosticExportWithCompletion:^{
                            [self runSearchChecksWithCompletion:^{ [self finish]; }];
                        }];
                    }];
                } @catch (NSException *exception) {
                    [self check:NO name:[@"options exception: " stringByAppendingString:exception.description]];
                    [self finish];
                }
            });
        } @catch (NSException *exception) {
            [self check:NO name:[@"layout exception: " stringByAppendingString:exception.description]];
            [self finish];
        }
    });
}
- (void)runLayoutChecks {
    [self layoutHost];
    UITableView *table = self.host.tableView;
    UIView *header = table.tableHeaderView;
    UIView *menu = HGFindMenus(header).firstObject;
    [self check:menu.window != nil && !menu.hidden && menu.alpha > 0 && !HGIsEnabled(HGGlass) &&
        CGRectIntersectsRect([menu convertRect:menu.bounds toView:self.window], self.window.bounds)
        name:@"menu is visible with the glass switch off"];
    UILabel *menuTitle=[menu valueForKey:@"titleLabel"];
    NSUInteger labels=0;
    for (UIView *child in menu.subviews) if ([child isKindOfClass:UILabel.class]) labels++;
    [self check:labels==1 && [menu.accessibilityLabel isEqualToString:@"Honggo"] && !menu.accessibilityValue &&
        menuTitle.textAlignment==NSTextAlignmentCenter && fabs(CGRectGetMidX(menuTitle.frame)-CGRectGetMidX(menu.bounds))<0.5 &&
        fabs(CGRectGetMidY(menuTitle.frame)-CGRectGetMidY(menu.bounds))<0.5
        name:@"the settings entry centers its single Honggo title independently of the chevron"];
    [self check:table.dataSource == self.host && [table numberOfRowsInSection:0] == 3
        name:@"host data source and original rows are preserved"];
    NSDictionary *savedAppearance = HGAppearanceValues();
    NSIndexPath *account = [NSIndexPath indexPathForRow:0 inSection:0];
    NSIndexPath *unknown = [NSIndexPath indexPathForRow:2 inSection:0];
    NSIndexPath *cache = [NSIndexPath indexPathForRow:0 inSection:1];
    HGAppearanceApply(@{@"enabled":@YES, @"mine.settings.show.account":@NO, @"mine.settings.show.cache":@NO,
        @"mine.settings.show.privacyPolicy":@NO}, nil);
    [table layoutIfNeeded];
    [self check:[table rectForRowAtIndexPath:account].size.height == 0 &&
        [table rectForRowAtIndexPath:unknown].size.height == 50 && [table rectForRowAtIndexPath:cache].size.height == 0 &&
        [table rectForHeaderInSection:1].size.height < 1 && [table rectForFooterInSection:1].size.height < 1 &&
        table.tableFooterView == nil && [table numberOfRowsInSection:0] == 3
        name:@"individual host rows, empty group spacing and the privacy footer hide without renumbering unknown items"];
    [self.host updatePrivacyPolicyFooterView];
    [self check:table.tableFooterView == nil name:@"host rebuilding the footer honors its switch"];
    HGAppearanceSet(@"enabled", @NO, nil);
    [table layoutIfNeeded];
    [self check:[table rectForRowAtIndexPath:account].size.height == 50 && [table rectForFooterInSection:1].size.height >= 8 &&
        table.tableFooterView != nil name:@"disabling customization restores host row heights and the original footer"];
    HGAppearanceSet(@"enabled", @YES, nil);
    HGAppearanceSet(@"mine.settings.show.account", @YES, nil);
    [table layoutIfNeeded];
    [self check:[table rectForRowAtIndexPath:account].size.height == 50 && [table rectForRowAtIndexPath:cache].size.height == 0 &&
        [table cellForRowAtIndexPath:account] && ![table cellForRowAtIndexPath:account].hidden && table.tableFooterView == nil
        name:@"showing one row restores its host cell while other saved switches stay hidden"];
    HGAppearanceApply(savedAppearance, nil);
    [self check:menu.bounds.size.height >= 44 && menu.bounds.size.width > 44 &&
        menu.isAccessibilityElement && (menu.accessibilityTraits & UIAccessibilityTraitButton)
        name:@"menu has a usable touch target and VoiceOver button semantics"];

    NSUInteger appearances = self.host.appearances;
    for (NSUInteger index = 0; index < 4; index++) {
        [self.host viewWillAppear:NO];
        [self.host viewDidAppear:NO];
        [self layoutHost];
    }
    [self check:table.tableHeaderView == header && HGFindMenus(header).count == 1
        name:@"repeated appearances reuse one menu header"];
    [self check:self.host.appearances == appearances + 4
        name:@"original host appearance implementation still executes once"];

    UIView *original = [[UIView alloc] initWithFrame:CGRectMake(0, 0, table.bounds.size.width, 52)];
    original.accessibilityIdentifier = @"host.header";
    original.backgroundColor = UIColor.tertiarySystemGroupedBackgroundColor;
    table.tableHeaderView = original;
    [self layoutHost];
    header = table.tableHeaderView;
    menu = HGFindMenus(header).firstObject;
    [self check:header != original && [original isDescendantOfView:header] &&
        fabs(original.bounds.size.height - 52) < 0.5 &&
        CGRectGetMinY([original convertRect:original.bounds toView:header]) >=
            CGRectGetMaxY([menu convertRect:menu.bounds toView:header])
        name:@"existing host header is retained below the new menu"];

    CGRect previousFrame = table.frame;
    table.autoresizingMask = UIViewAutoresizingNone;
    CGRect narrowFrame = previousFrame;
    narrowFrame.size.width = 320;
    table.frame = narrowFrame;
    [self layoutHost];
    [self check:fabs(table.tableHeaderView.bounds.size.width - table.bounds.size.width) < 0.5 &&
        CGRectGetMaxX(menu.frame) <= table.bounds.size.width + 0.5 && menu.bounds.size.width > 44
        name:@"header and menu resize with table width"];
    table.frame = previousFrame;
    table.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;

    UIView *replacement = [[UIView alloc] initWithFrame:CGRectMake(0, 0, table.bounds.size.width, 88)];
    table.tableHeaderView = replacement;
    [self layoutHost];
    [self check:[replacement isDescendantOfView:table.tableHeaderView] &&
        ![original isDescendantOfView:table.tableHeaderView] && HGFindMenus(table.tableHeaderView).count == 1
        name:@"host header replacement retains only the current content"];
    table.tableHeaderView = nil;
    [self layoutHost];
    [self check:HGFindMenus(table.tableHeaderView).count == 1 &&
        ![replacement isDescendantOfView:table.tableHeaderView]
        name:@"host clearing its header restores a single standalone menu"];

    UIViewController *child = [UIViewController new];
    [self.host addChildViewController:child];
    child.view.frame = self.host.view.bounds;
    UITableView *childTable = [[UITableView alloc] initWithFrame:child.view.bounds style:UITableViewStylePlain];
    [child.view addSubview:childTable];
    [self.host.view addSubview:child.view];
    [child didMoveToParentViewController:self.host];
    [self layoutHost];
    [self check:childTable.tableHeaderView == nil && HGFindMenus(table.tableHeaderView).count == 1
        name:@"embedded child controller tables are left untouched"];
    [child willMoveToParentViewController:nil];
    [child.view removeFromSuperview];
    [child removeFromParentViewController];
    [self layoutHost];
}
- (void)runOptionsChecks {
    UIViewController *presented = self.host.presentedViewController;
    BOOL isNavigation = [presented isKindOfClass:UINavigationController.class];
    HGSettingsController *settings = isNavigation ? (HGSettingsController *)
        ((UINavigationController *)presented).topViewController : nil;
    BOOL hasSettings = [settings isKindOfClass:HGSettingsController.class];
    [self check:hasSettings name:@"tapping the menu opens the real plugin settings controller"];
    if (!hasSettings) {
        return;
    }
    [settings loadViewIfNeeded];
    UIButton *version = HGNavigationButton(settings.navigationItem.leftBarButtonItem.customView,@"honggo.settings.version");
    UIButton *done=HGNavigationButton(settings.navigationItem.rightBarButtonItem.customView,@"honggo.settings.done");
    [self.window layoutIfNeeded];
    [self.searchStateTrace addObject:@{@"stage":@"navigationCapsules",
        @"version":version.configuration.title ?: @"", @"done":done.configuration.title ?: @"",
        @"versionSize":NSStringFromCGSize(version.bounds.size), @"doneSize":NSStringFromCGSize(done.bounds.size),
        @"versionCorner":@(version.configuration.cornerStyle), @"doneCorner":@(done.configuration.cornerStyle)}];
    [self check:[settings.title isEqualToString:@"Honggo"] && [version isKindOfClass:UIButton.class] &&
        [version.configuration.title isEqual:HG_PLUGIN_VERSION] && [done isKindOfClass:UIButton.class] &&
        [done.configuration.title isEqual:@"完成"] && done.enabled && version.bounds.size.height>=44 &&
        CGSizeEqualToSize(version.bounds.size,done.bounds.size) &&
        version.configuration.cornerStyle==UIButtonConfigurationCornerStyleCapsule &&
        done.configuration.cornerStyle==UIButtonConfigurationCornerStyleCapsule
        name:@"the settings homepage presents equal native capsules for release history and Done"];
    [settings.tableView reloadData];
    [settings.tableView layoutIfNeeded];
    BOOL level1HasNoInlineSwitches = settings.tableView.numberOfSections == 1 &&
        ![settings tableView:settings.tableView titleForHeaderInSection:0];
    NSMutableArray *titles = [NSMutableArray array];
    for (NSInteger section = 0; section < settings.tableView.numberOfSections; section++) {
        for (NSInteger r = 0; r < [settings.tableView numberOfRowsInSection:section]; r++) {
            UITableViewCell *row = [settings tableView:settings.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:r inSection:section]];
            if ([row.accessoryView isKindOfClass:UISwitch.class] || row.accessoryType != UITableViewCellAccessoryDisclosureIndicator)
                level1HasNoInlineSwitches = NO;
            if (row.textLabel.text) [titles addObject:row.textLabel.text];
        }
    }
    [self check:level1HasNoInlineSwitches && [titles isEqual:@[@"页面与导航",@"播放页",@"底栏 Dock",@"短剧",
        @"我的页面",@"评论区",@"外观",@"广告与推广",@"导出与备份",@"诊断日志",@"作者与频道"]]
        name:@"the home lists eleven feature destinations in one section"];

    // Verify Level-2 and Level-3 subpages expose all 18 switches, diagnostics, backup, and channel.
    Class subpageClass = NSClassFromString(@"HGSettingsSubpageController");
    NSIndexPath *logsPath=[NSIndexPath indexPathForRow:9 inSection:0];
    UITableViewCell *logsEntry=[settings tableView:settings.tableView cellForRowAtIndexPath:logsPath];
    [self check:[logsEntry.textLabel.text isEqual:@"诊断日志"] && HGEnhancedDiagnostics()
        name:@"the settings homepage exposes diagnostics directly"];

    UITableViewController *backupPage = [(UITableViewController *)[subpageClass alloc] initWithStyle:UITableViewStyleInsetGrouped];
    [backupPage setValue:@"backup" forKey:@"pageID"];
    [backupPage setValue:settings forKey:@"rootSettings"];
    [backupPage loadViewIfNeeded];
    [self check:[backupPage.tableView numberOfRowsInSection:0] == 2
        name:@"backup subpage exposes only the feature configuration export and restore"];

    UITableViewController *adsL2 = [(UITableViewController *)[subpageClass alloc] initWithStyle:UITableViewStyleInsetGrouped];
    [adsL2 setValue:@"ads" forKey:@"pageID"];
    [adsL2 setValue:settings forKey:@"rootSettings"];
    [adsL2 loadViewIfNeeded];
    [self check:[adsL2.tableView numberOfRowsInSection:0] == 4
        name:@"ads level-2 page splits ad blocking into four level-3 categories"];

    NSArray<NSString *> *keys = @[@"glass", @"splash", @"topview", @"feed", @"comment", @"pause", @"patch",
        @"commerce", @"marketing", @"redPacket", @"bookstoreAd", @"couponPendant", @"miniGamePendant",
        @"welfareGuide", @"survey", @"glassControls", @"sidebarGameCenter", @"screenshotShopping"];
    NSMutableSet<NSNumber *> *seenFeatures = [NSMutableSet set];
    BOOL saved = YES;
    for (NSString *pid in @[@"base", @"ads.startup", @"ads.playback", @"ads.popups", @"ads.misc"]) {
        UITableViewController *sub = [(UITableViewController *)[subpageClass alloc] initWithStyle:UITableViewStyleInsetGrouped];
        [sub setValue:pid forKey:@"pageID"];
        [sub setValue:settings forKey:@"rootSettings"];
        [sub loadViewIfNeeded];
        NSInteger count = [sub.tableView numberOfRowsInSection:0];
        for (NSInteger r = 0; r < count; r++) {
            UITableViewCell *cell = [sub tableView:sub.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:r inSection:0]];
            UISwitch *toggle = (UISwitch *)cell.accessoryView;
            NSUInteger index = (NSUInteger)toggle.tag;
            [seenFeatures addObject:@(index)];
            BOOL desired = index % 2 == 0;
            BOOL initialOn=index!=HGGlass && index!=HGGlassControls;
            saved = saved && [toggle isKindOfClass:UISwitch.class] && toggle.on==initialOn && index < HGFeatureCount;
            toggle.on = desired;
            [toggle sendActionsForControlEvents:UIControlEventValueChanged];
            NSString *key = [@"HongguoGlass." stringByAppendingString:keys[index]];
            saved = saved && [NSUserDefaults.standardUserDefaults boolForKey:key] == desired &&
                HGIsEnabled((HGFeature)index) == desired;
        }
    }
    [self check:seenFeatures.count == HGFeatureCount && HGFeatureCount == 18
        name:@"level-2 and level-3 subpages preserve all 18 feature switches without omission"];
    [self check:saved name:@"switch events across subpages write the existing preference keys"];
    BOOL restored = YES;
    for (NSString *pid in @[@"base", @"ads.startup", @"ads.playback", @"ads.popups", @"ads.misc"]) {
        UITableViewController *reopened = [(UITableViewController *)[subpageClass alloc] initWithStyle:UITableViewStyleInsetGrouped];
        [reopened setValue:pid forKey:@"pageID"];
        [reopened setValue:settings forKey:@"rootSettings"];
        [reopened loadViewIfNeeded];
        NSInteger count = [reopened.tableView numberOfRowsInSection:0];
        for (NSInteger r = 0; r < count; r++) {
            UITableViewCell *cell = [reopened tableView:reopened.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:r inSection:0]];
            UISwitch *toggle = (UISwitch *)cell.accessoryView;
            restored = restored && toggle.isOn == (((NSUInteger)toggle.tag) % 2 == 0);
        }
    }
    [self check:restored name:@"fresh subpage controllers restore saved switch values"];
    [self check:[[NSUserDefaults.standardUserDefaults stringForKey:@"host.setting"] isEqualToString:@"preserved"]
        name:@"unrelated host preferences remain unchanged"];
    UIControl *menu = (UIControl *)HGFindMenus(self.host.tableView.tableHeaderView).firstObject;
    [menu sendActionsForControlEvents:UIControlEventTouchUpInside];
    [self check:self.host.presentedViewController == presented && !presented.presentedViewController
        name:@"repeated menu action does not stack a second settings sheet"];
    [settings.tableView reloadData];
}
- (void)filterSettings:(HGSettingsController *)settings text:(NSString *)text {
    UISearchController *search = settings.navigationItem.searchController;
    search.searchBar.text = text;
    [search.searchResultsUpdater updateSearchResultsForSearchController:search];
    [settings.tableView layoutIfNeeded];
}
- (UITableViewCell *)settingsCell:(HGSettingsController *)settings section:(NSInteger)section row:(NSInteger)row {
    if ([settings.tableView numberOfRowsInSection:section] <= row) return nil;
    return [settings tableView:settings.tableView cellForRowAtIndexPath:
        [NSIndexPath indexPathForRow:row inSection:section]];
}
- (void)runReleaseNotesWithCompletion:(dispatch_block_t)completion {
    UINavigationController *navigation=(id)self.host.presentedViewController;
    UIViewController *settings=navigation.topViewController;
    UIButton *version=HGNavigationButton(settings.navigationItem.leftBarButtonItem.customView,@"honggo.settings.version");
    [version sendActionsForControlEvents:UIControlEventTouchUpInside];
    [self poll:^BOOL {
        UIViewController *presented=navigation.presentedViewController;
        return [presented isKindOfClass:UINavigationController.class] && !presented.isBeingPresented;
    } deadline:[NSDate dateWithTimeIntervalSinceNow:6] completion:^(BOOL opened) {
        [self check:opened name:@"the version button opens the current-version release notes card"];
        if(!opened){completion();return;}
        UINavigationController *notesNav=(UINavigationController *)navigation.presentedViewController;
        UITableViewController *notes=(id)notesNav.topViewController;
        if(@available(iOS 26.1,*)) {
            NSArray *detents=notesNav.sheetPresentationController.detents;
            BOOL frosted=detents.count==2;
            for(UISheetPresentationControllerDetent *detent in detents) frosted &= [detent.backgroundEffect isKindOfClass:UIBlurEffect.class];
            [self check:frosted name:@"both sizes of the release notes card use a frosted sheet material"];
        }
        UITableView *table=notes.tableView;
        [table layoutIfNeeded];
        NSIndexPath *first=[NSIndexPath indexPathForRow:0 inSection:0];
        UITableViewCell *currentCell=[table.dataSource tableView:table cellForRowAtIndexPath:first];
        UIListContentConfiguration *current=(id)currentCell.contentConfiguration;
        NSString *footer=[table.dataSource tableView:table titleForFooterInSection:0];
        [self check:[table numberOfSections]==1 && [table numberOfRowsInSection:0]==1 &&
            [current.text isEqual:HGReleaseNotes().firstObject.firstObject] &&
            current.secondaryAttributedText.length>0 && [current.secondaryAttributedText.string containsString:@"• "] &&
            [currentCell.accessibilityIdentifier isEqual:@"honggo.settings.releaseNotes.currentCard"] &&
            [footer containsString:@"点击上方更新内容卡片"] && [footer containsString:@"作者与频道"]
            name:@"top-left version sheet displays only the current version card with bottom hint and no extra jump section"];
        [notes tableView:table didSelectRowAtIndexPath:first];
        [self poll:^BOOL {
            UIViewController *level3=notesNav.presentedViewController;
            return [level3 isKindOfClass:UINavigationController.class] && !level3.isBeingPresented;
        } deadline:[NSDate dateWithTimeIntervalSinceNow:5] completion:^(BOOL openedLevel3) {
            UINavigationController *level3Nav=(UINavigationController *)notesNav.presentedViewController;
            UITableViewController *allNotes=(id)level3Nav.topViewController;
            UITableView *allTable=allNotes.tableView;
            NSInteger sections=[allTable numberOfSections];
            NSIndexPath *last=[NSIndexPath indexPathForRow:0 inSection:MAX(sections-1,0)];
            if(sections)[allTable scrollToRowAtIndexPath:last atScrollPosition:UITableViewScrollPositionBottom animated:NO];
            [allTable layoutIfNeeded];
            NSString *allFooter=sections ? [allTable.dataSource tableView:allTable titleForFooterInSection:sections-1] : nil;
            [self check:openedLevel3 && sections==(NSInteger)HGReleaseNotes().count &&
                [allTable.indexPathsForVisibleRows containsObject:last] && [allFooter containsString:@"提示："]
                name:@"tapping the current version card opens a level-3 modal sheet with all formatted historical versions and footer hint"];
            [level3Nav.presentingViewController dismissViewControllerAnimated:NO completion:^{
                [navigation dismissViewControllerAnimated:NO completion:completion];
            }];
        }];
    }];
}
- (void)runDiagnosticExportWithCompletion:(dispatch_block_t)completion {
    UINavigationController *navigation = (id)self.host.presentedViewController;
    if (![navigation isKindOfClass:UINavigationController.class]) { completion(); return; }
    UITableViewController *settings=(id)navigation.topViewController;
    [settings tableView:settings.tableView didSelectRowAtIndexPath:[NSIndexPath indexPathForRow:9 inSection:0]];
    [self poll:^BOOL {
        UIViewController *page=navigation.topViewController;
        return [page isKindOfClass:NSClassFromString(@"HGDiagnosticSessionsController")] &&
            page.viewIfLoaded.window.windowScene.activationState==UISceneActivationStateForegroundActive &&
            page.viewIfLoaded.window && !navigation.transitionCoordinator &&
            !settings.presentedViewController && [[page valueForKey:@"sessions"] count]>0;
    } deadline:[NSDate dateWithTimeIntervalSinceNow:5] completion:^(BOOL opened) {
        [self check:opened name:@"one tap from settings opens diagnostic export and sessions"];
        if (!opened) { completion(); return; }
        UITableViewController *logs=(id)navigation.topViewController;
        UITableViewCell *clear=[logs tableView:logs.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:2]];
        [self check:[logs.tableView numberOfRowsInSection:0]==2 &&
            [clear.accessibilityIdentifier isEqual:@"honggo.diagnostics.clear"] name:@"log management retains both exports and clearing"];
        [self runExportOn:logs from:settings completion:completion];
    }];
}
- (void)runExportOn:(UITableViewController *)diagnostics from:(UIViewController *)settings completion:(dispatch_block_t)completion {
    UINavigationController *navigation=diagnostics.navigationController;
    NSIndexPath *path = [NSIndexPath indexPathForRow:0 inSection:0];
    [diagnostics tableView:diagnostics.tableView didSelectRowAtIndexPath:path];
    UITableViewCell *preparing = [diagnostics tableView:diagnostics.tableView cellForRowAtIndexPath:path];
    [self check:[preparing.accessoryView isKindOfClass:UIActivityIndicatorView.class] &&
        [preparing.detailTextLabel.text containsString:@"正在准备"] name:@"export shows immediate preparation status"];
    UIViewController *cover = [UIViewController new];
    [navigation pushViewController:cover animated:NO];
    [self poll:^BOOL { return ![[diagnostics valueForKey:@"exporting"] boolValue]; }
        deadline:[NSDate dateWithTimeIntervalSinceNow:30] completion:^(BOOL finished) {
        NSURL *file=[diagnostics valueForKey:@"readyExport"];
        BOOL ready=finished && [file checkResourceIsReachableAndReturnError:nil];
        if(!ready || navigation.topViewController!=cover)
            [self.searchStateTrace addObject:@{@"stage":@"coveredExport",@"finished":@(finished),
                @"covered":@(navigation.topViewController==cover),@"file":@(file!=nil),
                @"error":[diagnostics valueForKey:@"exportError"] ?: @""}];
        [self check:ready && navigation.topViewController==cover && !diagnostics.presentedViewController
            name:@"a completed export waits while its page is covered"];
        [navigation popToViewController:diagnostics animated:NO];
        [self poll:^BOOL {
            return [diagnostics.presentedViewController isKindOfClass:UIActivityViewController.class] &&
                !diagnostics.presentedViewController.isBeingPresented;
        } deadline:[NSDate dateWithTimeIntervalSinceNow:30] completion:^(BOOL shared) {
            [self check:shared name:@"returning to the diagnostics page presents the prepared native share sheet"];
            dispatch_block_t finish = ^{
                [navigation popToViewController:settings animated:NO];
                dispatch_async(dispatch_get_main_queue(), completion);
            };
            if (diagnostics.presentedViewController) [diagnostics dismissViewControllerAnimated:NO completion:finish];
            else finish();
        }];
    }];
}
- (void)poll:(BOOL (^)(void))condition deadline:(NSDate *)deadline completion:(void (^)(BOOL))completion {
    if (condition()) { completion(YES); return; }
    if (deadline.timeIntervalSinceNow <= 0) { completion(NO); return; }
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 30 * NSEC_PER_MSEC), dispatch_get_main_queue(), ^{
        [self poll:condition deadline:deadline completion:completion];
    });
}
- (void)recordSearchState:(NSString *)stage settings:(HGSettingsController *)settings extra:(NSDictionary *)extra {
    UISearchController *search = settings.navigationItem.searchController;
    UINavigationController *navigation = settings.navigationController;
    NSMutableDictionary *state = [@{
        @"stage": stage,
        @"time": @(NSDate.date.timeIntervalSince1970),
        @"sceneState": @(self.window.windowScene.activationState),
        @"applicationState": @(UIApplication.sharedApplication.applicationState),
        @"windowKey": @(self.window.isKeyWindow),
        @"settingsInWindow": @(settings.viewIfLoaded.window == self.window),
        @"settingsBeingPresented": @(settings.isBeingPresented),
        @"settingsBeingDismissed": @(settings.isBeingDismissed),
        @"navigationBeingPresented": @(navigation.isBeingPresented),
        @"navigationBeingDismissed": @(navigation.isBeingDismissed),
        @"settingsTransition": @(settings.transitionCoordinator != nil),
        @"navigationTransition": @(navigation.transitionCoordinator != nil),
        @"active": @(search.active),
        @"searchBeingPresented": @(search.isBeingPresented),
        @"searchBeingDismissed": @(search.isBeingDismissed),
        @"searchBarInWindow": @(search.searchBar.window != nil),
        @"searchFieldInWindow": @(search.searchBar.searchTextField.window != nil),
        @"searchBarFirstResponder": @(search.searchBar.isFirstResponder),
        @"searchFieldFirstResponder": @(search.searchBar.searchTextField.isFirstResponder),
        @"searchPresenter": NSStringFromClass(search.presentingViewController.class) ?: @"none",
        @"settingsPresented": NSStringFromClass(settings.presentedViewController.class) ?: @"none",
        @"navigationTop": NSStringFromClass(navigation.topViewController.class) ?: @"none",
        @"delegate": NSStringFromClass([(id)search.delegate class]) ?: @"none",
        @"forwardedDelegate": NSStringFromClass([(id)self.searchEvents.downstream class]) ?: @"none",
        @"forwardsDismissal": @([self.searchEvents.downstream respondsToSelector:@selector(didDismissSearchController:)]),
        @"presentations": @(self.searchEvents.presentations),
        @"dismissals": @(self.searchEvents.dismissals)
    } mutableCopy];
    if (extra) [state addEntriesFromDictionary:extra];
    [self.searchStateTrace addObject:state.copy];
    NSLog(@"[HonggoSettingsTests] SEARCH_STATE %@", state);
}
- (void)activateSearch:(HGSettingsController *)settings query:(NSString *)query completion:(void (^)(BOOL))completion {
    UISearchController *search = settings.navigationItem.searchController;
    [self recordSearchState:@"activation.waitForScene" settings:settings extra:@{@"query": query}];
    // The original fixture ran from willConnectToSession, before this cold
    // simulator's scene became active. Wait for the same state in which a user
    // can touch the settled settings sheet; an attached view alone is not enough.
    [self poll:^BOOL {
        UINavigationController *navigation = settings.navigationController;
        return self.window.windowScene.activationState == UISceneActivationStateForegroundActive &&
            UIApplication.sharedApplication.applicationState == UIApplicationStateActive &&
            self.window.isKeyWindow && settings.viewIfLoaded.window == self.window &&
            navigation.topViewController == settings && navigation.presentingViewController != nil &&
            !navigation.isBeingPresented && !navigation.isBeingDismissed &&
            !settings.isBeingPresented && !settings.isBeingDismissed &&
            !navigation.transitionCoordinator && !settings.transitionCoordinator &&
            settings.presentedViewController == nil;
    } deadline:[NSDate dateWithTimeIntervalSinceNow:8] completion:^(BOOL ready) {
        [self recordSearchState:ready ? @"activation.sceneReady" : @"activation.sceneTimeout"
            settings:settings extra:@{@"query": query}];
        [self check:ready name:[NSString stringWithFormat:@"settings is foreground key and settled before search (%@)", query]];
        if (!ready) {
            [self capture:@"settings-search-failure-simulator.png"];
            completion(NO);
            return;
        }
        NSUInteger presentationsBefore = self.searchEvents.presentations;
        // These are public interaction APIs. Do not reparent the search bar or
        // require its inactive toolbar representation to be the same UIView.
        BOOL beganEditing = [search.searchBar becomeFirstResponder];
        if (!beganEditing) beganEditing = [search.searchBar.searchTextField becomeFirstResponder];
        search.active = YES;
        [self filterSettings:settings text:query];
        [self recordSearchState:@"activation.requested" settings:settings
            extra:@{@"query": query, @"editingRequestAccepted": @(beganEditing)}];
        [self poll:^BOOL {
            return search.active && !search.isBeingPresented &&
                self.searchEvents.presentations > presentationsBefore;
        } deadline:[NSDate dateWithTimeIntervalSinceNow:8] completion:^(BOOL active) {
            [self recordSearchState:active ? @"activation.presented" : @"activation.timeout"
                settings:settings extra:@{@"query": query}];
            [self check:active name:[NSString stringWithFormat:@"native search actually presents before result action (%@)", query]];
            if (!active) [self capture:@"settings-search-failure-simulator.png"];
            completion(active);
        }];
    }];
}
- (void)runSearchChecksWithCompletion:(dispatch_block_t)completion {
    UINavigationController *navigation = (UINavigationController *)self.host.presentedViewController;
    HGSettingsController *settings = [navigation isKindOfClass:UINavigationController.class] ?
        (HGSettingsController *)navigation.topViewController : nil;
    if (![settings isKindOfClass:HGSettingsController.class]) { completion(); return; }
    UISearchController *search = settings.navigationItem.searchController;
    [self check:[search isKindOfClass:UISearchController.class] && search.searchResultsUpdater != nil &&
        [search.searchBar.accessibilityIdentifier isEqual:@"honggo.settings.search"]
        name:@"settings search uses the native UIKit search controller"];
    self.searchEvents = [HGSearchTransitionRecorder new];
    self.searchEvents.downstream = search.delegate;
    __weak HGSettingsTests *weakSelf = self;
    __weak HGSettingsController *weakSettings = settings;
    self.searchEvents.record = ^(NSString *stage) {
        HGSettingsTests *tests = weakSelf;
        HGSettingsController *panel = weakSettings;
        if (tests && panel) [tests recordSearchState:stage settings:panel extra:nil];
    };
    search.delegate = self.searchEvents;

    Class subpageClass = NSClassFromString(@"HGSettingsSubpageController");
    UITableViewController *communityPage = [(UITableViewController *)[subpageClass alloc] initWithStyle:UITableViewStyleInsetGrouped];
    [communityPage setValue:@"community" forKey:@"pageID"];
    [communityPage setValue:settings forKey:@"rootSettings"];
    [communityPage loadViewIfNeeded];
    UITableViewCell *versionRow = [communityPage tableView:communityPage.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]];
    UITableViewCell *communityRow = [communityPage tableView:communityPage.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:1 inSection:0]];
    [self check:[communityPage.tableView numberOfRowsInSection:0] == 2 &&
        [versionRow.accessibilityIdentifier isEqual:@"honggo.about.version"] &&
        versionRow.accessoryType == UITableViewCellAccessoryDisclosureIndicator &&
        [communityRow.accessibilityIdentifier isEqual:@"honggo.community.channel"]
        name:@"Telegram channel and interactive version history entry are visible in the author/channel subpage"];

    NSDictionary *savedAppearance = [HGAppearanceValues() copy];
    NSDictionary *savedFeatures = HGPreferencesParse(HGPreferencesExport(nil), nil);
    HGSetEnabled(HGSplash, NO);
    HGAppearanceApply(@{@"enabled": @YES, @"dock.height": @68, @"panels.commentInputMaterial": @"blur",
        @"video.blockedKeywords": @[@"测试词"]}, nil);
    NSError *backupErr = nil;
    NSData *fullBackupData = HGConfigurationBackupExport(&backupErr);
    NSDictionary *parsedFull = HGConfigurationBackupParse(fullBackupData, &backupErr);
    HGPreferencesReset();
    HGAppearanceReset();
    NSUInteger pendingChanges = HGConfigurationBackupChangeCount(parsedFull);
    BOOL appliedFull = HGConfigurationBackupApply(parsedFull, &backupErr);
    [self check:fullBackupData.length > 0 && parsedFull != nil && backupErr == nil &&
        pendingChanges >= 4 && appliedFull && !HGIsEnabled(HGSplash) &&
        HGAppearanceEnabled() && HGAppearanceNumber(@"dock.height") == 68 &&
        [HGAppearanceString(@"panels.commentInputMaterial") isEqual:@"blur"] &&
        [HGVideoBlockedKeywords() isEqual:@[@"测试词"]] &&
        HGConfigurationBackupChangeCount(parsedFull) == 0
        name:@"unified configuration backup exports and restores both feature switches and full appearance settings"];
    NSData *legacyPrefData = HGPreferencesExport(nil);
    NSDictionary *parsedLegacy = HGConfigurationBackupParse(legacyPrefData, &backupErr);
    [self check:parsedLegacy[@"features"] != nil && parsedLegacy[@"appearance"] == nil && backupErr == nil
        name:@"configuration restore remains backward-compatible with v1 feature-only backups"];
    HGPreferencesApply(savedFeatures, nil);
    HGAppearanceApply(savedAppearance, nil);
    [self check:[HGCommunityURLString() isEqual:@"https://t.me/Naloiafeed"]
        name:@"channel destination matches the owner supplied HTTPS address exactly"];
    for (NSString *query in @[@"TG", @"频道", @"nAlOiAfEeD", @"telegram 更新"]) {
        [self filterSettings:settings text:query];
        [self check:[settings.tableView numberOfRowsInSection:3] == 1 &&
            [settings.tableView numberOfRowsInSection:0] == 0 &&
            [[self settingsCell:settings section:3 row:0].accessibilityIdentifier isEqual:@"honggo.community.channel"]
            name:@"channel search uses a stable identity without modifying feature rows"];
    }
    [self check:!HGCommunityMatchesQuery(@"unrelated-channel-123")
        name:@"unmatched channel searches cannot expose a stale result"];
    [self filterSettings:settings text:@"信息流"];
    UISwitch *feed = (UISwitch *)[self settingsCell:settings section:0 row:0].accessoryView;
    [self check:[settings.tableView numberOfRowsInSection:0] == 1 && feed.tag == HGFeed &&
        [feed.accessibilityIdentifier isEqual:@"hg.option.feed"]
        name:@"search aliases find the feed filter by its stable feature identity"];

    [self filterSettings:settings text:@"侧栏 游戏中心"];
    UISwitch *sidebarGame = (UISwitch *)[self settingsCell:settings section:0 row:0].accessoryView;
    [self check:[settings.tableView numberOfRowsInSection:0] == 1 && sidebarGame.tag == HGSidebarGameCenter &&
        [sidebarGame.accessibilityIdentifier isEqual:@"hg.option.sidebarGameCenter"]
        name:@"search finds the independent sidebar Game Center switch"];

    [settings.tableView layoutIfNeeded];
    NSIndexPath *sidebarResult = [NSIndexPath indexPathForRow:0 inSection:0];
    BOOL enabledBeforeTap = HGIsEnabled(HGSidebarGameCenter);
    [settings tableView:settings.tableView didSelectRowAtIndexPath:sidebarResult];
    [self check:HGIsEnabled(HGSidebarGameCenter) != enabledBeforeTap
        name:@"the filtered switch row title toggles the same feature as its accessory"];
    [settings tableView:settings.tableView didSelectRowAtIndexPath:sidebarResult];

    [self filterSettings:settings text:@"截图 找同款"];
    UITableViewCell *screenshotRow = [self settingsCell:settings section:0 row:0];
    UISwitch *screenshot = (UISwitch *)screenshotRow.accessoryView;
    [self check:[settings.tableView numberOfRowsInSection:0] == 1 && screenshot.tag == HGScreenshotShopping &&
        [screenshot.accessibilityIdentifier isEqual:@"hg.option.screenshotShopping"] &&
        [screenshotRow.textLabel.text isEqual:@"隐藏截图识图找同款"]
        name:@"search finds the independent screenshot shopping switch"];

    [self filterSettings:settings text:@"直播 领券"];
    UISwitch *marketing = (UISwitch *)[self settingsCell:settings section:0 row:0].accessoryView;
    BOOL glassBefore = HGIsEnabled(HGGlass);
    [self check:[settings.tableView numberOfRowsInSection:0] == 1 && marketing.tag == HGMarketing
        name:@"multiple search terms must match the same feature"];
    marketing.on = NO;
    [marketing sendActionsForControlEvents:UIControlEventValueChanged];
    [self check:[marketing isKindOfClass:UISwitch.class] &&
        ![NSUserDefaults.standardUserDefaults boolForKey:@"HongguoGlass.marketing"] &&
        HGIsEnabled(HGGlass) == glassBefore
        name:@"filtered row zero changes marketing without changing original row zero"];

    [self filterSettings:settings text:@"gLaSs cOnTrOlS"];
    UISwitch *controls = (UISwitch *)[self settingsCell:settings section:0 row:0].accessoryView;
    [self check:[settings.tableView numberOfRowsInSection:0] == 1 && controls.tag == HGGlassControls
        name:@"English aliases match case-insensitively across multiple terms"];

    [self filterSettings:settings text:@"日志"];
    [self check:[settings.tableView numberOfRowsInSection:1] == 2 &&
        [[self settingsCell:settings section:1 row:0].accessibilityIdentifier isEqual:@"honggo.diagnostics.export"] &&
        [[self settingsCell:settings section:1 row:1].accessibilityIdentifier isEqual:@"honggo.diagnostics.clear"]
        name:@"filtered diagnostic rows retain export and clear identities"];
    for (NSString *removed in @[@"本次启动兼容状态", @"增强诊断"]) {
        [self filterSettings:settings text:removed];
        [self check:[settings.tableView numberOfRowsInSection:1] == 0
            name:@"search omits the removed diagnostic entries"];
    }
    [self filterSettings:settings text:@"restore import"];
    [self check:[settings.tableView numberOfRowsInSection:2] == 1 &&
        [[self settingsCell:settings section:2 row:0].accessibilityIdentifier isEqual:@"honggo.preferences.import"]
        name:@"restore result keeps its action identity at filtered row zero"];
    [self filterSettings:settings text:@"backup export"];
    [self check:[settings.tableView numberOfRowsInSection:2] == 1 &&
        [[self settingsCell:settings section:2 row:0].accessibilityIdentifier isEqual:@"honggo.preferences.export"]
        name:@"backup export is distinct from diagnostic log export"];
    [self filterSettings:settings text:@"reset"];
    [self check:[settings.tableView numberOfRowsInSection:2] == 0
        name:@"search cannot surface the removed global reset action"];
    [self filterSettings:settings text:@"双指 清屏"];
    [self check:[settings.tableView numberOfRowsInSection:4]==1 &&
        [[self settingsCell:settings section:4 row:0].accessibilityIdentifier isEqual:@"honggo.appearance.setting.gestures.pinchClean"]
        name:@"global search finds the two-finger gesture by its Chinese name"];
    [self filterSettings:settings text:@"我的 订单"];
    [self check:[settings.tableView numberOfRowsInSection:4]==1 &&
        [[self settingsCell:settings section:4 row:0].accessibilityIdentifier isEqual:@"honggo.appearance.setting.mine.hide.shortcut.store"]
        name:@"global search combines page and card names"];
    [self filterSettings:settings text:@"not-a-honggo-setting-123"];
    [self check:[settings.tableView numberOfRowsInSection:0] == 0 &&
        [settings.tableView numberOfRowsInSection:1] == 0 &&
        [settings.tableView numberOfRowsInSection:2] == 0 &&
        [settings.tableView.backgroundView.accessibilityIdentifier isEqual:@"honggo.settings.search.empty"]
        name:@"unmatched search has no stale rows and exposes an accessible empty state"];
    [self filterSettings:settings text:@" \n\t "];
    [self check:[settings.tableView numberOfSections] == 1 &&
        [settings.tableView numberOfRowsInSection:0] == 11 &&
        settings.tableView.backgroundView == nil
        name:@"whitespace search restores the clean level-1 category list and removes the empty state"];
    [self filterSettings:settings text:@""];
    [self check:[settings.tableView numberOfSections] == 1 &&
        [settings.tableView numberOfRowsInSection:0] == 11
        name:@"clearing the query restores the clean level-1 category list"];
    [self runSearchAction:0 settings:settings completion:completion];
}
- (void)runSearchAction:(NSUInteger)index settings:(HGSettingsController *)settings completion:(dispatch_block_t)completion {
    NSArray<NSString *> *queries = @[@"诊断日志", @"restore import", @"backup export", @"TG Naloiafeed"];
    NSArray<NSString *> *names = @[@"active search opens diagnostics after search dismissal",
        @"active search opens the JSON picker after search dismissal",
        @"active search exports configuration to the native share sheet",
        @"active search opens the Telegram menu only after search dismissal"];
    if (index >= queries.count) {
        [self activateSearch:settings query:@"玻璃" completion:^(BOOL active) {
            if (!active) { completion(); return; }
            NSUInteger dismissalsBefore = self.searchEvents.dismissals;
            UIButton *done = HGNavigationButton(settings.navigationItem.rightBarButtonItem.customView,@"honggo.settings.done");
            [self recordSearchState:@"close.requested" settings:settings extra:nil];
            [done sendActionsForControlEvents:UIControlEventTouchUpInside];
            [self poll:^BOOL { return self.host.presentedViewController == nil &&
                    self.searchEvents.dismissals > dismissalsBefore; }
                deadline:[NSDate dateWithTimeIntervalSinceNow:8] completion:^(BOOL closed) {
                [self recordSearchState:closed ? @"close.finished" : @"close.timeout" settings:settings extra:nil];
                [self check:closed name:@"Done dismisses the entire settings sheet while search is active"];
                if (!closed) [self capture:@"settings-search-failure-simulator.png"];
                completion();
            }];
        }];
        return;
    }
    NSDictionary *before = HGPreferencesParse(HGPreferencesExport(nil), nil);
    [self activateSearch:settings query:queries[index] completion:^(BOOL active) {
        if (!active) { completion(); return; }
        NSInteger section = index == 0 ? 1 : index == 3 ? 3 : 2;
        if ([settings.tableView numberOfRowsInSection:section] != 1) {
            [self check:NO name:@"active result list resolves exactly one action"];
            completion();
            return;
        }
        NSUInteger dismissalsBefore = self.searchEvents.dismissals;
        // The cold simulator launches the system ShareSheet service lazily.
        // Keep every presentation assertion; only that external-service startup
        // gets a larger bounded budget. Record elapsed time instead of sleeping.
        NSTimeInterval presentationBudget = index == 2 ? 30 : 8;
        NSDate *actionStarted = NSDate.date;
        [self recordSearchState:@"action.selected" settings:settings extra:@{@"query": queries[index],
            @"presentationBudgetSeconds": @(presentationBudget)}];
        [settings tableView:settings.tableView didSelectRowAtIndexPath:
            [NSIndexPath indexPathForRow:0 inSection:section]];
        [self poll:^BOOL {
            if (settings.navigationItem.searchController.active || self.searchEvents.dismissals <= dismissalsBefore) return NO;
            if (index == 0) return settings.navigationController.topViewController != settings;
            UIViewController *presented = settings.presentedViewController;
            Class expected = index == 3 ? UIAlertController.class :
                index == 1 ? UIDocumentPickerViewController.class : UIActivityViewController.class;
            return [presented isKindOfClass:expected] && !presented.isBeingPresented;
        } deadline:[NSDate dateWithTimeInterval:presentationBudget sinceDate:actionStarted] completion:^(BOOL performed) {
            [self recordSearchState:performed ? @"action.finished" : @"action.timeout"
                settings:settings extra:@{@"query": queries[index],
                    @"elapsedSeconds": @(-actionStarted.timeIntervalSinceNow),
                    @"presentationBudgetSeconds": @(presentationBudget)}];
            [self check:performed name:names[index]];
            if (!performed) {
                [self capture:@"settings-search-failure-simulator.png"];
                completion();
                return;
            }
            [self check:[HGPreferencesParse(HGPreferencesExport(nil), nil) isEqual:before] &&
                HGEnhancedDiagnostics() &&
                [[NSUserDefaults.standardUserDefaults stringForKey:@"host.setting"] isEqual:@"preserved"]
                name:@"opening configuration tools preserves feature and host settings with enhanced diagnostics enabled"];
            if (index == 0) {
                UIViewController *logs = settings.navigationController.topViewController;
                [logs loadViewIfNeeded];
                [self check:[logs.view.accessibilityIdentifier isEqual:@"honggo.diagnostics.sessions"] &&
                    [logs isKindOfClass:UITableViewController.class]
                    name:@"diagnostics search opens the session list"];
                [settings.navigationController popToViewController:settings animated:NO];
                dispatch_async(dispatch_get_main_queue(), ^{
                    [self runSearchAction:index + 1 settings:settings completion:completion];
                });
            } else {
                if (index == 3) {
                    UIAlertController *menu = (UIAlertController *)settings.presentedViewController;
                    NSMutableSet *titles = [NSMutableSet set];
                    for (UIAlertAction *action in menu.actions) [titles addObject:action.title];
                    [self check:[menu.message isEqual:HGCommunityURLString()] &&
                        [titles containsObject:@"打开频道"] && [titles containsObject:@"复制地址"] &&
                        [titles containsObject:@"取消"] && menu.actions.count == 3
                        name:@"channel menu shows the exact destination and explicit open copy cancel choices"];
                    [self capture:@"settings-community-simulator.png"];
                } else if (index == 1) {
                    UIDocumentPickerViewController *picker = (UIDocumentPickerViewController *)settings.presentedViewController;
                    [self check:!picker.allowsMultipleSelection && picker.delegate == (id)settings
                        name:@"restore uses a single-document native picker with a plugin delegate"];
                }
                [settings dismissViewControllerAnimated:NO completion:^{
                    [self runSearchAction:index + 1 settings:settings completion:completion];
                }];
            }
        }];
    }];
}
- (void)finish {
    NSUInteger failed = 0;
    for (NSDictionary *check in self.checks) {
        failed += ![check[@"passed"] boolValue];
    }
    NSDictionary *report = @{@"plugin": HG_PLUGIN_NAME, @"version": HG_PLUGIN_VERSION, @"scope": @"real UIKit with a simulated host; not a Hongguo device test",
        @"simulatorOS": UIDevice.currentDevice.systemVersion,
        @"status": failed == 0 ? @"passed" : @"failed", @"failed": @(failed),
        @"total": @(self.checks.count), @"checks": self.checks,
        @"searchStateTrace": self.searchStateTrace ?: @[]};
    NSData *data = [NSJSONSerialization dataWithJSONObject:report options:NSJSONWritingPrettyPrinted error:nil];
    [data writeToURL:[self outputURL:@"settings-results.json"] options:NSDataWritingAtomic error:nil];
}
@end

@interface HGSettingsTestAppDelegate : UIResponder <UIApplicationDelegate>
@end

@implementation HGSettingsTestAppDelegate
- (UISceneConfiguration *)application:(UIApplication *)application
        configurationForConnectingSceneSession:(UISceneSession *)session
        options:(UISceneConnectionOptions *)options {
    UISceneConfiguration *configuration = [[UISceneConfiguration alloc]
        initWithName:@"HonggoSettingsTest" sessionRole:session.role];
    configuration.delegateClass = HGSettingsTests.class;
    return configuration;
}
@end

int main(int argc, char *argv[]) {
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass(HGSettingsTestAppDelegate.class));
    }
}
