#import "Community.h"
// Real UIKit integration checks with a controlled host, not the Hongguo IPA.
#import "Honggo.h"
#import "Appearance.h"
#import <dispatch/dispatch.h>
#include <math.h>

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
    NSString *rootHeader = [settings tableView:settings.tableView titleForHeaderInSection:0];
    [self check:[settings.title isEqualToString:@"Honggo"] &&
        [rootHeader containsString:HG_PLUGIN_VERSION] &&
        [rootHeader containsString:@"当前调节页面："]
        name:@"settings title is Honggo with a separate plugin version and target page label"];
    [settings.tableView reloadData];
    [settings.tableView layoutIfNeeded];
    NSArray<NSString *> *expectedFolders = @[
        @"honggo.folder.appearance.root", @"honggo.folder.appearance.dock",
        @"honggo.folder.appearance.pages", @"honggo.folder.ads", @"honggo.folder.controls"
    ];
    BOOL folderStructure = [settings.tableView numberOfRowsInSection:0] == (NSInteger)expectedFolders.count && HGFeatureCount == 18;
    for (NSUInteger i = 0; folderStructure && i < expectedFolders.count; i++) {
        UITableViewCell *folderCell = [settings tableView:settings.tableView
            cellForRowAtIndexPath:[NSIndexPath indexPathForRow:(NSInteger)i inSection:0]];
        folderStructure &= [folderCell.accessibilityIdentifier isEqual:expectedFolders[i]] &&
            [folderCell.detailTextLabel.text containsString:@"当前调节页面："];
    }
    [self check:folderStructure name:@"settings root exposes five categorized entry folders noting regulated pages"];
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

    HGSettingsController *adsHub = [[HGSettingsController alloc] initWithStyle:UITableViewStyleInsetGrouped];
    [adsHub setValue:@"ads" forKey:@"menuPath"];
    [adsHub loadViewIfNeeded];
    [self check:[adsHub.tableView numberOfRowsInSection:0] == 4 &&
        [[adsHub tableView:adsHub.tableView titleForHeaderInSection:0] containsString:@"当前调节页面："]
        name:@"ad filter level-2 folder organizes four page-scoped submenus"];

    NSArray<NSString *> *keys = @[@"glass", @"splash", @"topview", @"feed", @"comment", @"pause", @"patch",
        @"commerce", @"marketing", @"redPacket", @"bookstoreAd", @"couponPendant", @"miniGamePendant",
        @"welfareGuide", @"survey", @"glassControls", @"sidebarGameCenter", @"screenshotShopping"];
    NSArray<NSString *> *leafPaths = @[@"controls", @"ads.startup", @"ads.playback", @"ads.popup", @"ads.discovery"];
    NSMutableSet<NSNumber *> *seenFeatures = [NSMutableSet set];
    BOOL saved = YES, leafHeaders = YES;
    for (NSString *path in leafPaths) {
        HGSettingsController *sub = [[HGSettingsController alloc] initWithStyle:UITableViewStyleInsetGrouped];
        [sub setValue:path forKey:@"menuPath"];
        [sub loadViewIfNeeded];
        leafHeaders &= [[sub tableView:sub.tableView titleForHeaderInSection:0] containsString:@"当前调节页面："];
        NSInteger rows = [sub.tableView numberOfRowsInSection:0];
        for (NSInteger r = 0; r < rows; r++) {
            UITableViewCell *cell = [sub tableView:sub.tableView
                cellForRowAtIndexPath:[NSIndexPath indexPathForRow:r inSection:0]];
            UISwitch *toggle = (UISwitch *)cell.accessoryView;
            saved = saved && [toggle isKindOfClass:UISwitch.class] && !toggle.on;
            HGFeature feature = (HGFeature)toggle.tag;
            [seenFeatures addObject:@(feature)];
            BOOL desired = ((NSUInteger)feature) % 2 == 0;
            toggle.on = desired;
            [toggle sendActionsForControlEvents:UIControlEventValueChanged];
            NSString *key = [@"HongguoGlass." stringByAppendingString:keys[(NSUInteger)feature]];
            saved = saved && [NSUserDefaults.standardUserDefaults boolForKey:key] == desired &&
                HGIsEnabled(feature) == desired;
        }
    }
    [self check:leafHeaders && seenFeatures.count == HGFeatureCount && saved
        name:@"all 18 feature switches live in labeled submenus and write existing preference keys"];

    BOOL restored = YES;
    for (NSString *path in leafPaths) {
        HGSettingsController *reopened = [[HGSettingsController alloc] initWithStyle:UITableViewStyleInsetGrouped];
        [reopened setValue:path forKey:@"menuPath"];
        [reopened loadViewIfNeeded];
        NSInteger rows = [reopened.tableView numberOfRowsInSection:0];
        for (NSInteger r = 0; r < rows; r++) {
            UITableViewCell *cell = [reopened tableView:reopened.tableView
                cellForRowAtIndexPath:[NSIndexPath indexPathForRow:r inSection:0]];
            UISwitch *toggle = (UISwitch *)cell.accessoryView;
            HGFeature feature = (HGFeature)toggle.tag;
            restored = restored && toggle.isOn == (((NSUInteger)feature) % 2 == 0);
        }
    }
    [self check:restored name:@"fresh settings submenu controllers restore all 18 saved switch values"];

    HGOpenAppearance(settings, @"dock.geometry");
    UITableViewController *appearanceSub = (UITableViewController *)((UINavigationController *)presented).topViewController;
    [appearanceSub loadViewIfNeeded];
    BOOL openedDockGeom = appearanceSub != (UITableViewController *)settings &&
        [[appearanceSub.tableView.dataSource tableView:appearanceSub.tableView titleForHeaderInSection:0] containsString:@"当前调节页面："] &&
        [appearanceSub.tableView.dataSource tableView:appearanceSub.tableView numberOfRowsInSection:0] == 1;
    UITableViewCell *geomCell = openedDockGeom ? [appearanceSub.tableView.dataSource tableView:appearanceSub.tableView
        cellForRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]] : nil;
    UISwitch *geomSwitch = [geomCell.accessoryView isKindOfClass:UISwitch.class] ? (UISwitch *)geomCell.accessoryView : nil;
    geomSwitch.on = YES;
    [geomSwitch sendActionsForControlEvents:UIControlEventValueChanged];
    openedDockGeom &= [appearanceSub.tableView.dataSource tableView:appearanceSub.tableView numberOfRowsInSection:0] == 6;
    geomSwitch.on = NO;
    [geomSwitch sendActionsForControlEvents:UIControlEventValueChanged];
    openedDockGeom &= [appearanceSub.tableView.dataSource tableView:appearanceSub.tableView numberOfRowsInSection:0] == 1;
    [((UINavigationController *)presented) popToViewController:settings animated:NO];
    [self check:openedDockGeom name:@"dock geometry level-3 submenu toggles its sliders inside the live settings stack"];

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

    [self check:[settings.tableView numberOfRowsInSection:3] == 1 &&
        [[self settingsCell:settings section:3 row:0].accessibilityIdentifier isEqual:@"honggo.community.channel"]
        name:@"Telegram channel is visible without enabling any feature"];
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

    [self filterSettings:settings text:@"截图 找同款"];
    UITableViewCell *screenshotRow = [self settingsCell:settings section:0 row:0];
    UISwitch *screenshot = (UISwitch *)screenshotRow.accessoryView;
    [self check:[settings.tableView numberOfRowsInSection:0] == 1 && screenshot.tag == HGScreenshotShopping &&
        [screenshot.accessibilityIdentifier isEqual:@"hg.option.screenshotShopping"] &&
        [screenshotRow.textLabel.text isEqual:@"隐藏截图识图找同款"] &&
        [screenshotRow.detailTextLabel.text containsString:@"位置："]
        name:@"search finds the independent screenshot shopping switch with its menu path"];

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
    [self check:[settings.tableView numberOfRowsInSection:0] == 5 &&
        [settings.tableView numberOfRowsInSection:1] == 4 && [settings.tableView numberOfRowsInSection:2] == 3 &&
        settings.tableView.backgroundView == nil
        name:@"whitespace search restores the root folder catalog and removes the empty state"];
    [self filterSettings:settings text:@""];
    [self check:[settings.tableView numberOfRowsInSection:0] == 5
        name:@"clearing the query restores the categorized root menu"];
    [self runSearchAction:0 settings:settings completion:completion];
}
- (void)runSearchAction:(NSUInteger)index settings:(HGSettingsController *)settings completion:(dispatch_block_t)completion {
    NSArray<NSString *> *queries = @[@"兼容", @"reset", @"restore import", @"backup export", @"TG Naloiafeed"];
    NSArray<NSString *> *names = @[@"active search opens compatibility after search dismissal",
        @"active search opens the reset confirmation after search dismissal",
        @"active search opens the JSON picker after search dismissal",
        @"active search exports configuration to the native share sheet",
        @"active search opens the Telegram menu only after search dismissal"];
    if (index >= queries.count) {
        [self activateSearch:settings query:@"玻璃" completion:^(BOOL active) {
            if (!active) { completion(); return; }
            NSUInteger dismissalsBefore = self.searchEvents.dismissals;
            UIBarButtonItem *done = settings.navigationItem.rightBarButtonItem;
            [self recordSearchState:@"close.requested" settings:settings extra:nil];
            [UIApplication.sharedApplication sendAction:done.action to:done.target from:done forEvent:nil];
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
    BOOL diagnosticsConsent = HGEnhancedDiagnostics();
    [self activateSearch:settings query:queries[index] completion:^(BOOL active) {
        if (!active) { completion(); return; }
        NSInteger section = index == 0 ? 1 : index == 4 ? 3 : 2;
        if ([settings.tableView numberOfRowsInSection:section] != 1) {
            [self check:NO name:@"active result list resolves exactly one action"];
            completion();
            return;
        }
        NSUInteger dismissalsBefore = self.searchEvents.dismissals;
        // The cold simulator launches the system ShareSheet service lazily.
        // Keep every presentation assertion; only that external-service startup
        // gets a larger bounded budget. Record elapsed time instead of sleeping.
        NSTimeInterval presentationBudget = index == 3 ? 30 : 8;
        NSDate *actionStarted = NSDate.date;
        [self recordSearchState:@"action.selected" settings:settings extra:@{@"query": queries[index],
            @"presentationBudgetSeconds": @(presentationBudget)}];
        [settings tableView:settings.tableView didSelectRowAtIndexPath:
            [NSIndexPath indexPathForRow:0 inSection:section]];
        [self poll:^BOOL {
            if (settings.navigationItem.searchController.active || self.searchEvents.dismissals <= dismissalsBefore) return NO;
            if (index == 0) return settings.navigationController.topViewController != settings;
            UIViewController *presented = settings.presentedViewController;
            Class expected = (index == 1 || index == 4) ? UIAlertController.class :
                index == 2 ? UIDocumentPickerViewController.class : UIActivityViewController.class;
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
                HGEnhancedDiagnostics() == diagnosticsConsent &&
                [[NSUserDefaults.standardUserDefaults stringForKey:@"host.setting"] isEqual:@"preserved"]
                name:@"opening configuration tools does not change features host settings or diagnostic consent"];
            if (index == 0) {
                UITableViewController *compatibility = (UITableViewController *)settings.navigationController.topViewController;
                [compatibility loadViewIfNeeded];
                [self check:[compatibility.view.accessibilityIdentifier isEqual:@"honggo.compatibility"] &&
                    [compatibility isKindOfClass:UITableViewController.class] &&
                    [[compatibility.tableView.dataSource tableView:compatibility.tableView titleForHeaderInSection:0] containsString:@"当前调节页面："]
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
                } else if (index == 4) {
                    UIAlertController *menu = (UIAlertController *)settings.presentedViewController;
                    NSMutableSet *titles = [NSMutableSet set];
                    for (UIAlertAction *action in menu.actions) [titles addObject:action.title];
                    [self check:[menu.message isEqual:HGCommunityURLString()] &&
                        [titles containsObject:@"打开频道"] && [titles containsObject:@"复制地址"] &&
                        [titles containsObject:@"取消"] && menu.actions.count == 3
                        name:@"channel menu shows the exact destination and explicit open copy cancel choices"];
                    [self capture:@"settings-community-simulator.png"];
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
