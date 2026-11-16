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
typedef NS_ENUM(NSUInteger,HGAActionIconStyle) {
    HGAActionIconClassic,
    HGAActionIconGeometric,
    HGAActionIconSoft
};

static UIColor *HGAActionIconColor(NSString *role,BOOL selected) {
    if(!selected)return UIColor.whiteColor;
    return [role isEqual:@"like"] ? UIColor.systemPinkColor : UIColor.systemYellowColor;
}

static UIBezierPath *HGAActionStar(HGAActionIconStyle style) {
    CGPoint tips[10];
    CGFloat inner=style==HGAActionIconGeometric ? 6.4 : (style==HGAActionIconSoft ? 8.2 : 7.5);
    CGFloat roundness=style==HGAActionIconGeometric ? 0.035 : (style==HGAActionIconSoft ? 0.25 : 0.12);
    for(NSUInteger i=0;i<10;i++) {
        CGFloat angle=i*M_PI/5-M_PI_2;
        CGFloat radius=i%2 ? inner : 14.3;
        tips[i]=CGPointMake(16+cos(angle)*radius,16.9+sin(angle)*radius);
    }
    UIBezierPath *path=[UIBezierPath bezierPath];
    for(NSUInteger i=0;i<10;i++) {
        CGPoint point=tips[i],previous=tips[(i+9)%10],next=tips[(i+1)%10];
        CGPoint start=CGPointMake(point.x+(previous.x-point.x)*roundness,point.y+(previous.y-point.y)*roundness);
        CGPoint end=CGPointMake(point.x+(next.x-point.x)*roundness,point.y+(next.y-point.y)*roundness);
        if(i==0)[path moveToPoint:start];else [path addLineToPoint:start];
        [path addQuadCurveToPoint:end controlPoint:point];
    }
    [path closePath];
    return path;
}

static UIBezierPath *HGAActionBubble(HGAActionIconStyle style) {
    UIBezierPath *path=[UIBezierPath bezierPath];
    CGFloat dotsY=14;
    if(style==HGAActionIconGeometric) {
        [path moveToPoint:CGPointMake(8,4.5)];
        [path addLineToPoint:CGPointMake(24,4.5)];
        [path addQuadCurveToPoint:CGPointMake(29,9.5) controlPoint:CGPointMake(29,4.5)];
        [path addLineToPoint:CGPointMake(29,19)];
        [path addQuadCurveToPoint:CGPointMake(24,24) controlPoint:CGPointMake(29,24)];
        [path addLineToPoint:CGPointMake(16,24)];
        [path addLineToPoint:CGPointMake(9,29)];
        [path addLineToPoint:CGPointMake(9,24)];
        [path addQuadCurveToPoint:CGPointMake(3,19) controlPoint:CGPointMake(3,24)];
        [path addLineToPoint:CGPointMake(3,9.5)];
        [path addQuadCurveToPoint:CGPointMake(8,4.5) controlPoint:CGPointMake(3,4.5)];
    } else if(style==HGAActionIconSoft) {
        dotsY=14.5;
        [path moveToPoint:CGPointMake(16,3.8)];
        [path addCurveToPoint:CGPointMake(29.7,14.5) controlPoint1:CGPointMake(24.5,3.8) controlPoint2:CGPointMake(29.7,7.5)];
        [path addCurveToPoint:CGPointMake(20.5,24.2) controlPoint1:CGPointMake(29.7,20.3) controlPoint2:CGPointMake(26.3,23.5)];
        [path addQuadCurveToPoint:CGPointMake(17.2,28.4) controlPoint:CGPointMake(21,26.3)];
        [path addQuadCurveToPoint:CGPointMake(14.8,24.7) controlPoint:CGPointMake(15,29.2)];
        [path addCurveToPoint:CGPointMake(2.3,14.5) controlPoint1:CGPointMake(7.0,24.7) controlPoint2:CGPointMake(2.3,20.4)];
        [path addCurveToPoint:CGPointMake(16,3.8) controlPoint1:CGPointMake(2.3,7.6) controlPoint2:CGPointMake(7.5,3.8)];
    } else {
        [path moveToPoint:CGPointMake(16,2.8)];
        [path addCurveToPoint:CGPointMake(29.6,14.1) controlPoint1:CGPointMake(23.5,2.8) controlPoint2:CGPointMake(29.6,7.9)];
        [path addCurveToPoint:CGPointMake(15.9,29.1) controlPoint1:CGPointMake(29.6,20.5) controlPoint2:CGPointMake(23.7,25.9)];
        [path addQuadCurveToPoint:CGPointMake(14,27.1) controlPoint:CGPointMake(13.4,30.1)];
        [path addLineToPoint:CGPointMake(14.6,25.2)];
        [path addCurveToPoint:CGPointMake(2.4,14.1) controlPoint1:CGPointMake(7.5,24.5) controlPoint2:CGPointMake(2.4,20)];
        [path addCurveToPoint:CGPointMake(16,2.8) controlPoint1:CGPointMake(2.4,7.9) controlPoint2:CGPointMake(8.5,2.8)];
    }
    [path closePath];
    path.usesEvenOddFillRule=YES;
    CGFloat dot=style==HGAActionIconSoft ? 4 : 3.5;
    for(NSUInteger i=0;i<3;i++) {
        CGFloat x=10+i*6;
        [path appendPath:[UIBezierPath bezierPathWithOvalInRect:CGRectMake(x-dot/2,dotsY-dot/2,dot,dot)]];
    }
    return path;
}

