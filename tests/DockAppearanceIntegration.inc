// Included after the controlled host, Check, Find and Layout helpers. These
// checks render real UIKit button layers and exercise their state changes.
#import <QuartzCore/QuartzCore.h>
#import "RuntimeSupport.h"
#import "SiriaiDockMotion.h"

@interface HGADockThemeFixture : NSObject
@property(nonatomic,strong) UIColor *selectedImageColor;
@end
@implementation HGADockThemeFixture @end
static char HGADockThemeFixtureKey;
@implementation SSTabBar (HGADockThemeFixture)
- (HGADockThemeFixture *)appearanceStyle { return objc_getAssociatedObject(self,&HGADockThemeFixtureKey); }
@end
static NSUInteger HGFixtureDockImpacts;
@implementation SSTabBarController (HGADockFeedbackFixture)
- (void)impactFeedback { HGFixtureDockImpacts++; }
@end
@interface HGAAppearanceDock (HGATestQuietLayout)
- (void)renderGeometry;
- (NSString *)applySystemMaterial:(NSString *)style;
- (BOOL)isFeedCompact;
- (BOOL)feedControlsHidden;
- (CALayer *)siriaiMinimumFill;
- (DKFeedSpringState)feedSpring;
- (void)setFeedSpring:(DKFeedSpringState)state;
- (void)setFeedCompact:(BOOL)compact animated:(BOOL)animated;
- (void)advanceFeedSpring:(CADisplayLink *)link;
@end
@interface NSObject (HGATestDockScrollObserver)
+ (instancetype)shared;
- (void)pan:(UIPanGestureRecognizer *)gesture;
@end
static void HGRunDockFeedMotionChecks(SSTabBarController *host);
@interface HGTestDockStoredPlatter : UIView {
@public
    UIVisualEffect *_glassEffect;
}
@end
@implementation HGTestDockStoredPlatter @end
@interface HGTestDockCopyPropertyPlatter : UIView
@property(nonatomic) NSUInteger glassWrites;
@property(nonatomic,strong) UIVisualEffect *backingEffect;
@end
@implementation HGTestDockCopyPropertyPlatter
- (UIVisualEffect *)glassEffect {
    if(@available(iOS 26.0,*)) {
        if([self.backingEffect isKindOfClass:UIGlassEffect.class]) {
            UIGlassEffect *copy=[UIGlassEffect effectWithStyle:UIGlassEffectStyleRegular];
            copy.interactive=((UIGlassEffect *)self.backingEffect).interactive;
            return copy;
        }
    }
    return self.backingEffect;
}
- (void)setGlassEffect:(UIVisualEffect *)effect {
    self.glassWrites++;
    self.backingEffect=effect;
}
@end
@interface HGTestDockKVCOnlyPlatter : UIView
@property(nonatomic) NSUInteger kvcWrites;
@property(nonatomic,strong) UIVisualEffect *internalGlass;
@end
@implementation HGTestDockKVCOnlyPlatter
- (id)valueForKey:(NSString *)key {
    if([key isEqualToString:@"glassEffect"])return self.internalGlass;
    return [super valueForKey:key];
}
- (void)setValue:(id)value forKey:(NSString *)key {
    if([key isEqualToString:@"glassEffect"]){self.kvcWrites++;self.internalGlass=value;return;}
    [super setValue:value forKey:key];
}
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
    UIColor *color=[(button.configuration.baseForegroundColor ?: button.tintColor) resolvedColorWithTraitCollection:button.traitCollection];
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
        Check(selectedTone>0.03 && selectedTone>=glassTone-0.025 && selectedTone<=glassTone+0.20,
            @"the idle indicator preserves the underlying glass without casting a dark patch");
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
    Check([bar.backgroundColor isEqual:UIColor.systemOrangeColor],
        @"a host bar without a mounted custom Dock keeps its assigned fill");
    [host.view addSubview:bar];HGAppearanceLayoutDock(bar,host);Layout(bar);
    Check(source.hidden && Find(bar,@"honggo.appearance.dock"),
        @"restoration reentry fixture starts with a hidden observed native source");
    BOOL transparent=YES;
    for(UIColor *fill in @[UIColor.whiteColor,UIColor.blackColor,UIColor.whiteColor]) {
        bar.backgroundColor=fill;
        transparent &= CGColorGetAlpha(bar.backgroundColor.CGColor)==0;
    }
    Check(transparent,@"repeated light and dark page settlements keep the root clear before another layout");
    HGAppearanceSuspendDockForSidebar(bar);
    bar.backgroundColor=UIColor.blackColor;
    Check(CGColorGetAlpha(bar.backgroundColor.CGColor)==0 &&
        !Find(bar,@"honggo.appearance.dock").userInteractionEnabled,
        @"a host recolor during the drawer leaves the Dock frozen and the native strip transparent");
    HGAppearanceLayoutDock(bar,host);
    bar.backgroundColor=UIColor.whiteColor;
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
    Check([bar.backgroundColor isEqual:UIColor.whiteColor] && bar.opaque,
        @"nested restore returns the last host-requested color and opacity");
    // The coalesced tab-bar update must run after the restore has completed,
    // then legacy mode can acquire the complete original baseline safely.
    CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.02,false);
    Layout(bar);bar.hidden=YES;
    Check([bar.backgroundColor isEqual:UIColor.whiteColor] && bar.opaque && !source.hidden,
        @"deferred legacy reconciliation retains the restored host baseline");
    source.onRestore=nil;
    [bar removeFromSuperview];
    HGAppearanceApply(saved,nil);Layout(host.customTabBar);
}

@interface SSTabBarTextButton : UIControl
@property(nonatomic,copy) void (^tabBarButtonTappedBlock)(UIControl *);
@end
@implementation SSTabBarTextButton @end

static void HGRunLateDockActionChecks(SSTabBarController *host) {
    NSDictionary *saved=HGAppearanceValues();
    HGAppearanceApply(@{@"enabled":@NO,@"dock.enabled":@YES,@"dock.style":@"glass",@"dock.material":@"regular",
        @"dock.extra":@NO,@"dock.radius":@20,@"dock.haptics":@NO,@"dock.hide.home":@NO,@"dock.hide.theater":@NO,
        @"dock.hide.store":@NO,@"dock.hide.mine":@NO},nil);
    SSTabBarController *fixture=[SSTabBarController new];
    [host addChildViewController:fixture];
    fixture.view.frame=host.view.bounds;
    SSTabBar *bar=fixture.customTabBar;
    bar.frame=CGRectMake(0,CGRectGetHeight(fixture.view.bounds)-84,402,84);
    bar.contentView.frame=bar.bounds;
    UIControl *original=bar.contentView.subviews.lastObject;
    SSTabBarTextButton *last=[[SSTabBarTextButton alloc] initWithFrame:original.frame];
    last.tag=original.tag;
    [last addSubview:original.subviews.firstObject];
    [original removeFromSuperview];
    [bar.contentView addSubview:last];
    NSArray<UIControl *> *items=bar.contentView.subviews.copy;
    for (NSUInteger i=0;i<items.count;i++) {
        items[i].frame=CGRectMake(402.0*i/4,0,402.0/4,84);
        items[i].subviews.firstObject.frame=items[i].bounds;
    }
    UIColor *fill=UIColor.systemOrangeColor;
    bar.backgroundColor=fill;
    bar.opaque=YES;
    [host.view addSubview:fixture.view];
    [fixture didMoveToParentViewController:host];
    Layout(bar);
    // Enable while already mounted. Do not request layout before inspecting the result.
    HGAppearanceSet(@"enabled",@YES,nil);
    UIVisualEffectView *fallback=(id)Find(bar,@"honggo.tabbar.glass");
    Check(items.count==4 && !Find(bar,@"honggo.appearance.dock") && fallback.superview==bar &&
        fallback.effect && !fallback.hidden && !fallback.userInteractionEnabled && !bar.contentView.hidden &&
        [bar.subviews indexOfObject:fallback]<[bar.subviews indexOfObject:bar.contentView],
        @"enabling appearance while the fourth text tab is unbound mounts a material below the live native tabs");
    if (HGNativeGlassConfigured()) {
        UIVisualEffect *previous=fallback.effect;
        HGAppearanceSet(@"dock.material",@"clear",nil); Layout(bar);
        UIVisualEffect *clear=fallback.effect;
        BOOL changed=[[fallback valueForKey:@"clearGlass"] boolValue] && clear && clear!=previous;
        HGAppearanceSet(@"dock.material",@"regular",nil); Layout(bar);
        Check(changed && ![[fallback valueForKey:@"clearGlass"] boolValue] &&
            fallback.effect && fallback.effect!=clear && fallback==Find(bar,@"honggo.tabbar.glass"),
            @"the mounted text-bar backdrop changes between Clear and Regular without replacing the host tabs");
    }
    UIControl *third=items[2];
    CGPoint point=[third convertPoint:CGPointMake(CGRectGetMidX(third.bounds),CGRectGetMidY(third.bounds)) toView:bar];
    UIView *hit=[bar hitTest:point withEvent:nil];
    if (hit==third) [third sendActionsForControlEvents:UIControlEventTouchUpInside];
    Check(hit==third && fixture.nativeTaps==1 && fixture.selectedIndex==2,
        @"the fallback passes the native text tab hit and its original action through exactly once");
    NSUInteger before=HGFixtureDockImpacts;
    [fixture impactFeedback];
    Check(HGFixtureDockImpacts==before,@"the disabled haptic switch also silences the original unbound text bar");
    fill=UIColor.systemGrayColor;
    bar.backgroundColor=fill;
    Check(CGColorGetAlpha(bar.backgroundColor.CGColor)==0,
        @"a settled native page fill stays transparent behind the fallback capsule before the next layout");
    __weak SSTabBarController *weakFixture=fixture;
    last.tabBarButtonTappedBlock=^(UIControl *sender){[weakFixture nativeTap:sender];};
    Layout(bar);
    UIView *dock=Find(bar,@"honggo.appearance.dock");
    UIControl *mine=(id)Find(dock,@"honggo.appearance.tab.mine");
    [mine sendActionsForControlEvents:UIControlEventTouchUpInside];
    Check(dock.superview==bar && bar.contentView.hidden && !fallback.superview &&
        !Find(bar,@"honggo.tabbar.glass") && fixture.nativeTaps==2 && fixture.selectedIndex==3,
        @"binding the host text tab callback upgrades to one custom Dock and forwards its original control once");
    HGAppearanceSet(@"dock.width",@0.70,nil);Layout(bar);
    UIView *capsule=Find(dock,@"honggo.appearance.dock.capsule");
    CGFloat narrow=CGRectGetWidth(capsule.bounds);
    HGAppearanceSet(@"dock.width",@0.95,nil);Layout(bar);
    Check(fabs(narrow-402*0.70)<1 && fabs(CGRectGetWidth(capsule.bounds)-402*0.95)<1,
        @"both configured widths reach the mounted four-item text-tab capsule");
    HGAppearanceSet(@"dock.hide.store",@YES,nil);Layout(bar);
    Check(Find(dock,@"honggo.appearance.tab.store").hidden && bar.contentView.hidden && !mine.hidden,
        @"the text-tab host hides both copies of Store with the remaining navigation available");
    HGAppearanceSet(@"enabled",@NO,nil);
    Check(!bar.contentView.hidden && !Find(bar,@"honggo.appearance.dock") && !Find(bar,@"honggo.tabbar.glass") &&
        [bar.backgroundColor isEqual:fill] && bar.opaque && [bar hitTest:point withEvent:nil]==third,
        @"disabling the master immediately restores native color, opacity and navigation after renderer handoff");
    [fixture willMoveToParentViewController:nil];
    [fixture.view removeFromSuperview];
    [fixture removeFromParentViewController];
    HGAppearanceApply(saved,nil);
}

