#import "Honggo.h"
#import "Appearance.h"
#import <dispatch/dispatch.h>
#include <math.h>

// Controlled host identities exercise production adapters, not the Hongguo IPA.
@interface HGAActionFixture : UIControl
@property(nonatomic,strong) UIImageView *iconImageView;
@property(nonatomic,strong) UILabel *countLabel;
@property(nonatomic) NSUInteger taps;
@end
@implementation HGAActionFixture
- (instancetype)initWithFrame:(CGRect)frame {
    if((self=[super initWithFrame:frame])) {
        _iconImageView=[[UIImageView alloc] initWithImage:[UIImage systemImageNamed:@"bolt"]];
        _iconImageView.frame=CGRectMake(12,0,30,30);[self addSubview:_iconImageView];
        _countLabel=[[UILabel alloc] initWithFrame:CGRectMake(0,35,54,20)];_countLabel.text=@"123";
        _countLabel.font=[UIFont systemFontOfSize:18];[self addSubview:_countLabel];
        [self addTarget:self action:@selector(hit:) forControlEvents:UIControlEventTouchUpInside];
    }return self;
}
- (void)hit:(id)sender{self.taps++;}
- (void)layoutSubviews{[super layoutSubviews];}
@end
@interface SSShortVideoRightInteractionLikeModel : NSObject
@property(nonatomic) BOOL isLiked;
@end
@implementation SSShortVideoRightInteractionLikeModel @end
@interface SSShortVideoRightInteractionLikeView : HGAActionFixture
@property(nonatomic,strong) SSShortVideoRightInteractionLikeModel *model;
@end
@implementation SSShortVideoRightInteractionLikeView @end
@interface SSShortVideoRightInteractionCollectView : HGAActionFixture
@property(nonatomic) BOOL isCollected;
@end
@implementation SSShortVideoRightInteractionCollectView @end
@interface SSShortVideoRightInteractionCommentView : HGAActionFixture @end
@implementation SSShortVideoRightInteractionCommentView @end
@interface SSShortVideoRightInteractionShareView : HGAActionFixture @end
@implementation SSShortVideoRightInteractionShareView @end
@interface SSVideoSeriesFeedHeaderView : UIView @end
@implementation SSVideoSeriesFeedHeaderView @end
@interface SSShortVideoBottomDistributionView : UIView @end
@implementation SSShortVideoBottomDistributionView @end
@interface SSShortVideoOutterSeriesInfoView : UIView @end
@implementation SSShortVideoOutterSeriesInfoView @end
@interface FQVShortVideoPlayerCellView : UIView @end
@implementation FQVShortVideoPlayerCellView @end
@interface TTPlayerView : UIView @end
@implementation TTPlayerView @end
@interface FQVCollectionView : UICollectionView @end
@implementation FQVCollectionView @end
@interface FQCShortVideoCommentPanelViewController : UIViewController @end
@implementation FQCShortVideoCommentPanelViewController @end
@interface SSVideoSharePanelViewController : UIViewController @end
@implementation SSVideoSharePanelViewController @end
@interface FQVShortVideoSettingsViewController : UIViewController @end
@implementation FQVShortVideoSettingsViewController @end
@interface FQVShortVideoNewSpeedSelectController : UIViewController @end
@implementation FQVShortVideoNewSpeedSelectController @end
@interface FQCShortVideoCommentPanelTitleView : UIView @end
@implementation FQCShortVideoCommentPanelTitleView @end
@interface SSTabBar : UIView
@property(nonatomic,strong) UIView *contentView;
@end
@implementation SSTabBar
- (instancetype)initWithFrame:(CGRect)frame{if((self=[super initWithFrame:frame])){_contentView=[[UIView alloc] initWithFrame:self.bounds];[self addSubview:_contentView];}return self;}
@end
@interface SSTabBarController : UIViewController
@property(nonatomic,strong) SSTabBar *customTabBar;
@property(nonatomic) NSUInteger selectedIndex;
@property(nonatomic) NSUInteger nativeTaps;
@end
@implementation SSTabBarController
- (void)viewDidLoad {
    [super viewDidLoad];self.view.backgroundColor=UIColor.systemBackgroundColor;
    self.customTabBar=[[SSTabBar alloc] initWithFrame:CGRectMake(0,self.view.bounds.size.height-90,self.view.bounds.size.width,60)];
    self.customTabBar.backgroundColor=UIColor.systemGrayColor;[self.view addSubview:self.customTabBar];
    NSArray *titles=@[@"首页",@"剧场",@"商城",@"我的"];
    for(NSUInteger i=0;i<4;i++) {
        UIControl *button=[[UIControl alloc] initWithFrame:CGRectMake(i*self.view.bounds.size.width/4,0,self.view.bounds.size.width/4,60)];
        button.tag=i;button.selected=i==0;
        UILabel *label=[[UILabel alloc] initWithFrame:button.bounds];label.text=titles[i];[button addSubview:label];
        [button addTarget:self action:@selector(nativeTap:) forControlEvents:UIControlEventTouchUpInside];
        [self.customTabBar.contentView addSubview:button];
    }
}
- (void)nativeTap:(UIControl *)button {
    self.nativeTaps++;self.selectedIndex=button.tag;
    for(UIControl *control in self.customTabBar.contentView.subviews)control.selected=control==button;
}
@end
@interface HGAAppearanceDock : UIView
- (void)setCollapsed:(BOOL)value animated:(BOOL)animated;
@end
static NSMutableArray *checks;
static void Check(BOOL passed,NSString *name){[checks addObject:@{@"name":name,@"passed":@(passed)}];NSLog(@"%@ %@",passed?@"PASS":@"FAIL",name);}
static UIView *Find(UIView *root,NSString *identifier) {
    if([root.accessibilityIdentifier isEqual:identifier])return root;
    for(UIView *child in root.subviews){UIView *match=Find(child,identifier);if(match)return match;}return nil;
}
static NSUInteger Count(UIView *root,NSString *identifier) {
    NSUInteger n=[root.accessibilityIdentifier isEqual:identifier];for(UIView *v in root.subviews)n+=Count(v,identifier);return n;
}
static BOOL ImageContentEqual(UIImage *actual,UIImage *expected) {
    if(actual==expected || [actual isEqual:expected])return YES;
    NSData *a=UIImagePNGRepresentation(actual),*b=UIImagePNGRepresentation(expected);
    BOOL equal=a.length && b.length && [a isEqual:b];
    if(!equal)NSLog(@"IMAGE_RESTORE expected=%@ actual=%@ bytes=%lu/%lu",expected,actual,(unsigned long)b.length,(unsigned long)a.length);
    return equal;
}
static void Layout(UIView *view){[view setNeedsLayout];[view layoutIfNeeded];}
@interface HGAAppearanceTests : UIResponder <UIWindowSceneDelegate>
@property(nonatomic,strong) UIWindow *window;
@property(nonatomic,strong) SSTabBarController *host;
@end
@implementation HGAAppearanceTests
- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)options {
    checks=[NSMutableArray array];HGAppearanceReset();
    HGSetEnabled(HGGlass,YES);HGPrepareNativeGlass();
    Check(HGInstallTabBar(),@"legacy tab-bar adapter installs alongside appearance");
    HGInstallAppearance();HGInstallAppearance();
    self.host=[SSTabBarController new];
    self.window=[[UIWindow alloc] initWithWindowScene:(UIWindowScene *)scene];self.window.rootViewController=self.host;[self.window makeKeyAndVisible];
    dispatch_async(dispatch_get_main_queue(),^{
        @try {[self runChecks];} @catch(NSException *e){Check(NO,[@"exception: " stringByAppendingString:e.description]);}
        [self finish];
    });
}
- (void)runChecks {
    UIView *root=self.host.view;
    SSShortVideoRightInteractionLikeView *like=[[SSShortVideoRightInteractionLikeView alloc] initWithFrame:CGRectMake(20,140,54,65)];
    like.model=[SSShortVideoRightInteractionLikeModel new];like.backgroundColor=UIColor.darkGrayColor;[root addSubview:like];Layout(like);
    UIImage *nativeImage=like.iconImageView.image;UIFont *nativeFont=like.countLabel.font;
    CGRect original=like.frame;
    Check(Count(like,@"honggo.appearance.material")==0,@"disabled appearance adds no playback glass");
    Check(CGAffineTransformIsIdentity(like.transform),@"disabled appearance preserves native geometry");
    Check(HGAppearanceApply(HGAppearancePreset(@"glass"),nil),@"visible preset can be applied");Layout(like);
    UIView *glass=Find(like,@"honggo.appearance.material");
    Check(glass!=nil && !glass.userInteractionEnabled,@"playback glass is attached and does not intercept touches");
    Check(like.iconImageView.image!=nativeImage,@"icon preset actually replaces the native image");
    Check([like.countLabel.text isEqual:@"123"],@"counts remain host-owned");
    for(NSUInteger i=0;i<5;i++)Layout(like);
    Check(Count(like,@"honggo.appearance.material")==1,@"repeated layout does not stack material views");
    for(NSUInteger cycle=0;cycle<3;cycle++) {
        HGAppearanceSet(@"icons.enabled",@NO,nil);
        Check(like.iconImageView.image==nativeImage,@"repeated icon toggles restore the same native image");
        if(like.iconImageView.image!=nativeImage)NSLog(@"ICON RESTORE native=%@ actual=%@",nativeImage,like.iconImageView.image);
        HGAppearanceSet(@"icons.enabled",@YES,nil);Layout(like);
    }
    HGAppearanceSet(@"icons.enabled",@NO,nil);
    like.iconImageView.image=[UIImage systemImageNamed:@"bolt.fill"];
    UIImage *updatedHostImage=like.iconImageView.image;
    HGAppearanceSet(@"icons.enabled",@YES,nil);Layout(like);
    HGAppearanceSet(@"icons.enabled",@NO,nil);
    Check(like.iconImageView.image==updatedHostImage,@"new host image becomes the next restoration baseline");
    like.iconImageView.image=nativeImage;
    HGAppearanceSet(@"icons.enabled",@YES,nil);Layout(like);
    [like sendActionsForControlEvents:UIControlEventTouchUpInside];Check(like.taps==1,@"native playback action executes exactly once");
    UIImage *unliked=like.iconImageView.image;
    like.model.isLiked=YES;Layout(like);
    Check(![like.iconImageView.image isEqual:unliked] && [like.iconImageView.tintColor isEqual:UIColor.systemPinkColor],@"like icon follows actual model selection state");
    like.model=[SSShortVideoRightInteractionLikeModel new];
    Check(![like.iconImageView.tintColor isEqual:UIColor.systemPinkColor],@"model replacement restores the unselected appearance");
    HGAppearanceApply(@{@"layout.like.x":@12,@"layout.like.y":@8,@"layout.like.scale":@0.9},nil);Layout(like);
    CGAffineTransform expected=like.transform;
    Check(fabs(expected.tx-12)<0.01 && fabs(expected.ty-8)<0.01 && fabs(expected.a-0.9)<0.01,@"per-control offsets and scale affect the actual view");
    for(NSUInteger i=0;i<6;i++)Layout(like);
    Check(CGAffineTransformEqualToTransform(expected,like.transform),@"repeated layout does not compound scaling or offsets");
    HGAppearanceSet(@"layout.like.hidden",@YES,nil);Check(like.hidden,@"per-control hide takes effect");
    HGAppearanceSet(@"layout.like.hidden",@NO,nil);Check(!like.hidden,@"unhide restores original visibility");
    HGAppearanceApply(@{@"fonts.enabled":@YES,@"fonts.scale":@0.8},nil);Layout(like);
    Check(fabs(like.countLabel.font.pointSize-nativeFont.pointSize*0.8)<0.01,@"font scaling changes native labels");
    Layout(like);Layout(like);Check(fabs(like.countLabel.font.pointSize-nativeFont.pointSize*0.8)<0.01,@"font scaling is not multiplied on reuse");
    HGAppearanceSet(@"fonts.enabled",@NO,nil);Check([like.countLabel.font isEqual:nativeFont],@"disabling font overrides restores the native font");

    SSShortVideoRightInteractionCollectView *collect=[[SSShortVideoRightInteractionCollectView alloc] initWithFrame:CGRectMake(90,140,54,65)];[root addSubview:collect];Layout(collect);
    collect.isCollected=YES;Check([collect.iconImageView.tintColor isEqual:UIColor.systemYellowColor],@"collection setter refreshes actual selected icon");
    for(Class cls in @[SSShortVideoRightInteractionCommentView.class,SSShortVideoRightInteractionShareView.class]) {
        HGAActionFixture *view=[[cls alloc] initWithFrame:CGRectMake(160,140,54,65)];[root addSubview:view];Layout(view);
        Check(Find(view,@"honggo.appearance.material")!=nil,@"comment and share actions receive scoped glass");
        [view sendActionsForControlEvents:UIControlEventTouchUpInside];Check(view.taps==1,@"comment and share preserve original actions");
    }
    SSVideoSeriesFeedHeaderView *header=[[SSVideoSeriesFeedHeaderView alloc] initWithFrame:CGRectMake(20,60,280,44)];[root addSubview:header];Layout(header);
    Check(Find(header,@"honggo.appearance.material")!=nil,@"top channel glass is active");
    SSShortVideoBottomDistributionView *episodes=[[SSShortVideoBottomDistributionView alloc] initWithFrame:CGRectMake(20,220,280,44)];[root addSubview:episodes];Layout(episodes);
    Check(Find(episodes,@"honggo.appearance.material")!=nil,@"episode entry glass is active");

    SSShortVideoOutterSeriesInfoView *info=[[SSShortVideoOutterSeriesInfoView alloc] initWithFrame:CGRectMake(20,280,280,60)];[root addSubview:info];
    UILabel *rich=[[UILabel alloc] initWithFrame:CGRectMake(0,0,180,24)];
    NSDictionary *attributes=@{NSFontAttributeName:[UIFont systemFontOfSize:20],NSForegroundColorAttributeName:UIColor.systemRedColor};
    rich.attributedText=[[NSAttributedString alloc] initWithString:@"Rich text" attributes:attributes];[info addSubview:rich];
    UITextField *secure=[[UITextField alloc] initWithFrame:CGRectMake(0,30,180,24)];secure.secureTextEntry=YES;secure.font=[UIFont systemFontOfSize:18];[info addSubview:secure];
    HGAppearanceApply(@{@"fonts.enabled":@YES,@"fonts.scale":@0.75},nil);Layout(info);
    Check(fabs([(UIFont *)[rich.attributedText attribute:NSFontAttributeName atIndex:0 effectiveRange:nil] pointSize]-15)<0.01,@"rich text font runs scale");
    Check([[rich.attributedText attribute:NSForegroundColorAttributeName atIndex:0 effectiveRange:nil] isEqual:UIColor.systemRedColor],@"rich text colors survive typography customization");
    Check(secure.font.pointSize==18,@"secure text fields remain untouched");
    HGAppearanceSet(@"fonts.enabled",@NO,nil);
    Check(fabs([(UIFont *)[rich.attributedText attribute:NSFontAttributeName atIndex:0 effectiveRange:nil] pointSize]-20)<0.01,@"rich text restores its original font runs");

    FQVShortVideoPlayerCellView *canvas=[[FQVShortVideoPlayerCellView alloc] initWithFrame:CGRectMake(10,350,300,320)];[root addSubview:canvas];
    TTPlayerView *render=[[TTPlayerView alloc] initWithFrame:CGRectMake(50,0,200,300)];[canvas addSubview:render];
    UIView *hud=[[UIView alloc] initWithFrame:CGRectMake(250,100,44,44)];[canvas addSubview:hud];CGRect hudFrame=hud.frame;
    HGAppearanceSet(@"player.fullscreen",@YES,nil);Layout(render);
    Check(render.transform.a>1 && fabs(render.transform.a-render.transform.d)<0.001,@"portrait fullscreen uses uniform rather than stretched scaling");
    Check(CGRectEqualToRect(hud.frame,hudFrame),@"fullscreen preserves HUD frame and hit target");
    HGAppearanceSet(@"player.fullscreen",@NO,nil);Check(CGAffineTransformIsIdentity(render.transform),@"fullscreen can be reversed");
    render.frame=CGRectMake(0,70,300,170);CGRect wide=render.frame;
    HGAppearanceSet(@"player.fullscreen",@YES,nil);Layout(render);
    Check(CGAffineTransformIsIdentity(render.transform) && CGRectEqualToRect(wide,render.frame),@"landscape video preserves size and placement");
    render.frame=CGRectMake(0,0,300,300);Layout(render);
    Check(CGAffineTransformIsIdentity(render.transform),@"square video remains native sized");

    for(Class cls in @[FQCShortVideoCommentPanelViewController.class,SSVideoSharePanelViewController.class,FQVShortVideoSettingsViewController.class,FQVShortVideoNewSpeedSelectController.class]) {
        UIViewController *panel=[cls new];[self.host addChildViewController:panel];[panel loadViewIfNeeded];
        panel.view.frame=CGRectMake(0,300,root.bounds.size.width,250);panel.view.backgroundColor=UIColor.whiteColor;
        UIView *backdrop=[[UIView alloc] initWithFrame:panel.view.bounds];backdrop.backgroundColor=UIColor.whiteColor;[panel.view addSubview:backdrop];
        UILabel *label=[[UILabel alloc] initWithFrame:CGRectMake(10,10,100,30)];label.text=@"original";[backdrop addSubview:label];
        [root addSubview:panel.view];[panel didMoveToParentViewController:self.host];Layout(panel.view);[panel viewDidLayoutSubviews];
        Check(Find(panel.view,@"honggo.appearance.material")!=nil,@"each panel category receives its own native material");
        Check([label.text isEqual:@"original"] && label.superview==backdrop,@"panel decoration preserves content and hierarchy");
        HGAppearanceSet(@"enabled",@NO,nil);
        Check(Find(panel.view,@"honggo.appearance.material")==nil && [backdrop.backgroundColor isEqual:UIColor.whiteColor],@"panel reset restores all owned backgrounds");
        [panel.view removeFromSuperview];[panel removeFromParentViewController];HGAppearanceSet(@"enabled",@YES,nil);
    }

    FQCShortVideoCommentPanelTitleView *commentSurface=[[FQCShortVideoCommentPanelTitleView alloc] initWithFrame:CGRectMake(20,300,280,52)];
    commentSurface.backgroundColor=UIColor.whiteColor;[root addSubview:commentSurface];Layout(commentSurface);
    Check(Find(commentSurface,@"honggo.appearance.material")!=nil,@"known comment surfaces work without a guessed controller class");
    HGAppearanceSet(@"panels.comment",@NO,nil);
    Check(Find(commentSurface,@"honggo.appearance.material")==nil && [commentSurface.backgroundColor isEqual:UIColor.whiteColor],@"comment surface restores without changing its parent");
    HGAppearanceSet(@"panels.comment",@YES,nil);[commentSurface removeFromSuperview];

    SSTabBar *bar=self.host.customTabBar;HGAppearanceLayoutDock(bar,self.host);Layout(bar);
    HGAAppearanceDock *dock=(id)Find(bar,@"honggo.appearance.dock");
    Check(dock!=nil && bar.contentView.hidden,@"custom floating dock replaces only the verified native content");
    Check(Find(dock,@"honggo.appearance.dock.extra")!=nil,@"independent circular button is available");
    UIButton *theater=(id)Find(dock,@"honggo.appearance.tab.theater");
    [theater sendActionsForControlEvents:UIControlEventTouchUpInside];
    Check(self.host.nativeTaps==1 && self.host.selectedIndex==1,@"custom tab forwards exactly one native navigation action");
    HGAppearanceSet(@"dock.content",@"icon",nil);
    Check(theater.configuration.title.length==0 && theater.configuration.image!=nil,@"icon-only content is actually rendered");
    HGAppearanceSet(@"dock.content",@"text",nil);
    Check(theater.configuration.image==nil && theater.configuration.title.length>0,@"text-only content is actually rendered");
    HGAppearanceSet(@"dock.content",@"both",nil);
    Check(theater.configuration.image!=nil && theater.configuration.title.length>0,@"mixed icon and text content is rendered");
    HGAppearanceSet(@"dock.hide.store",@YES,nil);
    Check(Find(dock,@"honggo.appearance.tab.store").hidden,@"individual tab visibility works");
    HGAppearanceSet(@"dock.height",@56,nil);
    Check(fabs(Find(dock,@"honggo.appearance.dock.capsule").bounds.size.height-56)<0.01,@"custom dock height respects available host space");
    HGAppearanceSet(@"dock.collapse",@YES,nil);[dock setCollapsed:YES animated:NO];
    UIButton *expand=(id)Find(dock,@"honggo.appearance.dock.expand");
    Check(!expand.hidden && expand.bounds.size.width>=44,@"collapsed dock exposes an accessible expansion button");
    [expand sendActionsForControlEvents:UIControlEventTouchUpInside];
    Check(expand.hidden && !Find(dock,@"honggo.appearance.dock.capsule").hidden,@"tapping collapsed dock expands it");
    for(NSUInteger i=0;i<5;i++)Layout(bar);
    Check(Count(bar,@"honggo.appearance.dock")==1,@"dock does not duplicate across repeated host layouts");
    HGAppearanceReset();Layout(like);Layout(bar);
    Check(CGRectEqualToRect(like.frame,original) && !like.hidden,@"reset restores original playback geometry and visibility");
    Check(like.iconImageView.image==nativeImage,@"reset restores original host image");
    if(like.iconImageView.image!=nativeImage)NSLog(@"FINAL ICON RESTORE native=%@ actual=%@",nativeImage,like.iconImageView.image);
    Check(Count(like,@"honggo.appearance.material")==0,@"reset removes playback materials");
    Check(!bar.contentView.hidden && !Find(bar,@"honggo.appearance.dock"),@"reset restores native tab content");
    HGAppearanceApply(HGAppearancePreset(@"glass"),nil);
    like.iconImageView.image=[UIImage systemImageNamed:@"leaf"];
    UIImage *hostUpdate=like.iconImageView.image;Layout(like);HGAppearanceReset();
    Check(ImageContentEqual(like.iconImageView.image,hostUpdate),@"a later native image becomes the restoration baseline");
    HGAppearanceApply(HGAppearancePreset(@"glass"),nil);like.model=nil;Layout(like);
    Check(ImageContentEqual(like.iconImageView.image,hostUpdate),@"missing host selection state retains the native icon");
    HGAppearanceReset();
    UIView *unrelated=[[UIView alloc] initWithFrame:CGRectMake(0,0,50,50)];unrelated.backgroundColor=UIColor.purpleColor;[root addSubview:unrelated];
    HGAppearanceApply(HGAppearancePreset(@"glass"),nil);HGAppearanceProcessView(unrelated);
    Check(unrelated.subviews.count==0 && [unrelated.backgroundColor isEqual:UIColor.purpleColor],@"unknown views remain untouched");
    Class settings=NSClassFromString(@"HGAAppearanceController");UITableViewController *page=[(UITableViewController *)[settings alloc] initWithStyle:UITableViewStyleInsetGrouped];
    [page loadViewIfNeeded];Check([page.tableView.dataSource tableView:page.tableView numberOfRowsInSection:0]>=10,@"appearance home exposes presets groups import export and reset");
    UISearchController *search=page.navigationItem.searchController;search.searchBar.text=@"layout.like";
    [search.searchResultsUpdater updateSearchResultsForSearchController:search];
    Check([page.tableView.dataSource tableView:page.tableView numberOfRowsInSection:0]==4,@"appearance search resolves stable per-control keys");
    HGAppearanceReset();
}
- (void)finish {
    NSUInteger failed=0;for(NSDictionary *c in checks)failed+=![c[@"passed"] boolValue];
    NSDictionary *report=@{@"plugin":@"Honggo",@"version":HG_PLUGIN_VERSION,@"status":failed?@"failed":@"passed",@"total":@(checks.count),@"failed":@(failed),@"checks":checks,@"scope":@"production visual code with controlled UIKit hosts; not Hongguo device acceptance"};
    NSURL *directory=[NSFileManager.defaultManager URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask].firstObject;
    [NSFileManager.defaultManager createDirectoryAtURL:directory withIntermediateDirectories:YES attributes:nil error:nil];
    [[NSJSONSerialization dataWithJSONObject:report options:NSJSONWritingPrettyPrinted error:nil] writeToURL:[directory URLByAppendingPathComponent:@"appearance-results.json"] atomically:YES];
}
@end
@interface HGAAppearanceAppDelegate : UIResponder <UIApplicationDelegate> @end
@implementation HGAAppearanceAppDelegate
- (UISceneConfiguration *)application:(UIApplication *)app configurationForConnectingSceneSession:(UISceneSession *)session options:(UISceneConnectionOptions *)options {
    UISceneConfiguration *config=[[UISceneConfiguration alloc] initWithName:@"Appearance" sessionRole:session.role];config.delegateClass=HGAAppearanceTests.class;return config;
}
@end
int main(int argc,char *argv[]){@autoreleasepool{return UIApplicationMain(argc,argv,nil,NSStringFromClass(HGAAppearanceAppDelegate.class));}}
