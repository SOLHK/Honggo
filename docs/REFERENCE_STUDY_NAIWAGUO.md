# NaiwaGuo 功能研究与 Honggo 独立实现范围

## 研究基准

参考项目为 [XIaodou0416/NaiwaGuo](https://github.com/XIaodou0416/NaiwaGuo/tree/9e81c0008fe4e7b506b1cd747a93022846ccf08a)，固定提交 `9e81c0008fe4e7b506b1cd747a93022846ccf08a`，版本 0.1.1。它主要定制红果短剧界面，依据红果 7.4.3 的宿主接口。其[验证记录][validation]明确写明：该版未完成 iOS 设备编译、模拟器回归和真机验证；旧模拟器截图只能说明设计意图。

本次按用户要求研究功能行为与通用实现原则。参考研究与生产代码编写分开，实施者收到行为规格；Honggo 没有引入参考项目的源码文件、图标、字体、赞赏素材或代码片段。参考项目采用 [MIT][license] 并声明[第三方来源][thirdparty]，但这不改变本次独立实现的约束，也不构成法律上的独立性认证。

## 功能与实现思路

下表表示仓库存在相应实现路径，不表示已经在红果真机上验证通过。

| 功能 | 用户可见行为 | 可学习的通用原则与来源 |
| --- | --- | --- |
| 播放界面布局 | 单独隐藏头像、操作按钮、频道入口和文字信息，调整位置及大小；竖屏扩展画面，宽屏和方形保持原比例 | 将宿主元素识别与布局修改分开，关闭时恢复原状态。[布局入口][main]、[全屏模块][fullscreen] |
| 悬浮底栏 | 胶囊与独立按钮、材质与颜色、图文模式、尺寸、项目显隐、触感反馈和收缩；独立按钮可打开宿主页面或设置 | 定制外观仍应保持宿主导航和选中状态；新旧系统分开处理。[底栏][dock]、[交互][actions] |
| 播放页材质 | 顶部频道、全集入口、右侧按钮、评论及分享、长按、倍速面板的玻璃效果 | 仅处理已识别的面板，保留文字可读性和点击区域。[设置入口][main]、[面板][panels] |
| 图标定制 | 动作图标预设、相册图片与 SF Symbols；点赞和收藏根据真实状态显示 | 图标呈现跟随宿主状态，不能以点击推测操作成功。[图标模块][icons] |
| 字体与字号 | 原生文字比例、系统字体选择、字体文件导入及恢复 | 保留富文本和图标字体语义，校验导入文件，不在布局回调里反复改变排版。[字体模块][fonts] |
| 设置操作 | 宿主设置内入口、备用手势、搜索后跳转并定位相关选项 | 设置元数据可以同时支持列表、搜索与说明，入口需要处理重复安装和页面切换。[设置模块][main] |
| 备份与恢复 | 导出插件配置及其自定义资源，从本地文件恢复 | 使用明确格式和字段白名单；校验完整后再修改配置，不携带宿主账号。[备份模块][backup] |
| 问题诊断 | 页面结构与截图导出、音频专项采样、可选只读调试接口 | 日常诊断与详细采集分级；用户主动导出，失败必须可见。[诊断说明][debug] |
| 作者与交流 | 赞赏和社群入口 | 属于原作者的产品内容，不纳入 Honggo 的功能迁移。[项目说明][readme] |

未将去广告、资源下载或会员修改列为该参考项目的已核实功能。Honggo 原有广告规则继续以自身宿主审计为依据；相关边界见 [7.4.3.33 审计](AD_AUDIT_7.4.3.33.md)。

## 1.1.15-R4 本轮范围

本轮优先补齐设置和兼容性基础能力，沿用 Honggo 现有 Objective-C、UIKit 与 runtime 架构。

- **设置搜索**：搜索现有功能名称和说明，使用 Honggo 自己的设置数据呈现结果，减少查找开关的步骤。
- **本地配置管理**：备份、恢复和重置仅处理 Honggo 当前的 16 个功能开关。先校验格式、字段及类型，再写入允许的偏好；不处理宿主登录数据。Honggo 尚无对应字体和图标管理，本轮备份也不宣称包含这些资源。
- **兼容性信息与运行快照**：集中呈现插件、系统、宿主及功能安装状态，说明当前匹配情况与跳过原因；快照服务于排错，不代表功能已经通过真机验收。
- **模块与方法边界**：将功能安装结果分开记录，独立检查所需方法和 getter 的 ABI。缺少某项能力时保留宿主行为，避免一个模块的前提被误用为另一模块的有效性证明。

本轮没有把参考项目全部视觉功能加入 Honggo，也没有新增参考项目的音频旁路、全局字体替换或私有底栏收缩实现。版本保持测试后缀 `1.1.15-R4`；具体文件和验证结果以本轮修复记录及 CI 产物为准。

## 1.1.15-R5 外观实现

R5 已独立加入上述八类外观功能的实现和专用原生设置，并保留 R4 的设置、广告与诊断基础。详见 [R5 实现范围与限制](REPAIR_1.1.15-R5.md)。这不是将参考项目全部细节逐行重做：头像底栏、音频旁路、FLEX、作者赞赏/群入口不在本轮八类外观功能中；信息布局采用已确认容器整体调整，字体与材质仅作用于已适配界面。运行时接口匹配和受控宿主测试仍不能替代红果真机验收。

## 兼容策略与待验证边界

参考项目既有公共 UIKit 材质路径，也使用私有底栏结构和主 Bundle 兼容状态覆盖；它的宿主接口说明不能证明未来版本继续兼容。[宿主适配说明][adapter]记载了这些依赖。因此，不能把其源码当作通用稳定性的证据，或只凭系统版本号就启用全部功能。

Honggo 已有的玻璃启动路径也需要如实说明：在宿主校验和开关条件满足时，向当前进程 `NSArgumentDomain` 设置 `com.apple.SwiftUI.IgnoreSolariumOptOut`，随后使用系统材质。该参数路径不是公共 UIKit 兼容保证；本轮保留已有行为，未据此宣称 iOS 27.3 的实际渲染已验证。旧系统的模糊回退和双玻璃开关关闭后的重启恢复仍需按 README 检查。

后续若加入字体、图标、播放布局或面板效果，应先确认目标宿主构建，补齐准确的类型、方法签名和视图归属证据。修改须限定在已确认对象，保留原值和关闭恢复路径；视图复用、旋转、主题切换及页面离开时也要可恢复。不能为寻找效果而全局隐藏 UIKit/Lynx 视图或扫描任意私有数据。

诊断优先记录能力、版本和有限事件。截图、文字、音频属于更详细采集，需要独立的主动操作与清楚的内容范围。本轮没有增加自动上传或外部服务依赖。

验证应分别记录源码检查、iOS 编译、模拟宿主回归和真实红果设备结果。参考项目的测试结论不能继承到 Honggo，CI 成功也不能替代真机验收。

[readme]: https://github.com/XIaodou0416/NaiwaGuo/blob/9e81c0008fe4e7b506b1cd747a93022846ccf08a/README-%E4%B8%AD%E6%96%87.md
[validation]: https://github.com/XIaodou0416/NaiwaGuo/blob/9e81c0008fe4e7b506b1cd747a93022846ccf08a/docs/VALIDATION.md
[main]: https://github.com/XIaodou0416/NaiwaGuo/blob/9e81c0008fe4e7b506b1cd747a93022846ccf08a/src/NaiwaGuo.m
[fullscreen]: https://github.com/XIaodou0416/NaiwaGuo/blob/9e81c0008fe4e7b506b1cd747a93022846ccf08a/src/NGFullscreenPlayback.inc
[dock]: https://github.com/XIaodou0416/NaiwaGuo/blob/9e81c0008fe4e7b506b1cd747a93022846ccf08a/src/NGDock.inc
[actions]: https://github.com/XIaodou0416/NaiwaGuo/blob/9e81c0008fe4e7b506b1cd747a93022846ccf08a/src/NGDockActions.inc
[panels]: https://github.com/XIaodou0416/NaiwaGuo/blob/9e81c0008fe4e7b506b1cd747a93022846ccf08a/src/NGPanelGlass.inc
[icons]: https://github.com/XIaodou0416/NaiwaGuo/blob/9e81c0008fe4e7b506b1cd747a93022846ccf08a/src/NGPlaybackPresets.inc
[fonts]: https://github.com/XIaodou0416/NaiwaGuo/blob/9e81c0008fe4e7b506b1cd747a93022846ccf08a/src/NGTypography.inc
[backup]: https://github.com/XIaodou0416/NaiwaGuo/blob/9e81c0008fe4e7b506b1cd747a93022846ccf08a/src/NGSettingsBackup.inc
[debug]: https://github.com/XIaodou0416/NaiwaGuo/blob/9e81c0008fe4e7b506b1cd747a93022846ccf08a/docs/DEBUG_TOOLS.md
[adapter]: https://github.com/XIaodou0416/NaiwaGuo/blob/9e81c0008fe4e7b506b1cd747a93022846ccf08a/docs/HOST_ADAPTER.md
[license]: https://github.com/XIaodou0416/NaiwaGuo/blob/9e81c0008fe4e7b506b1cd747a93022846ccf08a/LICENSE
[thirdparty]: https://github.com/XIaodou0416/NaiwaGuo/blob/9e81c0008fe4e7b506b1cd747a93022846ccf08a/THIRD_PARTY_NOTICES.md
