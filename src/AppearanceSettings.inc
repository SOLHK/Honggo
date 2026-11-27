#import "SettingsTransition.h"
#import "Community.h"

@interface HGAAppearanceController : UITableViewController <UISearchResultsUpdating,UISearchControllerDelegate,UIDocumentPickerDelegate,UIFontPickerViewControllerDelegate>
@property(nonatomic,copy) NSString *group;
@property(nonatomic,copy) NSArray<NSArray<NSDictionary *> *> *sections;
@property(nonatomic,copy) NSArray<NSString *> *sectionTitles;
@property(nonatomic,strong) UISearchController *search;
@property(nonatomic,copy) dispatch_block_t pendingAction;
@property(nonatomic) BOOL busy;
@property(nonatomic,copy) NSDictionary *initialChoice;
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
    return MAX(lo,MIN(hi,raw));
}

static NSString *HGAPercentDetailText(NSDictionary *def,NSInteger pct) {
    NSInteger defaultPct=HGAPercentFromValue(def,[def[@"default"] doubleValue]);
    if(pct==defaultPct)return [NSString stringWithFormat:@"%ld%%（默认）",(long)pct];
    return [NSString stringWithFormat:@"%ld%%",(long)pct];
}

@interface HGAPercentCell : UITableViewCell
@property(nonatomic,strong) UISlider *slider;
@property(nonatomic,strong) UIStepper *stepper;
@property(nonatomic,strong) UILabel *valueLabel;
@property(nonatomic,copy) NSDictionary *definition;
@property(nonatomic,copy) void (^savePercent)(NSInteger);
@property(nonatomic) BOOL adjusting;
- (instancetype)initWithDefinition:(NSDictionary *)definition title:(NSString *)title;
@end

@implementation HGAPercentCell
- (instancetype)initWithDefinition:(NSDictionary *)definition title:(NSString *)title {
    if ((self=[super initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil])) {
        _definition=definition;
        NSString *key=definition[@"key"];
        self.accessibilityIdentifier=[@"honggo.appearance.setting." stringByAppendingString:key];
        self.selectionStyle=UITableViewCellSelectionStyleNone;
        self.isAccessibilityElement=NO;
        UILabel *heading=[UILabel new];
        heading.font=[UIFont preferredFontForTextStyle:UIFontTextStyleBody];
        heading.adjustsFontForContentSizeCategory=YES;
        heading.text=title;
        heading.numberOfLines=0;
        _valueLabel=[UILabel new];
        _valueLabel.font=[UIFont preferredFontForTextStyle:UIFontTextStyleSubheadline];
        _valueLabel.adjustsFontForContentSizeCategory=YES;
        _valueLabel.textColor=UIColor.secondaryLabelColor;
        _valueLabel.numberOfLines=0;
        _valueLabel.isAccessibilityElement=NO;
        UIStackView *labels=[[UIStackView alloc] initWithArrangedSubviews:@[heading,_valueLabel]];
        labels.axis=UILayoutConstraintAxisVertical;
        _stepper=[UIStepper new];
        _stepper.minimumValue=1;
        _stepper.maximumValue=100;
        _stepper.stepValue=1;
        _stepper.accessibilityLabel=[title stringByAppendingString:@"，微调"];
        [_stepper addTarget:self action:@selector(step:) forControlEvents:UIControlEventValueChanged];
        UIStackView *top=[[UIStackView alloc] initWithArrangedSubviews:@[labels,_stepper]];
        top.alignment=UIStackViewAlignmentCenter;
        top.spacing=8;
        _slider=[UISlider new];
        _slider.minimumValue=1;
        _slider.maximumValue=100;
        _slider.continuous=YES;
        _slider.accessibilityIdentifier=key;
        _slider.accessibilityLabel=title;
        [_slider.heightAnchor constraintGreaterThanOrEqualToConstant:44].active=YES;
        [_slider addTarget:self action:@selector(beginAdjustment) forControlEvents:UIControlEventTouchDown];
        [_slider addTarget:self action:@selector(preview:) forControlEvents:UIControlEventValueChanged];
        [_slider addTarget:self action:@selector(commit) forControlEvents:UIControlEventTouchUpInside|UIControlEventTouchUpOutside];
        [_slider addTarget:self action:@selector(reloadValue) forControlEvents:UIControlEventTouchCancel];
        UIStackView *body=[[UIStackView alloc] initWithArrangedSubviews:@[top,_slider]];
        body.axis=UILayoutConstraintAxisVertical;
        body.translatesAutoresizingMaskIntoConstraints=NO;
        [self.contentView addSubview:body];
        UILayoutGuide *margins=self.contentView.layoutMarginsGuide;
        [NSLayoutConstraint activateConstraints:@[
            [body.leadingAnchor constraintEqualToAnchor:margins.leadingAnchor],
            [body.trailingAnchor constraintEqualToAnchor:margins.trailingAnchor],
            [body.topAnchor constraintEqualToAnchor:margins.topAnchor],
            [body.bottomAnchor constraintEqualToAnchor:margins.bottomAnchor]
        ]];
        [self reloadValue];
    }
    return self;
}
- (void)displayPercent:(NSInteger)value {
    self.stepper.value=value;
    self.valueLabel.text=HGAPercentDetailText(self.definition,value);
    self.stepper.accessibilityValue=self.valueLabel.text;
    self.slider.accessibilityValue=self.valueLabel.text;
}
- (void)reloadValue {
    self.adjusting=NO;
    NSInteger value=HGAPercentFromValue(self.definition,HGAppearanceNumber(self.definition[@"key"]));
    self.slider.value=(float)value;
    [self displayPercent:value];
}
- (void)commit {
    NSInteger pct=lround(self.slider.value);
    if (self.savePercent && pct!=HGAPercentFromValue(self.definition,HGAppearanceNumber(self.definition[@"key"])))
        self.savePercent(pct);
    [self reloadValue];
}
- (void)beginAdjustment {
    self.adjusting=YES;
}
- (void)preview:(UISlider *)slider {
    [self displayPercent:lround(slider.value)];
    // VoiceOver and keyboard increments have no touch-up event.
    if (!self.adjusting && !slider.isTracking) [self commit];
}
- (void)step:(UIStepper *)stepper {
    self.slider.value=(float)stepper.value;
    [self commit];
}
@end

