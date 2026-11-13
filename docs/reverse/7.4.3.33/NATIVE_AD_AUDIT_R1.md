# Honggo 7.4.3 原生广告入口复核 — R1

## 身份与可重用证据

- 输入 IPA：红果短剧 7.4.3，CFBundleVersion 7.4.3.33，主程序 Eggplant。
- IPA SHA-256：`37118984f159cc2ac8e30654d35196ea31dbf0e829b2bf081df9fd00ee74b036`。
- 主程序 SHA-256：`01bcb8b48ad07cb6e1cac5917dfcb67cdba9bf3f5239df9171bc46bb637fb8ab`。
- 本次重新从 ZIP 读流计算，确认与历史解包 Eggplant 逐字节哈希一致；没有用版本号代替一致性校验。cryptid=0，arm64，__TEXT 基址 `0x100000000`。
- `objc-all.json.gz` 保存全部 classlist 中 78,840 个类、1,349,749 条类/实例方法元数据；`ad-candidates.json.gz` 保存命名筛选的 3,714 个候选类、73,629 条方法。**这不是 3,714 种广告，也不是全部都实际执行。** Categories 中独立附加的方法、动态加载代码、Lynx/JS 下发内容不在此完整性声明内。
- `query_index.py --class 'SSShortVideo.*Live' --selector 'create|insert'` 可直接查询缓存。`--all` 切换到全量；不需要重新解析 IPA。完整索引19.5 MB压缩，候选索引1.05 MB压缩。
- 每条方法含类名、实例/类方法、selector、ABI encoding、文件偏移和 VM 地址。本目录反汇编以 **文件偏移** 输出；方法所在 __TEXT 的 VM 地址 = 文件偏移 + `0x100000000`，与 ASLR 后运行时地址不同。
- `inspect.cjs` 为本次使用的原生元数据提取脚本；`disasm.py` 用 Capstone 输出指令及 selector 引用；REA 6.9.0 + Rizin 0.9.1 的独立真实执行证据在相邻 `host-analysis/rea-ad-gates-rizin.json/.txt`，不是将自编脚本冒充 REA。

## 新增 11 个已核对入口

`verified-new-gates.json` 是机器可读清单，以下是实现映射建议。所有方法均已从本 IPA 验证签名，禁用分支已核对；仍待设备运行验证。

| 功能 | 类 / 方法 | ABI | 文件偏移 | 处理 |
|---|---|---|---|---|
| 信息流 | `+[SSBizVideoAdABTestHelper enableEcomForceCard]` | `B16@0:8` | `0xfd280d8` | NO |
| 信息流 | `-[SSShortVideoLiveService createFeedLiveInsertModelWithRoomModel:]` | `@24@0:8@16` | `0xff1950c` | nil |
| 信息流 | `-[SSShortVideoRecommendOutterListDataSource liveFeedDataModelWithCellViewData:]` | `@24@0:8@16` | `0xf5a18e0` | nil |
| 商业营销弹窗 | `-[IESLiveUGBenefitCouponFragment shouldShowBalanceCouponPopup]` | `B16@0:8` | `0xbbb1f80` | NO |
| 商业营销弹窗 | `-[IESLiveUGBenefitCouponFragment shouldShowExpansionCouponPopup]` | `B16@0:8` | `0xbbb1ecc` | NO |
| 启动电商推广 | `+[SSCouponPopupFrequencyHelper canShowPopup:]` | `B24@0:8@16` | `0xec57d90` | NO |
| 播放插播 | `-[BDADBackPatchAdService shouldShowAdWithContext:]` | `B24@0:8@16` | `0xfd2c11c` | NO |
| 播放插播 | `-[BDADBackPatchAdService shouldRequestAdWithPlaybackTime:duration:context:]` | `B40@0:8d16d24@32` | `0xfd2bffc` | NO |
| 播放插播 | `-[BDADPlotTouchAdService canRequestAdWithPlaybackTime:context:]` | `B32@0:8d16@24` | `0xfd7afc4` | NO |
| 播放插播 | `-[BDADPlotTouchAdService canShowPlotAdWithContext:]` | `B24@0:8@16` | `0xfd7b158` | NO |
| 播放插播 | `-[BDADShortVideoSideBarPatchSwitchComponent isAvailableWithCtx:]` | `B24@0:8@16` | `0xd547778` | NO |

