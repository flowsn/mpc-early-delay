#!/usr/bin/env python3
"""Static v0.5 acceptance + independent byte reproduction comparison (not host touch simulation)."""
import hashlib, json, pathlib, sys
ROOT=pathlib.Path(__file__).resolve().parents[1]
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    skin=ROOT/'plugin/skins'; repro=ROOT/'build/v05-repro/skins'
    a={p.name:digest(p) for p in skin.iterdir() if p.is_file()}
    b={p.name:digest(p) for p in repro.iterdir() if p.is_file()}
    assert a==b,'Skin generation is not byte reproducible'
    for name in ('preview-delay.png','preview-delay-more1.png'):
        assert digest(ROOT/'reports/v0.5'/name)==digest(ROOT/'build/v05-repro'/name),name
    size=sum(p.stat().st_size for p in skin.iterdir() if p.is_file());assert size<=600*1024,size
    tui=json.loads((skin/'TUI.json').read_text())['pageData']; defs={x['key']:x['value'] for x in tui['componentDefinitions']['localComponentDefinitions']}
    assert len(tui['tabs'])==2
    for d in defs.values():
        assert not any(c['componentData']['type']=='Focus' for c in d['componentsData'])
    for key in ('hold','stereoize','invert'):
        d=defs['control '+key]; actions=d['actions']
        assert [(a['onAction'],a['handler']) for a in actions]==[('Mouse Down','Toggle Switch'),('Enter Pressed','Toggle Switch')]
        types=[c['componentData']['type'] for c in d['componentsData']]
        assert types.count('Indicator')==1 and not any(t in types for t in ('Button','Slider','Knob'))
        lamp=next(c['componentData']['data'] for c in d['componentsData'] if c['componentData']['type']=='Indicator')
        assert lamp['indicatorId']==1 and lamp['numIndicatorsInGroup']==2
    names=[x['componentData']['name'] for x in tui['tabs'][0]['componentDefinition']['componentsData']]
    assert names.index('TONE')+1==names.index('DRIVE') and names.index('DRIVE')+1==names.index('BANDWIDTH')
    assert names.index('FEEDBACK')+1==names.index('INVERT')
    params=json.loads((ROOT/'build/params.json').read_text());meta=json.loads((ROOT/'plugin/plugin.json').read_text())
    assert params['name']==meta['name']=='80s Lofi Delay Box' and params['vendor']=='Flat Peach'
    assert params['uid']==meta['uid']=='FpEd' and params['version']==500 and meta['version']=='0.5.0'
    assert len(params['params'])==14 and params['numPrograms']==17
    print(f'PASS byte-identical skin ({len(a)} files, {size} bytes / {size/1024:.2f} KiB) and both page previews')
    print('PASS whole-slot one-handler tap/Enter toggles, passive state children, no Focus widgets')
    print('PASS Feedback/Invert adjacency; Tone/Drive/Bandwidth order; product identity, 14 parameters/17 presets')
    print('LIMIT: static SDK validation is not physical-device gesture execution')
if __name__=='__main__':main()
