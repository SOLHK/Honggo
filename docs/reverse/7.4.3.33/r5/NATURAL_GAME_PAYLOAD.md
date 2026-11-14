# R5 natural game payload evidence

Host: Honggo app 7.4.3 / build 7.4.3.33. Verified executable SHA256
`01bcb8b48ad07cb6e1cac5917dfcb67cdba9bf3f5239df9171bc46bb637fb8ab`.
All addresses below are file offsets in that exact executable. REA 6.9.0 ran
Rizin 0.9.1 against raw ARM64 bytes; `r5-rea-summary.json` binds commands,
evidence IDs, original record hashes, and unchanged-input results.

## What the screenshot does and does not establish

The new screenshot contains `欢喜斗地主`, `点击即玩`, and `上滑继续看短剧`.
A bounded scan of the main executable and 3,841 other IPA file members searched
UTF-8, UTF-16LE/BE, and JSON Unicode escapes. The exact title and CTA were absent.
The shorter footer occurs as one native UTF-16 CFString at `0x1a3f0858` (characters
at `0x171044ee`). This does not establish the actual runtime view: remote templates,
compressed nested content, and service data can provide text. Existing static
footer references include questionnaire and ranking cards, so footer wording is
not a reliable ad-class discriminator. The supplied log has no scene capture.

## Lazy game classification

`-[SSNaturalAdData parseGameDict]`, ABI `v16@0:8`, starts at `0xfd63a28`.
It checks `hasParseGame`, synchronizes on the data object, checks again, and parses
the object's `lynxData` JSON array. Missing/empty data still reaches the parsed
flag; callers must finish assigning the payload before forcing parsing.

For each dictionary, `feed_game_channel` defaults to zero. The actual discriminator
is this payload field, not `type`, `showType`, the title, or the CTA:

| Channel | Additional condition | Result |
| --- | --- | --- |
| 1 or 2 | Nonempty `game_content` dictionary | Store whole entry as `directGameDict`; set `isGameContent=YES` |
| 4 or 6 | Nonempty `game_content` dictionary | Store whole entry as `videoGameDict`; set `isGameContent=YES` |
| Other / missing / malformed content | No matching branch | No new positive game classification |

Key instructions are `0xfd63c24` (read channel), `0xfd63c30–0xfd63c34`
(`channel - 1 <= 1` unsigned), `0xfd63d10–0xfd63d24` (direct cache + game flag),
and `0xfd63d2c–0xfd63d74` (`channel & ~2 == 4`, video dictionary + game flag).
String helpers `0xfd64780` and `0xe974010` resolve to `feed_game_channel`
and `game_content`. Parser writes are limited to this object's parsed/cache/game
state and local logging/temporary objects; no insertion, player mounting, or SDK
preload call was observed in this method. The separate
`proloadVideoContentGameIfNeed` method starts at `0xfd63f8c`.

`isGameContent` is a plain cached BOOL getter (`0xfd66060`), so reading it before
parsing can miss a game. `directGameDict` (`0xfd639f4`) itself triggers parsing if
the direct dictionary is not cached. `directGameContent` (`0xfd639bc`) returns the
nested dictionary. `videoGameDict` is a plain getter. These are different contracts.

`updateLynxDataForFeedGame` (`0xfd64108`, `B16@0:8`) copies entries and updates the
nested game `schema` using series/video attribution. A missing or empty schema is
preserved rather than used as a display veto. When rewriting succeeds, it
serializes the array back to `lynxData` and clears the array/dictionary/direct/video
caches and `hasParseGame` (`0xfd642bc–0xfd64300`). Its BOOL describes rewriting,
not whether an ad should be inserted. Returning NO from it would not be a reliable
suppression gate. It does not reset `isGameContent` to NO; the implementation does
not claim arbitrary payload replacement on an already-parsed object is supported.

## Assignment order and force-watch separation

The ordinary AdService handler takes input `lynxStr` at `0xfd57104` and writes it
to the newly constructed natural data at `0xfd5711c`. It separately copies
`forced_viewing_time` into `forceWatchTime` at `0xfd57180–0xfd57190`. Both precede
the manager call at `0xfd57664`. The corresponding MUnion handler likewise assigns
Lynx content at `0xfd4e104` before insertion. The sibling native audit records the
complete handler and retry/copy caller paths.

Game classification and force-watch duration can coexist; they are not exclusive
types. A positive duration must pass through before lazy parsing and optional-game
filtering. `forceWatchTime` is a signed 64-bit integer (`q16@0:8`, getter
`0xfd65e5c`). The proposed/compiled R5 wrapper therefore checks exact host data
class, duration less than one, then calls the real parser and reads the flag.

## Full-page natural container

`createInsertView` (`0xfd64838`) resolves the manager by unique ID and calls either
`shortVideoAdViewWithUniqueId:naturalAdData:` or the rotation bridge factory.
Those selector slots are `0x1ce85248` and `0x1cd2f6a8` respectively. It retains the
returned `naturalAdContainerView`; there is no basis here for substituting a nil
UIView after the host has accepted the data.

`SSNaturalAdContainerView` is a UIView. `refreshLynxContainerView` (`0xfd67d48`)
uses `defaultFullPageConfig`, the data object's `lynxUrl`, `createDataDict`, and
the container's bounds to build `SSReadingLynxContainerView`
(`0xfd67ddc`), then adds that view as its own child (`0xfd67e44`).
`layoutSubviews` updates that child's frame to its own bounds
(`0xfd68aa4–0xfd68abc`). Thus this is a real full-page remote-template route,
separate from `SSShortVideoFeedDirectGameContentView` covered by R4.
`SSDirectGameNaturalCard` itself is an NSObject lifecycle adapter, not a view.

This proves a separate full-page game-capable path in this binary, but does not
prove the screenshot executed this path or establish ownership of every remotely
rendered pixel. The pre-enqueue gate avoids depending on those visual details.

## Actual compiled R5 wrapper check

`Honggo-1.1.15-R5.dylib` SHA256
`8aa7108d4bc09291e06ad982e3bb5d2d1c77539334ce814fbc2e0a23196b5b18`.
The actual `_HGNaturalAdInsert` at VM offset `0x47c8` is 53 ARM64 instructions.
`../check_natural_arm64.py` executed these bytes under Unicorn: 160/160 cases
passed. The matrix covers exact/unknown/subclass/nil data, signed duration
MIN/-1/0/1/MAX, both initial cached values and both parser results, and both original
BOOL returns. It verifies one original call or one suppression diagnostic,
unchanged original arguments/results, parsing before cached-flag read, no private
selectors for unknown data, and balanced input retain/release.

Objective-C/runtime/parser/original IMP boundaries are mocked in this machine-code
test. It is not a UIKit simulator run, live host execution, device frame capture,
hook-installation test, or proof that the supplied screenshot is fixed.

The annotated Capstone companion files are navigation aids: their simple per-block
ADRP register tracking can label a caller's stale register after a helper call.
For example `0xfd63b5c` is actually `+[NSMutableArray array]` through helper-set
`x8=0x1cab9000`, not the stale `setHasMention:` label. Conclusions above were checked
against helper bytes and actual selector slots; raw REA records preserve bytes
without these inferred Objective-C annotations.
