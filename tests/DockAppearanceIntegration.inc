// Included after the controlled host, Check, Find and Layout helpers. These
// checks render real UIKit button layers and exercise their state changes.
#import <QuartzCore/QuartzCore.h>
#import "RuntimeSupport.h"

@interface HGADockThemeFixture : NSObject
@property(nonatomic,strong) UIColor *selectedImageColor;
@end
@implementation HGADockThemeFixture @end
static char HGADockThemeFixtureKey;
@implementation SSTabBar (HGADockThemeFixture)
- (HGADockThemeFixture *)appearanceStyle { return objc_getAssociatedObject(self,&HGADockThemeFixtureKey); }
@end

typedef struct {
    NSUInteger blue;
    NSUInteger brightNeutral;
} HGADockPixelCounts;

static UIImage *HGARenderDockView(UIView *view) {
    if(!view || CGRectIsEmpty(view.bounds))return nil;
    [view setNeedsLayout];[view layoutIfNeeded];
    UIGraphicsImageRendererFormat *format=[UIGraphicsImageRendererFormat defaultFormat];
    format.scale=2;format.opaque=YES;
    format.preferredRange=UIGraphicsImageRendererFormatRangeStandard;
    UIGraphicsImageRenderer *renderer=[[UIGraphicsImageRenderer alloc] initWithSize:view.bounds.size format:format];
    return [renderer imageWithActions:^(UIGraphicsImageRendererContext *context){
        [[UIColor colorWithWhite:0.12 alpha:1] setFill];
        UIRectFill((CGRect){CGPointZero,view.bounds.size});
        [view.layer renderInContext:context.CGContext];
    }];
}
static HGADockPixelCounts HGADockPixels(UIView *view) {
    UIImage *image=HGARenderDockView(view);CGImageRef cg=image.CGImage;
    HGADockPixelCounts result={0,0};
    if(!cg)return result;
    size_t width=CGImageGetWidth(cg),height=CGImageGetHeight(cg);
    if(!width || !height)return result;
    NSMutableData *pixels=[NSMutableData dataWithLength:width*height*4];
    CGColorSpaceRef space=CGColorSpaceCreateDeviceRGB();
    CGContextRef context=CGBitmapContextCreate(pixels.mutableBytes,width,height,8,width*4,space,
        kCGImageAlphaPremultipliedLast|kCGBitmapByteOrder32Big);
    CGColorSpaceRelease(space);if(!context)return result;
    CGContextDrawImage(context,CGRectMake(0,0,width,height),cg);CGContextRelease(context);
    const uint8_t *bytes=pixels.bytes;
    for(size_t i=0;i<width*height;i++) {
        CGFloat r=bytes[i*4]/255.0,g=bytes[i*4+1]/255.0,b=bytes[i*4+2]/255.0;
        if(b>0.4 && b>r+0.08 && b>g+0.04)result.blue++;
        if(r>0.68 && fabs(r-g)<0.06 && fabs(r-b)<0.06)result.brightNeutral++;
    }
    return result;
}
static BOOL HGADockNeutralTranslucent(UIColor *color,UITraitCollection *traits) {
    CGFloat r=0,g=0,b=0,a=0;
    return [[color resolvedColorWithTraitCollection:traits] getRed:&r green:&g blue:&b alpha:&a] &&
        fabs(r-g)<0.001 && fabs(r-b)<0.001 && a>0 && a<1;
}
static BOOL HGADockGlyphMatches(UIButton *button,BOOL lightGlyph) {
    CGFloat r,g,b,a;
    UIColor *color=[button.configuration.baseForegroundColor resolvedColorWithTraitCollection:button.traitCollection];
    return [color getRed:&r green:&g blue:&b alpha:&a] &&
        a>0.75 && (lightGlyph ? MIN(r,MIN(g,b))>0.90 : MAX(r,MAX(g,b))<0.10);
}
static CGFloat HGADockCompositorLuminance(UIImage *snapshot,CGPoint point) {
    CGRect pixels=CGRectIntegral(CGRectMake(point.x*snapshot.scale-2,point.y*snapshot.scale-2,4,4));
    CGImageRef sample=CGImageCreateWithImageInRect(snapshot.CGImage,pixels);
    if(!sample)return -1;
    uint8_t pixel[4]={};
    CGColorSpaceRef space=CGColorSpaceCreateDeviceRGB();
    CGContextRef context=CGBitmapContextCreate(pixel,1,1,8,4,space,kCGImageAlphaPremultipliedLast|kCGBitmapByteOrder32Big);
    CGColorSpaceRelease(space);
    if(context){CGContextDrawImage(context,CGRectMake(0,0,1,1),sample);CGContextRelease(context);}
    CGImageRelease(sample);
    return context ? (pixel[0]+pixel[1]+pixel[2])/(3.0*255) : -1;
}
static BOOL HGADockRectMatches(CGRect actual,CGRect expected) {
    return fabs(actual.origin.x-expected.origin.x)<0.5 && fabs(actual.origin.y-expected.origin.y)<0.5 &&
        fabs(actual.size.width-expected.size.width)<0.5 && fabs(actual.size.height-expected.size.height)<0.5;
}
static BOOL HGADockPresentationMatches(UIView *view) {
    CALayer *present=view.layer.presentationLayer;
    return present && HGADockRectMatches(present.frame,view.layer.frame) &&
        HGADockRectMatches(present.bounds,view.layer.bounds) && present.hidden==view.layer.hidden &&
        fabs(present.opacity-view.layer.opacity)<0.01;
}
static BOOL HGAWaitForDockPresentation(UIView *dock) {
    if(!dock.window)return NO;
    NSMutableArray<UIView *> *views=[NSMutableArray arrayWithObject:dock];
    for(NSString *identifier in @[@"honggo.appearance.dock.capsule",@"honggo.appearance.dock.selection",
        @"honggo.appearance.tab.home",@"honggo.appearance.tab.theater",@"honggo.appearance.dock.extra"]) {
        UIView *view=Find(dock,identifier);if(!view)return NO;[views addObject:view];
    }
    // The preceding expansion check exercises a real 0.22 s cross-dissolve.
    // Model layers change immediately; a window snapshot can still contain the
    // old collapsed presentation. Drive frames, without sleeping the UI thread,
    // and require agreement on two successive frames within a bounded deadline.
    CFTimeInterval start=CACurrentMediaTime(),deadline=start+1.25;
    NSUInteger stableFrames=0;
    do {
        [dock.window layoutIfNeeded];[CATransaction flush];
        CFRunLoopRunInMode(kCFRunLoopDefaultMode,1.0/60.0,false);
        BOOL matches=YES;for(UIView *view in views)matches&=HGADockPresentationMatches(view);
        if(CACurrentMediaTime()-start>=0.30 && matches && dock.layer.animationKeys.count==0)stableFrames++;
        else stableFrames=0;
        if(stableFrames>=2)break;
    }while(CACurrentMediaTime()<deadline);
    for(UIView *view in views)NSLog(@"DOCK_PREVIEW %@ model=%@ presentation=%@ hidden=%d/%d opacity=%.3f/%.3f",
        view.accessibilityIdentifier,NSStringFromCGRect(view.layer.frame),NSStringFromCGRect(view.layer.presentationLayer.frame),
        view.layer.hidden,view.layer.presentationLayer.hidden,view.layer.opacity,view.layer.presentationLayer.opacity);
    NSLog(@"DOCK_PREVIEW settled=%d elapsed=%.3f",stableFrames>=2,CACurrentMediaTime()-start);
    return stableFrames>=2;
}
static BOOL HGADockCompositorGlyphIsCrisp(UIImage *snapshot,UIButton *button,UIWindow *window) {
    if(!snapshot.CGImage || !button || button.hidden)return NO;
    // Exclude the glass rim. Only the central profile glyph can contribute
    // white foreground pixels here, so a bright reflection cannot pass this.
    CGRect region=[button convertRect:CGRectInset(button.bounds,8,8) toView:window];
    CGRect pixels=CGRectIntegral(CGRectApplyAffineTransform(region,
        CGAffineTransformMakeScale(snapshot.scale,snapshot.scale)));
    CGImageRef glyph=CGImageCreateWithImageInRect(snapshot.CGImage,pixels);
    if(!glyph)return NO;
    size_t width=CGImageGetWidth(glyph),height=CGImageGetHeight(glyph);
    NSMutableData *buffer=[NSMutableData dataWithLength:width*height*4];
    CGColorSpaceRef space=CGColorSpaceCreateDeviceRGB();
    CGContextRef context=CGBitmapContextCreate(buffer.mutableBytes,width,height,8,width*4,space,
        kCGImageAlphaPremultipliedLast|kCGBitmapByteOrder32Big);
    CGColorSpaceRelease(space);
    if(!context){CGImageRelease(glyph);return NO;}
    CGContextDrawImage(context,CGRectMake(0,0,width,height),glyph);
    CGContextRelease(context);CGImageRelease(glyph);
    const uint8_t *bytes=buffer.bytes;NSUInteger bright=0,sharp=0;
    for(size_t y=0;y<height;y++)for(size_t x=0;x<width;x++) {
        size_t i=(y*width+x)*4;
        CGFloat r=bytes[i]/255.0,g=bytes[i+1]/255.0,b=bytes[i+2]/255.0;
        if(MIN(r,MIN(g,b))<0.85 || MAX(r,MAX(g,b))-MIN(r,MIN(g,b))>0.07)continue;
        bright++;
        CGFloat luminance=(r+g+b)/3;
        if(x+1<width) {
            CGFloat next=(bytes[i+4]+bytes[i+5]+bytes[i+6])/(3*255.0);
            if(luminance-next>0.18)sharp++;
        }
        if(y+1<height) {
            size_t nextIndex=i+width*4;
            CGFloat next=(bytes[nextIndex]+bytes[nextIndex+1]+bytes[nextIndex+2])/(3*255.0);
            if(luminance-next>0.18)sharp++;
        }
    }
    NSLog(@"DOCK_COMPOSITOR profile brightPixels=%lu sharpEdges=%lu",(unsigned long)bright,(unsigned long)sharp);
    // R9's blurred compositor output had no pixels above 0.85 in this region;
    // separate CALayer renders missed the defect because they omit backdrop blur.
    return bright>=12 && sharp>=4;
}
static BOOL HGASaveDockPreview(SSTabBarController *host,UIView *dock) {
    UIWindow *window=dock.window;if(!window)return NO;
    UIView *backdrop=[[UIView alloc] initWithFrame:CGRectInset(host.customTabBar.frame,-12,-12)];
    CAGradientLayer *gradient=[CAGradientLayer layer];gradient.frame=backdrop.bounds;
    gradient.colors=@[(id)[UIColor colorWithRed:0.10 green:0.12 blue:0.18 alpha:1].CGColor,
        (id)[UIColor colorWithWhite:0.27 alpha:1].CGColor];
    [backdrop.layer addSublayer:gradient];
    [host.view insertSubview:backdrop belowSubview:host.customTabBar];
    Layout(host.customTabBar);Layout(dock);
    BOOL settled=HGAWaitForDockPresentation(dock);
    Check(settled,@"dock snapshot waits for expanded model and presentation frames to agree");
    if(!settled){[backdrop removeFromSuperview];return NO;}
    UIGraphicsImageRendererFormat *format=[UIGraphicsImageRendererFormat defaultFormat];
    format.scale=2;format.opaque=YES;format.preferredRange=UIGraphicsImageRendererFormatRangeStandard;
    UIGraphicsImageRenderer *fullRenderer=[[UIGraphicsImageRenderer alloc] initWithSize:window.bounds.size format:format];
    __block BOOL captured=NO;
    UIImage *full=[fullRenderer imageWithActions:^(UIGraphicsImageRendererContext *context){
        captured=[window drawViewHierarchyInRect:window.bounds afterScreenUpdates:YES];
    }];
    CGRect crop=CGRectIntersection(window.bounds,[dock convertRect:CGRectInset(dock.bounds,-8,-8) toView:window]);
    if(!captured || !full.CGImage || CGRectIsEmpty(crop)) {[backdrop removeFromSuperview];return NO;}
    if(!UIAccessibilityIsReduceTransparencyEnabled()) {
        UIView *selection=Find(dock,@"honggo.appearance.dock.selection");
        UIView *capsule=Find(dock,@"honggo.appearance.dock.capsule");
        CGPoint selectedPoint=[selection convertPoint:CGPointMake(CGRectGetMidX(selection.bounds)-25,CGRectGetMidY(selection.bounds)) toView:window];
        CGPoint glassPoint=[capsule convertPoint:CGPointMake(CGRectGetMaxX(selection.frame)+14,CGRectGetMidY(selection.frame)) toView:window];
        CGFloat selectedTone=HGADockCompositorLuminance(full,selectedPoint);
        CGFloat glassTone=HGADockCompositorLuminance(full,glassPoint);
        Check(selectedTone>0.03 && selectedTone+0.015<glassTone,
            @"selected fill adds contrast while preserving the composited material underneath");
    }
    UIButton *profile=(id)Find(dock,@"honggo.appearance.dock.extra");
    Check(HGADockCompositorGlyphIsCrisp(full,profile,window),
        @"window compositor keeps the independent profile glyph bright and sharp above glass");
    CGImageRef cg=CGImageCreateWithImageInRect(full.CGImage,CGRectApplyAffineTransform(crop,CGAffineTransformMakeScale(2,2)));
    UIImage *cropped=cg?[UIImage imageWithCGImage:cg scale:2 orientation:UIImageOrientationUp]:nil;
    if(cg)CGImageRelease(cg);
    [backdrop removeFromSuperview];
    if(!cropped)return NO;
    UIGraphicsImageRenderer *previewRenderer=[[UIGraphicsImageRenderer alloc]
        initWithSize:CGSizeMake(crop.size.width+24,crop.size.height+48) format:format];
    UIImage *preview=[previewRenderer imageWithActions:^(UIGraphicsImageRendererContext *context){
        [[UIColor colorWithWhite:0.10 alpha:1] setFill];
        UIRectFill(CGRectMake(0,0,crop.size.width+24,crop.size.height+48));
        [cropped drawAtPoint:CGPointMake(12,12)];
        [[NSString stringWithFormat:@"Controlled UIKit fixture · Honggo %@",HG_PLUGIN_VERSION] drawAtPoint:CGPointMake(14,crop.size.height+23)
            withAttributes:@{NSFontAttributeName:[UIFont systemFontOfSize:11],
                NSForegroundColorAttributeName:[UIColor colorWithWhite:1 alpha:0.72]}];
    }];
    NSURL *directory=[NSFileManager.defaultManager URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask].firstObject;
    [NSFileManager.defaultManager createDirectoryAtURL:directory withIntermediateDirectories:YES attributes:nil error:nil];
    return [UIImagePNGRepresentation(preview) writeToURL:[directory URLByAppendingPathComponent:@"dock-preview.png"] atomically:YES];
}
@interface HGADockRestorationProbe : UIView
@property(nonatomic,copy) void (^onRestore)(void);
@end
@implementation HGADockRestorationProbe
- (void)setHidden:(BOOL)hidden {
    BOOL wasHidden=self.hidden;
    [super setHidden:hidden];
    if(wasHidden && !hidden && self.onRestore)self.onRestore();
}
- (void)nativeTab:(UIControl *)sender {}
@end

