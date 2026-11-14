# R5 natural-game insertion boundary

Host: Eggplant 7.4.3.33 arm64, SHA-256 `01bcb8b48ad07cb6e1cac5917dfcb67cdba9bf3f5239df9171bc46bb637fb8ab`. All addresses are file offsets; add `0x100000000` for text VM addresses.

## Supported narrow change

The earliest shared verified boundary for the inspected natural-card delivery paths is:

`-[SSNaturalAdManager tryInsertDataToSaaS:]`, encoding `B24@0:8@16`, offset `0xfd60c64`.

Under the existing feed suppression option, for an exact `SSNaturalAdData` instance with `forceWatchTime < 1`, invoke the host's `parseGameDict` and read its `isGameContent`. Return `NO` when the host classifies it as a game; preserve the original method for all other inputs. Check the positive forced-watch value first so that preserved forced flows need no extra parse. Do not change `hasWatch`, `hasInserted`, force-watch duration, benefit state, session state, or completion callbacks.

This prevents newly delivered, host-recognized natural game cards from reaching portrait or landscape queues. It covers both direct game channels and video game promotion channels. It does not prove the screenshot uses this path; title/CTA are delivered dynamically and no live hierarchy was logged. A runtime hit is still needed to establish screenshot attribution.

Selection logic:

```text
if feed suppression enabled
   and exact runtime class(data) == SSNaturalAdData
   and data.forceWatchTime < 1:
    data.parseGameDict()
    if data.isGameContent:
        log the blocked rule without payload/URL
        return NO
return original(manager, selector, data)
```

The native method itself returns `NO` for nil input and can return `NO` when the portrait/landscape insertion is declined. Its three verified delivery callers discard the Boolean result. Returning `NO` is rejection, not synthetic insertion success. Do not return nil from `createInsertView`: downstream generic wrappers can unconditionally add that view.

## Hard game classifier

`-[SSNaturalAdData parseGameDict]` (`v16@0:8`, `0xfd63a28`) lazily parses the serialized `lynxData` JSON array through the host's `lynxDataArray` getter (`0xfd65d10`). `isGameContent` alone is a raw Boolean getter and can be false before parsing. The parser uses `hasParseGame` and synchronization around cached parsing. Its inspected body performs dictionary validation, cache assignment and local diagnostics; actual game preloading is a separate method (`proloadVideoContentGameIfNeed`, `0xfd63f8c`) and is not called by this parser.

For each dictionary entry, the parser reads integer key `feed_game_channel` (CFString `0x1a3f50b8`, materialized by helper `0xfd64780`). It requires a nonempty dictionary at `game_content` (CFString `0x1a356398`, helper `0xe974010`).

| Channel | Native branch | Result |
| --- | --- | --- |
| 1 or 2 | `0xfd63c30..0xfd63c34`; successful content at `0xfd63d10..0xfd63d24` | Stores the whole entry in `directGameDict`; sets `isGameContent=YES` |
| 4 or 6 | `(channel & ~2) == 4` at `0xfd63d2c..0xfd63d34`; success `0xfd63d68..0xfd63d74` | Stores the whole entry in `videoGameDict`; sets `isGameContent=YES` |
| Other or missing/empty content | Rejected branches in parser | Does not establish game classification |

The diagnostic strings explicitly distinguish `direct game content empty` from `video game content empty`. These channel values are from the executable's parser, not guessed from title or domain names. Reuse the host classifier instead of independently parsing untrusted remote dictionaries in the plugin.

`forceWatchTime` (`q16@0:8`, `0xfd65e5c`) uses a signed comparison in the original manager: `cmp x0,#1; b.lt` at `0xfd60c8c..0xfd60c90`. Values below one, including zero and negative values, take its non-forced branch. Values at least one with `!hasWatch` can enter the finished-force-watch-key logic. Keeping all positive values on the original route is a conservative boundary; it may leave a forced promotional game visible and is not proof that every positive value represents a reward.

Evidence: `natural-game-parse.disasm`, `natural-data-lazy-array.disasm`, `natural-entry-full.disasm`, `natural-game-entry-evidence.json`.

## Three incoming routes and callback contract

The validated selector-call sites are:

