#import "Diagnostics.h"
#import "HonggoVersion.h"
#import <dispatch/dispatch.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <stdio.h>
#include <string.h>
#include <TargetConditionals.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <unistd.h>

static const NSUInteger HGMaxBytes = 50 * 1024 * 1024;
static const NSUInteger HGSegmentBytes = 1024 * 1024;
static const NSUInteger HGMaxRecord = 224 * 1024;
static const NSTimeInterval HGRetention = 3600;
static const NSTimeInterval HGSegmentDuration = 300;
static NSString *const HGUnexpected = @"Previous session ended unexpectedly (reason unknown)";

static dispatch_queue_t HGLogQueue, HGReadQueue, HGWatchQueue;
static dispatch_source_t HGWatchTimer, HGMaintenanceTimer;
static NSObject *HGLogLock;
static NSURL *HGLogDirectory, *HGExportDirectory, *HGLegacyDirectory, *HGCurrentDirectory;
static NSDictionary *HGSessionInfo;
static NSDictionary *HGCurrentMetadata;
static NSMutableArray<NSDictionary *> *HGIncoming, *HGRecent, *HGFallback;
static NSUInteger HGDropped;
static BOOL HGDrainScheduled, HGUrgentScheduled, HGEnhanced;
static BOOL HGFlushScheduled, HGDirty, HGExporting;
static NSUInteger HGGeneration, HGArchiveBytes, HGSegmentSize, HGSegmentNumber;
static NSTimeInterval HGSegmentOpened, HGWriteRetry, HGOpenOldest;
static NSMutableDictionary<NSString *, NSNumber *> *HGSegmentOldest;
static NSString *HGOpenPath;
static int HGLogFD = -1;
static NSError *HGStorageError;
static NSMutableArray<NSDictionary *> *HGMaskScenes;
static NSUInteger HGMaskBytes, HGMaskPruned, HGMaskRejected, HGMaskPending;

// Mutable file and session state belongs to HGLogQueue. Producers only hold the
// lock while exchanging bounded in-memory records. Readers use pinned file
// descriptors and byte lengths captured by HGLogQueue.
@interface HGLogSlice : NSObject
@property(nonatomic) int fd;
@property(nonatomic) NSUInteger size;
@end
@implementation HGLogSlice
- (instancetype)init { if ((self = [super init])) _fd = -1; return self; }
- (void)dealloc { if (_fd >= 0) close(_fd); }
@end

static NSError *HGError(NSInteger code, NSString *message) {
    return [NSError errorWithDomain:@"Honggo.Diagnostics" code:code
        userInfo:@{NSLocalizedDescriptionKey: message}];
}

static void HGRememberError(NSError *error) {
    if (error) HGStorageError = error;
}

static NSError *HGIOError(void) {
    return [NSError errorWithDomain:NSPOSIXErrorDomain code:errno userInfo:nil];
}

static NSData *HGJSON(id value) {
    return [NSJSONSerialization isValidJSONObject:value] ?
        [NSJSONSerialization dataWithJSONObject:value options:0 error:nil] : nil;
}

// Call sites supply plugin-generated fields. Apply the same restrictions to
// imported files, structural snapshots and exported rows at the file boundary.
static NSRegularExpression *HGSensitiveText, *HGSensitiveKey;
static NSSet<NSString *> *HGTextFields;
static id HGSanitized(id value, NSString *key, NSUInteger depth) {
    if (depth > 10) return nil;
    if ([value isKindOfClass:NSNumber.class] || value == NSNull.null) return value;
    if ([value isKindOfClass:NSString.class]) {
        if (![HGTextFields containsObject:key] || [value length] > 512) return nil;
        if ([key isEqual:@"session"] || [key isEqual:@"previousSession"])
            return [[NSUUID alloc] initWithUUIDString:value] ? value : nil;
        if ([HGSensitiveText firstMatchInString:value options:0 range:NSMakeRange(0, [value length])]) return nil;
        return value;
    }
    if ([value isKindOfClass:NSArray.class]) {
        NSMutableArray *result = [NSMutableArray array];
        for (id item in value) {
            id clean = HGSanitized(item, key, depth + 1);
            if (clean) [result addObject:clean];
            if (result.count == 256) break;
        }
        return result;
    }
    if ([value isKindOfClass:NSDictionary.class]) {
        NSMutableDictionary *result = [NSMutableDictionary dictionary];
        for (id name in value) {
            if (![name isKindOfClass:NSString.class] || [name length] > 64 ||
                    [HGSensitiveKey firstMatchInString:name options:0 range:NSMakeRange(0, [name length])]) continue;
            id clean = HGSanitized(value[name], name, depth + 1);
            if (clean) result[name] = clean;
            if (result.count == 64) break;
        }
        return result;
    }
    return nil;
}

static BOOL HGWrite(int fd, NSData *data) {
    const unsigned char *bytes = data.bytes;
    NSUInteger sent = 0;
    while (sent < data.length) {
        ssize_t count = write(fd, bytes + sent, data.length - sent);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return NO;
        sent += (NSUInteger)count;
    }
    return YES;
}

static BOOL HGSync(int fd, BOOL critical) {
    if (critical && fcntl(fd, F_FULLFSYNC) == 0) return YES;
    return fsync(fd) == 0;
}

static BOOL HGCreateDirectory(NSURL *url, NSError **error) {
    NSMutableDictionary *attributes = [@{NSFilePosixPermissions: @0700} mutableCopy];
#if TARGET_OS_IOS
    attributes[NSFileProtectionKey] = NSFileProtectionCompleteUntilFirstUserAuthentication;
#endif
    return [NSFileManager.defaultManager createDirectoryAtURL:url withIntermediateDirectories:YES attributes:attributes error:error];
}

static NSArray<NSURL *> *HGFiles(NSURL *directory) {
    return [NSFileManager.defaultManager contentsOfDirectoryAtURL:directory
        includingPropertiesForKeys:@[NSURLFileSizeKey, NSURLContentModificationDateKey] options:NSDirectoryEnumerationSkipsHiddenFiles
        error:nil] ?: @[];
}

static NSUInteger HGFileSize(NSURL *url) {
    NSNumber *size = nil;
    [url getResourceValue:&size forKey:NSURLFileSizeKey error:nil];
    return size.unsignedIntegerValue;
}

static NSArray<NSURL *> *HGSegments(NSURL *directory) {
    NSMutableArray *segments = [NSMutableArray array];
    for (NSURL *url in HGFiles(directory)) if ([url.pathExtension isEqual:@"ndjson"]) [segments addObject:url];
    return [segments sortedArrayUsingComparator:^NSComparisonResult(NSURL *a, NSURL *b) {
        return [a.lastPathComponent compare:b.lastPathComponent];
    }];
}

static id HGReadJSON(NSURL *url, NSUInteger limit) {
    if (HGFileSize(url) > limit) return nil;
    NSData *data = [NSData dataWithContentsOfURL:url];
    return data.length && data.length <= limit ? [NSJSONSerialization JSONObjectWithData:data options:0 error:nil] : nil;
}

