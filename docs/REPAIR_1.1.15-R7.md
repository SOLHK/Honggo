# Honggo 1.1.15-R7 beta — iOS 27.2 与 TG 更新频道

基于已通过 CI 的 R6，保留全部独立外观模块和原有功能开关。本版不扩大宿主适配范围；仍针对红果 7.4.3 / Eggplant 7.4.3.33，arm64，最低编译目标 iOS 15.0.1。

## TG 入口

项目所有者提供的固定地址为 https://t.me/Naloiafeed 。主设置底部「更新与频道」及「外观」首页均有「TG 更新频道」。支持 TG、Telegram、频道、更新、Naloiafeed 搜索，多词须同时匹配。

点击后先显示准确地址和「打开频道」「复制地址」「取消」。打开使用 HTTPS，由系统选择 Telegram 或浏览器；无法打开时显示错误及手动复制选项。不检测已安装 App，不修改宿主 URL scheme 白名单，不自动打开、不自动订阅、不读取剪贴板、不上传诊断。复制只在用户点击时发生。

## 27.2 验证

GitHub Actions 使用 `xcode-27` runner，显式选择 `/Applications/Xcode_27.2.app/Contents/Developer`，验证 Xcode、iPhoneOS SDK 和 Simulator SDK 都为 27.2。各 UIKit 套件选择已安装且可用的 iOS 27.2 runtime，通过 `simctl create` 创建独立 iPhone，启动后复核 UDID、runtime 和 Booted 状态，记录 `simulator-target.json`。新版本系统即使也已安装，也不会取代 27.2；缺失指定 runtime 时直接失败。

保留底栏负例对照、原设置搜索/转场、广告、诊断、runtime、外观等发布门槛。新增运行时选择测试、频道搜索及菜单的真实 UIKit 回归。受控宿主测试不执行外部 Telegram 跳转，不自动复制或加入频道。

发布附件保存 `BUILD-ENVIRONMENT.json`、源码提交及 SHA-256。测试结论仅以本提交实际 CI 结果为准。模拟器能验证插件代码在 UIKit 中的行为，但不能运行普通 arm64 iPhone IPA 来替代红果真机验收。

## 安装

只注入一份 `Honggo-1.1.15-R7.dylib`，移除旧 Honggo/HongguoGlass。打开「我的 → 设置 → Honggo → 外观」，应用「玻璃外观」预设后完全退出红果再打开。字体、控件位置与竖屏填满按需单独设置；横屏/方形画面不强制放大。具体视觉覆盖边界沿用 R5/R6 文档。

## 首轮 27.2 验证修正

首次 27.2 CI（43709722973）中，当前底栏 121 项、启动配置 123 项、材质像素、外观 74 项和广告 42 项均通过，发布仍被两项验证问题拦截，未生成发布版：

1. 对照 App 使用 Xcode 27.2 SDK 构建。Apple 明确说明此类主程序会忽略 `UIDesignRequiresCompatibility`，因此旧对照条件“清空后必须是空壳”不再成立。测试现在记录编译 SDK，并验证其与 xcrun 选择一致；27 SDK 对照必须实际呈现玻璃，26 SDK 对照仍保留旧负例。没有取消像素校验、透明/纯色对照或历史遮罩负例。此处只调整测试 App 的预期，不能据此推断旧 SDK 红果主程序也忽略该键。
2. 设置搜索后导出配置的检查在 8 秒时超时，日志显示随后系统分享服务仍在完成冷启动。只将系统分享面板的检查上限设为 30 秒，其他动作仍为 8 秒，并记录实际耗时；必须真的显示且完成呈现才能通过。整套测试预算按新增频道场景扩展到 180 秒，最低完整检查数量从 21 提升为 72，任何失败仍阻止发布。

说明依据：[Apple 兼容键文档](https://developer.apple.com/documentation/bundleresources/information-property-list/uidesignrequirescompatibility)与 [Apple Frameworks Engineer 的 SDK 说明](https://developer.apple.com/forums/thread/838637)。这次只修改测试及验证文档，不改 R7 插件二进制源码；版本沿用未发布的 R7。重新运行的实际 CI 结果才是最终发布依据。
