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
- (void)layoutSubviews {
    [super layoutSubviews];
    self.titleLabel.bounds = CGRectMake(0,0,36,25);
    self.titleLabel.center = CGPointMake(CGRectGetMidX(self.contentView.bounds),CGRectGetMidY(self.contentView.bounds));
}
- (void)reloadData:(NSString *)title {
    self.hidden=NO;
    self.titleLabel.text=title;
}
@end
@interface FQRJXCategoryIndicatorLineView : UIView @end
@implementation FQRJXCategoryIndicatorLineView @end

@interface SSVideoSeriesFeedHeaderView (PageChromeFixture)
- (void)setupSideBarViewIfNeeded;
- (void)relayoutHeaderWithSearchBar;
- (void)tryShowSearchIconWithInteract:(BOOL)interactive ratio:(double)ratio toShowSearchIcon:(BOOL)visible animated:(BOOL)animated;
- (void)applySearchIconVisible:(BOOL)visible tabContainerTop:(double)top;
- (void)finishSearchIconVisibilityTransition:(BOOL)visible;
@end

@interface SSHGBookStoreCategoryTitleView : FQRJXCategoryTitleView <UICollectionViewDataSource,UICollectionViewDelegateFlowLayout>
@property(nonatomic,strong) UICollectionView *collectionView;
@property(nonatomic,strong) UIScrollView *contentScrollView;
@property(nonatomic,weak) FQRJXCategoryListContainerView *listContainer;
@property(nonatomic) NSInteger lastNativeTap;
@property(nonatomic) NSUInteger nativeSelections;
@property(nonatomic) BOOL nativeSelectionAnimated;
@property(nonatomic,weak) SSVideoSeriesFeedHeaderView *channelHeaderFixture;
- (BOOL)selectItemAtIndex:(NSInteger)index needAnimation:(BOOL)animated;
- (BOOL)selectCellAtIndex:(NSInteger)index selectedType:(NSUInteger)type needAnimation:(BOOL)animated;
- (CGRect)getTargetCellFrame:(NSInteger)index;
- (CGRect)getTargetSelectedCellFrame:(NSInteger)index selectedType:(NSUInteger)type;
- (void)refreshState;
- (void)refreshSelectedCellModel:(NSNumber *)selected unselectedCellModel:(NSNumber *)unselected;
@end
@implementation SSHGBookStoreCategoryTitleView
- (NSInteger)collectionView:(UICollectionView *)collection numberOfItemsInSection:(NSInteger)section { return self.titles.count; }
- (UICollectionViewCell *)collectionView:(UICollectionView *)collection cellForItemAtIndexPath:(NSIndexPath *)path {
    SSHGBookStoreCategoryTitleCell *cell = [collection dequeueReusableCellWithReuseIdentifier:@"channel" forIndexPath:path];
    if (!cell.titleLabel) {
        cell.titleLabel = [[UILabel alloc] initWithFrame:CGRectZero];
        [cell.contentView addSubview:cell.titleLabel];
    }
    [cell reloadData:self.titles[path.item]];
    return cell;
}
- (CGSize)collectionView:(UICollectionView *)collection layout:(UICollectionViewLayout *)layout sizeForItemAtIndexPath:(NSIndexPath *)path {
    return CGSizeMake(path.item==2 ? 54 : 36,44);
}
- (CGFloat)collectionView:(UICollectionView *)collection layout:(UICollectionViewLayout *)layout minimumLineSpacingForSectionAtIndex:(NSInteger)section { return 24; }
- (CGFloat)collectionView:(UICollectionView *)collection layout:(UICollectionViewLayout *)layout minimumInteritemSpacingForSectionAtIndex:(NSInteger)section { return 24; }
- (UIEdgeInsets)collectionView:(UICollectionView *)collection layout:(UICollectionViewLayout *)layout insetForSectionAtIndex:(NSInteger)section {
    return UIEdgeInsetsMake(0,16,0,16);
}
- (CGRect)getTargetCellFrame:(NSInteger)index {
    return CGRectMake(16+60*index+(index>2?18:0),0,index==2?54:36,44);
}
- (CGRect)getTargetSelectedCellFrame:(NSInteger)index selectedType:(NSUInteger)type {
    return [self getTargetCellFrame:index];
}
- (void)collectionView:(UICollectionView *)collection didSelectItemAtIndexPath:(NSIndexPath *)path {
    self.lastNativeTap = path.item;
    self.selectedIndex = path.item;
    CGPoint center = self.indicators.firstObject.center;
    center.x = CGRectGetMidX([self getTargetSelectedCellFrame:path.item selectedType:1]);
    self.indicators.firstObject.center = center;
}
- (BOOL)selectItemAtIndex:(NSInteger)index needAnimation:(BOOL)animated {
    return [self selectCellAtIndex:index selectedType:1 needAnimation:animated];
}
- (BOOL)selectCellAtIndex:(NSInteger)index selectedType:(NSUInteger)type needAnimation:(BOOL)animated {
    self.nativeSelections++;
    self.nativeSelectionAnimated=animated;
    [self.channelHeaderFixture tryShowSearchIconWithInteract:NO ratio:0.5 toShowSearchIcon:NO animated:YES];
    [self.contentScrollView setContentOffset:CGPointMake(self.contentScrollView.bounds.size.width*index,0) animated:animated];
    [self refreshSelectedCellModel:@(index) unselectedCellModel:@(self.selectedIndex)];
    [self.listContainer listDidAppear:index];
    return YES;
}
- (void)refreshState {self.indicators.firstObject.hidden=NO;}
- (void)refreshSelectedCellModel:(NSNumber *)selected unselectedCellModel:(NSNumber *)unselected {
    self.selectedIndex=selected.integerValue;
    [self refreshState];
}
@end

@interface HGATestChannelPan : UIPanGestureRecognizer
@property(nonatomic) UIGestureRecognizerState fixtureState;
@property(nonatomic) CGPoint fixtureTravel,fixtureVelocity;
@end

@protocol HGATestChannelPaging <UIGestureRecognizerDelegate>
- (void)page:(UIPanGestureRecognizer *)gesture;
@end
@implementation HGATestChannelPan
- (UIGestureRecognizerState)state { return self.fixtureState; }
- (CGPoint)translationInView:(UIView *)view { return self.fixtureTravel; }
- (CGPoint)velocityInView:(UIView *)view { return self.fixtureVelocity; }
@end

@interface HGATestEpisodeSwipeContext : NSObject
@property(nonatomic,weak) UIViewController *viewController;
@end
@implementation HGATestEpisodeSwipeContext @end

@interface SSVideoSeriesFeedViewController : UIViewController
@property(nonatomic,weak) UIView *containerView;
@property(nonatomic) NSUInteger menuTaps,searchTaps;
@end
@implementation SSVideoSeriesFeedViewController
- (void)simpleSideBarViewDidClicked { self.menuTaps++; }
- (void)simpleSearchViewDidClicked { self.searchTaps++; }
@end

@interface SSShortVideoPanGestureLeftTowardDelegate : NSObject
@property(nonatomic) NSUInteger calls;
- (BOOL)shouldLeftSlideEnterOtherPage:(id)navigation context:(id)context;
@end
@implementation SSShortVideoPanGestureLeftTowardDelegate
- (BOOL)shouldLeftSlideEnterOtherPage:(id)navigation context:(id)context { self.calls++;return YES; }
@end

@interface HGATestEpisodeSwipeOwner : UIViewController
@property(nonatomic,copy) BOOL (^ss_shouldBeginPushInteractiveTransitionDirectionHorizontalBlock)(UIPanGestureRecognizer *);
@property(nonatomic) BOOL needsGesture;
@property(nonatomic) NSUInteger swipeChecks;
@property(nonatomic) NSUInteger episodeTaps;
- (void)installLeftSwipe;
- (void)openEpisodes:(UIControl *)sender;
@end
@implementation HGATestEpisodeSwipeOwner
- (void)installLeftSwipe {
    __weak HGATestEpisodeSwipeOwner *weakSelf=self;
    self.ss_shouldBeginPushInteractiveTransitionDirectionHorizontalBlock=^BOOL(UIPanGestureRecognizer *pan) {
        HGATestEpisodeSwipeOwner *owner=weakSelf;
        owner.swipeChecks++;
        return !owner.needsGesture || pan!=nil;
    };
}
- (void)openEpisodes:(UIControl *)sender { self.episodeTaps++; }
@end

@interface SSBizVideoImpl : NSObject
+ (void)setLeftSlideToShortsListVCWithParentVC:(HGATestEpisodeSwipeOwner *)owner;
@end
@implementation SSBizVideoImpl
+ (void)setLeftSlideToShortsListVCWithParentVC:(HGATestEpisodeSwipeOwner *)owner { [owner installLeftSwipe]; }
@end

@interface SSShortVideoGestureComponent : NSObject
- (void)setupPanGestureTowardsLeftEnterOtherPage:(HGATestEpisodeSwipeContext *)context;
@end
@implementation SSShortVideoGestureComponent
- (void)setupPanGestureTowardsLeftEnterOtherPage:(HGATestEpisodeSwipeContext *)context {
    [(HGATestEpisodeSwipeOwner *)context.viewController installLeftSwipe];
}
@end