static void HGRunDockRestoreReentryChecks(SSTabBarController *host) {
    NSDictionary *saved=HGAppearanceValues();
    HGAppearanceApply(@{@"enabled":@YES,@"dock.enabled":@YES,@"dock.extra":@NO,
        @"dock.hide.home":@NO,@"dock.hide.theater":@NO,@"dock.hide.store":@NO,@"dock.hide.mine":@NO},nil);
    SSTabBar *bar=[[SSTabBar alloc] initWithFrame:host.customTabBar.frame];
    [bar.contentView removeFromSuperview];
    HGADockRestorationProbe *source=[[HGADockRestorationProbe alloc] initWithFrame:bar.bounds];
    bar.contentView=source;[bar addSubview:source];
    UIView *previousGlass=[[UIView alloc] initWithFrame:bar.bounds];
    previousGlass.accessibilityIdentifier=@"honggo.tabbar.glass";
    [bar insertSubview:previousGlass belowSubview:source];
    NSArray *titles=@[@"首页",@"剧场",@"商城",@"我的"];
    for(NSUInteger i=0;i<titles.count;i++) {
        UIControl *item=[[UIControl alloc] initWithFrame:CGRectMake(i*bar.bounds.size.width/4,0,
            bar.bounds.size.width/4,bar.bounds.size.height)];
        UILabel *label=[[UILabel alloc] initWithFrame:item.bounds];label.text=titles[i];
        [item addSubview:label];[item addTarget:source action:@selector(nativeTab:) forControlEvents:UIControlEventTouchUpInside];
        [source addSubview:item];
    }
    bar.backgroundColor=UIColor.systemOrangeColor;bar.opaque=YES;
    [host.view addSubview:bar];HGAppearanceLayoutDock(bar,host);Layout(bar);
    Check(source.hidden && Find(bar,@"honggo.appearance.dock"),
        @"restoration reentry fixture starts with a hidden observed native source");
    __block NSUInteger restoreCallbacks=0;
    __weak SSTabBar *weakBar=bar;
    source.onRestore=^{
        restoreCallbacks++;
        // A host setter can synchronously ask both modules to update. This
        // used to clear hiddenSources while the outer restore enumerated it.
        HGAppearanceRestoreDock(weakBar);
        Layout(weakBar);
    };
    HGAppearanceSet(@"enabled",@NO,nil);
    Check(restoreCallbacks==1 && !source.hidden && !Find(bar,@"honggo.appearance.dock"),
        @"reentrant host source restoration runs once and leaves native navigation visible");
    Check([bar.backgroundColor isEqual:UIColor.systemOrangeColor] && bar.opaque,
        @"nested layout cannot capture a half-restored dock background or opacity");
    // The coalesced tab-bar update must run after the restore has completed,
    // then legacy mode can acquire the complete original baseline safely.
    CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.02,false);
    Layout(bar);bar.hidden=YES;
    Check([bar.backgroundColor isEqual:UIColor.systemOrangeColor] && bar.opaque && !source.hidden,
        @"deferred legacy reconciliation retains the restored host baseline");
    source.onRestore=nil;
    [bar removeFromSuperview];
    HGAppearanceApply(saved,nil);Layout(host.customTabBar);
}