static UIBezierPath *HGAActionHeart(HGAActionIconStyle style) {
    UIBezierPath *path=[UIBezierPath bezierPath];
    if(style==HGAActionIconGeometric) {
        [path moveToPoint:CGPointMake(16,7.5)];
        [path addQuadCurveToPoint:CGPointMake(28,7.3) controlPoint:CGPointMake(22,0.6)];
        [path addQuadCurveToPoint:CGPointMake(28.4,18) controlPoint:CGPointMake(32,11.8)];
        [path addLineToPoint:CGPointMake(17.3,28.2)];
        [path addQuadCurveToPoint:CGPointMake(14.7,28.2) controlPoint:CGPointMake(16,29.5)];
        [path addLineToPoint:CGPointMake(3.6,18)];
        [path addQuadCurveToPoint:CGPointMake(4,7.3) controlPoint:CGPointMake(0,11.8)];
        [path addQuadCurveToPoint:CGPointMake(16,7.5) controlPoint:CGPointMake(10,0.6)];
    } else {
        BOOL soft=style==HGAActionIconSoft;
        CGFloat outer=soft ? 29.8 : 29.2;
        CGFloat middle=soft ? 12.8 : 12.2;
        CGFloat tip=soft ? 27.5 : 28.6;
        CGFloat split=soft ? 8.1 : 7.3;
        [path moveToPoint:CGPointMake(16,split)];
        [path addCurveToPoint:CGPointMake(outer,middle) controlPoint1:CGPointMake(21,1)
            controlPoint2:CGPointMake(outer,4)];
        [path addCurveToPoint:CGPointMake(16,tip) controlPoint1:CGPointMake(outer,19.8)
            controlPoint2:CGPointMake(22,tip-3.1)];
        [path addCurveToPoint:CGPointMake(32-outer,middle) controlPoint1:CGPointMake(10,tip-3.1)
            controlPoint2:CGPointMake(32-outer,19.8)];
        [path addCurveToPoint:CGPointMake(16,split) controlPoint1:CGPointMake(32-outer,4)
            controlPoint2:CGPointMake(11,1)];
    }
    [path closePath];
    return path;
}

