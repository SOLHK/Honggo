# Honggo 1.0.4

## 版本与依据

从 `test` 的 `80321f1`（Honggo 1.0.3）继续，插件版本递增为 **1.0.4**，产品名称保持 **Honggo**。分发动态库为 `Honggo-1.0.4.dylib`，嵌入路径和安装名保持 `Frameworks/Honggo.dylib`、`@rpath/Honggo.dylib`。支持的宿主仍为红果短剧 **7.4.3 / 7.4.3.33**。

1.0.3 的 UIKit 行为检查通过 56/57 项，但整体门禁失败。系统截图进一步证明：兼容模式开启时，给原生 Regular 材质赋值后再清背景，只产生了与无效果透明对照完全相同的采样像素，浅色、深色都没有恢复玻璃。同一原生参照在兼容模式关闭后通过材质像素检查。详见 [1.0.3 后续验收结果](REPAIR_1.0.3.md)。

因此，对这个已核验宿主，不能仅靠 `UIGlassEffect` 对象存在、背景透明或单独注入新 dylib 判断原生 Liquid Glass 已生效。本轮把必要的宿主设计配置纳入预处理，并为保留原配置的注入路径提供系统模糊回退。

## 主 App 预处理

`tools/prepare_ipa.py` 先验证主 App 的四项身份信息，全部符合才配置原生设计：

| 字段 | 必须匹配 |
| --- | --- |
| `CFBundleIdentifier` | `com.phoenix.video` |
| `CFBundleShortVersionString` | `7.4.3` |
| `CFBundleVersion` | `7.4.3.33` |
| `CFBundleExecutable` | `Eggplant` |

预处理仅将主 App `Info.plist` 的 `UIDesignRequiresCompatibility` 明确设为布尔值 `false`，不修改其他元数据值。保留原来的 XML 或二进制 plist 格式；如果该键原本就是布尔值 `false`，主 plist 的原字节也保持不变。应用扩展的 plist、其他未修改的 ZIP 条目内容保持原字节，原包的条目顺序、注释与文件权限等信息继续保留。

动态库注入仍沿用已有约束：验证设备 arm64 Mach-O 与插件安装名，拒绝加密主程序、重复插件和不足或非零的头部空隙。主程序仅在已经核验的空隙中增加加载命令，动态库放入稳定路径。整个操作写入新的待签名 IPA，源 IPA 不变，已有目标文件不覆盖；写入失败会清理不完整输出。

纯函数 `configure_native_glass(info: dict) -> dict` 返回仅修改该键的副本，不修改输入。真实 IPA 处理与 UIKit 模拟宿主复用这个转换，避免测试使用独立的元数据修复实现。真实主 App 的严格身份校验留在 `prepare()` 内，纯函数本身不代替身份验证。

Apple 将该键定义为应用的设计兼容选择，参见 [UIDesignRequiresCompatibility 官方文档](https://developer.apple.com/documentation/bundleresources/information-property-list/uidesignrequirescompatibility)。这属于主 App 的启动配置，会影响应用使用的系统设计，不仅影响 Honggo 的底栏。

## 材质与关闭开关的语义

| 环境与配置 | 底栏策略 |
| --- | --- |
| iOS 26+ 且主 App 的兼容键明确为布尔值 `false` | 使用公开 `UIGlassEffect` Regular 原生材质 |
| 保留原包兼容模式，或没有明确的 `false` 配置 | 使用系统模糊材质 |
| 较早 iOS 系统 | 使用系统模糊材质 |
| Honggo 底栏开关关闭并重启 | 不应用插件的底栏和内容布局改动 |

单独注入 dylib 不会可靠地替代主 App 的设计配置。继续使用原兼容配置时，系统模糊是明确的回退路径，不应称为原生 Liquid Glass。

关闭 Honggo 底栏开关不会在运行时恢复 `UIDesignRequiresCompatibility=true`。这个开关只控制插件自己的改动；预处理写入的全局配置仍然生效。恢复原包的全局设计配置需使用保留的原 IPA 重新签名安装。旧版 `HongguoGlass.*` 偏好键继续兼容，产品名称、设置标题和新产物均使用 Honggo。

## 验证与交付边界

本轮本地 Python 回归为 **20/20**：13 项 IPA 预处理检查与 7 项版本规则检查。新增覆盖包括 XML / 二进制主 plist、原兼容键为 true / false / 缺失、仅主 App 兼容键允许变化、其他元数据相等、扩展 plist 原字节保留、源 IPA 未修改，以及身份不符时不得调用配置转换。版本规则验证已覆盖 IPA 预处理逻辑：打包变化不递增版本会失败，递增到 1.0.4 后通过。相对 `80321f1` 的实际版本校验也通过。

UIKit 回归应同时验证经共享预处理配置的原生路径与原配置下的模糊回退，保留独立原生参照、兼容模式负对照、无效果透明参照、旧源码负对照和完整系统截图。行为与像素门禁均通过后，CI 才构建和上传设备动态库；实际结果以本版本对应提交的 [GitHub Actions](https://github.com/SOLHK/Honggo/actions) 报告、截图和产物为准。控制器模型的模拟器检查不能代替真实红果的真机验证。

交付包含版本化 dylib、预处理工具和说明，原始红果 IPA 需另外提供。待签名 IPA 需要取得原始宿主包后生成，再由用户的签名工具为主应用和内嵌代码重签。本记录不代表已经制作或安装真实红果 IPA。
