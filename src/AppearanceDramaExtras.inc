// Verified ByteDance short-drama utilities ported from FanqieX (all ABI-checked):
// 1. Highest available drama resolution (player.defaultMaxQuality)
// 2. Two-finger spread/pinch pure clean screen + haptic (player.pinchClean / player.pinchHaptic)
// 3. Double-tap video to open comments (player.doubleTapComment)
// 4. Hide exit collect alert (player.hideExitCollectAlert)
// 5. Inner-drama bottom black mask removal & bottom gradient feathering

static id HGAExtraRawIvarObj(id obj,const char *ivarName) {
    if(!obj || !ivarName)return nil;
    Ivar ivar=class_getInstanceVariable(object_getClass(obj),ivarName);
    if(!ivar)return nil;
    const char *enc=ivar_getTypeEncoding(ivar);
    if(!enc || enc[0]!='@')return nil;
    return object_getIvar(obj,ivar);
}

static NSInteger HGAExtraIntGet(id obj,const char *selName,NSInteger fallback) {
    if(!obj || !selName)return fallback;
    SEL sel=sel_registerName(selName);
    Method m=class_getInstanceMethod(object_getClass(obj),sel);
    const char *enc=m ? method_getTypeEncoding(m) : NULL;
    if(!enc)return fallback;
    if(!strcmp(enc,"q16@0:8"))return ((NSInteger (*)(id,SEL))objc_msgSend)(obj,sel);
    if(!strcmp(enc,"Q16@0:8"))return (NSInteger)((NSUInteger (*)(id,SEL))objc_msgSend)(obj,sel);
    if(!strcmp(enc,"i16@0:8"))return ((int (*)(id,SEL))objc_msgSend)(obj,sel);
    if(!strcmp(enc,"I16@0:8"))return (NSInteger)((unsigned int (*)(id,SEL))objc_msgSend)(obj,sel);
    return fallback;
}

#pragma mark - 1. Highest Available Drama Resolution

static char HGAQualityModelKey;
static char HGAQualityManualKey;
static char HGAQualityAttemptKey;
static char HGALandscapeQualityKey;

static BOOL HGAHighestQualityEnabled(void) {
    return HGAppearanceEnabled() && HGAppearanceFlag(@"player.defaultMaxQuality");
}

static NSString *HGAVideoID(id model) {
    id value=HGReadObjectGetter(model,"videoId");
    return [value isKindOfClass:NSString.class] && [value length] ? value : nil;
}

static BOOL HGAUsesAutomaticHighest(id model) {
    Class modelCls=NSClassFromString(@"FQVShortVideoDataModel");
    if(!HGAHighestQualityEnabled() || !modelCls || ![model isKindOfClass:modelCls])return NO;
    NSString *vid=HGAVideoID(model);
    BOOL locked=NO,downloaded=NO;
    HGABoolGetter(model,@"isVideoLocked",&locked);
    HGABoolGetter(model,@"useDownloadedEngineData",&downloaded);
    return vid && ![vid isEqual:objc_getAssociatedObject(model,&HGAQualityManualKey)] && !locked && !downloaded;
}

static NSNumber *HGABestResolutionNumber(id videoModel,NSArray *playable) {
    id variants=HGReadObjectGetter(videoModel,"supportedResolutionTypes");
    SEL selector=sel_registerName("videoInfoForType:");
    Method method=videoModel ? class_getInstanceMethod(object_getClass(videoModel),selector) : NULL;
    if(![variants isKindOfClass:NSArray.class] || !method || strcmp(method_getTypeEncoding(method),"@24@0:8Q16")!=0)return nil;
    Class infoCls=NSClassFromString(@"TTVideoEngineURLInfo");
    BOOL hasPlayable=[playable isKindOfClass:NSArray.class] && playable.count>0;
    NSNumber *best=nil;
    double bestPixels=0;
    NSInteger bestBitrate=0;
    for(id value in variants) {
        if(![value isKindOfClass:NSNumber.class] || (hasPlayable && ![playable containsObject:value]))continue;
        id info=((id (*)(id,SEL,NSUInteger))objc_msgSend)(videoModel,selector,[value unsignedIntegerValue]);
        if(infoCls && ![info isKindOfClass:infoCls])continue;
        id width=HGReadObjectGetter(info,"vWidth");
        id height=HGReadObjectGetter(info,"vHeight");
        if(![width respondsToSelector:@selector(doubleValue)] || ![height respondsToSelector:@selector(doubleValue)])continue;
        double w=[width doubleValue],h=[height doubleValue],pixels=w*h;
        if(w<=0 || h<=0 || !isfinite(pixels))continue;
        NSInteger bitrate=HGAExtraIntGet(info,"bitrate",0);
        if(pixels>bestPixels || (pixels==bestPixels && bitrate>bestBitrate)) {
            best=value;bestPixels=pixels;bestBitrate=bitrate;
        }
    }
    return best;
}

