# Honggo 1.1.15-R5 自然小游戏推广入队拦截

用户在红果 7.4.3.33 安装 R4 后仍看到「欢喜斗地主」的全屏视频推广和「点击即玩」按钮。当前进程 48 条广告规则均安装，13 条产生 52 条记录。规则 43–47 无决定日志，包含 R4 专用视图初始化记录。显示回调的日志只在通过清理条件后才写。导出快照是设置页，`maskScenes` 无广告现场。[脱敏日志统计](reverse/7.4.3.33/r5/r4-log-summary.json)

原始 IPA 对照发现 `SSNaturalAdData` 还有独立的 Lynx 游戏推广路径。宿主在 `parseGameDict` 遍历 `lynxDataArray`，对有 `game_content` 的 `feed_game_channel` 1/2 条目设置试玩分类，对 4/6 条目设置视频推广分类。两组均标记为 `isGameContent`。`SSNaturalAdContainerView` 会按自身尺寸挂载全页 Lynx 内容。截图的推广标题和按钮文字未在 IPA 的已扫描文本编码中找到，当次投放仍待现场层级确认。[模板与分类分析](reverse/7.4.3.33/r5/NATURAL_GAME_PAYLOAD.md) · [REA 摘要](reverse/7.4.3.33/r5/r5-rea-summary.json) · [REA 指令](reverse/7.4.3.33/r5/rea-instructions.txt)

`SSBizVideoAdCommonAdClient`、`SSShortVideoMUnionAdService` 和 `SSShortVideoAdService` 共同进入 `SSNaturalAdManager tryInsertDataToSaaS:`。R5 在开启「隐藏短剧广告、直播和小游戏插播」后，对精确的 `SSNaturalAdData` 使用宿主原有的分类器，并在 `forceWatchTime < 1` 且为游戏推广时返回宿主已有的 `NO`。拦截发生在横竖屏复制、视图构造、列表插入和重试入队之前。普通数据、未知类型和强制观看的数据继续走原方法。[原生分支和回调核对](reverse/7.4.3.33/r5/NATURAL_GAME_ENTRY_GATE.md)

本地 ARM64 编译链接、26 项单元测试、49 条广告规则 ABI 及 3 个辅助方法对照通过。用实际 R5 包执行了 160 项受控 ARM64 分支回放，覆盖延迟分类、强制观看边界、类型匹配和原方法的传参及返回。新增 UIKit 用例已交叉编译。UIKit 模拟器和红果真机未运行。

真机验收仍需复测原场景。签名只加载 R5，开启该开关并完全退出重开。如果广告仍出现，请先打开增强诊断，再在广告停留时用实体键截图，随后回设置导出日志。现场结构能区分这次的 Lynx、原生试玩和其他容器。
