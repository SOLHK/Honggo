// Included only by RuntimeIntegration.m. These controlled host notifications
// verify observer transparency; they do not reproduce the Hongguo player.
@interface SSBizVideoPatchAdService : NSObject
@property(nonatomic) NSUInteger cleanCalls;
@property(nonatomic) NSUInteger modelCalls;
@property(nonatomic) BOOL lastCleanMode;
@property(nonatomic) BOOL nestedModelNotification;
@property(nonatomic, strong) id lastContext;
@property(nonatomic, strong) NSThread *lastThread;
@property(nonatomic, strong) NSMutableArray<NSString *> *order;
- (void)videoListVCContextDidChangeCleanMode:(id)context isCleanMode:(BOOL)cleanMode;
- (void)videoListVCContextPlayModelDidUpdatedNotifyAtScrollEnd:(id)context;
@end
@implementation SSBizVideoPatchAdService
- (void)videoListVCContextDidChangeCleanMode:(id)context isCleanMode:(BOOL)cleanMode {
    self.cleanCalls++;
    self.lastContext = context;
    self.lastCleanMode = cleanMode;
    self.lastThread = NSThread.currentThread;
    [self.order addObject:@"clean.enter"];
    if (self.nestedModelNotification) [self videoListVCContextPlayModelDidUpdatedNotifyAtScrollEnd:context];
    [self.order addObject:@"clean.return"];
}
- (void)videoListVCContextPlayModelDidUpdatedNotifyAtScrollEnd:(id)context {
    self.modelCalls++;
    self.lastContext = context;
    self.lastThread = NSThread.currentThread;
    [self.order addObject:@"model"];
}
@end

static IMP HGPlaybackFixtureForeignCleanOriginal;
static NSUInteger HGPlaybackFixtureForeignCleanCalls;
static void HGPlaybackFixtureForeignClean(id object, SEL selector, id context, BOOL cleanMode) {
    HGPlaybackFixtureForeignCleanCalls++;
    ((void (*)(id, SEL, id, BOOL))HGPlaybackFixtureForeignCleanOriginal)(object, selector, context, cleanMode);
}

// Deliberately incompatible with the production void notification. The
// production observer must leave this implementation and its scalar result.
@interface HGPlaybackMismatchedPauseParent : NSObject
@property(nonatomic) NSUInteger calls;
@property(nonatomic, strong) id lastContext;
@property(nonatomic, strong) NSThread *lastThread;
- (NSInteger)videoListVCContextPlayStateDidUpdated:(id)context;
@end
@implementation HGPlaybackMismatchedPauseParent
- (NSInteger)videoListVCContextPlayStateDidUpdated:(id)context {
    self.calls++;
    self.lastContext = context;
    return 91;
}
@end
@interface SSBizVideoPauseAdManager : HGPlaybackMismatchedPauseParent @end
@implementation SSBizVideoPauseAdManager @end

static void HGPlaybackValidState(id object, SEL selector, id context) {
    SSBizVideoPauseAdManager *pause = object;
    pause.calls++;
    pause.lastContext = context;
    pause.lastThread = NSThread.currentThread;
}

@interface SSBizVideoAdImpl : NSObject
@property(nonatomic) NSUInteger calls;
@property(nonatomic) BOOL result;
@property(nonatomic, strong) id lastContext;
@property(nonatomic, strong) id lastModel;
@property(nonatomic, strong) NSThread *lastThread;
@property(nonatomic, strong) NSException *exception;
- (BOOL)shouldInterceptVideoFinishWithContext:(id)context videoDataModel:(id)model;
@end
@implementation SSBizVideoAdImpl
- (BOOL)shouldInterceptVideoFinishWithContext:(id)context videoDataModel:(id)model {
    self.calls++;
    self.lastContext = context;
    self.lastModel = model;
    self.lastThread = NSThread.currentThread;
    if (self.exception) @throw self.exception;
    return self.result;
}
@end

