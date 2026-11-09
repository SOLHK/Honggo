// Real UIKit behavior checks for opt-in ad-scene diagnostics. The host and
// advertisement are controlled fixtures, not the Hongguo IPA or a device claim.
#import "AdSceneDiagnostics.h"
#import "Diagnostics.h"
#import "HonggoVersion.h"
#import <dispatch/dispatch.h>
#import <objc/runtime.h>
#import <math.h>

static BOOL enhancedDiagnostics;
static NSMutableArray<NSDictionary *> *storedScenes;
static NSMutableArray<NSDictionary *> *recordedEvents;
static NSUInteger businessGetterCalls;
static NSUInteger contentGetterCalls;
static NSUInteger actionCalls;
static NSUInteger backgroundWindowReads;

// Keep persistence outside this test's scope. DiagnosticsIntegration already
// checks HGStoreMaskScene's ring, byte limits, export and retention behavior.
BOOL HGEnhancedDiagnostics(void) { return enhancedDiagnostics; }
void HGStoreMaskScene(NSDictionary *scene) {
    if (!storedScenes) storedScenes = [NSMutableArray array];
    [storedScenes addObject:scene];
}
void HGLog(NSString *event, NSDictionary *fields) {
    if (!recordedEvents) recordedEvents = [NSMutableArray array];
    [recordedEvents addObject:@{@"event": event ?: @"", @"fields": fields ?: @{}}];
}

static NSString *const SensitiveMarker = @"HG_PRIVATE_CONTENT_b57b98d1";

@interface HGAdSceneSecretLabel : UILabel
@end
@implementation HGAdSceneSecretLabel
- (NSString *)text { contentGetterCalls++; return [super text]; }
- (NSAttributedString *)attributedText { contentGetterCalls++; return [super attributedText]; }
- (NSString *)accessibilityLabel { contentGetterCalls++; return [super accessibilityLabel]; }
- (NSString *)accessibilityValue { contentGetterCalls++; return [super accessibilityValue]; }
- (NSString *)accessibilityIdentifier { contentGetterCalls++; return [super accessibilityIdentifier]; }
@end

@interface HGAdSceneActionTarget : NSObject
- (void)claimCoupon:(id)sender;
- (void)enterLiveRoom:(id)sender;
- (id)sensitiveBusinessModel;
@end
@implementation HGAdSceneActionTarget
- (void)claimCoupon:(id)sender { actionCalls++; }
- (void)enterLiveRoom:(id)sender { actionCalls++; }
- (id)sensitiveBusinessModel { businessGetterCalls++; return SensitiveMarker; }
@end

@interface HGAdScenePlayerController : UIViewController
- (id)sensitiveBusinessModel;
@end
@implementation HGAdScenePlayerController
- (id)sensitiveBusinessModel { businessGetterCalls++; return SensitiveMarker; }
@end

@interface HGAdSceneCouponController : UIViewController
- (BOOL)canShowPrivateCoupon;
@end
@implementation HGAdSceneCouponController
- (BOOL)canShowPrivateCoupon { businessGetterCalls++; return YES; }
@end

@interface HGAdSceneOverlayController : UIViewController
@end
@implementation HGAdSceneOverlayController
@end

@interface HGAdSceneHiddenController : UIViewController
@end
@implementation HGAdSceneHiddenController
@end

@interface HGAdSceneHiddenContentView : UIView
@end
@implementation HGAdSceneHiddenContentView
@end

@interface HGAdSceneHiddenAncestorController : UIViewController
@end
@implementation HGAdSceneHiddenAncestorController
- (void)loadView { self.view = [HGAdSceneHiddenContentView new]; }
@end

@interface HGAdSceneBusyChildController : UIViewController
@end
@implementation HGAdSceneBusyChildController
@end

