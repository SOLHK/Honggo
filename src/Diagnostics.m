#import "Diagnostics.h"
#import "HonggoVersion.h"
#import <dispatch/dispatch.h>

static dispatch_queue_t HGLogQueue;
static NSObject *HGLogLock;
static NSURL *HGLogDirectory;
static NSDictionary *HGEnvironment;
static NSString *HGSession;
static NSMutableArray<NSDictionary *> *HGEvents;
static NSUInteger HGPending, HGDropped, HGStoredBytes;
static BOOL HGDrainScheduled, HGEnhanced;
static NSMutableArray<NSDictionary *> *HGIncoming;
static NSDictionary *HGPreviousStartup;
static NSUInteger HGCapacityPruned;
static NSError *HGStorageError;
static const NSUInteger HGMaxBytes = 2 * 1024 * 1024;
static NSMutableArray<NSDictionary *> *HGMaskScenes;
static NSUInteger HGMaskBytes, HGMaskPruned, HGMaskRejected, HGMaskPending;
void HGStoreMaskScene(NSDictionary *scene) {
    if (!HGLogQueue) return;
    @synchronized (HGLogLock) {
        if (HGMaskPending >= 4) { HGMaskRejected++; return; }
        HGMaskPending++;
    }
    dispatch_async(HGLogQueue, ^{
        NSDictionary *stored = scene;
        NSData *encoded = [NSJSONSerialization dataWithJSONObject:stored options:0 error:nil];
        if (encoded.length > 192 * 1024 && [scene[@"nodes"] isKindOfClass:NSArray.class]) {
            NSMutableDictionary *trimmed = scene.mutableCopy;
            NSMutableArray *nodes = [scene[@"nodes"] mutableCopy];
            trimmed[@"nodes"] = nodes;
            trimmed[@"truncated"] = @YES;
            trimmed[@"byteLimited"] = @YES;
            while (nodes.count && encoded.length > 192 * 1024) {
                [nodes removeObjectsInRange:NSMakeRange(nodes.count - MIN(nodes.count, 8), MIN(nodes.count, 8))];
                encoded = [NSJSONSerialization dataWithJSONObject:trimmed options:0 error:nil];
            }
            stored = trimmed;
        }
        if (encoded && encoded.length <= 192 * 1024) {
            if (!HGMaskScenes) HGMaskScenes = [NSMutableArray array];
            [HGMaskScenes addObject:@{@"scene": stored, @"bytes": @(encoded.length)}];
            HGMaskBytes += encoded.length;
            while (HGMaskScenes.count > 10 || HGMaskBytes > 768 * 1024) {
                HGMaskBytes -= [HGMaskScenes.firstObject[@"bytes"] unsignedIntegerValue];
                [HGMaskScenes removeObjectAtIndex:0]; HGMaskPruned++;
            }
        } else { @synchronized (HGLogLock) { HGMaskRejected++; } }
        @synchronized (HGLogLock) { HGMaskPending--; }
    });
}


