#import "Honggo.h"
#import "Appearance.h"
#import "Community.h"
#import "SettingsTransition.h"
#import "ReleaseNotes.h"
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

static BOOL HGActivateSwitchRow(UITableView *table, NSIndexPath *path) {
    UIView *accessory = [table cellForRowAtIndexPath:path].accessoryView;
    if (![accessory isKindOfClass:UISwitch.class]) return NO;
    UISwitch *toggle = (UISwitch *)accessory;
    [toggle setOn:!toggle.on animated:YES];
    [toggle sendActionsForControlEvents:UIControlEventValueChanged];
    return YES;
}

typedef NS_ENUM(NSUInteger, HGDiagnosticAction) {
    HGDiagnosticActionOpen,
    HGDiagnosticActionClear,
};

static NSArray<NSDictionary *> *HGDiagnosticActions(void) {
    return @[
        @{@"kind": @"action", @"section": @1, @"action": @(HGDiagnosticActionOpen),
          @"title": @"诊断日志", @"id": @"honggo.diagnostics.export", @"aliases": @"导出 分享 日志 log"},
        @{@"kind": @"action", @"section": @1, @"action": @(HGDiagnosticActionClear),
          @"title": @"清除全部日志", @"id": @"honggo.diagnostics.clear", @"aliases": @"清除 删除 日志 log",
          @"destructive": @YES}
    ];
}


@interface HGDiagnosticSessionsController : UITableViewController
@property(nonatomic, copy) NSArray<NSDictionary *> *sessions;
@property(nonatomic, copy) NSArray<NSDictionary *> *events;
@property(nonatomic, copy) NSDictionary *session;
@property(nonatomic, strong) NSDateFormatter *formatter;
@property(nonatomic) BOOL exporting;
@property(nonatomic) NSInteger exportRow;
@property(nonatomic) BOOL exportPresentationScheduled;
@property(nonatomic, strong) NSURL *readyExport;
@property(nonatomic, copy) NSString *exportError;
@property(nonatomic) BOOL hasUnexpected;
@property(nonatomic,copy) dispatch_block_t clearLogs;
@property(nonatomic, copy) NSString *statusMessage;
@end

