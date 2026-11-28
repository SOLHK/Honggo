#import "Honggo.h"
#import "RuntimeSupport.h"
#import <objc/message.h>
#include <stdatomic.h>
#include <string.h>

// Observers for the exact Eggplant 7.4.3.33 method encodings in the cached
// audit index. These events describe host notifications, not measured video
// frames, playback rate or a proven episode transition.
static _Atomic(IMP) HGOriginalCleanMode;
static _Atomic(IMP) HGOriginalPlayModelUpdate;
static _Atomic(IMP) HGOriginalPlayStateUpdate;
static _Atomic(IMP) HGOriginalFinishGate;
static _Atomic(IMP) HGOriginalDiggOperation;
static atomic_ullong HGPlaybackInvocation;

// Eggplant 7.4.7.32 added FQVShortVideoPreDecodeComponent, which shrinks the
// next pooled TTVideoEngine's vc2_dec frame queue (option 0x6015 = 24597) to
// 1 frame while Episode N plays. On transition to Episode N+1,
// -[FQVShortVideoListVCPlayComponent playWithListVCDataModel:] clears
// currentVideoDataModel via resetToPlayNoneVideo before cellView:playWithType:
// invokes handleVideoWillPlay:, so restorePlayDecodeBufferIfNeededWithModel:
// reads listVCContext.currentPlayer == nil and never restores option 0x6015.
// Metal holds decoded Frame 1 on screen while vc2_dec has 0 free frame buffers
// for Frame 2, freezing the picture while audio and progress keep advancing.
static const NSInteger HGTTVideoEngineOptionPreDecodeSize = 0x6015;
static _Atomic(IMP) HGOriginalPreDecodeEnable;
static _Atomic(IMP) HGOriginalPreDecodeWillPlay;
static _Atomic(IMP) HGOriginalPreDecodeRestore;

static BOOL HGClassHasMethodEncoding(Class cls, SEL selector, const char *expected) {
    if (!cls || !selector || !expected) return NO;
    Method method = class_getInstanceMethod(cls, selector);
    const char *encoding = method ? method_getTypeEncoding(method) : NULL;
    return encoding && strcmp(encoding, expected) == 0;
}

static id HGResolvePreDecodePlayer(id component, id model) {
    id player = HGReadObjectGetter(HGReadObjectGetter(model, "cellView"), "vPlayer");
    if (!player) player = HGReadObjectGetter(HGReadObjectGetter(model, "cellViewContext"), "vPlayer");
    id context = HGReadObjectGetter(component, "listVCContext");
    if (!player) {
        id playerComp = HGReadObjectGetter(context, "playerComponent");
        id pool = HGReadObjectGetter(playerComp, "playerPool");
        SEL lookup = sel_registerName("playerWithModel:");
        if (HGClassHasMethodEncoding(object_getClass(pool), lookup, "@24@0:8@16"))
            player = ((id (*)(id, SEL, id))objc_msgSend)(pool, lookup, model);
    }
    if (!player) player = HGReadObjectGetter(context, "currentPlayer");
    return player;
}

static BOOL HGPreDecodeEnable(id object, SEL selector) {
    static _Atomic uint64_t nextLog;
    IMP original = atomic_load_explicit(&HGOriginalPreDecodeEnable, memory_order_acquire);
    BOOL hostEnabled = original ? ((BOOL (*)(id, SEL))original)(object, selector) : NO;
    if (hostEnabled && HGEnhancedDiagnostics() && HGDiagnosticSample(&nextLog))
        HGLog(@"playback.predecode-guard", @{@"suppressed": @YES});
    return NO;
}

