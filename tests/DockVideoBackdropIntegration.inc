// Real CAMetalLayer drawables exercise the live compositor path. A UIView
// background or a CALayer render would not prove that video updates propagate.
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <objc/message.h>
#import <objc/runtime.h>

@interface HGVideoBackdrop : NSObject
- (void)refreshWithBranches:(NSArray<UIView *> *)branches root:(UIView *)root bar:(UIView *)bar active:(BOOL)active;
- (void)restore;
@end
BOOL HGVideoBackdropRefreshForFeed(UIScrollView *feed);
void HGVideoBackdropPrepareRendererTransfer(UIView *renderer);

@interface FQVShortVideoListViewController : UIViewController @end
@implementation FQVShortVideoListViewController @end

@interface HGBackdropMetalFixture : TTMetalView
@property(nonatomic,strong) id<MTLDevice> device;
@property(nonatomic,strong) id<MTLCommandQueue> commands;
- (BOOL)presentRed:(double)red green:(double)green blue:(double)blue;
@end
@implementation HGBackdropMetalFixture
+ (Class)layerClass {return CAMetalLayer.class;}
- (instancetype)initWithFrame:(CGRect)frame {
    if((self=[super initWithFrame:frame])) {
        self.device=MTLCreateSystemDefaultDevice();
        self.commands=[self.device newCommandQueue];
        CAMetalLayer *layer=(id)self.layer;
        layer.device=self.device;layer.pixelFormat=MTLPixelFormatBGRA8Unorm;
        layer.framebufferOnly=YES;layer.opaque=YES;
    }return self;
}
- (BOOL)presentRed:(double)red green:(double)green blue:(double)blue {
    if(!self.window || !self.commands || CGRectIsEmpty(self.bounds))return NO;
    CAMetalLayer *layer=(id)self.layer;
    CGFloat scale=self.window.screen.scale;
    layer.contentsScale=scale;
    layer.drawableSize=CGSizeMake(ceil(self.bounds.size.width*scale),ceil(self.bounds.size.height*scale));
    id<CAMetalDrawable> drawable=[layer nextDrawable];
    if(!drawable)return NO;
    MTLRenderPassDescriptor *pass=[MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture=drawable.texture;
    pass.colorAttachments[0].loadAction=MTLLoadActionClear;
    pass.colorAttachments[0].storeAction=MTLStoreActionStore;
    pass.colorAttachments[0].clearColor=MTLClearColorMake(red,green,blue,1);
    id<MTLCommandBuffer> command=[self.commands commandBuffer];
    id<MTLRenderCommandEncoder> encoder=[command renderCommandEncoderWithDescriptor:pass];
    if(!command || !encoder)return NO;
    [encoder endEncoding];[command presentDrawable:drawable];[command commit];[command waitUntilCompleted];
    return command.status==MTLCommandBufferStatusCompleted;
}
@end

@interface HGBackdropVideoCell : UICollectionViewCell
@property(nonatomic,strong) FQVShortVideoPlayerCellView *canvas;
@property(nonatomic,strong) TTPlayerView *renderer;
@property(nonatomic,strong) HGBackdropMetalFixture *metal;
@property(nonatomic) CGFloat videoHeight;
@end
@implementation HGBackdropVideoCell
- (instancetype)initWithFrame:(CGRect)frame {
    if((self=[super initWithFrame:frame])) {
        self.canvas=[[FQVShortVideoPlayerCellView alloc] initWithFrame:self.contentView.bounds];
        self.renderer=[[TTPlayerView alloc] initWithFrame:self.canvas.bounds];
        for(UIView *view in self.renderer.subviews.copy)[view removeFromSuperview];
        self.metal=[[HGBackdropMetalFixture alloc] initWithFrame:self.renderer.bounds];
        [self.renderer addSubview:self.metal];[self.canvas addSubview:self.renderer];
        // A conspicuous native HUD must not be copied into the extension.
        UIView *hud=[[UIView alloc] initWithFrame:CGRectMake(0,0,frame.size.width,32)];
        hud.backgroundColor=UIColor.magentaColor;hud.autoresizingMask=UIViewAutoresizingFlexibleWidth;
        [self.canvas addSubview:hud];[self.contentView addSubview:self.canvas];
    }return self;
}
- (void)layoutSubviews {
    [super layoutSubviews];self.canvas.frame=self.contentView.bounds;
    CGRect video=self.canvas.bounds;
    if(self.videoHeight>0) {
        video.origin.y=CGRectGetMidY(video)-self.videoHeight/2;
        video.size.height=self.videoHeight;
    }
    self.renderer.frame=video;self.metal.frame=self.renderer.bounds;
}
@end
@interface HGBackdropVideoDataSource : NSObject <UICollectionViewDataSource> @end
@implementation HGBackdropVideoDataSource
- (NSInteger)collectionView:(UICollectionView *)view numberOfItemsInSection:(NSInteger)section {return 3;}
- (__kindof UICollectionViewCell *)collectionView:(UICollectionView *)view cellForItemAtIndexPath:(NSIndexPath *)path {
    return [view dequeueReusableCellWithReuseIdentifier:@"live-metal" forIndexPath:path];
}
@end

static NSArray<NSDictionary *> *HGBackdropNativeSnapshot(NSArray<UIView *> *views) {
    NSMutableArray *snapshot=[NSMutableArray array];
    for(UIView *view in views) {
        NSMutableDictionary *record=[@{@"view":view,@"parent":view.superview?:NSNull.null,
            @"frame":[NSValue valueWithCGRect:view.frame],@"bounds":[NSValue valueWithCGRect:view.bounds],
            @"transform":[NSValue valueWithCGAffineTransform:view.transform],
            @"sublayerTransform":[NSValue valueWithCATransform3D:view.layer.sublayerTransform]} mutableCopy];
        if([view isKindOfClass:UIScrollView.class]) {
            UIScrollView *scroll=(id)view;
            record[@"offset"]=[NSValue valueWithCGPoint:scroll.contentOffset];
            record[@"size"]=[NSValue valueWithCGSize:scroll.contentSize];
            record[@"inset"]=[NSValue valueWithUIEdgeInsets:scroll.contentInset];
            record[@"indicator"]=[NSValue valueWithUIEdgeInsets:scroll.verticalScrollIndicatorInsets];
        }
        [snapshot addObject:record];
    }return snapshot;
}
static BOOL HGBackdropNativeUnchanged(NSArray<NSDictionary *> *snapshot) {
    for(NSDictionary *record in snapshot) {
        UIView *view=record[@"view"];
        if((view.superview?:NSNull.null)!=record[@"parent"] ||
            !CGRectEqualToRect(view.frame,[record[@"frame"] CGRectValue]) ||
            !CGRectEqualToRect(view.bounds,[record[@"bounds"] CGRectValue]) ||
            !CGAffineTransformEqualToTransform(view.transform,[record[@"transform"] CGAffineTransformValue]) ||
            !CATransform3DEqualToTransform(view.layer.sublayerTransform,[record[@"sublayerTransform"] CATransform3DValue]))return NO;
        if([view isKindOfClass:UIScrollView.class]) {
            UIScrollView *scroll=(id)view;
            if(!CGPointEqualToPoint(scroll.contentOffset,[record[@"offset"] CGPointValue]) ||
                !CGSizeEqualToSize(scroll.contentSize,[record[@"size"] CGSizeValue]) ||
                !UIEdgeInsetsEqualToEdgeInsets(scroll.contentInset,[record[@"inset"] UIEdgeInsetsValue]) ||
                !UIEdgeInsetsEqualToEdgeInsets(scroll.verticalScrollIndicatorInsets,[record[@"indicator"] UIEdgeInsetsValue]))return NO;
        }
    }return YES;
}
static void HGBackdropFlushFrames(NSTimeInterval interval) {
    CFTimeInterval end=CACurrentMediaTime()+interval;
    do {[CATransaction flush];CFRunLoopRunInMode(kCFRunLoopDefaultMode,1.0/60.0,false);}
    while(CACurrentMediaTime()<end);
}
static NSDictionary *HGBackdropViewGeometry(UIView *view,UIWindow *window) {
    if(!view)return @{@"missing":@YES};
    return @{@"class":NSStringFromClass(view.class),
        @"superclass":NSStringFromClass(class_getSuperclass(view.class))?:@"",
        @"parent":view.superview?NSStringFromClass(view.superview.class):@"none",
        @"identifier":view.accessibilityIdentifier?:@"",
        @"frame":NSStringFromCGRect(view.frame),@"bounds":NSStringFromCGRect(view.bounds),
        @"windowRect":NSStringFromCGRect([view convertRect:view.bounds toView:window]),
        @"safeArea":NSStringFromUIEdgeInsets(view.safeAreaInsets),
        @"hidden":@(view.hidden),@"alpha":@(view.alpha),@"mounted":@(view.window==window)};
}
static NSMethodSignature *HGBackdropProbeSignature(id target,NSString *name) {
    Method method=target?class_getInstanceMethod([target class],NSSelectorFromString(name)):NULL;
    const char *encoding=method?method_getTypeEncoding(method):NULL;
    return encoding?[NSMethodSignature signatureWithObjCTypes:encoding]:nil;
}
static NSDictionary *HGBackdropReadProbeGetter(id target,NSString *name,id expected) {
    Method method=target?class_getInstanceMethod([target class],NSSelectorFromString(name)):NULL;
    const char *encoding=method?method_getTypeEncoding(method):NULL;
    NSMethodSignature *signature=HGBackdropProbeSignature(target,name);
    NSMutableDictionary *value=[@{@"encoding":encoding?@(encoding):@"missing"} mutableCopy];
    if(!signature || signature.numberOfArguments!=2)return value;
    char result=signature.methodReturnType[0];SEL selector=NSSelectorFromString(name);
    if(result=='B' || result=='c')value[@"value"]=@(((BOOL(*)(id,SEL))objc_msgSend)(target,selector));
    else if(result=='@') {
        id object=((id(*)(id,SEL))objc_msgSend)(target,selector);
        value[@"class"]=object?NSStringFromClass([object class]):@"nil";
        value[@"matchesExpected"]=@(expected && object==expected);
    }
    return value;
}
static NSDictionary *HGBackdropPortalMetadata(UIView *portal,UIView *source) {
    NSMutableDictionary *view=[NSMutableDictionary dictionary],*layer=[NSMutableDictionary dictionary];
    for(NSString *name in @[@"sourceView",@"hidesSourceView",@"matchesPosition",@"matchesTransform",
        @"matchesAlpha",@"allowsHitTesting",@"forwardsClientHitTestingToSourceView"])
        view[name]=HGBackdropReadProbeGetter(portal,name,[name isEqual:@"sourceView"]?source:nil);
    for(NSString *name in @[@"sourceLayer",@"hidesSourceLayer",@"matchesPosition",@"matchesTransform",@"matchesOpacity"])
        layer[name]=HGBackdropReadProbeGetter(portal.layer,name,[name isEqual:@"sourceLayer"]?source.layer:nil);
    return @{@"portalClass":portal?NSStringFromClass(portal.class):@"nil",
        @"layerClass":portal.layer?NSStringFromClass(portal.layer.class):@"nil",@"viewGetters":view,@"layerGetters":layer};
}
static BOOL HGBackdropSetProbeFlag(UIView *portal,NSString *name,BOOL value) {
    NSMethodSignature *signature=HGBackdropProbeSignature(portal,name);
    if(!signature || signature.numberOfArguments!=3 || signature.methodReturnType[0]!='v')return NO;
    char argument=[signature getArgumentTypeAtIndex:2][0];
    if(argument!='B' && argument!='c')return NO;
    ((void(*)(id,SEL,BOOL))objc_msgSend)(portal,NSSelectorFromString(name),value);return YES;
}
static BOOL HGBackdropSetProbeSource(UIView *portal,UIView *source) {
    NSMethodSignature *signature=HGBackdropProbeSignature(portal,@"setSourceView:");
    if(!signature || signature.numberOfArguments!=3 || signature.methodReturnType[0]!='v' ||
        [signature getArgumentTypeAtIndex:2][0]!='@')return NO;
    ((void(*)(id,SEL,id))objc_msgSend)(portal,NSSelectorFromString(@"setSourceView:"),source);return YES;
}
static NSArray<NSDictionary *> *HGBackdropCreateDiagnosticProbes(UIView *page,TTPlayerView *renderer,UIView *metal,CGFloat feedHeight) {
    UIView *green=[[UIView alloc] initWithFrame:CGRectMake(8,MAX(36,page.safeAreaInsets.top+4),64,18)];
    green.backgroundColor=[UIColor colorWithRed:0.04 green:0.90 blue:0.12 alpha:1];
    green.userInteractionEnabled=NO;[page addSubview:green];
    NSArray *specs=@[@{@"name":@"view-identity",@"source":green},
        @{@"name":@"player-identity",@"source":renderer},
        @{@"name":@"metal-identity",@"source":metal},
        @{@"name":@"metal-flipped",@"source":metal,@"flip":@YES},
        @{@"name":@"player-source-first",@"source":renderer,@"sourceFirst":@YES},
        @{@"name":@"player-flipped-reinsert",@"source":renderer,@"flip":@YES,@"reinsert":@YES},
        @{@"name":@"player-bottom-flipped-reinsert",@"source":renderer,@"flip":@YES,@"reinsert":@YES,@"bottom":@YES}];
    Class portalClass=NSClassFromString(@"_UIPortalView");
    NSMutableArray *probes=[NSMutableArray array];
    for(NSUInteger index=0;index<specs.count;index++) {
        NSDictionary *spec=specs[index];UIView *source=spec[@"source"];
        CGFloat y=[spec[@"bottom"] boolValue]?feedHeight+8:MAX(72,page.safeAreaInsets.top+36)+index*56;
        UIView *clip=[[UIView alloc] initWithFrame:CGRectMake(8,y,64,40)];
        // An opaque neutral backing prevents the native red/blue movie below
        // an empty portal from producing a false positive in these probes.
        clip.backgroundColor=[UIColor colorWithWhite:0.06 alpha:1];clip.opaque=YES;
        clip.clipsToBounds=YES;clip.userInteractionEnabled=NO;[page addSubview:clip];
        UILabel *label=[[UILabel alloc] initWithFrame:CGRectMake(8,CGRectGetMaxY(clip.frame)+1,94,12)];
        label.font=[UIFont systemFontOfSize:8];label.textColor=UIColor.whiteColor;
        label.text=spec[@"name"];[page addSubview:label];
        UIView *portal=portalClass && [portalClass isSubclassOfClass:UIView.class]?[[portalClass alloc] initWithFrame:CGRectZero]:nil;
        if(!portal){[probes addObject:@{@"name":spec[@"name"],@"view":clip,@"label":label,@"source":source,@"error":@"portal class unavailable"}];continue;}
        portal.userInteractionEnabled=NO;portal.accessibilityElementsHidden=YES;[clip addSubview:portal];
        BOOL configured=HGBackdropSetProbeFlag(portal,@"setHidesSourceView:",NO);
        if([spec[@"sourceFirst"] boolValue])configured&=HGBackdropSetProbeSource(portal,source);
        configured&=HGBackdropSetProbeFlag(portal,@"setMatchesPosition:",NO);
        configured&=HGBackdropSetProbeFlag(portal,@"setMatchesTransform:",NO);
        configured&=HGBackdropSetProbeFlag(portal,@"setMatchesAlpha:",YES);
        configured&=HGBackdropSetProbeFlag(portal,@"setAllowsHitTesting:",NO);
        configured&=HGBackdropSetProbeFlag(portal,@"setForwardsClientHitTestingToSourceView:",NO);
        if(![spec[@"sourceFirst"] boolValue])configured&=HGBackdropSetProbeSource(portal,source);
        NSDictionary *assigned=HGBackdropPortalMetadata(portal,source);
        if([spec[@"reinsert"] boolValue]) {
            NSUInteger position=[clip.subviews indexOfObjectIdenticalTo:portal];
            if(position!=NSNotFound)[clip insertSubview:portal atIndex:position];
        }
        portal.bounds=source.bounds;portal.center=CGPointMake(source.bounds.size.width/2,source.bounds.size.height/2);
        portal.transform=[spec[@"flip"] boolValue]?CGAffineTransformMakeScale(1,-1):CGAffineTransformIdentity;
        [probes addObject:@{@"name":spec[@"name"],@"view":clip,@"label":label,@"portal":portal,@"source":source,
            @"configured":@(configured),@"afterAssignment":assigned}];
    }return probes;
}
static NSArray<NSDictionary *> *HGBackdropProbeGeometry(NSArray<NSDictionary *> *probes,UIWindow *window) {
    NSMutableArray *result=[NSMutableArray array];
    for(NSDictionary *probe in probes) {
        UIView *portal=probe[@"portal"],*source=probe[@"source"];
        [result addObject:@{@"name":probe[@"name"],@"configured":probe[@"configured"]?:@NO,
            @"clip":HGBackdropViewGeometry(probe[@"view"],window),@"portal":HGBackdropViewGeometry(portal,window),
            @"source":HGBackdropViewGeometry(source,window),@"afterAssignment":probe[@"afterAssignment"]?:@{},
            @"afterLayout":HGBackdropPortalMetadata(portal,source)}];
    }return result;
}
static NSDictionary *HGBackdropCaptureGeometry(UIWindow *window) {
    UIView *extension=Find(window,@"honggo.video.backdrop");
    UIView *canvas=Find(extension,@"honggo.video.backdrop.canvas");
    UIView *portal=Find(extension,@"honggo.video.backdrop.portal");
    SEL sourceSelector=NSSelectorFromString(@"sourceView");
    UIView *source=[portal respondsToSelector:sourceSelector]?((id(*)(id,SEL))objc_msgSend)(portal,sourceSelector):nil;
    NSMutableArray *subtree=[NSMutableArray array],*queue=[NSMutableArray array];
    if(extension)[queue addObject:extension];
    for(NSUInteger index=0;index<queue.count && index<16;index++) {
        UIView *view=queue[index];[subtree addObject:HGBackdropViewGeometry(view,window)];
        for(UIView *child in view.subviews){if(queue.count>=16)break;[queue addObject:child];}
    }
    return @{@"extension":HGBackdropViewGeometry(extension,window),@"canvas":HGBackdropViewGeometry(canvas,window),
        @"portal":HGBackdropViewGeometry(portal,window),@"source":HGBackdropViewGeometry(source,window),
        @"portalRuntime":HGBackdropPortalMetadata(portal,source),@"subtree":subtree};
}
static UIImage *HGBackdropCompositorCapture(NSString *stage,UIWindow *window,CGRect nativeRect,CGRect extensionRect,NSArray *probes) {
    NSURL *documents=[NSFileManager.defaultManager URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask].firstObject;
    [NSFileManager.defaultManager createDirectoryAtURL:documents withIntermediateDirectories:YES attributes:nil error:nil];
    NSString *base=[@"video-backdrop-" stringByAppendingString:stage];
    NSURL *ready=[documents URLByAppendingPathComponent:[base stringByAppendingString:@"-ready.json"]];
    NSURL *captured=[documents URLByAppendingPathComponent:[base stringByAppendingString:@"-captured"]];
    NSURL *image=[documents URLByAppendingPathComponent:[base stringByAppendingString:@"-simulator.png"]];
    NSURL *active=[documents URLByAppendingPathComponent:@"video-backdrop-active-phase.txt"];
    [NSFileManager.defaultManager removeItemAtURL:captured error:nil];
    [NSFileManager.defaultManager removeItemAtURL:image error:nil];
    [window layoutIfNeeded];Layout(Find(window,@"honggo.video.backdrop"));
    HGBackdropFlushFrames(0.35);
    NSDictionary *request=@{@"stage":stage,@"window":NSStringFromCGRect(window.bounds),
        @"nativeSample":NSStringFromCGRect(nativeRect),@"extensionSample":NSStringFromCGRect(extensionRect),
        @"source":@"presented CAMetalDrawable; compositor capture required",@"geometry":HGBackdropCaptureGeometry(window),
        @"diagnosticProbes":HGBackdropProbeGeometry(probes,window)};
    [stage writeToURL:active atomically:YES encoding:NSUTF8StringEncoding error:nil];
    [[NSJSONSerialization dataWithJSONObject:request options:NSJSONWritingPrettyPrinted error:nil]
        writeToURL:ready atomically:YES];
    // The host transport has two attempts inside a 40s total budget. Keep the
    // displayed Metal phase alive for 60s, running the UI loop throughout.
    CFTimeInterval started=CACurrentMediaTime(),deadline=started+60;
    while(![NSFileManager.defaultManager fileExistsAtPath:captured.path] && CACurrentMediaTime()<deadline)
        CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.03,false);
    UIImage *result=[UIImage imageWithContentsOfFile:image.path];
    NSLog(@"VIDEO_BACKDROP_CAPTURE %@ ack=%d image=%d elapsed=%.3f",stage,
        [NSFileManager.defaultManager fileExistsAtPath:captured.path],result!=nil,CACurrentMediaTime()-started);
    if([[NSString stringWithContentsOfURL:active encoding:NSUTF8StringEncoding error:nil] isEqual:stage])
        [NSFileManager.defaultManager removeItemAtURL:active error:nil];
    return result;
}
typedef struct {CGFloat red,green,blue;BOOL valid;} HGBackdropRGB;
static HGBackdropRGB HGBackdropAverageColor(UIImage *image,UIWindow *window,CGRect rect) {
    HGBackdropRGB result={0,0,0,NO};
    if(!image.CGImage || CGRectIsEmpty(window.bounds))return result;
    CGFloat scale=CGImageGetWidth(image.CGImage)/window.bounds.size.width;
    CGRect pixels=CGRectIntegral(CGRectApplyAffineTransform(rect,CGAffineTransformMakeScale(scale,scale)));
    CGImageRef crop=CGImageCreateWithImageInRect(image.CGImage,pixels);if(!crop)return result;
    uint8_t bytes[16*16*4]={0};CGColorSpaceRef space=CGColorSpaceCreateDeviceRGB();
    CGContextRef context=CGBitmapContextCreate(bytes,16,16,8,16*4,space,
        kCGImageAlphaPremultipliedLast|kCGBitmapByteOrder32Big);
    CGColorSpaceRelease(space);
    if(!context){CGImageRelease(crop);return result;}
    CGContextDrawImage(context,CGRectMake(0,0,16,16),crop);CGContextRelease(context);CGImageRelease(crop);
    for(NSUInteger i=0;i<16*16;i++){result.red+=bytes[i*4];result.green+=bytes[i*4+1];result.blue+=bytes[i*4+2];}
    CGFloat denominator=16*16*255.0;
    result.red/=denominator;result.green/=denominator;result.blue/=denominator;result.valid=YES;
    return result;
}
static NSArray<NSDictionary *> *HGBackdropProbeColors(NSArray<NSDictionary *> *probes,UIImage *image,UIWindow *window) {
    NSMutableArray *colors=[NSMutableArray array];
    for(NSDictionary *probe in probes) {
        UIView *clip=probe[@"view"];
        // Use the top 10pt for every probe: the visible green source is only
        // 18pt high and a center crop would include its black backing.
        CGRect sample=[clip convertRect:CGRectMake(8,4,40,10) toView:window];
        HGBackdropRGB rgb=HGBackdropAverageColor(image,window,sample);
        NSMutableDictionary *color=[@{@"name":probe[@"name"],@"sample":NSStringFromCGRect(sample),
            @"valid":@(rgb.valid),@"r":@(rgb.red),@"g":@(rgb.green),@"b":@(rgb.blue)} mutableCopy];
        if([probe[@"name"] isEqual:@"view-identity"]) {
            UIView *source=probe[@"source"];
            HGBackdropRGB direct=HGBackdropAverageColor(image,window,[source convertRect:CGRectMake(8,4,40,10) toView:window]);
            color[@"visibleGreenSource"]=@{@"valid":@(direct.valid),@"r":@(direct.red),@"g":@(direct.green),@"b":@(direct.blue)};
        }
        [colors addObject:color];
        NSLog(@"VIDEO_BACKDROP_PROBE %@ RGB=%.3f/%.3f/%.3f valid=%d",probe[@"name"],rgb.red,rgb.green,rgb.blue,rgb.valid);
    }return colors;
}
static void HGBackdropRemoveDiagnosticProbes(NSArray<NSDictionary *> *probes) {
    for(NSDictionary *probe in probes) {
        HGBackdropSetProbeSource(probe[@"portal"],nil);
        [probe[@"view"] removeFromSuperview];[probe[@"label"] removeFromSuperview];
        if([probe[@"name"] isEqual:@"view-identity"])[probe[@"source"] removeFromSuperview];
    }
}
static void HGBackdropWriteColorEvidence(NSString *stage,HGBackdropRGB native,HGBackdropRGB extension,NSArray *probeColors) {
    NSDictionary *sample=@{@"stage":stage,
        @"native":@{@"valid":@(native.valid),@"r":@(native.red),@"g":@(native.green),@"b":@(native.blue)},
        @"extension":@{@"valid":@(extension.valid),@"r":@(extension.red),@"g":@(extension.green),@"b":@(extension.blue)},
        @"diagnosticProbes":probeColors};
    NSURL *documents=[NSFileManager.defaultManager URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask].firstObject;
    NSURL *url=[documents URLByAppendingPathComponent:[NSString stringWithFormat:@"video-backdrop-%@-sample.json",stage]];
    [[NSJSONSerialization dataWithJSONObject:sample options:NSJSONWritingPrettyPrinted error:nil] writeToURL:url atomically:YES];
    NSLog(@"VIDEO_BACKDROP_SAMPLE %@ native=%.3f/%.3f/%.3f valid=%d extension=%.3f/%.3f/%.3f valid=%d",
        stage,native.red,native.green,native.blue,native.valid,extension.red,extension.green,extension.blue,extension.valid);
}
static UIView *HGBackdropPortalSource(UIView *portal) {
    SEL selector=NSSelectorFromString(@"sourceView");
    return [portal respondsToSelector:selector]?((id(*)(id,SEL))objc_msgSend)(portal,selector):nil;
}
static void HGBackdropLogPortalSignatures(void) {
    Class type=NSClassFromString(@"_UIPortalView");
    NSLog(@"VIDEO_BACKDROP_ABI class=%@",type?NSStringFromClass(type):@"missing");
    const char *selectors[]={"sourceView","setSourceView:","setHidesSourceView:",
        "setMatchesPosition:","setMatchesTransform:","setMatchesAlpha:",
        "setAllowsHitTesting:","setForwardsClientHitTestingToSourceView:"};
    for(NSUInteger i=0;i<sizeof(selectors)/sizeof(selectors[0]);i++) {
        Method method=type?class_getInstanceMethod(type,sel_registerName(selectors[i])):NULL;
        const char *encoding=method?method_getTypeEncoding(method):NULL;
        NSLog(@"VIDEO_BACKDROP_ABI %s %s",selectors[i],encoding?:"missing");
    }
}

