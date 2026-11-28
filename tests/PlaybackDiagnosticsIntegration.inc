// Included only by RuntimeIntegration.m. These controlled host notifications
// verify observer transparency; they do not reproduce the Hongguo player.
@interface BDRpcError : NSError @end
@implementation BDRpcError @end
@interface FQCRpcDoArticleActionResponse : NSObject
@property(nonatomic) int code;
@end
@implementation FQCRpcDoArticleActionResponse @end
@interface SSShortVideoDiggManager : NSObject
@property(nonatomic,strong) BDRpcError *error;
@property(nonatomic,strong) FQCRpcDoArticleActionResponse *response;
@end
@implementation SSShortVideoDiggManager
- (void)performDiggWithData:(void (^)(void))data request:(void (^)(void))request
                completion:(void (^)(BDRpcError *,FQCRpcDoArticleActionResponse *,BOOL))completion {
    data();request();completion(self.error,self.response,YES);
}
@end
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

@interface HGPlaybackPreDecodeEngineFixture : NSObject
@property(nonatomic, strong) NSMutableDictionary<NSNumber *, id> *options;
@property(nonatomic) NSUInteger setCalls;
- (id)getOptionBykey:(id)key;
- (void)setOptionForKey:(NSInteger)key value:(id)value;
@end
@implementation HGPlaybackPreDecodeEngineFixture
- (instancetype)init {
    if ((self = [super init])) _options = [NSMutableDictionary dictionary];
    return self;
}
- (id)getOptionBykey:(id)key { return key ? self.options[key] : nil; }
- (void)setOptionForKey:(NSInteger)key value:(id)value {
    self.setCalls++;
    if (value) self.options[@(key)] = value;
    else [self.options removeObjectForKey:@(key)];
}
@end

@interface HGPlaybackPreDecodePlayerFixture : NSObject
@property(nonatomic, strong) HGPlaybackPreDecodeEngineFixture *videoEngine;
@property(nonatomic, copy) NSString *codecid;
@end
@implementation HGPlaybackPreDecodePlayerFixture @end

@interface HGPlaybackPreDecodeCellViewFixture : NSObject
@property(nonatomic, strong) HGPlaybackPreDecodePlayerFixture *vPlayer;
@end
@implementation HGPlaybackPreDecodeCellViewFixture @end

@interface HGPlaybackPreDecodeModelFixture : NSObject
@property(nonatomic, copy) NSString *vid;
@property(nonatomic, strong) HGPlaybackPreDecodeCellViewFixture *cellView;
@property(nonatomic, strong) HGPlaybackPreDecodeCellViewFixture *cellViewContext;
@end
@implementation HGPlaybackPreDecodeModelFixture @end

@interface HGPlaybackPreDecodePoolFixture : NSObject
@property(nonatomic, strong) NSMapTable<id, HGPlaybackPreDecodePlayerFixture *> *players;
- (id)playerWithModel:(id)model;
@end
@implementation HGPlaybackPreDecodePoolFixture
- (instancetype)init {
    if ((self = [super init])) _players = [NSMapTable strongToStrongObjectsMapTable];
    return self;
}
- (id)playerWithModel:(id)model { return [self.players objectForKey:model]; }
@end

@interface HGPlaybackPreDecodePlayerCompFixture : NSObject
@property(nonatomic, strong) HGPlaybackPreDecodePoolFixture *playerPool;
@end
@implementation HGPlaybackPreDecodePlayerCompFixture @end

@interface HGPlaybackPreDecodeContextFixture : NSObject
@property(nonatomic, strong) HGPlaybackPreDecodePlayerFixture *currentPlayer;
@property(nonatomic, strong) HGPlaybackPreDecodePlayerCompFixture *playerComponent;
@property(nonatomic, strong) HGPlaybackPreDecodeModelFixture *nextModel;
@end
@implementation HGPlaybackPreDecodeContextFixture @end

