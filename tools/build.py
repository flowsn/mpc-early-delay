#!/usr/bin/env python3
"""Build Early Delay (plugin/) for native + arm32 + arm64, validate, and run the host simulator.

Usage:
    python3 tools/build.py [--skip-test] [--target all|native|arm32|arm64] [extra args for sdk build.py]

Steps:
    1. sdk/tools/build.py plugin --target all   (native build, host_sim dump, arm32, arm64,
                                                 output folder, version.xml, validator)
    2. copy plugin/build/params.json -> build/params.json   (input for tools/make_skin.py)
    3. host_sim test on the native build at block sizes 32, 128 and 512 (WAV of the 128 run
       goes to build/host-sim.wav)

The SDK's zig toolchain is reused from $ZIG if set, else from a sibling SDK checkout's
.sdk-tools/zigenv if one exists (saves the ~50 MB download), else the SDK downloads it.
Exit code 0 only if every step passed.
"""
import glob
import os
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SDK = os.path.join(ROOT, "sdk")
PROJECT = os.path.join(ROOT, "plugin")


def find_zig():
    if os.environ.get("ZIG"):
        return os.environ["ZIG"]
    for base in (os.path.join(SDK, ".sdk-tools"), os.path.join(ROOT, "..", "sdk", ".sdk-tools")):
        hits = glob.glob(os.path.join(base, "zigenv", "lib", "python*", "site-packages", "ziglang", "zig"))
        if hits:
            return os.path.abspath(hits[0])
    return None


def main():
    args = sys.argv[1:]
    skip_test = "--skip-test" in args
    args = [a for a in args if a != "--skip-test"]
    if "--target" not in args:
        args += ["--target", "all"]
    env = dict(os.environ)
    zig = find_zig()
    if zig:
        env["ZIG"] = zig
    cmd = [sys.executable, os.path.join(SDK, "tools", "build.py"), PROJECT] + args
    print("$ " + " ".join(cmd), flush=True)
    rc = subprocess.call(cmd, env=env)
    if rc != 0:
        print("SDK build failed (exit %d)" % rc)
        return rc

    src = os.path.join(PROJECT, "build", "params.json")
    if os.path.exists(src):
        os.makedirs(os.path.join(ROOT, "build"), exist_ok=True)
        shutil.copyfile(src, os.path.join(ROOT, "build", "params.json"))
        print("params.json -> build/params.json")
    else:
        print("WARNING: plugin/build/params.json missing (host_sim dump did not run)")
        return 1

    if skip_test:
        return 0
    host_sim = os.path.join(SDK, ".sdk-tools", "host_sim")
    libs = glob.glob(os.path.join(PROJECT, "build", "native", "*.so")) + \
        glob.glob(os.path.join(PROJECT, "build", "native", "*.dylib"))
    if not (os.path.exists(host_sim) and libs):
        print("host_sim or native build missing; cannot test")
        return 1
    worst = 0
    for block in (32, 64, 96, 128, 192, 256, 512):
        cmd = [host_sim, libs[0], "test", "--seconds", "3", "--block", str(block)]
        if block == 128:
            cmd += ["--wav", os.path.join(ROOT, "build", "host-sim.wav")]
        print("$ " + " ".join(cmd), flush=True)
        p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        out = p.stdout.decode("utf-8", "replace")
        tail = [l for l in out.splitlines() if l.strip()][-3:]
        print("\n".join(tail))
        if "WARN " in out or "FAIL " in out:
            print("\n".join(l for l in out.splitlines() if l.startswith(("WARN", "FAIL"))))
        worst = max(worst, p.returncode)
    print("host_sim: %s" % ("PASS" if worst == 0 else "FAIL (exit %d)" % worst))
    return worst


if __name__ == "__main__":
    sys.exit(main())
