#import "HongguoGlass.h"

typedef struct {
    NSString *key;
    NSString *title;
} HGOption;

static const HGOption HGOptions[HGFeatureCount] = {
    [HGGlass] = {@"glass", @"液态玻璃底栏"},
    [HGSplash] = {@"splash", @"隐藏开屏广告"},
    [HGTopView] = {@"topview", @"隐藏首页 TopView 广告"},
    [HGFeed] = {@"feed", @"隐藏短剧信息流广告"},
    [HGComment] = {@"comment", @"隐藏短剧评论区广告"},
    [HGPause] = {@"pause", @"隐藏暂停广告"},
    [HGPatch] = {@"patch", @"隐藏播放插播广告"},
    [HGEcommerce] = {@"commerce", @"隐藏启动电商推广"},
    [HGMarketing] = {@"marketing", @"隐藏商业营销弹窗"},
    [HGRedPacket] = {@"redPacket", @"隐藏福利红包弹窗"},
    [HGBookstoreAd] = {@"bookstoreAd", @"隐藏书城底部广告"},
    [HGCouponPendant] = {@"couponPendant", @"隐藏领券悬浮挂件"},
    [HGMiniGamePendant] = {@"miniGamePendant", @"隐藏小游戏悬浮挂件"},
    [HGWelfareGuide] = {@"welfareGuide", @"隐藏福利标签引导气泡"},
    [HGSurvey] = {@"survey", @"隐藏短剧满意度问卷"},
};

static void (*HGOriginalSettingsDidAppear)(UIViewController *, SEL, BOOL);

static NSString *HGPreferenceKey(HGFeature feature) {
    return [@"HongguoGlass." stringByAppendingString:HGOptions[feature].key];
}

BOOL HGIsEnabled(HGFeature feature) {
    return [NSUserDefaults.standardUserDefaults boolForKey:HGPreferenceKey(feature)];
}

@implementation HGSettingsController

- (void)viewDidLoad {
    [super viewDidLoad];
    self.title = @"红果插件设置";
    self.navigationItem.rightBarButtonItem = [[UIBarButtonItem alloc]
        initWithBarButtonSystemItem:UIBarButtonSystemItemDone
        target:self action:@selector(closeSettings)];
}

- (void)closeSettings {
    [self dismissViewControllerAnimated:YES completion:nil];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return HGFeatureCount;
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
    return @"更改后请完全退出红果短剧，再重新打开。关闭开关并重启即可恢复原行为。iOS 26 及以上使用系统液态玻璃，旧版系统使用模糊材质。";
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"HongguoGlassOption"];
    if (!cell) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault
            reuseIdentifier:@"HongguoGlassOption"];
        cell.selectionStyle = UITableViewCellSelectionStyleNone;
        cell.textLabel.adjustsFontForContentSizeCategory = YES;
        cell.textLabel.numberOfLines = 0;
        UISwitch *toggle = [[UISwitch alloc] init];
        [toggle addTarget:self action:@selector(toggleChanged:) forControlEvents:UIControlEventValueChanged];
        cell.accessoryView = toggle;
    }

    HGFeature feature = (HGFeature)indexPath.row;
    NSString *title = HGOptions[feature].title;
    cell.textLabel.text = title;
    UISwitch *toggle = (UISwitch *)cell.accessoryView;
    toggle.tag = (NSInteger)feature;
    toggle.accessibilityLabel = title;
    toggle.on = HGIsEnabled(feature);
    return cell;
}

- (void)toggleChanged:(UISwitch *)toggle {
    HGFeature feature = (HGFeature)toggle.tag;
    [NSUserDefaults.standardUserDefaults setBool:toggle.on forKey:HGPreferenceKey(feature)];
}

+ (void)openFromController:(UIViewController *)presenter {
    if (!presenter || presenter.presentedViewController) {
        return;
    }

    HGSettingsController *settings = [[self alloc] initWithStyle:UITableViewStyleInsetGrouped];
    UINavigationController *navigation = [[UINavigationController alloc] initWithRootViewController:settings];
    [presenter presentViewController:navigation animated:YES completion:nil];
}

+ (void)openFromTabBar:(UILongPressGestureRecognizer *)gesture {
    if (gesture.state != UIGestureRecognizerStateBegan) {
        return;
    }

    UIResponder *responder = gesture.view;
    while (responder && ![responder isKindOfClass:UIViewController.class]) {
        responder = responder.nextResponder;
    }
    [self openFromController:(UIViewController *)responder];
}

@end

static void HGOpenSettings(UIViewController *controller, SEL cmd) {
    [HGSettingsController openFromController:controller];
}

static void HGSettingsDidAppear(UIViewController *controller, SEL cmd, BOOL animated) {
    HGOriginalSettingsDidAppear(controller, cmd, animated);

    static char itemKey;
    UIBarButtonItem *item = objc_getAssociatedObject(controller, &itemKey);
    if (!item) {
        item = [[UIBarButtonItem alloc] initWithTitle:@"红果插件" style:UIBarButtonItemStylePlain
            target:controller action:sel_registerName("hgGlassOpenSettings")];
        objc_setAssociatedObject(controller, &itemKey, item, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }

    UINavigationItem *navigationItem = controller.navigationItem;
    NSArray<UIBarButtonItem *> *items = navigationItem.rightBarButtonItems;
    if (![items containsObject:item]) {
        navigationItem.rightBarButtonItems = items ? [items arrayByAddingObject:item] : @[item];
    }
}

void HGInstallSettings(void) {
    Class settings = objc_lookUpClass("SSSettingViewController");
    SEL open = sel_registerName("hgGlassOpenSettings");
    if (!settings || !class_addMethod(settings, open, (IMP)HGOpenSettings, "v16@0:8")) {
        return;
    }
    HGOriginalSettingsDidAppear = (void (*)(UIViewController *, SEL, BOOL))HGReplaceMethod(settings,
        @selector(viewDidAppear:), "v20@0:8B16", (IMP)HGSettingsDidAppear);
}

void HGAttachSettings(UIView *tabBar) {
    static char gestureKey;
    if (objc_getAssociatedObject(tabBar, &gestureKey)) {
        return;
    }

    UILongPressGestureRecognizer *gesture = [[UILongPressGestureRecognizer alloc]
        initWithTarget:HGSettingsController.class action:@selector(openFromTabBar:)];
    gesture.numberOfTouchesRequired = 2;
    gesture.minimumPressDuration = 0.8;
    gesture.cancelsTouchesInView = NO;
    [tabBar addGestureRecognizer:gesture];
    objc_setAssociatedObject(tabBar, &gestureKey, gesture, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
