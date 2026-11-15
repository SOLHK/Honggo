// R12 device hierarchy: Mine coordinator -> tabs -> horizontal category pager
// -> page -> second horizontal pager -> vertical list. Unlike the older clip
// fixture, these are actual UIScrollViews and taps can move them without an
// outer layout/delegate callback. Keep checks synchronous to expose stale masks.
- (void)runMinePagingChecks {
    SSMine737KMPViewController *owner = [SSMine737KMPViewController new];
    UIView *page = [[SSMine737KMPRootView alloc] initWithFrame:self.host.view.bounds];
    owner.view = page; [self.host addChildViewController:owner];
    CGFloat w = page.bounds.size.width, h = page.bounds.size.height;
    SSMyUser637NestedContainerScrollView *outer = [[SSMyUser637NestedContainerScrollView alloc]
        initWithFrame:CGRectMake(0, 0, w, h - 84)];
    outer.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentNever;
    outer.contentSize = CGSizeMake(w, h * 8); outer.clipsToBounds = YES;
    owner.scrollView = outer;
    HGClipTabsFixture *tabs = [[HGClipTabsFixture alloc] initWithFrame:CGRectMake(0, 310, w, h - 175)];
    tabs.clipsToBounds = YES; owner.bottomView = tabs;
    UIScrollView *pager = [[UIScrollView alloc] initWithFrame:CGRectMake(0, 50, w, h - 225)];
    pager.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentNever;
    pager.pagingEnabled = YES; pager.contentSize = CGSizeMake(w * 5, h - 225); pager.clipsToBounds = YES;
    NSMutableArray<UIScrollView *> *subpagers = [NSMutableArray array];
    NSMutableArray<UIScrollView *> *lists = [NSMutableArray array];
    for (NSUInteger section = 0; section < 2; section++) {
        UIView *sectionView = [[UIView alloc] initWithFrame:CGRectMake(section * w, 0, w, h - 225)];
        UIScrollView *subpager = [[UIScrollView alloc] initWithFrame:CGRectMake(0, 42, w, h - 267)];
        subpager.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentNever;
        // Exercise both native paging and a programmatically paged scroll view.
        subpager.pagingEnabled = section == 0;
        subpager.contentSize = CGSizeMake(w * 3, h - 267); subpager.clipsToBounds = YES;
        UIScrollView *list = [[UIScrollView alloc] initWithFrame:subpager.bounds];
        list.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentNever;
        // An empty guest list and a populated signed-in list use the same fix.
        list.contentSize = CGSizeMake(w, section ? 0 : h * 9);
        list.contentInset = UIEdgeInsetsMake(0, 0, 16, 0); list.clipsToBounds = YES;
        [subpager addSubview:list]; [sectionView addSubview:subpager]; [pager addSubview:sectionView];
        [subpagers addObject:subpager]; [lists addObject:list];
    }
    [tabs addSubview:pager]; [outer addSubview:tabs]; [page addSubview:outer];
    tabs.currentVCScrollView = lists[0];
    CAShapeLayer *existingMask = [CAShapeLayer layer]; existingMask.frame = pager.bounds;
    existingMask.path = [UIBezierPath bezierPathWithRect:existingMask.bounds].CGPath;
    pager.layer.mask = existingMask;
    [self.host.view insertSubview:page belowSubview:self.host.customTabBar];
    CGRect pagerFrame = pager.frame, outerFrame = outer.frame;
    CGSize pagerSize = pager.contentSize, outerSize = outer.contentSize;
    [outer layoutSubviews];
    [self check:pager.layer.mask == existingMask && pager.clipsToBounds
        name:@"an existing host pager mask is never replaced by Mine clipping"];
    pager.layer.mask = nil; [outer layoutSubviews];
    [self check:pager.layer.mask == nil && subpagers[0].layer.mask == nil &&
        !pager.clipsToBounds && !subpagers[0].clipsToBounds && outer.layer.mask && tabs.layer.mask
        name:@"Mine horizontal pagers use the stationary outer crop without a content-coordinate mask"];

    BOOL taps = YES;
    for (NSUInteger i = 0; i < 100; i++) {
        NSUInteger section = i % 2;
        tabs.currentVCScrollView = lists[section];
        // Deliberately no outer layout or Mine scroll delegate between taps.
        [pager setContentOffset:CGPointMake(section * w, 0) animated:YES];
        [subpagers[section] setContentOffset:CGPointMake((i % 3) * w, 0) animated:YES];
        taps = taps && pager.layer.mask == nil && subpagers[section].layer.mask == nil &&
            CGRectEqualToRect(pager.frame, pagerFrame) && CGSizeEqualToSize(pager.contentSize, pagerSize);
    }
    [self check:taps name:@"100 rapid animated History/Collection taps cannot leave a stale page-width mask"];

    BOOL intermediate = YES;
    for (NSUInteger i = 0; i <= 20; i++) {
        // Test every fractional page position without waiting for a timer or a
        // render pass that could conceal the original clipping failure.
        CGFloat x = w * i / 20.0;
        [pager setContentOffset:CGPointMake(x, 0) animated:NO];
        intermediate = intermediate && pager.layer.mask == nil &&
            fabs(pager.contentOffset.x - x) < 0.01 && outer.layer.mask != nil;
    }
    [self check:intermediate name:@"fractional pager offsets remain visible without forcing an outer layout"];

    BOOL structures = YES;
    for (NSUInteger section = 0; section < 2; section++) {
        [pager setContentOffset:CGPointMake(section * w, 0) animated:NO];
        [subpagers[section] setContentOffset:CGPointZero animated:NO];
        tabs.currentVCScrollView = lists[section]; [outer layoutSubviews];
        UIScrollView *list = lists[section];
        structures = structures && pager.layer.mask == nil && subpagers[section].layer.mask == nil &&
            !pager.clipsToBounds && !subpagers[section].clipsToBounds &&
            list.layer.mask != nil && list.contentInset.bottom == 16 &&
            CGRectEqualToRect(outer.frame, outerFrame) && CGSizeEqualToSize(outer.contentSize, outerSize) &&
            fabs(pager.contentOffset.x - section * w) < 0.01;
    }
    [self check:structures name:@"empty and populated Mine lists retain geometry in both nested pager structures"];

    CGSize emptyListSize = lists[1].contentSize;
    lists[1].pagingEnabled = YES; lists[1].contentSize = CGSizeMake(w, h * 9); [outer layoutSubviews];
    [self check:lists[1].layer.mask != nil && subpagers[1].layer.mask == nil
        name:@"a known vertical paged list retains its top crop while its horizontal parent stays mask-free"];
    lists[1].pagingEnabled = NO; lists[1].contentSize = emptyListSize;

    subpagers[1].contentSize = subpagers[1].bounds.size; [outer layoutSubviews];
    BOOL formerlyMasked = subpagers[1].layer.mask != nil;
    subpagers[1].contentSize = CGSizeMake(w * 3, h - 267); [outer layoutSubviews];
    [self check:formerlyMasked && subpagers[1].layer.mask == nil && !subpagers[1].clipsToBounds
        name:@"a list that becomes a pager sheds its old plugin mask without recapturing mutated clipping"];

    CAShapeLayer *hostMask = [CAShapeLayer layer]; hostMask.frame = subpagers[1].bounds;
    hostMask.path = [UIBezierPath bezierPathWithRect:hostMask.bounds].CGPath;
    subpagers[1].layer.mask = hostMask; [outer layoutSubviews];
    [self check:subpagers[1].layer.mask == hostMask && subpagers[1].clipsToBounds
        name:@"a host-installed pager mask is preserved and plugin clipping ownership is released"];
    subpagers[1].layer.mask = nil; [outer layoutSubviews];
    [self check:subpagers[1].layer.mask == nil && !subpagers[1].clipsToBounds
        name:@"removing the host mask returns to a mask-free horizontal viewport"];

    // Login/logout can temporarily remove the current leaf while the category
    // controls remain. Restore the old path immediately instead of retaining it.
    tabs.currentVCScrollView = nil; [outer layoutSubviews];
    [self check:pager.clipsToBounds && pager.layer.mask == nil && subpagers[1].clipsToBounds &&
        lists[1].clipsToBounds && lists[1].layer.mask == nil && outer.layer.mask
        name:@"replacing a Mine list restores the removed ancestor path without losing the outer crop"];
    tabs.currentVCScrollView = lists[0]; [pager setContentOffset:CGPointZero animated:NO];
    [subpagers[0] setContentOffset:CGPointZero animated:NO]; [outer layoutSubviews];
    [self check:!pager.clipsToBounds && pager.layer.mask == nil &&
        !subpagers[0].clipsToBounds && subpagers[0].layer.mask == nil
        name:@"a rebuilt Mine list reuses native pager navigation after login-state replacement"];

    CGRect shifted = page.frame; shifted.origin.x = -252; page.frame = shifted;
    CGPoint offset = pager.contentOffset; [outer layoutSubviews];
    [self check:CGRectEqualToRect(page.frame, shifted) && CGPointEqualToPoint(pager.contentOffset, offset) &&
        pager.layer.mask == nil && outer.layer.mask != nil
        name:@"the native 252-point Mine side drawer translation is not reset by paging repair"];
    [page removeFromSuperview];
    [self check:outer.clipsToBounds && outer.layer.mask == nil && tabs.clipsToBounds && tabs.layer.mask == nil &&
        pager.clipsToBounds && pager.layer.mask == nil && subpagers[0].clipsToBounds &&
        subpagers[0].layer.mask == nil && lists[0].clipsToBounds && lists[0].layer.mask == nil
        name:@"leaving Mine restores both stationary masks and horizontal pager clipping"];
    [owner removeFromParentViewController];
}
