# Remaining compatibility work

Checkpoint: 24cc925, 2026-11-06. Completion is not yet certified.

- Return inference: unannotated mutual recursion, loop-dependent returns,
  numeric promotion across branches, and unknown-parameter forward signatures.
- Generic callable combinations and inference constraints.
- Dynamic Any containers/user objects with shared mutation and aliasing.
- First-class source-module values.
- Extension parity: completion_report.json lists 367 missing member lowerings;
  source scanning is an inventory, not a conformance test.
- Native callbacks, by-value C structs and macOS/Windows ABI validation.
- Comprehensive source-position diagnostics and portable AST serialization.

Implementation checks must include real execution/rejection cases and a fresh
strict Stage2/3/4 and direct multi-module self-hosting proof. Unimplemented items
must remain visible instead of being marked complete by an inventory scan.
