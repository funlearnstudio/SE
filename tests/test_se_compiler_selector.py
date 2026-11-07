#!/usr/bin/env python3
"""Zero-dependency backend selection regression tests; no compiler build needed."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent
LAUNCHER = ROOT / "tools" / "se_compiler.py"


class BackendSelectionTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.folder = Path(self.tmp.name)
        self.source = self.folder / "app.se"
        self.source.write_text('say "ok"\n', encoding="utf-8")
        self.log = self.folder / "called.txt"
        self.stub = self.folder / "stub.py"
        self.stub.write_text(
            '#!/usr/bin/env python3\n'
            'import os,sys,pathlib\n'
            'pathlib.Path(os.environ["SE_TEST_LOG"]).write_text('
            'str(sys.argv[1:])+"\\n"+str([os.environ.get(x) for x in '
            '("SE_SELFHOST_SOURCE","SE_SELFHOST_OUTPUT","SE_SELFHOST_TARGET")]))\n'
            'p=os.environ.get("SE_SELFHOST_OUTPUT")\n'
            'if p and os.environ.get("SE_SELFHOST_TARGET") != "web": '
            'pathlib.Path(p).write_text("// generated cpp\\n")\n',
            encoding="utf-8",
        )
        self.stub.chmod(0o755)
        self.env = dict(os.environ, SE_TEST_LOG=str(self.log))

    def invoke(self, *args):
        return subprocess.run([sys.executable, str(LAUNCHER), *args],
                              env=self.env, capture_output=True, text=True)

    def test_production_check_is_forwarded(self):
        result = self.invoke("--backend", "cmake", "--cmake-bin", str(self.stub),
                             "check", str(self.source))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("'check'", self.log.read_text())
        self.assertIn(str(self.source), self.log.read_text())

    def test_selfhost_emits_cpp_and_marks_native_target(self):
        output = self.folder / "out.cpp"
        result = self.invoke("--backend", "se", "--selfhost-bin", str(self.stub),
                             "-o", str(output), "compile", str(self.source))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue(output.is_file())
        self.assertIn("'native'", self.log.read_text())

    def test_selfhost_web_target(self):
        result = self.invoke("--backend", "se", "--selfhost-bin", str(self.stub),
                             "web", str(self.source))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("'web'", self.log.read_text())

    def test_no_silent_production_fallback(self):
        result = self.invoke("--backend", "se", "--selfhost-bin",
                             str(self.folder / "missing"), "compile", str(self.source))
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("not found", result.stderr)

    def test_reject_unsupported_selfhost_check(self):
        result = self.invoke("--backend", "se", "--selfhost-bin",
                             str(self.stub), "check", str(self.source))
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse(self.log.exists())


if __name__ == "__main__":
    unittest.main()
