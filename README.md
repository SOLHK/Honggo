# Honggo

当前版本补齐播放器、领券弹窗和独立窗口的截屏现场诊断，用于排查短剧流中漏拦截的直播带货推广。**本版尚未增加该直播广告的拦截规则，不能视为已去除广告。** 已有证据、缺失资料与验证边界见 [1.1.13 说明](docs/DIAGNOSTIC_1.1.13.md)。

**插件版本：1.1.13（广告现场诊断版）。** 支持红果短剧 7.4.3 / 7.4.3.33（Eggplant），arm64，最低 iOS 15.0.1。

本版保留 1.1.12 的底栏、按钮、设置及广告规则。新增诊断只在开启增强诊断后截屏时运行，不隐藏视图、不自动滑动、不触发按钮，也不读取图片或文字。仅提供 dylib，不制作注入 IPA。旧版本记录保留历史验证边界；当前实现以本版说明为准。

## 安装与使用

1. 在签名工具中移除旧 Honggo / HongguoGlass 插件，只添加 `Honggo-1.1.13.dylib`，随红果短剧 7.4.3 / 7.4.3.33 重签安装。插件不是独立 App。
2. 打开「我的 → 设置 → Honggo」，开启「液态玻璃底栏」，完全退出应用后重开。旧版本已经打开的偏好会保留；新安装所有功能默认关闭。
3. iOS 26+ 的此版本在宿主身份校验通过后、插件首次使用 UIKit 前，向本进程的 `NSArgumentDomain` 设置 `com.apple.SwiftUI.IgnoreSolariumOptOut=true`，请求启用系统玻璃。无需先改主 App 的 `UIDesignRequiresCompatibility`；此路径的 iOS 27.3 真机渲染仍待验收。底栏效果使用 `UIGlassEffect` Regular，旧系统继续使用 UIKit 模糊。
4. 「原生液态玻璃按钮」独立控制已适配的宿主按钮。任一玻璃开关打开都会请求新的系统设计，这可能影响其他系统控件。关闭两个玻璃开关并重启后，插件不再设置该进程参数；移除插件也不会留下这个参数。若宿主 Info.plist 本来就开启新设计，插件不会撤销宿主配置。
5. 先测试“我的”页从顶部到历史列表中段、底部再返回顶部，以及四个底栏标签的点击。最后一项内容结束后的正常留白仍会存在。若仍有遮罩，打开增强诊断、复现后导出日志；新的 `glass.startup`、`nestedCoordinator`、`frameInRoot` 和 `bottomEdgeHidden` 字段用于区分启动配置、内层裁切和系统边缘效果。

保留原有设置菜单、导航栏入口和双指长按底栏入口。切换功能后完全退出再重开。此轮未修改广告 hook 或其他业务规则。

### 采集漏拦截的直播领券广告

打开「我的 → 设置 → Honggo → 增强诊断」，返回短剧播放器。等直播推广或领券弹窗出现时截屏，然后回设置导出最近一小时日志。不要先退出应用或清除日志。增强诊断开关即时生效，无需重启；广告开关仍按原有方式重启生效。

导出的 `maskScenes` 中新增 `kind: ad.scene`，包含当前可见窗口、控制器、视图结构，以及受数量和容量限制的类方法签名与按钮 action 名。它保留广告现场，即使随后进入设置导出也不会被设置页快照覆盖。该结构不包含截图像素、标签文字、无障碍标识、账号或请求数据。关闭增强诊断后不再采集。旧的个人页遮罩诊断仍独立保留。

历史 `tools/prepare_ipa.py` 仍保留在仓库；本次测试不需要运行它，也没有使用它修改真实 IPA。

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

底栏使用单一系统材质，保留原按钮和红点；仅增强已识别标题的颜色与字重。个人页通过当前列表的有限祖先链调整视觉裁切，保留宿主滚动几何；不再对个人页整树拉伸或增加内层留白。细节见 [1.1.9 记录](docs/REPAIR_1.1.9.md)。其他页仍沿用原有视口适配。

