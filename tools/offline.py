#!/usr/bin/env python3
"""Build and run the offline engine harness (tests/offline/offline.cpp).

Usage:
    python3 tools/offline.py selftest              # pass/fail checks (exit code)
    python3 tools/offline.py measure [out.md]       # measurement tables (default: stdout); if out.md
                                                    # contains <!-- BEGIN GENERATED --> / <!-- END GENERATED -->
                                                    # only the part between the markers is replaced
    python3 tools/offline.py demos [dir]            # demo WAVs (default: demos/, gitignored)

Compiles with the host C++ compiler (c++/g++/clang++), -O2, against the same engine header the
plugin uses. Binary goes to build/offline.
"""
import os
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def build():
    cxx = os.environ.get("CXX") or shutil.which("c++") or shutil.which("g++") or shutil.which("clang++")
    if not cxx:
        sys.exit("no C++ compiler found")
    out = os.path.join(ROOT, "build", "offline")
    os.makedirs(os.path.dirname(out), exist_ok=True)
    src = os.path.join(ROOT, "tests", "offline", "offline.cpp")
    deps = [src, os.path.join(ROOT, "plugin", "src", "early_delay_dsp.h")]
    if os.path.exists(out) and all(os.path.getmtime(out) > os.path.getmtime(d) for d in deps):
        return out
    cmd = [cxx, "-O2", "-std=c++17", "-Wall", "-Wextra", "-I", os.path.join(ROOT, "sdk", "framework"),
           "-I", os.path.join(ROOT, "plugin", "src"), src, "-o", out]
    print("$ " + " ".join(cmd), file=sys.stderr, flush=True)
    subprocess.check_call(cmd)
    return out


def main():
    if len(sys.argv) < 2 or sys.argv[1] not in ("selftest", "measure", "demos"):
        sys.exit(__doc__)
    exe = build()
    cmd = sys.argv[1]
    if cmd == "demos":
        d = sys.argv[2] if len(sys.argv) > 2 else os.path.join(ROOT, "demos")
        os.makedirs(d, exist_ok=True)
        return subprocess.call([exe, "demos", d])
    if cmd == "measure" and len(sys.argv) > 2:
        p = subprocess.run([exe, "measure"], stdout=subprocess.PIPE)
        text = p.stdout.decode()
        target = sys.argv[2]
        begin, end = "<!-- BEGIN GENERATED -->", "<!-- END GENERATED -->"
        if os.path.exists(target):
            old = open(target).read()
            if begin in old and end in old:
                text = old[:old.index(begin) + len(begin)] + "\n" + text + old[old.index(end):]
        with open(target, "w") as f:
            f.write(text)
        return p.returncode
    return subprocess.call([exe, cmd])


if __name__ == "__main__":
    sys.exit(main())
