#import "AdSceneDiagnostics.h"
#import "Diagnostics.h"
#import <objc/runtime.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static const NSUInteger HGAdNodeLimit = 256, HGAdControllerLimit = 64;
static const NSUInteger HGAdClassLimit = 32, HGAdMethodLimit = 48;
static const NSUInteger HGAdByteLimit = 128 * 1024;
static const NSTimeInterval HGAdTimeBudget = 0.025;

static NSTimeInterval HGAdNow(void) { return NSProcessInfo.processInfo.systemUptime; }
static BOOL HGAdExpired(NSTimeInterval start, NSMutableOrderedSet *reasons) {
    if (HGAdNow() - start < HGAdTimeBudget) return NO;
    [reasons addObject:@"timeLimit"];
    return YES;
}
static BOOL HGAdSubclass(Class type, Class base) {
    for (Class c = type; c; c = class_getSuperclass(c)) if (c == base) return YES;
    return NO;
}
static NSString *HGAdString(const char *value, NSUInteger limit) {
    NSString *text = value ? [NSString stringWithUTF8String:value] : @"";
    if (!text) return @"";
    return text.length <= limit ? text : [text substringToIndex:limit];
}
static NSString *HGAdClassName(Class type) { return HGAdString(class_getName(type), 256); }
static NSArray *HGAdRect(CGRect frame) {
    if (!isfinite(frame.origin.x) || !isfinite(frame.origin.y) ||
        !isfinite(frame.size.width) || !isfinite(frame.size.height)) return @[];
    return @[@(frame.origin.x), @(frame.origin.y), @(frame.size.width), @(frame.size.height)];
}
static NSNumber *HGAdID(NSMapTable *identifiers, id object, NSUInteger *next) {
    if (!object) return @0;
    NSNumber *identifier = [identifiers objectForKey:object];
    if (!identifier) {
        identifier = @(++*next);
        [identifiers setObject:identifier forKey:object];
    }
    return identifier;
}
static BOOL HGAdVisibleInWindow(UIView *view, UIWindow *window, NSMutableOrderedSet *reasons) {
    if (!view || view.window != window) return NO;
    UIView *ancestor = view;
    for (NSUInteger depth = 0; ancestor && depth < 24; depth++, ancestor = ancestor.superview) {
        if (ancestor.hidden || ancestor.alpha <= 0.01) return NO;
        if (ancestor == window) return YES;
    }
    if (ancestor) [reasons addObject:@"ancestorLimit"];
    return NO;
}
static BOOL HGAdNonSystemClass(Class type) {
    if (!type) return NO;
    // Runtime-created subclasses inherit their first known image's ownership.
    for (Class c = type; c; c = class_getSuperclass(c)) {
        const char *image = class_getImageName(c);
        if (!image) continue;
        return !strstr(image, "/System/Library/") && !strstr(image, "/usr/lib/") &&
            !strstr(image, "/Library/Apple/");
    }
    return NO;
}
static void HGAdAddClass(Class type, NSMutableArray *classes, NSHashTable *seen, NSMutableOrderedSet *reasons) {
    if (!HGAdNonSystemClass(type) || [seen containsObject:type]) return;
    if (classes.count >= 64) { [reasons addObject:@"classQueueLimit"]; return; }
    [seen addObject:type];
    [classes addObject:type];
}

