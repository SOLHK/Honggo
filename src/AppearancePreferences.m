#import "AppearancePreferences.h"
#import <CoreFoundation/CoreFoundation.h>
#import <dispatch/dispatch.h>
#include <math.h>

NSString *const HGAppearanceDidChangeNotification = @"Honggo.Appearance.changed";
NSString *const HGVideoBlockedKeywordsDidChangeNotification = @"Honggo.VideoKeywords.changed";
static NSString *const HGAStore = @"Honggo.Appearance.v1";
static NSString *const HGAR15GeometryMigration = @"Honggo.Appearance.r15GeometryMigrated";
static NSString *const HGADockWidthMigration = @"Honggo.Appearance.dockWidthMigrated";
static NSString *const HGAR20GeometryMigration = @"Honggo.Appearance.r20GeometryMigrated";
static NSString *const HGABalancedDockMigration = @"Honggo.Appearance.balancedDockMigrated";
const double HGADockDefaultWidth = 0.90;
static const double HGADockDefaultHeight = 58;
static const double HGADockDefaultIconSize = 24;
static const double HGADockDefaultExtraSize = 54;
static NSDictionary *HGACachedValues;
static NSObject *HGALock(void) { static NSObject *lock; static dispatch_once_t once;
    dispatch_once(&once, ^{ lock = [NSObject new];
        [NSNotificationCenter.defaultCenter addObserverForName:NSUserDefaultsDidChangeNotification
            object:NSUserDefaults.standardUserDefaults queue:nil usingBlock:^(NSNotification *note) {
                @synchronized(lock) { HGACachedValues=nil; }
            }];
    }); return lock; }
