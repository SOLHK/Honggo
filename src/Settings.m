#import "Honggo.h"
#import "AppearancePreferences.h"
#import <QuartzCore/QuartzCore.h>
#include <math.h>
#include <string.h>

static NSString *const HGSettingsTitle = HG_PLUGIN_NAME;
static void (*HGOriginalSettingsWillAppear)(UIViewController *, SEL, BOOL);
static void (*HGOriginalSettingsDidLayout)(UIViewController *, SEL);
static void (*HGOriginalSettingsDidAppear)(UIViewController *, SEL, BOOL);
static CGFloat (*HGOriginalRowHeight)(id, SEL, UITableView *, NSIndexPath *);
static UITableViewCell *(*HGOriginalSettingsCell)(id, SEL, UITableView *, NSIndexPath *);
static CGFloat (*HGOriginalHeaderHeight)(id, SEL, UITableView *, NSInteger);
static CGFloat (*HGOriginalFooterHeight)(id, SEL, UITableView *, NSInteger);
static void (*HGOriginalPrivacyFooter)(id, SEL);
static char HGSettingsEntryKey;

// A real menu entry in the settings list, independent of the host navigation bar.
@interface HGSettingsMenuControl : UIControl
@property(nonatomic, strong) UILabel *titleLabel;
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
        self.accessibilityHint = @"打开插件功能开关";

        _titleLabel = [[UILabel alloc] init];
        _titleLabel.text = HGSettingsTitle;
        _titleLabel.textColor = UIColor.labelColor;
        _titleLabel.textAlignment = NSTextAlignmentCenter;
        _titleLabel.adjustsFontForContentSizeCategory = YES;
        _titleLabel.isAccessibilityElement = NO;
        [self addSubview:_titleLabel];
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
    UIFont *font = [UIFont systemFontOfSize:22 weight:UIFontWeightSemibold];
    self.titleLabel.font = [[UIFontMetrics metricsForTextStyle:UIFontTextStyleTitle2]
        scaledFontForFont:font compatibleWithTraitCollection:self.traitCollection];
    [self setNeedsLayout];
}

- (CGSize)sizeThatFits:(CGSize)size {
    CGFloat textWidth = MAX(0, size.width - 80);
    CGFloat titleHeight = [self.titleLabel sizeThatFits:CGSizeMake(textWidth, CGFLOAT_MAX)].height;
    return CGSizeMake(size.width, MAX(64, ceil(titleHeight + 28)));
}

- (void)layoutSubviews {
    [super layoutSubviews];
    CGFloat width = CGRectGetWidth(self.bounds);
    BOOL rtl = self.effectiveUserInterfaceLayoutDirection == UIUserInterfaceLayoutDirectionRightToLeft;
    self.titleLabel.frame = CGRectMake(40, 0, MAX(0,width-80), CGRectGetHeight(self.bounds));
    self.chevron.frame = CGRectMake(rtl ? 16 : width - 28,
        (CGRectGetHeight(self.bounds) - 18) / 2, 12, 18);
    HGStyleControl(self);
}

- (void)setHighlighted:(BOOL)highlighted {
    [super setHighlighted:highlighted];
    self.backgroundColor = highlighted ? UIColor.tertiarySystemFillColor : UIColor.secondarySystemGroupedBackgroundColor;
    // Touch tracking writes the normal/highlight fill without requesting a
    // layout. Reconcile immediately or the square host fill remains visible
    // outside the glass capsule when the user returns from the settings sheet.
    HGStyleControl(self);
}