NSDictionary *HGAdSceneSnapshot(NSArray<UIWindow *> *windows) {
    if (!NSThread.isMainThread) return nil;
    NSTimeInterval start = HGAdNow();
    NSMutableOrderedSet *reasons = [NSMutableOrderedSet orderedSet];
    NSMapTable *identifiers = [NSMapTable mapTableWithKeyOptions:
        NSPointerFunctionsStrongMemory | NSPointerFunctionsObjectPointerPersonality
        valueOptions:NSPointerFunctionsStrongMemory];
    NSUInteger nextID = 0, skippedWindows = 0, hiddenSubtrees = 0;
    NSMutableArray *visibleWindows = [NSMutableArray array];
    NSHashTable *windowSeen = [NSHashTable hashTableWithOptions:NSPointerFunctionsObjectPointerPersonality];
    for (id object in windows) {
        if (HGAdExpired(start, reasons)) break;
        if (!HGAdSubclass(object_getClass(object), UIWindow.class)) { skippedWindows++; continue; }
        UIWindow *window = object;
        if ([windowSeen containsObject:window]) continue;
        [windowSeen addObject:window];
        UISceneActivationState state = window.windowScene.activationState;
        if (window.hidden || window.alpha <= 0.01 || !window.rootViewController ||
            (window.windowScene && state != UISceneActivationStateForegroundActive &&
                state != UISceneActivationStateForegroundInactive)) {
            skippedWindows++; continue;
        }
        [visibleWindows addObject:window];
    }
    [visibleWindows sortUsingComparator:^NSComparisonResult(UIWindow *a, UIWindow *b) {
        if (a.windowLevel != b.windowLevel) return a.windowLevel > b.windowLevel ? NSOrderedAscending : NSOrderedDescending;
        if (a.isKeyWindow != b.isKeyWindow) return a.isKeyWindow ? NSOrderedAscending : NSOrderedDescending;
        return NSOrderedSame;
    }];
    if (visibleWindows.count > 8) {
        skippedWindows += visibleWindows.count - 8;
        [visibleWindows removeObjectsInRange:NSMakeRange(8, visibleWindows.count - 8)];
        [reasons addObject:@"windowLimit"];
    }

    NSMutableArray *windowRecords = [NSMutableArray array], *controllers = [NSMutableArray array];
    NSMutableArray *nodes = [NSMutableArray array], *classRecords = [NSMutableArray array];
    NSMutableArray *viewQueue = [NSMutableArray array], *classQueue = [NSMutableArray array];
    NSHashTable *viewSeen = [NSHashTable hashTableWithOptions:NSPointerFunctionsObjectPointerPersonality];
    NSHashTable *controllerSeen = [NSHashTable hashTableWithOptions:NSPointerFunctionsObjectPointerPersonality];
    NSHashTable *classSeen = [NSHashTable hashTableWithOptions:NSPointerFunctionsObjectPointerPersonality];
    void (^addView)(UIView *) = ^(UIView *view) {
        if (!view || [viewSeen containsObject:view]) return;
        if (viewQueue.count >= HGAdNodeLimit) { [reasons addObject:@"nodeLimit"]; return; }
        [viewSeen addObject:view];
        [viewQueue addObject:view];
    };
    for (UIWindow *window in visibleWindows) {
        if (HGAdExpired(start, reasons)) break;
        UIViewController *root = window.rootViewController;
        NSNumber *windowID = HGAdID(identifiers, window, &nextID);
        [windowRecords addObject:@{@"id": windowID, @"rootController": HGAdID(identifiers, root, &nextID),
            @"class": HGAdClassName(object_getClass(window)), @"frame": HGAdRect(window.frame),
            @"windowLevel": @(window.windowLevel), @"key": @(window.isKeyWindow),
            @"hidden": @(window.hidden), @"alpha": @(window.alpha)}];
        NSMutableArray *stack = [NSMutableArray arrayWithObject:root];
        NSMutableArray *visibleRoots = [NSMutableArray array];
        NSMutableArray *presentedRoots = [NSMutableArray array];
        while (stack.count && !HGAdExpired(start, reasons)) {
            UIViewController *controller = stack.lastObject;
            [stack removeLastObject];
            if ([controllerSeen containsObject:controller]) continue;
            [controllerSeen addObject:controller];
            if (controllers.count >= HGAdControllerLimit) { [reasons addObject:@"controllerLimit"]; break; }
            UIView *view = controller.viewIfLoaded;
            BOOL visible = HGAdVisibleInWindow(view, window, reasons);
            UIViewController *presented = controller.presentedViewController;
            NSUInteger skippedChildren = 0;
            // LIFO: the presented/popover chain is processed before child content.
            for (UIViewController *child in controller.childViewControllers) {
                if (HGAdExpired(start, reasons)) break;
                // Reserve one slot for the presented chain ahead of child trees.
                if (controllers.count + stack.count + 1 + (presented ? 1 : 0) >= HGAdControllerLimit) {
                    [reasons addObject:@"controllerLimit"]; break;
                }
                UIView *childView = child.viewIfLoaded;
                if (HGAdVisibleInWindow(childView, window, reasons))
                    [stack addObject:child];
                else skippedChildren++;
            }
            if (presented) {
                if (controllers.count + stack.count + 1 < HGAdControllerLimit) [stack addObject:presented];
                else [reasons addObject:@"controllerLimit"];
            }
            [controllers addObject:@{@"id": HGAdID(identifiers, controller, &nextID),
                @"parent": HGAdID(identifiers, controller.parentViewController, &nextID),
                @"presenting": HGAdID(identifiers, controller.presentingViewController, &nextID),
                @"presented": HGAdID(identifiers, presented, &nextID),
                @"class": HGAdClassName(object_getClass(controller)),
                @"view": HGAdID(identifiers, view, &nextID), @"window": windowID,
                @"visible": @(visible), @"childrenSkipped": @(skippedChildren)}];
            if (visible) {
                // Presentation chains are visited outermost first. Only reverse
                // that chain, so its foremost popup precedes all ordinary children.
                if (controller.presentingViewController) [presentedRoots insertObject:view atIndex:0];
                else [visibleRoots addObject:view];
            }
            HGAdAddClass(object_getClass(controller), classQueue, classSeen, reasons);
        }
        // Seed the foremost controller roots and their ancestors before ordinary
        // breadth-first traversal can spend the budget on the underlying player.
        for (UIView *view in [presentedRoots arrayByAddingObjectsFromArray:visibleRoots]) {
            NSUInteger depth = 0;
            for (UIView *ancestor = view; ancestor && depth < 24; ancestor = ancestor.superview, depth++) {
                if (HGAdExpired(start, reasons)) break;
                addView(ancestor);
            }
        }
        addView(window);
    }

    for (NSUInteger index = 0; index < viewQueue.count && !HGAdExpired(start, reasons); index++) {
        UIView *view = viewQueue[index];
        Class type = object_getClass(view);
        UIWindow *window = view.window;
        NSMutableDictionary *node = [@{@"id": HGAdID(identifiers, view, &nextID),
            @"parent": HGAdID(identifiers, view.superview, &nextID),
            @"window": HGAdID(identifiers, window, &nextID), @"class": HGAdClassName(type),
            @"frame": HGAdRect(view.frame), @"hidden": @(view.hidden),
            @"alpha": @(isfinite(view.alpha) ? view.alpha : 0),
            @"childCount": @(view.subviews.count)} mutableCopy];
        BOOL hidden = view.hidden || view.alpha <= 0.01;
        if (hidden) {
            node[@"childrenSkipped"] = @"hidden";
            hiddenSubtrees++;
        } else {
            if (HGAdSubclass(type, UIControl.class)) {
                UIControl *control = (UIControl *)view;
                NSMutableArray *actions = [NSMutableArray array];
                NSUInteger targetCount = 0;
                for (id target in control.allTargets) {
                    if (targetCount++ >= 4 || HGAdExpired(start, reasons)) break;
                    BOOL responderChain = target == NSNull.null;
                    HGAdAddClass(responderChain ? Nil : object_getClass(target), classQueue, classSeen, reasons);
                    // UIKit queries a specific registered event. The synthetic
                    // AllEvents mask returned nil for real touch-up registrations
                    // in the simulator; enumerate actual bits without sending them.
                    UIControlEvents registered = control.allControlEvents;
                    for (NSUInteger bit = 0; bit < sizeof(UIControlEvents) * 8; bit++) {
                        if (actions.count >= 8 || HGAdExpired(start, reasons)) break;
                        UIControlEvents event = (UIControlEvents)((NSUInteger)1 << bit);
                        if (!(registered & event)) continue;
                        NSArray *selectors = [control actionsForTarget:responderChain ? nil : target
                            forControlEvent:event];
                        for (NSString *selector in selectors) {
                            if (actions.count >= 8) break;
                            [actions addObject:@{@"targetClass": responderChain ? @"responder-chain" : HGAdClassName(object_getClass(target)),
                                @"selector": selector.length <= 256 ? selector : [selector substringToIndex:256],
                                @"events": @(event)}];
                        }
                    }
                    if (actions.count >= 8) break;
                }
                if (actions.count) node[@"actions"] = actions;
            }
            // UIKit material internals do not help identify the advertising owner.
            if (HGAdSubclass(type, UIVisualEffectView.class)) node[@"childrenSkipped"] = @"systemEffect";
            else for (UIView *child in view.subviews.reverseObjectEnumerator) {
                if (HGAdExpired(start, reasons)) break;
                if ([viewSeen containsObject:child]) continue;
                if (viewQueue.count >= HGAdNodeLimit) { [reasons addObject:@"nodeLimit"]; break; }
                addView(child);
            }
            HGAdAddClass(type, classQueue, classSeen, reasons);
        }
        [nodes addObject:node];
    }

    for (Class type in classQueue) {
        if (HGAdExpired(start, reasons)) break;
        if (classRecords.count >= HGAdClassLimit) { [reasons addObject:@"classLimit"]; break; }
        NSMutableArray *methods = [NSMutableArray array];
        BOOL methodsTruncated = NO;
        for (NSUInteger kind = 0; kind < 2; kind++) {
            unsigned int count = 0;
            Method *list = class_copyMethodList(kind ? object_getClass(type) : type, &count);
            for (unsigned int i = 0; i < count; i++) {
                if (methods.count >= HGAdMethodLimit || HGAdExpired(start, reasons)) { methodsTruncated = YES; break; }
                [methods addObject:@{@"selector": HGAdString(sel_getName(method_getName(list[i])), 256),
                    @"encoding": HGAdString(method_getTypeEncoding(list[i]), 512),
                    @"kind": kind ? @"class" : @"instance"}];
            }
            free(list);
        }
        if (methodsTruncated) [reasons addObject:@"methodLimit"];
        [classRecords addObject:@{@"name": HGAdClassName(type), @"superclass": HGAdClassName(class_getSuperclass(type)),
            @"methods": methods, @"methodsTruncated": @(methodsTruncated)}];
    }
    NSMutableDictionary *scene = [@{@"kind": @"ad.scene", @"reason": @"user.screenshot",
        @"time": @(NSDate.date.timeIntervalSince1970), @"windows": windowRecords,
        @"controllers": controllers, @"nodes": nodes, @"classes": classRecords,
        @"skippedWindows": @(skippedWindows), @"hiddenSubtrees": @(hiddenSubtrees),
        @"limits": @{@"nodeLimit": @(HGAdNodeLimit), @"controllerLimit": @(HGAdControllerLimit),
            @"classLimit": @(HGAdClassLimit), @"methodLimit": @(HGAdMethodLimit),
            @"timeBudgetMs": @25, @"byteLimit": @(HGAdByteLimit)},
        @"privacy": @"Structure and runtime metadata only; no text, images, accessibility values, URLs or request data."} mutableCopy];
    // Leave headroom for capture timing and bounded explanatory fields. Drop
    // metadata first, then the lowest-priority tail of the captured hierarchy.
    while (YES) {
        HGAdExpired(start, reasons);
        scene[@"captureMs"] = @((HGAdNow() - start) * 1000);
        scene[@"truncated"] = @(reasons.count != 0);
        scene[@"truncationReasons"] = reasons.array;
        NSData *data = [NSJSONSerialization dataWithJSONObject:scene options:0 error:nil];
        if (!data) return nil;
        if (data.length < HGAdByteLimit) break;
        [reasons addObject:@"byteLimit"];
        if (classRecords.count) {
            NSUInteger count = MIN(classRecords.count, 8);
            [classRecords removeObjectsInRange:NSMakeRange(classRecords.count - count, count)];
        } else if (nodes.count) {
            NSUInteger count = MIN(nodes.count, 8);
            [nodes removeObjectsInRange:NSMakeRange(nodes.count - count, count)];
        } else if (controllers.count) {
            NSUInteger count = MIN(controllers.count, 8);
            [controllers removeObjectsInRange:NSMakeRange(controllers.count - count, count)];
        }
        else return nil;
    }
    return scene.copy;
}

