@interface FQRJXCategoryTitleCell : UICollectionViewCell
@property(nonatomic,strong) UILabel *titleLabel;
@end
@implementation FQRJXCategoryTitleCell @end
@interface FQRJXCategoryTitleView : UIView
@property(nonatomic,copy) NSArray *titles;
@property(nonatomic,copy) NSArray *indicators;
@property(nonatomic) NSInteger selectedIndex;
@end
@implementation FQRJXCategoryTitleView @end
@interface SSVideoSeriesFeedHeaderSearchView : UIView
@property(nonatomic,strong) UIView *searchView;
@property(nonatomic,strong) UIView *searchBgView;
@property(nonatomic,strong) UIView *imageSearchContainer;
@end
@implementation SSVideoSeriesFeedHeaderSearchView @end
@interface FQTheaterViewController : UIViewController
@property(nonatomic,strong) UIView *searchView;
@property(nonatomic,strong) UIView *searchBgView;
@property(nonatomic,strong) UIView *imageSearchContainer;
@property(nonatomic,strong) UIView *aiSearchEntranceView;
@end
@implementation FQTheaterViewController @end
@interface SSBookStoreEntranceView : UIView
@property(nonatomic,strong) UILabel *nameLabel;
@end
@implementation SSBookStoreEntranceView @end
@interface SSMyUser637TabsView : UIView
@property(nonatomic,strong) UIButton *searchBtn;
@end
@implementation SSMyUser637TabsView @end
@interface SSVideoFilterPanelEntranceView : UIView
@property(nonatomic,strong) UIView *backgroundView;
@end
@implementation SSVideoFilterPanelEntranceView @end
@interface FQReaderSaaSBaseButton : UIButton @end
@implementation FQReaderSaaSBaseButton @end

static FQRJXCategoryTitleCell *PageChromeTitle(UIView *container,NSString *title,CGFloat x) {
    FQRJXCategoryTitleCell *cell=[[FQRJXCategoryTitleCell alloc] initWithFrame:CGRectMake(x,0,52,32)];
    cell.titleLabel=[[UILabel alloc] initWithFrame:cell.contentView.bounds];
    cell.titleLabel.text=title;
    [cell.contentView addSubview:cell.titleLabel];[container addSubview:cell];Layout(cell);
    return cell;
}

static UIView *PageChromeControl(UIView *parent,CGRect frame) {
    UIView *view=[[UIView alloc] initWithFrame:frame];
    view.backgroundColor=UIColor.systemGrayColor;
    [parent addSubview:view];
    return view;
}

