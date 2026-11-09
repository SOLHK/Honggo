# Honggo

当前版本提高底栏文字对比度并适度加粗，保留宿主角标。红点来源核查及验证边界见 [1.1.10 说明](docs/REPAIR_1.1.10.md)。本轮已获用户授权推送 test。复现后截屏并立即导出 JSON，导出前不要退出应用。

**插件版本：1.1.10（底栏文字优化版）。** 支持红果短剧 7.4.3 / 7.4.3.33（Eggplant），arm64，最低 iOS 15.0.1。

本地测试版修复 1.1.5 移除滚动回调后遗漏个人页晚加载和设置返回的处理入口，接入个人页自身布局及出现回调。仍需真机确认滚动和返回遮罩。变更、证据和验证边界见 [1.1.6 说明](docs/REPAIR_1.1.6.md)。本轮仅交付 dylib，未推送 GitHub，也未制作注入 IPA。

## 安装与使用

1. 在签名工具中移除旧 Honggo / HongguoGlass 插件，只添加 `Honggo-1.1.6.dylib`，随红果短剧 7.4.3 / 7.4.3.33 重签安装。插件不是独立 App。
2. 打开「我的 → 设置 → Honggo」，开启「液态玻璃底栏」，完全退出应用后重开。旧版本已经打开的偏好会保留；新安装所有功能默认关闭。
3. iOS 26+ 的此版本在宿主身份校验通过后、插件首次使用 UIKit 前，向本进程的 `NSArgumentDomain` 设置 `com.apple.SwiftUI.IgnoreSolariumOptOut=true`，请求启用系统玻璃。无需先改主 App 的 `UIDesignRequiresCompatibility`；此路径的 iOS 27.3 真机渲染仍待验收。底栏效果使用 `UIGlassEffect` Regular，旧系统继续使用 UIKit 模糊。
4. 「原生液态玻璃按钮」独立控制已适配的宿主按钮。任一玻璃开关打开都会请求新的系统设计，这可能影响其他系统控件。关闭两个玻璃开关并重启后，插件不再设置该进程参数；移除插件也不会留下这个参数。若宿主 Info.plist 本来就开启新设计，插件不会撤销宿主配置。
5. 先测试“我的”页从顶部到历史列表中段、底部再返回顶部，以及四个底栏标签的点击。最后一项内容结束后的正常留白仍会存在。若仍有遮罩，打开增强诊断、复现后导出日志；新的 `glass.startup`、`nestedCoordinator`、`frameInRoot` 和 `bottomEdgeHidden` 字段用于区分启动配置、内层裁切和系统边缘效果。

保留原有设置菜单、导航栏入口和双指长按底栏入口。切换功能后完全退出再重开。此轮未修改广告 hook 或其他业务规则。

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

底栏使用单一系统材质背景，保留标签按钮、切换、红点和触摸区域。iOS 26+ 在主包显式选择新设计或插件已设置启动参数时创建 `UIGlassEffect` Regular；旧系统使用 UIKit 模糊。主列表延伸到底栏背后并补足底部留白。`SSNestedContainerScrollView` 和 `SSMyUser637NestedContainerScrollView` 均按嵌套协调器处理，只处理外层视口并由宿主管理子列表，避免滚动过程中的尺寸反馈；隐藏协调器重复的系统上下边缘效果，离开时恢复。Compose 的 UIKitTransparentContainerView 按容器继续遍历，不添加列表底部补偿。初始化日志与效果类只能证明执行路径，不能替代系统合成画面或真机验收。

初次改动的范围见 [1.0.1 修复记录](docs/REPAIR_1.0.1.md)，背景写回证据见 [1.0.2 历史记录](docs/REPAIR_1.0.2.md)，透明空壳的失败证据见 [1.0.3 验证记录](docs/REPAIR_1.0.3.md)，打包与回退策略及实际验证数据见 [1.0.4 记录](docs/REPAIR_1.0.4.md)，本轮圆角与验证修订见 [1.0.5 记录](docs/REPAIR_1.0.5.md)。设计兼容键的作用以 [Apple 官方说明](https://developer.apple.com/documentation/bundleresources/information-property-list/uidesignrequirescompatibility) 为准。广告和营销开关处理上表列出的本地展示分支。服务端实验、同一广告位的其他入口和未识别的推广仍需逐项补证。付费、激励解锁、系统通知权限与防沉迷流程不属于上表。

## 版本约定

产品名称固定为 **Honggo**，设置标题不附加其他名称。分发文件使用 `Honggo-<版本>.dylib`，例如 `Honggo-1.1.6.dylib`。宿主版本 **7.4.3 / 7.4.3.33** 与插件版本分别管理。

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
- 此前核对的原始 IPA SHA-256 为 `37118984f159cc2ac8e30654d35196ea31dbf0e829b2bf081df9fd00ee74b036`。1.1.6 当前只有本地交叉编译测试产物，尚无 GitHub Actions 结果。待用户授权推送后，远程产物应以附带的 `VERSION`、`source-commit.txt` 和 `SHA256SUMS.txt` 确认版本及来源。GitHub 构建只接触本仓库源码，不生成真实红果 IPA。
- 真机回归先保持所有开关关闭，验证冷启动、进入设置、首页、搜索、登录、连续播放和切集，再检查开启玻璃后的全局系统设计变化。然后逐个开关检查广告触发和恢复。iOS 26 分别检查首页、播放器返回、横竖屏、浅色和深色；在 iOS 15–18 复查模糊材质。分别记录直接注入的启动参数路径、双玻璃开关关闭恢复和旧系统模糊回退。设备日志统一用 `[Honggo]` 筛选，初始化日志包含插件版本。

实现方法参考 FanqieX 对单一玻璃背景层和首次布局的处理、Apple UIKit 的公开玻璃 API，以及 FanqieHook 的广告位分类。宿主适配点来自这份 7.4.3 IPA 的方法元数据和调用点检查。[FanqieX 1.6.2 交接](https://github.com/SOLHK/FanqieX/blob/f23fbc7af2ac58123569a9be23ef27f0b5d7fdf2/docs/REPAIR_1.6.2.md) · [Apple UIKit](https://developer.apple.com/videos/play/wwdc2025/284/) · [FanqieHook](https://github.com/afwfv/FanqieHook)
