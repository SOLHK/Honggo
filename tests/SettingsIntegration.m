// Real UIKit integration checks with a controlled host, not the Hongguo IPA.
#import "HongguoGlass.h"
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
                } @catch (NSException *exception) {
                    [self check:NO name:[@"options exception: " stringByAppendingString:exception.description]];
                }
                [self finish];
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
    [settings.tableView reloadData];
    [settings.tableView layoutIfNeeded];
    [self check:[settings.tableView numberOfRowsInSection:0] == HGFeatureCount && HGFeatureCount == 15
        name:@"settings presents all 15 existing feature switches"];
    NSArray<NSString *> *keys = @[@"glass", @"splash", @"topview", @"feed", @"comment", @"pause", @"patch",
        @"commerce", @"marketing", @"redPacket", @"bookstoreAd", @"couponPendant", @"miniGamePendant",
        @"welfareGuide", @"survey"];
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
- (void)finish {
    NSUInteger failed = 0;
    for (NSDictionary *check in self.checks) {
        failed += ![check[@"passed"] boolValue];
    }
    NSDictionary *report = @{@"scope": @"real UIKit with a simulated host; not a Hongguo device test",
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
