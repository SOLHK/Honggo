# R4：零原生游戏卡像素的显示前边界核查

目标：允许短暂中性背景与宿主跳转，但不得先显示游戏卡、按钮、文案或游戏画面。分析对象仍为SHA256 `01bcb8b48ad07cb6e1cac5917dfcb67cdba9bf3f5239df9171bc46bb637fb8ab` 的Eggplant 7.4.3.33。

## 精确构建顺序

### 原生混排页

`FQVShortVideoFeedPageCellView refreshUI`，ABI `v16@0:8`，文件偏移 `0x84055dc`：

1. 从 `model.getFeedPageModel.contentModel` 得到内容模型。
2. 调用 `feedContentViewClass`，确认UIView子类与feed协议。
3. `0x8405654` 分配内容视图；`0x840565c` 调用 initWithFrame:。
4. `0x84056a8` 调用内容视图setDataModel:。
5. **之后**才在 `0x84056c4` 向wrapper addSubview。

因此专用DirectGameContentView的初始化/绑定边界发生在混排wrapper挂载前。通用wrapper自己没有创建游戏按钮、标题、播放画面或滑动提示的代码；其基类FQVShortVideoCellView只提供回收/复用标识等接口。

### 插入页

`FQVShortVideoPageListCellView setModel:` (`v24@0:8@16`, `0x849bc58`) 更新模型并调用refreshUI，继而refreshLayout。refreshUI (`0x849bcf4`) 取 `model.insertModel`，在 `0x849bd2c` 调用createInsertView。

`SSShortVideoFeedDirectGameContentModel createInsertView` (`@16@0:8`, `0xfff8fc0`) 分配 `SSShortVideoFeedDirectGameContentView`，用CGRectZero初始化，再在 `0xfff8fe4` 调用bindContentModel:，然后返回。

通用wrapper在收到返回视图后才setInsertView，并在 `0x849bd64` addSubview。由此同样可在专用视图bind之前阻断任何游戏像素，但必须确保后续 mountGameViewController/update状态路径不能重新显示专用视图，且继续调用宿主正式删除模型流程。具体专用视图hook由REA并行审计提供。

### modelWillDisplay不是足够早的唯一拦截点

`SSShortVideoFeedDirectGameContentModel modelWillDisplay` (`v16@0:8`, `0xfff8ff8`) 主要将listVisible设YES并记录统计，没有在这里创建视图。视图可由setModel→refreshUI提前构造和挂载。不能仅凭生命周期命名就宣称该回调一定早于首次绘制；零像素要求应落实在专用视图构造/绑定/挂载路径。

## 可以额外拒绝的新对象边界，但不能处理已排队wrapper

`+[FQVShortVideoPlayerService createShortVideoFeedPageDataModelWithContentModel:]` (`@24@0:8@16`, `0x84b26d4`) 是最后的混排wrapper工厂。可对精确的DirectGame内容模型返回nil，并保持其他模型原行为；DirectGame调用方 `0x10006248` 调用后在 `0x10006258` 判空，拒绝后不进行bindPageDataModel。

插入页底层API：
- `FQVShortVideoListVCInsertComponent insertModel:videoIndex:isAfter:`，`q36@0:8@16q24B32`，`0x843ed64`。
- 同类 `insertModel:videoDataModelUUID:`，`q32@0:8@16q24`，`0x8440000`。
- 横屏类 `FQVShortVideoListVCLandSpaceInsertComponent` 对应入口在 `0x845e540` / `0x845e6e0`。

这些入口在canInsertModel为NO时返回2，发生在修改数组前。但2被series-end协调器理解为待滚动结束后一次重试；不能拿7冒充通用失败，因为7实际表示当前重复模型。因此不建议仅为了冗余扩展这些通用hook。它们也不能影响已经进入列表的wrapper；专用视图提前中性化+宿主删除路径才解决“已经排队但不显示卡片”的要求。

## 屏幕归属仍有明确不确定性

“点击进入游戏”的静态UTF16只有一个字符串对象，对应专用DirectGameContentView；“魔法幸存者”在主程序UTF8/UTF16均不存在，来自模型内容。“上滑继续看短剧”较短版本的已验证原生引用属于偏好问卷、问卷与排行卡，而DirectGame source=2使用“上滑继续观看短剧”（多一个观）。**本次通用wrapper检查没有找到它在DirectGame视图外绘制短提示的证据。**

因此不能把整张截图100%认定为这个原生类，也不能解释R2工厂没有命中的事实。另一个真实候选是 `SSShortVideoFeedMiniGameContainerView refreshLynxContainerView` (`0x10001488`)：它从miniGameData读取lynxUrl与lynxData，动态创建SSReadingLynxContainerView，模板可以包含相同文案。SSNaturalAdContainerView也存在动态模板途径。当前日志没有场景或URL，无法证明截图对应哪一个服务器模板，也没有证据支持批量屏蔽任意Lynx视图。

本目录包含wrapper元数据、混排刷新与挂载反汇编、legacy模板读取证据及截图字符串检索结果。相关插入枚举与早期模型证据保留于 `work/r3/native/zero-*.disasm`。
