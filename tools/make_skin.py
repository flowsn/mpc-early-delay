#!/usr/bin/env python3
"""Generate the complete Early Delay skin from an actual host_sim parameter dump.
No fixed parameter indices; explicit name aliases live in design/layout.json.
"""
import argparse
import json
from pathlib import Path
import re
import shutil
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'sdk/tools'))
sys.path.insert(0,str(ROOT/'design'))
import gen_ui as ui
from render_assets import render_assets, THEMES, FRAMES, XS, YS, SLOT_W, SLOT_H, DEFAULT_WEAR, DEFAULT_SEED, wear_amount


def normalized(name):
    return re.sub(r'[^a-z0-9]','',name.lower())

def argb(s): return 'ff'+s.lstrip('#')

def rect(*r): return ' '.join(str(v) for v in r)

def get(o,k): return next(v for key,v in o if key==k)

def replace(o,k,value):
    for i,(key,_) in enumerate(o):
        if key==k: o[i]=(k,value); return
    o.append((k,value))

def element(name,kind,data,bounds,focus='No',remap=()):
    return ui.entry(name,kind,data,rect(*bounds),focus,'Show','Always',remap=remap)

def image_entry(name,file,bounds):
    data=ui.obj(('version',2),('imageType','Regular'),('colour','0'),('image',file))
    return element(name,'Image',data,bounds)

def label(name,value,bounds,colour,size=22,kind='Text',bold=False,align='left verticallyCentred'):
    data=ui.label_data(kind,argb(colour),size,align,value if kind=='Text' else None)
    if kind=='Text': replace(data,'handleName','Invalid')
    if bold: replace(get(get(data,'textStyle'),'font'),'style','SemiBold')
    return element(name,'Label',data,bounds)

def match_params(params,layout,mapping):
    known={c['key'] for c in layout['controls']}
    unknown=set(mapping)-known
    if unknown: raise ValueError('Unknown mapping keys: '+', '.join(sorted(unknown)))
    names={}
    for p in params: names.setdefault(normalized(p.name),[]).append(p)
    if len({p.index for p in params})!=len(params): raise ValueError('Duplicate parameter indices')
    if sorted(p.index for p in params)!=list(range(len(params))):
        raise ValueError('Expected contiguous host_sim parameter indices starting at zero')
    assigned=[]; used=set()
    for spec in layout['controls']:
        if spec.get('fallbackOnly'):
            if spec['key'] in mapping:
                raise ValueError('Control is reserved for fallback pages: '+spec['key'])
            assigned.append((spec,None))
            continue
        matches=[]
        if spec['key'] in mapping:
            matches=[p for p in params if p.name==mapping[spec['key']]]
            if len(matches)!=1: raise ValueError('Explicit mapping does not identify one parameter: '+spec['key'])
        else:
            for alias in spec['aliases']:
                matches.extend(p for p in names.get(normalized(alias),[]) if p not in matches)
        if len(matches)>1:
            raise ValueError('Ambiguous '+spec['key']+': '+', '.join(p.name for p in matches)+'; use --mapping')
        p=matches[0] if matches else None
        if p and p.index in used: raise ValueError('Parameter matched twice: '+p.name)
        if p: used.add(p.index)
        assigned.append((spec,p))
    return assigned,[p for p in params if p.index not in used]


def control_definition(spec,p,index,th):
    is_switch=spec.get('switch') and p.is_discrete and len(p.choices)==2
    bipolar=bool(spec.get('bipolar')) and not p.is_discrete
    overlay='menu overlay' if p.is_discrete else ('knob overlay bipolar' if bipolar else 'knob overlay')
    children=[]
    if is_switch:
        key=spec['key']
        children.append(label('caption',spec['caption'],(4,0,276,28),th['muted'],18))
        children.append(element('state lamp','Indicator',ui.obj(('version',1),('onImage',key+'_on.png'),('offImage',key+'_off.png'),('indicatorId',1),('numIndicatorsInGroup',2),('handleName','Data')),(3,35,278,76)))
        children.append(image_entry('name',f'name_{spec["key"]}.png',(48,46,176,45)) if th.get('engraved') else label('name',spec['label'],(48,46,176,45),th.get('buttonText',th['text']),25,bold=True))
        children.append(label('state','',(220,49,52,40),th.get('buttonText',th['text']),22,kind='Value',align='right verticallyCentred'))
        children.append(label('hint','TAP TO TOGGLE',(4,115,276,24),th['muted'],15))
    else:
        size=112 if index<4 else 88
        ky=33 if size==112 else 45
        children.append(image_entry('name',f'name_{spec["key"]}' + ('_knob.png' if spec.get('switch') else '.png'),(4,0,276,30)) if th.get('engraved') else label('name',spec['label'],(4,0,276,30),th['text'],22,bold=True))
        children.append(element('knob','Knob',ui.knob_filmstrip_data(f'knob_{size}_{"bi" if bipolar else "uni"}.png',FRAMES),(0,ky,size,size)))
        children.append(image_entry('readout plate','value_plate.png',(128,48,150,48)))
        children.append(label('value','',(132,48,142,48),th['value'],28,kind='Value',align='horizontallyCentred verticallyCentred'))
        hint='NEGATIVE = INVERTED' if bipolar else ('SELECT / ENTER' if p.is_discrete else spec.get('caption',''))
        children.append(label('hint',hint,(128,103,150,28),th['muted'],14))
        if bipolar: children.append(label('zero','−          0          +',(2,132,108,16),th['muted'],12,align='horizontallyCentred verticallyCentred'))
    # One touch owner: passive Indicator/labels cannot compete with the parent toggle.
    actions=([ui.action('Mouse Down','Toggle Switch','Data',''),
              ui.action('Enter Pressed','Toggle Switch','Data','')] if is_switch else ui.control_actions(overlay))
    return ui.component_definition(actions,children,'00000000')