@implementation HGDiagnosticSessionsController
- (void)viewDidLoad {
    [super viewDidLoad];
    self.title = self.session ? @"会话日志" : @"诊断日志";
    self.tableView.accessibilityIdentifier = self.session ? @"honggo.diagnostics.session" : @"honggo.diagnostics.sessions";
    self.tableView.rowHeight = UITableViewAutomaticDimension;
    self.tableView.estimatedRowHeight = 64;
    self.formatter = [NSDateFormatter new];
    self.formatter.dateStyle = NSDateFormatterShortStyle;
    self.formatter.timeStyle = NSDateFormatterMediumStyle;
    self.navigationItem.rightBarButtonItem = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemRefresh
        target:self action:@selector(reloadLogs)];
    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(diagnosticAppDidBecomeActive:)
        name:UIApplicationDidBecomeActiveNotification object:nil];
}
- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    [self reloadLogs];
}
- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    [self presentReadyExport];
}
- (void)diagnosticAppDidBecomeActive:(NSNotification *)notification {
    [self presentReadyExport];
}
- (void)presentReadyExport {
    if (!self.readyExport || self.exportPresentationScheduled) return;
    self.exportPresentationScheduled = YES;
    HGAfterViewTransition(self, ^{
        self.exportPresentationScheduled = NO;
        if (!self.readyExport || !self.viewIfLoaded.window || self.navigationController.topViewController != self ||
                self.presentedViewController || self.isBeingDismissed ||
                self.view.window.windowScene.activationState != UISceneActivationStateForegroundActive) return;
        NSURL *url = self.readyExport;
        if (![url checkResourceIsReachableAndReturnError:nil]) {
            self.readyExport = nil;
            self.exportError = @"导出文件已清理。";
            [self.tableView reloadSections:[NSIndexSet indexSetWithIndex:0] withRowAnimation:UITableViewRowAnimationNone];
            return;
        }
        UIActivityViewController *share = [[UIActivityViewController alloc] initWithActivityItems:@[url] applicationActivities:nil];
        share.popoverPresentationController.sourceView = self.tableView;
        share.popoverPresentationController.sourceRect = CGRectMake(CGRectGetMidX(self.tableView.bounds),
            CGRectGetMidY(self.tableView.bounds), 1, 1);
        [self presentViewController:share animated:YES completion:^{
            self.readyExport = nil;
            [self.tableView reloadSections:[NSIndexSet indexSetWithIndex:0] withRowAnimation:UITableViewRowAnimationNone];
        }];
    });
}
- (void)reloadLogs {
    self.navigationItem.rightBarButtonItem.enabled = NO;
    if (self.session) {
        HGReadDiagnosticSession(self.session[@"session"], ^(NSArray<NSDictionary *> *events, NSError *error) {
            self.events = events;
            self.statusMessage = error.localizedDescription;
            if (!error && !events.count) self.statusMessage = @"此会话暂无日志。";
            self.navigationItem.rightBarButtonItem.enabled = YES;
            [self.tableView reloadData];
        });
    } else {
        HGListDiagnosticSessions(^(NSArray<NSDictionary *> *sessions, NSError *error) {
            self.sessions = sessions.reverseObjectEnumerator.allObjects;
            self.hasUnexpected = [[sessions valueForKey:@"state"] containsObject:@"unexpected"];
            self.statusMessage = error ? @"日志存储发生过错误，部分记录可能未保存。" : nil;
            self.navigationItem.rightBarButtonItem.enabled = YES;
            [self.tableView reloadData];
        });
    }
}
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView { return self.session ? 2 : 3; }
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    if (section == 2) return 1;
    return section == 0 ? (self.session ? 1 : 2) : (NSInteger)(self.session ? self.events.count : self.sessions.count);
}
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    if (section == 2) return nil;
    return section == 0 ? @"导出" : self.session ? @"最近事件" : @"启动会话";
}
- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
    if (section != 1) return nil;
    if (self.statusMessage) return self.statusMessage;
    return self.session ? @"页面显示最近最多 120 条事件。普通场景保留摘要，手动截图保留结构详情。" :
        @"保留最近 1 小时的日志。普通场景保留摘要，手动截图保留结构详情。结束原因未知也可能是手动退出或系统终止。";
}
- (NSString *)timeLabel:(NSNumber *)timestamp {
    NSDate *date = [NSDate dateWithTimeIntervalSince1970:timestamp.doubleValue];
    return [self.formatter stringFromDate:date];
}
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)path {
    UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:nil];
    cell.textLabel.numberOfLines = 0;
    cell.detailTextLabel.numberOfLines = 0;
    cell.textLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
    cell.detailTextLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleFootnote];
    cell.textLabel.adjustsFontForContentSizeCategory = YES;
    cell.detailTextLabel.adjustsFontForContentSizeCategory = YES;
    if (path.section == 2) {
        cell.textLabel.text=@"清除全部日志";
        cell.textLabel.textColor=UIColor.systemRedColor;
        cell.textLabel.enabled=!self.exporting;
        cell.accessibilityIdentifier=@"honggo.diagnostics.clear";
    } else if (path.section == 0) {
        BOOL available = self.session || path.row == 0 || self.hasUnexpected;
        cell.textLabel.text = self.session ? @"导出此会话" : path.row == 0 ? @"导出最近 1 小时日志" : @"导出上一次异常结束的会话";
        if (!available) cell.detailTextLabel.text = @"暂无结束原因未知的历史会话";
        if (path.row == self.exportRow) {
            if (self.exporting) {
                UIActivityIndicatorView *progress = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleMedium];
                [progress startAnimating];
                cell.accessoryView = progress;
                cell.detailTextLabel.text = @"正在准备日志…";
            } else if (self.readyExport) {
                cell.detailTextLabel.text = @"日志已就绪，轻点分享。";
            } else if (self.exportError.length) {
                cell.detailTextLabel.text = [NSString stringWithFormat:@"导出失败，轻点重试。%@", self.exportError];
            }
        }
        cell.textLabel.enabled = available;
        cell.selectionStyle = available && !self.exporting ? UITableViewCellSelectionStyleDefault : UITableViewCellSelectionStyleNone;
        cell.accessibilityIdentifier = path.row == 0 ? @"honggo.diagnostics.prepare" : @"honggo.diagnostics.previous";
    } else if (!self.session) {
        NSDictionary *item = self.sessions[path.row];
        NSString *state = item[@"state"];
        NSString *label = [state isEqual:@"current"] ? @"当前会话" : [state isEqual:@"ended"] ? @"已记录结束" : @"结束原因未知";
        cell.textLabel.text = [NSString stringWithFormat:@"%@ · %@", label, [self timeLabel:item[@"startedAt"]]];
        cell.detailTextLabel.text = [NSString stringWithFormat:@"%@ · %@\n%@", item[@"pluginVersion"] ?: @"", item[@"device"] ?: @"", item[@"session"]];
        cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
    } else {
        NSDictionary *row = self.events[path.row];
        cell.textLabel.text = row[@"event"];
        NSString *detail = [self timeLabel:row[@"time"]];
        NSDictionary *fields = row[@"fields"];
        NSString *context = fields[@"selector"] ?: fields[@"phase"] ?: fields[@"stage"] ?: fields[@"class"];
        if ([context isKindOfClass:NSString.class]) detail = [detail stringByAppendingFormat:@" · %@", context];
        if ([fields[@"durationSeconds"] isKindOfClass:NSNumber.class])
            detail = [detail stringByAppendingFormat:@" · %.2f 秒", [fields[@"durationSeconds"] doubleValue]];
        cell.detailTextLabel.text = detail;
        cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
    }
    return cell;
}
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)path {
    [tableView deselectRowAtIndexPath:path animated:YES];
    if (path.section == 2) {
        if (!self.exporting && self.clearLogs) self.clearLogs();
        return;
    }
    if (path.section == 1) {
        if (!self.session) {
            HGDiagnosticSessionsController *detail = [[HGDiagnosticSessionsController alloc] initWithStyle:UITableViewStyleInsetGrouped];
            detail.session = self.sessions[path.row];
            [self.navigationController pushViewController:detail animated:YES];
        } else {
            NSDictionary *row = self.events[path.row];
            NSData *data = [NSJSONSerialization dataWithJSONObject:row options:NSJSONWritingPrettyPrinted | NSJSONWritingSortedKeys error:nil];
            UIViewController *detail = [UIViewController new];
            detail.title = row[@"event"];
            UITextView *text = [UITextView new];
            text.backgroundColor = UIColor.systemBackgroundColor;
            text.font = [UIFont preferredFontForTextStyle:UIFontTextStyleFootnote];
            text.adjustsFontForContentSizeCategory = YES;
            text.editable = NO;
            text.text = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
            detail.view = text;
            [self.navigationController pushViewController:detail animated:YES];
        }
        return;
    }
    if (self.exporting || (!self.session && path.row == 1 && !self.hasUnexpected)) return;
    if (self.readyExport && self.exportRow == path.row) { [self presentReadyExport]; return; }
    self.readyExport = nil;
    self.exportError = nil;
    self.exportRow = path.row;
    self.exporting = YES;
    [self.tableView reloadSections:[NSIndexSet indexSetWithIndex:0] withRowAnimation:UITableViewRowAnimationNone];
    HGDiagnosticScope scope = self.session ? HGDiagnosticScopeSession :
        path.row == 0 ? HGDiagnosticScopeAll : HGDiagnosticScopePreviousUnexpected;
    dispatch_async(dispatch_get_main_queue(), ^{
        BOOL capture = self.viewIfLoaded.window &&
            (scope == HGDiagnosticScopeAll || [self.session[@"state"] isEqual:@"current"]);
        HGExportDiagnosticSessions(scope, self.session[@"session"], capture ? HGInterfaceSnapshot() : nil, ^(NSURL *url, NSError *error) {
            self.exporting = NO;
            self.readyExport = url;
            self.exportError = url ? nil : error.localizedDescription ?: @"未生成文件。";
            [self.tableView reloadSections:[NSIndexSet indexSetWithIndex:0] withRowAnimation:UITableViewRowAnimationNone];
            [self presentReadyExport];
        });
    });
}
@end


const NSUInteger HGConfigurationBackupMaximumBytes = 192 * 1024;

static BOOL HGBackupFailure(NSError **error, NSInteger code, NSString *description) {
    if (error) *error = [NSError errorWithDomain:HGPreferencesErrorDomain code:code
        userInfo:@{NSLocalizedDescriptionKey: description}];
    return NO;
}

static BOOL HGBackupIsIntegerNumber(id value, NSInteger expected) {
    if (![value isKindOfClass:NSNumber.class] ||
        CFGetTypeID((__bridge CFTypeRef)value) == CFBooleanGetTypeID() ||
        CFNumberIsFloatType((__bridge CFNumberRef)value)) return NO;
    return [value isEqualToNumber:@(expected)];
}

