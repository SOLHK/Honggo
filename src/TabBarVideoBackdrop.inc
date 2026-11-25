// The vertical feed's native height is also its paging stride. The full-screen
// window can clip the top, sides and newly exposed bottom while a realized cell
// straddles the feed edge. Let UIKit draw that overflow in place.

@interface HGVideoBackdrop : NSObject
- (void)refreshWithBranches:(NSArray<UIView *> *)branches root:(UIView *)root
                       bar:(UIView *)bar active:(BOOL)active;
- (void)restore;
@end

@interface HGVideoBackdropLink : NSObject
@property(nonatomic, weak) HGVideoBackdrop *owner;
@end
@implementation HGVideoBackdropLink @end
static char HGVideoBackdropLinkKey;

typedef struct {
    CGRect frame;
    CGRect appliedFrame;
    UIViewContentMode contentMode;
} HGVideoPortraitCover;

@interface HGVideoBackdrop ()
@property(nonatomic, weak) UICollectionView *feed;
@property(nonatomic, weak) UIView *root;
@property(nonatomic, weak) UIView *bar;
@property(nonatomic, weak) UIViewController *owner;
@property(nonatomic, strong) UIView *underlay;
@property(nonatomic, strong) NSArray<CALayer *> *pagingMattes;
@property(nonatomic, strong) NSMapTable<UIView *, NSValue *> *portraitRenderers;
@property(nonatomic, strong) NSMapTable<UIImageView *, NSValue *> *portraitCovers;
@property(nonatomic, strong) NSHashTable<UIView *> *portraitClips;
@property(nonatomic, strong) NSMapTable<CAGradientLayer *, NSNumber *> *portraitGradients;
@property(nonatomic, weak) UICollectionViewCell *frontPortraitCell;
@property(nonatomic, strong) NSMapTable<UIImageView *, NSNumber *> *cornerAlpha;
@property(nonatomic, weak) UIView *detailTray;
@property(nonatomic, strong) UIColor *detailTrayPaint;
@property(nonatomic) BOOL ownsClipping;
@property(nonatomic) BOOL refreshing;
- (void)refreshFeed;
- (void)prepareRendererTransfer:(UIView *)view;
- (void)refreshDetail:(UIView *)canvas;
@end

static BOOL HGVideoBackdropVisible(UIView *view, UIView *root) {
    if (!view.window) return NO;
    for (UIView *node = view; node; node = node.superview) {
        if (node.hidden || node.alpha < 0.99 ||
            !CGAffineTransformIsIdentity(node.transform) ||
            !CATransform3DIsIdentity(node.layer.transform)) return NO;
        if (node == root) return YES;
    }
    return NO;
}

static UIViewController *HGVideoBackdropOwner(UIView *view) {
    Class expected = NSClassFromString(@"FQVShortVideoListViewController");
    if (!expected) return nil;
    for (UIResponder *responder = view.nextResponder; responder; responder = responder.nextResponder) {
        if ([responder isKindOfClass:UIViewController.class])
            return [responder isKindOfClass:expected] ? (UIViewController *)responder : nil;
    }
    return nil;
}

static BOOL HGVideoBackdropHasTransition(UIViewController *owner) {
    for (UIViewController *controller = owner; controller; controller = controller.parentViewController)
        if (controller.transitionCoordinator || controller.isBeingDismissed || controller.isBeingPresented)
            return YES;
    return NO;
}

static BOOL HGVideoBackdropFullWindow(UIView *view, UIWindow *window) {
    CGRect rect = [view convertRect:view.bounds toView:window];
    CGRect bounds = window.bounds;
    return fabs(CGRectGetMinX(rect) - CGRectGetMinX(bounds)) <= 0.75 &&
        fabs(CGRectGetMaxX(rect) - CGRectGetMaxX(bounds)) <= 0.75 &&
        fabs(CGRectGetMinY(rect) - CGRectGetMinY(bounds)) <= 0.75 &&
        fabs(CGRectGetMaxY(rect) - CGRectGetMaxY(bounds)) <= 0.75;
}

static BOOL HGVideoBackdropFeedGeometry(UICollectionView *feed, UIViewController *owner,
                                        UIView *root, UIView *bar) {
    UIView *parent = feed.superview;
    UIWindow *window = root.window;
    UIView *ownerView = owner.viewIfLoaded;
    if (!window || !parent || !ownerView || (parent != ownerView && ![parent isDescendantOfView:ownerView]) ||
        window.bounds.size.height <= window.bounds.size.width || feed.layer.mask ||
        !HGVideoBackdropVisible(feed, window) || bar.window!=window ||
        !HGVideoBackdropFullWindow(parent, window) || !HGVideoBackdropFullWindow(root, window) ||
        HGVideoBackdropHasTransition(owner)) return NO;
    CGRect frame = feed.frame, full = parent.bounds;
    CGFloat uncovered = CGRectGetMaxY(full) - CGRectGetMaxY(frame);
    return uncovered >= -0.75 && uncovered <= 160 &&
        fabs(CGRectGetMinX(frame) - CGRectGetMinX(full)) <= 0.75 &&
        fabs(CGRectGetMinY(frame) - CGRectGetMinY(full)) <= 0.75 &&
        fabs(CGRectGetMaxX(frame) - CGRectGetMaxX(full)) <= 0.75;
}

static UICollectionViewCell *HGVideoBackdropCellAtY(UICollectionView *feed, CGFloat y, CGRect *edgeFrame) {
    CGRect viewport = feed.bounds;
    // Only inspect realized, attached cells. Asking UICollectionView for its
    // visible cells can synchronously trigger a layout during host recycling.
    for (UIView *child in feed.subviews) {
        if (![child isKindOfClass:UICollectionViewCell.class] || !HGVideoBackdropVisible(child, feed)) continue;
        UICollectionViewCell *cell = (UICollectionViewCell *)child;
        CGRect frame = [cell convertRect:cell.bounds toView:feed];
        if (CGRectGetMinY(frame) <= y && CGRectGetMaxY(frame) > y &&
            fabs(CGRectGetMinX(frame) - CGRectGetMinX(viewport)) <= 0.75 &&
            fabs(CGRectGetMaxX(frame) - CGRectGetMaxX(viewport)) <= 0.75) {
            *edgeFrame = frame;
            return cell;
        }
    }
    return nil;
}