强制电商卡的请求入口在 `0xfd31b90` 调用 enableEcomForceCard，`0xfd31b94` 的 cbz 直接走正常返回，发生在请求状态/插入状态改变之前。两条直播分别处理原生插入和 showType=605 的混排列表。原生 nil 经过 `0xf580864` 的 conformsToProtocol 检查，被正常跳过；混排工厂 nil 在 `0xf599a00` 被检查，跳过 addObject，不产生空白页。不能把 `createFeedLiveContentModelWithRoomModel:` 直接改为 nil：它的调用方没有同等清晰的空值保护。

直播优惠券两个 gate 均位于 API 请求前；不做领券、奖励、登录结果伪造。启动领券 gate 返回 NO 后，宿主移除弹窗队列的当前条目，再处理余下条目和完成路径；不能简单吞掉异步展示方法。Plot 请求 gate 的原有拒绝路径本来就不调用 complement，保留这一行为；没有人为伪造成功/奖励。

后贴片、剧情触达、侧栏均存在 `shouldBypassLocalAdControl` 分支，所以不能只改最内层 AB 开关。上述外层 gate 覆盖该绕行。侧栏只需 isAvailableWithCtx，shouldShowWithCtx 本身调用它并正常返回 NO，无需重复 hook。

## 截图判断与尚未证实部分

截图的主体是短剧列表里的直播电商预览，上层为3元直减券弹窗。给定日志所有32个旧入口均成功安装，没有 hook.skipped，因此旧规则未覆盖直播插入/强制电商路径是有证据支持的缺口。日志没有广告现场层级，截图文字也没有在 native __cstring 找到匹配，故不能断言券弹窗一定来自某一个原生类；它可能由直播 SDK、Lynx 模板、强制电商卡动态渲染。R1 在上游拦截这些已证实的插入渠道，并补直播自动券门控，真实复现场景仍需验收。

## 其他未知候选分类

- 短剧业务广告：普通信息流、TopView、暂停、插播、评论（已有32入口）；新增强制电商、原生/混排直播、后贴片、剧情触达、侧栏。
- 启动与商业营销：独立电商券队列、QCPX、商业弹窗、红包、挂件，分别走不同服务，不能把“弹窗”视为一个通用类。
- 阅读/漫画/听书候选：章节前后卡、底部banner、正文触达、音频贴片等打包在同一宿主。静态存在不证明红果短剧当前页面可达；本次不批量 hook 名字。
- 激励/付费/解锁：Inspire/Reward/Unlock/VIP/Pay/Charge 名称的候选单列；通常包括显式用户请求、付费或奖励确认，不能以“广告”名义伪造成功或无差别切断。
- 直播与电商手动业务：用户主动进直播间、商品页、订单、登录授权和领券操作与自动推广区分；没有全局禁用直播 SDK 或所有 UI 弹窗。
- 内置广告 SDK：BDAD、BDA、BUAd、IESLive 等下层基础设施中包含缓存、监控、合规说明、渲染、支付/交互，不等同于独立广告位。
- 动态资源/服务端实验：网络返回、Lynx模板及未来新增入口需要现场证据。索引及诊断保留用于后续增量，不能宣称“未知广告全部杀死”。

`candidate-taxonomy.json` 提供互斥名称分类的数量与示例。分类是导航用启发式，不是广告真值标签。`candidate-gates.txt` 是更广泛待审核 BOOL 名单，只有 verified-new-gates.json 中11项被本轮推荐实现。没有证据说明额外的 homepage/landscape/natural-commerce AB 标志必须新增：已有 feed gate 正在触发，且部分配置关闭会切换策略或备用路径。先保持这些候选有记录、不盲目拦截。

## 最终生产 ABI 校验

`gates-abi-check.json` 将最终 src/Ads.m 中全部43条规则逐项与该主程序索引比较：类名、类/实例方法、selector、完整 encoding，**43/43 匹配，无缺失或签名偏差**。这验证静态 ABI，不替代运行时 hook 安装或设备行为验收。
