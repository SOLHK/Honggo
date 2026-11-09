# Honggo 1.1.8 现场裁切修复测试版

基线 29d6f0e35f6b366aec7f2439f66d84600ff4201f，来自 1.1.7 诊断版。仅本地，未推送，未制作真实 IPA。

## 直接现场证据

本次 JSON 保留两份 user.screenshot 现场（时间 1794212951.19004、1794212951.24406），拒绝计数 0。节点数到达 192 上限，较深节点被截断，但以下角色及祖先链完整：

- page id=1，SSMine737KMPRootView，窗口范围 0,0,440,956。
- scrollView id=2，SSMyUser637NestedContainerScrollView，窗口范围 0,0,440,872，clips=true，mask=0。
- content id=37，parent=2，clips=false。
- bottomView id=3，parent=37，SSMyUser637TabsView，窗口 y=170.6667、高度 781，底部=951.6667。
- 内部分页 id=212 与列表 id=328 的窗口底部同为 951.6667。
- 玻璃 id=99 位于 y=872.3333..922，宿主原有两个效果层隐藏，页面 maskView 的 alpha=0。

因此至少这两份现场中，内容已经绘制越过底栏上边界，却被外层 scrollView 在 872pt 截断。不能再将此现场解释为普通 blur 回退或仅凭不存在的日志猜测回调遗漏。其他时间的三张截图并非都有独立完整现场，仍需设备验证覆盖范围。

## 修改

仅处理主包版本校验后的 SSMyUser637NestedContainerScrollView，且必须直接属于 SSMine737KMPRootView、起点为父视图原点、同宽、无特殊 transform 或外部 mask、剩余高度不超过 160pt。

保留宿主 frame、bounds、contentSize、offset、inset、内部分页尺寸与布局。关闭这个外层的 bounds 裁切，使用插件拥有的矩形 CAShapeLayer mask 将可见区域限定为父页面范围；mask 随 bounds.origin 移动，不允许顶部或侧面无限溢出。只有尺寸变化才重建 path，滚动布局时仅执行固定数量的属性操作，不扫描整树、不 hook setContentOffset。隐藏并保留原始上下边缘效果状态。离窗或条件不匹配时恢复原裁切和边缘效果，外部替换的 mask 不会被清除。

个人页整体退出此前通用视口拉伸遍历，避免新视觉裁切与旧 frame/inset 修正相互覆盖；其他页处理保留。原生个人页 layoutSubviews/didMoveToWindow 和个人页返回入口维护此局部状态。

诊断采集保留。遮罩复现时仍可截屏后导出 JSON，观察该外层 frame 仍为宿主管理值、clips=false、mask 的高度为父页面高度，而非被反复修改内容尺寸。

## 实际验证与边界

- 两份真实截屏结构数据的祖先矩形交集回放：修改前可见底部 872；仅替换外层裁切后 951.6667，覆盖玻璃底部 922。此为几何回放，不是 UIKit 合成截图。
- ARM64 编译/链接（-Wall -Werror）、45 项既有产物指令/结构检查、7 项版本测试、递增与 diff 检查通过。既有指令检查不验证新增 CALayer 渲染。
- 4 个 UIKit/Foundation 测试源文件编译通过。新增用例检查 200 次偏移下 mask 范围、滚动几何不变、顶部/底部边界和五轮离窗恢复；当前 Windows 环境未运行 UIKit 用例、Simulator 或真机。
- 未测实际帧率；真机滚动、回到订单、设置返回、侧栏动画、深浅模式和最后一行触摸仍待验收。不能以几何回放代替设备结论。

移除旧插件，仅注入 Honggo-1.1.8.dylib，随宿主重签并彻底重启。先滚动复查底栏，再从设置返回复查。若仍异常，开启增强诊断，截屏后立即导出，勿先退出或清日志。