// Probe the background contract without deliberately making unsafe UIKit reads.
// If a regression touches a window before checking its thread, record it and
// return a harmless value; the background assertion must still fail.
@interface HGAdSceneGuardedWindow : UIWindow
@end
@implementation HGAdSceneGuardedWindow
- (BOOL)isHidden {
    if (!NSThread.isMainThread) { backgroundWindowReads++; return YES; }
    return [super isHidden];
}
- (CGFloat)alpha {
    if (!NSThread.isMainThread) { backgroundWindowReads++; return 0; }
    return [super alpha];
}
- (UIWindowScene *)windowScene {
    if (!NSThread.isMainThread) { backgroundWindowReads++; return nil; }
    return [super windowScene];
}
- (UIViewController *)rootViewController {
    if (!NSThread.isMainThread) { backgroundWindowReads++; return nil; }
    return [super rootViewController];
}
- (CGRect)frame {
    if (!NSThread.isMainThread) { backgroundWindowReads++; return CGRectZero; }
    return [super frame];
}
@end

@interface HGAdSceneFloatingWindow : HGAdSceneGuardedWindow
@end
@implementation HGAdSceneFloatingWindow
@end

@interface HGAdSceneHiddenWindow : HGAdSceneGuardedWindow
@end
@implementation HGAdSceneHiddenWindow
@end

static NSDictionary *RecordWithValue(NSArray<NSDictionary *> *records,
                                    NSString *key, id value) {
    for (NSDictionary *record in records) {
        if ([record[key] isEqual:value]) return record;
    }
    return nil;
}

static NSData *EncodeScene(NSDictionary *scene) {
    if (![scene isKindOfClass:NSDictionary.class] ||
        ![NSJSONSerialization isValidJSONObject:scene]) return nil;
    return [NSJSONSerialization dataWithJSONObject:scene options:0 error:nil];
}

// Save real object identity, hierarchy and geometry before capture. This does
// not invoke host business/content getters and never becomes exported JSON.
static NSArray<NSDictionary *> *ViewState(NSArray<UIWindow *> *windows) {
    NSMutableArray<NSDictionary *> *state = [NSMutableArray array];
    NSMutableArray<UIView *> *pending = [NSMutableArray arrayWithArray:windows];
    for (NSUInteger cursor = 0; cursor < pending.count; cursor++) {
        UIView *view = pending[cursor];
        NSArray<UIView *> *children = [view.subviews copy];
        [state addObject:@{@"view": view, @"parent": view.superview ?: NSNull.null,
            @"children": children, @"frame": [NSValue valueWithCGRect:view.frame],
            @"bounds": [NSValue valueWithCGRect:view.bounds],
            @"hidden": @(view.hidden), @"alpha": @(view.alpha)}];
        [pending addObjectsFromArray:children];
    }
    return state;
}

static BOOL StateUnchanged(NSArray<NSDictionary *> *state) {
    for (NSDictionary *item in state) {
        UIView *view = item[@"view"];
        id parent = view.superview ?: NSNull.null;
        if (parent != item[@"parent"] || ![view.subviews isEqual:item[@"children"]] ||
            !CGRectEqualToRect(view.frame, [item[@"frame"] CGRectValue]) ||
            !CGRectEqualToRect(view.bounds, [item[@"bounds"] CGRectValue]) ||
            view.hidden != [item[@"hidden"] boolValue] ||
            fabs(view.alpha - [item[@"alpha"] doubleValue]) > 0.000001) return NO;
    }
    return YES;
}

@interface HGAdSceneTests : UIResponder <UIWindowSceneDelegate>
@property(nonatomic, strong) UIWindow *window;
@property(nonatomic, strong) HGAdSceneFloatingWindow *overlayWindow;
@property(nonatomic, strong) HGAdSceneHiddenWindow *hiddenWindow;
@property(nonatomic, strong) HGAdScenePlayerController *player;
@property(nonatomic, strong) HGAdSceneCouponController *coupon;
@property(nonatomic, strong) HGAdSceneOverlayController *overlay;
@property(nonatomic, strong) HGAdSceneActionTarget *actionTarget;
@property(nonatomic, strong) NSMutableArray<NSDictionary *> *checks;
@property(nonatomic) BOOL finished;
@end