static void VerifyPageChrome(UIView *root,SSTabBar *bar,SSTabBarController *host) {
    NSString *material=@"honggo.appearance.material";
    SSVideoSeriesFeedHeaderView *header=[[SSVideoSeriesFeedHeaderView alloc]
        initWithFrame:CGRectMake(0,64,CGRectGetWidth(bar.bounds),44)];
    [root addSubview:header];
    header.sideBar=PageChromeControl(header,CGRectMake(16,0,38,44));
    FQRJXCategoryTitleView *channels=[[FQRJXCategoryTitleView alloc] initWithFrame:CGRectMake(56,0,220,44)];
    header.tabContainer=channels;[header addSubview:channels];
    SSVideoSeriesFeedHeaderSearchView *search=[[SSVideoSeriesFeedHeaderSearchView alloc]
        initWithFrame:CGRectMake(CGRectGetWidth(header.bounds)-68,0,52,44)];
    header.searchViewContainer=search;[header addSubview:search];
    NSArray *titles=@[@"关注",@"漫剧",@"真人剧",@"推荐"];
    channels.titles=titles;channels.selectedIndex=3;
    UIView *underline=PageChromeControl(channels,CGRectMake(167,30,28,2));
    channels.indicators=@[underline];
    NSMutableArray *controls=[NSMutableArray arrayWithObject:header.sideBar];
    for(NSUInteger i=0;i<titles.count;i++)[controls addObject:PageChromeTitle(header.tabContainer,titles[i],52*i)];
    [controls addObject:search];
    Layout(header);Layout(channels);HGAppearanceLayoutDock(bar,host);
    UIView *capsule=Find(bar,@"honggo.appearance.dock.capsule");
    UIView *accessory=Find(bar,@"honggo.appearance.dock.extra");
    CGRect dock=CGRectUnion([capsule convertRect:capsule.bounds toView:header],
        [accessory convertRect:accessory.bounds toView:header]);
    UIView *headerGlass=Find(header,material);
    Check(capsule && accessory && headerGlass && fabs(CGRectGetWidth(dock)-CGRectGetWidth(headerGlass.frame))<0.5 &&
        fabs(CGRectGetMidX(dock)-CGRectGetMidX(headerGlass.frame))<0.5,
        @"the top capsule matches the expanded bottom dock including its round button");

    NSArray *keys=@[@"sidebar",@"follow",@"comic",@"live",@"recommend",@"search"];
    BOOL independent=YES;
    for(NSUInteger i=0;i<keys.count;i++) {
        NSString *key=[@"layout.channels.hide." stringByAppendingString:keys[i]];
        HGAppearanceSet(key,@YES,nil);
        for(NSUInteger j=0;j<controls.count;j++)independent&=((UIView *)controls[j]).hidden==(i==j);
        independent&=underline.hidden==(i==4);
        HGAppearanceSet(key,@NO,nil);
        independent&=!((UIView *)controls[i]).hidden && !underline.hidden;
    }
    Check(independent,@"each of the six header controls hides and restores independently");

    search.searchView=PageChromeControl(search,CGRectMake(0,0,52,44));
    search.searchBgView=PageChromeControl(search.searchView,search.searchView.bounds);
    search.imageSearchContainer=PageChromeControl(search,CGRectMake(12,0,30,24));
    header.findDramaAIEntranceViewV731=PageChromeControl(header,CGRectMake(240,0,42,44));
    HGAppearanceSet(@"layout.channels.hide.search",@YES,nil);
    header.isFindDramaSearchPresentationActiveV731=YES;Layout(header);
    Check(!search.hidden && Find(search.searchBgView,material) && Find(search.imageSearchContainer,material) &&
        Find(header.findDramaAIEntranceViewV731,material) && ![header.subviews.firstObject.accessibilityIdentifier isEqual:material],
        @"finder mode restores the shared search control and uses scoped search, recognition and AI glass");
    HGAppearanceSet(@"layout.channels.hide.search",@NO,nil);[header removeFromSuperview];

    FQTheaterViewController *finder=[FQTheaterViewController new];
    [host addChildViewController:finder];finder.view.frame=CGRectMake(0,140,280,90);
    [root addSubview:finder.view];[finder didMoveToParentViewController:host];
    finder.searchView=PageChromeControl(finder.view,CGRectMake(0,0,180,34));
    finder.searchBgView=PageChromeControl(finder.searchView,finder.searchView.bounds);
    finder.imageSearchContainer=PageChromeControl(finder.view,CGRectMake(183,0,40,34));
    finder.aiSearchEntranceView=PageChromeControl(finder.view,CGRectMake(226,0,40,34));
    NSArray *labels=@[@"筛选",@"排行榜",@"新剧",@"预约",@"其他"];
    NSMutableArray *shortcuts=[NSMutableArray array];
    for(NSUInteger i=0;i<labels.count;i++) {
        SSBookStoreEntranceView *shortcut=[[SSBookStoreEntranceView alloc] initWithFrame:CGRectMake(i*55,44,52,34)];
        shortcut.nameLabel=[[UILabel alloc] initWithFrame:shortcut.bounds];shortcut.nameLabel.text=labels[i];
        [shortcut addSubview:shortcut.nameLabel];[finder.view addSubview:shortcut];[shortcuts addObject:shortcut];Layout(shortcut);
    }
    HGAppearanceProcessController(finder);
    BOOL scoped=Find(finder.searchBgView,material) && Find(finder.imageSearchContainer,material) && Find(finder.aiSearchEntranceView,material);
    for(NSUInteger i=0;i<shortcuts.count;i++)scoped&=(Find(shortcuts[i],material)!=nil)==(i<4);
    HGAppearanceSet(@"player.finderGlass",@NO,nil);
    scoped&=!Find(finder.view,material) && [finder.searchBgView.backgroundColor isEqual:UIColor.systemGrayColor];
    Check(scoped,@"finder glass targets the four shortcuts and restores native search backgrounds on disable");
    [finder willMoveToParentViewController:nil];[finder.view removeFromSuperview];[finder removeFromParentViewController];
    HGAppearanceSet(@"player.finderGlass",@YES,nil);

    SSMyUser637TabsView *mine=[[SSMyUser637TabsView alloc] initWithFrame:CGRectMake(0,230,290,80)];
    [root addSubview:mine];mine.searchBtn=[[UIButton alloc] initWithFrame:CGRectMake(255,0,30,32)];
    [mine addSubview:mine.searchBtn];
    UIView *toolbar=PageChromeControl(mine,CGRectMake(0,40,285,38));
    UIScrollView *categoryWrapper=[[UIScrollView alloc] initWithFrame:CGRectMake(0,0,190,34)];
    [toolbar addSubview:categoryWrapper];
    FQRJXCategoryTitleView *categories=[[FQRJXCategoryTitleView alloc] initWithFrame:categoryWrapper.bounds];
    [categoryWrapper addSubview:categories];
    SSVideoFilterPanelEntranceView *filter=[[SSVideoFilterPanelEntranceView alloc] initWithFrame:CGRectMake(192,0,42,34)];
    [toolbar addSubview:filter];filter.backgroundView=PageChromeControl(filter,filter.bounds);
    FQReaderSaaSBaseButton *edit=[[FQReaderSaaSBaseButton alloc] initWithFrame:CGRectMake(239,0,46,34)];
    [edit setTitle:@"编辑" forState:UIControlStateNormal];[toolbar addSubview:edit];
    FQRJXCategoryTitleCell *all=PageChromeTitle(categories,@"全部",0);
    FQRJXCategoryTitleCell *watched=PageChromeTitle(categories,@"已看完",54);
    FQRJXCategoryTitleCell *unwatched=PageChromeTitle(categories,@"未看完",110);
    Layout(filter);Layout(mine);
    BOOL history=Find(mine.searchBtn,material) && Find(filter.backgroundView,material) && Find(edit,material) &&
        Find(all,material) && Find(watched,material) && Find(unwatched,material);
    HGAppearanceSet(@"player.mineGlass",@NO,nil);
    Check(history && !Find(mine,material),@"Mine chrome follows the native search, filter, edit and history status controls");
    HGAppearanceSet(@"player.mineGlass",@YES,nil);[mine removeFromSuperview];
}