NSArray *HGAppearanceGroups(void) { return @[@"general",@"dock",@"chrome",@"mine",@"player",@"gestures",@"panels",@"fonts",@"drama"]; }
NSArray *HGAppearanceGroupTitles(void) { return @[@"外观总开关",@"悬浮底栏 Dock",@"顶部导航与找剧",@"我的页面",@"播放布局与控件",@"播放手势",@"弹出面板",@"字体与字号",@"短剧"]; }
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
        void (^buttonStyle)(NSString *,NSString *,NSString *,NSString *) = ^(NSString *key, NSString *group, NSString *title, NSString *initial) {
            [a addObject:HGADef(key,group,title,@"choice",initial,
                @{@"choices":@[@"flat",@"glass"], @"labels":@[@"平面",@"液态玻璃"]})];
        };
        flag(@"enabled",@"general",@"启用外观定制");
        flag(@"dock.enabled",@"dock",@"使用可定制悬浮底栏");
        buttonStyle(@"dock.style",@"dock",@"底栏按钮风格",@"glass");
        choice(@"dock.selectionStyle",@"dock",@"底栏选中效果",@[@"glass",@"plain"],@[@"液态玻璃选中",@"普通选中"]);
        choice(@"dock.material",@"dock",@"底栏材质",@[@"regular",@"clear",@"blur",@"solid"],@[@"普通液态玻璃",@"清透液态玻璃",@"经典磨砂（iOS 18）",@"纯色"]);
        choice(@"dock.content",@"dock",@"底栏显示",@[@"both",@"text",@"icon"],@[@"图标和文字",@"仅文字",@"仅图标"]);
        number(@"dock.width",@"dock",@"底栏宽度比例",HGADockDefaultWidth,0.55,1);
        number(@"dock.height",@"dock",@"底栏高度",HGADockDefaultHeight,44,80);
        number(@"dock.lift",@"dock",@"底栏上移距离",0,0,24);
        number(@"dock.radius",@"dock",@"底栏圆角",HGADockDefaultHeight/2,8,40);
        number(@"dock.iconSize",@"dock",@"底栏图标大小",HGADockDefaultIconSize,16,30);
        flag(@"dock.haptics",@"dock",@"导航触感反馈");
        flag(@"dock.collapse",@"dock",@"开启上滑收缩／下滑展开");
        flag(@"dock.autoCollapse",@"dock",@"跟随播放器滑动收缩");
        choice(@"dock.anchor",@"dock",@"收缩位置",@[@"right",@"left",@"center"],@[@"右侧",@"左侧",@"中间"]);
        flag(@"dock.extra",@"dock",@"显示独立圆形按钮");
        number(@"dock.extraSize",@"dock",@"独立按钮直径",HGADockDefaultExtraSize,44,80);
        choice(@"dock.action",@"dock",@"独立按钮操作",@[@"settings",@"search",@"home",@"theater",@"store",@"mine"],@[@"Honggo 设置",@"搜索",@"首页",@"剧场",@"商城",@"我的"]);
        NSArray *tabIDs = @[@"home",@"theater",@"store",@"mine"];
        NSArray *tabNames = @[@"首页",@"剧场",@"商城",@"我的"];
        for (NSUInteger i=0; i<tabIDs.count; i++) flag([@"dock.hide." stringByAppendingString:tabIDs[i]],@"dock",[@"隐藏" stringByAppendingString:tabNames[i]]);
        flag(@"player.rightGlass",@"player",@"自定义播放动作按钮");
        buttonStyle(@"player.actionStyle",@"player",@"播放动作按钮风格",@"glass");
        choice(@"player.collectGlyph",@"player",@"收藏按钮图标",@[@"star",@"sparkles"],@[@"五角星",@"闪光星"]);
        flag(@"player.headerGlass",@"chrome",@"自定义顶部按钮");
        buttonStyle(@"player.headerStyle",@"chrome",@"顶部按钮风格",@"flat");
        choice(@"player.headerSelectionStyle",@"chrome",@"顶部频道选中效果",@[@"plain",@"glass"],@[@"普通选中",@"液态玻璃选中"]);
        NSArray *channelIDs=@[@"sidebar",@"follow",@"comic",@"live",@"recommend",@"search"];
        NSArray *channelNames=@[@"菜单",@"关注",@"漫剧",@"真人剧",@"推荐",@"搜索"];
        for(NSUInteger i=0;i<channelIDs.count;i++)
            flag([@"layout.channels.hide." stringByAppendingString:channelIDs[i]],@"chrome",
                [@"隐藏顶部" stringByAppendingString:channelNames[i]]);
        [a addObject:HGADef(@"player.finderGlass",@"chrome",@"找剧搜索与快捷入口玻璃",@"bool",@YES,nil)];
        [a addObject:HGADef(@"player.finderAI",@"chrome",@"显示找剧 AI 入口",@"bool",@YES,nil)];
        [a addObject:HGADef(@"player.mineGlass",@"mine",@"我的页面控件玻璃",@"bool",@YES,nil)];
        flag(@"finder.hide.category.row",@"chrome",@"隐藏整个找剧频道栏");
        NSArray *finderTabs=@[@"dramas",@"live",@"community",@"comics",@"films",@"audio",@"novels",@"manga",@"long"];
        NSArray *finderNames=@[@"找剧",@"真人剧",@"社区",@"漫剧",@"电影",@"听书",@"小说",@"漫画",@"长视频"];
        for(NSUInteger i=0;i<finderTabs.count;i++)
            flag([@"finder.hide.category." stringByAppendingString:finderTabs[i]],@"chrome",
                [@"隐藏" stringByAppendingString:finderNames[i]]);
        NSArray *finderShortcuts=@[@"filter",@"ranking",@"new",@"reserve"];
        NSArray *finderShortcutNames=@[@"筛选",@"排行榜",@"新剧",@"预约"];
        for(NSUInteger i=0;i<finderShortcuts.count;i++)
            flag([@"finder.hide.shortcut." stringByAppendingString:finderShortcuts[i]],@"chrome",
                [@"隐藏" stringByAppendingString:finderShortcutNames[i]]);
        flag(@"mine.hide.shortcut.row",@"mine",@"隐藏整行快捷入口");
        [a addObject:HGADef(@"mine.show.publish",@"mine",@"显示悬浮发布按钮",@"bool",@YES,
            @{@"keywords":@"我的 作品 写剧评 加号 ＋ 发布"})];
        [a addObject:HGADef(@"mine.show.theme",@"mine",@"显示日夜模式按钮",@"bool",@YES,
            @{@"keywords":@"我的 月亮 太阳 深色模式 夜间模式 主题"})];
        NSArray *mineShortcuts=@[@"store",@"messages",@"games",@"updates",@"more"];
        NSArray *mineShortcutNames=@[@"订单／商城",@"消息",@"游戏",@"追更",@"更多功能"];
        for(NSUInteger i=0;i<mineShortcuts.count;i++)
            flag([@"mine.hide.shortcut." stringByAppendingString:mineShortcuts[i]],@"mine",
                [@"隐藏" stringByAppendingString:mineShortcutNames[i]]);
        [a addObject:HGADef(@"gestures.pinchClean",@"gestures",@"双指展开清屏、收缩恢复",@"bool",@NO,
            @{@"keywords":@"双指清屏 捏合恢复 双指放大 双指缩小"})];
        [a addObject:HGADef(@"gestures.cleanWhilePlaying",@"gestures",@"播放时自动清屏",@"bool",@NO,
            @{@"keywords":@"默认清屏 沉浸观看 暂停显示控件"})];
        [a addObject:HGADef(@"gestures.doubleTapComments",@"gestures",@"双击短剧画面打开评论区",@"bool",@NO,
            @{@"keywords":@"双击评论 双击打开评论"})];
        [a addObject:HGADef(@"player.episodeGlass",@"player",@"全集入口玻璃",@"bool",@YES,nil)];
        [a addObject:HGADef(@"player.episodeDockWidth",@"player",@"全集入口与 Dock 等宽",@"bool",@NO,
            @{@"keywords":@"观看完整短剧 观看完整剧集 合集 液态玻璃 底栏 长度 宽度"})];
        [a addObject:HGADef(@"player.controlsGlass",@"player",@"返回、倍速、弹幕和全屏按钮玻璃",@"bool",@YES,nil)];
        flag(@"player.hide.followRank",@"player",@"隐藏追更榜按钮");
        flag(@"player.hide.seriesBadge",@"player",@"隐藏系列剧按钮");
        flag(@"player.hideAIFictionNotice",@"player",@"隐藏 AI 剧情虚构提示");
        [a addObject:HGADef(@"video.blockedKeywords",@"drama",@"屏蔽视频关键词",@"keywordList",@[],
            @{@"keywords":@"视频过滤 屏蔽词 关键词屏蔽"})];
        [a addObject:HGADef(@"player.commentMovesVideo",@"panels",@"打开评论区时上移视频",@"bool",@YES,
            @{@"keywords":@"评论 画面 上移 评论区上移 视频上移"})];
        [a addObject:HGADef(@"panels.episodes",@"panels",@"选集面板与选集按钮玻璃",@"bool",@YES,nil)];
        flag(@"player.fullscreen",@"player",@"横屏铺满");
        flag(@"player.highestQuality",@"player",@"默认使用最高可用清晰度");
        flag(@"player.hideNowPlayingIsland",@"player",@"前台播放时隐藏红果灵动岛播放信息");
        flag(@"player.disableEpisodeSwipe",@"player",@"关闭左滑进入全集");
        choice(@"player.separateProgress",@"player",@"合集与进度条分开布局",@[@"show",@"hide"],@[@"自动留出间距",@"原始布局"]);
        choice(@"player.material",@"player",@"播放控件材质",@[@"regular",@"clear",@"blur"],@[@"普通液态玻璃",@"清透液态玻璃",@"经典磨砂（iOS 18）"]);
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
        choice(@"panels.commentMaterial",@"panels",@"评论区材质",@[@"inherit",@"clear",@"frosted",@"gaussian"],
            @[@"跟随面板材质",@"清透液态玻璃",@"模糊玻璃",@"高斯模糊"]);
        choice(@"panels.material",@"panels",@"面板材质",@[@"regular",@"clear",@"blur"],@[@"普通液态玻璃",@"清透液态玻璃",@"经典磨砂（iOS 18）"]);
        number(@"panels.opacity",@"panels",@"面板底色不透明度",0.28,0.08,0.85);
        flag(@"fonts.enabled",@"fonts",@"启用原生文字字体与缩放");
        number(@"fonts.scale",@"fonts",@"原生文字字号比例",1,0.5,1.3);
        [a addObject:HGADef(@"fonts.name",@"fonts",@"选择已安装字体",@"font",@"",nil)];
        [a addObject:HGADef(@"fonts.asset",@"fonts",@"导入 TTF／OTF／TTC 字体",@"fontfile",@"",nil)];
        catalog = a.copy;
    });
    return catalog;
}
NSArray<NSDictionary *> *HGAppearanceSearch(NSString *query) {
    NSArray *words=[query componentsSeparatedByCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
    NSArray *groups=HGAppearanceGroups(), *titles=HGAppearanceGroupTitles();
    NSMutableArray *matches=[NSMutableArray array];
    for(NSDictionary *entry in HGAppearanceCatalog()) {
        NSString *key=entry[@"key"];
        NSUInteger index=[groups indexOfObject:entry[@"group"]];
        NSString *description=[NSString stringWithFormat:@"%@ %@ %@ %@ %@",entry[@"title"],key,
            index==NSNotFound ? @"" : titles[index],[entry[@"labels"] componentsJoinedByString:@" "] ?: @"",entry[@"keywords"] ?: @""];
        BOOL matched=YES;
        for(NSString *word in words) {
            if(word.length && [description rangeOfString:word options:NSCaseInsensitiveSearch|NSDiacriticInsensitiveSearch|NSWidthInsensitiveSearch].location==NSNotFound) {
                matched=NO;
                break;
            }
        }
        if(matched)[matches addObject:entry];
    }
    return matches;
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
    NSString *pattern=[kind isEqual:@"fontfile"] ? @"^[a-f0-9]{64}\\.(ttf|otf|ttc)$" : @"^[^\\x00-\\x1F/\\\\]{1,128}$";
    return [value rangeOfString:pattern options:NSRegularExpressionSearch].location!=NSNotFound;
}
static id HGAValidatedValue(NSDictionary *definition,id value) {
    if (![definition[@"kind"] isEqual:@"keywordList"])
        return HGAValid(definition,value) ? value : nil;
    if (![value isKindOfClass:NSArray.class]) return nil;
    NSMutableArray *words=[NSMutableArray array];
    NSMutableSet *seen=[NSMutableSet set];
    NSCharacterSet *whitespace=NSCharacterSet.whitespaceAndNewlineCharacterSet;
    for (id entry in value) {
        if (![entry isKindOfClass:NSString.class]) return nil;
        NSString *word=[entry stringByTrimmingCharactersInSet:whitespace];
        if (!word.length) continue;
        if ([word rangeOfCharacterFromSet:NSCharacterSet.newlineCharacterSet].location!=NSNotFound) return nil;
        NSString *folded=[word stringByFoldingWithOptions:NSCaseInsensitiveSearch|NSWidthInsensitiveSearch locale:nil];
        if ([seen containsObject:folded]) continue;
        [seen addObject:folded];
        [words addObject:[word copy]];
    }
    return words.copy;
}
NSDictionary *HGAppearanceValues(void) {
    @synchronized(HGALock()) {
        if (HGACachedValues) return HGACachedValues;
        NSDictionary *stored=[NSUserDefaults.standardUserDefaults dictionaryForKey:HGAStore];
        // Earlier presets persisted every default. Migrate their stock geometry
        // once so installing R15 actually changes the existing thin dock. Values
        // customized away from the old defaults remain the user's settings.
        if(![NSUserDefaults.standardUserDefaults boolForKey:HGAR15GeometryMigration]) {
            NSMutableDictionary *updated=[stored mutableCopy];
            NSDictionary *stock=@{@"dock.height":@[@52,@62],@"dock.radius":@[@26,@31],
                @"dock.iconSize":@[@21,@26],@"dock.extraSize":@[@48,@62]};
            BOOL stockHeight=[stored[@"dock.height"] isEqual:@52];
            for(NSString *key in stock) {
                if([key isEqual:@"dock.radius"] && !stockHeight)continue;
                if([stored[key] isEqual:stock[key][0]])updated[key]=stock[key][1];
            }
            if(updated && ![updated isEqual:stored]) {
                [NSUserDefaults.standardUserDefaults setObject:updated forKey:HGAStore];
                stored=updated;
            }
            [NSUserDefaults.standardUserDefaults setBool:YES forKey:HGAR15GeometryMigration];
        }
        if(![NSUserDefaults.standardUserDefaults boolForKey:HGAR20GeometryMigration]) {
            // A previous preset stored the whole geometry, including defaults.
            // Only update that complete combination; mixed values are user choices.
            BOOL stockWidth=[stored[@"dock.width"] isEqual:@0.905] ||
                (![NSUserDefaults.standardUserDefaults boolForKey:HGADockWidthMigration] &&
                 [stored[@"dock.width"] isEqual:@0.94]);
            if(stockWidth && [stored[@"dock.height"] isEqual:@62] && [stored[@"dock.radius"] isEqual:@31] &&
                [stored[@"dock.iconSize"] isEqual:@26] && [stored[@"dock.extraSize"] isEqual:@62] &&
                (!stored[@"dock.lift"] || [stored[@"dock.lift"] isEqual:@0])) {
                NSMutableDictionary *updated=[stored mutableCopy];
                [updated addEntriesFromDictionary:@{@"dock.width":@(HGADockDefaultWidth),
                    @"dock.height":@(HGADockDefaultHeight), @"dock.radius":@(HGADockDefaultHeight/2),
                    @"dock.iconSize":@(HGADockDefaultIconSize), @"dock.extraSize":@(HGADockDefaultExtraSize)}];
                [NSUserDefaults.standardUserDefaults setObject:updated forKey:HGAStore];
                stored=updated;
            }
            [NSUserDefaults.standardUserDefaults setBool:YES forKey:HGAR20GeometryMigration];
        }
        if(![NSUserDefaults.standardUserDefaults boolForKey:HGABalancedDockMigration]) {
            NSDictionary *oldDefaults=@{@"dock.width":@0.94,@"dock.height":@70,@"dock.radius":@35,
                @"dock.iconSize":@30,@"dock.extraSize":@70,@"dock.lift":@0};
            BOOL stock=stored!=nil;
            for(NSString *key in oldDefaults)
                if(stored[key] && ![stored[key] isEqual:oldDefaults[key]]){stock=NO;break;}
            if(stock) {
                NSMutableDictionary *updated=[stored mutableCopy];
                // Missing values now follow the new catalog defaults.
                [updated removeObjectsForKeys:oldDefaults.allKeys];
                [NSUserDefaults.standardUserDefaults setObject:updated forKey:HGAStore];
                stored=updated;
            }
            [NSUserDefaults.standardUserDefaults setBool:YES forKey:HGABalancedDockMigration];
        }
        NSMutableDictionary *values=[NSMutableDictionary dictionary];
        for(NSDictionary *def in HGAppearanceCatalog()) { id value=stored[def[@"key"]];
            values[def[@"key"]]=HGAValidatedValue(def,value) ?: def[@"default"]; }
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
NSArray<NSString *> *HGVideoBlockedKeywords(void) { return HGAppearanceValue(@"video.blockedKeywords"); }
BOOL HGFlatDockStyle(void) { return [HGAppearanceString(@"dock.style") isEqual:@"flat"]; }
BOOL HGFlatTopStyle(void) { return [HGAppearanceString(@"player.headerStyle") isEqual:@"flat"]; }
BOOL HGFlatPlayerStyle(void) { return [HGAppearanceString(@"player.actionStyle") isEqual:@"flat"]; }
static BOOL HGAError(NSError **error,NSString *message) { if(error)*error=[NSError errorWithDomain:@"Honggo.Appearance" code:1 userInfo:@{NSLocalizedDescriptionKey:message}]; return NO; }
static NSData *HGAConfigurationData(NSDictionary *values,NSError **error) {
    return [NSJSONSerialization dataWithJSONObject:@{@"format":@"Honggo.appearance",@"schema":@1,@"values":values}
        options:NSJSONWritingPrettyPrinted|NSJSONWritingSortedKeys error:error];
}
BOOL HGAppearanceApply(NSDictionary *values,NSError **error) {
    if(![values isKindOfClass:NSDictionary.class] || values.count==0 || values.count>HGAppearanceCatalog().count) return HGAError(error,@"外观配置内容无效。");
    NSMutableDictionary *snapshot=values.mutableCopy;
    for(id key in values) {
        id value=HGAValidatedValue(HGADefinition(key),values[key]);
        if(!value)return HGAError(error,@"存在未知设置、错误类型或超出范围的数值。");
        snapshot[key]=value;
    }
    BOOL keywordsChanged=NO;
    @synchronized(HGALock()) {
        NSDictionary *current=HGAppearanceValues();
        keywordsChanged=snapshot[@"video.blockedKeywords"] &&
            ![current[@"video.blockedKeywords"] isEqual:snapshot[@"video.blockedKeywords"]];
        NSMutableDictionary *merged=[current mutableCopy]; [merged addEntriesFromDictionary:snapshot];
        if ([merged[@"video.blockedKeywords"] count] && HGAConfigurationData(merged,nil).length>128*1024)
            return HGAError(error,@"关键词列表使配置超出 128 KiB 备份上限。请减少关键词后再保存。");
        if([merged[@"dock.hide.home"] boolValue] && [merged[@"dock.hide.theater"] boolValue] && [merged[@"dock.hide.store"] boolValue] && [merged[@"dock.hide.mine"] boolValue]) return HGAError(error,@"至少保留一个底栏导航项目。");
        // One persisted dictionary, after complete validation: no partial imports.
        [NSUserDefaults.standardUserDefaults setObject:merged.copy forKey:HGAStore];
        HGACachedValues=merged.copy;
    }
    void (^notify)(void)=^{
        [NSNotificationCenter.defaultCenter postNotificationName:HGAppearanceDidChangeNotification object:nil];
        if(keywordsChanged)
            [NSNotificationCenter.defaultCenter postNotificationName:HGVideoBlockedKeywordsDidChangeNotification object:nil];
    };
    if(NSThread.isMainThread) notify(); else dispatch_async(dispatch_get_main_queue(),notify);
    return YES;
}
BOOL HGAppearanceSet(NSString *key,id value,NSError **error) { if(!key || !value)return HGAError(error,@"设置不能为空。"); return HGAppearanceApply(@{key:value},error); }
void HGAppearanceReset(void) { NSMutableDictionary *d=[NSMutableDictionary dictionary]; for(NSDictionary *def in HGAppearanceCatalog()) d[def[@"key"]]=def[@"default"]; HGAppearanceApply(d,nil); }
BOOL HGAppearanceEnabled(void){return HGAppearanceFlag(@"enabled");}
BOOL HGAppearanceNeedsGlass(void) {
    if(!HGAppearanceEnabled())return NO;
    if(HGAppearanceFlag(@"player.finderGlass") || HGAppearanceFlag(@"player.mineGlass"))return YES;
    for(NSString *key in @[@"dock.enabled",@"player.rightGlass",@"player.headerGlass",@"player.episodeGlass",@"player.controlsGlass",@"panels.episodes",@"panels.comment",@"panels.share",@"panels.longpress",@"panels.speed"])
        if(HGAppearanceFlag(key))return YES;
    return NO;
}
NSDictionary *HGAppearancePreset(NSString *name) {
    NSMutableDictionary *d=[NSMutableDictionary dictionary]; for(NSDictionary *def in HGAppearanceCatalog()) d[def[@"key"]]=def[@"default"];
    d[@"video.blockedKeywords"]=HGVideoBlockedKeywords();
    if([name isEqual:@"glass"] || [name isEqual:@"focus"]) {
        for(NSString *key in @[@"enabled",@"dock.enabled",@"dock.extra",@"dock.haptics",@"player.rightGlass",@"player.headerGlass",@"player.episodeGlass",@"player.controlsGlass",@"panels.episodes",@"panels.comment",@"panels.share",@"panels.longpress",@"panels.speed"])d[key]=@YES;
        if([name isEqual:@"focus"]) {d[@"dock.collapse"]=@YES; d[@"dock.autoCollapse"]=@YES; for(NSString *role in @[@"avatar",@"tags",@"notice"])d[[NSString stringWithFormat:@"layout.%@.hidden",role]]=@YES; }
    }
    return d.copy;
}
NSData *HGAppearanceExport(NSError **error) { return HGAConfigurationData(HGAppearanceValues(),error); }
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
    // Ignore only retired keys from earlier appearance backups.
    [d removeObjectsForKeys:@[@"dock.tint",@"dock.selected",@"dock.normal",@"icons.enabled",@"icons.preset",@"icons.size"]];
    for(NSString *role in @[@"like",@"collect",@"comment",@"share"])
        for(NSString *state in @[@"normal",@"selected"])
            for(NSString *field in @[@"asset",@"symbol"])
                [d removeObjectForKey:[NSString stringWithFormat:@"icons.%@.%@.%@",role,state,field]];
    if(!d.count){HGAError(error,@"文件未包含可用的外观参数。");return nil;}
    for(id key in d.allKeys) {
        id value=HGAValidatedValue(HGADefinition(key),d[key]);
        if(!value){HGAError(error,@"文件包含未知或无效的外观参数。");return nil;}
        d[key]=value;
    }
    return d.copy;
}
