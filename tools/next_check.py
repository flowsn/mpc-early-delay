#!/usr/bin/env python3
"""Rebuild and run repaired-design regressions, preserving each log as it finishes.
Use --sanitize for ASan/UBSan/float-cast-overflow runs in addition to normal probes.
"""
import argparse, pathlib, subprocess, re, sys
ROOT=pathlib.Path(__file__).resolve().parents[1]
R=ROOT/'reports/next-20261114'; LOG=R/'logs'; LOG.mkdir(exist_ok=True)
def run(cmd, log):
    print(' '.join(map(str,cmd)),flush=True)
    with (LOG/log).open('w') as f:
        subprocess.run(list(map(str,cmd)),cwd=ROOT,stdout=f,stderr=subprocess.STDOUT,check=True)
    return (LOG/log).read_text()
def compile_run(name,src,sanitize=False,args=()):
    exe=ROOT/'build'/(name+('_sanitized' if sanitize else ''))
    flags=['-O1','-g','-fsanitize=address,undefined,float-cast-overflow','-fno-omit-frame-pointer'] if sanitize else ['-O2']
    run(['c++',*flags,'-std=c++17','-I','sdk/framework','-I','plugin/src',src,'-o',exe],exe.name+'-build.log')
    return run([exe,*args],exe.name+'.log')
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--sanitize',action='store_true');a=ap.parse_args()
    probe=compile_run('probe',R/'probe.cpp')
    assert 'RESET max_delta=0 ' in probe and 'INPLACE identical=1' in probe
    assert 'CALLBACK_NEW_COUNT 0' in probe
    assert len(re.findall(r'BAD_AUDIO.*nonfinite=0',probe))==2
    assert 'AUTOMATION frames=2560000 nonfinite=0' in probe
    for got,want in re.findall(r'after_Hz=([\d.]+) expected_Hz=([\d.]+)',probe):
        assert abs(float(got)/float(want)-1)<.02,(got,want)
    for loss in re.findall(r'mono_loss_dB=(-?[\d.]+)',probe):assert -3.1<float(loss)<.1
    life=compile_run('lifecycle_probe',R/'lifecycle_probe.cpp')
    assert len(re.findall(r'RESET history=\d+ max_delta=0',life))==2
    compile_run('spectral_probe',R/'spectral_probe.cpp')
    framework=compile_run('framework_probe',R/'framework_probe.cpp')
    assert 'PARAMS 12 PROGRAMS 11' in framework and framework.count('roundtrip=1')==11
    with (LOG/'parameter-nan.log').open('w') as f:
        for i in range(12):
            text=subprocess.check_output([str(ROOT/'build/framework_probe'),str(i)],text=True)
            f.write(text);f.flush();assert 'nonfinite=0' in text
    demos=ROOT/'demos/next-20261114';demos.mkdir(parents=True,exist_ok=True)
    compile_run('next_regression',ROOT/'tests/offline/next_regression.cpp',args=[demos])
    if a.sanitize:
        for name,src in [('probe',R/'probe.cpp'),('next_regression',ROOT/'tests/offline/next_regression.cpp'),('framework_probe',R/'framework_probe.cpp')]:
            compile_run(name,src,True)
    (LOG/'regression-summary.txt').write_text('PASS: old-fault assertions, new stereo/HOLD assertions, 11 presets, all 12 invalid controls; '+('ASan/UBSan/float-cast-overflow passed.\n' if a.sanitize else 'sanitizers not requested.\n'))
    print((LOG/'regression-summary.txt').read_text())
if __name__=='__main__':main()
