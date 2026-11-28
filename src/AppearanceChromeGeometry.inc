#import "DockSelectionGeometry.h"

static UIColor *HGADockSelectionColor(UITraitCollection *traits,BOOL flat) {
    BOOL dark=traits.userInterfaceStyle==UIUserInterfaceStyleDark;
    BOOL strong=UIAccessibilityIsReduceTransparencyEnabled() || traits.accessibilityContrast==UIAccessibilityContrastHigh;
    CGFloat opacity=flat ? (dark?0.24:0.12) : (strong ? (dark?0.18:0.10) : (dark?0.09:0.065));
    return [UIColor colorWithWhite:dark?1:0 alpha:opacity];
}

@protocol HGAHeaderSelecting <NSObject>
@property(nonatomic) NSInteger selectedSegmentIndex;
@end

static const CGFloat HGAChannelTitleSize=16;
static const CGFloat HGAChannelRailHeight=38;

// Titles, hit targets and the selected surface share the same button frames.
API_AVAILABLE(ios(26.0))
@interface HGAHeaderGlassChannels : UIControl <HGAHeaderSelecting>
@property(nonatomic,copy) NSArray<UIButton *> *buttons;
@property(nonatomic,strong) UIView *railSurface;
@property(nonatomic,strong) HGAAppearanceMaterial *rail;
@property(nonatomic) HGASelectionColorMode selectionColor;
- (instancetype)initWithTitles:(NSArray<NSString *> *)titles material:(NSString *)material;
@end
@implementation HGAHeaderGlassChannels
@synthesize selectedSegmentIndex=_selectedSegmentIndex;
- (instancetype)initWithTitles:(NSArray<NSString *> *)titles material:(NSString *)material {
    if ((self=[super initWithFrame:CGRectZero])) {
        _selectedSegmentIndex=UISegmentedControlNoSegment;
        self.isAccessibilityElement=NO;
        _railSurface=[[UIView alloc] initWithFrame:self.bounds];
        _railSurface.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
        [self addSubview:_railSurface];
        _rail=[[HGAAppearanceMaterial alloc] initWithEffect:nil];
        BOOL flat=[material isEqual:@"flat"];
        _rail.style=flat ? @"solid" : material;
        _rail.untintedGlass=!flat;
        _rail.overlayColor=flat ? UIColor.tertiarySystemFillColor : UIColor.clearColor;
        _rail.layer.cornerCurve=kCACornerCurveContinuous;
        _rail.clipsToBounds=YES;
        _rail.accessibilityIdentifier=@"honggo.appearance.header.channelRail";
        _rail.accessibilityElementsHidden=YES;
        _rail.userInteractionEnabled=NO;
        _rail.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
        [_railSurface addSubview:_rail];
        UIStackView *row=[[UIStackView alloc] initWithFrame:self.bounds];
        row.distribution=UIStackViewDistributionFillEqually;
        row.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
        [self addSubview:row];
        NSMutableArray *buttons=[NSMutableArray arrayWithCapacity:titles.count];
        for (NSUInteger index=0;index<titles.count;index++) {
            UIButton *button=[UIButton buttonWithType:UIButtonTypeCustom];
            button.tag=(NSInteger)index;
            [button setTitle:titles[index] forState:UIControlStateNormal];
            [button setTitleColor:[UIColor.whiteColor colorWithAlphaComponent:0.66] forState:UIControlStateNormal];
            [button setTitleColor:[UIColor.whiteColor colorWithAlphaComponent:0.86] forState:UIControlStateHighlighted];
            [button setTitleColor:UIColor.whiteColor forState:UIControlStateSelected];
            button.titleLabel.font=[UIFont systemFontOfSize:HGAChannelTitleSize weight:UIFontWeightMedium];
            button.titleLabel.lineBreakMode=NSLineBreakByClipping;
            [button addTarget:self action:@selector(choose:) forControlEvents:UIControlEventPrimaryActionTriggered];
            [row addArrangedSubview:button];
            [buttons addObject:button];
        }
        _buttons=[buttons copy];
    }
    return self;
}
- (void)layoutSubviews {
    [super layoutSubviews];
    if(self.window && ![self.rail.style isEqual:@"solid"]) {
        if(self.rail.overrideUserInterfaceStyle!=UIUserInterfaceStyleDark)
            self.rail.overrideUserInterfaceStyle=UIUserInterfaceStyleDark;
    }
    CGFloat radius=CGRectGetHeight(self.rail.bounds)/2;
    self.rail.materialCornerRadius=radius;
    if (self.rail.layer.cornerRadius!=radius) self.rail.layer.cornerRadius=radius;
}
- (void)setSelectionColor:(HGASelectionColorMode)selectionColor {
    if (_selectionColor==selectionColor) return;
    _selectionColor=selectionColor;
    BOOL matte=selectionColor==HGASelectionColorSoftRoseMist;
    UIColor *selectedInk=matte ? HGASoftRoseMistInkColor() : UIColor.whiteColor;
    for (UIButton *button in self.buttons) {
        [button setTitleColor:selectedInk forState:UIControlStateSelected];
        [button setTitleColor:matte ? selectedInk : [UIColor.whiteColor colorWithAlphaComponent:0.86]
            forState:UIControlStateSelected|UIControlStateHighlighted];
    }
}
- (void)applySelection:(BOOL)selected toButton:(UIButton *)button {
    button.selected=selected;
    button.accessibilityTraits=UIAccessibilityTraitButton | (selected ? UIAccessibilityTraitSelected : 0);
    button.titleLabel.font=[UIFont systemFontOfSize:HGAChannelTitleSize weight:selected ? UIFontWeightSemibold : UIFontWeightMedium];
    BOOL matte=self.selectionColor==HGASelectionColorSoftRoseMist;
    UIColor *selectedInk=matte ? HGASoftRoseMistInkColor() : UIColor.whiteColor;
    [button setTitleColor:selectedInk forState:UIControlStateSelected];
    [button setTitleColor:matte ? selectedInk : [UIColor.whiteColor colorWithAlphaComponent:0.86]
        forState:UIControlStateSelected|UIControlStateHighlighted];
}
- (void)setSelectedSegmentIndex:(NSInteger)index {
    if (_selectedSegmentIndex==index) return;
    NSInteger previous=_selectedSegmentIndex;
    _selectedSegmentIndex=index;
    if (previous>=0 && previous<(NSInteger)self.buttons.count) [self applySelection:NO toButton:self.buttons[previous]];
    if (index>=0 && index<(NSInteger)self.buttons.count) [self applySelection:YES toButton:self.buttons[index]];
}
- (BOOL)isTracking {
    for (UIButton *button in self.buttons) if (button.isTracking) return YES;
    return [super isTracking];
}
- (void)choose:(UIButton *)button {
    self.selectedSegmentIndex=button.tag;
    [self sendActionsForControlEvents:UIControlEventValueChanged];
}
@end

// On UIKit versions that ask an interactive glass surface for its gesture
// view, observe the channel buttons' common ancestor while leaving hits on them.
@interface HGAHeaderTouchGlass : HGAAppearanceMaterial
@property(nonatomic,weak) UIView *channelControl;
@end
@implementation HGAHeaderTouchGlass
- (UIView *)_flexInteractionGestureView {
    UIView *control=self.channelControl;
    if (control.window && control.window==self.window) return control;
    IMP inherited=class_getMethodImplementation(HGAAppearanceMaterial.class,_cmd);
    return ((UIView *(*)(id,SEL))inherited)(self,_cmd);
}
@end

@class HGAHeaderToolButton;
@interface HGAHeaderInsetState : NSObject <UIGestureRecognizerDelegate>
@property(nonatomic,strong) HGAAppearanceMaterial *sidebarGlass;
@property(nonatomic,strong) HGAAppearanceMaterial *searchGlass;
@property(nonatomic,strong) HGAHeaderToolButton *sidebarButton;
@property(nonatomic,strong) HGAHeaderToolButton *searchButton;
@property(nonatomic,strong) UIControl<HGAHeaderSelecting> *channelControl;
@property(nonatomic,copy) NSString *channelStyle;
@property(nonatomic) BOOL glassSelection;
@property(nonatomic) HGASelectionColorMode headerSelectionColor;
@property(nonatomic,weak) UICollectionView *channelCollection;
@property(nonatomic,copy) NSArray<NSNumber *> *channelIndices;
@property(nonatomic,copy) NSArray<NSString *> *channelTitles;
@property(nonatomic,weak) UIPanGestureRecognizer *observedPagePan;
@property(nonatomic) BOOL scrubbingRail;
@property(nonatomic) BOOL railDragging;
@property(nonatomic) CGFloat railTouchStartX;
@property(nonatomic) CGFloat railContactPosition;
@property(nonatomic) CGFloat railLastX;
@property(nonatomic) CFTimeInterval railLastTimestamp;
@property(nonatomic) CGFloat railVelocityX;
@property(nonatomic) NSInteger railHoveredSegment;
@property(nonatomic,strong) UISelectionFeedbackGenerator *selectionFeedback;
@property(nonatomic,weak) UILabel *liftedTitle;
@property(nonatomic,strong) UIView *channelLens;
@property(nonatomic,strong) HGAAppearanceMaterial *channelRefraction;
@property(nonatomic) BOOL channelLensTracking;
@property(nonatomic) BOOL channelLensSettling;
@property(nonatomic) NSUInteger channelMotionGeneration;
@property(nonatomic,strong) CADisplayLink *pageCoasting;
@property(nonatomic) NSInteger previewOrigin;
@property(nonatomic,weak) UIView *categories;
@property(nonatomic,copy) NSArray *observedTitles;
@property(nonatomic) CGSize observedSize;
@property(nonatomic,weak) UICollectionView *observedCollection;
@property(nonatomic) BOOL observedChannelListReady;
@property(nonatomic,strong) NSHashTable<UIView *> *visibleSideTools;
@property(nonatomic) NSUInteger channelSelectionDepth;
@property(nonatomic) BOOL ownsSideToolTransition;
- (void)activateTool:(HGAHeaderToolButton *)sender;
- (void)changeChannel:(UIControl<HGAHeaderSelecting> *)sender;
- (void)followPagePan:(UIPanGestureRecognizer *)pan;
- (void)finishChannelMotion;
- (void)selectHeaderSegment:(NSInteger)next;
- (void)syncRestingChannel;
@end
static char HGAHeaderInsetKey;

static UIViewController *HGAHeaderToolOwner(UIView *source,SEL selector) {
    UIViewController *owner=HGAOwner(source);
    if (![owner isKindOfClass:objc_lookUpClass("SSVideoSeriesFeedViewController")]) return nil;
    Method method=class_getInstanceMethod(object_getClass(owner),selector);
    return method && !strcmp(method_getTypeEncoding(method),"v16@0:8") ? owner : nil;
}

@interface HGAHeaderToolButton : UIButton
@property(nonatomic,weak) UIView *source;
@property(nonatomic) BOOL previousAccessibilityHidden;
@property(nonatomic) BOOL previousAccessibilityElement;
@property(nonatomic,copy) NSString *glassStyle;
@property(nonatomic,strong) UIImageView *menuGlyph;
@property(nonatomic) BOOL toolPressed;
- (void)animatePressState:(BOOL)pressed offset:(CGPoint)offset;
@end
@implementation HGAHeaderToolButton
@synthesize source=_source;
- (void)setSource:(UIView *)source {
    if (_source==source) return;
    if (_source.accessibilityElementsHidden) _source.accessibilityElementsHidden=self.previousAccessibilityHidden;
    if (!_source.isAccessibilityElement) _source.isAccessibilityElement=self.previousAccessibilityElement;
    _source=source;
    self.previousAccessibilityHidden=source.accessibilityElementsHidden;
    self.previousAccessibilityElement=source.isAccessibilityElement;
    source.accessibilityElementsHidden=YES;
    source.isAccessibilityElement=NO;
}
- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent *)event {
    CGRect bounds=self.bounds;
    if (self.toolPressed) return CGRectContainsPoint(CGRectInset(bounds,-28,-28),point);
    if (![super pointInside:point withEvent:event]) return NO;
    CGFloat rx=bounds.size.width*0.5, ry=bounds.size.height*0.5;
    if (rx<=0 || ry<=0) return NO;
    CGFloat dx=(point.x-CGRectGetMidX(bounds))/rx, dy=(point.y-CGRectGetMidY(bounds))/ry;
    return dx*dx+dy*dy<=1.0;
}
- (void)animatePressState:(BOOL)pressed offset:(CGPoint)offset {
    self.toolPressed=pressed;
    if (UIAccessibilityIsReduceMotionEnabled()) {
        if (!CGAffineTransformIsIdentity(self.transform)) self.transform=CGAffineTransformIdentity;
        return;
    }
    CGAffineTransform target=CGAffineTransformIdentity;
    if (pressed) {
        CGFloat scale=[self.glassStyle isEqual:@"flat"] ? 1.06 : 1.14;
        target=CGAffineTransformConcat(CGAffineTransformMakeScale(scale,scale),
            CGAffineTransformMakeTranslation(offset.x,offset.y));
    }
    [UIView animateWithDuration:pressed ? 0.24 : 0.38 delay:0
        usingSpringWithDamping:pressed ? 0.60 : 0.58 initialSpringVelocity:pressed ? 0.40 : 0
        options:UIViewAnimationOptionBeginFromCurrentState|UIViewAnimationOptionAllowUserInteraction
        animations:^{ self.transform=target; } completion:nil];
}
- (BOOL)beginTrackingWithTouch:(UITouch *)touch withEvent:(UIEvent *)event {
    BOOL began=[super beginTrackingWithTouch:touch withEvent:event];
    if (began) [self animatePressState:YES offset:CGPointZero];
    return began;
}
- (BOOL)continueTrackingWithTouch:(UITouch *)touch withEvent:(UIEvent *)event {
    BOOL tracking=[super continueTrackingWithTouch:touch withEvent:event];
    if (!tracking || UIAccessibilityIsReduceMotionEnabled()) {
        if (self.toolPressed) [self animatePressState:NO offset:CGPointZero];
        return tracking;
    }
    CGPoint p=[touch locationInView:self];
    if (!CGRectContainsPoint(CGRectInset(self.bounds,-28,-28),p)) {
        if (self.toolPressed) [self animatePressState:NO offset:CGPointZero];
        return YES;
    }
    CGFloat dx=p.x-CGRectGetMidX(self.bounds), dy=p.y-CGRectGetMidY(self.bounds);
    CGPoint pull=CGPointMake(4.0*tanh(dx/22.0),4.0*tanh(dy/22.0));
    if (!self.toolPressed) [self animatePressState:YES offset:pull];
    else {
        CGFloat scale=[self.glassStyle isEqual:@"flat"] ? 1.06 : 1.14;
        self.transform=CGAffineTransformConcat(CGAffineTransformMakeScale(scale,scale),
            CGAffineTransformMakeTranslation(pull.x,pull.y));
    }
    return YES;
}
- (void)endTrackingWithTouch:(UITouch *)touch withEvent:(UIEvent *)event {
    [self animatePressState:NO offset:CGPointZero];
    [super endTrackingWithTouch:touch withEvent:event];
}
- (void)cancelTrackingWithEvent:(UIEvent *)event {
    [self animatePressState:NO offset:CGPointZero];
    [super cancelTrackingWithEvent:event];
}
- (void)willMoveToWindow:(UIWindow *)newWindow {
    if (!newWindow) {
        self.toolPressed=NO;
        self.transform=CGAffineTransformIdentity;
    }
    [super willMoveToWindow:newWindow];
}
@end

