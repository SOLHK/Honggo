#import "Honggo.h"
#import <QuartzCore/QuartzCore.h>
#include <math.h>
#include <string.h>

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
    [HGGlassControls] = {@"glassControls", @"原生液态玻璃按钮"},
};

static NSString *const HGSettingsTitle = HG_PLUGIN_NAME;
static NSString *const HGSettingsSubtitle = @"版本 " HG_PLUGIN_VERSION @" · 液态玻璃与广告开关";
static void (*HGOriginalSettingsWillAppear)(UIViewController *, SEL, BOOL);
static void (*HGOriginalSettingsDidLayout)(UIViewController *, SEL);
static void (*HGOriginalSettingsDidAppear)(UIViewController *, SEL, BOOL);

static NSString *HGPreferenceKey(HGFeature feature) {
    // Keep the shipped key namespace so upgrading does not reset existing switches.
    return [@"HongguoGlass." stringByAppendingString:HGOptions[feature].key];
}

BOOL HGIsEnabled(HGFeature feature) {
    return [NSUserDefaults.standardUserDefaults boolForKey:HGPreferenceKey(feature)];
}

@implementation HGSettingsController

- (void)viewDidLoad {
    [super viewDidLoad];
    self.title = HGSettingsTitle;
    HGDiagnosticsStart(nil, @{@"os": NSProcessInfo.processInfo.operatingSystemVersionString});
    HGLog(@"settings.open", @{@"nativeGlass": @(HGNativeGlassConfigured())});
    self.tableView.accessibilityIdentifier = @"honggo.settings.options";
    self.tableView.rowHeight = UITableViewAutomaticDimension;
    self.tableView.estimatedRowHeight = 52;
    self.navigationItem.rightBarButtonItem = [[UIBarButtonItem alloc]
        initWithBarButtonSystemItem:UIBarButtonSystemItemDone
        target:self action:@selector(closeSettings)];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView { return 2; }

- (void)closeSettings {
    [self dismissViewControllerAnimated:YES completion:nil];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return section == 0 ? HGFeatureCount : 3;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    return section == 0 ? @"版本 " HG_PLUGIN_VERSION : @"诊断与反馈";
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
    if (section == 1) return @"日志保留最近一小时。增强诊断记录底栏布局和已适配按钮触摸；不采集账号、阅读/观看内容、Cookie 或 Token。导出后通过系统分享保存。";
    NSString *restart = @"更改后请完全退出红果短剧，再重新打开。关闭底栏开关并重启可恢复宿主底栏。";
    NSString *mode = HGNativeGlassConfigured() ?
        @"本次启动已配置系统玻璃。新版系统样式可能影响其他控件；关闭两个玻璃开关并重启，插件将停止启用该配置。" :
        @"iOS 26 及以上开启玻璃开关并重启后，插件会在启动时请求系统液态玻璃，无需预先修改安装包配置。旧系统使用模糊材质。";
    return [restart stringByAppendingString:mode];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    if (indexPath.section == 1) {
        UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil];
        cell.textLabel.text = @[@"增强诊断", @"导出最近一小时日志", @"清除诊断日志"][indexPath.row];
        cell.accessibilityIdentifier = @[@"honggo.diagnostics.enhanced", @"honggo.diagnostics.export", @"honggo.diagnostics.clear"][indexPath.row];
        if (indexPath.row == 0) {
            UISwitch *toggle = [UISwitch new];
            toggle.on = HGEnhancedDiagnostics();
            [toggle addTarget:self action:@selector(enhancedChanged:) forControlEvents:UIControlEventValueChanged];
            cell.accessoryView = toggle;
        } else cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
        return cell;
    }
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"HonggoOption"];
    if (!cell) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault
            reuseIdentifier:@"HonggoOption"];
        cell.selectionStyle = UITableViewCellSelectionStyleNone;
        cell.textLabel.adjustsFontForContentSizeCategory = YES;
        cell.textLabel.numberOfLines = 0;
        UISwitch *toggle = [[UISwitch alloc] init];
        [toggle addTarget:self action:@selector(toggleChanged:) forControlEvents:UIControlEventValueChanged];
        cell.accessoryView = toggle;
    }

    HGFeature feature = (HGFeature)indexPath.row;
    NSString *title = feature == HGGlass && NSProcessInfo.processInfo.operatingSystemVersion.majorVersion < 26 ?
        @"系统模糊底栏" : HGOptions[feature].title;
    cell.textLabel.text = title;
    UISwitch *toggle = (UISwitch *)cell.accessoryView;
    toggle.tag = (NSInteger)feature;
    toggle.accessibilityIdentifier = [@"hg.option." stringByAppendingString:HGOptions[feature].key];
    toggle.accessibilityLabel = title;
    toggle.on = HGIsEnabled(feature);
    return cell;
}

