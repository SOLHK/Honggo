// Execute Honggo's runtime boundary on real Objective-C/UIKit. These fixtures
// establish behavior of the plugin, not acceptance on the real host app.
#import "Honggo.h"
#import "RuntimeSupport.h"
#import <objc/message.h>

static NSUInteger HGValueCalls, HGWrongGetterCalls, HGLateLayouts;
static IMP HGValueOriginal, HGClassValueOriginal;
static IMP HGStackFirstOriginal, HGStackSecondOriginal;

@interface HGRuntimeParent : NSObject
- (NSInteger)value;
+ (NSInteger)classValue;
@end
@implementation HGRuntimeParent
- (NSInteger)value { HGValueCalls++; return 17; }
+ (NSInteger)classValue { return 31; }
@end
@interface HGRuntimeChild : HGRuntimeParent @end
@implementation HGRuntimeChild @end
@interface HGRuntimeSibling : HGRuntimeParent @end
@implementation HGRuntimeSibling @end
@interface HGRuntimeStacked : HGRuntimeParent @end
@implementation HGRuntimeStacked @end

static NSInteger HGChildValue(id object, SEL selector) {
    return ((NSInteger (*)(id, SEL))HGValueOriginal)(object, selector) + 1;
}
static NSInteger HGForeignValue(id object, SEL selector) {
    return HGChildValue(object, selector) + 100;
}
static NSInteger HGChildClassValue(id object, SEL selector) {
    return ((NSInteger (*)(id, SEL))HGClassValueOriginal)(object, selector) + 2;
}
static NSInteger HGStackFirst(id object, SEL selector) {
    return ((NSInteger (*)(id, SEL))HGStackFirstOriginal)(object, selector) + 10;
}
static NSInteger HGStackSecond(id object, SEL selector) {
    return ((NSInteger (*)(id, SEL))HGStackSecondOriginal)(object, selector) + 20;
}

@interface HGRuntimeGetters : NSObject
- (id)objectValue;
- (NSInteger)scalarValue;
- (CGRect)rectValue;
@end
@implementation HGRuntimeGetters
- (id)objectValue { return @"fixture"; }
- (NSInteger)scalarValue { HGWrongGetterCalls++; return 123; }
- (CGRect)rectValue { HGWrongGetterCalls++; return CGRectMake(1, 2, 3, 4); }
@end

@interface SSReadingAdSplashService : NSObject
- (BOOL)shouldShowSplashAd;
@end
@implementation SSReadingAdSplashService
- (BOOL)shouldShowSplashAd { return YES; }
@end

static void HGLateControlLayout(id view, SEL selector) { HGLateLayouts++; }

static NSDictionary *HGCapability(NSString *identifier) {
    for (NSDictionary *entry in HGRuntimeSnapshot()[@"entries"])
        if ([entry[@"id"] isEqual:identifier]) return entry;
    return nil;
}