@implementation HGAHeaderInsetState
- (NSInteger)hostSegment {
    NSInteger host=((NSInteger (*)(id,SEL))objc_msgSend)(self.categories,sel_registerName("selectedIndex"));
    return (NSInteger)[self.channelIndices indexOfObject:@(host)];
}
- (CGFloat)segmentForPageOffset:(CGFloat)page {
    // The host pager is indexed before channel filtering. Interpolate within
    // each pair of visible host pages, including gaps such as 1 -> 3.
    NSArray<NSNumber *> *indices=self.channelIndices;
    for (NSUInteger i=1;i<indices.count;i++) {
        CGFloat left=indices[i-1].integerValue, right=indices[i].integerValue;
        if (page<=right) return (i-1)+MAX(0,(page-left)/(right-left));
    }
    return MAX(0,(NSInteger)indices.count-1);
}
- (BOOL)hasChannelButtons {
    return self.channelControl && ![self.channelControl isKindOfClass:UISegmentedControl.class];
}
- (CGRect)channelFrame:(NSUInteger)index {
    UIControl *control=self.channelControl;
    if (@available(iOS 26.0,*)) {
        if ([self hasChannelButtons]) {
            UIButton *button=((HGAHeaderGlassChannels *)control).buttons[index];
            if(CGRectGetWidth(button.bounds)>0)return [button convertRect:button.bounds toView:control];
        }
    }
    CGFloat slot=CGRectGetWidth(control.bounds)/self.channelIndices.count;
    NSUInteger physical=control.effectiveUserInterfaceLayoutDirection==UIUserInterfaceLayoutDirectionRightToLeft ?
        self.channelIndices.count-1-index : index;
    return CGRectMake(CGRectGetMinX(control.bounds)+physical*slot,CGRectGetMinY(control.bounds),slot,CGRectGetHeight(control.bounds));
}
- (CGFloat)segmentAtX:(CGFloat)x {
    CGFloat previous=CGRectGetMidX([self channelFrame:0]);
    for(NSUInteger i=1;i<self.channelIndices.count;i++) {
        CGFloat next=CGRectGetMidX([self channelFrame:i]);
        if(next==previous)continue;
        CGFloat fraction=(x-previous)/(next-previous);
        if(fraction<=1)return i-1+MAX(0,fraction);
        previous=next;
    }
    return self.channelIndices.count-1;
}
- (CGRect)lensFrameAtSegment:(CGFloat)index lifted:(BOOL)lifted {
    UIView *control=self.channelControl;
    NSUInteger left=(NSUInteger)floor(index),right=MIN(left+1,self.channelIndices.count-1);
    CGRect a=[self channelFrame:left],b=[self channelFrame:right];
    CGFloat fraction=index-left;
    CGFloat slot=CGRectGetWidth(a)+(CGRectGetWidth(b)-CGRectGetWidth(a))*fraction;
    CGFloat center=CGRectGetMidX(a)+(CGRectGetMidX(b)-CGRectGetMidX(a))*fraction;
    CGRect bounds=control.bounds;
    if (!lifted || ![self hasChannelButtons]) {
        CGFloat inset=lifted ? 3 : 4;
        CGFloat width=MAX(0,slot-2*inset), height=MAX(0,CGRectGetHeight(bounds)-2*inset);
        return [control convertRect:CGRectMake(center-width/2,CGRectGetMinY(bounds)+inset,width,height)
            toView:self.channelLens.superview];
    }
    CGFloat dist=fabs(index-round(index))*2.0;
    CGFloat stretch=dist*dist*(3.0-2.0*dist);
    if (!self.scrubbingRail) {
        CGFloat inset=2.0;
        CGFloat width=MIN(CGRectGetWidth(bounds)-2*inset,MAX(0,(slot-2*inset)+slot*(0.10*stretch)));
        CGFloat height=MAX(0,CGRectGetHeight(bounds)-2*inset);
        CGFloat x=MAX(CGRectGetMinX(bounds)+inset,MIN(center-width*0.5,CGRectGetMaxX(bounds)-inset-width));
        return [control convertRect:CGRectMake(x,CGRectGetMidY(bounds)-height*0.5,width,height)
            toView:self.channelLens.superview];
    }
    // Match Dock's Liquid Glass selection lens: press/long-press swells the
    // droplet slightly above the capsule rim and stretches fluidly between tabs.
    CGFloat height=CGRectGetHeight(bounds)*(1.14-0.05*stretch);
    CGFloat baseWidth=slot+4.0;
    CGFloat maxWidth=CGRectGetWidth(bounds)+4.0;
    CGFloat width=MIN(maxWidth,baseWidth+slot*(0.18*stretch));
    CGFloat minX=CGRectGetMinX(bounds)-2.0;
    CGFloat maxX=CGRectGetMaxX(bounds)+2.0-width;
    CGFloat x=MAX(minX,MIN(center-width*0.5,maxX));
    return [control convertRect:CGRectMake(x,CGRectGetMidY(bounds)-height*0.5,width,height)
        toView:self.channelLens.superview];
}
- (void)prepareChannelLens {
    if (self.channelLens || !self.channelControl.superview) return;
    UIView *lens=[UIView new];
    lens.userInteractionEnabled=NO;
    lens.accessibilityElementsHidden=YES;
    lens.accessibilityIdentifier=@"honggo.appearance.header.swipeLens";
    lens.layer.cornerCurve=kCACornerCurveContinuous;
    BOOL customColor=self.headerSelectionColor!=HGASelectionColorDefault;
    lens.backgroundColor=customColor ? UIColor.clearColor :
        (self.glassSelection ? [UIColor colorWithWhite:1 alpha:0.16] :
        HGADockSelectionColor(self.channelControl.traitCollection,YES));
    lens.hidden=YES;
    if (@available(iOS 26.0,*)) {
        if (self.glassSelection) {
            Method source=class_getInstanceMethod(HGAAppearanceMaterial.class,sel_registerName("_flexInteractionGestureView"));
            HGAAppearanceMaterial *refraction;
            if (source && !strcmp(method_getTypeEncoding(source),"@16@0:8")) {
                HGAHeaderTouchGlass *touchGlass=[[HGAHeaderTouchGlass alloc] initWithEffect:nil];
                touchGlass.channelControl=self.channelControl;
                refraction=touchGlass;
            } else refraction=[[HGAAppearanceMaterial alloc] initWithEffect:nil];
            refraction.style=@"clear";
            refraction.untintedGlass=YES;
            refraction.interactiveGlass=YES;
            refraction.layer.cornerCurve=kCACornerCurveContinuous;
            refraction.overlayColor=UIColor.clearColor;
            refraction.accessibilityIdentifier=@"honggo.appearance.header.refraction";
            refraction.accessibilityElementsHidden=YES;
            refraction.userInteractionEnabled=NO;
            refraction.hidden=YES;
            refraction.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
            [lens addSubview:refraction];
            self.channelRefraction=refraction;
        }
        if ([self hasChannelButtons]) {
            HGAHeaderGlassChannels *control=(id)self.channelControl;
            [control.railSurface insertSubview:lens aboveSubview:control.rail];
        }
    }
    HGASelectedPillView *pill=HGASyncSelectedPillViewInHost(lens,self.headerSelectionColor,
        self.headerSelectionColor==HGASelectionColorSoftRoseMist,0,1.0,@"honggo.appearance.header.selectionPill");
    if (pill && self.channelRefraction) [lens bringSubviewToFront:pill];
    if (!lens.superview) [self.channelControl.superview insertSubview:lens aboveSubview:self.channelControl];
    self.channelLens=lens;
}
- (void)positionLensForSegment:(CGFloat)index lifted:(BOOL)lifted {
    CGRect frame=[self lensFrameAtSegment:index lifted:lifted];
    UIView *lens=self.channelLens;
    CGRect targetBounds=(CGRect){CGPointZero,frame.size};
    CGPoint targetCenter=CGPointMake(CGRectGetMidX(frame),CGRectGetMidY(frame));
    if (!CGRectEqualToRect(lens.bounds,targetBounds)) lens.bounds=targetBounds;
    if (!CGPointEqualToPoint(lens.center,targetCenter)) lens.center=targetCenter;
    CGFloat radius=CGRectGetHeight(frame)/2;
    if (lens.layer.cornerRadius!=radius) lens.layer.cornerRadius=radius;
    if (self.channelRefraction && !CGRectEqualToRect(self.channelRefraction.frame,targetBounds))
        self.channelRefraction.frame=targetBounds;
    self.channelRefraction.materialCornerRadius=radius;
    BOOL customColor=self.headerSelectionColor!=HGASelectionColorDefault;
    HGASelectedPillView *pill=HGASyncSelectedPillViewInHost(lens,self.headerSelectionColor,
        self.headerSelectionColor==HGASelectionColorSoftRoseMist,0,1.0,@"honggo.appearance.header.selectionPill");
    if (pill && self.channelRefraction) [lens bringSubviewToFront:pill];
    if (self.channelRefraction) {
        if (lifted && self.headerSelectionColor!=HGASelectionColorSoftRoseMist) {
            [self.channelRefraction.layer removeAnimationForKey:@"opacity"];
            self.channelRefraction.alpha=1;
            self.channelRefraction.hidden=NO;
            lens.backgroundColor=UIColor.clearColor;
            lens.layer.borderWidth=0;
        } else if (!self.channelLensSettling) {
            self.channelRefraction.hidden=YES;
            self.channelRefraction.alpha=1;
            lens.backgroundColor=customColor ? UIColor.clearColor : [UIColor colorWithWhite:1 alpha:0.16];
            lens.layer.borderWidth=0;
        }
    } else if ([self hasChannelButtons]) {
        lens.backgroundColor=customColor ? UIColor.clearColor : [UIColor colorWithWhite:1 alpha:lifted ? 0.24 : 0.16];
        lens.layer.borderWidth=(lifted && !customColor) ? 0.5 : 0;
        if (lifted && !customColor) lens.layer.borderColor=[UIColor colorWithWhite:1 alpha:0.32].CGColor;
    }
    if(self.channelRefraction.window && !self.channelRefraction.effect)[self.channelRefraction layoutIfNeeded];
}
- (void)prepareSelectionHaptics {
    if (!self.selectionFeedback) self.selectionFeedback=[UISelectionFeedbackGenerator new];
    [self.selectionFeedback prepare];
}
- (void)emitSelectionHaptic {
    if (!self.selectionFeedback) self.selectionFeedback=[UISelectionFeedbackGenerator new];
    [self.selectionFeedback selectionChanged];
    [self.selectionFeedback prepare];
}
- (void)setRailPressed:(BOOL)pressed {
    if (@available(iOS 26.0,*)) {
        if (![self hasChannelButtons]) return;
        UIView *rail=((HGAHeaderGlassChannels *)self.channelControl).rail;
        if (!rail) return;
        if (UIAccessibilityIsReduceMotionEnabled()) {
            if (!CGAffineTransformIsIdentity(rail.transform)) rail.transform=CGAffineTransformIdentity;
            return;
        }
        CGAffineTransform target=pressed ? CGAffineTransformMakeScale(1.014,1.032) : CGAffineTransformIdentity;
        [UIView animateWithDuration:pressed ? 0.24 : 0.36 delay:0
            usingSpringWithDamping:pressed ? 0.68 : 0.72 initialSpringVelocity:pressed ? 0.25 : 0
            options:UIViewAnimationOptionBeginFromCurrentState|UIViewAnimationOptionAllowUserInteraction
            animations:^{ rail.transform=target; } completion:nil];
    }
}
- (void)liftTitle:(UILabel *)title {
    UILabel *previous=self.liftedTitle;
    if (title==previous) return;
    self.liftedTitle=title;
    [UIView animateWithDuration:title ? 0.24 : 0.36 delay:0 usingSpringWithDamping:title ? 0.74 : 0.80 initialSpringVelocity:0
        options:UIViewAnimationOptionBeginFromCurrentState|UIViewAnimationOptionAllowUserInteraction animations:^{
            previous.transform=CGAffineTransformIdentity;
            title.transform=CGAffineTransformMakeScale(1.08,1.08);
        } completion:nil];
}
- (void)syncRestingChannel {
    UIControl<HGAHeaderSelecting> *control=self.channelControl;
    if (!control || control.isTracking || self.channelLensTracking || self.channelLensSettling) return;
    NSInteger segment=[self hostSegment];
    if (segment==NSNotFound) return;
    if (control.selectedSegmentIndex!=segment) control.selectedSegmentIndex=segment;
    if ([self hasChannelButtons]) {
        self.channelLens.transform=CGAffineTransformIdentity;
        [self positionLensForSegment:segment lifted:NO];
        self.channelLens.hidden=NO;
    }
}
- (void)previewSegment:(CGFloat)index liftAnimated:(BOOL)animated {
    UIControl<HGAHeaderSelecting> *control=self.channelControl;
    if (!control.window || self.channelIndices.count<2 || UIAccessibilityIsReduceMotionEnabled()) return;
    index=MIN((CGFloat)self.channelIndices.count-1,MAX(0,index));
    if (!self.channelLensTracking) {
        NSInteger origin=[self hostSegment];
        if (origin==NSNotFound) origin=control.selectedSegmentIndex;
        if (origin<0 || origin>=(NSInteger)self.channelIndices.count) return;
        self.previewOrigin=origin;
        self.channelMotionGeneration++;
        self.channelLensSettling=NO;
        self.channelLensTracking=YES;
        [UIView performWithoutAnimation:^{
            self.channelLens.transform=CGAffineTransformIdentity;
            if (self.channelLens.hidden) [self positionLensForSegment:origin lifted:NO];
            self.channelLens.hidden=NO;
            if (![self hasChannelButtons] && control.selectedSegmentIndex!=UISegmentedControlNoSegment)
                control.selectedSegmentIndex=UISegmentedControlNoSegment;
        }];
        if (animated) {
            [UIView animateWithDuration:0.30 delay:0 usingSpringWithDamping:0.74 initialSpringVelocity:0.20
                options:UIViewAnimationOptionBeginFromCurrentState|UIViewAnimationOptionAllowUserInteraction
                animations:^{
                    [self positionLensForSegment:index lifted:YES];
                } completion:nil];
        } else {
            [self.channelLens.layer removeAllAnimations];
            [UIView performWithoutAnimation:^{[self positionLensForSegment:index lifted:YES];}];
        }
    } else if (!animated) {
        if (self.channelLens.layer.animationKeys.count) [self.channelLens.layer removeAllAnimations];
        [UIView performWithoutAnimation:^{[self positionLensForSegment:index lifted:YES];}];
    }
    if (@available(iOS 26.0,*)) {
        if ([self hasChannelButtons]) {
            UILabel *targetLabel=self.scrubbingRail ?
                ((HGAHeaderGlassChannels *)control).buttons[(NSUInteger)lround(index)].titleLabel : nil;
            [self liftTitle:targetLabel];
        }
    }
}
- (UIView *)pageCategoryContainer {
    UIView *scroll = self.observedPagePan.view;
    Class cls = objc_lookUpClass("FQRJXCategoryListContainerView");
    for (UIView *node = scroll.superview; node; node = node.superview)
        if (cls && [node isKindOfClass:cls]) return node;
    return nil;
}
- (void)logChannelPanPhase:(NSString *)phase scroll:(UIScrollView *)scroll {
    if (!HGEnhancedDiagnostics()) return;
    UIWindow *window = scroll.window ?: self.channelControl.window;
    NSMutableDictionary *payload = [@{
        @"phase": phase ?: @"unknown",
        @"offsetX": @(isfinite(scroll.contentOffset.x) ? round(scroll.contentOffset.x * 10.0) / 10.0 : 0),
        @"pageWidth": @(CGRectGetWidth(scroll.bounds))
    } mutableCopy];
    NSDictionary *dockDiag = HGAppearanceDockDiagnosticState(window);
    if (dockDiag) payload[@"dock"] = dockDiag;
    HGLog(@"appearance.channel-pan", payload);
}
- (void)finishChannelMotion {
    [self.pageCoasting invalidate];
    self.pageCoasting=nil;
    BOOL wasDragged=self.railDragging;
    CGFloat releaseVelocity=self.railVelocityX;
    self.railVelocityX=0;
    self.railDragging=NO;
    [self setRailPressed:NO];
    UIView *lens=self.channelLens;
    if (!self.channelLensTracking) return;
    self.channelLensTracking=NO;
    UIView *pager=[self pageCategoryContainer];
    if (pager) {
        HGRefreshVideoBackdropForCategoryPager(pager);
        HGAReconcileCategoryEpisodes(pager);
    }
    [self logChannelPanPhase:@"settled" scroll:(id)self.observedPagePan.view];
    [self liftTitle:nil];
    NSInteger selected=[self hostSegment];
    UIControl<HGAHeaderSelecting> *control=self.channelControl;
    if (!control.window) {
        lens.transform=CGAffineTransformIdentity;
        lens.hidden=YES;
        if (self.channelRefraction) {
            self.channelRefraction.hidden=YES;
            self.channelRefraction.alpha=1;
        }
        return;
    }
    if (selected==NSNotFound) selected=self.previewOrigin;
    if ([self hasChannelButtons] && selected>=0 && selected<(NSInteger)self.channelIndices.count &&
        control.selectedSegmentIndex!=selected) {
        control.selectedSegmentIndex=selected;
    }
    if (UIAccessibilityIsReduceMotionEnabled()) {
        [UIView performWithoutAnimation:^{
            lens.transform=CGAffineTransformIdentity;
            self.channelLensSettling=NO;
            [self positionLensForSegment:selected lifted:NO];
            control.selectedSegmentIndex=selected;
            lens.hidden=![self hasChannelButtons];
        }];
        [control.superview setNeedsLayout];
        return;
    }
    NSUInteger generation=++self.channelMotionGeneration;
    self.channelLensSettling=YES;
    CGRect restingTarget=[self lensFrameAtSegment:selected lifted:NO];
    CGFloat springVelocity=0;
    if (fabs(releaseVelocity)>10.0) {
        CGFloat dx=CGRectGetMidX(restingTarget)-lens.center.x;
        if (fabs(dx)>2.0) {
            CGFloat toward=(dx>=0 ? releaseVelocity : -releaseVelocity)/MAX(32.0,fabs(dx));
            springVelocity=MIN(4.0,MAX(-1.5,toward));
        }
    }
    BOOL fadeRefraction=self.channelRefraction && !self.channelRefraction.hidden;
    if (fadeRefraction) {
        UIColor *restFill=self.headerSelectionColor!=HGASelectionColorDefault ?
            UIColor.clearColor : [UIColor colorWithWhite:1 alpha:0.16];
        [UIView animateWithDuration:wasDragged ? 0.18 : 0.14 delay:wasDragged ? 0.10 : 0.04
            options:UIViewAnimationOptionBeginFromCurrentState|UIViewAnimationOptionAllowUserInteraction|UIViewAnimationOptionCurveEaseOut
            animations:^{
                self.channelRefraction.alpha=0;
                lens.backgroundColor=restFill;
            } completion:nil];
    }
    [UIView animateWithDuration:wasDragged ? 0.36 : 0.26 delay:0
        usingSpringWithDamping:wasDragged ? 0.76 : 0.82 initialSpringVelocity:springVelocity
        options:UIViewAnimationOptionBeginFromCurrentState|UIViewAnimationOptionAllowUserInteraction animations:^{
        lens.transform=CGAffineTransformIdentity;
        [self positionLensForSegment:selected lifted:NO];
    } completion:^(BOOL finished){
        if(self.channelMotionGeneration!=generation || self.channelLens!=lens)return;
        self.channelLensSettling=NO;
        NSInteger resting=[self hostSegment];
        if(resting==NSNotFound)resting=selected;
        [UIView performWithoutAnimation:^{
            lens.transform=CGAffineTransformIdentity;
            if(self.channelRefraction) {
                self.channelRefraction.hidden=YES;
                self.channelRefraction.alpha=1;
            }
            if(control.selectedSegmentIndex!=resting)control.selectedSegmentIndex=resting;
            lens.hidden=![self hasChannelButtons];
            if (!lens.hidden) [self positionLensForSegment:resting lifted:NO];
        }];
        // Apply any host geometry deferred while the channel was tracking.
        [control.superview setNeedsLayout];
    }];
}
- (void)followPageDeceleration:(CADisplayLink *)link {
    UIScrollView *page=(id)self.observedPagePan.view;
    if (!page.window || !self.channelControl.window || !page.decelerating) {
        [self finishChannelMotion];
        return;
    }
    [self previewSegment:[self segmentForPageOffset:page.contentOffset.x/MAX(1,CGRectGetWidth(page.bounds))] liftAnimated:NO];
}
- (void)followPagePan:(UIPanGestureRecognizer *)pan {
    if (self.scrubbingRail) return;
    if (pan.state!=UIGestureRecognizerStateBegan && pan.state!=UIGestureRecognizerStateChanged) {
        UIScrollView *scroll = [pan.view isKindOfClass:UIScrollView.class] ? (id)pan.view : (id)self.observedPagePan.view;
        [self logChannelPanPhase:(pan.state==UIGestureRecognizerStateEnded ? @"ended" : @"cancelled") scroll:scroll];
        if (pan==self.observedPagePan && self.channelLensTracking && pan.state==UIGestureRecognizerStateEnded) {
            if (!self.pageCoasting) {
                self.pageCoasting=[CADisplayLink displayLinkWithTarget:self selector:@selector(followPageDeceleration:)];
                [self.pageCoasting addToRunLoop:NSRunLoop.mainRunLoop forMode:NSRunLoopCommonModes];
            }
            return;
        }
        [self finishChannelMotion];
        return;
    }
    if (self.pageCoasting) {
        [self.pageCoasting invalidate];
        self.pageCoasting=nil;
    }
    UIView *page=pan.view ?: self.observedPagePan.view;
    if (!page.window || !self.channelControl.window || HGFeedSidebarOwnsLayout(page.window)) return;
    CGPoint travel=[pan translationInView:page];
    if (!self.channelLensTracking && fabs(travel.x)<=fabs(travel.y)) return;
    BOOL firstTrackingFrame = !self.channelLensTracking;
    if (pan==self.observedPagePan) {
        UIScrollView *scroll=(id)page;
        UIView *pager=[self pageCategoryContainer];
        if (pager) HGACategoryPagerDidScroll(pager, scroll);
        if (firstTrackingFrame) {
            if (pager) {
                HGRefreshVideoBackdropForCategoryPager(pager);
                HGAReconcileCategoryEpisodes(pager);
            }
            [self logChannelPanPhase:@"began" scroll:scroll];
        }
        [self previewSegment:[self segmentForPageOffset:scroll.contentOffset.x/MAX(1,CGRectGetWidth(page.bounds))] liftAnimated:NO];
        return;
    }
    if (firstTrackingFrame) [self logChannelPanPhase:@"began" scroll:(id)self.observedPagePan.view];
    NSInteger origin=self.channelLensTracking ? self.previewOrigin : [self hostSegment];
    if (origin==NSNotFound) return;
    // The filtered pager skips hidden host pages on release; its gesture has
    // no contentOffset updates. Drive the lens from the actual finger travel.
    CGFloat stride=MAX(1,CGRectGetWidth(page.bounds)*0.42);
    CGFloat direction=self.channelControl.effectiveUserInterfaceLayoutDirection==UIUserInterfaceLayoutDirectionRightToLeft ? 1 : -1;
    [self previewSegment:origin+MAX(-1,MIN(1,direction*travel.x/stride)) liftAnimated:NO];
}
// The channel rail owns a touch from contact through release, including a
// stationary hold before a slow drag. Page swipes outside it remain native.
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gesture shouldBeRequiredToFailByGestureRecognizer:(UIGestureRecognizer *)other {
    return [other isKindOfClass:UIPanGestureRecognizer.class] && [gesture.view isDescendantOfView:other.view];
}
- (void)scrubChannels:(UIGestureRecognizer *)pan {
    UIControl<HGAHeaderSelecting> *control=self.channelControl;
    if (!control.window || !self.channelIndices.count) return;
    CGPoint p=[pan locationInView:control];
    CGFloat position=[self segmentAtX:p.x];
    if (pan.state==UIGestureRecognizerStateBegan) {
        self.railContactPosition=position;
        self.railTouchStartX=p.x;
        self.railDragging=NO;
        self.railLastX=p.x;
        self.railLastTimestamp=CACurrentMediaTime();
        self.railVelocityX=0;
        self.railHoveredSegment=lround(position);
        [self prepareSelectionHaptics];
    }
    if (self.scrubbingRail || pan.state==UIGestureRecognizerStateBegan) {
        // Anchor the initial lift to the button. Blend the small grab offset
        // away as the finger travels so a stationary hold cannot jump sideways.
        CGFloat contact=self.railContactPosition;
        position+=(round(contact)-contact)*MAX(0,1-fabs(position-contact)/0.35);
    }
    if (pan.state==UIGestureRecognizerStateBegan || pan.state==UIGestureRecognizerStateChanged) {
        BOOL wasScrubbing=self.scrubbingRail;
        if (!wasScrubbing) {
            [self.pageCoasting invalidate];
            self.pageCoasting=nil;
            // A new rail contact takes over a preview started by the page pan.
            // Let its first sample lift at the touched button immediately.
            self.channelLensTracking=NO;
            if (pan.state==UIGestureRecognizerStateChanged) {
                self.railContactPosition=position;
                self.railTouchStartX=p.x;
                self.railDragging=YES;
                self.railLastX=p.x;
                self.railLastTimestamp=CACurrentMediaTime();
                self.railVelocityX=0;
                self.railHoveredSegment=lround(position);
            }
        }
        self.scrubbingRail=YES;
        [self setRailPressed:YES];
        if (pan.state==UIGestureRecognizerStateBegan) {
            [self previewSegment:round(position) liftAnimated:YES];
            return;
        }
        if (!self.railDragging) {
            // Ignore capacitive micro-jitter during stationary taps and holds so
            // the initial Liquid Glass inflation/glide spring runs uninterrupted.
            if (fabs(p.x-self.railTouchStartX)<=6.0) return;
            self.railDragging=YES;
        }
        CFTimeInterval now=CACurrentMediaTime();
        CFTimeInterval dt=self.railLastTimestamp>0 ? now-self.railLastTimestamp : 0;
        if (dt>0.001 && dt<0.12) {
            CGFloat vx=(p.x-self.railLastX)/dt;
            self.railVelocityX=self.railVelocityX*0.55+vx*0.45;
        } else if (dt>=0.12) {
            self.railVelocityX=0;
        }
        self.railLastX=p.x;
        self.railLastTimestamp=now;
        NSInteger hovered=MIN((NSInteger)self.channelIndices.count-1,MAX(0,lround(position)));
        if (hovered!=self.railHoveredSegment) {
            self.railHoveredSegment=hovered;
            [self emitSelectionHaptic];
        }
        if ([self hasChannelButtons] && !UIAccessibilityIsReduceMotionEnabled()) {
            CGFloat norm=MIN(1.0,fabs(self.railVelocityX)/1100.0);
            CGAffineTransform stretch=norm>0.02 ?
                CGAffineTransformMakeScale(1.0+norm*0.045,1.0-norm*0.028) : CGAffineTransformIdentity;
            if (!CGAffineTransformEqualToTransform(self.channelLens.transform,stretch))
                self.channelLens.transform=stretch;
        }
        [self previewSegment:position liftAnimated:NO];
    } else {
        BOOL dragged=self.railDragging;
        self.scrubbingRail=NO;
        CGRect commitZone=CGRectInset(control.bounds,dragged ? -24 : -8,dragged ? -44 : -12);
        if (pan.state==UIGestureRecognizerStateEnded && CGRectContainsPoint(commitZone,p)) {
            NSInteger index=MIN((NSInteger)self.channelIndices.count-1,MAX(0,lround(position)));
            [self selectHeaderSegment:index];
        }
        [self finishChannelMotion];
    }
}
- (void)activateTool:(HGAHeaderToolButton *)sender {
    BOOL sidebar=sender==self.sidebarButton;
    UIView *source=sender.source ?: (sidebar && sender.menuGlyph ? sender.superview : nil);
    if (!sender.window || !HGAVisible(source) || HGFeedSidebarOwnsLayout(sender.window)) return;
    SEL selector=sel_registerName(sidebar ? "simpleSideBarViewDidClicked" : "simpleSearchViewDidClicked");
    UIViewController *owner=HGAHeaderToolOwner(source,selector);
    if (owner) ((void (*)(id,SEL))objc_msgSend)(owner,selector);
}
- (void)changeChannel:(UIControl<HGAHeaderSelecting> *)sender {
    NSInteger next=sender.selectedSegmentIndex;
    BOOL shouldSettle=([self hasChannelButtons] || self.channelLensSettling) &&
        !self.channelLensTracking && !UIAccessibilityIsReduceMotionEnabled();
    if (shouldSettle) {
        self.previewOrigin=[self hostSegment];
        if (self.previewOrigin==NSNotFound) self.previewOrigin=next;
        self.channelLensSettling=NO;
        self.channelLensTracking=YES;
        if (self.previewOrigin!=next && [self hasChannelButtons]) {
            [UIView performWithoutAnimation:^{
                BOOL wasScrubbing=self.scrubbingRail;
                self.scrubbingRail=YES;
                [self positionLensForSegment:self.previewOrigin lifted:YES];
                self.scrubbingRail=wasScrubbing;
            }];
        }
    }
    [self selectHeaderSegment:next];
    if (shouldSettle) {
        if (![self hasChannelButtons])
            [UIView performWithoutAnimation:^{sender.selectedSegmentIndex=UISegmentedControlNoSegment;}];
        [self finishChannelMotion];
    } else if ([self hasChannelButtons]) [self finishChannelMotion];
}
- (void)selectHeaderSegment:(NSInteger)next {
    UIView *categories=self.categories;
    UIControl<HGAHeaderSelecting> *sender=self.channelControl;
    if (!sender.window || !categories.window || next<0 || next>=(NSInteger)self.channelIndices.count ||
        HGFeedSidebarOwnsLayout(sender.window)) return;
    SEL selected=sel_registerName("selectedIndex"), select=sel_registerName("selectItemAtIndex:needAnimation:");
    NSInteger current=((NSInteger (*)(id,SEL))objc_msgSend)(categories,selected);
    NSInteger index=self.channelIndices[next].integerValue;
    if (current!=index) ((BOOL (*)(id,SEL,NSInteger,BOOL))objc_msgSend)(categories,select,index,NO);
    if (sender==self.channelControl && categories==self.categories) [self syncRestingChannel];
}
@end