static void HGPreDecodeRestoreBuffer(id object, SEL selector, id model) {
    IMP original = atomic_load_explicit(&HGOriginalPreDecodeRestore, memory_order_acquire);
    if (original) ((void (*)(id, SEL, id))original)(object, selector, model);
    if (!model) return;
    id player = HGResolvePreDecodePlayer(object, model);
    id engine = HGReadObjectGetter(player, "videoEngine");
    Class engineClass = object_getClass(engine);
    SEL getOption = sel_registerName("getOptionBykey:");
    SEL setOption = sel_registerName("setOptionForKey:value:");
    if (!HGClassHasMethodEncoding(engineClass, getOption, "@24@0:8@16") ||
            !HGClassHasMethodEncoding(engineClass, setOption, "v32@0:8q16@24"))
        return;
    id current = ((id (*)(id, SEL, id))objc_msgSend)(engine, getOption, @(HGTTVideoEngineOptionPreDecodeSize));
    if (![current isKindOfClass:NSNumber.class]) return;
    NSInteger currentCap = [(NSNumber *)current integerValue];
    if (currentCap <= 0) return;
    Class componentClass = object_getClass(object);
    SEL codecSel = sel_registerName("videoCodecidWithPlayer:");
    SEL sizeSel = sel_registerName("playDecodeBufferSizeWithVideoCodecid:");
    id codecid = HGClassHasMethodEncoding(componentClass, codecSel, "@24@0:8@16")
        ? ((id (*)(id, SEL, id))objc_msgSend)(object, codecSel, player) : nil;
    NSInteger playSize = HGClassHasMethodEncoding(componentClass, sizeSel, "q24@0:8@16")
        ? ((NSInteger (*)(id, SEL, id))objc_msgSend)(object, sizeSel, codecid) : 8;
    if (playSize <= 0) playSize = 8;
    if (currentCap >= playSize) return;
    ((void (*)(id, SEL, NSInteger, id))objc_msgSend)(engine, setOption,
        HGTTVideoEngineOptionPreDecodeSize, @(playSize));
    id recordedVids = HGReadObjectGetter(object, "hasSetPreDecodeVids");
    id vid = HGReadObjectGetter(model, "vid");
    if ([recordedVids isKindOfClass:NSMutableSet.class] && [vid isKindOfClass:NSString.class])
        [(NSMutableSet *)recordedVids removeObject:vid];
    HGLog(@"playback.predecode-restored", @{@"from": @(currentCap), @"to": @(playSize)});
}

static void HGPreDecodeWillPlay(id object, SEL selector, id model) {
    IMP original = atomic_load_explicit(&HGOriginalPreDecodeWillPlay, memory_order_acquire);
    if (original) ((void (*)(id, SEL, id))original)(object, selector, model);
    if (!model) return;
    SEL restore = sel_registerName("restorePlayDecodeBufferIfNeededWithModel:");
    if (HGClassHasMethodEncoding(object_getClass(object), restore, "v24@0:8@16"))
        ((void (*)(id, SEL, id))objc_msgSend)(object, restore, model);
}

