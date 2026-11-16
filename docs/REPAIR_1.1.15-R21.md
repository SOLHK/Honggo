# 1.1.15-R21 · 玻璃控件和播放边界

宿主仍是红果 7.4.3.33，沿用 `AD_AUDIT_7.4.3.33.md` 的已核验二进制元数据。新日志中的系统为 iOS 27.3。

## 修改

- 顶部胶囊和展开底栏共用总宽度；六项顶部入口可独立隐藏，隐藏选中频道时同步处理指示条。
- 保留原生 `UIGlassEffect`，在每个底栏胶囊和圆钮的内部加同边界系统薄磨砂，使海报、文字和白色间隙先柔化再折射。移除默认白色底涂；两个效果视图始终 alpha=1，稳定布局复用实例。纯色、磨砂及降低透明度模式不叠加该效果；没有全宽遮罩或镜像截图。
- 原生 feed 两侧 12 pt 圆角贴片位于旧画面底边。仅当真实视频越过该边界时隐藏已匹配的对称非交互图片对；停止越界或转移播放器时恢复其原透明度。
- 详情播放器补入 `FQVShortVideoVPlayerMultiSeriesControlView` 间距适配，使合集和进度条保持现有 8 pt 间隔。宿主 factory 的 feed type 3 路径返回该类；不改进度拖动尺寸或底部评论入口。
- 详情返回/剧集标题、倍速、更多、全屏、弹幕和底部选集控件保留原点击行为，并在现有控件 bounds 内应用玻璃。选集面板只装饰有边界的内容区域，集数按钮保留原生选中底色的半透明色调；收藏按钮保留图标和文字状态，去掉其独立橙色渐变载体。
- 收藏、评论、点赞、分享使用三套重新绘制的实心轮廓：经典、几何、柔圆。旧线性预设迁移为经典实心；单项 SF Symbol 和相册图片可替换预设，收藏/点赞有独立选中态。
- 找剧及我的页面的已确认原生入口详见 `R21_PAGE_CHROME.md`。

新 hook 均按宿主完整 ABI 校验。选集 `setIsSelected:` 为 `v20@0:8B16`，收藏 `updateCollectStatus:collectText:` 为 `v28@0:8B16@20`，均先执行原方法再更新玻璃。关闭功能时按已持有的弱目标恢复，不为清理调用创建控件的 getter。

## 实际验证与边界

- iPhoneOS 27.2 SDK / arm64 生产代码编译、链接完成，`-Wall -Werror`。
- `AppearanceIntegration.m`、`AppearancePreferencesTests.m` 交叉编译通过。新增夹具覆盖顶部宽度/隐藏/恢复、动作轮廓、底栏实例复用、圆角贴片、详情间距、按钮点击和选集恢复。
- `test_version.py` 的 13 项测试实际运行通过；`git diff --check` 通过。
- Windows 当前环境没有 iOS Simulator 或已连接 iPhone，UIKit 夹具没有运行。编译和源码检查不能证明白闪完全消失；系统玻璃合成、高光及实际滚动观感仍需设备验收。
- 新版「我的」上半区由 Compose 整块绘制，目前没有已确认的各按钮目标，因此截图六中该区域的独立玻璃未完成。没有用固定坐标或整块遮罩代替。

公开 API 依据：Apple `UIGlassEffect`（iOS 26 起）和 `UIBlurEffect`；iOS 27 继续使用这些公开材质。相关文档：
https://developer.apple.com/documentation/uikit/uiglasseffect
https://developer.apple.com/design/human-interface-guidelines/materials
