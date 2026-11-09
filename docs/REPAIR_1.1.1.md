# Honggo 1.1.1

适配红果短剧 7.4.3 / 7.4.3.33，`Eggplant` / `com.phoenix.video`。本轮处理嵌套列表底部的旧布局残留、原生 Liquid Glass 按钮和可导出的诊断。设备视觉验收仍需签名安装后的 iPhone。

## 滚动到底部

1.0.6 遍历到竖向 UIScrollView 就停止，把内层列表当作普通内容而没有继续检查。原始主程序确认存在 `SSNestedContainerScrollView` 及其 `contentView`。新版本只对这个已确认的嵌套滚动类继续检查：外层视口延伸，内容包装层尺寸保持宿主控制，底部可滚动留白只加到内层列表。普通表格/集合仍不进入复用单元格。对贴边内容容器，将材质在安全区内保留约 20pt 对称留白；已有悬浮边距的宿主保持原外观。按钮框架和触摸范围不缩小，最后一行仍可完整滚到按钮上方。已观察滚动实例的 contentOffset setter 返回后同步复查，避免等到无关布局才处理边缘。

新增受控 UIKit 用例覆盖展开/收起头部、滚到底、重复布局和离开恢复；对精确的 `78a17ad`（1.0.6）执行相同用例作为负对照。这个代码缺口可通过模拟宿主验证，但没有设备日志证明用户截图中的每一次白色区域都来自同一路径。列表内容用尽后的正常留白也不等于底栏背景复现。

## 原生材质和控件

主 App 必须在启动前退出旧设计兼容模式。本机找到的原 IPA 哈希与此前核对版本一致：`37118984f159cc2ac8e30654d35196ea31dbf0e829b2bf081df9fd00ee74b036`。`prepare_ipa.py` 在新包中把主 App 的 `UIDesignRequiresCompatibility` 设置为布尔值 false 并注入一份插件，源 IPA 保留。

原生路径使用系统 `UIGlassEffect` Regular、interactive 和 `UICornerConfiguration`，不是用模糊叠加描边模拟折射。旧系统/未配置的包仍明确标为模糊底栏；新增「原生液态玻璃按钮」在未配置包上不套用模糊替代品。系统导航栏、菜单、开关等是否采用新设计仍由操作系统与宿主控件类型决定。

新增按钮开关默认关闭，保留既有设置。打开后对原 IPA 中核验的 20 类、35 个 getter 及一个手势按钮视图接入：

| 区域 | 适配入口 |
| --- | --- |
| 历史页 | 搜索、更多、筛选、管理/编辑 |
| 个人资料与角色资料 | 返回、分享、更多、关注、发布、投票、编辑资料 |
| 我的和视频列表 | 设置、追更编辑、收藏编辑、历史编辑 |
| 短剧详情与互动 | 返回、分享、更多、展开、重播 |
| 分类导航与工具栏 | 返回、左右操作、更多、关闭、游戏入口 |
| 插件设置 | Honggo 菜单卡片 |

精确名单在 `src/Controls.m`。只在这些已核对的类安装局部生命周期 hook，不修改全局 UIButton/UIView。按钮文字、前景图片、宿主 target-action、菜单和触摸区域保留，玻璃层不接收点击。重复布局保持一个玻璃层。关闭并重新布局/重启后恢复插件拥有的背景。

已有 UIButtonConfiguration、掩码、变换或不适合尺寸的自定义控件跳过，避免破坏原有状态和布局。Lynx/KMP/Web 内容里的绘制按钮不能仅凭同家公司或看起来像按钮就替换。未识别区域需要设备结构日志后继续适配；本版不宣称全 App 所有按钮均已通过真机验收。

## 日志和增强诊断

入口：我的 → 设置 → Honggo → 诊断与反馈。

