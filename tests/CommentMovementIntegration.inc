@interface FQVShortVideoListVCNavigationComponent : NSObject
@property(nonatomic) BOOL nativeStickyTop;
@property(nonatomic) CGRect videoFrame;
@property(nonatomic) NSUInteger stickyOpens;
@property(nonatomic) NSUInteger ordinaryOpens;
@property(nonatomic) NSUInteger presentations;
@property(nonatomic) NSUInteger dismissals;
- (BOOL)enabledScaleStickyTop;
- (void)openModalNavWithVC:(UIViewController *)panel loopPlay:(BOOL)loop
             willShowBlock:(dispatch_block_t)willShow dismissBlock:(dispatch_block_t)dismiss;
@end

@implementation FQVShortVideoListVCNavigationComponent
- (BOOL)enabledScaleStickyTop {return self.nativeStickyTop;}
- (void)openModalNavWithVC:(UIViewController *)panel loopPlay:(BOOL)loop
             willShowBlock:(dispatch_block_t)willShow dismissBlock:(dispatch_block_t)dismiss {
    self.ordinaryOpens++;
    willShow();
    dismiss();
}
@end

@interface SSBizShortVideoCommentUtil : NSObject
+ (void)openCommentVCWithContext:(FQVShortVideoListVCNavigationComponent *)navigation;
@end

@implementation SSBizShortVideoCommentUtil
+ (void)openCommentVCWithContext:(FQVShortVideoListVCNavigationComponent *)navigation {
    if([navigation enabledScaleStickyTop]) {
        navigation.stickyOpens++;
        navigation.presentations++;
        navigation.videoFrame=CGRectMake(40,0,220,400);
    } else {
        [navigation openModalNavWithVC:[UIViewController new] loopPlay:YES willShowBlock:^{
            navigation.presentations++;
        } dismissBlock:^{
            navigation.dismissals++;
        }];
    }
}
@end

static void RunCommentMovementChecks(void) {
    NSDictionary *saved=HGAppearanceValues();
    FQVShortVideoListVCNavigationComponent *navigation=[FQVShortVideoListVCNavigationComponent new];
    CGRect original=CGRectMake(0,40,390,848);
    navigation.videoFrame=original;
    navigation.nativeStickyTop=YES;
    HGAppearanceApply(@{@"enabled":@NO,@"player.commentMovesVideo":@NO},nil);
    Check([navigation enabledScaleStickyTop],@"the movement switch does not alter navigation queries outside comment opening");
    [SSBizShortVideoCommentUtil openCommentVCWithContext:navigation];
    Check(navigation.ordinaryOpens==1 && !navigation.stickyOpens && navigation.presentations==1 &&
        navigation.dismissals==1 && CGRectEqualToRect(navigation.videoFrame,original) && [navigation enabledScaleStickyTop],
        @"comments use the native stationary presentation and preserve its callbacks with appearance disabled");
    HGAppearanceSet(@"player.commentMovesVideo",@YES,nil);
    [SSBizShortVideoCommentUtil openCommentVCWithContext:navigation];
    Check(navigation.stickyOpens==1 && navigation.ordinaryOpens==1 && navigation.presentations==2 &&
        !CGRectEqualToRect(navigation.videoFrame,original),@"enabling comment movement honors the host sticky-player route");
    navigation.nativeStickyTop=NO;
    [SSBizShortVideoCommentUtil openCommentVCWithContext:navigation];
    Check(navigation.stickyOpens==1 && navigation.ordinaryOpens==2,
        @"enabling comment movement leaves the final decision to the host");
    HGAppearanceApply(saved,nil);
}