static void HGRunDockSurfaceChecks(SSTabBarController *host) {
    NSDictionary *saved=HGAppearanceValues();
    SSTabBar *bar=host.customTabBar;
    CALayer *savedMask=bar.layer.mask;
    BOOL savedClip=bar.clipsToBounds, savedHidden=bar.hidden;
    CGFloat width=host.view.bounds.size.width, bottom=host.view.bounds.size.height;
    CGFloat barTop=[bar convertRect:bar.bounds toView:host.view].origin.y;
    HGAppearanceApply(@{@"enabled":@YES,@"dock.enabled":@YES,@"dock.extra":@NO,@"dock.radius":@20,
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
    Check(!capsule.hidden && Find(capsule,@"honggo.appearance.material") &&
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
    CGRect homeFrame=bar.frame;
    bar.frame=CGRectOffset(homeFrame,-70,0);
    bar.hidden=NO;
    Layout(bar);
    Check(Find(bar,@"honggo.appearance.dock") && bar.contentView.hidden &&
        CGColorGetAlpha(bar.backgroundColor.CGColor)==0 && CGRectEqualToRect(bar.frame,CGRectOffset(homeFrame,-70,0)),
        @"the Mine drawer can reveal a previously hidden bar mid-slide without exposing its stock labels or fill");
    bar.frame=homeFrame;
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
    HGAppearanceSet(@"dock.radius",@20,nil);
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

    HGAppearanceApply(@{@"dock.width":@0.905,@"dock.height":@62,@"dock.radius":@20,
        @"dock.iconSize":@26,@"dock.extraSize":@62},nil);
    HGAppearanceLayoutDock(bar,host);Layout(home);
    CGFloat customIcon=MAX(home.imageView.bounds.size.width,home.imageView.bounds.size.height);
    Check(stockIcon>0 && customIcon>stockIcon && customIcon<=36,
        @"the stock glyph is smaller and custom sizes still scale proportionally");

    bar.frame=CGRectMake(0,rootHeight-60,320,60);
    HGAppearanceApply(HGAppearancePreset(@"glass"),nil);
    HGAppearanceSet(@"dock.radius",@20,nil);
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
- (BOOL)acceptsDockTouchAtPoint:(CGPoint)point fromView:(UIView *)hit;
- (void)beginDockTouchAtPoint:(CGPoint)point;
- (void)moveDockTouchToPoint:(CGPoint)point;
- (void)endDockTouchAtPoint:(CGPoint)point cancelled:(BOOL)cancelled;
@end

@interface HGADockCompatibilityWrapperFixture : UIView @end
@implementation HGADockCompatibilityWrapperFixture
- (Class)class { return NSClassFromString(@"UIKit._UITabBarContainerWrapperView") ?: [super class]; }
@end

static UIVisualEffectView *HGDockVisibleSelectionLens(UIView *selection) {
    UIVisualEffectView *lens=(id)Find(selection.superview,@"honggo.appearance.dock.drag-lens");
    return !lens.hidden && !selection.hidden ? lens : nil;
}

static void HGWaitForDockSelectionLensToSettle(UIView *selection) {
    HGAAppearanceDock *dock=(id)selection.superview.superview;
    CFTimeInterval deadline=CACurrentMediaTime()+0.55;
    while([[dock valueForKey:@"selectionSettling"] boolValue] && CACurrentMediaTime()<deadline)
        CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.02,false);
}

static void HGRunDockPressGestureChecks(SSTabBarController *host,HGAAppearanceDock *dock,UIView *capsule,
    UIView *selection,UIButton *home,UIButton *theater) {
    UIView *bar=host.customTabBar,*container=bar.superview;
    UIGestureRecognizer *installed=nil;
    for(UIGestureRecognizer *gesture in container.gestureRecognizers)
        if(gesture.delegate==(id)dock && [gesture.name isEqual:@"Honggo.DockPointer"])installed=gesture;
    Check(installed && installed.delaysTouchesBegan && installed.cancelsTouchesInView,
        @"the actual shared bar parent owns a finger-down recognizer for the Dock");
    CGPoint start=[home convertPoint:CGPointMake(CGRectGetMidX(home.bounds),CGRectGetMidY(home.bounds)) toView:dock];
    CGPoint finish=[theater convertPoint:CGPointMake(CGRectGetMidX(theater.bounds),CGRectGetMidY(theater.bounds)) toView:dock];
    Check([dock acceptsDockTouchAtPoint:start fromView:home] &&
        ![dock acceptsDockTouchAtPoint:CGPointMake(start.x,CGRectGetMaxY(capsule.frame)+4) fromView:bar],
        @"Dock presses claim visible tab surfaces and pass through the home-indicator strip below them");
    HGADockCompatibilityWrapperFixture *wrapper=[[HGADockCompatibilityWrapperFixture alloc] initWithFrame:bar.frame];
    [container insertSubview:wrapper aboveSubview:bar];
    if(NSClassFromString(@"UIKit._UITabBarContainerWrapperView")) {
        UIView *hit=[container hitTest:[dock convertPoint:start toView:container] withEvent:nil];
        Check(hit==wrapper && [dock acceptsDockTouchAtPoint:start fromView:hit],
            @"the verified UIKit wrapper may win hit testing without intercepting Dock motion");
        UIButton *visibleControl=[[UIButton alloc] initWithFrame:wrapper.bounds];
        [wrapper addSubview:visibleControl];
        Check(![dock acceptsDockTouchAtPoint:start fromView:visibleControl],
            @"the wrapper route leaves real controls alone");
        [visibleControl removeFromSuperview];
    }
    NSUInteger taps=host.nativeTaps;
    CGFloat resting=selection.bounds.size.height;
    CGFloat restingWidth=selection.bounds.size.width;
    UIView *highlight=Find(selection,@"honggo.appearance.dock.selection-highlight");
    [dock beginDockTouchAtPoint:start];
    if(!UIAccessibilityIsReduceMotionEnabled())
        Check(selection.bounds.size.height>resting+capsule.bounds.size.height*0.10 &&
            selection.bounds.size.width>restingWidth && CGRectGetMinY(selection.frame)<0 &&
            selection.alpha==1 && highlight.alpha==1 && (HGDockVisibleSelectionLens(selection) ?
            CGAffineTransformIsIdentity(home.imageView.transform) : home.imageView.transform.a>1.10),
            @"same-tab press lifts the selection with native optics or the non-glass highlight fallback");
    HGAppearanceLayoutDock(bar,host);Layout(bar);
    [dock moveDockTouchToPoint:CGPointMake((start.x+finish.x)/2,start.y)];
    Check(host.nativeTaps==taps && fabs(CGRectGetMidX(selection.frame)-(CGRectGetMidX(home.frame)+CGRectGetMidX(theater.frame))/2)<0.01,
        @"after host layout the same press tracks a live intermediate drag position without switching tabs");
    [home sendActionsForControlEvents:UIControlEventTouchUpInside];
    [dock endDockTouchAtPoint:finish cancelled:NO];
    [dock endDockTouchAtPoint:finish cancelled:NO];
    Check(host.nativeTaps==taps+1 && host.selectedIndex==1,
        @"shared Dock drag activation forwards one action even with an underlying control event and repeated end");
    Check(highlight.alpha==0 &&
        CGAffineTransformIsIdentity(theater.imageView.transform),
        @"release moves only the selection, fades the highlight and returns the glyph to rest");
    HGWaitForDockSelectionLensToSettle(selection);
    Check(!HGDockVisibleSelectionLens(selection),@"shared Dock release hides the settled glass lens");

    [dock beginDockTouchAtPoint:start];
    if(!UIAccessibilityIsReduceMotionEnabled()) {
        CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.09,false);
        CALayer *held=selection.layer.presentationLayer;
        Check(held && CGRectGetHeight(held.bounds)>resting+2 && host.nativeTaps==taps+1,
            @"the enlarged press reaches the display while the finger is still down and navigation is unchanged");
    }
    // Slight vertical thumb roll inside the capsule must not cancel a quick tap.
    [dock moveDockTouchToPoint:CGPointMake(start.x,start.y+15)];
    [dock endDockTouchAtPoint:start cancelled:NO];
    Check(host.nativeTaps==taps+2 && host.selectedIndex==0,
        @"release commits the selected destination immediately even after minor vertical thumb roll");
    CFTimeInterval settleBegin=CACurrentMediaTime();
    HGWaitForDockSelectionLensToSettle(selection);
    Check(CACurrentMediaTime()-settleBegin<0.40,
        @"Dock tap spring settles briskly without sluggish half-second trailing lag");
    [dock beginDockTouchAtPoint:start];
    [dock moveDockTouchToPoint:CGPointMake(finish.x,start.y)];
    [dock endDockTouchAtPoint:CGPointMake(finish.x,CGRectGetMinY(capsule.frame)-8) cancelled:NO];
    Check(host.nativeTaps==taps+2 && host.selectedIndex==0,
        @"a diagonal drag released above the Dock cancels without opening the destination");
    HGWaitForDockSelectionLensToSettle(selection);
    [wrapper removeFromSuperview];
}

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
        [dock acceptsSelectionTouchAtPoint:overlap inView:overlapHit] &&
        [dock acceptsDockTouchAtPoint:[capsule convertPoint:overlap toView:dock] fromView:overlapHit],
        @"a dock start can reach the pan through the shared ancestor even when the native scrubber wins hit testing, including the first stationary press");
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
    HGRunDockPressGestureChecks(host,dock,capsule,selection,home,theater);
    taps=host.nativeTaps;host.selectedIndex=0;HGAppearanceLayoutDock(host.customTabBar,host);
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
    HGAppearanceSet(@"dock.material",@"clear",nil);
    [theater sendActionsForControlEvents:UIControlEventTouchDown];
    Layout(HGDockVisibleSelectionLens(selection));
    UIView *previewHighlight=Find(selection,@"honggo.appearance.dock.selection-highlight");
    CGFloat edgeAllowance=UIAccessibilityIsReduceMotionEnabled() ? capsule.bounds.size.height*4.0/62.0 : 1;
    Check(CGRectGetMaxX(selection.frame)>=capsule.bounds.size.width-edgeAllowance-0.01 && host.nativeTaps==taps &&
        (UIAccessibilityIsReduceMotionEnabled() ||
         (previewHighlight.alpha==1 && !HGDockVisibleSelectionLens(selection) && theater.imageView.transform.a>1.10)),
        @"compatibility glass press glides its single recessed selection without stacking a second refracting lens");
    [theater sendActionsForControlEvents:UIControlEventTouchCancel];
    CGFloat homeFaceX=[home convertPoint:home.imageView.center toView:capsule].x;
    Check(fabs(CGRectGetMidX(selection.frame)-homeFaceX)<0.01 && host.nativeTaps==taps,
        @"cancelling a press springs back without sending a navigation action");
    [theater sendActionsForControlEvents:UIControlEventTouchDown];
    [theater sendActionsForControlEvents:UIControlEventTouchUpInside];
    Check(host.nativeTaps==taps+1 && host.selectedIndex==1 && theater.selected && !HGDockVisibleSelectionLens(selection),
        @"compatibility glass press and release send one navigation action without double-refraction lens artifacts");
    HGWaitForDockSelectionLensToSettle(selection);
    Check(!HGDockVisibleSelectionLens(selection),@"finished tap spring leaves no stacked lens");
    taps=host.nativeTaps;host.selectedIndex=0;HGAppearanceLayoutDock(host.customTabBar,host);
    HGADockPanFixture *pan=[HGADockPanFixture new];
    [home sendActionsForControlEvents:UIControlEventTouchDown];
    pan.fixtureLocation=CGPointMake(homeX,20);pan.fixtureState=UIGestureRecognizerStateBegan;
    [dock dragSelection:pan];
    [home sendActionsForControlEvents:UIControlEventTouchCancel];
    [home sendActionsForControlEvents:UIControlEventTouchUpInside];
    pan.fixtureLocation=CGPointMake(middle,20);pan.fixtureState=UIGestureRecognizerStateChanged;
    [dock dragSelection:pan];
    Check(!selection.hidden && fabs(CGRectGetMidX(selection.frame)-middle)<0.01 && host.nativeTaps==taps &&
        !HGDockVisibleSelectionLens(selection),
        @"compatibility glass drag follows an intermediate position over one continuous glass rail without double refraction");
    pan.fixtureState=UIGestureRecognizerStateCancelled;
    [dock dragSelection:pan];
    HGWaitForDockSelectionLensToSettle(selection);
    Check(host.nativeTaps==taps && host.selectedIndex==0 && fabs(CGRectGetMidX(selection.frame)-homeFaceX)<0.01 &&
        !HGDockVisibleSelectionLens(selection),@"cancelled drag returns to the original tab and hides the lens");

    pan.fixtureLocation=CGPointMake(homeX,20);pan.fixtureState=UIGestureRecognizerStateBegan;
    [dock dragSelection:pan];
    pan.fixtureLocation=CGPointMake(capsule.bounds.size.width+15,20);pan.fixtureState=UIGestureRecognizerStateChanged;
    [dock dragSelection:pan];
    Check(fabs(CGRectGetMaxX(selection.frame)-capsule.bounds.size.width)<0.01,
        @"drag clamps to the last visible tab when other native tabs are hidden");
    pan.fixtureState=UIGestureRecognizerStateEnded;[dock dragSelection:pan];
    HGWaitForDockSelectionLensToSettle(selection);
    Check(host.nativeTaps==taps+1 && host.selectedIndex==1 && theater.selected &&
        !HGDockVisibleSelectionLens(selection),@"drag release forwards one original action to the destination tab");

    HGAppearanceSet(@"dock.material",@"blur",nil);
    pan.fixtureLocation=CGPointMake(middle,20);pan.fixtureState=UIGestureRecognizerStateBegan;
    [dock dragSelection:pan];
    Check(!HGDockVisibleSelectionLens(selection) && fabs(CGRectGetMidX(selection.frame)-middle)<0.01,
        @"classic blur keeps its material while supporting the same drag interaction");
    pan.fixtureState=UIGestureRecognizerStateCancelled;[dock dragSelection:pan];
    HGAppearanceSet(@"dock.material",@"regular",nil);
}

