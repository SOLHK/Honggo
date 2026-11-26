#import "DockSelectionGeometry.h"

static NSInteger (*HGAOriginalPortraitFinishAction)(id,SEL,id);
static BOOL (*HGAOriginalLandscapeInflowBlocked)(id,SEL,id);

static NSInteger HGAPortraitFinishAction(id component,SEL cmd,id model) {
    NSInteger action=HGAOriginalPortraitFinishAction(component,cmd,model);
    // Eggplant 7.4.5.33, cellView:playFinishWithError:model: jump table.
    // 4 opens the series, 6 opens it at the next episode, 2 replays in place.
    if ((action==4 || action==6) && HGAppearanceFlag(@"player.disableAutoEnterSeries")) return 2;
    return action;
}

static BOOL HGALandscapeInflowBlocked(id source,SEL cmd,id model) {
    return HGAppearanceFlag(@"player.disableAutoEnterSeries") || HGAOriginalLandscapeInflowBlocked(source,cmd,model);
}

static void HGAInstallAutoEnterSeriesPolicy(void) {
    if (![[NSBundle.mainBundle objectForInfoDictionaryKey:@"CFBundleVersion"] isEqual:@"7.4.5.33"]) return;
    Class portrait=objc_lookUpClass("FQVShortVideoListVCPlayComponent");
    if (portrait && !HGAOriginalPortraitFinishAction)
        HGAOriginalPortraitFinishAction=(void *)HGInstallVerifiedHook(portrait,
            sel_registerName("finishActionTypeAtCurrentTimeWithVideoDataModel:"),"q24@0:8@16",
            (IMP)HGAPortraitFinishAction,@"appearance");
    // Landscape uses a separate native policy for automatic outer-to-inner playback.
    Class landscape=objc_lookUpClass("SSShortVideoLandscapeSessionDataSource");
    if (landscape && !HGAOriginalLandscapeInflowBlocked)
        HGAOriginalLandscapeInflowBlocked=(void *)HGInstallVerifiedHook(landscape,
            sel_registerName("isAutoInflowBlockedForVideoDataModel:"),"B24@0:8@16",
            (IMP)HGALandscapeInflowBlocked,@"appearance");
}

