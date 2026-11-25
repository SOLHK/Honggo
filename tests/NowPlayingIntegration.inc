// Exercise the host setter and lifecycle policy with an isolated media center.
// The simulator's actual Now Playing registration is never accessed.
#import <objc/message.h>
@interface HGMediaFixture : NSObject
@property(nonatomic,copy) NSDictionary *nowPlayingInfo;
+ (instancetype)defaultCenter;
@end
@implementation HGMediaFixture
+ (instancetype)defaultCenter {static HGMediaFixture *fixture;return fixture ?: (fixture=[self new]);}
@end
@interface HGEpisodeSourceFixture : NSObject
@property(nonatomic,copy) NSDictionary *nowPlayingInfo;
@end
@implementation HGEpisodeSourceFixture @end
@interface HGEpisodeCenterFixture : NSObject
@property(nonatomic,strong) id playingPlayer;
+ (instancetype)defaultCenter;
- (void)setNowPlayingInfo:(NSDictionary *)info;
- (void)refreshNowPlayingInfo;
@end
@implementation HGEpisodeCenterFixture
+ (instancetype)defaultCenter {static HGEpisodeCenterFixture *fixture;return fixture ?: (fixture=[self new]);}
- (void)setNowPlayingInfo:(NSDictionary *)info {HGMediaFixture.defaultCenter.nowPlayingInfo=info;}
- (void)refreshNowPlayingInfo {[self setNowPlayingInfo:HGReadObjectGetter(self.playingPlayer,"nowPlayingInfo")];}
@end
static Class HGMediaFixtureClass(const char *name) {
    if(!strcmp(name,"MPNowPlayingInfoCenter"))return HGMediaFixture.class;
    if(!strcmp(name,"FQVNowPlayingInfoCenter"))return HGEpisodeCenterFixture.class;
    if(!strcmp(name,"FQVShortVideoListVCRemoteControlComponent"))return HGEpisodeSourceFixture.class;
    return Nil;
}
#define objc_lookUpClass HGMediaFixtureClass
#include "../src/AppearanceNowPlaying.inc"
#undef objc_lookUpClass

static void RunEpisodeMediaChecks(void) {
    NSDictionary *saved=HGAppearanceValues();
    HGAInstallEpisodeMedia();
    HGEpisodeCenterFixture *center=HGEpisodeCenterFixture.defaultCenter;
    HGMediaFixture *system=HGMediaFixture.defaultCenter;
    HGEpisodeSourceFixture *source=[HGEpisodeSourceFixture new];
    NSDictionary *first=@{@"episode":@1},*second=@{@"episode":@2},*foreign=@{@"audio":@1};
    source.nowPlayingInfo=first;center.playingPlayer=source;
    HGAppearanceApply(@{@"enabled":@YES,@"player.hideNowPlayingIsland":@NO},nil);
    HGAUpdateEpisodeMedia(YES);
    [center setNowPlayingInfo:first];
    HGAppearanceSet(@"player.hideNowPlayingIsland",@YES,nil);HGAUpdateEpisodeMedia(YES);
    source.nowPlayingInfo=second;[center setNowPlayingInfo:second];
    Check(!system.nowPlayingInfo,@"enabling while playing clears the episode and holds the next foreground update");
    HGAUpdateEpisodeMedia(NO);
    Check([system.nowPlayingInfo isEqual:second],@"background entry republishes the current episode");
    HGAUpdateEpisodeMedia(YES);HGAppearanceSet(@"player.hideNowPlayingIsland",@NO,nil);HGAUpdateEpisodeMedia(YES);
    Check([system.nowPlayingInfo isEqual:second],@"disabling the setting immediately restores the current foreground episode");
    system.nowPlayingInfo=foreign;HGAppearanceSet(@"player.hideNowPlayingIsland",@YES,nil);HGAUpdateEpisodeMedia(YES);
    [center setNowPlayingInfo:second];HGAUpdateEpisodeMedia(NO);
    Check([system.nowPlayingInfo isEqual:foreign],@"hiding and restoring a drama never replace the audio that took ownership");
    HGAUpdateEpisodeMedia(YES);center.playingPlayer=[NSObject new];[center setNowPlayingInfo:foreign];
    Check([system.nowPlayingInfo isEqual:foreign],@"other host publishers keep their native setter");
    center.playingPlayer=nil;system.nowPlayingInfo=nil;HGAppearanceApply(saved,nil);HGAUpdateEpisodeMedia(NO);
}