static void HGRunDockSurfaceChecks(SSTabBarController *host) {
    NSDictionary *saved=HGAppearanceValues();
    SSTabBar *bar=host.customTabBar;
    CALayer *savedMask=bar.layer.mask;
    BOOL savedClip=bar.clipsToBounds, savedHidden=bar.hidden;
    CGFloat width=host.view.bounds.size.width, bottom=host.view.bounds.size.height;
    CGFloat barTop=[bar convertRect:bar.bounds toView:host.view].origin.y;
    HGAppearanceApply(@{@"enabled":@YES,@"dock.enabled":@YES,@"dock.extra":@NO,
        @"dock.hide.home":@NO,@"dock.hide.theater":@NO,@"dock.hide.store":@NO,@"dock.hide.mine":@NO},nil);

    // A clipped library list ending at the host bar is the R10 hard edge.
    // The real capsule and TabBar implementation both run in this fixture.
    UIView *viewport=[[UIView alloc] initWithFrame:CGRectMake(0,0,width,barTop)];
    viewport.clipsToBounds=YES;
    UIScrollView *list=[[UIScrollView alloc] initWithFrame:viewport.bounds];
    list.contentInsetAdjustmentBehavior=UIScrollViewContentInsetAdjustmentNever;
    list.contentInset=UIEdgeInsetsMake(3,4,12,5);
    list.verticalScrollIndicatorInsets=UIEdgeInsetsMake(0,0,9,0);
    list.contentSize=CGSizeMake(width,bottom*3);
    [viewport addSubview:list];
    // The host root's observed didAddSubview: refreshes the dock synchronously.
    // Capture native values before attachment can extend this fixture.
    CGRect originalViewport=viewport.frame,originalList=list.frame;
    UIEdgeInsets originalInset=list.contentInset;
    [host.view insertSubview:viewport belowSubview:bar];
    CAShapeLayer *hostMask=[CAShapeLayer layer];
    hostMask.frame=bar.bounds;hostMask.path=[UIBezierPath bezierPathWithRect:bar.bounds].CGPath;
    bar.layer.mask=hostMask;bar.clipsToBounds=YES;Layout(bar);
    Check(!bar.layer.mask && !bar.clipsToBounds && Find(bar,@"honggo.appearance.dock"),
        @"custom dock removes the inherited host mask without masking the glass capsule");
    Check(fabs(CGRectGetMaxY(viewport.frame)-bottom)<0.75 && fabs(list.bounds.size.height-bottom)<0.75,
        @"custom dock extends the ordinary library viewport beneath the home-indicator area");
    CGFloat expectedInset=originalInset.bottom+bottom-barTop;
    BOOL insetPreserved=fabs(list.contentInset.bottom-expectedInset)<0.75 && list.contentInset.top==3 &&
        list.contentInset.left==4 && list.contentInset.right==5;
    if(!insetPreserved)NSLog(@"DOCK_VIEWPORT inset native=%@ actual=%@ expectedBottom=%.3f barTop=%.3f bottom=%.3f",
        NSStringFromUIEdgeInsets(originalInset),NSStringFromUIEdgeInsets(list.contentInset),expectedInset,barTop,bottom);
    Check(insetPreserved,
        @"extended library list keeps its last row reachable and preserves the other native insets");
    CGRect settledViewport=viewport.frame,settledList=list.frame;
    UIEdgeInsets settledInset=list.contentInset;
    for(NSUInteger i=0;i<25;i++)Layout(bar);
    Check(CGRectEqualToRect(viewport.frame,settledViewport) && CGRectEqualToRect(list.frame,settledList) &&
        UIEdgeInsetsEqualToEdgeInsets(list.contentInset,settledInset),
        @"custom dock repeated layouts do not accumulate viewport height or bottom padding");
    UIView *capsule=Find(bar,@"honggo.appearance.dock.capsule");
    Check(Find(capsule,@"honggo.appearance.material") && !capsule.hidden &&
        Find(bar,@"honggo.appearance.dock.selection"),
        @"host background cleanup leaves the plugin-owned capsule and selection intact");
    bar.hidden=YES;
    BOOL geometryRestored=bar.layer.mask==hostMask && bar.clipsToBounds &&
        CGRectEqualToRect(viewport.frame,originalViewport) && CGRectEqualToRect(list.frame,originalList) &&
        UIEdgeInsetsEqualToEdgeInsets(list.contentInset,originalInset);
    if(!geometryRestored)NSLog(@"DOCK_VIEWPORT restore viewport=%@ expected=%@ list=%@ expected=%@ inset=%@ expected=%@ mask=%d clips=%d",
        NSStringFromCGRect(viewport.frame),NSStringFromCGRect(originalViewport),NSStringFromCGRect(list.frame),
        NSStringFromCGRect(originalList),NSStringFromUIEdgeInsets(list.contentInset),NSStringFromUIEdgeInsets(originalInset),
        bar.layer.mask==hostMask,bar.clipsToBounds);
    Check(geometryRestored,
        @"hiding the custom dock restores native masking and exact list geometry");
    UIColor *nativeBackground=bar.backgroundColor;
    BOOL nativeOpaque=bar.opaque;
    [viewport removeFromSuperview];bar.layer.mask=savedMask;bar.clipsToBounds=savedClip;bar.hidden=NO;

    // The ancestor deliberately autoresizes its feed. Extending the ancestor
    // alone would change pagination even if the feed itself were skipped.
    UIView *videoPage=[[UIView alloc] initWithFrame:CGRectMake(0,0,width,barTop)];
    FQVCollectionView *video=[[FQVCollectionView alloc] initWithFrame:videoPage.bounds
        collectionViewLayout:[UICollectionViewFlowLayout new]];
    video.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
    video.pagingEnabled=YES;video.contentInsetAdjustmentBehavior=UIScrollViewContentInsetAdjustmentNever;
    [videoPage addSubview:video];Layout(video);
    CGRect originalPage=videoPage.frame,originalVideo=video.frame;
    UIEdgeInsets videoInset=video.contentInset;
    CGSize videoContent=video.contentSize;
    [host.view insertSubview:videoPage belowSubview:bar];
    for(NSUInteger i=0;i<25;i++)Layout(bar);
    Check(CGRectEqualToRect(videoPage.frame,originalPage) && CGRectEqualToRect(video.frame,originalVideo) &&
        UIEdgeInsetsEqualToEdgeInsets(video.contentInset,videoInset) && CGSizeEqualToSize(video.contentSize,videoContent),
        @"video page and autoresizing ancestors retain the native episode stride under a custom dock");
    [videoPage removeFromSuperview];

    UIScrollView *verticalPager=[[UIScrollView alloc] initWithFrame:CGRectMake(0,0,width,barTop)];
    verticalPager.pagingEnabled=YES;verticalPager.contentInsetAdjustmentBehavior=UIScrollViewContentInsetAdjustmentNever;
    verticalPager.contentSize=CGSizeMake(width,barTop*3);
    CGRect nativePager=verticalPager.frame;
    [host.view insertSubview:verticalPager belowSubview:bar];Layout(bar);
    Check(CGRectEqualToRect(verticalPager.frame,nativePager) && verticalPager.contentInset.bottom==0,
        @"unmapped vertical paging views also keep their native page size and no added padding");
    [verticalPager removeFromSuperview];

    bar.backgroundColor=UIColor.systemPurpleColor;bar.opaque=YES;Layout(bar);
    HGAppearanceSet(@"dock.enabled",@NO,nil);Layout(bar);
    Check(!Find(bar,@"honggo.appearance.dock") && !bar.contentView.hidden &&
        Find(bar,@"honggo.tabbar.glass"),
        @"switching custom dock off restores native navigation with one legacy glass surface");
    HGAppearanceSet(@"dock.enabled",@YES,nil);Layout(bar);bar.hidden=YES;
    Check([bar.backgroundColor isEqual:UIColor.systemPurpleColor] && bar.opaque,
        @"custom and legacy dock mode switches preserve the newest host fill and opacity");
    bar.hidden=NO;
    host.view.hidden=YES;
    Check(!Find(bar,@"honggo.appearance.dock") && [bar.backgroundColor isEqual:UIColor.systemPurpleColor],
        @"hiding an observed ancestor releases the custom dock and its host background ownership");
    host.view.hidden=NO;
    Check(Find(bar,@"honggo.appearance.dock") && !bar.layer.mask,
        @"showing the same ancestor restores the custom dock without an extra layout request");
    bar.hidden=YES;bar.backgroundColor=nativeBackground;bar.opaque=nativeOpaque;
    bar.layer.mask=savedMask;bar.clipsToBounds=savedClip;
    HGAppearanceApply(saved,nil);bar.hidden=savedHidden;Layout(bar);
}

