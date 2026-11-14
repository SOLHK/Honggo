# Legacy mini-game feed card assessment — host 7.4.3.33

This is a static assessment of the verified supplied Eggplant executable, SHA-256 `01bcb8b48ad07cb6e1cac5917dfcb67cdba9bf3f5239df9171bc46bb637fb8ab`. Addresses below are file offsets; text VM addresses are offset + `0x100000000`. No repository code was changed for this assessment.

**R4 decision:** retain this assessment for future diagnosis; do not add legacy behavior without runtime evidence. The already installed R2 factory covers every confirmed legacy constructor. The conditional cleanup design below records available semantics, not a recommendation to expand this release's scope.

## Conclusions

1. `SSShortVideoFeedMiniGameContentView` is a dedicated **feed card root**, not a generic manually entered mini-game controller. Its `setDataModel:` extracts `getFeedPageModel.contentModel` and checks `SSShortVideoFeedMiniGameContentModel` before binding. The legacy service branch is selected for feed show type `0x28d` (653), requires `miniGameData` and nonempty `lynxUrl`, and wraps the content in a short-video feed page. This establishes promotional feed placement, not whether every impression is paid advertising.
2. Every confirmed static legacy construction route is already under the R2 hook `-[SSVideoMiniGameService createFeedPageDataModelWithCellViewData:listVCContext:]`. Exactly one validated selector-call site initializes the legacy content model, within that factory. Full text direct B/BL scans to the factory, legacy initializer and view initializer found no direct bypass; relevant category scans found no override. There is **no newly proven uncovered producer**. Reflection, runtime replacement, a different loaded image, or a remote alternate template are not excluded by these scans.
3. There is a synchronous pre-mount visual boundary on this exact view root: the generic mixed-feed wrapper calls `setDataModel:` before `addSubview:`; binding attaches the pooled Lynx container inside this root. Hiding only this exact root before original binding prevents that subtree's promotional pixels, including when its container is reused. Do not hide the shared container globally. This is bounded to this native subtree; remote code that creates an independent window/modal is not statically ruled out.
4. The model has an app-owned local removal method, `removeCurrentModelAfterDislike`, whose implementation performs scrolling and exact-page removal, with logging, but does not call the feedback request path. This is an available queued-card cleanup mechanism, **not** a zero-visible-frame mechanism by itself. It is less guarded than the direct-game host removal path and must not be repeatedly called while its delayed removal is pending.
5. The user's game screenshot remains unproven as either this legacy template or the direct-game native template. The short bottom phrase differs from the native direct-game source-2 phrase; R2's lack of hits is still unexplained. Do not present this additional native coverage as a confirmed diagnosis of that miss.

## Construction evidence

`SSVideoMiniGameService canHandleCellViewData:` at `0x100053c0` compares show type to `0x28d` at `0x100053d4`. Its factory at `0x1000553c` dispatches the native direct-game branch first, then legacy at `0x10005620`:

- `cellExtraData.miniGameData`, then `miniGameData.lynxUrl.length`; invalid data returns nil.
- `0x100056d8` / `0x100056dc`: load `SSShortVideoFeedMiniGameContentModel`; allocate at `0x100056e0`.
- `0x100056e8`: selector `initWithCellViewData:miniGameData:`; call through argument/objc_msgSend thunk at `0x100056ec`.
- `0x10005700`: wrap via `createShortVideoFeedPageDataModelWithContentModel:`; `0x10005710` checks nonnil; success binds the list context and page backlink.

Evidence: `legacy-feed-type-gate.disasm`, `legacy-feed-factory.disasm`, `legacy-constructor-refs.txt`, `legacy-direct-imp-calls.jsonl`, `legacy-categories.jsonl`. Empty scan files mean zero matching records from successful scans; selector reference candidates were manually validated because the lightweight register tracker can produce stale-register false positives.

## Rendering order

The mixed wrapper `FQVShortVideoFeedPageCellView refreshUI` at `0x84055dc` obtains `feedContentViewClass`, allocates at `0x8405654`, initializes at `0x840565c`, calls the dedicated view's `setDataModel:` at `0x84056a8`, then adds that root at `0x84056c4`.

Legacy root `setDataModel:` at `0x100032d8` checks content model type at `0x1000330c..0x10003320`; `0x10003364` calls `bindContentModel:`. Binding at `0x10003374` either reuses the existing same-model/same-data container or resets and calls model `attachContentView:` at `0x1000341c`. It removes the pooled container from its old superview at `0x10003464`, adds it to this root at `0x10003480`, then updates frame. The generic wrapper has not yet mounted this root on the initial path.

