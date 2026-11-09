// Only verified host tab titles; never style the adjacent badge label.
@interface HGTabTitleRecord : NSObject
@property(nonatomic, weak) UILabel *label;
@property(nonatomic, strong) UIColor *color;
@property(nonatomic, strong) UIFont *font;
@property(nonatomic, strong) UIColor *appliedColor;
@property(nonatomic, strong) UIFont *appliedFont;
- (void)restore;
@end
@implementation HGTabTitleRecord
- (void)restore {
    if ([self.label.textColor isEqual:self.appliedColor]) self.label.textColor = self.color;
    if ([self.label.font isEqual:self.appliedFont]) self.label.font = self.font;
}
@end
