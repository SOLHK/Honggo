@interface FQVShortVideoSettingsViewController_hmd_subfix_ : FQVShortVideoSettingsViewController @end
@implementation FQVShortVideoSettingsViewController_hmd_subfix_ @end
@interface SSShortVideoMorePanelShareHeaderView : UIView
@property(nonatomic,strong) UICollectionView *collectionView;
@end
@implementation SSShortVideoMorePanelShareHeaderView @end
@interface SSShortVideoMorePanelShareHeaderCell : UICollectionViewCell
@property(nonatomic,strong) UIImageView *iconView;
@property(nonatomic,strong) UIView *iconBackgroundView;
@end
@implementation SSShortVideoMorePanelShareHeaderCell @end
@interface FQVShortVideoSettingsCell : UITableViewCell @end
@implementation FQVShortVideoSettingsCell @end
@interface FQVShortVideoSettingsArrowCell : FQVShortVideoSettingsCell @end
@implementation FQVShortVideoSettingsArrowCell @end
@interface FQVShortVideoSettingsOptionView : UIView
@property(nonatomic,getter=isSelected,setter=setIsSelected:) BOOL selected;
@end
@implementation FQVShortVideoSettingsOptionView
- (void)setIsSelected:(BOOL)value {
    _selected=value;
    self.layer.backgroundColor=(value ? UIColor.whiteColor : UIColor.clearColor).CGColor;
}
@end
@interface FQVShortVideoSettingsSwitchOptionView : UIView
@property(nonatomic) BOOL optionSelected;
@property(nonatomic,strong) UIView *selectionBackgroundView;
@end
@implementation FQVShortVideoSettingsSwitchOptionView
- (void)setOptionSelected:(BOOL)value {
    _optionSelected=value;
    self.selectionBackgroundView.backgroundColor=value ? UIColor.whiteColor : UIColor.clearColor;
}
@end
@interface FQCShortVideoCommentListViewController : UIViewController @end
@implementation FQCShortVideoCommentListViewController @end
@interface FQCShortVideoCommentListViewController_hmd_subfix_ : FQCShortVideoCommentListViewController @end
@implementation FQCShortVideoCommentListViewController_hmd_subfix_ @end
@interface FQCShortVideoCommentListCollectionView : UICollectionView @end
@implementation FQCShortVideoCommentListCollectionView @end
@interface FQCShortVideoBottomToolBar : UIView @end
@implementation FQCShortVideoBottomToolBar @end
static NSInteger HGAFixtureShareMode=2;
@interface SSShortVideoSharePanelPresenter : NSObject
+ (NSInteger)presentationMode;
+ (void)presentNativeSharePanelWithParams:(id)params;
@end
@implementation SSShortVideoSharePanelPresenter
+ (NSInteger)presentationMode {return HGAFixtureShareMode;}
+ (void)presentNativeSharePanelWithParams:(id)params {}
@end
@interface SSShortVideoRightInteractionShareViewManager : NSObject
@property(nonatomic) NSInteger requestedMode;
@property(nonatomic,strong) id receivedView;
- (void)shareViewDidClick:(id)view;
@end
@implementation SSShortVideoRightInteractionShareViewManager
- (void)shareViewDidClick:(id)view {
    self.receivedView=view;self.requestedMode=[SSShortVideoSharePanelPresenter presentationMode];
}
@end
@interface HGASheetDrawingFixture : UIView @end
@implementation HGASheetDrawingFixture @end
@interface HGASharePreviewRows : NSObject <UITableViewDataSource> @end
@implementation HGASharePreviewRows
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView{return 2;}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section{return section==0?5:1;}
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)path {
    UITableViewCell *cell=[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:nil];
    NSArray *titles=@[@"当前倍速",@"清晰度",@"清屏播放",@"弹幕",@"画幅设置",@"投屏"];
    NSArray *symbols=@[@"speedometer",@"play.rectangle",@"viewfinder",@"text.bubble",@"aspectratio",@"airplay.video"];
    NSUInteger index=path.section?5:(NSUInteger)path.row;
    cell.textLabel.text=titles[index];cell.textLabel.font=[UIFont systemFontOfSize:16 weight:UIFontWeightMedium];
    cell.imageView.image=[UIImage systemImageNamed:symbols[index]];cell.imageView.tintColor=UIColor.labelColor;
    if(index<2)cell.detailTextLabel.text=index==0?@"1x":@"720P";
    else if(index==3){UISwitch *toggle=[UISwitch new];toggle.on=YES;toggle.onTintColor=UIColor.systemOrangeColor;cell.accessoryView=toggle;}
    else cell.accessoryType=UITableViewCellAccessoryDisclosureIndicator;
    return cell;
}
@end
static void RunCommentSheetAppearanceChecks(UIViewController *host) {
    UIView *root=host.view;
    FQCShortVideoCommentListViewController *panel=[FQCShortVideoCommentListViewController_hmd_subfix_ new];
    [host addChildViewController:panel];[panel loadViewIfNeeded];
    CGFloat width=root.bounds.size.width;
    panel.view.frame=CGRectMake(0,root.bounds.size.height-450,width,450);
    panel.view.backgroundColor=UIColor.whiteColor;
    FQCShortVideoCommentPanelTitleView *title=[[FQCShortVideoCommentPanelTitleView alloc] initWithFrame:CGRectMake(0,0,width,50)];
    FQCShortVideoCommentListCollectionView *list=[[FQCShortVideoCommentListCollectionView alloc]
        initWithFrame:CGRectMake(0,50,width,330) collectionViewLayout:[UICollectionViewFlowLayout new]];
    // UIKit reapplies its adaptive collection background when this list lays out.
    UIColor *nativeListColor=UIColor.systemBackgroundColor;
    list.backgroundColor=nativeListColor;title.backgroundColor=UIColor.whiteColor;
    FQCShortVideoBottomToolBar *toolbar=[[FQCShortVideoBottomToolBar alloc] initWithFrame:CGRectMake(0,380,width,70)];
    toolbar.backgroundColor=UIColor.whiteColor;
    UIBlurEffect *stockBlur=[UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemMaterial];
    UIVisualEffectView *nativeBlur=[[UIVisualEffectView alloc] initWithEffect:stockBlur];
    nativeBlur.frame=toolbar.bounds;[toolbar addSubview:nativeBlur];
    UITextField *editor=[[UITextField alloc] initWithFrame:CGRectMake(10,10,width-20,36)];
    editor.text=@"保留评论输入";[nativeBlur.contentView addSubview:editor];
    [panel.view addSubview:title];[panel.view addSubview:list];[panel.view addSubview:toolbar];
    [root addSubview:panel.view];[panel didMoveToParentViewController:host];
    for(NSUInteger i=0;i<3;i++){Layout(title);Layout(list);Layout(toolbar);[panel viewDidLayoutSubviews];}
    UIVisualEffectView *material=(id)Find(panel.view,@"honggo.appearance.material");
    Check(material && panel.view.subviews.firstObject==material && Count(panel.view,@"honggo.appearance.material")==1 &&
        !Find(list,@"honggo.appearance.material") && !Find(toolbar,@"honggo.appearance.material"),
        @"the observed comment controller installs one material behind its title list and composer");
    Check([list.backgroundColor isEqual:UIColor.clearColor] && [title.backgroundColor isEqual:UIColor.clearColor] &&
        [toolbar.backgroundColor isEqual:UIColor.clearColor] && !nativeBlur.effect && editor.superview==nativeBlur.contentView,
        @"comment covers and native toolbar blur are cleared without replacing the text field");
    HGAppearanceSet(@"panels.comment",@NO,nil);
    Check([nativeBlur.effect isKindOfClass:UIBlurEffect.class],@"disabling comment glass restores the native toolbar effect");
    Check(!Find(panel.view,@"honggo.appearance.material"),@"disabling comment glass removes its material");
    Check([list.backgroundColor isEqual:nativeListColor],@"disabling comment glass restores the adaptive native list color");
    Check([toolbar.backgroundColor isEqual:UIColor.whiteColor] && [title.backgroundColor isEqual:UIColor.whiteColor],
        @"disabling comment glass restores the title and toolbar colors");
    Check([editor.text isEqual:@"保留评论输入"],@"disabling comment glass keeps the original input text");
    [panel.view removeFromSuperview];[panel removeFromParentViewController];HGAppearanceSet(@"panels.comment",@YES,nil);
}
static void RunFeedShareRouteChecks(void) {
    SSShortVideoRightInteractionShareViewManager *manager=[SSShortVideoRightInteractionShareViewManager new];
    id tappedView=[NSObject new];
    [manager shareViewDidClick:tappedView];
    Check(manager.receivedView==tappedView && manager.requestedMode==1 && [SSShortVideoSharePanelPresenter presentationMode]==2,
        @"feed share keeps the original action and uses the native fallback within that action only");
    HGAFixtureShareMode=0;
    [manager shareViewDidClick:tappedView];
    Check(manager.requestedMode==0,@"existing legacy share mode remains available");
    HGAFixtureShareMode=2;
    HGAppearanceSet(@"panels.share",@NO,nil);
    [manager shareViewDidClick:tappedView];
    Check(manager.requestedMode==2,@"disabling share appearance restores the existing Compose route");
    HGAppearanceSet(@"panels.share",@YES,nil);
}
static BOOL HGAWriteModulePreview(UIView *view,NSString *filename) {
    Layout(view);
    for(UIView *child in view.subviews)Layout(child);
    [CATransaction flush];
    UIGraphicsImageRendererFormat *format=[UIGraphicsImageRendererFormat defaultFormat];
    format.scale=2;format.opaque=NO;format.preferredRange=UIGraphicsImageRendererFormatRangeStandard;
    UIGraphicsImageRenderer *renderer=[[UIGraphicsImageRenderer alloc] initWithSize:view.bounds.size format:format];
    __block BOOL captured=NO;
    UIImage *image=[renderer imageWithActions:^(UIGraphicsImageRendererContext *context){
        captured=[view drawViewHierarchyInRect:view.bounds afterScreenUpdates:YES];
    }];
    NSURL *directory=[NSFileManager.defaultManager URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask].firstObject;
    [NSFileManager.defaultManager createDirectoryAtURL:directory withIntermediateDirectories:YES attributes:nil error:nil];
    return captured && [UIImagePNGRepresentation(image) writeToURL:[directory URLByAppendingPathComponent:filename] atomically:YES];
}
static BOOL HGASaveActionPreview(UIView *root) {
    NSDictionary *before=HGAppearanceValues();
    NSMutableDictionary *layout=[NSMutableDictionary dictionary];
    for(NSString *role in @[@"like",@"collect",@"comment",@"share"]){
        layout[[@"layout." stringByAppendingFormat:@"%@.x",role]]=@0;
        layout[[@"layout." stringByAppendingFormat:@"%@.y",role]]=@0;
        layout[[@"layout." stringByAppendingFormat:@"%@.scale",role]]=@1;
        layout[[@"layout." stringByAppendingFormat:@"%@.hidden",role]]=@NO;
    }
    HGAppearanceApply(layout,nil);
    UIView *canvas=[[UIView alloc] initWithFrame:CGRectMake(8,100,160,348)];
    canvas.backgroundColor=[UIColor colorWithWhite:0.1 alpha:1];[root addSubview:canvas];
    NSArray *classes=@[SSShortVideoRightInteractionCollectView.class,SSShortVideoRightInteractionCommentView.class,
        SSShortVideoRightInteractionLikeView.class,SSShortVideoRightInteractionShareView.class];
    NSArray *counts=@[@"10.1万",@"572",@"2万",@"1256"];
    for(NSUInteger i=0;i<classes.count;i++) {
        HGAActionFixture *view=[[classes[i] alloc] initWithFrame:CGRectMake(57,14+i*82,46,62)];
        if([view isKindOfClass:SSShortVideoRightInteractionLikeView.class])
            ((SSShortVideoRightInteractionLikeView *)view).model=[SSShortVideoRightInteractionLikeModel new];
        view.iconImageView.frame=CGRectMake(8,4,30,30);view.iconImageView.tintColor=UIColor.systemBlueColor;
        view.countLabel.frame=CGRectMake(-4,44,54,18);view.countLabel.text=counts[i];
        view.countLabel.font=[UIFont systemFontOfSize:12];view.countLabel.textColor=UIColor.whiteColor;view.countLabel.textAlignment=NSTextAlignmentCenter;
        [canvas addSubview:view];Layout(view);
    }
    BOOL saved=HGAWriteModulePreview(canvas,@"action-preview.png");
    [canvas removeFromSuperview];HGAppearanceApply(before,nil);return saved;
}

