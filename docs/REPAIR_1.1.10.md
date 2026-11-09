# Honggo 1.1.10 底栏文字优化

本轮用户已明确授权优化后推送 test。未制作真实 IPA。

只调整已核验的 SSTabBarTextButton / SSTabBarButton 的 tabLabel：浅色材质上未选中文字从宿主浅灰提升为不透明深灰（white=0.12），选中使用黑色；深色材质对应白色/近白色。未选中采用 semibold，选中 bold，保留原字体描述和字号。先读取原始前景决定材质主题，退出玻璃状态恢复原字体和颜色。其他标签、红点、按钮位置、点击回调及裁切策略不改。

## 红点核查

用户最新 1.1.9 现场中，“我的”对应第四个 SSTabBarTextButton（节点308）的第三个子layer可见，为10×10的UILabel layer；其余三个按钮相同位置的角标layer隐藏。插件没有创建红点的代码，已有实现保留宿主角标。此轮样式仅经 tabLabel getter 定位标题，不遍历或修改旁边的角标。

原始 Eggplant 元数据存在 SSRedDot 的 subRedDots/parentId/shouldShow/isEnabled/canShow/hideInParent 及 SSRedDotService 的树更新方法，表明宿主有父子汇总和展示条件。个人页事件源有评论侧栏、游戏中心侧栏的同步入口，订单模型也关联红点。因此不能把“我的”红点直接等同于一条未读消息。

当前 JSON 只有显示层结构，没有宿主红点树的实时业务状态，无法确认用户这一次到底由哪一个子入口触发。没有将任何候选原因写成结论，没有调用宿主清除红点方法，也没有把红点隐藏来假装问题已解决。

## 检查

本地 ARM64 编译/链接、45项已有产物指令/结构检查、20项Python工具测试通过。4个UIKit/Foundation测试源文件编译通过。新增受控UIKit检查标题颜色/字号、红点不变、隐藏后字体恢复；实际Simulator结果以后续精确提交的GitHub Actions为准。真机字体观感仍需用户验证。

此提交包含之前经用户逐版测试的本地改动历史。test 普通快进推送，main 不变。保留现场采集；真实内容和账号不提交到仓库。