- (void)toggleChanged:(UISwitch *)toggle {
    HGFeature feature = (HGFeature)toggle.tag;
    [NSUserDefaults.standardUserDefaults setBool:toggle.on forKey:HGPreferenceKey(feature)];
    HGLog(@"settings.toggle", @{@"feature": HGOptions[feature].key, @"enabled": @(toggle.on)});
}

- (void)enhancedChanged:(UISwitch *)toggle { HGSetEnhancedDiagnostics(toggle.on); }
- (void)showDiagnosticError:(NSError *)error {
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"日志操作失败"
        message:[NSString stringWithFormat:@"%@ (%ld)", error.domain, (long)error.code]
        preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"好" style:UIAlertActionStyleDefault handler:nil]];
    [self presentViewController:alert animated:YES completion:nil];
}
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    if (indexPath.section != 1 || indexPath.row == 0) return;
    tableView.userInteractionEnabled = NO;
    if (indexPath.row == 2) {
        HGClearDiagnostics(^(NSError *error) {
            tableView.userInteractionEnabled = YES;
            if (error) [self showDiagnosticError:error];
        });
        return;
    }
    NSDictionary *snapshot = HGInterfaceSnapshot();
    HGExportDiagnostics(snapshot, ^(NSURL *url, NSError *error) {
        tableView.userInteractionEnabled = YES;
        if (error) { [self showDiagnosticError:error]; return; }
        if (!self.view.window || self.presentedViewController) return;
        UIActivityViewController *share = [[UIActivityViewController alloc] initWithActivityItems:@[url] applicationActivities:nil];
        share.popoverPresentationController.sourceView = tableView;
        share.popoverPresentationController.sourceRect = [tableView rectForRowAtIndexPath:indexPath];
        [self presentViewController:share animated:YES completion:nil];
    });
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

// A real menu entry in the settings list, independent of the host navigation bar.
@interface HGSettingsMenuControl : UIControl
@property(nonatomic, strong) UILabel *titleLabel;
@property(nonatomic, strong) UILabel *subtitleLabel;
@property(nonatomic, strong) UIImageView *chevron;
- (void)updateFonts;
@end

@implementation HGSettingsMenuControl

- (instancetype)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        self.backgroundColor = UIColor.secondarySystemGroupedBackgroundColor;
        self.layer.cornerRadius = 12;
        self.accessibilityIdentifier = @"honggo.settings.menu";
        self.isAccessibilityElement = YES;
        self.accessibilityTraits = UIAccessibilityTraitButton;
        self.accessibilityLabel = HGSettingsTitle;
        self.accessibilityValue = HGSettingsSubtitle;
        self.accessibilityHint = @"打开插件功能开关";

        _titleLabel = [[UILabel alloc] init];
        _titleLabel.text = HGSettingsTitle;
        _titleLabel.textColor = UIColor.labelColor;
        _subtitleLabel = [[UILabel alloc] init];
        _subtitleLabel.text = HGSettingsSubtitle;
        _subtitleLabel.textColor = UIColor.secondaryLabelColor;
        for (UILabel *label in @[_titleLabel, _subtitleLabel]) {
            label.numberOfLines = 0;
            label.adjustsFontForContentSizeCategory = YES;
            label.isAccessibilityElement = NO;
            [self addSubview:label];
        }
        _chevron = [[UIImageView alloc] initWithImage:[UIImage systemImageNamed:@"chevron.forward"]];
        _chevron.tintColor = UIColor.tertiaryLabelColor;
        _chevron.contentMode = UIViewContentModeScaleAspectFit;
        _chevron.isAccessibilityElement = NO;
        [self addSubview:_chevron];
        [self updateFonts];
    }
    return self;
}