- (void)didMoveToWindow {
    [super didMoveToWindow];
    HGStyleControl(self);
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
@property(nonatomic, copy) NSSet<NSNumber *> *hiddenTypes;
@property(nonatomic) BOOL hidesPolicy;
- (instancetype)initWithController:(UIViewController *)controller;
- (void)refreshLoggingIfMissing:(BOOL)logMissing;
@end

static NSArray *HGHostSettingsSection(id controller, NSInteger section) {
    NSArray *sections = HGReadObjectGetter(controller, "cellItemsArrs");
    if (![sections isKindOfClass:NSArray.class] || (NSUInteger)section >= sections.count) return nil;
    id rows = sections[section];
    return [rows isKindOfClass:NSArray.class] ? rows : nil;
}

// Keep the host's original data, index paths and switch/tap callbacks intact.
static BOOL HGHostSettingsRowHidden(id controller, NSIndexPath *path) {
    HGSettingsEntry *entry = objc_getAssociatedObject(controller, &HGSettingsEntryKey);
    if (!entry.hiddenTypes.count) return NO;
    NSArray *rows = HGHostSettingsSection(controller, path.section);
    return (NSUInteger)path.row < rows.count && [entry.hiddenTypes containsObject:rows[path.row]];
}

static CGFloat HGHostSettingsRowHeight(id controller, SEL cmd, UITableView *table, NSIndexPath *path) {
    return HGHostSettingsRowHidden(controller, path) ? 0 : HGOriginalRowHeight(controller, cmd, table, path);
}

static UITableViewCell *HGHostSettingsCell(id controller, SEL cmd, UITableView *table, NSIndexPath *path) {
    UITableViewCell *cell = HGOriginalSettingsCell(controller, cmd, table, path);
    // Reused host cells can draw outside a zero-height row unless hidden as well.
    static char hiddenByPlugin;
    if (HGHostSettingsRowHidden(controller, path)) {
        if (!cell.hidden) {
            cell.hidden = YES;
            objc_setAssociatedObject(cell, &hiddenByPlugin, @YES, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
    } else if (objc_getAssociatedObject(cell, &hiddenByPlugin)) {
        cell.hidden = NO;
        objc_setAssociatedObject(cell, &hiddenByPlugin, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    return cell;
}

static BOOL HGHostSettingsSectionHidden(id controller, NSInteger section) {
    HGSettingsEntry *entry = objc_getAssociatedObject(controller, &HGSettingsEntryKey);
    if (!entry.hiddenTypes.count) return NO;
    NSArray *rows = HGHostSettingsSection(controller, section);
    for (id row in rows) if (![entry.hiddenTypes containsObject:row]) return NO;
    return rows.count != 0;
}

static CGFloat HGHostSettingsHeaderHeight(id controller, SEL cmd, UITableView *table, NSInteger section) {
    return HGHostSettingsSectionHidden(controller, section) ? CGFLOAT_MIN : HGOriginalHeaderHeight(controller, cmd, table, section);
}

static CGFloat HGHostSettingsFooterHeight(id controller, SEL cmd, UITableView *table, NSInteger section) {
    return HGHostSettingsSectionHidden(controller, section) ? CGFLOAT_MIN : HGOriginalFooterHeight(controller, cmd, table, section);
}

static void HGHostSettingsPrivacyFooter(id controller, SEL cmd) {
    if (HGAppearanceEnabled() && !HGAppearanceFlag(@"mine.settings.show.privacyPolicy")) {
        UITableView *table = HGReadObjectGetter(controller, "tableView");
        if ([table isKindOfClass:UITableView.class]) table.tableFooterView = nil;
    } else {
        HGOriginalPrivacyFooter(controller, cmd);
    }
}

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
        [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(settingsVisibilityChanged:)
            name:HGAppearanceDidChangeNotification object:nil];
        [self settingsVisibilityChanged:nil];
    }
    return self;
}

- (void)dealloc {
    [NSNotificationCenter.defaultCenter removeObserver:self];
}

- (void)openSettings {
    [HGSettingsController openFromController:self.controller];
}

- (void)settingsVisibilityChanged:(NSNotification *)notification {
    NSDictionary *values = HGAppearanceValues();
    NSMutableSet *hidden = [NSMutableSet set];
    BOOL hidesPolicy = NO;
    if ([values[@"enabled"] boolValue]) {
        for (NSDictionary *entry in HGHostSettingsEntries()) {
            if ([values[entry[@"key"]] boolValue]) continue;
            if (entry[@"type"]) [hidden addObject:entry[@"type"]];
            else hidesPolicy = YES;
        }
    }
    if (![self.hiddenTypes isEqualToSet:hidden]) {
        self.hiddenTypes = hidden;
        [self.table reloadData];
    }
    if (self.hidesPolicy != hidesPolicy) {
        self.hidesPolicy = hidesPolicy;
        if (self.table && HGOriginalPrivacyFooter)
            HGHostSettingsPrivacyFooter(self.controller, sel_registerName("updatePrivacyPolicyFooterView"));
    }
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
        if (self.hiddenTypes.count) [table reloadData];
        if (self.hidesPolicy && HGOriginalPrivacyFooter)
            HGHostSettingsPrivacyFooter(self.controller, sel_registerName("updatePrivacyPolicyFooterView"));
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
    HGSettingsEntry *entry = objc_getAssociatedObject(controller, &HGSettingsEntryKey);
    if (!entry) {
        entry = [[HGSettingsEntry alloc] initWithController:controller];
        objc_setAssociatedObject(controller, &HGSettingsEntryKey, entry, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
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
        HGOriginalRowHeight = (void *)HGInstallVerifiedHook(settings, @selector(tableView:heightForRowAtIndexPath:),
            "d32@0:8@16@24", (IMP)HGHostSettingsRowHeight, @"settings");
        HGOriginalSettingsCell = (void *)HGInstallVerifiedHook(settings, @selector(tableView:cellForRowAtIndexPath:),
            "@32@0:8@16@24", (IMP)HGHostSettingsCell, @"settings");
        HGOriginalHeaderHeight = (void *)HGInstallVerifiedHook(settings, @selector(tableView:heightForHeaderInSection:),
            "d32@0:8@16q24", (IMP)HGHostSettingsHeaderHeight, @"settings");
        HGOriginalFooterHeight = (void *)HGInstallVerifiedHook(settings, @selector(tableView:heightForFooterInSection:),
            "d32@0:8@16q24", (IMP)HGHostSettingsFooterHeight, @"settings");
        HGOriginalPrivacyFooter = (void *)HGInstallVerifiedHook(settings, sel_registerName("updatePrivacyPolicyFooterView"),
            "v16@0:8", (IMP)HGHostSettingsPrivacyFooter, @"settings");
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