static void HGInstallPreDecodeBufferRepair(void) {
    Class target = objc_lookUpClass("FQVShortVideoPreDecodeComponent");
    if (!target) {
        HGRecordCapability(@"repair.playback.predecode", @{@"module": @"playback",
            @"state": @"missing-class", @"reason": @"host build predates FQVShortVideoPreDecodeComponent"});
        return;
    }
    static const struct {
        const char *selector;
        const char *encoding;
    } required[] = {
        {"enable", "B16@0:8"},
        {"handlePlaybackTimeChanged:duration:", "v32@0:8d16d24"},
        {"handleVideoWillPlay:", "v24@0:8@16"},
        {"restorePlayDecodeBufferIfNeededWithModel:", "v24@0:8@16"},
        {"videoCodecidWithPlayer:", "@24@0:8@16"},
        {"playDecodeBufferSizeWithVideoCodecid:", "q24@0:8@16"},
    };
    static const struct {
        const char *selector;
        const char *encoding;
        IMP replacement;
        _Atomic(IMP) *original;
    } hooks[] = {
        {"enable", "B16@0:8", (IMP)HGPreDecodeEnable, &HGOriginalPreDecodeEnable},
        {"handleVideoWillPlay:", "v24@0:8@16", (IMP)HGPreDecodeWillPlay, &HGOriginalPreDecodeWillPlay},
        {"restorePlayDecodeBufferIfNeededWithModel:", "v24@0:8@16",
            (IMP)HGPreDecodeRestoreBuffer, &HGOriginalPreDecodeRestore},
    };
    static BOOL preDecodeCaptured[sizeof(hooks) / sizeof(hooks[0])];
    @synchronized (target) {
        for (NSUInteger index = 0; index < sizeof(required) / sizeof(required[0]); index++) {
            if (!HGClassHasMethodEncoding(target, sel_registerName(required[index].selector),
                    required[index].encoding)) {
                HGLog(@"hook.skipped", @{@"module": @"playback",
                    @"class": @"FQVShortVideoPreDecodeComponent", @"reason": @"abi-mismatch"});
                HGRecordCapability(@"repair.playback.predecode", @{@"module": @"playback",
                    @"state": @"abi-mismatch",
                    @"reason": @"unexpected FQVShortVideoPreDecodeComponent signature"});
                return;
            }
        }
        NSUInteger installed = 0;
        for (NSUInteger index = 0; index < sizeof(hooks) / sizeof(hooks[0]); index++) {
            SEL selector = sel_registerName(hooks[index].selector);
            if (!preDecodeCaptured[index]) {
                Method method = class_getInstanceMethod(target, selector);
                const char *encoding = method ? method_getTypeEncoding(method) : NULL;
                IMP current = method ? method_getImplementation(method) : NULL;
                if (encoding && strcmp(encoding, hooks[index].encoding) == 0 &&
                        current && current != hooks[index].replacement)
                    atomic_store_explicit(hooks[index].original, current, memory_order_release);
            }
            IMP original = HGInstallVerifiedHook(target, selector, hooks[index].encoding,
                hooks[index].replacement, @"playback");
            if (!original || original == hooks[index].replacement) continue;
            atomic_store_explicit(hooks[index].original, original, memory_order_release);
            preDecodeCaptured[index] = YES;
            if (class_getMethodImplementation(target, selector) == hooks[index].replacement) installed++;
        }
        HGRecordCapability(@"repair.playback.predecode", @{@"module": @"playback",
            @"state": installed == sizeof(hooks) / sizeof(hooks[0]) ? @"installed"
                : installed ? @"partial" : @"interposed",
            @"installed": @(installed), @"selected": @(sizeof(hooks) / sizeof(hooks[0])),
            @"reason": @"declines 1-frame pre-decode cap and restores pooled TTVideoEngine decode buffer when currentPlayer is nil"});
    }
}

static unsigned long long HGPlaybackBegin(void) {
    return atomic_fetch_add_explicit(&HGPlaybackInvocation, 1, memory_order_relaxed) + 1;
}

static void HGPlaybackTrace(NSString *event, NSString *phase,
        unsigned long long invocation, NSDictionary *fields) {
    if (!invocation) return;
    NSMutableDictionary *record = [fields mutableCopy];
    record[@"phase"] = phase;
    record[@"invocation"] = @(invocation);
    record[@"mainThread"] = @(NSThread.isMainThread);
    HGLog(event, record);
}

static void HGPlaybackCleanMode(id object, SEL selector, id context, BOOL cleanMode) {
    unsigned long long invocation = HGEnhancedDiagnostics() ? HGPlaybackBegin() : 0;
    NSDictionary *fields = invocation ? @{@"hasContext": @(context != nil), @"cleanMode": @(cleanMode)} : nil;
    HGPlaybackTrace(@"playback.clean-mode", @"enter", invocation, fields);
    IMP original = atomic_load_explicit(&HGOriginalCleanMode, memory_order_acquire);
    ((void (*)(id, SEL, id, BOOL))original)(object, selector, context, cleanMode);
    HGPlaybackTrace(@"playback.clean-mode", @"return", invocation, fields);
}

