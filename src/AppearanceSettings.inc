#import "SettingsTransition.h"
#import "Community.h"

@interface HGAAppearanceController : UITableViewController <UISearchResultsUpdating,UISearchControllerDelegate,UIDocumentPickerDelegate,UIFontPickerViewControllerDelegate>
@property(nonatomic,copy) NSString *group;
@property(nonatomic,copy) NSArray<NSArray<NSDictionary *> *> *sections;
@property(nonatomic,copy) NSArray<NSString *> *sectionTitles;
@property(nonatomic,strong) UISearchController *search;
@property(nonatomic,copy) dispatch_block_t pendingAction;
@property(nonatomic) BOOL busy;
@end

static NSDictionary *HGATool(NSString *key,NSString *title) {
    return @{@"key":key,@"title":title,@"kind":@"action"};
}

static NSDictionary *HGASubpage(NSString *key,NSString *title,NSString *detail) {
    return @{@"key":key,@"title":title,@"detail":detail ?: @"",@"kind":@"subpage"};
}

static NSDictionary *HGACatalogDef(NSString *key) {
    for(NSDictionary *def in HGAppearanceCatalog()) {
        if([def[@"key"] isEqual:key])return def;
    }
    return nil;
}

static NSInteger HGAPercentFromValue(NSDictionary *def,double raw) {
    double lo=[def[@"min"] doubleValue];
    double hi=[def[@"max"] doubleValue];
    double dflt=[def[@"default"] doubleValue];
    if(!isfinite(raw) || hi<=lo)return 50;
    double clamped=MAX(lo,MIN(hi,raw));
    double pct=50.0;
    if(dflt>lo+1e-6 && dflt<hi-1e-6) {
        if(clamped<=dflt)pct=1.0+((clamped-lo)/(dflt-lo))*49.0;
        else pct=50.0+((clamped-dflt)/(hi-dflt))*50.0;
    } else {
        pct=1.0+((clamped-lo)/(hi-lo))*99.0;
    }
    NSInteger rounded=(NSInteger)llround(pct);
    return MAX(1,MIN(100,rounded));
}

static double HGAValueFromPercent(NSDictionary *def,NSInteger pct) {
    double lo=[def[@"min"] doubleValue];
    double hi=[def[@"max"] doubleValue];
    double dflt=[def[@"default"] doubleValue];
    pct=MAX(1,MIN(100,pct));
    double raw=dflt;
    if(dflt>lo+1e-6 && dflt<hi-1e-6) {
        if(pct==50)return dflt;
        if(pct<50)raw=lo+((pct-1.0)/49.0)*(dflt-lo);
        else raw=dflt+((pct-50.0)/50.0)*(hi-dflt);
    } else {
        raw=lo+((pct-1.0)/99.0)*(hi-lo);
    }
    double rounded=(hi-lo<=5.0) ? round(raw*100.0)/100.0 : round(raw);
    return MAX(lo,MIN(hi,rounded));
}

static NSString *HGAPercentDetailText(NSDictionary *def,NSInteger pct) {
    NSInteger defaultPct=HGAPercentFromValue(def,[def[@"default"] doubleValue]);
    if(pct==defaultPct)return [NSString stringWithFormat:@"%ld%%（默认）",(long)pct];
    return [NSString stringWithFormat:@"%ld%%",(long)pct];
}

static NSString *HGARoleSummary(NSString *role) {
    NSString *prefix=[@"layout." stringByAppendingString:role];
    BOOL hidden=HGAppearanceFlag([prefix stringByAppendingString:@".hidden"]);
    double x=HGAppearanceNumber([prefix stringByAppendingString:@".x"]);
    double y=HGAppearanceNumber([prefix stringByAppendingString:@".y"]);
    double scale=HGAppearanceNumber([prefix stringByAppendingString:@".scale"]);
    BOOL adjusted=(fabs(x)>0.01 || fabs(y)>0.01 || fabs(scale-1.0)>0.005);
    if(hidden && adjusted)return @"已隐藏 · 已调整位置与大小";
    if(hidden)return @"已隐藏";
    if(adjusted)return @"已自定义偏移／大小";
    return @"默认";
}

