# Honggo 1.1.15-R15 — 浮动底栏、全集间距与动作图标

基线：已发布的 R14，`13e8bc7fab65784bf6963db583bb056d5cc980a0`。
适配对象：Eggplant 7.4.3.33。保留 R 编号，只通过 `test` 发布测试包。

## 从本次现场确认的问题

播放页面为 440 × 956 pt。视频列表 `FQVCollectionView` 和原生单页高度为
866 pt；底栏位于 y=872，高 84 pt。R14 已经隐藏原底栏的背景图片、分隔线
和效果视图，但视频结束处到屏幕底部仍有 90 pt 没有视频背景，因而只重复
清空底栏颜色不能消除整条深色区域。

实际 HUD 是 `FQVShortVideoVPlayerOutterMultiSeriesControlView`。R14 的
间距适配没有包含该类。它的全集入口位于 y=822，高 44 pt；进度控件位于
y=841，高 30 pt，其中可见轨道位于局部 y=22，高 2 pt。全集底边为 866，
轨道为 863–865，确实发生视觉重叠。原始日志与截图不进入仓库。

## 全集、轨道和右侧控件

补上真实 HUD 类的精确适配，以可见轨道计算 8 pt 全集间距，以当前浮动
胶囊边界计算 10 pt 轨道间距。位置所有权记录原生基线，重复布局不累加。
进度控件内部尺寸和拖动行为保持原生；全集整个容器及原生点击一起移动。

`SSShortVideoOutterSeriesInfoView` 包含接近全屏的信息层与付费覆盖层，
不能整体上移。仅同步调整已识别的底部文字与右侧动作组；全屏覆盖层保持
原位置。上下滑动时，相邻页不使用当前屏幕底栏去改变它自己的轨道位置。

## 底栏与图标

默认主胶囊和独立按钮统一为 62 pt，圆角 31 pt，底栏图标默认 26 pt。
R14 会保存整套默认配置，因此增加一次性旧默认值迁移；非默认自定义数值
保留，迁移后用户再次选择旧数值也不会被覆盖。

右侧使用统一线条的心形、三点气泡、星形和回转分享箭头。图标按最长边
限制视觉尺寸，避免书签等细长图像容器造成过大缩放。保留选中状态、自选
图片、SF Symbol 设置、数量和原生操作。可选圆形玻璃缩小并减弱底色。

## 视频下方的动态背景

竖向分页列表继续不参与普通列表视口延伸。原生视频、cell、collection
view 的尺寸、分页步长、偏移、父视图和播放器业务回调均不由此功能修改。

新增的插件背景只镜像已识别的视频渲染视图，不镜像 HUD，以免形成玻璃
效果递归。系统背景延展位于原视频列表后方，补齐底栏下面的原始空白；
不拉伸竖屏视频、不拍摄周期性快照，也不创建第二个播放器。

镜像入口需要在当前系统逐项核对方法签名，并禁用交互转发；不可用时不
调用未知接口。窗口、宿主页面、可见渲染器或设置改变时释放镜像来源。
系统背景延展使用 iOS 26 的 `UIBackgroundExtensionView`，仅对插件创建
的内容容器设置 `automaticallyPlacesContentView = NO`。原生视图不重新挂载。

参考的是接口事实与平台行为，实现由 Honggo 独立编写：

- [Apple UIBackgroundExtensionView](https://developer.apple.com/documentation/uikit/uibackgroundextensionview)
- [Apple automaticallyPlacesContentView](https://developer.apple.com/documentation/uikit/uibackgroundextensionview/automaticallyplacescontentview)
- [Telegram UIKitRuntimeUtils 中的 portal 接口使用](https://github.com/TelegramMessenger/Telegram-iOS/blob/master/submodules/UIKitRuntimeUtils/Source/UIKitRuntimeUtils/UIKitUtils.m)

## 验证要求

真实类名的独立受控 HUD 覆盖全集/轨道/底栏间距、右栏和文字的同步位置、
底部点击与滑动命中、重复布局、相邻页和恢复。配置测试覆盖旧默认迁移与
自定义值保留。背景测试使用真正的 CAMetalLayer 呈现不同颜色，并通过
simctl 系统合成器截图验证下方背景随帧更新；普通 UIView 着色或
`renderInContext:` 截图不能作为视频背景已工作的证据。

设备 dylib 仍由固定 Xcode 27.2 / iPhoneOS 27.2 构建，全部发布门槛通过后
才发布。每个包附带源码提交和 SHA-256。模拟器使用受控宿主，不等同于
用户 iOS 27.3 上的真实红果验收；实际播放、连续切集与进入全集仍需真机
复测。确切运行结果见本次提交关联的 Actions。
