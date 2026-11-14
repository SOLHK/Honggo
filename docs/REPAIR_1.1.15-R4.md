# Honggo 1.1.15-R4 试玩卡首帧拦截

R3 在宿主 `modelDidDisplay` 回调后安排切页和删除。原生视图可在这之前完成构造和挂载，因此已入队的广告可能闪现。

用原始红果 7.4.3 IPA 中的 Eggplant 7.4.3.33 对照两条原生路径。文件偏移和调用顺序如下。[REA 摘要](reverse/7.4.3.33/r4/rea-pre-attachment-summary.json) · [指令记录](reverse/7.4.3.33/r4/rea-pre-attachment-gates.txt)

| 路径 | 调用顺序 |
| --- | --- |
| 混排试玩卡 | `FQVShortVideoFeedPageCellView refreshUI` 在 `0x840565c` 初始化专用视图、`0x84056a8` 传入模型、`0x84056c4` 加入子视图。模型失效时 `setDataModel:` 走重置分支并跳过绑定，通用页面仍会挂载该视图。 |
| 剧终插卡 | `FQVShortVideoPageListCellView refreshUI` 在 `0x849bd2c` 调用模型的 `createInsertView`。工厂先在 `0xfff8fe4` 绑定模型，再返回视图供宿主于 `0x849bd64` 加入。 |
| 异步游戏画面 | 游戏控制器准备完成后在 `0xfffa3b4` 调用 `mountGameViewController:`，把其视图挂入专用根视图的 `gameContainerView`。 |

开启「隐藏短剧广告、直播和小游戏插播」后，新增规则在 `SSShortVideoFeedDirectGameContentView initWithFrame:` 中调用宿主原始初始化器，于返回前同步隐藏专用根视图。这样也覆盖跳过绑定的混排卡。游戏画面、按钮、标题、简介和原生提示均位于这棵子视图内。普通短剧的通用列表页沿用原来的显示逻辑。R3 的入队前过滤及宿主切页、清理继续生效。已入队时切换过程可能经过空白背景，已核实路径的广告像素须始终不可见。[调用细节与边界](reverse/7.4.3.33/r4/DIRECT_GAME_VISUAL_GATE.md)

现有日志没有广告现场层级。截图的「点击进入游戏」与原生视图文案一致，截图底部的提示与专用原生剧终提示相差一字。宿主另有 Lynx 小游戏推广模板，已找到的唯一静态构造入口已由 R2 工厂规则覆盖。现有截图与静态 IPA 无法确认当次投放使用了哪条路径。[额外入口核对](reverse/7.4.3.33/r4/R4_PRE_RENDER_FINDINGS.md) · [Lynx 路径核对](reverse/7.4.3.33/r4/LEGACY_FEED_ASSESSMENT.md)

本地完成 ARM64 dylib 编译链接、26 项单元测试和 48 条宿主方法 ABI 对照。以实际 dylib 指令执行 18 组受控初始化测试，覆盖原对象、替换对象、nil 返回与 CGRect 参数。新增 UIKit 回归场景包括两种挂载顺序、跳过绑定时的默认文案、清理前像素、后续挂载的子视图、复用和普通内容。UIKit 用例已交叉编译，模拟器及 iPhone 尚未执行。[初始化器 REA 反汇编索引](reverse/7.4.3.33/r4/rea-initializer-summary.json)

真机验收需在双开版 7.4.3.33 安装单个 R4 插件，开启短剧信息流过滤并重新启动，在相同广告场景逐帧检查。复测仍有推广时，开启增强诊断后在推广可见时截图，再回设置导出最近一小时的日志。