初次改动的范围见 [1.0.1 修复记录](docs/REPAIR_1.0.1.md)，背景写回证据见 [1.0.2 历史记录](docs/REPAIR_1.0.2.md)，透明空壳的失败证据见 [1.0.3 验证记录](docs/REPAIR_1.0.3.md)，打包与回退策略及实际验证数据见 [1.0.4 记录](docs/REPAIR_1.0.4.md)，历史圆角与验证修订见 [1.0.5 记录](docs/REPAIR_1.0.5.md)。设计兼容键的作用以 [Apple 官方说明](https://developer.apple.com/documentation/bundleresources/information-property-list/uidesignrequirescompatibility) 为准。广告和营销开关处理上表列出的本地展示分支。服务端实验、同一广告位的其他入口和未识别的推广仍需逐项补证。付费、激励解锁、系统通知权限与防沉迷流程不属于上表。

## 版本约定

产品名称固定为 **Honggo**，设置标题不附加其他名称。分发文件使用 `Honggo-<版本>.dylib`，例如 `Honggo-1.1.13.dylib`。宿主版本 **7.4.3 / 7.4.3.33** 与插件版本分别管理。

`src/HonggoVersion.h` 是插件版本的唯一来源。设置页面、启动日志、构建产物名和 Mach-O 版本均从它读取。每次修改插件源码、设备构建参数或 IPA 注入逻辑，都必须递增 `MAJOR.MINOR.PATCH`；CI 与更新前提交比较，未递增或版本回退会直接失败。仅补充文档、测试或 CI 验证的提交可沿用正在验证的插件版本。版本格式同时满足 Mach-O 的数值限制。

## 构建与交接

```sh
python3 tools/version.py --check
bash tools/build-ios.sh
python3 -B -m unittest discover -s tests -v
bash tools/test-settings-ios.sh
bash tools/test-tabbar-ios.sh
bash tools/test-ad-scene-ios.sh
```

- `tools/test-settings-ios.sh` 需要 macOS、Xcode 和已安装的 iOS Simulator runtime。它使用真实 UIKit 与模拟的宿主设置控制器验证菜单注入、重复进入、原有页头保留、菜单点击和开关存储；结果不等于红果短剧真机验收。GitHub Actions 会执行版本递增校验、设备 dylib 编译、UIKit 设置入口与底栏回归，以及 IPA 工具测试。模拟器报告、截图与日志作为独立验证附件上传。
- `src/TabBar.m` 管底栏和材质。`src/Settings.m` 管设置入口和开关。`src/Ads.m` 集中保存广告营销类、选择器、ABI 与返回值。`src/AdSceneDiagnostics.m` 只负责截屏触发的可见现场采集，不能据此宣称新增广告拦截。更新宿主时先从目标主程序重新核对 Objective-C 方法与调用点，再更新构建号和测试矩阵。
- 插件用 Objective-C runtime 安装 hook，检查方法编码。重签工具更改 bundle identifier 时，插件仍通过主程序名、构建号和底栏类确认宿主。插件链接 Apple 系统库，安装名 `@rpath/Honggo.dylib`，Mach-O `current_version` 使用插件版本。
- 原始 IPA SHA-256 为 `37118984f159cc2ac8e30654d35196ea31dbf0e829b2bf081df9fd00ee74b036`。本轮已授权推送 test。远程产物以附带的 `VERSION`、`source-commit.txt` 和 `SHA256SUMS.txt` 核对版本及来源。设备构建依赖设置、底栏及广告现场诊断三组回归通过；未通过时不能声称已有验收产物。GitHub 构建不生成真实红果 IPA。
- 真机回归先保持所有开关关闭，验证冷启动、进入设置、首页、搜索、登录、连续播放和切集，再检查开启玻璃后的全局系统设计变化。然后逐个开关检查广告触发和恢复。iOS 26 分别检查首页、播放器返回、横竖屏、浅色和深色；在 iOS 15–18 复查模糊材质。分别记录直接注入的启动参数路径、双玻璃开关关闭恢复和旧系统模糊回退。设备日志统一用 `[Honggo]` 筛选，初始化日志包含插件版本。

实现方法参考 FanqieX 对单一玻璃背景层和首次布局的处理、Apple UIKit 的公开玻璃 API，以及 FanqieHook 的广告位分类。宿主适配点来自这份 7.4.3 IPA 的方法元数据和调用点检查。[FanqieX 1.6.2 交接](https://github.com/SOLHK/FanqieX/blob/f23fbc7af2ac58123569a9be23ef27f0b5d7fdf2/docs/REPAIR_1.6.2.md) · [Apple UIKit](https://developer.apple.com/videos/play/wwdc2025/284/) · [FanqieHook](https://github.com/afwfv/FanqieHook)