NSData *HGConfigurationBackupExport(NSError **error) {
    if (error) *error = nil;
    NSData *featureData = HGPreferencesExport(error);
    if (!featureData) return nil;
    NSDictionary *featureDoc = [NSJSONSerialization JSONObjectWithData:featureData options:0 error:error];
    NSDictionary *features = [featureDoc isKindOfClass:NSDictionary.class] ? featureDoc[@"features"] : nil;
    NSDictionary *appearance = HGAppearanceValues();
    if (![features isKindOfClass:NSDictionary.class] || ![appearance isKindOfClass:NSDictionary.class]) {
        HGBackupFailure(error, 1, @"无法整理当前功能与外观设置。");
        return nil;
    }
    NSMutableDictionary *currentFeatures=features.mutableCopy;
    [currentFeatures removeObjectsForKeys:@[@"glass",@"glassControls"]];
    NSDictionary *document = @{
        @"format": @"Honggo.backup",
        @"schema": @2,
        @"pluginVersion": HG_PLUGIN_VERSION,
        @"features": currentFeatures,
        @"appearance": appearance
    };
    return [NSJSONSerialization dataWithJSONObject:document
        options:NSJSONWritingPrettyPrinted | NSJSONWritingSortedKeys error:error];
}

static NSDictionary *HGMergeLegacyBackupGlass(NSDictionary *backup) {
    NSDictionary *features=backup[@"features"];
    if (![features isKindOfClass:NSDictionary.class] || (!features[@"glass"] && !features[@"glassControls"])) return backup;
    if (backup[@"appearance"] && ![backup[@"appearance"] isKindOfClass:NSDictionary.class]) return backup;
    NSMutableDictionary *appearance=[backup[@"appearance"] mutableCopy] ?: [NSMutableDictionary dictionary];
    BOOL enabled=[appearance[@"enabled"] boolValue] || [features[@"glass"] boolValue] || [features[@"glassControls"] boolValue];
    appearance[@"enabled"]=@(enabled);
    if ([features[@"glass"] boolValue]) appearance[@"dock.enabled"]=@YES;
    NSMutableDictionary *remaining=features.mutableCopy;
    [remaining removeObjectsForKeys:@[@"glass",@"glassControls"]];
    return remaining.count ? @{@"appearance":appearance, @"features":remaining} : @{@"appearance":appearance};
}

NSDictionary *HGConfigurationBackupParse(NSData *data, NSError **error) {
    if (error) *error = nil;
    if (![data isKindOfClass:NSData.class] || data.length == 0 || data.length > HGConfigurationBackupMaximumBytes) {
        HGBackupFailure(error, 1, @"请选择不超过 192 KB 的 Honggo 配置 JSON 文件。");
        return nil;
    }
    NSError *decodeError = nil;
    id root = [NSJSONSerialization JSONObjectWithData:data options:0 error:&decodeError];
    if (![root isKindOfClass:NSDictionary.class] || decodeError) {
        HGBackupFailure(error, 2, @"文件不是有效的 Honggo 配置 JSON。");
        return nil;
    }
    NSString *format = root[@"format"];
    if ([format isEqual:@"Honggo.preferences"]) {
        NSDictionary *features = HGPreferencesParse(data, error);
        return features ? HGMergeLegacyBackupGlass(@{@"features": features}) : nil;
    }
    if ([format isEqual:@"Honggo.appearance"]) {
        NSDictionary *appearance = HGAppearanceParse(data, error);
        return appearance ? @{@"appearance": appearance} : nil;
    }
    if (![format isEqual:@"Honggo.backup"]) {
        HGBackupFailure(error, 3, @"该文件不是支持的 Honggo 配置备份；不会导入其他应用的设置。");
        return nil;
    }
    if (!HGBackupIsIntegerNumber(root[@"schema"], 2)) {
        HGBackupFailure(error, 4, @"当前 Honggo 不支持这个备份格式版本。");
        return nil;
    }
    id version = root[@"pluginVersion"];
    if (![version isKindOfClass:NSString.class] || [version length] == 0 || [version length] > 64) {
        HGBackupFailure(error, 3, @"备份中的 Honggo 版本信息无效。");
        return nil;
    }
    NSSet *allowed = [NSSet setWithArray:@[@"format", @"schema", @"pluginVersion", @"features", @"appearance"]];
    for (id key in [root allKeys]) {
        if (![allowed containsObject:key]) {
            HGBackupFailure(error, 3, @"备份包含未知顶层字段，已拒绝导入。");
            return nil;
        }
    }
    id rawFeatures = root[@"features"];
    id rawAppearance = root[@"appearance"];
    if (!rawFeatures && !rawAppearance) {
        HGBackupFailure(error, 5, @"备份未包含任何可恢复的功能或外观设置。");
        return nil;
    }
    NSMutableDictionary *result = [NSMutableDictionary dictionary];
    if (rawFeatures) {
        if (![rawFeatures isKindOfClass:NSDictionary.class] || ![NSJSONSerialization isValidJSONObject:rawFeatures]) {
            HGBackupFailure(error, 5, @"备份中的功能开关格式无效。");
            return nil;
        }
        NSData *wrapped = [NSJSONSerialization dataWithJSONObject:@{
            @"format": @"Honggo.preferences", @"schema": @1,
            @"pluginVersion": version, @"features": rawFeatures
        } options:0 error:nil];
        NSDictionary *validatedFeatures = HGPreferencesParse(wrapped, error);
        if (!validatedFeatures) return nil;
        result[@"features"] = validatedFeatures;
    }
    if (rawAppearance) {
        if (![rawAppearance isKindOfClass:NSDictionary.class] || ![NSJSONSerialization isValidJSONObject:rawAppearance]) {
            HGBackupFailure(error, 6, @"备份中的外观参数格式无效。");
            return nil;
        }
        NSData *wrapped = [NSJSONSerialization dataWithJSONObject:@{
            @"format": @"Honggo.appearance", @"schema": @1, @"values": rawAppearance
        } options:0 error:nil];
        NSDictionary *validatedAppearance = HGAppearanceParse(wrapped, error);
        if (!validatedAppearance) return nil;
        result[@"appearance"] = validatedAppearance;
    }
    return HGMergeLegacyBackupGlass(result);
}

NSUInteger HGConfigurationBackupChangeCount(NSDictionary *backup) {
    if (![backup isKindOfClass:NSDictionary.class]) return 0;
    backup=HGMergeLegacyBackupGlass(backup);
    NSUInteger changes = 0;
    NSDictionary *features = backup[@"features"];
    if ([features isKindOfClass:NSDictionary.class]) changes += HGPreferencesChangeCount(features);
    NSDictionary *appearance = backup[@"appearance"];
    if ([appearance isKindOfClass:NSDictionary.class]) {
        NSDictionary *current = HGAppearanceValues();
        for (NSString *key in appearance) {
            id target = appearance[key];
            id existing = current[key];
            if (existing && ![existing isEqual:target]) changes++;
        }
    }
    return changes;
}