// Deliberately incompatible -enable signature ("q16@0:8" instead of "B16@0:8")
// to verify preflight ABI rejection before overriding on the child class.
@interface HGPlaybackMismatchedPreDecodeParent : NSObject
@property(nonatomic) BOOL rawEnable;
@property(nonatomic) NSInteger prepareDecodeBufferSize;
@property(nonatomic) NSInteger playDecodeBufferSize;
@property(nonatomic) NSUInteger willPlayCalls;
@property(nonatomic) NSUInteger restoreCalls;
@property(nonatomic, strong) NSMutableSet<NSString *> *hasSetPreDecodeVids;
@property(nonatomic, strong) HGPlaybackPreDecodeContextFixture *listVCContext;
- (NSInteger)enable;
- (void)handlePlaybackTimeChanged:(double)playbackTime duration:(double)duration;
- (void)handleVideoWillPlay:(id)model;
- (void)restorePlayDecodeBufferIfNeededWithModel:(id)model;
- (id)videoCodecidWithPlayer:(id)player;
- (NSInteger)playDecodeBufferSizeWithVideoCodecid:(id)codecid;
@end
@implementation HGPlaybackMismatchedPreDecodeParent
- (instancetype)init {
    if ((self = [super init])) {
        _rawEnable = YES;
        _prepareDecodeBufferSize = 1;
        _playDecodeBufferSize = 8;
        _hasSetPreDecodeVids = [NSMutableSet set];
    }
    return self;
}
- (NSInteger)enable { return 77; }
- (void)handlePlaybackTimeChanged:(double)playbackTime duration:(double)duration {
    BOOL active = ((BOOL (*)(id, SEL))objc_msgSend)(self, @selector(enable));
    if (!active || duration - playbackTime > 2.0) return;
    HGPlaybackPreDecodeModelFixture *next = self.listVCContext.nextModel;
    HGPlaybackPreDecodePlayerFixture *player = [self.listVCContext.playerComponent.playerPool playerWithModel:next];
    if (player.videoEngine && next.vid) {
        [player.videoEngine setOptionForKey:0x6015 value:@(self.prepareDecodeBufferSize)];
        [self.hasSetPreDecodeVids addObject:next.vid];
    }
}
- (void)handleVideoWillPlay:(id)model {
    self.willPlayCalls++;
    BOOL active = ((BOOL (*)(id, SEL))objc_msgSend)(self, @selector(enable));
    if (active) [self restorePlayDecodeBufferIfNeededWithModel:model];
}
- (void)restorePlayDecodeBufferIfNeededWithModel:(id)model {
    self.restoreCalls++;
    // Mirrors Eggplant 7.4.7.32: reads listVCContext.currentPlayer, which is nil
    // after resetToPlayNoneVideo clears currentVideoDataModel during transition.
    HGPlaybackPreDecodePlayerFixture *player = self.listVCContext.currentPlayer;
    HGPlaybackPreDecodeEngineFixture *engine = player.videoEngine;
    if (!player || !engine) return;
    if ([[engine getOptionBykey:@(0x6015)] intValue] == 0) return;
    id codecid = [self videoCodecidWithPlayer:player];
    NSInteger playSize = [self playDecodeBufferSizeWithVideoCodecid:codecid];
    [engine setOptionForKey:0x6015 value:@(playSize)];
}
- (id)videoCodecidWithPlayer:(id)player {
    return ((HGPlaybackPreDecodePlayerFixture *)player).codecid ?: @"h265";
}
- (NSInteger)playDecodeBufferSizeWithVideoCodecid:(id)codecid {
    return [codecid isEqual:@"bytevc2"] ? 12 : self.playDecodeBufferSize;
}
@end

static BOOL HGPlaybackValidPreDecodeEnable(id object, SEL selector) {
    (void)selector;
    return ((HGPlaybackMismatchedPreDecodeParent *)object).rawEnable;
}

