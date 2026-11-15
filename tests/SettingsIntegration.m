// Real UIKit integration checks with a controlled host, not the Hongguo IPA.
#import "Honggo.h"
#import <dispatch/dispatch.h>
#import <math.h>

@interface SSSettingViewController : UIViewController <UITableViewDataSource>
@property(nonatomic, strong) UITableView *tableView;
@property(nonatomic) NSUInteger appearances;
@end

@implementation SSSettingViewController
- (void)viewDidLoad {
    [super viewDidLoad];
    self.title = @"设置 · 模拟宿主";
    self.view.backgroundColor = UIColor.systemGroupedBackgroundColor;
    self.tableView = [[UITableView alloc] initWithFrame:self.view.bounds style:UITableViewStyleInsetGrouped];
    self.tableView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    self.tableView.dataSource = self;
    [self.view addSubview:self.tableView];
}
- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    self.appearances++;
}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return 3;
}
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil];
    cell.textLabel.text = @[@"账号与安全", @"隐私设置", @"关于应用"][indexPath.row];
    return cell;
}
@end

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

@interface HGSettingsTests : UIResponder <UIWindowSceneDelegate>
@property(nonatomic, strong) UIWindow *window;
@property(nonatomic, strong) SSSettingViewController *host;
@property(nonatomic, strong) NSMutableArray<NSDictionary *> *checks;
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
                    [self runSearchChecksWithCompletion:^{ [self finish]; }];
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
        name:@"menu is visible with all feature switches off"];
    [self check:[menu.accessibilityLabel isEqualToString:@"Honggo"] &&
        [menu.accessibilityValue containsString:HG_PLUGIN_VERSION]
        name:@"menu uses Honggo and shows the current plugin version"];
    [self check:table.dataSource == self.host && [table numberOfRowsInSection:0] == 3
        name:@"host data source and original rows are preserved"];
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
    [self check:[settings.title isEqualToString:@"Honggo"] &&
        [[settings tableView:settings.tableView titleForHeaderInSection:0] containsString:HG_PLUGIN_VERSION]
        name:@"settings title is Honggo with a separate plugin version"];
    [settings.tableView reloadData];
    [settings.tableView layoutIfNeeded];
    [self check:[settings.tableView numberOfRowsInSection:0] == HGFeatureCount && HGFeatureCount == 16
        name:@"settings preserves 15 switches and adds native control glass"];
    [self check:[settings.tableView numberOfRowsInSection:1] == 4
        name:@"settings exposes diagnostics export clear and compatibility"];
    [self check:[settings.tableView numberOfRowsInSection:2] == 3
        name:@"configuration management exposes export restore and reset"];
    UITableViewCell *diagnostics = [settings tableView:settings.tableView cellForRowAtIndexPath:
        [NSIndexPath indexPathForRow:0 inSection:1]];
    UISwitch *enhanced = (UISwitch *)diagnostics.accessoryView;
    [self check:!enhanced.on name:@"enhanced diagnostics is opt-in"];
    enhanced.on = YES;
    [enhanced sendActionsForControlEvents:UIControlEventValueChanged];
    [self check:HGEnhancedDiagnostics() name:@"enhanced diagnostics switch takes effect without restart"];
    NSArray<NSString *> *keys = @[@"glass", @"splash", @"topview", @"feed", @"comment", @"pause", @"patch",
        @"commerce", @"marketing", @"redPacket", @"bookstoreAd", @"couponPendant", @"miniGamePendant",
        @"welfareGuide", @"survey", @"glassControls"];
    BOOL saved = YES;
    for (NSUInteger index = 0; index < HGFeatureCount; index++) {
        NSIndexPath *path = [NSIndexPath indexPathForRow:(NSInteger)index inSection:0];
        UITableViewCell *cell = [settings tableView:settings.tableView cellForRowAtIndexPath:path];
        UISwitch *toggle = (UISwitch *)cell.accessoryView;
        BOOL desired = index % 2 == 0;
        saved = saved && [toggle isKindOfClass:UISwitch.class] && !toggle.on;
        toggle.on = desired;
        [toggle sendActionsForControlEvents:UIControlEventValueChanged];
        NSString *key = [@"HongguoGlass." stringByAppendingString:keys[index]];
        saved = saved && [NSUserDefaults.standardUserDefaults boolForKey:key] == desired &&
            HGIsEnabled((HGFeature)index) == desired;
    }
    [self check:saved name:@"switch events write the existing preference keys"];
    HGSettingsController *reopened = [[HGSettingsController alloc] initWithStyle:UITableViewStyleInsetGrouped];
    [reopened loadViewIfNeeded];
    BOOL restored = YES;
    for (NSUInteger index = 0; index < HGFeatureCount; index++) {
        UITableViewCell *cell = [reopened tableView:reopened.tableView
            cellForRowAtIndexPath:[NSIndexPath indexPathForRow:(NSInteger)index inSection:0]];
        restored = restored && [(UISwitch *)cell.accessoryView isOn] == (index % 2 == 0);
    }
    [self check:restored name:@"a fresh settings controller restores saved switch values"];
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
- (void)poll:(BOOL (^)(void))condition deadline:(NSDate *)deadline completion:(void (^)(BOOL))completion {
    if (condition()) { completion(YES); return; }
    if (deadline.timeIntervalSinceNow <= 0) { completion(NO); return; }
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 30 * NSEC_PER_MSEC), dispatch_get_main_queue(), ^{
        [self poll:condition deadline:deadline completion:completion];
    });
}
- (void)activateSearch:(HGSettingsController *)settings query:(NSString *)query completion:(void (^)(BOOL))completion {
    UISearchController *search = settings.navigationItem.searchController;
    search.active = YES;
    [self filterSettings:settings text:query];
    // Give UIKit a turn to begin its presentation before checking completion.
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 150 * NSEC_PER_MSEC), dispatch_get_main_queue(), ^{
        [self poll:^BOOL {
            return search.active && !search.isBeingPresented && search.searchBar.window != nil;
        } deadline:[NSDate dateWithTimeIntervalSinceNow:4] completion:completion];
    });
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

    [self filterSettings:settings text:@"信息流"];
    UISwitch *feed = (UISwitch *)[self settingsCell:settings section:0 row:0].accessoryView;
    [self check:[settings.tableView numberOfRowsInSection:0] == 1 && feed.tag == HGFeed &&
        [feed.accessibilityIdentifier isEqual:@"hg.option.feed"]
        name:@"search aliases find the feed filter by its stable feature identity"];

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
    [self filterSettings:settings text:@"兼容"];
    [self check:[settings.tableView numberOfRowsInSection:1] == 1 &&
        [[self settingsCell:settings section:1 row:0].accessibilityIdentifier isEqual:@"honggo.compatibility.open"]
        name:@"compatibility remains a distinct diagnostic action after filtering"];
    [self filterSettings:settings text:@"restore import"];
    [self check:[settings.tableView numberOfRowsInSection:2] == 1 &&
        [[self settingsCell:settings section:2 row:0].accessibilityIdentifier isEqual:@"honggo.preferences.import"]
        name:@"restore result keeps its action identity at filtered row zero"];
    [self filterSettings:settings text:@"backup export"];
    [self check:[settings.tableView numberOfRowsInSection:2] == 1 &&
        [[self settingsCell:settings section:2 row:0].accessibilityIdentifier isEqual:@"honggo.preferences.export"]
        name:@"backup export is distinct from diagnostic log export"];
    [self filterSettings:settings text:@"reset"];
    [self check:[settings.tableView numberOfRowsInSection:2] == 1 &&
        [[self settingsCell:settings section:2 row:0].accessibilityIdentifier isEqual:@"honggo.preferences.reset"]
        name:@"reset search result keeps its destructive action identity"];
    [self filterSettings:settings text:@"not-a-honggo-setting-123"];
    [self check:[settings.tableView numberOfRowsInSection:0] == 0 &&
        [settings.tableView numberOfRowsInSection:1] == 0 &&
        [settings.tableView numberOfRowsInSection:2] == 0 &&
        [settings.tableView.backgroundView.accessibilityIdentifier isEqual:@"honggo.settings.search.empty"]
        name:@"unmatched search has no stale rows and exposes an accessible empty state"];
    [self filterSettings:settings text:@" \n\t "];
    [self check:[settings.tableView numberOfRowsInSection:0] == HGFeatureCount &&
        [settings.tableView numberOfRowsInSection:1] == 4 && [settings.tableView numberOfRowsInSection:2] == 3 &&
        settings.tableView.backgroundView == nil
        name:@"whitespace search restores every section and removes the empty state"];
    [self filterSettings:settings text:@""];
    [self check:[settings.tableView numberOfRowsInSection:0] == HGFeatureCount
        name:@"clearing the query restores the full feature catalog"];
    [self runSearchAction:0 settings:settings completion:completion];
}
- (void)runSearchAction:(NSUInteger)index settings:(HGSettingsController *)settings completion:(dispatch_block_t)completion {
    NSArray<NSString *> *queries = @[@"兼容", @"reset", @"restore import", @"backup export"];
    NSArray<NSString *> *names = @[@"active search opens compatibility after search dismissal",
        @"active search opens the reset confirmation after search dismissal",
        @"active search opens the JSON picker after search dismissal",
        @"active search exports configuration to the native share sheet"];
    if (index >= queries.count) {
        [self activateSearch:settings query:@"玻璃" completion:^(BOOL active) {
            [self check:active name:@"search can be reactivated before closing settings"];
            UIBarButtonItem *done = settings.navigationItem.rightBarButtonItem;
            [UIApplication.sharedApplication sendAction:done.action to:done.target from:done forEvent:nil];
            [self poll:^BOOL { return self.host.presentedViewController == nil; }
                deadline:[NSDate dateWithTimeIntervalSinceNow:4] completion:^(BOOL closed) {
                [self check:closed name:@"Done dismisses the entire settings sheet while search is active"];
                completion();
            }];
        }];
        return;
    }
    NSDictionary *before = HGPreferencesParse(HGPreferencesExport(nil), nil);
    BOOL diagnosticsConsent = HGEnhancedDiagnostics();
    [self activateSearch:settings query:queries[index] completion:^(BOOL active) {
        if (!active) { [self check:NO name:names[index]]; completion(); return; }
        NSInteger section = index == 0 ? 1 : 2;
        if ([settings.tableView numberOfRowsInSection:section] != 1) {
            [self check:NO name:@"active result list resolves exactly one action"];
            completion();
            return;
        }
        [settings tableView:settings.tableView didSelectRowAtIndexPath:
            [NSIndexPath indexPathForRow:0 inSection:section]];
        [self poll:^BOOL {
            if (index == 0) return settings.navigationController.topViewController != settings;
            UIViewController *presented = settings.presentedViewController;
            Class expected = index == 1 ? UIAlertController.class :
                index == 2 ? UIDocumentPickerViewController.class : UIActivityViewController.class;
            return [presented isKindOfClass:expected] && !presented.isBeingPresented;
        } deadline:[NSDate dateWithTimeIntervalSinceNow:4] completion:^(BOOL performed) {
            [self check:performed name:names[index]];
            if (!performed) { completion(); return; }
            [self check:[HGPreferencesParse(HGPreferencesExport(nil), nil) isEqual:before] &&
                HGEnhancedDiagnostics() == diagnosticsConsent &&
                [[NSUserDefaults.standardUserDefaults stringForKey:@"host.setting"] isEqual:@"preserved"]
                name:@"opening configuration tools does not change features host settings or diagnostic consent"];
            if (index == 0) {
                UIViewController *compatibility = settings.navigationController.topViewController;
                [compatibility loadViewIfNeeded];
                [self check:[compatibility.view.accessibilityIdentifier isEqual:@"honggo.compatibility"] &&
                    [compatibility isKindOfClass:UITableViewController.class]
                    name:@"compatibility action displays the dedicated native status table"];
                [settings.navigationController popToViewController:settings animated:NO];
                dispatch_async(dispatch_get_main_queue(), ^{
                    [self runSearchAction:index + 1 settings:settings completion:completion];
                });
            } else {
                if (index == 1) {
                    UIAlertController *alert = (UIAlertController *)settings.presentedViewController;
                    BOOL hasCancel = NO, hasDestructive = NO;
                    for (UIAlertAction *action in alert.actions) {
                        hasCancel |= action.style == UIAlertActionStyleCancel;
                        hasDestructive |= action.style == UIAlertActionStyleDestructive;
                    }
                    [self check:[alert.title isEqual:@"关闭全部功能开关？"] && hasCancel && hasDestructive
                        name:@"reset requires a concrete confirmation before changing switches"];
                } else if (index == 2) {
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
        @"status": failed == 0 ? @"passed" : @"failed", @"failed": @(failed),
        @"total": @(self.checks.count), @"checks": self.checks};
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