- (void)updateFonts {
    self.titleLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody
        compatibleWithTraitCollection:self.traitCollection];
    self.subtitleLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleFootnote
        compatibleWithTraitCollection:self.traitCollection];
    [self setNeedsLayout];
}

- (CGSize)sizeThatFits:(CGSize)size {
    CGFloat textWidth = MAX(0, size.width - 56);
    CGFloat titleHeight = [self.titleLabel sizeThatFits:CGSizeMake(textWidth, CGFLOAT_MAX)].height;
    CGFloat subtitleHeight = [self.subtitleLabel sizeThatFits:CGSizeMake(textWidth, CGFLOAT_MAX)].height;
    return CGSizeMake(size.width, MAX(72, ceil(titleHeight + subtitleHeight + 28)));
}

- (void)layoutSubviews {
    [super layoutSubviews];
    CGFloat width = CGRectGetWidth(self.bounds);
    CGFloat textWidth = MAX(0, width - 56);
    CGFloat titleHeight = ceil([self.titleLabel sizeThatFits:CGSizeMake(textWidth, CGFLOAT_MAX)].height);
    CGFloat subtitleHeight = ceil([self.subtitleLabel sizeThatFits:CGSizeMake(textWidth, CGFLOAT_MAX)].height);
    CGFloat top = (CGRectGetHeight(self.bounds) - titleHeight - subtitleHeight - 4) / 2;
    BOOL rtl = self.effectiveUserInterfaceLayoutDirection == UIUserInterfaceLayoutDirectionRightToLeft;
    CGFloat textX = rtl ? 40 : 16;
    self.titleLabel.frame = CGRectMake(textX, top, textWidth, titleHeight);
    self.subtitleLabel.frame = CGRectMake(textX, top + titleHeight + 4, textWidth, subtitleHeight);
    self.chevron.frame = CGRectMake(rtl ? 16 : width - 28,
        (CGRectGetHeight(self.bounds) - 18) / 2, 12, 18);
    HGStyleControl(self);
}

- (void)setHighlighted:(BOOL)highlighted {
    [super setHighlighted:highlighted];
    self.backgroundColor = highlighted ? UIColor.tertiarySystemFillColor : UIColor.secondarySystemGroupedBackgroundColor;
}

@end

static void HGFindSettingsTable(UIView *view, UIViewController *owner, UITableView **best, CGFloat *bestArea) {
    if (view.hidden || view.alpha <= 0.01) {
        return;
    }
    // A child controller owns its own settings/list data; do not inject there.
    UIResponder *responder = view.nextResponder;
    if ([responder isKindOfClass:UIViewController.class] && responder != owner) {
        return;
    }
    if ([view isKindOfClass:UITableView.class]) {
        CGRect visible = CGRectIntersection(owner.view.bounds, [view convertRect:view.bounds toView:owner.view]);
        CGFloat area = CGRectIsNull(visible) ? 0 : CGRectGetWidth(visible) * CGRectGetHeight(visible);
        if (area > *bestArea) {
            *best = (UITableView *)view;
            *bestArea = area;
        }
        return; // Do not descend into tables nested inside reusable cells.
    }
    for (UIView *child in view.subviews) {
        HGFindSettingsTable(child, owner, best, bestArea);
    }
}