@interface HGADockBottomInsetFixture : SSTabBar
@end
@implementation HGADockBottomInsetFixture
- (UIEdgeInsets)safeAreaInsets { return UIEdgeInsetsMake(0,0,34,0); }
@end

static void HGRunExpandedDockGeometryChecks(SSTabBarController *host) {
    NSDictionary *saved=HGAppearanceValues();
    CGFloat rootHeight=host.view.bounds.size.height;
    HGADockBottomInsetFixture *bar=[[HGADockBottomInsetFixture alloc]
        initWithFrame:CGRectMake(0,rootHeight-84,320,84)];
    NSArray<NSString *> *titles=@[@"首页",@"剧场",@"商城",@"我的"];
    for(NSUInteger i=0;i<titles.count;i++) {
        UIControl *source=[[UIControl alloc] initWithFrame:CGRectMake(i*80,0,80,84)];
        UILabel *title=[[UILabel alloc] initWithFrame:source.bounds];title.text=titles[i];
        [source addSubview:title];
        [source addTarget:host action:@selector(nativeTap:) forControlEvents:UIControlEventTouchUpInside];
        [bar.contentView addSubview:source];
    }
    [host.view addSubview:bar];
    HGAppearanceApply(HGAppearancePreset(@"glass"),nil);
    HGAppearanceSet(@"dock.content",@"icon",nil);
    HGAppearanceLayoutDock(bar,host);
    UIView *dock=Find(bar,@"honggo.appearance.dock");
    UIView *capsule=Find(dock,@"honggo.appearance.dock.capsule");
    UIButton *home=(id)Find(dock,@"honggo.appearance.tab.home");
    UIButton *extra=(id)Find(dock,@"honggo.appearance.dock.extra");
    Layout(dock);Layout(home);Layout(extra);
    CGRect circle=[extra convertRect:extra.bounds toView:dock];
    CGRect occupied=CGRectUnion(capsule.frame,circle);
    CGFloat stockIcon=MAX(home.imageView.bounds.size.width,home.imageView.bounds.size.height);
    Check(dock && fabs(CGRectGetWidth(occupied)-320*0.90)<0.01 &&
        fabs(CGRectGetMidX(occupied)-160)<0.01 && fabs(capsule.bounds.size.height-58)<0.01 &&
        fabs(circle.size.width-54)<0.01 && fabs(circle.size.height-54)<0.01 &&
        fabs(CGRectGetMidY(circle)-CGRectGetMidY(capsule.frame))<0.01 &&
        fabs(CGRectGetMinX(circle)-CGRectGetMaxX(capsule.frame)-12)<0.01 &&
        CGRectGetMinY(occupied)>=0 && CGRectGetMaxY(occupied)<=70 && home.bounds.size.width>=44,
        @"the stock dock fits the native bar with a balanced capsule, smaller accessory and accessible targets");
    CGPoint extraPoint=[extra convertPoint:CGPointMake(CGRectGetMidX(extra.bounds),CGRectGetMidY(extra.bounds)) toView:bar];
    UIView *hit=[bar hitTest:extraPoint withEvent:nil];
    Check(hit==extra || [hit isDescendantOfView:extra],@"the compact independent action remains inside the native tap area");

    HGAppearanceApply(@{@"dock.width":@0.905,@"dock.height":@62,@"dock.radius":@31,
        @"dock.iconSize":@26,@"dock.extraSize":@62},nil);
    HGAppearanceLayoutDock(bar,host);Layout(home);
    CGFloat customIcon=MAX(home.imageView.bounds.size.width,home.imageView.bounds.size.height);
    Check(stockIcon>0 && customIcon>stockIcon && customIcon<=36,
        @"the stock glyph is smaller and custom sizes still scale proportionally");

    bar.frame=CGRectMake(0,rootHeight-60,320,60);
    HGAppearanceApply(HGAppearancePreset(@"glass"),nil);
    HGAppearanceLayoutDock(bar,host);
    circle=[extra convertRect:extra.bounds toView:dock];
    occupied=CGRectUnion(capsule.frame,circle);
    Check(fabs(capsule.bounds.size.height-46)<0.01 && fabs(circle.size.height-46)<0.01 &&
        CGRectGetMinY(occupied)>=0 && CGRectGetMaxY(occupied)<=46 && home.bounds.size.width>=44,
        @"a short dock host keeps safe clearance and accessible targets by shrinking the row");
    HGAppearanceRestoreDock(bar);[bar removeFromSuperview];
    HGAppearanceApply(saved,nil);
}

