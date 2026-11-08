# Honggo 1.0.1

## 基线和问题

从 `test` 的 `65758841996f7e7ce8187c776032b04fd1b0e2c5` 继续，保留上一版设置菜单、15 个功能开关和既有广告适配。插件独立版本首次明确为 `1.0.1`；红果短剧 `7.4.3 / 7.4.3.33` 是宿主版本。

用户提供的三张截图显示：底栏是一条实心白色胶囊，视频页白色文字无法辨认，列表底部还有遮挡区域。截图不能单独确定运行时每个视图的类和背景来源；以下根因来自旧源码，并通过受控 UIKit 宿主复现验证。

旧 `TabBar.m` 把 `UIVisualEffectView` 放在底栏 index 0，只清除 `bar.backgroundColor`，仅隐藏四个 getter 返回且 `superview == bar` 的背景。`contentView`、中间容器和嵌套图片仍能以实心填充覆盖玻璃。旧代码没有处理主列表在底栏上方结束的 viewport，也没有处理底栏自身的挂窗、布局和后插入背景；只依赖控制器布局及两个宿主回调。

## 改动

- **原生材质：** 支持时使用公开 `UIGlassEffect` 的 Regular 样式；旧系统回退 `UIBlurEffectStyleSystemMaterial`。不伪造玻璃纹理、不改变宿主全局 `UIDesignRequiresCompatibility`。
- **材质生命周期：** 自有 `HGGlassBackdrop` 在已有有效 window、最终尺寸和可见布局时安装效果。圆角和几何先配置，再挂载；移窗前清效果，返回时重装，普通布局复用同一材质。背景效果及内容容器保持 alpha 1。
- **背景处理：** 清除已确认内容路径和大容器的实心填充，处理嵌套的已知背景；不递归隐藏文字、按钮、图像内容、小红点。保留宿主按钮父层级、action、选择状态和触摸区域。
- **前景对比：** 根据宿主当前按钮文字的明暗设置玻璃自己的主题，视频页白字对应深色材质；不改变宿主整体主题。
- **滚动遮罩：** 仅检查已识别底栏控制器下、与底栏相邻、横向覆盖主页面且末端接近底栏或安全区的内容 viewport，延伸到窗口底部。拒绝小卡片、单元格、横向轮播、带变换或 mask 的不安全分支。
- **列表末项：** 扩展视窗同时增加底部可滚动留白和滚动条留白，扣除 UIKit 新增的自动安全区量。只拥有 bottom 分量，宿主后来修改 top/left/right 不会使底部重复累加。支持时隐藏已确认主列表的公开 `bottomEdgeEffect`。
- **恢复：** 保存背景和几何，离窗/隐藏时有条件恢复仍由插件拥有的值；嵌套 viewport 按父到子恢复，避免 autoresizing 把子视图缩小两次。
- **范围：** 全局 hook 只针对宿主 `SSTabBarController`/`SSTabBar`。需要跟踪的具体容器使用局部实例观测子类；没有全局替换 `UIView` 或 `UIScrollView` 的方法。系统材质的内部子视图不进入遍历。
- **名称与版本：** 名称准确为 Honggo；设置页、日志、文件名和 Mach-O current_version 使用同一版本头。安装名为 `@rpath/Honggo.dylib`，分发名为 `Honggo-1.0.1.dylib`。保留旧偏好键以免丢失已有开关。
- **更新约束：** CI 检查插件源码或打包逻辑变化时版本必须递增。注入工具拒绝同时加载旧名或其他版本的 Honggo。

## 验证方法

```sh
python3 -B -m unittest discover -s tests -v
python3 tools/version.py --check --base 65758841996f7e7ce8187c776032b04fd1b0e2c5
bash tools/build-ios.sh
bash tools/test-settings-ios.sh
bash tools/test-tabbar-ios.sh
```

Python 测试验证 IPA 操作和版本规则。设备构建使用真实 Apple SDK，启用 `-Wall -Werror`；构建日志记录实际 Xcode/SDK 版本。设置测试与底栏测试在真实 UIKit 模拟器中运行，但宿主是受控的 ABI/层级模型，并非红果 IPA。

底栏测试针对嵌套背景、首帧、后添加背景、布局复用、深浅色、滚动到底、浮点边界、恢复、跨窗口、非目标视图、按钮和小红点。另一个独立进程验证开关关闭时保持宿主行为。相同测试还运行旧版 `6575884` 的真实 `TabBar.m`：必须检测出嵌套白底、嵌套旧背景和视窗未延伸至少三项失败，避免只有“存在一个 effect view”即可通过的无效测试。

实际结果以交付提交对应的 GitHub Actions 和附件 JSON 为准。没有红果真机在此环境运行；最终材质浓度、视频 GPU 合成、手势以及用户设备上的短暂遮罩仍需要安装新版后观察。系统“减少透明度”、增强对比度和玻璃显示偏好由 UIKit 尊重，插件不会强行覆盖。

## 参考与采用边界

- [FanqieX 1.6.2](https://github.com/SOLHK/FanqieX/blob/f23fbc7af2ac58123569a9be23ef27f0b5d7fdf2/docs/REPAIR_1.6.2.md)：保留单一 Regular 底座、首帧先配置后挂载、浮点容差、viewport/留白恢复。未复制其二进制补丁或沿用其未完成的真机验收结论。
- [Apple：Build a UIKit app with the new design](https://developer.apple.com/videos/play/wwdc2025/284/)：UIGlassEffect、胶囊形状、contentView、动态颜色及效果动画。
- [Apple：UIVisualEffectView](https://developer.apple.com/documentation/uikit/uivisualeffectview/)：alpha 和祖先 mask 的合成限制。这里保留宿主控件层级，用单独的背景效果，而没有把原控件直接搬进 contentView。
- [Apple：UIGlassEffect](https://developer.apple.com/documentation/uikit/uiglasseffect?language=objc)：公开 Objective-C 接口；iOS 26 起可用。
- [Expo：GlassView.swift](https://github.com/expo/expo/blob/main/packages/expo-glass-effect/ios/GlassView.swift)：进入 window 后在布局安装、离窗清效果、重新进入时重新布局的方法。
- [Callstack：LiquidGlassView.swift](https://github.com/callstack/liquid-glass/blob/main/ios/LiquidGlassView.swift)：原生效果可用性、可见性与挂载生命周期管理。仅学习方法，未复制项目源码。
