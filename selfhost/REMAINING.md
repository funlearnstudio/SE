# Remaining compatibility work

Checkpoint: experimental self-hosting branch, 2026-11-07. Completion is not certified.

- Return inference: unannotated mutual recursion, loop-dependent returns,
  numeric promotion across branches, and unknown-parameter forward signatures.
- Generic callable combinations and inference constraints.
- Dynamic Any containers/user objects with shared mutation and aliasing.
- First-class source-module values.
- Extension parity: `completion_report.json` inventories 230 advertised members
  without a detected lowering. Source scanning is not a conformance test; APIs
  with a lowering still need full arity, type, error, and runtime comparisons.
- Native callbacks, by-value C structs and macOS/Windows ABI validation.
- Comprehensive source-position diagnostics and portable AST serialization.

Implementation checks must include real execution/rejection cases and a fresh
strict Stage2/3/4 and direct multi-module self-hosting proof. Unimplemented items
must remain visible instead of being marked complete by an inventory scan.
