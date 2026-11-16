# 1.1.15-R24 · 单独搜索、截图推荐和播放玻璃

本轮基于 R23。核对红果 7.4.3.33 的同一 Eggplant 二进制，SHA-256 为
`01bcb8b48ad07cb6e1cac5917dfcb67cdba9bf3f5239df9171bc46bb637fb8ab`。
最新 R23 日志包含首页与横屏控件结构，但没有商城现场结构；商城另外核对同构建的原生挂载方法。

## 修改及证据

- 顶部四个频道隐藏且仅剩搜索或菜单时，玻璃缩为与顶部高度相等的圆形，中心跟随原生按钮。保留原按钮位置、手势与尺寸；恢复频道后恢复整条胶囊，全部隐藏时仍移除材质。
- 新增「隐藏截图识图找同款」，使用独立偏好 `screenshotShopping`，默认关闭。开启后重启红果生效。仅拦截 `SSBizVideoScanShoppingService.enableVideoSearch`，ABI `B16@0:8`，文件偏移 `0xed6dee8`。同一宿主中两个调用点分别位于截图通知流程及 `SSUGCleanScreenshotHelper.canSearch`；原生拒绝分支已有退出或保留独立截图分享的处理。没有拦截系统截图、手动 `openPicSearch` 或 `startScanSearchByMorePanel:`。
- 商城创建器向 `FQMSEcomXTabMallViewController` 注入 `safeBottomGetter`，返回根控制器的原生底栏高度。`mountMallViewIfNeeded`（`v16@0:8`，`0x808a8dc`）首次挂载时从页面高度扣除此值；已挂载时直接返回。配置加载及 `mallViewPageReset` 最终均经过该方法。修复限定此挂载节点，复用既有底栏内容延伸与恢复处理，保持原商城安全区 getter 和商品交互。
- 全集入口使用准确的 `defaultContentView`，在原生 `renderWithModel:` 返回后重新应用材质。原生 `setup`（`0xf6fbf84`）给内容设置黑色背景，并在其首项插入 15% 白色纯底视图；现在限定该组件、首项及颜色识别清除底漆，关闭玻璃后恢复。保留侧边距、进度条间距和原生跳转。未保存开关时全集入口玻璃默认开启；已有明确关闭值不覆盖。
- 横屏原生 HUD 在祖先透明度为零时准备，随后只通过透明度显示；之前可见性筛选会漏掉此时的控件。现在提前准备已识别的玻璃，由原控件管理显隐，并在已核对 ABI 的显示/刷新入口回访导航、底部互动、弹幕输入、倍速、清晰度及选集。补齐实际 `SSShortVideoLandscapeAutoInflowTipView` 完整 AI 剧入口和音量控件。输入区只隐藏精确 `danmakuInputBGView` 的旧材质，关闭后恢复；原输入、字形、数量和点击保持。

## FanqieX 参考

按用户授权参考其 FanqieX `a476679c7d77e51fffa4b96ca8e10257572edbca`：
`native/AlignedGlass.m` 的短剧入口原生背景处理和 `renderWithModel:` 刷新，
`native/ControlGlass.m` 的控件透明/裁剪恢复，以及 `native/PublicGlassAPI.m` 的公开材质配置。
移植前以红果自己的类型、getter、方法 ABI 和真机层级重新核对；没有整体复制番茄的播放布局或修改 FanqieX。

用户选定的「经典磨砂（iOS 18）」继续使用 `UIBlurEffectStyleRegular`，液态玻璃选项继续使用支持系统的 `UIGlassEffect`。本轮不切换用户已有的底栏材质。

## 验证边界

- 生产 arm64 编译及链接通过，iPhoneOS 27.2 SDK，最低目标 iOS 15.0.1，`-Wall -Werror`。
- 六个相关测试入口交叉编译通过：`AppearanceIntegration.m`、`AppearancePreferencesTests.m`、`AdsIntegration.m`、`PreferencesIntegration.m`、`SettingsIntegration.m`、`TabBarIntegration.m`。包含只剩搜索与恢复、全集重绘、横屏显示/弹幕刷新原生回调、截图开关边界、商城晚挂载/重建/还原的夹具。
- `test_version.py` 实际执行 13 项通过；差异空白检查通过。产物与源码归档检查结果见随包验证说明。

Windows 没有 iOS Simulator 或连接的 iPhone，UIKit 夹具不能执行；真实商城遮罩、横屏淡入和按钮视觉仍需真机复测，静态宿主调用链及编译不能替代视觉验收。
