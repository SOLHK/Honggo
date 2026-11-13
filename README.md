# Honggo

R2 在 R1 基础上补齐小游戏信息流与剧集结束插卡两个入口，总计 45 个。用户日志证明双开后缀 `com.phoenix.video1` 没有阻止 R1 加载或安装规则。详见 [R2 修复与双开核查](docs/REPAIR_1.1.15-R2.md)；完整索引与 R1 证据见 [7.4.3.33 广告逆向档案](docs/AD_AUDIT_7.4.3.33.md)。

**插件版本：1.1.15-R2（广告过滤测试版）。** 支持红果短剧 7.4.3 / 7.4.3.33（Eggplant），arm64，最低 iOS 15.0.1。

保留已有底栏、按钮、设置和现场诊断。新规则在宿主判断/生成插入模型处拦截；未用全局隐藏视图或自动滑动处理广告。仅提供 dylib。**截图对应的准确弹窗类与真实 iPhone 消失效果仍需验收；静态候选清单不等于所有候选都是广告或已全部消除。**

## 安装与使用

1. 在签名工具中移除旧 Honggo / HongguoGlass 插件，只添加 `Honggo-1.1.15-R2.dylib`，随红果短剧 7.4.3 / 7.4.3.33 重签安装。插件不是独立 App。
2. 打开「我的 → 设置 → Honggo」，确认「隐藏短剧广告、直播和小游戏插播」「隐藏商业与直播领券弹窗」「隐藏播放插播广告」「隐藏启动电商推广」开启，再完全退出重开。沿用旧开关键，已开启的偏好会保留；新安装所有功能默认关闭。玻璃开关与广告规则独立。
3. iOS 26+ 的此版本在宿主身份校验通过后、插件首次使用 UIKit 前，向本进程的 `NSArgumentDomain` 设置 `com.apple.SwiftUI.IgnoreSolariumOptOut=true`，请求启用系统玻璃。无需先改主 App 的 `UIDesignRequiresCompatibility`；此路径的 iOS 27.3 真机渲染仍待验收。底栏效果使用 `UIGlassEffect` Regular，旧系统继续使用 UIKit 模糊。
4. 「原生液态玻璃按钮」独立控制已适配的宿主按钮。任一玻璃开关打开都会请求新的系统设计，这可能影响其他系统控件。关闭两个玻璃开关并重启后，插件不再设置该进程参数；移除插件也不会留下这个参数。若宿主 Info.plist 本来就开启新设计，插件不会撤销宿主配置。
5. 先测试“我的”页从顶部到历史列表中段、底部再返回顶部，以及四个底栏标签的点击。最后一项内容结束后的正常留白仍会存在。若仍有遮罩，打开增强诊断、复现后导出日志；新的 `glass.startup`、`nestedCoordinator`、`frameInRoot` 和 `bottomEdgeHidden` 字段用于区分启动配置、内层裁切和系统边缘效果。

保留原有设置菜单、导航栏入口和双指长按底栏入口。广告开关修改后完全退出再重开。测试时检查连续切集、前后滑动、主动进入直播、返回播放器及弹窗遮罩是否正常。

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

产品名称固定为 **Honggo**。测试版从 `1.1.15-R1` 开始，继续迭代使用 `-R2`、`-R3`；只有发布到 `main` 的正式版才去掉 R 后缀。设置、日志、标签和分发文件都保留后缀，例如 `Honggo-1.1.15-R1.dylib`。宿主版本独立管理。

`src/HonggoVersion.h` 是版本唯一来源。`tools/version.py --macho` 派生数值 `1.1.15` 供 Mach-O 链接；显示版本仍是 `1.1.15-R1`。源码或打包变化必须递增版本，排序为 `R1 < R2 < 无后缀正式版`。仅文档、测试或 CI 验证提交可沿用正在验证的版本。`--channel test` 要求 R 后缀，`--channel main` 要求无后缀。

## 构建与交接

```sh
python3 tools/version.py --check
bash tools/build-ios.sh
python3 -B -m unittest discover -s tests -v
bash tools/test-settings-ios.sh
bash tools/test-tabbar-ios.sh
bash tools/test-ad-scene-ios.sh
bash tools/test-ads-ios.sh
```

- `tools/test-settings-ios.sh` 需要 macOS、Xcode 和已安装的 iOS Simulator runtime。它使用真实 UIKit 与模拟的宿主设置控制器验证菜单注入、重复进入、原有页头保留、菜单点击和开关存储；结果不等于红果短剧真机验收。GitHub Actions 会执行版本递增校验、设备 dylib 编译、UIKit 设置入口与底栏回归，以及 IPA 工具测试。模拟器报告、截图与日志作为独立验证附件上传。
- `src/TabBar.m` 管底栏和材质。`src/Settings.m` 管设置入口和开关。`src/Ads.m` 集中保存广告营销类、选择器、ABI 与返回值。`src/AdSceneDiagnostics.m` 只负责截屏触发的可见现场采集，不能据此宣称新增广告拦截。更新宿主时先从目标主程序重新核对 Objective-C 方法与调用点，再更新构建号和测试矩阵。
- 插件用 Objective-C runtime 安装 hook，检查方法编码。重签工具更改 bundle identifier 时，插件仍通过主程序名、构建号和底栏类确认宿主。插件链接 Apple 系统库，安装名 `@rpath/Honggo.dylib`，Mach-O `current_version` 使用插件版本。
- 原始 IPA SHA-256 为 `37118984f159cc2ac8e30654d35196ea31dbf0e829b2bf081df9fd00ee74b036`。远程产物以附带的 `VERSION`、`source-commit.txt` 和 `SHA256SUMS.txt` 核对来源。设备构建等待设置、底栏、广告现场诊断和广告过滤行为检查通过。GitHub 构建不生成真实红果 IPA，也不代表真机验收。
- 真机回归先保持所有开关关闭，验证冷启动、进入设置、首页、搜索、登录、连续播放和切集，再检查开启玻璃后的全局系统设计变化。然后逐个开关检查广告触发和恢复。iOS 26 分别检查首页、播放器返回、横竖屏、浅色和深色；在 iOS 15–18 复查模糊材质。分别记录直接注入的启动参数路径、双玻璃开关关闭恢复和旧系统模糊回退。设备日志统一用 `[Honggo]` 筛选，初始化日志包含插件版本。

实现方法参考 FanqieX 对单一玻璃背景层和首次布局的处理、Apple UIKit 的公开玻璃 API，以及 FanqieHook 的广告位分类。宿主适配点来自这份 7.4.3 IPA 的方法元数据和调用点检查。[FanqieX 1.6.2 交接](https://github.com/SOLHK/FanqieX/blob/f23fbc7af2ac58123569a9be23ef27f0b5d7fdf2/docs/REPAIR_1.6.2.md) · [Apple UIKit](https://developer.apple.com/videos/play/wwdc2025/284/) · [FanqieHook](https://github.com/afwfv/FanqieHook)
