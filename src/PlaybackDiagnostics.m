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
}
