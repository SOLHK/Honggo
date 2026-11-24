static void (*HGAOriginalOpenVideoComments)(id,SEL,id);
static BOOL (*HGAOriginalCommentStickyTop)(id,SEL);
static _Thread_local BOOL HGAPresentingCommentsWithoutVideoMovement;

static void HGAPresentVideoComments(id utility,SEL command,id parameters) {
    BOOL previous=HGAPresentingCommentsWithoutVideoMovement;
    HGAPresentingCommentsWithoutVideoMovement=!HGAppearanceFlag(@"player.commentMovesVideo");
    @try {
        HGAOriginalOpenVideoComments(utility,command,parameters);
    } @finally {
        HGAPresentingCommentsWithoutVideoMovement=previous;
    }
}

static BOOL HGACommentStickyTop(id navigation,SEL command) {
    if(HGAPresentingCommentsWithoutVideoMovement)return NO;
    return HGAOriginalCommentStickyTop(navigation,command);
}

static void HGAInstallCommentMovement(void) {
    // Red Fruit 7.4.5.33: the comment utility uses this query to choose
    // between the native sticky-player transition and the ordinary modal.
    // In the latter branch the same panel is presented without borrowing
    // or resizing the active player's rendering view.
    if(HGAOriginalOpenVideoComments)return;
    Class utility=object_getClass(objc_lookUpClass("SSBizShortVideoCommentUtil"));
    Class navigation=objc_lookUpClass("FQVShortVideoListVCNavigationComponent");
    SEL open=sel_registerName("openCommentVCWithContext:");
    SEL sticky=sel_registerName("enabledScaleStickyTop");
    SEL ordinary=sel_registerName("openModalNavWithVC:loopPlay:willShowBlock:dismissBlock:");
    Method openMethod=class_getInstanceMethod(utility,open);
    Method ordinaryMethod=class_getInstanceMethod(navigation,ordinary);
    if(!openMethod || strcmp(method_getTypeEncoding(openMethod),"v24@0:8@16") ||
        !ordinaryMethod || strcmp(method_getTypeEncoding(ordinaryMethod),"v44@0:8@16B24@?28@?36"))return;
    if(!HGAOriginalCommentStickyTop)
        HGAOriginalCommentStickyTop=(BOOL (*)(id,SEL))HGInstallVerifiedHook(navigation,sticky,
            "B16@0:8",(IMP)HGACommentStickyTop,@"appearance");
    if(HGAOriginalCommentStickyTop)
        HGAOriginalOpenVideoComments=(void (*)(id,SEL,id))HGInstallVerifiedHook(utility,open,
            "v24@0:8@16",(IMP)HGAPresentVideoComments,@"appearance");
}
