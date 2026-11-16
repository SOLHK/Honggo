# Honggo 1.1.15-R16 — 合并 R15 与本地 R5 广告适配

## 源码来源

- 已发布 R15：`0708b028db5f0dd1a28e15ecdca81ed50817c0d1`，标签 `v1.1.15-R15`。
- 截图中的本地 R5：`6d124f7fb30ea0375f1a85ebd1f49d03a943088f`，原 dylib 为 282424 字节，SHA-256 `8aa7108d4bc09291e06ad982e3bb5d2d1c77539334ce814fbc2e0a23196b5b18`。
- 共同祖先为 R3：`770112cb9bf7d7ce5caa97afebedb206909836a7`。

本地广告 R5 与远程外观 R5 是不同开发线的同名版本。原来的外观 R4/R5 文档保留；本地广告文档分别保存为 [LOCAL_AD_REPAIR_1.1.15-R4.md](LOCAL_AD_REPAIR_1.1.15-R4.md) 和 [LOCAL_AD_REPAIR_1.1.15-R5.md](LOCAL_AD_REPAIR_1.1.15-R5.md)，相关宿主证据一并合并。

## 合并行为

保留 R15 的全部外观、配置、播放布局和诊断模块。在广告模块补回两个入口，均由「隐藏短剧广告、直播和小游戏插播」控制：

1. `SSShortVideoFeedDirectGameContentView initWithFrame:` 完整调用原初始化器，保留 ARC 初始化方法的所有权约定，并在专用视图返回宿主前隐藏它。普通列表和短剧视图不使用这个入口。
2. `SSNaturalAdManager tryInsertDataToSaaS:` 仅处理精确的 `SSNaturalAdData`。`forceWatchTime < 1` 时使用宿主 `parseGameDict` 延迟分类，游戏推广返回宿主已有的 `NO`；其他数据与原方法的参数、返回值保持一致。

新增入口沿用 R15 的精确 ABI 检查、能力记录、重复安装与第三方 hook 链保护。原始实现先于 wrapper 发布保存，避免首次并发调用读到空指针。

保留 R8 至 R15 的 `HGForwardPatchPresentation`：原方法负责同步、异步、拒绝和重入分支的回调时序，不恢复本地 R5 旧的 `ready(nil)` 替代。强制观看数据不扩大拦截范围，未合入本地 R6 的后续修改。

## 验证与交付

R16 使用开发分支的定向 CI：固定 Xcode 27.2、iPhoneOS 27.2 与 iOS Simulator 27.2；检查版本递增、Python 工具、广告 UIKit 集成、runtime 安装器，再编译设备 dylib。R15 外观与播放布局源码保持原样，R15 全套 CI 的已有结果不能充作 R16 新增广告入口的结果。

广告回归合并两版用例：初始化与两种挂载顺序、自然推广分类与强制边界、关闭开关、未知类型、重复安装，以及新版贴片同步/延后/拒绝/重入回调。最终执行结果、源码提交与产物 SHA-256 以 R16 交付记录为准。

受控 UIKit 宿主不能替代真实红果 iPhone 验收。安装时移除旧插件，只注入 `Honggo-1.1.15-R16.dylib`，确认短剧广告开关后完全退出重开；复测原推广、连续换集、清屏长按、全集跳转及浮动底栏。出现问题时在现场截屏后回设置导出增强诊断。