@implementation HGAdSceneTests
- (void)check:(BOOL)passed name:(NSString *)name {
    [self.checks addObject:@{@"name": name, @"passed": @(passed)}];
    NSLog(@"[HonggoAdSceneTests] %@: %@", passed ? @"PASS" : @"FAIL", name);
}
- (NSURL *)outputURL:(NSString *)name {
    NSURL *directory = [NSFileManager.defaultManager URLsForDirectory:NSDocumentDirectory
        inDomains:NSUserDomainMask].firstObject;
    [NSFileManager.defaultManager createDirectoryAtURL:directory withIntermediateDirectories:YES
        attributes:nil error:nil];
    return [directory URLByAppendingPathComponent:name];
}
- (NSArray<UIWindow *> *)fixtureWindows {
    return @[self.window, self.overlayWindow, self.hiddenWindow];
}
- (void)addPrivateContentToView:(UIView *)view {
    HGAdSceneSecretLabel *label = [[HGAdSceneSecretLabel alloc] initWithFrame:CGRectMake(20, 32, 220, 44)];
    label.text = [SensitiveMarker stringByAppendingString:@"_text"];
    label.attributedText = [[NSAttributedString alloc]
        initWithString:[SensitiveMarker stringByAppendingString:@"_attributed"]];
    label.accessibilityLabel = [SensitiveMarker stringByAppendingString:@"_accessibilityLabel"];
    label.accessibilityValue = [SensitiveMarker stringByAppendingString:@"_accessibilityValue"];
    label.accessibilityIdentifier = [@"https://example.invalid/private?token="
        stringByAppendingString:SensitiveMarker];
    [view addSubview:label];
}
- (void)addButtonToView:(UIView *)view action:(SEL)action {
    UIButton *button = [UIButton buttonWithType:UIButtonTypeSystem];
    button.frame = CGRectMake(30, 100, 180, 44);
    [button setTitle:SensitiveMarker forState:UIControlStateNormal];
    [button addTarget:self.actionTarget action:action forControlEvents:UIControlEventTouchUpInside];
    [view addSubview:button];
}
- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session
        options:(UISceneConnectionOptions *)options {
    self.checks = [NSMutableArray array];
    storedScenes = [NSMutableArray array];
    recordedEvents = [NSMutableArray array];
    enhancedDiagnostics = NO;
    [UIView setAnimationsEnabled:NO];
    self.actionTarget = [HGAdSceneActionTarget new];
    UIWindowScene *windowScene = (UIWindowScene *)scene;
    self.player = [HGAdScenePlayerController new];
    self.player.title = SensitiveMarker;
    self.window = [[HGAdSceneGuardedWindow alloc] initWithWindowScene:windowScene];
    self.window.frame = windowScene.coordinateSpace.bounds;
    self.window.rootViewController = self.player;
    [self.window makeKeyAndVisible];
    [self addPrivateContentToView:self.player.view];

    self.overlay = [HGAdSceneOverlayController new];
    self.overlayWindow = [[HGAdSceneFloatingWindow alloc] initWithWindowScene:windowScene];
    self.overlayWindow.frame = windowScene.coordinateSpace.bounds;
    self.overlayWindow.windowLevel = UIWindowLevelAlert + 1;
    self.overlayWindow.rootViewController = self.overlay;
    self.overlayWindow.hidden = NO;
    self.overlay.view.backgroundColor = UIColor.clearColor;
    [self addPrivateContentToView:self.overlay.view];
    [self addButtonToView:self.overlay.view action:@selector(enterLiveRoom:)];

    self.hiddenWindow = [[HGAdSceneHiddenWindow alloc] initWithWindowScene:windowScene];
    self.hiddenWindow.frame = windowScene.coordinateSpace.bounds;
    self.hiddenWindow.rootViewController = [HGAdSceneHiddenController new];
    [self.hiddenWindow.rootViewController loadViewIfNeeded];
    self.hiddenWindow.hidden = YES;

    self.coupon = [HGAdSceneCouponController new];
    self.coupon.modalPresentationStyle = UIModalPresentationOverFullScreen;
    self.coupon.title = SensitiveMarker;
    [self.coupon loadViewIfNeeded];
    self.coupon.view.backgroundColor = [UIColor.blackColor colorWithAlphaComponent:0.35];
    [self addPrivateContentToView:self.coupon.view];
    [self addButtonToView:self.coupon.view action:@selector(claimCoupon:)];

    dispatch_async(dispatch_get_main_queue(), ^{
        @try {
            [self.player presentViewController:self.coupon animated:NO completion:^{
                dispatch_async(dispatch_get_main_queue(), ^{ [self runChecksSafely]; });
            }];
        } @catch (NSException *exception) {
            [self check:NO name:[@"fixture presentation exception: " stringByAppendingString:exception.name]];
            [self finish];
        }
    });
}
- (void)runChecksSafely {
    @try {
        [self runCaptureChecks];
        [self runNotificationChecks];
        [self runVisibilityPriorityChecks];
        [self runBudgetChecks];
        [self runBackgroundCheck];
    } @catch (NSException *exception) {
        [self check:NO name:[@"capture exception: " stringByAppendingString:exception.name]];
        [self finish];
    }
}
- (void)runCaptureChecks {
    [self check:self.player.presentedViewController == self.coupon &&
        self.coupon.view.window == self.window && self.overlay.view.window == self.overlayWindow
        name:@"fixture presents a coupon over a player and a separate overlay window"];
    enhancedDiagnostics = YES;
    NSArray<NSDictionary *> *before = ViewState(self.fixtureWindows);
    NSUInteger initialContentReads = contentGetterCalls;
    NSUInteger initialBusinessReads = businessGetterCalls;
    NSDictionary *scene = HGAdSceneSnapshot(self.fixtureWindows);
    [self check:contentGetterCalls == initialContentReads && businessGetterCalls == initialBusinessReads
        name:@"snapshot does not read text accessibility or host business getters"];
    [self check:StateUnchanged(before)
        name:@"snapshot preserves visibility alpha geometry and the complete hierarchy"];
    [self check:actionCalls == 0 name:@"snapshot does not invoke registered control actions"];
    [self check:[scene[@"kind"] isEqual:@"ad.scene"] && [scene[@"reason"] isEqual:@"user.screenshot"]
        name:@"scene identifies an explicit screenshot diagnostic"];
    BOOL arrays = YES;
    for (NSString *key in @[@"windows", @"controllers", @"nodes", @"classes"])
        arrays = arrays && [scene[key] isKindOfClass:NSArray.class];
    [self check:arrays name:@"scene exposes windows controllers nodes and class metadata arrays"];
    [self check:[scene[@"captureMs"] isKindOfClass:NSNumber.class] &&
        [scene[@"captureMs"] doubleValue] >= 0 &&
        [scene[@"truncated"] isKindOfClass:NSNumber.class]
        name:@"scene reports measured capture duration and truncation state"];

    NSDictionary *player = RecordWithValue(scene[@"controllers"], @"class", @"HGAdScenePlayerController");
    NSDictionary *coupon = RecordWithValue(scene[@"controllers"], @"class", @"HGAdSceneCouponController");
    NSDictionary *overlay = RecordWithValue(scene[@"controllers"], @"class", @"HGAdSceneOverlayController");
    [self check:player != nil name:@"captures the player without relying on a mine page"];
    [self check:coupon != nil && [coupon[@"presenting"] isEqual:player[@"id"]] &&
        [player[@"presented"] isEqual:coupon[@"id"]]
        name:@"captures the presented coupon and its presentation relationship"];
    [self check:overlay != nil && [scene[@"windows"] count] == 2 &&
        ![overlay[@"window"] isEqual:player[@"window"]]
        name:@"captures both visible windows with distinct controller ownership"];
    [self check:RecordWithValue(scene[@"controllers"], @"class", @"HGAdSceneHiddenController") == nil &&
        RecordWithValue(scene[@"nodes"], @"class", @"HGAdSceneHiddenWindow") == nil
        name:@"hidden windows and their controller trees are excluded"];

    NSMutableSet<NSString *> *actions = [NSMutableSet set];
    for (NSDictionary *node in scene[@"nodes"]) {
        for (NSDictionary *action in node[@"actions"]) {
            if ([action[@"targetClass"] isEqual:@"HGAdSceneActionTarget"] && action[@"selector"])
                [actions addObject:action[@"selector"]];
        }
    }
    [self check:[actions containsObject:@"claimCoupon:"] && [actions containsObject:@"enterLiveRoom:"]
        name:@"records only the action target class and selector for both ad controls"];
    NSDictionary *target = RecordWithValue(scene[@"classes"], @"name", @"HGAdSceneActionTarget");
    NSDictionary *method = RecordWithValue(target[@"methods"], @"selector", @"claimCoupon:");
    Method actualMethod = class_getInstanceMethod(HGAdSceneActionTarget.class, @selector(claimCoupon:));
    NSString *actualEncoding = [NSString stringWithUTF8String:method_getTypeEncoding(actualMethod)];
    [self check:[method[@"encoding"] isEqual:actualEncoding] && [method[@"kind"] isEqual:@"instance"]
        name:@"captures the runtime ABI for a non-system action method"];

    NSData *json = EncodeScene(scene);
    NSString *serialized = json ? [[NSString alloc] initWithData:json encoding:NSUTF8StringEncoding] : nil;
    [self check:json != nil && json.length <= 128 * 1024
        name:@"the normal capture is valid JSON within the 128 KiB storage limit"];
    [self check:serialized != nil && ![serialized containsString:SensitiveMarker] &&
        ![serialized containsString:@"example.invalid"] && ![serialized containsString:@"token="]
        name:@"JSON excludes private text titles URLs and accessibility values"];
    [self check:storedScenes.count == 0 name:@"direct snapshot does not persist an unsolicited capture"];
    if (json) [json writeToURL:[self outputURL:@"ad-scene-snapshot.json"] atomically:YES];
}
- (void)runNotificationChecks {
    enhancedDiagnostics = NO;
    NSArray<NSDictionary *> *before = ViewState(self.fixtureWindows);
    HGInstallAdSceneDiagnostics();
    HGInstallAdSceneDiagnostics();
    [self check:StateUnchanged(before) && storedScenes.count == 0
        name:@"installing twice does not change the interface or capture a scene"];
    [NSNotificationCenter.defaultCenter postNotificationName:UIApplicationUserDidTakeScreenshotNotification
        object:nil];
    [self check:storedScenes.count == 0 name:@"screenshots are ignored while enhanced diagnostics is off"];
    enhancedDiagnostics = YES;
    before = ViewState(self.fixtureWindows);
    NSUInteger initialContentReads = contentGetterCalls;
    NSUInteger initialBusinessReads = businessGetterCalls;
    [NSNotificationCenter.defaultCenter postNotificationName:UIApplicationUserDidTakeScreenshotNotification
        object:nil];
    [self check:storedScenes.count == 1
        name:@"one enabled screenshot stores exactly one scene after repeated installation"];
    NSDictionary *captured = storedScenes.lastObject;
    [self check:RecordWithValue(captured[@"controllers"], @"class", @"HGAdScenePlayerController") != nil &&
        RecordWithValue(captured[@"controllers"], @"class", @"HGAdSceneCouponController") != nil &&
        RecordWithValue(captured[@"controllers"], @"class", @"HGAdSceneOverlayController") != nil
        name:@"the screenshot observer discovers player popup and overlay from connected scenes"];
    [self check:StateUnchanged(before) && actionCalls == 0 &&
        contentGetterCalls == initialContentReads && businessGetterCalls == initialBusinessReads
        name:@"screenshot capture neither mutates UI nor evaluates private content or actions"];
    NSData *json = EncodeScene(captured);
    NSString *serialized = json ? [[NSString alloc] initWithData:json encoding:NSUTF8StringEncoding] : nil;
    [self check:serialized != nil && ![serialized containsString:SensitiveMarker] &&
        ![serialized containsString:@"example.invalid"]
        name:@"the persisted screenshot scene contains no private fixture content"];
    enhancedDiagnostics = NO;
    [NSNotificationCenter.defaultCenter postNotificationName:UIApplicationUserDidTakeScreenshotNotification
        object:nil];
    [self check:storedScenes.count == 1
        name:@"disabling enhanced diagnostics takes effect without reinstalling"];
    enhancedDiagnostics = YES;
}
- (void)runVisibilityPriorityChecks {
    // The child itself has a window, alpha=1 and hidden=NO. Only its ancestor
    // makes it invisible, so a check limited to the child would incorrectly
    // promote private background content into the scene's visible roots.
    UIView *hiddenAncestor = [[UIView alloc] initWithFrame:self.player.view.bounds];
    hiddenAncestor.hidden = YES;
    [self.player.view addSubview:hiddenAncestor];
    HGAdSceneHiddenAncestorController *hiddenChild = [HGAdSceneHiddenAncestorController new];
    [self.player addChildViewController:hiddenChild];
    hiddenChild.view.frame = CGRectMake(0, 0, 120, 120);
    [hiddenAncestor addSubview:hiddenChild.view];
    [hiddenChild didMoveToParentViewController:self.player];
    NSDictionary *hiddenScene = HGAdSceneSnapshot(self.fixtureWindows);
    [self check:hiddenScene != nil && hiddenChild.view.window == self.window && !hiddenChild.view.hidden &&
        RecordWithValue(hiddenScene[@"controllers"], @"class", @"HGAdSceneHiddenAncestorController") == nil &&
        RecordWithValue(hiddenScene[@"nodes"], @"class", @"HGAdSceneHiddenContentView") == nil
        name:@"a child beneath a hidden ancestor is not captured as a visible controller or seed"];
    [EncodeScene(hiddenScene) writeToURL:[self outputURL:@"ad-scene-hidden-ancestor.json"] atomically:YES];
    [hiddenChild willMoveToParentViewController:nil];
    [hiddenChild.view removeFromSuperview];
    [hiddenChild removeFromParentViewController];
    [hiddenAncestor removeFromSuperview];

    // A player can own many nested content controllers while a small modal is
    // presented. Build actual containment and distinct ancestor branches; the
    // diagnostic must spend its bounded capture on that foreground modal first.
    NSMutableArray<UIViewController *> *children = [NSMutableArray array];
    NSMutableArray<UIView *> *containers = [NSMutableArray array];
    for (NSUInteger index = 0; index < 100; index++) {
        UIView *container = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 80, 80)];
        [self.player.view addSubview:container];
        [containers addObject:container];
        UIView *parent = container;
        for (NSUInteger level = 0; level < 5; level++) {
            UIView *nested = [[UIView alloc] initWithFrame:parent.bounds];
            [parent addSubview:nested];
            parent = nested;
        }
        HGAdSceneBusyChildController *child = [HGAdSceneBusyChildController new];
        [self.player addChildViewController:child];
        child.view.frame = parent.bounds;
        [parent addSubview:child.view];
        [child didMoveToParentViewController:self.player];
        for (NSUInteger item = 0; item < 8; item++) {
            [child.view addSubview:[[UIView alloc] initWithFrame:CGRectMake(2, 2, 8, 8)]];
        }
        [children addObject:child];
    }
    NSDictionary *busyScene = HGAdSceneSnapshot(self.fixtureWindows);
    NSDictionary *coupon = RecordWithValue(busyScene[@"controllers"], @"class", @"HGAdSceneCouponController");
    NSDictionary *couponView = coupon ? RecordWithValue(busyScene[@"nodes"], @"id", coupon[@"view"]) : nil;
    [self check:self.player.presentedViewController == self.coupon &&
        [coupon[@"visible"] boolValue] && couponView != nil
        name:@"a complex player cannot crowd the presented coupon controller or view out of capture"];
    [EncodeScene(busyScene) writeToURL:[self outputURL:@"ad-scene-priority-snapshot.json"] atomically:YES];
    for (UIViewController *child in children) {
        [child willMoveToParentViewController:nil];
        [child.view removeFromSuperview];
        [child removeFromParentViewController];
    }
    for (UIView *container in containers) [container removeFromSuperview];
}
- (void)runBudgetChecks {
    UIView *largeTree = [[UIView alloc] initWithFrame:self.overlay.view.bounds];
    [self.overlay.view addSubview:largeTree];
    for (NSUInteger index = 0; index < 900; index++) {
        UIView *child = [[UIView alloc] initWithFrame:CGRectMake(4, 4, 12, 12)];
        [largeTree addSubview:child];
    }
    NSArray<NSDictionary *> *before = ViewState(self.fixtureWindows);
    NSDictionary *scene = HGAdSceneSnapshot(self.fixtureWindows);
    NSData *json = EncodeScene(scene);
    [self check:[scene[@"truncated"] boolValue] && [scene[@"nodes"] count] > 0 &&
        [scene[@"nodes"] count] < 900
        name:@"a large real view tree reports truncation instead of walking all nodes"];
    [self check:json != nil && json.length <= 128 * 1024
        name:@"large-tree captures remain valid JSON within 128 KiB"];
    [self check:StateUnchanged(before)
        name:@"budget exhaustion preserves every omitted view and hierarchy edge"];
    [largeTree removeFromSuperview];
    if (json) [json writeToURL:[self outputURL:@"ad-scene-budget-snapshot.json"] atomically:YES];
}
- (void)runBackgroundCheck {
    NSArray<UIWindow *> *windows = self.fixtureWindows;
    NSUInteger initialWindowReads = backgroundWindowReads;
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        BOOL safelyReturnedNil = NO;
        @try { safelyReturnedNil = HGAdSceneSnapshot(windows) == nil; }
        @catch (NSException *exception) { safelyReturnedNil = NO; }
        dispatch_async(dispatch_get_main_queue(), ^{
            [self check:safelyReturnedNil && backgroundWindowReads == initialWindowReads
                name:@"background snapshot returns nil without accessing UIKit"];
            [self finish];
        });
    });
}
- (void)finish {
    if (self.finished) return;
    self.finished = YES;
    NSUInteger failed = 0;
    for (NSDictionary *check in self.checks) failed += ![check[@"passed"] boolValue];
    NSDictionary *report = @{@"plugin": HG_PLUGIN_NAME, @"version": HG_PLUGIN_VERSION,
        @"scope": @"real UIKit with simulated player, coupon and overlay; not Hongguo device validation",
        @"status": failed ? @"failed" : @"passed", @"failed": @(failed),
        @"total": @(self.checks.count), @"checks": self.checks,
        @"storedSceneCount": @(storedScenes.count), @"actionCalls": @(actionCalls)};
    NSData *data = [NSJSONSerialization dataWithJSONObject:report options:NSJSONWritingPrettyPrinted error:nil];
    NSError *error = nil;
    BOOL written = [data writeToURL:[self outputURL:@"ad-scene-results.json"]
        options:NSDataWritingAtomic error:&error];
    NSLog(@"[HonggoAdSceneTests] finished: %lu checks, %lu failures, report written=%d%@",
        (unsigned long)self.checks.count, (unsigned long)failed, written,
        error ? [@" error=" stringByAppendingString:error.localizedDescription] : @"");
}
@end

@interface HGAdSceneTestAppDelegate : UIResponder <UIApplicationDelegate>
@end
@implementation HGAdSceneTestAppDelegate
- (UISceneConfiguration *)application:(UIApplication *)application
        configurationForConnectingSceneSession:(UISceneSession *)session
        options:(UISceneConnectionOptions *)options {
    UISceneConfiguration *configuration = [[UISceneConfiguration alloc]
        initWithName:@"HonggoAdSceneTest" sessionRole:session.role];
    configuration.delegateClass = HGAdSceneTests.class;
    return configuration;
}
@end

int main(int argc, char *argv[]) {
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass(HGAdSceneTestAppDelegate.class));
    }
}
