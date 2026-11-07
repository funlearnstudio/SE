#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
SE_BIN="${SE_BIN:-se}"
out="$(printf '12 3.5 Alice\n' | "$SE_BIN" run examples/ask-multiple.se)"
grep -q '12' <<<"$out"
grep -q '3.5' <<<"$out"
grep -q 'Alice' <<<"$out"
# Invalid arity and invalid type must report errors.
if printf '12 3.5\n' | "$SE_BIN" run examples/ask-multiple.se >/dev/null 2>&1; then
  echo "Expected an error for missing values" >&2
  exit 1
fi
if printf 'abc 3.5 Alice\n' | "$SE_BIN" run examples/ask-multiple.se >/dev/null 2>&1; then
  echo "Expected an error for invalid Int" >&2
  exit 1
fi
echo "multi-value ask smoke tests passed"