// The episode entry has a native content view inside an insert wrapper. A
// rounded material alone leaves their rectangular fills visible at its corners.
// Decorate only this observed component; preserve native control proportions.
@interface HGAEpisodeInsetState : NSObject
@property(nonatomic) CGRect sourceFrame;
@property(nonatomic) CGRect appliedFrame;
@property(nonatomic) CGRect sourceBounds;
@property(nonatomic) CGRect appliedBounds;
@property(nonatomic) CGFloat sourceCenterX;
@property(nonatomic) CGFloat appliedCenterX;
@property(nonatomic,weak) UIView *parent;
@end
@implementation HGAEpisodeInsetState @end
static char HGAEpisodeInsetKey;
static void HGARestoreEpisodeInset(UIView *entry) {
    HGAEpisodeInsetState *state=objc_getAssociatedObject(entry,&HGAEpisodeInsetKey);
    if(!state)return;
    BOOL changed=NO;
    if(CGRectEqualToRect(entry.bounds,state.appliedBounds)){entry.bounds=state.sourceBounds;changed=YES;}
    if(state.sourceCenterX!=state.appliedCenterX && fabs(entry.center.x-state.appliedCenterX)<0.5) {
        entry.center=CGPointMake(state.sourceCenterX,entry.center.y);changed=YES;
    } else if(fabs(CGRectGetMinX(entry.frame)-CGRectGetMinX(state.appliedFrame))<0.5 &&
        fabs(CGRectGetMinX(state.appliedFrame)-CGRectGetMinX(state.sourceFrame))>=0.5) {
        // A reused distribution entry can resize its width while keeping the
        // shifted left edge from our previous inset pass.
        CGRect frame=entry.frame;
        frame.origin.x=state.sourceFrame.origin.x;
        entry.frame=frame;changed=YES;
    }
    objc_setAssociatedObject(entry,&HGAEpisodeInsetKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    if(changed)[entry setNeedsLayout];
}
static void HGAApplyEpisodeInset(UIView *entry,BOOL enabled) {
    HGAEpisodeInsetState *state=objc_getAssociatedObject(entry,&HGAEpisodeInsetKey);
    if(state && state.parent!=entry.superview){HGARestoreEpisodeInset(entry);state=nil;}
    BOOL keepsAppliedWidth=state && CGRectEqualToRect(entry.bounds,state.appliedBounds);
    BOOL keepsAppliedLeft=state && fabs(CGRectGetMinX(entry.frame)-CGRectGetMinX(state.appliedFrame))<0.5 &&
        fabs(CGRectGetMinX(state.appliedFrame)-CGRectGetMinX(state.sourceFrame))>=0.5;
    CGRect source=keepsAppliedWidth ? state.sourceBounds : entry.bounds;
    CGRect sourceFrame=keepsAppliedWidth ? state.sourceFrame : entry.frame;
    if(!keepsAppliedWidth && keepsAppliedLeft)sourceFrame.origin.x=state.sourceFrame.origin.x;
    CGFloat center=(state && fabs(entry.center.x-state.appliedCenterX)<0.5) ? state.sourceCenterX : CGRectGetMidX(sourceFrame);
    CGFloat width=source.size.width;
    UIWindow *window=entry.window;
    CGRect dock=enabled && HGAppearanceFlag(@"player.episodeDockWidth") ? HGAExpandedCapsuleInWindow(window) : CGRectNull;
    BOOL match=!CGRectIsNull(dock) && !CGRectIsEmpty(dock);
    CGFloat targetWidth=window ? (match ? CGRectGetWidth([entry convertRect:dock fromView:window]) :
        HGADockRowWidth(window.bounds.size.width,HGADockDefaultWidth,0,4,4,true)) : 0;
    BOOL fullWidthParent=window && entry.superview && CGRectGetWidth(entry.superview.bounds)>=window.bounds.size.width*0.88;
    BOOL needsInset=window && (width>=window.bounds.size.width*0.88 || (match && fullWidthParent && width>targetWidth+0.5));
    // Only the edge-to-edge distribution entry (or a collection entry wider
    // than the matched Dock inside a full-width player row) needs inset sizing.
    // Change its layout width, not its transform, so native text/icons keep
    // their proportions.
    if(!enabled || !window || (!match && !CGAffineTransformIsIdentity(entry.transform)) ||
        window.bounds.size.width>=window.bounds.size.height ||
        !needsInset || width>window.bounds.size.width+1 || source.size.height>80) {
        HGARestoreEpisodeInset(entry);return;
    }
    if(!state){state=[HGAEpisodeInsetState new];objc_setAssociatedObject(entry,&HGAEpisodeInsetKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);}
    state.sourceFrame=sourceFrame;state.sourceBounds=source;state.parent=entry.superview;
    // The fixed reference is the stock four-tab Dock with no accessory.
    // It remains centered even when the live Dock is narrower or collapsed.
    CGRect bounds=source;
    bounds.size.width=targetWidth;
    CGFloat baseCenter=fullWidthParent ? CGRectGetMidX(entry.superview.bounds) : center;
    CGFloat targetCenter=baseCenter;
    if(match) {
        // Width is measured in the entry's coordinates so a user scale still
        // produces the requested visible length. Carry the capsule offset
        // relative to the window center so an incoming cell stays on its page.
        CGPoint screenCenter=CGPointMake(CGRectGetMidX(window.bounds),CGRectGetMidY(dock));
        CGPoint dockCenter=CGPointMake(CGRectGetMidX(dock),screenCenter.y);
        targetCenter+=[entry.superview convertPoint:dockCenter fromView:window].x-
            [entry.superview convertPoint:screenCenter fromView:window].x;
    }
    state.appliedBounds=bounds;state.sourceCenterX=center;state.appliedCenterX=targetCenter;
    if(!CGRectEqualToRect(entry.bounds,bounds)){entry.bounds=bounds;[entry setNeedsLayout];}
    if(fabs(entry.center.x-targetCenter)>0.001)entry.center=CGPointMake(targetCenter,entry.center.y);
    state.appliedFrame=entry.frame;
}
static CGRect HGAEpisodeGlassFrame(UIView *entry) {
    CGRect frame=entry.bounds;
    if(objc_getAssociatedObject(entry,&HGAEpisodeInsetKey) && frame.size.height>36)
        frame=CGRectInset(frame,0,(frame.size.height-36)/2);
    return frame;
}
@interface HGAEpisodeSurfaceState : NSObject
@property(nonatomic,weak) UIView *view;
@property(nonatomic,weak) UIView *owner;
@property(nonatomic,strong) UIColor *background;
@property(nonatomic) BOOL clearsBackground;
@property(nonatomic,strong) UIColor *layerBackground;
@property(nonatomic) BOOL clearsLayerBackground;
@property(nonatomic) CGFloat radius;
@property(nonatomic) CGFloat appliedRadius;
@property(nonatomic,copy) NSString *curve;
@property(nonatomic) CACornerMask corners;
@property(nonatomic) BOOL clips;
@property(nonatomic) BOOL rounded;
@property(nonatomic) BOOL resizedContent;
@property(nonatomic) CGFloat sourceWidth;
@property(nonatomic) CGFloat appliedWidth;
@end
@implementation HGAEpisodeSurfaceState @end
static char HGAEpisodeSurfacesKey;
static char HGAEpisodeWrapperKey;
static char HGAEpisodeLabelsKey;
static const CACornerMask HGAEpisodeAllCorners = kCALayerMinXMinYCorner |
    kCALayerMaxXMinYCorner | kCALayerMinXMaxYCorner | kCALayerMaxXMaxYCorner;

@interface HGAEpisodeForegroundState : NSObject
@property(nonatomic,weak) UIView *view;
@property(nonatomic) CGRect sourceFrame;
@property(nonatomic) CGRect appliedFrame;
@property(nonatomic) BOOL ownsFrame;
@property(nonatomic) NSLineBreakMode sourceBreakMode;
@property(nonatomic) NSLineBreakMode appliedBreakMode;
@property(nonatomic) BOOL ownsBreakMode;
@property(nonatomic,copy) NSString *sourceText;
@property(nonatomic,copy) NSString *appliedText;
@property(nonatomic) BOOL ownsText;
@property(nonatomic,copy) NSAttributedString *sourceRichText;
@property(nonatomic,copy) NSAttributedString *appliedRichText;
@property(nonatomic) BOOL ownsRichText;
@property(nonatomic) BOOL sourceHidden;
@property(nonatomic) BOOL ownsHidden;
@end
@implementation HGAEpisodeForegroundState @end

static void HGARestoreEpisodeForegroundItem(HGAEpisodeForegroundState *state) {
    UIView *view=state.view;
    if(!view)return;
    if(state.ownsHidden && view.hidden)view.hidden=state.sourceHidden;
    if([view isKindOfClass:UILabel.class]) {
        UILabel *label=(UILabel *)view;
        if(state.ownsRichText && [label.attributedText isEqual:state.appliedRichText])
            label.attributedText=state.sourceRichText;
        else if(state.ownsText && [label.text isEqualToString:state.appliedText ?: @""])
            label.text=state.sourceText;
        if(state.ownsBreakMode && label.lineBreakMode==state.appliedBreakMode)
            label.lineBreakMode=state.sourceBreakMode;
    }
    if(state.ownsFrame && CGRectEqualToRect(view.frame,state.appliedFrame))
        view.frame=state.sourceFrame;
    state.ownsHidden=NO;state.ownsRichText=NO;state.ownsText=NO;state.ownsBreakMode=NO;state.ownsFrame=NO;
}

static void HGARestoreEpisodeLabels(UIView *entry) {
    NSArray<HGAEpisodeForegroundState *> *states=objc_getAssociatedObject(entry,&HGAEpisodeLabelsKey);
    if(!states)return;
    for(HGAEpisodeForegroundState *state in states)HGARestoreEpisodeForegroundItem(state);
    objc_setAssociatedObject(entry,&HGAEpisodeLabelsKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

static NSAttributedString *HGAClippedEpisodeRichText(NSAttributedString *source,NSUInteger prefixLength) {
    if(!source.length)return nil;
    NSAttributedString *slice=prefixLength<source.length ?
        [source attributedSubstringFromRange:NSMakeRange(0,prefixLength)] : source;
    if(!slice.length)return slice;
    NSMutableAttributedString *updated=[slice mutableCopy];
    [slice enumerateAttribute:NSParagraphStyleAttributeName inRange:NSMakeRange(0,slice.length) options:0
        usingBlock:^(id value,NSRange range,__unused BOOL *stop) {
            NSMutableParagraphStyle *copy=[value isKindOfClass:NSParagraphStyle.class] ?
                [(NSParagraphStyle *)value mutableCopy] : [NSMutableParagraphStyle new];
            copy.lineBreakMode=NSLineBreakByClipping;
            [updated addAttribute:NSParagraphStyleAttributeName value:copy range:range];
        }];
    return updated;
}

static CGFloat HGAEpisodeSingleLineWidth(UILabel *label,NSAttributedString *rich,NSString *plain) {
    if(rich.length) {
        NSMutableAttributedString *measure=[rich mutableCopy];
        UIFont *fallbackFont=label.font ?: [UIFont systemFontOfSize:14];
        [rich enumerateAttribute:NSFontAttributeName inRange:NSMakeRange(0,rich.length) options:0
            usingBlock:^(id value,NSRange range,__unused BOOL *stop) {
                if(!value)[measure addAttribute:NSFontAttributeName value:fallbackFont range:range];
            }];
        CGRect bounds=[measure boundingRectWithSize:CGSizeMake(4096,MAX(24,CGRectGetHeight(label.bounds)))
            options:NSStringDrawingUsesLineFragmentOrigin|NSStringDrawingUsesFontLeading context:nil];
        return ceil(CGRectGetWidth(bounds));
    }
    if(plain.length) {
        UIFont *font=label.font ?: [UIFont systemFontOfSize:14];
        CGRect bounds=[plain boundingRectWithSize:CGSizeMake(4096,MAX(24,CGRectGetHeight(label.bounds)))
            options:NSStringDrawingUsesLineFragmentOrigin|NSStringDrawingUsesFontLeading
            attributes:@{NSFontAttributeName:font} context:nil];
        return ceil(CGRectGetWidth(bounds));
    }
    return 0;
}

static NSUInteger HGAEpisodeFittingPrefixLength(UILabel *label,NSAttributedString *rich,NSString *plain,CGFloat maxWidth) {
    NSString *full=rich.length ? rich.string : plain;
    if(!full.length || maxWidth<=1.0)return 0;
    if(HGAEpisodeSingleLineWidth(label,rich,plain)<=maxWidth+0.25)return full.length;
    NSUInteger cursor=full.length;
    while(cursor>0) {
        NSRange seq=[full rangeOfComposedCharacterSequenceAtIndex:cursor-1];
        cursor=seq.location;
        while(cursor>0) {
            unichar tail=[full characterAtIndex:cursor-1];
            if(tail==' ' || tail==0x00A0 || tail==0x3000)cursor--;
            else break;
        }
        if(!cursor)break;
        NSAttributedString *subRich=rich.length ? [rich attributedSubstringFromRange:NSMakeRange(0,cursor)] : nil;
        NSString *subPlain=rich.length ? nil : [full substringToIndex:cursor];
        if(HGAEpisodeSingleLineWidth(label,subRich,subPlain)<=maxWidth+0.25)return cursor;
    }
    return 0;
}

static void HGAApplyEpisodeLabels(UIView *entry,CGRect glassFrame,BOOL enabled) {
    if(!enabled || !entry.window || CGRectIsEmpty(glassFrame)) {
        HGARestoreEpisodeLabels(entry);
        return;
    }
    for(UIView *child in entry.subviews) {
        if([child isKindOfClass:objc_lookUpClass("SSShortVideoDistributionContentBaseView")] ||
           [child isKindOfClass:objc_lookUpClass("SSShortVideoDistributionContentView")]) {
            [child layoutIfNeeded];
            break;
        }
    }
    NSArray<HGAEpisodeForegroundState *> *previous=objc_getAssociatedObject(entry,&HGAEpisodeLabelsKey);
    NSMapTable<UIView *,HGAEpisodeForegroundState *> *byView=[NSMapTable weakToStrongObjectsMapTable];
    for(HGAEpisodeForegroundState *s in previous)if(s.view)[byView setObject:s forKey:s.view];

    NSMutableArray<UILabel *> *labels=[NSMutableArray array];
    NSMutableArray<UIImageView *> *leadIcons=[NSMutableArray array];
    UIImageView *trailingIcon=nil;
    CGFloat glassW=CGRectGetWidth(glassFrame);
    for(UIView *node in HGADescendants(entry,48)) {
        if(node==entry || !CGAffineTransformIsIdentity(node.transform))continue;
        HGAEpisodeForegroundState *old=[byView objectForKey:node];
        BOOL wasHidden=old && old.ownsHidden ? old.sourceHidden : node.hidden;
        if(wasHidden || node.alpha<0.01)continue;
        CGRect baseFrame=(old && old.ownsFrame && CGRectEqualToRect(node.frame,old.appliedFrame)) ? old.sourceFrame : node.frame;
        CGRect inEntry=[node.superview convertRect:baseFrame toView:entry];
        CGRect inGlass=CGRectOffset(inEntry,-glassFrame.origin.x,-glassFrame.origin.y);
        if([node isKindOfClass:UILabel.class]) {
            UILabel *lbl=(UILabel *)node;
            NSString *str=(old && old.ownsRichText && [lbl.attributedText isEqual:old.appliedRichText]) ?
                old.sourceRichText.string : ((old && old.ownsText && [lbl.text isEqualToString:old.appliedText ?: @""]) ?
                old.sourceText : (lbl.attributedText.string ?: lbl.text));
            if(str.length && CGRectGetHeight(inGlass)>0)[labels addObject:lbl];
        } else if([node isKindOfClass:UIImageView.class] && ![(UIImageView *)node isAnimating] &&
                  CGRectGetWidth(inGlass)>0 && CGRectGetWidth(inGlass)<=36 && CGRectGetHeight(inGlass)<=36) {
            if(CGRectGetMinX(inGlass)<glassW*0.35)[leadIcons addObject:(UIImageView *)node];
            else if(CGRectGetWidth(inGlass)<=18 && CGRectGetMaxX(inGlass)<=glassW-6 && CGRectGetMinX(inGlass)>glassW*0.75)
                trailingIcon=(UIImageView *)node;
        }
    }
    if(!labels.count) {
        HGARestoreEpisodeLabels(entry);
        return;
    }

    [leadIcons sortUsingComparator:^NSComparisonResult(UIImageView *a,UIImageView *b) {
        HGAEpisodeForegroundState *sa=[byView objectForKey:a],*sb=[byView objectForKey:b];
        CGRect fa=(sa && sa.ownsFrame && CGRectEqualToRect(a.frame,sa.appliedFrame)) ? sa.sourceFrame : a.frame;
        CGRect fb=(sb && sb.ownsFrame && CGRectEqualToRect(b.frame,sb.appliedFrame)) ? sb.sourceFrame : b.frame;
        CGFloat xa=CGRectGetMinX([a.superview convertRect:fa toView:entry]);
        CGFloat xb=CGRectGetMinX([b.superview convertRect:fb toView:entry]);
        return xa<xb ? NSOrderedAscending : (xa>xb ? NSOrderedDescending : NSOrderedSame);
    }];
    [labels sortUsingComparator:^NSComparisonResult(UILabel *a,UILabel *b) {
        HGAEpisodeForegroundState *sa=[byView objectForKey:a],*sb=[byView objectForKey:b];
        CGRect fa=(sa && sa.ownsFrame && CGRectEqualToRect(a.frame,sa.appliedFrame)) ? sa.sourceFrame : a.frame;
        CGRect fb=(sb && sb.ownsFrame && CGRectEqualToRect(b.frame,sb.appliedFrame)) ? sb.sourceFrame : b.frame;
        CGFloat xa=CGRectGetMinX([a.superview convertRect:fa toView:entry]);
        CGFloat xb=CGRectGetMinX([b.superview convertRect:fb toView:entry]);
        return xa<xb ? NSOrderedAscending : (xa>xb ? NSOrderedDescending : NSOrderedSame);
    }];

    CGFloat leadX=CGFLOAT_MAX;
    for(UIImageView *icon in leadIcons) {
        HGAEpisodeForegroundState *old=[byView objectForKey:icon];
        CGRect base=(old && old.ownsFrame && CGRectEqualToRect(icon.frame,old.appliedFrame)) ? old.sourceFrame : icon.frame;
        CGRect r=CGRectOffset([icon.superview convertRect:base toView:entry],-glassFrame.origin.x,-glassFrame.origin.y);
        leadX=MIN(leadX,CGRectGetMinX(r));
    }
    for(UILabel *lbl in labels) {
        HGAEpisodeForegroundState *old=[byView objectForKey:lbl];
        CGRect base=(old && old.ownsFrame && CGRectEqualToRect(lbl.frame,old.appliedFrame)) ? old.sourceFrame : lbl.frame;
        CGRect r=CGRectOffset([lbl.superview convertRect:base toView:entry],-glassFrame.origin.x,-glassFrame.origin.y);
        leadX=MIN(leadX,CGRectGetMinX(r));
    }
    CGFloat padLeft=isfinite(leadX) ? MIN(18.0,MAX(12.0,leadX)) : 14.0;
    CGFloat rightLimit=glassW-padLeft;
    if(trailingIcon) {
        CGRect tr=CGRectOffset([trailingIcon.superview convertRect:trailingIcon.frame toView:entry],-glassFrame.origin.x,-glassFrame.origin.y);
        rightLimit=MIN(rightLimit,CGRectGetMinX(tr)-6.0);
    }

    NSMutableArray<HGAEpisodeForegroundState *> *active=[NSMutableArray array];
    NSMutableArray<HGAEpisodeForegroundState *> *visibleStates=[NSMutableArray array];
    CGFloat cursorX=padLeft;
    BOOL needsAdjustment=NO;

    for(UIImageView *icon in leadIcons) {
        HGAEpisodeForegroundState *s=[byView objectForKey:icon] ?: [HGAEpisodeForegroundState new];
        if(!s.ownsFrame || !CGRectEqualToRect(icon.frame,s.appliedFrame))s.sourceFrame=icon.frame;
        s.view=icon;
        CGRect inGlass=CGRectOffset([icon.superview convertRect:s.sourceFrame toView:entry],-glassFrame.origin.x,-glassFrame.origin.y);
        if(CGRectGetMinX(inGlass)<cursorX-0.25) {
            inGlass.origin.x=cursorX;
            needsAdjustment=YES;
        }
        CGRect targetInEntry=CGRectOffset(inGlass,glassFrame.origin.x,glassFrame.origin.y);
        CGRect targetFrame=[icon.superview convertRect:targetInEntry fromView:entry];
        s.appliedFrame=targetFrame;s.ownsFrame=YES;
        if(!CGRectEqualToRect(icon.frame,targetFrame))icon.frame=targetFrame;
        cursorX=MAX(cursorX,CGRectGetMaxX(inGlass)+6.0);
        [active addObject:s];
        [visibleStates addObject:s];
    }

    for(UILabel *lbl in labels) {
        HGAEpisodeForegroundState *s=[byView objectForKey:lbl] ?: [HGAEpisodeForegroundState new];
        s.view=lbl;
        if(!s.ownsFrame || !CGRectEqualToRect(lbl.frame,s.appliedFrame))s.sourceFrame=lbl.frame;
        if(!s.ownsHidden || !lbl.hidden)s.sourceHidden=lbl.hidden;
        if(!s.ownsBreakMode || lbl.lineBreakMode!=s.appliedBreakMode)s.sourceBreakMode=lbl.lineBreakMode;
        BOOL matchesRich=s.ownsRichText && [lbl.attributedText isEqual:s.appliedRichText];
        BOOL matchesPlain=s.ownsText && [lbl.text isEqualToString:s.appliedText ?: @""];
        if(!matchesRich && !matchesPlain) {
            s.sourceRichText=[lbl.attributedText copy];
            s.sourceText=[lbl.text copy];
        }
        NSAttributedString *baseRich=s.sourceRichText;
        NSString *baseText=s.sourceText;
        NSString *full=baseRich.length ? baseRich.string : baseText;

        CGRect inEntry=[lbl.superview convertRect:s.sourceFrame toView:entry];
        CGRect inGlass=CGRectOffset(inEntry,-glassFrame.origin.x,-glassFrame.origin.y);
        if(CGRectGetMinX(inGlass)<cursorX-0.25) {
            inGlass.origin.x=cursorX;
            needsAdjustment=YES;
        }
        CGFloat availW=rightLimit-CGRectGetMinX(inGlass);
        NSUInteger fitLen=HGAEpisodeFittingPrefixLength(lbl,baseRich,baseText,availW);
        if(fitLen==0 || availW<12.0) {
            if(s.ownsRichText && [lbl.attributedText isEqual:s.appliedRichText])lbl.attributedText=s.sourceRichText;
            if(s.ownsText && [lbl.text isEqualToString:s.appliedText ?: @""])lbl.text=s.sourceText;
            if(s.ownsBreakMode && lbl.lineBreakMode==s.appliedBreakMode)lbl.lineBreakMode=s.sourceBreakMode;
            if(s.ownsFrame && CGRectEqualToRect(lbl.frame,s.appliedFrame))lbl.frame=s.sourceFrame;
            s.ownsRichText=NO;s.ownsText=NO;s.ownsBreakMode=NO;s.ownsFrame=NO;
            s.ownsHidden=YES;
            if(!lbl.hidden)lbl.hidden=YES;
            needsAdjustment=YES;
            [active addObject:s];
            continue;
        }
        if(s.ownsHidden && lbl.hidden)lbl.hidden=s.sourceHidden;
        s.ownsHidden=NO;

        BOOL trimmed=(fitLen<full.length);
        if(trimmed)needsAdjustment=YES;
        if(baseRich.length) {
            NSAttributedString *clipped=HGAClippedEpisodeRichText(baseRich,fitLen);
            if(clipped && (trimmed || ![baseRich isEqual:clipped])) {
                s.appliedRichText=clipped;s.ownsRichText=YES;s.ownsText=NO;
                if(![lbl.attributedText isEqual:clipped])lbl.attributedText=clipped;
            } else if(s.ownsRichText) {
                if([lbl.attributedText isEqual:s.appliedRichText])lbl.attributedText=baseRich;
                s.ownsRichText=NO;s.appliedRichText=nil;
            }
            CGFloat measuredW=HGAEpisodeSingleLineWidth(lbl,s.ownsRichText ? s.appliedRichText : baseRich,nil);
            inGlass.size.width=MIN(availW,MAX(1.0,measuredW+1.0));
        } else {
            NSString *clippedText=trimmed ? [full substringToIndex:fitLen] : full;
            if(trimmed) {
                s.appliedText=clippedText;s.ownsText=YES;s.ownsRichText=NO;
                if(![lbl.text isEqualToString:clippedText])lbl.text=clippedText;
            } else if(s.ownsText) {
                if([lbl.text isEqualToString:s.appliedText ?: @""])lbl.text=baseText;
                s.ownsText=NO;s.appliedText=nil;
            }
            CGFloat measuredW=HGAEpisodeSingleLineWidth(lbl,nil,clippedText);
            inGlass.size.width=MIN(availW,MAX(1.0,measuredW+1.0));
        }
        s.ownsBreakMode=YES;
        s.appliedBreakMode=NSLineBreakByClipping;
        if(lbl.lineBreakMode!=NSLineBreakByClipping)lbl.lineBreakMode=NSLineBreakByClipping;

        CGRect targetInEntry=CGRectOffset(inGlass,glassFrame.origin.x,glassFrame.origin.y);
        CGRect targetFrame=[lbl.superview convertRect:targetInEntry fromView:entry];
        s.appliedFrame=targetFrame;s.ownsFrame=YES;
        if(!CGRectEqualToRect(lbl.frame,targetFrame))lbl.frame=targetFrame;
        cursorX=CGRectGetMaxX(inGlass)+2.0;
        [active addObject:s];
        [visibleStates addObject:s];
    }

    // Center the visible [icon + whole-character labels] group horizontally inside
    // the capsule so left and right empty margins match identically.
    if(!trailingIcon && visibleStates.count) {
        CGFloat minInk=CGFLOAT_MAX,maxInk=0;
        for(HGAEpisodeForegroundState *s in visibleStates) {
            UIView *v=s.view;
            if([v isKindOfClass:UILabel.class]) {
                UILabel *lbl=(UILabel *)v;
                CGRect tr=[lbl textRectForBounds:lbl.bounds limitedToNumberOfLines:1];
                if(CGRectIsEmpty(tr))tr=lbl.bounds;
                CGRect r=CGRectOffset([lbl convertRect:tr toView:entry],-glassFrame.origin.x,-glassFrame.origin.y);
                minInk=MIN(minInk,CGRectGetMinX(r));maxInk=MAX(maxInk,CGRectGetMaxX(r));
            } else {
                CGRect r=CGRectOffset([v convertRect:v.bounds toView:entry],-glassFrame.origin.x,-glassFrame.origin.y);
                minInk=MIN(minInk,CGRectGetMinX(r));maxInk=MAX(maxInk,CGRectGetMaxX(r));
            }
        }
        if(isfinite(minInk) && maxInk>minInk) {
            CGFloat rightPad=glassW-maxInk;
            CGFloat shift=(rightPad-minInk)*0.5;
            if(fabs(shift)>0.25 && (needsAdjustment || minInk<11.0 || rightPad<11.0 || fabs(rightPad-minInk)>3.0) &&
               minInk+shift>=8.0) {
                for(HGAEpisodeForegroundState *s in visibleStates) {
                    UIView *v=s.view;
                    CGRect base=s.ownsFrame ? s.appliedFrame : s.sourceFrame;
                    CGRect inEntry=[v.superview convertRect:base toView:entry];
                    inEntry.origin.x+=shift;
                    CGRect shifted=[v.superview convertRect:inEntry fromView:entry];
                    s.appliedFrame=shifted;s.ownsFrame=YES;
                    if(!CGRectEqualToRect(v.frame,shifted))v.frame=shifted;
                }
            }
        }
    }

    for(HGAEpisodeForegroundState *old in previous)
        if(![active containsObject:old])HGARestoreEpisodeForegroundItem(old);
    objc_setAssociatedObject(entry,&HGAEpisodeLabelsKey,active,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

static void HGARestoreEpisodeContentWidth(HGAEpisodeSurfaceState *state) {
    UIView *view=state.view;
    if(state.resizedContent && CGRectGetWidth(view.frame)==state.appliedWidth) {
        CGRect frame=view.frame;frame.size.width=state.sourceWidth;view.frame=frame;
    }
    state.resizedContent=NO;
}
static void HGARestoreEpisodeSurface(HGAEpisodeSurfaceState *state) {
    UIView *view=state.view;
    if(!view)return;
    HGARestoreEpisodeContentWidth(state);
    if(state.clearsBackground && [view.backgroundColor isEqual:UIColor.clearColor])
        view.backgroundColor=state.background;
    if(state.clearsLayerBackground && (!view.layer.backgroundColor || CGColorGetAlpha(view.layer.backgroundColor)==0))
        view.layer.backgroundColor=state.layerBackground.CGColor;
    if(state.rounded) {
        if(fabs(view.layer.cornerRadius-state.appliedRadius)<0.001)view.layer.cornerRadius=state.radius;
        if([view.layer.cornerCurve isEqual:kCACornerCurveContinuous])view.layer.cornerCurve=state.curve;
        if(view.layer.maskedCorners==HGAEpisodeAllCorners)view.layer.maskedCorners=state.corners;
        if(view.clipsToBounds)view.clipsToBounds=state.clips;
    }
    state.clearsBackground=NO;state.clearsLayerBackground=NO;state.rounded=NO;
    if(objc_getAssociatedObject(view,&HGAEpisodeWrapperKey)==state)
        objc_setAssociatedObject(view,&HGAEpisodeWrapperKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static void HGAApplyEpisodeSurface(HGAEpisodeSurfaceState *state,CGFloat radius,BOOL clear,BOOL round) {
    UIView *view=state.view;
    if(clear) {
        if(!state.clearsBackground || ![view.backgroundColor isEqual:UIColor.clearColor])
            state.background=view.backgroundColor;
        state.clearsBackground=YES;
        if(![view.backgroundColor isEqual:UIColor.clearColor])view.backgroundColor=UIColor.clearColor;
    }
    CGColorRef layerPaint=view.layer.backgroundColor;
    if(layerPaint && CGColorGetAlpha(layerPaint)>0) {
        state.layerBackground=[UIColor colorWithCGColor:layerPaint];
        state.clearsLayerBackground=YES;
        view.layer.backgroundColor=nil;
    }
    if(!round)return;
    if(!state.rounded || fabs(view.layer.cornerRadius-state.appliedRadius)>0.001)state.radius=view.layer.cornerRadius;
    if(!state.rounded || ![view.layer.cornerCurve isEqual:kCACornerCurveContinuous])state.curve=view.layer.cornerCurve;
    if(!state.rounded || view.layer.maskedCorners!=HGAEpisodeAllCorners)state.corners=view.layer.maskedCorners;
    if(!state.rounded || !view.clipsToBounds)state.clips=view.clipsToBounds;
    state.rounded=YES;state.appliedRadius=radius;
    if(fabs(view.layer.cornerRadius-radius)>0.001)view.layer.cornerRadius=radius;
    if(![view.layer.cornerCurve isEqual:kCACornerCurveContinuous])view.layer.cornerCurve=kCACornerCurveContinuous;
    if(view.layer.maskedCorners!=HGAEpisodeAllCorners)view.layer.maskedCorners=HGAEpisodeAllCorners;
    if(!view.clipsToBounds)view.clipsToBounds=YES;
}
static BOOL HGAEpisodeSameSurface(UIView *candidate,UIView *entry) {
    if(!candidate || !CGAffineTransformIsIdentity(candidate.transform))return NO;
    HGAEpisodeInsetState *inset=objc_getAssociatedObject(entry,&HGAEpisodeInsetKey);
    if(candidate==entry.superview && inset &&
        CGSizeEqualToSize(candidate.bounds.size,inset.sourceBounds.size) &&
        fabs(CGRectGetMidX(candidate.bounds)-inset.sourceCenterX)<0.5 &&
        fabs(CGRectGetMidY(candidate.bounds)-entry.center.y)<0.5)return YES;
    if(!inset && !CGAffineTransformIsIdentity(entry.transform))return NO;
    CGRect rect=[candidate convertRect:candidate.bounds toView:entry];
    if(inset && fabs(rect.size.width-inset.sourceBounds.size.width)<0.5 &&
        fabs(rect.origin.y-entry.bounds.origin.y)<0.5 && fabs(rect.size.height-entry.bounds.size.height)<0.5) {
        CGFloat margin=(inset.sourceBounds.size.width-entry.bounds.size.width)/2;
        // The verified single-child wrapper retains native width. A native
        // content child may still be waiting for its next width layout pass.
        if(fabs(rect.origin.x-entry.bounds.origin.x+margin+inset.appliedCenterX-inset.sourceCenterX)<0.5 ||
            fabs(rect.origin.x-entry.bounds.origin.x)<0.5)return YES;
    }
    return fabs(rect.origin.x-entry.bounds.origin.x)<0.5 &&
        fabs(rect.origin.y-entry.bounds.origin.y)<0.5 &&
        fabs(rect.size.width-entry.bounds.size.width)<0.5 &&
        fabs(rect.size.height-entry.bounds.size.height)<0.5;
}
static void HGAEpisodeWrapperChanged(UIView *wrapper) {
    HGAEpisodeSurfaceState *state=objc_getAssociatedObject(wrapper,&HGAEpisodeWrapperKey);
    if(!state)return;
    UIView *entry=state.owner;
    if(!entry || entry.superview!=wrapper || wrapper.subviews.count!=1 ||
        !HGAEpisodeSameSurface(wrapper,entry)) {
        HGARestoreEpisodeSurface(state);
        [entry setNeedsLayout];
    }
}
static void HGAApplyEpisodeCorners(UIView *entry,BOOL enabled,CGFloat radius) {
    if(!enabled) {
        HGARestoreEpisodeLabels(entry);
        HGARestoreEpisodeInset(entry);
    }
    NSArray<HGAEpisodeSurfaceState *> *previous=objc_getAssociatedObject(entry,&HGAEpisodeSurfacesKey);
    NSMutableArray<UIView *> *surfaces=[NSMutableArray array];
    NSMutableSet<UIView *> *shades=[NSMutableSet set];
    UIView *content=nil;
    HGAEpisodeInsetState *inset=objc_getAssociatedObject(entry,&HGAEpisodeInsetKey);
    if(enabled && entry.window && !CGRectIsEmpty(entry.bounds)) {
        [surfaces addObject:entry];
        for(UIView *child in entry.subviews) {
            if([child isKindOfClass:objc_lookUpClass("SSShortVideoDistributionContentBaseView")] ||
                [child isKindOfClass:objc_lookUpClass("SSShortVideoDistributionContentView")]) {
                content=child;break;
            }
        }
        // Eggplant repaints separate backing plates on entry or content when it
        // renders a new model, sometimes directly via CALayer.backgroundColor.
        UIView *backing=HGAViewGetter(entry,"subBackgroundView",entry);
        if(backing!=entry && object_getClass(backing)==UIView.class && !backing.subviews.count)
            [shades addObject:backing];
        for(UIView *owner in (content ? @[entry,content] : @[entry])) {
            for(UIView *paint in owner.subviews) {
                if(object_getClass(paint)!=UIView.class || paint.subviews.count)continue;
                if(CGRectGetWidth(paint.bounds)>=CGRectGetWidth(entry.bounds)*0.5 && CGRectGetHeight(paint.bounds)>=24)
                    [shades addObject:paint];
            }
        }
        for(UIView *shade in shades)if(![surfaces containsObject:shade])[surfaces addObject:shade];
        if(content)[surfaces addObject:content];
        for(UIView *child in HGADescendants(entry,32)) {
            NSString *name=NSStringFromClass(child.class);
            if(child!=entry && child!=content && ([name isEqual:@"SSShortVideoDistributionContentView"] ||
                [name isEqual:@"SSShortVideoBottomDistributionTextView"]) &&
                HGAEpisodeSameSurface(child,entry))[surfaces addObject:child];
        }
        UIView *wrapper=entry.superview;
        // The diagnostic hierarchy shows one entry inside this exact wrapper.
        // A wrapper with any other content must retain all of its own styling.
        if([NSStringFromClass(wrapper.class) isEqual:@"FQVShortVideoBaseInsertView"] &&
            wrapper.subviews.count==1 && HGAEpisodeSameSurface(wrapper,entry))
            [surfaces addObject:wrapper];
    }
    for(HGAEpisodeSurfaceState *state in previous)
        if(!state.view || ![surfaces containsObject:state.view])HGARestoreEpisodeSurface(state);
    if(!surfaces.count) {
        objc_setAssociatedObject(entry,&HGAEpisodeSurfacesKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        return;
    }
    NSMutableArray<HGAEpisodeSurfaceState *> *current=[NSMutableArray array];
    for(UIView *view in surfaces) {
        HGAEpisodeSurfaceState *state=nil;
        for(HGAEpisodeSurfaceState *candidate in previous)if(candidate.view==view){state=candidate;break;}
        if(!state){state=[HGAEpisodeSurfaceState new];state.view=view;state.owner=entry;}
        if(view==content && inset && CGAffineTransformIsIdentity(content.transform)) {
            // The host can reapply a 440pt-wide content frame under a 297pt
            // entry. Size its native layout (including the trailing arrow)
            // against the same width as the glass and keep the 44pt hit target.
            CGRect frame=content.frame;
            CGFloat width=CGRectGetWidth(frame),target=CGRectGetWidth(entry.bounds);
            if(fabs(CGRectGetMinX(frame)-CGRectGetMinX(entry.bounds))<0.5 && width!=target) {
                if(!state.resizedContent || width!=state.appliedWidth)state.sourceWidth=width;
                state.resizedContent=YES;state.appliedWidth=target;
                frame.size.width=target;content.frame=frame;
            }
        } else if(state.resizedContent)HGARestoreEpisodeContentWidth(state);
        HGAApplyEpisodeSurface(state,radius,view!=entry,![shades containsObject:view]);
        if(view==entry.superview)
            objc_setAssociatedObject(view,&HGAEpisodeWrapperKey,state,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        [current addObject:state];
    }
    objc_setAssociatedObject(entry,&HGAEpisodeSurfacesKey,current,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static char HGAEpisodeDiagKey;
static void HGALogEpisodeGlassState(UIView *view,HGAAppearanceMaterial *glass,NSString *reason) {
    if(!HGEnhancedDiagnostics() || !view)return;
    NSString *effectClass=glass.effect ? NSStringFromClass(glass.effect.class) : @"none";
    NSString *materialId=glass ? [NSString stringWithFormat:@"%p",glass] : @"none";
    NSString *sig=[NSString stringWithFormat:@"%@|%@|%@|%d|%d",reason ?: @"",effectClass,materialId,view.hidden,view.window!=nil];
    NSString *prev=objc_getAssociatedObject(view,&HGAEpisodeDiagKey);
    if([prev isEqual:sig])return;
    objc_setAssociatedObject(view,&HGAEpisodeDiagKey,sig,OBJC_ASSOCIATION_COPY_NONATOMIC);
    HGLog(@"appearance.episode-glass",@{
        @"viewClass":NSStringFromClass(view.class),
        @"superviewClass":view.superview ? NSStringFromClass(view.superview.class) : @"none",
        @"reason":reason ?: @"",
        @"effectClass":effectClass,
        @"materialId":materialId,
        @"materialSignature":glass.materialSignature ?: @"",
        @"hidden":@(view.hidden),
        @"hasWindow":@(view.window!=nil),
        @"bounds":NSStringFromCGRect(view.bounds)
    });
}
static void HGAEpisodeReconcileWrapper(UIView *wrapper) {
    HGAEpisodeWrapperChanged(wrapper);
    if(!wrapper.window || wrapper.subviews.count!=1)return;
    UIView *entry=wrapper.subviews.firstObject;
    if([HGARole(entry) isEqual:@"episodes"])HGAppearanceProcessView(entry);
}
static void HGAEpisodeWrapperWillMove(UIView *wrapper) {
    if(wrapper.subviews.count!=1)return;
    UIView *entry=wrapper.subviews.firstObject;
    if(!objc_getAssociatedObject(entry,&HGAEpisodeInsetKey))return;
    HGAApplyEpisodeCorners(entry,NO,0);
    HGARemoveGlass(entry);
    HGALogEpisodeGlassState(entry,nil,@"wrapper-reparent");
    [entry setNeedsLayout];
}