static UIBezierPath *HGAActionShare(HGAActionIconStyle style) {
    UIBezierPath *path=[UIBezierPath bezierPath];
    BOOL geometric=style==HGAActionIconGeometric;
    BOOL soft=style==HGAActionIconSoft;
    CGFloat shoulder=geometric ? 16.6 : 14.5;
    CGFloat upper=soft ? 5.2 : 3.8;
    CGFloat lower=soft ? 26.1 : 27.2;
    CGFloat tip=geometric ? 29.9 : 29;
    [path moveToPoint:CGPointMake(shoulder,10.9)];
    [path addLineToPoint:CGPointMake(shoulder,upper)];
    [path addQuadCurveToPoint:CGPointMake(shoulder+2,upper-0.8) controlPoint:CGPointMake(shoulder,upper-2)];
    [path addLineToPoint:CGPointMake(tip-0.8,14.1)];
    [path addQuadCurveToPoint:CGPointMake(tip-0.8,16.7) controlPoint:CGPointMake(tip+1,15.4)];
    [path addLineToPoint:CGPointMake(shoulder+2,lower+0.8)];
    [path addQuadCurveToPoint:CGPointMake(shoulder,lower) controlPoint:CGPointMake(shoulder,lower+2)];
    [path addLineToPoint:CGPointMake(shoulder,21.3)];
    [path addCurveToPoint:CGPointMake(3.7,soft?23.4:24.5) controlPoint1:CGPointMake(9.7,21.8) controlPoint2:CGPointMake(6.1,23.1)];
    [path addQuadCurveToPoint:CGPointMake(soft?2.7:2.3,22.5) controlPoint:CGPointMake(1.7,25)];
    [path addCurveToPoint:CGPointMake(shoulder,10.9) controlPoint1:CGPointMake(3.2,geometric?14.7:16.1)
        controlPoint2:CGPointMake(7.5,soft?9.9:11.5)];
    [path closePath];
    return path;
}

static UIImage *HGAActionPresetImage(NSString *role,BOOL selected,NSString *preset) {
    static NSMutableDictionary<NSString *,UIImage *> *images;
    if(!images)images=[NSMutableDictionary dictionary];
    NSString *key=[NSString stringWithFormat:@"%@/%@/%d",preset,role,selected];
    UIImage *image=images[key];
    if(image)return image;

    HGAActionIconStyle style=[preset isEqual:@"square"] ? HGAActionIconGeometric :
        ([preset isEqual:@"circle"] ? HGAActionIconSoft : HGAActionIconClassic);
    UIBezierPath *path=nil;
    if([role isEqual:@"collect"])path=HGAActionStar(style);
    else if([role isEqual:@"comment"])path=HGAActionBubble(style);
    else if([role isEqual:@"like"])path=HGAActionHeart(style);
    else if([role isEqual:@"share"])path=HGAActionShare(style);
    if(!path)return nil;

    UIGraphicsImageRendererFormat *format=[UIGraphicsImageRendererFormat defaultFormat];
    format.opaque=NO;
    format.scale=4;
    UIGraphicsImageRenderer *renderer=[[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(32,32) format:format];
    image=[[renderer imageWithActions:^(UIGraphicsImageRendererContext *context){
        [HGAActionIconColor(role,selected) setFill];
        [path fill];
    }] imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
    images[key]=image;
    return image;
}

static UIImage *HGAIcon(NSString *role,BOOL selected,BOOL *drawnPreset) {
    NSString *slot=[NSString stringWithFormat:@"icons.%@.%@",role,selected?@"selected":@"normal"];
    NSString *token=HGAppearanceString([slot stringByAppendingString:@".asset"]);
    UIImage *image=token.length ? [HGAImages objectForKey:token] : nil;
    if(image)return image;
    NSString *symbol=HGAppearanceString([slot stringByAppendingString:@".symbol"]);
    if(symbol.length) image=[UIImage systemImageNamed:symbol];
    if(image)return image;
    *drawnPreset=YES;
    return HGAActionPresetImage(role,selected,HGAppearanceString(@"icons.preset"));
}
