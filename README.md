# Honggo · 红果短剧插件

支持 **红果短剧 7.4.3（7.4.3.33）**，主程序 `Eggplant`，原包标识 `com.phoenix.video`，设备架构 `arm64`，部署目标 iOS 15.0.1。当前是 `test` 分支的真机验收候选。`HongguoGlass.dylib` 可以单独注入。仓库包含插件源码、构建脚本和本地 IPA 注入工具。

## 安装与使用

1. 使用这次核对过的红果短剧 7.4.3 IPA。待签名的宿主主程序需要为 iOS arm64 且 `cryptid=0`。
2. **支持 dylib 注入的签名器**可以把 `HongguoGlass.dylib` 添加到原 IPA 后签名。Feather 和 Sideloadly 都有添加 dylib 的入口。先用单个插件做基线，便于定位签名或注入问题。[Feather](https://feather.claration.dev/) · [Sideloadly](https://github.com/SideloadlyiOS/Sideloadly-Download)
3. **只接收 IPA 的签名器**先在电脑上运行以下命令，再导入生成的待签名 IPA。签名器需能为内嵌 dylib 和应用一并重签。源 IPA 不会被覆盖。工具遇到加密包、其他构建号、重复注入或没有足够的 Mach-O 头部空间时会退出。

   ```text
   python tools/prepare_ipa.py "红果短剧_7.4.3.ipa" HongguoGlass.dylib Honggo-unsigned.ipa
   ```

4. 安装后打开 **「我的 → 设置 → Honggo 插件设置」**，点击设置列表顶部的菜单，即可看到液态玻璃、广告与弹窗的 15 个开关。导航栏的「Honggo」按钮和双指长按底栏约一秒仍可打开同一个设置页。开关默认关闭，每次改动后从应用切换器彻底退出并重开红果。关闭对应开关并重启可恢复。若启动即退出，使用原 IPA 重新签名安装可撤除插件。

签名方式的兼容性还受证书、描述文件、应用扩展、签名器版本和安装环境影响。AltStore Classic 和 SideStore 可导入预先处理的 IPA，仍需真机验证对应版本是否正确重签内嵌 dylib。LiveContainer 需要单独验证其加载器和虚拟宿主识别。[AltStore Classic](https://faq.altstore.io/altstore-classic/your-altstore) · [SideStore](https://docs.sidestore.io/docs/faq) · [Apple 签名说明](https://developer.apple.com/documentation/technotes/tn3125-inside-code-signing-provisioning-profiles)

## 开关与适配位置

| 开关 | 入口 |
| --- | --- |
| 液态玻璃底栏 | `SSTabBarController` / `SSTabBar` |
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

底栏只替换宿主背景层，保留标签按钮、切换、红点和触摸区域。iOS 26+ 尝试使用 `UIGlassEffect` 胶囊材质，旧系统用 UIKit 模糊材质。宿主启用了 `UIDesignRequiresCompatibility`，本插件保留这个全局选项，需在真机核对系统玻璃的实际渲染及明暗对比。广告和营销开关处理上表列出的本地展示分支。服务端实验、同一广告位的其他入口和未识别的推广仍需逐项补证。付费、激励解锁、系统通知权限与防沉迷流程不属于上表。

## 构建与交接

```sh
bash tools/build-ios.sh build/HongguoGlass.dylib
python3 -B -m unittest discover -s tests -v
bash tools/test-settings-ios.sh
```

- `tools/test-settings-ios.sh` 需要 macOS、Xcode 和已安装的 iOS Simulator runtime。它使用真实 UIKit 与模拟的宿主设置控制器验证菜单注入、重复进入、原有页头保留、菜单点击和开关存储；结果不等于红果短剧真机验收。GitHub Actions 会同时执行设备 dylib 编译、模拟器设置入口验证和 IPA 工具测试。
- `src/TabBar.m` 管底栏和材质。`src/Settings.m` 管设置入口和开关。`src/Ads.m` 集中保存广告营销类、选择器、ABI 与返回值。更新宿主时先从目标主程序重新核对 Objective-C 方法与调用点，再更新构建号和测试矩阵。
- 插件用 Objective-C runtime 安装 hook，检查方法编码。重签工具更改 bundle identifier 时，插件仍通过主程序名、构建号和底栏类确认宿主。插件链接 Apple 系统库，安装名 `@rpath/HongguoGlass.dylib`。
- 已检查的原始 IPA SHA-256 为 `37118984f159cc2ac8e30654d35196ea31dbf0e829b2bf081df9fd00ee74b036`。在 GitHub Actions 下载 `HongguoGlass-iPhoneOS` 时，以附带的 `source-commit.txt` 和 `SHA256SUMS.txt` 确认来源。GitHub 构建只接触本仓库源码。
- 真机回归先保持所有开关关闭，验证冷启动、进入设置、首页、搜索、登录、连续播放和切集。然后逐个开关检查广告触发和恢复。iOS 26 分别检查首页、播放器返回、横竖屏、浅色和深色；在 iOS 15–18 复查模糊材质。分别记录直接注入与预注入 IPA 两条签名路径。设备日志用 `[HongguoGlass]` 筛选广告入口数量与签名不匹配的选择器，用 `[Honggo]` 筛选设置菜单与初始化状态。

实现方法参考 FanqieX 对单一玻璃背景层和首次布局的处理、Apple UIKit 的公开玻璃 API，以及 FanqieHook 的广告位分类。宿主适配点来自这份 7.4.3 IPA 的方法元数据和调用点检查。[FanqieX 1.6.2 交接](https://github.com/SOLHK/FanqieX/blob/f23fbc7af2ac58123569a9be23ef27f0b5d7fdf2/docs/REPAIR_1.6.2.md) · [Apple UIKit](https://developer.apple.com/videos/play/wwdc2025/284/) · [FanqieHook](https://github.com/afwfv/FanqieHook)
