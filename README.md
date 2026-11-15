# Honggo

红果短剧去广告与原生液态玻璃界面增强插件（iOS dylib）。

## 下载插件

**[前往 GitHub Releases 下载 Honggo](https://github.com/SOLHK/Honggo/releases)**

打开最新的 **Pre-release（测试版）**，在 Assets 中下载 `Honggo-版本号.dylib`。这是注入签名工具使用的插件文件，不是独立 iOS App，也不是 Source code ZIP。升级时移除旧的 Honggo / HongguoGlass，仅注入一份新插件。

- **测试版源码和适配说明：** [test 分支](https://github.com/SOLHK/Honggo/tree/test)
- **构建与验证记录：** [GitHub Actions](https://github.com/SOLHK/Honggo/actions/workflows/ios.yml)
- **发布规则：** 自动编译、外观与原有 UIKit 回归、配置校验和插件检查全部成功后，才由 Actions 发布 Pre-release。真实红果 App 的使用体验仍需真机验收。

最新通过自动验证的测试版以 Releases 为准。源码已经增加某功能，不代表对应安装包已发布；请核对发布页的版本、`source-commit.txt` 和 `SHA256SUMS.txt`。

## 新增外观定制

`test` 分支的 1.1.15-R5 源码现已加入：

- 自定义悬浮底栏：材质、颜色、尺寸、图标/文字/图文、项目显隐、触感反馈，独立圆形按钮，上滑收缩和下滑/点击展开。
- 播放页：右侧、顶部及全集入口玻璃；已适配控件的隐藏、位移和缩放；竖屏画面填满；评论、分享、长按/更多和倍速面板材质。
- 图标与字体：动作图标预设、SF Symbols 和相册图片，真实点赞/收藏状态；已安装字体选择、TTF/OTF/TTC 导入和原生文字字号调整。
- 管理：玻璃/专注预设、设置搜索、外观参数备份/恢复及恢复默认。保留原有去广告开关、诊断导出和兼容状态。

### 安装后如何看到变化

进入 **我的 → 设置 → Honggo → 左上角「外观」→ 应用「玻璃外观」预设**，确认后完全退出红果再打开。也可以在各分组中单独开启和调整。

新外观默认关闭，不会擅自覆盖旧的去广告选择。只替换 dylib 而未开启新外观时，界面可能仍与以前相同。

### 兼容范围与边界

当前适配基线仍为红果短剧 **7.4.3 / 7.4.3.33（Eggplant）**，arm64，最低编译目标 iOS 15.0.1。系统或启动条件不满足原生玻璃时使用磨砂回退。

未匹配的宿主接口保留原行为。横屏和方形视频不强行放大；作者与剧集信息按已确认容器整体调整；字体不修改视频内字幕、网页或自绘内容。部分自绘面板背景仍需真机确认。外观参数备份不包含字体和图片文件。

具体说明见 [R5 实现范围与使用边界](https://github.com/SOLHK/Honggo/blob/test/docs/REPAIR_1.1.15-R5.md)。参考功能按 Honggo 自身结构独立实现，没有引入 NaiwaGuo 的源码文件或素材；详见 [功能研究与独立实现说明](https://github.com/SOLHK/Honggo/blob/test/docs/REFERENCE_STUDY_NAIWAGUO.md)。

测试版不是已完成设备验收的稳定版。正式版应在真机验收后另行发布。
