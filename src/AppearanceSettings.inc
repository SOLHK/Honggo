#import "SettingsTransition.h"
@interface HGAAppearanceController : UITableViewController <UISearchResultsUpdating,UISearchControllerDelegate,PHPickerViewControllerDelegate,UIDocumentPickerDelegate,UIFontPickerViewControllerDelegate,UIColorPickerViewControllerDelegate>
@property(nonatomic,copy) NSString *group;
@property(nonatomic,copy) NSArray<NSDictionary *> *rows;
@property(nonatomic,strong) UISearchController *search;
@property(nonatomic,copy) dispatch_block_t pendingAction;
@property(nonatomic,copy) NSString *assetKey;
@property(nonatomic,copy) NSString *colorKey;
@property(nonatomic,copy) NSString *documentPurpose;
@property(nonatomic) BOOL busy;
@end
static NSDictionary *HGATool(NSString *key,NSString *title){return @{@"key":key,@"title":title,@"kind":@"action"};}
@implementation HGAAppearanceController
- (void)viewDidLoad {
    [super viewDidLoad];
    NSUInteger groupIndex=[HGAppearanceGroups() indexOfObject:self.group ?: @"general"];
    self.title=self.group && groupIndex!=NSNotFound ? HGAppearanceGroupTitles()[groupIndex] : @"外观定制";
    self.tableView.accessibilityIdentifier=@"honggo.appearance.settings";
    self.tableView.rowHeight=UITableViewAutomaticDimension;self.tableView.estimatedRowHeight=68;
    self.search=[[UISearchController alloc] initWithSearchResultsController:nil];
    self.search.searchResultsUpdater=self;self.search.delegate=self;
    self.search.obscuresBackgroundDuringPresentation=NO;
    self.search.searchBar.placeholder=@"搜索底栏、图标、字体或控件名称";
    self.navigationItem.searchController=self.search;self.navigationItem.hidesSearchBarWhenScrolling=NO;
    self.definesPresentationContext=YES;
    if(self.navigationController.viewControllers.firstObject==self)
        self.navigationItem.rightBarButtonItem=[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemDone target:self action:@selector(done)];
    [self reloadRows];
}
- (void)viewWillAppear:(BOOL)animated {[super viewWillAppear:animated];[self reloadRows];}
- (void)done {
    UIViewController *presenter=self.navigationController.presentingViewController;
    [self afterSearch:^{[presenter dismissViewControllerAnimated:YES completion:nil];}];
}
- (void)afterSearch:(dispatch_block_t)action {
    if(self.search.active || self.search.isBeingDismissed || self.presentedViewController==self.search) {
        self.pendingAction=action;
        if(!self.search.isBeingDismissed) HGAfterViewTransition(self.search, ^{
            if(self.pendingAction)self.search.active=NO;
        });
    }else if(!self.presentedViewController) HGAfterViewTransition(self, ^{
        if(self.viewIfLoaded.window && !self.presentedViewController &&
            !self.isBeingDismissed && !self.navigationController.isBeingDismissed)action();
    });
}
- (void)didDismissSearchController:(UISearchController *)searchController {
    HGAfterViewTransition(self, ^{
        dispatch_block_t action=self.pendingAction;self.pendingAction=nil;
        if(action && self.viewIfLoaded.window && !self.presentedViewController &&
            !self.isBeingDismissed && !self.navigationController.isBeingDismissed)action();
    });
}
- (void)updateSearchResultsForSearchController:(UISearchController *)controller {[self reloadRows];}
- (void)reloadRows {
    NSString *query=[self.search.searchBar.text ?: @"" stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
    NSMutableArray *rows=[NSMutableArray array];
    if(!self.group && !query.length) {
        [rows addObject:HGAppearanceCatalog().firstObject];
        [rows addObject:HGATool(@"@preset.glass",@"应用「玻璃外观」预设")];
        [rows addObject:HGATool(@"@preset.focus",@"应用「专注播放」预设")];
        for(NSUInteger i=1;i<HGAppearanceGroups().count;i++)[rows addObject:HGATool([@"@group." stringByAppendingString:HGAppearanceGroups()[i]],HGAppearanceGroupTitles()[i])];
        [rows addObject:HGATool(@"@export",@"导出外观参数")];
        [rows addObject:HGATool(@"@import",@"从文件恢复外观参数")];
        [rows addObject:HGATool(@"@reset",@"恢复全部外观默认值")];
    }else {
        for(NSDictionary *def in HGAppearanceCatalog()) {
            if(self.group && ![def[@"group"] isEqual:self.group])continue;
            NSString *haystack=[NSString stringWithFormat:@"%@ %@",def[@"title"],def[@"key"]];
            BOOL match=YES;
            for(NSString *word in [query componentsSeparatedByCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet])
                if(word.length && [haystack rangeOfString:word options:NSCaseInsensitiveSearch|NSDiacriticInsensitiveSearch].location==NSNotFound){match=NO;break;}
            if(match)[rows addObject:def];
        }
        if([self.group isEqual:@"fonts"] && !query.length)[rows addObject:HGATool(@"@fonts.reset",@"恢复宿主原字体与字号")];
    }
    self.rows=rows;
    UILabel *empty=nil;
    if(!rows.count){empty=[UILabel new];empty.text=@"没有匹配的外观设置";empty.textAlignment=NSTextAlignmentCenter;empty.textColor=UIColor.secondaryLabelColor;}
    self.tableView.backgroundView=empty;[self.tableView reloadData];
}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section{return self.rows.count;}
- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
    if([self.group isEqual:@"fonts"])return @"只改变已适配播放页、面板及底栏的原生文字，不修改视频内字幕、网页或自绘文字。编辑中的输入框、密码、图标字体保持原样。字体导入上限 50 MiB，仅本机使用。";
    if([self.group isEqual:@"player"])return @"偏移单位为点，比例 1 表示原始大小。作者与剧集信息按容器整体调整。横屏／方形画面不放大；竖屏填满只修改画面，不修改翻页距离、播放器逻辑和系统状态栏。";
    if([self.group isEqual:@"dock"])return @"底栏优先保留至少 44 点的点击区域，尺寸会受宿主可用空间限制。上滑收缩，下滑或点击圆钮展开；语音朗读开启时保持展开。搜索需要在首页有可用宿主入口。";
    if([self.group isEqual:@"icons"])return @"图片优先于 SF Symbol，最后使用预设。收藏和点赞按宿主真实状态显示；无法读取状态时保留原图标。自选图片在本地缩小为不超过 256 像素的 PNG。";
    if([self.group isEqual:@"panels"])return @"仅处理已识别面板，不隐藏绘制内容、输入框或点击遮罩。未匹配的面板保留原样，适配状态可以随日志导出。";
    return @"先开启「启用外观定制」，或应用预设。多数调整即时生效；首次开启原生玻璃后请完全退出红果再打开，启动前未启用玻璃时先使用磨砂。外观参数备份不包含图片与字体文件；恢复时需在同一设备保留这些资源。R5 为测试版，真实宿主效果仍需验收。";
}
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)path {
    NSDictionary *row=self.rows[path.row];NSString *kind=row[@"kind"],*key=row[@"key"];
    UITableViewCell *cell=[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:nil];
    cell.textLabel.text=row[@"title"];cell.textLabel.numberOfLines=0;cell.detailTextLabel.numberOfLines=0;
    cell.textLabel.font=[UIFont preferredFontForTextStyle:UIFontTextStyleBody];
    cell.detailTextLabel.font=[UIFont preferredFontForTextStyle:UIFontTextStyleCaption1];
    cell.textLabel.adjustsFontForContentSizeCategory=YES;cell.detailTextLabel.adjustsFontForContentSizeCategory=YES;
    cell.accessibilityIdentifier=[@"honggo.appearance.setting." stringByAppendingString:key];
    if([kind isEqual:@"bool"]) {
        UISwitch *toggle=[UISwitch new];toggle.on=HGAppearanceFlag(key);toggle.accessibilityIdentifier=key;
        toggle.accessibilityLabel=row[@"title"];
        [toggle addTarget:self action:@selector(toggle:) forControlEvents:UIControlEventValueChanged];
        cell.accessoryView=toggle;cell.selectionStyle=UITableViewCellSelectionStyleNone;
    }else if([kind isEqual:@"number"]) {
        UISlider *slider=[[UISlider alloc] initWithFrame:CGRectMake(0,0,125,44)];
        slider.minimumValue=[row[@"min"] floatValue];slider.maximumValue=[row[@"max"] floatValue];
        slider.value=HGAppearanceNumber(key);slider.accessibilityIdentifier=key;slider.accessibilityLabel=row[@"title"];
        [slider addTarget:self action:@selector(slider:) forControlEvents:UIControlEventValueChanged];
        cell.accessoryView=slider;cell.detailTextLabel.text=[NSString stringWithFormat:@"%.2f（%@～%@）",slider.value,row[@"min"],row[@"max"]];
        cell.selectionStyle=UITableViewCellSelectionStyleNone;
    }else {
        cell.accessoryType=UITableViewCellAccessoryDisclosureIndicator;
        NSString *value=HGAppearanceString(key);
        if([kind isEqual:@"choice"]) {
            NSUInteger i=[row[@"choices"] indexOfObject:value];cell.detailTextLabel.text=i!=NSNotFound?row[@"labels"][i]:@"默认";
        }else if([kind isEqual:@"image"] || [kind isEqual:@"fontfile"])cell.detailTextLabel.text=value.length?@"已选择本地资源，点击可更换或清除":@"未选择";
        else if(![kind isEqual:@"action"])cell.detailTextLabel.text=value.length?value:@"跟随系统／预设";
        if([key isEqual:@"@reset"])cell.textLabel.textColor=UIColor.systemRedColor;
    }return cell;
}
- (void)message:(NSString *)title detail:(NSString *)detail {
    if(!self.viewIfLoaded.window || self.presentedViewController)return;
    UIAlertController *alert=[UIAlertController alertControllerWithTitle:title message:detail preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"好" style:UIAlertActionStyleDefault handler:nil]];
    [self presentViewController:alert animated:YES completion:nil];
}
- (void)save:(NSString *)key value:(id)value {
    NSError *error=nil;
    if(!HGAppearanceSet(key,value,&error))[self message:@"设置未更改" detail:error.localizedDescription];
}
- (void)toggle:(UISwitch *)toggle {
    NSString *key=toggle.accessibilityIdentifier;[self save:key value:@(toggle.on)];toggle.on=HGAppearanceFlag(key);
}
- (void)slider:(UISlider *)slider {
    NSString *key=slider.accessibilityIdentifier;
    double value=slider.maximumValue<=2?round(slider.value*100)/100:round(slider.value);
    [self save:key value:@(value)];slider.value=HGAppearanceNumber(key);
    UIView *node=slider;while(node && ![node isKindOfClass:UITableViewCell.class])node=node.superview;
    ((UITableViewCell *)node).detailTextLabel.text=[NSString stringWithFormat:@"%.2f",value];
}
- (void)presentSheet:(UIAlertController *)sheet {
    sheet.popoverPresentationController.sourceView=self.view;
    sheet.popoverPresentationController.sourceRect=CGRectMake(CGRectGetMidX(self.view.bounds),CGRectGetMidY(self.view.bounds),1,1);
    [self presentViewController:sheet animated:YES completion:nil];
}
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)path {
    [tableView deselectRowAtIndexPath:path animated:YES];
    NSDictionary *row=self.rows[path.row];
    [self afterSearch:^{[self openRow:row];}];
}
- (void)confirm:(NSString *)title detail:(NSString *)detail action:(dispatch_block_t)action {
    UIAlertController *a=[UIAlertController alertControllerWithTitle:title message:detail preferredStyle:UIAlertControllerStyleAlert];
    [a addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:nil]];
    [a addAction:[UIAlertAction actionWithTitle:@"应用" style:UIAlertActionStyleDestructive handler:^(UIAlertAction *button){action();[self reloadRows];}]];
    [self presentViewController:a animated:YES completion:nil];
}
- (void)openRow:(NSDictionary *)row {
    if(self.busy)return;
    NSString *key=row[@"key"],*kind=row[@"kind"];
    if([key hasPrefix:@"@group."]){HGOpenAppearance(self,[key substringFromIndex:7]);return;}
    if([key hasPrefix:@"@preset."]) {
        NSString *preset=[key substringFromIndex:8];
        [self confirm:row[@"title"] detail:@"会重置本页外观参数并开启这套预设，不改变去广告开关。首次开启原生玻璃后请重启红果。"
            action:^{HGAppearanceApply(HGAppearancePreset(preset),nil);}];return;
    }
    if([key isEqual:@"@reset"]){[self confirm:@"恢复外观默认值？" detail:@"关闭新外观定制并恢复宿主控件。原有去广告设置不变。" action:^{HGAppearanceReset();}];return;}
    if([key isEqual:@"@fonts.reset"]){HGAppearanceApply(@{@"fonts.enabled":@NO,@"fonts.name":@"",@"fonts.asset":@"",@"fonts.scale":@1},nil);[self reloadRows];return;}
    if([key isEqual:@"@export"]){[self exportConfiguration];return;}
    if([key isEqual:@"@import"]){[self pickDocument:@"configuration"];return;}
    if([kind isEqual:@"choice"]) {
        UIAlertController *a=[UIAlertController alertControllerWithTitle:row[@"title"] message:nil preferredStyle:UIAlertControllerStyleActionSheet];
        NSArray *values=row[@"choices"],*labels=row[@"labels"];
        for(NSUInteger i=0;i<values.count;i++)[a addAction:[UIAlertAction actionWithTitle:labels[i] style:UIAlertActionStyleDefault handler:^(UIAlertAction *button){[self save:key value:values[i]];[self reloadRows];}]];
        [a addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:nil]];[self presentSheet:a];return;
    }
    if([kind isEqual:@"symbol"]) {
        UIAlertController *a=[UIAlertController alertControllerWithTitle:row[@"title"] message:@"输入系统图标名，如 heart、star.fill。留空恢复预设。" preferredStyle:UIAlertControllerStyleAlert];
        [a addTextFieldWithConfigurationHandler:^(UITextField *text){text.text=HGAppearanceString(key);text.autocorrectionType=UITextAutocorrectionTypeNo;text.autocapitalizationType=UITextAutocapitalizationTypeNone;}];
        [a addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:nil]];
        [a addAction:[UIAlertAction actionWithTitle:@"保存" style:UIAlertActionStyleDefault handler:^(UIAlertAction *button){
            NSString *value=[a.textFields.firstObject.text ?: @"" stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
            if(value.length && ![UIImage systemImageNamed:value]) {dispatch_async(dispatch_get_main_queue(),^{[self message:@"图标不存在" detail:@"此系统没有该 SF Symbol，请检查名称。"];});return;}
            [self save:key value:value];[self reloadRows];
        }]];[self presentViewController:a animated:YES completion:nil];return;
    }
    if([kind isEqual:@"color"] || [kind isEqual:@"image"] || [kind isEqual:@"font"] || [kind isEqual:@"fontfile"]) {
        UIAlertController *a=[UIAlertController alertControllerWithTitle:row[@"title"] message:nil preferredStyle:UIAlertControllerStyleActionSheet];
        [a addAction:[UIAlertAction actionWithTitle:@"选择…" style:UIAlertActionStyleDefault handler:^(UIAlertAction *button){
            // Wait for the action sheet's dismissal; presenting on the next
            // run-loop tick is too early on devices with animated transitions.
            [a dismissViewControllerAnimated:YES completion:^{[self launchPicker:row];}];
        }]];
        [a addAction:[UIAlertAction actionWithTitle:@"恢复默认／清除" style:UIAlertActionStyleDestructive handler:^(UIAlertAction *button){
            if([kind isEqual:@"fontfile"])HGAppearanceApply(@{@"fonts.asset":@"",@"fonts.name":@""},nil);
            else [self save:key value:@""];[self reloadRows];
        }]];
        [a addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:nil]];[self presentSheet:a];
    }
}
- (void)launchPicker:(NSDictionary *)row {
    if(self.presentedViewController || !self.viewIfLoaded.window)return;
    NSString *key=row[@"key"],*kind=row[@"kind"];
    if([kind isEqual:@"color"]) {
        self.colorKey=key;UIColorPickerViewController *picker=[UIColorPickerViewController new];
        picker.delegate=self;picker.supportsAlpha=NO;picker.selectedColor=HGAppearanceColor(HGAppearanceString(key),UIColor.labelColor);
        [self presentViewController:picker animated:YES completion:nil];
    }else if([kind isEqual:@"image"]) {
        self.assetKey=key;PHPickerConfiguration *configuration=[PHPickerConfiguration new];
        configuration.filter=PHPickerFilter.imagesFilter;configuration.selectionLimit=1;
        PHPickerViewController *picker=[[PHPickerViewController alloc] initWithConfiguration:configuration];picker.delegate=self;
        [self presentViewController:picker animated:YES completion:nil];
    }else if([kind isEqual:@"font"]) {
        UIFontPickerViewControllerConfiguration *configuration=[UIFontPickerViewControllerConfiguration new];
        configuration.includeFaces=YES;
        UIFontPickerViewController *picker=[[UIFontPickerViewController alloc] initWithConfiguration:configuration];picker.delegate=self;
        [self presentViewController:picker animated:YES completion:nil];
    }else [self pickDocument:@"font"];
}
- (void)colorPickerViewController:(UIColorPickerViewController *)controller didSelectColor:(UIColor *)color continuously:(BOOL)continuously {
    CGFloat r=0,g=0,b=0,a=0;
    if([[color resolvedColorWithTraitCollection:self.traitCollection] getRed:&r green:&g blue:&b alpha:&a]) {
        NSString *hex=[NSString stringWithFormat:@"#%02X%02X%02X",(unsigned)lround(r*255),(unsigned)lround(g*255),(unsigned)lround(b*255)];
        [self save:self.colorKey value:hex];
    }
}
- (void)colorPickerViewControllerDidFinish:(UIColorPickerViewController *)controller {[self reloadRows];}
- (void)fontPickerViewControllerDidPickFont:(UIFontPickerViewController *)controller {
    NSString *name=[controller.selectedFontDescriptor objectForKey:UIFontDescriptorNameAttribute];
    [controller dismissViewControllerAnimated:YES completion:^{
        if(name.length && [UIFont fontWithName:name size:16])HGAppearanceApply(@{@"fonts.name":name,@"fonts.asset":@""},nil);
        [self reloadRows];
    }];
}
- (void)fontPickerViewControllerDidCancel:(UIFontPickerViewController *)controller {[controller dismissViewControllerAnimated:YES completion:nil];}
- (void)picker:(PHPickerViewController *)picker didFinishPicking:(NSArray<PHPickerResult *> *)results {
    NSString *key=self.assetKey;self.assetKey=nil;
    [picker dismissViewControllerAnimated:YES completion:^{
        NSItemProvider *provider=results.firstObject.itemProvider;if(!provider)return;
        self.busy=YES;
        [provider loadFileRepresentationForTypeIdentifier:UTTypeImage.identifier completionHandler:^(NSURL *url,NSError *failure) {
            NSError *error=failure;
            NSData *data=url?HGAReadFile(url,12*1024*1024,&error):nil;
            NSString *token=data?HGAImportImage(data,&error):nil;
            dispatch_async(dispatch_get_main_queue(),^{
                self.busy=NO;
                if(token){[self save:key value:token];[self reloadRows];}
                else [self message:@"图片未导入" detail:error.localizedDescription ?: @"无法读取所选图片。"];
            });
        }];
    }];
}
- (void)pickDocument:(NSString *)purpose {
    self.documentPurpose=purpose;
    NSMutableArray<UTType *> *types=[NSMutableArray array];
    if([purpose isEqual:@"configuration"])[types addObject:UTTypeJSON];
    else for(NSString *extension in @[@"ttf",@"otf",@"ttc"]) {
        UTType *type=[UTType typeWithFilenameExtension:extension];if(type)[types addObject:type];
    }
    if(!types.count)[types addObject:UTTypeData];
    UIDocumentPickerViewController *picker=[[UIDocumentPickerViewController alloc] initForOpeningContentTypes:types asCopy:YES];
    picker.delegate=self;picker.allowsMultipleSelection=NO;[self presentViewController:picker animated:YES completion:nil];
}
- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls {
    NSURL *url=urls.firstObject;NSString *purpose=self.documentPurpose;self.documentPurpose=nil;if(!url)return;
    self.busy=YES;
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED,0),^{
        BOOL scope=[url startAccessingSecurityScopedResource];
        __block NSData *data=nil;__block NSError *readError=nil;NSError *coordinationError=nil;
        NSFileCoordinator *coordinator=[[NSFileCoordinator alloc] initWithFilePresenter:nil];
        [coordinator coordinateReadingItemAtURL:url options:0 error:&coordinationError byAccessor:^(NSURL *local){
            data=HGAReadFile(local,[purpose isEqual:@"font"]?50*1024*1024:128*1024,&readError);
        }];
        if(scope)[url stopAccessingSecurityScopedResource];
        NSError *error=coordinationError ?: readError;NSDictionary *configuration=nil;NSArray *fonts=nil;NSURL *stored=nil;
        if(data && !error && [purpose isEqual:@"font"]) {
            NSString *extension=url.pathExtension.lowercaseString;
            if(![@[@"ttf",@"otf",@"ttc"] containsObject:extension])error=HGAAssetError(@"仅支持 TTF、OTF 或 TTC 字体。");
            else {stored=HGAStoreAsset(data,extension,&error);if(stored)fonts=HGARegisterFont(stored,&error);}
        }else if(data && !error)configuration=HGAppearanceParse(data,&error);
        dispatch_async(dispatch_get_main_queue(),^{
            self.busy=NO;
            // The native picker dismisses itself; defer UI until that transition finishes.
            void (^finish)(void)=^{
                if(error){[self message:@"导入失败" detail:error.localizedDescription];return;}
                if(fonts.count){
                    UIAlertController *choice=[UIAlertController alertControllerWithTitle:@"选择字体" message:@"导入完成；选择字形后再开启字体功能。" preferredStyle:UIAlertControllerStyleActionSheet];
                    for(NSString *name in fonts)[choice addAction:[UIAlertAction actionWithTitle:name style:UIAlertActionStyleDefault handler:^(UIAlertAction *a){
                        HGAppearanceApply(@{@"fonts.asset":stored.lastPathComponent,@"fonts.name":name},nil);[self reloadRows];
                    }]];
                    [choice addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:nil]];[self presentSheet:choice];
                }else if(configuration){[self confirm:@"恢复外观参数？" detail:@"只修改文件中列出的 Honggo 外观设置。字体与图片需要在本机存在。" action:^{
                    NSError *applyError=nil;
                    if(!HGAppearanceApply(configuration,&applyError))dispatch_async(dispatch_get_main_queue(),^{[self message:@"未恢复" detail:applyError.localizedDescription];});
                }];}
            };
            if(self.presentedViewController==controller)[controller dismissViewControllerAnimated:YES completion:finish];
            else if(controller.transitionCoordinator)[controller.transitionCoordinator animateAlongsideTransition:nil completion:^(id<UIViewControllerTransitionCoordinatorContext> context){finish();}];
            else finish();
        });
    });
}
- (void)documentPickerWasCancelled:(UIDocumentPickerViewController *)controller{self.documentPurpose=nil;}
- (void)exportConfiguration {
    NSError *error=nil;NSData *data=HGAppearanceExport(&error);
    if(!data){[self message:@"导出失败" detail:error.localizedDescription];return;}
    self.busy=YES;
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY,0),^{
        NSURL *directory=[NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingPathComponent:NSUUID.UUID.UUIDString] isDirectory:YES];
        NSError *failure=nil;
        BOOL ok=[NSFileManager.defaultManager createDirectoryAtURL:directory withIntermediateDirectories:YES attributes:nil error:&failure];
        NSURL *url=[directory URLByAppendingPathComponent:@"Honggo-appearance.json"];
        if(ok)ok=[data writeToURL:url options:NSDataWritingAtomic error:&failure];
        dispatch_async(dispatch_get_main_queue(),^{
            self.busy=NO;
            if(!ok){[self message:@"导出失败" detail:failure.localizedDescription];return;}
            UIActivityViewController *share=[[UIActivityViewController alloc] initWithActivityItems:@[url] applicationActivities:nil];
            share.popoverPresentationController.sourceView=self.view;
            share.popoverPresentationController.sourceRect=CGRectMake(CGRectGetMidX(self.view.bounds),CGRectGetMidY(self.view.bounds),1,1);
            share.completionWithItemsHandler=^(UIActivityType type,BOOL completed,NSArray *items,NSError *shareError){
                dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY,0),^{[NSFileManager.defaultManager removeItemAtURL:directory error:nil];});
            };
            if(self.viewIfLoaded.window && !self.presentedViewController)[self presentViewController:share animated:YES completion:nil];
        });
    });
}
@end
void HGOpenAppearance(UIViewController *presenter,NSString *group) {
    if(!NSThread.isMainThread || !presenter)return;
    UIViewController *current=presenter;
    for(NSUInteger i=0;i<16;i++) {
        if(current.presentedViewController && !current.presentedViewController.isBeingDismissed)current=current.presentedViewController;
        else if([current isKindOfClass:UINavigationController.class])current=[(UINavigationController *)current topViewController];
        else if([current isKindOfClass:UITabBarController.class])current=[(UITabBarController *)current selectedViewController];
        else break;
    }
    if(!current.viewIfLoaded.window || [current isKindOfClass:UIAlertController.class] || current.isBeingDismissed)return;
    if([current isKindOfClass:HGAAppearanceController.class] && !group)return;
    HGAAppearanceController *panel=[[HGAAppearanceController alloc] initWithStyle:UITableViewStyleInsetGrouped];panel.group=group;
    if([current isKindOfClass:HGSettingsController.class] || [current isKindOfClass:HGAAppearanceController.class])
        [current.navigationController pushViewController:panel animated:YES];
    else {
        UINavigationController *navigation=[[UINavigationController alloc] initWithRootViewController:panel];
        [current presentViewController:navigation animated:YES completion:nil];
    }
}
