#import "Diagnostics.h"
#import <dispatch/dispatch.h>
#include <math.h>
#include <sys/stat.h>
#include <unistd.h>

static NSUInteger checks, failures;

static void Check(BOOL passed, NSString *name) {
    checks++;
    if (!passed) { failures++; NSLog(@"FAIL: %@", name); }
}

static void Wait(BOOL (^complete)(void)) {
    NSDate *deadline = [NSDate dateWithTimeIntervalSinceNow:15];
    while (!complete() && deadline.timeIntervalSinceNow > 0)
        [NSRunLoop.currentRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
    Check(complete(), @"asynchronous operation completed");
}

static NSDictionary *Export(HGDiagnosticScope scope, NSString *identifier, NSURL **result) {
    __block BOOL done = NO;
    __block NSURL *url;
    __block NSError *error;
    HGExportDiagnosticSessions(scope, identifier, @{@"nativeGlassConfigured": @YES, @"cookie": @"private-cookie"},
        ^(NSURL *file, NSError *err) {
            Check(NSThread.isMainThread, @"completion is on main");
            url = file; error = err; done = YES;
        });
    Wait(^BOOL{ return done; });
    Check(url != nil && error == nil, error.description ?: @"valid export");
    if (result) *result = url;
    NSData *data = url ? [NSData dataWithContentsOfURL:url] : nil;
    id report = data ? [NSJSONSerialization JSONObjectWithData:data options:0 error:nil] : nil;
    Check([report isKindOfClass:NSDictionary.class], @"export is complete JSON");
    return report;
}

static NSArray *Sessions(void) {
    __block NSArray *rows;
    HGListDiagnosticSessions(^(NSArray<NSDictionary *> *sessions, NSError *error) { rows = sessions; });
    Wait(^BOOL{ return rows != nil; });
    return rows;
}

static NSDictionary *Event(NSDictionary *report, NSString *name) {
    for (NSDictionary *row in report[@"events"]) if ([row[@"event"] isEqual:name]) return row;
    return nil;
}

static void SaveJSON(id object, NSURL *target) {
    NSData *data = [NSJSONSerialization dataWithJSONObject:object options:0 error:nil];
    Check([data writeToURL:target atomically:YES], @"write fixture");
}

static NSString *Seed(NSURL *root, NSTimeInterval age, NSUInteger segments) {
    NSString *identifier = NSUUID.UUID.UUIDString;
    NSURL *directory = [root URLByAppendingPathComponent:identifier];
    NSFileManager *manager = NSFileManager.defaultManager;
    Check([manager createDirectoryAtURL:directory withIntermediateDirectories:YES attributes:nil error:nil], @"create capacity fixture");
    NSDate *time = [NSDate dateWithTimeIntervalSinceNow:-age];
    SaveJSON(@{@"session": identifier, @"startedAt": @(time.timeIntervalSince1970), @"pluginVersion": @"fixture"},
        [directory URLByAppendingPathComponent:@"session.json"]);
    for (NSUInteger i = 1; i <= segments; i++) {
        NSURL *url = [directory URLByAppendingPathComponent:[NSString stringWithFormat:@"%08lu.ndjson", (unsigned long)i]];
        Check([manager createFileAtPath:url.path contents:nil attributes:nil] &&
            truncate(url.fileSystemRepresentation, 1024 * 1024) == 0, @"seed segment allocation");
        [manager setAttributes:@{NSFileModificationDate: time} ofItemAtPath:url.path error:nil];
    }
    return identifier;
}

static void CheckCapacity(NSURL *root) {
    NSString *expired = Seed(root, 3700, 1);
    NSString *old = Seed(root, 3000, 25);
    NSString *new = Seed(root, 2400, 25);
    NSString *latest = Seed(root, 1200, 1);
    HGDiagnosticsStart(root, @{});
    HGLog(@"capacity.after-start", nil);
    NSDictionary *report = Export(HGDiagnosticScopeSession, [Sessions() lastObject][@"session"], NULL);
    NSArray *ids = [Sessions() valueForKey:@"session"];
    Check(![ids containsObject:expired] && ![ids containsObject:old] &&
        [ids containsObject:new] && [ids containsObject:latest], @"age and capacity remove the oldest sessions");
    Check(Event(report, @"capacity.after-start") != nil, @"current session remains writable after pruning");
    NSUInteger bytes = 0;
    for (NSURL *url in [NSFileManager.defaultManager enumeratorAtURL:root includingPropertiesForKeys:@[NSURLFileSizeKey]
        options:0 errorHandler:nil]) {
        if ([url.pathComponents containsObject:@"Exports"]) continue;
        NSNumber *size = nil;
        [url getResourceValue:&size forKey:NSURLFileSizeKey error:nil];
        bytes += size.unsignedIntegerValue;
    }
    Check(bytes < 50 * 1024 * 1024, @"retained logs remain under the logical byte limit");
}

static void CheckEvents(NSURL *root) {
    Check(HGDiagnosticCPUPercent(8.0, 2.0) == 25.0 && HGDiagnosticCPUPercent(4.0, 6.0) == 150.0,
        @"the process CPU sample uses one full core as 100 percent and permits concurrent cores");
    Check(HGDiagnosticCPUPercent(8.0, 0.0) == 0.0 && HGDiagnosticCPUPercent(0.0, 1.0) == 0.0 &&
        HGDiagnosticCPUPercent(-1.0, 1.0) == 0.0 && HGDiagnosticCPUPercent(8.0, -1.0) == 0.0,
        @"idle CPU and invalid sample intervals do not produce a negative or undefined result");
    static _Atomic uint64_t repeated;
    static _Atomic uint64_t independent;
    static atomic_uint accepted;
    uint64_t now = 1000000000ULL;
    dispatch_apply(128, dispatch_get_global_queue(QOS_CLASS_DEFAULT, 0), ^(size_t index) {
        if (HGDiagnosticSampleAt(&repeated, now))
            atomic_fetch_add_explicit(&accepted, 1, memory_order_relaxed);
    });
    Check(atomic_load_explicit(&accepted, memory_order_relaxed) == 1 &&
        HGDiagnosticSampleAt(&independent, now) &&
        !HGDiagnosticSampleAt(&repeated, now + 1999999999ULL) &&
        HGDiagnosticSampleAt(&repeated, now + 2000000000ULL),
        @"high-frequency diagnostics allow one sample per two-second interval and keep independent events");
    NSString *legacy = NSUUID.UUID.UUIDString;
    SaveJSON(@[
        @{@"session": legacy, @"time": @([NSDate dateWithTimeIntervalSinceNow:-1800].timeIntervalSince1970),
          @"event": @"legacy.retained", @"fields": @{@"accessToken": @"diagnostic-private-token", @"class": @"Fixture"}},
        @{@"session": legacy, @"time": @([NSDate dateWithTimeIntervalSinceNow:-3610].timeIntervalSince1970),
          @"event": @"legacy.expired", @"fields": @{}},
        @{@"session": legacy, @"time": @(NSDate.date.timeIntervalSince1970), @"event": @"legacy.bad", @"fields": @[]}
    ], [root URLByAppendingPathComponent:@"events.json"]);
    [NSUserDefaults.standardUserDefaults setBool:NO forKey:@"Honggo.enhancedDiagnostics"];
    HGDiagnosticsStart(root, @{@"hostBuild": @"7.4.3.33"});
    Check(HGEnhancedDiagnostics(), @"enhanced diagnostics starts enabled even after an earlier version stored off");
    [NSUserDefaults.standardUserDefaults removeObjectForKey:@"Honggo.enhancedDiagnostics"];
    HGLoadStage(@"entry");
    HGLog(@"nil.fields", nil);
    HGLog(@"privacy.fixture", @{@"cookie": @"diagnostic-private-cookie", @"手机号": @13800138000,
        @"accountId": @495202, @"reason": @"https://example.test/private", @"selector": @"safeAction:"});
    HGLog(@"navigation.dock", @{@"tab": @"home", @"phase": @"enter", @"mode": @"glass"});
    HGLog(@"playback.preview-entry", @{@"reason": @"unmapped"});
    HGLog(@"appearance.dock-renderer", @{@"mode": @"uikit-tabbar", @"material": @"factory-glass",
        @"requestedMaterial": @"clear", @"platter": @"UIKit._UITabBarItemPlatterView", @"interfaceStyle": @"light"});
    NSURL *immutable = nil;
    NSDictionary *report = Export(HGDiagnosticScopeAll, nil, &immutable);
    NSArray *sessions = report[@"sessions"];
    Check(sessions.count == 2 && [[sessions.lastObject objectForKey:@"previousSession"] isEqual:legacy],
        @"valid legacy events migrate into their original session");
    Check(Event(report, @"legacy.retained") && !Event(report, @"legacy.expired") && !Event(report, @"legacy.bad"),
        @"migration retains old valid records and rejects expired or malformed input");
    Check(Event(report, @"nil.fields") && Event(report, @"startup.stage"), @"startup stages and nil payloads use the same journal");
    Check([Event(report, @"navigation.dock")[@"fields"][@"mode"] isEqual:@"glass"],
        @"the Dock interaction mode survives journal filtering and export");
    Check([Event(report, @"playback.preview-entry")[@"fields"][@"reason"] isEqual:@"unmapped"],
        @"the technical reason for a preview route survives export");
    NSDictionary *dock = Event(report, @"appearance.dock-renderer")[@"fields"];
    Check([dock[@"material"] isEqual:@"factory-glass"] && [dock[@"requestedMaterial"] isEqual:@"clear"] &&
        [dock[@"platter"] isEqual:@"UIKit._UITabBarItemPlatterView"] && [dock[@"interfaceStyle"] isEqual:@"light"],
        @"the export preserves the effective Dock theme, provider and both material values");
    Check([sessions.lastObject objectForKey:@"device"] && [sessions.lastObject objectForKey:@"appVersion"] &&
        [sessions.lastObject objectForKey:@"pluginVersion"], @"current session contains environment metadata");
    NSData *saved = [NSData dataWithContentsOfURL:immutable];
    NSString *serialized = [[NSString alloc] initWithData:saved encoding:NSUTF8StringEncoding];
    Check(![serialized containsString:@"diagnostic-private"] && ![serialized containsString:@"13800138000"] &&
        Event(report, @"privacy.fixture")[@"fields"][@"accountId"] == nil && ![serialized containsString:@"example.test"] &&
        ![serialized containsString:@"private-cookie"] && [serialized containsString:@"safeAction:"],
        @"new events, old events and the structural snapshot protect sensitive fields");
    dispatch_apply(6000, dispatch_get_global_queue(QOS_CLASS_DEFAULT, 0), ^(size_t i) {
        HGLog(@"stress", @{@"index": @(i)});
    });
    NSDictionary *stress = Export(HGDiagnosticScopeAll, nil, NULL);
    Check([stress[@"droppedRecords"] unsignedIntegerValue] > 0, @"bounded producer queue reports dropped records");
    Check([[NSData dataWithContentsOfURL:immutable] isEqual:saved], @"earlier exports remain immutable");
    for (NSUInteger i = 0; i < 12; i++) HGStoreMaskScene(@{@"reason": @"user.screenshot", @"sequence": @(i)});
    NSDictionary *scene = Export(HGDiagnosticScopeAll, nil, NULL);
    Check([scene[@"maskScenes"] count] > 0 && [scene[@"maskScenes"] count] <= 10 &&
        Event(scene, @"scene.capture"), @"bounded structural scenes are recorded and remain exportable");
    NSArray *nodes = @[@{@"class": @"UIView", @"frame": @"{{0,0},{32,32}}"}];
    HGStoreMaskScene(@{@"reason": @"did.appear", @"nodes": nodes, @"captureMs": @2});
    HGStoreMaskScene(@{@"reason": @"user.screenshot", @"nodes": nodes});
    NSDictionary *compact = Export(HGDiagnosticScopeAll, nil, NULL);
    NSDictionary *automatic = nil, *manual = nil;
    for (NSDictionary *row in compact[@"events"]) {
        NSDictionary *item = row[@"fields"][@"scene"];
        if ([item[@"reason"] isEqual:@"did.appear"]) automatic = item;
        if ([item[@"reason"] isEqual:@"user.screenshot"] && item[@"nodes"]) manual = item;
    }
    NSPredicate *preview = [NSPredicate predicateWithFormat:@"reason == %@", @"did.appear"];
    NSDictionary *recent = [[compact[@"maskScenes"] filteredArrayUsingPredicate:preview] lastObject];
    Check([automatic[@"summaryOnly"] boolValue] && [automatic[@"nodeCount"] isEqual:@1] &&
        [automatic[@"captureMs"] isEqual:@2] && !automatic[@"nodes"] && [recent[@"nodes"] isEqual:nodes],
        @"automatic captures retain timing in the journal and recent detail in the bounded scene list");
    Check([manual[@"nodes"] isEqual:nodes] && !manual[@"summaryOnly"],
        @"manual screenshots remain detailed in the exported journal");
    __block BOOL cleared = NO;
    HGClearDiagnostics(^(NSError *error) { Check(!error, @"clear completes"); cleared = YES; });
    Wait(^BOOL{ return cleared; });
    NSDictionary *empty = Export(HGDiagnosticScopeAll, nil, NULL);
    Check([empty[@"events"] count] == 0 && [empty[@"sessions"] count] == 1 && [empty[@"maskScenes"] count] == 0 &&
        ![NSFileManager.defaultManager fileExistsAtPath:immutable.path], @"clear removes history, scenes and exported copies");
}

static void SaveRows(NSArray *rows, NSURL *url) {
    NSMutableData *data = [NSMutableData data];
    for (NSDictionary *row in rows) {
        [data appendData:[NSJSONSerialization dataWithJSONObject:row options:0 error:nil]];
        [data appendBytes:"\n" length:1];
    }
    Check([data writeToURL:url atomically:YES], @"write session journal fixture");
}

static void CheckRetention(NSURL *root) {
    NSString *expired = Seed(root, 3700, 1);
    NSString *longSession = Seed(root, 7200, 0);
    NSURL *directory = [root URLByAppendingPathComponent:longSession];
    NSURL *mixed = [directory URLByAppendingPathComponent:@"00000001.ndjson"];
    NSURL *touched = [directory URLByAppendingPathComponent:@"00000002.ndjson"];
    NSTimeInterval now = NSDate.date.timeIntervalSince1970;
    NSDictionary *old = @{@"session": longSession, @"time": @(now - 3610), @"event": @"fixture.expired", @"fields": @{}};
    NSDictionary *fresh = @{@"session": longSession, @"time": @(now - 3590), @"event": @"fixture.retained", @"fields": @{}};
    NSDictionary *watchdog = @{@"session": longSession, @"time": @(now - 20), @"event": @"watchdog.severe",
        @"fields": @{}, @"recentEvents": @[old, fresh]};
    NSDictionary *legacyScene = @{@"session": longSession, @"time": @(now - 15), @"event": @"scene.capture",
        @"fields": @{@"scene": @{@"reason": @"before.leave", @"nodes": @[@{@"class": @"UIView"}], @"roles": @{@"page": @1}}}};
    SaveRows(@[old, fresh, watchdog, legacyScene], mixed);
    // Even a recently modified old journal must be checked by event time.
    SaveRows(@[old], touched);
    HGDiagnosticsStart(root, @{});
    HGLog(@"fixture.current", nil);

    NSDictionary *all = Export(HGDiagnosticScopeAll, nil, NULL);
    NSDictionary *one = Export(HGDiagnosticScopePreviousUnexpected, nil, NULL);
    Check([all[@"retentionSeconds"] unsignedIntegerValue] == 3600 &&
        Event(all, @"fixture.current") && Event(all, @"fixture.retained") && !Event(all, @"fixture.expired"),
        @"all-session export uses the rolling one-hour event window");
    Check([one[@"sessions"] count] == 1 && [one[@"sessions"][0][@"session"] isEqual:longSession] &&
        Event(one, @"fixture.retained") && !Event(one, @"fixture.expired") &&
        [[Event(one, @"watchdog.severe")[@"recentEvents"] valueForKey:@"event"] isEqual:@[@"fixture.retained"]],
        @"an old-start session retains its recent watchdog and in-window context");
    NSDictionary *historicalScene = Event(one, @"scene.capture")[@"fields"][@"scene"];
    Check([historicalScene[@"summaryOnly"] boolValue] && [historicalScene[@"nodeCount"] isEqual:@1] &&
        [historicalScene[@"roles"] isEqual:@{@"page": @1}] && !historicalScene[@"nodes"],
        @"previous beta journals export routine scene trees as compact summaries");
    NSFileManager *manager = NSFileManager.defaultManager;
    NSString *file = [NSString stringWithContentsOfURL:mixed encoding:NSUTF8StringEncoding error:nil];
    Check([file containsString:@"fixture.retained"] && ![file containsString:@"fixture.expired"] &&
        ![manager fileExistsAtPath:touched.path] &&
        ![manager fileExistsAtPath:[root URLByAppendingPathComponent:expired].path],
        @"retention compacts a mixed segment and deletes wholly expired files from disk");
}

static void CheckRestart(void) {
    NSArray *sessions = Sessions();
    NSDictionary *lastAbnormal = sessions.count >= 3 ? sessions[sessions.count - 2] : @{};
    Check(sessions.count == 3 && [sessions.firstObject[@"state"] isEqual:@"ended"] &&
        [lastAbnormal[@"state"] isEqual:@"unexpected"] && [sessions.lastObject[@"state"] isEqual:@"current"],
        @"a normal end and a forced end remain distinguishable after relaunch");
    Check([lastAbnormal[@"endStatus"] isEqual:@"Previous session ended unexpectedly (reason unknown)"],
        @"a forced end keeps its cause unknown");
    NSDictionary *all = Export(HGDiagnosticScopeAll, nil, NULL);
    NSDictionary *one = Export(HGDiagnosticScopePreviousUnexpected, nil, NULL);
    Check([one[@"sessions"] count] == 1 && [one[@"sessions"] isEqual:@[lastAbnormal]] &&
        Event(all, @"fixture.normal") && !Event(one, @"fixture.normal"), @"whole and abnormal-session exports select the right history");
    NSDictionary *stall = Event(one, @"watchdog.stall");
    NSDictionary *severe = Event(one, @"watchdog.severe");
    Check([stall[@"fields"][@"durationSeconds"] doubleValue] >= 2 && [severe[@"fields"][@"durationSeconds"] doubleValue] >= 5 &&
        [severe[@"fields"][@"mainThreadStack"] isEqual:@"unavailable"] &&
        [[severe[@"recentEvents"] valueForKey:@"event"] containsObject:@"fixture.before-stall"] &&
        [severe[@"session"] isEqual:lastAbnormal[@"session"]], @"a killed process leaves both stall levels and pre-stall context");
    NSDictionary *specified = Export(HGDiagnosticScopeSession, lastAbnormal[@"session"], NULL);
    Check([specified[@"events"] isEqual:one[@"events"]], @"specified session export matches the selected historical session");
    __block NSArray *tail;
    HGReadDiagnosticSession(lastAbnormal[@"session"], ^(NSArray<NSDictionary *> *rows, NSError *error) { tail = rows; });
    Wait(^BOOL{ return tail != nil; });
    Check([[tail valueForKey:@"event"] containsObject:@"watchdog.severe"], @"historical events are available in the viewer");
}

int main(int argc, const char **argv) {
    @autoreleasepool {
        if (argc != 3) return 2;
        NSString *mode = @(argv[1]);
        NSURL *root = [NSURL fileURLWithPath:@(argv[2]) isDirectory:YES];
        if ([mode isEqual:@"--capacity"]) CheckCapacity(root);
        else if ([mode isEqual:@"--retention"]) CheckRetention(root);
        else if ([mode isEqual:@"--events"]) CheckEvents(root);
        else {
            HGDiagnosticsStart(root, @{});
            if ([mode isEqual:@"--normal"]) {
                HGLog(@"fixture.normal", nil);
                HGDiagnosticsEnd();
                NSDictionary *report = Export(HGDiagnosticScopeAll, nil, NULL);
                Check(Event(report, @"session.ended") != nil, @"end event persisted before orderly process exit");
            } else if ([mode isEqual:@"--stall"]) {
                HGLog(@"fixture.before-stall", @{@"selector": @"fixtureAction:"});
                Export(HGDiagnosticScopeAll, nil, NULL);
                // The parent sends SIGKILL after the severe event is on disk.
                sleep(30);
                return 2;
            } else if ([mode isEqual:@"--recover"]) {
                HGLog(@"fixture.before-recovery", nil);
                Export(HGDiagnosticScopeAll, nil, NULL);
                sleep(6);
                [NSRunLoop.currentRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:1]];
                NSDictionary *report = Export(HGDiagnosticScopeAll, nil, NULL);
                Check([Event(report, @"watchdog.recovered")[@"fields"][@"durationSeconds"] doubleValue] >= 5,
                    @"recovered stall includes its measured duration");
                NSDictionary *performance = Event(report, @"device.performance")[@"fields"];
                NSNumber *cpu = performance[@"processCPUPercent"], *interval = performance[@"intervalSeconds"];
                NSNumber *thermal = performance[@"thermalState"];
                Check([cpu isKindOfClass:NSNumber.class] && isfinite(cpu.doubleValue) && cpu.doubleValue >= 0 &&
                    [interval isKindOfClass:NSNumber.class] && interval.doubleValue > 0,
                    @"foreground CPU sampling reaches the exported journal even while the main thread is stalled");
                NSNumber *startupThermal = Event(report, @"device.thermal")[@"fields"][@"thermalState"];
                Check([startupThermal isKindOfClass:NSNumber.class] && [thermal isKindOfClass:NSNumber.class] &&
                    thermal.integerValue >= NSProcessInfoThermalStateNominal && thermal.integerValue <= NSProcessInfoThermalStateCritical,
                    @"the journal includes the system thermal tier at startup and alongside foreground CPU");
            } else if ([mode isEqual:@"--restart"]) CheckRestart();
            else if ([mode isEqual:@"--write-failure"]) {
                HGLogCritical(@"fixture.fallback", nil);
                NSDictionary *report = Export(HGDiagnosticScopeAll, nil, NULL);
                Check(report[@"storageError"] && Event(report, @"fixture.fallback"), @"storage failure preserves a bounded memory tail");
            } else return 2;
        }
        NSLog(@"Diagnostics %@: %lu checks, %lu failures", mode, (unsigned long)checks, (unsigned long)failures);
        return failures ? 1 : 0;
    }
}