static BOOL HGADockDrawsEveryCaption(UIView *capsule,NSArray<UIButton *> *buttons) {
    if(!capsule.window || buttons.count==0)return NO;
    for(UIButton *button in buttons) {
        Layout(button);
        UILabel *title=button.titleLabel;
        if(button.hidden || title.hidden || CGRectIsEmpty(title.frame) || !button.currentImage ||
            !CGRectContainsRect(button.bounds,title.frame) || CGRectGetMinY(title.frame)<CGRectGetMaxY(button.imageView.frame)-0.5)return NO;
    }
    UIImage *visible=HGARenderDockView(capsule);
    [CATransaction begin];[CATransaction setDisableActions:YES];
    for(UIButton *button in buttons)button.titleLabel.layer.opacity=0;
    UIImage *masked=HGARenderDockView(capsule);
    for(UIButton *button in buttons)button.titleLabel.layer.opacity=1;
    [CATransaction commit];
    for(UIButton *button in buttons) {
        CGRect area=[button.titleLabel convertRect:button.titleLabel.bounds toView:capsule];
        CGRect pixels=CGRectIntegral(CGRectApplyAffineTransform(area,CGAffineTransformMakeScale(visible.scale,visible.scale)));
        CGImageRef ink=CGImageCreateWithImageInRect(visible.CGImage,pixels),blank=CGImageCreateWithImageInRect(masked.CGImage,pixels);
        BOOL drawn=ink && blank && ![UIImagePNGRepresentation([UIImage imageWithCGImage:ink])
            isEqual:UIImagePNGRepresentation([UIImage imageWithCGImage:blank])];
        if(ink)CGImageRelease(ink);
        if(blank)CGImageRelease(blank);
        if(!drawn)return NO;
    }
    return YES;
}
static BOOL HGADockFullGlass(UIVisualEffectView *surface) {
    if(@available(iOS 26.0,*)) {
        return [surface.effect isKindOfClass:UIGlassEffect.class] && !((UIGlassEffect *)surface.effect).tintColor &&
            !surface.maskView && (!surface.contentView.backgroundColor || CGColorGetAlpha(surface.contentView.backgroundColor.CGColor)==0);
    }
    return NO;
}

static void HGRunDockCaptionChecks(SSTabBarController *host) {
    NSDictionary *saved=HGAppearanceValues();
    NSUInteger previous=host.selectedIndex;
    host.selectedIndex=0;
    HGAppearanceApply(@{@"enabled":@YES,@"dock.enabled":@YES,@"dock.material":@"clear",@"dock.style":@"glass",
        @"dock.content":@"both",@"dock.haptics":@NO,@"dock.extra":@NO,@"dock.height":@58,@"dock.radius":@21,
        @"dock.hide.home":@NO,@"dock.hide.theater":@NO,@"dock.hide.mine":@NO},nil);
    for(NSString *style in @[@"glass",@"plain"]) {
        BOOL showStore=[style isEqual:@"plain"];
        BOOL applied=HGAppearanceApply(@{@"dock.selectionStyle":style,@"dock.hide.store":showStore ? @NO : @YES},nil);
        Check(applied && [HGAppearanceString(@"dock.selectionStyle") isEqual:style] &&
            HGAppearanceFlag(@"dock.hide.store")==!showStore,@"quiet Dock fixture applies its selected style and visible tab count");
        HGAppearanceLayoutDock(host.customTabBar,host);
        HGAAppearanceDock *dock=(id)Find(host.customTabBar,@"honggo.appearance.dock");
        UIView *capsule=Find(dock,@"honggo.appearance.dock.capsule");
        NSMutableArray *items=[NSMutableArray array];
        for(NSString *key in (showStore ? @[@"home",@"theater",@"store",@"mine"] : @[@"home",@"theater",@"mine"])) {
            UIView *button=Find(capsule,[@"honggo.appearance.tab." stringByAppendingString:key]);
            if(button)[items addObject:button];
        }
        Check(items.count==(showStore?4:3) && HGADockDrawsEveryCaption(capsule,items),
            [NSString stringWithFormat:@"%@ quiet-mode tab captions contribute pixels to the visible Dock",@(items.count)]);
        UIView *rest=Find(capsule,@"honggo.appearance.dock.selection");
        BOOL nativeGlass=HGNativeGlassConfigured() && !UIAccessibilityIsReduceTransparencyEnabled();
        Check(rest && !rest.hidden && !Find(capsule,@"honggo.appearance.dock.resting-lens") &&
            (showStore || !nativeGlass || [rest.backgroundColor isEqual:UIColor.secondarySystemFillColor]),
            @"quiet navigation has one recessed resting thumb with no second refracting surface");
        if(showStore) {
            UIButton *last=items.lastObject;
            NSUInteger taps=host.nativeTaps;
            CGPoint point=[last convertPoint:CGPointMake(CGRectGetMidX(last.bounds),CGRectGetMidY(last.bounds)) toView:dock];
            [dock beginDockTouchAtPoint:point];
            UIView *selection=Find(capsule,@"honggo.appearance.dock.selection");
            UIView *highlight=Find(selection,@"honggo.appearance.dock.selection-highlight");
            Check(!Find(capsule,@"honggo.appearance.dock.drag-lens") && highlight.alpha==0 && selection.layer.borderWidth==0 &&
                Find(capsule,@"honggo.appearance.material") && CGAffineTransformIsIdentity(last.imageView.transform),
                @"plain selection retains its glass rail while pressing without a lens or an enlarged symbol");
            [dock endDockTouchAtPoint:point cancelled:NO];
            Check(host.selectedIndex==3 && host.nativeTaps==taps+1 && HGADockDrawsEveryCaption(capsule,items),
                @"quiet four-tab selection forwards once and the switched Dock still paints every caption");
            HGWaitForDockSelectionLensToSettle(selection);
        }
    }
    HGAppearanceApply(@{@"dock.extra":@YES,@"dock.action":@"mine", @"dock.selectionStyle":@"glass"},nil);
    HGAppearanceLayoutDock(host.customTabBar,host);
    UIButton *extra=(id)Find(host.customTabBar,@"honggo.appearance.dock.extra");
    UIView *thumb=Find(extra.superview,@"honggo.appearance.dock.extra.selection");
    if(HGNativeGlassConfigured() && !UIAccessibilityIsReduceTransparencyEnabled())
        Check(extra.selected && thumb && !thumb.hidden && [thumb.backgroundColor isEqual:UIColor.secondarySystemFillColor] &&
            CGRectContainsRect(extra.frame,thumb.frame) && CGRectGetHeight(thumb.frame)<CGRectGetHeight(extra.frame),
            @"the detached selected tab carries the same inset resting thumb on its own circle");
    HGAppearanceSet(@"dock.selectionStyle",@"plain",nil);
    HGAppearanceLayoutDock(host.customTabBar,host);
    Check(extra.selected && !Find(extra.superview,@"honggo.appearance.dock.resting-lens"),
        @"plain selection keeps the detached tab current without a second glass surface");
    host.selectedIndex=previous;
    HGAppearanceApply(saved,nil);HGAppearanceLayoutDock(host.customTabBar,host);
}