static NSString *HGAGroupDisplayTitle(NSString *group) {
    if(!group.length)return @"外观";
    if([group hasPrefix:@"setting."])return HGAGroupDisplayTitle(HGACatalogDef([group substringFromIndex:8])[@"group"]);
    NSDictionary<NSString *,NSString *> *titles=@{
        @"general":@"外观总开关",
        @"dock":@"悬浮底栏",
        @"dock.geometry":@"底栏尺寸与圆角",
        @"dock.extra":@"独立圆形按钮",
        @"dock.collapse":@"收缩与手势",
        @"dock.hide":@"底栏导航项隐藏",
        @"chrome":@"顶部导航与找剧",
        @"mine":@"我的页面",
        @"gestures":@"播放手势",
        @"chrome.hide":@"顶部按钮与频道隐藏",
        @"chrome.finder.hide":@"找剧频道与快捷入口",
        @"chrome.mine.hide":@"我的快捷入口",
        @"player":@"播放布局与控件",
        @"panels":@"弹出面板",
        @"fonts":@"字体与字号"
    };
    if(titles[group])return titles[group];
    if([group hasPrefix:@"role."]) {
        NSString *role=[group substringFromIndex:5];
        NSUInteger idx=[HGAppearanceRoles() indexOfObject:role];
        if(idx!=NSNotFound)return HGAppearanceRoleTitles()[idx];
    }
    return @"外观";
}

static void HGAAddDefs(NSMutableArray *rows,NSArray<NSString *> *keys) {
    for(NSString *key in keys) {
        NSDictionary *def=HGACatalogDef(key);
        if(def)[rows addObject:def];
    }
}