static NSDictionary *HGPlaybackCapabilityEntry(NSString *identifier) {
    for (NSDictionary *entry in HGRuntimeSnapshot()[@"entries"])
        if ([entry[@"id"] isEqual:identifier]) return entry;
    return nil;
}

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
    SSShortVideoDiggManager *digg=[SSShortVideoDiggManager new];
    digg.error=[BDRpcError errorWithDomain:NSURLErrorDomain code:-1009 userInfo:nil];
    digg.response=[FQCRpcDoArticleActionResponse new];
    digg.response.code=403;
    __block NSUInteger steps=0;
    [digg performDiggWithData:^{steps++;} request:^{steps++;}
        completion:^(BDRpcError *error,FQCRpcDoArticleActionResponse *response,BOOL flag) {
            if (error==digg.error && response==digg.response && flag) steps++;
        }];
    check(steps==3,@"like failure observation forwards the original data, request, error, response and flag exactly once");
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

    check([HGPlaybackCapabilityEntry(@"repair.playback.predecode")[@"state"] isEqual:@"missing-class"],
        @"pre-decode repair records missing-class before Eggplant 7.4.7.32 component exists");
    Class preDecodeClass = objc_allocateClassPair(HGPlaybackMismatchedPreDecodeParent.class,
        "FQVShortVideoPreDecodeComponent", 0);
    if (preDecodeClass) objc_registerClassPair(preDecodeClass);
    HGInstallPlaybackDiagnostics();
    HGPlaybackMismatchedPreDecodeParent *preDecode = [preDecodeClass new];
    check(preDecodeClass != Nil &&
        [HGPlaybackCapabilityEntry(@"repair.playback.predecode")[@"state"] isEqual:@"abi-mismatch"] &&
        ((NSInteger (*)(id, SEL))objc_msgSend)(preDecode, @selector(enable)) == 77,
        @"pre-decode repair rejects an incompatible -enable ABI without touching host methods");

    BOOL preDecodeEnableAdded = class_addMethod(preDecodeClass, @selector(enable),
        (IMP)HGPlaybackValidPreDecodeEnable, "B16@0:8");
    HGInstallPlaybackDiagnostics();
    check(preDecodeEnableAdded &&
        [HGPlaybackCapabilityEntry(@"repair.playback.predecode")[@"state"] isEqual:@"installed"] &&
        !((BOOL (*)(id, SEL))objc_msgSend)(preDecode, @selector(enable)),
        @"pre-decode repair installs on verified 7.4.7.32 signatures and declines 1-frame pre-decode");

    HGPlaybackPreDecodePoolFixture *pool = [HGPlaybackPreDecodePoolFixture new];
    HGPlaybackPreDecodePlayerCompFixture *playerComp = [HGPlaybackPreDecodePlayerCompFixture new];
    playerComp.playerPool = pool;
    HGPlaybackPreDecodeContextFixture *preDecodeCtx = [HGPlaybackPreDecodeContextFixture new];
    preDecodeCtx.playerComponent = playerComp;
    preDecode.listVCContext = preDecodeCtx;

    HGPlaybackPreDecodeEngineFixture *ep2Engine = [HGPlaybackPreDecodeEngineFixture new];
    HGPlaybackPreDecodePlayerFixture *ep2Player = [HGPlaybackPreDecodePlayerFixture new];
    ep2Player.videoEngine = ep2Engine;
    HGPlaybackPreDecodeCellViewFixture *ep2Cell = [HGPlaybackPreDecodeCellViewFixture new];
    ep2Cell.vPlayer = ep2Player;
    HGPlaybackPreDecodeModelFixture *ep2Model = [HGPlaybackPreDecodeModelFixture new];
    ep2Model.vid = @"vid-ep2";
    ep2Model.cellView = ep2Cell;
    [pool.players setObject:ep2Player forKey:ep2Model];
    preDecodeCtx.nextModel = ep2Model;

    [preDecode handlePlaybackTimeChanged:9.5 duration:10.0];
    check([ep2Engine getOptionBykey:@(0x6015)] == nil && ep2Engine.setCalls == 0 &&
        preDecode.hasSetPreDecodeVids.count == 0,
        @"declined pre-decode keeps Episode 2 pooled TTVideoEngine frame queue uncapped during Episode 1 playback");

    // Reproduce a pooled engine already capped to 1 frame when playWithListVCDataModel:
    // clears currentVideoDataModel (so listVCContext.currentPlayer == nil on transition).
    [ep2Engine setOptionForKey:0x6015 value:@1];
    [preDecode.hasSetPreDecodeVids addObject:@"vid-ep2"];
    preDecodeCtx.currentPlayer = nil;
    [preDecode handleVideoWillPlay:ep2Model];
    check([[ep2Engine getOptionBykey:@(0x6015)] isEqual:@8] &&
        ![preDecode.hasSetPreDecodeVids containsObject:@"vid-ep2"] &&
        preDecode.willPlayCalls == 1 && preDecode.restoreCalls == 1,
        @"will-play fallback restores cellView TTVideoEngine option 0x6015 from 1 to 8 when currentPlayer is nil");

    // Also verify playerPool fallback when cellView is nil and codec-specific target size.
    HGPlaybackPreDecodeEngineFixture *ep3Engine = [HGPlaybackPreDecodeEngineFixture new];
    [ep3Engine setOptionForKey:0x6015 value:@1];
    HGPlaybackPreDecodePlayerFixture *ep3Player = [HGPlaybackPreDecodePlayerFixture new];
    ep3Player.videoEngine = ep3Engine;
    ep3Player.codecid = @"bytevc2";
    HGPlaybackPreDecodeModelFixture *ep3Model = [HGPlaybackPreDecodeModelFixture new];
    ep3Model.vid = @"vid-ep3";
    [pool.players setObject:ep3Player forKey:ep3Model];
    [preDecode handleVideoWillPlay:ep3Model];
    NSUInteger ep3Writes = ep3Engine.setCalls;
    [ep3Engine setOptionForKey:0x6015 value:@16];
    [preDecode handleVideoWillPlay:ep3Model];
    check([[ep3Engine getOptionBykey:@(0x6015)] isEqual:@16] && ep3Writes == 2 && ep3Engine.setCalls == 3,
        @"playerPool fallback restores codec buffer size and never shrinks an already larger decode buffer");

    IMP enableInstalled = class_getMethodImplementation(preDecodeClass, @selector(enable));
    IMP willPlayInstalled = class_getMethodImplementation(preDecodeClass, @selector(handleVideoWillPlay:));
    IMP restoreInstalled = class_getMethodImplementation(preDecodeClass, @selector(restorePlayDecodeBufferIfNeededWithModel:));
    HGInstallPlaybackDiagnostics();
    HGInstallPlaybackDiagnostics();
    [preDecode handleVideoWillPlay:ep2Model];
    check(class_getMethodImplementation(preDecodeClass, @selector(enable)) == enableInstalled &&
        class_getMethodImplementation(preDecodeClass, @selector(handleVideoWillPlay:)) == willPlayInstalled &&
        class_getMethodImplementation(preDecodeClass, @selector(restorePlayDecodeBufferIfNeededWithModel:)) == restoreInstalled &&
        preDecode.willPlayCalls == 4 && preDecode.restoreCalls == 4,
        @"repeated pre-decode repair installation retains one nonrecursive original invocation chain");

    HGSetEnhancedDiagnostics(wasEnhanced);
}