@interface HGADockPanFixture : UIPanGestureRecognizer
@property(nonatomic) CGPoint fixtureLocation;
@property(nonatomic) UIGestureRecognizerState fixtureState;
@end
@implementation HGADockPanFixture
- (CGPoint)locationInView:(UIView *)view {return self.fixtureLocation;}
- (UIGestureRecognizerState)state {return self.fixtureState;}
@end

@interface HGAAppearanceDock (TouchFixture) <UIGestureRecognizerDelegate>
- (BOOL)acceptsSelectionTouchAtPoint:(CGPoint)point inView:(UIView *)hit;
@end

static void HGRunDockTouchRoutingChecks(HGAAppearanceDock *dock,UIView *bar,UIView *capsule,UIButton *home) {
    UIView *container=bar.superview;
    UIPanGestureRecognizer *installed=nil;
    for(UIGestureRecognizer *gesture in container.gestureRecognizers)
        if(gesture.delegate==(id)dock && [gesture isKindOfClass:UIPanGestureRecognizer.class])installed=(id)gesture;
    Check(installed && installed.cancelsTouchesInView && installed.maximumNumberOfTouches==1,
        @"dock drag is installed on the actual shared container and cancels the original button touch");

    // The host progress control covers a larger rectangle than its visible rail.
    UIView *player=[[NSClassFromString(@"FQVShortVideoVPlayerControlView") alloc] initWithFrame:container.bounds];
    CGRect surface=[capsule convertRect:capsule.bounds toView:container];
    UISlider *rail=[[UISlider alloc] initWithFrame:CGRectMake(CGRectGetMinX(surface),CGRectGetMinY(surface)-22,
        CGRectGetWidth(surface),40)];
    [player addSubview:rail];
    [container insertSubview:player aboveSubview:bar];
    CGPoint overlap=CGPointMake(CGRectGetMidX(home.frame),10);
    CGPoint above=CGPointMake(overlap.x,-8);
    UIView *overlapHit=[container hitTest:[capsule convertPoint:overlap toView:container] withEvent:nil];
    UIView *railHit=[container hitTest:[capsule convertPoint:above toView:container] withEvent:nil];
    Check([overlapHit isDescendantOfView:player] && [overlapHit isDescendantOfView:installed.view] &&
        [dock acceptsSelectionTouchAtPoint:overlap inView:overlapHit],
        @"a dock start can reach the pan through the shared ancestor even when the native scrubber wins hit testing");
    Check([railHit isDescendantOfView:player] && ![dock acceptsSelectionTouchAtPoint:above inView:railHit],
        @"the progress rail above the rounded capsule retains its native touch route");
    UIPanGestureRecognizer *unrelated=[UIPanGestureRecognizer new];
    unrelated.delegate=(id)player;[player addGestureRecognizer:unrelated];
    NSUInteger gestureCount=player.gestureRecognizers.count;
    Check(![dock gestureRecognizer:installed shouldBeRequiredToFailByGestureRecognizer:unrelated] &&
        player.gestureRecognizers.count==gestureCount,
        @"asking about a native swipe leaves an uninitialized player progress gesture uncreated");
    UIPanGestureRecognizer *progress=HGReadObjectGetter(player,"progressPanGesture");
    Check(progress && [dock gestureRecognizer:installed shouldBeRequiredToFailByGestureRecognizer:progress] &&
        ![dock gestureRecognizer:installed shouldBeRequiredToFailByGestureRecognizer:unrelated],
        @"the identified progress pan alone waits for the dock pan");
    UIView *sheet=[[UIView alloc] initWithFrame:container.bounds];
    [container addSubview:sheet];
    UIView *sheetHit=[container hitTest:[capsule convertPoint:overlap toView:container] withEvent:nil];
    Check(sheetHit==sheet && ![dock acceptsSelectionTouchAtPoint:overlap inView:sheetHit],
        @"an unrelated surface covering the dock retains its touches");
    [sheet removeFromSuperview];[player removeFromSuperview];
}

