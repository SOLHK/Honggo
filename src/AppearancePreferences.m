#import "AppearancePreferences.h"
#import <CoreFoundation/CoreFoundation.h>
#import <dispatch/dispatch.h>
#include <math.h>

NSString *const HGAppearanceDidChangeNotification = @"Honggo.Appearance.changed";
static NSString *const HGAStore = @"Honggo.Appearance.v1";
static NSDictionary *HGACachedValues;
static NSObject *HGALock(void) { static NSObject *lock; static dispatch_once_t once;
    dispatch_once(&once, ^{ lock = [NSObject new];
        [NSNotificationCenter.defaultCenter addObserverForName:NSUserDefaultsDidChangeNotification
            object:NSUserDefaults.standardUserDefaults queue:nil usingBlock:^(NSNotification *note) {
                @synchronized(lock) { HGACachedValues=nil; }
            }];
    }); return lock; }
NSArray *HGAppearanceGroups(void) { return @[@"general", @"dock", @"player", @"panels", @"icons", @"fonts"]; }
NSArray *HGAppearanceGroupTitles(void) { return @[@"总开关与预设", @"悬浮底栏", @"播放布局与玻璃", @"弹出面板", @"动作图标", @"字体与字号"]; }
NSArray *HGAppearanceRoles(void) { return @[@"avatar", @"collect", @"comment", @"like", @"share", @"more", @"channels", @"info", @"tags", @"progress", @"danmaku", @"episodes", @"notice"]; }
NSArray *HGAppearanceRoleTitles(void) { return @[@"作者头像", @"收藏", @"评论", @"点赞", @"分享", @"更多", @"顶部频道", @"作者与剧集信息", @"分类标签", @"进度条", @"弹幕入口", @"观看全集入口", @"内容声明"]; }
static NSDictionary *HGADef(NSString *key, NSString *group, NSString *title, NSString *kind, id value, NSDictionary *extra) {
    NSMutableDictionary *d = [@{@"key":key, @"group":group, @"title":title, @"kind":kind, @"default":value} mutableCopy];
    if (extra) [d addEntriesFromDictionary:extra]; return d.copy;
}
NSArray<NSDictionary *> *HGAppearanceCatalog(void) {
    static NSArray *catalog; static dispatch_once_t once;
    dispatch_once(&once, ^{
        NSMutableArray *a = [NSMutableArray array];
        void (^flag)(NSString *,NSString *,NSString *) = ^(NSString *key, NSString *group, NSString *title) {
            [a addObject:HGADef(key,group,title,@"bool",@NO,nil)]; };
        void (^number)(NSString *,NSString *,NSString *,double,double,double) = ^(NSString *key, NSString *group, NSString *title, double value, double low, double high) {
            [a addObject:HGADef(key,group,title,@"number",@(value),@{@"min":@(low),@"max":@(high)})]; };
        void (^choice)(NSString *,NSString *,NSString *,NSArray *,NSArray *) = ^(NSString *key, NSString *group, NSString *title, NSArray *values, NSArray *titles) {
            [a addObject:HGADef(key,group,title,@"choice",values.firstObject,@{@"choices":values,@"labels":titles})]; };
        flag(@"enabled",@"general",@"启用外观定制");
        flag(@"dock.enabled",@"dock",@"使用可定制悬浮底栏");
        choice(@"dock.material",@"dock",@"底栏材质",@[@"regular",@"clear",@"blur",@"solid"],@[@"普通液态玻璃",@"清透液态玻璃",@"系统磨砂",@"纯色"]);
        choice(@"dock.content",@"dock",@"底栏显示",@[@"both",@"text",@"icon"],@[@"图标和文字",@"仅文字",@"仅图标"]);
        number(@"dock.width",@"dock",@"底栏宽度比例",0.94,0.55,1);
        number(@"dock.height",@"dock",@"底栏高度",52,44,80);
        number(@"dock.lift",@"dock",@"底栏上移距离",0,0,24);
        number(@"dock.radius",@"dock",@"底栏圆角",26,8,40);
        number(@"dock.iconSize",@"dock",@"底栏图标大小",21,16,30);
        for (NSString *field in @[@"tint",@"selected",@"normal"]) {
            NSString *title = [@{@"tint":@"底栏底色",@"selected":@"选中颜色",@"normal":@"未选中颜色"} objectForKey:field];
            [a addObject:HGADef([@"dock." stringByAppendingString:field],@"dock",title,@"color",@"",nil)];
        }
        flag(@"dock.haptics",@"dock",@"导航触感反馈");
        flag(@"dock.collapse",@"dock",@"开启上滑收缩／下滑展开");
        flag(@"dock.autoCollapse",@"dock",@"跟随播放器滑动收缩");
        choice(@"dock.anchor",@"dock",@"收缩位置",@[@"right",@"left",@"center"],@[@"右侧",@"左侧",@"中间"]);
        flag(@"dock.extra",@"dock",@"显示独立圆形按钮");
        number(@"dock.extraSize",@"dock",@"独立按钮直径",48,44,64);
        choice(@"dock.action",@"dock",@"独立按钮操作",@[@"settings",@"search",@"home",@"theater",@"store",@"mine"],@[@"Honggo 设置",@"搜索",@"首页",@"剧场",@"商城",@"我的"]);
        NSArray *tabIDs = @[@"home",@"theater",@"store",@"mine"];
        NSArray *tabNames = @[@"首页",@"剧场",@"商城",@"我的"];
        for (NSUInteger i=0; i<tabIDs.count; i++) flag([@"dock.hide." stringByAppendingString:tabIDs[i]],@"dock",[@"隐藏" stringByAppendingString:tabNames[i]]);
        flag(@"player.rightGlass",@"player",@"右侧动作按钮玻璃");
        flag(@"player.headerGlass",@"player",@"顶部频道玻璃");
        flag(@"player.episodeGlass",@"player",@"全集入口玻璃");
        flag(@"player.fullscreen",@"player",@"横屏铺满");
        choice(@"player.separateProgress",@"player",@"合集与进度条分开布局",@[@"show",@"hide"],@[@"自动留出间距",@"原始布局"]);
        choice(@"player.material",@"player",@"播放控件材质",@[@"regular",@"clear",@"blur"],@[@"普通液态玻璃",@"清透液态玻璃",@"系统磨砂"]);
        NSArray *roles = HGAppearanceRoles(), *titles = HGAppearanceRoleTitles();
        for (NSUInteger i=0; i<roles.count; i++) {
            NSString *prefix = [@"layout." stringByAppendingString:roles[i]];
            flag([prefix stringByAppendingString:@".hidden"],@"player",[@"隐藏" stringByAppendingString:titles[i]]);
            number([prefix stringByAppendingString:@".x"],@"player",[titles[i] stringByAppendingString:@" · 水平偏移"],0,-160,160);
            number([prefix stringByAppendingString:@".y"],@"player",[titles[i] stringByAppendingString:@" · 垂直偏移"],0,-240,240);
            number([prefix stringByAppendingString:@".scale"],@"player",[titles[i] stringByAppendingString:@" · 大小比例"],1,0.65,1.5);
        }
        for (NSString *kind in @[@"comment",@"share",@"longpress",@"speed"]) {
            NSString *title = [@{@"comment":@"评论面板玻璃",@"share":@"分享面板玻璃",@"longpress":@"长按／更多面板玻璃",@"speed":@"倍速面板玻璃"} objectForKey:kind];
            flag([@"panels." stringByAppendingString:kind],@"panels",title);
        }
        choice(@"panels.material",@"panels",@"面板材质",@[@"regular",@"clear",@"blur"],@[@"普通液态玻璃",@"清透液态玻璃",@"系统磨砂"]);
        number(@"panels.opacity",@"panels",@"面板底色不透明度",0.28,0.08,0.85);
        flag(@"icons.enabled",@"icons",@"使用自定义动作图标");
        choice(@"icons.preset",@"icons",@"图标预设",@[@"outline",@"filled",@"circle",@"square"],@[@"线性",@"实心",@"圆形",@"方形"]);
        number(@"icons.size",@"icons",@"动作图标大小",26,20,48);
        for (NSString *role in @[@"like",@"collect",@"comment",@"share"]) {
            NSString *title = [HGAppearanceRoleTitles() objectAtIndex:[HGAppearanceRoles() indexOfObject:role]];
            for (NSString *state in @[@"normal",@"selected"]) {
                NSString *base = [NSString stringWithFormat:@"icons.%@.%@",role,state];
                NSString *caption = [NSString stringWithFormat:@"%@%@",title,[state isEqual:@"selected"] ? @" · 选中" : @" · 普通"];
                [a addObject:HGADef([base stringByAppendingString:@".symbol"],@"icons",[caption stringByAppendingString:@" SF Symbol"],@"symbol",@"",nil)];
                [a addObject:HGADef([base stringByAppendingString:@".asset"],@"icons",[caption stringByAppendingString:@" 自选图片"],@"image",@"",nil)];
            }
        }
        flag(@"fonts.enabled",@"fonts",@"启用原生文字字体与缩放");
        number(@"fonts.scale",@"fonts",@"原生文字字号比例",1,0.5,1.3);
        [a addObject:HGADef(@"fonts.name",@"fonts",@"选择已安装字体",@"font",@"",nil)];
        [a addObject:HGADef(@"fonts.asset",@"fonts",@"导入 TTF／OTF／TTC 字体",@"fontfile",@"",nil)];
        catalog = a.copy;
    });
    return catalog;
}
static NSDictionary *HGADefinition(NSString *key) {
    static NSDictionary *map; static dispatch_once_t once;
    dispatch_once(&once, ^{ NSMutableDictionary *d=[NSMutableDictionary dictionary];
        for(NSDictionary *item in HGAppearanceCatalog()) d[item[@"key"]]=item; map=d.copy; });
    return [key isKindOfClass:NSString.class] ? map[key] : nil;
}
static BOOL HGAValid(NSDictionary *def,id value) {
    NSString *kind=def[@"kind"]; if(!def || !value) return NO;
    if([kind isEqual:@"bool"]) return [value isKindOfClass:NSNumber.class] && CFGetTypeID((__bridge CFTypeRef)value)==CFBooleanGetTypeID();
    if([kind isEqual:@"number"]) return [value isKindOfClass:NSNumber.class] && CFGetTypeID((__bridge CFTypeRef)value)!=CFBooleanGetTypeID() && isfinite([value doubleValue]) && [value doubleValue]>=[def[@"min"] doubleValue] && [value doubleValue]<=[def[@"max"] doubleValue];
    if(![value isKindOfClass:NSString.class] || [value length]>128) return NO;
    if([kind isEqual:@"choice"]) return [def[@"choices"] containsObject:value];
    if(![value length]) return YES;
    NSString *pattern = [kind isEqual:@"color"] ? @"^#[0-9A-Fa-f]{6}$" :
        ([kind isEqual:@"image"] ? @"^[a-f0-9]{64}\\.png$" :
        ([kind isEqual:@"fontfile"] ? @"^[a-f0-9]{64}\\.(ttf|otf|ttc)$" :
        ([kind isEqual:@"symbol"] ? @"^[A-Za-z0-9.]+$" : @"^[^\\x00-\\x1F/\\\\]{1,128}$")));
    return [value rangeOfString:pattern options:NSRegularExpressionSearch].location!=NSNotFound;
}
NSDictionary *HGAppearanceValues(void) {
    @synchronized(HGALock()) {
        if (HGACachedValues) return HGACachedValues;
        NSDictionary *stored=[NSUserDefaults.standardUserDefaults dictionaryForKey:HGAStore];
        NSMutableDictionary *values=[NSMutableDictionary dictionary];
        for(NSDictionary *def in HGAppearanceCatalog()) { id value=stored[def[@"key"]];
            values[def[@"key"]]=HGAValid(def,value)?value:def[@"default"]; }
        HGACachedValues=values.copy; return HGACachedValues;
    }
}
id HGAppearanceValue(NSString *key) {
    NSDictionary *def=HGADefinition(key); if(!def) return nil;
    return HGAppearanceValues()[key];
}
BOOL HGAppearanceFlag(NSString *key){ return [HGAppearanceValue(key) boolValue]; }
double HGAppearanceNumber(NSString *key){ return [HGAppearanceValue(key) doubleValue]; }
NSString *HGAppearanceString(NSString *key){ id v=HGAppearanceValue(key); return [v isKindOfClass:NSString.class]?v:@""; }
static BOOL HGAError(NSError **error,NSString *message) { if(error)*error=[NSError errorWithDomain:@"Honggo.Appearance" code:1 userInfo:@{NSLocalizedDescriptionKey:message}]; return NO; }
BOOL HGAppearanceApply(NSDictionary *values,NSError **error) {
    if(![values isKindOfClass:NSDictionary.class] || values.count==0 || values.count>HGAppearanceCatalog().count) return HGAError(error,@"外观配置内容无效。");
    NSDictionary *snapshot=values.copy;
    for(id key in snapshot) if(!HGAValid(HGADefinition(key),snapshot[key])) return HGAError(error,@"存在未知设置、错误类型或超出范围的数值。");
    @synchronized(HGALock()) {
        NSMutableDictionary *merged=[HGAppearanceValues() mutableCopy]; [merged addEntriesFromDictionary:snapshot];
        if([merged[@"dock.hide.home"] boolValue] && [merged[@"dock.hide.theater"] boolValue] && [merged[@"dock.hide.store"] boolValue] && [merged[@"dock.hide.mine"] boolValue]) return HGAError(error,@"至少保留一个底栏导航项目。");
        // One persisted dictionary, after complete validation: no partial imports.
        [NSUserDefaults.standardUserDefaults setObject:merged.copy forKey:HGAStore];
        HGACachedValues=merged.copy;
    }
    void (^notify)(void)=^{ [NSNotificationCenter.defaultCenter postNotificationName:HGAppearanceDidChangeNotification object:nil]; };
    if(NSThread.isMainThread) notify(); else dispatch_async(dispatch_get_main_queue(),notify);
    return YES;
}
BOOL HGAppearanceSet(NSString *key,id value,NSError **error) { if(!key || !value)return HGAError(error,@"设置不能为空。"); return HGAppearanceApply(@{key:value},error); }
void HGAppearanceReset(void) { NSMutableDictionary *d=[NSMutableDictionary dictionary]; for(NSDictionary *def in HGAppearanceCatalog()) d[def[@"key"]]=def[@"default"]; HGAppearanceApply(d,nil); }
BOOL HGAppearanceEnabled(void){return HGAppearanceFlag(@"enabled");}
BOOL HGAppearanceNeedsGlass(void) { if(!HGAppearanceEnabled())return NO; for(NSString *key in @[@"dock.enabled",@"player.rightGlass",@"player.headerGlass",@"player.episodeGlass",@"panels.comment",@"panels.share",@"panels.longpress",@"panels.speed"])if(HGAppearanceFlag(key))return YES; return NO; }
NSDictionary *HGAppearancePreset(NSString *name) {
    NSMutableDictionary *d=[NSMutableDictionary dictionary]; for(NSDictionary *def in HGAppearanceCatalog()) d[def[@"key"]]=def[@"default"];
    if([name isEqual:@"glass"] || [name isEqual:@"focus"]) {
        for(NSString *key in @[@"enabled",@"dock.enabled",@"dock.extra",@"dock.haptics",@"player.rightGlass",@"player.headerGlass",@"player.episodeGlass",@"panels.comment",@"panels.share",@"panels.longpress",@"panels.speed",@"icons.enabled"])d[key]=@YES;
        if([name isEqual:@"focus"]) {d[@"dock.collapse"]=@YES; d[@"dock.autoCollapse"]=@YES; for(NSString *role in @[@"avatar",@"tags",@"notice"])d[[NSString stringWithFormat:@"layout.%@.hidden",role]]=@YES; }
    }
    return d.copy;
}
NSData *HGAppearanceExport(NSError **error) { return [NSJSONSerialization dataWithJSONObject:@{@"format":@"Honggo.appearance",@"schema":@1,@"values":HGAppearanceValues()} options:NSJSONWritingPrettyPrinted|NSJSONWritingSortedKeys error:error]; }
NSDictionary *HGAppearanceParse(NSData *data,NSError **error) {
    if(![data isKindOfClass:NSData.class] || !data.length || data.length>128*1024){HGAError(error,@"请选择不超过 128 KiB 的外观 JSON。");return nil;}
    id root=[NSJSONSerialization JSONObjectWithData:data options:0 error:error];
    if(![root isKindOfClass:NSDictionary.class] || [root count]!=3 || ![root[@"format"] isEqual:@"Honggo.appearance"] || ![root[@"schema"] isKindOfClass:NSNumber.class] || CFGetTypeID((__bridge CFTypeRef)root[@"schema"])==CFBooleanGetTypeID() || [root[@"schema"] doubleValue]!=1 || ![root[@"values"] isKindOfClass:NSDictionary.class] || ![root[@"values"] count]){HGAError(error,@"不是受支持的 Honggo 外观配置。");return nil;}
    NSMutableDictionary *d=[root[@"values"] mutableCopy];
    // R13 backups contain the retired on-screen button preference. Accept only
    // its former valid values, then discard it; it must not recreate a control.
    id oldFillButton=d[@"player.fillButton"];
    if(oldFillButton) {
        if(![oldFillButton isKindOfClass:NSString.class] ||
            ![@[@"show",@"hide"] containsObject:oldFillButton]) {
            HGAError(error,@"文件包含未知或无效的外观参数。");return nil;
        }
        [d removeObjectForKey:@"player.fillButton"];
    }
    if(!d.count){HGAError(error,@"文件未包含可用的外观参数。");return nil;}
    for(id key in d) if(!HGAValid(HGADefinition(key),d[key])){HGAError(error,@"文件包含未知或无效的外观参数。");return nil;}
    return d.copy;
}
