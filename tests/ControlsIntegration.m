#import "Honggo.h"

@interface SSMineHistoryActionControlsView : UIView
@property(nonatomic, strong) UIButton *sortButton;
@property(nonatomic, strong) UIButton *managementButton;
@property(nonatomic) NSUInteger taps;
@end
@implementation SSMineHistoryActionControlsView
- (instancetype)initWithFrame:(CGRect)frame {
    if ((self = [super initWithFrame:frame])) {
        _sortButton = [UIButton buttonWithType:UIButtonTypeCustom];
        [_sortButton setTitle:@"筛选" forState:UIControlStateNormal];
        _sortButton.backgroundColor = UIColor.redColor;
        [_sortButton addTarget:self action:@selector(tap:) forControlEvents:UIControlEventTouchUpInside];
        [self addSubview:_sortButton];
        _managementButton = [UIButton buttonWithType:UIButtonTypeSystem];
        _managementButton.configuration = UIButtonConfiguration.filledButtonConfiguration;
        [self addSubview:_managementButton];
    }
    return self;
}
- (void)tap:(id)sender { self.taps++; }
- (void)layoutSubviews {
    [super layoutSubviews];
    self.sortButton.frame = CGRectMake(0, 0, 70, 36);
    self.managementButton.frame = CGRectMake(80, 0, 70, 36);
}
@end

NSArray<NSDictionary *> *HGRunControlTests(UIView *root) {
    NSMutableArray *results = [NSMutableArray array];
    void (^check)(BOOL, NSString *) = ^(BOOL pass, NSString *name) {
        [results addObject:@{@"passed": @(pass), @"name": name}];
    };
    [NSUserDefaults.standardUserDefaults setBool:YES forKey:@"HongguoGlass.glassControls"];
    IMP before = class_getMethodImplementation(UIButton.class, @selector(layoutSubviews));
    HGInstallControls();
    check(before == class_getMethodImplementation(UIButton.class, @selector(layoutSubviews)), @"control styling does not hook UIKit button globally");
    SSMineHistoryActionControlsView *owner = [[SSMineHistoryActionControlsView alloc] initWithFrame:CGRectMake(20, 180, 180, 40)];
    [root addSubview:owner];
    [owner layoutSubviews];
    UIButton *button = owner.sortButton;
    NSArray *targets = button.allTargets.allObjects;
    UIVisualEffectView *glass = (UIVisualEffectView *)button.subviews.firstObject;
    check([glass isKindOfClass:UIVisualEffectView.class] &&
        [NSStringFromClass(glass.effect.class) isEqual:@"UIGlassEffect"], @"known history button receives native UIGlassEffect after attachment");
    [button setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];
    [owner layoutSubviews];
    check(glass.overrideUserInterfaceStyle == UIUserInterfaceStyleDark, @"white host title retains contrast with dark glass");
    [button setTitleColor:UIColor.blackColor forState:UIControlStateNormal];
    [owner layoutSubviews];
    check(glass.overrideUserInterfaceStyle == UIUserInterfaceStyleLight, @"black host title retains contrast with light glass");
    check(!glass.userInteractionEnabled && CGRectEqualToRect(glass.frame, button.bounds) &&
        CGRectEqualToRect(button.frame, CGRectMake(0, 0, 70, 36)), @"glass preserves button geometry and cannot intercept taps");
    [button sendActionsForControlEvents:UIControlEventTouchUpInside];
    check(owner.taps == 1 && [button.currentTitle isEqual:@"筛选"], @"host button title and target action survive glass styling");
    for (NSUInteger i = 0; i < 20; i++) [owner layoutSubviews];
    check(button.allTargets.count == targets.count && button.subviews.firstObject == glass,
        @"repeated layouts retain one material and one diagnostic target");
    check(owner.managementButton.configuration != nil &&
        ![owner.managementButton.subviews.firstObject.accessibilityIdentifier isEqual:@"honggo.control.glass"],
        @"UIKit configured button retains its existing appearance contract");
    [NSUserDefaults.standardUserDefaults setBool:NO forKey:@"HongguoGlass.glassControls"];
    [owner layoutSubviews];
    check(glass.superview == nil && [button.backgroundColor isEqual:UIColor.redColor], @"disabling custom glass restores host button fill");
    [owner removeFromSuperview];
    return results;
}