static void HGRunEpisodeSwipeDecisionChecks(void) {
    HGATestEpisodeSwipeOwner *home=[HGATestEpisodeSwipeOwner new];
    HGATestEpisodeSwipeOwner *list=[HGATestEpisodeSwipeOwner new];
    HGATestEpisodeSwipeContext *context=[HGATestEpisodeSwipeContext new];
    context.viewController=list;
    SSShortVideoGestureComponent *component=[SSShortVideoGestureComponent new];
    [SSBizVideoImpl setLeftSlideToShortsListVCWithParentVC:home];
    [component setupPanGestureTowardsLeftEnterOtherPage:context];
    home.needsGesture=YES;list.needsGesture=YES;
    UIPanGestureRecognizer *pan=[UIPanGestureRecognizer new];
    Check(home.ss_shouldBeginPushInteractiveTransitionDirectionHorizontalBlock(pan) &&
        !list.ss_shouldBeginPushInteractiveTransitionDirectionHorizontalBlock(nil) &&
        home.swipeChecks==1 && list.swipeChecks==1,
        @"both verified swipe setups preserve the native gesture decision and its argument by default");

    HGAppearanceSet(@"player.disableEpisodeSwipe",@YES,nil);
    Check(!home.ss_shouldBeginPushInteractiveTransitionDirectionHorizontalBlock(pan) &&
        !list.ss_shouldBeginPushInteractiveTransitionDirectionHorizontalBlock(pan) &&
        home.swipeChecks==1 && list.swipeChecks==1,
        @"changing the episode switch blocks both installed directional pushes before they call the original");
    UIControl *episodeButton=[UIControl new];
    [episodeButton addTarget:home action:@selector(openEpisodes:) forControlEvents:UIControlEventTouchUpInside];
    [episodeButton sendActionsForControlEvents:UIControlEventTouchUpInside];
    Check(home.episodeTaps==1,@"the explicit episode button still opens the episode view");

    [SSBizVideoImpl setLeftSlideToShortsListVCWithParentVC:home];
    Check(!home.ss_shouldBeginPushInteractiveTransitionDirectionHorizontalBlock(pan) && home.swipeChecks==1,
        @"a recreated home swipe preserves the current switch value");
    HGAppearanceSet(@"player.disableEpisodeSwipe",@NO,nil);
    Check(home.ss_shouldBeginPushInteractiveTransitionDirectionHorizontalBlock(pan) &&
        list.ss_shouldBeginPushInteractiveTransitionDirectionHorizontalBlock(pan) && home.swipeChecks==2 && list.swipeChecks==2,
        @"switching back restores both native gesture decisions without recreating the screens");
}