static void HGPlaybackModelUpdate(id object, SEL selector, id context) {
    static _Atomic uint64_t nextLog;
    unsigned long long invocation = HGEnhancedDiagnostics() && HGDiagnosticSample(&nextLog) ? HGPlaybackBegin() : 0;
    NSDictionary *fields = invocation ? @{@"hasContext": @(context != nil)} : nil;
    HGPlaybackTrace(@"playback.model-updated", @"enter", invocation, fields);
    IMP original = atomic_load_explicit(&HGOriginalPlayModelUpdate, memory_order_acquire);
    ((void (*)(id, SEL, id))original)(object, selector, context);
    HGPlaybackTrace(@"playback.model-updated", @"return", invocation, fields);
}

static void HGPlaybackStateUpdate(id object, SEL selector, id context) {
    static _Atomic uint64_t nextLog;
    unsigned long long invocation = HGEnhancedDiagnostics() && HGDiagnosticSample(&nextLog) ? HGPlaybackBegin() : 0;
    NSDictionary *fields = invocation ? @{@"hasContext": @(context != nil)} : nil;
    HGPlaybackTrace(@"playback.state-updated", @"enter", invocation, fields);
    IMP original = atomic_load_explicit(&HGOriginalPlayStateUpdate, memory_order_acquire);
    ((void (*)(id, SEL, id))original)(object, selector, context);
    HGPlaybackTrace(@"playback.state-updated", @"return", invocation, fields);
}

static BOOL HGPlaybackFinishGate(id object, SEL selector, id context, id model) {
    static _Atomic uint64_t nextLog;
    BOOL enabled = HGEnhancedDiagnostics();
    unsigned long long invocation = enabled && HGDiagnosticSample(&nextLog) ? HGPlaybackBegin() : 0;
    NSDictionary *fields = invocation ? @{@"hasContext": @(context != nil), @"hasModel": @(model != nil)} : nil;
    HGPlaybackTrace(@"playback.finish-gate", @"enter", invocation, fields);
    IMP original = atomic_load_explicit(&HGOriginalFinishGate, memory_order_acquire);
    BOOL result = ((BOOL (*)(id, SEL, id, id))original)(object, selector, context, model);
    if (invocation || (enabled && result)) {
        // An interception is exceptional. Preserve it even between samples.
        HGPlaybackTrace(@"playback.finish-gate", @"return", invocation ?: HGPlaybackBegin(),
            @{@"hasContext": @(context != nil), @"hasModel": @(model != nil), @"intercepted": @(result)});
    }
    return result;
}

typedef void (^HGLikeCompletion)(NSError *,id,BOOL);
typedef void (*HGLikeOperation)(id,SEL,id,id,HGLikeCompletion);

static BOOL HGIsEpisodeLikeCompletion(HGLikeCompletion completion) {
    if (!completion) return NO;
    // The same service can also consume recovery blocks. Only the verified
    // episode-like callback includes these exact response and error classes.
    struct BlockHeader { void *isa; int flags,reserved; void *invoke; const uintptr_t *descriptor; };
    const struct BlockHeader *block=(__bridge const void *)completion;
    if (!(block->flags & (1<<30))) return NO;
    NSUInteger index=(block->flags & (1<<25)) ? 4 : 2;
    const char *types=(const char *)block->descriptor[index];
    return types && !strcmp(types,"v28@?0@\"BDRpcError\"8@\"FQCRpcDoArticleActionResponse\"16B24");
}

static void HGObserveDiggOperation(id owner,SEL cmd,id data,id request,HGLikeCompletion completion) {
    HGLikeOperation original=(HGLikeOperation)atomic_load_explicit(&HGOriginalDiggOperation,memory_order_acquire);
    if (!HGEnhancedDiagnostics() || !HGIsEpisodeLikeCompletion(completion)) {
        original(owner,cmd,data,request,completion);
        return;
    }
    original(owner,cmd,data,request,^(NSError *error,id response,BOOL flag) {
        if (HGEnhancedDiagnostics()) {
            SEL code=sel_registerName("code");
            Method getCode=class_getInstanceMethod(object_getClass(response),code);
            BOOL hasCode=getCode && !strcmp(method_getTypeEncoding(getCode),"i16@0:8");
            int result=hasCode ? ((int(*)(id,SEL))objc_msgSend)(response,code) : 0;
            if ([error isKindOfClass:NSError.class] || result!=0) {
                NSMutableDictionary *fields=[NSMutableDictionary dictionary];
                if ([error isKindOfClass:NSError.class]) {
                    NSString *domain=error.domain;
                    if (domain.length && domain.length<=80 &&
                        [domain rangeOfString:@"^[A-Za-z][A-Za-z0-9._-]*$" options:NSRegularExpressionSearch].location!=NSNotFound)
                        fields[@"errorDomain"]=domain;
                    fields[@"errorCode"]=@(error.code);
                }
                if (hasCode) fields[@"responseCode"]=@(result);
                HGLog(@"playback.like-failed",fields);
            }
        }
        completion(error,response,flag);
    });
}