static NSArray<NSDictionary *> *HGSessions(void) {
    NSMutableArray *sessions = [NSMutableArray array];
    for (NSURL *url in HGFiles(HGLogDirectory)) {
        NSString *identifier = url.lastPathComponent;
        if (![[NSUUID alloc] initWithUUIDString:identifier]) continue;
        id meta = HGReadJSON([url URLByAppendingPathComponent:@"session.json"], 4096);
        if (![meta isKindOfClass:NSDictionary.class] || ![meta[@"session"] isEqual:identifier] ||
                ![meta[@"startedAt"] isKindOfClass:NSNumber.class]) continue;
        NSMutableDictionary *entry = [HGSanitized(meta, nil, 0) mutableCopy];
        entry[@"state"] = [identifier isEqual:HGSessionInfo[@"session"]] ? @"current" :
            [NSFileManager.defaultManager fileExistsAtPath:[url URLByAppendingPathComponent:@"ended"].path] ?
            @"ended" : @"unexpected";
        if ([entry[@"state"] isEqual:@"unexpected"]) entry[@"endStatus"] = HGUnexpected;
        [sessions addObject:entry];
    }
    return [sessions sortedArrayUsingComparator:^NSComparisonResult(NSDictionary *a, NSDictionary *b) {
        return [a[@"startedAt"] compare:b[@"startedAt"]];
    }];
}

static NSUInteger HGSessionBytes(NSURL *directory) {
    NSUInteger size = 0;
    for (NSURL *url in HGFiles(directory)) size += HGFileSize(url);
    return size;
}

static void HGRemove(NSURL *url, NSUInteger bytes) {
    NSError *error = nil;
    if ([NSFileManager.defaultManager removeItemAtURL:url error:&error])
        HGArchiveBytes -= MIN(bytes, HGArchiveBytes);
    else HGRememberError(error);
}

static void HGRemoveSession(NSURL *directory) {
    for (NSURL *segment in HGSegments(directory)) [HGSegmentOldest removeObjectForKey:segment.path];
    HGRemove(directory, HGSessionBytes(directory));
}

static void HGPruneCapacity(NSUInteger incoming) {
    if (HGArchiveBytes + incoming <= HGMaxBytes) return;
    for (NSDictionary *session in HGSessions()) {
        if (HGArchiveBytes + incoming <= HGMaxBytes) break;
        if ([session[@"state"] isEqual:@"current"]) continue;
        NSURL *directory = [HGLogDirectory URLByAppendingPathComponent:session[@"session"] isDirectory:YES];
        HGRemoveSession(directory);
    }
    for (NSURL *url in HGSegments(HGCurrentDirectory)) {
        if (HGArchiveBytes + incoming <= HGMaxBytes) break;
        if ([url.path isEqual:HGOpenPath]) continue;
        [HGSegmentOldest removeObjectForKey:url.path];
        HGRemove(url, HGFileSize(url));
    }
}

static NSDataWritingOptions HGAtomicWriteOptions(void) {
#if TARGET_OS_IOS
    return NSDataWritingAtomic | NSDataWritingFileProtectionCompleteUntilFirstUserAuthentication;
#else
    return NSDataWritingAtomic;
#endif
}

static BOOL HGWriteMetadata(NSURL *directory, NSDictionary *meta) {
    NSError *error = nil;
    NSData *data = HGJSON(meta);
    if (!HGCreateDirectory(directory, &error) ||
            ![data writeToURL:[directory URLByAppendingPathComponent:@"session.json"]
            options:HGAtomicWriteOptions() error:&error]) {
        HGRememberError(error ?: HGError(1, @"无法保存诊断会话信息"));
        return NO;
    }
    HGArchiveBytes += data.length;
    return YES;
}

static void HGFlush(BOOL critical) {
    if (!HGDirty || HGLogFD < 0) return;
    if (HGSync(HGLogFD, critical)) HGDirty = NO;
    else HGRememberError(HGIOError());
}

static void HGCloseSegment(void) {
    HGFlush(NO);
    if (HGLogFD >= 0) close(HGLogFD);
    HGLogFD = -1;
    HGDirty = NO;
    if (HGOpenPath) HGSegmentOldest[HGOpenPath] = @(HGOpenOldest);
    HGOpenPath = nil;
}

