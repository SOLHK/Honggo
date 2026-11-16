# R21 页面控件适配

宿主为红果 7.4.3.33。适配复用 `AD_AUDIT_7.4.3.33.md` 所述宿主 Objective-C 元数据。运行时通过精确类型、方法签名和所属视图范围识别控件。

- 首页顶部玻璃与展开底栏共用宽度计算，包含独立圆形按钮。通过 `SSVideoSeriesFeedHeaderView` 和其 `FQRJXCategoryTitleCell` 分别控制菜单、关注、漫剧、真人剧、推荐、搜索，六项默认显示。选中频道隐藏时，同时隐藏原生选中提示。
- 找剧搜索、识剧、AI 入口使用首页找剧状态下的 `SSVideoSeriesFeedHeaderView` 和剧场视图控制器各自的原生 getter。筛选、排行榜、新剧、预约按明确的入口类、所属页面和标题定位。
- 我的页面使用 `SSMyUser637TabsView.searchBtn` 与原生历史工具栏适配搜索、全部、已看完、未看完、筛选和编辑。旧版原生页面挂载时，另适配 `SSMyUser637Bar` 的深色模式和菜单、`SSMyUserFollowItemView.editButton` 及 `SSMyUser637JGZoneCell` 的订单、消息、游戏、追更和更多功能入口。
- 截图 6 对应设备记录中的新版「我的」上半部分使用 Compose。记录里只识别到整块 Compose 标题栏和内容宿主，月亮、菜单、编辑资料、订单、消息、游戏、追更、更多功能没有已确认的独立 UIKit 或无障碍节点。这部分单独玻璃待取得该分支的具体控件证据。

找剧与我的页面玻璃默认随外观总开关生效，可分别关闭。iPhoneOS arm64 编译覆盖生产适配及 UIKit 测试夹具。夹具针对顶部宽度、选择提示与独立隐藏、找剧材质和恢复、原生历史控件，运行结果由 Simulator CI 提供，实际宿主效果仍需真机核验。