static id (*HGOrigResWithModel)(id,SEL,id);
static id HGHookResWithModel(id self,SEL cmd,id model) {
    if(HGAUsesAutomaticHighest(model)) {
        NSNumber *best=HGABestResolutionNumber(HGReadObjectGetter(HGReadObjectGetter(model,"videoInfo"),"videoModel"),nil);
        if(best)return best;
    }
    return HGOrigResWithModel(self,cmd,model);
}
static id (*HGOrigResItemWithModel)(id,SEL,id);
static id HGHookResItemWithModel(id self,SEL cmd,id model) {
    id item=HGOrigResItemWithModel(self,cmd,model);
    if(item) {
        NSHashTable *ref=[NSHashTable weakObjectsHashTable];
        if(model)[ref addObject:model];
        objc_setAssociatedObject(item,&HGAQualityModelKey,ref,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    return item;
}
static NSUInteger (*HGOrigFinalResType)(id,SEL,id,id);
static NSUInteger HGHookFinalResType(id self,SEL cmd,NSArray *supported,id videoModel) {
    id model=[objc_getAssociatedObject(self,&HGAQualityModelKey) anyObject];
    id curVM=HGReadObjectGetter(HGReadObjectGetter(model,"videoInfo"),"videoModel");
    if(HGAUsesAutomaticHighest(model) && curVM && (!videoModel || curVM==videoModel)) {
        NSNumber *best=HGABestResolutionNumber(curVM,supported);
        if(best)return best.unsignedIntegerValue;
    }
    return HGOrigFinalResType(self,cmd,supported,videoModel);
}
static void (*HGOrigControlUpdateRes)(id,SEL,id,id,id);
static void HGHookControlUpdateRes(id self,SEL cmd,id model,id resolution,id finishBlock) {
    if(model && resolution)objc_setAssociatedObject(model,&HGAQualityManualKey,HGAVideoID(model),OBJC_ASSOCIATION_COPY_NONATOMIC);
    HGOrigControlUpdateRes(self,cmd,model,resolution,finishBlock);
}
static void (*HGOrigUpdatePlayerRes)(id,SEL,id,id,id,id);
static void HGHookUpdatePlayerRes(id self,SEL cmd,id player,id resolution,id model,id finish) {
    if(model && resolution && [objc_getAssociatedObject(self,&HGALandscapeQualityKey) boolValue]) {
        objc_setAssociatedObject(model,&HGAQualityManualKey,HGAVideoID(model),OBJC_ASSOCIATION_COPY_NONATOMIC);
    }
    HGOrigUpdatePlayerRes(self,cmd,player,resolution,model,finish);
}
static void (*HGOrigLandscapeSelectRes)(id,SEL,id);
static void HGHookLandscapeSelectRes(id self,SEL cmd,id resolution) {
    id playerComp=HGReadObjectGetter(HGReadObjectGetter(self,"listVCContext"),"playerComponent");
    if(!playerComp){HGOrigLandscapeSelectRes(self,cmd,resolution);return;}
    id prev=objc_getAssociatedObject(playerComp,&HGALandscapeQualityKey);
    objc_setAssociatedObject(playerComp,&HGALandscapeQualityKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    HGOrigLandscapeSelectRes(self,cmd,resolution);
    objc_setAssociatedObject(playerComp,&HGALandscapeQualityKey,prev,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static void (*HGOrigPreparePlayerAsync)(id,SEL,id,id,BOOL);
static void HGHookPreparePlayerAsync(id self,SEL cmd,id player,id model,BOOL asyncFlag) {
    if(player) {
        NSHashTable *ref=[NSHashTable weakObjectsHashTable];
        if(model)[ref addObject:model];
        objc_setAssociatedObject(player,&HGAQualityModelKey,ref,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(player,&HGAQualityAttemptKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    HGOrigPreparePlayerAsync(self,cmd,player,model,asyncFlag);
}
static void (*HGOrigPlayerReadyToPlay)(id,SEL,id);
static void HGHookPlayerReadyToPlay(id player,SEL cmd,id engine) {
    HGOrigPlayerReadyToPlay(player,cmd,engine);
    dispatch_block_t upgrade=^{
        id model=[objc_getAssociatedObject(player,&HGAQualityModelKey) anyObject];
        if(!HGAUsesAutomaticHighest(model) || engine!=HGReadObjectGetter(player,"videoEngine") ||
           ![HGAVideoID(model) isEqual:HGReadObjectGetter(engine,"getVideoID")])return;
        NSArray *supported=HGReadObjectGetter(engine,"supportedResolutionTypes");
        if(![supported isKindOfClass:NSArray.class] || !supported.count)return;
        id videoModel=HGReadObjectGetter(HGReadObjectGetter(model,"videoInfo"),"videoModel");
        NSNumber *best=HGABestResolutionNumber(videoModel,supported);
        if(!best || HGAExtraIntGet(engine,"currentResolution",-1)==best.integerValue ||
           [best isEqual:objc_getAssociatedObject(player,&HGAQualityAttemptKey)])return;
        SEL sel=sel_registerName("configResolution:");
        Method m=class_getInstanceMethod(object_getClass(engine),sel);
        if(!m || strcmp(method_getTypeEncoding(m),"B24@0:8Q16")!=0)return;
        objc_setAssociatedObject(player,&HGAQualityAttemptKey,best,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        ((BOOL (*)(id,SEL,NSUInteger))objc_msgSend)(engine,sel,best.unsignedIntegerValue);
    };
    if(NSThread.isMainThread)upgrade();
    else dispatch_async(dispatch_get_main_queue(),upgrade);
}

#pragma mark - 2. Two-Finger Spread/Pinch Pure Clean Screen & 3. Double-Tap Comments

static char HGAPinchActiveKey;
static char HGAPinchHiddenViewsKey;
static char HGAPinchRecognizerKey;
static char HGADoubleTapGestureKey;

static BOOL HGAPinchCleanEnabled(void) {
    return HGAppearanceEnabled() && HGAppearanceFlag(@"player.pinchClean");
}
static BOOL HGAPinchHapticEnabled(void) {
    return HGAppearanceFlag(@"player.pinchHaptic");
}
static BOOL HGADoubleTapCommentEnabled(void) {
    return HGAppearanceEnabled() && HGAppearanceFlag(@"player.doubleTapComment");
}

static NSHashTable<UIView *> *HGAPinchControls(void) {
    static NSHashTable<UIView *> *table;
    static dispatch_once_t once;
    dispatch_once(&once,^{ table=[NSHashTable weakObjectsHashTable]; });
    return table;
}

@interface HGAPinchViewState : NSObject
@property(nonatomic,strong) CALayer *mask;
@property(nonatomic,strong) CALayer *originalMask;
@property(nonatomic) BOOL interactive;
@end
@implementation HGAPinchViewState @end

static void HGAHideForPinchClean(UIView *view,NSMapTable<UIView *,HGAPinchViewState *> *views) {
    if(![view isKindOfClass:UIView.class] || !view.window || view.hidden ||
       view.alpha==0.0 || [views objectForKey:view])return;
    HGAPinchViewState *st=[HGAPinchViewState new];
    st.originalMask=view.layer.mask;
    st.mask=[CALayer layer];
    st.mask.frame=CGRectZero;
    st.interactive=view.userInteractionEnabled;
    [views setObject:st forKey:view];
    view.layer.mask=st.mask;
    view.userInteractionEnabled=NO;
}

static UIView *HGAFindDockForControl(UIView *control) {
    Class tbcCls=NSClassFromString(@"SSTabBarController");
    Class barCls=NSClassFromString(@"SSTabBar");
    for(UIResponder *resp=control.nextResponder;resp;resp=resp.nextResponder) {
        if(tbcCls && [resp isKindOfClass:tbcCls]) {
            UIViewController *selVC=HGReadObjectGetter(resp,"selectedViewController");
            UIView *selView=[selVC isKindOfClass:UIViewController.class] ? selVC.viewIfLoaded : nil;
            BOOL owns=NO;
            for(UIView *cur=control;cur;cur=cur.superview)if(cur==selView){owns=YES;break;}
            if(owns) {
                UIView *bar=HGReadObjectGetter(resp,"customTabBar");
                if(barCls && [bar isKindOfClass:barCls])return bar;
            }
            break;
        }
    }
    return nil;
}

static void HGAApplyPinchClean(UIView *control,BOOL enterClean) {
    if(![control isKindOfClass:UIView.class])return;
    if(enterClean) {
        objc_setAssociatedObject(control,&HGAPinchActiveKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        NSMapTable<UIView *,HGAPinchViewState *> *hiddenSet=objc_getAssociatedObject(control,&HGAPinchHiddenViewsKey);
        if(!hiddenSet) {
            hiddenSet=[NSMapTable weakToStrongObjectsMapTable];
            objc_setAssociatedObject(control,&HGAPinchHiddenViewsKey,hiddenSet,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        Class outerCls=NSClassFromString(@"SSShortVideoOutterMultiGenreFeedControlView");
        if(outerCls && [control isKindOfClass:outerCls]) {
            const char *outerIvars[]={"_aboveProgressView","_progressView","_baseInfoView","_bookCardView",
                "_progressFloatView","_playBtn","_topGradientView","_bottomGradientView"};
            for(size_t i=0;i<sizeof(outerIvars)/sizeof(*outerIvars);i++) {
                HGAHideForPinchClean(HGAExtraRawIvarObj(control,outerIvars[i]),hiddenSet);
            }
        } else {
            const char *keys[]={"baseInfoView","topGradientView","bottomGradientView","playBtn","muteBtnView",
                "fullscreenBtn","progressContainer","progressFloatView","bottomView","singleSeriesBottomView",
                "insertAboveProgressView","insertBelowVideoInfoContainerView","insertAboveTopControlView",
                "fastPlayTipView","bottomFastPlayTipView","resolutionTipsView","playRateTipsView",
                "cleanScreenModeRightInteractionView","cleanScreenModeRightInteractionSubView","cancelCleanScreenView"};
            for(size_t i=0;i<sizeof(keys)/sizeof(*keys);i++) {
                HGAHideForPinchClean(HGReadObjectGetter(control,keys[i]),hiddenSet);
            }
        }
        id hostCtx=HGReadObjectGetter(control,"hostContext");
        id ctx=HGReadObjectGetter(control,"listVCContext") ?: HGReadObjectGetter(hostCtx,"listVCContext");
        id navComp=ctx ? HGReadObjectGetter(ctx,"navBarComponent") : nil;
        HGAHideForPinchClean(navComp ? HGReadObjectGetter(navComp,"navBar") : nil,hiddenSet);
        HGAHideForPinchClean(HGReadObjectGetter(hostCtx,"currentTabHeaderView"),hiddenSet);
        Class feedCls=NSClassFromString(@"SSVideoSeriesFeedViewController");
        for(UIResponder *resp=control.nextResponder;resp;resp=resp.nextResponder) {
            if(feedCls && [resp isKindOfClass:feedCls]) {
                HGAHideForPinchClean(HGReadObjectGetter(resp,"headerView"),hiddenSet);
                break;
            }
        }
        HGAHideForPinchClean(HGAFindDockForControl(control),hiddenSet);
    } else {
        objc_setAssociatedObject(control,&HGAPinchActiveKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        NSMapTable<UIView *,HGAPinchViewState *> *hiddenSet=objc_getAssociatedObject(control,&HGAPinchHiddenViewsKey);
        objc_setAssociatedObject(control,&HGAPinchHiddenViewsKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        for(UIView *v in hiddenSet.keyEnumerator.allObjects) {
            HGAPinchViewState *st=[hiddenSet objectForKey:v];
            if(v.layer.mask==st.mask)v.layer.mask=st.originalMask;
            if(![objc_getAssociatedObject(v,@selector(hg_dockHiddenByCommentSheet)) boolValue]) {
                v.userInteractionEnabled=st.interactive;
            }
        }
    }
}

@interface HGADramaPinchGestureRecognizer : UIPinchGestureRecognizer
@end
@implementation HGADramaPinchGestureRecognizer
- (BOOL)canBePreventedByGestureRecognizer:(UIGestureRecognizer *)other {
    if([other isKindOfClass:UIPanGestureRecognizer.class] ||
       [other isKindOfClass:UILongPressGestureRecognizer.class])return NO;
    return [super canBePreventedByGestureRecognizer:other];
}
@end

static BOOL HGAPinchConflicts(UIGestureRecognizer *gesture) {
    if(!HGAPinchCleanEnabled())return NO;
    UIWindow *win=gesture.view.window;
    if(!win || CGRectGetHeight(win.bounds)<=CGRectGetWidth(win.bounds))return NO;
    for(UIView *control in HGAPinchControls()) {
        if(control.window!=win ||
           (![control isDescendantOfView:gesture.view] && ![gesture.view isDescendantOfView:control]))continue;
        UIGestureRecognizer *pinch=objc_getAssociatedObject(control,&HGAPinchRecognizerKey);
        if(pinch.numberOfTouches>=2 || pinch.state==UIGestureRecognizerStateBegan ||
           pinch.state==UIGestureRecognizerStateChanged)return YES;
    }
    return NO;
}

@interface HGADramaPinchHandler : NSObject <UIGestureRecognizerDelegate>
+ (instancetype)shared;
@property(nonatomic,strong) UIImpactFeedbackGenerator *haptic;
- (void)handlePinch:(HGADramaPinchGestureRecognizer *)pinch;
@end
@implementation HGADramaPinchHandler
+ (instancetype)shared {
    static HGADramaPinchHandler *inst;
    static dispatch_once_t once;
    dispatch_once(&once,^{ inst=[HGADramaPinchHandler new]; });
    return inst;
}
- (BOOL)gestureRecognizerShouldBegin:(UIGestureRecognizer *)gr {
    if(!HGAPinchCleanEnabled())return NO;
    UIView *v=gr.view;
    if(!v || !v.window || gr.numberOfTouches<2)return NO;
    if(CGRectGetHeight(v.window.bounds)<=CGRectGetWidth(v.window.bounds))return NO;
    id player=HGReadObjectGetter(v,"vPlayer");
    BOOL fs=NO;
    if(player && HGABoolGetter(player,@"isFullScreen",&fs) && fs)return NO;
    return YES;
}
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)g shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)o {
    (void)g;(void)o;return NO;
}
- (void)handlePinch:(HGADramaPinchGestureRecognizer *)pinch {
    UIView *control=pinch.view;
    if(pinch.state==UIGestureRecognizerStateBegan && HGAPinchHapticEnabled()) {
        if(!self.haptic)self.haptic=[[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleLight];
        [self.haptic prepare];
    }
    if(pinch.state!=UIGestureRecognizerStateEnded || !control.window || !HGAPinchCleanEnabled())return;
    BOOL clean=[objc_getAssociatedObject(control,&HGAPinchActiveKey) boolValue];
    BOOL spread=pinch.scale>=1.10 || (pinch.scale>1.03 && pinch.velocity>0.35);
    BOOL shrink=pinch.scale<=0.90 || (pinch.scale<0.97 && pinch.velocity<-0.35);
    if((!clean && spread) || (clean && shrink)) {
        __weak UIView *weakControl=control;
        UIWindow *win=control.window;
        id model=HGReadObjectGetter(control,"contentModel");
        dispatch_async(dispatch_get_main_queue(),^{
            UIView *cur=weakControl;
            if(!cur || cur.window!=win || !HGAPinchCleanEnabled() ||
               HGReadObjectGetter(cur,"contentModel")!=model ||
               [objc_getAssociatedObject(cur,&HGAPinchActiveKey) boolValue]!=clean)return;
            [CATransaction begin];
            [CATransaction setDisableActions:YES];
            HGAApplyPinchClean(cur,!clean);
            [CATransaction commit];
            if(HGAPinchHapticEnabled())[self.haptic impactOccurred];
        });
    }
}
@end

static void HGAAttachPinchIfNeeded(UIView *control) {
    if(![control isKindOfClass:UIView.class])return;
    HGADramaPinchGestureRecognizer *existing=objc_getAssociatedObject(control,&HGAPinchRecognizerKey);
    if(existing && existing.view==control)return;
    HGADramaPinchGestureRecognizer *pinch=[[HGADramaPinchGestureRecognizer alloc]
        initWithTarget:[HGADramaPinchHandler shared] action:@selector(handlePinch:)];
    pinch.cancelsTouchesInView=YES;
    pinch.delaysTouchesBegan=NO;
    pinch.delaysTouchesEnded=NO;
    pinch.delegate=[HGADramaPinchHandler shared];
    [control addGestureRecognizer:pinch];
    [HGAPinchControls() addObject:control];
    objc_setAssociatedObject(control,&HGAPinchRecognizerKey,pinch,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

static UIView *HGAFindDramaCommentActionView(UIView *root,NSInteger depth) {
    if(![root isKindOfClass:UIView.class] || depth>8)return nil;
    Class c1=NSClassFromString(@"SSShortVideoRightInteractionCommentView");
    Class c2=NSClassFromString(@"SSShortVideoHostRightInteractionCommentView");
    if((c1 && [root isKindOfClass:c1]) || (c2 && [root isKindOfClass:c2]))return root;
    for(UIView *sub in root.subviews) {
        UIView *found=HGAFindDramaCommentActionView(sub,depth+1);
        if(found)return found;
    }
    return nil;
}

static BOOL HGATriggerCommentFromControl(UIView *control) {
    if(![control isKindOfClass:UIView.class])return NO;
    UIView *commentView=HGAFindDramaCommentActionView(control,0);
    if(!commentView) {
        Class cellCls=NSClassFromString(@"FQVShortVideoPlayerCellView");
        for(UIView *cur=control.superview;cur;cur=cur.superview) {
            if((cellCls && [cur isKindOfClass:cellCls]) || [cur isKindOfClass:UICollectionViewCell.class]) {
                commentView=HGAFindDramaCommentActionView(cur,0);
                break;
            }
        }
    }
    if(!commentView)return NO;
    SEL sel=sel_registerName("commentViewDidClicked");
    Method m=class_getInstanceMethod(object_getClass(commentView),sel);
    if(!m || strcmp(method_getTypeEncoding(m),"v16@0:8")!=0)return NO;
    ((void (*)(id,SEL))objc_msgSend)(commentView,sel);
    return YES;
}

@interface HGADoubleTapCommentHandler : NSObject <UIGestureRecognizerDelegate>
+ (instancetype)shared;
- (void)handleDoubleTap:(UITapGestureRecognizer *)gesture;
@end
@implementation HGADoubleTapCommentHandler
+ (instancetype)shared {
    static HGADoubleTapCommentHandler *inst;
    static dispatch_once_t once;
    dispatch_once(&once,^{ inst=[HGADoubleTapCommentHandler new]; });
    return inst;
}
- (BOOL)gestureRecognizerShouldBegin:(UIGestureRecognizer *)gr {
    return HGADoubleTapCommentEnabled() && gr.view.window!=nil;
}
- (void)handleDoubleTap:(UITapGestureRecognizer *)gr {
    if(gr.state!=UIGestureRecognizerStateEnded || !HGADoubleTapCommentEnabled())return;
    HGATriggerCommentFromControl(gr.view);
}
@end

static void HGAsyncOuterDoubleTapComment(UIView *outer) {
    if(![outer isKindOfClass:UIView.class])return;
    UITapGestureRecognizer *dt=objc_getAssociatedObject(outer,&HGADoubleTapGestureKey);
    BOOL enabled=HGADoubleTapCommentEnabled();
    if(!enabled){if(dt)dt.enabled=NO;return;}
    if(!dt || dt.view!=outer) {
        dt=[[UITapGestureRecognizer alloc] initWithTarget:[HGADoubleTapCommentHandler shared] action:@selector(handleDoubleTap:)];
        dt.numberOfTapsRequired=2;
        dt.cancelsTouchesInView=YES;
        dt.delegate=[HGADoubleTapCommentHandler shared];
        [outer addGestureRecognizer:dt];
        objc_setAssociatedObject(outer,&HGADoubleTapGestureKey,dt,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    dt.enabled=YES;
    UIGestureRecognizer *single=HGAExtraRawIvarObj(outer,"_playPauseTapGesture");
    if([single isKindOfClass:UIGestureRecognizer.class] && single!=dt)[single requireGestureRecognizerToFail:dt];
}

static void (*HGOrigVPlayerSetupGestures)(id,SEL);
static void HGHookVPlayerSetupGestures(UIView *self,SEL cmd) {
    HGOrigVPlayerSetupGestures(self,cmd);
    HGAAttachPinchIfNeeded(self);
}
static void (*HGOrigVPlayerDoubleTap)(id,SEL,id);
static void HGHookVPlayerDoubleTap(UIView *self,SEL cmd,id gesture) {
    if(HGADoubleTapCommentEnabled() && HGATriggerCommentFromControl(self))return;
    HGOrigVPlayerDoubleTap(self,cmd,gesture);
}
static void (*HGOrigVPlayerWillDisplay)(id,SEL);
static void HGHookVPlayerWillDisplay(UIView *self,SEL cmd) {
    HGOrigVPlayerWillDisplay(self,cmd);
    HGAAttachPinchIfNeeded(self);
}
static void (*HGOrigVPlayerWillEndDisplay)(id,SEL);
static void HGHookVPlayerWillEndDisplay(UIView *self,SEL cmd) {
    if([objc_getAssociatedObject(self,&HGAPinchActiveKey) boolValue])HGAApplyPinchClean(self,NO);
    HGOrigVPlayerWillEndDisplay(self,cmd);
}
static void (*HGOrigVPlayerUpdateState)(id,SEL);
static void HGHookVPlayerUpdateState(UIView *self,SEL cmd) {
    HGOrigVPlayerUpdateState(self,cmd);
    if([objc_getAssociatedObject(self,&HGAPinchActiveKey) boolValue])HGAApplyPinchClean(self,YES);
}
static void (*HGOrigOuterLayout)(id,SEL);
static void HGHookOuterLayout(UIView *self,SEL cmd) {
    HGOrigOuterLayout(self,cmd);
    HGAAttachPinchIfNeeded(self);
    HGAsyncOuterDoubleTapComment(self);
}
static void (*HGOrigOuterDidMoveWindow)(id,SEL);
static void HGHookOuterDidMoveWindow(UIView *self,SEL cmd) {
    HGOrigOuterDidMoveWindow(self,cmd);
    if(!self.window && [objc_getAssociatedObject(self,&HGAPinchActiveKey) boolValue])HGAApplyPinchClean(self,NO);
}
static void (*HGOrigOuterBindContent)(id,SEL,id);
static void HGHookOuterBindContent(UIView *self,SEL cmd,id model) {
    if(HGReadObjectGetter(self,"contentModel")!=model &&
       [objc_getAssociatedObject(self,&HGAPinchActiveKey) boolValue])HGAApplyPinchClean(self,NO);
    HGOrigOuterBindContent(self,cmd,model);
}
static BOOL (*HGOrigGestureCompShouldBegin)(id,SEL,id);
static BOOL HGHookGestureCompShouldBegin(id self,SEL cmd,UIGestureRecognizer *gr) {
    if(([gr isKindOfClass:UIPinchGestureRecognizer.class] || [gr isKindOfClass:UILongPressGestureRecognizer.class]) &&
       HGAPinchConflicts(gr))return NO;
    return HGOrigGestureCompShouldBegin(self,cmd,gr);
}

#pragma mark - 4. Hide Exit Collect Alert

static BOOL HGAHideExitCollectEnabled(void) {
    return HGAppearanceEnabled() && HGAppearanceFlag(@"player.hideExitCollectAlert");
}
static BOOL (*HGOrigExitShouldInterrupt)(id,SEL,id,NSUInteger);
static BOOL HGHookExitShouldInterrupt(id self,SEL cmd,id ctx,NSUInteger scene) {
    if(HGAHideExitCollectEnabled())return NO;
    return HGOrigExitShouldInterrupt(self,cmd,ctx,scene);
}
static BOOL (*HGOrigExitShouldShowOpt)(id,SEL,id,id);
static BOOL HGHookExitShouldShowOpt(id self,SEL cmd,id ctx,id seriesId) {
    if(HGAHideExitCollectEnabled())return NO;
    return HGOrigExitShouldShowOpt(self,cmd,ctx,seriesId);
}
static void (*HGOrigExitShowAddShelf)(id,SEL,id);
static void HGHookExitShowAddShelf(id self,SEL cmd,id ctx) {
    if(HGAHideExitCollectEnabled())return;
    HGOrigExitShowAddShelf(self,cmd,ctx);
}
static BOOL (*HGOrigExitCompShould)(id,SEL,id);
static BOOL HGHookExitCompShould(id self,SEL cmd,id ctx) {
    if(HGAHideExitCollectEnabled())return NO;
    return HGOrigExitCompShould(self,cmd,ctx);
}

#pragma mark - 5. Inner-Drama Bottom Black Mask Removal

static char HGADramaFeatheredGradKey;

static void HGAFeatherBottomGradient(UIView *controlView) {
    if(![controlView isKindOfClass:UIView.class] || !(HGAppearanceEnabled() || HGIsEnabled(HGGlass)))return;
    UIView *gradView=HGReadObjectGetter(controlView,"bottomGradientView");
    if(![gradView isKindOfClass:UIView.class])return;
    CALayer *raw=HGReadObjectGetter(gradView,"gradientLayer") ?: gradView.layer;
    if(![raw isKindOfClass:CAGradientLayer.class])return;
    CAGradientLayer *grad=(CAGradientLayer *)raw;
    NSArray *colors=grad.colors;
    if(colors.count<2)return;
    if([objc_getAssociatedObject(grad,&HGADramaFeatheredGradKey) isEqual:colors])return;
    id clearCG=(__bridge id)UIColor.clearColor.CGColor;
    id midCG=colors.count==2 ? colors[1] : colors[colors.count-2];
    UIColor *midColor=[UIColor colorWithCGColor:(__bridge CGColorRef)midCG];
    CGFloat r=0,g=0,b=0,a=0;
    if([midColor getRed:&r green:&g blue:&b alpha:&a] && a>0.32) {
        midCG=(__bridge id)[UIColor colorWithRed:r green:g blue:b alpha:0.28].CGColor;
    }
    NSArray *feathered=@[clearCG,midCG,clearCG];
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    grad.colors=feathered;
    grad.locations=@[@0.0,@0.84,@1.0];
    [CATransaction commit];
    objc_setAssociatedObject(grad,&HGADramaFeatheredGradKey,feathered,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

static void HGAClearDramaBottomMask(UIView *view) {
    if(![view isKindOfClass:UIView.class])return;
    BOOL active=HGAppearanceEnabled() || HGIsEnabled(HGGlass);
    HGACommentSetClearSurface(view,active);
    HGACommentSetClearSurface(HGReadObjectGetter(view,"bottomContainerView"),active);
    UIView *parent=view.superview;
    if(parent) {
        NSString *pc=NSStringFromClass(parent.class);
        if([pc containsString:@"Bottom"] || [pc containsString:@"Insert"])HGACommentSetClearSurface(parent,active);
    }
    for(UIView *cur=view;cur;cur=cur.superview) {
        NSString *cn=NSStringFromClass(cur.class);
        if([cn isEqual:@"FQVShortVideoVPlayerSeriesControlView"] ||
           [cn isEqual:@"FQVShortVideoVPlayerSingleSeriesControlView"] ||
           [cn isEqual:@"FQVShortVideoVPlayerControlView"]) {
            HGACommentSetClearSurface(HGReadObjectGetter(cur,"bottomView"),active);
            HGACommentSetClearSurface(HGReadObjectGetter(cur,"singleSeriesBottomView"),active);
            HGACommentSetClearSurface(HGReadObjectGetter(cur,"episodeNewInfoView"),active);
            HGACommentSetClearSurface(HGReadObjectGetter(cur,"playerContainerMaskView"),active);
            HGAFeatherBottomGradient(cur);
            break;
        }
    }
}

static void (*HGOrigBottomInsertSetup)(id,SEL);
static void HGHookBottomInsertSetup(UIView *self,SEL cmd) {
    HGOrigBottomInsertSetup(self,cmd);HGAClearDramaBottomMask(self);
}
static void (*HGOrigBottomInsertResize)(id,SEL);
static void HGHookBottomInsertResize(UIView *self,SEL cmd) {
    HGOrigBottomInsertResize(self,cmd);HGAClearDramaBottomMask(self);
}
static void (*HGOrigSingleBottomSetup)(id,SEL);
static void HGHookSingleBottomSetup(UIView *self,SEL cmd) {
    HGOrigSingleBottomSetup(self,cmd);HGAClearDramaBottomMask(self);
}
static void (*HGOrigPlayerBottomSetup)(id,SEL);
static void HGHookPlayerBottomSetup(UIView *self,SEL cmd) {
    HGOrigPlayerBottomSetup(self,cmd);HGAClearDramaBottomMask(self);
}
static void (*HGOrigSeriesSingleBottom)(id,SEL);
static void HGHookSeriesSingleBottom(UIView *self,SEL cmd) {
    HGOrigSeriesSingleBottom(self,cmd);HGAClearDramaBottomMask(self);
}
static void (*HGOrigSingleSeriesBottom)(id,SEL);
static void HGHookSingleSeriesBottom(UIView *self,SEL cmd) {
    HGOrigSingleSeriesBottom(self,cmd);HGAClearDramaBottomMask(self);
}

static void HGADramaExtrasRefresh(void) {
    if(!HGAPinchCleanEnabled()) {
        for(UIView *c in HGAPinchControls().allObjects) {
            if([objc_getAssociatedObject(c,&HGAPinchActiveKey) boolValue])HGAApplyPinchClean(c,NO);
        }
    }
    Class outerCls=NSClassFromString(@"SSShortVideoOutterMultiGenreFeedControlView");
    for(UIView *c in HGAPinchControls().allObjects) {
        if(outerCls && [c isKindOfClass:outerCls])HGAsyncOuterDoubleTapComment(c);
    }
}

static void HGAInstallDramaExtras(void) {
    static dispatch_once_t once;
    dispatch_once(&once,^{
        NSUInteger qualityHooks=0,extraHooks=0;
#define HGE_HOOK(clsName,selName,enc,origVar,hookFn,counter) do { \
    Class _c=NSClassFromString(@clsName); \
    if(_c) { \
        IMP _o=HGInstallVerifiedHook(_c,sel_registerName(selName),enc,(IMP)(hookFn),@"appearance.dramaExtras"); \
        if(_o){ origVar=(__typeof__(origVar))_o; counter++; } \
    } \
} while(0)
        // 1. Highest resolution
        HGE_HOOK("FQVShortVideoListVCPlayerComponent","resolutionWithModel:","@24@0:8@16",HGOrigResWithModel,HGHookResWithModel,qualityHooks);
        HGE_HOOK("FQVShortVideoListVCPlayerComponent","resolutionItemWithModel:","@24@0:8@16",HGOrigResItemWithModel,HGHookResItemWithModel,qualityHooks);
        HGE_HOOK("FQVPlayerEngineDataResolutionItem","finalResolutionTypeWithSupportResolutions:videoModel:","Q32@0:8@16@24",HGOrigFinalResType,HGHookFinalResType,qualityHooks);
        HGE_HOOK("FQVShortVideoListVCControlComponent","updateCurrentPlayResolutionWithVideoDataModel:resolution:finishBlock:","v40@0:8@16@24@?32",HGOrigControlUpdateRes,HGHookControlUpdateRes,qualityHooks);
        HGE_HOOK("FQVShortVideoListVCLandscapeSettingComponent","didSelectResolution:","v24@0:8@16",HGOrigLandscapeSelectRes,HGHookLandscapeSelectRes,qualityHooks);
        HGE_HOOK("FQVShortVideoListVCPlayerComponent","updatePlayer:resolution:model:finish:","v48@0:8@16@24@32@?40",HGOrigUpdatePlayerRes,HGHookUpdatePlayerRes,qualityHooks);
        HGE_HOOK("FQVShortVideoListVCPlayerComponent","preparePlayer:model:async:","v36@0:8@16@24B32",HGOrigPreparePlayerAsync,HGHookPreparePlayerAsync,qualityHooks);
        HGE_HOOK("FQVPlayer","videoEngineReadyToPlay:","v24@0:8@16",HGOrigPlayerReadyToPlay,HGHookPlayerReadyToPlay,qualityHooks);

        // 2 & 3. Two-finger pinch clean + double-tap comment
        HGE_HOOK("FQVShortVideoVPlayerControlView","setupGestures","v16@0:8",HGOrigVPlayerSetupGestures,HGHookVPlayerSetupGestures,extraHooks);
        HGE_HOOK("FQVShortVideoVPlayerControlView","doubleTapGesture:","v24@0:8@16",HGOrigVPlayerDoubleTap,HGHookVPlayerDoubleTap,extraHooks);
        HGE_HOOK("FQVShortVideoVPlayerControlView","controlViewWillDisplay","v16@0:8",HGOrigVPlayerWillDisplay,HGHookVPlayerWillDisplay,extraHooks);
        HGE_HOOK("FQVShortVideoVPlayerControlView","controlViewWillEndDisplay","v16@0:8",HGOrigVPlayerWillEndDisplay,HGHookVPlayerWillEndDisplay,extraHooks);
        HGE_HOOK("FQVShortVideoVPlayerControlView","updateControlState","v16@0:8",HGOrigVPlayerUpdateState,HGHookVPlayerUpdateState,extraHooks);
        HGE_HOOK("SSShortVideoOutterMultiGenreFeedControlView","layoutSubviews","v16@0:8",HGOrigOuterLayout,HGHookOuterLayout,extraHooks);
        HGE_HOOK("SSShortVideoOutterMultiGenreFeedControlView","didMoveToWindow","v16@0:8",HGOrigOuterDidMoveWindow,HGHookOuterDidMoveWindow,extraHooks);
        HGE_HOOK("SSShortVideoOutterMultiGenreFeedControlView","bindContentModel:","v24@0:8@16",HGOrigOuterBindContent,HGHookOuterBindContent,extraHooks);
        HGE_HOOK("FQVShortVideoListVCGestureComponent","gestureRecognizerShouldBegin:","B24@0:8@16",HGOrigGestureCompShouldBegin,HGHookGestureCompShouldBegin,extraHooks);

        // 4. Hide exit collect alert
        HGE_HOOK("SSShortVideoPlayerExitInterruptDelegate","shouldInterruptWhenExitPlayer:exitScene:","B32@0:8@16Q24",HGOrigExitShouldInterrupt,HGHookExitShouldInterrupt,extraHooks);
        HGE_HOOK("SSShortVideoPlayerExitInterruptDelegate","shouldShowCollectExitRetentioDialogOptWithContext:seriesId:","B32@0:8@16@24",HGOrigExitShouldShowOpt,HGHookExitShouldShowOpt,extraHooks);
        HGE_HOOK("SSShortVideoPlayerExitInterruptDelegate","showAddBookShelfAlert:","v24@0:8@16",HGOrigExitShowAddShelf,HGHookExitShowAddShelf,extraHooks);
        HGE_HOOK("SSShortVideoPlayerExitInterruptComponent","shouldInterruptWhenExitPlayer:","B24@0:8@16",HGOrigExitCompShould,HGHookExitCompShould,extraHooks);

        // 5. Inner drama bottom black mask removal
        HGE_HOOK("SSShortVideoBottomInsertView","setupBottomSubviews","v16@0:8",HGOrigBottomInsertSetup,HGHookBottomInsertSetup,extraHooks);
        HGE_HOOK("SSShortVideoBottomInsertView","refreshLayoutForResizing","v16@0:8",HGOrigBottomInsertResize,HGHookBottomInsertResize,extraHooks);
        HGE_HOOK("SSShortVideoSingleBottomInsertView","setupViews","v16@0:8",HGOrigSingleBottomSetup,HGHookSingleBottomSetup,extraHooks);
        HGE_HOOK("FQVShortVideoPlayerBottomView","setup","v16@0:8",HGOrigPlayerBottomSetup,HGHookPlayerBottomSetup,extraHooks);
        HGE_HOOK("FQVShortVideoVPlayerSeriesControlView","setupSingleBottomViewIfNeed","v16@0:8",HGOrigSeriesSingleBottom,HGHookSeriesSingleBottom,extraHooks);
        HGE_HOOK("FQVShortVideoVPlayerSingleSeriesControlView","setupSingleBottomViewIfNeed","v16@0:8",HGOrigSingleSeriesBottom,HGHookSingleSeriesBottom,extraHooks);
#undef HGE_HOOK
        HGRecordCapability(@"appearance.defaultMaxQuality",@{@"module":@"appearance",
            @"state":qualityHooks?@"installed":@"unavailable",@"installedHooks":@(qualityHooks)});
        HGRecordCapability(@"appearance.dramaExtras",@{@"module":@"appearance",
            @"state":extraHooks?@"installed":@"unavailable",@"installedHooks":@(extraHooks)});
    });
}