static BOOL HGVideoBackdropCrossesBottom(UICollectionView *feed, CGRect cellFrame) {
    CGRect viewport = feed.bounds;
    CGFloat top = CGRectGetMinY(viewport);
    CGFloat bottom = CGRectGetMaxY(viewport);
    UIEdgeInsets insets = feed.adjustedContentInset;
    // Pull-to-refresh can move the first cell across the edge without a page
    // entering. Let the host keep its clip outside the regular scroll range.
    if (top < -insets.top - 0.75 || bottom > feed.contentSize.height + insets.bottom + 0.75)
        return NO;
    return CGRectGetMinY(cellFrame) > top + 0.75 && CGRectGetMaxY(cellFrame) > bottom + 0.75;
}

static UIView *HGVideoBackdropCanvas(UICollectionViewCell *cell, UICollectionView *feed, CGFloat y) {
    Class canvasClass = NSClassFromString(@"FQVShortVideoPlayerCellView");
    CGRect viewport = feed.bounds;
    for (UIView *canvas in cell.contentView.subviews) {
        if (![canvas isKindOfClass:canvasClass] || !HGVideoBackdropVisible(canvas, cell)) continue;
        CGRect frame = [canvas convertRect:canvas.bounds toView:feed];
        if (CGRectGetMinY(frame) > y || CGRectGetMaxY(frame) <= y ||
            fabs(CGRectGetMinX(frame) - CGRectGetMinX(viewport)) > 0.75 ||
            fabs(CGRectGetMaxX(frame) - CGRectGetMaxX(viewport)) > 0.75) continue;
        return canvas;
    }
    return nil;
}

static UIColor *HGVideoBackdropMatte(UIView *canvas) {
    // The host's refreshBgColor resolves its per-video palette on this view.
    UIColor *color = [canvas.backgroundColor resolvedColorWithTraitCollection:canvas.traitCollection];
    CGFloat alpha = 0;
    return [color getRed:NULL green:NULL blue:NULL alpha:&alpha] && alpha >= 0.99 ? color : nil;
}

static BOOL HGVideoBackdropHasLetterbox(UIView *canvas) {
    for (UIView *wrapper in canvas.subviews) {
        UIView *player=wrapper.subviews.firstObject;
        if (![player isKindOfClass:objc_lookUpClass("FQVPlayer")]) continue;
        for (UIView *renderer in player.subviews) {
            if (![renderer isKindOfClass:objc_lookUpClass("TTPlayerView")] || renderer.hidden) continue;
            CGRect frame=[renderer convertRect:renderer.bounds toView:canvas];
            return frame.size.width>frame.size.height && frame.size.height>0 &&
                CGRectGetMinY(frame)>CGRectGetMinY(canvas.bounds)+1 &&
                CGRectGetMaxY(frame)<CGRectGetMaxY(canvas.bounds)-1;
        }
    }
    return NO;
}

static BOOL HGVideoBackdropSameRect(CGRect a, CGRect b, CGFloat tolerance) {
    return fabs(CGRectGetMinX(a)-CGRectGetMinX(b)) <= tolerance &&
        fabs(CGRectGetMinY(a)-CGRectGetMinY(b)) <= tolerance &&
        fabs(CGRectGetMaxX(a)-CGRectGetMaxX(b)) <= tolerance &&
        fabs(CGRectGetMaxY(a)-CGRectGetMaxY(b)) <= tolerance;
}

// Measure the actual picture, which can extend beyond the native renderer
// frame. Scaling from the frame instead can zoom a tall source unnecessarily.
static CATransform3D HGVideoBackdropCoverTransform(CGSize picture, CGRect native, CGRect full) {
    CGFloat scale=MAX(full.size.width/picture.width,full.size.height/picture.height);
    CATransform3D transform=CATransform3DMakeScale(scale,scale,1);
    transform.m42=CGRectGetMidY(full)-CGRectGetMidY(native);
    return transform;
}

static UIView *HGVideoBackdropMetal(UIView *renderer, UIView *player, CGRect viewport) {
    if (HGAppearancePlayerIsTransferring(renderer) || renderer.subviews.count != 1 ||
        renderer.layer.sublayers.count != 1 ||
        !HGVideoBackdropSameRect([renderer convertRect:renderer.bounds toView:player],viewport,0.75) ||
        !CGPointEqualToPoint(renderer.layer.anchorPoint,CGPointMake(0.5,0.5))) return nil;
    UIView *metal=renderer.subviews.firstObject;
    if (![metal isKindOfClass:objc_lookUpClass("TTMetalView")] || renderer.layer.sublayers.firstObject!=metal.layer ||
        metal.bounds.size.width<=0 || metal.bounds.size.height<=metal.bounds.size.width ||
        fabs(metal.center.x-CGRectGetMidX(renderer.bounds))>0.75 ||
        fabs(metal.center.y-CGRectGetMidY(renderer.bounds))>0.75) return nil;
    for (UIView *node=metal; node; node=node.superview) {
        if (node.layer.mask || !CATransform3DIsIdentity(node.layer.transform) ||
            (node!=renderer && !CATransform3DIsIdentity(node.layer.sublayerTransform))) return nil;
        if (node==player) return metal;
    }
    return nil;
}

