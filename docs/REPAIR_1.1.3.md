# Honggo 1.1.3 本地测试版

基线：`466599abf81b8c54b1863332593aebf6f2d9127d`（GitHub test / 1.1.2）。本轮遵循先交付插件测试、用户明确要求后才推送的约定。未推送分支、未调用远程 CI、未生成或改动真实 IPA。

## 设备证据与修复

用户的 1.1.2 日志来自 Eggplant 7.4.3.33、iOS 27.3（24C5037i）。兼容配置为 true，减少透明度和增强对比度均为 false；所有已记录底栏布局的材质为 UIBlurEffect。因此旧版普通模糊是代码选择结果，不能用 UIGlassEffect 类存在来解释为已启用原生玻璃。

1.1.3 在已校验宿主的 constructor 中，且在插件调用 UIKit 前，为任一开启的玻璃功能设置进程内的 `com.apple.SwiftUI.IgnoreSolariumOptOut`。使用 `NSUserDefaults` 的 volatile `NSArgumentDomain`，保留其他参数，不写入持久域、不拦截 NSBundle、不修改 Info.plist。两个开关都关闭或移除插件后，下一进程沿用宿主自身配置。系统样式是进程级设置，可能影响其他系统控件。

机制依据是 [insidegui 的 LiquidGlassFlag 一手示例](https://github.com/insidegui/LiquidGlassFlag)：系统在启动早期缓存该键；本实现将其限制在进程参数域。该键不是 Apple 公开承诺的 UIKit API，iOS 27.3 是否尊重该域以及宿主其他依赖是否更早缓存设计状态，必须由设备测试确定。`startupOverride=true` 和 `material=UIGlassEffect` 只能证明本插件请求了配置并创建该效果，不能单凭日志声称折射已渲染。

第二处证据是 `SSMyUser637NestedContainerScrollView` 从高度 872 延长到 956，同时被加上 84pt contentInset，内层列表没有出现在记录中。重新读取原始宿主 Objective-C 元数据确认它与 `SSNestedContainerScrollView` 均直接继承 UIScrollView，前者不是后者的子类。两类均提供 `contentView @16@0:8`。旧代码只识别后者，从而给前者加了外层底部留白并提前结束遍历。

修复将两者都识别为嵌套协调器，遍历其 contentView 下的真实列表，仅为内层分配底部补偿；两层的 bottomEdgeEffect 都隐藏并在退出时恢复。不会改写滚动内容的高度或复制内容。真实 Compose 内容及滚到底的视觉效果仍需真机复核；内容已经结束后的正常留白不会被填成伪造内容。

增加的诊断字段包含启动配置、嵌套分类、内容大小、根视图坐标、底边效果状态，不记录账号或正文。未提交用户原始日志和截图。

## 实际验证

- Windows 上使用现有 Zig 0.15.2 / Clang 20.1.2、iPhoneOS 16.5 SDK 声明及 Rust LLD 23.1.1 完成全部 6 个插件模块的 ARM64 编译与动态库链接，`-Wall -Werror` 通过。iOS 26 API 按运行时选择器调用；这不是 Xcode 26/27 SDK 构建。
- Mach-O 验证：arm64、iOS device、最低 15.0.1、current_version 1.1.3、安装名 `@rpath/Honggo.dylib`、仅系统依赖。产物未签名，需要随宿主重签。
- 实际产物的 37 项 ARM64 指令仿真检查通过（`tests/check_arm64_startup.py`）：独立开关、旧系统、双关闭、缺省/已存在 true/false 偏好、重复初始化、保留无关参数、持久域不变、两种嵌套类及普通列表分类。ObjC/Foundation 边界由 fixture 模拟，**不属于 UIKit 运行或玻璃画面验证**。
- 现有 Python 工具测试 20/20 通过，版本递增检查与 diff whitespace 检查通过。工具测试仅使用合成归档，没有注入用户 IPA。
- 4 个 UIKit/Foundation 测试源文件均完成 ARM64 编译；更新的底栏测试脚本通过 bash 语法检查。新增真实 UIKit 测试覆盖独立的个人页嵌套类、双层底边效果、1.1.2 负对照、保留原 plist 的启动路径及系统截图像素对照。**这些 UIKit 测试未在本轮执行**，当前机器无 macOS/iOS Simulator，且未获准推送，因此没有启动 GitHub Actions。
- 未完成真机签名安装、滚动视觉、触摸、播放器返回、深浅模式及 iOS 27.3 系统合成验证。此交付用于用户测试，不能标记为最终验收通过。

## 测试方法

在签名工具中移除旧 Honggo/HongguoGlass，仅注入此版 dylib 并重新签名安装。开启底栏玻璃后彻底退出重开。原来已经开启则保留偏好，直接冷启动。重点看“我的”页中段、底部及返回顶部，四个标签的点击与文字，浅深模式，以及关闭两个玻璃开关并重启后的恢复。

若仍有问题，打开增强诊断、复现、导出 JSON。新日志应显示 `glass.startup.processOverride=true`，底栏请求材质为 `UIGlassEffect`；个人页外层应为 `nestedCoordinator=true` 且不再由插件补外层 84pt 留白。是否已呈现正确液态玻璃，以设备画面和操作为准。
