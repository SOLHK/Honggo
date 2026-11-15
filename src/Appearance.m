#import "Appearance.h"
#import "Honggo.h"
#import <CoreText/CoreText.h>
#import <PhotosUI/PhotosUI.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <ImageIO/ImageIO.h>
#import <CommonCrypto/CommonDigest.h>
#import <objc/message.h>
#import <dispatch/dispatch.h>
#include <math.h>
#include <string.h>

// Original Honggo implementation. Adapter names are interoperability facts;
// no reference-project implementation, graphics or fonts are incorporated.
static BOOL HGAApplying;
static NSHashTable<UIView *> *HGAManaged;
static NSHashTable<UIViewController *> *HGAControllers;
static NSHashTable<UIView *> *HGABars;
static void HGARefreshAssets(void);
static void HGAApplyTypography(UIView *root);
static void HGAObserveFeed(UIScrollView *feed);
static void HGARefreshDock(UIView *bar);

UIColor *HGAppearanceColor(NSString *hex, UIColor *fallback) {
    if (hex.length != 7 || ![hex hasPrefix:@"#"]) return fallback;
    unsigned value = 0;
    NSScanner *scanner = [NSScanner scannerWithString:[hex substringFromIndex:1]];
    if (![scanner scanHexInt:&value] || !scanner.isAtEnd) return fallback;
    return [UIColor colorWithRed:((value >> 16) & 255)/255.0
        green:((value >> 8) & 255)/255.0 blue:(value & 255)/255.0 alpha:1];
}
UIVisualEffect *HGAppearanceMaterial(NSString *style) {
    if (UIAccessibilityIsReduceTransparencyEnabled() || [style isEqual:@"solid"]) return nil;
    if (@available(iOS 26.0, *)) {
        if (![style isEqual:@"blur"] && HGNativeGlassConfigured()) {
            UIGlassEffect *glass = [UIGlassEffect effectWithStyle:
                [style isEqual:@"clear"] ? UIGlassEffectStyleClear : UIGlassEffectStyleRegular];
            glass.interactive = NO;
            return glass;
        }
    }
    return [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemMaterial];
}
static BOOL HGAOwnView(UIView *view) {
    return [view.accessibilityIdentifier hasPrefix:@"honggo.appearance."];
}
static NSArray<UIView *> *HGADescendants(UIView *root, NSUInteger budget) {
    if (!root || !budget) return @[];
    NSMutableArray *queue = [NSMutableArray arrayWithObject:root];
    for (NSUInteger i=0; i<queue.count && queue.count<budget; i++) {
        UIView *node = queue[i];
        if (HGAOwnView(node)) continue;
        for (UIView *child in node.subviews) {
            if (queue.count >= budget) break;
            if (!HGAOwnView(child)) [queue addObject:child];
        }
    }
    return queue;
}
static BOOL HGAVisible(UIView *view) {
    if (!view.window || CGRectIsEmpty(view.bounds)) return NO;
    for (UIView *node=view; node; node=node.superview)
        if (node.hidden || node.alpha<0.01) return NO;
    return YES;
}
static BOOL HGABoolGetter(id object, NSString *name, BOOL *value) {
    if (!object) return NO;
    SEL selector=NSSelectorFromString(name);
    Method method=class_getInstanceMethod(object_getClass(object),selector);
    const char *encoding=method ? method_getTypeEncoding(method) : NULL;
    if (!encoding || (strcmp(encoding,"B16@0:8") && strcmp(encoding,"c16@0:8"))) return NO;
    *value=((BOOL (*)(id,SEL))objc_msgSend)(object,selector);
    return YES;
}
static UIView *HGAViewGetter(id object, const char *name, UIView *ancestor) {
    id value=HGReadObjectGetter(object,name);
    return [value isKindOfClass:UIView.class] && (value==ancestor || [value isDescendantOfView:ancestor]) ? value : nil;
}
static void HGARegisterView(UIView *view) {
    if (!HGAManaged) HGAManaged=[NSHashTable weakObjectsHashTable];
    [HGAManaged addObject:view];
}
static UIViewController *HGAOwner(UIView *view) {
    UIResponder *node=view;
    for (NSUInteger i=0; node && i<32; i++,node=node.nextResponder)
        if ([node isKindOfClass:UIViewController.class]) return (id)node;
    return nil;
}
static NSError *HGAAssetError(NSString *message) {
    return [NSError errorWithDomain:@"Honggo.AppearanceAssets" code:1
        userInfo:@{NSLocalizedDescriptionKey:message}];
}
#include "AppearanceAssets.inc"
#include "AppearanceViews.inc"
#include "AppearanceDock.inc"
#include "AppearanceSettings.inc"