// The cell's model is available before its poster or Metal surface. Both
// source sizes are authored by the host, independent of the screen bounds.
static BOOL HGVideoBackdropPortraitModel(id model) {
    const char *keys[]={"shortVideoModel","videoInfo"};
    for(NSUInteger i=0;i<2;i++) {
        id info=HGReadObjectGetter(model,keys[i]);
        SEL width=sel_registerName("videoWidth"),height=sel_registerName("videoHeight");
        Method w=class_getInstanceMethod(object_getClass(info),width),h=class_getInstanceMethod(object_getClass(info),height);
        if(!w || !h || strcmp(method_getTypeEncoding(w),"d16@0:8") || strcmp(method_getTypeEncoding(h),"d16@0:8"))continue;
        double x=((double(*)(id,SEL))objc_msgSend)(info,width),y=((double(*)(id,SEL))objc_msgSend)(info,height);
        if(x>0 && y>0 && isfinite(x) && isfinite(y))return y>x;
    }
    return NO;
}
static BOOL HGVideoBackdropSingleList(UIViewController *owner) {
    id list=HGReadObjectGetter(owner,"listPlayer");
    SEL single=sel_registerName("isSingleSeriesList");
    Method method=class_getInstanceMethod(object_getClass(list),single);
    return method && !strcmp(method_getTypeEncoding(method),"B16@0:8") && ((BOOL(*)(id,SEL))objc_msgSend)(list,single);
}

static UIView *HGDetailVideoBackdropCanvas(UIView *view);

static UIView *HGVideoBackdropPlayer(UIView *canvas) {
    for (UIView *wrapper in canvas.subviews) {
        UIView *player=wrapper.subviews.firstObject;
        if (wrapper.class==UIView.class && wrapper.subviews.count==1 &&
            [player isKindOfClass:objc_lookUpClass("FQVPlayer")] &&
            HGVideoBackdropSameRect(wrapper.frame,canvas.bounds,0.75) &&
            HGVideoBackdropSameRect(player.frame,wrapper.bounds,0.75) &&
            !wrapper.layer.mask && CATransform3DIsIdentity(wrapper.layer.transform) &&
            CATransform3DIsIdentity(wrapper.layer.sublayerTransform)) return player;
    }
    return nil;
}

static CAGradientLayer *HGVideoBackdropBottomGradient(UIView *canvas, CGRect viewport) {
    Class controlsClass = NSClassFromString(@"FQVShortVideoVPlayerControlView");
    Class gradientClass = NSClassFromString(@"FQVShortVideoGradientLayerView");
    for (UIView *control in canvas.subviews) {
        if (![control isKindOfClass:controlsClass]) continue;
        for (UIView *view in control.subviews) {
            if (![view isKindOfClass:gradientClass]) continue;
            CGRect frame = [view convertRect:view.bounds toView:canvas];
            CGRect expected = CGRectMake(CGRectGetMinX(viewport),CGRectGetMaxY(viewport)-frame.size.height,
                viewport.size.width,frame.size.height);
            if (!HGVideoBackdropSameRect(frame,expected,0.75) ||
                view != HGReadObjectGetter(control,"bottomGradientView") ||
                ![view.layer isKindOfClass:CAGradientLayer.class]) continue;
            return (CAGradientLayer *)view.layer;
        }
    }
    return nil;
}

@implementation HGVideoBackdrop
- (void)dealloc {
    for (CALayer *layer in _pagingMattes) [layer removeFromSuperlayer];
    [self restorePortraitExcept:nil];
    [self bringPortraitForward:nil];
    [self restoreDetailTray];
    if (_ownsClipping) _feed.clipsToBounds = YES;
    [self restoreCornerImages];
    [_underlay removeFromSuperview];
}

- (void)restoreDetailTray {
    if ([self.detailTray.backgroundColor isEqual:UIColor.clearColor])
        self.detailTray.backgroundColor = self.detailTrayPaint;
    self.detailTray = nil;
    self.detailTrayPaint = nil;
}

- (void)suppressPortraitGradient:(CAGradientLayer *)layer active:(NSMutableSet *)active {
    // The bottom HUD gradient ends at the old viewport edge. The native
    // picture now covers that edge as well as the formerly reserved strip.
    if (!layer) return;
    [active addObject:layer];
    if (layer.opacity != 0) {
        if (!self.portraitGradients) self.portraitGradients=[NSMapTable weakToStrongObjectsMapTable];
        [self.portraitGradients setObject:@(layer.opacity) forKey:layer];
        layer.opacity=0;
    }
}

- (void)bringPortraitForward:(UICollectionViewCell *)cell {
    if (cell==self.frontPortraitCell && (!cell || cell.layer.zPosition==1)) return;
    [CATransaction begin]; [CATransaction setDisableActions:YES];
    if (self.frontPortraitCell.layer.zPosition==1) self.frontPortraitCell.layer.zPosition=0;
    self.frontPortraitCell=nil;
    if (cell && cell.layer.zPosition==0) {
        cell.layer.zPosition=1;
        self.frontPortraitCell=cell;
    }
    [CATransaction commit];
}

- (void)restoreCornerImages {
    for (UIImageView *image in self.cornerAlpha)
        if (image.alpha == 0) image.alpha = [[self.cornerAlpha objectForKey:image] doubleValue];
    [self.cornerAlpha removeAllObjects];
}