static NSArray<NSString *> *HGARightButtonRoles(void) {
    return @[@"avatar",@"collect",@"comment",@"like",@"share",@"more"];
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

static NSDictionary *HGARoleEntry(NSString *role) {
    NSString *title=HGAppearanceRoleTitles()[[HGAppearanceRoles() indexOfObject:role]];
    return HGASubpage([@"@group.role." stringByAppendingString:role],title,HGARoleSummary(role));
}

static NSString *HGAGroupDisplayTitle(NSString *group) {
    if(!group.length)return @"外观";
    if([group hasPrefix:@"setting."])return HGAGroupDisplayTitle(HGACatalogDef([group substringFromIndex:8])[@"group"]);
    NSDictionary<NSString *,NSString *> *titles=@{
        @"general":@"外观总开关",
        @"dock":@"底栏 Dock",
        @"dock.geometry":@"底栏尺寸与圆角",
        @"dock.extra":@"独立圆形按钮",
        @"dock.collapse":@"收缩与手势",
        @"dock.hide":@"底栏导航项显示",
        @"chrome":@"页面与导航",
        @"mine":@"我的页面",
        @"mine.settings":@"红果设置项",
        @"gestures":@"播放手势",
        @"chrome.hide":@"顶部按钮与频道显示",
        @"chrome.finder.hide":@"找剧频道与快捷入口",
        @"chrome.mine.hide":@"我的快捷入口",
        @"player":@"播放页",
        @"rightButtons":@"右侧按钮",
        @"player.visibility":@"画面控件显隐",
        @"drama":@"短剧",
        @"comments":@"评论区",
        @"panelMaterial":@"面板材质",
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

@interface HGVideoKeywordEditor : UIViewController
@property(nonatomic,strong) UITextView *editor;
@end

@implementation HGVideoKeywordEditor
- (void)viewDidLoad {
    [super viewDidLoad];
    self.title=@"屏蔽视频关键词";
    self.view.backgroundColor=UIColor.systemGroupedBackgroundColor;
    self.navigationItem.rightBarButtonItem=[[UIBarButtonItem alloc] initWithTitle:@"保存"
        style:UIBarButtonItemStyleDone target:self action:@selector(saveKeywords)];
    UILabel *hint=[UILabel new];
    hint.text=@"每行一个关键词。视频信息包含任意一个词时自动屏蔽。清空并保存即可关闭。";
    hint.numberOfLines=0;
    hint.font=[UIFont preferredFontForTextStyle:UIFontTextStyleFootnote];
    hint.adjustsFontForContentSizeCategory=YES;
    hint.textColor=UIColor.secondaryLabelColor;
    self.editor=[UITextView new];
    self.editor.accessibilityLabel=@"屏蔽关键词，每行一个";
    self.editor.accessibilityIdentifier=@"honggo.video.keywordEditor";
    self.editor.backgroundColor=UIColor.secondarySystemGroupedBackgroundColor;
    self.editor.font=[UIFont preferredFontForTextStyle:UIFontTextStyleBody];
    self.editor.adjustsFontForContentSizeCategory=YES;
    self.editor.textContainerInset=UIEdgeInsetsMake(12,8,12,8);
    self.editor.layer.cornerRadius=12;
    self.editor.text=[HGVideoBlockedKeywords() componentsJoinedByString:@"\n"];
    hint.translatesAutoresizingMaskIntoConstraints=NO;
    self.editor.translatesAutoresizingMaskIntoConstraints=NO;
    [self.view addSubview:hint];
    [self.view addSubview:self.editor];
    UILayoutGuide *margin=self.view.layoutMarginsGuide;
    [NSLayoutConstraint activateConstraints:@[
        [hint.topAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor constant:16],
        [hint.leadingAnchor constraintEqualToAnchor:margin.leadingAnchor],
        [hint.trailingAnchor constraintEqualToAnchor:margin.trailingAnchor],
        [self.editor.topAnchor constraintEqualToAnchor:hint.bottomAnchor constant:10],
        [self.editor.leadingAnchor constraintEqualToAnchor:margin.leadingAnchor],
        [self.editor.trailingAnchor constraintEqualToAnchor:margin.trailingAnchor],
        [self.editor.bottomAnchor constraintEqualToAnchor:self.view.keyboardLayoutGuide.topAnchor constant:-12]
    ]];
}
- (void)saveKeywords {
    NSArray *lines=[self.editor.text componentsSeparatedByCharactersInSet:NSCharacterSet.newlineCharacterSet];
    NSError *error=nil;
    NSMutableDictionary *payload=[@{@"video.blockedKeywords":lines} mutableCopy];
    BOOL hasNonEmpty=NO;
    for(NSString *line in lines) {
        if([line stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet].length){hasNonEmpty=YES;break;}
    }
    if(hasNonEmpty && !HGAppearanceEnabled())payload[@"enabled"]=@YES;
    if (HGAppearanceApply(payload,&error)) {
        if(HGAppearanceEnabled())HGPrepareNativeGlass();
        [self.navigationController popViewControllerAnimated:YES];
        return;
    }
    UIAlertController *alert=[UIAlertController alertControllerWithTitle:@"关键词未保存"
        message:error.localizedDescription preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"好" style:UIAlertActionStyleDefault handler:nil]];
    [self presentViewController:alert animated:YES completion:nil];
}
@end

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
- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    NSDictionary *choice=self.initialChoice;
    if (choice) {
        self.initialChoice=nil;
        [self afterSearch:^{[self openRow:choice];}];
    }
}
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
        NSString *collapseMode=HGAppearanceString(@"dock.collapseMode");
        for(NSDictionary *match in HGAppearanceSearch(query)) {
            NSString *matchKey=match[@"key"];
            if([matchKey isEqual:@"dock.collapse"])continue;
            if([matchKey isEqual:@"dock.siriaiHideLine"] && ![collapseMode isEqual:@"siriai"])continue;
            if(([matchKey isEqual:@"dock.autoCollapse"] || [matchKey isEqual:@"dock.anchor"]) && ![collapseMode isEqual:@"legacy"])continue;
            [rows addObject:match];
        }
        if(!self.group && HGCommunityMatchesQuery(query))
            [rows addObject:HGATool(@"@community",@"TG 更新频道 · @Naloiafeed")];
    } else if(!self.group.length) {
        HGAAddDefs(rows, @[@"enabled"]);
        section(@"界面与播放");
        [rows addObject:HGASubpage(@"@group.dock",@"底栏 Dock",HGAppearanceFlag(@"dock.enabled")?@"已开启":@"默认")];
        [rows addObject:HGASubpage(@"@group.chrome",@"页面与导航",@"频道、搜索与快捷入口")];
        [rows addObject:HGASubpage(@"@group.mine",@"我的页面",@"订单、消息、游戏、追更与更多功能")];
        [rows addObject:HGASubpage(@"@group.player",@"播放页",@"手势、控件位置与大小")];
        [rows addObject:HGASubpage(@"@group.drama",@"短剧",@"全集入口、选集与剧集信息")];
        [rows addObject:HGASubpage(@"@group.comments",@"评论区",@"面板、播放画面与打开方式")];
        [rows addObject:HGASubpage(@"@group.panelMaterial",@"面板材质",@"材质与底色不透明度")];
        section(@"文字");
        [rows addObject:HGASubpage(@"@group.fonts",@"字体与字号",HGAppearanceFlag(@"fonts.enabled")?@"已开启":@"默认")];
    } else if([self.group hasPrefix:@"setting."]) {
        HGAAddDefs(rows,@[[self.group substringFromIndex:8]]);
    } else if([self.group isEqual:@"general"]) {
        HGAAddDefs(rows,@[@"enabled"]);
    } else if([self.group isEqual:@"dock"]) {
        HGAAddDefs(rows,@[@"dock.enabled",@"dock.style",@"dock.selectionStyle"]);
        if(!HGFlatDockStyle())HGAAddDefs(rows,@[@"dock.material"]);
        HGAAddDefs(rows,@[@"dock.content",@"dock.haptics"]);
        section(@"布局与交互");
        [rows addObject:HGASubpage(@"@group.dock.extra",@"独立圆形按钮",HGAppearanceFlag(@"dock.extra")?@"已开启":@"已关闭")];
        [rows addObject:HGASubpage(@"@group.dock.geometry",@"底栏尺寸与圆角",@"宽度、高度、圆角与图标大小（1–100%）")];
        NSString *collapseMode=HGAppearanceString(@"dock.collapseMode");
        NSString *collapseDetail=@"已关闭";
        if([collapseMode isEqual:@"legacy"])collapseDetail=@"原有收缩模式";
        else if([collapseMode isEqual:@"tiktok"])collapseDetail=@"TikTok 交互";
        else if([collapseMode isEqual:@"siriai"])collapseDetail=@"Siri AI 交互";
        [rows addObject:HGASubpage(@"@group.dock.collapse",@"收缩与手势",collapseDetail)];
        [rows addObject:HGASubpage(@"@group.dock.hide",@"底栏导航项显示",@"选择要显示的首页、剧场、商城或我的入口")];
    } else if([self.group isEqual:@"dock.geometry"]) {
        HGAAddDefs(rows,@[@"dock.width",@"dock.height",@"dock.lift",@"dock.radius",@"dock.iconSize"]);
    } else if([self.group isEqual:@"dock.extra"]) {
        HGAAddDefs(rows,@[@"dock.extra",@"dock.action",@"dock.extraSize"]);
    } else if([self.group isEqual:@"dock.collapse"]) {
        NSString *collapseMode=HGAppearanceString(@"dock.collapseMode");
        HGAAddDefs(rows,@[@"dock.collapseMode"]);
        if([collapseMode isEqual:@"siriai"]) {
            HGAAddDefs(rows,@[@"dock.siriaiHideLine"]);
        } else if([collapseMode isEqual:@"legacy"]) {
            HGAAddDefs(rows,@[@"dock.autoCollapse",@"dock.anchor"]);
        }
    } else if([self.group isEqual:@"dock.hide"]) {
        HGAAddDefs(rows,@[@"dock.hide.home",@"dock.hide.theater",@"dock.hide.store",@"dock.hide.mine"]);
    } else if([self.group isEqual:@"chrome"]) {
        section(@"顶部导航");
        HGAAddDefs(rows,@[@"player.headerGlass",@"player.headerStyle",@"player.headerSelectionStyle",@"player.hideFollowRecommendUsers"]);
        [rows addObject:HGASubpage(@"@group.chrome.hide",@"顶部按钮与频道显示",@"菜单、频道与搜索")];
        section(@"找剧");
        HGAAddDefs(rows,@[@"player.finderGlass",@"player.finderSearch",@"player.finderRecognition",@"player.finderAI"]);
        [rows addObject:HGASubpage(@"@group.chrome.finder.hide",@"找剧频道与快捷入口",@"选择要显示的频道、筛选等入口")];
    } else if([self.group isEqual:@"mine"] || [self.group isEqual:@"chrome.mine.hide"]) {
        HGAAddDefs(rows,@[@"player.mineGlass",@"mine.show.publish",@"mine.show.theme"]);
        section(@"我的快捷入口");
        HGAAddDefs(rows,@[@"mine.hide.shortcut.row",@"mine.hide.shortcut.store",@"mine.hide.shortcut.messages",
            @"mine.hide.shortcut.games",@"mine.hide.shortcut.updates",@"mine.hide.shortcut.more"]);
        section(@"右侧菜单");
        HGAAddDefs(rows,[HGMineDrawerEntries() valueForKey:@"key"]);
        section(@"设置页");
        [rows addObject:HGASubpage(@"@group.mine.settings",@"红果设置项",@"选择在红果设置页显示的项目")];
    } else if([self.group isEqual:@"mine.settings"]) {
        section(@"显示以下项目");
        HGAAddDefs(rows,[HGHostSettingsEntries() valueForKey:@"key"]);
    } else if([self.group isEqual:@"gestures"]) {
        HGAAddDefs(rows,@[@"gestures.cleanWhilePlaying",@"gestures.pinchClean",@"gestures.doubleTapComments"]);
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
            @"layout.channels.hide.live",@"layout.channels.hide.recommend",@"layout.channels.hide.search",
            @"player.hideFollowRecommendUsers"]);
    } else if([self.group isEqual:@"player"]) {
        section(@"播放与手势");
        HGAAddDefs(rows,@[@"player.fullscreen",@"player.highestQuality",@"player.hideNowPlayingIsland",
            @"player.disableEpisodeSwipe",@"gestures.cleanWhilePlaying",@"gestures.pinchClean"]);
        section(@"播放控件");
        [rows addObject:HGASubpage(@"@group.player.visibility",@"画面控件显隐",@"顶部频道、全屏观看、右侧按钮与底部信息")];
        [rows addObject:HGASubpage(@"@group.rightButtons",@"右侧按钮",@"头像、收藏、评论、点赞、分享、更多")];
        HGAAddDefs(rows,@[@"player.rightGlass",@"player.actionStyle",@"player.collectGlyph",@"player.material",@"player.controlsGlass"]);
        section(@"位置与大小");
        for (NSString *role in @[@"channels",@"info",@"tags",@"progress",@"danmaku",@"notice"])
            [rows addObject:HGARoleEntry(role)];
    } else if([self.group isEqual:@"player.visibility"]) {
        section(@"顶部按钮与频道");
        HGAAddDefs(rows,@[@"layout.channels.hide.sidebar",@"layout.channels.hide.follow",@"layout.channels.hide.comic",
            @"layout.channels.hide.live",@"layout.channels.hide.recommend",@"layout.channels.hide.search"]);
        section(@"右侧操作");
        HGAAddDefs(rows,@[@"player.hideFeedAvatarPhoto",@"player.hideFeedFollow",@"layout.collect.hidden",
            @"player.hideFeedCollectCount",@"layout.comment.hidden",@"player.hideFeedCommentCount",
            @"layout.like.hidden",@"player.hideFeedLikeCount",@"layout.share.hidden",@"player.hideFeedShareCaption"]);
        section(@"播放器与底部信息");
        HGAAddDefs(rows,@[@"player.fullscreenEntry",@"player.hideFeedDanmakuToggle",@"player.hideFeedDanmakuPublish",
            @"player.hideFeedAuthor",@"player.hideFeedDescription",@"layout.episodes.hidden",@"layout.progress.hidden"]);
    } else if([self.group isEqual:@"rightButtons"]) {
        for (NSString *role in HGARightButtonRoles()) {
            section(HGAppearanceRoleTitles()[[HGAppearanceRoles() indexOfObject:role]]);
            for (NSString *setting in @[@"hidden",@"x",@"y",@"scale"]) {
                NSString *key=[NSString stringWithFormat:@"layout.%@.%@",role,setting];
                [rows addObject:HGACatalogDef(key)];
            }
            [rows addObject:HGATool([@"@role.copyRightSize." stringByAppendingString:role],@"将此大小应用到全部右侧按钮")];
            [rows addObject:HGATool([@"@role.reset." stringByAppendingString:role],@"恢复此项默认")];
        }
    } else if([self.group isEqual:@"drama"]) {
        section(@"内容过滤");
        HGAAddDefs(rows,@[@"video.blockedKeywords",@"player.hideFollowRecommendUsers"]);
        section(@"全集入口");
        HGAAddDefs(rows,@[@"player.homePreviewRepair",@"player.disableAutoEnterSeries"]);
        HGAAddDefs(rows,@[@"player.episodeGlass",@"player.episodeDockWidth",@"player.separateProgress"]);
        [rows addObject:HGARoleEntry(@"episodes")];
        section(@"剧集信息");
        HGAAddDefs(rows,@[@"player.hide.followRank",@"player.hide.seriesBadge",@"player.hideAIFictionNotice"]);
        section(@"播放面板");
        HGAAddDefs(rows,@[@"panels.episodes",@"panels.share",@"panels.shareMaterial",@"panels.longpress",@"panels.longpressMaterial",@"panels.speed"]);
    } else if([self.group isEqual:@"comments"]) {
        section(@"评论面板与交互");
        HGAAddDefs(rows,@[@"panels.comment",@"panels.commentMaterial",@"player.commentMovesVideo",@"gestures.doubleTapComments"]);
        section(@"评论输入框");
        HGAAddDefs(rows,@[@"panels.commentFloatingBar",@"panels.commentDetachedInput",@"panels.commentInputMaterial"]);
    } else if([self.group isEqual:@"panelMaterial"]) {
        HGAAddDefs(rows,@[@"panels.material",@"panels.longpressMaterial",@"panels.shareMaterial",@"panels.opacity"]);
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
        if ([HGARightButtonRoles() containsObject:role])
            [rows addObject:HGATool([@"@role.copyRightSize." stringByAppendingString:role],@"将此大小应用到全部右侧按钮")];
        [rows addObject:HGATool([@"@role.reset." stringByAppendingString:role],@"恢复此项默认")];
    } else if([self.group isEqual:@"panels"]) {
        section(@"评论区");
        HGAAddDefs(rows,@[@"panels.comment",@"panels.commentMaterial",@"player.commentMovesVideo"]);
        section(@"启用玻璃的面板");
        HGAAddDefs(rows,@[@"panels.episodes",@"panels.share",@"panels.shareMaterial",@"panels.longpress",@"panels.longpressMaterial",@"panels.speed"]);
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
        return @"此开关同时控制外观定制、液态底栏和原生控件玻璃。iOS 26 及以上首次启用或关闭系统玻璃后，需重开红果才能更新系统启动配置。";
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
    NSString *title = row[@"title"];
    if ([self.group hasPrefix:@"role."] || [self.group isEqual:@"rightButtons"]) {
        NSString *label = @{@"hidden": @"显示此控件", @"x": @"水平偏移", @"y": @"垂直偏移",
            @"scale": @"大小"}[key.pathExtension];
        if (label) title = label;
    }
    if ([kind isEqual:@"number"]) {
        HGAPercentCell *cell=[[HGAPercentCell alloc] initWithDefinition:row title:title];
        __weak typeof(self) weakSelf=self;
        cell.savePercent=^(NSInteger pct){ [weakSelf save:key value:@(HGAValueFromPercent(row,pct))]; };
        return cell;
    }
    UITableViewCell *cell=[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:nil];
    cell.textLabel.text=title;cell.textLabel.numberOfLines=0;cell.detailTextLabel.numberOfLines=0;
    cell.textLabel.font=[UIFont preferredFontForTextStyle:UIFontTextStyleBody];
    cell.detailTextLabel.font=[UIFont preferredFontForTextStyle:UIFontTextStyleCaption1];
    cell.textLabel.adjustsFontForContentSizeCategory=YES;cell.detailTextLabel.adjustsFontForContentSizeCategory=YES;
    cell.detailTextLabel.textColor=UIColor.secondaryLabelColor;
    cell.accessibilityIdentifier=[@"honggo.appearance.setting." stringByAppendingString:key];
    if([kind isEqual:@"bool"]) {
        UISwitch *toggle=[UISwitch new];toggle.on=HGAppearanceFlag(key) != [row[@"inverted"] boolValue];toggle.accessibilityIdentifier=key;
        toggle.accessibilityLabel=row[@"title"];
        [toggle addTarget:self action:@selector(toggle:) forControlEvents:UIControlEventValueChanged];
        cell.accessoryView=toggle;
        if([key isEqual:@"player.fullscreen"])
            cell.detailTextLabel.text=@"横屏播放宽画幅视频时铺满。竖屏短剧已默认等比铺满。";
        if([key isEqual:@"player.disableAutoEnterSeries"])
            cell.detailTextLabel.text=@"开启后，推荐视频播完时可自动进入全集。";
        if([key isEqual:@"player.homePreviewRepair"])
            cell.detailTextLabel.text=@"点首页预览下方的全集入口时，如果红果提供了当前片段对应的剧集，就打开那一集。关闭后沿用红果的选集方式。";
        if([key isEqual:@"gestures.cleanWhilePlaying"])
            cell.detailTextLabel.text=@"播放时隐藏控件，点按画面暂停后显示，继续播放时再次隐藏。";
        if([key isEqual:@"player.hideAIFictionNotice"])
            cell.detailTextLabel.text=@"显示播放器左侧的独立文字提示，保留原始画幅。";
        if([key isEqual:@"player.hideNowPlayingIsland"])
            cell.detailTextLabel.text=@"进入后台后恢复红果的系统播放信息";
        if([key isEqual:@"player.highestQuality"])
            cell.detailTextLabel.text=@"新视频在预加载或起播时选择最高可用画质，保留已缓存或手动选择的画质。";
        if([key isEqual:@"player.commentMovesVideo"])
            cell.detailTextLabel.text=@"关闭后，打开评论区时视频留在原位。";
        if([key isEqual:@"mine.show.theme"])
            cell.detailTextLabel.text=@"新版个人页在创建时读取此项，重新打开红果后生效。";
        if([key isEqual:@"player.hideFollowRecommendUsers"])
            cell.detailTextLabel.text=@"关注内容刷完后显示「为你推荐其他精选用户」推荐卡片。";
    }else if([kind isEqual:@"subpage"]) {
        cell.accessoryType=UITableViewCellAccessoryDisclosureIndicator;
        NSString *detail=row[@"detail"];
        if(detail.length && ![detail isEqual:@"默认"] && ![detail isEqual:@"跟随预设"])cell.detailTextLabel.text=detail;
    }else {
        cell.accessoryType=UITableViewCellAccessoryDisclosureIndicator;
        NSString *value=HGAppearanceString(key);
        if([kind isEqual:@"choice"]) {
            NSUInteger i=[row[@"choices"] indexOfObject:value];cell.detailTextLabel.text=i!=NSNotFound?row[@"labels"][i]:@"默认";
        }else if([kind isEqual:@"keywordList"]) {
            NSUInteger count=HGVideoBlockedKeywords().count;
            cell.detailTextLabel.text=count ? [NSString stringWithFormat:@"已设置 %lu 个关键词",(unsigned long)count] : @"未设置";
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
    BOOL autoEnableMaster=!HGAppearanceEnabled() && ![key isEqual:@"enabled"] && ![key isEqual:@"player.homePreviewRepair"] &&
        (![self.group hasPrefix:@"setting."] || [key hasPrefix:@"mine.settings.show."]);
    NSString *area=@{@"dock.style":@"dock.enabled",@"player.headerStyle":@"player.headerGlass",
        @"player.actionStyle":@"player.rightGlass",
        @"panels.commentMaterial":@"panels.comment",@"panels.shareMaterial":@"panels.share",
        @"panels.longpressMaterial":@"panels.longpress"}[key];
    if([key hasPrefix:@"dock."] && ![key isEqual:@"dock.enabled"] && !HGAppearanceFlag(@"dock.enabled"))
        area=@"dock.enabled";
    NSMutableDictionary *change=[@{key:value} mutableCopy];
    if(autoEnableMaster)change[@"enabled"]=@YES;
    if(area) {
        change[area]=@YES;
        if([key isEqual:@"dock.style"] && [value isEqual:@"glass"] &&
            [@[@"blur",@"solid"] containsObject:HGAppearanceString(@"dock.material")])
            change[@"dock.material"]=@"regular";
    }
    if(change.count>1)HGAppearanceApply(change,&error);
    else HGAppearanceSet(key,value,&error);
    if(!error) {
        if(HGAppearanceEnabled())HGPrepareNativeGlass();
        if(autoEnableMaster)[self reloadRows];
    } else [self message:@"设置未更改" detail:error.localizedDescription];
}
- (void)toggle:(UISwitch *)toggle {
    NSString *key=toggle.accessibilityIdentifier;
    BOOL inverted=[HGACatalogDef(key)[@"inverted"] boolValue];
    [self save:key value:(toggle.on != inverted ? @YES : @NO)];
    toggle.on=HGAppearanceFlag(key) != inverted;
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
    if([kind isEqual:@"keywordList"]){
        [self.navigationController pushViewController:[HGVideoKeywordEditor new] animated:YES];
        return;
    }
    if([key isEqual:@"@community"]){HGShowCommunity(self);return;}
    if([key hasPrefix:@"@group."]){HGOpenAppearance(self,[key substringFromIndex:7]);return;}
    if([key hasPrefix:@"@role.copyRightSize."]) {
        NSString *source=[NSString stringWithFormat:@"layout.%@.scale",key.pathExtension];
        NSNumber *value=@(HGAppearanceNumber(source));
        NSMutableDictionary *sizes=[NSMutableDictionary dictionary];
        for (NSString *role in HGARightButtonRoles())
            sizes[[NSString stringWithFormat:@"layout.%@.scale",role]]=value;
        NSError *error=nil;
        if (HGAppearanceApply(sizes,&error)) {
            if ([self.group isEqual:@"rightButtons"]) [self.tableView reloadData];
            NSDictionary *definition=HGACatalogDef(source);
            NSInteger pct=HGAPercentFromValue(definition,value.doubleValue);
            [self message:@"已统一右侧按钮大小" detail:HGAPercentDetailText(definition,pct)];
        } else [self message:@"设置未更改" detail:error.localizedDescription];
        return;
    }
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
        NSString *current=HGAppearanceString(key);
        for(NSUInteger i=0;i<values.count;i++) {
            NSString *title=[values[i] isEqual:current] ? [@"✓ " stringByAppendingString:labels[i]] : labels[i];
            [a addAction:[UIAlertAction actionWithTitle:title style:UIAlertActionStyleDefault handler:^(UIAlertAction *button){
                [self save:key value:values[i]];
                [self reloadRows];
            }]];
        }
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
    if ([group hasPrefix:@"setting."]) {
        NSDictionary *row=HGACatalogDef([group substringFromIndex:8]);
        if ([row[@"kind"] isEqual:@"choice"]) panel.initialChoice=row;
    }
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
