# 红果短剧 7.4.5.33 静态分析与 Honggo R35 界面诊断

分析日期为 2026-11-23。Honggo 基线为 `dc64a375bd36a2523afffb63a223066458171e16`，对应 `v1.1.15-R35`。结构化结果、哈希、文件偏移、Objective-C 类型编码和诊断计数见 [evidence.json](evidence.json)。

## IPA 完整性和覆盖范围

原文件 `红果短剧_7.4.5.ipa` 为 311,558,144 字节，SHA-256 为 `c525aeed5df0d0f826ecaf346b68df699e43014e51939480553757bba194eee5`。标准 ZIP 读取和 REA 均未找到有效的中央目录结束记录。根据 ZIP 本地文件头扫描出 4,027 个可识别记录，逐条校验解压状态、长度与 CRC-32 后恢复 4,023 个条目，其中有 3,643 个文件和 380 个目录。文件尾及缺失记录限制了原始包的统计上限。

| 未恢复的条目 | 观察结果 |
| --- | --- |
| 主包 `Assets.car` | Deflate 距离码无效 |
| `growth_incentive-monorepo_novel-lite.zst` | 外层 ZIP Deflate 块损坏 |
| `DanceUIRenderBoxShader-iphonesimulator.metallib` | Deflate 数据流提前中断 |
| `book-upload.zip` | 声明的压缩数据超出文件末尾 |

主程序、主包及两个扩展的 `Info.plist`、两个扩展可执行文件均通过 CRC 校验。重建 ZIP 仅用于离线分析。REA 的 `complete-within-inventory` 指重建包内的覆盖范围。原始包的中央目录、主 Asset Catalog、丢失的尾部数据和 iOS 运行时行为仍缺少完整证据。

## 包结构与主程序

| 组件 | 宿主标识 | 入口或用途 |
| --- | --- | --- |
| `Eggplant.app` | `com.phoenix.video` | arm64 `Eggplant`，红果短剧主程序 |
| `EggplantNotificationExtension.appex` | `com.phoenix.video.appPushExtension` | `com.apple.usernotifications.service` |
| `EggplantWidgetExtension.appex` | `com.phoenix.video.WidgetExtension` | `com.apple.widgetkit-extension` |

三个可执行文件为 arm64 Mach-O。主程序长度 527,025,520 字节，UUID `b067066f-d7f2-3df8-8e5d-c181f8c7ce1f`，SHA-256 `40edce7297acca3d4f5fedba2c3299354a5fb3b7841036f4da298403bd711bd7`。主包构建号为 `7.4.5.33`，链接 SDK 为 iOS 26.0，最低系统版本为 15.0.1。提取的 Mach-O 的加密标记为 `cryptid=0`，存在签名载荷描述，系统安装和签名信任状态未验证。

REA 应用图识别了 137 个 bundle、124 项 bundle 元数据、3 个可执行文件、17 个 JavaScript 组件和 3 项签名资源。恢复内容包含 2,563 个 PNG、295 个 JSON、155 个 plist、53 个其他 `.car`、18 份隐私清单和 14 个 Metal library。图中未发现独立 framework 或 dylib 文件。主可执行文件有 143 条动态库／框架加载命令，这与主包中识别出大量集成模块一致。

按 Mach-O 段映射 Objective-C 类表，读出 79,471 个类名以及 1,244,534 条可解码的实例方法表记录。这是元数据记录数，可能包含重名和共享实现。证据文件列出此次界面问题直接相关的 77 条声明方法及其文件偏移。继承方法须与运行时 ABI 校验一起判断。

## 六张截图的证据