static void HGRunDockDragChecks(SSTabBarController *host,HGAAppearanceDock *dock,UIView *capsule,
    UIView *selection,UIButton *home,UIButton *theater) {
    NSUInteger taps=host.nativeTaps;
    host.selectedIndex=0;
    HGAppearanceLayoutDock(host.customTabBar,host);
    HGRunDockTouchRoutingChecks(dock,host.customTabBar,capsule,home);
    BOOL originalCollapse=HGAppearanceFlag(@"dock.collapse");
    HGAppearanceSet(@"dock.collapse",@YES,nil);
    [dock setCollapsed:YES animated:YES];
    [dock setCollapsed:NO animated:YES];
    Check(!capsule.hidden && (UIAccessibilityIsReduceMotionEnabled() ||
        [capsule.layer animationForKey:@"honggo.dock.collapse"]!=nil),
        @"expanding the dock animates its visible surface directly after a swipe");
    [capsule.layer removeAnimationForKey:@"honggo.dock.collapse"];
    HGAppearanceSet(@"dock.collapse",@(originalCollapse),nil);

    CGFloat homeX=CGRectGetMidX(home.frame),theaterX=CGRectGetMidX(theater.frame);
    CGFloat middle=(homeX+theaterX)/2;
    HGADockPanFixture *pan=[HGADockPanFixture new];
    pan.fixtureLocation=CGPointMake(homeX,20);pan.fixtureState=UIGestureRecognizerStateBegan;
    [dock dragSelection:pan];
    pan.fixtureLocation=CGPointMake(middle,20);pan.fixtureState=UIGestureRecognizerStateChanged;
    [dock dragSelection:pan];
    Check(!selection.hidden && fabs(CGRectGetMidX(selection.frame)-middle)<0.01 && host.nativeTaps==taps,
        @"selection follows an intermediate drag position without opening a tab during the drag");
    UIVisualEffectView *lens=(id)Find(selection,@"honggo.appearance.dock.drag-lens");
    if(@available(iOS 26.0,*)) {
        if(HGNativeGlassConfigured() && !UIAccessibilityIsReduceTransparencyEnabled()) {
            Layout(lens);
            Check([lens.effect isKindOfClass:UIGlassEffect.class] && ((UIGlassEffect *)lens.effect).interactive,
                @"Liquid Glass drag installs an interactive native refraction lens beneath the glyphs");
        }else Check(!lens,@"dock drag omits the refraction lens when native transparency is unavailable");
    }
    pan.fixtureState=UIGestureRecognizerStateCancelled;
    [dock dragSelection:pan];
    Check(host.nativeTaps==taps && host.selectedIndex==0 && fabs(CGRectGetMidX(selection.frame)-homeX)<0.01 &&
        !Find(selection,@"honggo.appearance.dock.drag-lens"),@"cancelled drag returns to the original tab and clears the lens");

    pan.fixtureLocation=CGPointMake(homeX,20);pan.fixtureState=UIGestureRecognizerStateBegan;
    [dock dragSelection:pan];
    pan.fixtureLocation=CGPointMake(capsule.bounds.size.width+15,20);pan.fixtureState=UIGestureRecognizerStateChanged;
    [dock dragSelection:pan];
    Check(fabs(CGRectGetMidX(selection.frame)-theaterX)<0.01,
        @"drag clamps to the last visible tab when other native tabs are hidden");
    pan.fixtureState=UIGestureRecognizerStateEnded;[dock dragSelection:pan];
    Check(host.nativeTaps==taps+1 && host.selectedIndex==1 && theater.selected &&
        !Find(selection,@"honggo.appearance.dock.drag-lens"),@"drag release forwards one original action to the destination tab");

    HGAppearanceSet(@"dock.material",@"blur",nil);
    pan.fixtureLocation=CGPointMake(middle,20);pan.fixtureState=UIGestureRecognizerStateBegan;
    [dock dragSelection:pan];
    Check(!Find(selection,@"honggo.appearance.dock.drag-lens") && fabs(CGRectGetMidX(selection.frame)-middle)<0.01,
        @"classic blur keeps its material while supporting the same drag interaction");
    pan.fixtureState=UIGestureRecognizerStateCancelled;[dock dragSelection:pan];
    HGAppearanceSet(@"dock.material",@"regular",nil);
}