- (void)updateCornerImages:(NSArray<UIView *> *)siblings frame:(CGRect)frame {
    // The host mounts a pair of 12pt corner images as siblings of its feed
    // (FQVShortVideoListVCUIComponent setupCornerRadiusImageView). They stay
    // at the old bottom edge even when a live renderer or matching matte
    // now continues through the dock strip.
    CGRect leftFrame = CGRectMake(CGRectGetMinX(frame), CGRectGetMaxY(frame)-12, 12, 12);
    CGRect rightFrame = CGRectOffset(leftFrame, CGRectGetWidth(frame)-12, 0);
    UIImageView *left = nil, *right = nil;
    for (UIView *view in siblings) {
        if (![view isKindOfClass:UIImageView.class] || view.userInteractionEnabled) continue;
        if (HGVideoBackdropSameRect(view.frame,leftFrame,0.5)) {
            if (left) { [self restoreCornerImages]; return; }
            left = (UIImageView *)view;
        } else if (HGVideoBackdropSameRect(view.frame,rightFrame,0.5)) {
            if (right) { [self restoreCornerImages]; return; }
            right = (UIImageView *)view;
        }
    }
    if (!left || !right) { [self restoreCornerImages]; return; }
    for (UIImageView *image in self.cornerAlpha.keyEnumerator.allObjects) {
        if (image == left || image == right) continue;
        if (image.alpha == 0) image.alpha = [[self.cornerAlpha objectForKey:image] doubleValue];
        [self.cornerAlpha removeObjectForKey:image];
    }
    if (!self.cornerAlpha) self.cornerAlpha = [NSMapTable weakToStrongObjectsMapTable];
    for (UIImageView *image in @[left,right]) {
        if (image.alpha == 0) continue;
        [self.cornerAlpha setObject:@(image.alpha) forKey:image];
        image.alpha = 0;
    }
}

- (void)restoreClipping {
    BOOL owned = self.ownsClipping;
    self.ownsClipping = NO;
    if (owned && !self.feed.clipsToBounds) self.feed.clipsToBounds = YES;
}

- (void)restorePortraitExcept:(NSSet *)active {
    [CATransaction begin]; [CATransaction setDisableActions:YES];
    for (UIView *renderer in self.portraitRenderers.keyEnumerator.allObjects) {
        if ([active containsObject:renderer]) continue;
        CATransform3D applied=[[self.portraitRenderers objectForKey:renderer] CATransform3DValue];
        if (CATransform3DEqualToTransform(renderer.layer.sublayerTransform,applied))
            renderer.layer.sublayerTransform=CATransform3DIdentity;
        [self.portraitRenderers removeObjectForKey:renderer];
    }
    for (UIImageView *cover in self.portraitCovers.keyEnumerator.allObjects) {
        if ([active containsObject:cover]) continue;
        HGVideoPortraitCover saved;
        [[self.portraitCovers objectForKey:cover] getValue:&saved size:sizeof(saved)];
        if (CGRectEqualToRect(cover.frame,saved.appliedFrame)) cover.frame=saved.frame;
        if (cover.contentMode==UIViewContentModeScaleAspectFill) cover.contentMode=saved.contentMode;
        [self.portraitCovers removeObjectForKey:cover];
    }
    for (UIView *view in self.portraitClips.allObjects) {
        if ([active containsObject:view]) continue;
        if (!view.clipsToBounds) view.clipsToBounds=YES;
        [self.portraitClips removeObject:view];
    }
    for (CAGradientLayer *layer in self.portraitGradients.keyEnumerator.allObjects) {
        if ([active containsObject:layer]) continue;
        if (layer.opacity==0) layer.opacity=[[self.portraitGradients objectForKey:layer] floatValue];
        [self.portraitGradients removeObjectForKey:layer];
    }
    [CATransaction commit];
}

- (void)releasePortraitClippingFrom:(UIView *)view to:(UIView *)boundary active:(NSMutableSet *)active {
    for (UIView *node=view; node && node!=boundary; node=node.superview) {
        [active addObject:node];
        if (!node.clipsToBounds) continue;
        if (!self.portraitClips) self.portraitClips=[NSHashTable weakObjectsHashTable];
        [self.portraitClips addObject:node];
        node.clipsToBounds=NO;
    }
}