@interface HGRuntimeTests : UIResponder <UIWindowSceneDelegate>
@property(nonatomic, strong) UIWindow *window;
@property(nonatomic, strong) NSMutableArray *checks;
@end
@implementation HGRuntimeTests
- (void)check:(BOOL)passed name:(NSString *)name {
    [self.checks addObject:@{@"passed": @(passed), @"name": name}];
}
- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session
        options:(UISceneConnectionOptions *)options {
    self.window = [[UIWindow alloc] initWithWindowScene:(UIWindowScene *)scene];
    self.window.rootViewController = [UIViewController new];
    [self.window makeKeyAndVisible];
    [self runChecks];
}
- (void)runChecks {
    self.checks = [NSMutableArray array];
    NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
    [defaults removePersistentDomainForName:NSBundle.mainBundle.bundleIdentifier];

    SEL valueSelector = @selector(value);
    Method inherited = class_getInstanceMethod(HGRuntimeChild.class, valueSelector);
    IMP parentBefore = method_getImplementation(inherited);
    const char *valueEncoding = method_getTypeEncoding(inherited);
    HGValueOriginal = HGInstallVerifiedHook(HGRuntimeChild.class, valueSelector,
        valueEncoding, (IMP)HGChildValue, @"fixture");
    [self check:HGValueOriginal == parentBefore name:@"inherited hook returns the parent's original implementation"];
    [self check:[[HGRuntimeParent new] value] == 17 && [[HGRuntimeSibling new] value] == 17 &&
        [[HGRuntimeChild new] value] == 18
        name:@"inherited replacement affects only the requested subclass"];
    [self check:class_getMethodImplementation(HGRuntimeParent.class, valueSelector) == parentBefore
        name:@"parent method table is unchanged"];

    NSUInteger beforeCalls = HGValueCalls;
    IMP repeated = HGInstallVerifiedHook(HGRuntimeChild.class, valueSelector,
        valueEncoding, (IMP)HGChildValue, @"fixture");
    HGValueOriginal = repeated;
    [self check:repeated == parentBefore && [[HGRuntimeChild new] value] == 18 &&
        HGValueCalls == beforeCalls + 1 name:@"duplicate installation cannot recurse or overwrite the first original"];

    class_replaceMethod(HGRuntimeChild.class, valueSelector, (IMP)HGForeignValue, valueEncoding);
    HGValueOriginal = HGInstallVerifiedHook(HGRuntimeChild.class, valueSelector,
        valueEncoding, (IMP)HGChildValue, @"fixture");
    [self check:class_getMethodImplementation(HGRuntimeChild.class, valueSelector) == (IMP)HGForeignValue &&
        HGValueOriginal == parentBefore && [[HGRuntimeChild new] value] == 118
        name:@"retry preserves a later foreign hook and retains a nonrecursive original chain"];
    [self check:[HGCapability(@"hook.fixture.instance.HGRuntimeChild.value")[@"state"] isEqual:@"interposed"]
        name:@"inventory distinguishes a later replacement from a currently installed hook"];

    Class childMeta = object_getClass(HGRuntimeChild.class);
    SEL classSelector = @selector(classValue);
    const char *classEncoding = method_getTypeEncoding(class_getInstanceMethod(childMeta, classSelector));
    HGClassValueOriginal = HGInstallVerifiedHook(childMeta, classSelector, classEncoding,
        (IMP)HGChildClassValue, @"fixture");
    [self check:[HGRuntimeChild classValue] == 33 && [HGRuntimeParent classValue] == 31 &&
        [HGRuntimeSibling classValue] == 31 name:@"metaclass replacement is also isolated from parent and sibling classes"];

    HGStackFirstOriginal = HGInstallVerifiedHook(HGRuntimeStacked.class, valueSelector, valueEncoding,
        (IMP)HGStackFirst, @"fixture");
    HGStackSecondOriginal = HGInstallVerifiedHook(HGRuntimeStacked.class, valueSelector, valueEncoding,
        (IMP)HGStackSecond, @"fixture");
    [self check:HGStackFirstOriginal == parentBefore && HGStackSecondOriginal == (IMP)HGStackFirst &&
        [[HGRuntimeStacked new] value] == 47 name:@"distinct local observers can intentionally compose on one method"];
    HGStackFirstOriginal = HGInstallVerifiedHook(HGRuntimeStacked.class, valueSelector, valueEncoding,
        (IMP)HGStackFirst, @"fixture");
    beforeCalls = HGValueCalls;
    [self check:class_getMethodImplementation(HGRuntimeStacked.class, valueSelector) == (IMP)HGStackSecond &&
        [[HGRuntimeStacked new] value] == 47 && HGValueCalls == beforeCalls + 1
        name:@"retrying an inner local observer preserves the outer observer and original call count"];

    HGRuntimeGetters *getters = [HGRuntimeGetters new];
    [self check:[HGReadObjectGetter(getters, "objectValue") isEqual:@"fixture"]
        name:@"verified object getter returns its ordinary result"];
    [self check:HGReadObjectGetter(getters, "scalarValue") == nil &&
        HGReadObjectGetter(getters, "rectValue") == nil && HGWrongGetterCalls == 0
        name:@"scalar and structure getters are rejected before invocation"];
    [self check:HGReadObjectGetter(getters, "notPresent") == nil && HGReadObjectGetter(nil, "objectValue") == nil &&
        HGReadObjectGetter(getters, NULL) == nil name:@"missing getter targets fail closed"];
    Method scalar = class_getInstanceMethod(HGRuntimeGetters.class, @selector(scalarValue));
    IMP scalarBefore = method_getImplementation(scalar);
    [self check:HGInstallVerifiedHook(HGRuntimeGetters.class, @selector(scalarValue), "@16@0:8",
        (IMP)HGChildValue, @"fixture") == NULL && method_getImplementation(scalar) == scalarBefore &&
        [getters scalarValue] == 123 name:@"wrong hook ABI preserves the original scalar method"];
    [self check:HGInstallVerifiedHook(Nil, valueSelector, valueEncoding, (IMP)HGChildValue, @"fixture") == NULL &&
        HGInstallVerifiedHook(HGRuntimeSibling.class, NULL, valueEncoding, (IMP)HGChildValue, @"fixture") == NULL &&
        HGInstallVerifiedHook(HGRuntimeSibling.class, valueSelector, NULL, (IMP)HGChildValue, @"fixture") == NULL &&
        HGInstallVerifiedHook(HGRuntimeSibling.class, valueSelector, valueEncoding, NULL, @"fixture") == NULL
        name:@"null hook inputs cannot mutate a method or crash preflight"];

    SSReadingAdSplashService *splash = [SSReadingAdSplashService new];
    [self check:[splash shouldShowSplashAd] && objc_lookUpClass("SSTabBar") == Nil &&
        objc_lookUpClass("SSSettingViewController") == Nil
        name:@"unsupported fixture identity leaves production startup inactive"];
    [defaults setBool:YES forKey:@"HongguoGlass.splash"];
    [self check:!HGInstallRuntimeModules() && ![splash shouldShowSplashAd]
        name:@"advertisement rules install even when settings and tab-bar classes are absent"];
    [self check:[HGCapability(@"module.tabbar")[@"state"] isEqual:@"unavailable"] &&
        [HGCapability(@"module.ads")[@"installed"] unsignedIntegerValue] == 1
        name:@"module inventory reports independent tab-bar failure and ad success"];
    [self check:[HGCapability(@"control.SS511ProfileEditButtonView")[@"state"] isEqual:@"missing-class"]
        name:@"first pass records a missing optional control owner"];

    Class lateControl = objc_allocateClassPair(UIView.class, "SS511ProfileEditButtonView", 0);
    if (lateControl) {
        class_addMethod(lateControl, @selector(layoutSubviews), (IMP)HGLateControlLayout, "v16@0:8");
        objc_registerClassPair(lateControl);
    }
    [self check:lateControl != Nil name:@"fixture registers an optional owner after the first startup pass"];
    HGInstallRuntimeModules();
    IMP lateInstalled = lateControl ? class_getMethodImplementation(lateControl, @selector(layoutSubviews)) : NULL;
    UIView *lateView = lateControl ? [[lateControl alloc] initWithFrame:CGRectMake(0, 0, 80, 40)] : nil;
    NSUInteger layoutBefore = HGLateLayouts;
    [lateView layoutSubviews];
    [self check:lateInstalled && lateInstalled != (IMP)HGLateControlLayout && HGLateLayouts == layoutBefore + 1
        name:@"second startup pass installs a newly available control and calls its original once"];
    [self check:[HGCapability(@"control.SS511ProfileEditButtonView")[@"state"] isEqual:@"installed"]
        name:@"retry replaces stale missing-class status with actual installation status"];
    HGInstallControls();
    layoutBefore = HGLateLayouts;
    [lateView layoutSubviews];
    [self check:class_getMethodImplementation(lateControl, @selector(layoutSubviews)) == lateInstalled &&
        HGLateLayouts == layoutBefore + 1 name:@"repeated control installation reuses its hook without stacking blocks"];

    Class tooLate = objc_allocateClassPair(UIView.class, "SS703MineHistoryRightView", 0);
    if (tooLate) {
        class_addMethod(tooLate, @selector(layoutSubviews), (IMP)HGLateControlLayout, "v16@0:8");
        objc_registerClassPair(tooLate);
    }
    HGInstallRuntimeModules();
    [self check:tooLate && class_getMethodImplementation(tooLate, @selector(layoutSubviews)) == (IMP)HGLateControlLayout
        name:@"startup orchestration is bounded to two passes"];

    NSDictionary *beforeCapacity = HGRuntimeSnapshot();
    [self check:[NSJSONSerialization isValidJSONObject:beforeCapacity] &&
        [beforeCapacity[@"entries"] count] <= [beforeCapacity[@"capacity"] unsignedIntegerValue]
        name:@"structural runtime snapshot is bounded and JSON serializable"];
    [self check:[HGInterfaceSnapshot()[@"runtime"] isKindOfClass:NSDictionary.class]
        name:@"exported interface snapshot includes runtime compatibility evidence"];
    for (NSUInteger index = 0; index < 300; index++)
        HGRecordCapability([NSString stringWithFormat:@"capacity.%lu", (unsigned long)index],
            @{@"module": @"fixture", @"state": @"disabled"});
    NSDictionary *bounded = HGRuntimeSnapshot();
    [self check:[bounded[@"entries"] count] == 256 && [bounded[@"omitted"] unsignedIntegerValue] > 0
        name:@"inventory refuses growth beyond its fixed entry limit"];
    HGRecordCapability(@"module.ads", @{@"module": @"ads", @"state": @"partial", @"reason": @"fixture refresh"});
    [self check:[HGCapability(@"module.ads")[@"reason"] isEqual:@"fixture refresh"]
        name:@"full inventory still permits refreshing an existing record"];

    NSUInteger failed = 0;
    for (NSDictionary *check in self.checks) failed += ![check[@"passed"] boolValue];
    NSDictionary *report = @{@"plugin": HG_PLUGIN_NAME, @"version": HG_PLUGIN_VERSION,
        @"scope": @"real Objective-C runtime with simulated host classes; not device acceptance",
        @"status": failed ? @"failed" : @"passed", @"failed": @(failed),
        @"total": @(self.checks.count), @"checks": self.checks, @"runtime": beforeCapacity};
    NSURL *directory = [NSFileManager.defaultManager URLsForDirectory:NSDocumentDirectory
        inDomains:NSUserDomainMask].firstObject;
    [NSFileManager.defaultManager createDirectoryAtURL:directory withIntermediateDirectories:YES attributes:nil error:nil];
    NSData *data = [NSJSONSerialization dataWithJSONObject:report options:NSJSONWritingPrettyPrinted error:nil];
    [data writeToURL:[directory URLByAppendingPathComponent:@"runtime-results.json"] atomically:YES];
}
@end

@interface HGRuntimeAppDelegate : UIResponder <UIApplicationDelegate> @end
@implementation HGRuntimeAppDelegate
- (UISceneConfiguration *)application:(UIApplication *)application
        configurationForConnectingSceneSession:(UISceneSession *)session options:(UISceneConnectionOptions *)options {
    UISceneConfiguration *configuration = [[UISceneConfiguration alloc] initWithName:@"HonggoRuntimeTest" sessionRole:session.role];
    configuration.delegateClass = HGRuntimeTests.class;
    return configuration;
}
@end

int main(int argc, char *argv[]) {
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass(HGRuntimeAppDelegate.class));
    }
}
