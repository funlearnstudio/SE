#!/usr/bin/env python3
"""Compile Web with SE-written backend and compare actual browser assets."""
from pathlib import Path
import json
import os
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parent.parent
SE = Path(sys.argv[1]).resolve()
NATIVE = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else None

def run(command, env=None):
    proc = subprocess.run([str(x) for x in command], cwd=ROOT, env=env,
                          capture_output=True, text=True, timeout=120)
    assert proc.returncode == 0, (command, proc.stdout, proc.stderr)
    return proc

def canonical_js(source):
    # Production uses an unordered set; route registration order is immaterial.
    return re.sub(r'const __seRoutes = new Set\((\[.*?\])\);',
                  lambda m: 'const __seRoutes = new Set(' + json.dumps(sorted(json.loads(m[1]))) + ');', source)

with tempfile.TemporaryDirectory(prefix='se-web-') as folder:
    tmp = Path(folder)
    for case in ['component-web.se', 'browser-api.se']:
        actual = tmp / (case + '-self')
        expected = tmp / (case + '-prod')
        env = dict(os.environ, SE_SELFHOST_SOURCE=str(ROOT / 'examples' / case),
                   SE_SELFHOST_OUTPUT=str(actual), SE_SELFHOST_TARGET='web')
        run([NATIVE] if NATIVE else [SE, 'run', 'selfhost/compiler.se'], env)
        run([SE, 'web', 'build', ROOT / 'examples' / case, expected])
        for name in ['index.html', 'style.css', 'app.js', 'app.ts']:
            a = (actual / name).read_text()
            b = (expected / name).read_text()
            if name.endswith(('.js', '.ts')):
                a, b = canonical_js(a), canonical_js(b)
            assert a == b, (case, name, 'browser assets differ')
        run(['node', '--check', actual / 'app.js'])
        print('PASS Web assets and JS syntax:', case, flush=True)
    for source, message in [
        ('make Button text\n    html\n        button text\npage "/"\n    Button "a" "b"\n', 'Wrong arity'),
        ('make Button\n    html\n        button "ok"\npage "/"\n    Button\npage "/"\n    Button\n', 'Duplicate page route'),
        ('make Button\n    html\n        button "ok"\npage "bad"\n    Button\n', "must begin with '/'")
    ]:
        src = tmp / 'invalid.se'
        src.write_text(source)
        out = tmp / 'invalid-output'
        env = dict(os.environ, SE_SELFHOST_SOURCE=str(src),
                   SE_SELFHOST_OUTPUT=str(out), SE_SELFHOST_TARGET='web')
        proc = subprocess.run([str(NATIVE)] if NATIVE else [str(SE), 'run', 'selfhost/compiler.se'],
                              cwd=ROOT, env=env, capture_output=True, text=True, timeout=120)
        assert proc.returncode != 0 and message in proc.stderr, proc.stderr
        assert not out.exists(), 'Invalid Web program wrote output'
        print('PASS rejected Web:', message, flush=True)
print('SE selfhost Web tests passed')