def make_documents(params,assigned,extras,th,title):
    styles={k:argb(th[v]) for k,v in [('background','bg'),('accent','accent'),('text','text'),('muted','muted')]}
    defs=[('knob overlay',ui.overlay_component('KnobOverlay',ui.knob_overlay_data(False),styles['background'])),
          ('knob overlay bipolar',ui.overlay_component('KnobOverlay',ui.knob_overlay_data(True),styles['background'])),
          ('menu overlay',ui.overlay_component('MenuOverlay',ui.menu_overlay_data(),styles['background']))]
    children=[image_entry('panel','panel.png',(0,0,1280,628)),image_entry('product title','logo_plate.png',(30,10,500,64)),
              label('header right','MEMORY  /  MOVEMENT  /  SPACE',(740,20,506,28),th['muted'],20,align='right verticallyCentred'),
              label('header hint','VARIABLE SPEED.  IMPERFECT REPEATS.',(710,48,536,24),th['muted'],16,align='right verticallyCentred')]
    maps=[]; mainmap={f'Q-Link {q}':-1 for q in range(1,17)}
    mapping_report=[]
    for i,(spec,p) in enumerate(assigned):
        if not p: continue
        name='control '+spec['key']
        defs.append((name,control_definition(spec,p,i,th)))
        r,c=divmod(i,4)
        children.append(ui.entry(spec['label'],name,ui.obj(('version',1),('handleName','Data')),rect(XS[c],YS[r],SLOT_W,SLOT_H),'Yes','Hide','Always',remap=[('Data',p.handle)]))
        q=ui.slot_qlink(r,c); mainmap[f'Q-Link {q}']=p.index
        mapping_report.append(dict(control=spec['key'],parameter=p.name,index=p.index,page='DELAY',qlink=q))
    tabs=[ui.obj(('version',1),('tabName','DELAY'),('fnKeyIndex',0),('fnKeySubIndex',0),
                 ('componentDefinition',ui.component_definition([],children,styles['background'])),
                 ('qlinkBoundsData',[rect(20,y-14,1240,166) for y in YS]+['0 0 0 0']))]
    maps.append(ui.obj(('Tab',1),('SubTab',1),('Bank Direction','RowTopDown'),('Q-Links',mainmap)))
    # Any unmatched real parameter remains editable, without guessing its semantics.
    for start in range(0,len(extras),16):
        chunk=extras[start:start+16]; page_name='MORE' if start==0 else f'MORE{start//16+1}'
        contents=[image_entry('panel','panel.png',(0,0,1280,628)),image_entry('product title','logo_plate.png',(30,10,500,64))]; qmap={f'Q-Link {q}':-1 for q in range(1,17)}
        for i,p in enumerate(chunk):
            r,c=divmod(i,4)
            defname='extra '+str(p.index)
            extra_spec=dict(key='extra',label=p.name.upper(),caption='WET PAN SEPARATION' if p.name=='Ping Width' else 'POST-MIX LEVEL')
            extra_th=dict(th,engraved=False)
            defs.append((defname,control_definition(extra_spec,p,4,extra_th)))
            contents.append(ui.entry(p.name,defname,ui.obj(('version',1),('handleName','Data')),rect(XS[c],YS[r],SLOT_W,SLOT_H),'Yes','Hide','Always',remap=[('Data',p.handle)]))
            q=ui.slot_qlink(r,c); qmap[f'Q-Link {q}']=p.index
            mapping_report.append(dict(control='fallback',parameter=p.name,index=p.index,page=page_name,qlink=q))
        tabidx=len(tabs)
        tabs.append(ui.obj(('version',1),('tabName',page_name),('fnKeyIndex',tabidx),('fnKeySubIndex',0),
                           ('componentDefinition',ui.component_definition([],contents,styles['background'])),
                           ('qlinkBoundsData',[rect(20,y-14,1240,166) for y in YS]+['0 0 0 0'])))
        maps.append(ui.obj(('Tab',tabidx+1),('SubTab',1),('Bank Direction','RowTopDown'),('Q-Links',qmap)))
    if len(tabs)>32: raise ValueError('Host supports at most 32 pages')
    replacements=[ui.obj(('key',p.handle),('value',p.choices)) for p in params if p.is_discrete]
    tui=ui.obj(('pageData',ui.obj(('version',1),('componentDefinitions',ui.obj(('version',2),('importFiles',[]),
        ('localComponentDefinitions',[ui.obj(('key',k),('value',v)) for k,v in defs]))),('replacementStrings',replacements),
        ('info',ui.obj(('version',1),('type','CompleteDescription'))),('tabs',tabs))))
    qlinks=ui.obj(('version',4),('info',ui.obj(('version',1),('type','CompleteDescription'))),
        ('Screen Mode Q-Links',ui.obj(('version',4),('map',maps))),('Program Mode Q-Links',{f'Q-Link {q}':q-1 if q<=len(params) else -1 for q in range(1,17)}))
    return tui,qlinks,mapping_report