BOOL HGConfigurationBackupApply(NSDictionary *backup, NSError **error) {
    if (error) *error = nil;
    if (![backup isKindOfClass:NSDictionary.class] || !backup.count)
        return HGBackupFailure(error, 5, @"没有可恢复的配置内容。");
    NSDictionary *features = backup[@"features"];
    NSDictionary *appearance = backup[@"appearance"];
    if (!features && !appearance)
        return HGBackupFailure(error, 5, @"没有可恢复的配置内容。");
    if (features) {
        if (![features isKindOfClass:NSDictionary.class] || ![NSJSONSerialization isValidJSONObject:features])
            return HGBackupFailure(error, 5, @"功能开关内容无效。");
        NSData *wrapped = [NSJSONSerialization dataWithJSONObject:@{
            @"format": @"Honggo.preferences", @"schema": @1,
            @"pluginVersion": HG_PLUGIN_VERSION, @"features": features
        } options:0 error:nil];
        if (!HGPreferencesParse(wrapped, error)) return NO;
    }
    if (appearance) {
        if (![appearance isKindOfClass:NSDictionary.class] || ![NSJSONSerialization isValidJSONObject:appearance])
            return HGBackupFailure(error, 6, @"外观配置内容无效。");
        NSData *wrapped = [NSJSONSerialization dataWithJSONObject:@{
            @"format": @"Honggo.appearance", @"schema": @1, @"values": appearance
        } options:0 error:nil];
        NSDictionary *parsedAppearance = HGAppearanceParse(wrapped, error);
        if (!parsedAppearance) return NO;
        if ([parsedAppearance[@"dock.hide.home"] boolValue] &&
            [parsedAppearance[@"dock.hide.theater"] boolValue] &&
            [parsedAppearance[@"dock.hide.store"] boolValue] &&
            [parsedAppearance[@"dock.hide.mine"] boolValue])
            return HGBackupFailure(error, 6, @"至少保留一个底栏导航项目。");
        appearance = parsedAppearance;
    }
    if (features[@"glass"] || features[@"glassControls"]) {
        NSMutableDictionary *validated=[@{@"features":features} mutableCopy];
        if (appearance) validated[@"appearance"]=appearance;
        NSDictionary *merged=HGMergeLegacyBackupGlass(validated);
        features=merged[@"features"];
        appearance=merged[@"appearance"];
    }
    if (appearance && !HGAppearanceApply(appearance, error)) return NO;
    if (features && !HGPreferencesApply(features, error)) return NO;
    if (features)
        [NSNotificationCenter.defaultCenter postNotificationName:@"Honggo.FeaturePreferencesDidChange" object:nil];
    HGAppearanceRefresh();
    return YES;
}

@interface HGSettingsController () <UISearchResultsUpdating, UISearchControllerDelegate, UIDocumentPickerDelegate>
@property(nonatomic, strong) UISearchController *settingsSearch;
@property(nonatomic, copy) NSString *activeQuery;
@property(nonatomic, copy) NSArray<NSDictionary *> *rootCategories;
@property(nonatomic, copy) NSArray<NSNumber *> *visibleFeatures;
@property(nonatomic, copy) NSArray<NSNumber *> *visibleDiagnostics;
@property(nonatomic, copy) NSArray<NSNumber *> *visibleConfiguration;
@property(nonatomic, copy) NSArray<NSDictionary *> *visibleAppearance;
@property(nonatomic) BOOL preferenceOperationInFlight;
@property(nonatomic) BOOL communityVisible;
@property(nonatomic, weak) UIViewController *pendingRestorePresenter;
@property(nonatomic, copy) dispatch_block_t afterSearchDismissal;
- (void)performAction:(NSUInteger)action inSection:(NSInteger)section presenter:(UIViewController *)presenter;
- (void)toggleChanged:(UISwitch *)toggle;
@end

// Level 2 & Level 3 subpage controller mirroring FanqieX's clean hierarchy.
@interface HGReleaseNotesController : UITableViewController
@property(nonatomic) BOOL currentOnly;
@property(nonatomic, copy) NSArray<NSArray<NSString *> *> *entries;
@end

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
    HGLog(@"settings.page", @{@"stage": self.pageID ?: @"unknown", @"phase": @"enter"});
    [self reloadPage];
}
- (void)viewDidDisappear:(BOOL)animated {
    [super viewDidDisappear:animated];
    HGLog(@"settings.page", @{@"stage": self.pageID ?: @"unknown", @"phase": @"leave"});
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
    } else if ([pid isEqual:@"appearance"]) {
        self.title = @"外观";
        [rows addObject:@{@"kind":@"appearance",@"group":@"general",@"title":@"外观总开关",@"id":@"honggo.settings.appearance.general"}];
        [rows addObject:@{@"kind":@"appearance",@"group":@"fonts",@"title":@"字体与字号",@"id":@"honggo.settings.appearance.fonts"}];
        [rows addObject:@{@"kind":@"appearance",@"group":@"panelMaterial",@"title":@"面板材质",@"id":@"honggo.settings.appearance.material"}];
    } else if ([pid isEqual:@"backup"]) {
        self.title = @"导出与备份";
        [rows addObject:@{@"kind": @"action", @"section": @2, @"action": @0, @"title": @"导出功能配置", @"id": @"honggo.preferences.export"}];
        [rows addObject:@{@"kind": @"action", @"section": @2, @"action": @1, @"title": @"从文件恢复配置", @"id": @"honggo.preferences.import"}];
    } else if ([pid isEqual:@"community"]) {
        self.title = @"作者与频道";
        [rows addObject:@{@"kind": @"changelog", @"title": @"当前版本", @"detail": @"Honggo " HG_PLUGIN_VERSION @" · 点击查看全部版本更新内容", @"id": @"honggo.about.version"}];
        [rows addObject:@{@"kind": @"community", @"title": @"TG 更新频道", @"detail": @"@Naloiafeed · 打开频道或复制地址", @"id": @"honggo.community.channel"}];
    }
    self.rows = rows;
    [self.tableView reloadData];
}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section { return self.rows.count; }
- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
    NSString *pid = self.pageID ?: @"";
    if ([pid isEqual:@"backup"])
        return @"配置文件保存 Honggo 的全部功能开关、外观布局与屏蔽词设置（不含账号与日志）。恢复后界面外观立即生效；若更改了广告拦截开关，请完全退出红果后重开。";
    if ([pid isEqual:@"community"])
        return @"查看插件更新与测试版公告。只有点击打开或复制时才执行操作，不会自动加入频道。";
    if ([pid hasPrefix:@"ads."]) return @"功能开关更改后请完全退出红果短剧，再重新打开。";
    return nil;
}
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    NSDictionary *row = self.rows[indexPath.row];
    NSString *kind = row[@"kind"];
    if ([kind isEqual:@"feature"]) {
        HGFeature feature = [row[@"feature"] unsignedIntegerValue];
        UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:nil];
        NSString *title = HGFeatureTitle(feature);
        cell.textLabel.text = title;
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
    if ([kind isEqual:@"info"]) {
        cell.selectionStyle = UITableViewCellSelectionStyleNone;
    } else {
        cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
        if ([row[@"destructive"] boolValue]) cell.textLabel.textColor = UIColor.systemRedColor;
    }
    return cell;
}
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    if (HGActivateSwitchRow(tableView, indexPath)) return;
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
    } else if ([kind isEqual:@"changelog"]) {
        HGReleaseNotesController *all = [[HGReleaseNotesController alloc] initWithStyle:UITableViewStyleInsetGrouped];
        all.currentOnly = NO;
        if (self.navigationController) {
            [self.navigationController pushViewController:all animated:YES];
        } else {
            UINavigationController *nav = [[UINavigationController alloc] initWithRootViewController:all];
            [self presentViewController:nav animated:YES completion:nil];
        }
    } else if ([kind isEqual:@"action"]) {
        [self.rootSettings performAction:[row[@"action"] unsignedIntegerValue]
            inSection:[row[@"section"] integerValue] presenter:self];
    }
}
@end

