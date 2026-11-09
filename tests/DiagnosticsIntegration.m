#import "Diagnostics.h"
#import <dispatch/dispatch.h>

static NSUInteger failures, checks;
static void Check(BOOL passed, NSString *name) {
    checks++; failures += !passed;
    NSLog(@"%@ %@", passed ? @"PASS" : @"FAIL", name);
}
static NSDictionary *Export(NSDictionary *snapshot, NSURL **file, NSError **failure) {
    __block BOOL done = NO;
    __block NSURL *url;
    __block NSError *error;
    HGExportDiagnostics(snapshot, ^(NSURL *result, NSError *err) { url = result; error = err; done = YES; });
    NSDate *deadline = [NSDate dateWithTimeIntervalSinceNow:15];
    while (!done && deadline.timeIntervalSinceNow > 0)
        [NSRunLoop.currentRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
    Check(done, @"export completes asynchronously");
    if (file) *file = url;
    if (failure) *failure = error;
    NSData *data = url ? [NSData dataWithContentsOfURL:url] : nil;
    return data ? [NSJSONSerialization JSONObjectWithData:data options:0 error:nil] : nil;
}
int main(int argc, const char *argv[]) {
    @autoreleasepool {
        NSURL *directory = [NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingPathComponent:NSUUID.UUID.UUIDString]];
        NSFileManager *manager = NSFileManager.defaultManager;
        [manager createDirectoryAtURL:directory withIntermediateDirectories:YES attributes:nil error:nil];
        NSTimeInterval now = NSDate.date.timeIntervalSince1970;
        NSArray *seed = @[
            @{@"event": @"expired", @"time": @(now-7200), @"session": @"old", @"fields": @{}},
            @{@"event": @"retained", @"time": @(now), @"session": @"old", @"fields": @{}},
            @{@"event": @"bad", @"time": NSNull.null}, NSNull.null
        ];
        [[NSJSONSerialization dataWithJSONObject:seed options:0 error:nil]
            writeToURL:[directory URLByAppendingPathComponent:@"events.json"] atomically:YES];
        if (argc > 1 && strcmp(argv[1], "--write-failure") == 0) {
            NSURL *file = [directory URLByAppendingPathComponent:@"not-a-directory"];
            [@"x" writeToURL:file atomically:YES encoding:NSUTF8StringEncoding error:nil];
            HGDiagnosticsStart(file, @{});
            NSError *error = nil;
            NSDictionary *report = Export(@{}, nil, &error);
            Check(report == nil && error != nil, @"storage failure reaches export caller without throwing");
        } else {
            HGDiagnosticsStart(directory, @{@"test": @YES});
            HGLoadStage(@"entry");
            HGLog(@"nil.fields", nil);
            HGSetEnhancedDiagnostics(YES);
            Check(HGEnhancedDiagnostics(), @"enhanced mode changes immediately");
            NSDictionary *first = Export(@{@"native": @YES}, nil, nil);
            Check([first[@"environment"] isEqual:@{@"test": @YES}], @"export retains environment");
            NSArray *names = [first[@"events"] valueForKey:@"event"];
            Check([names containsObject:@"retained"] && ![names containsObject:@"expired"] && ![names containsObject:@"bad"],
                @"persisted events validate types and expire after one hour");
            Check([names containsObject:@"nil.fields"] && [names containsObject:@"startup.stage"], @"nil event payload and startup event survive export");
            NSData *journal = [NSData dataWithContentsOfURL:[directory URLByAppendingPathComponent:@"startup.json"]];
            Check([[[NSJSONSerialization JSONObjectWithData:journal options:0 error:nil] objectForKey:@"stage"] isEqual:@"entry"],
                @"small startup journal is persisted before stage returns");
            dispatch_apply(6000, dispatch_get_global_queue(QOS_CLASS_DEFAULT, 0), ^(size_t i) {
                HGLog(@"stress", @{@"index": @(i)});
            });
            NSURL *immutable;
            NSDictionary *stress = Export(@{}, &immutable, nil);
            NSData *saved = [NSData dataWithContentsOfURL:immutable];
            Check([stress[@"events"] count] <= 4096 && saved.length <= 2*1024*1024, @"parallel producer burst respects event and byte bounds");
            Check([stress[@"droppedRecords"] unsignedIntegerValue] > 0, @"bounded queue reports dropped records");
            HGLog(@"after.export", @{});
            Export(@{}, nil, nil);
            Check([[NSData dataWithContentsOfURL:immutable] isEqual:saved], @"shared export file is immutable");
            HGStoreMaskScene(@{@"reason": @"user.screenshot", @"nodes": @[@{@"id": @1, @"parent": @0} ]});
            NSDictionary *sceneReport = Export(@{@"page": @"settings"}, nil, nil);
            Check([sceneReport[@"maskScenes"][0][@"reason"] isEqual:@"user.screenshot"],
                @"export in settings retains the earlier failure scene");
            for (NSUInteger i = 0; i < 12; i++) {
                HGStoreMaskScene(@{@"sequence": @(i)});
                sceneReport = Export(@{}, nil, nil);
            }
            Check([sceneReport[@"maskScenes"] count] == 10 && [sceneReport[@"maskScenesPruned"] unsignedIntegerValue] == 3,
                @"scene ring evicts oldest captures independently of event traffic");
            NSString *large = [@"x" stringByPaddingToLength:200*1024 withString:@"x" startingAtIndex:0];
            HGStoreMaskScene(@{@"oversized": large});
            sceneReport = Export(@{}, nil, nil);
            Check([sceneReport[@"maskScenesRejected"] unsignedIntegerValue] == 1 && [sceneReport[@"maskScenes"] count] == 10,
                @"oversized scene is counted and does not discard earlier captures");
            Export(@{}, nil, nil);
            for (NSUInteger i = 0; i < 5; i++) Export(@{}, nil, nil);
            NSUInteger exports = 0;
            for (NSURL *url in [manager contentsOfDirectoryAtURL:directory includingPropertiesForKeys:nil options:0 error:nil])
                exports += [url.lastPathComponent hasPrefix:@"Honggo-log-"];
            Check(exports <= 4, @"old export files are bounded");
            __block BOOL cleared = NO;
            HGClearDiagnostics(^(NSError *error) { Check(error == nil, @"clear completes without storage error"); cleared = YES; });
            NSDate *deadline = [NSDate dateWithTimeIntervalSinceNow:10];
            while (!cleared && deadline.timeIntervalSinceNow > 0)
                [NSRunLoop.currentRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
            NSDictionary *empty = Export(@{}, nil, nil);
            Check([empty[@"maskScenes"] count] == 0, @"clear removes mask scenes too");
            Check(cleared && [empty[@"events"] count] == 0 && ![manager fileExistsAtPath:immutable.path], @"clear removes retained events and previously exported files");
        }
        NSLog(@"Diagnostics: %lu checks, %lu failures", (unsigned long)checks, (unsigned long)failures);
        [manager removeItemAtURL:directory error:nil];
        return failures ? 1 : 0;
    }
}
