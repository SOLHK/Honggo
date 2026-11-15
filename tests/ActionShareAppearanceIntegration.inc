@interface FQVShortVideoSettingsViewController_hmd_subfix_ : FQVShortVideoSettingsViewController @end
@implementation FQVShortVideoSettingsViewController_hmd_subfix_ @end
@interface SSShortVideoMorePanelShareHeaderView : UIView
@property(nonatomic,strong) UICollectionView *collectionView;
@end
@implementation SSShortVideoMorePanelShareHeaderView @end
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
    NSArray *classes=@[SSShortVideoRightInteractionLikeView.class,SSShortVideoRightInteractionCommentView.class,
        SSShortVideoRightInteractionCollectView.class,SSShortVideoRightInteractionShareView.class];
    NSArray *counts=@[@"2.8万",@"1077",@"19.3万",@"分享"];
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

static BOOL HGASymbolHasOnlyNeutralPixels(UIImage *image) {
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
static void RunActionShareAppearanceChecks(SSTabBarController *host) {
    UIView *root=host.view;
    NSDictionary *before=HGAppearanceValues();
    HGAppearanceApply(@{@"enabled":@YES,@"icons.enabled":@YES,@"icons.preset":@"outline",@"icons.size":@26,
        @"player.rightGlass":@YES,@"panels.longpress":@YES,@"panels.share":@YES},nil);
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
            HGASymbolHasOnlyNeutralPixels(comment.iconImageView.image),
            @"right action symbols render neutral white instead of inherited host blue");
        Check(comment.iconImageView.contentMode==UIViewContentModeScaleAspectFit &&
            CGRectEqualToRect(comment.frame,touchFrame) && CGRectEqualToRect(comment.countLabel.frame,countFrame),
            @"SF symbol proportions improve without resizing the native action or count");
        UIView *material=Find(comment,@"honggo.appearance.material");
        Check(material && material.bounds.size.width<=40.01 && !material.userInteractionEnabled,
            @"right actions use compact unobtrusive material with unchanged native hit handling");
        [comment sendActionsForControlEvents:UIControlEventTouchUpInside];
        Check(comment.taps==1,@"redrawn right action still delivers one original tap");
        for(NSUInteger i=0;i<8;i++)Layout(comment);
        Check(Count(comment,@"honggo.appearance.material")==1 && HGASymbolHasOnlyNeutralPixels(comment.iconImageView.image),
            @"right action reuse keeps a single surface and stable neutral rendering");
        HGAppearanceSet(@"icons.enabled",@NO,nil);
        Check([comment.iconImageView.tintColor isEqual:UIColor.systemBlueColor] &&
            comment.iconImageView.contentMode==UIViewContentModeScaleToFill &&
            [comment.iconImageView.preferredSymbolConfiguration isEqual:nativeConfiguration] &&
            ImageContentEqual(comment.iconImageView.image,nativeImage),
            @"disabling redrawn icons restores native tint content mode symbol configuration and image");
    }@finally {[comment removeFromSuperview];HGAppearanceSet(@"icons.enabled",@YES,nil);}
    Check(HGASaveActionPreview(root),@"controlled UIKit right-action preview renders to PNG for visual review");

    FQVShortVideoSettingsViewController_hmd_subfix_ *panel=[FQVShortVideoSettingsViewController_hmd_subfix_ new];
    [host addChildViewController:panel];[panel loadViewIfNeeded];
    panel.view.frame=CGRectMake(0,200,root.bounds.size.width,500);panel.view.backgroundColor=UIColor.grayColor;
    panel.view.layer.cornerRadius=6;panel.view.layer.maskedCorners=kCALayerMinXMaxYCorner;panel.view.clipsToBounds=NO;
    UITableView *table=[[UITableView alloc] initWithFrame:CGRectMake(0,28,panel.view.bounds.size.width,472) style:UITableViewStyleInsetGrouped];
    table.backgroundColor=UIColor.grayColor;[panel.view addSubview:table];
    SSShortVideoMorePanelShareHeaderView *header=[[SSShortVideoMorePanelShareHeaderView alloc] initWithFrame:CGRectMake(0,0,panel.view.bounds.size.width,98)];
    header.backgroundColor=UIColor.whiteColor;table.tableHeaderView=header;
    UIButton *destination=[UIButton buttonWithType:UIButtonTypeCustom];destination.frame=CGRectMake(20,8,52,52);
    destination.backgroundColor=UIColor.systemGreenColor;[destination setTitle:@"微信" forState:UIControlStateNormal];
    [header addSubview:destination];
    HGAEpisodeTapFixture *action=[HGAEpisodeTapFixture new];
    [destination addTarget:action action:@selector(open:) forControlEvents:UIControlEventTouchUpInside];
    HGASheetDrawingFixture *drawing=[[HGASheetDrawingFixture alloc] initWithFrame:CGRectMake(0,120,panel.view.bounds.size.width,80)];
    drawing.backgroundColor=UIColor.whiteColor;[panel.view addSubview:drawing];
    UIView *handle=[[UIView alloc] initWithFrame:CGRectMake((panel.view.bounds.size.width-36)/2,8,36,4)];
    handle.backgroundColor=UIColor.darkGrayColor;[panel.view addSubview:handle];
    [root addSubview:panel.view];[panel didMoveToParentViewController:host];Layout(panel.view);[panel viewDidLayoutSubviews];
    CGRect originalTable=table.frame,originalDestination=destination.frame;
    @try {
        UIVisualEffectView *base=(id)Find(panel.view,@"honggo.appearance.material");Layout(base);
        CGFloat alpha=CGColorGetAlpha([base.contentView.backgroundColor resolvedColorWithTraitCollection:base.traitCollection].CGColor);
        Check(base && [base.effect isKindOfClass:UIBlurEffect.class] && alpha>=0.92 && panel.view.subviews.firstObject==base,
            @"longpress sheet uses a readable system blur base behind content without a glass lens over the dock");
        Check([table.backgroundColor isEqual:UIColor.clearColor] && [header.backgroundColor isEqual:UIColor.clearColor] &&
            [destination.backgroundColor isEqual:UIColor.systemGreenColor] && [drawing.backgroundColor isEqual:UIColor.whiteColor],
            @"share sheet clears neutral backings while retaining app colours and unknown drawing surfaces");
        Check(panel.view.clipsToBounds && fabs(panel.view.layer.cornerRadius-28)<0.01 &&
            panel.view.layer.maskedCorners==(kCALayerMinXMinYCorner|kCALayerMaxXMinYCorner) &&
            CGRectEqualToRect(table.frame,originalTable) && CGRectEqualToRect(destination.frame,originalDestination),
            @"sheet clips continuous top corners without moving native table rows or share destinations");
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
        Check(!Find(panel.view,@"honggo.appearance.material") &&
            [table.backgroundColor isEqual:UIColor.grayColor] && [header.backgroundColor isEqual:UIColor.whiteColor] &&
            [panel.view.backgroundColor isEqual:UIColor.grayColor] && !panel.view.clipsToBounds &&
            fabs(panel.view.layer.cornerRadius-6)<0.01 && panel.view.layer.maskedCorners==kCALayerMinXMaxYCorner,
            @"disabling longpress styling fully restores its native fills clipping and corners");
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
        [fullScreen.view removeFromSuperview];[fullScreen removeFromParentViewController];
    }@finally {
        [panel.view removeFromSuperview];[panel removeFromParentViewController];
        HGAppearanceReset();HGAppearanceApply(before,nil);
    }
}
