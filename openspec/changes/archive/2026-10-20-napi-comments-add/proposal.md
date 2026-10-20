# Proposal

## Why

The NAPI binding tree `frameworks/js/napi/request` (49 files, ~10,100 lines)
has almost no functional comments: after the copyright headers there are only
~97 comment lines against ~8,500 code lines (1.1% density). Wave 1
(`upload-download-comments-fix`) corrected the wrong comments that existed
here; this change supplies the other half of the original task — adding the
missing comments — starting with the JS-facing layer, whose thread model
(`napi_send_event` switching), API9/API10 dual event tables and in-process
curl upload path are exactly the parts a reader cannot reconstruct from the
code alone.

## What Changes

- Add from-zero functional comments to all 49 files under
  `frameworks/js/napi/request/`: file-level purpose, class/struct docs,
  public-method docs (behavior contract, units, return semantics), key member
  docs, and inline notes for non-obvious logic.
- Style: English, Doxygen `///` for declarations (exemplar:
  `frameworks/native/request/include/multi_instance_runcount_manager.h`),
  plain `//` for implementation-internal notes. Density bar per exemplar:
  explain what/why/who, no line-by-line narration of obvious code.
- Every new comment must be verified against actual behavior before writing:
  trace call chains, and anchor on `docs/agent/feature-client.md` (§3.1
  NAPI thread model, §3.2 IPC contract, §3.3 UDS protocol, V9/V10 event
  tables) plus the Wave 1 verified fact base (units: bytes/s, ms, seconds;
  lifecycle semantics such as API9 error suppression).
- Comments Wave 1 already corrected stay untouched (they are the verified
  baseline); wrong comments newly discovered while reading a file are fixed
  in passing and recorded.
- Comment-only discipline: no code, log-string, DFX or build changes. Code
  bugs found while reading are recorded in the tasks appendix, not fixed.

Out of scope (later batches): `frameworks/native/request/` and
`services/src/cxx/` (batch 2), `frameworks/cj/`, `frameworks/native/
request_action/` and ANI-adjacent trees, all tests.

## Capabilities

### New Capabilities

- None. Comment-only change with no spec-level behavior changes;
  `.openspec.yaml` sets `skip_specs: true`.

### Modified Capabilities

- None.

## Impact

- Code: all 49 `.h`/`.cpp` files in `frameworks/js/napi/request/`,
  additions in comment lines only.
- No API, IPC, UDS, symbol-export or build-script changes.
- Verification: per-group eyeball review of `git diff` plus a full
  non-comment-line filter (`git diff -U0`; every `+` line must be a comment
  or blank, any `-` line must come from a comment rewrite). Build waived for
  comment-only changes (user ruling, 2026-10-20).
