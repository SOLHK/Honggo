#include "AppearancePlayerGestures.inc"
// Host 7.4.3.33: each named object getter has @16@0:8. Keep the original
// controls, gestures and labels; decorate only their existing bounds.
static NSDictionary<NSString *,NSArray<NSString *> *> *HGAPlayerControlGetters(void) {
    static NSDictionary *getters;static dispatch_once_t once;
    dispatch_once(&once,^{getters=@{
        @"FQVShortVideoVPlayerControlView":@[@"fullscreenBtn"],
        @"SSShortVideoBottomInsertView":@[@"playRateButton",@"cleanScreenView"],
        @"SSShortVideoSingleBottomInsertView":@[@"episodeInfoView"],
        @"SSShortVideoAlbumSeriesBottomInsertView":@[@"containerView"],
        @"SSShortVideoPugcVideoBottomInsertView":@[@"commentContainerView"],
        @"SSShortVideoTagListBottomInsertView":@[@"episodeInfoView"],
        @"FQVShortVideoPlayerBottomView":@[],
        @"FQVShortVideoVPlayerEpisodeInfoView":@[],
        @"FQVShortVideoControlCleanSwitchView":@[],
        @"FQCShortVideoDanmakuEntranceView":@[],
        @"SSShortVideoRcmdReasonView":@[],
        @"SSShortVideoRcmdReasonTag":@[],
        @"SSShortVideoEpisodePanelCollectView":@[@"containerView",@"shareContainerView"],
        @"SSShortVideoEpisodesRelatedInfoView":@[@"contentView"],
        @"FQVShortVideoEpisodesNumberView":@[],
        @"FQVShortVideoEpisodesNumberCell":@[],
        @"SSShortVideoEpisodesViewItemCell":@[@"bgView"],
        @"SSShortVideoEpisodesViewSwitchView":@[@"bgView"]
    };});return getters;
}
static NSArray<NSString *> *HGAPlayerControlClasses(void) {return HGAPlayerControlGetters().allKeys;}
static BOOL HGAEpisodeControl(UIView *root) {
    return [@[@"SSShortVideoEpisodePanelCollectView",@"SSShortVideoEpisodesRelatedInfoView",
        @"FQVShortVideoEpisodesNumberView",@"FQVShortVideoEpisodesNumberCell",
        @"SSShortVideoEpisodesViewItemCell",@"SSShortVideoEpisodesViewSwitchView"] containsObject:NSStringFromClass(root.class)];
}
static HGAActionSheetState *HGAEpisodeSheetForControl(UIView *control) {
    for(UIView *node=control.superview;node;node=node.superview) {
        HGAActionSheetState *state=objc_getAssociatedObject(node,&HGAActionSheetKey);
        if(state && [control isDescendantOfView:state.surface] && [HGAPanelKind(HGAOwner(node)) isEqual:@"episodes"])
            return state;
    }
    return nil;
}
@interface HGAEpisodeCollectInk : NSObject
@property(nonatomic,weak) UILabel *label;
@property(nonatomic,strong) UIColor *sourceColor;
@property(nonatomic,strong) UIColor *appliedColor;
@property(nonatomic,weak) UIImageView *icon;
@property(nonatomic,strong) UIImage *sourceImage;
@property(nonatomic,strong) UIImage *appliedImage;
@property(nonatomic,strong) UIColor *sourceTint;
@property(nonatomic,strong) UIColor *appliedTint;
@end
@implementation HGAEpisodeCollectInk @end
static char HGAEpisodeCollectInkKey;
static void HGAApplyEpisodeCollectInk(UIView *root,BOOL enabled) {
    HGAEpisodeCollectInk *state=objc_getAssociatedObject(root,&HGAEpisodeCollectInkKey);
    if(!enabled) {
        if(state.appliedColor && [state.label.textColor isEqual:state.appliedColor])state.label.textColor=state.sourceColor;
        if(state.appliedTint && [state.icon.tintColor isEqual:state.appliedTint])state.icon.tintColor=state.sourceTint;
        if(state.icon.image==state.appliedImage)state.icon.image=state.sourceImage;
        objc_setAssociatedObject(root,&HGAEpisodeCollectInkKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);return;
    }
    UIView *label=HGAViewGetter(root,"textL",root),*icon=HGAViewGetter(root,"iconImageView",root);
    if(![label isKindOfClass:UILabel.class] || ![icon isKindOfClass:UIImageView.class]) {
        HGAApplyEpisodeCollectInk(root,NO);return;
    }
    if(state && (state.label!=label || state.icon!=icon)){HGAApplyEpisodeCollectInk(root,NO);state=nil;}
    if(!state){
        state=[HGAEpisodeCollectInk new];
        state.sourceColor=((UILabel *)label).textColor;state.sourceTint=icon.tintColor;
        objc_setAssociatedObject(root,&HGAEpisodeCollectInkKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    state.label=(id)label;state.icon=(id)icon;
    if(!state.appliedColor || ![state.label.textColor isEqual:state.appliedColor])state.sourceColor=state.label.textColor;
    if(!state.appliedTint || ![state.icon.tintColor isEqual:state.appliedTint])state.sourceTint=state.icon.tintColor;
    if(state.icon.image!=state.appliedImage) {
        state.sourceImage=state.icon.image;
        state.appliedImage=[state.sourceImage imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
    }
    HGAActionSheetState *sheet=HGAEpisodeSheetForControl(root);
    BOOL dark=(sheet && (sheet.surface.overrideUserInterfaceStyle==UIUserInterfaceStyleDark ||
        sheet.surface.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark)) ||
        root.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark;
    BOOL hostDark=NO;
    if(HGAHostExplicitDarkMode(root,&hostDark) && hostDark)dark=YES;
    UIColor *desired=dark ? UIColor.whiteColor : UIColor.labelColor;
    state.appliedColor=desired;state.appliedTint=desired;
    if(![state.label.textColor isEqual:desired])state.label.textColor=desired;
    if(![state.icon.tintColor isEqual:desired])state.icon.tintColor=desired;
    if(state.icon.image!=state.appliedImage)state.icon.image=state.appliedImage;
}
static char HGAEpisodeLayerPaintKey;
static CGRect HGAPlayerDetailEntryFrame(UIView *view,BOOL disc) {
    CGRect bounds=view.bounds;
    CGRect visible=[view convertRect:bounds toView:view.window];
    CGFloat width=bounds.size.width,height=bounds.size.height;
    CGFloat rowHeight=40;
    if(visible.size.height>rowHeight)height*=rowHeight/visible.size.height;
    if(disc) {
        if(visible.size.width>rowHeight)width*=rowHeight/visible.size.width;
        width=height=MIN(width,height);
    }
    return CGRectInset(bounds,(bounds.size.width-width)/2,(bounds.size.height-height)/2);
}
static void HGARestorePlayerControlGlass(UIView *target) {
    HGARemoveGlass(target);
    UIColor *paint=objc_getAssociatedObject(target,&HGAEpisodeLayerPaintKey);
    if(!paint)return;
    CGColorRef current=target.layer.backgroundColor;
    if(!current || CGColorGetAlpha(current)==0)target.layer.backgroundColor=paint.CGColor;
    objc_setAssociatedObject(target,&HGAEpisodeLayerPaintKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static void HGAPlayerControlGlass(UIView *target,BOOL sheet,BOOL episodeEntry) {
    if(!target.window || CGRectIsEmpty(target.bounds)){HGARestorePlayerControlGlass(target);return;}
    CGRect frame=episodeEntry ? HGAPlayerDetailEntryFrame(target,NO) : target.bounds;
    CGFloat radius=MIN(22,MIN(frame.size.width,frame.size.height)/2);
    if(episodeEntry) {
        // The native episode entries paint their color directly onto CALayer.
        // Preserve that paint separately if UIView doesn't report the color.
        CGColorRef paint=target.layer.backgroundColor;
        UIColor *background=target.backgroundColor;
        if(paint && CGColorGetAlpha(paint)>0 && (!background || !CGColorEqualToColor(paint,background.CGColor)))
            objc_setAssociatedObject(target,&HGAEpisodeLayerPaintKey,[UIColor colorWithCGColor:paint],OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    if(sheet) {
        NSString *panelStyle=HGAppearanceString(@"panels.material");
        if([target isKindOfClass:UIControl.class] ||
           [target.superview isKindOfClass:objc_lookUpClass("SSShortVideoEpisodePanelCollectView")]) {
            NSString *style=([panelStyle isEqual:@"regular"] || [panelStyle isEqual:@"clear"]) ?
                HGALiquidButtonMaterial() : panelStyle;
            HGAInstallAdaptiveButtonGlass(target,style,frame,radius);
        } else HGAInstallGlass(target,panelStyle,radius,0.04);
    } else HGAInstallButtonGlass(target,HGAppearanceString(@"player.material"),frame,radius);
    if(episodeEntry && target.layer.backgroundColor && CGColorGetAlpha(target.layer.backgroundColor)>0)
        target.layer.backgroundColor=nil;
}

@interface HGADanmakuLegacyInk : NSObject
@property(nonatomic,weak) UIImageView *view;
@property(nonatomic,strong) UIImage *original;
@property(nonatomic,strong) UIImage *foreground;
@end
@implementation HGADanmakuLegacyInk @end
static char HGADanmakuLegacyInkKey;

static UIImage *HGADanmakuForegroundImage(UIImage *image) {
    CGImageRef source=image.CGImage;
    if(!source)return nil;
    size_t width=CGImageGetWidth(source),height=CGImageGetHeight(source);
    if(!width || !height || width>256 || height>256)return nil;
    CGColorSpaceRef space=CGColorSpaceCreateDeviceRGB();
    CGContextRef context=CGBitmapContextCreate(NULL,width,height,8,width*4,space,
        kCGBitmapByteOrder32Big|kCGImageAlphaPremultipliedLast);
    CGColorSpaceRelease(space);
    if(!context)return nil;
    CGContextDrawImage(context,CGRectMake(0,0,width,height),source);
    UInt8 *pixels=CGBitmapContextGetData(context);
    // The host's compact gray-and-alpha artwork contains a dark plate baked
    // into its pixels. Its white mark is the foreground of our new glass.
    for(size_t i=0;i<width*height;i++) {
        UInt8 *pixel=pixels+4*i;
        unsigned value=MAX(pixel[0],MAX(pixel[1],pixel[2]));
        UInt8 alpha=value>48 ? (UInt8)((value-48)*255/207) : 0;
        pixel[0]=pixel[1]=pixel[2]=pixel[3]=alpha;
    }
    CGImageRef foreground=CGBitmapContextCreateImage(context);
    UIImage *result=foreground ? [[UIImage imageWithCGImage:foreground scale:image.scale orientation:image.imageOrientation]
        imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal] : nil;
    if(foreground)CGImageRelease(foreground);
    CGContextRelease(context);
    return result;
}

static void HGAApplyDanmakuLegacyInk(UIView *root,BOOL enabled) {
    HGADanmakuLegacyInk *state=objc_getAssociatedObject(root,&HGADanmakuLegacyInkKey);
    UIImageView *icon=(id)(enabled ? HGAViewGetter(root,"iconView",root) : nil);
    UIImage *legacy=enabled ? HGReadObjectGetter(root,"legacyImage") : nil;
    if(![legacy isKindOfClass:UIImage.class] || ![icon isKindOfClass:UIImageView.class])icon=nil;
    if(state && (state.view!=icon || ![state.original isEqual:legacy])) {
        if(state.view.image==state.foreground)state.view.image=state.original;
        objc_setAssociatedObject(root,&HGADanmakuLegacyInkKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        state=nil;
    }
    if(!icon || ![icon.image isEqual:legacy])return;
    if(!state) {
        UIImage *foreground=HGADanmakuForegroundImage(legacy);
        if(!foreground)return;
        state=[HGADanmakuLegacyInk new];
        state.view=icon;state.original=icon.image;state.foreground=foreground;
        objc_setAssociatedObject(root,&HGADanmakuLegacyInkKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    icon.image=state.foreground;
}
@interface HGAPlayerChromeState : NSObject
@property(nonatomic,strong) NSHashTable<UIView *> *targets;
@property(nonatomic,strong) NSHashTable<UIView *> *paints;
@end
@implementation HGAPlayerChromeState @end
static char HGAPlayerChromeKey;
static HGAPlayerChromeState *HGAPlayerChromeTargets(id owner,NSArray<UIView *> *targets,NSArray<UIView *> *paints) {
    HGAPlayerChromeState *state=objc_getAssociatedObject(owner,&HGAPlayerChromeKey);
    for(UIView *previous in state.targets)if(![targets containsObject:previous])HGARestorePlayerControlGlass(previous);
    for(UIView *previous in state.paints)if(![paints containsObject:previous])HGASetHidden(previous,NO);
    if(!targets.count && !paints.count){objc_setAssociatedObject(owner,&HGAPlayerChromeKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);return nil;}
    if(!state) {
        state=[HGAPlayerChromeState new];state.targets=[NSHashTable weakObjectsHashTable];state.paints=[NSHashTable weakObjectsHashTable];
        objc_setAssociatedObject(owner,&HGAPlayerChromeKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    [state.targets removeAllObjects];[state.paints removeAllObjects];
    for(UIView *target in targets)[state.targets addObject:target];
    for(UIView *paint in paints){[state.paints addObject:paint];HGASetHidden(paint,YES);}
    return state;
}

static UIView *HGALazyControlSubview(UIView *owner,const char *ivarName,const char *getterName) {
    if(!owner)return nil;
    Ivar ivar=ivarName ? class_getInstanceVariable(object_getClass(owner),ivarName) : NULL;
    const char *type=ivar ? ivar_getTypeEncoding(ivar) : NULL;
    if(type && type[0]=='@') {
        id value=object_getIvar(owner,ivar);
        return [value isKindOfClass:UIView.class] && (value==owner || [value isDescendantOfView:owner]) ? value : nil;
    }
    return getterName ? HGAViewGetter(owner,getterName,owner) : nil;
}

// 7.4.5.33 exposes the portrait player's playBtn as a lazy UIButton getter that
// constructs and mounts the center button on first read, and hides it via
// alpha=0 during active playback. Read its ivar directly and hold only its
// layer presentation opacity at 0 during non-user startup/scroll handoffs so
// host-owned alpha=0/1 state is never overwritten.
@interface HGAPlaybackButtonSuppression : NSObject
@property(nonatomic) NSUInteger generation;
@property(nonatomic) CFAbsoluteTime scheduledUntil;
@end
@implementation HGAPlaybackButtonSuppression @end
static char HGAPlaybackButtonKey,HGAPlaybackButtonSuppressKey,HGACellPlayStartKey,HGACellUserPausedKey;
static CFAbsoluteTime HGAPlaybackLaunchTimestamp;
static NSString * const HGAPlaybackGraceOpacity = @"honggo.playback.grace.opacity";

static void HGARecordCellPlaybackStart(UIView *cell) {
    if(!cell)return;
    objc_setAssociatedObject(cell,&HGACellUserPausedKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    objc_setAssociatedObject(cell,&HGACellPlayStartKey,@(CFAbsoluteTimeGetCurrent()),OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static void HGARecordUserTapPlay(UIView *hud,BOOL play) {
    UIView *cell=HGAPlayerCanvas(hud);
    if(!cell)return;
    if(play)HGARecordCellPlaybackStart(cell);
    else {
        objc_setAssociatedObject(cell,&HGACellUserPausedKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(cell,&HGACellPlayStartKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
}
static void HGARestorePlaybackButtonAlpha(UIButton *button) {
    if(!button)return;
    HGAPlaybackButtonSuppression *state=objc_getAssociatedObject(button,&HGAPlaybackButtonSuppressKey);
    if(state) {
        state.generation++;
        state.scheduledUntil=0;
    }
    if([button.layer animationForKey:HGAPlaybackGraceOpacity])
        [button.layer removeAnimationForKey:HGAPlaybackGraceOpacity];
}
static BOOL HGAPlaybackCanvasIsScrollingOrDisplaced(UIView *cell) {
    Class feedClass=objc_lookUpClass("FQVCollectionView");
    for(UIView *node=cell.superview;node && node!=cell.window;node=node.superview) {
        if(![node isKindOfClass:UIScrollView.class])continue;
        UIScrollView *scroll=(UIScrollView *)node;
        if(scroll.tracking || scroll.dragging || scroll.decelerating)return YES;
        if(feedClass && [scroll isKindOfClass:feedClass] && !CGRectIsEmpty(scroll.bounds)) {
            CGRect inFeed=[cell convertRect:cell.bounds toView:scroll];
            if(fabs(CGRectGetMinY(inFeed)-CGRectGetMinY(scroll.bounds))>1.5)return YES;
        }
    }
    return NO;
}
static BOOL HGAPlaybackShouldCoverButton(UIView *root,NSTimeInterval *outGrace) {
    if(outGrace)*outGrace=0;
    if(!HGAppearanceEnabled() || !root.window)return NO;
    UIView *cell=HGAPlayerCanvas(root);
    if(!cell || [objc_getAssociatedObject(cell,&HGACellUserPausedKey) boolValue])return NO;
    BOOL playing=NO;
    if(HGABoolGetter(cell,@"isPlaying",&playing) && playing)return YES;
    if(HGAPlaybackCanvasIsScrollingOrDisplaced(cell))return YES;
    BOOL userPlaying=NO;
    if(!HGABoolGetter(cell,@"isUserPlaying",&userPlaying) || !userPlaying)return NO;
    if(!HGAPlaybackLaunchTimestamp)HGAPlaybackLaunchTimestamp=CFAbsoluteTimeGetCurrent();
    CFAbsoluteTime now=CFAbsoluteTimeGetCurrent();
    NSTimeInterval remaining=MAX(0,HGAPlaybackLaunchTimestamp+2.4-now);
    NSNumber *cellStart=objc_getAssociatedObject(cell,&HGACellPlayStartKey);
    if(cellStart)remaining=MAX(remaining,cellStart.doubleValue+1.25-now);
    if(remaining>0) {
        if(outGrace)*outGrace=remaining;
        return YES;
    }
    return NO;
}
static BOOL HGAPlaybackShouldSuppressLoadingSpinner(UIView *spinner,NSTimeInterval *outGrace) {
    if(outGrace)*outGrace=0;
    if(!HGAppearanceEnabled() || !spinner.window)return NO;
    UIView *cell=HGAPlayerCanvas(spinner);
    if(!cell)return NO;
    BOOL playing=NO;
    if(HGABoolGetter(cell,@"isPlaying",&playing) && playing)return YES;
    if(HGAPlaybackCanvasIsScrollingOrDisplaced(cell))return YES;
    CFAbsoluteTime now=CFAbsoluteTimeGetCurrent();
    NSTimeInterval remaining=0;
    NSNumber *cellStart=objc_getAssociatedObject(cell,&HGACellPlayStartKey);
    if(cellStart)remaining=MAX(remaining,cellStart.doubleValue+0.85-now);
    if(remaining>0) {
        if(outGrace)*outGrace=remaining;
        return YES;
    }
    return NO;
}
static void HGAApplyLoadingSpinner(UIView *spinner) {
    if(!spinner)return;
    NSTimeInterval grace=0;
    if(HGAPlaybackShouldSuppressLoadingSpinner(spinner,&grace)) {
        if(![spinner.layer animationForKey:HGAPlaybackGraceOpacity]) {
            CABasicAnimation *cover=[CABasicAnimation animationWithKeyPath:@"opacity"];
            cover.fromValue=@0;
            cover.toValue=@0;
            cover.duration=0.001;
            cover.fillMode=kCAFillModeBoth;
            cover.removedOnCompletion=NO;
            [spinner.layer addAnimation:cover forKey:HGAPlaybackGraceOpacity];
        }
        if(grace>0) {
            HGAPlaybackButtonSuppression *state=objc_getAssociatedObject(spinner,&HGAPlaybackButtonSuppressKey);
            if(!state) {
                state=[HGAPlaybackButtonSuppression new];
                objc_setAssociatedObject(spinner,&HGAPlaybackButtonSuppressKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            }
            CFAbsoluteTime deadline=CFAbsoluteTimeGetCurrent()+grace+0.04;
            if(deadline>state.scheduledUntil+0.02) {
                state.scheduledUntil=deadline;
                NSUInteger gen=++state.generation;
                __weak UIView *weakSpinner=spinner;
                __weak HGAPlaybackButtonSuppression *weakState=state;
                dispatch_after(dispatch_time(DISPATCH_TIME_NOW,(int64_t)((grace+0.04)*NSEC_PER_SEC)),dispatch_get_main_queue(),^{
                    HGAPlaybackButtonSuppression *liveState=weakState;
                    UIView *liveSpinner=weakSpinner;
                    if(!liveState || liveState.generation!=gen)return;
                    liveState.scheduledUntil=0;
                    if(liveSpinner && !HGAApplying)HGAApplyLoadingSpinner(liveSpinner);
                });
            }
        }
    } else if([spinner.layer animationForKey:HGAPlaybackGraceOpacity]) {
        HGAPlaybackButtonSuppression *state=objc_getAssociatedObject(spinner,&HGAPlaybackButtonSuppressKey);
        if(state){state.generation++;state.scheduledUntil=0;}
        [spinner.layer removeAnimationForKey:HGAPlaybackGraceOpacity];
    }
}
static void HGAUpdateCellLoadingSpinners(UIView *owner) {
    Class loadingClass=objc_lookUpClass("FQVShortVideoLoadingView");
    if(!loadingClass || !owner)return;
    UIView *scope=HGAPlayerCanvas(owner) ?: owner;
    for(UIView *child in HGADescendants(scope,64))
        if([child isKindOfClass:loadingClass])HGAApplyLoadingSpinner(child);
}
static void HGAApplyPlaybackButton(UIView *root) {
    HGAUpdateCellLoadingSpinners(root);
    UIButton *mounted=root.window ? (id)HGALazyControlSubview(root,"_playBtn","playBtn") : nil;
    if(![mounted isKindOfClass:UIButton.class])mounted=nil;
    NSTimeInterval grace=0;
    BOOL shouldCover=mounted ? HGAPlaybackShouldCoverButton(root,&grace) : NO;
    if(mounted) {
        if(shouldCover) {
            if(![mounted.layer animationForKey:HGAPlaybackGraceOpacity]) {
                CABasicAnimation *cover=[CABasicAnimation animationWithKeyPath:@"opacity"];
                cover.fromValue=@0;
                cover.toValue=@0;
                cover.duration=0.001;
                cover.fillMode=kCAFillModeBoth;
                cover.removedOnCompletion=NO;
                [mounted.layer addAnimation:cover forKey:HGAPlaybackGraceOpacity];
            }
            if(grace>0) {
                HGAPlaybackButtonSuppression *state=objc_getAssociatedObject(mounted,&HGAPlaybackButtonSuppressKey);
                if(!state) {
                    state=[HGAPlaybackButtonSuppression new];
                    objc_setAssociatedObject(mounted,&HGAPlaybackButtonSuppressKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                }
                CFAbsoluteTime deadline=CFAbsoluteTimeGetCurrent()+grace+0.04;
                if(deadline>state.scheduledUntil+0.02) {
                    state.scheduledUntil=deadline;
                    NSUInteger gen=++state.generation;
                    __weak UIView *weakRoot=root;
                    __weak HGAPlaybackButtonSuppression *weakState=state;
                    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,(int64_t)((grace+0.04)*NSEC_PER_SEC)),dispatch_get_main_queue(),^{
                        HGAPlaybackButtonSuppression *liveState=weakState;
                        UIView *liveRoot=weakRoot;
                        if(!liveState || liveState.generation!=gen)return;
                        liveState.scheduledUntil=0;
                        if(liveRoot.window && !HGAApplying)HGAApplyPlaybackButton(liveRoot);
                    });
                }
            }
        } else {
            HGARestorePlaybackButtonAlpha(mounted);
        }
    }
    BOOL styled=root.window && HGAppearanceEnabled() && HGAppearanceFlag(@"player.rightGlass");
    UIButton *button=(styled && mounted.currentImage) ? mounted : nil;
    UIButton *previous=objc_getAssociatedObject(root,&HGAPlaybackButtonKey);
    if(previous!=button) {
        if(previous) {
            HGARestorePlaybackButtonAlpha(previous);
            HGARemoveGlass(previous);
        }
        objc_setAssociatedObject(root,&HGAPlaybackButtonKey,button,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    if(!button)return;
    CGRect bounds=button.bounds;
    UIImageView *icon=button.imageView;
    CGRect glyph=icon.image ? [icon convertRect:icon.bounds toView:button] : bounds;
    CGFloat side=MIN(MIN(CGRectGetWidth(bounds),CGRectGetHeight(bounds)),
        MIN(64,MAX(44,MAX(CGRectGetWidth(glyph),CGRectGetHeight(glyph))+8)));
    CGRect frame=CGRectMake(CGRectGetMidX(glyph)-side/2,CGRectGetMidY(glyph)-side/2,side,side);
    if(HGFlatPlayerStyle())HGAInstallFlatPlaybackButton(button,frame);
    else HGAInstallButtonGlass(button,HGALiquidButtonMaterial(),frame,side/2);
}
#include "AppearancePlayerBadges.inc"
static void HGAApplyPlayerControls(UIView *root) {
    NSString *name=NSStringFromClass(root.class);
    if([name isEqual:@"SSShortVideoRcmdReasonView"] || [name isEqual:@"SSShortVideoRcmdReasonTag"]) {
        HGAApplyPlayerBadge(root,[name isEqual:@"SSShortVideoRcmdReasonTag"]);
        return;
    }
    if([root isKindOfClass:objc_lookUpClass("FQVTransitionPlayerTopContentView")]) {
        for(HGAActionSheetMount *mount in HGAMountedActionSheets) {
            HGAActionSheetState *state=objc_getAssociatedObject(mount.sheetRoot,&HGAActionSheetKey);
            if(state.episodePreview==root){HGAAlignEpisodePreview(state,root);break;}
        }
        return;
    }
    if([root isKindOfClass:objc_lookUpClass("FQVShortVideoVPlayerControlView")]) {
        HGAUpdatePlayerGestures(root);
        HGAApplyPlaybackButton(root);
    }
    BOOL danmaku=[name isEqual:@"FQCShortVideoDanmakuEntranceView"];
    BOOL sheet=HGAEpisodeControl(root);
    BOOL enabled=HGAppearanceEnabled() && HGAppearanceFlag(sheet?@"panels.episodes":@"player.controlsGlass") && root.window;
    BOOL ranges=[name isEqual:@"FQVShortVideoEpisodesNumberView"];
    if(ranges || [name isEqual:@"FQVShortVideoEpisodesNumberCell"]) {
        HGAActionSheetState *state=enabled ? HGAEpisodeSheetForControl(root) : nil;
        if(ranges) {
            NSMutableArray *fades=[NSMutableArray array];
            if(state) {
                HGAClearSheetBackground(state,root);
                if([root.superview isKindOfClass:UICollectionReusableView.class])
                    HGAClearSheetCover(state,root.superview);
                UICollectionView *list=(id)HGAViewGetter(root,"collectionView",root);
                if([list isKindOfClass:UICollectionView.class]) {
                    HGAClearSheetBackground(state,list);
                    HGAClearSheetCover(state,list.backgroundView);
                }
                for(NSString *getter in @[@"leftGradientView",@"rightGradientView"]) {
                    UIView *fade=HGAViewGetter(root,getter.UTF8String,root);
                    if(fade)[fades addObject:fade];
                }
            }
            HGAPlayerChromeTargets(root,nil,fades);
        } else if(state) {
            UICollectionViewCell *cell=(id)root;
            HGAClearSheetCover(state,cell);
            HGAClearSheetCover(state,cell.contentView);
            HGAClearSheetCover(state,cell.backgroundView);
            UILabel *label=(id)HGAViewGetter(cell,"titleLabel",cell);
            if(HGASheetUsesConvertedInk(state) && [label isKindOfClass:UILabel.class])HGAApplySheetLabel(state,label);
        }
        return;
    }
    if([root isKindOfClass:NSClassFromString(@"FQVShortVideoVPlayerEpisodeInfoView")]) {
        HGARegisterView(root);
        if(enabled)HGAPlayerControlGlass(root,NO,YES);
        else HGARestorePlayerControlGlass(root);
        return;
    }
    if([root isKindOfClass:NSClassFromString(@"FQVShortVideoControlCleanSwitchView")]) {
        HGARegisterView(root);
        if(!enabled || CGRectIsEmpty(root.bounds)){HGARemoveGlass(root);return;}
        CGRect frame=HGAPlayerDetailEntryFrame(root,YES);
        HGAInstallButtonGlass(root,HGALiquidButtonMaterial(),frame,frame.size.width/2);
        return;
    }
    if([root isKindOfClass:NSClassFromString(@"FQVShortVideoPlayerBottomView")]) {
        // This tray fades in after its native children have completed layout.
        // Attach to the existing controls even during that fade.
        for(UIView *child in root.subviews)
            if([child isKindOfClass:objc_lookUpClass("FQVShortVideoVPlayerEpisodeInfoView")] ||
               [child isKindOfClass:objc_lookUpClass("FQVShortVideoControlCleanSwitchView")])
                HGAApplyPlayerControls(child);
        HGRefreshVideoBackdropForPlayer(root);
        return;
    }
    if([root isKindOfClass:NSClassFromString(@"SSShortVideoBottomInsertView")]) {
        UIView *cleanSwitch=HGAViewGetter(root,"cleanSwitchView",root);
        if(cleanSwitch && cleanSwitch!=root)HGAApplyPlayerControls(cleanSwitch);
    }
    if(!enabled) {
        HGAPlayerChromeTargets(root,nil,nil);HGAApplyEpisodeCollectInk(root,NO);
        if(danmaku)HGAApplyDanmakuLegacyInk(root,NO);
        return;
    }
    NSMutableArray *targets=[NSMutableArray array],*paints=[NSMutableArray array];
    UIView *episodeEntry=nil;
    for(Class cls=object_getClass(root);cls && cls!=UIView.class;cls=class_getSuperclass(cls)) {
        for(NSString *getter in HGAPlayerControlGetters()[NSStringFromClass(cls)]) {
            UIView *target=[getter isEqual:@"fullscreenBtn"] ?
                HGALazyControlSubview(root,"_fullscreenBtn","fullscreenBtn") :
                HGAViewGetter(root,getter.UTF8String,root);
            if(!target)continue;
            if([getter isEqual:@"episodeInfoView"]) {
                if([target isKindOfClass:NSClassFromString(@"FQVShortVideoVPlayerEpisodeInfoView")]) {
                    HGAApplyPlayerControls(target);
                    continue;
                }
                episodeEntry=target;
            }
            [targets addObject:target];
        }
    }
    if(danmaku) {
        UIView *capsule=HGAViewGetter(root,"capsuleView",root);
        // updateStyleVisibility keeps capsuleView mounted and hides it in
        // compact mode. clickView returns the actual native toggle button.
        UIView *target=HGAVisible(capsule) ? capsule : HGAViewGetter(root,"clickView",root);
        BOOL visible=HGAVisible(target);
        if(visible)[targets addObject:target];
        HGAApplyDanmakuLegacyInk(root,visible && target!=capsule);
    }
    if([name isEqual:@"SSShortVideoDanmakuLeftAboveView"]) {
        UIView *entrance=HGAViewGetter(root,"danmakuEntranceView",root);
        if(entrance)HGAApplyPlayerControls(entrance);
        else {UIView *target=HGAViewGetter(root,"clickView",root);if(target)[targets addObject:target];}
    }
    // The collect button's orange paint is a separate gradient view, not its
    // tappable container. Hide only that named paint carrier, retaining the
    // foreground and the host's selected-state label/icon updates.
    if([name isEqual:@"SSShortVideoEpisodePanelCollectView"]) {
        HGAApplyEpisodeCollectInk(root,enabled);
        for(NSString *getter in @[@"bgView",@"shareBgView"]) {
            UIView *paint=HGAViewGetter(root,getter.UTF8String,root);
            if(paint)[paints addObject:paint];
        }
    }
    HGAPlayerChromeTargets(root,targets,paints);
    for(UIView *target in targets)HGAPlayerControlGlass(target,sheet,target==episodeEntry);
    if([name isEqual:@"SSShortVideoEpisodesViewItemCell"]) {
        // The host's custom setIsSelected: paints bgView independently of
        // UICollectionViewCell.selected. Preserve that authored state as tint.
        for(UIView *target in targets)HGASetGlassOverlay(target,
            [HGAState(target,NO).background colorWithAlphaComponent:0.18] ?: UIColor.clearColor);
        HGAActionSheetState *state=HGAEpisodeSheetForControl(root);
        UILabel *label=(id)HGAViewGetter(root,"textLabel",root);
        if(HGASheetUsesConvertedInk(state) && [label isKindOfClass:UILabel.class])HGAApplySheetLabel(state,label);
    }
}

static CGRect HGAPlayerNavigationContent(UIView *view,UIView *bar) {
    if(!HGAVisible(view))return CGRectNull;
    if([view isKindOfClass:UIButton.class]) {
        UIImageView *image=((UIButton *)view).imageView;
        if(image.image && HGAVisible(image))view=image;
    }
    CGRect bounds=view.bounds;
    if([view isKindOfClass:UILabel.class]) {
        UILabel *label=(UILabel *)view;
        if(!label.text.length)return CGRectNull;
        bounds=[label textRectForBounds:bounds limitedToNumberOfLines:label.numberOfLines];
    }
    return CGRectIsEmpty(bounds) ? CGRectNull : [view convertRect:bounds toView:bar];
}

static UIView *HGAPlayerDetailNavigationTitle(UIView *bar) {
    // Detail playback can put the episode label on the enclosing controller's
    // own navigation view. The player component owns the adjacent back arrow.
    Class detail=NSClassFromString(@"SSShortVideoDetailViewController");
    Class album=NSClassFromString(@"SSShortVideoAlbumDetailViewController");
    for(UIViewController *owner=HGAOwner(bar);owner;owner=owner.parentViewController) {
        if([owner isKindOfClass:detail] || [owner isKindOfClass:album]) {
            UIView *label=HGAViewGetter(owner,"navLeftLabel",owner.view);
            return label.window==bar.window ? label : nil;
        }
    }
    return nil;
}

static void HGAApplyPlayerNavigation(id component) {
    if(!NSThread.isMainThread || HGAApplying)return;
    HGAApplying=YES;
    @try {
        if(!HGAPlayerNavigations)HGAPlayerNavigations=[NSHashTable weakObjectsHashTable];
        [HGAPlayerNavigations addObject:component];
        HGAApplyCleanNavigation(component);
        if(!HGAppearanceEnabled() || !HGAppearanceFlag(@"player.controlsGlass")) {
            HGAPlayerChromeTargets(component,nil,nil);return;
        }
        id candidate=HGReadObjectGetter(component,"navBar");
        if(![candidate isKindOfClass:UIView.class]){HGAPlayerChromeTargets(component,nil,nil);return;}
        UIView *bar=candidate;
        if(!bar.window){HGAPlayerChromeTargets(component,nil,nil);return;}
        [bar layoutIfNeeded];
        NSMutableArray *targets=[NSMutableArray array];
        UIView *back=HGAViewGetter(component,"backBtn",bar);
        CGRect backInk=HGAPlayerNavigationContent(back,bar);
        CGRect title=HGAPlayerNavigationContent(HGAViewGetter(component,"navLeftLabel",bar),bar);
        if(CGRectIsNull(title))title=HGAPlayerNavigationContent(HGAPlayerDetailNavigationTitle(bar),bar);
        CGRect left=CGRectIsNull(backInk) ? CGRectNull : CGRectUnion(backInk,title);
        UIView *rate=HGAViewGetter(component,"playRateView",bar);
        CGRect rateInk=rate ? CGRectUnion(HGAPlayerNavigationContent(HGAViewGetter(component,"playRateImgView",rate),bar),
            HGAPlayerNavigationContent(HGAViewGetter(component,"playRateLabel",rate),bar)) : CGRectNull;
        if(CGRectIsNull(rateInk))rateInk=HGAPlayerNavigationContent(rate,bar);
        UIView *more=HGAViewGetter(component,"moreBtn",bar);
        CGRect moreInk=HGAPlayerNavigationContent(more,bar);
        CGRect anchor=!CGRectIsNull(backInk) ? [back convertRect:back.bounds toView:bar] :
            (!CGRectIsNull(rateInk) ? rateInk : moreInk);
        if(CGRectIsNull(anchor)){HGAPlayerChromeTargets(component,nil,nil);return;}
        CGFloat height=34,centerY=CGRectGetMidY(anchor);
        UIView *controls[]={bar,rate,more};
        CGRect content[]={left,rateInk,moreInk};
        NSString *style=HGAppearanceString(@"player.material");
        for(NSUInteger i=0;i<3;i++) {
            UIView *target=controls[i];
            if(CGRectIsNull(content[i]))continue;
            CGFloat width=MAX(height,CGRectGetWidth(content[i])+14);
            CGRect pill=CGRectMake(CGRectGetMidX(content[i])-width/2,centerY-height/2,width,height);
            HGAInstallButtonGlass(target,style,[bar convertRect:pill toView:target],height/2);
            if(i==0 && back)HGAState(target,NO).glass.flexTouchHost=back;
            [targets addObject:target];
        }
        HGAPlayerChromeTargets(component,targets,nil);
    } @finally {HGAApplying=NO;}
}

// The episode sheet owns a bounded content root in the exported hierarchy.
// Do not decorate its full-screen modal dismissal window.
static void HGAApplyEpisodeController(UIViewController *controller) {
    UIView *root=controller.view;
    BOOL enabled=HGAppearanceEnabled() && HGAppearanceFlag(@"panels.episodes") && root.window;
    CGRect rect=root.window?[root convertRect:root.bounds toView:root.window]:CGRectZero;
    if(rect.size.height>root.window.bounds.size.height*0.94)enabled=NO;
    if(enabled)HGAApplyActionSheet(root,@"episodes");
    else HGARestoreActionSheet(root);
    // Concrete surfaces receive their own layout hooks, including reused cells.
    for(UIView *child in HGADescendants(root,420))if(HGAEpisodeControl(child))HGAApplyPlayerControls(child);
}

static void HGARefreshEpisodePage(UIViewController *page) {
    // A child can repaint while its parent's glass is being removed.
    if(!page.viewIfLoaded.window || !HGAPanelWantsGlass(@"episodes"))return;
    for(UIViewController *parent=page.parentViewController;parent;parent=parent.parentViewController) {
        HGAActionSheetState *state=objc_getAssociatedObject(parent.viewIfLoaded,&HGAActionSheetKey);
        if(!state || ![HGAPanelKind(parent) isEqual:@"episodes"] ||
           ![page.view isDescendantOfView:state.surface])continue;
        // The host remounts and themes its active page after the encompassing
        // card has been styled. Walk only the ancestry of its own collection.
        UIView *collection=HGAViewGetter(page,"collectionView",page.view);
        if([collection isKindOfClass:UICollectionView.class]) {
            HGAClearSheetCover(state,((UICollectionView *)collection).backgroundView);
            for(UIView *node=collection;node && node!=state.surface;node=node.superview)
                HGAClearSheetCover(state,node);
        }
        return;
    }
}
