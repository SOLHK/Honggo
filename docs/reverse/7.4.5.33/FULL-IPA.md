# 红果短剧 7.4.5.33 完整 IPA 与首页侧栏分析

2026-11-23 使用用户补充的完整 IPA 复查。本文更新此前对损坏样本的包结构统计，并追踪 Honggo 左上角侧栏点击路径。逐文件类型统计、四个补全条目、17 个 Mach-O 的哈希、加载命令和 REA Evidence ID 见 [full-ipa-evidence.json](full-ipa-evidence.json)。

## 输入校验与 REA 清单

| 对象 | 校验结果 |
| --- | --- |
| 外层 ZIP | 331,085,760 字节，包含一个 IPA，SHA-256 `c56cbde907a0f207b292c83a9e363c319a7b587c17a6dd359b23d1d0544736ba` |
| 完整 IPA | 333,199,100 字节，SHA-256 `ec75df768d68fe5861ab057d53c4dfa1a6d856b908cd06f296826bc5d0fb278c` |
| ZIP 条目 | 4,458 项，含 4,017 个文件和 441 个目录，全部通过 CRC |
| `Eggplant` | `com.phoenix.video` 7.4.5.33，arm64，SHA-256 `40edce7297acca3d4f5fedba2c3299354a5fb3b7841036f4da298403bd711bd7` |
| REA Apple 应用图 | 182 个 bundle、17 个可执行文件、50 项 framework 组件记录、22 个 JavaScript 组件 |

主程序 SHA-256 与此前恢复出的文件完全一致，原来的 Objective-C 方法编码及文件偏移仍适用。这次完整 IPA 补齐了 435 个 ZIP 条目，其中包括主 `Assets.car`、`growth_incentive-monorepo_novel-lite.zst`、`DanceUIRenderBoxShader-iphonesimulator.metallib`、`book-upload.zip` 和此前无法列全的 Frameworks。17 个 arm64 Mach-O 包括主程序、两个扩展和 14 个 framework 可执行文件。

用 [REA 6.7.0](https://github.com/morluto/rea) 的 `inspect-artifact` 和 `project-apple-application-graph` 检查原始完整 IPA。用 REA 调起 [Rizin 0.9.1](https://github.com/rizinorg/rizin/releases/tag/v0.9.1)，完成六个 ARM64 窗口的反汇编，并用主程序的 Objective-C 方法表和 selector 引用标定分支。静态结果覆盖可恢复的包结构和本次入口调用路径。界面调度、渲染和实际触摸行为仍需要在设备上观察。

## 左上角按钮的宿主调用链

| 所属类与方法 | 文件偏移 | 证据 |
| --- | --- | --- |
| `SSVideoSeriesFeedViewController.simpleSideBarViewDidClicked` | `0x105550dc` | 先通知 `notifyDidClickSideBar`，随后进入侧栏切换。 |
| `toggleSidebar:toggleType:` | `0x1058f2cc` | 直接加载 `toggleSidebar:toggleType:duration:` selector，默认传入 `0.3` 秒。 |
| `toggleSidebar:toggleType:duration:` | `0x10554940` | ABI `v36@0:8B16@20d28`。检查当前 `isSidebarOpen`，启动打开或关闭动画并更新状态。 |
| 打开与关闭的动画回调 | `0x10554b28`、`0x10554c38` | 调整宿主 `maskView` 透明度。打开路径调用 `pauseVideoPlayIfNeeded` 与 `sideBarDidShow`，关闭路径对应恢复播放及 `sideBarDidClosed`。 |
| `SSTabBarController.toggleVideoSeriesFeedSideBarInAnimation:` | `0x0fd8c874` | ABI `v24@0:8d16`。设置视图的 `frame` 和 `center`，与侧栏动画一起移动视图。 |

R36 的问题可以定位到插件侧的布局交接。插件延展视频视口时暂时拥有部分视图 frame、滚动 inset、裁剪和播放器层变换。侧栏打开时宿主开始移动这些界面。R36 在调用宿主前只阻止新的底栏刷新，旧几何仍处于应用状态，定时器到期也会在侧栏继续打开时重新应用插件布局。这构成了明确的重叠写入窗口，与截图中的无响应相符。静态分析无法确认真机主线程当时卡在哪个调用。

`1.5.1beta1` 在宿主动手前恢复插件持有的视口和播放层几何。切换期间以及宿主仍报告 `isSidebarOpen` 时，由宿主管理布局。侧栏关闭动画结束后再刷新底栏。点击、侧栏手势、遮罩和播放的打开／恢复调用继续由宿主处理。诊断新增 `appearance.sidebar` 的 `host.enter` 和 `host.return`，用于确认入口是否返回。

真机验收重点为首页反复打开和关闭侧栏、侧栏内滑动、关闭后继续上下刷视频，以及切换 Dock 标签后再次打开侧栏。