static NSString *HGAFixtureReadOnlyMaterial(id dock,SEL selector,NSString *style) {return @"factory-glass";}
static void HGRunDockSystemAdapterChecks(SSTabBarController *host) {
    if(!HGNativeGlassConfigured() || UIAccessibilityIsReduceTransparencyEnabled())return;
    if(@available(iOS 26.0,*)) {
        HGAAppearanceDock *probe=[[HGAAppearanceDock alloc] initWithFrame:CGRectMake(0,0,400,84)];
        HGTestDockStoredPlatter *stored=[HGTestDockStoredPlatter new];
        stored->_glassEffect=[UIGlassEffect effectWithStyle:UIGlassEffectStyleRegular];
        [probe setValue:@0 forKey:@"systemHostSelection"];[probe setValue:stored forKey:@"systemPlatter"];
        Check([[probe applySystemMaterial:@"clear"] isEqual:@"clear-glass-slot"] &&
            [stored->_glassEffect isKindOfClass:UIGlassEffect.class] && !((UIGlassEffect *)stored->_glassEffect).tintColor &&
            stored.overrideUserInterfaceStyle==UIUserInterfaceStyleUnspecified,
            @"an item platter with a stored glass effect accepts the actual untinted Clear effect over video");
        HGTestDockCopyPropertyPlatter *copyPlatter=[HGTestDockCopyPropertyPlatter new];
        copyPlatter.backingEffect=[UIGlassEffect effectWithStyle:UIGlassEffectStyleRegular];
        [probe setValue:copyPlatter forKey:@"systemPlatter"];
        NSString *firstCopy=[probe applySystemMaterial:@"clear"];
        NSString *secondCopy=[probe applySystemMaterial:@"clear"];
        NSString *switchedCopy=[probe applySystemMaterial:@"regular"];
        Check([firstCopy isEqual:@"clear-glass"] && [secondCopy isEqual:@"clear-glass"] &&
            copyPlatter.glassWrites==2 && [switchedCopy isEqual:@"regular-glass"],
            @"copy-on-read platter glassEffect is not rewritten on unchanged layouts and updates on style switches");
        HGTestDockKVCOnlyPlatter *kvcPlatter=[HGTestDockKVCOnlyPlatter new];
        kvcPlatter.internalGlass=[UIGlassEffect effectWithStyle:UIGlassEffectStyleRegular];
        [probe setValue:kvcPlatter forKey:@"systemPlatter"];
        Check([[probe applySystemMaterial:@"clear"] isEqual:@"clear-glass"] &&
            [[probe applySystemMaterial:@"clear"] isEqual:@"clear-glass"] &&
            kvcPlatter.kvcWrites==1 && [kvcPlatter.internalGlass isKindOfClass:UIGlassEffect.class],
            @"Swift KVC-only platter accepts Clear glass once and skips redundant rewrites");
        UIView *factory=[UIView new];[probe setValue:factory forKey:@"systemPlatter"];
        Check([[probe applySystemMaterial:@"clear"] isEqual:@"factory-glass"] &&
            factory.overrideUserInterfaceStyle==UIUserInterfaceStyleUnspecified,
            @"a read-only video platter reports its unmodified factory material for native Clear fallback");
        [probe setValue:@1 forKey:@"systemHostSelection"];
        Check([[probe applySystemMaterial:@"clear"] isEqual:@"factory-glass"] &&
            factory.overrideUserInterfaceStyle==UIUserInterfaceStyleUnspecified,
            @"leaving playback releases the factory platter's optical override");
    }
    NSDictionary *saved=HGAppearanceValues();
    NSUInteger savedIndex=host.selectedIndex;
    UIUserInterfaceStyle savedStyle=host.overrideUserInterfaceStyle;
    host.overrideUserInterfaceStyle=UIUserInterfaceStyleLight;
    if(@available(iOS 17.0,*))[host.view updateTraitsIfNeeded];
    CGRect frame=host.customTabBar.frame;
    CGRect roomy=frame;roomy.size.height=84;roomy.origin.y=host.view.bounds.size.height-84;
    host.customTabBar.frame=roomy;host.selectedIndex=0;
    HGAppearanceApply(@{@"enabled":@YES,@"dock.enabled":@YES,@"dock.material":@"regular",
        @"dock.content":@"both",@"dock.haptics":@YES,@"dock.radius":@29,@"dock.height":@58,@"dock.extra":@NO,
        @"dock.width":@(HGADockDefaultWidth),@"dock.hide.home":@NO,@"dock.hide.theater":@NO,@"dock.hide.store":@NO,@"dock.hide.mine":@NO},nil);
    HGAppearanceLayoutDock(host.customTabBar,host);Layout(host.customTabBar);
    CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.02,false);
    HGAppearanceLayoutDock(host.customTabBar,host);
    UIView *dock=Find(host.customTabBar,@"honggo.appearance.dock");
    UIView *carrier=Find(dock,@"honggo.appearance.dock.system");
    UIView *capsule=Find(dock,@"honggo.appearance.dock.capsule");
    UIView *manual=Find(dock,@"honggo.appearance.tab.home");
    if(carrier && carrier.alpha>0.9) {
        UITabBar *native=[dock valueForKey:@"systemTabBar"];
        UITabBarController *managed=[dock valueForKey:@"systemTabController"];
        UIGestureRecognizer *dockPointer=[dock valueForKey:@"dockTouch"];
        Check([native isKindOfClass:UITabBar.class] && native.window==dock.window &&
            !Find(capsule,@"honggo.appearance.material") && manual.hidden && native.layer.speed==1 &&
            !dockPointer.enabled &&
            fabs(carrier.transform.a-carrier.transform.d)<0.001,
            @"the mounted UIKit floating Dock owns both selection and visible tab controls without root-view touch-delay interception");
        NSArray *videoFaces=[dock valueForKey:@"systemItemSignature"];
        Check(native.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark &&
            [native.tintColor isEqual:[UIColor.labelColor resolvedColorWithTraitCollection:native.traitCollection]],
            @"video playback keeps dark optical Regular glass and white tab faces even when the app theme is light");
        host.selectedIndex=1;
        HGAppearanceLayoutDock(host.customTabBar,host);Layout(native);
        Check([dock valueForKey:@"systemTabBar"]==native && native.traitCollection.userInterfaceStyle==UIUserInterfaceStyleLight &&
            [native.tintColor isEqual:[UIColor.labelColor resolvedColorWithTraitCollection:native.traitCollection]] &&
            ![videoFaces isEqual:[dock valueForKey:@"systemItemSignature"]],
            @"switching to a light catalogue page recolors the same native Dock and invalidates its composite glyphs and captions");
        host.selectedIndex=0;
        host.overrideUserInterfaceStyle=UIUserInterfaceStyleDark;
        if(@available(iOS 17.0,*))[host.view updateTraitsIfNeeded];
        HGAppearanceLayoutDock(host.customTabBar,host);Layout(native);
        NSUInteger taps=host.nativeTaps;
        if(managed) {
            if(@available(iOS 26.0,*)) {
                UITab *tab=[managed tabForIdentifier:@"theater"];
                Check(tab.title.length==0 && [tab.accessibilityLabel isEqual:@"剧场"] && tab.image.CGImage &&
                    tab.image.renderingMode==UIImageRenderingModeAlwaysOriginal,
                    @"managed tab keeps the caption and symbol in one face and exposes its spoken title");
                [managed.delegate tabBarController:managed didSelectTab:tab previousTab:managed.selectedTab];
            }
        }else {
            UITabBarItem *tab=native.items.count>=2 ? native.items[1] : nil;
            Check(native.items.count==4 && tab.tag==1,@"the standalone UIKit Dock owns four mapped UITabBarItems");
            Check(tab.title.length==0 && [tab.accessibilityLabel isEqual:@"剧场"] && tab.image.CGImage &&
                tab.selectedImage.CGImage && tab.selectedImage.renderingMode==UIImageRenderingModeAlwaysOriginal,
                @"standalone selection cannot omit or recolor the independently drawn caption");
            [native.delegate tabBar:native didSelectItem:tab];
        }
        Check(host.nativeTaps==taps+1 && host.selectedIndex==1,@"a UIKit selection forwards exactly one mapped host action");
        Check(HGAppearanceSuspendDockForSidebar(host.customTabBar) && !dock.userInteractionEnabled &&
            native.window==host.view.window && host.customTabBar.contentView.hidden,
            @"the drawer retains the mounted native platter with host labels hidden");
        HGAppearanceLayoutDock(host.customTabBar,host);
        Check(dock.userInteractionEnabled && [dock valueForKey:@"systemTabBar"]==native,
            @"returning from the drawer resumes the same UIKit tab bar");
    }else {
        Check(!carrier && !manual.hidden && Find(capsule,@"honggo.appearance.material"),
            @"a provider without a reachable floating platter leaves only the compatibility Dock mounted");
    }
    NSUInteger impacts=HGFixtureDockImpacts;
    UITabBar *touchBar=[dock valueForKey:@"systemTabBar"];
    [host impactFeedback];
    HGAppearanceSet(@"dock.haptics",@NO,nil);
    host.overrideUserInterfaceStyle=UIUserInterfaceStyleLight;
    if(@available(iOS 17.0,*))[host.view updateTraitsIfNeeded];
    host.selectedIndex=0;
    HGAppearanceLayoutDock(host.customTabBar,host);
    [host impactFeedback];
    UITabBar *quietBar=[dock valueForKey:@"systemTabBar"];
    Check(quietBar==touchBar && HGFixtureDockImpacts==impacts+1,
        @"turning feedback off keeps the working renderer and suppresses the mapped host impact");
    UIButton *theater=(id)Find(dock,@"honggo.appearance.tab.theater");
    NSUInteger taps=host.nativeTaps;
    if(quietBar) {
        CGPoint center=CGPointMake(CGRectGetMidX(theater.bounds),CGRectGetMidY(theater.bounds));
        UIView *hit=[dock hitTest:[theater convertPoint:center toView:dock] withEvent:nil];
        Check([[dock valueForKey:@"quietSystemTouches"] boolValue] && hit==theater &&
            !Find(capsule,@"honggo.appearance.material"),
            @"the silent slot owns the UIKit platter hit while retaining the native material");
        [(UIControl *)hit sendActionsForControlEvents:UIControlEventTouchUpInside];
        Check(host.selectedIndex==1 && host.nativeTaps==taps+1 && HGFixtureDockImpacts==impacts+1,
            @"native UIKit selection with haptics off forwards one tab action while suppressing host impact");
    }else {
        Check([theater.currentTitle isEqual:@"剧场"] && !theater.hidden,@"a quiet compatibility tab keeps its visible caption");
        [theater sendActionsForControlEvents:UIControlEventTouchUpInside];
        Check(host.selectedIndex==1 && host.nativeTaps==taps+1,@"quiet compatibility navigation delivers one original tab action");
    }
    HGAppearanceSet(@"dock.haptics",@YES,nil);HGAppearanceLayoutDock(host.customTabBar,host);
    if([dock valueForKey:@"systemTabBar"]) {
        Method material=class_getInstanceMethod(dock.class,sel_registerName("applySystemMaterial:"));
        IMP apply=method_setImplementation(material,(IMP)HGAFixtureReadOnlyMaterial);
        @try {
            host.selectedIndex=0;
            HGAppearanceSet(@"dock.material",@"clear",nil);HGAppearanceLayoutDock(host.customTabBar,host);
            UIView *glass=Find(capsule,@"honggo.appearance.material");
            Check(![dock valueForKey:@"systemTabBar"] && [[glass valueForKey:@"style"] isEqual:@"clear"] && !manual.hidden,
                @"a read-only platter hands off to one Clear glass capsule with live tab controls");
            for (NSNumber *hideStore in @[@NO,@YES]) {
                HGAppearanceSet(@"dock.hide.store",hideStore,nil);
                host.selectedIndex=0;HGAppearanceLayoutDock(host.customTabBar,host);
                for (NSUInteger pass=0;pass<3 && [[dock valueForKey:@"systemWarmupPending"] boolValue];pass++) {
                    CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.02,false);
                    HGAppearanceLayoutDock(host.customTabBar,host);
                }
                CGRect homeFrame=capsule.frame;
                for (NSNumber *index in @[@1,@3,@0]) {
                    host.selectedIndex=index.unsignedIntegerValue;HGAppearanceLayoutDock(host.customTabBar,host);
                    CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.02,false);HGAppearanceLayoutDock(host.customTabBar,host);
                    Check(![dock valueForKey:@"systemTabBar"] && CGRectEqualToRect(homeFrame,capsule.frame) &&
                        [[Find(capsule,@"honggo.appearance.material") valueForKey:@"style"] isEqual:@"clear"],
                        @"home, theatre and profile retain the selected Clear material and width across tab switches");
                }
            }
            HGAppearanceSet(@"dock.material",@"regular",nil);HGAppearanceLayoutDock(host.customTabBar,host);
            Check([[Find(capsule,@"honggo.appearance.material") valueForKey:@"style"] isEqual:@"regular"],
                @"the same read-only provider switches back to Regular on the mounted capsule");
        } @finally {
            method_setImplementation(material,apply);
        }
    } else {
        for (NSString *style in @[@"clear",@"regular"]) {
            HGAppearanceSet(@"dock.material",style,nil);
            HGAppearanceLayoutDock(host.customTabBar,host);
            UIView *glass=Find(capsule,@"honggo.appearance.material");
            Check([[glass valueForKey:@"style"] isEqual:style] && !manual.hidden,
                [NSString stringWithFormat:@"the initially fallback Dock displays %@ on its mounted capsule",style]);
        }
    }
    host.overrideUserInterfaceStyle=savedStyle;host.selectedIndex=savedIndex;host.customTabBar.frame=frame;
    HGAppearanceApply(saved,nil);HGAppearanceLayoutDock(host.customTabBar,host);
}