static HGLogSlice *HGOpenSlice(NSURL *url, NSError **error) {
    HGLogSlice *slice = [HGLogSlice new];
    slice.fd = open(url.fileSystemRepresentation, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    struct stat info;
    if (slice.fd < 0 || fstat(slice.fd, &info) != 0) {
        *error = HGIOError();
        return nil;
    }
    if (!S_ISREG(info.st_mode) || info.st_size < 0 || info.st_size > 2 * HGSegmentBytes) {
        *error = HGError(2, @"诊断日志分段大小或格式异常");
        return nil;
    }
    slice.size = (NSUInteger)info.st_size;
    return slice;
}

static BOOL HGReadRows(HGLogSlice *slice, BOOL (^visit)(NSDictionary *row, NSData *line)) {
    NSMutableData *data = [NSMutableData dataWithLength:slice.size];
    unsigned char *bytes = data.mutableBytes;
    NSUInteger count = 0;
    while (count < slice.size) {
        ssize_t length = pread(slice.fd, bytes + count, slice.size - count, (off_t)count);
        if (length < 0 && errno == EINTR) continue;
        if (length <= 0) return NO;
        count += (NSUInteger)length;
    }
    // A killed process can leave a partial last line. Every earlier complete
    // JSON line is independently recoverable.
    for (NSUInteger start = 0; start < count;) @autoreleasepool {
        unsigned char *end = memchr(bytes + start, '\n', count - start);
        if (!end) break;
        NSUInteger length = (NSUInteger)(end - (bytes + start));
        if (length && length <= HGMaxRecord) {
            NSData *line = [NSData dataWithBytesNoCopy:bytes + start length:length freeWhenDone:NO];
            id row = [NSJSONSerialization JSONObjectWithData:line options:0 error:nil];
            if ([row isKindOfClass:NSDictionary.class] && [row[@"event"] isKindOfClass:NSString.class] &&
                    [row[@"time"] isKindOfClass:NSNumber.class] && [row[@"session"] isKindOfClass:NSString.class] &&
                    [row[@"fields"] isKindOfClass:NSDictionary.class] && !visit(row, line)) return NO;
        }
        start += length + 1;
    }
    return YES;
}

static NSArray *HGRowsSince(NSArray *rows, NSTimeInterval cutoff) {
    return [rows filteredArrayUsingPredicate:[NSPredicate predicateWithBlock:^BOOL(id row, NSDictionary *bindings) {
        return [row isKindOfClass:NSDictionary.class] && [row[@"time"] isKindOfClass:NSNumber.class] &&
            [row[@"time"] doubleValue] >= cutoff;
    }]];
}

static BOOL HGPruneSegment(NSURL *url, NSTimeInterval cutoff) {
    NSString *path = url.path;
    if ([path isEqual:HGOpenPath]) return YES;
    NSNumber *cached = HGSegmentOldest[path];
    if (cached && cached.doubleValue >= cutoff) return YES;
    NSDate *modified = nil;
    [url getResourceValue:&modified forKey:NSURLContentModificationDateKey error:nil];
    if (modified && modified.timeIntervalSince1970 < cutoff) {
        [HGSegmentOldest removeObjectForKey:path];
        HGRemove(url, HGFileSize(url));
        return [NSFileManager.defaultManager fileExistsAtPath:path];
    }

    NSError *error = nil;
    HGLogSlice *slice = HGOpenSlice(url, &error);
    if (!slice) { HGRememberError(error); return YES; }
    NSMutableData *kept = [NSMutableData data];
    __block BOOL trimmed = NO;
    __block NSTimeInterval oldest = DBL_MAX;
    if (!HGReadRows(slice, ^BOOL(NSDictionary *row, NSData *line) {
        NSTimeInterval time = [row[@"time"] doubleValue];
        if (time < cutoff) { trimmed = YES; return YES; }
        oldest = MIN(oldest, time);
        NSArray *context = row[@"recentEvents"];
        if ([context isKindOfClass:NSArray.class]) {
            NSArray *recent = HGRowsSince(context, cutoff);
            for (NSDictionary *entry in recent) oldest = MIN(oldest, [entry[@"time"] doubleValue]);
            if (recent.count != context.count) {
                NSMutableDictionary *filtered = row.mutableCopy;
                filtered[@"recentEvents"] = recent;
                line = HGJSON(filtered);
                if (!line.length) return NO;
                trimmed = YES;
            }
        }
        [kept appendData:line];
        [kept appendBytes:"\n" length:1];
        return YES;
    })) { HGRememberError(HGIOError()); return YES; }
    if (trimmed) {
        if (!kept.length) {
            [HGSegmentOldest removeObjectForKey:path];
            HGRemove(url, slice.size);
            return [NSFileManager.defaultManager fileExistsAtPath:path];
        }
        // Replace the closed file atomically. Pinned export descriptors keep
        // their old inode, so retention cannot alter a snapshot in flight.
        if (![kept writeToURL:url options:HGAtomicWriteOptions() error:&error]) {
            HGRememberError(error);
            return YES;
        }
        HGArchiveBytes -= MIN(slice.size, HGArchiveBytes);
        HGArchiveBytes += kept.length;
    }
    HGSegmentOldest[path] = @(oldest < DBL_MAX ? oldest : modified.timeIntervalSince1970);
    return YES;
}

static void HGPruneAge(NSTimeInterval cutoff) {
    if (HGOpenPath && HGOpenOldest < cutoff) HGCloseSegment();
    for (NSDictionary *session in HGSessions()) {
        NSURL *directory = [HGLogDirectory URLByAppendingPathComponent:session[@"session"] isDirectory:YES];
        BOOL hasSegments = NO;
        for (NSURL *url in HGSegments(directory)) hasSegments |= HGPruneSegment(url, cutoff);
        if (!hasSegments && ![session[@"state"] isEqual:@"current"] && [session[@"startedAt"] doubleValue] < cutoff)
            HGRemoveSession(directory);
    }

    [HGRecent setArray:HGRowsSince(HGRecent, cutoff)];
    [HGFallback setArray:HGRowsSince(HGFallback, cutoff)];
    NSIndexSet *expired = [HGMaskScenes indexesOfObjectsPassingTest:^BOOL(NSDictionary *entry, NSUInteger idx, BOOL *stop) {
        if ([entry[@"time"] doubleValue] >= cutoff) return NO;
        HGMaskBytes -= [entry[@"bytes"] unsignedIntegerValue];
        HGMaskPruned++;
        return YES;
    }];
    [HGMaskScenes removeObjectsAtIndexes:expired];
}

static BOOL HGAppend(NSData *batch, BOOL critical, NSTimeInterval oldest) {
    NSTimeInterval uptime = NSProcessInfo.processInfo.systemUptime;
    if (uptime < HGWriteRetry) return NO;
    if (HGLogFD >= 0 && (HGSegmentSize + batch.length > HGSegmentBytes || uptime - HGSegmentOpened >= HGSegmentDuration))
        HGCloseSegment();
    HGPruneCapacity(batch.length + 4096);
    if (HGArchiveBytes + batch.length + 4096 > HGMaxBytes) return NO;
    if (HGLogFD < 0) {
        NSURL *meta = [HGCurrentDirectory URLByAppendingPathComponent:@"session.json"];
        if (![NSFileManager.defaultManager fileExistsAtPath:meta.path] && !HGWriteMetadata(HGCurrentDirectory, HGCurrentMetadata)) {
            HGWriteRetry = uptime + 5;
            return NO;
        }
        NSString *name = [NSString stringWithFormat:@"%08lu.ndjson", (unsigned long)++HGSegmentNumber];
        NSURL *url = [HGCurrentDirectory URLByAppendingPathComponent:name];
        HGLogFD = open(url.fileSystemRepresentation,
            O_WRONLY | O_CREAT | O_EXCL | O_APPEND | O_CLOEXEC, 0600);
        if (HGLogFD >= 0) HGOpenPath = url.path;
        HGSegmentSize = 0;
        HGSegmentOpened = uptime;
        HGOpenOldest = oldest;
    }
    HGOpenOldest = MIN(HGOpenOldest, oldest);
    if (HGLogFD < 0 || !HGWrite(HGLogFD, batch)) {
        HGRememberError(HGIOError());
        // Keep any complete lines from a partial write. The next batch uses a
        // new segment, so an incomplete last line cannot corrupt later rows.
        struct stat info;
        if (HGLogFD >= 0 && fstat(HGLogFD, &info) == 0 && info.st_size > (off_t)HGSegmentSize)
            HGArchiveBytes += (NSUInteger)info.st_size - HGSegmentSize;
        HGCloseSegment();
        HGWriteRetry = uptime + 5;
        return NO;
    }
    HGSegmentSize += batch.length;
    HGArchiveBytes += batch.length;
    HGDirty = YES;
    if (critical) HGFlush(YES);
    if (HGDirty && !HGFlushScheduled) {
        HGFlushScheduled = YES;
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 2 * NSEC_PER_SEC), HGLogQueue, ^{
            HGFlushScheduled = NO;
            HGFlush(NO);
        });
    }
    return YES;
}

