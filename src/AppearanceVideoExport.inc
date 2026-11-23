// FanqieX-adapted short-drama video export (non-DRM streams only).
// Captures the active FQVPlayer source URL via ABI-checked playbackStateDidChanged hook
// and exports the original container or MP4/MOV/M4V via dynamically loaded AVFoundation.
#include <dlfcn.h>

enum { HGAVideoPlaybackPlaying = 1 };

static void (*HGOrigVideoPlaybackChanged)(id,SEL,id,NSInteger);
static __weak UIView *HGRecentVideoPlayer;
static __weak id HGRecentVideoEngine;
static __weak id HGRecentVideoInfo;
static NSString *HGRecentVideoID;
static NSURL *HGRecentVideoSource;
static NSString *HGRecentVideoTitle;
static NSString *HGRecentVideoFormat;
static NSString *HGRecentVideoReason;

static BOOL HGAVideoHasProtection(id value) {
    if([value isKindOfClass:NSString.class] || [value isKindOfClass:NSData.class])return [value length]>0;
    if([value isKindOfClass:NSNumber.class])return [value boolValue];
    if([value isKindOfClass:NSDictionary.class])return [value count]>0;
    return value!=nil && value!=NSNull.null;
}

static NSURL *HGAVideoMediaURL(id value) {
    NSURL *source=nil;
    if([value isKindOfClass:NSURL.class])source=value;
    else if([value isKindOfClass:NSString.class] && [value length]) {
        source=[value hasPrefix:@"/"] ? [NSURL fileURLWithPath:value] : [NSURL URLWithString:value];
    }
    if(source.isFileURL) {
        NSURL *local=source.URLByResolvingSymlinksInPath.URLByStandardizingPath;
        NSString *home=[NSURL fileURLWithPath:NSHomeDirectory() isDirectory:YES].URLByResolvingSymlinksInPath.path;
        NSNumber *regular=nil;
        return [local.path hasPrefix:[home stringByAppendingString:@"/"]] &&
            [local getResourceValue:&regular forKey:NSURLIsRegularFileKey error:NULL] && regular.boolValue ? local : nil;
    }
    NSString *scheme=source.scheme.lowercaseString;
    NSString *host=source.host.lowercaseString;
    if(!([scheme isEqual:@"https"] || [scheme isEqual:@"http"]) || !host.length || source.user || source.password)return nil;
    return source;
}