static void HGRunDockAppearanceChecks(SSTabBarController *host) {
    HGRunLateDockActionChecks(host);
    HGRunDockRestoreReentryChecks(host);
    HGRunDockSurfaceChecks(host);
    NSDictionary *saved=HGAppearanceValues();
    NSUInteger savedIndex=host.selectedIndex;
    CGRect savedBarFrame=host.customTabBar.frame;
    UIUserInterfaceStyle savedStyle=host.overrideUserInterfaceStyle;
    UIColor *savedTint=host.view.tintColor;
    host.overrideUserInterfaceStyle=UIUserInterfaceStyleLight;
    if(@available(iOS 17.0,*))[host.view updateTraitsIfNeeded];
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
        @"dock.content":@"icon",@"dock.height":@62,@"dock.width":@0.905,@"dock.radius":@30,
        @"dock.iconSize":@26,@"dock.extra":@YES,@"dock.extraSize":@62,@"dock.action":@"mine",
        @"dock.hide.home":@NO,@"dock.hide.theater":@NO,@"dock.hide.store":@YES,@"dock.hide.mine":@YES},nil);
    HGAppearanceLayoutDock(host.customTabBar,host);
    UIView *dock=Find(host.customTabBar,@"honggo.appearance.dock");
    UIView *capsule=Find(dock,@"honggo.appearance.dock.capsule");
    UIView *selection=Find(dock,@"honggo.appearance.dock.selection");
    UIButton *home=(id)Find(dock,@"honggo.appearance.tab.home");
    UIButton *theater=(id)Find(dock,@"honggo.appearance.tab.theater");
    UIButton *extra=(id)Find(dock,@"honggo.appearance.dock.extra");
    Layout(dock);Layout(home);Layout(extra);
    UIView *extraSurface=[dock valueForKey:@"extra"];
    UIVisualEffectView *extraMaterial=(id)Find(extraSurface,@"honggo.appearance.material");
    BOOL nativeGlass=HGNativeGlassConfigured() && !UIAccessibilityIsReduceTransparencyEnabled();
    UIVisualEffectView *capsuleMaterial=(id)Find(capsule,@"honggo.appearance.material");
    Check(capsule.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark &&
        extraSurface.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark &&
        HGADockGlyphMatches(home,YES) && HGADockGlyphMatches(theater,YES) && HGADockGlyphMatches(extra,YES),
        @"video playback keeps dark optical Regular glass and white Dock glyphs without whitening in light app mode");
    host.overrideUserInterfaceStyle=UIUserInterfaceStyleDark;
    if(@available(iOS 17.0,*))[host.view updateTraitsIfNeeded];
    HGAppearanceLayoutDock(host.customTabBar,host);Layout(dock);
    CGRect extraInDock=[extra convertRect:extra.bounds toView:dock];
    Check([extraMaterial isKindOfClass:UIVisualEffectView.class] && extra.superview==extraMaterial.contentView &&
        home.superview==capsule && theater.superview==capsule && selection.superview==capsule &&
        [capsule.subviews indexOfObjectIdenticalTo:capsuleMaterial]<[capsule.subviews indexOfObjectIdenticalTo:selection] &&
        [capsule.subviews indexOfObjectIdenticalTo:selection]<[capsule.subviews indexOfObjectIdenticalTo:home],
        @"the moving selection composites above the capsule material and below the glyphs");
    Check(fabs(capsule.bounds.size.height-62)<0.01 && fabs(extra.bounds.size.width-62)<0.01 &&
        fabs(extra.bounds.size.height-62)<0.01 && fabs(CGRectGetMidY(capsule.frame)-CGRectGetMidY(extraInDock))<0.01 &&
        fabs(capsule.layer.cornerRadius-30)<0.01 && fabs(extraMaterial.layer.cornerRadius-31)<0.01,
        @"the custom capsule radius and equally thick circular action are preserved");
    Check(fabs(CGRectGetMinX(extraInDock)-CGRectGetMaxX(capsule.frame)-12)<0.01 &&
        fabs(CGRectGetWidth(CGRectUnion(capsule.frame,extraInDock))/dock.bounds.size.width-0.905)<0.001,
        @"reference-width dock uses symmetric margins and a twelve-point accessory gap");
    BOOL reduceTransparency=UIAccessibilityIsReduceTransparencyEnabled();
    BOOL increasedContrast=dock.traitCollection.accessibilityContrast==UIAccessibilityContrastHigh;
    if(!reduceTransparency) {
        Layout(capsuleMaterial);
        UIColor *regular=capsuleMaterial.contentView.backgroundColor;
        HGAppearanceSet(@"dock.material",@"clear",nil);
        Layout(capsuleMaterial);
        UIColor *clear=capsuleMaterial.contentView.backgroundColor;
        Check(CGColorGetAlpha(regular.CGColor)==0 && CGColorGetAlpha(clear.CGColor)==0 &&
            (!nativeGlass || HGADockFullGlass(capsuleMaterial)),
            @"regular and clear navigation use unmasked full-surface effects without a painted backing");
        HGAppearanceSet(@"dock.material",@"regular",nil);
        Layout(capsuleMaterial);
    }
    CGFloat selectionAlpha=reduceTransparency || increasedContrast ? 0.18 : 0.09;
    Check(selection && !selection.hidden && !selection.userInteractionEnabled &&
        (nativeGlass ? [selection.backgroundColor isEqual:UIColor.secondarySystemFillColor] && HGADockFullGlass(capsuleMaterial) :
         !capsuleMaterial.maskView && [selection.backgroundColor isEqual:[UIColor colorWithWhite:1 alpha:selectionAlpha]]),
        @"resting selection uses a recessed fill over a single glass rail and preserves the accessible fallback");
    CGFloat restingInset=capsule.bounds.size.height*4.0/62.0;
    Check(fabs(selection.bounds.size.height-(capsule.bounds.size.height-2*restingInset))<0.01 &&
        fabs(CGRectGetMinX(selection.frame)-restingInset)<0.01 && home.bounds.size.width>=44,
        @"the first resting selection has the reference platter inset and keeps the full tab hit zone");
    Check(capsule.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark &&
        extraSurface.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark,
        @"playback switches both glass surfaces in place when the app changes to dark");
    Check(!home.configuration,
        @"dock owns selection appearance instead of inheriting UIKit accent selection");
    HGADockPixelCounts normal=HGADockPixels(home),accessory=HGADockPixels(extra);
    Check(normal.brightNeutral>8 && accessory.brightNeutral>8,
        @"tab and independent profile glyphs render white on a dark surface");
    Check(normal.blue==0 && accessory.blue==0,
        @"rendered selected tab and independent button contain no inherited blue accent");
    CGFloat activeOpacity=CGColorGetAlpha([home.tintColor resolvedColorWithTraitCollection:home.traitCollection].CGColor);
    CGFloat inactiveOpacity=CGColorGetAlpha([theater.tintColor resolvedColorWithTraitCollection:theater.traitCollection].CGColor);
    Check(inactiveOpacity>0.50 && inactiveOpacity<activeOpacity && activeOpacity>0.90,
        @"unselected navigation glyphs stay neutral and quieter than the selected tab");
    Check(MAX(home.imageView.bounds.size.width,home.imageView.bounds.size.height)<36 &&
        MAX(extra.imageView.bounds.size.width,extra.imageView.bounds.size.height)<36,
        @"R15 default icons fit the thicker dock and equally sized independent action");
    home.highlighted=YES;[home setNeedsUpdateConfiguration];Layout(home);
    HGADockPixelCounts pressed=HGADockPixels(home);
    Check(pressed.blue==0 && (nativeGlass ? [selection.backgroundColor isEqual:UIColor.secondarySystemFillColor] : HGADockNeutralTranslucent(selection.backgroundColor,selection.traitCollection)),
        @"pressed selection stays neutral after a UIKit configuration update");
    home.highlighted=NO;
    Check(HGASaveDockPreview(host,dock),@"controlled UIKit dock preview renders to PNG for visual review");
    host.selectedIndex=1;HGAppearanceLayoutDock(host.customTabBar,host);Layout(dock);
    Layout(theater);
    Check(!home.selected && theater.selected &&
        fabs(CGRectGetMidX(selection.frame)-[theater convertPoint:theater.imageView.center toView:capsule].x)<0.01 &&
        (nativeGlass ? HGADockFullGlass(capsuleMaterial) : !capsuleMaterial.maskView),
        @"selection follows the host's selected tab over one continuous material");
    HGADockPixelCounts switched=HGADockPixels(theater);
    Check(switched.blue==0 && switched.brightNeutral>8,
        @"newly selected theater renders white after the selected state changes");
    // Model the host switching from dark playback into a light catalogue page
    // even while the app's trait differs from its per-tab appearance.
    host.overrideUserInterfaceStyle=UIUserInterfaceStyleDark;
    theme.selectedImageColor=UIColor.blackColor;
    HGAppearanceLayoutDock(host.customTabBar,host);Layout(capsuleMaterial);Layout(extraMaterial);
    UIColor *lightFill=[UIColor colorWithWhite:1 alpha:reduceTransparency?1:0];
    Class expectedEffect=HGNativeGlassConfigured() ? NSClassFromString(@"UIGlassEffect") : UIBlurEffect.class;
    Check(capsule.traitCollection.userInterfaceStyle==UIUserInterfaceStyleLight && (nativeGlass ? HGADockFullGlass(capsuleMaterial) : !capsuleMaterial.maskView) &&
        extra.traitCollection.userInterfaceStyle==UIUserInterfaceStyleLight &&
        [capsuleMaterial.contentView.backgroundColor isEqual:nativeGlass ? UIColor.clearColor : lightFill] &&
        [extraMaterial.contentView.backgroundColor isEqual:nativeGlass ? UIColor.clearColor : lightFill] &&
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
        @"the custom-radius compatibility surface supplies Clear glass on light pages");
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
        [dock hitTest:extraPoint withEvent:nil]==extra && [dock hitTest:paddingPoint withEvent:nil]==home &&
        ![dock hitTest:clearCorner withEvent:nil],
        @"button slots and rounded capsule padding acquire touches, while transparent corners pass through");
    [home sendActionsForControlEvents:UIControlEventTouchUpInside];
    [theater sendActionsForControlEvents:UIControlEventTouchUpInside];
    [extra sendActionsForControlEvents:UIControlEventTouchUpInside];
    Check(host.nativeTaps==taps+3 && host.selectedIndex==3,@"dock and accessory each forward one native navigation action");
    HGAppearanceSet(@"dock.action",@"home",nil);
    HGAppearanceLayoutDock(host.customTabBar,host);
    [extra sendActionsForControlEvents:UIControlEventTouchUpInside];
    [extra sendActionsForControlEvents:UIControlEventTouchUpInside];
    Check(host.nativeTaps==taps+5 && host.selectedIndex==0 && dock.superview==host.customTabBar &&
        host.customTabBar.contentView.hidden,@"selecting detached Home and tapping it again retains one Dock and forwards each native action once");
    // FanqieX-style auto-detach: when all 4 tabs are unhidden and dock.action is a tab,
    // that tab moves out of the main capsule into the separate circular glass button.
    HGAppearanceApply(@{@"dock.hide.home":@NO,@"dock.hide.theater":@NO,@"dock.hide.store":@NO,@"dock.hide.mine":@NO,
        @"dock.extra":@YES,@"dock.action":@"mine"},nil);
    host.selectedIndex=3;HGAppearanceLayoutDock(host.customTabBar,host);Layout(dock);
    UIView *mineInCapsule=Find(dock,@"honggo.appearance.tab.mine");
    UIView *extraSel=Find(dock,@"honggo.appearance.dock.extra.selection");
    Check(mineInCapsule.hidden && !extra.hidden && extra.selected &&
        (extra.accessibilityTraits & UIAccessibilityTraitSelected)!=0 &&
        extraSel && !extraSel.hidden && CGRectContainsRect(extra.bounds,extraSel.frame) &&
        CGRectGetHeight(extraSel.frame)<CGRectGetHeight(extra.bounds) && selection.hidden,
        @"detaching a tab into the circular dock button removes it from the capsule and highlights selection on the circle");
    HGAppearanceApply(@{@"dock.hide.store":@YES,@"dock.hide.mine":@YES},nil);
    host.selectedIndex=0;HGAppearanceLayoutDock(host.customTabBar,host);Layout(dock);
    HGRunDockDragChecks(host,(id)dock,capsule,selection,home,theater);
    HGAppearanceSet(@"dock.style",@"flat",nil);
    host.selectedIndex=0;
    HGAppearanceLayoutDock(host.customTabBar,host);
    // The preceding cancelled glass drag is still settling. The live Dock
    // applies the new style in its spring completion; then acquire its track.
    HGWaitForDockSelectionLensToSettle(selection);
    UISegmentedControl *track=(id)Find(capsule,@"honggo.appearance.dock.flat");
    UISegmentedControl *extraTrack=(id)Find(extra.superview,@"honggo.appearance.dock.flat");
    Check([track isKindOfClass:UISegmentedControl.class] && track.numberOfSegments==2 &&
        track.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark && track.selectedSegmentIndex==0 &&
        CGRectEqualToRect(track.frame,capsule.bounds) && !Find(capsule,@"honggo.appearance.material") &&
        !HGDockVisibleSelectionLens(selection).effect && selection.hidden && extraTrack.numberOfSegments==1,
        @"flat tabs and the independent action use the header's dark UIKit segment track without glass");
    NSUInteger flatTaps=host.nativeTaps;
    HGAAppearanceDock *flatDock=(id)dock;
    [flatDock beginDockTouchAtPoint:homePoint];
    CGPoint between=CGPointMake((homePoint.x+theaterPoint.x)/2,homePoint.y);
    [flatDock moveDockTouchToPoint:between];
    HGAppearanceLayoutDock(host.customTabBar,host);
    CGFloat relativeX=CGRectGetMidX(selection.frame);
    Check(track.selectedSegmentIndex==UISegmentedControlNoSegment && host.nativeTaps==flatTaps &&
        !selection.hidden && relativeX>CGRectGetMidX(home.frame) && relativeX<CGRectGetMidX(theater.frame) &&
        (UIAccessibilityIsReduceMotionEnabled() || CGRectGetHeight(selection.bounds)>CGRectGetHeight(capsule.bounds)),
        @"flat Dock lifts its selection and tracks the finger between the original two hit targets");
    if(HGNativeGlassConfigured() && !UIAccessibilityIsReduceTransparencyEnabled() && !UIAccessibilityIsReduceMotionEnabled())
        Check([HGDockVisibleSelectionLens(selection).effect isKindOfClass:NSClassFromString(@"UIGlassEffect")],
            @"an active optical surface travels beneath the flat Dock symbols");
    [flatDock moveDockTouchToPoint:theaterPoint];
    UIVisualEffectView *trackingLens=HGDockVisibleSelectionLens(selection);
    [flatDock endDockTouchAtPoint:theaterPoint cancelled:YES];
    if(trackingLens)Check(HGDockVisibleSelectionLens(selection)==trackingLens &&
        track.selectedSegmentIndex==UISegmentedControlNoSegment,
        @"flat Dock keeps a single live lens until it reaches the resting thumb");
    HGWaitForDockSelectionLensToSettle(selection);
    Check(track.selectedSegmentIndex==0 && host.nativeTaps==flatTaps,
        @"cancelled flat scrubbing restores the host selection");
    [theater sendActionsForControlEvents:UIControlEventTouchUpInside];
    HGAppearanceLayoutDock(host.customTabBar,host);
    Check(track.selectedSegmentIndex==1 && host.nativeTaps==flatTaps+1 && host.selectedIndex==1,
        @"the flat last segment activates its host tab exactly once");
    if(@available(iOS 17.0,*))[track updateTraitsIfNeeded];
    Check(track.traitCollection.userInterfaceStyle==UIUserInterfaceStyleLight && HGADockGlyphMatches(theater,NO),
        @"flat navigation on a light host page keeps the selected glyph dark");
    [flatDock beginDockTouchAtPoint:theaterPoint];
    [flatDock moveDockTouchToPoint:homePoint];
    Check(HGAppearanceSuspendDockForSidebar(host.customTabBar) &&
        host.customTabBar.contentView.hidden && [host.customTabBar.backgroundColor isEqual:UIColor.clearColor] &&
        flatDock.superview==host.customTabBar && !flatDock.userInteractionEnabled &&
        track.selectedSegmentIndex==1 && selection.hidden,
        @"opening the drawer cancels a live scrub while keeping the flat Dock and its clear host background");
    [flatDock endDockTouchAtPoint:homePoint cancelled:NO];
    [home sendActionsForControlEvents:UIControlEventTouchUpInside];
    CGRect beforeDrawer=host.customTabBar.frame;
    host.customTabBar.frame=CGRectOffset(beforeDrawer,240,0);
    HGAppearanceLayoutDock(host.customTabBar,host);
    Check(!flatDock.userInteractionEnabled && flatDock.superview==host.customTabBar &&
        host.customTabBar.contentView.hidden && host.nativeTaps==flatTaps+1,
        @"a displaced drawer leaves native labels hidden and discards a pending release");
    host.customTabBar.frame=beforeDrawer;
    HGAppearanceLayoutDock(host.customTabBar,host);
    Check(flatDock.userInteractionEnabled && track.selectedSegmentIndex==1 &&
        [dock hitTest:homePoint withEvent:nil]==home && host.customTabBar.contentView.hidden,
        @"a returned or cancelled drawer resumes hit testing on the same flat Dock");
    HGAppearanceSet(@"dock.style",@"glass",nil);
    HGAppearanceLayoutDock(host.customTabBar,host);
    Check(!Find(capsule,@"honggo.appearance.dock.flat") && Find(capsule,@"honggo.appearance.material"),
        @"switching back removes the flat track and restores the configured glass");
    host.overrideUserInterfaceStyle=UIUserInterfaceStyleLight;
    if(@available(iOS 17.0,*)) {
        [host updateTraitsIfNeeded];
        [host.view updateTraitsIfNeeded];
        [host.customTabBar updateTraitsIfNeeded];
    }
    NSUInteger preClearIndex=host.selectedIndex;
    HGAppearanceApply(@{@"dock.style":@"glass",@"dock.material":@"clear",@"dock.selectionStyle":@"glass",
        @"dock.radius":@29,@"dock.height":@58,@"dock.extra":@YES},nil);
    host.selectedIndex=0;
    HGAppearanceLayoutDock(host.customTabBar,host);Layout(dock);
    UIVisualEffectView *hostVeil=[[UIVisualEffectView alloc] initWithEffect:[UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemMaterial]];
    hostVeil.frame=host.customTabBar.bounds;
    [host.customTabBar insertSubview:hostVeil atIndex:0];
    HGAppearanceReclaimDockChrome(host.customTabBar);
    Check(hostVeil.hidden,
        @"custom Dock hides host UIVisualEffectView backdrops inside SSTabBar so channel swipes never flash a veil");
    [hostVeil removeFromSuperview];
    UIView *clearGlassBefore=Find(capsule,@"honggo.appearance.material");
    UIVisualEffectView *clearLensBefore=(id)Find(capsule,@"honggo.appearance.dock.drag-lens");
    if(clearGlassBefore && clearLensBefore && !UIAccessibilityIsReduceMotionEnabled()) {
        CGPoint homePt=[home convertPoint:CGPointMake(CGRectGetMidX(home.bounds),CGRectGetMidY(home.bounds)) toView:dock];
        CGPoint theaterPt=[theater convertPoint:CGPointMake(CGRectGetMidX(theater.bounds),CGRectGetMidY(theater.bounds)) toView:dock];
        [flatDock beginDockTouchAtPoint:homePt];
        BOOL lensClearedBacking=[selection.backgroundColor isEqual:UIColor.clearColor] && !clearLensBefore.hidden;
        [flatDock moveDockTouchToPoint:theaterPt];
        [flatDock endDockTouchAtPoint:theaterPt cancelled:YES];
        HGWaitForDockSelectionLensToSettle(selection);
        Check(lensClearedBacking &&
            Find(capsule,@"honggo.appearance.material")==clearGlassBefore &&
            Find(capsule,@"honggo.appearance.dock.drag-lens")==clearLensBefore &&
            [selection.backgroundColor isEqual:UIColor.secondarySystemFillColor] &&
            clearLensBefore.hidden && CGAffineTransformIsIdentity(clearLensBefore.transform),
            @"clear Dock reuses its glass and drag lens without recreation, clears muddy backing while tracking, and restores resting fill on settle");
    }
    UIButton *accBtn=(id)Find(host.customTabBar,@"honggo.appearance.dock.extra");
    UIView *accSurface=[flatDock valueForKey:@"extra"];
    if(accBtn && accSurface && !UIAccessibilityIsReduceMotionEnabled()) {
        [accBtn sendActionsForControlEvents:UIControlEventTouchDown];
        BOOL scaledSurface=accSurface.transform.a>1.01 && CGAffineTransformIsIdentity(accBtn.imageView.transform);
        [accBtn sendActionsForControlEvents:UIControlEventTouchCancel];
        Check(scaledSurface && CGAffineTransformIsIdentity(accSurface.transform),
            @"accessory press springs the glass container rather than shrinking the glyph");
    }
    host.selectedIndex=preClearIndex;
    HGAppearanceApply(@{@"dock.extra":@NO,@"dock.material":@"solid"},nil);HGAppearanceLayoutDock(host.customTabBar,host);Layout(dock);
    UIVisualEffectView *solidMaterial=(id)Find(capsule,@"honggo.appearance.material");
    Layout(solidMaterial);
    UIColor *solidFill=solidMaterial.contentView.backgroundColor;
    Check(capsule.traitCollection.userInterfaceStyle==UIUserInterfaceStyleLight && solidMaterial.superview==capsule &&
        !solidMaterial.maskView && !solidMaterial.effect && solidFill && CGColorGetAlpha(solidFill.CGColor)==1,
        @"solid dock respects the host light appearance with continuous solid fill");
    objc_setAssociatedObject(host.customTabBar,&HGADockThemeFixtureKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    host.selectedIndex=savedIndex;host.overrideUserInterfaceStyle=savedStyle;host.view.tintColor=savedTint;
    host.customTabBar.frame=savedBarFrame;
    HGAppearanceApply(saved,nil);HGAppearanceLayoutDock(host.customTabBar,host);
    HGRunDockCaptionChecks(host);
    HGRunDockSystemAdapterChecks(host);
    HGRunDockFeedMotionChecks(host);
}

