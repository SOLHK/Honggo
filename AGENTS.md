# Honggo engineering handoff

Read `docs/AD_AUDIT_7.4.3.33.md` before repeating host reverse engineering. The audit
directory records host hashes, Objective-C methods/ABI/file offsets, disassembly,
REA output and the boundary between verified branches and remaining candidates.
Reuse these results when the host executable SHA-256 matches; inspect only new
entry points or changed code. A different host build requires a new identity check.

Test versions use `MAJOR.MINOR.PATCH-R1`, then `-R2`, etc. Keep the suffix in the
settings screen, diagnostics and filenames. Remove it only for a formal release
to `main`. `tools/version.py --macho` derives the numeric Mach-O version. `test`
publishes prereleases after required CI checks; do not describe CI as device
acceptance. Keep every version's source commit and artifact SHA-256 together.

Keep fixes focused and validate host method ABI at the hook boundary. Do not
disable generic UIKit/Lynx views, forge reward callbacks, or suppress a callback
without checking the host's declined path. Preserve manual business actions and
ordinary drama playback. Add a targeted regression for changed control flow;
run compilation and critical checks without redundant unchanged full rebuilds.

Report evidence, remaining uncertainty and failed checks honestly. Raw IPA,
screenshots and user diagnostic logs stay outside the repository. Commit only
sanitized statistics, relevant symbol metadata and analysis evidence.