static void HGRunPlaybackDiagnosticsChecks(void (^check)(BOOL, NSString *)) {
    BOOL wasEnhanced = HGEnhancedDiagnostics();
    HGSetEnhancedDiagnostics(NO);
    SEL clean = @selector(videoListVCContextDidChangeCleanMode:isCleanMode:);
    SEL model = @selector(videoListVCContextPlayModelDidUpdatedNotifyAtScrollEnd:);
    SEL state = @selector(videoListVCContextPlayStateDidUpdated:);
    SEL finish = @selector(shouldInterceptVideoFinishWithContext:videoDataModel:);
    IMP cleanBefore = class_getMethodImplementation(SSBizVideoPatchAdService.class, clean);
    IMP modelBefore = class_getMethodImplementation(SSBizVideoPatchAdService.class, model);
    IMP stateBefore = class_getMethodImplementation(SSBizVideoPauseAdManager.class, state);
    IMP finishBefore = class_getMethodImplementation(SSBizVideoAdImpl.class, finish);
    HGInstallPlaybackDiagnostics();
    check(class_getMethodImplementation(SSBizVideoPatchAdService.class, clean) != cleanBefore &&
        class_getMethodImplementation(SSBizVideoPatchAdService.class, model) != modelBefore &&
        class_getMethodImplementation(SSBizVideoAdImpl.class, finish) != finishBefore,
        @"playback observers install only on supported host notification signatures");
    check(class_getMethodImplementation(SSBizVideoPauseAdManager.class, state) == stateBefore,
        @"playback state observer preserves an incompatible host notification ABI");

    NSObject *context = [NSObject new], *video = [NSObject new];
    SSBizVideoPatchAdService *patch = [SSBizVideoPatchAdService new];
    SSBizVideoAdImpl *gate = [SSBizVideoAdImpl new];
    [patch videoListVCContextDidChangeCleanMode:context isCleanMode:NO];
    check(patch.cleanCalls == 1 && patch.lastContext == context && !patch.lastCleanMode &&
        patch.lastThread == NSThread.currentThread,
        @"disabled playback tracing forwards clean-mode arguments once on the caller thread");
    [patch videoListVCContextPlayModelDidUpdatedNotifyAtScrollEnd:context];
    check(patch.modelCalls == 1 && patch.lastContext == context,
        @"disabled playback tracing forwards the original model notification once");
    BOOL returned = [gate shouldInterceptVideoFinishWithContext:context videoDataModel:video];
    check(!returned && gate.calls == 1 && gate.lastContext == context && gate.lastModel == video,
        @"disabled playback tracing preserves the host finish decision and both object identities");

    HGSetEnhancedDiagnostics(YES);
    [patch videoListVCContextDidChangeCleanMode:context isCleanMode:YES];
    check(patch.cleanCalls == 2 && patch.lastCleanMode && patch.lastContext == context,
        @"enabled playback tracing preserves entering clean mode and original call count");
    returned = [gate shouldInterceptVideoFinishWithContext:context videoDataModel:video];
    gate.result = YES;
    BOOL intercepted = [gate shouldInterceptVideoFinishWithContext:context videoDataModel:video];
    check(!returned && intercepted && gate.calls == 3,
        @"enabled playback tracing preserves both accepting and intercepting finish decisions");

    [patch videoListVCContextDidChangeCleanMode:nil isCleanMode:NO];
    [patch videoListVCContextPlayModelDidUpdatedNotifyAtScrollEnd:nil];
    returned = [gate shouldInterceptVideoFinishWithContext:nil videoDataModel:nil];
    check(patch.cleanCalls == 3 && patch.modelCalls == 2 && patch.lastContext == nil &&
        !patch.lastCleanMode && returned && gate.calls == 4 && gate.lastContext == nil && gate.lastModel == nil,
        @"playback observers preserve nil lifecycle arguments without manufacturing context");

    patch.nestedModelNotification = YES;
    patch.order = [NSMutableArray array];
    [patch videoListVCContextDidChangeCleanMode:context isCleanMode:YES];
    check(patch.cleanCalls == 4 && patch.modelCalls == 3 &&
        [patch.order isEqual:@[@"clean.enter", @"model", @"clean.return"]],
        @"nested model notification keeps host order without duplicate playback callbacks");
    patch.nestedModelNotification = NO;

    NSException *expected = [NSException exceptionWithName:@"PlaybackFixture" reason:@"fixture" userInfo:nil];
    gate.exception = expected;
    NSException *caught = nil;
    @try { [gate shouldInterceptVideoFinishWithContext:context videoDataModel:video]; }
    @catch (NSException *exception) { caught = exception; }
    check(caught == expected && gate.calls == 5,
        @"playback observer propagates the exact host exception after one original call");
    gate.exception = nil;

    IMP cleanInstalled = class_getMethodImplementation(SSBizVideoPatchAdService.class, clean);
    IMP modelInstalled = class_getMethodImplementation(SSBizVideoPatchAdService.class, model);
    IMP finishInstalled = class_getMethodImplementation(SSBizVideoAdImpl.class, finish);
    HGInstallPlaybackDiagnostics();
    HGInstallPlaybackDiagnostics();
    [patch videoListVCContextDidChangeCleanMode:context isCleanMode:NO];
    [patch videoListVCContextPlayModelDidUpdatedNotifyAtScrollEnd:context];
    returned = [gate shouldInterceptVideoFinishWithContext:context videoDataModel:video];
    check(class_getMethodImplementation(SSBizVideoPatchAdService.class, clean) == cleanInstalled &&
        class_getMethodImplementation(SSBizVideoPatchAdService.class, model) == modelInstalled &&
        class_getMethodImplementation(SSBizVideoAdImpl.class, finish) == finishInstalled &&
        patch.cleanCalls == 5 && patch.modelCalls == 4 && gate.calls == 6 && returned,
        @"reinstalling playback observers retains a single original invocation without recursion");

    __block BOOL callerThreadRetained = NO;
    dispatch_queue_t queue = dispatch_queue_create("org.honggo.playback-fixture", DISPATCH_QUEUE_SERIAL);
    dispatch_sync(queue, ^{
        NSThread *caller = NSThread.currentThread;
        [patch videoListVCContextDidChangeCleanMode:context isCleanMode:YES];
        [gate shouldInterceptVideoFinishWithContext:context videoDataModel:video];
        callerThreadRetained = patch.lastThread == caller && gate.lastThread == caller;
    });
    check(callerThreadRetained && patch.cleanCalls == 6 && gate.calls == 7,
        @"playback observations preserve the invoking queue instead of dispatching host work");

    SSBizVideoPauseAdManager *pause = [SSBizVideoPauseAdManager new];
    NSInteger value = [pause videoListVCContextPlayStateDidUpdated:context];
    check(value == 91 && pause.calls == 1 && pause.lastContext == context &&
        class_getMethodImplementation(SSBizVideoPauseAdManager.class, state) == stateBefore,
        @"retries preserve the rejected playback-state method, arguments and scalar result");

    // Override an inherited, incompatible method locally after rejection.
    // class_addMethod adds a new encoding without altering the parent slot.
    BOOL added = class_addMethod(SSBizVideoPauseAdManager.class, state,
        (IMP)HGPlaybackValidState, "v24@0:8@16");
    HGInstallPlaybackDiagnostics();
    HGSetEnhancedDiagnostics(NO);
    ((void (*)(id, SEL, id))objc_msgSend)(pause, state, context);
    IMP stateInstalled = class_getMethodImplementation(SSBizVideoPauseAdManager.class, state);
    check(added && stateInstalled != (IMP)HGPlaybackValidState && pause.calls == 2 &&
        pause.lastContext == context && pause.lastThread == NSThread.currentThread,
        @"late-compatible playback-state notification forwards once with tracing disabled");
    HGSetEnhancedDiagnostics(YES);
    ((void (*)(id, SEL, id))objc_msgSend)(pause, state, nil);
    check(pause.calls == 3 && pause.lastContext == nil,
        @"enabled playback-state observation preserves nil context and original execution");
    HGInstallPlaybackDiagnostics();
    ((void (*)(id, SEL, id))objc_msgSend)(pause, state, context);
    check(pause.calls == 4 && pause.lastContext == context &&
        class_getMethodImplementation(SSBizVideoPauseAdManager.class, state) == stateInstalled &&
        class_getMethodImplementation(HGPlaybackMismatchedPauseParent.class, state) == stateBefore,
        @"playback-state retry keeps one original invocation and preserves the parent method");
    dispatch_sync(queue, ^{
        NSThread *caller = NSThread.currentThread;
        ((void (*)(id, SEL, id))objc_msgSend)(pause, state, context);
        callerThreadRetained = pause.lastThread == caller;
    });
    check(callerThreadRetained && pause.calls == 5,
        @"playback-state observation preserves the original caller queue");

    HGPlaybackFixtureForeignCleanOriginal = cleanInstalled;
    class_replaceMethod(SSBizVideoPatchAdService.class, clean,
        (IMP)HGPlaybackFixtureForeignClean, "v28@0:8@16B24");
    HGInstallPlaybackDiagnostics();
    [patch videoListVCContextDidChangeCleanMode:context isCleanMode:YES];
    check(class_getMethodImplementation(SSBizVideoPatchAdService.class, clean) ==
        (IMP)HGPlaybackFixtureForeignClean && HGPlaybackFixtureForeignCleanCalls == 1 && patch.cleanCalls == 7,
        @"playback observer retry preserves a later interposer without making it the original");
    class_replaceMethod(SSBizVideoPatchAdService.class, clean, cleanInstalled, "v28@0:8@16B24");
    HGSetEnhancedDiagnostics(wasEnhanced);
}