@interface HGSettingsEntry : NSObject
@property(nonatomic, weak) UIViewController *controller;
@property(nonatomic, weak) UITableView *table;
@property(nonatomic, strong) UIView *header;
@property(nonatomic, strong) UIView *originalHeader;
@property(nonatomic, strong) HGSettingsMenuControl *menu;
@property(nonatomic, strong) UIBarButtonItem *navigationItem;
@property(nonatomic) BOOL refreshing;
@property(nonatomic) BOOL loggedMissingTable;
- (instancetype)initWithController:(UIViewController *)controller;
- (void)refreshLoggingIfMissing:(BOOL)logMissing;
@end

@implementation HGSettingsEntry

- (instancetype)initWithController:(UIViewController *)controller {
    self = [super init];
    if (self) {
        _controller = controller;
        _header = [[UIView alloc] init];
        _header.backgroundColor = UIColor.clearColor;
        _menu = [[HGSettingsMenuControl alloc] initWithFrame:CGRectZero];
        [_menu addTarget:self action:@selector(openSettings) forControlEvents:UIControlEventTouchUpInside];
        [_header addSubview:_menu];
        _navigationItem = [[UIBarButtonItem alloc] initWithTitle:@"Honggo" style:UIBarButtonItemStylePlain
            target:self action:@selector(openSettings)];
        _navigationItem.accessibilityLabel = HGSettingsTitle;
        [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(contentSizeDidChange:)
            name:UIContentSizeCategoryDidChangeNotification object:nil];
    }
    return self;
}

- (void)dealloc {
    [NSNotificationCenter.defaultCenter removeObserver:self];
}

- (void)openSettings {
    [HGSettingsController openFromController:self.controller];
}

- (void)contentSizeDidChange:(NSNotification *)notification {
    [self.menu updateFonts];
    [self.controller.viewIfLoaded setNeedsLayout];
}

- (void)restoreOriginalHeader {
    UITableView *table = self.table;
    UIView *original = self.originalHeader;
    if (table.tableHeaderView == self.header) {
        table.tableHeaderView = nil;
        [original removeFromSuperview];
        table.tableHeaderView = original;
    } else if (original.superview == self.header) {
        [original removeFromSuperview];
    }
    self.originalHeader = nil;
    self.table = nil;
}

- (void)updateHeaderInTable:(UITableView *)table {
    if (self.table != table) {
        [self restoreOriginalHeader];
        self.table = table;
    }
    BOOL needsAttachment = table.tableHeaderView != self.header;
    if (needsAttachment) {
        UIView *original = table.tableHeaderView;
        // Preserve a host replacement instead of nesting another copy of our header.
        if (original && [self.header isDescendantOfView:original]) {
            return;
        }
        if (self.originalHeader.superview == self.header) {
            [self.originalHeader removeFromSuperview];
        }
        table.tableHeaderView = nil;
        self.originalHeader = original;
        if (original) {
            [self.header addSubview:original];
        }
    }

    CGFloat width = CGRectGetWidth(table.bounds);
    CGFloat menuWidth = MAX(0, width - 32);
    CGFloat menuHeight = [self.menu sizeThatFits:CGSizeMake(menuWidth, CGFLOAT_MAX)].height;
    CGFloat menuRegionHeight = menuHeight + 24;
    CGFloat originalHeight = self.originalHeader ? MAX(0, CGRectGetHeight(self.originalHeader.bounds)) : 0;
    CGRect frame = CGRectMake(0, 0, width, menuRegionHeight + originalHeight);
    BOOL sizeChanged = !CGSizeEqualToSize(self.header.bounds.size, frame.size);
    if (sizeChanged) {
        self.header.frame = frame;
    }
    self.menu.frame = CGRectMake(16, 12, menuWidth, menuHeight);
    self.originalHeader.frame = CGRectMake(0, menuRegionHeight, width, originalHeight);
    if (needsAttachment || sizeChanged) {
        table.tableHeaderView = self.header;
    }
}

