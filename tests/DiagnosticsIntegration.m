#import "Diagnostics.h"
#import <dispatch/dispatch.h>
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
    NSString *expired = Seed(root, 8 * 86400, 1);
    NSString *old = Seed(root, 3 * 86400, 25);
    NSString *new = Seed(root, 2 * 86400, 25);
    NSString *latest = Seed(root, 86400, 1);
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
    NSString *legacy = NSUUID.UUID.UUIDString;
    SaveJSON(@[
        @{@"session": legacy, @"time": @([NSDate dateWithTimeIntervalSinceNow:-7200].timeIntervalSince1970),
          @"event": @"legacy.retained", @"fields": @{@"accessToken": @"diagnostic-private-token", @"class": @"Fixture"}},
        @{@"session": legacy, @"time": @([NSDate dateWithTimeIntervalSinceNow:-8*86400].timeIntervalSince1970),
          @"event": @"legacy.expired", @"fields": @{}},
        @{@"session": legacy, @"time": @(NSDate.date.timeIntervalSince1970), @"event": @"legacy.bad", @"fields": @[]}
    ], [root URLByAppendingPathComponent:@"events.json"]);
    HGDiagnosticsStart(root, @{@"hostBuild": @"7.4.3.33"});
    HGLoadStage(@"entry");
    HGLog(@"nil.fields", nil);
    HGLog(@"privacy.fixture", @{@"cookie": @"diagnostic-private-cookie", @"手机号": @13800138000,
        @"accountId": @495202, @"reason": @"https://example.test/private", @"selector": @"safeAction:"});
    HGSetEnhancedDiagnostics(YES);
    Check(HGEnhancedDiagnostics(), @"enhanced mode changes immediately");
    NSURL *immutable = nil;
    NSDictionary *report = Export(HGDiagnosticScopeAll, nil, &immutable);
    NSArray *sessions = report[@"sessions"];
    Check(sessions.count == 2 && [[sessions.lastObject objectForKey:@"previousSession"] isEqual:legacy],
        @"valid legacy events migrate into their original session");
    Check(Event(report, @"legacy.retained") && !Event(report, @"legacy.expired") && !Event(report, @"legacy.bad"),
        @"migration retains old valid records and rejects expired or malformed input");
    Check(Event(report, @"nil.fields") && Event(report, @"startup.stage"), @"startup stages and nil payloads use the same journal");
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
    __block BOOL cleared = NO;
    HGClearDiagnostics(^(NSError *error) { Check(!error, @"clear completes"); cleared = YES; });
    Wait(^BOOL{ return cleared; });
    NSDictionary *empty = Export(HGDiagnosticScopeAll, nil, NULL);
    Check([empty[@"events"] count] == 0 && [empty[@"sessions"] count] == 1 && [empty[@"maskScenes"] count] == 0 &&
        ![NSFileManager.defaultManager fileExistsAtPath:immutable.path], @"clear removes history, scenes and exported copies");
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
