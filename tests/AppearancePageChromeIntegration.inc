@interface FQRJXCategoryTitleCell : UICollectionViewCell
@property(nonatomic,strong) UILabel *titleLabel;
@end
@implementation FQRJXCategoryTitleCell @end
@interface FQRJXCategoryTitleView : UIView
@property(nonatomic,copy) NSArray *titles;
@property(nonatomic,copy) NSArray<UIView *> *indicators;
@property(nonatomic) NSInteger selectedIndex;
@end
@implementation FQRJXCategoryTitleView @end
@interface SSHGBookStoreCategoryTitleCell : FQRJXCategoryTitleCell
- (void)reloadData:(NSString *)title;
@end
@implementation SSHGBookStoreCategoryTitleCell
- (void)layoutSubviews {[super layoutSubviews];}
- (void)reloadData:(NSString *)title {
    self.hidden=NO;
    self.titleLabel.text=title;
}
@end
@interface SSHGBookStoreCategoryTitleView : FQRJXCategoryTitleView
- (void)refreshState;
- (void)refreshSelectedCellModel:(NSNumber *)selected unselectedCellModel:(NSNumber *)unselected;
@end
@implementation SSHGBookStoreCategoryTitleView
- (void)refreshState {self.indicators.firstObject.hidden=NO;}
- (void)refreshSelectedCellModel:(NSNumber *)selected unselectedCellModel:(NSNumber *)unselected {
    self.selectedIndex=selected.integerValue;
    [self refreshState];
}
@end
@interface SSVideoSeriesFeedHeaderSearchView : UIView
@property(nonatomic,strong) UIView *searchView;
@property(nonatomic,strong) UIView *searchBgView;
@property(nonatomic,strong) UIView *imageSearchContainer;
@property(nonatomic,strong) UIView *searchButtonSlotView;
@end
@implementation SSVideoSeriesFeedHeaderSearchView @end
@interface SSVideoSeriesFeedHeaderView (PageChromeFixture)
- (void)setupSideBarViewIfNeeded;
- (void)relayoutHeaderWithSearchBar;
@end
@implementation SSVideoSeriesFeedHeaderView (PageChromeFixture)
- (void)setupSideBarViewIfNeeded {self.sideBar.hidden=NO;}
- (void)relayoutHeaderWithSearchBar {
    SSVideoSeriesFeedHeaderSearchView *search=(id)self.searchViewContainer;
    search.searchButtonSlotView.hidden=NO;
}
@end
@interface HGAObservedHeaderFixture : SSVideoSeriesFeedHeaderView @end
@implementation HGAObservedHeaderFixture @end
@interface FQTheaterViewController : UIViewController
@property(nonatomic,strong) UIView *searchView;
@property(nonatomic,strong) UIView *searchBgView;
@property(nonatomic,strong) UIView *imageSearchContainer;
@property(nonatomic,strong) UIView *aiSearchEntranceView;
@end
@implementation FQTheaterViewController @end
@interface HGAObservedTheaterFixture : FQTheaterViewController @end
@implementation HGAObservedTheaterFixture @end
@interface SSBookStoreEntranceView : UIView
@property(nonatomic,strong) UILabel *nameLabel;
@property(nonatomic,strong) UIImageView *bgImgView;
@property(nonatomic) NSUInteger backgroundGetterCalls;
@end
@implementation SSBookStoreEntranceView
@synthesize bgImgView=_bgImgView;
- (UIImageView *)bgImgView {
    self.backgroundGetterCalls++;
    return _bgImgView;
}
@end
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
    Class cellClass=[container isKindOfClass:SSHGBookStoreCategoryTitleView.class] ?
        SSHGBookStoreCategoryTitleCell.class : FQRJXCategoryTitleCell.class;
    FQRJXCategoryTitleCell *cell=[[cellClass alloc] initWithFrame:CGRectMake(x,0,52,32)];
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
    SSVideoSeriesFeedHeaderView *header=[[HGAObservedHeaderFixture alloc]
        initWithFrame:CGRectMake(0,64,CGRectGetWidth(bar.bounds),44)];
    [root addSubview:header];
    header.sideBar=PageChromeControl(header,CGRectMake(16,10,24,24));
    UIView *tabContainer=PageChromeControl(header,CGRectMake(56,0,220,44));
    SSHGBookStoreCategoryTitleView *channels=[[SSHGBookStoreCategoryTitleView alloc] initWithFrame:tabContainer.bounds];
    header.tabContainer=tabContainer;[tabContainer addSubview:channels];
    SSVideoSeriesFeedHeaderSearchView *search=[[SSVideoSeriesFeedHeaderSearchView alloc] initWithFrame:header.bounds];
    header.searchViewContainer=search;[header addSubview:search];
    search.searchButtonSlotView=PageChromeControl(search,CGRectMake(CGRectGetWidth(header.bounds)-40,10,24,24));
    CGRect nativeSearch=search.frame,nativeTabs=tabContainer.frame;
    NSArray *titles=@[@"关注",@"漫剧",@"真人剧",@"推荐"];
    channels.titles=titles;channels.selectedIndex=3;
    UIView *categoryList=[[UIView alloc] initWithFrame:channels.bounds];
    [channels addSubview:categoryList];
    UIView *underline=PageChromeControl(categoryList,CGRectMake(167,30,28,2));
    channels.indicators=@[underline];
    NSMutableArray *controls=[NSMutableArray arrayWithObject:header.sideBar];
    for(NSUInteger i=0;i<titles.count;i++) {
        SSHGBookStoreCategoryTitleCell *cell=(id)PageChromeTitle(channels,titles[i],52*i);
        [categoryList addSubview:cell];
        Layout(cell);
        [controls addObject:cell];
    }
    [controls addObject:search.searchButtonSlotView];
    Layout(header);Layout(channels);HGAppearanceLayoutDock(bar,host);
    UIView *capsule=Find(bar,@"honggo.appearance.dock.capsule");
    UIView *accessory=Find(bar,@"honggo.appearance.dock.extra");
    CGRect dock=CGRectUnion([capsule convertRect:capsule.bounds toView:header],
        [accessory convertRect:accessory.bounds toView:header]);
    UIView *headerGlass=Find(header,material);
    Check(capsule && accessory && headerGlass && fabs(CGRectGetWidth(dock)-CGRectGetWidth(headerGlass.frame))<0.5 &&
        fabs(CGRectGetMidX(dock)-CGRectGetMidX(headerGlass.frame))<0.5,
        @"the top capsule matches the expanded bottom dock including its round button");
    CGRect left=[header.sideBar convertRect:header.sideBar.bounds toView:header];
    CGRect right=[search.searchButtonSlotView convertRect:search.searchButtonSlotView.bounds toView:header];
    Check(headerGlass && fabs(CGRectGetMidX(left)-CGRectGetMinX(headerGlass.frame)-23)<0.5 &&
        fabs(CGRectGetMaxX(headerGlass.frame)-CGRectGetMidX(right)-23)<0.5 &&
        left.size.width==24 && right.size.width==24 && CGRectEqualToRect(search.frame,nativeSearch) &&
        CGRectEqualToRect(tabContainer.frame,nativeTabs),
        @"the two native header slots get equal capsule insets without resizing their hit areas or moving channels");
    HGAppearanceSet(@"player.headerGlass",@NO,nil);
    Check(CGAffineTransformIsIdentity(header.sideBar.transform) &&
        CGAffineTransformIsIdentity(search.searchButtonSlotView.transform),
        @"disabling the home capsule restores both header slot positions");
    HGAppearanceSet(@"player.headerGlass",@YES,nil);

    NSArray *keys=@[@"sidebar",@"follow",@"comic",@"live",@"recommend",@"search"];
    BOOL independent=YES;
    for(NSUInteger i=0;i<keys.count;i++) {
        NSString *key=[@"layout.channels.hide." stringByAppendingString:keys[i]];
        HGAppearanceSet(key,@YES,nil);
        [header setupSideBarViewIfNeeded];
        [header relayoutHeaderWithSearchBar];
        for(NSUInteger j=1;j<=titles.count;j++)[controls[j] reloadData:titles[j-1]];
        [channels refreshSelectedCellModel:@3 unselectedCellModel:@0];
        for(NSUInteger j=0;j<controls.count;j++)independent&=((UIView *)controls[j]).hidden==(i==j);
        independent&=underline.hidden==(i==4) && !search.hidden;
        HGAppearanceSet(key,@NO,nil);
        independent&=!((UIView *)controls[i]).hidden && !underline.hidden;
    }
    Check(independent,@"each of the six header controls hides and restores independently");

    NSMutableDictionary *allHidden=[NSMutableDictionary dictionary];
    for(NSString *key in keys)allHidden[[@"layout.channels.hide." stringByAppendingString:key]]=@YES;
    HGAppearanceApply(allHidden,nil);
    Check(header.hidden && !Find(header,material),@"hiding all six native home controls also removes the empty capsule");
    HGAppearanceSet(@"layout.channels.hide.search",@NO,nil);
    Check(!header.hidden && Find(header,material) && !search.searchButtonSlotView.hidden && header.sideBar.hidden && underline.hidden,
        @"the R22 five-hidden combination preserves the native search as the only top control");
    for(NSString *key in allHidden.allKeys)allHidden[key]=@NO;
    HGAppearanceApply(allHidden,nil);

    search.searchBgView=PageChromeControl(search,search.bounds);
    search.searchView=PageChromeControl(search,CGRectMake(3,10,18,18));
    search.imageSearchContainer=PageChromeControl(search,CGRectMake(12,0,30,24));
    header.findDramaAIEntranceViewV731=PageChromeControl(header,CGRectMake(240,0,42,44));
    HGAppearanceSet(@"layout.channels.hide.search",@YES,nil);
    header.isFindDramaSearchPresentationActiveV731=YES;Layout(header);
    Check(!search.hidden && !search.searchButtonSlotView.hidden && Find(search.searchBgView,material) && !Find(search.searchView,material) && Find(search.imageSearchContainer,material) &&
        Find(header.findDramaAIEntranceViewV731,material) && ![header.subviews.firstObject.accessibilityIdentifier isEqual:material] &&
        CGAffineTransformIsIdentity(header.sideBar.transform) && CGAffineTransformIsIdentity(search.searchButtonSlotView.transform),
        @"finder mode restores the shared search control and uses scoped search, recognition and AI glass");
    HGAppearanceSet(@"layout.channels.hide.search",@NO,nil);[header removeFromSuperview];

    FQTheaterViewController *finder=[HGAObservedTheaterFixture new];
    [host addChildViewController:finder];finder.view.frame=CGRectMake(0,140,280,90);
    [root addSubview:finder.view];[finder didMoveToParentViewController:host];
    finder.searchBgView=PageChromeControl(finder.view,CGRectMake(0,0,180,34));
    finder.searchView=PageChromeControl(finder.view,CGRectMake(5,8,18,18));
    finder.imageSearchContainer=PageChromeControl(finder.view,CGRectMake(183,0,40,34));
    finder.aiSearchEntranceView=PageChromeControl(finder.view,CGRectMake(226,0,40,34));
    NSArray *labels=@[@"筛选",@"排行榜",@"新剧",@"预约",@"其他"];
    NSMutableArray *shortcuts=[NSMutableArray array];
    for(NSUInteger i=0;i<labels.count;i++) {
        SSBookStoreEntranceView *shortcut=[[SSBookStoreEntranceView alloc] initWithFrame:CGRectMake(i*55,44,52,34)];
        shortcut.bgImgView=[[UIImageView alloc] initWithImage:[UIImage systemImageNamed:@"rectangle.fill"]];
        shortcut.bgImgView.frame=shortcut.bounds;[shortcut addSubview:shortcut.bgImgView];
        shortcut.nameLabel=[[UILabel alloc] initWithFrame:shortcut.bounds];shortcut.nameLabel.text=labels[i];
        [shortcut addSubview:shortcut.nameLabel];[finder.view addSubview:shortcut];[shortcuts addObject:shortcut];Layout(shortcut);
    }
    HGAppearanceProcessController(finder);
    UIView *searchGlass=Find(finder.searchBgView,material);
    BOOL scoped=searchGlass && CGRectEqualToRect(searchGlass.frame,finder.searchBgView.bounds) && !Find(finder.searchView,material) &&
        Find(finder.imageSearchContainer,material) && Find(finder.aiSearchEntranceView,material);
    for(NSUInteger i=0;i<shortcuts.count;i++) {
        SSBookStoreEntranceView *shortcut=shortcuts[i];
        scoped&=(Find(shortcut,material)!=nil)==(i<4) && shortcut.bgImgView.hidden==(i<4) && !shortcut.nameLabel.hidden;
    }
    SSBookStoreEntranceView *reused=shortcuts.firstObject;
    UIImageView *previousCover=reused.bgImgView;
    UIImageView *currentCover=[[UIImageView alloc] initWithImage:previousCover.image];
    currentCover.frame=previousCover.frame;
    [previousCover removeFromSuperview];
    [reused insertSubview:currentCover belowSubview:reused.nameLabel];
    reused.bgImgView=currentCover;
    Layout(reused);
    scoped&=!previousCover.hidden && currentCover.hidden;
    NSUInteger getterCalls=reused.backgroundGetterCalls;
    HGAppearanceSet(@"player.finderGlass",@NO,nil);
    scoped&=reused.backgroundGetterCalls==getterCalls && !Find(finder.view,material) &&
        [finder.searchBgView.backgroundColor isEqual:UIColor.systemGrayColor];
    for(SSBookStoreEntranceView *shortcut in shortcuts)scoped&=!shortcut.bgImgView.hidden && shortcut.bgImgView.image!=nil;
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
    [toolbar addSubview:filter];
    filter.backgroundView=[[UIImageView alloc] initWithImage:[UIImage systemImageNamed:@"rectangle.fill"]];
    filter.backgroundView.frame=filter.bounds;[filter addSubview:filter.backgroundView];
    FQReaderSaaSBaseButton *edit=[[FQReaderSaaSBaseButton alloc] initWithFrame:CGRectMake(239,0,46,34)];
    [edit setTitle:@"编辑" forState:UIControlStateNormal];[toolbar addSubview:edit];
    FQRJXCategoryTitleCell *all=PageChromeTitle(categories,@"全部",0);
    FQRJXCategoryTitleCell *watched=PageChromeTitle(categories,@"已看完",54);
    FQRJXCategoryTitleCell *unwatched=PageChromeTitle(categories,@"未看完",110);
    Layout(filter);Layout(mine);
    BOOL history=Find(mine.searchBtn,material) && Find(filter,material) && filter.backgroundView.hidden && Find(edit,material) &&
        Find(all,material) && Find(watched,material) && Find(unwatched,material);
    HGAppearanceSet(@"player.mineGlass",@NO,nil);
    Check(history && !Find(mine,material) && !filter.backgroundView.hidden,
        @"Mine chrome follows native history controls and replaces and restores the filter background image");
    HGAppearanceSet(@"player.mineGlass",@YES,nil);[mine removeFromSuperview];
}