- 「增强诊断」即时生效，默认关闭；记录布局采样和适配按钮触摸。基础模式记录启动阶段、hook 安装/跳过和已启用广告规则的命中计数（首个及二次幂采样）。不拦截或修改宿主异常处理。
- 「导出最近一小时日志」生成独立 JSON，用系统分享保存。包含版本、会话、系统版本、实际材质、兼容标志、辅助功能状态，以及最多 200 个视图的结构/滚动几何快照；不读取标题、正文、图片、账号、Cookie、Token 或网络内容。
- 串行异步写入，待写最多 256 条，保留最多 4096 条/约 2 MiB。队列溢出和容量裁剪分别计数。旧日志在读取、写入和导出时按一小时清理；进程不活动时磁盘文件可能保留到下次维护。最多四个不可变导出文件。
- 小型同步启动阶段文件记录最后到达的插件阶段，下次启动可读取上一次阶段。它不等同于 iOS 崩溃报告；dyld 拒绝加载之前的故障无法记录，尚未写入的事件仍可能随异常退出丢失。
- 清除入口删除保留事件、导出文件和启动记录。存储失败会明确返回错误；历史存储错误保留在导出报告，直到清除。

复现建议：打开增强诊断，关闭设置，回到「我的」从顶部滚到底，再打开设置导出。这样日志中的布局采样保留复现期间的几何信息；导出快照本身对应打开设置后的当前层级。

## 与 FanqieX 的关系

分别检查了两份原始 7.4.3.33 主程序：红果为 `Eggplant`，番茄为 `Reading` / `com.dragon.read`。抽查的 `SSTabBar`、`SSNestedContainerScrollView`、历史操作视图、开屏广告服务、短剧插播服务、个人资料头栏，七类均存在于两份主程序。

这支持共享材质、日志和部分滚动适配思路；不支持直接互换整个 dylib。宿主身份校验、阅读/播放业务、导航状态、广告入口、方法签名和偏好命名空间各自不同。Honggo 继续独立适配，不改动 FanqieX 仓库或移植其二进制补丁。此次红果目标的 68 个方法签名已与原始 Eggplant 静态元数据核对，全部一致；这是静态证据，不替代运行验证。

## 开源调查与独立实现

按要求先检索 GitHub，再实施。参考 [CocoaLumberjack](https://github.com/CocoaLumberjack/CocoaLumberjack) 的串行写入与容量/时间轮转思路；它是成熟日志库，但本插件没有引入它的代码或依赖。查阅 [DroidKaigi 原生底栏接入说明](https://github.com/DroidKaigi/conference-app-2026/blob/main/docs/ios-liquid-glass.md)、[LiquidGlassTabBar](https://github.com/IchimTeodor/LiquidGlassTabBar) 的系统材质接入方式，未复制第三方实现，也不引入自行绘制的 Metal 折射。官方 API 依据：[Apple UIKit 新设计](https://developer.apple.com/videos/play/wwdc2025/284/)、[UIGlassEffect](https://developer.apple.com/documentation/uikit/uiglasseffect)。

## 验证

本地：六个生产 Objective-C 模块、四个集成测试模块通过 Apple iOS 16.5 头文件下的 ARM64 编译；20 项 Python 打包/版本测试通过；68 个目标 ABI 核对通过。动态 API 在 Apple CI 上继续编译和运行验证。CI 结果以对应 SHA 的 Actions 报告为准，完成后才能交付设备候选。

命令：`python3 -B -m unittest discover -s tests -v`、`bash tools/test-tabbar-ios.sh`、`bash tools/test-settings-ios.sh`、`bash tools/test-diagnostics.sh`、`bash tools/build-ios.sh`。

### GitHub Actions 安全检查

| 严重性 | 数量 |
| --- | ---: |
| Critical | 0 |
| High | 0 |
| Medium | 0 |
| Low | 0 |

检查 `.github/workflows/ios.yml`：仅受信任 `test` push，`contents: read`，托管临时 runner；actions 固定完整 SHA，checkout 不保留凭据，无不可信表达式直接拼接 shell，无额外秘密或写权限。设备编译仍依赖两组 UIKit/诊断门禁通过。安全评审没有需要应用的修订，本轮 CI 修改仅增加已授权的测试步骤和证据文件。