static UIView *HGAHeaderSearchSlotView(UIView *header) {
    UIView *search=HGAViewGetter(header,"searchViewContainer",header);
    if(!search)return nil;
    UIView *slot=HGAViewGetter(search,"searchButtonSlotView",search) ?: HGAViewGetter(search,"searchButtonView",search);
    if(slot && slot!=search)return slot;
    // Fallback for host builds where the right search icon wrapper is an
    // anonymous ~24x24 subview inside SSVideoSeriesFeedHeaderSearchView.
    CGFloat midX=CGRectGetMidX(search.bounds);
    UIView *best=nil;
    for(UIView *sub in search.subviews) {
        if(sub.hidden || sub.alpha<0.01 || HGAOwnView(sub))continue;
        CGFloat w=CGRectGetWidth(sub.frame),h=CGRectGetHeight(sub.frame);
        if(w>=18.0 && w<=64.0 && h>=18.0 && h<=44.0 && CGRectGetMidX(sub.frame)>midX) {
            if(!best || CGRectGetMidX(sub.frame)>CGRectGetMidX(best.frame))best=sub;
        }
    }
    return best;
}

// The host uses UICollectionViewFlowLayout to hit-test channel indices and its
// own two target-frame getters to drive the native underline during selection
// and horizontal paging. Keep all three in collection-content coordinates.
@interface HGAHeaderChannelLayout : NSObject
@property(nonatomic,weak) UICollectionView *collection;
@property(nonatomic,copy) NSArray<NSString *> *titles;
@property(nonatomic,copy) NSArray<NSValue *> *frames;
@property(nonatomic) NSUInteger hiddenMask;
@property(nonatomic) CGFloat width;
@property(nonatomic) CGFloat inset;
@property(nonatomic) CGRect capsule;
@end
@implementation HGAHeaderChannelLayout @end

