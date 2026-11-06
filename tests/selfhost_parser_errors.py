#!/usr/bin/env python3
"""Check that malformed SE syntax fails deterministically, not with an index error."""
from __future__ import annotations

import json
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parent.parent
SELFHOST = ROOT / "selfhost"


def parse_with_bootstrap(se: Path, source: str) -> subprocess.CompletedProcess[str]:
    with tempfile.NamedTemporaryFile(
        mode="w", encoding="utf-8", prefix=".parser-errors-", suffix=".se",
        dir=SELFHOST, delete=False,
    ) as handle:
        path = Path(handle.name)
    try:
        path.write_text(
            "use parser\n"
            + "nodes = try parse " + json.dumps(source, ensure_ascii=False) + "\n"
            + 'say "parsed"\n',
            encoding="utf-8",
        )
        return subprocess.run(
            [str(se), "run", str(path)], cwd=ROOT,
            capture_output=True, text=True, timeout=30, check=False,
        )
    finally:
        path.unlink(missing_ok=True)


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit("Usage: selfhost_parser_errors.py /path/to/se")
    se = Path(sys.argv[1]).resolve()
    valid = "make identity[Item] value:Item -> Item\n    give value\n"
    result = parse_with_bootstrap(se, valid)
    if result.returncode or "parsed" not in result.stdout:
        raise AssertionError(f"Valid generic function failed:\n{result.stdout}\n{result.stderr}")
    invalid = (
        ("make identity[] value:Int -> Int\n    give value\n",
         "A generic function needs at least one type name."),
        ("type Box[]\n    value:Int\n",
         "A generic type needs at least one type name."),
        ("make pair[T,T] x:T -> T\n    give x\n",
         "Duplicate generic type parameter: T"),
        ("make add x:Int x:Int -> Int\n    give x\n",
         "Duplicate function parameter: x"),
        ("for key key in items\n    say key\n",
         "A for-pairs loop needs two distinct variable names."),
    )
    for source, expected in invalid:
        result = parse_with_bootstrap(se, source)
        output = result.stdout + result.stderr
        if result.returncode == 0 or expected not in output:
            raise AssertionError(
                f"Unexpected parser result for {source!r}, expected {expected!r}: "
                f"status={result.returncode}\n{output}"
            )
    print("SE selfhost parser error regression passed")


if __name__ == "__main__":
    main()