| Caller | Caller offset | Selector materialization / call | Encoding |
| --- | --- | --- | --- |
| `SSBizVideoAdCommonAdClient handleNaturalAdData:uniqueId:seriesId:episodeIndex:videoIndex:` | `0xfd42b7c` | `0xfd4315c` / `0xfd43164` | `v56@0:8@16@24@32q40Q48` |
| `SSShortVideoMUnionAdService handleNaturalAdData:uniqueId:seriesId:episodeIndex:videoIndex:complement:` | `0xfd4e090` | `0xfd4e64c` / `0xfd4e654` | `v64@0:8@16@24@32q40Q48@?56` |
| `SSShortVideoAdService handleNaturalAdData:uniqueId:seriesId:episodeIndex:videoIndex:complement:` | `0xfd570a8` | `0xfd5765c` / `0xfd57664` | `v64@0:8@16@24@32q40Q48@?56` |

The common client has no completion argument. In the two inspected service handlers the `complement` argument is retained and released; no invocation or storage of that block was found in those complete natural-handling bodies. The shared manager method itself has no callback argument and does not execute a callback on its nil/rejection return. The delivery callers discard its Boolean result and proceed to release local references. Preserve this contract; do not guess a block signature or invoke it with manufactured success.

`SSNaturalAdData finishPendingInsertWithReason:` is not such a completion. Its inspected body checks `hasInserted` / `hasReportedAdMonitor`, optionally sets `errMsg`, and calls `monitorSeriesAd`. It should not be called as a fabricated completion when rejecting the card at entry.

The natural service handlers create `SSNaturalAdData`, set metadata and serialized template fields, check host configuration, and obtain/create an `SSNaturalAdManager`. The manager initializer creates bookkeeping arrays/sets, stores its context and registers listeners. No ad content/container is built or mounted before the shared insertion call in these inspected routes. Parent analysis independently retained the common-client transfer window in `../client-natural-transfer.disasm`.

Full text direct B/BL scan to `0xfd60c64` found zero direct calls. Relevant manager/selector category scan found no overrides. Selector scans reported three validated incoming calls. This does not exclude reflective calls, runtime replacement or another loaded image; the fast selector scanner's stale-register false candidates were manually checked.

## Why rejection precedes queues and display

After entry, the original manager can copy the data for landscape, regenerate session ID, and set peer links (`0xfd60dec..0xfd60e10`), then invoke `tryInsertLandScape:isRetry:` (`0xfd60e14..0xfd60e1c`) and `tryInsertPortrait:isRetry:` (`0xfd60e24..0xfd60e2c`). Returning before this body prevents both orientations for a newly delivered matching card.

Portrait insertion (`0xfd61408`) can reject at the initial `changeInsertIndexWithData:isRetry:` check and return `NO` without changing the old displayed card. At successful commit it sets `hasInserted`, removes the card from retry state, clears `lastNaturalAdData` only when that pointer equals this data (`0xfd61dcc..0xfd61df0`), adds the card to `naturalAdDataList` and creates/gets its container (`0xfd61e3c..0xfd61e44`). `naturalManager:addDataSuccess:` is dispatched afterward (`0xfd61e80..0xfd61e90`). Landscape has the analogous order.

The shared entry method has no `lastNaturalAdData` write and does not remove a previous card. An early rejection likewise leaves existing state alone, matching native decline behavior. The three scanned setter-call sites (`updateData:`, portrait success and landscape success) clear this field when identical. No global claim about all possible raw ivar writes is made.

Do not independently force `tryInsertPortrait:isRetry:` or `tryInsertLandScape:isRetry:` to `NO` as a substitute. The retry routine (`0xfd60be0`) calls those directly, and a false result while the object is still in the retry array removes and re-adds the object, rotating the queue. The selected entry gate prevents newly matched cards from entering that queue; it is not cleanup for already queued cards in a running process. Existing app restart requirements remain relevant.

Evidence: `natural-entry-refs.txt`, `natural-entry-direct-calls.txt`, `natural-entry-categories.txt`, `natural-manager-init.disasm`, `natural-portrait-insert.disasm`, `natural-landscape-insert.disasm`, `natural-portrait-commit.disasm`, `natural-landscape-commit.disasm`, `natural-retry.disasm`, and the copied complete natural handler windows.

## Verification boundary

Established: exact ABI, native game-channel classifier, signed forced-watch comparison, three shared delivery callers, Boolean rejection semantics, queue/display ordering, and absence of direct branch/category bypasses in this executable.

Not established: screenshot class identity, a runtime R5 rule hit, zero ad frames/audio on the user's phone, complete coverage of all remote template/game channels, or non-game natural advertisements. The exact new `点击即玩` and `欢喜斗地主` strings were absent from the IPA's static files in sibling analysis; their absence favors runtime template data but does not identify a unique class.
