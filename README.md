# Honggo

**插件版本：1.0.6。** 支持 **红果短剧 7.4.3（7.4.3.33）**，主程序 `Eggplant`，原包标识 `com.phoenix.video`，设备架构 `arm64`，部署目标 iOS 15.0.1。当前为 `test` 分支的真机验收候选。仓库包含插件源码、构建脚本和本地 IPA 预处理工具；原始红果 IPA 需自行提供。

**液态玻璃需要正确的宿主配置。** 1.0.3 的受控宿主系统截图证明：保留 `UIDesignRequiresCompatibility=true` 时，清除原生效果的实心背景只能得到与无效果视图相同的透明内容。从 1.0.4 起，预处理工具把受支持主 App 的该键明确设为布尔值 `false`，让本插件采用原生设计路径。单独注入 `Honggo-1.0.6.dylib` 且保持原兼容配置时，插件使用系统模糊回退，不能据此宣称启用了原生 Liquid Glass。这是 Honggo 当前对受支持宿主的启用策略；Apple 对该配置键的解释还取决于主 App 的构建 SDK，不能推广为所有 App 的统一条件。

**本轮补齐宿主直接重新显示背景的处理。** 1.0.5 的完整 CI 已通过并生成设备插件，回退胶囊圆角与截图同步已验证。复查发现，它主要观察背景父视图；宿主单独把已隐藏背景设为可见时，仍可能在下一次布局之前露出旧底板。1.0.6 只对已经确认的宿主背景实例观察可见性变化，在 setter 返回前重新清理，并在背景脱离底栏时解除观察。保留原生玻璃、回退圆角及全部既有验证，增加 1.0.5 源码的直接重显示负对照。实际结果见 [GitHub Actions](https://github.com/SOLHK/Honggo/actions) 与 [1.0.6 修复记录](docs/REPAIR_1.0.6.md)。模拟器使用受控宿主，不替代真实红果的设备验收。

## 安装与使用

1. 使用这次核对过的红果短剧 7.4.3 IPA。待签名的宿主主程序需要为 iOS arm64 且 `cryptid=0`。
2. **准备原生液态玻璃配置**：在电脑上用 Python 3 执行以下命令。工具严格核验宿主包标识、应用版本、构建号和主程序名，在新 IPA 中注入插件，并且只把主 App `Info.plist` 的 `UIDesignRequiresCompatibility` 改为布尔值 `false`。其他元数据值、应用扩展的 plist 和未修改条目内容保持原样，源 IPA 不会被覆盖。工具遇到加密包、其他构建号、重复注入或没有足够的 Mach-O 头部空间时会退出。

   ```text
   python3 tools/prepare_ipa.py "红果短剧_7.4.3.ipa" Honggo-1.0.6.dylib Honggo-1.0.6-unsigned.ipa
   ```

3. 将生成的待签名 IPA 导入签名器，重新签名主应用及内嵌代码后安装。预处理不会签名，也不会自动安装。已预处理的 IPA 不要再添加一次 dylib。
4. **仅注入 dylib 的路径**仍可使用：支持添加动态库的签名器可将 `Honggo-1.0.6.dylib` 注入原 IPA 再签名。保留原兼容配置时，底栏为系统模糊回退；只更换 dylib 不足以可靠启用此宿主的原生 Liquid Glass。[Feather](https://feather.claration.dev/) · [Sideloadly](https://github.com/SideloadlyiOS/Sideloadly-Download)
5. 安装后打开 **「我的 → 设置 → Honggo」**（菜单与设置页都会显示插件版本），点击设置列表顶部的菜单，即可看到底栏、广告与弹窗的 15 个开关。同一个底栏开关根据配置显示「液态玻璃底栏」或「系统模糊底栏」。导航栏的「Honggo」按钮和双指长按底栏约一秒仍可打开同一个设置页。开关默认关闭，每次改动后从应用切换器彻底退出并重开红果。

**关闭底栏开关的范围：** 关闭并重启后恢复插件对底栏和内容布局的改动。预处理写入的设计兼容标志是主 App 的全局启动配置，可能影响其他系统控件；底栏开关不会在运行时将它改回。要恢复原包的全局设计配置，使用保留的原 IPA 重新签名安装；不注入插件即可同时撤除 Honggo。

升级时从原 IPA 重新注入新版，避免同时加载旧版 `HongguoGlass.dylib` 和新版 Honggo。预注入工具会拒绝已经包含旧名或其他 Honggo 版本的 IPA，并将新版统一放在 `Frameworks/Honggo.dylib`。旧版的开关偏好保留，升级不会重新关闭已开启的功能。

签名方式的兼容性还受证书、描述文件、应用扩展、签名器版本和安装环境影响。AltStore Classic 和 SideStore 可导入预先处理的 IPA，仍需真机验证对应版本是否正确重签内嵌 dylib。LiveContainer 需要单独验证其加载器和虚拟宿主识别。[AltStore Classic](https://faq.altstore.io/altstore-classic/your-altstore) · [SideStore](https://docs.sidestore.io/docs/faq) · [Apple 签名说明](https://developer.apple.com/documentation/technotes/tn3125-inside-code-signing-provisioning-profiles)

## 开关与适配位置

| 开关 | 入口 |
| --- | --- |
| 液态玻璃底栏 / 系统模糊底栏 | `SSTabBarController` / `SSTabBar` |
| 开屏广告 | `SSReadingAdSplashService` |
| 首页 TopView 广告 | `SSBizVideoAdTopviewService` |
| 短剧信息流广告 | `SSBizVideoAdCommonAdClient`、`SSShortVideoMUnionAdService`、`SSShortVideoAdService` |
| 短剧评论区广告 | `SSShortVideoCommentAdService` |
| 暂停广告 | `SSBizVideoPauseAdServiceImpl`、`SSBizVideoPauseAdManager` |
| 播放插播广告 | `SSBizVideoInVideoPatchAdServiceImp`、`SSBizVideoPatchAdService` |
| 启动电商推广 | `SSStartUpEcomQCPXPopupOperation` |
| 商业营销弹窗 | `SSBizMonetizationAdPopupOperation` |
| 福利红包弹窗 | `SSWelfareRedPacketPopupAdapter` |
| 书城底部广告 | `SSBizAdBookShopBannerMaterialService` |
| 领券悬浮挂件 | `SSECommBookCouponPendantManager` |
| 小游戏悬浮挂件 | `SSBookStorePendantManager` |
| 福利标签引导气泡 | `SSWelfareTabGuideTipManager` |
| 短剧满意度问卷 | `SSShortVideoSatisfactionSurveyManager` |

底栏使用单一系统材质背景，清理嵌套内容容器与已确认的原背景，保留标签按钮、切换、红点和触摸区域。iOS 26+ 且主 App 明确设置 `UIDesignRequiresCompatibility=false` 时使用 `UIGlassEffect` Regular 胶囊材质；旧系统、兼容标志为 `true` 或缺少明确 `false` 配置时使用 UIKit 模糊材质。主列表延伸到底栏背后并补足底部留白；视频页按宿主白字选择深色材质。首帧、布局重置、背景重建和移窗有同步处理，离开时恢复插件拥有的状态。

初次改动的范围见 [1.0.1 修复记录](docs/REPAIR_1.0.1.md)，背景写回证据见 [1.0.2 历史记录](docs/REPAIR_1.0.2.md)，透明空壳的失败证据见 [1.0.3 验证记录](docs/REPAIR_1.0.3.md)，打包与回退策略及实际验证数据见 [1.0.4 记录](docs/REPAIR_1.0.4.md)，本轮圆角与验证修订见 [1.0.5 记录](docs/REPAIR_1.0.5.md)。设计兼容键的作用以 [Apple 官方说明](https://developer.apple.com/documentation/bundleresources/information-property-list/uidesignrequirescompatibility) 为准。广告和营销开关处理上表列出的本地展示分支。服务端实验、同一广告位的其他入口和未识别的推广仍需逐项补证。付费、激励解锁、系统通知权限与防沉迷流程不属于上表。

## 版本约定

产品名称固定为 **Honggo**，设置标题不附加其他名称。分发文件使用 `Honggo-<版本>.dylib`，例如 `Honggo-1.0.6.dylib`。宿主版本 **7.4.3 / 7.4.3.33** 与插件版本分别管理。

`src/HonggoVersion.h` 是插件版本的唯一来源。设置页面、启动日志、构建产物名和 Mach-O 版本均从它读取。每次修改插件源码、设备构建参数或 IPA 注入逻辑，都必须递增 `MAJOR.MINOR.PATCH`；CI 与更新前提交比较，未递增或版本回退会直接失败。仅补充文档、测试或 CI 验证的提交可沿用正在验证的插件版本。版本格式同时满足 Mach-O 的数值限制。

## 构建与交接

```sh
python3 tools/version.py --check
bash tools/build-ios.sh
python3 -B -m unittest discover -s tests -v
bash tools/test-settings-ios.sh
bash tools/test-tabbar-ios.sh
```

- `tools/test-settings-ios.sh` 需要 macOS、Xcode 和已安装的 iOS Simulator runtime。它使用真实 UIKit 与模拟的宿主设置控制器验证菜单注入、重复进入、原有页头保留、菜单点击和开关存储；结果不等于红果短剧真机验收。GitHub Actions 会执行版本递增校验、设备 dylib 编译、UIKit 设置入口与底栏回归，以及 IPA 工具测试。模拟器报告、截图与日志作为独立验证附件上传。
- `src/TabBar.m` 管底栏和材质。`src/Settings.m` 管设置入口和开关。`src/Ads.m` 集中保存广告营销类、选择器、ABI 与返回值。更新宿主时先从目标主程序重新核对 Objective-C 方法与调用点，再更新构建号和测试矩阵。
- 插件用 Objective-C runtime 安装 hook，检查方法编码。重签工具更改 bundle identifier 时，插件仍通过主程序名、构建号和底栏类确认宿主。插件链接 Apple 系统库，安装名 `@rpath/Honggo.dylib`，Mach-O `current_version` 使用插件版本。
- 此前核对的原始 IPA SHA-256 为 `37118984f159cc2ac8e30654d35196ea31dbf0e829b2bf081df9fd00ee74b036`。在 GitHub Actions 下载 `Honggo-1.0.6` 时，以附带的 `VERSION`、`source-commit.txt` 和 `SHA256SUMS.txt` 确认版本及来源。GitHub 构建只接触本仓库源码，不会生成或提供真实红果 IPA；待签名 IPA 需要原始宿主包和预处理工具。
- 真机回归先保持所有开关关闭，验证冷启动、进入设置、首页、搜索、登录、连续播放和切集，同时检查预处理后主 App 的全局设计变化。然后逐个开关检查广告触发和恢复。iOS 26 分别检查首页、播放器返回、横竖屏、浅色和深色；在 iOS 15–18 复查模糊材质。分别记录直接注入的模糊回退和预处理 IPA 的原生玻璃两条路径。设备日志统一用 `[Honggo]` 筛选，初始化日志包含插件版本。

实现方法参考 FanqieX 对单一玻璃背景层和首次布局的处理、Apple UIKit 的公开玻璃 API，以及 FanqieHook 的广告位分类。宿主适配点来自这份 7.4.3 IPA 的方法元数据和调用点检查。[FanqieX 1.6.2 交接](https://github.com/SOLHK/FanqieX/blob/f23fbc7af2ac58123569a9be23ef27f0b5d7fdf2/docs/REPAIR_1.6.2.md) · [Apple UIKit](https://developer.apple.com/videos/play/wwdc2025/284/) · [FanqieHook](https://github.com/afwfv/FanqieHook)
