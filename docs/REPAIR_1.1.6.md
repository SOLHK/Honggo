# Honggo 1.1.6 本地测试版

基线 7227cb9f695dc1d667a5832c408d5f2a0180619b。未推送 GitHub，未制作 IPA。

1.1.5 的新真机日志在“我的”页显示和设置返回期间，仅记录首页横向分页容器或过渡视图，完全没有此前出现的 SSMyUser637NestedContainerScrollView 和 Compose 渲染容器。移除滚动回调后，没有用个人页自身生命周期补齐晚加载/返回时的重新处理，这是确定的覆盖回归。图中的内容截断与页面未重新适配相符，但不能仅凭旧日志认定所有遮罩均来自同一层。

重新读取原始 Eggplant 的 Objective-C 元数据，确认 SSMine737KMPViewController 提供 viewDidLayoutSubviews（v16@0:8）和 viewDidAppear:（v20@0:8B16）。本版只 hook 此宿主类的这两个方法，各调用原实现一次；页面出现后同步处理，布局结束后合并到下一次主队列处理以等待 Compose 子视图挂载。通过父控制器链定位自定义底栏，并保留标准 tabBarController 路径。增加 mine.appear 事件供设备确认回调覆盖。

不恢复 setContentOffset: hook，不深入改写嵌套子列表布局；1.1.4 菜单白底修复保留。此轮没有用隐藏整个内容区或实心色块掩盖问题。

已运行：设备 ARM64 编译/链接（-Wall -Werror）、45 项已有产物指令/结构检查、7 项版本测试、版本递增及 diff 检查。4 个 UIKit/Foundation 测试文件编译通过。新增 UIKit 用例模拟晚出现的个人页，五次显示/返回均仅触发个人页 appearance，不触发滚动或底栏布局，并检查原实现调用次数和视口延伸。

上述新增 UIKit 用例尚未运行；当前 Windows 环境无 Simulator 或真机。已有 45 项指令检查不验证本次新的生命周期画面，也不证明遮罩消失。未执行 Xcode/远程 CI、未实测滚动帧率。真实红果滚动、设置返回、订单区可达性和最终画面仍待用户安装验收。

安装：移除旧 Honggo/HongguoGlass，仅保留 Honggo-1.1.6.dylib 随宿主重签，彻底退出重开。测试先进入“我的”直接滚动，再进入设置返回后继续滚动。若仍出现截断，日志应先确认 mine.appear 事件与个人页容器是否重新出现，再定位具体裁切层；不能把回调命中等同于视觉通过。
