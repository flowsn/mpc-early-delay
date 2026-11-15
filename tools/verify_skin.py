#!/usr/bin/env python3
"""Checks the generated skin beyond the SDK validator: names, bindings, dimensions,
film-strip geometry, complete Q-Link reachability, overlays and focus-row rectangles.
"""
import argparse
import json
from pathlib import Path
import sys
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'sdk/tools'))
import gen_ui as ui
import validate


def verify(skin,params_path,quiet=False):
    skin=Path(skin); params=json.loads(Path(params_path).read_text())['params']
    ids={p.get('index',i):p['name'] for i,p in enumerate(params)}
    tui=json.loads((skin/'TUI.json').read_text())['pageData']
    qlinks=json.loads((skin/'Q-Links.json').read_text())
    defs={v['key']:v['value'] for v in tui['componentDefinitions']['localComponentDefinitions']}
    builtin={'Knob','Label','Button','Indicator','Focus','Image','Slider','KnobOverlay','MenuOverlay'}
    errors=[]; rendered=set(); referenced=set()
    def check(cond,msg):
        if not cond: errors.append(msg)
    def handle(h,binding):
        h=binding if h=='Data' else h
        if h.startswith('Parameter '):
            check(h[10:].isdigit() and int(h[10:]) in ids,'Unresolved parameter: '+h)
            return int(h[10:]) if h[10:].isdigit() else None
        check(h in ('Invalid','Focus Lost'),'Unresolved handle: '+h)
        return None
    def component(comp,size,binding='Invalid',depth=0):
        check(depth<8,'Recursive component definition')
        if depth>=8:return
        for action in comp['actions']:
            handle(action['handleName'],binding)
            if action['handler']=='Show Overlay': check(action['additionalData'] in defs,'Missing overlay')
        for e in comp['componentsData']:
            bounds=e['bounds']; x,y,w,h=map(float,bounds['bounds'].split())
            name=e['componentData']['name']; typ=e['componentData']['type']; data=e['componentData']['data']
            check(x>=0 and y>=0 and w>0 and h>0 and x+w<=size[0] and y+h<=size[1],f'Out of bounds: {name} {bounds["bounds"]} in {size}')
            binding2=binding
            for m in e['handle remapping']['map']:
                if m['key']=='Data': binding2=m['value']
                handle(m['value'],binding)
            check(typ in builtin or typ in defs,'Unresolved widget: '+typ)
            if typ in defs:
                idx=handle(binding2,binding)
                if idx is not None: rendered.add(idx)
                component(defs[typ],(w,h),binding2,depth+1)
                continue
            if 'handleName' in data: handle(data['handleName'],binding2)
            if typ=='Label':
                check(data['textStyle']['font']['name'] in ('Titillium Web','Roboto','Nexa'),'Unsupported host font')
            for key in ('image','filmStrip','onImage','offImage'):
                file=data.get(key)
                if not file:continue
                referenced.add(file)
                check((skin/file).is_file(),'Missing image: '+file)
                if not (skin/file).is_file():continue
                with Image.open(skin/file) as im:
                    if key=='filmStrip':
                        frames=data['numFrames']
                        check(im.height%frames==0,'Non-integer film frame height: '+file)
                        check(im.width<=w and im.height//frames<=h,'Knob asset exceeds bounds: '+file)
                    if key in ('onImage','offImage'):check(im.width<=w and im.height<=h,'Button asset exceeds bounds: '+file)
    for tab in tui['tabs']:
        component(tab['componentDefinition'],(1280,628))
    # Overlays are full-screen, outside the tab tree, and retain their duplicate versions.
    for name in ('knob overlay','knob overlay bipolar','menu overlay'):
        component(defs[name],(1280,800),'Parameter '+str(min(ids)))
    ordered=json.loads((skin/'TUI.json').read_text(),object_pairs_hook=ui.Obj)
    versions=[]
    def pairs(node):
        if isinstance(node,ui.Obj):
            if ('overlayType','Knob With Header') in node or ('withNumbers',False) in node:
                versions.append([v for k,v in node if k=='version'])
            for k,v in node:pairs(v)
        elif isinstance(node,list):
            for v in node:pairs(v)
    pairs(ordered)
    check(versions.count([2,1])==2 and versions.count([1,1])==1,'Lost duplicate overlay version keys')
    maps=qlinks['Screen Mode Q-Links']['map']; mapped=set()
    for i,m in enumerate(maps):
        check(m['Bank Direction']=='RowTopDown','Wrong bank direction')
        qm=m['Q-Links']
        check(set(qm)=={f'Q-Link {n}' for n in range(1,17)},'Incomplete 16-Q-Link map')
        assigned=[v for v in qm.values() if v!=-1]
        check(all(v in ids for v in assigned),'Invalid Q-Link index')
        check(len(assigned)==len(set(assigned)),'Duplicate Q-Link assignment on page')
        mapped.update(assigned)
        tab=tui['tabs'][i]
        for e in tab['componentDefinition']['componentsData']:
            remap={x['key']:x['value'] for x in e['handle remapping']['map']}
            p=remap.get('Data','')
            if not p.startswith('Parameter '):continue
            idx=int(p[10:]); q=next((n for n in range(1,17) if qm[f'Q-Link {n}']==idx),None)
            check(q is not None,'Visible control missing Q-Link')
            if q is None:continue
            row=3-(q-1)//4
            rx,ry,rw,rh=map(float,tab['qlinkBoundsData'][row].split())
            x,y,w,h=map(float,e['bounds']['bounds'].split())
            check(x>=rx and y>=ry and x+w<=rx+rw and y+h<=ry+rh,'Control outside its Q-Link row: '+p)
    program=[v for v in qlinks['Program Mode Q-Links'].values() if v!=-1]
    check(set(program)==set(ids) and len(program)==len(ids),'Program Q-Links must cover each parameter exactly once')
    check(mapped==set(ids),'Not all parameters accessible via Q-Links')
    check(rendered==set(ids),'Not all parameters have touchscreen controls')
    report=json.loads((skin/'skin-map.json').read_text())
    for row in report['parameters']:
        check(ids.get(row['index'])==row['parameter'],'Stale name/index mapping')
    # Run the SDK skin checks as well (binary/manifest checks use validate.py separately).
    rep=validate.Report()
    if not quiet:validate.check_skins(rep,str(skin),len(params))
    check(rep.errors==0,'SDK skin validation failed')
    if errors: raise ValueError('\n'.join(errors))
    if not quiet:print(f'PASS: {len(ids)} named controls and Q-Links; {len(referenced)} images; bounds, bindings, overlays resolved')
    return dict(parameters=len(ids),images=len(referenced),pages=len(tui['tabs']))

if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__); ap.add_argument('skin',type=Path);ap.add_argument('--params',required=True,type=Path)
    a=ap.parse_args()
    try:verify(a.skin,a.params)
    except (ValueError,KeyError,OSError) as e:ap.exit(1,str(e)+'\n')