- (BOOL)fillPortraitInCanvas:(UIView *)canvas viewport:(CGRect)viewport full:(CGRect)full
                   boundary:(UIView *)boundary active:(NSMutableSet *)active {
    UIView *player=HGVideoBackdropPlayer(canvas);
    if (!player || CGRectIsEmpty(viewport) || CGRectIsEmpty(full)) return NO;
    for (UIView *node=player; node && node!=boundary; node=node.superview)
        if (node.layer.mask || !CATransform3DIsIdentity(node.layer.transform) ||
            !CATransform3DIsIdentity(node.layer.sublayerTransform)) return NO;
    for (UIView *renderer in player.subviews) {
        if (![renderer isKindOfClass:objc_lookUpClass("TTPlayerView")]) continue;
        // Leave native landscape playback, including its poster, to the player.
        if (HGAppearancePlayerIsTransferring(renderer) ||
            (!CGRectIsEmpty(renderer.frame) && renderer.bounds.size.width>=renderer.bounds.size.height)) return NO;
    }
    BOOL filled=NO;
    BOOL extends=CGRectGetMaxY(full)>CGRectGetMaxY(viewport)+0.75;
    [CATransaction begin]; [CATransaction setDisableActions:YES];
    for (UIView *renderer in player.subviews) {
        if (![renderer isKindOfClass:objc_lookUpClass("TTPlayerView")]) continue;
        UIView *metal=HGVideoBackdropMetal(renderer,player,viewport);
        if (!metal) continue;
        CATransform3D current=renderer.layer.sublayerTransform;
        NSValue *previous=[self.portraitRenderers objectForKey:renderer];
        if (!CATransform3DIsIdentity(current) &&
            !(previous && CATransform3DEqualToTransform(current,previous.CATransform3DValue))) continue;
        CATransform3D transform=HGVideoBackdropCoverTransform(metal.bounds.size,viewport,full);
        if (!self.portraitRenderers) self.portraitRenderers=[NSMapTable weakToStrongObjectsMapTable];
        if (!previous || !CATransform3DEqualToTransform(previous.CATransform3DValue,transform))
            [self.portraitRenderers setObject:[NSValue valueWithCATransform3D:transform] forKey:renderer];
        if (!CATransform3DEqualToTransform(current,transform)) renderer.layer.sublayerTransform=transform;
        [active addObject:renderer];
        if (extends) [self releasePortraitClippingFrom:renderer to:boundary active:active];
        else if ([self.portraitClips containsObject:renderer]) {
            renderer.clipsToBounds=YES;
            [self.portraitClips removeObject:renderer];
        }
        filled=YES;
    }

    UIImageView *cover=(id)HGView(player,"coverImageView");
    if ([cover isKindOfClass:UIImageView.class] && [cover isDescendantOfView:player] &&
        !cover.layer.mask && CGAffineTransformIsIdentity(cover.transform) &&
        (cover.image ? cover.image.size.height>cover.image.size.width :
            (filled || HGVideoBackdropPortraitModel(HGReadObjectGetter(canvas,"model"))))) {
        NSValue *old=[self.portraitCovers objectForKey:cover];
        HGVideoPortraitCover saved={cover.frame,CGRectZero,cover.contentMode};
        if (old) {
            [old getValue:&saved size:sizeof(saved)];
            if (!CGRectEqualToRect(cover.frame,saved.appliedFrame)) saved.frame=cover.frame;
            if (cover.contentMode!=UIViewContentModeScaleAspectFill) saved.contentMode=cover.contentMode;
        }
        CGRect nativeCover=[cover.superview convertRect:saved.frame toView:player];
        if (HGVideoBackdropSameRect(nativeCover,viewport,0.75) ||
            HGVideoBackdropSameRect(nativeCover,player.bounds,0.75)) {
            saved.appliedFrame=[player convertRect:full toView:cover.superview];
            if (!self.portraitCovers) self.portraitCovers=[NSMapTable weakToStrongObjectsMapTable];
            [self.portraitCovers setObject:[NSValue valueWithBytes:&saved objCType:@encode(HGVideoPortraitCover)] forKey:cover];
            if (cover.contentMode!=UIViewContentModeScaleAspectFill) cover.contentMode=UIViewContentModeScaleAspectFill;
            if (!CGRectEqualToRect(cover.frame,saved.appliedFrame)) cover.frame=saved.appliedFrame;
            [active addObject:cover];
            if (extends) [self releasePortraitClippingFrom:cover.superview to:boundary active:active];
            filled=YES;
        }
    }
    if (filled) [self suppressPortraitGradient:HGVideoBackdropBottomGradient(canvas,viewport) active:active];
    [CATransaction commit];
    return filled;
}

