# Remaining compatibility work

API inventory checkpoint: 2026-11-07. The catalog contains 117 modules and 586 advertised members; all currently have a self-host lowering. Inventory lowering coverage is not the same as full production parity, so the API cases and external-service behavior remain the runtime conformance source of truth.

The current self-host build still has language and portability work outside this API pass:

- Return inference for unannotated mutual recursion, loop-dependent returns, numeric promotion across branches, and unknown-parameter forward signatures.
- Generic callable combinations and inference constraints.
- Full dynamic `Any` behavior for arbitrary objects, nested containers, shared mutation, and aliasing.
- First-class source-module values.
- Native callbacks, by-value C structs, and macOS/Windows ABI validation.
- Broader source-position diagnostics and portable AST serialization.

The 2026-11-07 API batch adds executable cases for dynamic CLI/logging, dataset/ML/tensors, hash/pickle/SQLite/DNS, format and archive bridges, Node tools, routing/listening, and graphics/media. `tests/selfhost_differential.py` exercises generated C++ and rejected arities; `tests/selfhost_stage2.py` verifies strict stage hashes and direct multi-file compilation.
