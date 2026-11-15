// Local user-selected assets only. No host files, network loading or bundled fonts.
static NSURL *HGAAssetDirectory(void) {
    NSURL *base=[NSFileManager.defaultManager URLsForDirectory:NSApplicationSupportDirectory
        inDomains:NSUserDomainMask].firstObject;
    return [base URLByAppendingPathComponent:@"HonggoAppearance" isDirectory:YES];
}
static NSString *HGADigest(NSData *data) {
    unsigned char hash[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256(data.bytes,(CC_LONG)data.length,hash);
    NSMutableString *text=[NSMutableString string];
    for(NSUInteger i=0;i<sizeof(hash);i++) [text appendFormat:@"%02x",hash[i]];
    return text;
}
static NSData *HGAReadFile(NSURL *url, NSUInteger limit, NSError **error) {
    if (!url.isFileURL) { if(error)*error=HGAAssetError(@"仅允许本地文件。"); return nil; }
    NSInputStream *stream=[NSInputStream inputStreamWithURL:url];
    [stream open];
    NSMutableData *data=[NSMutableData data];
    uint8_t buffer[32768]; NSInteger read=0;
    while((read=[stream read:buffer maxLength:sizeof(buffer)])>0) {
        if (data.length+(NSUInteger)read>limit) { [stream close]; if(error)*error=HGAAssetError(@"文件超过允许大小。"); return nil; }
        [data appendBytes:buffer length:(NSUInteger)read];
    }
    NSError *failure=read<0 ? stream.streamError : nil;
    [stream close];
    if(failure || !data.length) {if(error)*error=failure ?: HGAAssetError(@"文件为空或不可读取。");return nil;}
    return data;
}
static NSURL *HGAStoreAsset(NSData *data,NSString *extension,NSError **error) {
    NSURL *directory=HGAAssetDirectory();
    if(![NSFileManager.defaultManager createDirectoryAtURL:directory withIntermediateDirectories:YES attributes:nil error:error])return nil;
    // Bound this plugin's asset store and remove only unreferenced content-addressed files.
    NSSet *referenced=[NSSet setWithArray:HGAppearanceValues().allValues];
    NSUInteger bytes=0;
    for(NSURL *file in [NSFileManager.defaultManager contentsOfDirectoryAtURL:directory includingPropertiesForKeys:@[NSURLFileSizeKey] options:NSDirectoryEnumerationSkipsHiddenFiles error:nil]) {
        NSString *name=file.lastPathComponent;
        if([name rangeOfString:@"^[a-f0-9]{64}\\.(png|ttf|otf|ttc)$" options:NSRegularExpressionSearch].location==NSNotFound)continue;
        if(![referenced containsObject:name]){[NSFileManager.defaultManager removeItemAtURL:file error:nil];continue;}
        NSNumber *size=nil;[file getResourceValue:&size forKey:NSURLFileSizeKey error:nil];bytes+=size.unsignedIntegerValue;
    }
    if(bytes+data.length>128*1024*1024){if(error)*error=HGAAssetError(@"本地外观资源已达到容量上限，请先清除不用的图标或字体。");return nil;}
    NSString *filename=[[HGADigest(data) stringByAppendingString:@"."] stringByAppendingString:extension];
    NSURL *url=[directory URLByAppendingPathComponent:filename];
    if(![data writeToURL:url options:NSDataWritingAtomic error:error])return nil;
    [url setResourceValue:@YES forKey:NSURLIsExcludedFromBackupKey error:nil];
    return url;
}
static NSString *HGAImportImage(NSData *data,NSError **error) {
    if(!data.length || data.length>12*1024*1024){if(error)*error=HGAAssetError(@"图片应不超过 12 MiB。");return nil;}
    CGImageSourceRef source=CGImageSourceCreateWithData((__bridge CFDataRef)data,NULL);
    if(!source){if(error)*error=HGAAssetError(@"不是可读取的图片。");return nil;}
    NSDictionary *options=@{(__bridge NSString *)kCGImageSourceCreateThumbnailFromImageAlways:@YES,
        (__bridge NSString *)kCGImageSourceThumbnailMaxPixelSize:@256,
        (__bridge NSString *)kCGImageSourceCreateThumbnailWithTransform:@YES};
    CGImageRef thumbnail=CGImageSourceCreateThumbnailAtIndex(source,0,(__bridge CFDictionaryRef)options);
    CFRelease(source);
    if(!thumbnail){if(error)*error=HGAAssetError(@"图片缩略图创建失败。");return nil;}
    NSData *png=UIImagePNGRepresentation([UIImage imageWithCGImage:thumbnail]);
    CGImageRelease(thumbnail);
    if(!png || png.length>1024*1024){if(error)*error=HGAAssetError(@"处理后的图标过大。");return nil;}
    return HGAStoreAsset(png,@"png",error).lastPathComponent;
}
static NSArray<NSString *> *HGARegisterFont(NSURL *url,NSError **error) {
    CFArrayRef descriptors=CTFontManagerCreateFontDescriptorsFromURL((__bridge CFURLRef)url);
    if(!descriptors || CFArrayGetCount(descriptors)==0) {
        if(descriptors) CFRelease(descriptors);
        if(error)*error=HGAAssetError(@"字体文件未包含可用字体。"); return nil;
    }
    NSMutableArray *names=[NSMutableArray array];
    for(id descriptor in (__bridge NSArray *)descriptors) {
        NSString *name=CFBridgingRelease(CTFontDescriptorCopyAttribute((__bridge CTFontDescriptorRef)descriptor,kCTFontNameAttribute));
        if([name isKindOfClass:NSString.class] && name.length) [names addObject:name];
        if(names.count>=64)break;
    }
    CFRelease(descriptors);
    CFErrorRef failure=NULL;
    BOOL registered=CTFontManagerRegisterFontsForURL((__bridge CFURLRef)url,kCTFontManagerScopeProcess,&failure);
    NSError *registrationError=CFBridgingRelease(failure);
    if(!registered && registrationError.code!=kCTFontManagerErrorAlreadyRegistered) {
        if(error)*error=registrationError ?: HGAAssetError(@"字体注册失败。"); return nil;
    }
    return names.copy;
}
static NSCache<NSString *,UIImage *> *HGAImages;
static NSString *HGARegisteredFontToken;
static void HGARefreshAssets(void) {
    if(!HGAImages) HGAImages=[NSCache new];
    HGAImages.countLimit=12;
    NSString *token=HGAppearanceString(@"fonts.asset");
    if(token.length && ![token isEqual:HGARegisteredFontToken]) {
        NSError *error=nil;
        if(HGARegisterFont([HGAAssetDirectory() URLByAppendingPathComponent:token],&error)) HGARegisteredFontToken=token;
        else HGRecordCapability(@"appearance.font.asset",@{@"module":@"appearance",@"state":@"unavailable",@"reason":@"font asset unavailable; native font retained"});
    }
    // Decode outside a host layout callback. There are only eight bounded slots.
    for(NSString *role in @[@"like",@"collect",@"comment",@"share"]) for(NSString *state in @[@"normal",@"selected"]) {
        NSString *key=[NSString stringWithFormat:@"icons.%@.%@.asset",role,state];
        NSString *imageToken=HGAppearanceString(key);
        if(!imageToken.length || [HGAImages objectForKey:imageToken])continue;
        NSData *data=HGAReadFile([HGAAssetDirectory() URLByAppendingPathComponent:imageToken],1024*1024,nil);
        UIImage *image=data ? [UIImage imageWithData:data] : nil;
        if(image) [HGAImages setObject:image forKey:imageToken];
    }
}
static UIImage *HGAIcon(NSString *role,BOOL selected) {
    NSString *slot=[NSString stringWithFormat:@"icons.%@.%@",role,selected?@"selected":@"normal"];
    NSString *token=HGAppearanceString([slot stringByAppendingString:@".asset"]);
    UIImage *image=token.length ? [HGAImages objectForKey:token] : nil;
    if(image)return image;
    NSString *symbol=HGAppearanceString([slot stringByAppendingString:@".symbol"]);
    if(symbol.length) image=[UIImage systemImageNamed:symbol];
    if(image)return image;
    NSString *base=@{@"like":@"heart",@"collect":@"star",@"comment":@"bubble.right",@"share":@"arrowshape.turn.up.right"}[role];
    if(!base)return nil;
    NSString *preset=HGAppearanceString(@"icons.preset");
    NSString *name=base;
    if([preset isEqual:@"circle"])name=[base stringByAppendingString:@".circle"];
    else if([preset isEqual:@"square"])name=[base stringByAppendingString:@".square"];
    if(selected || [preset isEqual:@"filled"])name=[name stringByAppendingString:@".fill"];
    image=[UIImage systemImageNamed:name] ?: [UIImage systemImageNamed:selected?[base stringByAppendingString:@".fill"]:base];
    return [image imageWithConfiguration:[UIImageSymbolConfiguration configurationWithPointSize:HGAppearanceNumber(@"icons.size") weight:UIImageSymbolWeightSemibold]];
}