static char HGAHeaderLayoutKey, HGAHeaderLineRestoreKey, HGAHeaderPagerKey, HGAHeaderSettlementKey;
static BOOL HGAHeaderAdaptersReady;
static IMP HGAHeaderOriginals[6];
static IMP HGAHeaderOriginalSelectCell;
static void HGARefreshHeaderChannelLine(UIView *categories,BOOL completed);
static void HGAKeepCompactHeaderSideTools(UIView *header,HGAHeaderInsetState *state,UIView *sidebar,UIView *searchSlot);

@interface HGAFixedChannelPan : UIPanGestureRecognizer
@property(nonatomic,weak) UIPanGestureRecognizer *nativePan;
@end
@implementation HGAFixedChannelPan
- (BOOL)canPreventGestureRecognizer:(UIGestureRecognizer *)other {
    return other==self.nativePan;
}
@end

@interface HGAFixedChannelPager : NSObject <UIGestureRecognizerDelegate>
@property(nonatomic,weak) UIView *categories;
@property(nonatomic,strong) HGAFixedChannelPan *pan;
@property(nonatomic) BOOL suspendsNativePan;
- (void)updateNativePan;
- (void)detach;
- (BOOL)isFiltering;
@end
@implementation HGAFixedChannelPager
- (void)dealloc { [self detach]; }

- (void)updateNativePan {
    UIScrollView *scroll=(UIScrollView *)self.pan.view;
    if (scroll.scrollEnabled && self.pan.nativePan.enabled) {
        self.suspendsNativePan=YES;
        self.pan.nativePan.enabled=NO;
    }
}