static void RunDockVideoBackdropChecks(SSTabBarController *host) {
    NSDictionary *saved=HGAppearanceValues();
    UIWindow *originalWindow=host.view.window;
    UIWindow *window=[[UIWindow alloc] initWithWindowScene:originalWindow.windowScene];
    window.windowLevel=UIWindowLevelNormal+1;
    UIViewController *container=[UIViewController new];window.rootViewController=container;
    [window makeKeyAndVisible];[container loadViewIfNeeded];Layout(container.view);
    FQVShortVideoListViewController *page=[FQVShortVideoListViewController new];
    [container addChildViewController:page];[page loadViewIfNeeded];
    page.view.frame=container.view.bounds;page.view.backgroundColor=UIColor.blackColor;
    [container.view addSubview:page.view];[page didMoveToParentViewController:container];
    CGFloat width=page.view.bounds.size.width,height=page.view.bounds.size.height,feedHeight=height-90;
    UICollectionViewFlowLayout *layout=[UICollectionViewFlowLayout new];
    layout.itemSize=CGSizeMake(width,feedHeight);layout.minimumLineSpacing=0;layout.minimumInteritemSpacing=0;
    FQVCollectionView *feed=[[FQVCollectionView alloc] initWithFrame:CGRectMake(0,0,width,feedHeight) collectionViewLayout:layout];
    feed.contentInsetAdjustmentBehavior=UIScrollViewContentInsetAdjustmentNever;
    feed.pagingEnabled=YES;feed.backgroundColor=UIColor.blackColor;
    feed.showsVerticalScrollIndicator=NO;feed.showsHorizontalScrollIndicator=NO;
    __attribute__((objc_precise_lifetime)) HGBackdropVideoDataSource *dataSource=[HGBackdropVideoDataSource new];
    feed.dataSource=dataSource;
    [feed registerClass:HGBackdropVideoCell.class forCellWithReuseIdentifier:@"live-metal"];
    [page.view addSubview:feed];[feed reloadData];Layout(feed);
    [feed setContentOffset:CGPointMake(0,feedHeight) animated:NO];Layout(feed);
    HGBackdropVideoCell *cell=(id)[feed cellForItemAtIndexPath:[NSIndexPath indexPathForItem:1 inSection:0]];
    Layout(cell);Layout(cell.renderer);
    UIView *bar=[[UIView alloc] initWithFrame:CGRectMake(0,height-84,width,84)];
    bar.backgroundColor=UIColor.clearColor;bar.userInteractionEnabled=NO;[container.view addSubview:bar];
    HGVideoBackdrop *manager=[HGVideoBackdrop new];
    NSArray *branches=@[page.view];
    @try {
        HGAppearanceApply(@{@"enabled":@YES,@"dock.enabled":@YES,@"player.fullscreen":@NO},nil);
        Check(cell && [cell.metal.layer isKindOfClass:CAMetalLayer.class] && cell.metal.device,
            @"video backdrop fixture uses a real Metal device and CAMetalLayer on a native middle feed page");
        if(!cell || !cell.metal.device)return;
        NSArray *geometry=HGBackdropNativeSnapshot(@[page.view,feed,cell,cell.contentView,cell.canvas,cell.renderer,cell.metal]);
        BOOL redPresented=[cell.metal presentRed:0.92 green:0.05 blue:0.08];
        [manager refreshWithBranches:branches root:container.view bar:bar active:YES];
        UIView *extension=Find(page.view,@"honggo.video.backdrop");
        UIView *portal=Find(page.view,@"honggo.video.backdrop.portal");
        UIView *backdropCanvas=Find(page.view,@"honggo.video.backdrop.canvas");
        if(!extension || !portal) {
            HGBackdropLogPortalSignatures();
            NSLog(@"VIDEO_BACKDROP_FIXTURE window=%@ root=%@ parent=%@ feed=%@ bounds=%@ offset=%@ renderer=%@ child=%@ layers=%lu",
                NSStringFromCGRect(window.bounds),NSStringFromCGRect(container.view.bounds),NSStringFromCGRect(page.view.bounds),
                NSStringFromCGRect(feed.frame),NSStringFromCGRect(feed.bounds),NSStringFromCGPoint(feed.contentOffset),
                NSStringFromCGRect(cell.renderer.frame),NSStringFromClass(cell.renderer.subviews.firstObject.class),
                (unsigned long)cell.renderer.layer.sublayers.count);
        }
        Check(redPresented && extension && portal && HGBackdropPortalSource(portal)==cell.renderer &&
            [page.view.subviews indexOfObject:extension]>[page.view.subviews indexOfObject:feed],
            @"live reflected strip attaches above the feed only in its uncovered area and excludes the native HUD");
        Check(extension.window==window && backdropCanvas.superview && backdropCanvas.window==window &&
            portal.window==window && [portal isDescendantOfView:extension],
            @"reflected edge canvas and portal are mounted in the live window hierarchy");
        CGRect strip=[backdropCanvas convertRect:backdropCanvas.bounds toView:page.view];
        CGPoint reflectedBottom=[portal convertPoint:CGPointMake(CGRectGetMidX(portal.bounds),CGRectGetMaxY(portal.bounds))
            toView:page.view];
        Check(extension.clipsToBounds && backdropCanvas.clipsToBounds &&
            fabs(strip.origin.x)<0.01 && fabs(strip.origin.y-feedHeight)<0.01 &&
            fabs(strip.size.width-width)<0.01 && fabs(strip.size.height-90)<0.01 &&
            CGRectEqualToRect(portal.bounds,cell.renderer.bounds) &&
            fabs(portal.center.x-width/2)<0.01 && fabs(portal.center.y-feedHeight/2)<0.01 &&
            CGAffineTransformEqualToTransform(portal.transform,CGAffineTransformMakeScale(1,-1)) &&
            fabs(reflectedBottom.y-feedHeight)<0.01 &&
            CGAffineTransformIsIdentity(cell.renderer.transform) &&
            CATransform3DIsIdentity(cell.renderer.layer.transform) &&
            CATransform3DIsIdentity(cell.renderer.layer.sublayerTransform),
            @"only the uncovered 90pt strip reflects the video edge while the native renderer remains untransformed");
        for(NSUInteger pass=0;pass<3;pass++)HGVideoBackdropRefreshForFeed(feed);
        Check(HGBackdropNativeUnchanged(geometry),
            @"enabling the backdrop preserves all native parents frames bounds transforms insets content size and page offset");
        Check(Count(page.view,@"honggo.video.backdrop")==1 &&
            Find(page.view,@"honggo.video.backdrop.portal")==portal && HGBackdropPortalSource(portal)==cell.renderer,
            @"feed relayout reuses one reflected strip and never duplicates its live portal");
        if(!redPresented || !extension || !portal)return;
        CGRect nativeRect=CGRectMake(width*0.35,feedHeight*0.45,width*0.3,24);
        CGRect extensionRect=CGRectMake(width*0.35,feedHeight+14,width*0.3,24);
        NSArray *probes=HGBackdropCreateDiagnosticProbes(page.view,cell.renderer,cell.metal,feedHeight);
        UIImage *red=HGBackdropCompositorCapture(@"red",window,nativeRect,extensionRect,probes);
        HGBackdropRGB nativeRed=HGBackdropAverageColor(red,window,nativeRect);
        HGBackdropRGB extendedRed=HGBackdropAverageColor(red,window,extensionRect);
        HGBackdropWriteColorEvidence(@"red",nativeRed,extendedRed,HGBackdropProbeColors(probes,red,window));
        Check(nativeRed.valid && nativeRed.red>nativeRed.blue+0.35 &&
            extendedRed.valid && extendedRed.red>extendedRed.blue+0.12,
            @"simulator compositor shows the red Metal video below the native viewport instead of a black bottom mask");
        BOOL bluePresented=[cell.metal presentRed:0.05 green:0.08 blue:0.92];
        // Intentionally do not refresh or recreate the manager/portal between
        // colors. The same live source must propagate the next GPU drawable.
        UIImage *blue=HGBackdropCompositorCapture(@"blue",window,nativeRect,extensionRect,probes);
        HGBackdropRGB nativeBlue=HGBackdropAverageColor(blue,window,nativeRect);
        HGBackdropRGB extendedBlue=HGBackdropAverageColor(blue,window,extensionRect);
        HGBackdropWriteColorEvidence(@"blue",nativeBlue,extendedBlue,HGBackdropProbeColors(probes,blue,window));
        HGBackdropRemoveDiagnosticProbes(probes);
        Check(bluePresented && nativeBlue.valid && nativeBlue.blue>nativeBlue.red+0.35 &&
            extendedBlue.valid && extendedBlue.blue>extendedBlue.red+0.12 &&
            Find(page.view,@"honggo.video.backdrop.portal")==portal && HGBackdropNativeUnchanged(geometry),
            @"the same live portal follows the next blue Metal frame without snapshots native resizing or repagination");
        NSLog(@"VIDEO_BACKDROP_COMPOSITOR red native=%.3f/%.3f/%.3f extension=%.3f/%.3f/%.3f blue native=%.3f/%.3f/%.3f extension=%.3f/%.3f/%.3f",
            nativeRed.red,nativeRed.green,nativeRed.blue,extendedRed.red,extendedRed.green,extendedRed.blue,
            nativeBlue.red,nativeBlue.green,nativeBlue.blue,extendedBlue.red,extendedBlue.green,extendedBlue.blue);
        cell.videoHeight=width*9/16;Layout(cell);Layout(cell.renderer);
        NSArray *wideGeometry=HGBackdropNativeSnapshot(@[page.view,feed,cell,cell.contentView,cell.canvas,cell.renderer,cell.metal]);
        CGRect wideVideo=[cell.renderer convertRect:cell.renderer.bounds toView:page.view];
        BOOL widePresented=[cell.metal presentRed:0.08 green:0.84 blue:0.12];
        HGVideoBackdropRefreshForFeed(feed);
        CGPoint reflectedWideBottom=[portal convertPoint:CGPointMake(CGRectGetMidX(portal.bounds),CGRectGetMaxY(portal.bounds))
            toView:page.view];
        Check(feedHeight-CGRectGetMaxY(wideVideo)>90 &&
            fabs(reflectedWideBottom.y-feedHeight)<0.01 && HGBackdropPortalSource(portal)==cell.renderer &&
            HGBackdropNativeUnchanged(wideGeometry),
            @"a centered wide video mirrors its visible edge while the host letterbox and paging geometry stay intact");
        UIImage *wide=HGBackdropCompositorCapture(@"wide",window,nativeRect,extensionRect,@[]);
        HGBackdropRGB nativeWide=HGBackdropAverageColor(wide,window,nativeRect);
        HGBackdropRGB extendedWide=HGBackdropAverageColor(wide,window,extensionRect);
        HGBackdropWriteColorEvidence(@"wide",nativeWide,extendedWide,@[]);
        Check(widePresented && nativeWide.valid && nativeWide.green>MAX(nativeWide.red,nativeWide.blue)+0.35 &&
            extendedWide.valid && extendedWide.green>MAX(extendedWide.red,extendedWide.blue)+0.12 &&
            HGBackdropNativeUnchanged(wideGeometry),
            @"wide Metal video reaches the dock strip through the live compositor despite a native letterbox");
        cell.videoHeight=0;Layout(cell);Layout(cell.renderer);
        HGVideoBackdropRefreshForFeed(feed);
        [manager refreshWithBranches:branches root:container.view bar:bar active:NO];
        Check(!Find(page.view,@"honggo.video.backdrop") && HGBackdropNativeUnchanged(geometry),
            @"disabling the reflected strip removes only plugin surfaces and preserves native paging geometry");
        [manager refreshWithBranches:branches root:container.view bar:bar active:YES];
        cell.renderer.hidden=YES;
        [manager refreshWithBranches:branches root:container.view bar:bar active:YES];
        UIView *hiddenExtension=Find(page.view,@"honggo.video.backdrop");
        Check((!hiddenExtension || hiddenExtension.hidden) &&
            !HGBackdropPortalSource(Find(page.view,@"honggo.video.backdrop.portal")),
            @"a hidden native renderer clears its live source instead of displaying a stale backdrop frame");
        cell.renderer.hidden=NO;
        [manager refreshWithBranches:branches root:container.view bar:bar active:YES];
        UIView *previousExtension=Find(page.view,@"honggo.video.backdrop");
        UIView *previousPortal=Find(page.view,@"honggo.video.backdrop.portal");
        HGVideoBackdropPrepareRendererTransfer(cell.renderer);
        Check((!previousExtension.superview || previousExtension.hidden) && !HGBackdropPortalSource(previousPortal),
            @"renderer handoff clears the portal source and hides only the extension before host movement");
        UIView *destination=[[UIView alloc] initWithFrame:page.view.bounds];[container.view addSubview:destination];
        __block BOOL transferStayedClear=NO;
        __weak TTPlayerView *transferringRenderer=cell.renderer;
        cell.renderer.nativeWillMoveObserver=^{
            [manager refreshWithBranches:branches root:container.view bar:bar active:YES];
            transferStayedClear=HGAppearancePlayerIsTransferring(transferringRenderer) &&
                !HGBackdropPortalSource(Find(page.view,@"honggo.video.backdrop.portal"));
        };
        CGRect sourceFrame=cell.renderer.frame;[destination addSubview:cell.renderer];
        cell.renderer.nativeWillMoveObserver=nil;
        Check(transferStayedClear,
            @"a synchronous native willMove layout cannot reattach the renderer before its transfer completes");
        [manager refreshWithBranches:branches root:container.view bar:bar active:YES];
        [manager restore];
        Check(!Find(page.view,@"honggo.video.backdrop") && cell.renderer.superview==destination &&
            CGRectEqualToRect(cell.renderer.frame,sourceFrame) && CGRectEqualToRect(feed.frame,CGRectMake(0,0,width,feedHeight)) &&
            fabs(feed.contentOffset.y-feedHeight)<0.01,
            @"host renderer reparenting is never reversed and leaves the original native page offset intact");
        [cell.canvas insertSubview:cell.renderer atIndex:0];[destination removeFromSuperview];
        [manager refreshWithBranches:branches root:container.view bar:bar active:YES];
        [feed removeFromSuperview];
        HGVideoBackdropRefreshForFeed(feed);
        Check(!Find(page.view,@"honggo.video.backdrop") && !feed.superview &&
            CGRectEqualToRect(feed.frame,CGRectMake(0,0,width,feedHeight)),
            @"detaching the native feed releases its backdrop without pulling the feed back into its old parent");
    }@finally {
        [manager restore];feed.dataSource=nil;
        window.hidden=YES;window.rootViewController=nil;[originalWindow makeKeyAndVisible];
        HGAppearanceApply(saved,nil);
    }
}