static NSAttributedString *HGFormattedReleaseNotesBody(NSString *raw) {
    if (!raw.length) return nil;
    NSCharacterSet *trimSet = [NSCharacterSet characterSetWithCharactersInString:@" \t\n\r；;。"];
    NSArray<NSString *> *clauses = [raw componentsSeparatedByString:@"；"];
    if (clauses.count <= 1) {
        NSArray<NSString *> *sentences = [raw componentsSeparatedByString:@"。"];
        NSMutableArray<NSString *> *nonEmpty = [NSMutableArray array];
        for (NSString *s in sentences) {
            NSString *c = [s stringByTrimmingCharactersInSet:trimSet];
            if (c.length) [nonEmpty addObject:c];
        }
        if (nonEmpty.count > 1) clauses = nonEmpty;
    }
    NSMutableArray<NSString *> *bullets = [NSMutableArray array];
    for (NSString *part in clauses) {
        NSString *cleaned = [part stringByTrimmingCharactersInSet:trimSet];
        if (!cleaned.length) continue;
        [bullets addObject:[NSString stringWithFormat:@"• %@", cleaned]];
    }
    NSString *joined = bullets.count ? [bullets componentsJoinedByString:@"\n"] : raw;
    NSMutableParagraphStyle *para = [NSMutableParagraphStyle new];
    para.lineSpacing = 4.5;
    para.paragraphSpacing = 7.0;
    para.firstLineHeadIndent = 0.0;
    para.headIndent = 12.5;
    return [[NSAttributedString alloc] initWithString:joined attributes:@{
        NSFontAttributeName: [UIFont preferredFontForTextStyle:UIFontTextStyleSubheadline],
        NSForegroundColorAttributeName: UIColor.secondaryLabelColor,
        NSParagraphStyleAttributeName: para
    }];
}

@implementation HGReleaseNotesController
- (void)viewDidLoad {
    [super viewDidLoad];
    NSArray<NSArray<NSString *> *> *all = HGReleaseNotes();
    if (self.currentOnly) {
        self.title = @"当前版本更新";
        NSArray<NSString *> *matched = nil;
        for (NSArray<NSString *> *item in all) {
            if ([item.firstObject isEqualToString:HG_PLUGIN_VERSION]) { matched = item; break; }
        }
        if (!matched) matched = all.firstObject;
        self.entries = matched ? @[matched] : @[];
    } else {
        self.title = @"全部版本更新记录";
        self.entries = all;
    }
    if (!self.navigationController || self.navigationController.viewControllers.firstObject == self) {
        self.navigationItem.rightBarButtonItem = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemClose
            target:self action:@selector(closeNotes)];
    }
    self.tableView.accessibilityIdentifier = self.currentOnly ?
        @"honggo.settings.releaseNotes.current" : @"honggo.settings.releaseNotes";
    self.tableView.rowHeight = UITableViewAutomaticDimension;
    self.tableView.estimatedRowHeight = 96;
    if (self.navigationController.presentingViewController && self.navigationController.modalPresentationStyle == UIModalPresentationPageSheet) {
        self.tableView.backgroundColor = UIColor.clearColor;
    }
    self.tableView.allowsSelection = self.currentOnly;
}
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    if (self.currentOnly) return 1;
    return self.entries.count;
}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    if (self.currentOnly && !self.entries.count) return 0;
    return 1;
}
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    if (self.currentOnly) return @"本次版本更新内容（点击卡片查看全部版本）";
    if (section == 0) return @"当前版本";
    if (section == 1) return @"历史版本记录";
    return nil;
}
- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
    if (self.currentOnly && section == 0)
        return @"提示：点击上方更新内容卡片可进入三级弹窗查看全部版本的更新记录；也可在「Honggo 设置 → 作者与频道 → 当前版本」中查看。";
    if (!self.currentOnly && self.entries.count && section == (NSInteger)self.entries.count - 1)
        return [NSString stringWithFormat:@"提示：以上为全部 %lu 个版本的完整更新记录，点击右上角关闭按钮可返回上一层。",
            (unsigned long)self.entries.count];
    return nil;
}
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)path {
    BOOL frostedSheet = self.navigationController.modalPresentationStyle == UIModalPresentationPageSheet &&
        self.navigationController.sheetPresentationController != nil;
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"release"];
    if (!cell) cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:@"release"];
    NSArray *entry = self.entries[self.currentOnly ? 0 : path.section];
    UIListContentConfiguration *content = [UIListContentConfiguration subtitleCellConfiguration];
    content.text = entry.firstObject;
    content.textProperties.font = [UIFont preferredFontForTextStyle:UIFontTextStyleHeadline];
    content.secondaryAttributedText = HGFormattedReleaseNotesBody(entry.lastObject);
    content.secondaryTextProperties.numberOfLines = 0;
    content.textToSecondaryTextVerticalPadding = 8;
    content.directionalLayoutMargins = NSDirectionalEdgeInsetsMake(16, 18, 16, 18);
    cell.contentConfiguration = content;
    if (self.currentOnly) {
        cell.selectionStyle = UITableViewCellSelectionStyleDefault;
        cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
        cell.accessibilityIdentifier = @"honggo.settings.releaseNotes.currentCard";
        cell.accessibilityHint = @"点击打开三级弹窗查看全部版本的更新内容";
    } else {
        cell.selectionStyle = UITableViewCellSelectionStyleNone;
        cell.accessoryType = UITableViewCellAccessoryNone;
        cell.accessibilityIdentifier = [NSString stringWithFormat:@"honggo.settings.releaseNotes.item.%ld", (long)path.section];
        cell.accessibilityHint = nil;
    }
    UIBackgroundConfiguration *bg = [UIBackgroundConfiguration listGroupedCellConfiguration];
    if (frostedSheet) bg.backgroundColor = [UIColor.secondarySystemGroupedBackgroundColor colorWithAlphaComponent:0.68];
    cell.backgroundConfiguration = bg;
    return cell;
}
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)path {
    [tableView deselectRowAtIndexPath:path animated:YES];
    if (!self.currentOnly || path.section != 0) return;
    UIViewController *presenter = self.navigationController ?: self;
    if (presenter.presentedViewController) return;
    HGReleaseNotesController *all = [[HGReleaseNotesController alloc] initWithStyle:UITableViewStyleInsetGrouped];
    all.currentOnly = NO;
    UINavigationController *level3 = [[UINavigationController alloc] initWithRootViewController:all];
    level3.modalPresentationStyle = UIModalPresentationPageSheet;
    UISheetPresentationController *sheet = level3.sheetPresentationController;
    sheet.detents = @[UISheetPresentationControllerDetent.mediumDetent, UISheetPresentationControllerDetent.largeDetent];
    sheet.selectedDetentIdentifier = UISheetPresentationControllerDetentIdentifierLarge;
    sheet.prefersGrabberVisible = YES;
    UIBlurEffect *frost = [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemMaterial];
    if (@available(iOS 26.1, *)) {
        for (UISheetPresentationControllerDetent *detent in sheet.detents) detent.backgroundEffect = frost;
    } else {
        all.tableView.backgroundView = [[UIVisualEffectView alloc] initWithEffect:frost];
    }
    [presenter presentViewController:level3 animated:YES completion:nil];
}
- (void)closeNotes {
    UIViewController *owner = self.navigationController ?: self;
    if (owner.presentingViewController) [owner.presentingViewController dismissViewControllerAnimated:YES completion:nil];
    else [owner dismissViewControllerAnimated:YES completion:nil];
}
@end

