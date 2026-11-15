#import "RuntimeSupport.h"
#import "Diagnostics.h"
#import <dispatch/dispatch.h>
#import <objc/message.h>
#include <stdlib.h>
#include <string.h>

static const NSUInteger HGCapabilityLimit = 256;
static NSObject *HGRuntimeLock;
static NSMutableDictionary<NSString *, NSDictionary *> *HGCapabilities;
static NSMutableDictionary<NSString *, NSMutableArray *> *HGHookChains;
static NSUInteger HGCapabilitiesOmitted;

@interface HGHookInstallation : NSObject
@property(nonatomic, assign) IMP replacement;
@property(nonatomic, assign) IMP original;
@end
@implementation HGHookInstallation
@end

static NSObject *HGRuntimeStorage(void) {
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        HGRuntimeLock = [NSObject new];
        HGCapabilities = [NSMutableDictionary dictionary];
        HGHookChains = [NSMutableDictionary dictionary];
    });
    return HGRuntimeLock;
}

static NSString *HGBoundedRuntimeString(NSString *value) {
    return value.length <= 512 ? value : [value substringToIndex:512];
}

void HGRecordCapability(NSString *identifier, NSDictionary *fields) {
    if (![identifier isKindOfClass:NSString.class] || !identifier.length) return;
    NSString *key = HGBoundedRuntimeString(identifier);
    NSMutableDictionary *entry = [NSMutableDictionary dictionaryWithObject:key forKey:@"id"];
    // A flat, immutable schema keeps this inventory independent of live views,
    // caller-owned mutable collections, and expensive arbitrary object graphs.
    for (id name in fields) {
        if (![name isKindOfClass:NSString.class] || [name isEqual:@"id"]) continue;
        id value = fields[name];
        if ([value isKindOfClass:NSString.class]) entry[HGBoundedRuntimeString(name)] = [HGBoundedRuntimeString(value) copy];
        else if ([value isKindOfClass:NSNumber.class]) entry[HGBoundedRuntimeString(name)] = value;
        if (entry.count >= 16) break;
    }
    @synchronized (HGRuntimeStorage()) {
        if (!HGCapabilities[key] && HGCapabilities.count >= HGCapabilityLimit) {
            HGCapabilitiesOmitted++;
            return;
        }
        HGCapabilities[key] = entry.copy;
    }
}

NSDictionary *HGRuntimeSnapshot(void) {
    @synchronized (HGRuntimeStorage()) {
        NSMutableArray *entries = [NSMutableArray arrayWithCapacity:HGCapabilities.count];
        NSMutableDictionary *summary = [NSMutableDictionary dictionary];
        for (NSString *state in @[@"installed", @"disabled", @"missing-class", @"missing-method",
                @"abi-mismatch", @"interposed", @"partial", @"unavailable"])
            summary[state] = @0;
        NSArray<NSString *> *keys = [HGCapabilities.allKeys sortedArrayUsingSelector:@selector(compare:)];
        for (NSString *key in keys) {
            NSDictionary *entry = HGCapabilities[key];
            [entries addObject:entry];
            NSString *state = entry[@"state"];
            if ([state isKindOfClass:NSString.class]) summary[state] = @([summary[state] unsignedIntegerValue] + 1);
        }
        return @{@"schema": @1, @"capacity": @(HGCapabilityLimit), @"omitted": @(HGCapabilitiesOmitted),
            @"summary": summary.copy, @"entries": entries.copy};
    }
}

static NSString *HGHookIdentifier(NSString *module, Class target, SEL selector) {
    return [NSString stringWithFormat:@"hook.%@.%@.%@.%@", module,
        class_isMetaClass(target) ? @"class" : @"instance",
        NSStringFromClass(target) ?: @"missing", selector ? NSStringFromSelector(selector) : @"missing"];
}

static Class HGMethodOwner(Class target, SEL selector) {
    for (Class type = target; type; type = class_getSuperclass(type)) {
        unsigned int count = 0;
        Method *methods = class_copyMethodList(type, &count);
        BOOL found = NO;
        for (unsigned int i = 0; i < count; i++) {
            if (method_getName(methods[i]) == selector) { found = YES; break; }
        }
        free(methods);
        if (found) return type;
    }
    return Nil;
}