@interface SSVideoSeriesFeedHeaderSearchView : UIView
@property(nonatomic,strong) UIView *searchView;
@property(nonatomic,strong) UIView *searchBgView;
@property(nonatomic,strong) UIView *imageSearchContainer;
@property(nonatomic,strong) UIView *searchButtonSlotView;
@end
@implementation SSVideoSeriesFeedHeaderSearchView @end
@interface HGAFinderRecognitionFixture : UIView
@property(nonatomic,strong) UIView *separatorView;
@property(nonatomic,strong) UIImageView *cameraIconView;
@property(nonatomic,strong) UILabel *textLabel;
@end
@implementation HGAFinderRecognitionFixture @end
static char HGAFixtureHeaderAnimationKey;
@implementation SSVideoSeriesFeedHeaderView (PageChromeFixture)
- (void)setupSideBarViewIfNeeded {self.sideBar.hidden=NO;}
- (void)relayoutHeaderWithSearchBar {
    SSVideoSeriesFeedHeaderSearchView *search=(id)self.searchViewContainer;
    search.searchButtonSlotView.hidden=NO;
}
- (BOOL)isHeaderViewLayoutAnimating {
    return [objc_getAssociatedObject(self,&HGAFixtureHeaderAnimationKey) boolValue];
}
- (void)tryShowSearchIconWithInteract:(BOOL)interactive ratio:(double)ratio toShowSearchIcon:(BOOL)visible animated:(BOOL)animated {
    objc_setAssociatedObject(self,&HGAFixtureHeaderAnimationKey,@(animated),OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [self applySearchIconVisible:visible tabContainerTop:0];
}
- (void)applySearchIconVisible:(BOOL)visible tabContainerTop:(double)top {
    SSVideoSeriesFeedHeaderSearchView *search=(id)self.searchViewContainer;
    self.sideBar.hidden=!visible;
    search.alpha=visible ? 1 : 0;
    search.searchButtonSlotView.hidden=!visible;
    if (!visible && [self isHeaderViewLayoutAnimating]) {
        CABasicAnimation *fade=[CABasicAnimation animationWithKeyPath:@"opacity"];
        fade.fromValue=@1;fade.toValue=@0;fade.duration=10;
        [self.sideBar.layer addAnimation:fade forKey:@"opacity"];
    }
}
- (void)finishSearchIconVisibilityTransition:(BOOL)visible {
    objc_setAssociatedObject(self,&HGAFixtureHeaderAnimationKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [self applySearchIconVisible:visible tabContainerTop:0];
}
@end
@interface HGAObservedHeaderFixture : SSVideoSeriesFeedHeaderView @end
@implementation HGAObservedHeaderFixture @end
@interface FQTheaterViewController : UIViewController
@property(nonatomic,strong) UIView *searchView;
@property(nonatomic,strong) UIView *searchBgView;
@property(nonatomic,strong) UIView *imageSearchContainer;
@property(nonatomic,strong) HGAFinderRecognitionFixture *normalImageSearchView;
@property(nonatomic,strong) UIView *aiSearchEntranceView;
@property(nonatomic,strong) UIImageView *aiSearchEntranceImageView;
@property(nonatomic,strong) UIView *tabContainer;
@end
@implementation FQTheaterViewController @end
@interface HGAObservedTheaterFixture : FQTheaterViewController @end
@implementation HGAObservedTheaterFixture @end
@interface SSBookStoreEntranceView : UIView
@property(nonatomic,strong) UILabel *nameLabel;
@property(nonatomic,strong) UIImageView *bgImgView;
@property(nonatomic,strong) UIImageView *iconImgView;
@property(nonatomic,copy) NSString *model;
@property(nonatomic) NSUInteger backgroundGetterCalls;
@end
@implementation SSBookStoreEntranceView
@synthesize bgImgView=_bgImgView;
- (UIImageView *)bgImgView {
    self.backgroundGetterCalls++;
    return _bgImgView;
}
- (void)setModel:(NSString *)model {
    _model=[model copy];
    self.nameLabel.text=model;
    _bgImgView.hidden=NO;
}
@end
@interface FQFeedTabFilterPanelEntranceView : UIView
@property(nonatomic,strong) UIImageView *backgroundView;
@end
@implementation FQFeedTabFilterPanelEntranceView @end
@interface HGAFinderGoldenItemFixture : NSObject
@property(nonatomic,copy) NSString *text;
@end
@implementation HGAFinderGoldenItemFixture @end
@interface HGAFinderGoldenModelFixture : NSObject
@property(nonatomic,copy) NSArray *goldenLineItemArr;
@end
@implementation HGAFinderGoldenModelFixture @end
@interface SSVideoDistributionGoldenPositionSubCell : UICollectionViewCell
@property(nonatomic,strong) UIView *innerContentView;
@property(nonatomic,strong) UIImageView *backgroundImageView;
@property(nonatomic,strong) UIView *borderView;
@property(nonatomic,strong) UILabel *titleLabel;
@property(nonatomic,strong) UIImageView *iconImgView;
@property(nonatomic,strong) HGAFinderGoldenItemFixture *itemData;
- (void)setData:(HGAFinderGoldenItemFixture *)item styleConfig:(id)config;
- (void)onDarkModeChange:(BOOL)dark;
@end
@implementation SSVideoDistributionGoldenPositionSubCell
- (void)setData:(HGAFinderGoldenItemFixture *)item styleConfig:(id)config {
    self.itemData=item;
    self.titleLabel.text=item.text;
    [self onDarkModeChange:NO];
}
- (void)onDarkModeChange:(BOOL)dark {
    UIColor *paint=dark ? UIColor.systemGrayColor : UIColor.whiteColor;
    self.backgroundColor=paint;
    self.contentView.backgroundColor=paint;
    self.innerContentView.backgroundColor=paint;
    self.backgroundImageView.hidden=NO;
    self.borderView.hidden=NO;
}
@end
@interface SSVideoDistributionGoldenPositionCell : UICollectionViewCell <UICollectionViewDelegateFlowLayout>
@property(nonatomic,strong) UICollectionView *collectionView;
@property(nonatomic,strong) HGAFinderGoldenModelFixture *model;
@end
@implementation SSVideoDistributionGoldenPositionCell
- (CGSize)collectionView:(UICollectionView *)collection layout:(UICollectionViewLayout *)layout
   sizeForItemAtIndexPath:(NSIndexPath *)path { return CGSizeMake(98,32); }
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

@interface SSMinePageKMPFunctionDepend : NSObject
@property(nonatomic,copy) NSArray *items;
- (NSArray *)functionItems;
- (NSArray *)getFunctionSnapshot;
@end
@implementation SSMinePageKMPFunctionDepend
- (NSArray *)functionItems { return self.items; }
- (NSArray *)getFunctionSnapshot { return self.functionItems; }
@end

@interface HGAMineShortcutFixture : NSObject
@property(nonatomic,copy) NSString *title;
@end
@implementation HGAMineShortcutFixture @end

@interface HGAMineTitleHandleFixture : NSObject
@property(nonatomic,strong) UIView *titleBarView;
@end
@implementation HGAMineTitleHandleFixture @end

@interface SSMine737KMPViewController : UIViewController
@property(nonatomic,strong) HGAMineTitleHandleFixture *minePageKMPHandle;
@end
@implementation SSMine737KMPViewController @end

static Class HGAPageFixtureViewClass(const char *name) {
    Class type=objc_getClass(name);
    if(type)return type;
    type=objc_allocateClassPair(UIView.class,name,0);
    objc_registerClassPair(type);
    return type;
}

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

static void HGReportChannelLayout(NSString *phase, SSHGBookStoreCategoryTitleView *categories,
        UIView *header, UIView *line, UIView *pill) {
    UICollectionView *list = categories.collectionView;
    UICollectionViewFlowLayout *flow = (id)list.collectionViewLayout;
    NSIndexPath *comicPath = [NSIndexPath indexPathForItem:1 inSection:0];
    NSIndexPath *recPath = [NSIndexPath indexPathForItem:3 inSection:0];
    CGRect comic = [list layoutAttributesForItemAtIndexPath:comicPath].frame;
    CGRect rec = [list layoutAttributesForItemAtIndexPath:recPath].frame;
    FQRJXCategoryTitleCell *cell = (id)[list cellForItemAtIndexPath:recPath];
    NSIndexPath *hit = [list indexPathForItemAtPoint:CGPointMake(CGRectGetMidX(comic),CGRectGetMidY(comic))];
    NSLog(@"HEADER_GEOMETRY %@ titles=%@ bounds=%@ adjustedInset=%@ delegateMatch=%d direction=%ld estimate=%@ "
           "delegateComic=%@ delegateHidden=%@ lineSpacing=%g itemSpacing=%g inset=%@ attrs=%@/%@ target=%@/%@ "
           "label=%@ pill=%@ line=%@ centerInList=%@ lineAnimations=%@ hit=%@ nativeTap=%ld selected=%ld",
        phase, categories.titles, NSStringFromCGRect(list.bounds), NSStringFromUIEdgeInsets(list.adjustedContentInset),
        list.delegate == categories, (long)flow.scrollDirection, NSStringFromCGSize(flow.estimatedItemSize),
        NSStringFromCGSize([categories collectionView:list layout:flow sizeForItemAtIndexPath:comicPath]),
        NSStringFromCGSize([categories collectionView:list layout:flow sizeForItemAtIndexPath:[NSIndexPath indexPathForItem:0 inSection:0]]),
        [categories collectionView:list layout:flow minimumLineSpacingForSectionAtIndex:0],
        [categories collectionView:list layout:flow minimumInteritemSpacingForSectionAtIndex:0],
        NSStringFromUIEdgeInsets([categories collectionView:list layout:flow insetForSectionAtIndex:0]),
        NSStringFromCGRect(comic), NSStringFromCGRect(rec), NSStringFromCGRect([categories getTargetCellFrame:1]),
        NSStringFromCGRect([categories getTargetSelectedCellFrame:3 selectedType:1]),
        NSStringFromCGRect([cell.titleLabel convertRect:cell.titleLabel.bounds toView:header]),
        NSStringFromCGRect(pill.frame), NSStringFromCGRect([line convertRect:line.bounds toView:header]),
        NSStringFromCGPoint([line.superview convertPoint:line.center toView:list]), line.layer.animationKeys,
        hit, (long)categories.lastNativeTap, (long)categories.selectedIndex);
}

static void VerifyPageChrome(UIView *root,SSTabBar *bar,SSTabBarController *host) {
    NSString *material=@"honggo.appearance.material";
    SSVideoSeriesFeedHeaderView *header=[[HGAObservedHeaderFixture alloc]
        initWithFrame:CGRectMake(0,64,CGRectGetWidth(bar.bounds),44)];
    [root addSubview:header];
    header.sideBar=PageChromeControl(header,CGRectMake(16,10,24,24));
    UIView *tabContainer=PageChromeControl(header,CGRectMake((CGRectGetWidth(header.bounds)-266)/2,0,266,44));
    SSHGBookStoreCategoryTitleView *channels=[[SSHGBookStoreCategoryTitleView alloc] initWithFrame:tabContainer.bounds];
    header.tabContainer=tabContainer;[tabContainer addSubview:channels];
    SSVideoSeriesFeedHeaderSearchView *search=[[SSVideoSeriesFeedHeaderSearchView alloc] initWithFrame:header.bounds];
    header.searchViewContainer=search;[header addSubview:search];
    search.searchButtonSlotView=PageChromeControl(search,CGRectMake(CGRectGetWidth(header.bounds)-40,10,24,24));
    CGRect nativeSearch=search.frame,nativeTabs=tabContainer.frame;
    NSArray *titles=@[@"关注",@"漫剧",@"真人剧",@"推荐"];
    channels.titles=titles;channels.selectedIndex=3;
    FQRJXCategoryListContainerView *listContainer=[[FQRJXCategoryListContainerView alloc]
        initWithFrame:CGRectMake(0,108,CGRectGetWidth(root.bounds),300)];
    UIScrollView *pageScroll=[[UIScrollView alloc] initWithFrame:listContainer.bounds];
    CGFloat pageWidth=CGRectGetWidth(pageScroll.bounds);
    pageScroll.contentSize=CGSizeMake(pageWidth*titles.count,CGRectGetHeight(pageScroll.bounds));
    pageScroll.pagingEnabled=YES;
    pageScroll.contentOffset=CGPointMake(pageWidth*3,0);
    channels.listContainer=listContainer;
    channels.contentScrollView=pageScroll;
    listContainer.contentScrollView=pageScroll;
    [listContainer addSubview:pageScroll];
    [root insertSubview:listContainer belowSubview:header];
    NSUInteger stockGestures=pageScroll.gestureRecognizers.count;
    UICollectionViewFlowLayout *flow = [UICollectionViewFlowLayout new];
    flow.scrollDirection = UICollectionViewScrollDirectionHorizontal;
    UICollectionView *categoryList = [[UICollectionView alloc] initWithFrame:channels.bounds collectionViewLayout:flow];
    [categoryList registerClass:SSHGBookStoreCategoryTitleCell.class forCellWithReuseIdentifier:@"channel"];
    categoryList.dataSource=channels; categoryList.delegate=channels; channels.collectionView=categoryList;
    [channels addSubview:categoryList];
    [categoryList reloadData]; Layout(categoryList);
    FQRJXCategoryIndicatorLineView *underline = [[FQRJXCategoryIndicatorLineView alloc] initWithFrame:CGRectMake(224,39,16,2)];
    [categoryList addSubview:underline];
    channels.indicators=@[underline];
    NSMutableArray *controls=[NSMutableArray arrayWithObject:header.sideBar];
    for(NSUInteger i=0;i<titles.count;i++) {
        UICollectionViewCell *cell = [categoryList cellForItemAtIndexPath:[NSIndexPath indexPathForItem:i inSection:0]];
        if (!cell) { Check(NO,@"home header fixture realizes all four native channel cells"); return; }
        [controls addObject:cell];
    }
    [controls addObject:search.searchButtonSlotView];
    Layout(header);Layout(channels);HGAppearanceLayoutDock(bar,host);
    UIView *headerGlass=Find(header,material);
    UIView *sidebarGlass=Find(header,@"honggo.appearance.header.sidebar");
    UIView *searchGlassTop=Find(header,@"honggo.appearance.header.search");
    CGRect left=[header.sideBar convertRect:header.sideBar.bounds toView:header];
    CGRect right=[search.searchButtonSlotView convertRect:search.searchButtonSlotView.bounds toView:header];
    Check(headerGlass && sidebarGlass && searchGlassTop &&
        fabs(CGRectGetWidth(sidebarGlass.frame)-CGRectGetHeight(sidebarGlass.frame))<0.5 &&
        fabs(CGRectGetWidth(searchGlassTop.frame)-CGRectGetHeight(searchGlassTop.frame))<0.5 &&
        CGRectGetMinX(headerGlass.frame)>=CGRectGetMaxX(sidebarGlass.frame)+7.5 &&
        CGRectGetMaxX(headerGlass.frame)<=CGRectGetMinX(searchGlassTop.frame)-7.5 &&
        fabs(CGRectGetMidX(sidebarGlass.frame)-CGRectGetMidX(left))<0.5 &&
        fabs(CGRectGetMidX(searchGlassTop.frame)-CGRectGetMidX(right))<0.5 &&
        left.size.width==24 && right.size.width==24 && CGRectEqualToRect(search.frame,nativeSearch) &&
        CGRectEqualToRect(tabContainer.frame,nativeTabs) &&
        CGAffineTransformIsIdentity(header.sideBar.transform) &&
        CGAffineTransformIsIdentity(search.searchButtonSlotView.transform),
        @"home header separates left menu circle, center channel capsule, and right search circle without moving native targets");
    HGAppearanceSet(@"player.headerGlass",@NO,nil);
    Check(!Find(header,material) && !Find(header,@"honggo.appearance.header.sidebar") &&
        !Find(header,@"honggo.appearance.header.search") &&
        CGAffineTransformIsIdentity(header.sideBar.transform) &&
        CGAffineTransformIsIdentity(search.searchButtonSlotView.transform),
        @"disabling header glass removes all three separated header surfaces");
    HGAppearanceSet(@"player.headerGlass",@YES,nil);

    if (HGNativeGlassConfigured()) {
        SSVideoSeriesFeedViewController *toolOwner=[SSVideoSeriesFeedViewController new];
        [host addChildViewController:toolOwner];
        toolOwner.view.frame=root.bounds;
        [root insertSubview:toolOwner.view belowSubview:header];
        [toolOwner.view addSubview:header];
        [toolOwner didMoveToParentViewController:host];
        Layout(header);
        UIButton *menu=(id)Find(header,@"honggo.appearance.header.sidebar");
        UIButton *searchButton=(id)Find(header,@"honggo.appearance.header.search");
        UIView *menuHit=[header hitTest:CGPointMake(CGRectGetMidX(menu.frame),CGRectGetMidY(menu.frame)) withEvent:nil];
        UIView *searchHit=[header hitTest:CGPointMake(CGRectGetMidX(searchButton.frame),CGRectGetMidY(searchButton.frame)) withEvent:nil];
        UIView *cornerHit=[header hitTest:CGPointMake(CGRectGetMinX(menu.frame)+1,CGRectGetMinY(menu.frame)+1) withEvent:nil];
        BOOL nativeTools=[menu isKindOfClass:UIButton.class] && [searchButton isKindOfClass:UIButton.class];
        Check(nativeTools && menu.configuration && searchButton.configuration &&
            (menuHit==menu || [menuHit isDescendantOfView:menu]) &&
            (searchHit==searchButton || [searchHit isDescendantOfView:searchButton]) &&
            cornerHit!=menu && ![cornerHit isDescendantOfView:menu] &&
            menu.layer.zPosition<0 && searchButton.layer.zPosition<0 &&
            !header.sideBar.hidden && !search.searchButtonSlotView.hidden &&
            header.sideBar.accessibilityElementsHidden && search.searchButtonSlotView.accessibilityElementsHidden,
            @"the native glass buttons receive touches in their circles and leave the original glyphs visible above");
        if (nativeTools) {
            [menu sendActionsForControlEvents:UIControlEventPrimaryActionTriggered];
            [searchButton sendActionsForControlEvents:UIControlEventPrimaryActionTriggered];
        }
        Check(toolOwner.menuTaps==1 && toolOwner.searchTaps==1,
            @"each top button runs its verified native action one time");
        HGAppearanceSet(@"player.headerGlass",@NO,nil);
        Check(!Find(header,@"honggo.appearance.header.sidebar") && !Find(header,@"honggo.appearance.header.search") &&
            !header.sideBar.accessibilityElementsHidden && !search.searchButtonSlotView.accessibilityElementsHidden,
            @"removing glass restores both native tools' accessibility");
        [root addSubview:header];
        [toolOwner willMoveToParentViewController:nil];
        [toolOwner.view removeFromSuperview];
        [toolOwner removeFromParentViewController];
        HGAppearanceSet(@"player.headerGlass",@YES,nil);
    }

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

    // Reproduce the device capture: 266pt list, native line x=224..240,
    // two non-adjacent items visible. UIKit hit testing uses its layout map.
    HGAppearanceApply(@{@"layout.channels.hide.follow":@YES,@"layout.channels.hide.live":@YES},nil);
    Layout(header); Layout(categoryList);
    UIView *compactedPill=Find(header,material);
    UISegmentedControl *segments=[compactedPill isKindOfClass:UISegmentedControl.class] ? (id)compactedPill : nil;
    if (HGNativeGlassConfigured()) {
        UIView *segmentHit=[header hitTest:CGPointMake(CGRectGetMidX(segments.frame),CGRectGetMidY(segments.frame)) withEvent:nil];
        Check(segments.numberOfSegments==2 && [[segments titleForSegmentAtIndex:0] isEqual:@"漫剧"] &&
            [[segments titleForSegmentAtIndex:1] isEqual:@"推荐"] && segments.selectedSegmentIndex==1 &&
            categoryList.hidden && (segmentHit==segments || [segmentHit isDescendantOfView:segments]),
            @"flat channels retain the standard segmented control, direct touch target and original host indices");
    }
    NSIndexPath *comicPath=[NSIndexPath indexPathForItem:1 inSection:0];
    NSIndexPath *recPath=[NSIndexPath indexPathForItem:3 inSection:0];
    CGRect comic=[categoryList layoutAttributesForItemAtIndexPath:comicPath].frame;
    CGRect rec=[categoryList layoutAttributesForItemAtIndexPath:recPath].frame;
    CGFloat lineX=CGRectGetMidX([underline convertRect:underline.bounds toView:header]);
    FQRJXCategoryTitleCell *visibleRec = (id)[categoryList cellForItemAtIndexPath:recPath];
    FQRJXCategoryTitleCell *visibleComic = (id)[categoryList cellForItemAtIndexPath:comicPath];
    CGRect recLabel=[visibleRec.titleLabel convertRect:visibleRec.titleLabel.bounds toView:header];
    CGRect comicLabel=[visibleComic.titleLabel convertRect:visibleComic.titleLabel.bounds toView:header];
    CGFloat recX=CGRectGetMidX(recLabel);
    HGReportChannelLayout(@"compact",channels,header,underline,compactedPill);
    for (NSDictionary *entry in HGRuntimeSnapshot()[@"entries"]) {
        if ([entry[@"class"] isEqual:@"SSHGBookStoreCategoryTitleView"])
            NSLog(@"HEADER_ADAPTER %@ %@ %@",entry[@"selector"],entry[@"state"],entry[@"actual"]);
    }
    Check(compactedPill && CGRectContainsPoint(compactedPill.frame,CGPointMake(lineX,CGRectGetMidY(compactedPill.frame))) &&
        fabs(lineX-recX)<0.5 && fabs(CGRectGetMinX(recLabel)-CGRectGetMaxX(comicLabel)-20)<0.5,
        @"the compact capsule contains the native underline below the selected channel with a 20pt label gap");
    NSIndexPath *tap=[categoryList indexPathForItemAtPoint:CGPointMake(CGRectGetMidX(comic),CGRectGetMidY(comic))];
    if (tap) [categoryList.delegate collectionView:categoryList didSelectItemAtIndexPath:tap];
    HGReportChannelLayout(@"tap",channels,header,underline,compactedPill);
    Check([tap isEqual:comicPath] && channels.lastNativeTap==1 && channels.selectedIndex==1 &&
        fabs(underline.center.x-CGRectGetMidX(comic))<0.5 &&
        fabs(CGRectGetMidX([channels getTargetCellFrame:3])-CGRectGetMidX(rec))<0.5,
        @"native taps and the host click and swipe indicator coordinates use the original channel indices");
    UIPanGestureRecognizer *filterPan=nil;
    for (UIGestureRecognizer *gesture in pageScroll.gestureRecognizers)
        if ([NSStringFromClass(gesture.class) isEqual:@"HGAFixedChannelPan"]) filterPan=(id)gesture;
    id<HGATestChannelPaging> channelPaging=(id)filterPan.delegate;
    HGATestChannelPan *sample=[HGATestChannelPan new];
    sample.fixtureVelocity=CGPointMake(-60,450);
    BOOL vertical=[channelPaging gestureRecognizerShouldBegin:sample];
    sample.fixtureVelocity=CGPointMake(-450,60);
    BOOL horizontal=[channelPaging gestureRecognizerShouldBegin:sample];
    Check(pageScroll.gestureRecognizers.count==stockGestures+1 && !pageScroll.panGestureRecognizer.enabled && !vertical && horizontal &&
        [channelPaging gestureRecognizer:filterPan shouldBeRequiredToFailByGestureRecognizer:pageScroll.panGestureRecognizer] &&
        ![channelPaging gestureRecognizer:filterPan shouldBeRequiredToFailByGestureRecognizer:sample],
        @"filtering prioritizes the horizontal channel gesture over the native pager only");
    UIViewController *swipeOwner=[UIViewController new];
    swipeOwner.view.frame=pageScroll.bounds;
    [pageScroll addSubview:swipeOwner.view];
    HGATestEpisodeSwipeContext *swipeContext=[HGATestEpisodeSwipeContext new];
    swipeContext.viewController=swipeOwner;
    SSShortVideoPanGestureLeftTowardDelegate *episodePush=[SSShortVideoPanGestureLeftTowardDelegate new];
    Check(![episodePush shouldLeftSlideEnterOtherPage:nil context:swipeContext] && episodePush.calls==0,
        @"the native interactive series push yields inside the filtered pager");
    SSVideoSeriesFeedViewController *feedOwner=[SSVideoSeriesFeedViewController new];
    feedOwner.view.frame=root.bounds;
    [root insertSubview:feedOwner.view belowSubview:header];
    [feedOwner.view addSubview:listContainer];
    feedOwner.containerView=listContainer;swipeContext.viewController=feedOwner;
    Check(![episodePush shouldLeftSlideEnterOtherPage:nil context:swipeContext] && episodePush.calls==0,
        @"the home feed controller resolves its verified native channel scroll view for the push decision");
    [root insertSubview:listContainer belowSubview:header];
    [feedOwner.view removeFromSuperview];swipeContext.viewController=swipeOwner;
    [root addSubview:swipeOwner.view];
    Check([episodePush shouldLeftSlideEnterOtherPage:nil context:swipeContext] && episodePush.calls==1,
        @"the same delegate retains its native decision outside the home pager");
    [pageScroll addSubview:swipeOwner.view];
    sample.fixtureState=UIGestureRecognizerStateChanged;
    sample.fixtureTravel=CGPointMake(-64,2);
    [channelPaging page:sample];
    BOOL delayed=channels.nativeSelections==0;
    sample.fixtureState=UIGestureRecognizerStateEnded;
    sample.fixtureVelocity=CGPointZero;
    [channelPaging page:sample];
    BOOL leftSwipe=channels.selectedIndex==3 && pageScroll.contentOffset.x==3*pageWidth &&
        channels.nativeSelections==1 && !channels.nativeSelectionAnimated;
    [channelPaging page:sample];
    BOOL edge=channels.nativeSelections==1;
    sample.fixtureTravel=CGPointMake(64,2);
    [channelPaging page:sample];
    Check(delayed && leftSwipe && edge && channels.selectedIndex==1 && pageScroll.contentOffset.x==pageWidth &&
        channels.nativeSelections==2 && listContainer.nativeEnds==2 && channels.lastNativeTap==1,
        @"completed swipes skip hidden indices in both directions with immediate native selection and keep native taps unchanged");
    BOOL stableChannels=YES;
    NSUInteger originalSelections=channels.nativeSelections;
    UIView *stableLeft=Find(header,@"honggo.appearance.header.sidebar");
    UIView *stableRight=Find(header,@"honggo.appearance.header.search");
    channels.channelHeaderFixture=header;
    for (NSUInteger pass=0;pass<8;pass++) {
        NSInteger source=channels.selectedIndex, destination=source==1 ? 3 : 1;
        stableChannels &= ![channels selectCellAtIndex:2 selectedType:2 needAnimation:NO] && channels.selectedIndex==source;
        CGPoint seam=CGPointMake(CGRectGetMidX([channels getTargetCellFrame:2]),underline.center.y);
        underline.center=seam;
        CABasicAnimation *stale=[CABasicAnimation animationWithKeyPath:@"position.x"];
        stale.fromValue=@(seam.x);stale.duration=10;
        [underline.layer addAnimation:stale forKey:@"fixtureInterruptedUnderline"];
        sample.fixtureTravel=CGPointMake(destination==3 ? -70 : 70,0);
        [channelPaging page:sample];
        [header applySearchIconVisible:NO tabContainerTop:0];
        [header finishSearchIconVisibilityTransition:NO];
        stableChannels &= channels.selectedIndex==destination && pageScroll.contentOffset.x==pageWidth*destination &&
            fabs(underline.center.x-CGRectGetMidX([channels getTargetCellFrame:destination]))<0.5 &&
            ![underline.layer animationForKey:@"fixtureInterruptedUnderline"] &&
            (!segments || (segments.selectedSegmentIndex==(destination==1 ? 0 : 1) && Find(header,material)==segments));
        stableChannels &= !header.sideBar.hidden && search.alpha==1 && !search.searchButtonSlotView.hidden &&
            ![header.sideBar.layer animationForKey:@"opacity"] &&
            Find(header,@"honggo.appearance.header.sidebar")==stableLeft &&
            Find(header,@"honggo.appearance.header.search")==stableRight;
    }
    channels.channelHeaderFixture=nil;
    Check(stableChannels && channels.nativeSelections==originalSelections+8 && !pageScroll.panGestureRecognizer.enabled,
        @"repeated alternating swipes keep the original side tools and glass mounted and settle the selected line");
    if (segments) {
        NSUInteger selections=channels.nativeSelections;
        segments.selectedSegmentIndex=1;
        [segments sendActionsForControlEvents:UIControlEventValueChanged];
        [segments sendActionsForControlEvents:UIControlEventValueChanged];
        Check(channels.nativeSelections==selections+1 && channels.selectedIndex==3 &&
            pageScroll.contentOffset.x==3*pageWidth && !channels.nativeSelectionAnimated,
            @"changing the second segment commits native channel three once and skips the hidden page");
        segments.selectedSegmentIndex=0;
        [segments sendActionsForControlEvents:UIControlEventValueChanged];
        Check(channels.nativeSelections==selections+2 && channels.selectedIndex==1 &&
            pageScroll.contentOffset.x==pageWidth,
            @"returning to the first segment commits native channel one");
    }
    if (@available(iOS 26.0,*)) {
        if (HGNativeGlassConfigured() && !UIAccessibilityIsReduceTransparencyEnabled()) {
            HGAppearanceSet(@"player.headerStyle",@"glass",nil);
            Layout(header);
            UIView *group=Find(header,material);
            Layout(group);
            NSMutableArray<UIButton *> *buttons=[NSMutableArray array];
            for (NSUInteger i=0;i<2;i++) {
                CGPoint center=CGPointMake(CGRectGetWidth(group.bounds)*(0.25+0.5*i),CGRectGetMidY(group.bounds));
                UIView *hit=[header hitTest:[group convertPoint:center toView:header] withEvent:nil];
                while (hit && hit!=group && ![hit isKindOfClass:UIButton.class]) hit=hit.superview;
                if ([hit isKindOfClass:UIButton.class] && [hit isDescendantOfView:group]) [buttons addObject:(UIButton *)hit];
            }
            BOOL native=buttons.count==2 && buttons[0]!=buttons[1] && buttons[0].selected &&
                [buttons[0].configuration.attributedTitle.string isEqual:@"漫剧"] &&
                [buttons[1].configuration.attributedTitle.string isEqual:@"推荐"];
            Check(native && !segments.window && categoryList.hidden,
                @"liquid glass replaces the flat strip with two directly hittable configured channel buttons");
            if (native) {
                NSUInteger before=channels.nativeSelections;
                [buttons[1] sendActionsForControlEvents:UIControlEventPrimaryActionTriggered];
                [buttons[1] sendActionsForControlEvents:UIControlEventPrimaryActionTriggered];
                Check(buttons[1].selected && !buttons[0].selected && channels.selectedIndex==3 &&
                    channels.nativeSelections==before+1 && pageScroll.contentOffset.x==3*pageWidth,
                    @"glass channel activation selects the originally visible host page exactly once");
                [buttons[0] sendActionsForControlEvents:UIControlEventPrimaryActionTriggered];
            }
            HGAppearanceSet(@"player.headerStyle",@"flat",nil);
            Layout(header);
            UISegmentedControl *flat=(id)Find(header,material);
            Check([flat isKindOfClass:UISegmentedControl.class] && flat.selectedSegmentIndex==0 && !group.window,
                @"returning to flat restores the stock UIKit control and preserves the selected channel");
        }
    }
    [header applySearchIconVisible:NO tabContainerTop:0];
    Check(header.sideBar.hidden && search.alpha==0 && search.searchButtonSlotView.hidden,
        @"native visibility changes outside a channel transition still hide the home tools");
    [header applySearchIconVisible:YES tabContainerTop:0];
    listContainer.currentIndex=2;
    pageScroll.contentOffset=CGPointMake(pageWidth*2,0);
    NSUInteger beforeRepair=channels.nativeSelections;
    [listContainer scrollViewDidEndDecelerating:pageScroll];
    CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.02,false);
    Check(channels.selectedIndex==1 && pageScroll.contentOffset.x==pageWidth && listContainer.currentIndex==1 &&
        channels.nativeSelections==beforeRepair && listContainer.nativeSelectionRepairs==1,
        @"a native deceleration at a hidden physical page restores the visible selection without another tab action");
    [header removeFromSuperview];
    HGAppearanceProcessView(header);
    BOOL restoredPan=pageScroll.panGestureRecognizer.enabled && filterPan.view==nil;
    channels.selectedIndex=2;
    pageScroll.contentOffset=CGPointMake(pageWidth*2,0);
    [root addSubview:header];Layout(header);Layout(categoryList);
    CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.02,false);
    for (UIGestureRecognizer *gesture in pageScroll.gestureRecognizers)
        if ([NSStringFromClass(gesture.class) isEqual:@"HGAFixedChannelPan"]) filterPan=(id)gesture;
    Check(restoredPan && filterPan.view==pageScroll && !pageScroll.panGestureRecognizer.enabled &&
        channels.selectedIndex==1 && pageScroll.contentOffset.x==pageWidth && !underline.hidden &&
        fabs(underline.center.x-CGRectGetMidX([channels getTargetCellFrame:1]))<0.5,
        @"leaving restores the host pan and re-entering repairs a stale hidden channel and underline");
    HGAppearanceSet(@"player.headerGlass",@NO,nil);
    Layout(header);
    Check(filterPan.view==pageScroll && !Find(header,material) && !categoryList.hidden,
        @"disabling header glass restores the host titles and preserves filtered channel swipes");
    HGAppearanceSet(@"player.headerGlass",@YES,nil);
    CGPoint displaced = CGPointMake(CGRectGetMidX(comic)+31,underline.center.y);
    underline.center = displaced;
    CABasicAnimation *movingLine = [CABasicAnimation animationWithKeyPath:@"position.x"];
    movingLine.fromValue = @(displaced.x); movingLine.toValue = @(CGRectGetMidX(comic));
    movingLine.duration = 10;
    [underline.layer addAnimation:movingLine forKey:@"fixtureNativeScroll"];
    Layout(header);
    BOOL hostAnimationOwned = fabs(underline.center.x-displaced.x)<0.5;
    [underline.layer removeAnimationForKey:@"fixtureNativeScroll"];
    Layout(header);
    Check(hostAnimationOwned && fabs(underline.center.x-CGRectGetMidX([channels getTargetCellFrame:1]))<0.5,
        @"a later stationary layout realigns the selected line after leaving its host animation untouched");
    CGRect originalBounds=categoryList.bounds;
    categoryList.bounds=CGRectMake(0,0,300,44); Layout(header); Layout(categoryList);
    CGRect rotated=[categoryList layoutAttributesForItemAtIndexPath:comicPath].frame;
    HGReportChannelLayout(@"resized",channels,header,underline,Find(header,material));
    Check(fabs(underline.center.x-CGRectGetMidX(rotated))<0.5 && CGRectGetMidX(rotated)>CGRectGetMidX(comic)+10,
        @"resizing the category collection recenters the selected title and underline together");
    categoryList.bounds=originalBounds;
    HGAppearanceApply(@{@"layout.channels.hide.follow":@NO,@"layout.channels.hide.live":@NO},nil);
    Layout(header); Layout(categoryList);
    CGRect stock=[categoryList layoutAttributesForItemAtIndexPath:comicPath].frame;
    HGReportChannelLayout(@"stock",channels,header,underline,Find(header,material));
    Check(fabs(CGRectGetMidX(stock)-CGRectGetMidX([channels getTargetCellFrame:1]))<0.5 &&
        fabs(underline.center.x-CGRectGetMidX(stock))<0.5 &&
        !filterPan.view && pageScroll.panGestureRecognizer.enabled && pageScroll.gestureRecognizers.count==stockGestures &&
        CGAffineTransformIsIdentity(((UIView *)controls[2]).transform) &&
        CGAffineTransformIsIdentity(((UIView *)controls[4]).transform),
        @"restoring the four channels restores native layout, selection geometry and unmodified cell transforms");
    Check([episodePush shouldLeftSlideEnterOtherPage:nil context:swipeContext] && episodePush.calls==2,
        @"turning off channel filtering restores the interactive series push");
    HGAppearanceSet(@"player.disableEpisodeSwipe",@YES,nil);
    Check(![episodePush shouldLeftSlideEnterOtherPage:nil context:swipeContext] && episodePush.calls==2,
        @"the full episode swipe switch blocks the interactive push with the regular channel layout");
    HGAppearanceSet(@"player.disableEpisodeSwipe",@NO,nil);
    Check([episodePush shouldLeftSlideEnterOtherPage:nil context:swipeContext] && episodePush.calls==3,
        @"turning the full episode swipe switch back off immediately restores native swipe behavior");
    HGRunEpisodeSwipeDecisionChecks();
    [listContainer removeFromSuperview];
    [channels refreshSelectedCellModel:@3 unselectedCellModel:@1];

    NSMutableDictionary *allHidden=[NSMutableDictionary dictionary];
    for(NSString *key in keys)allHidden[[@"layout.channels.hide." stringByAppendingString:key]]=@YES;
    HGAppearanceApply(allHidden,nil);
    Check(header.hidden && !Find(header,material) && !Find(header,@"honggo.appearance.header.sidebar") &&
        !Find(header,@"honggo.appearance.header.search"),
        @"hiding all six native home controls also removes all header glass surfaces");
    HGAppearanceSet(@"layout.channels.hide.search",@NO,nil);
    searchGlassTop=Find(header,@"honggo.appearance.header.search");
    Check(!header.hidden && !Find(header,material) && !Find(header,@"honggo.appearance.header.sidebar") &&
        searchGlassTop && !search.searchButtonSlotView.hidden && header.sideBar.hidden && underline.hidden,
        @"hiding sidebar and channels leaves only the separated right circular search glass");
    right=[search.searchButtonSlotView convertRect:search.searchButtonSlotView.bounds toView:header];
    Check(fabs(CGRectGetWidth(searchGlassTop.frame)-CGRectGetHeight(searchGlassTop.frame))<0.5 &&
        fabs(CGRectGetMidX(searchGlassTop.frame)-CGRectGetMidX(right))<0.5 &&
        CGRectEqualToRect(search.frame,nativeSearch) && right.size.width==24,
        @"a lone search uses its own round glass surface around the unchanged native target");
    HGAppearanceSet(@"layout.channels.hide.follow",@NO,nil);
    Check(Find(header,material)!=nil && Find(header,@"honggo.appearance.header.search")!=nil,
        @"restoring a channel restores the separate center pill alongside the search circle");
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
        !Find(header,@"honggo.appearance.header.sidebar") && !Find(header,@"honggo.appearance.header.search") &&
        CGAffineTransformIsIdentity(header.sideBar.transform) && CGAffineTransformIsIdentity(search.searchButtonSlotView.transform),
        @"finder mode restores the shared search control and removes separated home circles");
    HGAppearanceSet(@"layout.channels.hide.search",@NO,nil);[header removeFromSuperview];

    FQTheaterViewController *finder=[HGAObservedTheaterFixture new];
    [host addChildViewController:finder];finder.view.frame=CGRectMake(0,140,280,90);
    [root addSubview:finder.view];[finder didMoveToParentViewController:host];
    finder.searchBgView=PageChromeControl(finder.view,CGRectMake(0,0,180,34));
    finder.searchView=PageChromeControl(finder.view,CGRectMake(5,8,18,18));
    finder.imageSearchContainer=PageChromeControl(finder.view,CGRectMake(167,0,50,34));
    finder.normalImageSearchView=[[HGAFinderRecognitionFixture alloc] initWithFrame:finder.imageSearchContainer.bounds];
    finder.normalImageSearchView.separatorView=PageChromeControl(finder.normalImageSearchView,CGRectMake(2,7,1,20));
    UIImage *cameraOriginal=[[UIImage systemImageNamed:@"camera"] imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
    UIImageView *camera=[[UIImageView alloc] initWithImage:cameraOriginal];
    camera.tintColor=UIColor.systemOrangeColor;
    [finder.normalImageSearchView addSubview:camera];finder.normalImageSearchView.cameraIconView=camera;
    [finder.imageSearchContainer addSubview:finder.normalImageSearchView];
    finder.aiSearchEntranceView=PageChromeControl(finder.view,CGRectMake(225,0,40,34));
    finder.aiSearchEntranceImageView=[[UIImageView alloc] initWithImage:[UIImage systemImageNamed:@"face.smiling"]];
    [finder.aiSearchEntranceView addSubview:finder.aiSearchEntranceImageView];
    CGRect nativeRecognition=finder.imageSearchContainer.frame,nativeSearchField=finder.searchBgView.frame;
    FQFeedTabFilterPanelEntranceView *filterShortcut=[[FQFeedTabFilterPanelEntranceView alloc] initWithFrame:CGRectMake(0,0,48,34)];
    filterShortcut.backgroundView=[[UIImageView alloc] initWithFrame:filterShortcut.bounds];
    [filterShortcut addSubview:filterShortcut.backgroundView];
    [finder.view addSubview:filterShortcut];
    Layout(filterShortcut);
    NSArray *labels=@[@"筛选",@"排行榜",@"新剧",@"预约",@"其他"];
    NSMutableArray *shortcuts=[NSMutableArray array];
    for(NSUInteger i=0;i<labels.count;i++) {
        SSBookStoreEntranceView *shortcut=[[SSBookStoreEntranceView alloc] initWithFrame:CGRectMake(i*55,44,52,34)];
        shortcut.bgImgView=[[UIImageView alloc] initWithImage:[UIImage systemImageNamed:@"rectangle.fill"]];
        shortcut.bgImgView.frame=shortcut.bounds;[shortcut addSubview:shortcut.bgImgView];
        shortcut.iconImgView=[[UIImageView alloc] initWithImage:cameraOriginal];
        shortcut.iconImgView.tintColor=UIColor.systemOrangeColor;
        [shortcut addSubview:shortcut.iconImgView];
        shortcut.nameLabel=[[UILabel alloc] initWithFrame:shortcut.bounds];shortcut.nameLabel.text=labels[i];
        [shortcut addSubview:shortcut.nameLabel];[finder.view addSubview:shortcut];[shortcuts addObject:shortcut];Layout(shortcut);
    }
    HGAppearanceProcessController(finder);
    UIVisualEffectView *searchGlass=(id)Find(finder.searchBgView,material);
    UIView *recognitionGlass=Find(finder.imageSearchContainer,material);
    UIView *assistantGlass=Find(finder.aiSearchEntranceView,material);
    CGRect searchSurface=[searchGlass convertRect:searchGlass.bounds toView:finder.view];
    CGRect recognitionSurface=[recognitionGlass convertRect:recognitionGlass.bounds toView:finder.view];
    Check(searchGlass && recognitionGlass && assistantGlass && !Find(finder.searchView,material) &&
        fabs(CGRectGetMinX(recognitionSurface)-CGRectGetMaxX(searchSurface)-7)<0.01 &&
        assistantGlass.bounds.size.width==assistantGlass.bounds.size.height &&
        CGRectEqualToRect(finder.searchBgView.frame,nativeSearchField) &&
        CGRectEqualToRect(finder.imageSearchContainer.frame,nativeRecognition) &&
        finder.normalImageSearchView.separatorView.hidden,
        @"finder search and recognition have separate glass with a seven-point gap and an independent round AI control");
    UIImageView *aiGlyph=(id)Find(finder.aiSearchEntranceView,@"honggo.appearance.finder.ai");
    UIColor *light=[camera.tintColor resolvedColorWithTraitCollection:[UITraitCollection traitCollectionWithUserInterfaceStyle:UIUserInterfaceStyleLight]];
    UIColor *dark=[aiGlyph.tintColor resolvedColorWithTraitCollection:[UITraitCollection traitCollectionWithUserInterfaceStyle:UIUserInterfaceStyleDark]];
    CGFloat lightValue=1,darkValue=0;
    [light getRed:&lightValue green:NULL blue:NULL alpha:NULL];
    [dark getRed:&darkValue green:NULL blue:NULL alpha:NULL];
    Check(camera.image.renderingMode==UIImageRenderingModeAlwaysTemplate && lightValue<0.01 && darkValue>0.99 &&
        finder.aiSearchEntranceImageView.hidden && !aiGlyph.userInteractionEnabled,
        @"finder camera and independent AI glyph take light and dark ink without intercepting the native press");
    HGAppearanceSet(@"player.finderAI",@NO,nil);
    HGAppearanceProcessController(finder);
    CGRect moved=finder.imageSearchContainer.frame;
    HGAppearanceProcessController(finder);
    UIView *recognitionHit=[finder.view hitTest:CGPointMake(CGRectGetMidX(moved),CGRectGetMidY(moved)) withEvent:nil];
    Check(finder.aiSearchEntranceView.hidden && !Find(finder.aiSearchEntranceView,material) &&
        fabs(CGRectGetMaxX(moved)-CGRectGetMaxX(finder.aiSearchEntranceView.frame))<0.01 &&
        CGRectEqualToRect(moved,finder.imageSearchContainer.frame) &&
        (recognitionHit==finder.imageSearchContainer || [recognitionHit isDescendantOfView:finder.imageSearchContainer]) &&
        fabs(CGRectGetMaxX(searchGlass.frame)-CGRectGetMinX(moved)+7)<0.01,
        @"hiding AI shifts native recognition to the vacated slot and fills the search gap without cumulative movement");
    HGAppearanceSet(@"player.finderAI",@YES,nil);
    HGAppearanceProcessController(finder);
    Check(!finder.aiSearchEntranceView.hidden && Find(finder.aiSearchEntranceView,material) &&
        CGRectEqualToRect(finder.imageSearchContainer.frame,nativeRecognition),
        @"showing AI restores the original recognition layout and native entrance");
    finder.imageSearchContainer.hidden=YES;
    HGAppearanceProcessController(finder);
    Check(CGRectEqualToRect(searchGlass.frame,finder.searchBgView.bounds) &&
        !Find(finder.imageSearchContainer,material) && !finder.normalImageSearchView.separatorView.hidden,
        @"finder restores the full search surface when its recognition tool is unavailable");
    finder.imageSearchContainer.hidden=NO;
    HGAppearanceProcessController(finder);
    BOOL scoped=Find(filterShortcut,material)!=nil && filterShortcut.backgroundView.hidden;
    for(NSUInteger i=0;i<shortcuts.count;i++) {
        SSBookStoreEntranceView *shortcut=shortcuts[i];
        scoped&=(Find(shortcut,material)!=nil)==(i<4) && shortcut.bgImgView.hidden==(i<4) && !shortcut.nameLabel.hidden &&
            shortcut.iconImgView.image==cameraOriginal && [shortcut.iconImgView.tintColor isEqual:UIColor.systemOrangeColor];
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
    reused.model=@"其他";
    BOOL lateBinding=!currentCover.hidden && !Find(reused,material);
    reused.model=@"排行榜";
    Check(lateBinding && currentCover.hidden && Find(reused,material),
        @"reused catalogue shortcut reconciles its host paint when the model changes without a layout pass");
    NSUInteger getterCalls=reused.backgroundGetterCalls;
    HGAppearanceSet(@"player.finderGlass",@NO,nil);
    scoped&=reused.backgroundGetterCalls==getterCalls && !Find(finder.view,material) && !filterShortcut.backgroundView.hidden &&
        !finder.normalImageSearchView.separatorView.hidden &&
        [finder.searchBgView.backgroundColor isEqual:UIColor.systemGrayColor] && camera.image==cameraOriginal &&
        [camera.tintColor isEqual:UIColor.systemOrangeColor] && !finder.aiSearchEntranceImageView.hidden && !aiGlyph.superview;
    for(SSBookStoreEntranceView *shortcut in shortcuts)scoped&=!shortcut.bgImgView.hidden && shortcut.bgImgView.image!=nil;
    Check(scoped,@"finder glass keeps the four shortcut icons' native colors and restores search backgrounds on disable");
    HGAppearanceSet(@"player.finderAI",@NO,nil);
    HGAppearanceProcessController(finder);
    Check(finder.aiSearchEntranceView.hidden && !Find(finder.view,material) &&
        CGRectEqualToRect(moved,finder.imageSearchContainer.frame),
        @"finder AI visibility and recognition reflow also work without finder glass");
    HGAppearanceSet(@"player.finderAI",@YES,nil);
    HGAppearanceSet(@"player.finderGlass",@YES,nil);
    finder.view.frame=CGRectMake(0,140,440,180);
    finder.searchBgView.frame=CGRectMake(12,0,368,38);
    finder.aiSearchEntranceView.frame=CGRectMake(392,1,36,36);
    [finder.searchBgView addSubview:finder.imageSearchContainer];
    finder.imageSearchContainer.frame=CGRectMake(297.6666667,0,58.3333333,38);
    finder.normalImageSearchView.frame=CGRectMake(0,7.3333333,58.3333333,23.6666667);
    camera.frame=CGRectMake(12.3333333,3.6666667,16,16);
    UILabel *recognitionText=[[UILabel alloc] initWithFrame:CGRectMake(30.3333333,2,28,19.6666667)];
    finder.normalImageSearchView.textLabel=recognitionText;
    [finder.normalImageSearchView addSubview:recognitionText];
    finder.tabContainer=PageChromeControl(finder.view,CGRectMake(0,42,440,30));
    FQRJXCategoryTitleCell *novels=PageChromeTitle(finder.tabContainer,@"小说",10);
    FQRJXCategoryTitleCell *films=PageChromeTitle(finder.tabContainer,@"电影",64);
    HGAppearanceProcessController(finder);
    searchGlass=(id)Find(finder.searchBgView,material);
    recognitionGlass=Find(finder.imageSearchContainer,material);
    assistantGlass=Find(finder.aiSearchEntranceView,material);
    CGRect ink=CGRectUnion([recognitionText convertRect:recognitionText.bounds toView:finder.imageSearchContainer],
        [camera convertRect:camera.bounds toView:finder.imageSearchContainer]);
    CGRect recognizerRect=[finder.imageSearchContainer convertRect:finder.imageSearchContainer.bounds toView:finder.view];
    searchSurface=[searchGlass convertRect:searchGlass.bounds toView:finder.view];
    recognitionSurface=[recognitionGlass convertRect:recognitionGlass.bounds toView:finder.view];
    CGRect aiSurface=[assistantGlass convertRect:assistantGlass.bounds toView:finder.view];
    Check(searchGlass && recognitionGlass && assistantGlass &&
        fabs(finder.aiSearchEntranceView.frame.origin.x-CGRectGetMaxX(recognizerRect)-12)<0.01 &&
        fabs(CGRectGetMinX(aiSurface)-CGRectGetMaxX(recognitionSurface)-
            (CGRectGetMinX(recognitionSurface)-CGRectGetMaxX(searchSurface)))<0.01 &&
        fabs(CGRectGetMidY(searchSurface)-CGRectGetMidY(recognitionSurface))<0.01 &&
        fabs(CGRectGetMidY(aiSurface)-CGRectGetMidY(recognitionSurface))<0.01 &&
        CGRectGetHeight(searchSurface)==36 && CGRectGetHeight(recognitionSurface)==36 && CGRectGetHeight(aiSurface)==36,
        @"the nested theatre search, recognition and AI surfaces have equal gaps and aligned heights");
    Check(recognitionGlass && CGRectGetMaxX(ink)<CGRectGetMaxX(recognitionGlass.frame)-5 &&
        fabs(CGRectGetMidX(ink)-CGRectGetMidX(recognitionGlass.frame))<0.1,
        @"the nested theatre centers camera and recognition text inside the capsule");
    HGAppearanceSet(@"finder.hide.category.novels",@YES,nil);
    HGAppearanceSet(@"finder.hide.shortcut.filter",@YES,nil);
    Check(novels.hidden && !films.hidden && filterShortcut.hidden && !((UIView *)shortcuts[1]).hidden,
        @"finder category and shortcut hide switches affect only their selected items");
    SSVideoDistributionGoldenPositionCell *golden=[[SSVideoDistributionGoldenPositionCell alloc] initWithFrame:CGRectMake(12,100,416,32)];
    UICollectionViewLayout *goldenLayout=[UICollectionViewFlowLayout new];
    golden.collectionView=[[UICollectionView alloc] initWithFrame:golden.bounds collectionViewLayout:goldenLayout];
    [golden.contentView addSubview:golden.collectionView];[finder.view addSubview:golden];
    HGAFinderGoldenItemFixture *goldenFilter=[HGAFinderGoldenItemFixture new];goldenFilter.text=@"筛选";
    HGAFinderGoldenItemFixture *goldenRank=[HGAFinderGoldenItemFixture new];goldenRank.text=@"排行榜";
    golden.model=[HGAFinderGoldenModelFixture new];golden.model.goldenLineItemArr=@[goldenFilter,goldenRank];
    SSVideoDistributionGoldenPositionSubCell *entry=[[SSVideoDistributionGoldenPositionSubCell alloc]
        initWithFrame:CGRectMake(110,0,98,32)];
    entry.innerContentView=PageChromeControl(entry.contentView,entry.contentView.bounds);
    entry.backgroundImageView=[[UIImageView alloc] initWithFrame:entry.innerContentView.bounds];
    [entry.innerContentView addSubview:entry.backgroundImageView];
    entry.borderView=PageChromeControl(entry.innerContentView,entry.innerContentView.bounds);
    entry.iconImgView=[[UIImageView alloc] initWithImage:cameraOriginal];
    [entry.innerContentView addSubview:entry.iconImgView];
    entry.titleLabel=[[UILabel alloc] initWithFrame:CGRectMake(28,0,64,32)];
    [entry.innerContentView addSubview:entry.titleLabel];
    [golden.contentView addSubview:entry];
    [entry setData:goldenRank styleConfig:nil];
    [entry onDarkModeChange:YES];
    UIVisualEffectView *entryGlass=(id)Find(entry.innerContentView,material);
    Layout(entryGlass);
    Check(entryGlass && entryGlass.frame.size.height==32 && entryGlass.layer.cornerRadius==16 &&
        !entryGlass.userInteractionEnabled && entry.backgroundImageView.hidden && entry.borderView.hidden &&
        [entry.backgroundColor isEqual:UIColor.clearColor] && [entry.contentView.backgroundColor isEqual:UIColor.clearColor] &&
        [entry.innerContentView.backgroundColor isEqual:UIColor.clearColor] && entry.iconImgView.image==cameraOriginal,
        @"the real theatre shortcut cell exposes its glass after model and theme changes and preserves its raster icon");
    HGAppearanceSet(@"player.finderGlass",@NO,nil);
    Check(!Find(entry,material) && !entry.backgroundImageView.hidden && !entry.borderView.hidden &&
        [entry.backgroundColor isEqual:UIColor.systemGrayColor] && [entry.contentView.backgroundColor isEqual:UIColor.systemGrayColor],
        @"disabling theatre glass restores the last native shortcut fill and outline");
    HGAppearanceSet(@"player.finderGlass",@YES,nil);
    Layout(golden);
    NSIndexPath *filterIndex=[NSIndexPath indexPathForItem:0 inSection:0];
    NSIndexPath *rankIndex=[NSIndexPath indexPathForItem:1 inSection:0];
    Check([golden collectionView:golden.collectionView layout:goldenLayout sizeForItemAtIndexPath:filterIndex].width<0.1 &&
        [golden collectionView:golden.collectionView layout:goldenLayout sizeForItemAtIndexPath:rankIndex].width==98,
        @"the observed theatre shortcut row reclaims the hidden item without renumbering native actions");
    HGAppearanceSet(@"finder.hide.category.row",@YES,nil);
    Check(finder.tabContainer.hidden && !finder.searchBgView.hidden,@"finder hides its whole stock category row without affecting search");
    HGAppearanceSet(@"finder.hide.category.row",@NO,nil);
    HGAppearanceSet(@"finder.hide.category.novels",@NO,nil);
    HGAppearanceSet(@"finder.hide.shortcut.filter",@NO,nil);
    Check(!finder.tabContainer.hidden && !novels.hidden && !filterShortcut.hidden &&
        [golden collectionView:golden.collectionView layout:goldenLayout sizeForItemAtIndexPath:filterIndex].width==98,
        @"restoring finder switches reuses the original category and shortcut controls");
    [finder willMoveToParentViewController:nil];[finder.view removeFromSuperview];[finder removeFromParentViewController];

    SSMinePageKMPFunctionDepend *functions=[SSMinePageKMPFunctionDepend new];
    NSArray *mineNames=@[@"商城",@"消息",@"游戏",@"追更",@"更多\n功能",@"其他"];
    NSMutableArray *items=[NSMutableArray array];
    for(NSString *title in mineNames) {
        HGAMineShortcutFixture *item=[HGAMineShortcutFixture new];item.title=title;[items addObject:item];
    }
    functions.items=items;
    HGAppearanceSet(@"mine.hide.shortcut.games",@YES,nil);
    HGAppearanceSet(@"mine.hide.shortcut.more",@YES,nil);
    Check([functions.getFunctionSnapshot isEqual:@[items[0],items[1],items[3],items[5]]] && [functions.functionItems isEqual:items],
        @"Mine filters only the published navigation card snapshot and keeps its native route records");
    HGAppearanceSet(@"mine.hide.shortcut.games",@NO,nil);
    HGAppearanceSet(@"mine.hide.shortcut.more",@NO,nil);
    Check([functions.getFunctionSnapshot isEqual:items],@"Mine restores the original function card ordering");

    SSMine737KMPViewController *nativeMine=[SSMine737KMPViewController new];
    [host addChildViewController:nativeMine];
    nativeMine.view.frame=CGRectMake(0,0,440,150);[root addSubview:nativeMine.view];
    [nativeMine didMoveToParentViewController:host];
    nativeMine.minePageKMPHandle=[HGAMineTitleHandleFixture new];
    UIView *titleBar=PageChromeControl(nativeMine.view,CGRectMake(0,0,440,106));
    nativeMine.minePageKMPHandle.titleBarView=titleBar;
    Class rendererClass=HGAPageFixtureViewClass("DanceUICompose.ComposeRenderingUIViewImpl");
    Class glyphClass=HGAPageFixtureViewClass("DanceUI._InheritedView");
    UIView *renderer=[[rendererClass alloc] initWithFrame:titleBar.bounds];[titleBar addSubview:renderer];
    UIView *moon=[[glyphClass alloc] initWithFrame:CGRectMake(360,72,24,24)];[renderer addSubview:moon];
    UIView *menu=[[glyphClass alloc] initWithFrame:CGRectMake(400,72,24,24)];[renderer addSubview:menu];
    moon.clipsToBounds=YES;menu.clipsToBounds=YES;
    HGAppearanceProcessController(nativeMine);
    BOOL moonGlass=NO,menuGlass=NO;
    for(UIView *backing in titleBar.subviews) {
        UIView *glass=Find(backing,material);
        if(!glass || backing.userInteractionEnabled || glass.userInteractionEnabled)continue;
        CGRect frame=[glass convertRect:glass.bounds toView:titleBar];
        moonGlass|=CGRectEqualToRect(frame,CGRectMake(354,66,36,36));
        menuGlass|=CGRectEqualToRect(frame,CGRectMake(394,66,36,36));
    }
    Check(moonGlass && menuGlass && titleBar.subviews.lastObject==renderer &&
        moon.clipsToBounds && menu.clipsToBounds && CGRectEqualToRect(moon.frame,CGRectMake(360,72,24,24)) &&
        CGRectEqualToRect(menu.frame,CGRectMake(400,72,24,24)),
        @"Mine places equal 36pt glass behind both clipped title glyphs while keeping the native frames");
    HGAppearanceSet(@"player.mineGlass",@NO,nil);
    Check(!Find(nativeMine.view,material),@"Mine's title-bar materials restore together with the appearance switch");
    HGAppearanceSet(@"player.mineGlass",@YES,nil);
    [nativeMine willMoveToParentViewController:nil];[nativeMine.view removeFromSuperview];[nativeMine removeFromParentViewController];

    SSMyUser637TabsView *mine=[[SSMyUser637TabsView alloc] initWithFrame:CGRectMake(0,230,290,80)];
    [root addSubview:mine];mine.searchBtn=[[UIButton alloc] initWithFrame:CGRectMake(255,7,25,25)];
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
    UIView *mineSearchGlass=Find(mine.searchBtn,material);
    BOOL history=mineSearchGlass && CGAffineTransformIsIdentity(mine.searchBtn.transform) &&
        mineSearchGlass.frame.size.height==30 && mineSearchGlass.frame.size.width==30 &&
        Find(filter,material) && filter.backgroundView.hidden && Find(edit,material) &&
        Find(all,material) && Find(watched,material) && Find(unwatched,material);
    HGAppearanceSet(@"player.mineGlass",@NO,nil);
    Check(history && !Find(mine,material) && !filter.backgroundView.hidden,
        @"Mine chrome follows native history controls and replaces and restores the filter background image");
    HGAppearanceSet(@"player.mineGlass",@YES,nil);[mine removeFromSuperview];
}