- (void)removeFeed {
    for (CALayer *layer in self.pagingMattes) [layer removeFromSuperlayer];
    self.pagingMattes=nil;
    UICollectionView *feed = self.feed;
    HGVideoBackdropLink *link = objc_getAssociatedObject(feed, &HGVideoBackdropLinkKey);
    if (link.owner == self)
        objc_setAssociatedObject(feed, &HGVideoBackdropLinkKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [self restorePortraitExcept:nil];
    [self bringPortraitForward:nil];
    [self restoreClipping];
    [self restoreCornerImages];
    [self restoreDetailTray];
    [self.underlay removeFromSuperview];
    self.underlay = nil;
    self.feed = nil; self.root = nil; self.bar = nil; self.owner = nil;
}

- (void)updateUnderlay:(UIColor *)color {
    UIView *parent = self.feed.superview;
    UIView *underlay = self.underlay;
    if (!color) {
        underlay.hidden = YES;
        return;
    }
    if (!underlay) {
        underlay = [UIView new];
        underlay.accessibilityIdentifier = @"honggo.video.matte";
        underlay.accessibilityElementsHidden = YES;
        underlay.userInteractionEnabled = NO;
        self.underlay = underlay;
    }
    CGRect frame = self.feed.frame;
    CGFloat edge = CGRectGetMaxY(frame);
    CGRect strip = CGRectMake(CGRectGetMinX(frame), edge, CGRectGetWidth(frame),
        CGRectGetMaxY(parent.bounds) - edge);
    if (!CGRectEqualToRect(underlay.frame, strip)) underlay.frame = strip;
    if (![underlay.backgroundColor isEqual:color]) underlay.backgroundColor = color;
    NSUInteger index = [parent.subviews indexOfObjectIdenticalTo:underlay];
    NSUInteger feedIndex = [parent.subviews indexOfObjectIdenticalTo:self.feed];
    if (index == NSNotFound || index >= feedIndex) [parent insertSubview:underlay belowSubview:self.feed];
    underlay.hidden = NO;
}

- (void)restore {
    if (self.refreshing) return;
    self.refreshing = YES;
    [self removeFeed];
    self.refreshing = NO;
}

// These two native pages carry different opaque palettes. Blend only their
// letterbox mattes as the boundary travels, leaving native media and HUD on top.
- (UIColor *)pagingMatteForCanvas:(UIView *)bottom frame:(CGRect)bottomFrame color:(UIColor *)color {
    UICollectionView *feed=self.feed;
    CGRect topFrame=CGRectNull;
    CGFloat topY=CGRectGetMinY(feed.bounds)+0.75;
    UIView *top=nil;
    if (color && HGVideoBackdropCrossesBottom(feed,bottomFrame) && HGVideoBackdropHasLetterbox(bottom)) {
        UICollectionViewCell *cell=HGVideoBackdropCellAtY(feed,topY,&topFrame);
        if (fabs(CGRectGetMaxY(topFrame)-CGRectGetMinY(bottomFrame))<=0.75)
            top=HGVideoBackdropCanvas(cell,feed,topY);
    }
    UIColor *start=HGVideoBackdropMatte(top);
    if (!start || !HGVideoBackdropHasLetterbox(top) || [start isEqual:color]) {
        for (CALayer *layer in self.pagingMattes) [layer removeFromSuperlayer];
        self.pagingMattes=nil;
        return color;
    }
    CGFloat r,g,b,endR,endG,endB;
    [start getRed:&r green:&g blue:&b alpha:NULL];
    [color getRed:&endR green:&endG blue:&endB alpha:NULL];
    CGFloat fraction=MIN(1,MAX(0,(CGRectGetMaxY(feed.bounds)-CGRectGetMinY(bottomFrame))/feed.bounds.size.height));
    UIColor *blend=[UIColor colorWithRed:r+(endR-r)*fraction green:g+(endG-g)*fraction
                                    blue:b+(endB-b)*fraction alpha:1];
    if (!self.pagingMattes) self.pagingMattes=@[[CALayer layer],[CALayer layer]];
    [CATransaction begin]; [CATransaction setDisableActions:YES];
    NSUInteger index=0;
    for (UIView *canvas in @[top,bottom]) {
        CALayer *layer=self.pagingMattes[index++];
        if (layer.superlayer!=canvas.layer) [canvas.layer insertSublayer:layer atIndex:0];
        layer.zPosition=-1;
        layer.frame=canvas.bounds;
        layer.backgroundColor=blend.CGColor;
    }
    [CATransaction commit];
    return blend;
}

- (void)updateFeed {
    UICollectionView *feed = self.feed;
    if (feed.window && feed.window==self.root.window && HGVideoBackdropHasTransition(self.owner)) return;
    if (!HGVideoBackdropFeedGeometry(feed, self.owner, self.root, self.bar)) {
        [self removeFeed];
        return;
    }

    CGRect cellFrame = CGRectNull;
    CGFloat edge=CGRectGetMaxY(feed.bounds)-0.75;
    UICollectionViewCell *cell = HGVideoBackdropCellAtY(feed,edge,&cellFrame);
    UIView *canvas = HGVideoBackdropCanvas(cell,feed,edge);
    // Keep the page stride native. Both attached pages use a fixed full
    // picture size before, during and after a swipe.
    CGFloat extra=MAX(0,CGRectGetMaxY(feed.superview.bounds)-CGRectGetMaxY(feed.frame));
    CGRect topFrame=CGRectNull;
    CGFloat top=CGRectGetMinY(feed.bounds)+0.75;
    UICollectionViewCell *topCell=HGVideoBackdropCellAtY(feed,top,&topFrame);
    NSMutableSet *active=[NSMutableSet set];
    BOOL topPortrait=NO;
    if (topCell && topCell!=cell && fabs(topFrame.size.height-feed.bounds.size.height)<=0.75) {
        UIView *topCanvas=HGVideoBackdropCanvas(topCell,feed,top);
        CGRect full=topCanvas.bounds;
        full.size.height+=extra;
        topPortrait=[self fillPortraitInCanvas:topCanvas viewport:topCanvas.bounds full:full boundary:feed active:active];
    }
    CGRect full=canvas.bounds;
    full.size.height+=extra;
    BOOL portrait=cell && fabs(cellFrame.size.height-feed.bounds.size.height)<=0.75 &&
        [self fillPortraitInCanvas:canvas viewport:canvas.bounds full:full boundary:feed active:active];
    [self restorePortraitExcept:active];
    UIColor *matte=!portrait && extra>0.75 ? HGVideoBackdropMatte(canvas) : nil;
    [self updateUnderlay:[self pagingMatteForCanvas:canvas frame:cellFrame color:matte]];

    BOOL overflows=extra>0.75 && (portrait || (cell && HGVideoBackdropCrossesBottom(feed,cellFrame)));
    // At rest the next prefetched cell can still be attached immediately
    // below the feed. Paint the page meeting the visible edge in front of it.
    [self bringPortraitForward:overflows && (portrait || topPortrait) ? cell : nil];
    if (overflows || matte) [self updateCornerImages:feed.superview.subviews frame:feed.frame];
    else [self restoreCornerImages];
    if (!overflows) {
        [self restoreClipping];
        return;
    }

    if (feed.clipsToBounds) {
        self.ownsClipping = YES;
        feed.clipsToBounds = NO;
    }
}

- (void)prepareRendererTransfer:(UIView *)view {
    NSArray<UIView *> *renderers=self.portraitRenderers.keyEnumerator.allObjects;
    NSArray<UIImageView *> *covers=self.portraitCovers.keyEnumerator.allObjects;
    NSMutableSet *keep=[NSMutableSet set];
    BOOL moving=NO;
    for (UIView *surface in [(renderers ?: @[]) arrayByAddingObjectsFromArray:covers ?: @[]]) {
        if (surface==view || [surface isDescendantOfView:view] || [view isDescendantOfView:surface]) {
            moving=YES;
            continue;
        }
        UIView *canvas=HGDetailVideoBackdropCanvas(surface);
        UIView *boundary=self.feed ?: canvas;
        for (UIView *node=surface; node && node!=boundary; node=node.superview) [keep addObject:node];
        for (CAGradientLayer *layer in self.portraitGradients) {
            UIView *gradient=(id)layer.delegate;
            if ([gradient isKindOfClass:UIView.class] && [gradient isDescendantOfView:canvas]) [keep addObject:layer];
        }
    }
    if (!moving) return;
    [self restorePortraitExcept:keep];
    if (!self.frontPortraitCell || ![keep containsObject:self.frontPortraitCell]) {
        [self restoreClipping];
        [self bringPortraitForward:nil];
    }
    [self restoreCornerImages];
    [self restoreDetailTray];
}

- (void)refreshDetail:(UIView *)canvas {
    if (self.refreshing) return;
    UIViewController *owner = HGVideoBackdropOwner(canvas);
    UIWindow *window = canvas.window;
    UIView *root=owner.viewIfLoaded;
    CGRect page=[canvas convertRect:canvas.bounds toView:root];
    // Both visible cells move vertically during a swipe. Validate the page
    // stride and its horizontal alignment, independently of its scroll offset.
    BOOL fullPage=window && root.window==window &&
        fabs(CGRectGetWidth(root.bounds)-CGRectGetWidth(window.bounds))<=0.75 &&
        fabs(CGRectGetHeight(root.bounds)-CGRectGetHeight(window.bounds))<=0.75 &&
        fabs(CGRectGetMinX(page)-CGRectGetMinX(root.bounds))<=0.75 &&
        fabs(CGRectGetWidth(page)-CGRectGetWidth(window.bounds))<=0.75 &&
        fabs(CGRectGetHeight(page)-CGRectGetHeight(window.bounds))<=0.75;
    if (!HGAppearanceEnabled() || !owner ||
        window.bounds.size.width >= window.bounds.size.height || !fullPage) {
        [self restore];
        return;
    }
    self.refreshing=YES;
    UIView *tray=nil, *corners=nil, *hud=nil;
    UIView *player=HGVideoBackdropPlayer(canvas);
    Class controlsClass = NSClassFromString(@"FQVShortVideoVPlayerSingleSeriesControlView");
    Class trayClass = NSClassFromString(@"FQVShortVideoPlayerBottomView");
    for (UIView *child in canvas.subviews) {
        if ([child isKindOfClass:controlsClass]) {
            hud=child;
            // The black strip is the reserved player viewport, including the
            // frames during which the host animates the controls' alpha.
            for (UIView *candidate in child.subviews)
                if ([candidate isKindOfClass:trayClass]) { tray = candidate; break; }
        }
    }
    CGRect bottom = tray ? [tray convertRect:tray.bounds toView:canvas] : CGRectZero;
    CGRect viewport = canvas.bounds;
    viewport.size.height -= bottom.size.height;
    id model=HGReadObjectGetter(canvas,"model");
    BOOL portrait=HGVideoBackdropPortraitModel(model);
    if(portrait && CGRectIsEmpty(bottom)) {
        // Before the tray is laid out, use the viewport published by the HUD,
        // or the same per-model frame calculator used to initialize the player.
        CGRect predicted=CGRectZero;
        SEL frame=sel_registerName("playerViewFrame");
        Method method=class_getInstanceMethod(object_getClass(hud),frame);
        if(method && !strcmp(method_getTypeEncoding(method),"{CGRect={CGPoint=dd}{CGSize=dd}}16@0:8"))
            predicted=((CGRect(*)(id,SEL))objc_msgSend)(hud,frame);
        if(CGRectIsEmpty(predicted)) {
            id component=HGReadObjectGetter(HGReadObjectGetter(owner,"listPlayer"),"playerComponent");
            frame=sel_registerName("videoPlayerViewFrameWithModel:");
            method=class_getInstanceMethod(object_getClass(component),frame);
            if(method && !strcmp(method_getTypeEncoding(method),"{CGRect={CGPoint=dd}{CGSize=dd}}24@0:8@16"))
                predicted=((CGRect(*)(id,SEL,id))objc_msgSend)(component,frame,model);
        }
        if(!CGRectIsEmpty(predicted) && fabs(CGRectGetMinY(predicted)-CGRectGetMinY(canvas.bounds))<=0.75 &&
           fabs(CGRectGetMinX(predicted)-CGRectGetMinX(canvas.bounds))<=0.75 &&
           fabs(CGRectGetWidth(predicted)-CGRectGetWidth(canvas.bounds))<=0.75) {
            viewport=predicted;
            bottom=CGRectMake(CGRectGetMinX(viewport),CGRectGetMaxY(viewport),viewport.size.width,
                CGRectGetMaxY(canvas.bounds)-CGRectGetMaxY(viewport));
        }
    }
    NSMutableSet *active=[NSMutableSet set];
    BOOL alignedTray=bottom.size.height>=44 && bottom.size.height<=110 &&
        HGVideoBackdropSameRect(bottom,CGRectMake(CGRectGetMinX(viewport),CGRectGetMaxY(viewport),
            viewport.size.width,bottom.size.height),0.75);
    BOOL fills=(hud || HGVideoBackdropSingleList(owner)) && (alignedTray || CGRectIsEmpty(bottom)) &&
        [self fillPortraitInCanvas:canvas viewport:viewport full:canvas.bounds boundary:canvas active:active];
    [self restorePortraitExcept:active];
    if (fills) {
        // The portrait detail player places its lower 12pt corner cutouts in
        // a separate holder at the original renderer edge.
        for (UIView *child in player.subviews) {
            if (child.class != UIView.class || child.subviews.count < 4 ||
                !HGVideoBackdropSameRect(child.frame,viewport,0.75)) continue;
            if (corners) { corners = nil; break; }
            corners = child;
        }
        [self updateCornerImages:corners.subviews frame:corners.bounds];
        if (self.detailTray!=tray || !alignedTray) [self restoreDetailTray];
        UIColor *paint = alignedTray ? tray.backgroundColor : nil;
        if (paint && ![paint isEqual:UIColor.clearColor]) {
            self.detailTray = tray;
            self.detailTrayPaint = paint;
            tray.backgroundColor = UIColor.clearColor;
        }
    } else {
        [self restoreCornerImages];
        [self restoreDetailTray];
    }
    self.refreshing = NO;
}

- (void)refreshFeed {
    if (self.refreshing) return;
    self.refreshing = YES;
    [self updateFeed];
    self.refreshing = NO;
}

- (void)refreshWithBranches:(NSArray<UIView *> *)branches root:(UIView *)root
                       bar:(UIView *)bar active:(BOOL)active {
    if (self.refreshing) return;
    if (!active || !root.window || bar.window!=root.window) {
        [self restore];
        return;
    }
    if (self.feed.window==root.window && HGVideoBackdropHasTransition(self.owner)) return;

    Class feedClass = NSClassFromString(@"FQVCollectionView");
    UICollectionView *candidate = nil;
    UIViewController *owner = nil;
    NSMutableArray<UIView *> *queue = [branches mutableCopy];
    for (NSUInteger i = 0; feedClass && i < queue.count && i < 192; i++) {
        UIView *view = queue[i];
        if (!HGVideoBackdropVisible(view, root) ||
            CGRectIsEmpty(CGRectIntersection([view convertRect:view.bounds toView:root], root.bounds))) continue;
        if ([view isKindOfClass:feedClass] && [view isKindOfClass:UICollectionView.class]) {
            UICollectionView *feed = (UICollectionView *)view;
            UIViewController *controller = HGVideoBackdropOwner(feed);
            if (!HGVideoBackdropFeedGeometry(feed, controller, root, bar)) continue;
            if (candidate && candidate != feed) {
                [self restore];
                return;
            }
            candidate = feed; owner = controller;
            continue;
        }
        if ([view isKindOfClass:UIVisualEffectView.class] || [view isKindOfClass:UIControl.class] ||
            [view isKindOfClass:UILabel.class] || [view isKindOfClass:UIImageView.class] ||
            [view isKindOfClass:UITableViewCell.class] || [view isKindOfClass:UICollectionViewCell.class]) continue;
        for (UIView *child in view.subviews) {
            if (queue.count >= 192) break;
            [queue addObject:child];
        }
    }
    if (!candidate) {
        [self restore];
        return;
    }

    self.refreshing = YES;
    if (self.feed != candidate) {
        [self removeFeed];
        HGVideoBackdropLink *link = objc_getAssociatedObject(candidate, &HGVideoBackdropLinkKey);
        if (link.owner) {
            self.refreshing = NO;
            return;
        }
        self.feed = candidate;
        link = [HGVideoBackdropLink new];
        link.owner = self;
        objc_setAssociatedObject(candidate, &HGVideoBackdropLinkKey, link, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        HGRecordCapability(@"appearance.dock.video-backdrop", @{
            @"module": @"appearance", @"state": @"installed", @"reason": @"native-feed-overflow",
            @"nativeGeometryPreserved": @YES, @"nativeHierarchyPreserved": @YES, @"mirrorsFrames": @NO
        });
    }
    self.root = root; self.bar = bar; self.owner = owner;
    [self updateFeed];
    self.refreshing = NO;
}
@end

BOOL HGVideoBackdropRefreshForFeed(UIScrollView *feed) {
    if (HGFeedSidebarOwnsLayout(feed.window)) return NO;
    HGVideoBackdropLink *link = objc_getAssociatedObject(feed, &HGVideoBackdropLinkKey);
    HGVideoBackdrop *owner = link.owner;
    if (!owner) return NO;
    if (HGAppearanceEnabled()) [owner refreshFeed];
    else [owner restore];
    return owner.feed == feed;
}

static UIScrollView *HGVideoBackdropPlayerFeed(UIView *view) {
    Class feedClass = NSClassFromString(@"FQVCollectionView");
    for (UIView *node = view.superview; node; node = node.superview)
        if ([node isKindOfClass:feedClass] && [node isKindOfClass:UICollectionView.class])
            return (UIScrollView *)node;
    return nil;
}

static char HGDetailVideoBackdropKey;
static UIView *HGDetailVideoBackdropCanvas(UIView *view) {
    Class canvasClass = NSClassFromString(@"FQVShortVideoPlayerCellView");
    for (UIView *node = view; node; node = node.superview)
        if ([node isKindOfClass:canvasClass]) return node;
    return nil;
}

void HGRefreshVideoBackdropForPlayer(UIView *view) {
    if (!NSThread.isMainThread) return;
    UIScrollView *feed = HGVideoBackdropPlayerFeed(view);
    UIView *canvas = HGDetailVideoBackdropCanvas(view);
    HGVideoBackdrop *detail = objc_getAssociatedObject(canvas, &HGDetailVideoBackdropKey);
    HGVideoBackdropLink *feedLink=objc_getAssociatedObject(feed,&HGVideoBackdropLinkKey);
    if (feedLink ||
        (feed.window && CGRectGetMaxY(feed.frame) < CGRectGetMaxY(feed.superview.bounds)-20)) {
        [detail restore];
        if(feedLink && !HGAppearancePlayerIsTransferring(view) &&
           ((view==canvas && HGVideoBackdropPortraitModel(HGReadObjectGetter(canvas,"model"))) ||
            [view isKindOfClass:objc_lookUpClass("TTPlayerView")] || [view isKindOfClass:objc_lookUpClass("FQVPlayer")]))
            HGVideoBackdropRefreshForFeed(feed);
        HGScheduleFeedLayoutRefresh(feed);
        return;
    }
    if (!canvas) return;
    if (!detail) {
        Class control = NSClassFromString(@"FQVShortVideoVPlayerSingleSeriesControlView");
        BOOL hasSingleSeriesControls = NO;
        for (UIView *child in canvas.subviews) hasSingleSeriesControls |= [child isKindOfClass:control];
        if (!HGAppearanceEnabled() ||
            (!hasSingleSeriesControls && !HGVideoBackdropSingleList(HGVideoBackdropOwner(canvas)))) return;
        detail = [HGVideoBackdrop new];
        objc_setAssociatedObject(canvas, &HGDetailVideoBackdropKey, detail, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    [detail refreshDetail:canvas];
}

void HGVideoBackdropPrepareRendererTransfer(UIView *view) {
    if (!NSThread.isMainThread) return;
    HGVideoBackdrop *detail = objc_getAssociatedObject(HGDetailVideoBackdropCanvas(view), &HGDetailVideoBackdropKey);
    [detail prepareRendererTransfer:view];
    UIScrollView *feed = HGVideoBackdropPlayerFeed(view);
    if (!feed) return;
    HGVideoBackdropLink *link = objc_getAssociatedObject(feed, &HGVideoBackdropLinkKey);
    [link.owner prepareRendererTransfer:view];
}