BOOL HGEnhancedDiagnostics(void) {
    @synchronized (HGLogLock) { return HGEnhanced; }
}
void HGSetEnhancedDiagnostics(BOOL enabled) {
    @synchronized (HGLogLock) { HGEnhanced = enabled; }
    [NSUserDefaults.standardUserDefaults setBool:enabled forKey:@"Honggo.enhancedDiagnostics"];
    HGLog(@"diagnostics.mode", @{@"enabled": @(enabled)});
}
static void HGRememberError(NSError *error) { if (error && !HGStorageError) HGStorageError = error; }
static NSURL *HGLogURL(NSString *name) { return [HGLogDirectory URLByAppendingPathComponent:name]; }
static NSData *HGJSON(id value) {
    NSError *error = nil;
    NSData *data = [NSJSONSerialization dataWithJSONObject:value options:0 error:&error];
    HGRememberError(error);
    return data;
}
static void HGPrune(NSTimeInterval now) {
    while (HGEvents.count && ([HGEvents.firstObject[@"time"] doubleValue] < now - 3600 ||
            HGEvents.count > 4096 || HGStoredBytes > HGMaxBytes - 65536)) {
        if ([HGEvents.firstObject[@"time"] doubleValue] >= now - 3600) HGCapacityPruned++;
        HGStoredBytes -= HGJSON(HGEvents.firstObject).length + 1;
        [HGEvents removeObjectAtIndex:0];
    }
}
static void HGSave(void) {
    NSData *data = HGJSON(HGEvents);
    NSError *error = nil;
    if (data) [data writeToURL:HGLogURL(@"events.json") options:NSDataWritingAtomic error:&error];
    HGRememberError(error);
}
static void HGPruneExports(void) {
    NSError *error = nil;
    NSArray<NSURL *> *files = [NSFileManager.defaultManager contentsOfDirectoryAtURL:HGLogDirectory
        includingPropertiesForKeys:nil options:0 error:&error];
    HGRememberError(error);
    NSMutableArray<NSURL *> *exports = [NSMutableArray array];
    for (NSURL *url in files) if ([url.lastPathComponent hasPrefix:@"Honggo-log-"]) [exports addObject:url];
    [exports sortUsingComparator:^NSComparisonResult(NSURL *a, NSURL *b) {
        return [a.lastPathComponent compare:b.lastPathComponent];
    }];
    NSTimeInterval now = NSDate.date.timeIntervalSince1970;
    for (NSURL *url in exports.copy) {
        NSDictionary *attrs = [NSFileManager.defaultManager attributesOfItemAtPath:url.path error:&error];
        BOOL expired = now - [attrs[NSFileModificationDate] timeIntervalSince1970] > 3600;
        if (expired || exports.count > 4) {
            [NSFileManager.defaultManager removeItemAtURL:url error:&error];
            HGRememberError(error);
            [exports removeObject:url];
        }
    }
}
void HGDiagnosticsStart(NSURL *directory, NSDictionary *environment) {
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        HGLogLock = [NSObject new];
        HGLogQueue = dispatch_queue_create("org.honggo.diagnostics", DISPATCH_QUEUE_SERIAL);
        HGIncoming = [NSMutableArray array];
        HGSession = NSUUID.UUID.UUIDString;
        HGEnvironment = [environment copy];
        HGEnhanced = [NSUserDefaults.standardUserDefaults boolForKey:@"Honggo.enhancedDiagnostics"];
        HGLogDirectory = directory ?: [[NSFileManager.defaultManager URLsForDirectory:NSCachesDirectory
            inDomains:NSUserDomainMask].firstObject URLByAppendingPathComponent:@"HonggoDiagnostics" isDirectory:YES];
        // A small synchronous stage journal survives a failure before the async store runs.
        NSError *error = nil;
        [NSFileManager.defaultManager createDirectoryAtURL:HGLogDirectory
            withIntermediateDirectories:YES attributes:nil error:&error];
        HGStorageError = error;
        NSDictionary *journalAttributes = [NSFileManager.defaultManager attributesOfItemAtPath:HGLogURL(@"startup.json").path error:nil];
        if (journalAttributes && [journalAttributes[NSFileSize] unsignedLongLongValue] <= 4096) {
            NSData *data = [NSData dataWithContentsOfURL:HGLogURL(@"startup.json")];
            id old = data.length <= 4096 && data ? [NSJSONSerialization JSONObjectWithData:data options:0 error:nil] : nil;
            if ([old isKindOfClass:NSDictionary.class] && [old[@"stage"] isKindOfClass:NSString.class] &&
                [old[@"version"] isKindOfClass:NSString.class] && [old[@"time"] isKindOfClass:NSNumber.class])
                HGPreviousStartup = @{@"stage": old[@"stage"], @"version": old[@"version"], @"time": old[@"time"]};
        }
        dispatch_async(HGLogQueue, ^{
            HGEvents = [NSMutableArray array];
            NSDictionary *attrs = [NSFileManager.defaultManager attributesOfItemAtPath:HGLogURL(@"events.json").path error:nil];
            if ([attrs[NSFileSize] unsignedLongLongValue] <= HGMaxBytes && attrs) {
                NSData *data = [NSData dataWithContentsOfURL:HGLogURL(@"events.json")];
                id decoded = data && data.length <= HGMaxBytes ? [NSJSONSerialization JSONObjectWithData:data options:0 error:nil] : nil;
                // Persisted state is an external boundary. Retain only our bounded event schema.
                if ([decoded isKindOfClass:NSArray.class] && [decoded count] <= 4096) {
                    for (id item in decoded) {
                        if (![item isKindOfClass:NSDictionary.class] || ![item[@"time"] isKindOfClass:NSNumber.class] ||
                            ![item[@"event"] isKindOfClass:NSString.class] || ![item[@"fields"] isKindOfClass:NSDictionary.class] ||
                            ![item[@"session"] isKindOfClass:NSString.class]) continue;
                        NSData *encoded = HGJSON(item);
                        if (encoded.length > 16384) continue;
                        [HGEvents addObject:item];
                        HGStoredBytes += encoded.length + 1;
                    }
                }
            }
            [HGEvents sortUsingComparator:^NSComparisonResult(NSDictionary *a, NSDictionary *b) {
                return [a[@"time"] compare:b[@"time"]];
            }];
            HGPrune(NSDate.date.timeIntervalSince1970);
            HGSave();
            HGPruneExports();
        });
    });
}
void HGLoadStage(NSString *stage) {
    if (!HGLogQueue) return;
    NSDictionary *journal = @{@"version": HG_PLUGIN_VERSION, @"session": HGSession,
        @"stage": stage, @"time": @(NSDate.date.timeIntervalSince1970)};
    NSError *error = nil;
    NSData *data = [NSJSONSerialization dataWithJSONObject:journal options:0 error:&error];
    if (data) [data writeToURL:HGLogURL(@"startup.json") options:NSDataWritingAtomic error:&error];
    if (error) HGLog(@"startup.writeFailed", @{@"code": @(error.code)});
    HGLog(@"startup.stage", @{@"stage": stage});
}
void HGLog(NSString *event, NSDictionary *fields) {
    if (!HGLogQueue) return;
    NSDictionary *entry = @{@"event": event, @"fields": fields ?: @{},
        @"time": @(NSDate.date.timeIntervalSince1970), @"session": HGSession};
    @synchronized (HGLogLock) {
        if (HGPending >= 256) { HGDropped++; return; }
        [HGIncoming addObject:entry];
        HGPending++;
        if (HGDrainScheduled) return;
        HGDrainScheduled = YES;
    }
    dispatch_async(HGLogQueue, ^{
        NSArray *batch;
        @synchronized (HGLogLock) {
            batch = HGIncoming.copy;
            [HGIncoming removeAllObjects];
            HGPending = 0;
            HGDrainScheduled = NO;
        }
        for (NSDictionary *item in batch) {
            NSData *data = HGJSON(item);
            if (!data || data.length > 16384) {
                @synchronized (HGLogLock) { HGDropped++; }
                continue;
            }
            [HGEvents addObject:item];
            HGStoredBytes += data.length + 1;
        }
        HGPrune(NSDate.date.timeIntervalSince1970);
        HGSave();
    });
}
void HGExportDiagnostics(NSDictionary *snapshot, void (^completion)(NSURL *, NSError *)) {
    if (!HGLogQueue) HGDiagnosticsStart(nil, @{});
    NSDictionary *captured = [snapshot copy];
    dispatch_async(HGLogQueue, ^{
        HGPrune(NSDate.date.timeIntervalSince1970);
        NSUInteger dropped;
        @synchronized (HGLogLock) { dropped = HGDropped; }
        NSMutableDictionary *report = [@{@"schema": @1, @"plugin": @"Honggo", @"version": HG_PLUGIN_VERSION,
            @"environment": HGEnvironment ?: @{}, @"session": HGSession, @"events": HGEvents,
            @"snapshot": captured ?: @{}, @"droppedRecords": @(dropped),
            @"capacityPrunedRecords": @(HGCapacityPruned), @"previousStartup": HGPreviousStartup ?: (id)NSNull.null,
            @"retentionSeconds": @3600, @"maxEvents": @4096,
            @"privacy": @"No account, content, URL, request body, cookie or token is collected."} mutableCopy];
        NSMutableArray *scenes = [NSMutableArray array];
        for (NSDictionary *item in HGMaskScenes) [scenes addObject:item[@"scene"]];
        report[@"maskScenes"] = scenes;
        report[@"maskScenesPruned"] = @(HGMaskPruned);
        @synchronized (HGLogLock) { report[@"maskScenesRejected"] = @(HGMaskRejected); }
        report[@"maskSceneRetention"] = @"current process, last 10, max 768 KiB; no screenshot pixels";
        if (HGStorageError) report[@"storageError"] = @{@"domain": HGStorageError.domain, @"code": @(HGStorageError.code)};
        NSString *name = [NSString stringWithFormat:@"Honggo-log-%020.3f-%@.json", NSDate.date.timeIntervalSince1970, NSUUID.UUID.UUIDString];
        NSURL *url = HGLogURL(name);
        NSError *error = nil;
        NSData *data = [NSJSONSerialization dataWithJSONObject:report options:0 error:&error];
        if (data && data.length <= HGMaxBytes + 1024 * 1024) [data writeToURL:url options:NSDataWritingAtomic error:&error];
        else if (!error) error = [NSError errorWithDomain:@"Honggo.Diagnostics" code:1 userInfo:@{NSLocalizedDescriptionKey:@"诊断快照超出导出容量"}];
        HGRememberError(error);
        HGPruneExports();
        dispatch_async(dispatch_get_main_queue(), ^{ completion(error ? nil : url, error); });
    });
}
void HGClearDiagnostics(void (^completion)(NSError *)) {
    if (!HGLogQueue) HGDiagnosticsStart(nil, @{});
    dispatch_async(HGLogQueue, ^{
        [HGEvents removeAllObjects];
        HGStoredBytes = 0;
        [HGMaskScenes removeAllObjects]; HGMaskBytes = 0; HGMaskPruned = 0;
        @synchronized (HGLogLock) { HGMaskRejected = 0; }
        HGCapacityPruned = 0;
        HGPreviousStartup = nil;
        @synchronized (HGLogLock) { HGDropped = 0; }
        HGStorageError = nil;
        NSError *error = nil;
        NSArray<NSURL *> *files = [NSFileManager.defaultManager contentsOfDirectoryAtURL:HGLogDirectory
            includingPropertiesForKeys:nil options:0 error:&error];
        HGRememberError(error);
        for (NSURL *url in files) {
            if ([url.lastPathComponent hasPrefix:@"Honggo-log-"] || [url.lastPathComponent isEqual:@"startup.json"])
                [NSFileManager.defaultManager removeItemAtURL:url error:&error];
            HGRememberError(error);
        }
        HGSave();
        NSError *result = HGStorageError;
        dispatch_async(dispatch_get_main_queue(), ^{ completion(result); });
    });
}