static void HGMigrateLegacy(void) {
    NSURL *source = [HGLegacyDirectory URLByAppendingPathComponent:@"events.json"];
    id rows = HGReadJSON(source, 2 * 1024 * 1024);
    id journal = HGReadJSON([HGLegacyDirectory URLByAppendingPathComponent:@"startup.json"], 4096);
    BOOL validRows = [rows isKindOfClass:NSArray.class] && [rows count] <= 4096;
    if (!validRows && ![journal isKindOfClass:NSDictionary.class]) return;
    NSTimeInterval cutoff = NSDate.date.timeIntervalSince1970 - HGRetention;
    NSMutableDictionary<NSString *, NSMutableArray *> *groups = [NSMutableDictionary dictionary];
    for (id row in validRows ? rows : @[]) {
        if (![row isKindOfClass:NSDictionary.class] || ![row[@"time"] isKindOfClass:NSNumber.class] ||
                ![row[@"event"] isKindOfClass:NSString.class] || ![row[@"fields"] isKindOfClass:NSDictionary.class] ||
                ![row[@"session"] isKindOfClass:NSString.class] ||
                ![[NSUUID alloc] initWithUUIDString:row[@"session"]] || [row[@"time"] doubleValue] < cutoff) continue;
        if (!groups[row[@"session"]]) groups[row[@"session"]] = [NSMutableArray array];
        [groups[row[@"session"]] addObject:row];
    }
    if ([journal isKindOfClass:NSDictionary.class] && [journal[@"session"] isKindOfClass:NSString.class] &&
            [[NSUUID alloc] initWithUUIDString:journal[@"session"]] && [journal[@"time"] isKindOfClass:NSNumber.class] &&
            [journal[@"time"] doubleValue] >= cutoff && [journal[@"stage"] isKindOfClass:NSString.class]) {
        NSString *identifier = journal[@"session"];
        if (!groups[identifier]) groups[identifier] = [NSMutableArray array];
        [groups[identifier] addObject:@{@"session": identifier, @"time": journal[@"time"],
            @"event": @"startup.stage", @"fields": @{@"stage": journal[@"stage"], @"legacyJournal": @YES}}];
    }
    BOOL complete = YES;
    for (NSString *identifier in groups) {
        NSURL *directory = [HGLogDirectory URLByAppendingPathComponent:identifier isDirectory:YES];
        NSURL *manifest = [directory URLByAppendingPathComponent:@"session.json"];
        if ([NSFileManager.defaultManager fileExistsAtPath:manifest.path]) continue;
        if ([NSFileManager.defaultManager fileExistsAtPath:directory.path]) HGRemove(directory, HGSessionBytes(directory));
        NSMutableData *lines = [NSMutableData data];
        NSArray *events = [groups[identifier] sortedArrayUsingComparator:^NSComparisonResult(NSDictionary *a, NSDictionary *b) {
            return [a[@"time"] compare:b[@"time"]];
        }];
        for (NSDictionary *row in events) @autoreleasepool {
            NSData *data = HGJSON(HGSanitized(row, nil, 0));
            if (data.length && data.length <= HGMaxRecord) {
                [lines appendData:data];
                [lines appendBytes:"\n" length:1];
            }
        }
        BOOL journalMatches = [journal isKindOfClass:NSDictionary.class] && [journal[@"session"] isEqual:identifier];
        NSDictionary *meta = HGSanitized(@{@"session": identifier, @"startedAt": [events.firstObject objectForKey:@"time"],
            @"legacy": @YES, @"pluginVersion": journalMatches ? journal[@"version"] ?: @"unknown" : @"unknown"}, nil, 0);
        HGPruneCapacity(lines.length + 4096);
        NSError *error = nil;
        // Write the metadata last. A partial migration is retried on next launch.
        NSURL *target = [directory URLByAppendingPathComponent:@"00000001.ndjson"];
        if (HGArchiveBytes + lines.length + 4096 > HGMaxBytes ||
                !HGCreateDirectory(directory, &error) ||
                ![lines writeToURL:target options:HGAtomicWriteOptions() error:&error] ||
                !HGWriteMetadata(directory, meta)) {
            HGRememberError(error);
            complete = NO;
            [NSFileManager.defaultManager removeItemAtURL:directory error:nil];
        } else HGArchiveBytes += lines.length;
    }
    if (complete) {
        if (validRows) [NSFileManager.defaultManager removeItemAtURL:source error:nil];
        [NSFileManager.defaultManager removeItemAtURL:[HGLegacyDirectory URLByAppendingPathComponent:@"startup.json"] error:nil];
    }
}

static void HGRememberRecent(NSDictionary *row) {
    if (!row[@"event"] || [row[@"event"] isEqual:@"scene.capture"] || [row[@"event"] hasPrefix:@"watchdog."]) return;
    NSDictionary *fields = row[@"fields"];
    NSMutableDictionary *context = [NSMutableDictionary dictionary];
    for (NSString *key in @[@"class", @"selector", @"method", @"phase", @"stage", @"tab", @"rule",
            @"invocation", @"requestGate", @"returnOrSkip", @"code", @"enabled"]) {
        if (fields[key]) context[key] = fields[key];
    }
    [HGRecent addObject:@{@"event": row[@"event"], @"time": row[@"time"], @"fields": context}];
    if (HGRecent.count > 16) [HGRecent removeObjectAtIndex:0];
}

static void HGWriteRows(NSArray<NSDictionary *> *rows, BOOL critical) {
    NSMutableData *batch = [NSMutableData data];
    NSMutableArray *included = [NSMutableArray array];
    NSTimeInterval cutoff = NSDate.date.timeIntervalSince1970 - HGRetention;
    NSTimeInterval oldest = DBL_MAX;
    for (NSDictionary *event in rows) @autoreleasepool {
        NSMutableDictionary *row = [HGSanitized(event, nil, 0) mutableCopy];
        NSTimeInterval time = [row[@"time"] doubleValue];
        if (time < cutoff) continue;
        if ([row[@"event"] hasPrefix:@"watchdog."] && ![row[@"event"] isEqual:@"watchdog.recovered"]) {
            NSArray *context = HGRowsSince(HGRecent, cutoff);
            row[@"recentEvents"] = context;
            for (NSDictionary *entry in context) time = MIN(time, [entry[@"time"] doubleValue]);
        }
        NSData *data = HGJSON(row);
        if (!row[@"event"] || !data.length || data.length > ([row[@"event"] isEqual:@"scene.capture"] ? HGMaxRecord : 16 * 1024)) {
            @synchronized (HGLogLock) { HGDropped++; }
            continue;
        }
        if (batch.length + data.length + 1 > HGSegmentBytes) {
            if (!HGAppend(batch, critical, oldest)) [HGFallback addObjectsFromArray:included];
            [included removeAllObjects];
            [batch setLength:0];
            oldest = DBL_MAX;
        }
        HGRememberRecent(row);
        oldest = MIN(oldest, time);
        [included addObject:row];
        [batch appendData:data];
        [batch appendBytes:"\n" length:1];
    }
    if (batch.length && !HGAppend(batch, critical, oldest)) [HGFallback addObjectsFromArray:included];
    // Preserve a small recent diagnostic tail if the directory is unavailable.
    // Large structural captures are excluded from this fallback.
    NSIndexSet *scenes = [HGFallback indexesOfObjectsPassingTest:^BOOL(NSDictionary *row, NSUInteger idx, BOOL *stop) {
        return [row[@"event"] isEqual:@"scene.capture"];
    }];
    [HGFallback removeObjectsAtIndexes:scenes];
    if (HGFallback.count > 64) [HGFallback removeObjectsInRange:NSMakeRange(0, HGFallback.count - 64)];
}

