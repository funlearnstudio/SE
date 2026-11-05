#!/usr/bin/env python3
"""STRICT stage2/stage3 self-hosting verification (no substitutions).

This intentionally fails until the SE-written compiler can compile its complete
own module graph with the independent C++20 runtime. A green unit/differential
test is not proof of self-hosting. Usage:
    python3 tests/selfhost_stage2.py /absolute/path/to/se

Phase 0 uses the existing production SE executable only to bootstrap the
SE-written compiler once. All subsequent compilation of SE source is performed
by the generated native compiler without calling the production compiler.
"""
from __future__ import annotations

import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parent.parent
COMPILER_SOURCE = ROOT / "selfhost" / "compiler.se"
HEADER_DIR = ROOT / "selfhost"
CXX = os.environ.get("CXX", "c++")


def invoke(command: list[str], *, env: dict[str, str] | None = None) -> None:
    proc = subprocess.run(command, cwd=ROOT, env=env, text=True,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          timeout=120, check=False)
    if proc.returncode:
        raise RuntimeError(
            f"Command failed (exit {proc.returncode}): {' '.join(command)}\n"
            f"stdout:\n{proc.stdout}\nstderr:\n{proc.stderr}\n"
        )


def compile_native(generated_cpp: Path, output_binary: Path) -> None:
    # Compile ONLY the generated source and the standalone bootstrap header.
    # No production-SE object files, headers, or static/shared library.
    invoke([CXX, "-std=c++20", "-O0", "-g0",
            "-I", str(HEADER_DIR), str(generated_cpp), "-o", str(output_binary)])


def compile_source(native: Path, target: Path) -> None:
    env = dict(os.environ)
    env["SE_SELFHOST_SOURCE"] = str(COMPILER_SOURCE)
    env["SE_SELFHOST_OUTPUT"] = str(target)
    invoke([str(native)], env=env)
    if not target.is_file() or target.stat().st_size == 0:
        raise RuntimeError(f"Compiler did not emit non-empty C++: {target}")


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify(se: Path) -> None:
    with tempfile.TemporaryDirectory(prefix="se-stage3-") as tmp:
        root = Path(tmp)
        stage1_cpp = root / "stage1.cpp"
        stage2_cpp = root / "stage2.cpp"
        stage3_cpp = root / "stage3.cpp"
        stage4_cpp = root / "stage4.cpp"
        stage1 = root / "stage1"
        stage2 = root / "stage2"
        stage3 = root / "stage3"
        stage0_env = dict(os.environ)
        stage0_env.update(SE_SELFHOST_SOURCE=str(COMPILER_SOURCE),
                          SE_SELFHOST_OUTPUT=str(stage1_cpp))

        # Bootstrap only once using the current production SE CLI.
        invoke([str(se), "run", "selfhost/compiler.se"], env=stage0_env)
        if not stage1_cpp.exists():
            raise RuntimeError("Stage0 SE compiler did not generate stage1 C++.")
        compile_native(stage1_cpp, stage1)

        # Stage1 is standalone and must independently compile its own SE source.
        compile_source(stage1, stage2_cpp)
        compile_native(stage2_cpp, stage2)

        # Stage2 independently recompiles its own SE source to produce Stage3.
        compile_source(stage2, stage3_cpp)
        compile_native(stage3_cpp, stage3)
        compile_source(stage3, stage4_cpp)

        # Compare generated sources rather than platform-specific binary metadata.
        hashes = [digest(stage2_cpp), digest(stage3_cpp), digest(stage4_cpp)]
        if len(set(hashes)) != 1:
            raise RuntimeError(
                "Stage2/3/4 source mismatch (determinism not proven): "
                + ", ".join(hashes)
            )
        print("STRICT SELF-HOSTING VERIFIED: Stage2 = Stage3 = Stage4")
        print("No production s_core linked into generated native compilers.")
        print("C++20 source SHA256:", hashes[0])


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("Usage: python3 tests/selfhost_stage2.py /path/to/se")
    se_binary = Path(sys.argv[1]).expanduser().resolve()
    if not se_binary.is_file():
        raise SystemExit(f"SE bootstrap executable not found: {se_binary}")
    try:
        verify(se_binary)
    except (RuntimeError, subprocess.TimeoutExpired) as exc:
        print("STAGE2/3 NOT VERIFIED:", exc, file=sys.stderr)
        raise SystemExit(1)
