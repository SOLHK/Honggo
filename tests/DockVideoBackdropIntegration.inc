// Real CAMetalLayer drawables exercise the live compositor path. A UIView
// background or a CALayer render would not prove that video updates propagate.
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

@interface HGVideoBackdrop : NSObject
- (void)refreshWithBranches:(NSArray<UIView *> *)branches root:(UIView *)root bar:(UIView *)bar active:(BOOL)active;
- (void)restore;
@end

@interface FQVShortVideoListViewController : UIViewController @end
@implementation FQVShortVideoListViewController @end
@interface FQVPlayer : UIView @end
@implementation FQVPlayer @end

@interface FQVShortVideoPlayerCellView (HGBackdropPaletteFixture)
- (void)refreshBgColor;
@end
@implementation FQVShortVideoPlayerCellView (HGBackdropPaletteFixture)
- (void)refreshBgColor { self.backgroundColor=[UIColor colorWithRed:0.24 green:0.29 blue:0.32 alpha:1]; }
@end

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
@property(nonatomic,strong) UIView *videoWrapper;
@property(nonatomic,strong) FQVPlayer *player;
@property(nonatomic,strong) TTPlayerView *renderer;
@property(nonatomic,strong) HGBackdropMetalFixture *metal;
@property(nonatomic) CGFloat videoHeight;
@end
@implementation HGBackdropVideoCell
- (instancetype)initWithFrame:(CGRect)frame {
    if((self=[super initWithFrame:frame])) {
        self.canvas=[[FQVShortVideoPlayerCellView alloc] initWithFrame:self.contentView.bounds];
        self.videoWrapper=[[UIView alloc] initWithFrame:self.canvas.bounds];
        self.player=[[FQVPlayer alloc] initWithFrame:self.videoWrapper.bounds];
        self.renderer=[[TTPlayerView alloc] initWithFrame:self.player.bounds];
        for(UIView *view in self.renderer.subviews.copy)[view removeFromSuperview];
        self.metal=[[HGBackdropMetalFixture alloc] initWithFrame:self.renderer.bounds];
        [self.renderer addSubview:self.metal];[self.player addSubview:self.renderer];
        [self.videoWrapper addSubview:self.player];[self.canvas addSubview:self.videoWrapper];
        // Keep one positional HUD marker alongside the native renderer.
        UIView *hud=[[UIView alloc] initWithFrame:CGRectMake(0,0,frame.size.width,32)];
        hud.backgroundColor=UIColor.magentaColor;hud.autoresizingMask=UIViewAutoresizingFlexibleWidth;
        [self.canvas addSubview:hud];[self.contentView addSubview:self.canvas];
    }return self;
}
- (void)layoutSubviews {
    [super layoutSubviews];self.canvas.frame=self.contentView.bounds;
    self.videoWrapper.frame=self.canvas.bounds;self.player.frame=self.videoWrapper.bounds;
    CGRect video=self.player.bounds;
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
static UIImage *HGBackdropCompositorCapture(NSString *stage,UIWindow *window,CGRect nativeRect,CGRect extensionRect) {
    NSURL *documents=[NSFileManager.defaultManager URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask].firstObject;
    [NSFileManager.defaultManager createDirectoryAtURL:documents withIntermediateDirectories:YES attributes:nil error:nil];
    NSString *base=[@"video-backdrop-" stringByAppendingString:stage];
    NSURL *ready=[documents URLByAppendingPathComponent:[base stringByAppendingString:@"-ready.json"]];
    NSURL *captured=[documents URLByAppendingPathComponent:[base stringByAppendingString:@"-captured"]];
    NSURL *image=[documents URLByAppendingPathComponent:[base stringByAppendingString:@"-simulator.png"]];
    NSURL *active=[documents URLByAppendingPathComponent:@"video-backdrop-active-phase.txt"];
    [NSFileManager.defaultManager removeItemAtURL:captured error:nil];
    [NSFileManager.defaultManager removeItemAtURL:image error:nil];
    [window layoutIfNeeded];
    HGBackdropFlushFrames(0.35);
    NSDictionary *request=@{@"stage":stage,@"window":NSStringFromCGRect(window.bounds),
        @"nativeSample":NSStringFromCGRect(nativeRect),@"extensionSample":NSStringFromCGRect(extensionRect),
        @"source":@"native feed CAMetalDrawable and HUD; compositor capture required"};
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
    feed.pagingEnabled=YES;feed.clipsToBounds=YES;feed.backgroundColor=UIColor.blackColor;
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
        CGRect videoSample=CGRectMake(width*0.35,feedHeight*0.45,width*0.3,24);
        CGRect dockSample=CGRectMake(width*0.35,feedHeight+14,width*0.3,12);
        cell.renderer.clipsToBounds=YES;cell.player.clipsToBounds=YES;cell.canvas.clipsToBounds=YES;
        cell.canvas.backgroundColor=UIColor.blackColor;
        CGRect portraitFrame=cell.renderer.frame;
        NSArray *portraitGeometry=HGBackdropNativeSnapshot(@[page.view,feed,cell,cell.contentView,cell.canvas,
            cell.videoWrapper,cell.player,cell.metal]);
        BOOL portraitPresented=[cell.metal presentRed:0.91 green:0.06 blue:0.05];
        [manager refreshWithBranches:branches root:container.view bar:bar active:YES];
        CATransform3D fill=cell.renderer.layer.sublayerTransform;
        Check(!feed.clipsToBounds && !cell.canvas.clipsToBounds && !cell.player.clipsToBounds && !cell.renderer.clipsToBounds &&
            fabs(fill.m11-height/feedHeight)<0.001 && fabs(fill.m22-fill.m11)<0.001 && fabs(fill.m42-45)<0.001 &&
            CGRectEqualToRect(cell.renderer.frame,portraitFrame) && HGBackdropNativeUnchanged(portraitGeometry),
            @"a settled portrait renderer reaches the window with uniform magnification and native pager and HUD geometry");
        UIImage *portrait=HGBackdropCompositorCapture(@"portrait",window,videoSample,dockSample);
        HGBackdropRGB nativePortrait=HGBackdropAverageColor(portrait,window,videoSample);
        HGBackdropRGB dockPortrait=HGBackdropAverageColor(portrait,window,dockSample);
        Check(portraitPresented && nativePortrait.valid && nativePortrait.red>MAX(nativePortrait.green,nativePortrait.blue)+0.35 &&
            dockPortrait.valid && dockPortrait.red>MAX(dockPortrait.green,dockPortrait.blue)+0.35 &&
            HGBackdropNativeUnchanged(portraitGeometry),
            @"the settled portrait dock shows live upright native Metal pixels instead of a black band");

        // Exercise the actual player willMoveToSuperview hook: the native
        // renderer must see its original compositor before it is borrowed.
        __block BOOL restoredBeforeTransfer=NO;
        __weak HGBackdropVideoCell *movingCell=cell;
        cell.renderer.nativeWillMoveObserver=^{
            restoredBeforeTransfer=CATransform3DIsIdentity(movingCell.renderer.layer.sublayerTransform) &&
                movingCell.renderer.clipsToBounds && movingCell.player.clipsToBounds &&
                movingCell.canvas.clipsToBounds && feed.clipsToBounds;
        };
        UIView *temporary=[[UIView alloc] initWithFrame:cell.player.bounds];
        [cell.player addSubview:temporary];
        [temporary addSubview:cell.renderer];
        cell.renderer.nativeWillMoveObserver=nil;
        [cell.player addSubview:cell.renderer];
        [temporary removeFromSuperview];
        Check(restoredBeforeTransfer,@"native handoff sees original renderer and clipping before any reparenting");

        cell.videoHeight=width*9/16;Layout(cell);Layout(cell.renderer);
        cell.canvas.backgroundColor=[UIColor colorWithRed:0.28 green:0.18 blue:0.13 alpha:1];
        NSArray *geometry=HGBackdropNativeSnapshot(@[page.view,feed,cell,cell.contentView,cell.canvas,cell.renderer,cell.metal]);
        CGRect matteSample=CGRectMake(width*0.35,feedHeight-54,width*0.3,12);
        BOOL greenPresented=[cell.metal presentRed:0.08 green:0.84 blue:0.12];
        [manager refreshWithBranches:branches root:container.view bar:bar active:YES];
        UIImage *wide=HGBackdropCompositorCapture(@"wide",window,videoSample,dockSample);
        HGBackdropRGB nativeWide=HGBackdropAverageColor(wide,window,videoSample);
        HGBackdropRGB nativeMatte=HGBackdropAverageColor(wide,window,matteSample);
        HGBackdropRGB dockWide=HGBackdropAverageColor(wide,window,dockSample);
        Check(greenPresented && nativeWide.valid && nativeWide.green>MAX(nativeWide.red,nativeWide.blue)+0.35 &&
            nativeMatte.valid && nativeMatte.red>nativeMatte.blue+0.1 && dockWide.valid &&
            fabs(nativeMatte.red-dockWide.red)<0.03 && fabs(nativeMatte.green-dockWide.green)<0.03 &&
            fabs(nativeMatte.blue-dockWide.blue)<0.03 &&
            feed.clipsToBounds && HGBackdropNativeUnchanged(geometry),
            @"a centered wide video keeps its real RGB matte continuous past the feed edge into the dock strip");

        UIView *underlay=Find(page.view,@"honggo.video.matte");
        Check(underlay.superview==page.view && !underlay.userInteractionEnabled &&
            CGRectEqualToRect(underlay.frame,CGRectMake(0,feedHeight,width,90)) &&
            [page.view.subviews indexOfObjectIdenticalTo:underlay]<[page.view.subviews indexOfObjectIdenticalTo:feed],
            @"the matte occupies only the reserved strip behind native video and never receives touches");
        [cell.canvas refreshBgColor];
        Check(!underlay.hidden && [underlay.backgroundColor isEqual:cell.canvas.backgroundColor] &&
            HGBackdropNativeUnchanged(geometry),
            @"a native palette change updates the strip immediately on the same settled cell");
        cell.canvas.backgroundColor=[UIColor colorWithWhite:0.5 alpha:0.4];
        Layout(feed);
        BOOL translucentHidden=underlay.hidden;
        [cell.canvas refreshBgColor];
        BOOL restoredMatte=!underlay.hidden;
        cell.canvas.backgroundColor=nil;
        Layout(feed);
        Check(translucentHidden && restoredMatte && underlay.hidden,
            @"a translucent or absent native color cannot leave the previous matte in the dock strip");

        // Reveal the next real page with 52 points inside the original viewport.
        // A native HUD band at row 66 must appear 14 points below its edge.
        [feed setContentOffset:CGPointMake(0,feedHeight+52) animated:NO];Layout(feed);
        HGBackdropVideoCell *incoming=(id)[feed cellForItemAtIndexPath:[NSIndexPath indexPathForItem:2 inSection:0]];
        Check(incoming!=nil,@"native pagination lays out the incoming cell before showing its overflow");
        if(!incoming)return;
        Layout(incoming);Layout(incoming.renderer);
        UIView *marker=[[UIView alloc] initWithFrame:CGRectMake(0,66,width,14)];
        marker.backgroundColor=UIColor.yellowColor;
        [incoming.canvas addSubview:marker];
        [incoming.canvas refreshBgColor];
        BOOL bluePresented=[incoming.metal presentRed:0.05 green:0.08 blue:0.92];
        NSArray *swipeGeometry=HGBackdropNativeSnapshot(@[page.view,feed,incoming,incoming.contentView,
            incoming.canvas,incoming.renderer,incoming.metal,marker]);
        Check(!feed.clipsToBounds && !underlay.hidden && [underlay.backgroundColor isEqual:incoming.canvas.backgroundColor] &&
            HGBackdropNativeUnchanged(swipeGeometry),
            @"a crossing native page draws below the collection without changing its frames offset or paging stride");
        CGRect incomingSample=CGRectMake(width*0.35,feedHeight-13,width*0.3,9);
        UIImage *swipe=HGBackdropCompositorCapture(@"swipe",window,incomingSample,dockSample);
        HGBackdropRGB nativeBlue=HGBackdropAverageColor(swipe,window,incomingSample);
        HGBackdropRGB markerColor=HGBackdropAverageColor(swipe,window,dockSample);
        HGBackdropRGB dockBlue=HGBackdropAverageColor(swipe,window,CGRectOffset(dockSample,0,32));
        Check(bluePresented && nativeBlue.valid && nativeBlue.blue>MAX(nativeBlue.red,nativeBlue.green)+0.30 &&
            markerColor.valid && MIN(markerColor.red,markerColor.green)>markerColor.blue+0.35 &&
            dockBlue.valid && dockBlue.blue>MAX(dockBlue.red,dockBlue.green)+0.30 &&
            HGBackdropNativeUnchanged(swipeGeometry),
            @"compositor shows the upright incoming native HUD at its actual position over the live Metal frame");

        [manager refreshWithBranches:branches root:container.view bar:bar active:NO];
        Check(feed.clipsToBounds && !underlay.superview && HGBackdropNativeUnchanged(swipeGeometry),
            @"disabling floating navigation restores feed clipping without repagination");
        CAShapeLayer *hostMask=[CAShapeLayer layer];
        hostMask.frame=feed.bounds;
        CGPathRef maskPath=CGPathCreateWithRect((CGRect){CGPointZero,feed.bounds.size},NULL);
        hostMask.path=maskPath;CGPathRelease(maskPath);
        feed.layer.mask=hostMask;
        [manager refreshWithBranches:branches root:container.view bar:bar active:YES];
        Check(feed.layer.mask==hostMask && feed.clipsToBounds,
            @"a host-owned clipping mask is retained during a partial swipe");
        feed.layer.mask=nil;
        [manager refreshWithBranches:branches root:container.view bar:bar active:YES];

        incoming.videoHeight=width*9/16;
        Layout(incoming);
        [feed setContentOffset:CGPointMake(0,feedHeight*2) animated:NO];Layout(feed);
        Check(feed.clipsToBounds && fabs(feed.contentOffset.y-feedHeight*2)<0.01,
            @"settling on a native page restores clipping before preloaded neighbors can show through");
        [feed setContentOffset:CGPointMake(0,feedHeight+52) animated:NO];Layout(feed);
        Check(!feed.clipsToBounds,@"native feed layout opens clipping again for a second partial swipe");
        [feed removeFromSuperview];
        Check(feed.clipsToBounds && !feed.superview && CGRectEqualToRect(feed.frame,CGRectMake(0,0,width,feedHeight)),
            @"detaching the feed restores its clip without moving it back into the window");
    }@finally {
        [manager restore];feed.dataSource=nil;
        window.hidden=YES;window.rootViewController=nil;[originalWindow makeKeyAndVisible];
        HGAppearanceApply(saved,nil);
    }
}
