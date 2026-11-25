// Only the host short-video publishers use this gate. Keep PiP, audio and
// other owners of the app's MediaPlayer center on their native paths.
static char HGALastEpisodeMediaKey,HGAHeldEpisodeMediaKey;
static BOOL HGAHideEpisodeMedia;
static void (*HGAOriginalEpisodeMedia)(id,SEL,id);
static BOOL HGAEpisodeMediaSource(id player) {
    return [player isKindOfClass:objc_lookUpClass("FQVShortVideoListVCRemoteControlComponent")] ||
        [player isKindOfClass:objc_lookUpClass("FQVBackgroundPlayManager")];
}
static id HGASystemMediaCenter(void) {
    return HGReadObjectGetter(objc_lookUpClass("MPNowPlayingInfoCenter"),"defaultCenter");
}
static void HGAWithdrawEpisodeMedia(id center,id player) {
    if(objc_getAssociatedObject(center,&HGAHeldEpisodeMediaKey))return;
    id media=HGASystemMediaCenter();
    NSDictionary *shown=HGReadObjectGetter(media,"nowPlayingInfo");
    NSDictionary *published=objc_getAssociatedObject(center,&HGALastEpisodeMediaKey);
    if(shown && ([shown isEqual:published] || [shown isEqual:HGReadObjectGetter(player,"nowPlayingInfo")])) {
        // Clear only the dictionary we own. Calling the host's nil setter
        // here would also stop its remote-control event registration.
        ((void(*)(id,SEL,id))objc_msgSend)(media,sel_registerName("setNowPlayingInfo:"),nil);
    }
    objc_setAssociatedObject(center,&HGAHeldEpisodeMediaKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    objc_setAssociatedObject(center,&HGALastEpisodeMediaKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static void HGASetEpisodeMedia(id center,SEL selector,id info) {
    @synchronized(center) {
        id player=HGReadObjectGetter(center,"playingPlayer");
        BOOL episode=HGAEpisodeMediaSource(player);
        if(info && episode && __atomic_load_n(&HGAHideEpisodeMedia,__ATOMIC_ACQUIRE)) {
            HGAWithdrawEpisodeMedia(center,player);
            return;
        }
        BOOL held=objc_getAssociatedObject(center,&HGAHeldEpisodeMediaKey)!=nil;
        objc_setAssociatedObject(center,&HGAHeldEpisodeMediaKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(center,&HGALastEpisodeMediaKey,
            episode && [info isKindOfClass:NSDictionary.class] ? [info copy] : nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        // A late resignation from a suppressed episode must not clear media
        // that another publisher installed after it.
        if(!info && held && (!player || episode) && HGReadObjectGetter(HGASystemMediaCenter(),"nowPlayingInfo"))return;
        HGAOriginalEpisodeMedia(center,selector,info);
    }
}
static void HGAUpdateEpisodeMedia(BOOL foreground) {
    if(!HGAOriginalEpisodeMedia)return;
    BOOL hide=foreground && HGAppearanceEnabled() && HGAppearanceFlag(@"player.hideNowPlayingIsland");
    if(__atomic_exchange_n(&HGAHideEpisodeMedia,hide,__ATOMIC_ACQ_REL)==hide)return;
    id center=HGReadObjectGetter(objc_lookUpClass("FQVNowPlayingInfoCenter"),"defaultCenter");
    @synchronized(center) {
        id player=HGReadObjectGetter(center,"playingPlayer");
        if(hide) {
            if(HGAEpisodeMediaSource(player))HGAWithdrawEpisodeMedia(center,player);
        }else if(objc_getAssociatedObject(center,&HGAHeldEpisodeMediaKey)) {
            objc_setAssociatedObject(center,&HGAHeldEpisodeMediaKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            if(HGAEpisodeMediaSource(player) && !HGReadObjectGetter(HGASystemMediaCenter(),"nowPlayingInfo"))
                ((void(*)(id,SEL))objc_msgSend)(center,sel_registerName("refreshNowPlayingInfo"));
        }
    }
}
static void HGAInstallEpisodeMedia(void) {
    if(HGAOriginalEpisodeMedia)return;
    Class host=objc_lookUpClass("FQVNowPlayingInfoCenter"),media=objc_lookUpClass("MPNowPlayingInfoCenter");
    SEL setter=sel_registerName("setNowPlayingInfo:");
    Method write=class_getInstanceMethod(media,setter);
    Method refresh=class_getInstanceMethod(host,sel_registerName("refreshNowPlayingInfo"));
    if(!write || strcmp(method_getTypeEncoding(write),"v24@0:8@16") ||
        !refresh || strcmp(method_getTypeEncoding(refresh),"v16@0:8"))return;
    HGAOriginalEpisodeMedia=(void(*)(id,SEL,id))HGInstallVerifiedHook(host,setter,"v24@0:8@16",(IMP)HGASetEpisodeMedia,@"appearance");
    if(!HGAOriginalEpisodeMedia)return;
    for(NSString *name in @[UIApplicationWillEnterForegroundNotification,UIApplicationDidEnterBackgroundNotification]) {
        [NSNotificationCenter.defaultCenter addObserverForName:name object:nil queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification *note) {
            HGAUpdateEpisodeMedia([note.name isEqual:UIApplicationWillEnterForegroundNotification]);
        }];
    }
}
