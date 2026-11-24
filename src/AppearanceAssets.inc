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
    if(bytes+data.length>128*1024*1024){if(error)*error=HGAAssetError(@"本地外观资源已达到容量上限，请先清除不用的字体。");return nil;}
    NSString *filename=[[HGADigest(data) stringByAppendingString:@"."] stringByAppendingString:extension];
    NSURL *url=[directory URLByAppendingPathComponent:filename];
    if(![data writeToURL:url options:NSDataWritingAtomic error:error])return nil;
    [url setResourceValue:@YES forKey:NSURLIsExcludedFromBackupKey error:nil];
    return url;
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
static NSString *HGARegisteredFontToken;
static void HGARefreshAssets(void) {
    NSString *token=HGAppearanceString(@"fonts.asset");
    if(token.length && ![token isEqual:HGARegisteredFontToken]) {
        NSError *error=nil;
        if(HGARegisterFont([HGAAssetDirectory() URLByAppendingPathComponent:token],&error)) HGARegisteredFontToken=token;
        else HGRecordCapability(@"appearance.font.asset",@{@"module":@"appearance",@"state":@"unavailable",@"reason":@"font asset unavailable; native font retained"});
    }
}
static UIColor *HGAActionIconColor(NSString *role,BOOL selected) {
    if(!selected)return UIColor.whiteColor;
    return [role isEqual:@"like"] ? UIColor.systemPinkColor : UIColor.systemYellowColor;
}

static UIBezierPath *HGAActionStar(void) {
    CGPoint tips[10];
    CGFloat inner=8.2;
    CGFloat roundness=0.25;
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

static UIBezierPath *HGAActionBubble(void) {
    UIBezierPath *path=[UIBezierPath bezierPath];
    CGFloat dotsY=14.5;
    [path moveToPoint:CGPointMake(16,3.8)];
    [path addCurveToPoint:CGPointMake(29.7,14.5) controlPoint1:CGPointMake(24.5,3.8) controlPoint2:CGPointMake(29.7,7.5)];
    [path addCurveToPoint:CGPointMake(20.5,24.2) controlPoint1:CGPointMake(29.7,20.3) controlPoint2:CGPointMake(26.3,23.5)];
    [path addQuadCurveToPoint:CGPointMake(17.2,28.4) controlPoint:CGPointMake(21,26.3)];
    [path addQuadCurveToPoint:CGPointMake(14.8,24.7) controlPoint:CGPointMake(15,29.2)];
    [path addCurveToPoint:CGPointMake(2.3,14.5) controlPoint1:CGPointMake(7.0,24.7) controlPoint2:CGPointMake(2.3,20.4)];
    [path addCurveToPoint:CGPointMake(16,3.8) controlPoint1:CGPointMake(2.3,7.6) controlPoint2:CGPointMake(7.5,3.8)];
    [path closePath];
    path.usesEvenOddFillRule=YES;
    CGFloat dot=4;
    for(NSUInteger i=0;i<3;i++) {
        CGFloat x=10+i*6;
        [path appendPath:[UIBezierPath bezierPathWithOvalInRect:CGRectMake(x-dot/2,dotsY-dot/2,dot,dot)]];
    }
    return path;
}

static UIBezierPath *HGAActionHeart(void) {
    UIBezierPath *path=[UIBezierPath bezierPath];
    CGFloat outer=29.8;
    CGFloat middle=12.8;
    CGFloat tip=27.5;
    CGFloat split=8.1;
    [path moveToPoint:CGPointMake(16,split)];
    [path addCurveToPoint:CGPointMake(outer,middle) controlPoint1:CGPointMake(21,1)
        controlPoint2:CGPointMake(outer,4)];
    [path addCurveToPoint:CGPointMake(16,tip) controlPoint1:CGPointMake(outer,19.8)
        controlPoint2:CGPointMake(22,tip-3.1)];
    [path addCurveToPoint:CGPointMake(32-outer,middle) controlPoint1:CGPointMake(10,tip-3.1)
        controlPoint2:CGPointMake(32-outer,19.8)];
    [path addCurveToPoint:CGPointMake(16,split) controlPoint1:CGPointMake(32-outer,4)
        controlPoint2:CGPointMake(11,1)];
    [path closePath];
    return path;
}

static UIBezierPath *HGAActionShare(void) {
    UIBezierPath *path=[UIBezierPath bezierPath];
    CGFloat shoulder=14.5;
    CGFloat upper=5.2;
    CGFloat lower=26.1;
    CGFloat tip=29;
    [path moveToPoint:CGPointMake(shoulder,10.9)];
    [path addLineToPoint:CGPointMake(shoulder,upper)];
    [path addQuadCurveToPoint:CGPointMake(shoulder+2,upper-0.8) controlPoint:CGPointMake(shoulder,upper-2)];
    [path addLineToPoint:CGPointMake(tip-0.8,14.1)];
    [path addQuadCurveToPoint:CGPointMake(tip-0.8,16.7) controlPoint:CGPointMake(tip+1,15.4)];
    [path addLineToPoint:CGPointMake(shoulder+2,lower+0.8)];
    [path addQuadCurveToPoint:CGPointMake(shoulder,lower) controlPoint:CGPointMake(shoulder,lower+2)];
    [path addLineToPoint:CGPointMake(shoulder,21.3)];
    [path addCurveToPoint:CGPointMake(3.7,23.4) controlPoint1:CGPointMake(9.7,21.8) controlPoint2:CGPointMake(6.1,23.1)];
    [path addQuadCurveToPoint:CGPointMake(2.7,22.5) controlPoint:CGPointMake(1.7,25)];
    [path addCurveToPoint:CGPointMake(shoulder,10.9) controlPoint1:CGPointMake(3.2,16.1)
        controlPoint2:CGPointMake(7.5,9.9)];
    [path closePath];
    return path;
}

static UIImage *HGAFlatActionImage(NSString *role,BOOL selected) {
    static NSMutableDictionary<NSString *,UIImage *> *images;
    if(!images)images=[NSMutableDictionary dictionary];
    NSString *key=[NSString stringWithFormat:@"%@/%d",role,selected];
    UIImage *image=images[key];
    if(image)return image;

    UIBezierPath *path=nil;
    if([role isEqual:@"collect"])path=HGAActionStar();
    else if([role isEqual:@"comment"])path=HGAActionBubble();
    else if([role isEqual:@"like"])path=HGAActionHeart();
    else if([role isEqual:@"share"])path=HGAActionShare();
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
