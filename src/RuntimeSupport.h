#pragma once

#import <Foundation/Foundation.h>
#import <objc/runtime.h>

// The first original implementation is retained for each replacement. Repeating
// an installation never makes that replacement its own original or displaces a
// hook installed later by another component.
IMP HGInstallVerifiedHook(Class target, SEL selector, const char *encoding,
    IMP replacement, NSString *module);

// Read only an actual, object-returning, zero-argument Objective-C method.
// Forwarding-only and incompatible selectors are left untouched.
id HGReadObjectGetter(id object, const char *name);

// Structural plugin metadata only: no user text, request data or addresses.
// Records are bounded, copied, and safe to retrieve from diagnostic queues.
void HGRecordCapability(NSString *identifier, NSDictionary *fields);
NSDictionary *HGRuntimeSnapshot(void);

// At most two startup passes. Modules install independently; the return value
// describes whether both settings and tab-bar entry points are available.
BOOL HGInstallRuntimeModules(void);
