#pragma once
#import <Foundation/Foundation.h>
#include <stdatomic.h>
#include <stdint.h>
#include <time.h>

// Each high-frequency observer owns its next deadline. Sampling before
// building a payload saves work on both the caller and journal threads.
static inline BOOL HGDiagnosticSampleAt(_Atomic uint64_t *deadline, uint64_t now) {
    uint64_t expected = atomic_load_explicit(deadline, memory_order_relaxed);
    return now >= expected && atomic_compare_exchange_strong_explicit(deadline, &expected,
        now + 2000000000ULL, memory_order_relaxed, memory_order_relaxed);
}

static inline BOOL HGDiagnosticSample(_Atomic uint64_t *deadline) {
    return HGDiagnosticSampleAt(deadline, clock_gettime_nsec_np(CLOCK_UPTIME_RAW));
}

static inline double HGDiagnosticCPUPercent(double elapsed, double cpuDelta) {
    return elapsed > 0 && cpuDelta >= 0 ? 100.0 * cpuDelta / elapsed : 0;
}

// Fields must be plugin-generated technical metadata. Never pass user
// content, URLs, request headers or bodies, credentials or identifiers.
void HGDiagnosticsStart(NSURL *directory, NSDictionary *environment);
void HGDiagnosticsSetForeground(BOOL foreground);
void HGDiagnosticsEnd(void);
void HGLog(NSString *event, NSDictionary *fields);
void HGLogCritical(NSString *event, NSDictionary *fields);
void HGLoadStage(NSString *stage);
BOOL HGEnhancedDiagnostics(void);
void HGSetEnhancedDiagnostics(BOOL enabled);
void HGStoreMaskScene(NSDictionary *scene);

typedef NS_ENUM(NSUInteger, HGDiagnosticScope) {
    HGDiagnosticScopeAll,
    HGDiagnosticScopeSession,
    HGDiagnosticScopePreviousUnexpected,
};

void HGListDiagnosticSessions(void (^completion)(NSArray<NSDictionary *> *, NSError *));
void HGReadDiagnosticSession(NSString *identifier, void (^completion)(NSArray<NSDictionary *> *, NSError *));
void HGExportDiagnosticSessions(HGDiagnosticScope scope, NSString *identifier, NSDictionary *snapshot,
    void (^completion)(NSURL *, NSError *));
void HGExportDiagnostics(NSDictionary *snapshot, void (^completion)(NSURL *, NSError *));
void HGClearDiagnostics(void (^completion)(NSError *));