static void HGDrain(void) {
    NSArray *batch;
    @synchronized (HGLogLock) {
        batch = HGIncoming.copy;
        [HGIncoming removeAllObjects];
    }
    if (!batch.count) return;
    NSMutableArray *rows = [NSMutableArray arrayWithCapacity:batch.count];
    BOOL critical = NO, ended = NO;
    for (NSDictionary *item in batch) {
        [rows addObject:item[@"row"]];
        critical |= [item[@"critical"] boolValue];
        ended |= [item[@"row"][@"event"] isEqual:@"session.ended"];
    }
    HGWriteRows(rows, critical);
    if (ended && HGLogFD >= 0 && !HGDirty && ![HGFallback.lastObject[@"event"] isEqual:@"session.ended"]) {
        NSURL *url = [HGCurrentDirectory URLByAppendingPathComponent:@"ended"];
        int fd = open(url.fileSystemRepresentation, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
        if (fd >= 0) { HGSync(fd, YES); close(fd); }
    }
}

static NSDictionary *HGRow(NSString *event, NSDictionary *fields) {
    return @{@"event": event, @"fields": fields ?: @{}, @"time": @(NSDate.date.timeIntervalSince1970),
        @"session": HGSessionInfo[@"session"]};
}

static void HGEnqueue(NSString *event, NSDictionary *fields, BOOL critical, BOOL urgent) {
    if (!HGLogQueue) return;
    NSDictionary *entry = @{@"row": HGRow(event, fields.copy), @"critical": @(critical)};
    @synchronized (HGLogLock) {
        if (HGIncoming.count >= (critical ? 256 : 224)) {
            // An urgent event can replace a queued record even under saturation.
            if (urgent) [HGIncoming removeObjectAtIndex:0];
            HGDropped++;
            if (!urgent) return;
        }
        [HGIncoming addObject:entry];
        if (urgent && !HGUrgentScheduled) {
            HGUrgentScheduled = YES;
            dispatch_async(HGLogQueue, ^{
                @synchronized (HGLogLock) { HGUrgentScheduled = NO; }
                HGDrain();
            });
        }
        if (!HGDrainScheduled) {
            HGDrainScheduled = YES;
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 150 * NSEC_PER_MSEC), HGLogQueue, ^{
                @synchronized (HGLogLock) { HGDrainScheduled = NO; }
                HGDrain();
            });
        }
    }
}

void HGLog(NSString *event, NSDictionary *fields) {
    HGEnqueue(event, fields, [event hasPrefix:@"hook."], NO);
}

void HGLogCritical(NSString *event, NSDictionary *fields) {
    HGEnqueue(event, fields, YES, YES);
}

void HGLoadStage(NSString *stage) {
    HGLogCritical(@"startup.stage", @{@"stage": stage});
}

BOOL HGEnhancedDiagnostics(void) {
    @synchronized (HGLogLock) { return HGEnhanced; }
}

void HGSetEnhancedDiagnostics(BOOL enabled) {
    @synchronized (HGLogLock) { HGEnhanced = enabled; }
    [NSUserDefaults.standardUserDefaults setBool:enabled forKey:@"Honggo.enhancedDiagnostics"];
    HGLog(@"diagnostics.mode", @{@"enabled": @(enabled)});
}

// At most one main-queue probe is outstanding. The watchdog never waits on
// the main thread or tries to suspend it for stack capture.
static NSUInteger HGWatchProbe, HGWatchLevel;
static NSTimeInterval HGWatchStart, HGWatchWallTime;
static BOOL HGWatching;

static void HGWatchReport(NSString *event, NSTimeInterval duration) {
    HGLogCritical(event, @{@"startedAt": @(HGWatchWallTime), @"durationSeconds": @(duration),
        @"probe": @(HGWatchProbe), @"mainThreadStack": @"unavailable"});
}

static void HGWatchCheck(NSTimeInterval now) {
    NSTimeInterval duration = now - HGWatchStart;
    if (duration >= 2 && HGWatchLevel < 1) {
        HGWatchLevel = 1;
        HGWatchReport(@"watchdog.stall", duration);
    }
    if (duration >= 5 && HGWatchLevel < 2) {
        HGWatchLevel = 2;
        HGWatchReport(@"watchdog.severe", duration);
    }
}

static void HGWatchTick(void) {
    if (!HGWatching) return;
    NSTimeInterval now = NSProcessInfo.processInfo.systemUptime;
    if (HGWatchStart != 0) {
        HGWatchCheck(now);
        return;
    }
    const NSUInteger probe = ++HGWatchProbe;
    HGWatchStart = now;
    HGWatchWallTime = NSDate.date.timeIntervalSince1970;
    HGWatchLevel = 0;
    dispatch_async(dispatch_get_main_queue(), ^{
        NSTimeInterval answered = NSProcessInfo.processInfo.systemUptime;
        dispatch_async(HGWatchQueue, ^{
            if (!HGWatching || probe != HGWatchProbe) return;
            HGWatchCheck(answered);
            if (HGWatchLevel) HGWatchReport(@"watchdog.recovered", answered - HGWatchStart);
            HGWatchStart = 0;
        });
    });
}

void HGDiagnosticsSetForeground(BOOL foreground) {
    if (!HGWatchQueue) return;
    dispatch_async(HGWatchQueue, ^{
        if (HGWatching == foreground) return;
        HGWatching = foreground;
        HGWatchProbe++;
        HGWatchStart = 0;
        dispatch_source_set_timer(HGWatchTimer, foreground ? DISPATCH_TIME_NOW : DISPATCH_TIME_FOREVER,
            500 * NSEC_PER_MSEC, 100 * NSEC_PER_MSEC);
    });
    if (!foreground) HGLogCritical(@"session.background", nil);
}

void HGDiagnosticsEnd(void) {
    HGDiagnosticsSetForeground(NO);
    HGLogCritical(@"session.ended", nil);
}

static void HGPruneExports(void) {
    NSArray *files = [HGFiles(HGExportDirectory) sortedArrayUsingComparator:^NSComparisonResult(NSURL *a, NSURL *b) {
        return [b.lastPathComponent compare:a.lastPathComponent];
    }];
    NSUInteger count = 0, size = 0;
    NSTimeInterval now = NSDate.date.timeIntervalSince1970;
    for (NSURL *url in files) {
        if (![url.lastPathComponent hasPrefix:@"Honggo-log-"] || (HGExporting && [url.pathExtension isEqual:@"partial"])) continue;
        NSDate *modified = nil;
        [url getResourceValue:&modified forKey:NSURLContentModificationDateKey error:nil];
        size += HGFileSize(url);
        if (++count > 4 || (count > 1 && size > HGMaxBytes) || now - modified.timeIntervalSince1970 > HGRetention)
            [NSFileManager.defaultManager removeItemAtURL:url error:nil];
    }
}

