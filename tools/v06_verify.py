#!/usr/bin/env python3
"""Static v0.6 acceptance + independent byte reproduction comparison (not host touch simulation)."""
import hashlib, json, pathlib, sys
ROOT=pathlib.Path(__file__).resolve().parents[1]
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    skin=ROOT/'plugin/skins'; repro=ROOT/'build/v06-repro/skins'
    a={p.name:digest(p) for p in skin.iterdir() if p.is_file()}
    b={p.name:digest(p) for p in repro.iterdir() if p.is_file()}
    assert a==b,'Skin generation is not byte reproducible'
    for name in ('preview-delay.png','preview-delay-more1.png'):
        assert digest(ROOT/'reports/v0.6'/name)==digest(ROOT/'build/v06-repro'/name),name
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
    # No static Text widgets: v0.5 hardware omitted these despite SDK-valid JSON.
    def visit(node):
        if isinstance(node,dict):
            if node.get('type')=='Label': assert node['data']['type']!='Text'
            for value in node.values(): visit(value)
        elif isinstance(node,list):
            for value in node: visit(value)
    visit(tui)
    from PIL import Image, ImageChops
    for tab in tui['tabs']:
        for entry in tab['componentDefinition']['componentsData']:
            kind=entry['componentData']['type']
            if kind not in defs: continue
            control=defs[kind]
            x,y,w,h=map(int,entry['bounds']['bounds'].split())
            for child in control['componentsData']:
                if child['componentData']['name']!='name': continue
                xx,yy,ww,hh=map(int,child['bounds']['bounds'].split())
                data=child['componentData']
                assert data['type']=='Image', (kind,'legend not rasterized')
                with Image.open(skin/data['data']['image']) as im:
                    assert im.size==(ww,hh) and im.getbbox(),kind
                if yy==0:
                    assert xx>=36 and x+xx>=66,(kind,'Q badge collision')
    for key in ('hold','invert','stereoize'):
        with Image.open(skin/(key+'_off.png')) as off, Image.open(skin/(key+'_on.png')) as on:
            assert ImageChops.difference(off.convert('RGB'),on.convert('RGB')).crop((11,17,44,58)).getbbox(),key
    # Both MORE captions and labels must have actual PNGs; same name as host parameter.
    more=tui['tabs'][1]['componentDefinition']['componentsData']
    assert [e['componentData']['name'] for e in more if e['componentData']['type'] in defs]==['Ping Width','Output']
    names=[x['componentData']['name'] for x in tui['tabs'][0]['componentDefinition']['componentsData']]
    assert names.index('TONE')+1==names.index('DRIVE') and names.index('DRIVE')+1==names.index('BANDWIDTH')
    assert names.index('FEEDBACK')+1==names.index('INVERT')
    params=json.loads((ROOT/'build/params.json').read_text());meta=json.loads((ROOT/'plugin/plugin.json').read_text())
    assert params['name']==meta['name']=='80s Lofi Delay Box' and params['vendor']=='Flat Peach'
    assert params['uid']==meta['uid']=='FpEd' and params['version']==600 and meta['version']=='0.6.0'
    assert len(params['params'])==14 and params['numPrograms']==20
    print(f'PASS byte-identical skin ({len(a)} files, {size} bytes / {size/1024:.2f} KiB) and both page previews')
    print('PASS whole-slot one-handler tap/Enter toggles, passive state children, no Focus widgets')
    print('PASS Feedback/Invert adjacency; Tone/Drive/Bandwidth order; product identity, 14 parameters/20 presets')
    print('PASS both MORE names are nonempty image assets; 42px Q-badge envelope clears all top labels; slide thumb changes state')
    print('LIMIT: static SDK validation is not physical-device gesture execution')
if __name__=='__main__':main()
