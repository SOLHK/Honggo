// Real CAMetalLayer drawables exercise the live compositor path. A UIView
// background or a CALayer render would not prove that video updates propagate.
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

@interface HGVideoBackdrop : NSObject
- (void)refreshWithBranches:(NSArray<UIView *> *)branches root:(UIView *)root bar:(UIView *)bar active:(BOOL)active;
- (void)restore;
@end

@interface FQVShortVideoLockPlayRateTipsView : UIView @end
@implementation FQVShortVideoLockPlayRateTipsView @end
@interface FQVShortVideoListVCScrollComponent : NSObject <UIScrollViewDelegate> @end
@implementation FQVShortVideoListVCScrollComponent
- (void)scrollViewDidScroll:(UIScrollView *)scrollView {}
- (void)scrollViewDidEndDecelerating:(UIScrollView *)scrollView {}
- (void)scrollViewDidEndScrollingAnimation:(UIScrollView *)scrollView {}
- (void)scrollViewDidEndDragging:(UIScrollView *)scrollView willDecelerate:(BOOL)decelerate {}
@end
@interface FQVShortVideoListViewController : UIViewController @end
@implementation FQVShortVideoListViewController @end
// Surface a collection-layout query made by the plugin while cells are in
// flight. UIKit itself can query visibleCells during its own layout.
@interface HGBackdropFeedProbe : FQVCollectionView
@property(nonatomic) BOOL countVisibleQueries;
@property(nonatomic) NSUInteger visibleQueries;
@end
@implementation HGBackdropFeedProbe
- (NSArray<UICollectionViewCell *> *)visibleCells {
    if (self.countVisibleQueries) self.visibleQueries++;
    return [super visibleCells];
}
@end
@interface FQVPlayer : UIView
@property(nonatomic,strong) UIImageView *coverImageView;
@property(nonatomic,strong) id model;
@end
@implementation FQVPlayer @end
@interface HGBackdropFrameModel : NSObject
@property(nonatomic) BOOL readyToDisPlay;
@end
@implementation HGBackdropFrameModel @end
@interface FQVShortVideoGradientLayerView : UIView @end
@implementation FQVShortVideoGradientLayerView
+ (Class)layerClass { return CAGradientLayer.class; }
@end
@interface HGBackdropGradientControl : FQVShortVideoVPlayerControlView
@property(nonatomic,strong) FQVShortVideoGradientLayerView *bottomGradientView;
@end
@implementation HGBackdropGradientControl @end
@interface FQVShortVideoVPlayerSingleSeriesControlView : HGBackdropGradientControl
@property(nonatomic,strong) FQVShortVideoPlayerBottomView *singleSeriesBottomView;
@end
@implementation FQVShortVideoVPlayerSingleSeriesControlView @end
@interface HGAAspectSourceFixture : NSObject
@property(nonatomic) double videoWidth;
@property(nonatomic) double videoHeight;
@end
@implementation HGAAspectSourceFixture
- (id)shortVideoModel {return self;}
@end

