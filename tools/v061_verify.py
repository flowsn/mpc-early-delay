#!/usr/bin/env python3
"""Real VST dispatcher notifications + JSON-bound widget model; no MPC renderer."""
import argparse
import json
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
LOG = ROOT / 'reports/v0.6.1/logs'

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--sanitize', action='store_true')
    args = ap.parse_args()
    LOG.mkdir(parents=True, exist_ok=True)
    exe = ROOT / 'build/v061_gui_refresh'
    flags = ['-O2']
    if args.sanitize:
        exe = exe.with_name(exe.name + '_sanitized')
        flags = ['-O1', '-g', '-fsanitize=address,undefined,float-cast-overflow', '-fno-omit-frame-pointer']
    with (LOG / (exe.name + '-build.log')).open('w') as f:
        subprocess.run(['c++', *flags, '-std=c++17', '-I', 'sdk/framework', '-I', 'plugin/src',
                        'tests/offline/v061_gui_refresh.cpp', '-ldl', '-o', str(exe)],
                       cwd=ROOT, stdout=f, stderr=subprocess.STDOUT, check=True)
    # Sanitized harness also loads a sanitized plugin, so the adapter is instrumented.
    library = ROOT / 'plugin/build/native/80s-lofi-delay-box.so'
    if args.sanitize:
        library = ROOT / 'build/v061-plugin-sanitized.so'
        with (LOG / 'gui-plugin-sanitized-build.log').open('w') as f:
            subprocess.run(['c++', *flags, '-std=c++17', '-shared', '-fPIC', '-fvisibility=hidden',
                            '-I', 'sdk/framework', '-I', 'plugin/src', 'plugin/src/EarlyDelay.cpp',
                            '-o', str(library)], cwd=ROOT, stdout=f, stderr=subprocess.STDOUT, check=True)
    run = subprocess.run([str(exe), str(library)], cwd=ROOT, capture_output=True, text=True)
    (LOG / (exe.name + '.log')).write_text(run.stdout + run.stderr)
    assert run.returncode == 0, run.stdout + run.stderr
    states = {}
    for line in run.stdout.splitlines():
        if line.startswith('STATE '):
            _, name, *values = line.split()
            states[name] = list(map(float, values))
    assert len(states) == 82  # 20 * (program + repeat + two chunk modes) + initial + custom
    skin = ROOT / 'plugin/skins'
    tui = json.loads((skin / 'TUI.json').read_text())['pageData']
    defs = {d['key']: d['value'] for d in tui['componentDefinitions']['localComponentDefinitions']}
    params = json.loads((ROOT / 'plugin/build/params.json').read_text())
    assert params['version'] == 601 and params['numPrograms'] == 20 and len(params['params']) == 14
    assert params['uid'] == 'FpEd' and params['name'] == '80s Lofi Delay Box'
    bound = set()
    switches = {}
    widget_checks = 0
    pages = []
    for tab in tui['tabs']:
        pages.append(tab['tabName'])
        for item in tab['componentDefinition']['componentsData']:
            kind = item['componentData']['type']
            if kind not in defs:
                continue
            mapping = {m['key']: m['value'] for m in item['handle remapping']['map']}
            index = int(mapping['Data'].removeprefix('Parameter '))
            assert index not in bound
            bound.add(index)
            component = defs[kind]
            dynamic = [c['componentData'] for c in component['componentsData']
                       if c['componentData']['type'] in ('Knob', 'Indicator', 'Label')]
            assert len(dynamic) == 2  # visual value + parameter-bound text, no stale local state
            for widget in dynamic:
                assert widget['data']['handleName'] == 'Data', (kind, widget)
                if widget['type'] == 'Label':
                    assert widget['data']['type'] == 'Value'
            if kind in ('control hold', 'control invert', 'control stereoize'):
                assert [(a['onAction'], a['handler']) for a in component['actions']] == [
                    ('Mouse Down', 'Toggle Switch'), ('Enter Pressed', 'Toggle Switch')]
                indicator = next(w['data'] for w in dynamic if w['type'] == 'Indicator')
                assert indicator['indicatorId'] == 1 and indicator['numIndicatorsInGroup'] == 2
                assert (skin / indicator['onImage']).read_bytes() != (skin / indicator['offImage']).read_bytes()
                switches[kind] = set()
            # Simulate navigating to either page AFTER loading the host cache. The
            # hidden page has no independent parameter store; Data resolves identically.
            for values in states.values():
                v = values[index]
                assert 0 <= v <= 1
                for widget in dynamic:
                    if widget['type'] == 'Indicator':
                        selected = widget['data']['onImage' if v >= .5 else 'offImage']
                        assert (skin / selected).is_file()
                        switches[kind].add(v >= .5)
                    elif widget['type'] == 'Knob':
                        count = widget['data']['numFrames']
                        frame = round(v * (count - 1))
                        assert 0 <= frame < count
                    # Label contents were independently compared by the native host callback.
                    widget_checks += 1
    assert pages == ['DELAY', 'MORE'] and bound == set(range(14))
    assert all(seen == {False, True} for seen in switches.values())
    # This is a refresh-only hotfix: preserve every skin byte and factory/DSP behavior.
    tracked = subprocess.check_output(['git', 'ls-tree', '-r', '--name-only', '4f3b4ab', 'plugin/skins'], cwd=ROOT, text=True).splitlines()
    for name in tracked:
        old = subprocess.check_output(['git', 'show', '4f3b4ab:' + name], cwd=ROOT)
        assert (ROOT / name).read_bytes() == old, name
    current_source = (ROOT / 'plugin/src/EarlyDelay.cpp').read_text()
    old_source = subprocess.check_output(['git', 'show', '4f3b4ab:plugin/src/EarlyDelay.cpp'], cwd=ROOT, text=True)
    assert [s.strip() for s in current_source.splitlines() if 'addProgram(' in s] == [s.strip() for s in old_source.splitlines() if 'addProgram(' in s]
    for name in ('plugin/src/early_delay_dsp.h', 'plugin/ui.json'):
        assert (ROOT / name).read_bytes() == subprocess.check_output(['git', 'show', '4f3b4ab:' + name], cwd=ROOT)
    print(run.stdout.splitlines()[-1])
    print(f'PASS 82 loads x 14 controls x 2 bound widgets = {widget_checks} widget-model checks on DELAY/MORE')
    print(f'PASS all three switches both states; {len(tracked)} skin files byte-identical to v0.6; DSP/ui.json unchanged')
    print('LIMIT: SDK host callback/cache and skin binding model, not execution of proprietary MPC rendering/touch code')

if __name__ == '__main__':
    main()