void HGInstallPlaybackDiagnostics(void) {
    static const struct {
        const char *className;
        const char *selector;
        const char *encoding;
        IMP replacement;
        _Atomic(IMP) *original;
    } methods[] = {
        {"SSShortVideoDiggManager", "performDiggWithData:request:completion:",
            "v40@0:8@?16@?24@?32", (IMP)HGObserveDiggOperation, &HGOriginalDiggOperation},
        {"SSBizVideoPatchAdService", "videoListVCContextDidChangeCleanMode:isCleanMode:",
            "v28@0:8@16B24", (IMP)HGPlaybackCleanMode, &HGOriginalCleanMode},
        {"SSBizVideoPatchAdService", "videoListVCContextPlayModelDidUpdatedNotifyAtScrollEnd:",
            "v24@0:8@16", (IMP)HGPlaybackModelUpdate, &HGOriginalPlayModelUpdate},
        {"SSBizVideoPauseAdManager", "videoListVCContextPlayStateDidUpdated:",
            "v24@0:8@16", (IMP)HGPlaybackStateUpdate, &HGOriginalPlayStateUpdate},
        {"SSBizVideoAdImpl", "shouldInterceptVideoFinishWithContext:videoDataModel:",
            "B32@0:8@16@24", (IMP)HGPlaybackFinishGate, &HGOriginalFinishGate},
    };
    static BOOL originalCaptured[sizeof(methods) / sizeof(methods[0])];
    NSUInteger installed = 0;
    for (NSUInteger index = 0; index < sizeof(methods) / sizeof(methods[0]); index++) {
        Class target = objc_lookUpClass(methods[index].className);
        SEL selector = sel_registerName(methods[index].selector);
        @synchronized (target) {
            // Seed an ABI-checked original before publishing the observer so
            // a concurrent host call cannot see a null function pointer. Once
            // installed, never seed from a later interposer that may call us.
            if (!originalCaptured[index]) {
                Method method = target ? class_getInstanceMethod(target, selector) : NULL;
                const char *encoding = method ? method_getTypeEncoding(method) : NULL;
                IMP current = method ? method_getImplementation(method) : NULL;
                if (encoding && strcmp(encoding, methods[index].encoding) == 0 &&
                        current && current != methods[index].replacement)
                    atomic_store_explicit(methods[index].original, current, memory_order_release);
            }
            // The shared installer preserves the first original on retries
            // and leaves later interposers in place.
            IMP original = HGInstallVerifiedHook(target, selector, methods[index].encoding,
                methods[index].replacement, @"playback");
            if (!original || original == methods[index].replacement) continue;
            atomic_store_explicit(methods[index].original, original, memory_order_release);
            originalCaptured[index] = YES;
            if (class_getMethodImplementation(target, selector) == methods[index].replacement) installed++;
        }
    }
    HGRecordCapability(@"module.playback", @{@"module": @"playback",
        @"state": installed == sizeof(methods)/sizeof(methods[0]) ? @"installed" : installed ? @"partial" : @"unavailable",
        @"installed": @(installed), @"selected": @(sizeof(methods)/sizeof(methods[0])),
        @"reason": @"observational host notifications; original behavior retained; events require enhanced diagnostics"});
    HGInstallPreDecodeBufferRepair();
}