- (void)refreshLoggingIfMissing:(BOOL)logMissing {
    UIViewController *controller = self.controller;
    if (self.refreshing || !controller.isViewLoaded) {
        return;
    }
    self.refreshing = YES;
    NSArray<UIBarButtonItem *> *items = controller.navigationItem.rightBarButtonItems;
    if (![items containsObject:self.navigationItem]) {
        controller.navigationItem.rightBarButtonItems = items ? [items arrayByAddingObject:self.navigationItem]
            : @[self.navigationItem];
    }
    UITableView *table = nil;
    CGFloat area = 0;
    HGFindSettingsTable(controller.view, controller, &table, &area);
    if (table) {
        [self updateHeaderInTable:table];
    } else if (logMissing && !self.loggedMissingTable) {
        self.loggedMissingTable = YES;
        NSLog(@"[Honggo] 设置页未找到自有可见 UITableView，保留导航栏 Honggo 和底栏双指长按入口");
    }
    self.refreshing = NO;
}

@end

static void HGRefreshSettings(UIViewController *controller, BOOL logMissing) {
    static char entryKey;
    HGSettingsEntry *entry = objc_getAssociatedObject(controller, &entryKey);
    if (!entry) {
        entry = [[HGSettingsEntry alloc] initWithController:controller];
        objc_setAssociatedObject(controller, &entryKey, entry, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    [entry refreshLoggingIfMissing:logMissing];
}

static void HGSettingsWillAppear(UIViewController *controller, SEL cmd, BOOL animated) {
    if (HGOriginalSettingsWillAppear) {
        HGOriginalSettingsWillAppear(controller, cmd, animated);
    }
    HGRefreshSettings(controller, NO);
}

static void HGSettingsDidLayout(UIViewController *controller, SEL cmd) {
    if (HGOriginalSettingsDidLayout) {
        HGOriginalSettingsDidLayout(controller, cmd);
    }
    HGRefreshSettings(controller, NO);
}

static void HGSettingsDidAppear(UIViewController *controller, SEL cmd, BOOL animated) {
    if (HGOriginalSettingsDidAppear) {
        HGOriginalSettingsDidAppear(controller, cmd, animated);
    }
    HGRefreshSettings(controller, YES);
}

BOOL HGInstallSettings(void) {
    @synchronized (HGSettingsController.class) {
        static BOOL installed;
        if (installed) {
            return YES;
        }
        Class settings = objc_lookUpClass("SSSettingViewController");
        if (!settings || ![settings isSubclassOfClass:UIViewController.class]) {
            NSLog(@"[Honggo] 设置控制器尚未就绪，未安装菜单入口");
            return NO;
        }
        SEL selectors[] = {@selector(viewWillAppear:), @selector(viewDidLayoutSubviews), @selector(viewDidAppear:)};
        const char *encodings[] = {"v20@0:8B16", "v16@0:8", "v20@0:8B16"};
        Method methods[3];
        // Validate the complete set before replacing anything; a failed retry cannot re-hook itself.
        for (NSUInteger index = 0; index < 3; index++) {
            methods[index] = class_getInstanceMethod(settings, selectors[index]);
            if (!methods[index] || strcmp(method_getTypeEncoding(methods[index]), encodings[index]) != 0) {
                NSLog(@"[Honggo] 设置入口方法签名不匹配：%s", sel_getName(selectors[index]));
                return NO;
            }
        }
        HGOriginalSettingsWillAppear = (void (*)(UIViewController *, SEL, BOOL))method_getImplementation(methods[0]);
        HGOriginalSettingsDidLayout = (void (*)(UIViewController *, SEL))method_getImplementation(methods[1]);
        HGOriginalSettingsDidAppear = (void (*)(UIViewController *, SEL, BOOL))method_getImplementation(methods[2]);
        class_replaceMethod(settings, selectors[0], (IMP)HGSettingsWillAppear, encodings[0]);
        class_replaceMethod(settings, selectors[1], (IMP)HGSettingsDidLayout, encodings[1]);
        class_replaceMethod(settings, selectors[2], (IMP)HGSettingsDidAppear, encodings[2]);
        installed = YES;
        return YES;
    }
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