IMP HGInstallVerifiedHook(Class target, SEL selector, const char *encoding,
        IMP replacement, NSString *module) {
    if (![module isKindOfClass:NSString.class] || !module.length) module = @"runtime";
    NSString *identifier = HGHookIdentifier(module, target, selector);
    NSMutableDictionary *entry = [@{@"module": module,
        @"class": NSStringFromClass(target) ?: @"missing",
        @"selector": selector ? NSStringFromSelector(selector) : @"missing",
        @"kind": class_isMetaClass(target) ? @"class" : @"instance",
        @"expected": encoding ? @(encoding) : @"missing"} mutableCopy];
    @synchronized (HGRuntimeStorage()) {
        Method method = target && selector ? class_getInstanceMethod(target, selector) : NULL;
        const char *actual = method ? method_getTypeEncoding(method) : NULL;
        entry[@"actual"] = actual ? @(actual) : @"missing";
        if (!target || !selector || !encoding || !encoding[0] || !replacement || !method ||
                !actual || strcmp(actual, encoding) != 0) {
            entry[@"state"] = !target ? @"missing-class" :
                (!selector || !encoding || !encoding[0] || !replacement) ? @"unavailable" :
                !method ? @"missing-method" : @"abi-mismatch";
            entry[@"reason"] = @"hook preflight rejected; original method retained";
            HGRecordCapability(identifier, entry);
            HGLog(@"hook.skipped", entry);
            return NULL;
        }

        // A slot is shared across modules because several local observers may
        // intentionally wrap the same host lifecycle method. Each distinct
        // replacement retains its own original; repeated installs are no-ops.
        NSString *slot = HGHookIdentifier(@"slot", target, selector);
        NSMutableArray<HGHookInstallation *> *chain = HGHookChains[slot];
        IMP current = method_getImplementation(method);
        BOOL currentIsOurs = NO;
        HGHookInstallation *previous = nil;
        for (HGHookInstallation *record in chain) {
            if (record.replacement == current) currentIsOurs = YES;
            if (record.replacement == replacement) previous = record;
        }
        if (previous) {
            entry[@"state"] = currentIsOurs ? @"installed" : @"interposed";
            entry[@"reason"] = currentIsOurs ? @"already installed; original preserved" :
                @"a later replacement is present; it was left in place";
            HGRecordCapability(identifier, entry);
            return previous.original;
        }
        if (!current || current == replacement) {
            entry[@"state"] = @"unavailable";
            entry[@"reason"] = @"no safe distinct original implementation";
            HGRecordCapability(identifier, entry);
            return NULL;
        }

        Class owner = HGMethodOwner(target, selector);
        entry[@"owner"] = NSStringFromClass(owner) ?: @"missing";
        // class_replaceMethod creates a local override for inherited methods.
        // Mutating the inherited Method itself would also change its siblings.
        class_replaceMethod(target, selector, replacement, encoding);
        HGHookInstallation *record = [HGHookInstallation new];
        record.replacement = replacement;
        record.original = current;
        if (!chain) {
            chain = [NSMutableArray array];
            HGHookChains[slot] = chain;
        }
        [chain addObject:record];
        BOOL active = class_getMethodImplementation(target, selector) == replacement;
        entry[@"state"] = active ? @"installed" : @"interposed";
        entry[@"reason"] = active ? @"signature verified; replacement scoped to target class" :
            @"another replacement became current during installation";
        HGRecordCapability(identifier, entry);
        HGLog(@"hook.installed", entry);
        return current;
    }
}

id HGReadObjectGetter(id object, const char *name) {
    if (!object || !name || !name[0]) return nil;
    SEL selector = sel_registerName(name);
    Method method = class_getInstanceMethod(object_getClass(object), selector);
    const char *encoding = method ? method_getTypeEncoding(method) : NULL;
    if (!encoding || strcmp(encoding, "@16@0:8") != 0) return nil;
    return ((id (*)(id, SEL))objc_msgSend)(object, selector);
}