- (void)detach {
    UIScrollView *scroll=(UIScrollView *)self.pan.view;
    BOOL suspended=self.suspendsNativePan;
    self.suspendsNativePan=NO;
    if (self.pan.view) [scroll removeGestureRecognizer:self.pan];
    if (suspended && scroll.scrollEnabled) self.pan.nativePan.enabled=YES;
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gesture shouldBeRequiredToFailByGestureRecognizer:(UIGestureRecognizer *)other {
    return other==self.pan.nativePan;
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gesture shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)other {
    // Each channel contains a vertical feed. Its scroll pan can recognize a
    // horizontal drag first even when it cannot scroll along that axis.
    UIScrollView *feed=(id)other.view;
    return gesture==self.pan && [feed isKindOfClass:objc_lookUpClass("FQVCollectionView")] &&
        other==feed.panGestureRecognizer && [feed isDescendantOfView:self.pan.view];
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gesture shouldReceiveTouch:(UITouch *)touch {
    Class progress = objc_lookUpClass("FQVShortVideoProgressView");
    BOOL control=NO, episode=NO;
    for (UIView *view=touch.view; view && view!=gesture.view; view=view.superview) {
        if ([view isKindOfClass:progress]) return NO;
        control |= [view isKindOfClass:UIControl.class];
        episode |= [HGARole(view) isEqual:@"episodes"];
    }
    // A stationary touch still triggers the native episode button. A sideways
    // drag over that button belongs to the filtered home pager.
    return !control || episode;
}

- (BOOL)isFiltering {
    UIView *categories = self.categories;
    UIScrollView *scroll = (UIScrollView *)self.pan.view;
    return HGAppearanceEnabled() && categories.window && categories.window==scroll.window && scroll.scrollEnabled &&
        self.suspendsNativePan && !HGFeedSidebarOwnsLayout(scroll.window) &&
        HGAIsHomeChannel(categories);
}

- (NSInteger)nextVisibleChannelForTravel:(CGFloat)x {
    UIView *categories=self.categories;
    HGAHeaderChannelLayout *layout=objc_getAssociatedObject(categories,&HGAHeaderLayoutKey);
    NSInteger index=((NSInteger (*)(id,SEL))objc_msgSend)(categories,sel_registerName("selectedIndex"));
    if (!x || index<0 || index>=(NSInteger)layout.titles.count) return NSNotFound;
    NSInteger step=x<0 ? 1 : -1;
    if (categories.effectiveUserInterfaceLayoutDirection==UIUserInterfaceLayoutDirectionRightToLeft) step=-step;
    for (NSInteger next=index+step; next>=0 && next<(NSInteger)layout.titles.count; next+=step)
        if (!(layout.hiddenMask & (1UL << next))) return next;
    return NSNotFound;
}

- (BOOL)gestureRecognizerShouldBegin:(UIPanGestureRecognizer *)gesture {
    if (![self isFiltering] || ((UIScrollView *)self.pan.view).isDecelerating) return NO;
    CGPoint velocity=[gesture velocityInView:self.pan.view];
    return fabs(velocity.x)>fabs(velocity.y) && [self nextVisibleChannelForTravel:velocity.x]!=NSNotFound;
}

- (void)page:(UIPanGestureRecognizer *)gesture {
    UIView *categories = self.categories;
    UIView *header=categories.superview;
    while (header && !HGAIsHeaderView(header)) header=header.superview;
    HGAHeaderInsetState *headerState=objc_getAssociatedObject(header,&HGAHeaderInsetKey);
    if (gesture.state!=UIGestureRecognizerStateEnded) {
        [headerState followPagePan:gesture];
        return;
    }
    [self commitPage:gesture];
    [headerState finishChannelMotion];
}
- (void)commitPage:(UIPanGestureRecognizer *)gesture {
    UIView *categories=self.categories;
    if (![self isFiltering] || ((UIScrollView *)self.pan.view).isDecelerating) return;
    CGPoint travel = [gesture translationInView:self.pan.view];
    CGPoint velocity = [gesture velocityInView:self.pan.view];
    if (fabs(travel.x)<=fabs(travel.y) || (fabs(travel.x)<28 && fabs(velocity.x)<300)) return;

    NSInteger next=[self nextVisibleChannelForTravel:travel.x];
    if (next!=NSNotFound && ((BOOL (*)(id,SEL,NSInteger,BOOL))objc_msgSend)(categories,
        sel_registerName("selectItemAtIndex:needAnimation:"),next,NO))
        HGARefreshHeaderChannelLine(categories,YES);
}
@end

static BOOL HGAHomePagerHandlesEpisodeSwipe(UIViewController *owner) {
    if (![owner isKindOfClass:UIViewController.class]) return NO;
    UIView *view=owner.viewIfLoaded;
    if (!view.window) return NO;
    if ([owner isKindOfClass:objc_lookUpClass("SSVideoSeriesFeedViewController")]) {
        UIView *container=HGAViewGetter(owner,"containerView",view);
        view=HGAViewGetter(container,"contentScrollView",container);
    }
    for (; view && view!=view.window; view=view.superview) {
        if (![view isKindOfClass:UIScrollView.class]) continue;
        for (UIGestureRecognizer *recognizer in view.gestureRecognizers) {
            if (![recognizer isKindOfClass:HGAFixedChannelPan.class]) continue;
            HGAFixedChannelPager *pager=(id)recognizer.delegate;
            if ([pager isFiltering] && [pager nextVisibleChannelForTravel:-1]!=NSNotFound) return YES;
        }
    }
    return NO;
}

typedef BOOL (^HGALeftEpisodeDecision)(UIPanGestureRecognizer *gesture);
static char HGAEpisodeSwipeDecisionKey;

static void HGAWrapEpisodeSwipeDecision(UIViewController *owner) {
    if (![owner isKindOfClass:UIViewController.class]) return;
    SEL get=sel_registerName("ss_shouldBeginPushInteractiveTransitionDirectionHorizontalBlock");
    SEL set=sel_registerName("setSs_shouldBeginPushInteractiveTransitionDirectionHorizontalBlock:");
    Method getter=class_getInstanceMethod(object_getClass(owner),get);
    Method setter=class_getInstanceMethod(object_getClass(owner),set);
    if (!getter || !setter || strcmp(method_getTypeEncoding(getter),"@?16@0:8") ||
        strcmp(method_getTypeEncoding(setter),"v24@0:8@?16")) return;

    HGALeftEpisodeDecision original=((HGALeftEpisodeDecision(*)(id,SEL))objc_msgSend)(owner,get);
    if (!original || original==objc_getAssociatedObject(owner,&HGAEpisodeSwipeDecisionKey)) return;
    __weak UIViewController *weakOwner=owner;
    HGALeftEpisodeDecision decision=[^BOOL(UIPanGestureRecognizer *gesture) {
        if (HGAppearanceEnabled() && (HGAppearanceFlag(@"player.disableEpisodeSwipe") ||
            (NSThread.isMainThread && HGAHomePagerHandlesEpisodeSwipe(weakOwner)))) return NO;
        return original(gesture);
    } copy];
    objc_setAssociatedObject(owner,&HGAEpisodeSwipeDecisionKey,decision,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    ((void(*)(id,SEL,HGALeftEpisodeDecision))objc_msgSend)(owner,set,decision);
}

static void HGAInstallEpisodeSwipeSetup(Class target,SEL selector,BOOL argumentIsContext,IMP *installed) {
    if (*installed) return;
    Method method=class_getInstanceMethod(target,selector);
    if (!method || strcmp(method_getTypeEncoding(method),"v24@0:8@16")) return;
    __block IMP original=method_getImplementation(method);
    IMP replacement=imp_implementationWithBlock(^void(id receiver,id argument) {
        ((void(*)(id,SEL,id))original)(receiver,selector,argument);
        HGAWrapEpisodeSwipeDecision(argumentIsContext ? HGReadObjectGetter(argument,"viewController") : argument);
    });
    IMP previous=HGInstallVerifiedHook(target,selector,"v24@0:8@16",replacement,@"appearance");
    if (previous) {
        original=previous;
        *installed=previous;
    } else imp_removeBlock(replacement);
}

static void HGAInstallHomeEpisodeSwipeGate(void) {
    static IMP listSetup,homeSetup;
    // The home controller registers a separate interactive push that resolves
    // its destination through SSBizShortVideo; it bypasses the list delegate.
    HGAInstallEpisodeSwipeSetup(object_getClass(objc_lookUpClass("SSBizVideoImpl")),
        sel_registerName("setLeftSlideToShortsListVCWithParentVC:"),NO,&homeSetup);
    HGAInstallEpisodeSwipeSetup(objc_lookUpClass("SSShortVideoGestureComponent"),
        sel_registerName("setupPanGestureTowardsLeftEnterOtherPage:"),YES,&listSetup);

    static IMP original;
    if (original) return;
    Class delegateClass=objc_lookUpClass("SSShortVideoPanGestureLeftTowardDelegate");
    SEL selector=sel_registerName("shouldLeftSlideEnterOtherPage:context:");
    Method method=class_getInstanceMethod(delegateClass,selector);
    if (!method || strcmp(method_getTypeEncoding(method),"B32@0:8@16@24")) return;

    original=method_getImplementation(method);
    IMP replacement=imp_implementationWithBlock(^BOOL(id delegate,id navigation,id context) {
        // The host's interactive push consults this gate before it opens
        // episode detail. Explicit taps use their original action instead.
        if (NSThread.isMainThread && HGAppearanceEnabled()) {
            if (HGAppearanceFlag(@"player.disableEpisodeSwipe")) return NO;
            UIViewController *owner=HGReadObjectGetter(context,"viewController");
            if (HGAHomePagerHandlesEpisodeSwipe(owner)) {
                if (HGEnhancedDiagnostics()) HGLog(@"appearance.channel",
                    @{@"selector":NSStringFromSelector(selector),@"phase":@"deferred-to-channel"});
                return NO;
            }
        }
        return ((BOOL(*)(id,SEL,id,id))original)(delegate,selector,navigation,context);
    });
    IMP previous=HGInstallVerifiedHook(delegateClass,selector,"B32@0:8@16@24",replacement,@"appearance");
    if (previous) original=previous;
    else {
        imp_removeBlock(replacement);
        original=NULL;
    }
}

static UIScrollView *HGAHomeChannelScroll(id categories) {
    Class cls = objc_lookUpClass("FQRJXCategoryListContainerView");
    UIView *container = HGReadObjectGetter(categories,"listContainer");
    UIScrollView *scroll = [container isKindOfClass:cls] ? HGReadObjectGetter(container,"contentScrollView") : nil;
    if (![scroll isKindOfClass:UIScrollView.class] || ![scroll isDescendantOfView:container]) return nil;
    return scroll;
}

static void HGAUpdateChannelPager(UIView *categories,HGAHeaderChannelLayout *layout) {
    HGAFixedChannelPager *state = objc_getAssociatedObject(categories,&HGAHeaderPagerKey);
    UIScrollView *scroll = layout ? HGAHomeChannelScroll(categories) : nil;
    if (state.pan.view==scroll) {
        [state updateNativePan];
        return;
    }
    [state detach];
    objc_setAssociatedObject(categories,&HGAHeaderPagerKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    if (!scroll) return;
    Method select = class_getInstanceMethod(categories.class,sel_registerName("selectItemAtIndex:needAnimation:"));
    if (!select || strcmp(method_getTypeEncoding(select),"B28@0:8q16B24")) return;
    state = [HGAFixedChannelPager new];
    state.categories = categories;
    state.pan = [[HGAFixedChannelPan alloc] initWithTarget:state action:@selector(page:)];
    state.pan.delegate = state;
    state.pan.nativePan = scroll.panGestureRecognizer;
    state.pan.maximumNumberOfTouches = 1;
    [scroll addGestureRecognizer:state.pan];
    objc_setAssociatedObject(categories,&HGAHeaderPagerKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    // A native pan settling on a 0.01pt-hidden title still exposes that page.
    // The mounted filtered pager uses the host's selection API on release.
    [state updateNativePan];
}

static CGSize HGAChannelSize(id owner, SEL cmd, UICollectionView *collection, UICollectionViewLayout *layout, NSIndexPath *path) {
    CGSize size = ((CGSize (*)(id,SEL,id,id,id))HGAHeaderOriginals[0])(owner,cmd,collection,layout,path);
    HGAHeaderChannelLayout *plan = objc_getAssociatedObject(owner,&HGAHeaderLayoutKey);
    if (collection == plan.collection && path.section == 0 && path.item >= 0 && path.item < (NSInteger)plan.frames.count)
        size.width = plan.frames[path.item].CGRectValue.size.width;
    else if(path.section==0 && path.item>=0 && path.item<32 &&
        ([objc_getAssociatedObject(owner,&HGAFinderCategoryMaskKey) unsignedIntegerValue] & (1UL<<path.item)))
        size.width=0.01;
    return size;
}

static CGFloat HGAChannelSpacing(id owner, SEL cmd, UICollectionView *collection, UICollectionViewLayout *layout, NSInteger section) {
    HGAHeaderChannelLayout *plan = objc_getAssociatedObject(owner,&HGAHeaderLayoutKey);
    if (plan && collection == plan.collection && section == 0) return 0;
    NSUInteger slot = cmd == @selector(collectionView:layout:minimumInteritemSpacingForSectionAtIndex:) ? 5 : 1;
    return ((CGFloat (*)(id,SEL,id,id,NSInteger))HGAHeaderOriginals[slot])(owner,cmd,collection,layout,section);
}

static UIEdgeInsets HGAChannelInsets(id owner, SEL cmd, UICollectionView *collection, UICollectionViewLayout *layout, NSInteger section) {
    UIEdgeInsets insets = ((UIEdgeInsets (*)(id,SEL,id,id,NSInteger))HGAHeaderOriginals[2])(owner,cmd,collection,layout,section);
    HGAHeaderChannelLayout *plan = objc_getAssociatedObject(owner,&HGAHeaderLayoutKey);
    if (plan && collection == plan.collection && section == 0)
        insets.left = insets.right = plan.inset;
    return insets;
}

static CGRect HGACompactTargetFrame(id owner, NSInteger index, CGRect frame) {
    HGAHeaderChannelLayout *plan = objc_getAssociatedObject(owner,&HGAHeaderLayoutKey);
    if (index >= 0 && index < (NSInteger)plan.frames.count && !CGRectIsEmpty(frame)) {
        CGRect item = plan.frames[index].CGRectValue;
        if (plan.hiddenMask & (1UL << index)) frame.size.width = item.size.width;
        frame.origin.x = CGRectGetMidX(item) - frame.size.width / 2;
    }
    return frame;
}
static CGRect HGAChannelFrame(id owner, SEL cmd, NSInteger index) {
    CGRect native = ((CGRect (*)(id,SEL,NSInteger))HGAHeaderOriginals[3])(owner,cmd,index);
    return HGACompactTargetFrame(owner,index,native);
}
static CGRect HGAChannelSelectedFrame(id owner, SEL cmd, NSInteger index, NSUInteger type) {
    CGRect native = ((CGRect (*)(id,SEL,NSInteger,NSUInteger))HGAHeaderOriginals[4])(owner,cmd,index,type);
    return HGACompactTargetFrame(owner,index,native);
}

static BOOL HGAChannelSelectCell(UIView *categories, SEL cmd, NSInteger index, NSUInteger type, BOOL animated) {
    HGAHeaderChannelLayout *plan=objc_getAssociatedObject(categories,&HGAHeaderLayoutKey);
    BOOL compact=plan && categories.window && HGAppearanceEnabled() && HGAIsHomeChannel(categories);
    if (compact && index>=0 && index<(NSInteger)plan.frames.count && (plan.hiddenMask & (1UL<<index))) return NO;
    UIView *header=compact ? categories.superview : nil;
    while (header && !HGAIsHeaderView(header)) header=header.superview;
    HGAHeaderInsetState *state=compact ? objc_getAssociatedObject(header,&HGAHeaderInsetKey) : nil;
    BOOL selected=NO;
    if (state) state.channelSelectionDepth++;
    @try {
        selected=((BOOL (*)(id,SEL,NSInteger,NSUInteger,BOOL))HGAHeaderOriginalSelectCell)(categories,cmd,index,type,animated);
        if (state.channelControl) {
            // The host can change the side tools inside a channel callback.
            // Correct visibility here and leave the rest of the header mounted.
            HGAKeepCompactHeaderSideTools(header,state,HGAViewGetter(header,"sideBar",header),HGAHeaderSearchSlotView(header));
            [state syncRestingChannel];
        } else if (state) [header setNeedsLayout];
    } @finally {
        if (state) state.channelSelectionDepth--;
    }
    if (compact && selected && !animated) HGARefreshHeaderChannelLine(categories,YES);
    return selected;
}

static void HGAInstallHeaderLayoutAdapters(void) {
    if (HGAHeaderAdaptersReady) return;
    Class cls = NSClassFromString(@"SSHGBookStoreCategoryTitleView");
    if (!cls) return;
    static const struct { const char *name, *encoding; IMP replacement; } adapters[] = {
        {"collectionView:layout:sizeForItemAtIndexPath:", "{CGSize=dd}40@0:8@16@24@32", (IMP)HGAChannelSize},
        {"collectionView:layout:minimumLineSpacingForSectionAtIndex:", "d40@0:8@16@24q32", (IMP)HGAChannelSpacing},
        {"collectionView:layout:insetForSectionAtIndex:", "{UIEdgeInsets=dddd}40@0:8@16@24q32", (IMP)HGAChannelInsets},
        {"getTargetCellFrame:", "{CGRect={CGPoint=dd}{CGSize=dd}}24@0:8q16", (IMP)HGAChannelFrame},
        {"getTargetSelectedCellFrame:selectedType:", "{CGRect={CGPoint=dd}{CGSize=dd}}32@0:8q16Q24", (IMP)HGAChannelSelectedFrame},
        // 7.4.5.33 forwards both delegate spacing queries to innerCellSpacing.
        {"collectionView:layout:minimumInteritemSpacingForSectionAtIndex:", "d40@0:8@16@24q32", (IMP)HGAChannelSpacing}
    };
    static const struct { const char *name, *encoding; } properties[] = {
        {"titles", "@16@0:8"}, {"indicators", "@16@0:8"}, {"collectionView", "@16@0:8"},
        {"contentScrollView", "@16@0:8"}, {"selectedIndex", "q16@0:8"}
    };
    for (NSUInteger i = 0; i < sizeof(properties)/sizeof(properties[0]); i++) {
        Method m = class_getInstanceMethod(cls,sel_registerName(properties[i].name));
        if (!m || strcmp(method_getTypeEncoding(m),properties[i].encoding)) return;
    }
    for (NSUInteger i = 0; i < sizeof(adapters)/sizeof(adapters[0]); i++) {
        Method m = class_getInstanceMethod(cls,sel_registerName(adapters[i].name));
        if (!m || strcmp(method_getTypeEncoding(m),adapters[i].encoding)) return;
    }
    for (NSUInteger i = 0; i < sizeof(adapters)/sizeof(adapters[0]); i++) {
        SEL selector = sel_registerName(adapters[i].name);
        if (!HGAHeaderOriginals[i])
            HGAHeaderOriginals[i] = class_getMethodImplementation(cls,selector);
        IMP original = HGInstallVerifiedHook(cls,selector,adapters[i].encoding,adapters[i].replacement,@"appearance");
        if (!original || class_getMethodImplementation(cls,selector) != adapters[i].replacement) return;
        HGAHeaderOriginals[i] = original;
    }
    SEL select=sel_registerName("selectCellAtIndex:selectedType:needAnimation:");
    const char *selectEncoding="B36@0:8q16Q24B32";
    Method selection=class_getInstanceMethod(cls,select);
    if (selection && !strcmp(method_getTypeEncoding(selection),selectEncoding)) {
        HGAHeaderOriginalSelectCell=method_getImplementation(selection);
        HGAHeaderOriginalSelectCell=HGInstallVerifiedHook(cls,select,selectEncoding,(IMP)HGAChannelSelectCell,@"appearance");
    }
    HGAHeaderAdaptersReady = YES;
}

static BOOL HGAHeaderScrollInFlight(id categories, UIScrollView *collection) {
    UIScrollView *pager = HGAHomeChannelScroll(categories) ?: HGReadObjectGetter(categories,"contentScrollView");
    return collection.isTracking || collection.isDragging || collection.isDecelerating ||
        ([pager isKindOfClass:UIScrollView.class] && (pager.isTracking || pager.isDragging || pager.isDecelerating));
}

// A host can populate its titles after header layout. Notify the parent once
// for an actual change, not on each cell refresh or each scroll offset.
static void HGAHeaderCategoryGeometryChanged(UIView *categories) {
    UIView *header = categories.superview;
    while (header && !HGAIsHeaderView(header)) header = header.superview;
    HGAHeaderInsetState *state = objc_getAssociatedObject(header,&HGAHeaderInsetKey);
    if (!state || !HGAHeaderAdaptersReady) return;
    UICollectionView *collection = HGReadObjectGetter(categories,"collectionView");
    if (![collection isKindOfClass:UICollectionView.class] || HGAHeaderScrollInFlight(categories,collection)) return;
    NSArray *titles = HGReadObjectGetter(categories,"titles");
    if (![titles isKindOfClass:NSArray.class] || titles.count > 32) return;
    CGSize size = collection.bounds.size;
    BOOL ready=collection.delegate==(id)categories && collection.numberOfSections==1 &&
        [collection numberOfItemsInSection:0]==(NSInteger)titles.count;
    if (state.observedCollection == collection && CGSizeEqualToSize(state.observedSize,size) &&
        state.observedChannelListReady==ready && [state.observedTitles isEqualToArray:titles]) return;
    state.observedCollection = collection;
    state.observedChannelListReady=ready;
    state.observedTitles = titles;
    state.observedSize = size;
    [header setNeedsLayout];
}

// Capture original sizes through the verified flow delegate, never by moving or
// measuring reused cells. A nearly zero extent retains hidden entries' indices.
static HGAHeaderChannelLayout *HGAMakeChannelLayout(UIView *categories, UICollectionView *collection,
    NSArray *titles, NSUInteger mask) {
    CGFloat width = collection.bounds.size.width;
    NSUInteger visibleCount = titles.count - (NSUInteger)__builtin_popcountll(mask);
    CGFloat halfGap = visibleCount > 1 ? 10 : 0;
    NSMutableArray *frames = [NSMutableArray arrayWithCapacity:titles.count];
    CGFloat total = 0;
    for (NSUInteger i = 0; i < titles.count; i++) {
        CGFloat extent = 0.01;
        if (!(mask & (1UL << i))) {
            NSIndexPath *path = [NSIndexPath indexPathForItem:i inSection:0];
            CGSize size = ((CGSize (*)(id,SEL,id,id,id))HGAHeaderOriginals[0])(categories,
                @selector(collectionView:layout:sizeForItemAtIndexPath:), collection, collection.collectionViewLayout, path);
            if (!isfinite(size.width) || size.width <= 0 || size.height <= 0) return nil;
            extent = size.width + 2 * halfGap;
        }
        [frames addObject:[NSValue valueWithCGRect:CGRectMake(total,0,extent,collection.bounds.size.height)]];
        total += extent;
    }
    if (!visibleCount || !isfinite(width) || total > width) return nil;
    CGFloat inset = (width - total) / 2;
    for (NSUInteger i = 0; i < frames.count; i++)
        frames[i] = [NSValue valueWithCGRect:CGRectOffset([frames[i] CGRectValue],inset,0)];
    HGAHeaderChannelLayout *plan = [HGAHeaderChannelLayout new];
    plan.collection = collection;
    plan.frames = frames;
    plan.titles = titles;
    plan.hiddenMask = mask;
    plan.width = width;
    plan.inset = inset;
    plan.capsule = CGRectInset(CGRectMake(inset,0,total,collection.bounds.size.height),halfGap-16,0);
    return plan;
}

static void HGARefreshHeaderChannelLine(UIView *categories,BOOL completed) {
    HGAHeaderChannelLayout *plan = objc_getAssociatedObject(categories,&HGAHeaderLayoutKey);
    BOOL restoring = objc_getAssociatedObject(categories,&HGAHeaderLineRestoreKey) != nil;
    if ((!plan && !restoring) || !categories.window || HGFeedSidebarOwnsLayout(categories.window)) return;
    UICollectionView *collection = plan.collection ?: HGReadObjectGetter(categories,"collectionView");
    if (![collection isKindOfClass:UICollectionView.class] || !collection.numberOfSections ||
        HGAHeaderScrollInFlight(categories,collection)) return;
    UIView *header = categories.superview;
    while (header && !HGAIsHeaderView(header)) header = header.superview;
    BOOL animating = NO;
    if (HGABoolGetter(header,@"isHeaderViewLayoutAnimating",&animating) && animating) return;

    NSInteger index = ((NSInteger (*)(id,SEL))objc_msgSend)(categories,sel_registerName("selectedIndex"));
    if (index < 0 || index >= [collection numberOfItemsInSection:0] ||
        (plan && (index >= (NSInteger)plan.frames.count || (plan.hiddenMask & (1UL << index))))) return;
    CGRect target = plan ? plan.frames[index].CGRectValue :
        HGAChannelFrame(categories,sel_registerName("getTargetCellFrame:"),index);
    if (CGRectIsEmpty(target)) return;
    NSArray *lines = HGReadObjectGetter(categories,"indicators");
    if (![lines isKindOfClass:NSArray.class]) return;
    BOOL waitingForAnimation = NO;
    for (UIView *line in lines) {
        if (![line isKindOfClass:NSClassFromString(@"FQRJXCategoryIndicatorLineView")] ||
            ![line isDescendantOfView:collection]) continue;
        if (line.layer.animationKeys.count && !(completed && plan)) {
            waitingForAnimation = YES;
            continue;
        }
        CGPoint center = line.center;
        CGFloat x = [collection convertPoint:CGPointMake(CGRectGetMidX(target),CGRectGetMidY(target))
                                      toView:line.superview].x;
        if (!isfinite(x)) continue;
        if (completed && plan) [line.layer removeAllAnimations];
        if (fabs(x-center.x) > 0.25)
            [UIView performWithoutAnimation:^{line.center=CGPointMake(x,center.y);}];
    }
    if (restoring && !waitingForAnimation)
        objc_setAssociatedObject(categories,&HGAHeaderLineRestoreKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

static void HGASyncHeaderChannelLine(UIView *categories) {
    HGARefreshHeaderChannelLine(categories,NO);
    for (UIView *header=categories.superview;header;header=header.superview) {
        if (!HGAIsHeaderView(header)) continue;
        HGAHeaderInsetState *state=objc_getAssociatedObject(header,&HGAHeaderInsetKey);
        if (state.categories==categories) [state syncRestingChannel];
        break;
    }
}

static BOOL HGAHiddenChannelIndex(HGAHeaderChannelLayout *plan,NSInteger index) {
    return index>=0 && index<(NSInteger)plan.frames.count && (plan.hiddenMask & (1UL<<index));
}

static void HGASettleHomeChannel(UIView *categories) {
    HGAHeaderChannelLayout *plan=objc_getAssociatedObject(categories,&HGAHeaderLayoutKey);
    if (!plan || !categories.window || !HGAppearanceEnabled() ||
        HGFeedSidebarOwnsLayout(categories.window) || !HGAIsHomeChannel(categories) ||
        HGAHeaderScrollInFlight(categories,plan.collection)) return;
    HGAFixedChannelPager *state=objc_getAssociatedObject(categories,&HGAHeaderPagerKey);
    [state updateNativePan];
    NSInteger selected=((NSInteger (*)(id,SEL))objc_msgSend)(categories,sel_registerName("selectedIndex"));
    if (HGAHiddenChannelIndex(plan,selected)) {
        NSInteger nearest=NSNotFound;
        for (NSInteger index=0;index<(NSInteger)plan.frames.count;index++) {
            if (!(plan.hiddenMask & (1UL<<index)) &&
                (nearest==NSNotFound || labs(index-selected)<labs(nearest-selected))) nearest=index;
        }
        SEL select=sel_registerName("selectItemAtIndex:needAnimation:");
        Method method=class_getInstanceMethod(categories.class,select);
        if (nearest!=NSNotFound && method && !strcmp(method_getTypeEncoding(method),"B28@0:8q16B24") &&
            ((BOOL (*)(id,SEL,NSInteger,BOOL))objc_msgSend)(categories,select,nearest,NO)) selected=nearest;
    }
    UIScrollView *scroll=HGAHomeChannelScroll(categories);
    CGFloat stride=scroll ? CGRectGetWidth(scroll.bounds) : 0;
    if (selected>=0 && selected<(NSInteger)plan.frames.count && !HGAHiddenChannelIndex(plan,selected) && stride>0) {
        CGFloat page=scroll.contentOffset.x/stride;
        UIView *container=HGReadObjectGetter(categories,"listContainer");
        SEL current=sel_registerName("currentIndex"), click=sel_registerName("didClickSelectedItemAtIndex:");
        Method currentMethod=class_getInstanceMethod(object_getClass(container),current);
        BOOL hasCurrent=currentMethod && !strcmp(method_getTypeEncoding(currentMethod),"q16@0:8");
        NSInteger nativeIndex=hasCurrent ? ((NSInteger (*)(id,SEL))objc_msgSend)(container,current) : NSNotFound;
        BOOL hiddenOffset=isfinite(page) && page>=0 && page<(CGFloat)plan.frames.count &&
            HGAHiddenChannelIndex(plan,lround(page));
        if (hiddenOffset || HGAHiddenChannelIndex(plan,nativeIndex)) {
            // The selection veto can preserve the previous visible title
            // while the native scroll view has already reached a hidden page.
            // Restore its verified width × index position, then let the list
            // container finalize the transition if scrolling did not do so.
            [scroll setContentOffset:CGPointMake(stride*selected,scroll.contentOffset.y) animated:NO];
            Method clickMethod=class_getInstanceMethod(object_getClass(container),click);
            if (hasCurrent && ((NSInteger (*)(id,SEL))objc_msgSend)(container,current)!=selected &&
                clickMethod && !strcmp(method_getTypeEncoding(clickMethod),"v24@0:8q16"))
                ((void (*)(id,SEL,NSInteger))objc_msgSend)(container,click,selected);
        }
    }
    HGAApplyChannelIndicators(categories,YES);
    HGARefreshHeaderChannelLine(categories,YES);
}

static void HGAScheduleHomeChannelSettlement(UIView *categories) {
    if (!categories.window || objc_getAssociatedObject(categories,&HGAHeaderSettlementKey)) return;
    objc_setAssociatedObject(categories,&HGAHeaderSettlementKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    __weak UIView *weakCategories=categories;
    dispatch_async(dispatch_get_main_queue(),^{
        UIView *live=weakCategories;
        if (!live) return;
        HGASettleHomeChannel(live);
        objc_setAssociatedObject(live,&HGAHeaderSettlementKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    });
}

static void HGAHomeChannelPagerDidSettle(UIView *container) {
    UIScrollView *scroll=(id)HGAViewGetter(container,"contentScrollView",container);
    if (![scroll isKindOfClass:UIScrollView.class]) return;
    for (UIGestureRecognizer *recognizer in scroll.gestureRecognizers) {
        if (![recognizer isKindOfClass:HGAFixedChannelPan.class]) continue;
        HGAFixedChannelPager *state=(id)recognizer.delegate;
        if (state.categories) HGAScheduleHomeChannelSettlement(state.categories);
    }
}

static void HGASetChannelLayout(UIView *categories, HGAHeaderChannelLayout *next) {
    HGAHeaderChannelLayout *old = objc_getAssociatedObject(categories,&HGAHeaderLayoutKey);
    if (old != next) {
        objc_setAssociatedObject(categories,&HGAHeaderLayoutKey,next,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(categories,&HGAHeaderLineRestoreKey,next ? nil : @YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        [old.collection.collectionViewLayout invalidateLayout];
        if (old.collection != next.collection) [next.collection.collectionViewLayout invalidateLayout];
    }
    HGAUpdateChannelPager(categories,next);
    HGASyncHeaderChannelLine(categories);
    if (next && old != next) HGAScheduleHomeChannelSettlement(categories);
}

static void HGACompactHeaderChannels(UIView *header, HGAHeaderInsetState *state) {
    UIView *tabs = HGAViewGetter(header,"tabContainer",header);
    UIView *categories = nil;
    Class cls = NSClassFromString(@"SSHGBookStoreCategoryTitleView");
    if (HGAHeaderAdaptersReady && tabs && !tabs.hidden) {
        if ([state.categories isDescendantOfView:tabs]) categories=state.categories;
        else for (UIView *node in HGADescendants(tabs,64)) if ([node isKindOfClass:cls]) { categories=node; break; }
    }
    if (state.categories != categories) {
        HGASetChannelLayout(state.categories,nil);
        state.categories = categories;
    }
    if (!categories) return;
    BOOL animating=NO;
    if (HGABoolGetter(header,@"isHeaderViewLayoutAnimating",&animating) && animating) return;
    HGAHeaderChannelLayout *old = objc_getAssociatedObject(categories,&HGAHeaderLayoutKey);
    NSArray *titles = HGReadObjectGetter(categories,"titles");
    UICollectionView *collection = (id)HGAViewGetter(categories,"collectionView",categories);
    UICollectionViewFlowLayout *flow = [collection isKindOfClass:UICollectionView.class] ? (id)collection.collectionViewLayout : nil;
    if (![flow isKindOfClass:UICollectionViewFlowLayout.class] ||
        flow.scrollDirection != UICollectionViewScrollDirectionHorizontal || !CGSizeEqualToSize(flow.estimatedItemSize,CGSizeZero) ||
        collection.delegate != (id)categories ||
        collection.numberOfSections != 1 || ![titles isKindOfClass:NSArray.class] || !titles.count || titles.count > 32 ||
        [collection numberOfItemsInSection:0] != (NSInteger)titles.count) {
        HGASetChannelLayout(categories,nil);
        return;
    }
    if (HGAHeaderScrollInFlight(categories,collection)) return;
    state.observedCollection = collection;
    state.observedChannelListReady=YES;
    state.observedTitles = titles;
    state.observedSize = collection.bounds.size;
    NSUInteger mask = 0;
    for (NSUInteger i = 0; i < titles.count; i++) {
        if ([titles[i] isKindOfClass:NSString.class] && HGATopChannelHidden(titles[i])) mask |= 1UL << i;
    }
    if (!mask || (NSUInteger)__builtin_popcountll(mask) == titles.count) {
        HGASetChannelLayout(categories,nil);
        return;
    }
    if (old.collection == collection && old.hiddenMask == mask && old.width == collection.bounds.size.width &&
        [old.titles isEqualToArray:titles]) {
        HGASetChannelLayout(categories,old);
        return;
    }
    HGASetChannelLayout(categories,HGAMakeChannelLayout(categories,collection,titles,mask));
}

static void HGAStyleHeaderBackdrop(HGAAppearanceMaterial *glass,NSString *style) {
    BOOL flat=[style isEqual:@"flat"];
    NSString *material=flat ? @"solid" : style;
    BOOL reduced=UIAccessibilityIsReduceTransparencyEnabled();
    glass.untintedGlass=!flat;
    if(flat)glass.clipsToBounds=YES;
    if(![glass.style isEqual:material]) {
        glass.style=material;
        glass.materialSignature=nil;
        [glass setNeedsLayout];
    }
    UIUserInterfaceStyle desired=UIUserInterfaceStyleDark;
    if(glass.overrideUserInterfaceStyle!=desired) {
        glass.overrideUserInterfaceStyle=desired;
        if(@available(iOS 17.0,*))[glass updateTraitsIfNeeded];
    }
    glass.overlayColor=flat ? UIColor.tertiarySystemFillColor : (reduced ? UIColor.secondarySystemBackgroundColor : UIColor.clearColor);
    [glass requestMaterialRefresh];
}

static HGAAppearanceMaterial *HGAMakeHeaderCircleGlass(NSString *identifier,UIView *hitTarget) {
    HGAAppearanceMaterial *glass;
    Method gesture=hitTarget ? class_getInstanceMethod(HGAAppearanceMaterial.class,sel_registerName("_flexInteractionGestureView")) : NULL;
    if(gesture && !strcmp(method_getTypeEncoding(gesture),"@16@0:8")) {
        HGAHeaderTouchGlass *interactive=[[HGAHeaderTouchGlass alloc] initWithEffect:nil];
        interactive.channelControl=hitTarget;glass=interactive;
    }else glass=[[HGAAppearanceMaterial alloc] initWithEffect:nil];
    glass.accessibilityIdentifier=identifier;
    glass.userInteractionEnabled=NO;
    glass.accessibilityElementsHidden=YES;
    glass.layer.cornerCurve=kCACornerCurveContinuous;
    glass.clipsToBounds=YES;
    return glass;
}

static void HGARemoveHeaderToolButton(HGAHeaderToolButton *button) {
    button.toolPressed=NO;
    button.transform=CGAffineTransformIdentity;
    button.source=nil;
    [button removeFromSuperview];
}

static void HGAApplyHeaderTool(UIView *header,HGAHeaderInsetState *state,UIView *source,CGRect frame,
    NSString *style,BOOL sidebar) {
    NSString *identifier=sidebar ? @"honggo.appearance.header.sidebar" : @"honggo.appearance.header.search";
    SEL selector=sel_registerName(sidebar ? "simpleSideBarViewDidClicked" : "simpleSearchViewDidClicked");
    HGAAppearanceMaterial *glass=sidebar ? state.sidebarGlass : state.searchGlass;
    HGAHeaderToolButton *button=sidebar ? state.sidebarButton : state.searchButton;
    BOOL active=!CGRectIsNull(frame);
    BOOL flat=[style isEqual:@"flat"];
    BOOL glyphOnly=[style isEqual:@"glyph"];
    BOOL native=active && (flat || glyphOnly || HGNativeGlassConfigured()) && HGAHeaderToolOwner(source ?: header,selector);
    if (native) {
        if(!button) {
            button=[[HGAHeaderToolButton alloc] initWithFrame:frame];
            button.accessibilityIdentifier=identifier;
            // Keep the original glyph and badge visible above the button.
            [button addTarget:state action:@selector(activateTool:) forControlEvents:UIControlEventPrimaryActionTriggered];
        }
        BOOL nativeGlass=HGAUsesNativeGlass(style);
        NSString *configurationStyle=flat ? @"flat" : @"backdrop";
        if (![button.glassStyle isEqual:configurationStyle]) {
            button.glassStyle=configurationStyle;
            UIButtonConfiguration *configuration=nil;
            if (flat) {
                configuration=UIButtonConfiguration.filledButtonConfiguration;
                configuration.baseBackgroundColor=UIColor.tertiarySystemFillColor;
            }
            configuration.cornerStyle=UIButtonConfigurationCornerStyleCapsule;
            button.configuration=configuration;
            if (@available(iOS 26.0,*)) button.cornerConfiguration=UICornerConfiguration.capsuleConfiguration;
        }
        button.source=source;
        button.accessibilityLabel=source.accessibilityLabel ?: (sidebar ? @"菜单" : @"搜索");
        if (CGAffineTransformIsIdentity(button.transform)) {
            if (!CGRectEqualToRect(button.frame,frame)) button.frame=frame;
        } else {
            CGSize targetSize=frame.size;
            CGPoint targetCenter=CGPointMake(CGRectGetMidX(frame),CGRectGetMidY(frame));
            if (!CGSizeEqualToSize(button.bounds.size,targetSize)) button.bounds=(CGRect){CGPointZero,targetSize};
            if (!CGPointEqualToPoint(button.center,targetCenter)) button.center=targetCenter;
        }
        button.layer.zPosition=source ? -1 : 0;
        if (sidebar && !source && !button.menuGlyph) {
            UIImageSymbolConfiguration *ink=[UIImageSymbolConfiguration configurationWithPointSize:17 weight:UIImageSymbolWeightRegular];
            UIImage *symbol=[UIImage systemImageNamed:@"line.3.horizontal" withConfiguration:ink];
            UIImageView *glyph=[[UIImageView alloc] initWithImage:[symbol imageWithTintColor:UIColor.whiteColor renderingMode:UIImageRenderingModeAlwaysOriginal]];
            glyph.frame=button.bounds;
            glyph.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
            glyph.contentMode=UIViewContentModeCenter;
            glyph.isAccessibilityElement=NO;
            [button addSubview:glyph];
            button.menuGlyph=glyph;
        } else if (source && button.menuGlyph) {
            [button.menuGlyph removeFromSuperview];
            button.menuGlyph=nil;
        }
        if (button.superview!=header) [header addSubview:button];
        else {
            for (UIView *sibling in header.subviews.reverseObjectEnumerator) {
                if (sibling==button) break;
                if (HGAOwnView(sibling)) continue;
                [header bringSubviewToFront:button];
                break;
            }
        }
        if(!flat && !glyphOnly) {
            HGAViewState *material=HGAState(button,YES);
            if(!material.glass)material.glass=HGAMakeHeaderCircleGlass(@"honggo.appearance.material",button);
        }
        if(nativeGlass && !glyphOnly) {
            // The button remains the action target. Its configuration would install
            // a second system background behind the glyph, especially on bright footage.
            HGAInstallGlassInRect(button,style,button.bounds,CGRectGetHeight(frame)/2,0);
            HGAAppearanceMaterial *circle=HGAState(button,NO).glass;
            circle.interactiveGlass=YES;
            HGAStyleHeaderBackdrop(circle,style);
        }
        else if(!flat && !glyphOnly)HGAInstallButtonGlass(button,style,button.bounds,CGRectGetHeight(frame)/2);
        else HGARemoveGlass(button);
    } else {
        HGARemoveHeaderToolButton(button);
        button=nil;
    }
    if (active && !button) {
        if(!glass)glass=HGAMakeHeaderCircleGlass(identifier,nil);
        if(!CGRectEqualToRect(glass.frame,frame))glass.frame=frame;
        CGFloat radius=CGRectGetHeight(frame)/2;
        glass.materialCornerRadius=radius;
        if(glass.layer.cornerRadius!=radius)glass.layer.cornerRadius=radius;
        if(glass.superview!=header)[header insertSubview:glass atIndex:0];
        HGAStyleHeaderBackdrop(glass,style);
    } else {
        [glass removeFromSuperview];
        glass=nil;
    }
    if (sidebar) { state.sidebarButton=button; state.sidebarGlass=glass; }
    else { state.searchButton=button; state.searchGlass=glass; }
}

static void HGARemoveHeaderChannelControl(HGAHeaderInsetState *state) {
    [state.pageCoasting invalidate];
    state.pageCoasting=nil;
    [state.observedPagePan removeTarget:state action:@selector(followPagePan:)];
    state.observedPagePan=nil;
    state.scrubbingRail=NO;
    state.railDragging=NO;
    state.railVelocityX=0;
    state.liftedTitle.transform=CGAffineTransformIdentity;
    state.liftedTitle=nil;
    if (@available(iOS 26.0,*)) {
        if ([state hasChannelButtons])
            ((HGAHeaderGlassChannels *)state.channelControl).rail.transform=CGAffineTransformIdentity;
    }
    [state.channelLens removeFromSuperview];
    state.channelLens=nil;
    state.channelRefraction=nil;
    state.channelLensTracking=NO;
    state.channelLensSettling=NO;
    state.channelMotionGeneration++;
    [state.channelControl removeFromSuperview];
    state.channelControl=nil;
    state.channelStyle=nil;
    state.headerSelectionColor=HGASelectionColorDefault;
    HGASetHidden(state.channelCollection,NO);
    state.channelCollection=nil;
    state.channelIndices=nil;
    state.channelTitles=nil;
}

static BOOL HGAApplyHeaderChannelControl(UIView *header,HGAHeaderInsetState *state,CGRect frame,NSString *style,BOOL glassSelection,HGASelectionColorMode selectionColor) {
    UIView *categories=state.categories;
    HGAHeaderChannelLayout *layout=objc_getAssociatedObject(categories,&HGAHeaderLayoutKey);
    NSArray *titles=layout.titles ?: HGReadObjectGetter(categories,"titles");
    UICollectionView *collection=layout.collection ?: (id)HGAViewGetter(categories,"collectionView",categories);
    Method selected=class_getInstanceMethod(object_getClass(categories),sel_registerName("selectedIndex"));
    Method select=class_getInstanceMethod(object_getClass(categories),sel_registerName("selectItemAtIndex:needAnimation:"));
    if (!selected || strcmp(method_getTypeEncoding(selected),"q16@0:8") || !select ||
        strcmp(method_getTypeEncoding(select),"B28@0:8q16B24") || ![titles isKindOfClass:NSArray.class] ||
        ![collection isKindOfClass:UICollectionView.class] ||
        (collection!=state.channelCollection && !HGAVisible(collection))) {
        HGARemoveHeaderChannelControl(state);
        return NO;
    }
    if (!titles.count || titles.count>32) {
        HGARemoveHeaderChannelControl(state);
        return NO;
    }
    // The host supplies titles before collection layout has measured their
    // cells. Build the rail from that model even on the first layout pass.
    NSMutableArray<NSString *> *items=[NSMutableArray arrayWithCapacity:titles.count];
    NSMutableArray<NSNumber *> *indices=[NSMutableArray arrayWithCapacity:titles.count];
    for (NSUInteger index=0;index<titles.count;index++) {
        NSString *title=titles[index];
        if (![title isKindOfClass:NSString.class] || !title.length) {
            HGARemoveHeaderChannelControl(state);
            return NO;
        }
        if (HGATopChannelHidden(title)) continue;
        [items addObject:title];
        [indices addObject:@(index)];
    }
    NSUInteger visibleCount=items.count;
    if (!visibleCount) {
        HGARemoveHeaderChannelControl(state);
        return NO;
    }
    // Size the replacement from the model. Reused or partially clipped host
    // cells can report different extents while the feed scrolls.
    NSDictionary *ink=@{NSFontAttributeName:[UIFont systemFontOfSize:HGAChannelTitleSize weight:UIFontWeightSemibold]};
    CGFloat slotWidth=48;
    for (NSString *title in items) slotWidth=MAX(slotWidth,ceil([title sizeWithAttributes:ink].width)+6);
    CGFloat width=slotWidth*visibleCount;
    if (width>CGRectGetWidth(frame)) {
        HGARemoveHeaderChannelControl(state);
        return NO;
    }
    frame.origin.x=MAX(CGRectGetMinX(frame),MIN(CGRectGetMaxX(frame)-width,CGRectGetMidX(header.bounds)-width/2));
    frame.size.width=width;
    BOOL useGlass=![style isEqual:@"flat"] && HGNativeGlassConfigured() && !UIAccessibilityIsReduceTransparencyEnabled();
    NSString *controlStyle=useGlass ? style : @"flat";
    UIControl<HGAHeaderSelecting> *control=state.channelControl;
    if (collection!=state.channelCollection || ![indices isEqualToArray:state.channelIndices] ||
        ![items isEqualToArray:state.channelTitles] || ![controlStyle isEqual:state.channelStyle] ||
        glassSelection!=state.glassSelection || selectionColor!=state.headerSelectionColor) {
        HGARemoveHeaderChannelControl(state);
        control=nil;
        NSArray<UIButton *> *glassButtons=nil;
        if (@available(iOS 26.0,*)) {
            if (useGlass || glassSelection || selectionColor!=HGASelectionColorDefault) {
                HGAHeaderGlassChannels *glassControl=[[HGAHeaderGlassChannels alloc] initWithTitles:items material:controlStyle];
                glassControl.selectionColor=selectionColor;
                control=glassControl;
                glassButtons=glassControl.buttons;
            }
        }
        if (!control) {
            UISegmentedControl *segments=[[UISegmentedControl alloc] initWithItems:items];
            [segments setTitleTextAttributes:@{NSFontAttributeName:[UIFont systemFontOfSize:HGAChannelTitleSize weight:UIFontWeightMedium]} forState:UIControlStateNormal];
            [segments setTitleTextAttributes:ink forState:UIControlStateSelected];
            control=(UIControl<HGAHeaderSelecting> *)segments;
        }
        control.accessibilityIdentifier=@"honggo.appearance.material";
        control.overrideUserInterfaceStyle=UIUserInterfaceStyleDark;
        [control addTarget:state action:@selector(changeChannel:) forControlEvents:UIControlEventValueChanged];
        // Recognize on the actual hit control. A button's default action
        // otherwise takes priority over a gesture on its enclosing rail.
        for (UIView *hit in (glassButtons ?: @[control])) {
            UILongPressGestureRecognizer *scrub=[[UILongPressGestureRecognizer alloc] initWithTarget:state action:@selector(scrubChannels:)];
            scrub.minimumPressDuration=0;
            scrub.delaysTouchesBegan=(glassButtons==nil);
            scrub.cancelsTouchesInView=YES;
            scrub.delegate=state;
            [hit addGestureRecognizer:scrub];
        }
        state.channelControl=control;
        state.channelIndices=indices;
        state.channelStyle=controlStyle;
        state.glassSelection=glassSelection;
        state.headerSelectionColor=selectionColor;
        state.channelTitles=items;
        state.channelCollection=collection;
    }
    if (control.superview!=header) [header addSubview:control];
    if (!CGRectEqualToRect(control.frame,frame)) {
        control.frame=frame;
        [control setNeedsLayout];
        [control layoutIfNeeded];
    }
    [state prepareChannelLens];
    UIPanGestureRecognizer *pagePan=HGAHomeChannelScroll(categories).panGestureRecognizer;
    if (pagePan!=state.observedPagePan) {
        [state.observedPagePan removeTarget:state action:@selector(followPagePan:)];
        state.observedPagePan=pagePan;
        [pagePan addTarget:state action:@selector(followPagePan:)];
    }
    HGASyncHeaderChannelLine(categories);
    HGASetHidden(collection,YES);
    return YES;
}

static void HGARemoveHeaderDetachedGlass(UIView *header) {
    HGAHeaderInsetState *state=objc_getAssociatedObject(header,&HGAHeaderInsetKey);
    if(!state)return;
    HGARemoveHeaderChannelControl(state);
    HGARemoveHeaderToolButton(state.sidebarButton);
    HGARemoveHeaderToolButton(state.searchButton);
    HGASetChannelLayout(state.categories,nil);
    [state.sidebarGlass removeFromSuperview];
    [state.searchGlass removeFromSuperview];
    objc_setAssociatedObject(header,&HGAHeaderInsetKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

static CGRect HGAHeaderVisibleChannelsRect(UIView *header) {
    BOOL allChannelsHidden=HGATopChannelHidden(@"关注") && HGATopChannelHidden(@"漫剧") &&
        HGATopChannelHidden(@"真人剧") && HGATopChannelHidden(@"推荐");
    if(allChannelsHidden)return CGRectNull;
    HGAHeaderInsetState *state = objc_getAssociatedObject(header,&HGAHeaderInsetKey);
    HGAHeaderChannelLayout *plan = objc_getAssociatedObject(state.categories,&HGAHeaderLayoutKey);
    if (plan.collection) return [plan.collection convertRect:plan.capsule toView:header];
    UIView *tabs=HGAViewGetter(header,"tabContainer",header);
    if(!tabs)return CGRectInset(header.bounds,8.0,0);
    if(tabs.hidden || tabs.alpha<0.01 || CGRectIsEmpty(tabs.bounds))return CGRectNull;
    UIView *collection=HGAViewGetter(state.categories,"collectionView",tabs);
    if (collection && !CGRectIsEmpty(collection.bounds))
        return [collection convertRect:collection.bounds toView:header];
    Class cellCls=NSClassFromString(@"FQRJXCategoryTitleCell");
    CGRect unionRect=CGRectNull;
    if(cellCls) {
        for(UIView *node in HGADescendants(tabs,96)) {
            if(![node isKindOfClass:cellCls] || node.hidden || node.alpha<0.01 || CGRectIsEmpty(node.bounds))continue;
            CGRect r=[node convertRect:node.bounds toView:header];
            if(!isfinite(r.origin.x) || !isfinite(r.size.width) || r.size.width<8.0)continue;
            unionRect=CGRectIsNull(unionRect) ? r : CGRectUnion(unionRect,r);
        }
    }
    if(CGRectIsNull(unionRect)) {
        unionRect=[tabs convertRect:tabs.bounds toView:header];
        if(CGRectIsEmpty(unionRect))return CGRectNull;
        return unionRect;
    }
    return CGRectInset(unionRect,-16.0,0);
}

static BOOL HGAHeaderToolTransitionAnimating(UIView *header) {
    BOOL layout=NO,dismissing=NO;
    HGABoolGetter(header,@"isHeaderViewLayoutAnimating",&layout);
    HGABoolGetter(header,@"isSearchBarDismissAnimating",&dismissing);
    return layout || dismissing;
}

static BOOL HGABeginNativeHeaderToolTransition(UIView *header,BOOL animated) {
    HGAHeaderInsetState *state=objc_getAssociatedObject(header,&HGAHeaderInsetKey);
    if (!animated || !state.channelSelectionDepth || !state.visibleSideTools.count) return NO;
    state.ownsSideToolTransition=YES;
    return YES;
}

static void HGAFinishNativeHeaderToolTransition(UIView *header) {
    HGAHeaderInsetState *state=objc_getAssociatedObject(header,&HGAHeaderInsetKey);
    if (!HGAHeaderToolTransitionAnimating(header)) state.ownsSideToolTransition=NO;
}

static void HGAKeepHomeHeaderToolVisible(HGAHeaderInsetState *state,UIView *view,BOOL keep,BOOL repairing) {
    if (!view) return;
    if (!keep) { [state.visibleSideTools removeObject:view]; return; }
    if (CGRectIsEmpty(view.bounds)) return;
    if (HGAVisible(view) && view.alpha>=0.999) {
        if (!state.visibleSideTools) state.visibleSideTools=[NSHashTable weakObjectsHashTable];
        [state.visibleSideTools addObject:view];
    }
    if (!repairing || ![state.visibleSideTools containsObject:view]) return;
    BOOL fading=[view.layer animationForKey:@"opacity"]!=nil;
    if (!view.hidden && view.alpha>=0.999 && !fading) return;

    // The compact video channels use the same mounted native side tools. A
    // stock transition can briefly fade or hide those tools before settling.
    // Retain known visible instances rather than manufacturing any missing one.
    [UIView performWithoutAnimation:^{
        if (view.hidden) view.hidden=NO;
        if (view.alpha<0.999) view.alpha=1;
        if (fading) [view.layer removeAnimationForKey:@"opacity"];
    }];
}

static void HGAKeepCompactHeaderSideTools(UIView *header,HGAHeaderInsetState *state,UIView *sidebar,UIView *searchSlot) {
    HGAHeaderChannelLayout *plan=objc_getAssociatedObject(state.categories,&HGAHeaderLayoutKey);
    NSUInteger visibleCount=plan.titles.count-(NSUInteger)__builtin_popcountll(plan.hiddenMask);
    NSUInteger comic=[plan.titles indexOfObject:@"漫剧"],recommend=[plan.titles indexOfObject:@"推荐"];
    BOOL keepsTools=visibleCount==2 && comic<plan.titles.count && recommend<plan.titles.count &&
        !HGAHiddenChannelIndex(plan,(NSInteger)comic) && !HGAHiddenChannelIndex(plan,(NSInteger)recommend) &&
        HGAVisible(header) && !HGFeedSidebarOwnsLayout(header.window);
    if (!keepsTools) {
        [state.visibleSideTools removeAllObjects];
        state.ownsSideToolTransition=NO;
        return;
    }

    BOOL repairing=state.channelSelectionDepth || (state.ownsSideToolTransition && HGAHeaderToolTransitionAnimating(header));
    HGAKeepHomeHeaderToolVisible(state,sidebar,!HGAppearanceFlag(@"layout.channels.hide.sidebar"),repairing);
    UIView *search=HGAViewGetter(header,"searchViewContainer",header);
    BOOL keepSearch=!HGAppearanceFlag(@"layout.channels.hide.search") && searchSlot && searchSlot!=search;
    HGAKeepHomeHeaderToolVisible(state,search,keepSearch,repairing);
    HGAKeepHomeHeaderToolVisible(state,searchSlot,keepSearch,repairing);
    HGAKeepHomeHeaderToolVisible(state,HGAViewGetter(search,"searchButtonView",searchSlot),keepSearch,repairing);
}

// The verified home header routes native glass controls to the existing
// controller actions. Other header layouts retain their host hit targets.
static void HGAApplyHeaderSeparatedGlass(UIView *header,BOOL enabled,BOOL layoutEnabled,NSString *style) {
    if(!layoutEnabled || CGRectIsEmpty(header.bounds)) {
        HGARemoveHeaderDetachedGlass(header);
        HGARemoveGlass(header);
        return;
    }
    // Never churn header glass while the feed controller is sliding horizontally
    // for the sidebar drawer or pushing the search controller.
    if(header.window && fabs(CGRectGetMinX([header convertRect:header.bounds toView:header.window]))>24.0) {
        return;
    }
    HGAHeaderInsetState *state=objc_getAssociatedObject(header,&HGAHeaderInsetKey);
    if(!state) {
        state=[HGAHeaderInsetState new];
        objc_setAssociatedObject(header,&HGAHeaderInsetKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    if(HGFlatTopStyle())style=@"flat";
    else if(HGAUsesNativeGlass(style))style=HGALiquidButtonMaterial();
    HGASelectionColorMode headerSelectionColor=HGASelectionColorModeForKey(@"player.headerSelectionColor");
    BOOL glassSelection=HGNativeGlassConfigured() && !UIAccessibilityIsReduceTransparencyEnabled() &&
        [HGAppearanceString(@"player.headerSelectionStyle") isEqual:@"glass"];
    BOOL tracksChannels=state.channelControl && [state.channelStyle isEqual:style] &&
        state.glassSelection==glassSelection && state.headerSelectionColor==headerSelectionColor &&
        (state.channelLensTracking || state.channelLensSettling || state.channelSelectionDepth);
    // Compact remaining visible tabs when any channel is hidden so there is no
    // empty gap inside the center capsule.
    if (!tracksChannels || !enabled) HGACompactHeaderChannels(header,state);
    UIView *sidebar=HGAViewGetter(header,"sideBar",header);
    BOOL hideSidebar=HGAppearanceFlag(@"layout.channels.hide.sidebar");
    CGFloat side=MIN(HGAChannelRailHeight,CGRectGetHeight(header.bounds));
    // Some feed templates mount only the search tool. The drawer action
    // belongs to the feed controller and takes no icon or sender argument.
    BOOL missingSidebar=!sidebar && !hideSidebar && HGAVisible(header) &&
        HGAHeaderToolOwner(header,sel_registerName("simpleSideBarViewDidClicked"));
    CGRect fallbackMenu=missingSidebar ? CGRectMake(9,CGRectGetMidY(header.bounds)-side/2,side,side) : CGRectNull;
    if(!enabled) {
        [state.visibleSideTools removeAllObjects];
        HGARemoveHeaderChannelControl(state);
        HGARemoveHeaderToolButton(state.searchButton);
        state.searchButton=nil;
        HGAApplyHeaderTool(header,state,nil,fallbackMenu,@"glyph",YES);
        [state.searchGlass removeFromSuperview];
        HGARemoveGlass(header);
        return;
    }
    UIView *searchSlot=HGAHeaderSearchSlotView(header);
    HGAKeepCompactHeaderSideTools(header,state,sidebar,searchSlot);

    CGFloat headerW=CGRectGetWidth(header.bounds);
    CGFloat midY=CGRectGetMidY(header.bounds);

    BOOL showSidebar=!hideSidebar &&
        sidebar && !sidebar.hidden && sidebar.alpha>=0.01 && !CGRectIsEmpty(sidebar.bounds);
    BOOL showSearch=!HGAppearanceFlag(@"layout.channels.hide.search") &&
        searchSlot && !searchSlot.hidden && searchSlot.alpha>=0.01 && !CGRectIsEmpty(searchSlot.bounds);

    CGRect leftCircle=fallbackMenu;
    if(showSidebar) {
        CGRect r=[sidebar convertRect:sidebar.bounds toView:header];
        CGFloat x=MAX(4.0,MIN(headerW-side-4.0,CGRectGetMidX(r)-side*0.5));
        leftCircle=CGRectMake(x,CGRectGetMidY(r)-side*0.5,side,side);
    }
    HGAApplyHeaderTool(header,state,sidebar,leftCircle,missingSidebar && !HGNativeGlassConfigured() ? @"glyph" : style,YES);

    CGRect rightCircle=CGRectNull;
    if(showSearch) {
        CGRect r=[searchSlot convertRect:searchSlot.bounds toView:header];
        CGFloat x=MAX(4.0,MIN(headerW-side-4.0,CGRectGetMidX(r)-side*0.5));
        rightCircle=CGRectMake(x,CGRectGetMidY(r)-side*0.5,side,side);
    }
    HGAApplyHeaderTool(header,state,searchSlot,rightCircle,style,NO);

    if (state.channelControl && (tracksChannels || HGAHeaderToolTransitionAnimating(header))) {
        [state syncRestingChannel];
        return;
    }
    CGRect channels=HGAHeaderVisibleChannelsRect(header);
    if(!CGRectIsNull(channels)) {
        CGFloat minAllowed=!CGRectIsNull(leftCircle) ? CGRectGetMaxX(leftCircle)+8.0 : 8.0;
        CGFloat maxAllowed=!CGRectIsNull(rightCircle) ? CGRectGetMinX(rightCircle)-8.0 : headerW-8.0;
        CGFloat minX=MAX(minAllowed,CGRectGetMinX(channels));
        CGFloat maxX=MIN(maxAllowed,CGRectGetMaxX(channels));
        if(maxX-minX<side && maxAllowed-minAllowed>=side) { minX=minAllowed; maxX=maxAllowed; }
        if(maxX-minX>=24.0) {
            CGRect centerFrame=CGRectMake(minX,midY-side*0.5,maxX-minX,side);
            CGRect available=CGRectMake(minAllowed,centerFrame.origin.y,maxAllowed-minAllowed,side);
            if (HGAApplyHeaderChannelControl(header,state,available,style,glassSelection,headerSelectionColor)) {
                HGAViewState *material=HGAState(header,YES);
                [material.glass removeFromSuperview];
                material.glass=nil;
                HGASetGlassHostBackground(header,YES);
            } else {
                // A scrolling or unrecognized strip owns its original titles and touches.
                HGAInstallGlassInRect(header,[style isEqual:@"flat"] ? @"solid" : style,centerFrame,side*0.5,0);
                HGAStyleHeaderBackdrop(HGAState(header,NO).glass,style);
            }
            return;
        }
    }
    HGARemoveHeaderChannelControl(state);
    HGARemoveGlass(header);
}

void HGAppearanceUpdateSidebarLayout(UIWindow *window, BOOL suspended) {
    if (!window) return;
    for (UIView *view in HGAManaged.allObjects) {
        if (view.window != window) continue;
        if (HGAIsHeaderView(view)) {
            if (suspended) {
                HGARemoveHeaderDetachedGlass(view);
                HGARemoveGlass(view);
                HGARestoreGeometry(view);
            } else {
                [view setNeedsLayout];
            }
        } else if (HGAIsBottomControl(view)) {
            if (suspended) HGARelinquishBottomLayout(view);
            else [view setNeedsLayout];
        }
    }
}