@interface HGAFeedPanFixture : UIPanGestureRecognizer
@property(nonatomic,weak) UIView *fixtureView;
@property(nonatomic) CGPoint fixtureTranslation;
@property(nonatomic) CGPoint fixtureVelocity;
@property(nonatomic) UIGestureRecognizerState fixtureState;
@end
@implementation HGAFeedPanFixture
- (UIView *)view { return self.fixtureView ?: [super view]; }
- (CGPoint)translationInView:(__unused UIView *)view { return self.fixtureTranslation; }
- (CGPoint)velocityInView:(__unused UIView *)view { return self.fixtureVelocity; }
- (UIGestureRecognizerState)state { return self.fixtureState; }
@end

@interface HGAFeedDisplayLinkStandIn : NSObject
@property(nonatomic) CFTimeInterval timestamp;
@property(nonatomic) CFTimeInterval targetTimestamp;
@property(nonatomic) BOOL paused;
@end
@implementation HGAFeedDisplayLinkStandIn
@end

static void HGStepDockFeedSpring(HGAAppearanceDock *dock, double seconds, double *clock) {
    int steps = (int)ceil(seconds * 120.0);
    HGAFeedDisplayLinkStandIn *tick = [HGAFeedDisplayLinkStandIn new];
    for (int i = 0; i < steps; i++) {
        tick.timestamp = *clock;
        *clock += 1.0 / 120.0;
        tick.targetTimestamp = *clock;
        [dock advanceFeedSpring:(CADisplayLink *)tick];
    }
}