void HGDiagnosticsStart(NSURL *directory, NSDictionary *environment) {
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        HGLogLock = [NSObject new];
        HGIncoming = [NSMutableArray array];
        HGRecent = [NSMutableArray array];
        HGFallback = [NSMutableArray array];
        HGMaskScenes = [NSMutableArray array];
        HGSegmentOldest = [NSMutableDictionary dictionary];
        HGEnhanced = [NSUserDefaults.standardUserDefaults boolForKey:@"Honggo.enhancedDiagnostics"];
        HGSensitiveKey = [NSRegularExpression regularExpressionWithPattern:
            @"(?i)account|token|cookie|auth(?:orization)?header|password|passwd|secret|credential|bearer|mobile|phone|e-?mail|session[_-]?key|user[_-]?id|request(?:body|headers?)|手机号|账号|密码|令牌|邮箱"
            options:0 error:nil];
        HGSensitiveText = [NSRegularExpression regularExpressionWithPattern:
            @"(?i)https?://|www\\.|[\\w.+-]+@[\\w.-]+\\.[a-z]{2,}|\\b(?:bearer|token|cookie|password|authorization)\\b[ =:]|(?<![\\w.-])(?:\\+?\\d[ ()-]?){9,}\\d(?![\\w.-])"
            options:0 error:nil];
        HGTextFields = [NSSet setWithArray:@[@"session", @"event", @"appVersion", @"hostBuild", @"os", @"device",
            @"pluginVersion", @"previousSession", @"previousSessionStatus", @"class", @"owner", @"targetClass",
            @"selector", @"method", @"module", @"kind", @"reason", @"expected", @"actual", @"encoding", @"state",
            @"phase", @"stage", @"feature", @"action", @"endStatus", @"material", @"mode", @"status", @"name", @"superclass",
            @"childrenSkipped", @"truncationReasons", @"effect", @"frame", @"bounds", @"windowRect", @"maskFrame",
            @"maskBounds", @"maskPathBounds", @"presentationWindowRect", @"windowBounds", @"offset", @"contentSize",
            @"inset", @"adjustedInset", @"transform", @"mainThreadStack", @"executable", @"tab"]];
        char machine[128] = {0};
        size_t length = sizeof(machine);
        NSString *device = sysctlbyname("hw.machine", machine, &length, NULL, 0) == 0 ? @(machine) : @"unknown";
        NSMutableDictionary *info = [@{@"session": NSUUID.UUID.UUIDString, @"startedAt": @(NSDate.date.timeIntervalSince1970),
            @"pluginVersion": HG_PLUGIN_VERSION, @"device": device ?: @"unknown",
            @"os": NSProcessInfo.processInfo.operatingSystemVersionString,
            @"appVersion": [NSBundle.mainBundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"] ?: @"unknown",
            @"hostBuild": [NSBundle.mainBundle objectForInfoDictionaryKey:@"CFBundleVersion"] ?: @"unknown"} mutableCopy];
        NSDictionary *safeEnvironment = HGSanitized(environment, nil, 0) ?: @{};
        for (NSString *key in @[@"hostBuild", @"os", @"executable"]) {
            if ([safeEnvironment[key] isKindOfClass:NSString.class]) info[key] = safeEnvironment[key];
        }
        HGSessionInfo = info.copy;
        dispatch_queue_attr_t utility = dispatch_queue_attr_make_with_qos_class(DISPATCH_QUEUE_SERIAL, QOS_CLASS_UTILITY, 0);
        HGLogQueue = dispatch_queue_create("org.honggo.diagnostics", utility);
        HGReadQueue = dispatch_queue_create("org.honggo.diagnostics.reader", utility);
        HGWatchQueue = dispatch_queue_create("org.honggo.diagnostics.watchdog", utility);
        HGWatchTimer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, HGWatchQueue);
        dispatch_source_set_event_handler(HGWatchTimer, ^{ HGWatchTick(); });
        dispatch_source_set_timer(HGWatchTimer, DISPATCH_TIME_FOREVER, 500 * NSEC_PER_MSEC, 100 * NSEC_PER_MSEC);
        dispatch_resume(HGWatchTimer);
        dispatch_async(HGLogQueue, ^{
            NSFileManager *manager = NSFileManager.defaultManager;
            NSURL *support = [manager URLsForDirectory:NSApplicationSupportDirectory inDomains:NSUserDomainMask].firstObject;
            NSURL *cache = [manager URLsForDirectory:NSCachesDirectory inDomains:NSUserDomainMask].firstObject;
            HGLogDirectory = directory ?: [support URLByAppendingPathComponent:@"Honggo/Logs" isDirectory:YES];
            HGLegacyDirectory = directory ?: [cache URLByAppendingPathComponent:@"HonggoDiagnostics" isDirectory:YES];
            HGExportDirectory = directory ? [directory URLByAppendingPathComponent:@"Exports" isDirectory:YES] :
                [cache URLByAppendingPathComponent:@"HonggoDiagnosticExports" isDirectory:YES];
            HGCurrentDirectory = [HGLogDirectory URLByAppendingPathComponent:info[@"session"] isDirectory:YES];
            NSError *error = nil;
            if (HGCreateDirectory(HGLogDirectory, &error)) {
                [HGLogDirectory setResourceValue:@YES forKey:NSURLIsExcludedFromBackupKey error:nil];
                for (NSURL *url in HGFiles(HGLogDirectory))
                    if ([[NSUUID alloc] initWithUUIDString:url.lastPathComponent]) {
                        if ([manager fileExistsAtPath:[url URLByAppendingPathComponent:@"session.json"].path])
                            HGArchiveBytes += HGSessionBytes(url);
                        else HGRemove(url, 0);
                    }
                HGPruneAge(NSDate.date.timeIntervalSince1970 - HGRetention);
                HGMigrateLegacy();
                HGPruneCapacity(0);
            } else HGRememberError(error);
            NSDictionary *previous = HGSessions().lastObject;
            NSMutableDictionary *fields = [info mutableCopy];
            [fields removeObjectForKey:@"session"];
            if (previous) {
                fields[@"previousSession"] = previous[@"session"];
                fields[@"previousSessionStatus"] = [previous[@"state"] isEqual:@"ended"] ? @"ended" : HGUnexpected;
            }
            NSMutableDictionary *metadata = fields.mutableCopy;
            metadata[@"session"] = info[@"session"];
            HGCurrentMetadata = metadata.copy;
            HGWriteRows(@[HGRow(@"session.started", fields)], YES);
            HGPruneExports();
            HGMaintenanceTimer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, HGLogQueue);
            dispatch_source_set_event_handler(HGMaintenanceTimer, ^{
                HGPruneAge(NSDate.date.timeIntervalSince1970 - HGRetention);
                HGPruneExports();
            });
            dispatch_source_set_timer(HGMaintenanceTimer, dispatch_time(DISPATCH_TIME_NOW, 60 * NSEC_PER_SEC),
                60 * NSEC_PER_SEC, 5 * NSEC_PER_SEC);
            dispatch_resume(HGMaintenanceTimer);
        });
        HGDiagnosticsSetForeground(YES);
    });
}