def generate(params_path,out,style='A',title=None,mapping=None,assets_out=None,preview=None,wear=DEFAULT_WEAR,seed=DEFAULT_SEED):
    wear=wear_amount(wear)
    layout=json.loads((ROOT/'design/layout.json').read_text())
    title=title or layout['title']; params=ui.load_params(str(params_path))
    assigned,extras=match_params(params,layout,mapping or {})
    out=Path(out); out.mkdir(parents=True,exist_ok=True)
    temporary_assets=tempfile.TemporaryDirectory(prefix='early-delay-assets-') if assets_out is None else None
    assets=Path(assets_out) if assets_out else Path(temporary_assets.name)
    previous=set()
    if (out/'TUI.json').exists():
        old=json.loads((out/'TUI.json').read_text(),object_pairs_hook=ui.Obj)
        previous=ui.referenced_images(old,set())
    th=render_assets(assets,style,title,wear,seed)
    tui,qlinks,report=make_documents(params,assigned,extras,th,title)
    required=ui.referenced_images(tui,set())
    if assets.resolve()!=out.resolve():
        for filename in sorted(required): shutil.copyfile(assets/filename,out/filename)
    # Remove only formerly referenced generated assets, never unrelated files.
    for filename in sorted(previous-required):
        if Path(filename).name==filename:
            (out/filename).unlink(missing_ok=True)
    (out/'TUI.json').write_text(ui.dump_json(tui)+'\n')
    (out/'Q-Links.json').write_text(ui.dump_json(qlinks)+'\n')
    (out/'skin-map.json').write_text(json.dumps(dict(style=style,title=title,wear=wear if style=='A' else 0,seed=seed,parameters=report,
        missing=[s['key'] for s,p in assigned if p is None],fallback=[p.name for p in extras]),indent=2)+'\n')
    from verify_skin import verify
    verify(out,params_path,quiet=True)
    if preview:
        from render_preview import render_previews
        render_previews(out,params_path,Path(preview),style)
    print(f'Skin {style}: {len(report)}/{len(params)} parameters; {len(extras)} fallback; '+str(out))
    missing=[s['key'] for s,p in assigned if p is None]
    if missing: print('Unbound design slots (omitted): '+', '.join(missing))
    if temporary_assets: temporary_assets.cleanup()
    return out


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--params',type=Path,required=True,help='Real build/params.json from host_sim dump')
    ap.add_argument('--out',type=Path,required=True,help='Destination Plugin Skins/ or project skins/')
    ap.add_argument('--style',choices=THEMES,default='A')
    ap.add_argument('--title',help='Only changes logo_plate.png; plugin identity unchanged')
    ap.add_argument('--mapping',type=Path,help='JSON object canonical key -> exact plugin parameter name')
    ap.add_argument('--assets-out',type=Path,help='Optional source asset directory (copies only used images to skin)')
    ap.add_argument('--preview',type=Path,help='Main page mock-up path; fallback pages get -moreN suffix')
    ap.add_argument('--wear',type=wear_amount,default=DEFAULT_WEAR,help='0 clean .. 1 most worn (style A only)')
    ap.add_argument('--seed',type=int,default=DEFAULT_SEED,help='Stable procedural wear seed')
    a=ap.parse_args()
    try:
        generate(a.params,a.out,a.style,a.title,json.loads(a.mapping.read_text()) if a.mapping else None,a.assets_out,a.preview,a.wear,a.seed)
    except (ValueError,OSError,ui.UiError) as e: ap.exit(1,str(e)+'\n')

if __name__=='__main__': main()