static void HGRunDockAppearanceChecks(SSTabBarController *host) {
    HGRunDockRestoreReentryChecks(host);
    HGRunDockSurfaceChecks(host);
    NSDictionary *saved=HGAppearanceValues();
    NSUInteger savedIndex=host.selectedIndex;
    CGRect savedBarFrame=host.customTabBar.frame;
    UIUserInterfaceStyle savedStyle=host.overrideUserInterfaceStyle;
    UIColor *savedTint=host.view.tintColor;
    host.overrideUserInterfaceStyle=UIUserInterfaceStyleLight;
    host.view.tintColor=UIColor.systemBlueColor;
    host.selectedIndex=0;
    HGADockThemeFixture *theme=[HGADockThemeFixture new];
    theme.selectedImageColor=UIColor.whiteColor;
    objc_setAssociatedObject(host.customTabBar,&HGADockThemeFixtureKey,theme,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    // The real R14 capture has an 84pt native tab-bar host. The original 60pt
    // fixture would clamp a 62pt request and hide whether the new size works.
    CGRect roomyBar=savedBarFrame;roomyBar.size.height=84;
    roomyBar.origin.y=host.view.bounds.size.height-84;
    host.customTabBar.frame=roomyBar;
    HGAppearanceApply(@{@"enabled":@YES,@"dock.enabled":@YES,@"dock.material":@"regular",
        @"dock.content":@"icon",@"dock.height":@62,@"dock.width":@0.905,@"dock.radius":@31,
        @"dock.iconSize":@26,@"dock.extra":@YES,@"dock.extraSize":@62,@"dock.action":@"mine",
        @"dock.hide.home":@NO,@"dock.hide.theater":@NO,@"dock.hide.store":@YES,@"dock.hide.mine":@YES,
        @"dock.selected":@"",@"dock.normal":@"",@"dock.tint":@""},nil);
    HGAppearanceLayoutDock(host.customTabBar,host);
    UIView *dock=Find(host.customTabBar,@"honggo.appearance.dock");
    UIView *capsule=Find(dock,@"honggo.appearance.dock.capsule");
    UIView *selection=Find(dock,@"honggo.appearance.dock.selection");
    UIButton *home=(id)Find(dock,@"honggo.appearance.tab.home");
    UIButton *theater=(id)Find(dock,@"honggo.appearance.tab.theater");
    UIButton *extra=(id)Find(dock,@"honggo.appearance.dock.extra");
    Layout(dock);Layout(home);Layout(extra);
    UIView *extraAncestor=extra.superview;
    while(extraAncestor && extraAncestor!=dock && ![extraAncestor isKindOfClass:UIVisualEffectView.class])
        extraAncestor=extraAncestor.superview;
    UIVisualEffectView *extraMaterial=extraAncestor!=dock ? (id)extraAncestor : nil;
    UIVisualEffectView *capsuleMaterial=(id)Find(capsule,@"honggo.appearance.material");
    CGRect extraInDock=[extra convertRect:extra.bounds toView:dock];
    Check([extraMaterial isKindOfClass:UIVisualEffectView.class] && extra.superview==extraMaterial.contentView &&
        home.superview==capsuleMaterial.contentView && theater.superview==capsuleMaterial.contentView,
        @"all navigation and accessory glyphs belong to the actual system material foreground");
    Check(fabs(capsule.bounds.size.height-62)<0.01 && fabs(extra.bounds.size.width-62)<0.01 &&
        fabs(extra.bounds.size.height-62)<0.01 && fabs(CGRectGetMidY(capsule.frame)-CGRectGetMidY(extraInDock))<0.01 &&
        fabs(capsule.layer.cornerRadius-31)<0.01 && fabs(extraMaterial.layer.cornerRadius-31)<0.01,
        @"capsule and independent action are equally thick with aligned circular ends");
    Check(fabs(CGRectGetMinX(extraInDock)-CGRectGetMaxX(capsule.frame)-12)<0.01 &&
        fabs(CGRectGetWidth(CGRectUnion(capsule.frame,extraInDock))/dock.bounds.size.width-0.905)<0.001,
        @"reference-width dock uses symmetric margins and a twelve-point accessory gap");
    BOOL reduceTransparency=UIAccessibilityIsReduceTransparencyEnabled();
    Check(selection && !selection.hidden && !selection.userInteractionEnabled &&
        HGADockNeutralTranslucent(selection.backgroundColor,selection.traitCollection) &&
        !capsuleMaterial.maskView && (reduceTransparency ||
            [selection.backgroundColor isEqual:[UIColor colorWithWhite:0 alpha:0.30]]),
        @"selection remains translucent over the unmasked system material");
    Check(CGRectGetMinY(selection.frame)>=3 && CGRectGetMaxY(selection.frame)<=capsule.bounds.size.height-3 &&
        selection.bounds.size.width<=capsule.bounds.size.height*1.53 && home.bounds.size.width>=44,
        @"two-tab dock keeps a compact inset selection pill and full accessible touch targets");
    Check(capsule.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark &&
        host.overrideUserInterfaceStyle==UIUserInterfaceStyleLight,
        @"video dock keeps dark glass without changing the host light theme");
    HGAppearanceSet(@"dock.tint",@"#604080",nil);
    Layout(capsuleMaterial);Layout(extraMaterial);
    UIColor *customColor=[HGAppearanceColor(@"#604080",nil)
        colorWithAlphaComponent:UIAccessibilityIsReduceTransparencyEnabled()?1:0.30];
    Check([capsuleMaterial.contentView.backgroundColor isEqual:customColor] &&
        [extraMaterial.contentView.backgroundColor isEqual:customColor],
        @"custom dock tint colors both the capsule and the independent action");
    HGAppearanceSet(@"dock.tint",@"",nil);
    Layout(capsuleMaterial);Layout(extraMaterial);
    Check(!home.automaticallyUpdatesConfiguration && !home.configuration.automaticallyUpdateForSelection,
        @"dock owns selection appearance instead of inheriting UIKit accent selection");
    HGADockPixelCounts normal=HGADockPixels(home),accessory=HGADockPixels(extra);
    Check(normal.brightNeutral>8 && accessory.brightNeutral>8,
        @"tab and independent profile glyphs render white on a dark surface");
    Check(normal.blue==0 && accessory.blue==0,
        @"rendered selected tab and independent button contain no inherited blue accent");
    CGFloat activeOpacity=CGColorGetAlpha([home.configuration.baseForegroundColor resolvedColorWithTraitCollection:home.traitCollection].CGColor);
    CGFloat inactiveOpacity=CGColorGetAlpha([theater.configuration.baseForegroundColor resolvedColorWithTraitCollection:theater.traitCollection].CGColor);
    Check(inactiveOpacity>0.50 && inactiveOpacity<activeOpacity && activeOpacity>0.90,
        @"unselected navigation glyphs stay neutral and quieter than the selected tab");
    Check(MAX(home.imageView.bounds.size.width,home.imageView.bounds.size.height)<36 &&
        MAX(extra.imageView.bounds.size.width,extra.imageView.bounds.size.height)<36,
        @"R15 default icons fit the thicker dock and equally sized independent action");
    home.highlighted=YES;[home setNeedsUpdateConfiguration];Layout(home);
    HGADockPixelCounts pressed=HGADockPixels(home);
    Check(pressed.blue==0 && HGADockNeutralTranslucent(selection.backgroundColor,selection.traitCollection),
        @"pressed selection stays neutral after a UIKit configuration update");
    home.highlighted=NO;
    Check(HGASaveDockPreview(host,dock),@"controlled UIKit dock preview renders to PNG for visual review");
    host.selectedIndex=1;HGAppearanceLayoutDock(host.customTabBar,host);Layout(dock);
    Check(!home.selected && theater.selected &&
        fabs(CGRectGetMidX(selection.frame)-CGRectGetMidX(theater.frame))<0.01 &&
        !capsuleMaterial.maskView,
        @"selection follows the host's selected tab over one continuous material");
    HGADockPixelCounts switched=HGADockPixels(theater);
    Check(switched.blue==0 && switched.brightNeutral>8,
        @"newly selected theater renders white after the selected state changes");
    HGAppearanceSet(@"dock.selected",@"#26B37E",nil);Layout(theater);
    CGFloat r=0,g=0,b=0,a=0;
    [theater.configuration.baseForegroundColor getRed:&r green:&g blue:&b alpha:&a];
    Check(fabs(r-38/255.0)<0.001 && fabs(g-179/255.0)<0.001 &&
        HGADockNeutralTranslucent(selection.backgroundColor,selection.traitCollection),
        @"explicit user glyph colors are preserved while selection background remains neutral");
    // Model the host switching from dark playback into a light catalogue page
    // even while the app's trait differs from its per-tab appearance.
    host.overrideUserInterfaceStyle=UIUserInterfaceStyleDark;
    theme.selectedImageColor=UIColor.blackColor;
    HGAppearanceSet(@"dock.selected",@"",nil);
    HGAppearanceLayoutDock(host.customTabBar,host);Layout(capsuleMaterial);Layout(extraMaterial);
    UIColor *lightFill=[UIColor colorWithWhite:1 alpha:reduceTransparency?1:0];
    Class expectedEffect=HGNativeGlassConfigured() ? NSClassFromString(@"UIGlassEffect") : UIBlurEffect.class;
    Check(capsule.traitCollection.userInterfaceStyle==UIUserInterfaceStyleLight && !capsuleMaterial.maskView &&
        extra.traitCollection.userInterfaceStyle==UIUserInterfaceStyleLight &&
        [capsuleMaterial.contentView.backgroundColor isEqual:lightFill] &&
        [extraMaterial.contentView.backgroundColor isEqual:lightFill] &&
        (reduceTransparency ? !capsuleMaterial.effect && !extraMaterial.effect :
            [capsuleMaterial.effect isKindOfClass:expectedEffect] && [extraMaterial.effect isKindOfClass:expectedEffect]),
        @"light catalogue keeps regular system glass across both dock shapes");
    Check(HGADockGlyphMatches(home,NO) && HGADockGlyphMatches(theater,NO) && HGADockGlyphMatches(extra,NO),
        @"light catalogue keeps selected unselected and independent glyphs readable");
    HGAppearanceSet(@"dock.material",@"blur",nil);Layout(capsuleMaterial);Layout(extraMaterial);
    Check((reduceTransparency ? !capsuleMaterial.effect && !extraMaterial.effect :
        [capsuleMaterial.effect isKindOfClass:UIBlurEffect.class] && [extraMaterial.effect isKindOfClass:UIBlurEffect.class]) &&
        capsuleMaterial.alpha==1 && extraMaterial.alpha==1 && capsule.layer.borderWidth==0 &&
        [capsuleMaterial.contentView.backgroundColor isEqual:lightFill] &&
        !Find(capsule,@"honggo.appearance.dock.diffuser"),
        @"classic blur uses one real blur effect in both shapes without a glass rim or opaque cover");
    UIVisualEffect *classicEffect=capsuleMaterial.effect;
    NSUInteger classicWrites=HGMaterialEffectSetterCount(capsuleMaterial);
    for(NSUInteger pass=0;pass<3;pass++){HGAppearanceLayoutDock(host.customTabBar,host);Layout(capsuleMaterial);}
    Check(HGMaterialEffectSetterCount(capsuleMaterial)==classicWrites &&
        (!classicEffect ? !capsuleMaterial.effect : [capsuleMaterial.effect isEqual:classicEffect]) &&
        HGADockGlyphMatches(home,NO) && HGADockGlyphMatches(extra,NO),
        @"classic blur survives repeated layout with readable foreground and stable effect identity");
    HGAppearanceSet(@"dock.material",@"clear",nil);Layout(capsuleMaterial);
    Check(reduceTransparency || !HGNativeGlassConfigured() ||
        [capsuleMaterial.effect isKindOfClass:NSClassFromString(@"UIGlassEffect")],
        @"explicit clear glass remains available on light pages");
    host.selectedIndex=0;
    HGAppearanceSet(@"dock.material",@"regular",nil);Layout(capsuleMaterial);
    Check(capsule.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark &&
        HGADockGlyphMatches(home,YES) && HGADockGlyphMatches(theater,YES) && HGADockGlyphMatches(extra,YES),
        @"returning to dark playback restores readable glyphs despite the stale catalogue foreground");
    CGPoint homePoint=[home convertPoint:CGPointMake(CGRectGetMidX(home.bounds),CGRectGetMidY(home.bounds)) toView:dock];
    CGPoint theaterPoint=[theater convertPoint:CGPointMake(CGRectGetMidX(theater.bounds),CGRectGetMidY(theater.bounds)) toView:dock];
    CGPoint extraPoint=[extra convertPoint:CGPointMake(CGRectGetMidX(extra.bounds),CGRectGetMidY(extra.bounds)) toView:dock];
    CGPoint paddingPoint=[capsule convertPoint:CGPointMake(2,CGRectGetMidY(capsule.bounds)) toView:dock];
    CGPoint clearCorner=[capsule convertPoint:CGPointMake(1,1) toView:dock];
    NSUInteger taps=host.nativeTaps;
    Check([dock hitTest:homePoint withEvent:nil]==home && [dock hitTest:theaterPoint withEvent:nil]==theater &&
        [dock hitTest:extraPoint withEvent:nil]==extra && [dock hitTest:paddingPoint withEvent:nil]==capsule &&
        ![dock hitTest:clearCorner withEvent:nil],
        @"button slots and rounded capsule padding acquire touches, while transparent corners pass through");
    [home sendActionsForControlEvents:UIControlEventTouchUpInside];
    [theater sendActionsForControlEvents:UIControlEventTouchUpInside];
    [extra sendActionsForControlEvents:UIControlEventTouchUpInside];
    Check(host.nativeTaps==taps+3 && host.selectedIndex==3,@"dock and accessory each forward one native navigation action");
    // FanqieX-style auto-detach: when all 4 tabs are unhidden and dock.action is a tab,
    // that tab moves out of the main capsule into the separate circular glass button.
    HGAppearanceApply(@{@"dock.hide.home":@NO,@"dock.hide.theater":@NO,@"dock.hide.store":@NO,@"dock.hide.mine":@NO,
        @"dock.extra":@YES,@"dock.action":@"mine"},nil);
    host.selectedIndex=3;HGAppearanceLayoutDock(host.customTabBar,host);Layout(dock);
    UIView *mineInCapsule=Find(dock,@"honggo.appearance.tab.mine");
    UIView *extraSel=Find(dock,@"honggo.appearance.dock.extra.selection");
    Check(mineInCapsule.hidden && !extra.hidden && extra.selected &&
        (extra.accessibilityTraits & UIAccessibilityTraitSelected)!=0 &&
        extraSel && !extraSel.hidden && selection.hidden,
        @"detaching a tab into the circular dock button removes it from the capsule and highlights selection on the circle");
    HGAppearanceApply(@{@"dock.hide.store":@YES,@"dock.hide.mine":@YES},nil);
    host.selectedIndex=0;HGAppearanceLayoutDock(host.customTabBar,host);Layout(dock);
    HGRunDockDragChecks(host,(id)dock,capsule,selection,home,theater);
    host.overrideUserInterfaceStyle=UIUserInterfaceStyleLight;
    HGAppearanceApply(@{@"dock.material":@"solid",@"dock.selected":@""},nil);Layout(dock);
    Check(capsule.traitCollection.userInterfaceStyle==UIUserInterfaceStyleLight && !capsuleMaterial.maskView,
        @"solid dock respects the host light appearance with continuous solid fill");
    objc_setAssociatedObject(host.customTabBar,&HGADockThemeFixtureKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    host.selectedIndex=savedIndex;host.overrideUserInterfaceStyle=savedStyle;host.view.tintColor=savedTint;
    host.customTabBar.frame=savedBarFrame;
    HGAppearanceApply(saved,nil);HGAppearanceLayoutDock(host.customTabBar,host);
}
