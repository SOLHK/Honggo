# 红果短剧 7.4.3.33 广告逆向档案 · Honggo 1.1.15-R1

日期：2026-11-13。代码基线：`b5e9092ed0f29d9737ac0793f70ade9bce0a812a`（1.1.14）。

后续增量：[1.1.15-R2 小游戏插卡及双开核查](REPAIR_1.1.15-R2.md)，在本档案的 43 条规则基础上增加两条；完整索引继续复用。

这份档案供后续聊天直接接手。先核对主程序哈希，再查下列索引；同一二进制无需重复全量分析。测试版从 R1 开始递增，正式发布到 main 才去掉 R 后缀。

## 身份与分析方式

| 对象 | 核对结果 |
| --- | --- |
| 宿主 | Eggplant，显示版本 7.4.3，构建 7.4.3.33，arm64 |
| IPA SHA-256 | `37118984f159cc2ac8e30654d35196ea31dbf0e829b2bf081df9fd00ee74b036` |
| 主程序 SHA-256 | `01bcb8b48ad07cb6e1cac5917dfcb67cdba9bf3f5239df9171bc46bb637fb8ab` |
| REA | [morluto/rea](https://github.com/morluto/rea)，本地 rea-agents 6.9.0，Node 24.22.1 |
| 反汇编 | REA 调用 Rizin 0.9.1，指定 ARM64 和文件偏移；另用 Objective-C 元数据解析与 Capstone 跟踪 selector/caller |

实际执行 REA 的 IPA inventory、Apple bundle graph、LLVM section headers 和 Rizin 广告调用窗口。完整 Rizin Mach-O loader 超时；Windows 的 REA 深度 Mach-O provider 不支持本样本。因此使用已经核对的文件偏移进行原始 ARM64 解码，10 个窗口、238 条指令完成，退出码 0、无截断、分析前后主程序哈希一致。**没有声称获得 Ghidra/Hopper 全量反编译。**

反汇编中的地址是文件偏移；本样本 `__TEXT` 基址为 `0x100000000`，不要把文件偏移直接当运行时 ASLR 地址。插件通过 Objective-C 类/方法和完整 ABI 安装，不使用硬编码运行时地址。

## 截图与日志结论

截图显示短剧流中的直播带货卡片，叠加“恭喜获得直减券 / 3 元 / 去领取”，底部有“上滑继续看短剧”。旧插件的领券开关只处理书城悬浮券挂件，启动电商开关只处理 QCPX 启动弹窗。

用户 1.1.12 日志共 395 条事件、4 个会话；当前会话 108 条，原有 32 个广告入口全部安装成功，11 条规则有拦截命中。短剧信息流禁用和插播跳过均记录到至少 256 次。普通日志按 2 的幂次抽样，不能将此视为精确总次数。现场快照是设置页 126 个节点，`maskScenes=0`，没有实际直播弹窗层级。

因此确认的是“旧规则漏掉独立直播/电商插入链”，不是旧 hook 安装失败。R1 从播放列表生成处拦截两条直播路径，并阻断强制电商卡片。两类直播自动优惠券也增加独立展示判断。**截图那张具体券卡属于哪一个动态/Lynx 分支尚无真机层级证据，不能将它与某个类一一认定。**

## R1 新增的 11 个入口

原有 32 个入口保留顺序，新增入口附加在末尾，避免旧日志的规则编号变化；总计 43 个入口，沿用已有偏好键。

| 类型 / 开关 | 类与方法 | 处理与证据 |
| --- | --- | --- |
| 强制电商卡 / 信息流 | `+[SSBizVideoAdABTestHelper enableEcomForceCard]` | NO；请求在 `fd31b94` 走原有早退，尚未写入插卡状态 |
| 客户端直播插卡 / 信息流 | `-[SSShortVideoLiveService createFeedLiveInsertModelWithRoomModel:]` | nil；转交插入管理器后 `conformsToProtocol:` 返回 NO，正常不入队 |
| 服务端混排直播 / 信息流 | `-[SSShortVideoRecommendOutterListDataSource liveFeedDataModelWithCellViewData:]` | nil；showType 605 的转换调用方在 `f599a00` 判空跳过 addObject |
| 直播普通券 / 商业营销 | `-[IESLiveUGBenefitCouponFragment shouldShowBalanceCouponPopup]` | NO；在请求券数据之前走正常不展示分支 |
| 直播膨胀券 / 商业营销 | `-[IESLiveUGBenefitCouponFragment shouldShowExpansionCouponPopup]` | NO；在请求 QCPX 券数据前退出 |
| 启动券队列 / 启动电商 | `+[SSCouponPopupFrequencyHelper canShowPopup:]` | NO；宿主移除不符合条件的队首并继续处理，未吞完成回调 |
| 返回插屏展示 / 播放插播 | `-[BDADBackPatchAdService shouldShowAdWithContext:]` | NO；视图 canShow 使用此结果 |
| 返回插屏预取 / 播放插播 | `-[BDADBackPatchAdService shouldRequestAdWithPlaybackTime:duration:context:]` | NO；覆盖内部 bypass 可能绕过的请求判断；两个 double + object ABI |
| 剧情触点预取 / 播放插播 | `-[BDADPlotTouchAdService canRequestAdWithPlaybackTime:context:]` | NO；走宿主原有早退；该早退本来就不调用 complement，未伪造奖励或成功 |
| 剧情触点展示 / 播放插播 | `-[BDADPlotTouchAdService canShowPlotAdWithContext:]` | NO；使用外层判定，覆盖内部 bypass |
| 侧栏贴片 / 播放插播 | `-[BDADShortVideoSideBarPatchSwitchComponent isAvailableWithCtx:]` | NO；shouldShowWithCtx 调用此判定后正常返回 NO |

精确方法编码、文件偏移、分支理由及证据文件名见 [verified-new-gates.json](reverse/7.4.3.33/verified-new-gates.json)。全规则对照见 [gates-abi-check.json](reverse/7.4.3.33/gates-abi-check.json)。普通短剧模型生成、主动进入直播、支付和奖励发放入口未纳入这些规则。

## 全量发现与未确认项

REA 列出 3,843 个文件、435 个目录、182 个 Apple bundle；这是容器目录覆盖，不是运行行为覆盖。

主程序 Objective-C 元数据索引包含 78,840 个类记录、1,349,749 个方法记录。按名称规则筛选得到的广告/营销候选数和分组，以 [inventory-counts.json](reverse/7.4.3.33/inventory-counts.json) 与 [candidate-taxonomy.json](reverse/7.4.3.33/candidate-taxonomy.json) 为准。候选包括 SDK 组件、数据模型、广告、营销、手动购物、奖励和非广告弹窗，**不能当作已确认广告数量，也不能按关键词全部替换。**

候选分类覆盖：宿主与短剧广告、直播插入/营销、电商/商业化、优惠券、弹窗/挂件、奖励/支付/解锁、其余 SDK。新入口只在 ABI 和调用方拒绝分支确认后启用。其余候选保留完整类/方法索引，供按现场查找。

仍需现场确认：动态下发的 Lynx/H5 模板及服务端实验；主页、横屏和阅读器中的其他投放分支；直播授权券配置 `IESLiveSaaSCouponAuthConfig` 的完整触发条件；尚未证明在红果实际可达的共用 SDK 模块。`createFeedLiveContentModelWithRoomModel:` 的调用方没有足够的安全判空证据，故使用外层已判空的混排工厂。没有采用隐藏通用弹窗、Lynx 页面或 UIKit 容器的办法。

## 文件索引与新窗口接手

- [host-identity.json](reverse/7.4.3.33/host-identity.json)：目标身份。
- [rea-audit-manifest.json](reverse/7.4.3.33/rea-audit-manifest.json)：脱敏 REA 证据 ID、工具版本、成功/失败边界；原始 REA 结果保存在本地交付 ZIP。
- [rea-ad-gates-rizin.txt](reverse/7.4.3.33/rea-ad-gates-rizin.txt)：REA 实际输出。
- `reverse/7.4.3.33/ad-candidates.json.gz`：完整候选类/方法索引，解压后 JSON，可按 class/selector 检索。
- `reverse/7.4.3.33/disassembly/`：附带 selector 解析的相关调用点反汇编。
- [log-triage-1.1.12.json](reverse/7.4.3.33/log-triage-1.1.12.json)：脱敏旧日志统计与 32 个入口安装/命中矩阵。
- 本地交付的完整逆向档案 ZIP 另含全部 Objective-C 索引、原始 REA Evidence、复用脚本及校验和；不含 IPA。

下次仅需读取本文件、对应候选类及分支证据；若同一宿主出现遗漏，先用增强诊断截屏采集 `ad.scene`，再核对相关动态分支。若哈希变化，重新提取改变后的元数据，不沿用旧地址推断新版本。

## 验证与交付边界

新增 `AdsIntegration.m` 使用生产 hook 安装器在真实 Objective-C/UIKit 环境验证关闭状态、客户端/服务端直播过滤、普通短剧和主动直播保留、独立开关、ABI 不匹配不替换、double 参数和旧插播回调。版本测试覆盖 R 次序、通道限制及实际构建脚本的数值链接参数。现有设置、底栏、现场诊断、日志及 IPA 工具检查继续保留。

设备包只在 CI 必需检查通过后发布到 test 的 [v1.1.15-R1 prerelease](https://github.com/SOLHK/Honggo/releases/tag/v1.1.15-R1)。核对其 `VERSION`、`source-commit.txt` 和 `SHA256SUMS.txt`；确切运行结果以该提交关联的 Actions 为准。当前档案不把静态分析、受控测试或构建通过写成真实 iPhone 验收。

真机验收：替换旧插件、确认四类开关已开、彻底退出后启动，连续切集/前后滑动、复现原广告位置、检查是否留空页/遮罩/音频、主动进入直播再返回。关闭相关开关后重启应恢复宿主行为。若仍有广告，在广告出现时开启增强诊断截屏，再回设置导出，期间不要退出。
