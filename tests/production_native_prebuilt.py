#!/usr/bin/env python3
"""Parity verification for the optional prebuilt-SE-runtime native linker path.

The generated executable intentionally links the production s_core archive.
This test is NOT used as self-hosting proof.
"""
from __future__ import annotations

import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parent.parent
ARCHIVE = ROOT / "build" / "libs_core.a"


def run(*args: str, env: dict[str, str] | None = None) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(
        args, cwd=ROOT, env=env, text=True,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=90
    )
    if result.returncode:
        raise RuntimeError(
            f"Command failed: {args}\nstdout={result.stdout[-4000:]}\nstderr={result.stderr[-4000:]}"
        )
    return result


def main(executable: Path) -> None:
    if not ARCHIVE.is_file():
        raise RuntimeError(f"Build the s_core static archive first: {ARCHIVE}")
    with tempfile.TemporaryDirectory(prefix="se-native-prebuilt-") as temp:
        for name in ("hello.se", "function.se", "type.se", "expansion-all.se"):
            source = Path(temp) / name
            shutil.copyfile(ROOT / "examples" / name, source)
            interpreted = run(str(executable), "run", str(source))
            env = dict(os.environ)
            env["SE_NATIVE_CORE_LIB"] = str(ARCHIVE)
            env["SE_NATIVE_OPT"] = "0"
            run(str(executable), "build", str(source), env=env)
            native = source.with_suffix("")
            generated = run(str(native))
            assert generated.stdout == interpreted.stdout, (
                name, repr(interpreted.stdout), repr(generated.stdout)
            )
            assert generated.stderr == interpreted.stderr, (
                name, repr(interpreted.stderr), repr(generated.stderr)
            )
            print("PASS production prebuilt native parity:", name, flush=True)
    print("SE production prebuilt runtime build parity passed")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("Usage: python3 tests/production_native_prebuilt.py /path/to/se")
    main(Path(sys.argv[1]).resolve())
