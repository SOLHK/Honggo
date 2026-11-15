// Included after the controlled host, Check, Find and Layout helpers. These
// checks render real UIKit button layers and exercise their state changes.
#import <QuartzCore/QuartzCore.h>

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
        [@"Controlled UIKit fixture · R9 dock" drawAtPoint:CGPointMake(14,crop.size.height+23)
            withAttributes:@{NSFontAttributeName:[UIFont systemFontOfSize:11],
                NSForegroundColorAttributeName:[UIColor colorWithWhite:1 alpha:0.72]}];
    }];
    NSURL *directory=[NSFileManager.defaultManager URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask].firstObject;
    [NSFileManager.defaultManager createDirectoryAtURL:directory withIntermediateDirectories:YES attributes:nil error:nil];
    return [UIImagePNGRepresentation(preview) writeToURL:[directory URLByAppendingPathComponent:@"dock-r9-preview.png"] atomically:YES];
}
static void HGRunDockAppearanceChecks(SSTabBarController *host) {
    NSDictionary *saved=HGAppearanceValues();
    NSUInteger savedIndex=host.selectedIndex;
    UIUserInterfaceStyle savedStyle=host.overrideUserInterfaceStyle;
    UIColor *savedTint=host.view.tintColor;
    host.overrideUserInterfaceStyle=UIUserInterfaceStyleLight;
    host.view.tintColor=UIColor.systemBlueColor;
    host.selectedIndex=0;
    HGAppearanceApply(@{@"enabled":@YES,@"dock.enabled":@YES,@"dock.material":@"regular",
        @"dock.content":@"icon",@"dock.height":@52,@"dock.width":@0.94,@"dock.radius":@26,
        @"dock.iconSize":@30,@"dock.extra":@YES,@"dock.extraSize":@48,@"dock.action":@"mine",
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
    Check(selection && !selection.hidden && !selection.userInteractionEnabled &&
        HGADockNeutralTranslucent(selection.backgroundColor,selection.traitCollection),
        @"selected dock pill is neutral translucent gray and cannot intercept navigation");
    Check(CGRectGetMinY(selection.frame)>=3 && CGRectGetMaxY(selection.frame)<=capsule.bounds.size.height-3 &&
        selection.bounds.size.width<=capsule.bounds.size.height*1.4 && home.bounds.size.width>=44,
        @"two-tab dock keeps a compact inset selection pill and full accessible touch targets");
    Check(capsule.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark &&
        host.overrideUserInterfaceStyle==UIUserInterfaceStyleLight,
        @"video dock keeps dark glass without changing the host light theme");
    Check(!home.automaticallyUpdatesConfiguration && !home.configuration.automaticallyUpdateForSelection,
        @"dock owns selection appearance instead of inheriting UIKit accent selection");
    HGADockPixelCounts normal=HGADockPixels(home),accessory=HGADockPixels(extra);
    Check(normal.brightNeutral>8 && accessory.brightNeutral>8,
        @"tab and independent profile glyphs render white on a dark surface");
    Check(normal.blue==0 && accessory.blue==0,
        @"rendered selected tab and independent button contain no inherited blue accent");
    Check(MAX(home.imageView.bounds.size.width,home.imageView.bounds.size.height)<36 &&
        MAX(extra.imageView.bounds.size.width,extra.imageView.bounds.size.height)<36,
        @"maximum icon preference still fits the compact dock and accessory proportions");
    home.highlighted=YES;[home setNeedsUpdateConfiguration];Layout(home);
    HGADockPixelCounts pressed=HGADockPixels(home);
    Check(pressed.blue==0 && HGADockNeutralTranslucent(selection.backgroundColor,selection.traitCollection),
        @"pressed selection stays neutral after a UIKit configuration update");
    home.highlighted=NO;
    Check(HGASaveDockPreview(host,dock),@"controlled UIKit dock preview renders to PNG for visual review");
    host.selectedIndex=1;HGAppearanceLayoutDock(host.customTabBar,host);Layout(dock);
    Check(!home.selected && theater.selected &&
        fabs(CGRectGetMidX(selection.frame)-CGRectGetMidX(theater.frame))<0.01,
        @"gray selection follows the host's current tab without covering other items");
    HGADockPixelCounts switched=HGADockPixels(theater);
    Check(switched.blue==0 && switched.brightNeutral>8,
        @"newly selected theater renders white after the selected state changes");
    HGAppearanceSet(@"dock.selected",@"#26B37E",nil);Layout(theater);
    CGFloat r=0,g=0,b=0,a=0;
    [theater.configuration.baseForegroundColor getRed:&r green:&g blue:&b alpha:&a];
    Check(fabs(r-38/255.0)<0.001 && fabs(g-179/255.0)<0.001 &&
        HGADockNeutralTranslucent(selection.backgroundColor,selection.traitCollection),
        @"explicit user glyph colors are preserved while selection background remains neutral");
    HGAppearanceApply(@{@"dock.material":@"solid",@"dock.selected":@""},nil);Layout(dock);
    Check(capsule.traitCollection.userInterfaceStyle==UIUserInterfaceStyleLight,
        @"solid dock continues to respect the host light appearance");
    host.selectedIndex=savedIndex;host.overrideUserInterfaceStyle=savedStyle;host.view.tintColor=savedTint;
    HGAppearanceApply(saved,nil);HGAppearanceLayoutDock(host.customTabBar,host);
}
