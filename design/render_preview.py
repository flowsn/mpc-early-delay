#!/usr/bin/env python3
"""Faithful static composition of the GENERATED TUI tree (not a separate layout).
Host fonts and value strings approximated; host header/footer intentionally neutral.
No screenshot, hardware proof or live interaction is implied.
"""
import json
from pathlib import Path
from PIL import Image, ImageDraw
from render_assets import THEMES, font, text, save


def render_previews(skin,params_path,output,style):
    skin=Path(skin); output=Path(output); output.parent.mkdir(parents=True,exist_ok=True)
    tui=json.loads((skin/'TUI.json').read_text())['pageData']
    params={p.get('index',i):p for i,p in enumerate(json.loads(Path(params_path).read_text())['params'])}
    definitions={x['key']:x['value'] for x in tui['componentDefinitions']['localComponentDefinitions']}
    th=THEMES[style]

    def p_for(handle,binding):
        h=binding if handle=='Data' else handle
        return params.get(int(h.split()[-1])) if h.startswith('Parameter ') else None

    def colour(argb):
        if len(argb)!=8: return (0,0,0,0)
        return tuple(int(argb[i:i+2],16) for i in (2,4,6,0))

    def value(p):
        if not p:return ''
        v=str(p.get('display',''))
        unit=p.get('label','')
        return v+(' '+unit if unit and not v.endswith(unit) else '')

    def render_component(im,comp,offset,binding='Invalid'):
        for child in comp['componentsData']:
            b=child['bounds']
            if b['whenVisible']=='WhenFocussed': continue
            x,y,w,h=map(lambda n:int(float(n)),b['bounds'].split()); x+=offset[0];y+=offset[1]
            remap={m['key']:m['value'] for m in child['handle remapping']['map']}
            bind=remap.get('Data',binding)
            data=child['componentData']; kind=data['type']; d=data['data']
            if kind in definitions:
                render_component(im,definitions[kind],(x,y),bind); continue
            p=p_for(d.get('handleName','Invalid'),bind)
            if kind=='Image':
                asset=Image.open(skin/d['image']).convert('RGBA')
                if asset.size!=(w,h): asset=asset.resize((w,h),Image.Resampling.LANCZOS)
                im.alpha_composite(asset,(x,y))
            elif kind=='Knob':
                strip=Image.open(skin/d['filmStrip']).convert('RGBA'); n=d['numFrames']; fh=strip.height//n
                index=round((p.get('default',.5) if p else .5)*(n-1))
                im.alpha_composite(strip.crop((0,index*fh,strip.width,(index+1)*fh)),(x+(w-strip.width)//2,y+(h-fh)//2))
            elif kind=='Button':
                on=bool(p and p.get('default',0)>=.5)
                asset=Image.open(skin/d['onImage' if on else 'offImage']).convert('RGBA')
                im.alpha_composite(asset,(x+(w-asset.width)//2,y+(h-asset.height)//2))
            elif kind=='Label':
                s=d['textStyle']; content=d.get('text','') if d['type']=='Text' else (p.get('name','') if d['type']=='Name' and p else value(p))
                ft=font(round(s['font']['height']),s['font']['style']=='SemiBold')
                # Clip to the same label bounds as the host. Never silently shrink values.
                tile=Image.new('RGBA',(w,h)); td=ImageDraw.Draw(tile)
                bb=td.textbbox((0,0),content,font=ft); tw=td.textlength(content,font=ft)
                tx=(w-tw)/2 if 'horizontallyCentred' in s['justification'] else (w-tw if 'right' in s['justification'] else 0)
                ty=(h-(bb[3]-bb[1]))/2-bb[1]
                td.text((tx,ty),content,font=ft,fill=colour(s['colour']))
                im.alpha_composite(tile,(x,y))
            elif kind=='Slider':
                dd=ImageDraw.Draw(im)
                dd.rounded_rectangle((x,y,x+w,y+h),4,fill=th['display'],outline=th['edge'])
                text(dd,(x+10,y+7),p['name'] if p else '',th['muted'],21)
                text(dd,(x+10,y+38),value(p),th['value'],26)

    outputs=[]
    for i,tab in enumerate(tui['tabs']):
        im=Image.new('RGBA',(1280,800),th['bg']); d=ImageDraw.Draw(im)
        d.rectangle((0,0,1279,109),fill='#111519')
        text(d,(30,16),'UI MOCK-UP  /  OPTION '+style+'  /  '+th['name'],'#f0f0eb',24,True)
        text(d,(30,58),'1280 × 800  •  Illustrative defaults  •  Not a hardware screenshot','#aeb6bb',18)
        text(d,(1248,86),'HOST HEADER RESERVED','#78858b',14,anchor='rt')
        render_component(im,tab['componentDefinition'],(0,110))
        d=ImageDraw.Draw(im); d.rectangle((0,738,1279,799),fill='#111519')
        for j,t in enumerate(tui['tabs']):
            bx=26+j*152
            d.rounded_rectangle((bx,750,bx+132,789),4,fill=th['panel'] if i==j else '#111519',outline=th['accent'] if i==j else '#39434a')
            text(d,(bx+66,758),t['tabName'],th['text'] if i==j else '#aeb6bb',20,True,anchor='mt')
        text(d,(1248,767),'HOST FUNCTION-KEY AREA','#78858b',14,anchor='rt')
        path=output if i==0 else output.with_stem(output.stem+'-more'+str(i))
        save(im.convert('RGB'),path); outputs.append(path)
    return outputs


def render_material_study(assets, output):
    """Review-only contact sheet; never copied into a runtime skin."""
    assets=Path(assets)
    im=Image.new('RGB',(1280,800),'#1d242b');d=ImageDraw.Draw(im)
    text(d,(32,24),'DISTRESSED MATERIAL STUDY / ORIGINAL ARTWORK','#e5dfcb',30,True)
    text(d,(32,70),'Review details • wear 0.55 • seed 1471 • not a hardware screenshot','#c6bda5',20)
    panel=Image.open(assets/'panel.png').convert('RGB')
    im.paste(panel.crop((0,0,580,92)).resize((1160,184)),(40,117))
    text(d,(40,307),'2× perimeter detail — restrained silver chips and matte grain','#c6bda5',20)
    strip=Image.open(assets/'knob_112_bi.png').convert('RGBA')
    frame=strip.crop((0,63*112,112,64*112)).resize((224,224),Image.Resampling.LANCZOS)
    im.paste(frame,(48,374),frame)
    text(d,(40,615),'2× knob / centre zero','#e5dfcb',22)
    for i,state in enumerate(('off','on')):
        img=Image.open(assets/f'hold_{state}.png').convert('RGBA')
        im.paste(img,(370,390+i*112),img)
    text(d,(370,615),'Slide-switch states / 1×','#e5dfcb',22)
    meter=Image.open(assets/'headroom_unbound.png').resize((360,60),Image.Resampling.LANCZOS)
    im.paste(meter,(800,400))
    text(d,(800,478),'Five dull-glass lenses / 2×','#e5dfcb',22)
    text(d,(800,512),'Unbound study only; not installed.','#c6bda5',18)
    text(d,(800,544),'No live headroom source is available.','#c6bda5',18)
    text(d,(40,710),'Touch targets and live values remain protected; physical-screen checks are still pending.','#c6bda5',21)
    save(im,Path(output))