void HGStoreMaskScene(NSDictionary *scene) {
    if (!HGLogQueue) return;
    NSDictionary *entry = HGRow(@"scene.capture", @{@"scene": scene});
    @synchronized (HGLogLock) {
        if (HGMaskPending >= 4) { HGMaskRejected++; return; }
        HGMaskPending++;
    }
    dispatch_async(HGLogQueue, ^{
        NSDictionary *stored = HGSanitized(scene, nil, 0);
        NSData *encoded = HGJSON(stored);
        if (encoded.length > 192 * 1024 && [stored[@"nodes"] isKindOfClass:NSArray.class]) {
            NSMutableDictionary *trimmed = stored.mutableCopy;
            NSMutableArray *nodes = [stored[@"nodes"] mutableCopy];
            trimmed[@"nodes"] = nodes;
            trimmed[@"truncated"] = @YES;
            trimmed[@"byteLimited"] = @YES;
            while (nodes.count && encoded.length > 192 * 1024) {
                NSUInteger count = MIN(nodes.count, 8);
                [nodes removeObjectsInRange:NSMakeRange(nodes.count - count, count)];
                encoded = HGJSON(trimmed);
            }
            stored = trimmed;
        }
        if (encoded.length && encoded.length <= 192 * 1024) {
            [HGMaskScenes addObject:@{@"scene": stored, @"bytes": @(encoded.length), @"time": entry[@"time"]}];
            HGMaskBytes += encoded.length;
            while (HGMaskScenes.count > 10 || HGMaskBytes > 768 * 1024) {
                HGMaskBytes -= [HGMaskScenes.firstObject[@"bytes"] unsignedIntegerValue];
                [HGMaskScenes removeObjectAtIndex:0];
                HGMaskPruned++;
            }
            NSMutableDictionary *row = entry.mutableCopy;
            row[@"fields"] = @{@"scene": stored};
            HGDrain();
            HGWriteRows(@[row], NO);
        } else {
            @synchronized (HGLogLock) { HGMaskRejected++; }
        }
        @synchronized (HGLogLock) { HGMaskPending--; }
    });
}


static NSArray<HGLogSlice *> *HGSlices(NSArray<NSDictionary *> *sessions, BOOL tail, NSError **error) {
    NSMutableArray *slices = [NSMutableArray array];
    for (NSDictionary *session in sessions) {
        NSURL *directory = [HGLogDirectory URLByAppendingPathComponent:session[@"session"] isDirectory:YES];
        NSArray *segments = HGSegments(directory);
        NSUInteger first = tail && segments.count > 2 ? segments.count - 2 : 0;
        for (NSUInteger i = first; i < segments.count; i++) {
            HGLogSlice *slice = HGOpenSlice(segments[i], error);
            if (!slice) return nil;
            [slices addObject:slice];
        }
    }
    return slices;
}

static NSDictionary *HGCleanRowForRead(NSDictionary *row, NSTimeInterval cutoff) {
    if ([row[@"time"] doubleValue] < cutoff) return nil;
    NSDictionary *clean = HGSanitized(row, nil, 0);
    if (!clean[@"event"] || !clean[@"session"]) return nil;
    NSArray *context = clean[@"recentEvents"];
    if (![context isKindOfClass:NSArray.class]) return clean;
    NSArray *recent = HGRowsSince(context, cutoff);
    if (recent.count == context.count) return clean;
    NSMutableDictionary *filtered = clean.mutableCopy;
    filtered[@"recentEvents"] = recent;
    return filtered;
}

static NSArray<NSDictionary *> *HGVisibleSessions(void) {
    NSArray *sessions = HGSessions();
    for (NSDictionary *session in sessions)
        if ([session[@"session"] isEqual:HGSessionInfo[@"session"]]) return sessions;
    NSMutableDictionary *current = HGCurrentMetadata.mutableCopy;
    current[@"state"] = @"current";
    return [sessions arrayByAddingObject:current];
}

void HGListDiagnosticSessions(void (^completion)(NSArray<NSDictionary *> *, NSError *)) {
    HGDiagnosticsStart(nil, @{});
    dispatch_async(HGLogQueue, ^{
        HGPruneAge(NSDate.date.timeIntervalSince1970 - HGRetention);
        NSArray *sessions = HGVisibleSessions();
        NSError *error = HGStorageError;
        dispatch_async(dispatch_get_main_queue(), ^{ completion(sessions, error); });
    });
}

void HGReadDiagnosticSession(NSString *identifier, void (^completion)(NSArray<NSDictionary *> *, NSError *)) {
    HGDiagnosticsStart(nil, @{});
    dispatch_async(HGLogQueue, ^{
        HGDrain();
        NSTimeInterval cutoff = NSDate.date.timeIntervalSince1970 - HGRetention;
        HGPruneAge(cutoff);
        NSDictionary *selected = nil;
        for (NSDictionary *session in HGVisibleSessions())
            if ([session[@"session"] isEqual:identifier]) { selected = session; break; }
        NSError *error = selected ? nil : HGError(3, @"此诊断会话已被清理");
        NSArray *slices = selected ? HGSlices(@[selected], YES, &error) : nil;
        NSArray *fallback = [selected[@"state"] isEqual:@"current"] ? HGFallback.copy : @[];
        NSUInteger generation = HGGeneration;
        dispatch_async(HGReadQueue, ^{
            NSMutableArray *rows = [NSMutableArray array];
            BOOL success = !error;
            for (HGLogSlice *slice in slices) {
                success = HGReadRows(slice, ^BOOL(NSDictionary *row, NSData *line) {
                    // Keep the event visible without copying large captures into UIKit.
                    NSDictionary *clean = HGCleanRowForRead(row, cutoff);
                    if (!clean) return YES;
                    [rows addObject:[clean[@"event"] isEqual:@"scene.capture"] ?
                        @{@"event": clean[@"event"], @"time": clean[@"time"], @"session": clean[@"session"]} : clean];
                    if (rows.count > 120) [rows removeObjectAtIndex:0];
                    return YES;
                });
                if (!success) break;
            }
            for (NSDictionary *row in fallback) {
                NSDictionary *clean = HGCleanRowForRead(row, cutoff);
                if (clean) [rows addObject:clean];
            }
            [rows sortUsingComparator:^NSComparisonResult(NSDictionary *a, NSDictionary *b) {
                return [b[@"time"] compare:a[@"time"]];
            }];
            NSArray *tail = [rows subarrayWithRange:NSMakeRange(0, MIN(rows.count, 120))];
            dispatch_async(HGLogQueue, ^{
                NSError *result = generation != HGGeneration ? HGError(3, @"诊断日志已清除") :
                    error ?: (success ? nil : HGError(2, @"无法读取诊断日志"));
                dispatch_async(dispatch_get_main_queue(), ^{ completion(result ? @[] : tail, result); });
            });
        });
    });
}

