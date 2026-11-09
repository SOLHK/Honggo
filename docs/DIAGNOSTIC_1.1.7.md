# Honggo 1.1.7 遮罩现场诊断版

基于 1.1.6 / 3c7503e9f924655685ea1d9481d72fa46a0c1583。此版只新增现场采集，不修改布局、遮罩修复、玻璃和广告策略。未推送，未生成 IPA。

## 使用

1. 移除旧 Honggo/HongguoGlass，仅注入 1.1.7，随宿主重签并彻底重启。
2. 我的 → 设置 → Honggo → 打开「增强诊断」。保留复现问题时的玻璃开关状态。
3. 返回“我的”，复现滚动遮罩。停止滑动，直接用手机截屏；截屏通知会触发现场结构采集。
4. 随即进入设置导出日志，发送 JSON 和对应截图。不要先强退应用或清除日志。
5. 再复现「打开设置 → 返回」的遮罩，截屏、立即导出第二份 JSON。两种场景分开导出，避免后续操作挤掉前一现场。

只获取系统截屏通知，不读取截图像素，不保存页面文字、头像、书名、账号、网络内容。用户发送的截图与 JSON 时间配对。

## 采集内容

`maskScenes` 独立于普通限频事件：截图、个人页即将离开、出现、0.5 秒后、实际转场完成/取消、滚动停止/减速结束。自动滚动停止依赖此宿主控制器的已验证 delegate 方法；内部滚动未经过该 delegate 时仍可用截屏触发。

记录会话内稳定 view id、parent id、已知 getter 的角色映射（scrollView、bottomView、Compose 内容等）、frame/bounds/window 坐标、presentation 坐标（存在时）、hidden/alpha、背景 alpha、clip、mask frame/path bounds、最多四个子 layer、滚动 offset/contentSize/insets、系统边缘效果状态。优先放入关键视图和祖先链；不遍历 cell 内部、图片和系统材质内部。

采集不修改 frame/inset/offset/alpha/mask，不安装逐帧 scroll 回调。主线程遍历最多 192 个节点、约 12ms 软时间预算（单次 UIKit 属性调用可能超出）；JSON 编码在串行后台队列完成。超节点/时间/字节会明确记录 truncated、timeLimited、byteLimited。

保留本次进程最近 10 份、合计最多 768 KiB；单份最多 192 KiB，超限截短末尾节点；待编码最多 4 份。导出包含裁剪和拒绝计数，普通事件不会淘汰独立现场。导出上限增加至 3 MiB。现场不跨进程保存，清除日志同时清除现场。

## 验证边界

6 个模块 ARM64 编译/链接通过（Clang 20.1.2、iPhoneOS16.5 SDK、LLD 23.1.1，-Wall -Werror）。45 项既有二进制/指令检查、7 项版本测试通过，4 个 UIKit/Foundation 测试文件编译通过。新增 Foundation 用例检查设置页导出保留旧现场、10 份环形容量、超大记录拒绝及清空。Windows 环境未运行这些 Foundation/UIKit 用例；没有真机确认采集结果，不声称遮罩修复。

宿主原方法编码复核：viewWillAppear:/viewDidAppear:/viewWillDisappear: 为 v20@0:8B16；scrollViewDidEndDragging:willDecelerate: 为 v28@0:8@16B24；scrollViewDidEndDecelerating: 为 v24@0:8@16。每个 hook 只调用原实现一次。