static NSString *HGAVideoSafeTitle(id value) {
    if(![value isKindOfClass:NSString.class])return @"短剧视频";
    NSMutableCharacterSet *unsafe=NSCharacterSet.controlCharacterSet.mutableCopy;
    [unsafe addCharactersInString:@"/\\:*?\"<>|"];
    NSString *title=[[(NSString *)value componentsSeparatedByCharactersInSet:unsafe] componentsJoinedByString:@" "];
    title=[title stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
    if(!title.length || [title hasPrefix:@"."])return @"短剧视频";
    if(title.length>70)title=[title substringToIndex:[title rangeOfComposedCharacterSequenceAtIndex:69].location];
    return title.length ? title : @"短剧视频";
}

static void HGAVideoCaptureSource(UIView *player,id engine) {
    id source=HGReadObjectGetter(engine,"playSource");
    id urlInfo=HGReadObjectGetter(source,"usingUrlInfo");
    id videoInfo=HGReadObjectGetter(HGReadObjectGetter(player,"model"),"videoInfo");
    id videoID=HGReadObjectGetter(videoInfo,"videoId");
    HGRecentVideoPlayer=player;
    HGRecentVideoEngine=engine;
    HGRecentVideoInfo=videoInfo;
    HGRecentVideoID=[videoID isKindOfClass:NSString.class] ? [videoID copy] : nil;
    HGRecentVideoSource=nil;
    HGRecentVideoFormat=nil;
    HGRecentVideoTitle=HGAVideoSafeTitle(HGRecentVideoID.length ? [@"短剧视频-" stringByAppendingString:HGRecentVideoID] : nil);
    HGRecentVideoReason=nil;
    BOOL encFlag=NO,liveFlag=NO,dashFlag=NO,bashFlag=NO;
    HGABoolGetter(urlInfo,@"encrypt",&encFlag);
    HGABoolGetter(source,@"isLivePlayback",&liveFlag);
    HGABoolGetter(engine,@"isDashSource",&dashFlag);
    HGABoolGetter(engine,@"isBashSource",&bashFlag);
    if(encFlag || HGAVideoHasProtection(HGReadObjectGetter(urlInfo,"fps")) ||
       HGAVideoHasProtection(HGReadObjectGetter(engine,"fpsLicenseUrl")) ||
       HGAVideoHasProtection(HGReadObjectGetter(engine,"encryptedDecryptionKey")) ||
       HGAVideoHasProtection(HGReadObjectGetter(engine,"decryptionKey"))) {
        HGRecentVideoReason=@"当前视频的来源受到保护，无法导出。";
        return;
    }
    if(liveFlag || dashFlag || bashFlag) {
        HGRecentVideoReason=@"当前视频是直播或分离音视频流，无法作为完整文件导出。";
        return;
    }
    HGRecentVideoSource=HGAVideoMediaURL(HGReadObjectGetter(source,"currentUrl"));
    id fmt=HGReadObjectGetter(engine,"getFileFormat");
    if([fmt isKindOfClass:NSString.class])HGRecentVideoFormat=[(NSString *)fmt lowercaseString];
    if(!HGRecentVideoSource)HGRecentVideoReason=@"请先播放一条短剧视频，再尝试导出。";
}

static void HGHookVideoPlaybackChanged(id self,SEL cmd,id engine,NSInteger state) {
    HGOrigVideoPlaybackChanged(self,cmd,engine,state);
    if(state!=HGAVideoPlaybackPlaying || !engine || ![self isKindOfClass:UIView.class])return;
    UIView *player=(UIView *)self;
    void (^capture)(void)=^{
        UIWindow *win=player.window;
        if(!win || !CGRectIntersectsRect([player convertRect:player.bounds toView:win],win.bounds) ||
           HGReadObjectGetter(player,"videoEngine")!=engine)return;
        for(UIView *v=player;v;v=v.superview)if(v.hidden || v.alpha==0)return;
        HGAVideoCaptureSource(player,engine);
    };
    if(NSThread.isMainThread)capture();
    else dispatch_async(dispatch_get_main_queue(),capture);
}

static NSString *HGAVideoSniffExtension(NSURL *file,NSString *fallbackHint) {
    NSFileHandle *h=[NSFileHandle fileHandleForReadingFromURL:file error:NULL];
    NSData *prefix=[h readDataUpToLength:512 error:NULL];
    [h closeAndReturnError:NULL];
    const uint8_t *b=prefix.bytes;
    NSUInteger n=prefix.length;
    if(n>=7 && !memcmp(b,"#EXTM3U",7))return @"m3u8";
    if(n>=12 && !memcmp(b+4,"ftyp",4)) {
        if(!memcmp(b+8,"qt  ",4))return @"mov";
        if(!memcmp(b+8,"M4V ",4))return @"m4v";
        return @"mp4";
    }
    if(n>=3 && !memcmp(b,"FLV",3))return @"flv";
    if(n>=4 && b[0]==0x1A && b[1]==0x45 && b[2]==0xDF && b[3]==0xA3)return @"mkv";
    if(n>=189 && b[0]==0x47 && b[188]==0x47)return @"ts";
    NSString *ext=fallbackHint.lowercaseString;
    if([@[@"mp4",@"mov",@"m4v",@"ts",@"flv",@"mkv",@"webm"] containsObject:ext])return ext;
    return @"mp4";
}

@interface HGVideoExportController : UITableViewController
@property(nonatomic,strong) NSURL *source;
@property(nonatomic,copy) NSString *sourceFormat;
@property(nonatomic,copy) NSString *mediaTitle;
@property(nonatomic,copy) NSString *unavailableReason;
@property(nonatomic,strong) NSURL *directory;
@property(nonatomic,strong) UIAlertController *progress;
@property(nonatomic,strong) NSURLSession *session;
@property(nonatomic,strong) NSURLSessionTask *task;
@property(nonatomic,strong) id exporter;
@end

@implementation HGVideoExportController
- (void)closePage { [self dismissViewControllerAnimated:YES completion:nil]; }
- (NSInteger)tableView:(UITableView *)t numberOfRowsInSection:(NSInteger)s {
    (void)t;(void)s;
    return self.source ? 4 : 0;
}
- (NSString *)tableView:(UITableView *)t titleForHeaderInSection:(NSInteger)s {
    (void)t;(void)s;
    return self.source ? self.mediaTitle : nil;
}
- (NSString *)tableView:(UITableView *)t titleForFooterInSection:(NSInteger)s {
    (void)t;(void)s;
    return self.source ? @"原文件直接保存当前短剧媒体流。MP4、MOV、M4V 转换取决于当前视频与系统支持。" : self.unavailableReason;
}
- (UITableViewCell *)tableView:(UITableView *)t cellForRowAtIndexPath:(NSIndexPath *)ip {
    UITableViewCell *cell=[t dequeueReusableCellWithIdentifier:@"HGVideoFormat"] ?:
        [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"HGVideoFormat"];
    UIListContentConfiguration *c=cell.defaultContentConfiguration;
    c.text=@[@"原文件（最快）",@"导出为 MP4",@"导出为 MOV",@"导出为 M4V"][ip.row];
    cell.contentConfiguration=c;
    cell.accessoryType=UITableViewCellAccessoryDisclosureIndicator;
    return cell;
}
- (void)tableView:(UITableView *)t didSelectRowAtIndexPath:(NSIndexPath *)ip {
    [t deselectRowAtIndexPath:ip animated:YES];
    if(!self.source || self.directory)return;
    [self beginExport:@[@"",@"mp4",@"mov",@"m4v"][ip.row]];
}
- (void)showError:(NSString *)msg {
    UIAlertController *a=[UIAlertController alertControllerWithTitle:@"无法导出视频" message:msg preferredStyle:UIAlertControllerStyleAlert];
    [a addAction:[UIAlertAction actionWithTitle:@"好" style:UIAlertActionStyleDefault handler:nil]];
    [self presentViewController:a animated:YES completion:nil];
}
- (void)cancelExport {
    [self.task cancel];
    if(self.exporter && [self.exporter respondsToSelector:sel_registerName("cancelExport")]) {
        ((void (*)(id,SEL))objc_msgSend)(self.exporter,sel_registerName("cancelExport"));
    }
    [self.session invalidateAndCancel];
    if(self.directory)[NSFileManager.defaultManager removeItemAtURL:self.directory error:NULL];
    self.task=nil;self.exporter=nil;self.session=nil;self.directory=nil;self.progress=nil;
}
- (void)finishDirectory:(NSURL *)dir file:(NSURL *)file error:(NSString *)err {
    if(![self.directory isEqual:dir])return;
    self.directory=nil;
    [self.session finishTasksAndInvalidate];
    self.session=nil;self.task=nil;self.exporter=nil;
    UIAlertController *prog=self.progress;self.progress=nil;
    void (^show)(void)=^{
        NSFileManager *fm=NSFileManager.defaultManager;
        if(!file || !self.view.window) {
            [fm removeItemAtURL:dir error:NULL];
            if(self.view.window)[self showError:err ?: @"暂时无法处理该视频来源。"];
            return;
        }
        UIActivityViewController *share=[[UIActivityViewController alloc] initWithActivityItems:@[file] applicationActivities:nil];
        share.popoverPresentationController.sourceView=self.view;
        share.popoverPresentationController.sourceRect=CGRectMake(CGRectGetMidX(self.view.bounds),CGRectGetMidY(self.view.bounds),1,1);
        share.completionWithItemsHandler=^(UIActivityType type,BOOL done,NSArray *items,NSError *e) {
            (void)type;(void)done;(void)items;(void)e;
            [fm removeItemAtURL:dir error:NULL];
        };
        [self presentViewController:share animated:YES completion:nil];
    };
    if(prog.presentingViewController)[prog dismissViewControllerAnimated:YES completion:show];
    else show();
}
- (void)beginExport:(NSString *)ext {
    NSURL *dir=[NSFileManager.defaultManager.temporaryDirectory URLByAppendingPathComponent:NSUUID.UUID.UUIDString isDirectory:YES];
    if(![NSFileManager.defaultManager createDirectoryAtURL:dir withIntermediateDirectories:NO attributes:nil error:NULL]) {
        [self showError:@"无法创建临时目录，请检查储存空间。"];
        return;
    }
    self.directory=dir;
    UIAlertController *prog=[UIAlertController alertControllerWithTitle:@"正在准备视频" message:@"完成后可保存到相册/文件或分享。" preferredStyle:UIAlertControllerStyleAlert];
    __weak typeof(self) ws=self;
    [prog addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:^(UIAlertAction *a){(void)a;[ws cancelExport];}]];
    self.progress=prog;
    [self presentViewController:prog animated:YES completion:^{
        if(![self.directory isEqual:dir])return;
        if(self.source.isFileURL)[self processLocalInput:self.source extension:ext directory:dir];
        else [self downloadInputWithExtension:ext directory:dir];
    }];
}
- (void)downloadInputWithExtension:(NSString *)ext directory:(NSURL *)dir {
    NSURLSessionConfiguration *cfg=NSURLSessionConfiguration.ephemeralSessionConfiguration;
    cfg.HTTPShouldSetCookies=NO;cfg.URLCredentialStorage=nil;
    self.session=[NSURLSession sessionWithConfiguration:cfg];
    self.task=[self.session downloadTaskWithURL:self.source completionHandler:^(NSURL *loc,NSURLResponse *resp,NSError *err) {
        NSHTTPURLResponse *http=[resp isKindOfClass:NSHTTPURLResponse.class] ? (NSHTTPURLResponse *)resp : nil;
        NSURL *tmp=[dir URLByAppendingPathComponent:@"download.bin"];
        BOOL ok=!err && http.statusCode==200 && loc && [NSFileManager.defaultManager moveItemAtURL:loc toURL:tmp error:NULL];
        dispatch_async(dispatch_get_main_queue(),^{
            if(![self.directory isEqual:dir])return;
            if(ok)[self processLocalInput:tmp extension:ext directory:dir];
            else [self finishDirectory:dir file:nil error:@"当前视频源无法下载，请重新播放后再试。"];
        });
    }];
    [self.task resume];
}
- (void)processLocalInput:(NSURL *)input extension:(NSString *)ext directory:(NSURL *)dir {
    NSString *detected=HGAVideoSniffExtension(input,self.sourceFormat ?: self.source.pathExtension);
    if([detected isEqual:@"m3u8"]) {
        [self finishDirectory:dir file:nil error:@"当前视频为分段 HLS 流，不支持直接单文件导出。"];
        return;
    }
    BOOL wantOriginal=!ext.length || [ext isEqual:detected];
    NSURL *staged=[[dir URLByAppendingPathComponent:wantOriginal ? self.mediaTitle : @"staged"] URLByAppendingPathExtension:detected];
    BOOL inDir=[input.URLByDeletingLastPathComponent.path isEqual:dir.path];
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED,0),^{
        NSFileManager *fm=NSFileManager.defaultManager;
        BOOL ok=inDir ? [fm moveItemAtURL:input toURL:staged error:NULL] : [fm copyItemAtURL:input toURL:staged error:NULL];
        dispatch_async(dispatch_get_main_queue(),^{
            if(![self.directory isEqual:dir])return;
            if(!ok){[self finishDirectory:dir file:nil error:@"无法写入临时视频文件。"];return;}
            if(wantOriginal){[self finishDirectory:dir file:staged error:nil];return;}
            [self transcodeFile:staged toExtension:ext directory:dir presetIndex:0];
        });
    });
}
- (void)transcodeFile:(NSURL *)input toExtension:(NSString *)ext directory:(NSURL *)dir presetIndex:(NSUInteger)idx {
    static dispatch_once_t avOnce;
    dispatch_once(&avOnce,^{ dlopen("/System/Library/Frameworks/AVFoundation.framework/AVFoundation",RTLD_LAZY); });
    Class assetCls=NSClassFromString(@"AVURLAsset");
    Class exportCls=NSClassFromString(@"AVAssetExportSession");
    if(!assetCls || !exportCls) {
        [self finishDirectory:dir file:nil error:@"当前系统不支持格式转换，请选择“原文件”保存。"];
        return;
    }
    id asset=((id (*)(id,SEL,id,id))objc_msgSend)(assetCls,sel_registerName("URLAssetWithURL:options:"),input,nil);
    NSString *preset=idx ? @"AVAssetExportPresetHighestQuality" : @"AVAssetExportPresetPassthrough";
    id exporter=((id (*)(id,SEL,id,id))objc_msgSend)(exportCls,sel_registerName("exportSessionWithAsset:presetName:"),asset,preset);
    if(!exporter) {
        if(idx==0)[self transcodeFile:input toExtension:ext directory:dir presetIndex:1];
        else [self finishDirectory:dir file:nil error:@"当前视频无法转换为该格式，请尝试保存原文件。"];
        return;
    }
    self.exporter=exporter;
    NSString *uti=[ext isEqual:@"mov"] ? @"com.apple.quicktime-movie" : ([ext isEqual:@"m4v"] ? @"com.apple.m4v-video" : @"public.mpeg-4");
    NSURL *outURL=[[dir URLByAppendingPathComponent:self.mediaTitle] URLByAppendingPathExtension:ext];
    ((void (*)(id,SEL,id))objc_msgSend)(exporter,sel_registerName("setOutputURL:"),outURL);
    ((void (*)(id,SEL,id))objc_msgSend)(exporter,sel_registerName("setOutputFileType:"),uti);
    ((void (*)(id,SEL,void(^)(void)))objc_msgSend)(exporter,sel_registerName("exportAsynchronouslyWithCompletionHandler:"),^{
        dispatch_async(dispatch_get_main_queue(),^{
            if(![self.directory isEqual:dir])return;
            NSInteger status=((NSInteger (*)(id,SEL))objc_msgSend)(exporter,sel_registerName("status"));
            if(status==3)[self finishDirectory:dir file:outURL error:nil];
            else if(idx==0)[self transcodeFile:input toExtension:ext directory:dir presetIndex:1];
            else [self finishDirectory:dir file:nil error:@"转换未完成，请尝试保存原文件。"];
        });
    });
}
@end