static UIButton *HGSettingsCapsule(UIButtonConfiguration *style,NSString *title,
        NSString *identifier,id target,SEL action) {
    UIButtonConfiguration *config=[style copy];
    config.title=title;
    UIButton *button=[UIButton buttonWithConfiguration:config primaryAction:nil];
    button.accessibilityIdentifier=identifier;
    [button addTarget:target action:action forControlEvents:UIControlEventTouchUpInside];
    return button;
}

static UIBarButtonItem *HGSettingsCapsuleItem(UIButton *button,CGSize size) {
    // A bare configured UIButton is resized by the navigation bar to its own title.
    UIView *slot=[[UIView alloc] initWithFrame:(CGRect){CGPointZero,size}];
    [slot.widthAnchor constraintEqualToConstant:size.width].active=YES;
    [slot.heightAnchor constraintEqualToConstant:size.height].active=YES;
    button.frame=slot.bounds;
    button.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
    [slot addSubview:button];
    UIBarButtonItem *item=[[UIBarButtonItem alloc] initWithCustomView:slot];
    if (@available(iOS 26.0,*)) item.hidesSharedBackground=YES;
    return item;
}

@implementation HGSettingsController
- (void)showReleaseNotes:(UIButton *)button {
    if(self.presentedViewController)return;
    HGReleaseNotesController *notes=[[HGReleaseNotesController alloc] initWithStyle:UITableViewStyleInsetGrouped];
    notes.currentOnly=YES;
    UINavigationController *card=[[UINavigationController alloc] initWithRootViewController:notes];
    card.modalPresentationStyle=UIModalPresentationPageSheet;
    UISheetPresentationController *sheet=card.sheetPresentationController;
    sheet.detents=@[UISheetPresentationControllerDetent.mediumDetent,UISheetPresentationControllerDetent.largeDetent];
    sheet.prefersGrabberVisible=YES;
    UIBlurEffect *frost=[UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemMaterial];
    if(@available(iOS 26.1,*)) {
        for(UISheetPresentationControllerDetent *detent in sheet.detents)
            detent.backgroundEffect=frost;
    } else {
        notes.tableView.backgroundView=[[UIVisualEffectView alloc] initWithEffect:frost];
    }
    if(@available(iOS 18.0,*)) {
        __weak UIButton *source=button;
        card.preferredTransition=[UIViewControllerTransition zoomWithOptions:nil
            sourceViewProvider:^UIView *(UIZoomTransitionSourceViewProviderContext *context) { return source; }];
    }
    [self presentViewController:card animated:YES completion:nil];
}
- (void)viewDidLoad {
    [super viewDidLoad];
    self.title = HG_PLUGIN_NAME;
    HGDiagnosticsStart(nil, @{@"os": NSProcessInfo.processInfo.operatingSystemVersionString});
    HGLog(@"settings.open", @{@"nativeGlass": @(HGNativeGlassConfigured())});
    self.tableView.accessibilityIdentifier = @"honggo.settings.options";
    self.tableView.rowHeight = UITableViewAutomaticDimension;
    self.tableView.estimatedRowHeight = 56;
    UIButtonConfiguration *style;
    if (@available(iOS 26.0,*)) style=[HGALiquidButtonMaterial() isEqual:@"clear"] ?
        UIButtonConfiguration.clearGlassButtonConfiguration : UIButtonConfiguration.glassButtonConfiguration;
    else style=[UIButtonConfiguration grayButtonConfiguration];
    style.buttonSize=UIButtonConfigurationSizeMedium;
    style.cornerStyle=UIButtonConfigurationCornerStyleCapsule;
    style.baseForegroundColor=UIColor.labelColor;
    UIButton *version=HGSettingsCapsule(style,HG_PLUGIN_VERSION,@"honggo.settings.version",self,@selector(showReleaseNotes:));
    UIButton *done=HGSettingsCapsule(style,@"完成",@"honggo.settings.done",self,@selector(closeSettings));
    CGSize size=CGSizeMake(MAX(44,ceil(MAX(version.intrinsicContentSize.width,done.intrinsicContentSize.width))),44);
    version.accessibilityLabel=[NSString stringWithFormat:@"版本 %@，更新记录",HG_PLUGIN_VERSION];
    self.navigationItem.leftBarButtonItem=HGSettingsCapsuleItem(version,size);
    self.navigationItem.rightBarButtonItem=HGSettingsCapsuleItem(done,size);
    self.rootCategories = @[
        @{@"kind": @"appearance", @"group": @"chrome", @"title": @"页面与导航", @"id": @"honggo.settings.category.chrome"},
        @{@"kind": @"appearance", @"group": @"player", @"title": @"播放页", @"id": @"honggo.settings.category.player"},
        @{@"kind": @"appearance", @"group": @"dock", @"title": @"底栏 Dock", @"id": @"honggo.settings.category.dock"},
        @{@"kind": @"appearance", @"group": @"drama", @"title": @"短剧", @"id": @"honggo.settings.category.drama"},
        @{@"kind": @"appearance", @"group": @"mine", @"title": @"我的页面", @"id": @"honggo.settings.category.mine"},
        @{@"kind": @"appearance", @"group": @"comments", @"title": @"评论区", @"id": @"honggo.settings.category.comments"},
        @{@"kind": @"subpage", @"page": @"appearance", @"title": @"外观", @"id": @"honggo.settings.category.appearance"},
        @{@"kind": @"subpage", @"page": @"ads", @"title": @"广告与推广", @"id": @"honggo.settings.category.ads"},
        @{@"kind": @"subpage", @"page": @"backup", @"title": @"导出与备份", @"id": @"honggo.settings.category.backup"},
        @{@"kind": @"diagnostics", @"title": @"诊断日志", @"id": @"honggo.settings.category.diagnostics"},
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
- (NSArray<NSString *> *)configurationTitles { return @[@"导出功能配置", @"从文件恢复配置"]; }
- (void)updateSearchResultsForSearchController:(UISearchController *)searchController {
    NSString *trimmed = [searchController.searchBar.text ?: @""
        stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
    self.activeQuery = trimmed;
    if (!trimmed.length) {
        self.visibleFeatures = @[];
        self.visibleDiagnostics = @[];
        self.visibleConfiguration = @[];
        self.visibleAppearance = @[];
        self.communityVisible = NO;
        self.tableView.backgroundView = nil;
        [self.tableView reloadData];
        return;
    }
    NSMutableArray *features = [NSMutableArray array], *diagnostics = [NSMutableArray array], *configuration = [NSMutableArray array];
    for (HGFeature feature = 0; feature < HGFeatureCount; feature++) {
        if (feature==HGGlass || feature==HGGlassControls) continue;
        NSString *text = HGFeatureSearchText(feature);
        if (HGSettingsMatches(text, trimmed)) [features addObject:@(feature)];
    }
    NSArray *configurationAliases = @[@"备份 导出 配置 backup export", @"备份 导入 恢复 配置 restore import"];
    for (NSDictionary *entry in HGDiagnosticActions()) {
        if (HGSettingsMatches([entry[@"title"] stringByAppendingFormat:@" %@", entry[@"aliases"]], trimmed))
            [diagnostics addObject:entry[@"action"]];
    }
    for (NSUInteger index = 0; index < self.configurationTitles.count; index++) {
        if (HGSettingsMatches([self.configurationTitles[index] stringByAppendingFormat:@" %@", configurationAliases[index]], trimmed))
            [configuration addObject:@(index)];
    }
    self.visibleFeatures = features;
    self.visibleDiagnostics = diagnostics;
    self.visibleConfiguration = configuration;
    self.visibleAppearance = HGAppearanceSearch(trimmed);
    self.communityVisible = HGCommunityMatchesQuery(trimmed);
    UILabel *empty = nil;
    if (features.count + diagnostics.count + configuration.count + self.visibleAppearance.count == 0 && !self.communityVisible) {
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
    if (self.settingsSearch && (self.settingsSearch.active || self.settingsSearch.isBeingDismissed ||
            self.presentedViewController == self.settingsSearch)) {
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
    return self.isFiltering ? 5 : 1;
}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    if (!self.isFiltering) return self.rootCategories.count;
    if (section == 4) return self.visibleAppearance.count;
    if (section == 3) return self.communityVisible ? 1 : 0;
    return section == 0 ? self.visibleFeatures.count : section == 1 ? self.visibleDiagnostics.count :
        section == 2 ? self.visibleConfiguration.count : 0;
}
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    if (![self tableView:tableView numberOfRowsInSection:section]) return nil;
    if (!self.isFiltering) return nil;
    return @[@"功能开关", @"诊断日志", @"配置管理", @"更新与频道", @"界面与播放设置"][section];
}
- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
    if (![self tableView:tableView numberOfRowsInSection:section]) return nil;
    if (!self.isFiltering || section == 4) return nil;
    if (section == 3) return @"查看插件更新与测试版公告。只有点击打开或复制时才执行操作，不会自动加入频道。";
    if (section == 1) return @"复现问题后可截屏记录控件结构，再到诊断日志导出对应会话。";
    if (section == 2) return @"配置文件保存 Honggo 的全部功能开关、外观布局与屏蔽词设置。恢复后界面外观立即生效。";
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
    if (indexPath.section == 4) {
        NSDictionary *setting=self.visibleAppearance[indexPath.row];
        UITableViewCell *cell=[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:nil];
        cell.textLabel.text=setting[@"title"];
        NSUInteger group=[HGAppearanceGroups() indexOfObject:setting[@"group"]];
        if(group!=NSNotFound)cell.detailTextLabel.text=HGAppearanceGroupTitles()[group];
        cell.textLabel.numberOfLines=0;
        cell.accessoryType=UITableViewCellAccessoryDisclosureIndicator;
        cell.accessibilityIdentifier=[@"honggo.appearance.setting." stringByAppendingString:setting[@"key"]];
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
        NSString *title = HGFeatureTitle(feature);
        cell.textLabel.text = title;
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
        NSDictionary *entry = HGDiagnosticActions()[action];
        cell.textLabel.text = entry[@"title"];
        cell.accessibilityIdentifier = entry[@"id"];
        cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
        if ([entry[@"destructive"] boolValue]) cell.textLabel.textColor = UIColor.systemRedColor;
    } else {
        NSUInteger action = self.visibleConfiguration[indexPath.row].unsignedIntegerValue;
        cell.textLabel.text = self.configurationTitles[action];
        cell.accessibilityIdentifier = @[@"honggo.preferences.export", @"honggo.preferences.import"][action];
        cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
    }
    return cell;
}
- (void)toggleChanged:(UISwitch *)toggle {
    HGFeature feature = (HGFeature)toggle.tag;
    if (HGSetEnabled(feature, toggle.on)) {
        HGLog(@"settings.toggle", @{@"feature": HGFeatureIdentifier(feature), @"enabled": @(toggle.on)});
        [NSNotificationCenter.defaultCenter postNotificationName:@"Honggo.FeaturePreferencesDidChange" object:@(feature)];
    }
}
- (UIViewController *)activePresenter:(UIViewController *)preferred {
    UIViewController *top = preferred.viewIfLoaded.window ? preferred :
        (self.navigationController.topViewController.viewIfLoaded.window ? self.navigationController.topViewController : self);
    return top.viewIfLoaded.window ? top : self;
}
- (void)presentControllerWhenReady:(UIViewController *)modal on:(UIViewController *)preferred retries:(NSUInteger)retries {
    if (!modal) return;
    UIViewController *host = [self activePresenter:preferred ?: self.pendingRestorePresenter];
    if (self.isBeingDismissed || self.navigationController.isBeingDismissed) return;
    BOOL busy = !host.viewIfLoaded.window || host.isBeingPresented || host.isBeingDismissed ||
        host.transitionCoordinator != nil || self.navigationController.transitionCoordinator != nil ||
        host.presentedViewController != nil;
    if (!busy) {
        [host presentViewController:modal animated:YES completion:nil];
        return;
    }
    if (retries == 0) return;
    __weak HGSettingsController *weakSelf = self;
    __weak UIViewController *weakPreferred = preferred;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 40 * NSEC_PER_MSEC), dispatch_get_main_queue(), ^{
        [weakSelf presentControllerWhenReady:modal on:weakPreferred retries:retries - 1];
    });
}
- (void)showMessage:(NSString *)title detail:(NSString *)detail on:(UIViewController *)presenter {
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:title message:detail preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"好" style:UIAlertActionStyleDefault handler:nil]];
    [self presentControllerWhenReady:alert on:presenter retries:35];
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
    NSData *data = HGConfigurationBackupExport(&encodingError);
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
- (void)confirmPreferenceRestore:(NSDictionary *)backup on:(UIViewController *)presenter {
    NSUInteger changed = HGConfigurationBackupChangeCount(backup);
    if (!changed) {
        [self showMessage:@"无需恢复" detail:@"文件中的功能开关与外观设置已与当前设置一致。" on:presenter];
        return;
    }
    NSString *message = [NSString stringWithFormat:@"将更新 %lu 项 Honggo 设置（包含功能开关与外观布局，账号与日志不受影响）。恢复后界面外观立即生效；如更改了广告拦截开关，请完全退出再打开红果。", (unsigned long)changed];
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"恢复配置备份？" message:message preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:nil]];
    __weak HGSettingsController *weakSelf = self;
    __weak UIViewController *weakPresenter = presenter;
    [alert addAction:[UIAlertAction actionWithTitle:@"恢复" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
        HGSettingsController *strongSelf = weakSelf;
        if (!strongSelf) return;
        NSError *error = nil;
        if (HGConfigurationBackupApply(backup, &error)) {
            HGLog(@"settings.restored", @{@"changed": @(changed)});
            [strongSelf.tableView reloadData];
            for (UIViewController *vc in strongSelf.navigationController.viewControllers) {
                if ([vc isKindOfClass:UITableViewController.class] && vc.isViewLoaded)
                    [((UITableViewController *)vc).tableView reloadData];
            }
            [strongSelf showMessage:@"配置已恢复"
                detail:[NSString stringWithFormat:@"已成功恢复 %lu 项设置。", (unsigned long)changed]
                on:weakPresenter];
        } else {
            [strongSelf showMessage:@"配置恢复失败" detail:error.localizedDescription on:weakPresenter];
        }
    }]];
    [self presentControllerWhenReady:alert on:presenter retries:45];
}
- (void)readPreferenceURL:(NSURL *)url on:(UIViewController *)presenter {
    if (self.preferenceOperationInFlight) return;
    if (!url.isFileURL) { [self showMessage:@"无法恢复配置" detail:@"请选择本地或文件提供方中的 JSON 文件。" on:presenter]; return; }
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
            NSUInteger limit = HGConfigurationBackupMaximumBytes + 1;
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
        NSDictionary *backup = error ? nil : HGConfigurationBackupParse(data, &error);
        dispatch_async(dispatch_get_main_queue(), ^{
            [self setPreferenceBusy:NO];
            if (backup) [self confirmPreferenceRestore:backup on:presenter];
            else [self showMessage:@"无法恢复配置" detail:error.localizedDescription ?: @"无法读取所选文件。" on:presenter];
        });
    });
}
- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls {
    NSURL *url = urls.firstObject;
    if (!url) return;
    UIViewController *presenter = self.pendingRestorePresenter;
    if (controller.presentingViewController && !controller.isBeingDismissed) {
        [controller dismissViewControllerAnimated:YES completion:^{ [self readPreferenceURL:url on:presenter]; }];
    } else {
        [self readPreferenceURL:url on:presenter];
    }
}
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    if (self.preferenceOperationInFlight || HGActivateSwitchRow(tableView, indexPath)) return;
    if (!self.isFiltering) {
        NSDictionary *category = self.rootCategories[indexPath.row];
        NSString *kind = category[@"kind"];
        if ([kind isEqual:@"appearance"]) {
            NSString *group = [category[@"group"] length] ? category[@"group"] : nil;
            [self performAfterSearchDismissal:^{ HGOpenAppearance(self, group); }];
        } else if ([kind isEqual:@"diagnostics"]) {
            [self performAfterSearchDismissal:^{ [self performAction:HGDiagnosticActionOpen inSection:1 presenter:self]; }];
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
    if (indexPath.section == 4) {
        NSString *destination=[@"setting." stringByAppendingString:self.visibleAppearance[indexPath.row][@"key"]];
        [self performAfterSearchDismissal:^{ HGOpenAppearance(self,destination); }];
        return;
    }
    if (indexPath.section == 3) {
        [self performAfterSearchDismissal:^{ HGShowCommunity(self); }];
        return;
    }
    NSInteger section = indexPath.section;
    NSArray<NSNumber *> *actions = section == 2 ? self.visibleConfiguration : self.visibleDiagnostics;
    NSUInteger action = actions[indexPath.row].unsignedIntegerValue;
    [self performAfterSearchDismissal:^{ [self performAction:action inSection:section presenter:self]; }];
}
- (void)performAction:(NSUInteger)action inSection:(NSInteger)section presenter:(UIViewController *)presenter {
    UIViewController *host = [self activePresenter:presenter];
    if (section == 2) {
        if (action == 0) [self exportPreferencesOn:host];
        else if (action == 1) {
            self.pendingRestorePresenter = host;
            UIDocumentPickerViewController *picker = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:@[UTTypeJSON] asCopy:YES];
            picker.delegate = self;
            picker.allowsMultipleSelection = NO;
            [host presentViewController:picker animated:YES completion:nil];
        }
        return;
    }
    if (action == HGDiagnosticActionOpen) {
        HGDiagnosticSessionsController *logs=[[HGDiagnosticSessionsController alloc] initWithStyle:UITableViewStyleInsetGrouped];
        __weak HGDiagnosticSessionsController *weakLogs=logs;
        __weak HGSettingsController *weakSettings=self;
        logs.clearLogs=^{ [weakSettings performAction:HGDiagnosticActionClear inSection:1 presenter:weakLogs]; };
        [host.navigationController pushViewController:logs animated:YES];
        return;
    }
    if (action != HGDiagnosticActionClear) return;
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"清除全部日志"
        message:@"将删除当前会话、历史会话及本机的临时导出文件。" preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:nil]];
    [alert addAction:[UIAlertAction actionWithTitle:@"清除" style:UIAlertActionStyleDestructive handler:^(UIAlertAction *item) {
        HGClearDiagnostics(^(NSError *error) {
            if (error && host.viewIfLoaded.window) [self showDiagnosticError:error on:host];
            else if (!error && [host isKindOfClass:HGDiagnosticSessionsController.class]) {
                HGDiagnosticSessionsController *logs=(id)host;
                logs.readyExport=nil;
                logs.exportError=nil;
                [logs reloadLogs];
            }
        });
    }]];
    [host presentViewController:alert animated:YES completion:nil];
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
