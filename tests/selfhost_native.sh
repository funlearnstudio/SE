#!/usr/bin/env sh
set -eu

SE_BIN="$1"
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"

"$SE_BIN" run selfhost/bootstrap.se
OUT="$(mktemp "${TMPDIR:-/tmp}/se-selfhost-XXXXXX")"
trap 'rm -f "$OUT" selfhost/generated_sample.cpp' EXIT INT TERM
c++ -std=c++20 -O0 selfhost/generated_sample.cpp -o "$OUT"
result="$("$OUT")"
if [ "$result" != "Hello SE" ]; then
  printf 'Bootstrap generated program returned unexpected output: %s\n' "$result" >&2
  exit 1
fi
printf 'SE selfhost native bootstrap passed\n'