| 截图 | 确认的路径与现象 | 验证边界 |
| --- | --- | --- |
| 1 分享／更多面板 | `SSShortVideoPlayerMorePanelController` 有独立的 `showCustomShareHeaderWithShortVideoVC:channels:dataModel:`，`FQVShortVideoSettingsViewController` 提供 `customHeaderView`、`effectViewContentView`、`tableView` 和 `useBlurBackground`。R35 的面板适配固定选择普通 blur，并加 0.92 以上的底色。修正方向是对有界内容区使用用户选的素材，并保留分享目标、播放设置和退出动作。 | 图片支持组合面板的判定。各入口的实时视图层次仍需真机确认。 |
| 2 外观预设 | 两行由 Honggo 的 `AppearanceSettings.inc` 直接生成。删除行及其点击路径即可，外观开关仍通过现有偏好键保存。 | 宿主二进制无对应改动。 |
| 3 Dock 下方黑带 | 截图的分界线与视频页边齐平。R35 只在严格匹配 feed、当前 cell、`TTPlayerView` 和 `TTMetalView` 几何时向下扩展 live renderer，判定失败时才使用 host matte。宿主有 `refreshBgColor`、`bottomGradientView` 和 `handleOutterFeedClipBounds:`，也有独立 `SSTabBar.darkView`。 | 这次日志没有同一帧的 Metal 几何，无法确定截图里的黑色来自宿主 matte、视频画面未延伸还是其他背景。向下延展路径覆盖可见 cell 包装层和 Metal 竖向过扫描。 |
| 4 评论和发评论 | `FQCShortVideoCommentPanelViewController` 提供 `tabContentVCs` 及面板生命周期，列表为 `FQCShortVideoCommentListCollectionView`，宿主简写栏 `FQCShortVideoBottomToolBar` 继承带 `bgView` 和 `effectView` 的 `FQCCommentBottomToolBar`，展开编辑器 `FQCShortVideoNewEditorView` 分别提供 `bgView` 及 `contentView`。R35 还给列表和标题独立铺材质，截图的折射位于列表边缘。修正方向是单一面板材质、输入栏单独使用可控背景、展开编辑器只作用于内容区。 | 列表、输入栏、键盘、表情和图片入口的组合仍需真机核对。 |
| 5 首页左上角侧栏 | `simpleSideBarViewDidClicked` 入口文件偏移 `0x105550dc`，静态调用关联到 `toggleSidebar:toggleType:`。同一宿主另有 `toggleSidebar:toggleType:duration:`，偏移 `0x10554940`，编码 `v36@0:8B16@20d28`，其中能看到位移、遮罩透明度和侧栏可见状态更新。R35 会在某些视图变更中同步刷新 Dock 和视频 viewport，可能与转场叠加。 | 两个截图场景记录出现了侧栏单元。日志尾部为插件设置打开事件，没有无响应时的主线程栈。暂停转场期间重入的修正需要实机再现验证。 |
| 6 Dock 动画 | 当前 FanqieX `test` 基线 `e0283f768f0e38db715c3ca7594c964ab5b24bf6` 的 `native/FanqieXMotion.m` 使用 `UITabBar` 原生 platter 和 delegate，把选中项映射回宿主按钮。Honggo R35 由自建 pan、拖动透镜和 UIView 动画处理。修正沿用 FanqieX 的原生选择路径，同时使用 Honggo 自己的 tab 次序和按钮动作。 | 需要真机检查点按、按住拖动、松手吸附、取消手势和独立圆钮同步。 |

设备日志记录了 iOS 27.3（`24C5049f`）和 7.4.5.33，插件版本为 R35。共采集 2,912 个事件、7 个视图场景，5 次设置打开报告原生玻璃可用，16 条 `tabbar.layout` 报告 `UIGlassEffect`。主包 plist 明确启用 `UIDesignRequiresCompatibility`，因此原生玻璃仍依赖实际运行环境中的启动配置；一次 `UIGlassEffect` 记录无法证明其他界面的最终合成结果。

## 复现分析方法

1. 记录输入 SHA-256。通过 ZIP 本地文件头恢复合法 `Payload/` 条目，按记录验证 Deflate 流、解压长度和 CRC-32。保留失败项及偏移，重新封装校验通过的记录。
2. 使用 [REA 6.7.0](https://github.com/morluto/rea) 运行 `inspect-artifact`。从结果取 `data.normalized_result.substeps[0].evidence`，作为 `inventory_evidence` 数组传给 `project-apple-application-graph`。结果的 Evidence ID 和图投影 ID 可与证据文件比对。
3. 解析三个 Mach-O 的 load commands、段和签名描述。主程序全文件 REA/Rizin 索引超过 Windows 环境的 120 秒限制。对侧栏、分享和评论入口按偏移截取四个 ARM64 窗口，用 REA 的 `execute-rizin-command` 和 [Rizin 0.9.1](https://github.com/rizinorg/rizin/releases/tag/v0.9.1) 解码。证据文件给出了窗口位置、长度和 SHA-256；分支重定位以原 Mach-O 的段映射为准。
4. 将静态方法及调用与 R35 源码、日志和截图对照。报告覆盖可恢复资源清单、应用结构、三个 Mach-O 及本次界面的调用路径。未执行红果 iOS 进程，也未逐函数反编译主程序。