@interface FQVShortVideoPlayerCellView (HGBackdropPaletteFixture)
@property(nonatomic,strong) UIImageView *coverImageView;
- (void)refreshBgColor;
- (void)refreshCoverImageView;
- (void)vPlayerReadyToDisPlay:(FQVPlayer *)player;
@end
@implementation FQVShortVideoPlayerCellView (HGBackdropPaletteFixture)
static char HGBackdropCellCoverKey;
- (UIImageView *)coverImageView {return objc_getAssociatedObject(self,&HGBackdropCellCoverKey);}
- (void)setCoverImageView:(UIImageView *)cover {
    objc_setAssociatedObject(self,&HGBackdropCellCoverKey,cover,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
- (void)refreshCoverImageView {}
- (void)refreshBgColor { self.backgroundColor=[UIColor colorWithRed:0.24 green:0.29 blue:0.32 alpha:1]; }
- (void)vPlayerReadyToDisPlay:(FQVPlayer *)player { self.coverImageView.hidden=YES; }
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
@property(nonatomic) CGFloat metalOverscanX;
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
    self.renderer.frame=video;self.metal.frame=CGRectInset(self.renderer.bounds,-self.metalOverscanX,0);
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
@interface FQVShortVideoVPlayerVideoListControlView : FQVShortVideoVPlayerMultiSeriesControlView @end
@implementation FQVShortVideoVPlayerVideoListControlView
- (CGFloat)bottomHeight { return 89; }
@end

static void RunInnerAlbumBackdropChecks(UIView *root) {
    UIViewController *parent=(UIViewController *)root.nextResponder;
    UIViewController *album=[FQVShortVideoListViewController new];
    [parent addChildViewController:album];album.view.frame=root.bounds;
    [root addSubview:album.view];[album didMoveToParentViewController:parent];
    FQVShortVideoPlayerCellView *canvas=[[FQVShortVideoPlayerCellView alloc] initWithFrame:album.view.bounds];
    UIView *wrapper=[[UIView alloc] initWithFrame:canvas.bounds];
    FQVPlayer *player=[[FQVPlayer alloc] initWithFrame:wrapper.bounds];
    // 14.mp4 has an 880x1920 screen; the authored portrait is visible between
    // y=77 and y=1656. Its gray captions and landscape inset are part of that picture.
    CGFloat w=CGRectGetWidth(canvas.bounds),h=CGRectGetHeight(canvas.bounds);
    CGRect original=CGRectMake(0,h*77/1920,w,h*(1656-77)/1920);
    TTPlayerView *renderer=[[TTPlayerView alloc] initWithFrame:original];
    for (UIView *sub in renderer.subviews.copy) [sub removeFromSuperview];
    TTMetalView *metal=[[TTMetalView alloc] initWithFrame:renderer.bounds];
    renderer.clipsToBounds=YES;
    [renderer addSubview:metal];[player addSubview:renderer];[wrapper addSubview:player];[canvas addSubview:wrapper];
    FQVShortVideoVPlayerVideoListControlView *hud=[[FQVShortVideoVPlayerVideoListControlView alloc] initWithFrame:canvas.bounds];
    [canvas addSubview:hud];[album.view addSubview:canvas];Layout(hud);
    CGRect painted=[metal.layer convertRect:metal.layer.bounds toLayer:canvas.layer];
    CATransform3D fit=renderer.layer.sublayerTransform;
    Check(CGRectGetMinY(painted)<=0.75 && CGRectGetMaxY(painted)>=h-0.75 &&
        CGRectGetMinX(painted)<=0.75 && CGRectGetMaxX(painted)>=w-0.75 &&
        fabs(fit.m11-fit.m22)<0.001 && CGRectEqualToRect(renderer.frame,original) &&
        CGRectEqualToRect(hud.frame,canvas.bounds) && !renderer.clipsToBounds,
        @"a width-fitted album episode fills its detail page without stretching the media or moving its controls");
    [renderer removeFromSuperview];
    Check(CATransform3DIsIdentity(renderer.layer.sublayerTransform) && renderer.clipsToBounds,
        @"leaving album detail releases the picture transform and clip before renderer reuse");
    renderer.frame=CGRectMake(0,CGRectGetMidY(original)-w*9/32,w,w*9/16);
    metal.frame=renderer.bounds;
    [player addSubview:renderer];HGRefreshVideoBackdropForPlayer(renderer);
    Check(CATransform3DIsIdentity(renderer.layer.sublayerTransform) && renderer.clipsToBounds,
        @"an actual landscape renderer retains its native aspect ratio and letterbox in the same album");
    [album willMoveToParentViewController:nil];[album.view removeFromSuperview];[album removeFromParentViewController];
}

static void RunDetailVideoBackdropChecks(UIView *root) {
    RunInnerAlbumBackdropChecks(root);
    FQVShortVideoPlayerCellView *canvas=[[FQVShortVideoPlayerCellView alloc] initWithFrame:root.bounds];
    canvas.clipsToBounds=YES;
    canvas.backgroundColor=[UIColor colorWithRed:0.12 green:0.14 blue:0.11 alpha:1];
    CGRect crop=canvas.bounds;crop.size.height-=89;
    UIView *wrapper=[[UIView alloc] initWithFrame:canvas.bounds];
    FQVPlayer *player=[[FQVPlayer alloc] initWithFrame:wrapper.bounds];
    TTPlayerView *renderer=[[TTPlayerView alloc] initWithFrame:crop];
    for(UIView *view in renderer.subviews.copy)[view removeFromSuperview];
    TTMetalView *metal=[[TTMetalView alloc] initWithFrame:CGRectInset(renderer.bounds,-24,0)];
    [renderer addSubview:metal];renderer.clipsToBounds=YES;
    [player addSubview:renderer];[wrapper addSubview:player];[canvas addSubview:wrapper];
    UIView *cornerHolder=[[UIView alloc] initWithFrame:crop];cornerHolder.userInteractionEnabled=NO;
    UIImageView *leftBottom=nil,*rightBottom=nil,*leftTop=nil;
    for(NSUInteger i=0;i<4;i++) {
        UIImageView *image=[[UIImageView alloc] initWithFrame:CGRectMake(i%2?crop.size.width-12:0,i>=2?crop.size.height-12:0,12,12)];
        [cornerHolder addSubview:image];
        if(i==0)leftTop=image;
        if(i==2)leftBottom=image;
        if(i==3)rightBottom=image;
    }
    [player addSubview:cornerHolder];
    FQVShortVideoVPlayerSingleSeriesControlView *hud=[[FQVShortVideoVPlayerSingleSeriesControlView alloc] initWithFrame:canvas.bounds];
    CGRect bottom=CGRectMake(0,crop.size.height,crop.size.width,89);
    hud.singleSeriesBottomView=[[FQVShortVideoPlayerBottomView alloc] initWithFrame:bottom];
    hud.singleSeriesBottomView.backgroundColor=UIColor.blackColor;
    [hud addSubview:hud.singleSeriesBottomView];[canvas addSubview:hud];[root addSubview:canvas];
    Layout(hud);
    CATransform3D fit=renderer.layer.sublayerTransform;
    CGFloat scale=MAX(canvas.bounds.size.width/metal.bounds.size.width,canvas.bounds.size.height/metal.bounds.size.height);
    CGRect fitted=[metal.layer convertRect:metal.layer.bounds toLayer:player.layer];
    Check(CATransform3DIsIdentity(wrapper.layer.sublayerTransform) &&
        fabs(fit.m11-scale)<0.001 && fabs(fit.m22-scale)<0.001 &&
        CGRectGetMinX(fitted)<=0.75 && CGRectGetMaxX(fitted)>=crop.size.width-0.75 &&
        fabs(CGRectGetMinY(fitted))<0.75 && fabs(CGRectGetMaxY(fitted)-CGRectGetMaxY(canvas.bounds))<0.75 &&
        CGRectEqualToRect(renderer.frame,crop) && CGRectEqualToRect(hud.singleSeriesBottomView.frame,bottom) &&
        canvas.clipsToBounds && !renderer.clipsToBounds,
        @"portrait detail fills the screen behind its episode tray using the minimum uniform scale");
    Check(leftBottom.alpha==0 && rightBottom.alpha==0 && leftTop.alpha==1 &&
        [hud.singleSeriesBottomView.backgroundColor isEqual:UIColor.clearColor],
        @"detail continuation clears the matched bottom cutouts and old black tray paint");
    canvas.center=CGPointMake(canvas.center.x,canvas.center.y+240);
    HGRefreshVideoBackdropForPlayer(canvas);
    Check(CATransform3DEqualToTransform(renderer.layer.sublayerTransform,fit),
        @"a partially visible incoming detail cell already uses its settled crop");
    [renderer removeFromSuperview];
    Check(CATransform3DIsIdentity(renderer.layer.sublayerTransform) && renderer.clipsToBounds &&
        leftBottom.alpha==1 && rightBottom.alpha==1 && [hud.singleSeriesBottomView.backgroundColor isEqual:UIColor.blackColor],
        @"renderer transfer restores the native detail crop, corner images and tray fill");
    hud.singleSeriesBottomView.frame=CGRectZero;
    hud.playerViewFrame=crop;
    HGAAspectSourceFixture *source=[HGAAspectSourceFixture new];source.videoWidth=720;source.videoHeight=1280;
    canvas.model=source;
    UIImageView *cover=[[UIImageView alloc] initWithFrame:crop];
    cover.contentMode=UIViewContentModeScaleAspectFit;
    player.coverImageView=cover;
    [player addSubview:cover];HGRefreshVideoBackdropForPlayer(canvas);
    Check(cover.contentMode==UIViewContentModeScaleAspectFill && CGRectEqualToRect(cover.frame,canvas.bounds) &&
        CATransform3DIsIdentity(wrapper.layer.sublayerTransform) &&
        !renderer.superview && CGRectIsEmpty(hud.singleSeriesBottomView.frame),
        @"a known episode fills behind the controls before its first Metal frame or tray layout arrives");
    [player addSubview:renderer];hud.singleSeriesBottomView.frame=bottom;Layout(hud);
    Check(CATransform3DEqualToTransform(renderer.layer.sublayerTransform,fit) &&
        cover.contentMode==UIViewContentModeScaleAspectFill && CGRectEqualToRect(cover.frame,canvas.bounds) &&
        CATransform3DIsIdentity(wrapper.layer.sublayerTransform),
        @"the first Metal frame and the poster fill the same screen viewport with the same anchoring");
    // Overscan poster frames (matching TTMetalView aspect-fill sizing) and
    // incoming detail cells without a transferred HUD yet must use the exact
    // same full-screen anchor so settling never jumps +44.5pt on handoff.
    HGVideoBackdropPrepareRendererTransfer(player);
    [renderer removeFromSuperview];
    cover.frame=CGRectInset(crop,-24,0);
    cover.contentMode=UIViewContentModeScaleAspectFill;
    HGRefreshVideoBackdropForPlayer(canvas);
    Check(CGRectEqualToRect(cover.frame,canvas.bounds),
        @"an overscanned portrait poster centered on the detail viewport aligns to full screen before Metal arrives");
    FQVShortVideoPlayerCellView *incomingDetail=[[FQVShortVideoPlayerCellView alloc] initWithFrame:root.bounds];
    incomingDetail.model=source;
    UIImageView *incomingCover=[[UIImageView alloc] initWithFrame:CGRectInset(crop,-24,0)];
    incomingCover.contentMode=UIViewContentModeScaleAspectFill;
    incomingDetail.coverImageView=incomingCover;
    [incomingDetail addSubview:incomingCover];
    [root addSubview:incomingDetail];
    HGRefreshVideoBackdropForPlayer(incomingDetail);
    Check(CGRectEqualToRect(incomingCover.frame,incomingDetail.bounds),
        @"an incoming detail episode without its transferred HUD inherits the verified bottom tray viewport from frame zero");
    [incomingDetail removeFromSuperview];
    [player addSubview:renderer];hud.singleSeriesBottomView.frame=CGRectMake(0,0,crop.size.width,89);
    HGRefreshVideoBackdropForPlayer(canvas);
    Check(CATransform3DEqualToTransform(renderer.layer.sublayerTransform,fit),
        @"a detail episode whose bottom tray is temporarily at y=0 before AutoLayout still fills from its native renderer viewport on frame zero");
    hud.singleSeriesBottomView.frame=bottom;Layout(hud);
    HGRefreshVideoBackdropForPlayer(canvas);
    HGVideoBackdropPrepareRendererTransfer(player);
    Check(cover.contentMode==UIViewContentModeScaleAspectFill && CGRectEqualToRect(cover.frame,CGRectInset(crop,-24,0)) &&
        CATransform3DIsIdentity(renderer.layer.sublayerTransform),
        @"player recycling restores the native overscan poster frame and Metal transform");
    [renderer removeFromSuperview];
    HGAAspectSourceFixture *wide=[HGAAspectSourceFixture new];wide.videoWidth=1920;wide.videoHeight=1080;canvas.model=wide;
    Check(CATransform3DIsIdentity(wrapper.layer.sublayerTransform),@"a wide source does not acquire a speculative portrait crop");
    [canvas removeFromSuperview];
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
    HGBackdropFeedProbe *feed=[[HGBackdropFeedProbe alloc] initWithFrame:CGRectMake(0,0,width,feedHeight) collectionViewLayout:layout];
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
    UIImageView *leftCorner=[[UIImageView alloc] initWithFrame:CGRectMake(0,feedHeight-12,12,12)];
    UIImageView *rightCorner=[[UIImageView alloc] initWithFrame:CGRectMake(width-12,feedHeight-12,12,12)];
    UIImageView *otherImage=[[UIImageView alloc] initWithFrame:CGRectMake(14,feedHeight-12,12,12)];
    leftCorner.backgroundColor=UIColor.blackColor;rightCorner.backgroundColor=UIColor.blackColor;
    [page.view addSubview:leftCorner];[page.view addSubview:rightCorner];[page.view addSubview:otherImage];
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
        HGBackdropGradientControl *hud=[[HGBackdropGradientControl alloc] initWithFrame:cell.canvas.bounds];
        hud.bottomGradientView=[[FQVShortVideoGradientLayerView alloc] initWithFrame:CGRectMake(0,feedHeight-280,width,280)];
        hud.userInteractionEnabled=NO;[hud addSubview:hud.bottomGradientView];[cell.canvas addSubview:hud];
        [hud handleOutterFeedClipBounds:YES];
        Check(hud.nativeClipPasses==1 && feed.clipsToBounds && cell.canvas.clipsToBounds,
            @"the seek close keeps original clipping when no dock backdrop is attached");
        CAGradientLayer *gradient=(CAGradientLayer *)hud.bottomGradientView.layer;
        NSArray *stockGradient=@[(__bridge id)UIColor.clearColor.CGColor,
            (__bridge id)[UIColor colorWithWhite:0 alpha:0.2].CGColor];
        gradient.colors=stockGradient;gradient.locations=@[@0,@1];
        gradient.startPoint=CGPointMake(0.5,0);gradient.endPoint=CGPointMake(0.5,1);

        UIImage *firstPoster=[[[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(16,28)]
            imageWithActions:^(UIGraphicsImageRendererContext *context) {
                [UIColor.cyanColor setFill];UIRectFill(CGRectMake(0,0,16,28));
            }];
        UIImageView *firstCover=[[UIImageView alloc] initWithImage:firstPoster];
        firstCover.frame=cell.canvas.bounds;cell.canvas.coverImageView=firstCover;
        [cell.videoWrapper removeFromSuperview];
        [cell.canvas insertSubview:firstCover belowSubview:hud];
        [manager refreshWithBranches:branches root:container.view bar:bar active:YES];
        Check(CGRectGetHeight(firstCover.frame)==height && !feed.clipsToBounds && !cell.videoWrapper.superview,
            @"cold playback can fill behind the Dock using the cell-owned poster before the player exists");
        [cell.canvas insertSubview:cell.videoWrapper belowSubview:firstCover];
        cell.renderer.layer.sublayerTransform=CATransform3DIdentity;
        [cell.canvas vPlayerReadyToDisPlay:cell.player];
        Check(firstCover.hidden && fabs(cell.renderer.layer.sublayerTransform.m42-45)<0.75 && !feed.clipsToBounds,
            @"the renderer reaches its full viewport synchronously when the native callback hides the cold poster");
        [firstCover removeFromSuperview];cell.canvas.coverImageView=nil;

        cell.videoHeight=width*16.0/9.0;
        Layout(cell);
        CGRect insetRenderer=cell.renderer.frame;
        [manager refreshWithBranches:branches root:container.view bar:bar active:YES];
        CGRect insetPicture=[cell.metal.layer convertRect:cell.metal.layer.bounds toLayer:cell.canvas.layer];
        Check(fabs(CGRectGetMinY(insetPicture))<0.75 && fabs(CGRectGetMaxY(insetPicture)-height)<0.75 &&
            CGRectGetMinX(insetPicture)<=0 && CGRectGetMaxX(insetPicture)>=width &&
            CGRectEqualToRect(cell.renderer.frame,insetRenderer) && !cell.renderer.clipsToBounds,
            @"centered 9:16 home previews reach both screen edges while retaining the native renderer frame");
        cell.videoHeight=0;
        cell.metalOverscanX=24;Layout(cell);
        CGRect portraitFrame=cell.renderer.frame;
        CGRect nativeMetalBounds=cell.metal.bounds;
        CGPoint nativeMetalCenter=cell.metal.center;
        NSArray *portraitGeometry=HGBackdropNativeSnapshot(@[page.view,feed,cell,cell.contentView,cell.canvas,
            cell.player,cell.videoWrapper]);
        BOOL portraitPresented=[cell.metal presentRed:0.91 green:0.06 blue:0.05];
        feed.countVisibleQueries = YES;
        [manager refreshWithBranches:branches root:container.view bar:bar active:YES];
        feed.countVisibleQueries = NO;
        Check(feed.visibleQueries == 0, @"backdrop uses mounted cells without forcing collection layout");
        CATransform3D fit=cell.renderer.layer.sublayerTransform;
        CGRect nativePicture=[cell.metal.layer convertRect:cell.metal.layer.bounds toLayer:cell.canvas.layer];
        CGFloat scale=height/feedHeight;
        Check(!feed.clipsToBounds && !cell.canvas.clipsToBounds && !cell.player.clipsToBounds && !cell.renderer.clipsToBounds &&
            fabs(fit.m11-scale)<0.001 && fabs(fit.m22-fit.m11)<0.001 && fabs(fit.m42-45)<0.001 &&
            CGRectGetMinX(nativePicture)<=0.75 && CGRectGetMaxX(nativePicture)>=width-0.75 &&
            fabs(CGRectGetMinY(nativePicture))<=0.75 && fabs(CGRectGetMaxY(nativePicture)-height)<=0.75 &&
            CATransform3DIsIdentity(cell.videoWrapper.layer.sublayerTransform) &&
            CGRectEqualToRect(cell.renderer.frame,portraitFrame) && HGBackdropNativeUnchanged(portraitGeometry) &&
            cell.layer.zPosition==1,
            @"a settled portrait paints from the status bar through the Dock without resizing the pager or HUD");
        bar.alpha=0;
        [manager refreshWithBranches:branches root:container.view bar:bar active:YES];
        Check(bar.alpha==0 && !feed.clipsToBounds && CATransform3DEqualToTransform(cell.renderer.layer.sublayerTransform,fit),
            @"fading out the Dock leaves the portrait geometry untouched before the next frame");
        [manager refreshWithBranches:branches root:container.view bar:bar active:YES];
        bar.hidden=YES;bar.alpha=1;
        HGAppearanceSet(@"dock.enabled",@NO,nil);
        HGRefreshVideoBackdropForFeed(feed);
        Check(!feed.clipsToBounds &&
            CATransform3DEqualToTransform(cell.renderer.layer.sublayerTransform,fit),
            @"an actually hidden Dock and disabled custom Dock preference preserve the same portrait coverage");
        HGAppearanceSet(@"dock.enabled",@YES,nil);
        bar.hidden=NO;
        [manager refreshWithBranches:branches root:container.view bar:bar active:YES];
        [hud handleOutterFeedClipBounds:NO];
        BOOL previewUnclipped=!feed.clipsToBounds && !cell.canvas.clipsToBounds;
        [hud handleOutterFeedClipBounds:YES];
        HGBackdropFlushFrames(0.04);
        Check(hud.nativeClipPasses==3 && previewUnclipped && !feed.clipsToBounds && !cell.canvas.clipsToBounds &&
            CATransform3DEqualToTransform(cell.renderer.layer.sublayerTransform,fit) && HGBackdropNativeUnchanged(portraitGeometry),
            @"native seek preview and completion preserve the live portrait backdrop and pager geometry");
        // Reselecting Home recycles the video surface. The host may attach
        // Metal before assigning its final bounds, without scrolling again.
        [cell.metal removeFromSuperview];
        cell.metal.frame=CGRectZero;
        [cell.renderer addSubview:cell.metal];
        HGBackdropFlushFrames(0.03);
        cell.metal.frame=CGRectMake(nativeMetalCenter.x-nativeMetalBounds.size.width/2,
            nativeMetalCenter.y-nativeMetalBounds.size.height/2,nativeMetalBounds.size.width,nativeMetalBounds.size.height);
        Check(!feed.clipsToBounds && CATransform3DEqualToTransform(cell.renderer.layer.sublayerTransform,fit),
            @"late Metal sizing restores the portrait backdrop before the next display transaction");
        // A reselected Home feed may show a poster with the retained renderer
        // fully faded. Match the observed 0 -> 1 handoff and check the image,
        // HUD and material separately at native screen coordinates.
        UIGraphicsImageRenderer *painter=[[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(8,16)];
        UIImage *poster=[painter imageWithActions:^(UIGraphicsImageRendererContext *context) {
            [UIColor.greenColor setFill];UIRectFill(CGRectMake(0,0,8,16));
        }];
        UIImageView *cover=[[UIImageView alloc] initWithImage:poster];
        cover.frame=CGRectInset(cell.player.bounds,-24,0);cell.player.coverImageView=cover;
        [cell.player addSubview:cover];cell.renderer.alpha=0;
        [cell.canvas refreshCoverImageView];HGBackdropFlushFrames(0.04);
        [cell.renderer removeFromSuperview];
        [cell.canvas refreshCoverImageView];HGBackdropFlushFrames(0.04);
        CGRect posterFrame=[cover.layer convertRect:cover.layer.bounds toLayer:window.layer];
        UIView *matteView=Find(feed.superview,@"honggo.video.matte");
        Check(!feed.clipsToBounds && fabs(CGRectGetMinY(posterFrame))<=0.75 && fabs(CGRectGetMaxY(posterFrame)-height)<=0.75 &&
            cover.contentMode==UIViewContentModeScaleAspectFill && (!matteView || matteView.hidden) &&
            CGRectEqualToRect(hud.frame,cell.canvas.bounds),
            @"the portrait poster remains full screen with the renderer detached and no painted tray behind it");
        [cell.player insertSubview:cell.renderer belowSubview:cover];cell.renderer.alpha=0.35;cover.alpha=0.65;
        [cell.canvas refreshCoverImageView];HGBackdropFlushFrames(0.04);
        CGRect handoffFrame=[cell.metal.layer convertRect:cell.metal.layer.bounds toLayer:window.layer];
        Check(fabs(CGRectGetMinY(posterFrame)-CGRectGetMinY(handoffFrame))<=0.75 &&
            fabs(CGRectGetMaxY(posterFrame)-CGRectGetMaxY(handoffFrame))<=0.75 && !feed.clipsToBounds &&
            CATransform3DEqualToTransform(cell.renderer.layer.sublayerTransform,fit),
            @"the retained renderer and its fading cover reach the same edge at intermediate alpha");
        [cover removeFromSuperview];cell.player.coverImageView=nil;cell.renderer.alpha=1;
        portraitPresented=[cell.metal presentRed:1 green:0 blue:0];
        UIImage *portrait=HGBackdropCompositorCapture(@"portrait",window,videoSample,dockSample);
        HGBackdropRGB nativePortrait=HGBackdropAverageColor(portrait,window,videoSample);
        HGBackdropRGB dockPortrait=HGBackdropAverageColor(portrait,window,dockSample);
        HGBackdropRGB leftEdge=HGBackdropAverageColor(portrait,window,CGRectMake(2,CGRectGetMidY(nativePicture),5,5));
        HGBackdropRGB rightEdge=HGBackdropAverageColor(portrait,window,CGRectMake(width-7,CGRectGetMidY(nativePicture),5,5));
        HGBackdropRGB upperEdge=HGBackdropAverageColor(portrait,window,CGRectMake(width*0.76,2,5,5));
        HGBackdropRGB lowerEdge=HGBackdropAverageColor(portrait,window,CGRectMake(width*0.76,height-7,5,5));
        HGBackdropRGB beforeSeam=HGBackdropAverageColor(portrait,window,CGRectMake(width*0.35,feedHeight-3,width*0.3,2));
        HGBackdropRGB afterSeam=HGBackdropAverageColor(portrait,window,CGRectMake(width*0.35,feedHeight+1,width*0.3,2));
        Check(portraitPresented && nativePortrait.valid && nativePortrait.red>MAX(nativePortrait.green,nativePortrait.blue)+0.35 &&
            dockPortrait.valid && dockPortrait.red>MAX(dockPortrait.green,dockPortrait.blue)+0.35 &&
            HGBackdropNativeUnchanged(portraitGeometry),
            @"the live portrait appears behind the Dock instead of a black or colored underlay");
        Check(leftCorner.alpha==0 && rightCorner.alpha==0 && otherImage.alpha==1 &&
            leftEdge.valid && leftEdge.red>MAX(leftEdge.green,leftEdge.blue)+0.35 &&
            rightEdge.valid && rightEdge.red>MAX(rightEdge.green,rightEdge.blue)+0.35 &&
            upperEdge.valid && upperEdge.red>MAX(upperEdge.green,upperEdge.blue)+0.35 &&
            lowerEdge.valid && lowerEdge.red>MAX(lowerEdge.green,lowerEdge.blue)+0.35,
            @"the compositor contains video pixels at both sides and at the physical top and bottom");
        Check(gradient.opacity==0 && gradient.colors.count==2 &&
            beforeSeam.valid && afterSeam.valid && fabs(beforeSeam.red-afterSeam.red)<0.06,
            @"the old HUD gradient no longer darkens the portrait or exposes a seam at the original tray edge");

        FQVShortVideoListVCScrollComponent *scrollDriver=[FQVShortVideoListVCScrollComponent new];
        [feed setContentOffset:CGPointMake(0,feedHeight+60) animated:NO];Layout(feed);
        HGBackdropVideoCell *nextPortrait=(id)[feed cellForItemAtIndexPath:[NSIndexPath indexPathForItem:2 inSection:0]];
        HGBackdropFrameModel *frameModel=[HGBackdropFrameModel new];
        nextPortrait.player.model=frameModel;
        [scrollDriver scrollViewDidScroll:feed];
        Check(nextPortrait && cell.layer.zPosition==1 && nextPortrait.layer.zPosition==0 &&
            CATransform3DEqualToTransform(cell.renderer.layer.sublayerTransform,fit),
            @"scrollViewDidScroll synchronously orders the outgoing portrait above an unready neighbor without shifting its transform");
        frameModel.readyToDisPlay=YES;
        [nextPortrait.canvas vPlayerReadyToDisPlay:nextPortrait.player];
        Check(cell.layer.zPosition==0 && nextPortrait.layer.zPosition==1 &&
            CATransform3DEqualToTransform(cell.renderer.layer.sublayerTransform,fit) &&
            fabs(nextPortrait.renderer.layer.sublayerTransform.m42-fit.m42)<0.75,
            @"native first-frame arrival hands over drawing order cleanly while both cells maintain constant 1:1 scroll transforms");
        [feed setContentOffset:CGPointMake(0,feedHeight+80) animated:NO];Layout(feed);
        [scrollDriver scrollViewDidScroll:feed];
        Check(CATransform3DEqualToTransform(cell.renderer.layer.sublayerTransform,fit) &&
            fabs(cell.renderer.layer.sublayerTransform.m11-fit.m11)<0.001,
            @"outgoing placement follows scrolling 1:1 with constant zoom and zero rubber-band shift");
        nextPortrait.player.model=nil;
        [feed setContentOffset:CGPointMake(0,feedHeight) animated:NO];Layout(feed);
        [manager refreshWithBranches:branches root:container.view bar:bar active:YES];

        [feed setContentOffset:CGPointMake(0,feedHeight+60) animated:NO];Layout(feed);
        HGBackdropVideoCell *wideNeighbor=(id)[feed cellForItemAtIndexPath:[NSIndexPath indexPathForItem:2 inSection:0]];
        wideNeighbor.videoHeight=width*9/16;Layout(wideNeighbor);Layout(wideNeighbor.renderer);
        wideNeighbor.canvas.backgroundColor=[UIColor colorWithWhite:0.10 alpha:1];
        CGFloat pictureArrival=(feedHeight-wideNeighbor.videoHeight)/2;
        BOOL stableCoverage=wideNeighbor!=nil && pictureArrival>100;
        CGFloat offsets[]={60,90,pictureArrival-7,pictureArrival+7,pictureArrival-7,60};
        for (NSUInteger i=0;i<sizeof(offsets)/sizeof(*offsets);i++) {
            CGFloat offset=offsets[i];
            [feed setContentOffset:CGPointMake(0,feedHeight+offset) animated:NO];Layout(feed);
            [scrollDriver scrollViewDidScroll:feed];
            BOOL keepUpper=offset<90;
            CATransform3D placement=cell.renderer.layer.sublayerTransform;
            stableCoverage &= CATransform3DEqualToTransform(placement,fit) && !feed.clipsToBounds &&
                (keepUpper ? (cell.layer.zPosition==1 && wideNeighbor.layer.zPosition==0) :
                    (cell.layer.zPosition==0 && wideNeighbor.layer.zPosition==0)) &&
                CATransform3DIsIdentity(wideNeighbor.renderer.layer.sublayerTransform) &&
                CGRectEqualToRect(cell.renderer.frame,portraitFrame) && CGRectEqualToRect(hud.frame,cell.canvas.bounds);
        }
        Check(stableCoverage,
            @"dragging past a wide neighbor keeps natural 1:1 portrait motion over the Dock overlap without freezing or 2x whip");
        wideNeighbor.videoHeight=0;Layout(wideNeighbor);Layout(wideNeighbor.renderer);
        [feed setContentOffset:CGPointMake(0,feedHeight) animated:NO];Layout(feed);
        [manager refreshWithBranches:branches root:container.view bar:bar active:YES];
        Check(CATransform3DEqualToTransform(cell.renderer.layer.sublayerTransform,fit) && wideNeighbor.layer.zPosition==0,
            @"cancelling the gesture restores the settled portrait placement");

        // Move towards the previous episode without resizing either picture.
        [feed setContentOffset:CGPointMake(0,feedHeight-40) animated:NO];Layout(feed);
        cell.metal.bounds=(CGRect){CGPointZero,CGSizeMake(width,height+36)};
        cell.metal.center=CGPointMake(width/2,feedHeight/2);
        [manager refreshWithBranches:branches root:container.view bar:bar active:YES];
        CATransform3D movingFit=cell.renderer.layer.sublayerTransform;
        Check(!feed.clipsToBounds && movingFit.m42==45 && movingFit.m22==1 &&
            CATransform3DIsIdentity(cell.videoWrapper.layer.sublayerTransform) && CGRectEqualToRect(cell.renderer.frame,portraitFrame),
            @"a taller native picture already covers the screen and is not enlarged again during reverse paging");
        HGBackdropVideoCell *previous=(id)[feed cellForItemAtIndexPath:[NSIndexPath indexPathForItem:0 inSection:0]];
        previous.metalOverscanX=12;Layout(previous);Layout(cell);
        [manager refreshWithBranches:branches root:container.view bar:bar active:YES];
        CATransform3D previousFit=previous.renderer.layer.sublayerTransform;
        CATransform3D currentFit=cell.renderer.layer.sublayerTransform;
        [feed setContentOffset:CGPointMake(0,feedHeight-24) animated:NO];Layout(feed);
        [manager refreshWithBranches:branches root:container.view bar:bar active:YES];
        Check(previous && fabs(previousFit.m11-scale)<0.001 && fabs(currentFit.m11-scale)<0.001 &&
            CATransform3DEqualToTransform(previousFit,previous.renderer.layer.sublayerTransform) &&
            CATransform3DEqualToTransform(currentFit,cell.renderer.layer.sublayerTransform) &&
            previous.layer.zPosition==0 && cell.layer.zPosition==1,
            @"outgoing and incoming portraits keep stable independent frames and only the bottom page paints above prefetched neighbors");
        [feed setContentOffset:CGPointMake(0,feedHeight) animated:NO];Layout(feed);
        Layout(cell);[manager refreshWithBranches:branches root:container.view bar:bar active:YES];

        // Exercise the actual player willMoveToSuperview hook: the native
        // renderer must see its original compositor before it is borrowed.
        __block BOOL restoredBeforeTransfer=NO;
        __weak HGBackdropVideoCell *movingCell=cell;
        cell.renderer.nativeWillMoveObserver=^{
            restoredBeforeTransfer=CATransform3DIsIdentity(movingCell.renderer.layer.sublayerTransform) &&
                movingCell.renderer.clipsToBounds && movingCell.player.clipsToBounds &&
                movingCell.canvas.clipsToBounds && feed.clipsToBounds &&
                leftCorner.alpha==1 && rightCorner.alpha==1 &&
                gradient.opacity==1 && [gradient.colors isEqualToArray:stockGradient] &&
                [gradient.locations isEqualToArray:@[@0,@1]];
        };
        UIView *temporary=[[UIView alloc] initWithFrame:cell.player.bounds];
        [cell.player addSubview:temporary];
        [temporary addSubview:cell.renderer];
        cell.renderer.nativeWillMoveObserver=nil;
        [cell.player addSubview:cell.renderer];
        [temporary removeFromSuperview];
        [hud removeFromSuperview];
        Check(restoredBeforeTransfer,@"native handoff sees original renderer and clipping before any reparenting");

        cell.metalOverscanX=0;cell.videoHeight=width*9/16;Layout(cell);Layout(cell.renderer);
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
            feed.clipsToBounds && leftCorner.alpha==0 && rightCorner.alpha==0 && HGBackdropNativeUnchanged(geometry),
            @"a centered wide video keeps its real RGB matte continuous past the feed edge into the dock strip");

        UIView *underlay=Find(page.view,@"honggo.video.matte");
        Check(underlay.superview==page.view && !underlay.userInteractionEnabled &&
            CGRectEqualToRect(underlay.frame,CGRectMake(0,feedHeight,width,90)) &&
            [page.view.subviews indexOfObjectIdenticalTo:underlay]<[page.view.subviews indexOfObjectIdenticalTo:feed],
            @"the matte occupies only the reserved strip behind native video and never receives touches");
        UIColor *previousMatte = underlay.backgroundColor;
        [cell.canvas refreshBgColor];
        Check([underlay.backgroundColor isEqual:previousMatte],
            @"native model callback returns before the backdrop is changed");
        HGBackdropFlushFrames(0.04);
        Check(!underlay.hidden && [underlay.backgroundColor isEqual:cell.canvas.backgroundColor] &&
            HGBackdropNativeUnchanged(geometry),
            @"a native palette change updates the same settled cell on the next run-loop pass");
        cell.canvas.backgroundColor=[UIColor colorWithWhite:0.5 alpha:0.4];
        Layout(feed); HGBackdropFlushFrames(0.04);
        BOOL translucentHidden=underlay.hidden;
        [cell.canvas refreshBgColor]; HGBackdropFlushFrames(0.04);
        BOOL restoredMatte=!underlay.hidden;
        cell.canvas.backgroundColor=nil;
        Layout(feed); HGBackdropFlushFrames(0.04);
        Check(translucentHidden && restoredMatte && underlay.hidden && leftCorner.alpha==1 && rightCorner.alpha==1,
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
        [incoming.canvas refreshBgColor]; HGBackdropFlushFrames(0.04);
        BOOL bluePresented=[incoming.metal presentRed:0.05 green:0.08 blue:0.92];
        NSArray *swipeGeometry=HGBackdropNativeSnapshot(@[page.view,feed,incoming,incoming.contentView,
            incoming.canvas,incoming.metal,marker]);
        Check(!feed.clipsToBounds && underlay.hidden &&
            HGBackdropNativeUnchanged(swipeGeometry),
            @"a crossing portrait draws below the collection without a matte or changes to paging geometry");
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
        Check(feed.clipsToBounds && !underlay.superview && HGBackdropNativeUnchanged(swipeGeometry) &&
            CATransform3DIsIdentity(incoming.renderer.layer.sublayerTransform) && incoming.layer.zPosition==0,
            @"releasing portrait display restores feed ownership and clipping without repagination");
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
        cell.canvas.backgroundColor=[UIColor colorWithRed:0.1 green:0.12 blue:0.13 alpha:1];
        incoming.canvas.backgroundColor=[UIColor colorWithRed:0.3 green:0.21 blue:0.17 alpha:1];
        [manager refreshWithBranches:branches root:container.view bar:bar active:YES];
        underlay=Find(page.view,@"honggo.video.matte");
        CGFloat mixedRed=0;
        [underlay.backgroundColor getRed:&mixedRed green:NULL blue:NULL alpha:NULL];
        CALayer *upperMatte=cell.canvas.layer.sublayers.firstObject;
        CALayer *lowerMatte=incoming.canvas.layer.sublayers.firstObject;
        Check(fabs(mixedRed-(0.1+0.2*52/feedHeight))<0.002 &&
            upperMatte.backgroundColor && lowerMatte.backgroundColor &&
            CGColorEqualToColor(upperMatte.backgroundColor,underlay.backgroundColor.CGColor) &&
            CGColorEqualToColor(lowerMatte.backgroundColor,underlay.backgroundColor.CGColor),
            @"two letterboxed episodes share a continuous palette across the incoming edge and Dock");
        [feed setContentOffset:CGPointMake(0,feedHeight*2) animated:NO];Layout(feed);
        HGBackdropFlushFrames(0.04);
        Check(!upperMatte.superlayer && !lowerMatte.superlayer &&
            [underlay.backgroundColor isEqual:incoming.canvas.backgroundColor],
            @"settling removes the paging mattes and shows the new episode palette");
        Check(feed.clipsToBounds && fabs(feed.contentOffset.y-feedHeight*2)<0.01,
            @"settling on a native page restores clipping before preloaded neighbors can show through");
        [feed setContentOffset:CGPointMake(0,feedHeight+52) animated:NO];Layout(feed);
        HGBackdropFlushFrames(0.04);
        Check(!feed.clipsToBounds,@"native feed layout opens clipping again for a second partial swipe");

        // 7.4.5.33 mounts a full-window vertical owner inside its horizontal
        // channel scroll view. The collection can have a nested host wrapper.
        [manager restore];
        UIScrollView *category=[[UIScrollView alloc] initWithFrame:container.view.bounds];
        category.contentInsetAdjustmentBehavior=UIScrollViewContentInsetAdjustmentNever;
        category.contentSize=CGSizeMake(3*width,height);
        [container.view insertSubview:category belowSubview:bar];
        [category addSubview:page.view];
        page.view.frame=CGRectMake(width,0,width,height);
        [category setContentOffset:CGPointMake(width,0) animated:NO];
        UIView *wrapper=[[UIView alloc] initWithFrame:page.view.bounds];
        [page.view addSubview:wrapper];
        [wrapper addSubview:feed];
        [feed setContentOffset:CGPointMake(0,feedHeight*2) animated:NO];Layout(feed);
        incoming=(id)[feed cellForItemAtIndexPath:[NSIndexPath indexPathForItem:2 inSection:0]];
        incoming.videoHeight=width*9/16;Layout(incoming);Layout(incoming.renderer);
        incoming.canvas.backgroundColor=[UIColor colorWithRed:0.28 green:0.18 blue:0.13 alpha:1];
        [manager refreshWithBranches:@[category] root:container.view bar:bar active:YES];
        underlay=Find(wrapper,@"honggo.video.matte");
        Check(underlay.superview==wrapper && !underlay.hidden && feed.clipsToBounds,
            @"a settled horizontal channel continues the native matte through its nested feed wrapper");
        [category setContentOffset:CGPointMake(width+64,0) animated:NO];
        [manager refreshWithBranches:@[category] root:container.view bar:bar active:YES];
        Check(!underlay.superview && feed.clipsToBounds,
            @"a displaced horizontal page releases backdrop ownership to the host");
        [category setContentOffset:CGPointMake(width+0.25,0) animated:NO];
        [manager refreshWithBranches:@[category] root:container.view bar:bar active:YES];
        UIView *returnedMatte=Find(wrapper,@"honggo.video.matte");
        Check(returnedMatte.superview==wrapper && !returnedMatte.hidden && feed.clipsToBounds &&
            CGRectEqualToRect(feed.frame,CGRectMake(0,0,width,feedHeight)),
            @"returning to a settled channel restores its matte with subpixel host alignment");
        [feed removeFromSuperview];
        Check(feed.clipsToBounds && !feed.superview && CGRectEqualToRect(feed.frame,CGRectMake(0,0,width,feedHeight)),
            @"detaching the feed restores its clip without moving it back into the window");
        [manager restore];
        [wrapper removeFromSuperview];
        [category removeFromSuperview];

        // Dual-channel horizontal swipe inside FQRJXCategoryListContainerView:
        // both outgoing and incoming portrait channels must extend behind the
        // Dock simultaneously so no black strip or corner cutout appears mid-swipe.
        FQRJXCategoryListContainerView *pager=[[FQRJXCategoryListContainerView alloc] initWithFrame:container.view.bounds];
        UIScrollView *pagerScroll=[[UIScrollView alloc] initWithFrame:pager.bounds];
        pagerScroll.contentInsetAdjustmentBehavior=UIScrollViewContentInsetAdjustmentNever;
        pagerScroll.pagingEnabled=YES;
        pagerScroll.contentSize=CGSizeMake(2*width,height);
        pager.contentScrollView=pagerScroll;
        [pager addSubview:pagerScroll];
        [container.view insertSubview:pager belowSubview:bar];
        page.view.frame=CGRectMake(0,0,width,height);
        [pagerScroll addSubview:page.view];
        [page.view insertSubview:feed atIndex:0];
        cell.videoHeight=0;incoming.videoHeight=0;
        [feed setContentOffset:CGPointMake(0,feedHeight) animated:NO];Layout(feed);
        HGBackdropVideoCell *firstCell=(id)[feed cellForItemAtIndexPath:[NSIndexPath indexPathForItem:1 inSection:0]];
        firstCell.videoHeight=0;Layout(firstCell);Layout(firstCell.renderer);
        FQVShortVideoListViewController *secondPage=[FQVShortVideoListViewController new];
        [container addChildViewController:secondPage];[secondPage loadViewIfNeeded];
        secondPage.view.frame=CGRectMake(width,0,width,height);
        secondPage.view.backgroundColor=UIColor.blackColor;
        [pagerScroll addSubview:secondPage.view];[secondPage didMoveToParentViewController:container];
        UICollectionViewFlowLayout *secondLayout=[UICollectionViewFlowLayout new];
        secondLayout.itemSize=CGSizeMake(width,feedHeight);secondLayout.minimumLineSpacing=0;secondLayout.minimumInteritemSpacing=0;
        HGBackdropFeedProbe *secondFeed=[[HGBackdropFeedProbe alloc] initWithFrame:CGRectMake(0,0,width,feedHeight) collectionViewLayout:secondLayout];
        secondFeed.contentInsetAdjustmentBehavior=UIScrollViewContentInsetAdjustmentNever;
        secondFeed.pagingEnabled=YES;secondFeed.clipsToBounds=YES;secondFeed.backgroundColor=UIColor.blackColor;
        __attribute__((objc_precise_lifetime)) HGBackdropVideoDataSource *secondDataSource=[HGBackdropVideoDataSource new];
        secondFeed.dataSource=secondDataSource;
        [secondFeed registerClass:HGBackdropVideoCell.class forCellWithReuseIdentifier:@"live-metal"];
        [secondPage.view addSubview:secondFeed];[secondFeed reloadData];Layout(secondFeed);
        [secondFeed setContentOffset:CGPointMake(0,feedHeight) animated:NO];Layout(secondFeed);
        HGBackdropVideoCell *secondCell=(id)[secondFeed cellForItemAtIndexPath:[NSIndexPath indexPathForItem:1 inSection:0]];
        Layout(secondCell);Layout(secondCell.renderer);
        UIImageView *secondLeftCorner=[[UIImageView alloc] initWithFrame:CGRectMake(0,feedHeight-12,12,12)];
        UIImageView *secondRightCorner=[[UIImageView alloc] initWithFrame:CGRectMake(width-12,feedHeight-12,12,12)];
        [secondPage.view addSubview:secondLeftCorner];[secondPage.view addSubview:secondRightCorner];
        [pagerScroll setContentOffset:CGPointMake(width*0.5,0) animated:NO];
        [manager refreshWithBranches:@[pager] root:container.view bar:bar active:YES];
        Check(!feed.clipsToBounds && !secondFeed.clipsToBounds &&
            fabs(firstCell.renderer.layer.sublayerTransform.m42-45)<0.75 &&
            fabs(secondCell.renderer.layer.sublayerTransform.m42-45)<0.75 &&
            leftCorner.alpha==0 && rightCorner.alpha==0 &&
            secondLeftCorner.alpha==0 && secondRightCorner.alpha==0 &&
            page.view.clipsToBounds && secondPage.view.clipsToBounds,
            @"horizontal channel swipe extends both visible portrait feeds behind the Dock with clean seam clipping");
        [pagerScroll setContentOffset:CGPointMake(width,0) animated:NO];
        [manager refreshWithBranches:@[pager] root:container.view bar:bar active:YES];
        Check(!secondFeed.clipsToBounds && !secondPage.view.clipsToBounds &&
            feed.clipsToBounds && !page.view.clipsToBounds &&
            leftCorner.alpha==1 && rightCorner.alpha==1,
            @"settling on the incoming horizontal channel keeps its portrait backdrop live and restores the offscreen channel");
        [manager restore];
        secondFeed.dataSource=nil;
        [secondPage willMoveToParentViewController:nil];[secondPage.view removeFromSuperview];[secondPage removeFromParentViewController];
        [pager removeFromSuperview];
        [container.view addSubview:page.view];page.view.frame=container.view.bounds;
        RunDetailVideoBackdropChecks(page.view);
    }@finally {
        [manager restore];feed.dataSource=nil;
        window.hidden=YES;window.rootViewController=nil;[originalWindow makeKeyAndVisible];
        HGAppearanceApply(saved,nil);
    }
}
