# Honggo

红果短剧去广告与原生液态玻璃界面增强插件（iOS dylib）。

## 下载插件

**[前往 GitHub Releases 下载 Honggo](https://github.com/SOLHK/Honggo/releases)**

打开最新的 **Pre-release（测试版）**，在 Assets 中下载 `Honggo-版本号.dylib`。下载的是可供注入签名工具使用的插件文件，不是独立安装的 iOS App，也不是 GitHub 的 Source code ZIP。

- **测试版源码和适配说明：** [test 分支](https://github.com/SOLHK/Honggo/tree/test)
- **构建与验证记录：** [GitHub Actions](https://github.com/SOLHK/Honggo/actions/workflows/ios.yml)
- **测试版发布规则：** 只有自动编译、UIKit 回归与插件校验成功后，才会由 GitHub Actions 发布 Pre-release。真机使用体验仍需单独验收。

最新通过自动验证的测试版以 Releases 为准；支持的宿主版本、兼容范围和安装步骤请以 test 分支 README 为准。测试版发布不代表已经通过全部真机验收。

现有测试源码已加入设置搜索、功能开关配置备份/恢复和本次启动兼容状态。参考功能按 Honggo 自身结构独立实现；功能对照、实现范围和后续适配要求见 [NaiwaGuo 功能研究与独立实现说明](https://github.com/SOLHK/Honggo/blob/test/docs/REFERENCE_STUDY_NAIWAGUO.md)。

正式版只有在设备验收后才应另行发布，避免把测试候选标成稳定版。
