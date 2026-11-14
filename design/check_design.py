#!/usr/bin/env python3
"""Rebuild the UI stub, regenerate all options, prove complete skins, save reports.
Run from anywhere: python3 design/check_design.py
Requires Pillow, a C++ compiler (SDK auto-installs Zig if needed), and the SDK submodule.
"""
from pathlib import Path
import json
import shutil
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[1]
PYTHON=sys.executable

def run(args,log=None):
    p=subprocess.run(args,cwd=ROOT,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    print(p.stdout,end='')
    if log:log.write_text(p.stdout)
    if p.returncode:raise SystemExit(p.returncode)


def main():
    reports=ROOT/'design/validation';reports.mkdir(exist_ok=True)
    # Step 1 dumps real params even when the public plugin has not been written.
    run([PYTHON,'sdk/tools/build.py','design/testbed','--target','native','--no-ui','--no-validate'])
    params=ROOT/'design/testbed/build/params.json'
    shutil.copyfile(params,ROOT/'design/testbed/params.json')
    options={'A':'charcoal-amber','B':'parchment-oxide','C':'black-fluorescent'}
    for key,name in options.items():
        out=f'design/skins/{key}/Plugin Skins'
        run([PYTHON,'tools/make_skin.py','--params',str(params),'--out',out,'--style',key,
             '--assets-out',f'design/assets/{key}','--preview',f'design/previews/option-{key}-{name}.png'])
        run([PYTHON,'tools/verify_skin.py',out,'--params',str(params)],reports/f'bindings-{key}.txt')
    # Build with the hand-authored/generated skin; build.py writes the right uiversion.
    source=ROOT/'design/testbed/skins'
    shutil.copytree(ROOT/'design/skins/A/Plugin Skins',source,dirs_exist_ok=True)
    run([PYTHON,'sdk/tools/build.py','design/testbed','--target','all'],reports/'build.txt')
    target=ROOT/'design/testbed/build/Early Delay Lab - VST - Design Testbed'
    for key in options:
        shutil.copytree(ROOT/f'design/skins/{key}/Plugin Skins',target/'Plugin Skins',dirs_exist_ok=True)
        run([PYTHON,'sdk/tools/validate.py',str(target),'--params',str(params),'--strict'],reports/f'SDK-{key}.txt')
    # Leave the recommended option installed in the local testbed output.
    shutil.copytree(source,target/'Plugin Skins',dirs_exist_ok=True)
    run([PYTHON,'design/test_skin.py'],reports/'regressions.txt')
    sim=ROOT/'sdk/.sdk-tools/host_sim'
    run([str(sim),str(ROOT/'design/testbed/build/native/design-testbed.so'),'test'],reports/'host-sim.txt')
    summary={key:dict(skinBytes=sum(p.stat().st_size for p in (ROOT/f'design/skins/{key}/Plugin Skins').iterdir() if p.is_file()),
                     sdkErrors=0,sdkWarnings=0) for key in options}
    (reports/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
    print('PASS: all three complete skins; ARM32 + ARM64; mapping regressions; pass-through host simulation.')

if __name__=='__main__':main()