The pool loads `miniGameData.lynxUrl` and `lynxData` into `SSReadingLynxContainerView`; the dynamic template is not contained in this executable. The generic feed wrapper inspected here adds the content root and has no independent game title, CTA or guide-label construction. Preserve lifecycle and ownership; do not blanket suppress `SSReadingLynxContainerView` or game SDK classes.

Evidence: `mixed-wrapper-refresh.disasm`, `mixed-wrapper-attachment.disasm`, `legacy-view-init.disasm`, `legacy-view-set-bind.disasm`, `legacy-model-attach.disasm`, `legacy-game-template-load.disasm`.

## Lifecycle and app-owned removal

`modelDidDisplay` at `0x100020fc` sets `isDisplaying=YES` and `isListViewVisible=YES`, reports series appearance and sends the template lifecycle event. `modelDidEndDisplay` at `0x1000213c` clears those flags and sends the corresponding lifecycle event. Keeping original lifecycle is necessary for the host's consistent state. Hiding the root alone does not prove template audio is stopped.

`removeCurrentModelAfterDislike` at `0x10002408` obtains the exact `pageDataModel`, `listVCContext`, and `dataContext`; nil dependencies only log and return. It captures page/data/self in a block. At `0x10002504` it compares `dataContext.currentDataModel` to the captured page. For the current page it invokes `scrollToNextIndexWithReserve:NO animate:YES` at `0x100025a0`, then dispatches its removal block on the main queue after 500,000,000 ns at `0x100025a8..0x100025ac`. For a non-current page it executes the block immediately at `0x100025b4`.

The block at `0x100025d4` asks `cellIndexForDataModel:` and compares to `NSNotFound` (`0x7fffffffffffffff`), then calls `removeDataModel:completion:nil` at `0x100025fc`. Remaining branches are local diagnostics and reference cleanup. The function does not invoke `handleOpenFeedbackEventWithParams:` or a dislike network request. No internal removal token/pending guard was found, so duplicate requests can initiate multiple next-page scrolls before removal.

If adding queued legacy coverage, use the exact model/view classes under the existing feed-ad switch. Hide the exact root synchronously before original binding; preserve original display lifecycle; schedule a single main-queue cleanup per model, and only invoke the app's removal after confirming `isDisplaying`, `isListViewVisible`, `pageDataModel.contentModel == model`, and `dataContext.currentDataModel == pageDataModel`. If the page is no longer current, skip the additional navigation. Do not fabricate a dislike event. This is a targeted queued-card guard; another generic factory hook is redundant with the proven R2 construction path.

Evidence: `legacy-model-display.disasm`, `legacy-model-remove.disasm`, `legacy-assessment.json` (exact ABI records and evidence hashes).

## Exact direct-game initialization and ordinary content

A separate short re-check of the proposed direct-game visual guard found one `SSShortVideoFeedDirectGameContentView` class definition, 69 declared methods, UIView superclass, and no declared subclasses in the full cached index. It owns `initWithFrame:` (`@48@0:8{CGRect={CGPoint=dd}{CGSize=dd}}16`) at `0xfffb66c`. The original initializer calls UIView's initializer, sets black background, builds its own game-specific children, updates fonts and registers the view. It does not attach itself to a parent in the inspected method.

The mixed route binds through the direct view's `setDataModel:` at `0xfffbce4`, whose accepted content is `SSShortVideoFeedDirectGameContentModel`; the insert route allocates this same dedicated class from the direct model's `createInsertView`. Generic feed and insert wrappers remove the dedicated child during recycling. Consequently, setting hidden only on the returned **exact-class** direct view after original initialization, under the feed suppression option, does not hide the ordinary drama wrapper/player in the inspected native routes. Apply the check to the returned initialized object, not a shared superclass implementation or generic collection cell. Preserve original initialization. An exact-class runtime guard also avoids affecting an unobserved dynamic subclass.

This confirms native scope isolation, not arbitrary future runtime behavior or screenshot identity. The host version and hook ABI still matter. Evidence reviewed from sibling analysis: `../rea/direct-view-init.disasm`, `../rea/feed-wrapper.disasm`, `../rea/insert-wrapper-set.disasm`, `../rea/DIRECT_GAME_VISUAL_GATE.md`; construction and wrapper order also retained in this directory.

## Validation limits

Verified: native class/selector ABI, constructor and wrapper call order, local removal semantics, absence of direct text branch bypasses and relevant categories in this supplied binary. Not verified: actual runtime hook hits for this legacy class, screenshot class identity, R4 device pixels/audio/navigation, arbitrary remote Lynx behavior. These require fresh device evidence and must not be reported as passed from this static analysis.
