# 1.1.15-R22 · 边距、背景载体与玻璃透底

依据 R21 的新截图和同一红果 7.4.3.33 宿主。新日志有七个场景记录，均为「我的」页面；播放/找剧入口继续核对先前场景和已匹配哈希的宿主方法。

## 根因与修改

- 首页侧栏和搜索图标仍沿用原始边距，顶部玻璃缩宽后显得贴边。现仅向内移动原生 `sideBar` 和 `searchButtonSlotView`，图标中心距玻璃边至少 23 pt；中间频道和覆盖全行的 `searchViewContainer` 不移动。保留原有 24 pt 点击区及动作，关闭或切换找剧状态恢复。
- 紧凑弹幕入口同时持有已挂载但隐藏的 `capsuleView`。现选择可见胶囊，否则使用真实 `clickView`。已确认的 `legacyImage` 本身带暗色方块，只在图标仍等于该原图时提取白色笔画，缓存结果，原生更新 `refreshIcon` 后重新应用；组合模式、自定义/未知状态图和关闭后的原图保持可恢复。
- 竖屏的全宽全集入口两侧增加约 3% 留白（12–20 pt），调整原生 bounds 宽度以重新排版文字和箭头，不拉伸图标或字体。窄入口和横屏保持原样，重复布局不累积，关闭及单控件/整块同窗转移均恢复。
- 宽视频停留时，底部背景已接到原生 matte，但旧 12 pt 圆角贴片仍在。现对这条同样已确认的衔接路径恢复完整边角，不放开宽视频 feed 的裁剪。
- 竖视频画面延伸到 dock 后，原生 `bottomGradientView` 仍在旧 y=866 边界以非零透明度结束。仅当实际竖视频延伸时，让最后 16 pt 渐变归零；其余原生文字衬底保留，退出/移交时恢复 colors 和 locations。没有添加新的全宽遮罩。
- R21 的原生玻璃加系统薄磨砂会在浅色页面重复染白。底栏撤掉附加磨砂，保留单层 `UIGlassEffect` 和透明 contentView；深浅前景与原有自定义选项继续保留。
- 找剧搜索的 `searchBgView` 和 `searchView` 是兄弟节点。改为使用整条背景，避免把玻璃只放到小搜索图标。快捷入口的 `bgImgView`、历史筛选的背景 UIImageView 会挡住玻璃，现仅隐藏已验证的背景图，保持文字、图标和动作；替换/关闭时恢复旧目标。浅色控件使用低透明度中性灰底涂，降低白底上的纯白细圈观感。

## 尚未完成的部分

新版「我的」上半区为 Compose。再次核验了 `RKMPIIOSMinePageControllerHandle` 桥接协议及 `SSMinePageKMPFunctionDepend.getFunctionSnapshot`：仅导出整体视图、生命周期/滚动/前景状态；快捷项配置只有 id、title、subtitle、显示/可点状态、红点等内容。内部 `functionItemBackgroundColor` 没有 Objective-C selector 或 Mach-O 导出入口，区域坐标仅为整行。月亮、菜单、编辑资料和快捷卡片的独立玻璃仍未完成，没有用猜测坐标或整块遮罩替代。

## 验证

- 生产源码 arm64 / iPhoneOS 27.2 SDK 编译、链接通过，`-Wall -Werror`。
- `AppearanceIntegration.m` 交叉编译通过；更新了顶部内边距、全集边距/恢复、可见弹幕、原生背景图、缺角和渐变边界夹具。
- `test_version.py` 实际运行 13 项通过；`git diff --check` 通过。
- Windows 没有 iOS Simulator 或连接的 iPhone，UIKit 夹具没有运行。白底玻璃、真实滚动和 Compose 尚未完成部分需要明确区分，不能以编译作为视觉验收。