static NSArray<UIWindow *> *HGAdForegroundWindows(void) {
    NSMutableArray *windows = [NSMutableArray array];
    UIApplication *application = UIApplication.sharedApplication;
    for (UIScene *scene in application.connectedScenes) {
        if (!HGAdSubclass(object_getClass(scene), UIWindowScene.class) ||
            (scene.activationState != UISceneActivationStateForegroundActive &&
                scene.activationState != UISceneActivationStateForegroundInactive)) continue;
        [windows addObjectsFromArray:((UIWindowScene *)scene).windows];
    }
    // Some supported hosts still use UIApplicationDelegate's legacy lifecycle.
    // There is no UIWindowScene in that configuration; keep this fallback local.
    if (application.connectedScenes.count == 0 && application.applicationState != UIApplicationStateBackground) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        [windows addObjectsFromArray:application.windows];
#pragma clang diagnostic pop
    }
    return windows;
}

void HGInstallAdSceneDiagnostics(void) {
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        [NSNotificationCenter.defaultCenter addObserverForName:UIApplicationUserDidTakeScreenshotNotification
            object:nil queue:nil usingBlock:^(NSNotification *notification) {
                if (!HGEnhancedDiagnostics()) return;
                void (^capture)(void) = ^{
                    if (!HGEnhancedDiagnostics()) return;
                    NSDictionary *scene = HGAdSceneSnapshot(HGAdForegroundWindows());
                    if (scene && [scene[@"windows"] count]) HGStoreMaskScene(scene);
                };
                if (NSThread.isMainThread) capture();
                else dispatch_async(dispatch_get_main_queue(), capture);
            }];
    });
}
