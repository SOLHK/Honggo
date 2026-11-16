# 1.1.15-R23 · 顶部隐藏、经典磨砂和横屏控件

宿主为已匹配二进制的红果 7.4.3.33。用户确认底栏选择「系统磨砂」，要求使用 iOS 18 已有的磨砂 API。最新日志版本为 R22，开启原生玻璃且没有开启降低透明度；七个场景均在「我的」及转场，首页和横屏结构另用同构建既有记录与方法元数据核验。

## 修改

- 首页真实频道类为 `SSHGBookStoreCategoryTitleCell` / `SSHGBookStoreCategoryTitleView`，此前精确父类名称分发漏掉它们。增加实际类和原生重载/选择刷新适配，并按继承关系识别头部及频道。菜单/搜索在宿主重排后重新应用隐藏；搜索只操作独立 `searchButtonSlotView`。六项全部隐藏时连空胶囊一起消失，五项隐藏而搜索保留时只显示搜索。
- `blur` 键值继续有效，名称改为「经典磨砂（iOS 18）」。工厂直接返回 `UIBlurEffectStyleRegular`（传统 Light/Dark 样式），从 R22 的 SystemMaterial 改成经典磨砂，不调用 `UIGlassEffect`、不叠加第二层材质。保留 alpha=1 和透明内容底色；磨砂底栏移除玻璃边框，选中态用更轻的中性色。
- 详情页的集数标题可以由 `SSShortVideoDetailViewController` 自己的 `navLeftLabel` 提供，与播放组件的返回箭头不同属。现在限定当前详情页和同一窗口，合并实际文字与箭头；兼容观察到的宿主派生类。返回、倍速、更多同轴且玻璃高度统一 34 pt，不改原生控件帧和动作。
- 补入当前实际挂载的 `SSShortVideoBottomInsertView.cleanSwitchView`，对应右下方形入口，继续执行原有清屏业务。
- 横屏精确适配 `FQVShortVideoPlayerLandscapeBaseInfoView`、`FQVShortVideoLandscapeNavBar`、`FQVShortVideoPlayerLandscapeBaseControlView`、`FQVLandscapeVolumeButton`、`SSShortVideoLandscapeBizInteractionView` 和 `FQVShortVideoLandscapeBottomRightView`。覆盖返回+标题、分享/更多、锁/音量、下一集、点赞/评论/收藏/弹幕入口和输入、倍速/清晰度/选集。常规按钮玻璃高度 40 pt，内容较长的按钮保留原生宽度。图片本身绘制的字形放在玻璃上方；锁/音量的材质跟随其自身消隐。进度条、字幕和播放器不增加玻璃层。
- 横屏换集的 `setVideoDataModel:` 在原方法返回后更新标题玻璃宽度；导航 `refreshLayout` 和锁按钮样式更新采用对应已核验 ABI。关闭开关恢复已修改的底色、剪裁和视图。

## 验证与边界

- iPhoneOS 27.2 SDK / arm64 生产编译、链接通过，`-Wall -Werror`。
- `AppearanceIntegration.m` 最新夹具交叉编译通过，覆盖真实频道子类、原生重载后的隐藏、五项/六项组合、经典磨砂类型/连续布局、详情标题所有者、方形入口、横屏各组按钮/原生动作/换集刷新/恢复。
- `test_version.py` 实际执行 13 项通过；`git diff --check` 通过。
- Windows 无 iOS Simulator 或连接的 iPhone，UIKit 夹具未运行，不能把编译等同于玻璃/磨砂视觉验收。传统 API 在当前系统上由当前系统渲染，没有加载旧系统私有框架。
- 前版记录的新「我的」Compose 上半区独立按钮尚未完成，R23 没有扩大修改该区域。

Apple 公开 API 文档：https://developer.apple.com/documentation/uikit/uiblureffect/style/regular
