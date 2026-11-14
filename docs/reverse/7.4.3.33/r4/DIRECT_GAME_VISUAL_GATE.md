# R4 dedicated direct-game initializer gate

Target: Eggplant 7.4.3.33 arm64, executable SHA-256 `01bcb8b48ad07cb6e1cac5917dfcb67cdba9bf3f5239df9171bc46bb637fb8ab`.

## Final recommendation

Hook only `-[SSShortVideoFeedDirectGameContentView initWithFrame:]` (`@48@0:8{CGRect={CGPoint=dd}{CGSize=dd}}16`, file offset `0xfffb66c`) when feed suppression is installed. Preserve the original initializer, set `hidden=YES` on its returned object before returning that same object, and retain initializer ownership semantics (consumed receiver, retained result). Do not substitute the input receiver if the original initializer returns another object or nil.

This supersedes the earlier bind-only recommendation. The mixed wrapper initializes a root and calls `setDataModel:`; invalidated or incompatible models cause that setter to call `resetContent` and skip `bindContentModel:`. The wrapper still adds the initialized root. Reset restores the default CTA, so a bind-only gate cannot cover this unbound branch. Initialization precedes both branches and is the stronger common gate.

Do not return nil from `createInsertView`: the inserted wrapper calls `addSubview:` without a nil check. A hidden initialized root preserves host binding and cleanup while allowing a neutral background during the host's transition.

## Creation and reuse evidence

- The only direct classref-slot loads found in the native text are at `0xfff8fb8` (`feedContentViewClass`, returns the dedicated class) and `0xfff8fd0` (`createInsertView`, allocates it). The class has no decoded subclasses. The Objective-C category tables contain no category targeting this exact dedicated class.
- Mixed feed: `FQVShortVideoFeedPageCellView refreshUI` (`0x84055dc`) resolves `model.getFeedPageModel.contentModel.feedContentViewClass`, allocates a fresh view, calls `initWithFrame:` at `0x840565c`, calls `setDataModel:` at `0x84056a8`, then `addSubview:` at `0x84056c4`.
- The direct setter (`0xfffbce4`) requires a direct-game content model. Its nil/wrong/invalidated branches reset content at `0xfffbd60`; the valid branch binds at `0xfffbd74`.
- Series insert: `FQVShortVideoPageListCellView refreshUI` (`0x849bcf4`) requests `model.insertModel.createInsertView`. The direct model's factory (`0xfff8fc0`) allocates and initializes the dedicated root, binds at `0xfff8fe4`, then returns; the wrapper adds it afterward.
- The original initializer calls UIView's initializer, sets a black background, builds subviews, updates fonts and registers the view. It does not attach itself to a parent in this inspected method.
- Generic wrappers remove their dedicated child during `resetToRecycle`; they are not ordinary drama rendering views. Do not hide the generic wrapper or collection cell, which can be reused for ordinary pages.

## Containment and visibility

The root owns `gameContainerView`, entry button/icon/text, game name, category labels, summary, badge, gradient and series-end guide/more button. `mountGameViewController:` (`0xfffbd84`) adds the game controller's view inside that same game container. No separate rendering window is created in the inspected path.

The async `directGameCardController:didPrepareGameViewController:` callback (`0xfffa26c`) verifies controller identity and invalidation state, then mounts through the currently attached content view. `updateControllerStatus:message:` changes the entry button's alpha/enabled state, not the root's alpha. Inspected initialization, binding/reset, layout, presentation, badges/categories and async mount paths contain no root unhide operation; visibility changes target child views.

A UIView category implements `iesll_perfFMP_setHidden:` at `0xc4973d0`. It forwards the original boolean unchanged and returns immediately when hiding; the additional performance-record update occurs only for an unhide. This observed wrapper does not negate `hidden=YES`. Other runtime swizzles, dynamically downloaded code and unrelated plugins are not exhaustively proven absent.

Preserving original initialization and binding keeps the model/view attachment, controller cleanup and model-driven removal lifecycle. Do not unhide this dedicated root on nil/reset: reset deliberately repopulates the default CTA.

## Final local artifact proof

Final local R4 dylib SHA-256: `1bd4db7c83a3b0a7c3b5e44787d2d37cc61747ca07ae6c42b1805e635c385e15`.

`../check_init_arm64.py` executes the actual compiled `_HGDirectGameInit` using Unicorn with mocked original-init, UIKit setter and diagnostic boundaries. All 18 cases passed: original returns receiver/replacement/nil, initially shown/hidden, and three CGRects including fractional values and signed zero. The wrapper preserves D0-D3 bit-for-bit, returns the original result, hides valid returned roots before return, and inserts no retain/release/autorelease calls. This verifies machine control flow and ownership, not UIKit rendering.

REA 6.9.0 + Rizin 0.9.1 separately captured the final 19-instruction plugin initializer and host init/reset/category windows. Both records completed with provider exit 0, no truncation, empty stderr and unchanged input hashes. See `rea-initializer-summary.json`, `rea-final-r4-init.json`, `rea-host-init-visibility.json`, and their readable `.txt` projections. The earlier bind-only artifact and `bind-arm64-results.json` are obsolete investigation evidence, not final R4 verification.

## Limits

No fresh device hierarchy was supplied. The existing screenshot's native CTA/title/icon strongly match this class, but its short guide text differs from the native series-end guide. A server-supplied Lynx/template may use similar wording. Static native-path coverage and controlled ARM64 tests do not establish zero promotional frames on the user's iPhone or complete remote-template coverage.
