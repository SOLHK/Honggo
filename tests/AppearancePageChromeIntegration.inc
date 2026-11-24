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

@interface SSHGBookStoreCategoryTitleView : FQRJXCategoryTitleView <UICollectionViewDataSource,UICollectionViewDelegateFlowLayout>
@property(nonatomic,strong) UICollectionView *collectionView;
@property(nonatomic,strong) UIScrollView *contentScrollView;
@property(nonatomic) NSInteger lastNativeTap;
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
@interface HGAFinderRecognitionFixture : UIView
@property(nonatomic,strong) UIView *separatorView;
@end
@implementation HGAFinderRecognitionFixture @end
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
@property(nonatomic,strong) HGAFinderRecognitionFixture *normalImageSearchView;
@property(nonatomic,strong) UIView *aiSearchEntranceView;
@end
@implementation FQTheaterViewController @end
@interface HGAObservedTheaterFixture : FQTheaterViewController @end
@implementation HGAObservedTheaterFixture @end
@interface SSBookStoreEntranceView : UIView
@property(nonatomic,strong) UILabel *nameLabel;
@property(nonatomic,strong) UIImageView *bgImgView;
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
        CGAffineTransformIsIdentity(((UIView *)controls[2]).transform) &&
        CGAffineTransformIsIdentity(((UIView *)controls[4]).transform),
        @"restoring the four channels restores native layout, selection geometry and unmodified cell transforms");
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
    [finder.imageSearchContainer addSubview:finder.normalImageSearchView];
    finder.aiSearchEntranceView=PageChromeControl(finder.view,CGRectMake(225,0,40,34));
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
        fabs(CGRectGetMinX(recognitionSurface)-CGRectGetMaxX(searchSurface)-6)<0.01 &&
        assistantGlass.bounds.size.width==assistantGlass.bounds.size.height &&
        CGRectEqualToRect(finder.searchBgView.frame,nativeSearchField) &&
        CGRectEqualToRect(finder.imageSearchContainer.frame,nativeRecognition) &&
        finder.normalImageSearchView.separatorView.hidden,
        @"finder search and recognition have separate glass with a six-point gap and an independent round AI control");
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
    reused.model=@"其他";
    BOOL lateBinding=!currentCover.hidden && !Find(reused,material);
    reused.model=@"排行榜";
    Check(lateBinding && currentCover.hidden && Find(reused,material),
        @"reused catalogue shortcut reconciles its host paint when the model changes without a layout pass");
    NSUInteger getterCalls=reused.backgroundGetterCalls;
    HGAppearanceSet(@"player.finderGlass",@NO,nil);
    scoped&=reused.backgroundGetterCalls==getterCalls && !Find(finder.view,material) && !filterShortcut.backgroundView.hidden &&
        !finder.normalImageSearchView.separatorView.hidden &&
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