static void HGRunDockFeedMotionChecks(SSTabBarController *host) {
    if (UIAccessibilityIsReduceMotionEnabled() || UIAccessibilityIsVoiceOverRunning()) return;
    NSDictionary *saved = HGAppearanceValues();
    NSInteger savedTab = host.selectedIndex;
    host.selectedIndex = 0;
    HGAppearanceApply(@{
        @"enabled": @YES, @"dock.enabled": @YES, @"dock.style": @"glass", @"dock.material": @"clear",
        @"dock.selectionStyle": @"glass", @"dock.content": @"both", @"dock.radius": @20,
        @"dock.height": @58, @"dock.extra": @YES, @"dock.extraSize": @54, @"dock.action": @"mine",
        @"dock.hide.home": @NO, @"dock.hide.theater": @NO, @"dock.hide.store": @NO, @"dock.hide.mine": @NO,
        @"dock.collapseMode": @"tiktok", @"dock.siriaiHideLine": @NO
    }, nil);
    SSTabBar *bar = host.customTabBar;
    HGAppearanceLayoutDock(bar, host);
    Layout(bar);
    HGAAppearanceDock *dock = (id)Find(bar, @"honggo.appearance.dock");
    UIView *capsule = Find(dock, @"honggo.appearance.dock.capsule");
    UIButton *home = (id)Find(dock, @"honggo.appearance.tab.home");
    UIButton *theater = (id)Find(dock, @"honggo.appearance.tab.theater");
    UIButton *extraBtn = (id)Find(dock, @"honggo.appearance.dock.extra");
    UIView *extraView = extraBtn.superview;
    while (extraView && extraView.superview != dock) extraView = extraView.superview;
    Layout(dock); Layout(home); Layout(theater); Layout(extraView);

    UICollectionViewFlowLayout *flow = [UICollectionViewFlowLayout new];
    FQVCollectionView *feed = [[FQVCollectionView alloc] initWithFrame:host.view.bounds collectionViewLayout:flow];
    [host.view insertSubview:feed belowSubview:bar];
    HGAppearanceProcessView(feed);

    UIView *materialBefore = Find(capsule, @"honggo.appearance.material");
    CGRect expCap = [capsule convertRect:capsule.bounds toView:dock];
    CGRect expExtra = [extraView convertRect:extraView.bounds toView:dock];
    CGRect expGroup = CGRectUnion(expCap, expExtra);

    id observer = [NSClassFromString(@"HGADockScrollObserver") shared];
    HGAFeedPanFixture *pan = [HGAFeedPanFixture new];
    pan.fixtureView = feed;

    // Horizontal swipe must be ignored by vertical lock (1.2x ratio).
    pan.fixtureTranslation = CGPointZero;
    pan.fixtureState = UIGestureRecognizerStateBegan;
    [observer pan:pan];
    pan.fixtureTranslation = CGPointMake(-42, -12);
    pan.fixtureState = UIGestureRecognizerStateChanged;
    [observer pan:pan];
    pan.fixtureState = UIGestureRecognizerStateEnded;
    [observer pan:pan];
    Check(![dock isFeedCompact] && CGRectEqualToRect([capsule convertRect:capsule.bounds toView:dock], expCap),
        @"horizontal category swipe does not trigger TikTok Dock collapse");

    // Slight gentle upward lift (< 18pt) follows slightly then springs gracefully back to expanded.
    double clock = CACurrentMediaTime();
    pan.fixtureTranslation = CGPointZero;
    pan.fixtureState = UIGestureRecognizerStateBegan;
    [observer pan:pan];
    for (NSNumber *ty in @[@-6, @-10, @-12]) {
        pan.fixtureTranslation = CGPointMake(0, ty.doubleValue);
        pan.fixtureState = UIGestureRecognizerStateChanged;
        [observer pan:pan];
        HGStepDockFeedSpring(dock, 1.0 / 60.0, &clock);
    }
    pan.fixtureVelocity = CGPointMake(0, 0);
    pan.fixtureState = UIGestureRecognizerStateEnded;
    [observer pan:pan];
    HGStepDockFeedSpring(dock, 0.50, &clock);
    Check(![dock isFeedCompact] &&
        fabs(CGRectGetWidth([capsule convertRect:capsule.bounds toView:dock]) - CGRectGetWidth(expCap)) < 0.5,
        @"slight gentle upward lift springs back smoothly to expanded TikTok Dock instead of collapsing");

    // Upward vertical drag shrinks group to 81% x 80%, icons to 91% uniform, fades captions, keeps material & clickability.
    pan.fixtureTranslation = CGPointZero;
    pan.fixtureState = UIGestureRecognizerStateBegan;
    [observer pan:pan];
    for (NSNumber *ty in @[@-6, @-60, @-130, @-200]) {
        pan.fixtureTranslation = CGPointMake(0, ty.doubleValue);
        pan.fixtureState = UIGestureRecognizerStateChanged;
        [observer pan:pan];
        HGStepDockFeedSpring(dock, 1.0 / 60.0, &clock);
    }
    pan.fixtureVelocity = CGPointMake(0, -420);
    pan.fixtureState = UIGestureRecognizerStateEnded;
    [observer pan:pan];
    HGStepDockFeedSpring(dock, 0.55, &clock);

    CGRect compCap = [capsule convertRect:capsule.bounds toView:dock];
    CGRect compExtra = [extraView convertRect:extraView.bounds toView:dock];
    CGRect compGroup = CGRectUnion(compCap, compExtra);
    CGFloat capScaleX = CGRectGetWidth(compCap) / CGRectGetWidth(expCap);
    CGFloat capScaleY = CGRectGetHeight(compCap) / CGRectGetHeight(expCap);
    CGFloat worldGlyphX = home.imageView.transform.a * capScaleX;
    CGFloat worldGlyphY = home.imageView.transform.d * capScaleY;
    Check([dock isFeedCompact] &&
        fabs(CGRectGetWidth(compGroup) / CGRectGetWidth(expGroup) - 0.81) < 0.015 &&
        fabs(capScaleY - 0.80) < 0.01 &&
        fabs(CGRectGetHeight(compExtra) / CGRectGetHeight(expExtra) - 0.80) < 0.01 &&
        CGRectGetMidX(compExtra) < CGRectGetMidX(expExtra) &&
        fabs(worldGlyphX - 0.91) < 0.015 && fabs(worldGlyphY - 0.91) < 0.015 &&
        home.titleLabel.alpha == 0.0 &&
        Find(capsule, @"honggo.appearance.material") == materialBefore,
        @"TikTok feed motion shrinks Dock group to 81%x80%, scales icons uniformly to 91%, fades captions, shifts detached circle and reuses glass material");

    CGPoint theaterCompactPt = [theater convertPoint:CGPointMake(CGRectGetMidX(theater.bounds), CGRectGetMidY(theater.bounds)) toView:dock];
    CGPoint extraCompactPt = [extraBtn convertPoint:CGPointMake(CGRectGetMidX(extraBtn.bounds), CGRectGetMidY(extraBtn.bounds)) toView:dock];
    CGPoint outsideCompactPt = CGPointMake(CGRectGetMinX(expCap) + 2.0, CGRectGetMidY(expCap));
    Check([dock hitTest:theaterCompactPt withEvent:nil] == theater &&
        [dock hitTest:extraCompactPt withEvent:nil] == extraBtn &&
        [dock hitTest:outsideCompactPt withEvent:nil] == nil,
        @"TikTok compact Dock keeps capsule tabs and detached circle clickable while passing touches outside shrunk bounds through to video");

    // Siri AI mode: continuous gather -> thin white bar -> touch pass-through -> optional fade -> pull-down recovery.
    HGAppearanceApply(@{@"dock.collapseMode": @"siriai", @"dock.siriaiHideLine": @NO}, nil);
    HGAppearanceLayoutDock(bar, host);
    pan.fixtureTranslation = CGPointZero;
    pan.fixtureState = UIGestureRecognizerStateBegan;
    [observer pan:pan];
    for (NSNumber *ty in @[@-6, @-45, @-95, @-130]) {
        pan.fixtureTranslation = CGPointMake(0, ty.doubleValue);
        pan.fixtureState = UIGestureRecognizerStateChanged;
        [observer pan:pan];
        HGStepDockFeedSpring(dock, 1.0 / 60.0, &clock);
    }
    pan.fixtureVelocity = CGPointMake(0, -360);
    pan.fixtureState = UIGestureRecognizerStateEnded;
    [observer pan:pan];
    HGStepDockFeedSpring(dock, 0.75, &clock);

    CALayer *whiteLine = [dock siriaiMinimumFill];
    Check([dock isFeedCompact] && [dock feedControlsHidden] &&
        whiteLine != nil && whiteLine.opacity > 0.99f &&
        fabs(CGRectGetWidth(whiteLine.bounds) - CGRectGetWidth(expGroup) * DKSiriaiLineWidthRatio) < 0.5 &&
        CGRectGetHeight(whiteLine.bounds) >= 2.0 && CGRectGetHeight(whiteLine.bounds) <= 4.0 &&
        extraView.alpha == 0.0 &&
        [dock hitTest:whiteLine.position withEvent:nil] == nil &&
        [dock hitTest:CGPointMake(CGRectGetMidX(expCap), CGRectGetMidY(expCap)) withEvent:nil] == nil,
        @"Siri AI collapse gathers capsule and detached circle into a 14.1% white thin line and passes all touches through");

    // Enable Siri AI line hide and trigger fade, then pull down to recover from invisible state.
    HGAppearanceSet(@"dock.siriaiHideLine", @YES, nil);
    HGAppearanceLayoutDock(bar, host);
    DKFeedSpringState s = [dock feedSpring];
    if (DKFeedSpringTriggerSiriaiFade(&s, s.siriaiFadeGeneration)) {
        [dock setFeedSpring:s];
        HGStepDockFeedSpring(dock, 0.30, &clock);
    }
    Check(whiteLine.opacity < 0.01f && [dock isFeedCompact],
        @"Siri AI optional hide-line fades the white thin bar out completely after settling");

    pan.fixtureTranslation = CGPointZero;
    pan.fixtureState = UIGestureRecognizerStateBegan;
    [observer pan:pan];
    for (NSNumber *ty in @[@6, @32, @80, @120]) {
        pan.fixtureTranslation = CGPointMake(0, ty.doubleValue);
        pan.fixtureState = UIGestureRecognizerStateChanged;
        [observer pan:pan];
        HGStepDockFeedSpring(dock, 1.0 / 60.0, &clock);
    }
    pan.fixtureVelocity = CGPointMake(0, 380);
    pan.fixtureState = UIGestureRecognizerStateEnded;
    [observer pan:pan];
    HGStepDockFeedSpring(dock, 0.75, &clock);

    CGRect recoveredCap = [capsule convertRect:capsule.bounds toView:dock];
    Check(![dock isFeedCompact] && ![dock feedControlsHidden] &&
        fabs(CGRectGetWidth(recoveredCap) - CGRectGetWidth(expCap)) < 0.5 &&
        fabs(CGRectGetHeight(recoveredCap) - CGRectGetHeight(expCap)) < 0.5 &&
        extraView.alpha == 1.0 && whiteLine.opacity == 0.0f &&
        Find(capsule, @"honggo.appearance.material") == materialBefore,
        @"pulling down after Siri AI line fade recovers the full glass capsule and detached circle without recreating materials");

    // Page switch resets feed compact state automatically.
    [dock setFeedCompact:YES animated:NO];
    Check([dock isFeedCompact], @"programmatic Siri AI compact state engages cleanly");
    host.selectedIndex = 1;
    HGAppearanceLayoutDock(bar, host);
    Check(![dock isFeedCompact] && ![dock feedControlsHidden],
        @"switching root tabs resets Dock feed collapse back to expanded state");

    [feed removeFromSuperview];
    host.selectedIndex = savedTab;
    HGAppearanceApply(saved, nil);
    HGAppearanceLayoutDock(bar, host);
}
