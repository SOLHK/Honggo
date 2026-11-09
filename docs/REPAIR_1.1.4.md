# Honggo 1.1.4 本地测试版

基线为本地 1.1.3 提交 `aea27775ef9c0ef6ee49ab153d18641459801af9`。按用户约定只提供 dylib，未推送 GitHub，未生成或修改真实 IPA。

## 为什么返回后才出现遮罩

用户反馈首次进入正常，进入设置或其他页面再返回后出现底部白色区域。新日志中当前会话始终使用 `UIGlassEffect`，启动参数也一直为 true，因此不是再次退回 UIBlurEffect。

同一会话的前后差异：

| Compose 透明容器 | 初次进入 | 从设置返回 |
| --- | --- | --- |
| 类 | `RKMPandroidx.compose.ui.scene.UIKitTransparentContainerView10` | 同左 |
| contentSize | `{0, 0}` | `{0, 0}` |
| 原始高度 | 956 | 872 |
| 插件处理后高度 | 956 | 956 |
| contentInset.bottom | 0 | 84 |

旧版把这层继承 UIScrollView 的容器当成真实列表，按延长量增加底部 padding，并停止遍历后代。第一次本来全高，延长量为零；返回后宿主重设为 872，错误才暴露。1.1.4 将日志确认的 Compose 类型识别为容器：继续处理内部视图，保持容器自己的 contentInset 和滚动条 inset。真正的列表仍保留原有底部补偿。生成的数字后缀通过固定类型前缀识别，不将所有空列表或 UIScrollView 一概排除。

## 设置入口两侧白块

Honggo 菜单是插件自有 UIControl。其 `setHighlighted:` 在按下/松开时写入灰色/白色圆角矩形背景，而玻璃胶囊只在后续布局时清理底色。触摸状态改变不保证再发生布局，因此背景会留在胶囊弧形外侧。

现在在 `setHighlighted:` 返回前同步执行已有的玻璃样式处理，并在 `didMoveToWindow` 中处理离开及返回：开启玻璃时保持宿主底色透明，关闭时保留正常及按下颜色。保留文字、箭头、菜单 target/action 和触摸范围。没有全局 hook UIControl 或 UIView。

诊断新增 `containerScroll`，以及控件的 `backgroundAlpha`、`opaque`、`inWindow`，用于复核白块来源。没有记录用户正文或账号，也未把原始日志和截图提交到仓库。

## 实际完成的检查

- 6 个生产模块 ARM64 交叉编译、动态库链接通过，`-Wall -Werror`；最后的诊断字段改动仅增量重编 Controls.m。
- 实际 1.1.4 dylib 的 44 项 ARM64 指令仿真检查通过。使用精确 1.1.3 dylib 作负对照，确认旧版未识别 Compose 容器、菜单 setter 在颜色写入后没有样式处理；这些是机器码路径验证，ObjC/Foundation/UIKit 边界由 fixture 模拟，不是画面验收。
- 版本工具 7/7 测试通过，1.1.3 → 1.1.4 递增检查、Mach-O 平台/版本/依赖检查通过。
- 4 个 UIKit/Foundation 测试文件编译通过；bash 及 6 段嵌入 Python 语法检查通过。
- 新增真实 UIKit 用例：初次全高容器、离开再返回后宿主缩短容器的五轮循环、容器和渲染子视图恢复、菜单连续按下/松开立即检查、移窗恢复、玻璃关闭颜色恢复。脚本保留 1.1.3 源码负对照。**当前 Windows 环境未执行这些 UIKit 用例，也没有运行 iOS Simulator、Xcode 或远程 CI。**

未更改启动玻璃机制和广告规则；未重复运行未变更的 IPA 工具测试。真机页面切换、Compose 最终画面、触摸、深浅模式及所有遮罩消失仍待用户安装验证，不能由指令仿真结果替代。

## 安装与验收

移除旧 Honggo / HongguoGlass 插件，仅添加 `Honggo-1.1.4.dylib` 并随宿主重新签名安装，彻底退出重开。原有开关保留。

1. 进入“我的”页，滚动到列表中段。
2. 进入宿主设置、打开 Honggo 设置、关闭并返回“我的”；再进入其他页面返回，重复几次。
3. 检查底栏外围白色区域、继续滚到底部、四个标签点击。
4. 按住再松开 Honggo 设置入口，检查两侧弧形外是否仍有白块。

若仍有遮罩，开启增强诊断并在复现后导出 JSON。日志中该 Compose 容器应显示 `containerScroll=true`、`insetOwned=false`；设置菜单玻璃开启时 `backgroundAlpha` 应为 0。最终以设备画面为准。