static void HGAInstallVideoExport(void) {
    static dispatch_once_t once;
    dispatch_once(&once,^{
        Class cls=NSClassFromString(@"FQVPlayer");
        SEL sel=sel_registerName("videoEngine:playbackStateDidChanged:");
        IMP orig=cls ? HGInstallVerifiedHook(cls,sel,"v32@0:8@16q24",(IMP)HGHookVideoPlaybackChanged,@"appearance.videoExport") : NULL;
        if(orig)HGOrigVideoPlaybackChanged=(void(*)(id,SEL,id,NSInteger))orig;
        HGRecordCapability(@"appearance.videoExport",@{@"module":@"appearance",@"state":orig?@"installed":@"unavailable"});
    });
}

void HGPresentVideoExport(UIViewController *owner) {
    if(!owner || owner.presentedViewController)return;
    UIView *player=HGRecentVideoPlayer;
    id engine=HGRecentVideoEngine;
    if(player && engine && HGReadObjectGetter(player,"videoEngine")==engine) {
        id vInfo=HGReadObjectGetter(HGReadObjectGetter(player,"model"),"videoInfo");
        id vID=HGReadObjectGetter(vInfo,"videoId");
        BOOL same=HGRecentVideoID.length ? ([vID isKindOfClass:NSString.class] && [HGRecentVideoID isEqual:vID]) : (vInfo && vInfo==HGRecentVideoInfo);
        if(same)HGAVideoCaptureSource(player,engine);
    }
    HGVideoExportController *page=[[HGVideoExportController alloc] initWithStyle:UITableViewStyleInsetGrouped];
    page.title=@"导出当前短剧视频";
    page.mediaTitle=HGRecentVideoTitle ?: @"短剧视频";
    page.source=HGRecentVideoSource;
    page.sourceFormat=HGRecentVideoFormat;
    page.unavailableReason=HGRecentVideoReason ?: @"请先播放一条短剧视频，再回来导出。";
    if(owner.navigationController)[owner.navigationController pushViewController:page animated:YES];
    else {
        page.navigationItem.rightBarButtonItem=[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemDone target:page action:@selector(closePage)];
        [owner presentViewController:[[UINavigationController alloc] initWithRootViewController:page] animated:YES completion:nil];
    }
}
