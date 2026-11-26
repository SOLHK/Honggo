// The list builds a fresh resolution item for preloading, initial playback and
// live history reconciliation. Only the first two ask the item for its final
// preload resolution. The live reconciliation reads forceResolutionTypeNumber.
static char HGAFeedResolutionLabelsKey;

static void HGATagFeedResolutionItem(id item,id component) {
    if(!item || !HGAppearanceFlag(@"player.highestQuality"))return;
    id context=HGReadObjectGetter(component,"listVCContext");
    BOOL hasEntrance=NO;
    if(!HGABoolGetter(HGReadObjectGetter(context,"config"),@"enableResolutionEntrance",&hasEntrance) || !hasEntrance)return;
    Class experiments=objc_lookUpClass("FQVShortVideoABTestHelper");
    SEL control=sel_registerName("shortVideoResolutionControl");
    Method method=class_getClassMethod(experiments,control);
    if(!method || strcmp(method_getTypeEncoding(method),"Q16@0:8") ||
        !((NSUInteger(*)(id,SEL))objc_msgSend)(experiments,control))return;

    // The native menu maps the player's supported IDs through this dictionary.
    // The incoming asset supplies its own supported IDs when the host selects a stream.
    id labels=HGReadObjectGetter(component,"videoAllowResolutionDict");
    if([labels isKindOfClass:NSDictionary.class])
        objc_setAssociatedObject(item,&HGAFeedResolutionLabelsKey,labels,OBJC_ASSOCIATION_COPY_NONATOMIC);
}

static NSUInteger HGAInitialFeedResolution(id item,NSArray *supported,id model,NSUInteger native) {
    NSDictionary *labels=objc_getAssociatedObject(item,&HGAFeedResolutionLabelsKey);
    if(!labels || !HGAppearanceFlag(@"player.highestQuality") || ![supported isKindOfClass:NSArray.class] ||
        ![model isKindOfClass:objc_lookUpClass("TTVideoEngineModel")] || model!=HGReadObjectGetter(item,"videoModel"))return native;
    // Keep downloaded streams, explicit selections, and bytes already preloaded
    // at another resolution. The host writes the preload decision back to the
    // episode and subsequently exposes it as forceResolutionTypeNumber.
    if(HGReadObjectGetter(item,"forceResolutionTypeNumber") || HGReadObjectGetter(item,"resolutionTypeNumberWithCache"))
        return native;

    NSInteger highest=0;
    NSUInteger choice=native;
    // FQVPlayer filters the engine's playable IDs to these five menu entries.
    NSArray *menuIDs=@[@0,@1,@8,@2,@3];
    for(NSNumber *identifier in supported) {
        if(![menuIDs containsObject:identifier])continue;
        NSString *label=labels[identifier];
        if(![label isKindOfClass:NSString.class])continue;
        NSInteger lines=label.integerValue;
        if(lines>highest && [label isEqualToString:[NSString stringWithFormat:@"%ldP",(long)lines]]) {
            highest=lines;
            choice=identifier.unsignedIntegerValue;
        }
    }
    return choice;
}