@implementation HGAAppearanceController
- (void)viewDidLoad {
    [super viewDidLoad];
    self.title=HGAGroupDisplayTitle(self.group);
    self.tableView.accessibilityIdentifier=@"honggo.appearance.settings";
    self.tableView.rowHeight=UITableViewAutomaticDimension;self.tableView.estimatedRowHeight=62;
    // Only attach a UISearchController when this controller is standalone/root
    // of its own navigation stack. Stacking a second UISearchController on pushed
    // Level 2/3/4 pages causes a UIKit pop transition freeze on iOS 26/27.
    BOOL ownsSearch=!self.group.length &&
        (!self.navigationController || self.navigationController.viewControllers.firstObject==self);
    if(ownsSearch) {
        self.search=[[UISearchController alloc] initWithSearchResultsController:nil];
        self.search.searchResultsUpdater=self;self.search.delegate=self;
        self.search.obscuresBackgroundDuringPresentation=NO;
        self.search.searchBar.placeholder=@"搜索外观、底栏、图标、字体或控件";
        self.navigationItem.searchController=self.search;self.navigationItem.hidesSearchBarWhenScrolling=NO;
        self.definesPresentationContext=YES;
    }
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
    UISearchController *search = self.search;
    if(search && (search.active || search.isBeingDismissed || self.presentedViewController == search)) {
        self.pendingAction=action;
        if(!self.search.isBeingDismissed) HGAfterViewTransition(self.search, ^{
            if(self.pendingAction)self.search.active=NO;
        });
    } else {
        dispatch_block_t perform = ^{
            UINavigationController *navigation = self.navigationController;
            if (self.viewIfLoaded.window && !self.presentedViewController && !self.isBeingDismissed &&
                    !navigation.isBeingDismissed && (!navigation || navigation.topViewController == self)) action();
        };
        if (self.transitionCoordinator.isAnimated) HGAfterViewTransition(self, perform);
        else perform();
    }
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
    __block NSMutableArray<NSDictionary *> *rows = [NSMutableArray array];
    NSMutableArray *sections = [NSMutableArray arrayWithObject:rows];
    NSMutableArray *titles = [NSMutableArray arrayWithObject:@""];
    void (^section)(NSString *) = ^(NSString *title) {
        if (rows.count) {
            rows = [NSMutableArray array];
            [sections addObject:rows];
            [titles addObject:title];
        } else {
            titles[titles.count - 1] = title;
        }
    };
    if(query.length) {
        [rows addObjectsFromArray:HGAppearanceSearch(query)];
        if(!self.group && HGCommunityMatchesQuery(query))
            [rows addObject:HGATool(@"@community",@"TG 更新频道 · @Naloiafeed")];
    } else if(!self.group.length) {
        HGAAddDefs(rows, @[@"enabled"]);
        section(@"界面与播放");
        [rows addObject:HGASubpage(@"@group.dock",@"悬浮底栏",HGAppearanceFlag(@"dock.enabled")?@"已开启":@"默认")];
        [rows addObject:HGASubpage(@"@group.chrome",@"顶部导航与找剧",@"频道、搜索与快捷入口")];
        [rows addObject:HGASubpage(@"@group.mine",@"我的页面",@"订单、消息、游戏、追更与更多功能")];
        [rows addObject:HGASubpage(@"@group.player",@"播放布局与控件",@"播放方式、控件位置与大小")];
        [rows addObject:HGASubpage(@"@group.gestures",@"播放手势",@"双指清屏与双击评论")];
        [rows addObject:HGASubpage(@"@group.panels",@"弹出面板",@"评论、选集、分享与倍速面板")];
        section(@"文字");
        [rows addObject:HGASubpage(@"@group.fonts",@"字体与字号",HGAppearanceFlag(@"fonts.enabled")?@"已开启":@"默认")];
    } else if([self.group hasPrefix:@"setting."]) {
        HGAAddDefs(rows,@[[self.group substringFromIndex:8]]);
    } else if([self.group isEqual:@"general"]) {
        HGAAddDefs(rows,@[@"enabled"]);
    } else if([self.group isEqual:@"dock"]) {
        HGAAddDefs(rows,@[@"dock.enabled",@"dock.style"]);
        if(!HGFlatDockStyle())HGAAddDefs(rows,@[@"dock.material"]);
        HGAAddDefs(rows,@[@"dock.content",@"dock.haptics"]);
        section(@"布局与交互");
        [rows addObject:HGASubpage(@"@group.dock.extra",@"独立圆形按钮",HGAppearanceFlag(@"dock.extra")?@"已开启":@"已关闭")];
        [rows addObject:HGASubpage(@"@group.dock.geometry",@"底栏尺寸与圆角",@"宽度、高度、圆角与图标大小（1–100%）")];
        [rows addObject:HGASubpage(@"@group.dock.collapse",@"收缩与手势",HGAppearanceFlag(@"dock.collapse")?@"已开启":@"已关闭")];
        [rows addObject:HGASubpage(@"@group.dock.hide",@"底栏导航项隐藏",@"单独隐藏首页、剧场、商城或我的")];
    } else if([self.group isEqual:@"dock.geometry"]) {
        HGAAddDefs(rows,@[@"dock.width",@"dock.height",@"dock.lift",@"dock.radius",@"dock.iconSize"]);
    } else if([self.group isEqual:@"dock.extra"]) {
        HGAAddDefs(rows,@[@"dock.extra",@"dock.action",@"dock.extraSize"]);
    } else if([self.group isEqual:@"dock.collapse"]) {
        HGAAddDefs(rows,@[@"dock.collapse",@"dock.autoCollapse",@"dock.anchor"]);
    } else if([self.group isEqual:@"dock.hide"]) {
        HGAAddDefs(rows,@[@"dock.hide.home",@"dock.hide.theater",@"dock.hide.store",@"dock.hide.mine"]);
    } else if([self.group isEqual:@"chrome"]) {
        section(@"顶部导航");
        HGAAddDefs(rows,@[@"player.headerGlass",@"player.headerStyle"]);
        [rows addObject:HGASubpage(@"@group.chrome.hide",@"顶部按钮与频道隐藏",@"菜单、频道与搜索")];
        section(@"找剧");
        HGAAddDefs(rows,@[@"player.finderGlass",@"player.finderAI"]);
        [rows addObject:HGASubpage(@"@group.chrome.finder.hide",@"找剧频道与快捷入口",@"逐项隐藏频道、筛选等入口")];
    } else if([self.group isEqual:@"mine"] || [self.group isEqual:@"chrome.mine.hide"]) {
        HGAAddDefs(rows,@[@"player.mineGlass",@"mine.show.publish",@"mine.show.theme"]);
        section(@"我的快捷入口");
        HGAAddDefs(rows,@[@"mine.hide.shortcut.row",@"mine.hide.shortcut.store",@"mine.hide.shortcut.messages",
            @"mine.hide.shortcut.games",@"mine.hide.shortcut.updates",@"mine.hide.shortcut.more"]);
    } else if([self.group isEqual:@"gestures"]) {
        HGAAddDefs(rows,@[@"gestures.pinchClean",@"gestures.doubleTapComments"]);
    } else if([self.group isEqual:@"chrome.finder.hide"]) {
        NSArray *prefixes=@[@"finder.hide.category.",@"finder.hide.shortcut."];
        for(NSString *prefix in prefixes) {
            section([prefix hasSuffix:@"category."] ? @"频道" : @"快捷入口");
            NSMutableArray *keys=[NSMutableArray array];
            for(NSDictionary *definition in HGAppearanceCatalog()) {
                NSString *key=definition[@"key"];
                if([key hasPrefix:prefix])[keys addObject:key];
            }
            HGAAddDefs(rows,keys);
        }
    } else if([self.group isEqual:@"chrome.hide"]) {
        HGAAddDefs(rows,@[@"layout.channels.hide.sidebar",@"layout.channels.hide.follow",@"layout.channels.hide.comic",
            @"layout.channels.hide.live",@"layout.channels.hide.recommend",@"layout.channels.hide.search"]);
    } else if([self.group isEqual:@"player"]) {
        section(@"播放与材质");
        HGAAddDefs(rows,@[@"player.fullscreen",@"player.disableEpisodeSwipe",@"player.separateProgress",
            @"player.rightGlass",@"player.actionStyle",@"player.collectGlyph",@"player.material",@"player.episodeGlass",@"player.controlsGlass"]);
        section(@"剧集信息按钮");
        HGAAddDefs(rows,@[@"player.hide.followRank",@"player.hide.seriesBadge"]);
        NSArray *roles = HGAppearanceRoles(), *roleTitles = HGAppearanceRoleTitles();
        for (NSUInteger i = 0; i < roles.count; i++) {
            NSString *role = roles[i];
            if ([role isEqual:@"avatar"]) section(@"右侧按钮");
            if ([role isEqual:@"channels"]) section(@"频道与剧集信息");
            if ([role isEqual:@"progress"]) section(@"进度与播放入口");
            [rows addObject:HGASubpage([@"@group.role." stringByAppendingString:role], roleTitles[i], HGARoleSummary(role))];
        }
    } else if([self.group hasPrefix:@"role."]) {
        NSString *role=[self.group substringFromIndex:5];
        NSString *prefix=[@"layout." stringByAppendingString:role];
        HGAAddDefs(rows,@[[prefix stringByAppendingString:@".hidden"]]);
        section(@"位置与大小");
        HGAAddDefs(rows,@[
            [prefix stringByAppendingString:@".x"],
            [prefix stringByAppendingString:@".y"],
            [prefix stringByAppendingString:@".scale"]
        ]);
        section(@"");
        [rows addObject:HGATool([@"@role.reset." stringByAppendingString:role],@"恢复此项默认")];
    } else if([self.group isEqual:@"panels"]) {
        section(@"评论区");
        HGAAddDefs(rows,@[@"panels.comment",@"panels.commentMaterial",@"player.commentMovesVideo"]);
        section(@"启用玻璃的面板");
        HGAAddDefs(rows,@[@"panels.episodes",@"panels.share",@"panels.longpress",@"panels.speed"]);
        section(@"材质");
        HGAAddDefs(rows,@[@"panels.material",@"panels.opacity"]);
    } else if([self.group isEqual:@"fonts"]) {
        HGAAddDefs(rows,@[@"fonts.enabled",@"fonts.scale"]);
        section(@"字体来源");
        HGAAddDefs(rows,@[@"fonts.name",@"fonts.asset"]);
        section(@"");
        [rows addObject:HGATool(@"@fonts.reset",@"恢复宿主原字体与字号")];
    }
    self.sections = sections;
    self.sectionTitles = titles;
    UILabel *empty=nil;
    if(!rows.count){empty=[UILabel new];empty.text=@"没有匹配的外观设置";empty.textAlignment=NSTextAlignmentCenter;empty.textColor=UIColor.secondaryLabelColor;}
    self.tableView.backgroundView=empty;[self.tableView reloadData];
}
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView { return self.sections.count; }
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return self.sections[section].count;
}
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    NSString *title = self.sectionTitles[section];
    return title.length ? title : nil;
}
- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
    if (self.search.searchBar.text.length) return nil;
    if ((!self.group.length || [self.group isEqual:@"general"] || [self.group isEqual:@"setting.enabled"]) && section == 0)
        return @"首次启用原生玻璃后，请完全退出红果再打开。";
    if ([self.group isEqual:@"gestures"])return @"在短剧播放画面上使用。开启双击评论后，双击会打开当前短剧的评论区。";
    if ([self.group hasPrefix:@"role."] && section == 1)
        return @"50% 对应控件原来的位置和大小。";
    if ([self.group isEqual:@"fonts"] && section == 0)
        return @"字号适用于已适配的播放页、面板和底栏文字。";
    if ([self.group isEqual:@"dock.extra"])
        return @"选择底栏页面后，对应入口将移到独立按钮。";
    return nil;
}
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)path {
    NSDictionary *row=self.sections[path.section][path.row];NSString *kind=row[@"kind"],*key=row[@"key"];
    UITableViewCell *cell=[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:nil];
    NSString *title = row[@"title"];
    if ([self.group hasPrefix:@"role."]) {
        NSString *label = @{@"hidden": @"隐藏此控件", @"x": @"水平偏移", @"y": @"垂直偏移",
            @"scale": @"大小"}[key.pathExtension];
        if (label) title = label;
    }
    cell.textLabel.text=title;cell.textLabel.numberOfLines=0;cell.detailTextLabel.numberOfLines=0;
    cell.textLabel.font=[UIFont preferredFontForTextStyle:UIFontTextStyleBody];
    cell.detailTextLabel.font=[UIFont preferredFontForTextStyle:UIFontTextStyleCaption1];
    cell.textLabel.adjustsFontForContentSizeCategory=YES;cell.detailTextLabel.adjustsFontForContentSizeCategory=YES;
    cell.detailTextLabel.textColor=UIColor.secondaryLabelColor;
    cell.accessibilityIdentifier=[@"honggo.appearance.setting." stringByAppendingString:key];
    if([kind isEqual:@"bool"]) {
        UISwitch *toggle=[UISwitch new];toggle.on=HGAppearanceFlag(key);toggle.accessibilityIdentifier=key;
        toggle.accessibilityLabel=row[@"title"];
        [toggle addTarget:self action:@selector(toggle:) forControlEvents:UIControlEventValueChanged];
        cell.accessoryView=toggle;
        if([key isEqual:@"player.fullscreen"])
            cell.detailTextLabel.text=@"仅横屏播放横向视频时铺满，竖屏保持原比例";
        if([key isEqual:@"player.commentMovesVideo"])
            cell.detailTextLabel.text=@"关闭后，打开评论区时视频留在原位。";
        if([key isEqual:@"mine.show.theme"])
            cell.detailTextLabel.text=@"新版个人页在创建时读取此项，重新打开红果后生效。";
    }else if([kind isEqual:@"number"]) {
        UISlider *slider=[[UISlider alloc] initWithFrame:CGRectMake(0,0,136,44)];
        slider.minimumValue=1;slider.maximumValue=100;
        slider.continuous = NO; // Apply the appearance refresh once when the user releases the slider.
        NSInteger pct=HGAPercentFromValue(row,HGAppearanceNumber(key));
        slider.value=(float)pct;slider.accessibilityIdentifier=key;slider.accessibilityLabel=row[@"title"];
        [slider addTarget:self action:@selector(slider:) forControlEvents:UIControlEventValueChanged];
        cell.accessoryView=slider;
        cell.detailTextLabel.text=HGAPercentDetailText(row,pct);
        cell.selectionStyle=UITableViewCellSelectionStyleNone;
    }else if([kind isEqual:@"subpage"]) {
        cell.accessoryType=UITableViewCellAccessoryDisclosureIndicator;
        NSString *detail=row[@"detail"];
        if(detail.length && ![detail isEqual:@"默认"] && ![detail isEqual:@"跟随预设"])cell.detailTextLabel.text=detail;
    }else {
        cell.accessoryType=UITableViewCellAccessoryDisclosureIndicator;
        NSString *value=HGAppearanceString(key);
        if([kind isEqual:@"choice"]) {
            NSUInteger i=[row[@"choices"] indexOfObject:value];cell.detailTextLabel.text=i!=NSNotFound?row[@"labels"][i]:@"默认";
        }else if([kind isEqual:@"fontfile"])cell.detailTextLabel.text=value.length?@"已选择本地资源，点击可更换或清除":@"未选择";
        else if(![kind isEqual:@"action"])cell.detailTextLabel.text=value.length?value:@"跟随系统／预设";
        if([key hasPrefix:@"@role.reset."])cell.textLabel.textColor=UIColor.systemRedColor;
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
    NSString *area=@{@"dock.style":@"dock.enabled",@"player.headerStyle":@"player.headerGlass",
        @"player.actionStyle":@"player.rightGlass",
        @"panels.commentMaterial":@"panels.comment"}[key];
    if(area) {
        NSMutableDictionary *change=[@{area:@YES,key:value} mutableCopy];
        if([key isEqual:@"dock.style"] && [value isEqual:@"glass"] &&
            [@[@"blur",@"solid"] containsObject:HGAppearanceString(@"dock.material")])
            change[@"dock.material"]=@"regular";
        HGAppearanceApply(change,&error);
    } else HGAppearanceSet(key,value,&error);
    if(error)[self message:@"设置未更改" detail:error.localizedDescription];
}
- (void)toggle:(UISwitch *)toggle {
    NSString *key=toggle.accessibilityIdentifier;[self save:key value:@(toggle.on)];toggle.on=HGAppearanceFlag(key);
}
- (void)slider:(UISlider *)slider {
    NSString *key=slider.accessibilityIdentifier;
    NSDictionary *def=HGACatalogDef(key);
    if(!def)return;
    NSInteger pct=MAX(1,MIN(100,(NSInteger)llround(slider.value)));
    slider.value=(float)pct;
    double raw=HGAValueFromPercent(def,pct);
    [self save:key value:@(raw)];
    NSInteger actualPct=HGAPercentFromValue(def,HGAppearanceNumber(key));
    slider.value=(float)actualPct;
    UIView *node=slider;while(node && ![node isKindOfClass:UITableViewCell.class])node=node.superview;
    ((UITableViewCell *)node).detailTextLabel.text=HGAPercentDetailText(def,actualPct);
}
- (void)presentSheet:(UIAlertController *)sheet {
    sheet.popoverPresentationController.sourceView=self.view;
    sheet.popoverPresentationController.sourceRect=CGRectMake(CGRectGetMidX(self.view.bounds),CGRectGetMidY(self.view.bounds),1,1);
    [self presentViewController:sheet animated:YES completion:nil];
}
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)path {
    [tableView deselectRowAtIndexPath:path animated:YES];
    NSDictionary *row=self.sections[path.section][path.row];
    if ([row[@"kind"] isEqual:@"bool"]) {
        // Logical negation in Objective-C yields int; this preference requires CFBoolean.
        [self save:row[@"key"] value:HGAppearanceFlag(row[@"key"]) ? @NO : @YES];
        [tableView reloadRowsAtIndexPaths:@[path] withRowAnimation:UITableViewRowAnimationNone];
    } else if (![row[@"kind"] isEqual:@"number"]) {
        [self afterSearch:^{[self openRow:row];}];
    }
}
- (void)openRow:(NSDictionary *)row {
    if(self.busy)return;
    NSString *key=row[@"key"],*kind=row[@"kind"];
    if([key isEqual:@"@community"]){HGShowCommunity(self);return;}
    if([key hasPrefix:@"@group."]){HGOpenAppearance(self,[key substringFromIndex:7]);return;}
    if([key hasPrefix:@"@role.reset."]) {
        NSString *role=[key substringFromIndex:12];
        NSString *prefix=[@"layout." stringByAppendingString:role];
        HGAppearanceApply(@{
            [prefix stringByAppendingString:@".hidden"]:@NO,
            [prefix stringByAppendingString:@".x"]:@0,
            [prefix stringByAppendingString:@".y"]:@0,
            [prefix stringByAppendingString:@".scale"]:@1
        },nil);
        [self reloadRows];
        return;
    }
    if([key isEqual:@"@fonts.reset"]){HGAppearanceApply(@{@"fonts.enabled":@NO,@"fonts.name":@"",@"fonts.asset":@"",@"fonts.scale":@1},nil);[self reloadRows];return;}
    if([kind isEqual:@"choice"]) {
        UIAlertController *a=[UIAlertController alertControllerWithTitle:row[@"title"] message:nil preferredStyle:UIAlertControllerStyleActionSheet];
        NSArray *values=row[@"choices"],*labels=row[@"labels"];
        for(NSUInteger i=0;i<values.count;i++)[a addAction:[UIAlertAction actionWithTitle:labels[i] style:UIAlertActionStyleDefault handler:^(UIAlertAction *button){[self save:key value:values[i]];[self reloadRows];}]];
        [a addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:nil]];[self presentSheet:a];return;
    }
    if([kind isEqual:@"font"] || [kind isEqual:@"fontfile"]) {
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
    NSString *kind=row[@"kind"];
    if([kind isEqual:@"font"]) {
        UIFontPickerViewControllerConfiguration *configuration=[UIFontPickerViewControllerConfiguration new];
        configuration.includeFaces=YES;
        UIFontPickerViewController *picker=[[UIFontPickerViewController alloc] initWithConfiguration:configuration];picker.delegate=self;
        [self presentViewController:picker animated:YES completion:nil];
    }else [self pickFontDocument];
}
- (void)fontPickerViewControllerDidPickFont:(UIFontPickerViewController *)controller {
    NSString *name=[controller.selectedFontDescriptor objectForKey:UIFontDescriptorNameAttribute];
    [controller dismissViewControllerAnimated:YES completion:^{
        if(name.length && [UIFont fontWithName:name size:16])HGAppearanceApply(@{@"fonts.name":name,@"fonts.asset":@""},nil);
        [self reloadRows];
    }];
}
- (void)fontPickerViewControllerDidCancel:(UIFontPickerViewController *)controller {[controller dismissViewControllerAnimated:YES completion:nil];}
- (void)pickFontDocument {
    NSMutableArray<UTType *> *types=[NSMutableArray array];
    for(NSString *extension in @[@"ttf",@"otf",@"ttc"]) {
        UTType *type=[UTType typeWithFilenameExtension:extension];if(type)[types addObject:type];
    }
    if(!types.count)[types addObject:UTTypeData];
    UIDocumentPickerViewController *picker=[[UIDocumentPickerViewController alloc] initForOpeningContentTypes:types asCopy:YES];
    picker.delegate=self;picker.allowsMultipleSelection=NO;[self presentViewController:picker animated:YES completion:nil];
}
- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls {
    NSURL *url=urls.firstObject;if(!url)return;
    self.busy=YES;
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED,0),^{
        BOOL scope=[url startAccessingSecurityScopedResource];
        __block NSData *data=nil;__block NSError *readError=nil;NSError *coordinationError=nil;
        NSFileCoordinator *coordinator=[[NSFileCoordinator alloc] initWithFilePresenter:nil];
        [coordinator coordinateReadingItemAtURL:url options:0 error:&coordinationError byAccessor:^(NSURL *local){
            data=HGAReadFile(local,50*1024*1024,&readError);
        }];
        if(scope)[url stopAccessingSecurityScopedResource];
        NSError *error=coordinationError ?: readError;NSArray *fonts=nil;NSURL *stored=nil;
        if(data && !error) {
            NSString *extension=url.pathExtension.lowercaseString;
            if(![@[@"ttf",@"otf",@"ttc"] containsObject:extension])error=HGAAssetError(@"仅支持 TTF、OTF 或 TTC 字体。");
            else {stored=HGAStoreAsset(data,extension,&error);if(stored)fonts=HGARegisterFont(stored,&error);}
        }
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
                }
            };
            if(self.presentedViewController==controller)[controller dismissViewControllerAnimated:YES completion:finish];
            else if(controller.transitionCoordinator)[controller.transitionCoordinator animateAlongsideTransition:nil completion:^(id<UIViewControllerTransitionCoordinatorContext> context){finish();}];
            else finish();
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
    BOOL insideSettingsNav=[current isKindOfClass:HGSettingsController.class] ||
        [current isKindOfClass:HGAAppearanceController.class] ||
        [NSStringFromClass(current.class) isEqual:@"HGSettingsSubpageController"];
    if(insideSettingsNav && current.navigationController)
        [current.navigationController pushViewController:panel animated:YES];
    else {
        UINavigationController *navigation=[[UINavigationController alloc] initWithRootViewController:panel];
        [current presentViewController:navigation animated:YES completion:nil];
    }
}
