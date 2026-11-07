#!/usr/bin/env python3
"""Regression checks for the SE-written bootstrap lexer's nested delimiters."""
from __future__ import annotations

import json
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parent.parent
SELFHOST = ROOT / "selfhost"


def run_lexer(se: Path, source: str) -> subprocess.CompletedProcess[str]:
    # Keep the temporary entrypoint alongside lexer.se for local 'use lexer'.
    with tempfile.NamedTemporaryFile(
        mode="w", encoding="utf-8", prefix=".lexer-check-", suffix=".se",
        dir=SELFHOST, delete=False,
    ) as handle:
        program = Path(handle.name)
    try:
        program.write_text(
            "use lexer\n"
            + "tokens = try lex " + json.dumps(source, ensure_ascii=False) + "\n"
            + 'say "lexer completed"\n',
            encoding="utf-8",
        )
        return subprocess.run(
            [str(se), "run", str(program)],
            cwd=ROOT, text=True, capture_output=True, check=False, timeout=30,
        )
    finally:
        program.unlink(missing_ok=True)


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit("Usage: selfhost_lexer_delimiters.py /path/to/se")
    se = Path(sys.argv[1]).resolve()
    for source in ("say ([1, 2][0])\n", 'say {"a": [1, (2)]}\n'):
        result = run_lexer(se, source)
        if result.returncode != 0 or "lexer completed" not in result.stdout:
            raise AssertionError(
                f"Valid delimiters rejected: {source!r}\n{result.stdout}\n{result.stderr}"
            )
    for source, expected in (
        ("say ([)]\n", "Mismatched closing bracket"),
        ("say (]\n", "Mismatched closing bracket"),
        ("say ]\n", "Closing bracket without opening bracket"),
        ("say ([1, 2]\n", "An opening bracket was not closed"),
    ):
        result = run_lexer(se, source)
        combined = result.stdout + result.stderr
        if result.returncode == 0 or expected not in combined:
            raise AssertionError(
                f"Missing diagnostic {expected!r} for {source!r}: "
                f"exit={result.returncode}\n{combined}"
            )
    print("SE selfhost lexer delimiter regression passed")


if __name__ == "__main__":
    main()