// Exact adapters only. UIKit base classes and arbitrary third-party classes
// never receive a global appearance hook. Missing adapters remain untouched.
static NSMutableDictionary<NSString *,NSValue *> *HGAHooks;
static void HGAHook(Class cls, SEL selector, const char *encoding, BOOL controller) {
    NSString *key=[NSString stringWithFormat:@"%@/%@",NSStringFromClass(cls),NSStringFromSelector(selector)];
    if (HGAHooks[key]) return;
    __block IMP original=NULL;
    IMP replacement=NULL;
    if (!strcmp(encoding,"v16@0:8")) {
        replacement=imp_implementationWithBlock(^(id object) {
            if (!controller && selector==@selector(layoutSubviews) && NSThread.isMainThread && !HGAApplying) HGARestoreGeometry(object);
            ((void(*)(id,SEL))original)(object,selector);
            if (controller) HGAppearanceProcessController(object); else HGAppearanceProcessView(object);
        });
    } else if (!strcmp(encoding,"v24@0:8@16")) {
        replacement=imp_implementationWithBlock(^(id object,id argument) {
            ((void(*)(id,SEL,id))original)(object,selector,argument);
            HGAppearanceProcessView(object);
        });
    } else if (!strcmp(encoding,"v28@0:8@16B24")) {
        replacement=imp_implementationWithBlock(^(id object,id argument,BOOL animated) {
            ((void(*)(id,SEL,id,BOOL))original)(object,selector,argument,animated);
            HGAppearanceProcessView(object);
        });
    } else if (!strcmp(encoding,"v20@0:8B16")) {
        replacement=imp_implementationWithBlock(^(id object,BOOL value) {
            ((void(*)(id,SEL,BOOL))original)(object,selector,value);
            HGAppearanceProcessView(object);
        });
    }
    if (!replacement) return;
    original=HGInstallVerifiedHook(cls,selector,encoding,replacement,@"appearance");
    if (original) HGAHooks[key]=[NSValue valueWithPointer:replacement];
    else imp_removeBlock(replacement);
}
void HGAppearanceRefresh(void) {
    if (!NSThread.isMainThread) { dispatch_async(dispatch_get_main_queue(), ^{HGAppearanceRefresh();}); return; }
    HGARefreshAssets();
    if (!HGAppearanceEnabled() || !HGAppearanceFlag(@"fonts.enabled")) for (UIView *text in HGATexts.allObjects) HGARestoreText(text);
    for (UIView *view in HGAManaged.allObjects) HGAppearanceProcessView(view);
    for (UIViewController *controller in HGAControllers.allObjects) HGAppearanceProcessController(controller);
    for (UIView *bar in HGABars.allObjects) { HGARefreshDock(bar); [bar setNeedsLayout]; }
}
void HGInstallAppearance(void) {
    @synchronized(NSClassFromString(@"HGSettingsController")) {
        if (!HGAHooks) HGAHooks=[NSMutableDictionary dictionary];
        NSUInteger found=0;
        NSMutableOrderedSet *classes=[NSMutableOrderedSet orderedSetWithArray:HGAViewRoles().allKeys];
        [classes addObjectsFromArray:@[@"FQVShortVideoPlayerCellView",@"TTPlayerView",@"FQVCollectionView",@"SSTabBar"]];
        for (NSString *name in classes) {
            Class cls=NSClassFromString(name);
            if (![cls isSubclassOfClass:UIView.class]) {
                HGRecordCapability([@"appearance.adapter." stringByAppendingString:name],
                    @{ @"module":@"appearance",@"state":@"missing-class",@"class":name });
                continue;
            }
            found++;
            HGAHook(cls,@selector(layoutSubviews),"v16@0:8",NO);
            HGAHook(cls,@selector(didMoveToWindow),"v16@0:8",NO);
            if ([name hasPrefix:@"SSShortVideoRightInteraction"]) {
                NSArray *names=@[@"setModel:",@"refreshUIWithModel:",@"refreshUIWithModel:animated:",@"setIsCollected:"];
                const char *types[]={"v24@0:8@16","v24@0:8@16","v28@0:8@16B24","v20@0:8B16"};
                for(NSUInteger i=0;i<names.count;i++) {
                    SEL sel=NSSelectorFromString(names[i]);
                    if (class_getInstanceMethod(cls,sel)) HGAHook(cls,sel,types[i],NO);
                }
            }
        }
        for (NSString *name in HGAPanelControllers()) {
            Class cls=NSClassFromString(name);
            if (![cls isSubclassOfClass:UIViewController.class]) continue;
            HGAHook(cls,@selector(viewDidLayoutSubviews),"v16@0:8",YES);
        }
        // The original navigation is still owned by the host controller.
        Class dockController=NSClassFromString(@"SSTabBarController");
        if ([dockController isSubclassOfClass:UIViewController.class])
            HGAHook(dockController,@selector(viewDidLayoutSubviews),"v16@0:8",YES);
        static dispatch_once_t once;
        dispatch_once(&once, ^{
            dispatch_async(dispatch_get_main_queue(), ^{HGAppearanceRefresh();});
            NSArray *notifications=@[HGAppearanceDidChangeNotification,
                UIAccessibilityReduceTransparencyStatusDidChangeNotification,
                UIAccessibilityReduceMotionStatusDidChangeNotification,
                UIAccessibilityVoiceOverStatusDidChangeNotification];
            for (NSString *name in notifications)
                [NSNotificationCenter.defaultCenter addObserverForName:name object:nil
                    queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification *note) { HGAppearanceRefresh(); }];
        });
        HGRecordCapability(@"module.appearance",@{@"module":@"appearance",
            @"state":HGAppearanceEnabled() ? (found ? @"partial":@"unavailable") : @"disabled",
            @"available":@(found),@"reason":@"scoped visual adapters; device behavior not verified"});
    }
}