static BOOL HGAIconHasOnlyNeutralPixels(UIImage *image) {
    UIGraphicsImageRendererFormat *format=[UIGraphicsImageRendererFormat defaultFormat];
    format.scale=1;format.opaque=NO;format.preferredRange=UIGraphicsImageRendererFormatRangeStandard;
    UIGraphicsImageRenderer *renderer=[[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(40,40) format:format];
    UIImage *rendered=[renderer imageWithActions:^(UIGraphicsImageRendererContext *context){[image drawInRect:CGRectMake(2,2,36,36)];}];
    uint8_t pixels[40*40*4]={0};CGColorSpaceRef space=CGColorSpaceCreateDeviceRGB();
    CGContextRef context=CGBitmapContextCreate(pixels,40,40,8,40*4,space,kCGImageAlphaPremultipliedLast|kCGBitmapByteOrder32Big);
    CGColorSpaceRelease(space);if(!context || !rendered.CGImage){if(context)CGContextRelease(context);return NO;}
    CGContextDrawImage(context,CGRectMake(0,0,40,40),rendered.CGImage);CGContextRelease(context);
    NSUInteger visible=0;
    for(NSUInteger i=0;i<40*40;i++) {
        const uint8_t *pixel=pixels+i*4;
        if(pixel[3]<180)continue;visible++;
        if(fabs((double)pixel[0]-(double)pixel[1])>4 || fabs((double)pixel[1]-(double)pixel[2])>4)return NO;
    }
    return visible>20;
}
static uint8_t HGAActionPixelAlpha(UIImage *image,CGFloat x,CGFloat y) {
    CGImageRef source=image.CGImage;
    if(!source)return 0;
    CGRect pixelRect=CGRectMake(floor(x*image.scale),floor(y*image.scale),1,1);
    CGImageRef pixelImage=CGImageCreateWithImageInRect(source,pixelRect);
    if(!pixelImage)return 0;
    uint8_t pixel[4]={0};
    CGColorSpaceRef space=CGColorSpaceCreateDeviceRGB();
    CGContextRef context=CGBitmapContextCreate(pixel,1,1,8,4,space,kCGImageAlphaPremultipliedLast|kCGBitmapByteOrder32Big);
    CGColorSpaceRelease(space);
    if(context){CGContextDrawImage(context,CGRectMake(0,0,1,1),pixelImage);CGContextRelease(context);}
    CGImageRelease(pixelImage);
    return pixel[3];
}
static void RunActionIconChoices(UIView *root) {
    NSDictionary *saved=HGAppearanceValues();
    NSMutableArray<HGAActionFixture *> *controls=[NSMutableArray array];
    NSArray *classes=@[SSShortVideoRightInteractionCollectView.class,SSShortVideoRightInteractionCommentView.class,
        SSShortVideoRightInteractionLikeView.class,SSShortVideoRightInteractionShareView.class];
    for(Class type in classes) {
        HGAActionFixture *control=[[type alloc] initWithFrame:CGRectMake(18+50*controls.count,170,46,62)];
        if([control isKindOfClass:SSShortVideoRightInteractionLikeView.class])
            ((SSShortVideoRightInteractionLikeView *)control).model=[SSShortVideoRightInteractionLikeModel new];
        [root addSubview:control];
        [controls addObject:control];
    }
    NSMutableArray<NSMutableSet<NSData *> *> *images=[NSMutableArray array];
    for(NSUInteger i=0;i<controls.count;i++)[images addObject:[NSMutableSet set]];
    BOOL neutral=YES;
    for(NSString *preset in @[@"filled",@"circle",@"square"]) {
        HGAppearanceSet(@"icons.preset",preset,nil);
        for(NSUInteger i=0;i<controls.count;i++) {
            Layout(controls[i]);
            UIImage *image=controls[i].iconImageView.image;
            NSData *bytes=UIImagePNGRepresentation(image);
            if(bytes)[images[i] addObject:bytes];
            neutral &= !image.isSymbolImage && HGAIconHasOnlyNeutralPixels(image);
        }
    }
    BOOL distinct=YES;
    for(NSSet *shape in images)distinct &= shape.count==3;
    Check(neutral && distinct,@"all four actions have three distinct original filled icon designs");
    HGAppearanceSet(@"icons.preset",@"filled",nil);
    UIImage *bubble=controls[1].iconImageView.image;
    Check(HGAActionPixelAlpha(controls[0].iconImageView.image,16,16)>240 &&
        HGAActionPixelAlpha(controls[2].iconImageView.image,16,16)>240 &&
        HGAActionPixelAlpha(controls[3].iconImageView.image,16,16)>240 &&
        HGAActionPixelAlpha(bubble,13,14)>240 && HGAActionPixelAlpha(bubble,10,14)<10 &&
        HGAActionPixelAlpha(bubble,16,14)<10 && HGAActionPixelAlpha(bubble,22,14)<10,
        @"reference-inspired preset uses solid silhouettes and three open chat dots");
    HGAppearanceSet(@"icons.comment.normal.symbol",@"paperplane.fill",nil);
    Check(controls[1].iconImageView.image.isSymbolImage &&
        ![UIImagePNGRepresentation(controls[1].iconImageView.image) isEqual:UIImagePNGRepresentation(bubble)],
        @"a custom system symbol overrides the selected preset");
    for(UIView *control in controls)[control removeFromSuperview];
    HGAppearanceApply(saved,nil);
}
static void RunActionShareAppearanceChecks(SSTabBarController *host) {
    UIView *root=host.view;
    NSDictionary *before=HGAppearanceValues();
    HGAppearanceApply(@{@"enabled":@YES,@"icons.enabled":@YES,@"icons.preset":@"filled",@"icons.size":@26,
        @"player.rightGlass":@YES,@"panels.longpress":@YES,@"panels.share":@YES,
        @"panels.material":@"regular",@"panels.opacity":@0.28},nil);
    SSShortVideoRightInteractionCommentView *comment=[[SSShortVideoRightInteractionCommentView alloc] initWithFrame:CGRectMake(20,100,46,62)];
    comment.iconImageView.tintColor=UIColor.systemBlueColor;comment.iconImageView.contentMode=UIViewContentModeScaleToFill;
    UIImageSymbolConfiguration *nativeConfiguration=[UIImageSymbolConfiguration configurationWithPointSize:32 weight:UIImageSymbolWeightHeavy];
    comment.iconImageView.preferredSymbolConfiguration=nativeConfiguration;
    UIImage *nativeImage=comment.iconImageView.image;
    CGRect touchFrame=comment.frame,countFrame=comment.countLabel.frame;
    [root addSubview:comment];Layout(comment);
    @try {
        Check([comment.iconImageView.tintColor isEqual:UIColor.whiteColor] &&
            comment.iconImageView.image.renderingMode==UIImageRenderingModeAlwaysOriginal &&
            HGAIconHasOnlyNeutralPixels(comment.iconImageView.image),
            @"right action glyphs render neutral white over the glass");
        Check(comment.iconImageView.contentMode==UIViewContentModeScaleAspectFit &&
            CGRectEqualToRect(comment.frame,touchFrame) && CGRectEqualToRect(comment.countLabel.frame,countFrame),
            @"solid glyph proportions preserve the native action and count geometry");
        UIView *material=Find(comment,@"honggo.appearance.material");
        Check(material && material.bounds.size.width<=40.01 && !material.userInteractionEnabled && material.alpha==1,
            @"right actions use compact glass at full effect-view opacity with unchanged native hit handling");
        [comment sendActionsForControlEvents:UIControlEventTouchUpInside];
        Check(comment.taps==1,@"redrawn right action still delivers one original tap");
        for(NSUInteger i=0;i<8;i++)Layout(comment);
        Check(Count(comment,@"honggo.appearance.material")==1 && HGAIconHasOnlyNeutralPixels(comment.iconImageView.image),
            @"right action reuse keeps a single surface and stable neutral rendering");
        HGAppearanceSet(@"icons.enabled",@NO,nil);
        Check([comment.iconImageView.tintColor isEqual:UIColor.systemBlueColor] &&
            comment.iconImageView.contentMode==UIViewContentModeScaleToFill &&
            [comment.iconImageView.preferredSymbolConfiguration isEqual:nativeConfiguration] &&
            ImageContentEqual(comment.iconImageView.image,nativeImage),
            @"disabling redrawn icons restores native tint content mode symbol configuration and image");
    }@finally {[comment removeFromSuperview];HGAppearanceSet(@"icons.enabled",@YES,nil);}

    // A real action image view can be rectangular even when its hit owner is
    // square. Verify that its long edge cannot balloon into the count/dock.
    SSShortVideoRightInteractionShareView *share=[[SSShortVideoRightInteractionShareView alloc] initWithFrame:CGRectMake(76,100,46,62)];
    share.iconImageView.frame=CGRectMake(14,2,18,36);
    CGRect shareTouch=share.frame,shareCount=share.countLabel.frame;
    [root addSubview:share];Layout(share);
    @try {
        CGRect iconRect=[share.iconImageView convertRect:share.iconImageView.bounds toView:share];
        UIView *material=Find(share,@"honggo.appearance.material");
        Check(fabs(material.bounds.size.width-38)<0.01 && MAX(iconRect.size.width,iconRect.size.height)<=22.37 &&
            CGRectEqualToRect(share.frame,shareTouch) && CGRectEqualToRect(share.countLabel.frame,shareCount),
            @"tall native action icon fits the requested longest edge without moving count or hit owner");
        CGAffineTransform first=share.iconImageView.transform;
        for(NSUInteger i=0;i<8;i++)Layout(share);
        Check(CGAffineTransformEqualToTransform(first,share.iconImageView.transform),
            @"rectangular right-action icon scale stays stable across native reuse");
        HGAppearanceSet(@"icons.size",@48,nil);Layout(share);
        iconRect=[share.iconImageView convertRect:share.iconImageView.bounds toView:share];
        Check(MAX(iconRect.size.width,iconRect.size.height)<=28.01,
            @"large custom action size leaves a five-point inset on each edge of the glass");
        HGAppearanceSet(@"icons.enabled",@NO,nil);
        Check(CGAffineTransformIsIdentity(share.iconImageView.transform),
            @"disabling rectangular icon styling restores the native image transform");
    }@finally {
        [share removeFromSuperview];HGAppearanceApply(@{@"icons.enabled":@YES,@"icons.size":@26},nil);
    }
    RunActionIconChoices(root);
    Check(HGASaveActionPreview(root),@"controlled UIKit right-action preview renders to PNG for visual review");

    FQVShortVideoSettingsViewController_hmd_subfix_ *panel=[FQVShortVideoSettingsViewController_hmd_subfix_ new];
    [host addChildViewController:panel];[panel loadViewIfNeeded];
    panel.view.frame=CGRectMake(0,root.bounds.size.height-500,root.bounds.size.width,500);panel.view.backgroundColor=UIColor.grayColor;
    panel.view.layer.cornerRadius=6;panel.view.layer.maskedCorners=kCALayerMinXMaxYCorner;panel.view.clipsToBounds=NO;
    UITableView *table=[[UITableView alloc] initWithFrame:CGRectMake(0,28,panel.view.bounds.size.width,472) style:UITableViewStyleInsetGrouped];
    table.backgroundColor=UIColor.grayColor;[panel.view addSubview:table];
    SSShortVideoMorePanelShareHeaderView *header=[[SSShortVideoMorePanelShareHeaderView alloc] initWithFrame:CGRectMake(0,0,panel.view.bounds.size.width,98)];
    header.backgroundColor=UIColor.whiteColor;table.tableHeaderView=header;
    SSShortVideoMorePanelShareHeaderCell *cell=[[SSShortVideoMorePanelShareHeaderCell alloc] initWithFrame:CGRectMake(100,8,52,70)];
    cell.iconBackgroundView=[[UIView alloc] initWithFrame:CGRectMake(0,0,52,52)];
    cell.iconBackgroundView.backgroundColor=UIColor.systemGreenColor;
    UIImage *nativeIcon=[[UIImage systemImageNamed:@"bubble.left.and.bubble.right.fill"] imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
    cell.iconView=[[UIImageView alloc] initWithImage:nativeIcon];cell.iconView.frame=CGRectMake(12,12,28,28);
    cell.iconView.tintColor=UIColor.systemBlueColor;
    [cell.contentView addSubview:cell.iconBackgroundView];[cell.contentView addSubview:cell.iconView];[header addSubview:cell];
    UIButton *destination=[UIButton buttonWithType:UIButtonTypeCustom];destination.frame=CGRectMake(20,8,52,52);
    destination.backgroundColor=UIColor.systemGreenColor;[destination setTitle:@"微信" forState:UIControlStateNormal];
    [header addSubview:destination];
    HGAEpisodeTapFixture *action=[HGAEpisodeTapFixture new];
    [destination addTarget:action action:@selector(open:) forControlEvents:UIControlEventTouchUpInside];
    HGASheetDrawingFixture *drawing=[[HGASheetDrawingFixture alloc] initWithFrame:CGRectMake(0,120,panel.view.bounds.size.width,80)];
    drawing.backgroundColor=UIColor.whiteColor;[panel.view addSubview:drawing];
    UIView *handle=[[UIView alloc] initWithFrame:CGRectMake((panel.view.bounds.size.width-36)/2,8,36,4)];
    handle.backgroundColor=UIColor.darkGrayColor;[panel.view addSubview:handle];
    HGAppearanceSet(@"dock.enabled",@YES,nil);
    HGAppearanceLayoutDock(host.customTabBar,host);
    UIView *dock=Find(host.customTabBar,@"honggo.appearance.dock");
    Check(dock && !dock.hidden,@"custom dock is visible before a bottom action sheet appears");
    [root addSubview:panel.view];[panel didMoveToParentViewController:host];Layout(panel.view);[panel viewDidLayoutSubviews];
    CGRect originalTable=table.frame,originalDestination=destination.frame;
    @try {
        UIVisualEffectView *base=(id)Find(panel.view,@"honggo.appearance.material");Layout(base);
        UITraitCollection *light=[UITraitCollection traitCollectionWithUserInterfaceStyle:UIUserInterfaceStyleLight];
        UITraitCollection *dark=[UITraitCollection traitCollectionWithUserInterfaceStyle:UIUserInterfaceStyleDark];
        BOOL nativeGlass=HGNativeGlassConfigured();
        UIColor *paint=base.contentView.backgroundColor;
        if(@available(iOS 26.0,*)) {
            if(nativeGlass && [base.effect isKindOfClass:UIGlassEffect.class]) {
                Check([paint isEqual:UIColor.clearColor],@"the native card has no flat overlay above its optical material");
                paint=[(UIGlassEffect *)base.effect tintColor];
            }
        }
        CGFloat lightWhite=0,lightAlpha=0,darkWhite=0,darkAlpha=0;
        [[paint resolvedColorWithTraitCollection:light] getWhite:&lightWhite alpha:&lightAlpha];
        [[paint resolvedColorWithTraitCollection:dark] getWhite:&darkWhite alpha:&darkAlpha];
        Class expectedEffect=nativeGlass ? NSClassFromString(@"UIGlassEffect") : UIBlurEffect.class;
        BOOL configuredTint=fabs(lightAlpha-0.28)<0.01 && fabs(darkAlpha-0.28)<0.01;
        BOOL readableBlur=lightAlpha>0.68 && lightAlpha<1 && darkAlpha>0.60 && darkAlpha<1;
        Check(base && [base.effect isKindOfClass:expectedEffect] && !base.userInteractionEnabled &&
            lightWhite>0.95 && darkWhite<0.2 && (nativeGlass ? configuredTint : readableBlur) &&
            panel.view.subviews.firstObject==base,
            @"the longpress card uses native glass tint or a readable adaptive blur fill");
        HGAppearanceLayoutDock(host.customTabBar,host);
        Check(dock.hidden,@"mounted longpress sheet keeps the Dock out of its backdrop even after a Dock refresh");

        // Match a row that is only dequeued after the user scrolls to the end.
        FQVShortVideoSettingsArrowCell *lateRow=[[FQVShortVideoSettingsArrowCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil];
        lateRow.frame=CGRectMake(20,425,panel.view.bounds.size.width-40,54);
        UIColor *nativeRowColor=UIColor.systemBackgroundColor;
        lateRow.backgroundColor=nativeRowColor;lateRow.contentView.backgroundColor=nativeRowColor;
        [panel.view addSubview:lateRow];Layout(lateRow);
        Check([lateRow.backgroundColor isEqual:UIColor.clearColor] &&
            [lateRow.contentView.backgroundColor isEqual:UIColor.clearColor],
            @"late-mounted bottom function rows blend into the main glass instead of making an opaque card");
        FQVShortVideoSettingsOptionView *resolution=[[FQVShortVideoSettingsOptionView alloc] initWithFrame:CGRectMake(120,310,45,30)];
        FQVShortVideoSettingsSwitchOptionView *speed=[[FQVShortVideoSettingsSwitchOptionView alloc] initWithFrame:CGRectMake(190,310,62,30)];
        speed.selectionBackgroundView=[[UIView alloc] initWithFrame:speed.bounds];[speed addSubview:speed.selectionBackgroundView];
        [panel.view addSubview:resolution];[panel.view addSubview:speed];
        [resolution setIsSelected:YES];speed.optionSelected=YES;
        Check(CGColorGetAlpha(resolution.layer.backgroundColor)<0.4 &&
            CGColorGetAlpha(resolution.layer.backgroundColor)>0.05 &&
            CGColorGetAlpha(speed.selectionBackgroundView.layer.backgroundColor)<0.4 &&
            CGColorGetAlpha(speed.selectionBackgroundView.layer.backgroundColor)>0.05,
            @"native speed and quality selection use a translucent highlight without hard white tiles");
        [resolution setIsSelected:NO];speed.optionSelected=NO;
        Check(CGColorGetAlpha(resolution.layer.backgroundColor)<0.01 &&
            CGColorGetAlpha(speed.selectionBackgroundView.layer.backgroundColor)<0.01,
            @"moving the selection leaves its previous positions clear");
        [resolution setIsSelected:YES];speed.optionSelected=YES;
        Check([table.backgroundColor isEqual:UIColor.clearColor] && [header.backgroundColor isEqual:UIColor.clearColor] &&
            [destination.backgroundColor isEqual:UIColor.systemGreenColor] && [drawing.backgroundColor isEqual:UIColor.whiteColor],
            @"share sheet clears neutral backings and preserves unknown drawing surfaces");
        Check(cell.iconView.image.renderingMode==UIImageRenderingModeAlwaysTemplate &&
            [[cell.iconView.tintColor resolvedColorWithTraitCollection:light] isEqual:UIColor.blackColor] &&
            [[cell.iconView.tintColor resolvedColorWithTraitCollection:dark] isEqual:UIColor.whiteColor] &&
            ![cell.iconBackgroundView.backgroundColor isEqual:UIColor.systemGreenColor],
            @"native share destinations retain their glyph with theme-aware black or white ink and a neutral tile");
        UIGraphicsImageRenderer *tile=[[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(48,48)];
        cell.iconView.image=[tile imageWithActions:^(UIGraphicsImageRendererContext *context) {
            [UIColor.greenColor setFill];[context fillRect:CGRectMake(0,0,48,48)];
            [UIColor.whiteColor setFill];[context fillRect:CGRectMake(14,16,20,16)];
        }];
        Layout(cell);
        Check(HGAActionPixelAlpha(cell.iconView.image,8,8)==0 && HGAActionPixelAlpha(cell.iconView.image,24,24)>200,
            @"a destination image with a baked brand tile keeps an opaque glyph and transparent surrounding pixels");
        cell.iconView.image=nativeIcon;Layout(cell);
        CACornerMask cardCorners=kCALayerMinXMinYCorner|kCALayerMaxXMinYCorner|kCALayerMinXMaxYCorner|kCALayerMaxXMaxYCorner;
        Check(panel.view.clipsToBounds && fabs(panel.view.layer.cornerRadius-28)<0.01 &&
            panel.view.layer.maskedCorners==cardCorners &&
            CGRectEqualToRect(table.frame,originalTable) && CGRectEqualToRect(destination.frame,originalDestination),
            @"sheet uses one continuous four-corner card without moving native table rows or share destinations");
        [destination sendActionsForControlEvents:UIControlEventTouchUpInside];
        Check(action.taps==1 && [destination titleForState:UIControlStateNormal].length>0 && destination.superview==header,
            @"longpress share destination retains its original title hierarchy and action");
        for(NSUInteger i=0;i<6;i++){Layout(header);[panel viewDidLayoutSubviews];}
        Check(Count(panel.view,@"honggo.appearance.material")==1,
            @"repeated native panel and header layouts never stack materials");
        HGASharePreviewRows *previewRows=[HGASharePreviewRows new];
        table.dataSource=previewRows;table.rowHeight=56;[table reloadData];Layout(table);
        drawing.hidden=YES;
        UIView *previewBackdrop=[[UIView alloc] initWithFrame:root.bounds];previewBackdrop.backgroundColor=[UIColor colorWithWhite:0.08 alpha:1];
        [root insertSubview:previewBackdrop belowSubview:panel.view];
        Check(HGAWriteModulePreview(panel.view,@"share-preview.png"),@"controlled UIKit longpress sheet preview renders to PNG for visual review");
        [previewBackdrop removeFromSuperview];drawing.hidden=NO;
        table.dataSource=nil;[table reloadData];
        HGAppearanceSet(@"panels.longpress",@NO,nil);
        Check(!dock.hidden,@"disabling longpress glass restores Dock visibility");
        Check([lateRow.backgroundColor isEqual:nativeRowColor] && [lateRow.contentView.backgroundColor isEqual:nativeRowColor],
            @"disabling longpress glass restores native function row colors");
        Check(CGColorGetAlpha(resolution.layer.backgroundColor)>0.99 &&
            CGColorGetAlpha(speed.selectionBackgroundView.layer.backgroundColor)>0.99,
            @"disabling longpress glass restores both native selected plates");
        Check(!Find(panel.view,@"honggo.appearance.material") &&
            [table.backgroundColor isEqual:UIColor.grayColor] && [header.backgroundColor isEqual:UIColor.whiteColor] &&
            [panel.view.backgroundColor isEqual:UIColor.grayColor] && !panel.view.clipsToBounds &&
            fabs(panel.view.layer.cornerRadius-6)<0.01 && panel.view.layer.maskedCorners==kCALayerMinXMaxYCorner,
            @"disabling longpress styling fully restores its native fills clipping and corners");
        Check(cell.iconView.image==nativeIcon && [cell.iconView.tintColor isEqual:UIColor.systemBlueColor] &&
            [cell.iconBackgroundView.backgroundColor isEqual:UIColor.systemGreenColor],@"share icon, tint and tile restore on disable");
        HGAppearanceSet(@"panels.longpress",@YES,nil);[panel viewDidLayoutSubviews];
        table.backgroundColor=UIColor.systemOrangeColor;panel.view.layer.cornerRadius=11;
        HGAppearanceSet(@"panels.longpress",@NO,nil);
        Check([table.backgroundColor isEqual:UIColor.systemOrangeColor] && fabs(panel.view.layer.cornerRadius-11)<0.01,
            @"panel cleanup preserves later host background and corner changes");
        SSVideoSharePanelViewController *fullScreen=[SSVideoSharePanelViewController new];
        [host addChildViewController:fullScreen];[fullScreen loadViewIfNeeded];
        fullScreen.view.frame=root.bounds;
        UIColor *dismissalColour=[UIColor.blackColor colorWithAlphaComponent:0.4];fullScreen.view.backgroundColor=dismissalColour;
        UIButton *dismiss=[UIButton buttonWithType:UIButtonTypeCustom];dismiss.frame=fullScreen.view.bounds;
        [dismiss addTarget:action action:@selector(open:) forControlEvents:UIControlEventTouchUpInside];
        [fullScreen.view addSubview:dismiss];[root addSubview:fullScreen.view];
        [fullScreen didMoveToParentViewController:host];[fullScreen viewDidLayoutSubviews];
        Check(!Find(fullScreen.view,@"honggo.appearance.material") && [fullScreen.view.backgroundColor isEqual:dismissalColour],
            @"an unverified full-screen share dismissal region is never converted to an opaque sheet");
        [dismiss sendActionsForControlEvents:UIControlEventTouchUpInside];
        Check(action.taps==2,@"native full-screen share dismissal action remains intact");
        UIView *shareSheet=[[UIView alloc] initWithFrame:CGRectMake(0,fullScreen.view.bounds.size.height-250,
            fullScreen.view.bounds.size.width,250)];
        shareSheet.backgroundColor=UIColor.whiteColor;
        fullScreen.panelView=shareSheet;[fullScreen.view addSubview:shareSheet];[fullScreen viewDidLayoutSubviews];
        UIVisualEffectView *nativeSharePanel=(id)Find(shareSheet,@"honggo.appearance.material");Layout(nativeSharePanel);
        Check(nativeSharePanel.superview==shareSheet && [nativeSharePanel.effect isKindOfClass:expectedEffect] &&
            Count(fullScreen.view,@"honggo.appearance.material")==1 && shareSheet.layer.maskedCorners==cardCorners &&
            [fullScreen.view.backgroundColor isEqual:dismissalColour] && !nativeSharePanel.userInteractionEnabled && dock.hidden,
            @"verified native share panel receives the same material within its dismissible full-screen wrapper");
        HGAppearanceSet(@"panels.share",@NO,nil);
        Check(!Find(shareSheet,@"honggo.appearance.material") && [shareSheet.backgroundColor isEqual:UIColor.whiteColor],
            @"standalone share control restores its native backing independently of longpress");
        HGAppearanceSet(@"panels.share",@YES,nil);[fullScreen viewDidLayoutSubviews];
        Check(dock.hidden,@"reattaching native share glass hides the underlying Dock again");
        [fullScreen.view removeFromSuperview];[fullScreen removeFromParentViewController];
        Check(!dock.hidden,@"native share dismissal restores the Dock when the sheet detaches");
    }@finally {
        [panel.view removeFromSuperview];[panel removeFromParentViewController];
        HGAppearanceReset();HGAppearanceApply(before,nil);
    }
}