static BOOL HGWriteExport(int fd, NSDictionary *report, NSArray<HGLogSlice *> *slices, NSArray<NSDictionary *> *fallback,
        NSTimeInterval cutoff) {
    NSData *header = HGJSON(report);
    if (!header.length ||
            !HGWrite(fd, [header subdataWithRange:NSMakeRange(0, header.length - 1)]) ||
            !HGWrite(fd, [@",\"events\":[" dataUsingEncoding:NSUTF8StringEncoding])) return NO;
    // Stream one slice at a time and batch the output. Export never builds a
    // memory copy of the archive and does not contend with appends for the writer queue.
    NSMutableData *output = [NSMutableData data];
    __block BOOL first = YES;
    BOOL (^appendRow)(NSDictionary *) = ^BOOL(NSDictionary *row) {
        NSDictionary *clean = HGCleanRowForRead(row, cutoff);
        if (!clean) return YES;
        NSData *line = HGJSON(clean);
        if (!line.length) return YES;
        if (!first) [output appendBytes:",\n" length:2];
        first = NO;
        [output appendData:line];
        if (output.length < 64 * 1024) return YES;
        BOOL written = HGWrite(fd, output);
        [output setLength:0];
        return written;
    };
    for (HGLogSlice *slice in slices)
        if (!HGReadRows(slice, ^BOOL(NSDictionary *row, NSData *line) { return appendRow(row); })) return NO;
    for (NSDictionary *row in fallback) @autoreleasepool {
        if (!appendRow(row)) return NO;
    }
    [output appendBytes:"]}\n" length:3];
    return HGWrite(fd, output);
}

void HGExportDiagnosticSessions(HGDiagnosticScope scope, NSString *identifier, NSDictionary *snapshot,
        void (^completion)(NSURL *, NSError *)) {
    HGDiagnosticsStart(nil, @{});
    dispatch_async(HGLogQueue, ^{
        if (HGExporting) {
            dispatch_async(dispatch_get_main_queue(), ^{ completion(nil, HGError(4, @"已有诊断日志正在导出")); });
            return;
        }
        HGDrain();
        NSTimeInterval cutoff = NSDate.date.timeIntervalSince1970 - HGRetention;
        HGPruneAge(cutoff);
        NSMutableArray *selected = [NSMutableArray array];
        for (NSDictionary *session in HGVisibleSessions()) {
            if (scope == HGDiagnosticScopeAll ||
                    (scope == HGDiagnosticScopeSession && [session[@"session"] isEqual:identifier])) {
                [selected addObject:session];
            } else if (scope == HGDiagnosticScopePreviousUnexpected && [session[@"state"] isEqual:@"unexpected"]) {
                [selected setArray:@[session]];
            }
        }
        NSError *error = selected.count ? nil : HGError(3, @"没有符合条件的诊断会话");
        NSArray *slices = selected.count ? HGSlices(selected, NO, &error) : nil;
        if (!error && !HGCreateDirectory(HGExportDirectory, &error) && !error)
            error = HGError(1, @"无法创建诊断导出目录");
        if (error) {
            dispatch_async(dispatch_get_main_queue(), ^{ completion(nil, error); });
            return;
        }
        HGExporting = YES;
        NSUInteger generation = HGGeneration;
        BOOL includesCurrent = [selected indexOfObjectPassingTest:^BOOL(NSDictionary *session, NSUInteger idx, BOOL *stop) {
                return [session[@"state"] isEqual:@"current"];
            }] != NSNotFound;
        NSArray *fallback = includesCurrent ? HGFallback.copy : @[];
        NSUInteger dropped, rejected;
        @synchronized (HGLogLock) { dropped = HGDropped; rejected = HGMaskRejected; }
        NSMutableDictionary *report = [@{@"schema": @2, @"plugin": @"Honggo", @"version": HG_PLUGIN_VERSION,
            @"sessions": selected.copy, @"retentionSeconds": @(HGRetention), @"maxBytes": @(HGMaxBytes),
            @"droppedRecords": @(dropped), @"unpersistedRecords": @(fallback.count),
            @"snapshot": includesCurrent ? HGSanitized(snapshot, nil, 0) ?: @{} : @{},
            @"maskScenes": includesCurrent ? [HGMaskScenes valueForKey:@"scene"] : @[],
            @"maskScenesPruned": @(includesCurrent ? HGMaskPruned : 0), @"maskScenesRejected": @(includesCurrent ? rejected : 0),
            @"privacy": @"Plugin-generated technical metadata only. No account, page text, URLs, request data, cookies or tokens collected."}
            mutableCopy];
        if (includesCurrent && HGStorageError)
            report[@"storageError"] = @{@"domain": HGStorageError.domain, @"code": @(HGStorageError.code)};
        NSString *name = [NSString stringWithFormat:@"Honggo-log-%020.3f-%@.json",
            NSDate.date.timeIntervalSince1970, NSUUID.UUID.UUIDString];
        NSURL *url = [HGExportDirectory URLByAppendingPathComponent:name];
        NSURL *staging = [url URLByAppendingPathExtension:@"partial"];
        int fd = open(staging.fileSystemRepresentation, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
        if (fd < 0) {
            HGExporting = NO;
            NSError *openError = HGIOError();
            dispatch_async(dispatch_get_main_queue(), ^{ completion(nil, openError); });
            return;
        }
        dispatch_async(HGReadQueue, ^{
            BOOL success = HGWriteExport(fd, report, slices, fallback, cutoff);
            close(fd);
            dispatch_async(HGLogQueue, ^{
                HGExporting = NO;
                NSError *result = nil;
                if (generation != HGGeneration) result = HGError(3, @"诊断日志已清除");
                else if (!success) result = HGError(2, @"无法写入完整诊断导出");
                else [NSFileManager.defaultManager moveItemAtURL:staging toURL:url error:&result];
                if (result) [NSFileManager.defaultManager removeItemAtURL:staging error:nil];
                HGPruneExports();
                dispatch_async(dispatch_get_main_queue(), ^{ completion(result ? nil : url, result); });
            });
        });
    });
}

void HGExportDiagnostics(NSDictionary *snapshot, void (^completion)(NSURL *, NSError *)) {
    HGExportDiagnosticSessions(HGDiagnosticScopeAll, nil, snapshot, completion);
}

void HGClearDiagnostics(void (^completion)(NSError *)) {
    HGDiagnosticsStart(nil, @{});
    dispatch_async(HGLogQueue, ^{
        ++HGGeneration;
        @synchronized (HGLogLock) {
            [HGIncoming removeAllObjects];
            HGDropped = 0;
            HGMaskRejected = 0;
        }
        HGCloseSegment();
        [HGSegmentOldest removeAllObjects];
        [HGRecent removeAllObjects];
        HGCurrentMetadata = HGSessionInfo;
        [HGFallback removeAllObjects];
        [HGMaskScenes removeAllObjects];
        HGMaskBytes = HGMaskPruned = 0;
        HGStorageError = nil;
        HGWriteRetry = 0;
        for (NSURL *url in HGFiles(HGLogDirectory)) {
            if ([[NSUUID alloc] initWithUUIDString:url.lastPathComponent]) HGRemove(url, HGSessionBytes(url));
        }
        NSMutableArray *files = [NSMutableArray arrayWithArray:HGFiles(HGExportDirectory)];
        [files addObjectsFromArray:HGFiles(HGLegacyDirectory)];
        for (NSURL *url in files) {
            if ([url.lastPathComponent hasPrefix:@"Honggo-log-"] ||
                    [@[@"events.json", @"startup.json"] containsObject:url.lastPathComponent]) {
                NSError *error = nil;
                [NSFileManager.defaultManager removeItemAtURL:url error:&error];
                HGRememberError(error);
            }
        }
        NSError *result = HGStorageError;
        dispatch_async(dispatch_get_main_queue(), ^{ completion(result); });
    });
}
