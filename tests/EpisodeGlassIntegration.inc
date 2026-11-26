// The R10 scene contains this exact three-level episode-entry hierarchy. Keep
// rectangular host fills in the fixture so a rounded material alone cannot pass.
@interface FQVShortVideoBaseInsertView : UIView @end
@implementation FQVShortVideoBaseInsertView @end
@interface SSShortVideoDistributionContentView : UIView @end
@implementation SSShortVideoDistributionContentView @end
@interface HGAEpisodeTapFixture : NSObject
@property(nonatomic) NSUInteger taps;
- (void)open:(id)sender;
@end
@implementation HGAEpisodeTapFixture
- (void)open:(id)sender {self.taps++;}
@end

static BOOL HGAEpisodeRenderedCornersAreClear(UIView *view) {
    UIGraphicsImageRendererFormat *format=[UIGraphicsImageRendererFormat defaultFormat];
    format.scale=1;format.opaque=NO;format.preferredRange=UIGraphicsImageRendererFormatRangeStandard;
    UIGraphicsImageRenderer *renderer=[[UIGraphicsImageRenderer alloc] initWithSize:view.bounds.size format:format];
    UIImage *image=[renderer imageWithActions:^(UIGraphicsImageRendererContext *context){
        [view.layer renderInContext:context.CGContext];
    }];
    CGImageRef cg=image.CGImage;if(!cg)return NO;
    size_t width=CGImageGetWidth(cg),height=CGImageGetHeight(cg);
    if(width<10 || height<10)return NO;
    NSMutableData *pixels=[NSMutableData dataWithLength:width*height*4];
    CGColorSpaceRef space=CGColorSpaceCreateDeviceRGB();
    CGContextRef context=CGBitmapContextCreate(pixels.mutableBytes,width,height,8,width*4,space,
        kCGImageAlphaPremultipliedLast|kCGBitmapByteOrder32Big);
    CGColorSpaceRelease(space);if(!context)return NO;
    CGContextDrawImage(context,CGRectMake(0,0,width,height),cg);CGContextRelease(context);
    const uint8_t *bytes=pixels.bytes;
    size_t samples[]={width+1,width*2-2,(height-2)*width+1,(height-1)*width-2};
    for(NSUInteger i=0;i<4;i++)if(bytes[samples[i]*4+3]>8)return NO;
    return bytes[((height/2)*width+width/2)*4+3]>245;
}
static void RunEpisodeGlassChecks(UIView *root) {
    HGAppearanceApply(@{@"enabled":@YES,@"player.episodeGlass":@YES,
        @"layout.episodes.hidden":@NO,@"layout.episodes.x":@0,
        @"layout.episodes.y":@0,@"layout.episodes.scale":@1},nil);
    CGFloat screenWidth=root.window.bounds.size.width;
    FQVShortVideoBaseInsertView *wideWrapper=[[FQVShortVideoBaseInsertView alloc] initWithFrame:CGRectMake(0,170,screenWidth,44)];
    wideWrapper.backgroundColor=UIColor.grayColor;
    SSShortVideoBottomDistributionView *wideEntry=[[SSShortVideoBottomDistributionView alloc] initWithFrame:wideWrapper.bounds];
    SSShortVideoDistributionContentView *wideContent=[[SSShortVideoDistributionContentView alloc] initWithFrame:wideEntry.bounds];
    // Device capture: the direct content retains the authored 440pt width.
    wideContent.autoresizingMask=UIViewAutoresizingNone;
    UIButton *trailing=[UIButton buttonWithType:UIButtonTypeCustom];
    trailing.frame=CGRectMake(screenWidth-32,0,32,44);trailing.autoresizingMask=UIViewAutoresizingFlexibleLeftMargin;
    [wideContent addSubview:trailing];
    UIView *wideBacking=[[UIView alloc] initWithFrame:wideContent.bounds];
    wideBacking.userInteractionEnabled=NO;wideBacking.backgroundColor=UIColor.blackColor;
    [wideContent addSubview:wideBacking];wideEntry.subBackgroundView=wideBacking;
    wideEntry.defaultContentView=wideContent;
    CGRect nativeWideFrame=wideEntry.frame;
    [wideEntry addSubview:wideContent];[wideWrapper addSubview:wideEntry];[root addSubview:wideWrapper];
    Layout(wideEntry);
    Check(CGRectGetMinX(wideEntry.frame)>=12 && CGRectGetMaxX(wideEntry.frame)<=screenWidth-12 &&
        CGRectEqualToRect(wideContent.bounds,wideEntry.bounds) && CGAffineTransformIsIdentity(wideEntry.transform) &&
        CGRectGetMaxX(trailing.frame)==CGRectGetWidth(wideEntry.bounds) &&
        [wideBacking.backgroundColor isEqual:UIColor.clearColor],
        @"edge-to-edge episode entry gains side margins through native layout width without scaling text");
    UIView *wideGlass=Find(wideEntry,@"honggo.appearance.material");
    Check(fabs(wideEntry.bounds.size.width-screenWidth*HGADockDefaultWidth)<0.01 && wideEntry.bounds.size.height==44 &&
        wideGlass.bounds.size.height==36 && HGTestMaterialRadius(wideGlass,18),
        @"the watching entry has a compact glass capsule inside its native-height hit target");
    CGRect insetWideFrame=wideEntry.frame;
    for(NSUInteger i=0;i<5;i++)Layout(wideEntry);
    Check(CGRectEqualToRect(wideEntry.frame,insetWideFrame) && [wideWrapper.backgroundColor isEqual:UIColor.clearColor],
        @"episode margin is stable across layouts and leaves no original full-width backing");
    UIView *capsule=Find(root.window,@"honggo.appearance.dock.capsule");
    id savedWidth=HGAppearanceValue(@"dock.width");
    HGAppearanceSet(@"player.episodeDockWidth",@YES,nil);
    for(NSNumber *ratio in @[@0.84,@0.97]) {
        HGAppearanceSet(@"dock.width",ratio,nil);
        Layout(wideEntry);
        CGRect entryRect=[wideEntry convertRect:wideGlass.frame toView:root.window];
        CGRect dockRect=[capsule convertRect:capsule.bounds toView:root.window];
        Check(capsule && fabs(CGRectGetMinX(entryRect)-CGRectGetMinX(dockRect))<0.5 &&
            fabs(CGRectGetMaxX(entryRect)-CGRectGetMaxX(dockRect))<0.5 &&
            CGRectGetWidth(wideContent.bounds)==CGRectGetWidth(wideGlass.bounds) &&
            wideEntry.bounds.size.height==44,@"the episode glass and native content follow both edges of the mounted Dock capsule");
    }
    HGAppearanceApply(@{@"player.episodeDockWidth":@NO,@"dock.width":@0.55},nil);
    Layout(wideEntry);
    Check(CGRectEqualToRect(wideEntry.frame,insetWideFrame),@"a shortened Dock leaves the fixed four-tab reference width centered when sync is off");
    HGAppearanceSet(@"dock.width",savedWidth,nil);
    wideContent.frame=nativeWideFrame;
    [wideEntry renderWithModel:@{@"background":UIColor.darkGrayColor,@"shade":UIColor.brownColor}];
    Check(CGRectEqualToRect(wideContent.bounds,wideEntry.bounds) &&
        [wideContent.backgroundColor isEqual:UIColor.clearColor] && [wideBacking.backgroundColor isEqual:UIColor.clearColor] &&
        [wideEntry hitTest:trailing.center withEvent:nil]==trailing,
        @"new-model native repaint removes the separate full-width backing and keeps the arrow hit target inside the glass");
    // Reproduce cell reuse for a medium-width collection entry (< 0.88 * screenWidth)
    // with a second inner pill layer and clipping rich text + episode suffix label.
    HGAppearanceSet(@"player.episodeDockWidth",@YES,nil);
    UIView *innerPlate=[[UIView alloc] initWithFrame:CGRectMake(0,4,297,36)];
    innerPlate.userInteractionEnabled=NO;
    innerPlate.layer.backgroundColor=[UIColor colorWithWhite:0 alpha:0.45].CGColor;
    [wideContent insertSubview:innerPlate aboveSubview:wideBacking];
    UIImageView *leadIcon=[[UIImageView alloc] initWithFrame:CGRectMake(14,15,14,14)];
    leadIcon.image=[UIImage systemImageNamed:@"square.stack.3d.up.fill"];
    [wideContent addSubview:leadIcon];
    NSMutableParagraphStyle *clipStyle=[NSMutableParagraphStyle new];
    clipStyle.lineBreakMode=NSLineBreakByClipping;
    UILabel *albumTitle=[[UILabel alloc] initWithFrame:CGRectMake(34,12,240,20)];
    albumTitle.numberOfLines=1;
    albumTitle.attributedText=[[NSAttributedString alloc] initWithString:@"合集 · 《盘点搞笑小品系列》"
        attributes:@{NSFontAttributeName:[UIFont systemFontOfSize:14 weight:UIFontWeightMedium],
                     NSParagraphStyleAttributeName:clipStyle}];
    UILabel *albumCount=[[UILabel alloc] initWithFrame:CGRectMake(276,12,80,20)];
    albumCount.numberOfLines=1;
    albumCount.font=[UIFont systemFontOfSize:14 weight:UIFontWeightMedium];
    albumCount.text=@"· 全244集";
    [wideContent addSubview:albumTitle];[wideContent addSubview:albumCount];
    [trailing removeFromSuperview];
    [wideEntry renderWithModel:@{@"entryWidth":@356,@"background":UIColor.darkGrayColor,@"shade":UIColor.brownColor}];
    Layout(wideEntry);
    CGRect reusedGlass=[wideEntry convertRect:wideGlass.frame toView:root.window];
    CGRect currentDock=[capsule convertRect:capsule.bounds toView:root.window];
    CGFloat leftPad=CGRectGetMinX([leadIcon convertRect:leadIcon.bounds toView:wideGlass]);
    CGFloat rightInk=0;
    for(UILabel *lbl in @[albumTitle,albumCount]) {
        if(lbl.hidden || lbl.alpha<0.01)continue;
        CGRect tr=[lbl textRectForBounds:lbl.bounds limitedToNumberOfLines:1];
        if(!CGRectIsEmpty(tr))rightInk=MAX(rightInk,CGRectGetMaxX([lbl convertRect:tr toView:wideGlass]));
    }
    CGFloat rightPad=CGRectGetWidth(wideGlass.bounds)-rightInk;
    Check(fabs(CGRectGetMinX(reusedGlass)-CGRectGetMinX(currentDock))<0.5 &&
        fabs(CGRectGetMaxX(reusedGlass)-CGRectGetMaxX(currentDock))<0.5 &&
        (!innerPlate.layer.backgroundColor || CGColorGetAlpha(innerPlate.layer.backgroundColor)==0) &&
        leftPad>=10 && rightPad>=10 && fabs(rightPad-leftPad)<=2.5,
        @"reused collection entry stays Dock-width without double-capsule paint and keeps symmetric left and right text padding");
    // Simulate host SSShortVideoDistributionContentView relayout colliding leadIcon
    // with the label and slicing the trailing character against the capsule border.
    [albumCount removeFromSuperview];
    leadIcon.frame=CGRectMake(14,15,16,16);
    albumTitle.frame=CGRectMake(20,12,260,20);
    albumTitle.attributedText=[[NSAttributedString alloc] initWithString:@"合集 · 《国产战争电影合集》更新至第88集"
        attributes:@{NSFontAttributeName:[UIFont systemFontOfSize:14 weight:UIFontWeightMedium],
                     NSParagraphStyleAttributeName:clipStyle}];
    Layout(wideContent);
    CGRect iconInGlass=[leadIcon convertRect:leadIcon.bounds toView:wideGlass];
    CGRect titleTextRect=[albumTitle textRectForBounds:albumTitle.bounds limitedToNumberOfLines:1];
    CGRect titleInGlass=[albumTitle convertRect:titleTextRect toView:wideGlass];
    CGFloat symLeft=CGRectGetMinX(iconInGlass);
    CGFloat symRight=CGRectGetWidth(wideGlass.bounds)-CGRectGetMaxX(titleInGlass);
    Check(CGRectGetMinX(titleInGlass)>=CGRectGetMaxX(iconInGlass)+5.0 &&
        albumTitle.attributedText.length<[@"合集 · 《国产战争电影合集》更新至第88集" length] &&
        ![albumTitle.attributedText.string containsString:@"…"] &&
        symLeft>=10 && symRight>=10 && fabs(symRight-symLeft)<=2.0,
        @"content relayout separates overlapping lead icon from text, hides overflowing trailing characters cleanly, and equalizes left/right capsule margins");
    [leadIcon removeFromSuperview];[albumTitle removeFromSuperview];[innerPlate removeFromSuperview];
    [wideContent addSubview:trailing];
    wideEntry.frame=nativeWideFrame;wideContent.frame=nativeWideFrame;
    HGAppearanceSet(@"player.episodeDockWidth",@NO,nil);
    Layout(wideEntry);
    HGAppearanceSet(@"player.episodeGlass",@NO,nil);
    Check(CGRectEqualToRect(wideEntry.frame,nativeWideFrame) && CGRectEqualToRect(wideContent.frame,nativeWideFrame) &&
        [wideBacking.backgroundColor isEqual:UIColor.brownColor] && [wideWrapper.backgroundColor isEqual:UIColor.grayColor],
        @"disabling episode glass restores the authored full-width entry and backing");
    [wideWrapper removeFromSuperview];HGAppearanceSet(@"player.episodeGlass",@YES,nil);
    CGRect frame=CGRectMake(0,0,300,44);
    FQVShortVideoBaseInsertView *wrapper=[[FQVShortVideoBaseInsertView alloc] initWithFrame:CGRectMake(10,220,300,44)];
    wrapper.backgroundColor=UIColor.darkGrayColor;
    SSShortVideoBottomDistributionView *entry=[[SSShortVideoBottomDistributionView alloc] initWithFrame:frame];
    entry.backgroundColor=UIColor.grayColor;entry.layer.cornerRadius=3;
    entry.layer.maskedCorners=kCALayerMinXMinYCorner;
    SSShortVideoDistributionContentView *content=[[SSShortVideoDistributionContentView alloc] initWithFrame:frame];
    content.backgroundColor=UIColor.lightGrayColor;
    UIView *shade=[[UIView alloc] initWithFrame:frame];
    UIColor *nativeShade=[UIColor colorWithWhite:1 alpha:0.15];
    shade.backgroundColor=nativeShade;
    [content addSubview:shade];
    UIControl *button=[[UIControl alloc] initWithFrame:frame];button.backgroundColor=UIColor.systemBlueColor;
    HGAEpisodeTapFixture *action=[HGAEpisodeTapFixture new];
    [button addTarget:action action:@selector(open:) forControlEvents:UIControlEventTouchUpInside];
    [content addSubview:button];entry.defaultContentView=content;
    [entry addSubview:content];[wrapper addSubview:entry];[root addSubview:wrapper];
    Layout(entry);
    @try {
        UIView *material=Find(entry,@"honggo.appearance.material");
        Check(entry.clipsToBounds && wrapper.clipsToBounds && content.clipsToBounds &&
            fabs(entry.layer.cornerRadius-22)<0.01 && fabs(wrapper.layer.cornerRadius-22)<0.01 &&
            HGTestMaterialRadius(material,22),
            @"episode glass clips the native wrapper and content to the same capsule");
        Check([wrapper.backgroundColor isEqual:UIColor.clearColor] &&
            [content.backgroundColor isEqual:UIColor.clearColor] &&
            [shade.backgroundColor isEqual:UIColor.clearColor] && !shade.hidden &&
            [button.backgroundColor isEqual:UIColor.systemBlueColor],
            @"episode glass clears the native content and shade fills while keeping foreground and hit testing");
        Check(HGAEpisodeRenderedCornersAreClear(wrapper),
            @"episode rendering has transparent pixels at all four corners with opaque native content");
        Check(CGRectEqualToRect(entry.frame,frame) && CGRectEqualToRect(content.frame,frame) &&
            CGRectEqualToRect(button.frame,frame) &&
            [entry hitTest:CGPointMake(150,22) withEvent:nil]==button,
            @"episode corners preserve native geometry and center hit target");
        [button sendActionsForControlEvents:UIControlEventTouchUpInside];
        Check(action.taps==1,@"rounded episode entry forwards exactly one native action");
        for(NSUInteger i=0;i<8;i++)Layout(entry);
        Check(Count(entry,@"honggo.appearance.material")==1 && HGAEpisodeRenderedCornersAreClear(wrapper),
            @"repeated episode layout keeps a single rounded surface");
        UIColor *renderedBackground=[UIColor colorWithWhite:0 alpha:0.4];
        [entry renderWithModel:@{@"background":renderedBackground,@"shade":nativeShade}];
        Check([content.backgroundColor isEqual:UIColor.clearColor] && [shade.backgroundColor isEqual:UIColor.clearColor] &&
            Find(entry,@"honggo.appearance.material")==material,
            @"re-rendering the native entry keeps its glass and clears the repainted shade before the next layout");
        HGAppearanceSet(@"player.episodeGlass",@NO,nil);
        Check(!wrapper.clipsToBounds && !entry.clipsToBounds && !content.clipsToBounds &&
            fabs(entry.layer.cornerRadius-3)<0.01 && entry.layer.maskedCorners==kCALayerMinXMinYCorner &&
            [wrapper.backgroundColor isEqual:UIColor.darkGrayColor] &&
            [entry.backgroundColor isEqual:UIColor.grayColor] &&
            [content.backgroundColor isEqual:renderedBackground] && [shade.backgroundColor isEqual:nativeShade],
            @"disabling episode glass restores the latest native fills, clipping and individual corners");

        HGAppearanceSet(@"player.episodeGlass",@YES,nil);Layout(entry);
        content.backgroundColor=UIColor.systemRedColor;entry.layer.cornerRadius=7;
        HGAppearanceSet(@"player.episodeGlass",@NO,nil);
        Check([content.backgroundColor isEqual:UIColor.systemRedColor] && fabs(entry.layer.cornerRadius-7)<0.01,
            @"episode cleanup preserves later native background and corner updates");

        HGAppearanceSet(@"player.episodeGlass",@YES,nil);Layout(entry);
        UIView *other=[[UIView alloc] initWithFrame:wrapper.frame];other.backgroundColor=UIColor.systemGreenColor;
        [root addSubview:other];[other addSubview:entry];
        Check(!wrapper.clipsToBounds && [wrapper.backgroundColor isEqual:UIColor.darkGrayColor] &&
            !other.clipsToBounds && [other.backgroundColor isEqual:UIColor.systemGreenColor],
            @"same-window episode reuse immediately restores its old wrapper without requiring layout");
        [wrapper addSubview:entry];[other removeFromSuperview];Layout(entry);
        UIView *stableGlass=Find(entry,@"honggo.appearance.material");
        entry.hidden=YES;Layout(entry);
        Check(Find(entry,@"honggo.appearance.material")==stableGlass && entry.clipsToBounds &&
            [wrapper.backgroundColor isEqual:UIColor.clearColor],
            @"transient host hidden state preserves episode glass and clear backing while attached to the window");
        entry.hidden=NO;
        Check(Find(entry,@"honggo.appearance.material")==stableGlass && !entry.hidden &&
            [content.backgroundColor isEqual:UIColor.clearColor] && [wrapper.backgroundColor isEqual:UIColor.clearColor],
            @"unhiding the episode entry immediately keeps the same live glass surface without waiting for layout");
        [wrapper bringSubviewToFront:entry];
        [root bringSubviewToFront:wrapper];
        Check(Find(entry,@"honggo.appearance.material")==stableGlass && entry.clipsToBounds && wrapper.clipsToBounds &&
            [wrapper.backgroundColor isEqual:UIColor.clearColor],
            @"same-parent z-order reordering never tears down episode glass or wrapper corners");
        UIView *lateBacking=[[UIView alloc] initWithFrame:entry.bounds];
        lateBacking.backgroundColor=UIColor.blackColor;
        [entry addSubview:lateBacking];
        Check([lateBacking.backgroundColor isEqual:UIColor.clearColor] &&
            entry.subviews.firstObject==stableGlass,
            @"dynamically added host backing plates are cleared immediately behind the live glass");
        [lateBacking removeFromSuperview];
        [wrapper removeFromSuperview];
        [entry renderWithModel:@{@"background":UIColor.brownColor,@"shade":UIColor.darkGrayColor}];
        [root addSubview:wrapper];
        Check(Find(entry,@"honggo.appearance.material")!=nil &&
            [content.backgroundColor isEqual:UIColor.clearColor] && [wrapper.backgroundColor isEqual:UIColor.clearColor],
            @"re-attaching a wrapper after offscreen model rendering synchronously restores episode glass on frame zero");
        UIView *sibling=[[UIView alloc] initWithFrame:CGRectMake(0,0,20,20)];[wrapper addSubview:sibling];
        Check(!wrapper.clipsToBounds && [wrapper.backgroundColor isEqual:UIColor.darkGrayColor],
            @"adding native sibling content immediately releases wrapper clipping without requiring entry layout");
        [sibling removeFromSuperview];Layout(entry);
        [entry removeFromSuperview];
        Check(!wrapper.clipsToBounds && [wrapper.backgroundColor isEqual:UIColor.darkGrayColor] &&
            !entry.clipsToBounds && [content.backgroundColor isEqual:UIColor.brownColor],
            @"episode detach restores every owned surface without retaining the old wrapper");
    }@finally {
        [entry removeFromSuperview];[wrapper removeFromSuperview];
        HGAppearanceSet(@"player.episodeGlass",@YES,nil);
    }
}
