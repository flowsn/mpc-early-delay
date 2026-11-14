#!/usr/bin/env python3
"""Original hardware-inspired artwork. Deterministic PIL drawing; no firmware assets.
The mock-up uses the same generated PNGs and component geometry as the skin.
"""
from pathlib import Path
import math
import hashlib
import json
import random
import re
from PIL import Image, ImageDraw, ImageFont, ImageChops

HERE = Path(__file__).resolve().parent
FRAMES = 127
THEMES = {
    'A': dict(name='CHARCOAL / AMBER', bg='#222930', panel='#2b3239', edge='#566069',
              text='#e5dfcb', muted='#c6bda5', accent='#efb34e', display='#151b20',
              value='#ffd48a', knob='#363c41', dark='#10171c', light='#6b747a', engraved=True,
              lamp='#ffc456', hold='#ff7658'),
    'B': dict(name='PARCHMENT / OXIDE', bg='#d4c9af', panel='#e0d7c1', edge='#aa9c7f',
              text='#3b3025', muted='#67533b', accent='#87462b', display='#32291f',
              value='#f5d7a0', knob='#62503b', dark='#241e17', light='#9b8668',
              lamp='#ff5f48', hold='#ff5f48', pointer='#f7ebd5', buttonText='#f7ebd5'),
    'C': dict(name='BLACK / FLUORESCENT', bg='#10171c', panel='#182129', edge='#394650',
              text='#e1edf3', muted='#99b5c2', accent='#88d6ef', display='#07151d',
              value='#b4edff', knob='#303e46', dark='#050b10', light='#627580',
              lamp='#a4ecff', hold='#ff745b'),
}

# Tab geometry (physical screen y = tab y + 110).
XS = (30, 342, 654, 966)
YS = (112, 288, 464)
SLOT_W, SLOT_H = 284, 148

def rgb(col):
    return tuple(int(col[i:i+2],16) for i in (1,3,5))

def blend(a,b,t):
    return tuple(round(x+(y-x)*t) for x,y in zip(rgb(a) if isinstance(a,str) else a,rgb(b) if isinstance(b,str) else b))

def font(size, bold=False):
    return ImageFont.truetype(str(HERE/'fonts'/('TitilliumWeb-SemiBold.ttf' if bold else 'TitilliumWeb-Regular.ttf')),size)

def text(draw, xy, value, fill, size=22, bold=False, anchor='lt'):
    draw.text(xy, value, font=font(size,bold), fill=fill, anchor=anchor)

def save(im,path):
    im.save(path, optimize=True, compress_level=9)

DEFAULT_WEAR = .55
DEFAULT_SEED = 1471


def wear_amount(value):
    value = float(value)
    if not math.isfinite(value) or not 0 <= value <= 1:
        raise ValueError('wear must be a finite number between 0 and 1')
    return value


def seeded(seed, part):
    # Stable per-asset streams; never Python's randomized hash or global RNG.
    digest = hashlib.sha256(f'{seed}:{part}'.encode()).digest()
    return random.Random(int.from_bytes(digest[:16], 'big'))


def check_art_text(value):
    if re.search(r'ibanez|dm[ -]?1000|vst|mpc|akai', value, re.I):
        raise ValueError('Commercial marks are not permitted in artwork text')


def engraved_label(value, size, colour, wear, seed, bold=True, font_size=22):
    check_art_text(value)
    mask = Image.new('L', size)
    d = ImageDraw.Draw(mask)
    ft = font(font_size, bold)
    bb = d.textbbox((0, 0), value, font=ft)
    y = (size[1] - (bb[3] - bb[1])) / 2 - bb[1]
    d.text((0, y), value, font=ft, fill=255)
    # Slight irregular ink fading; no erased strokes or full-width scratches.
    rng = seeded(seed, 'label:' + value)
    fade = Image.new('L', size, 255)
    fd = ImageDraw.Draw(fade)
    for _ in range(int(size[0] * size[1] * .018)):
        x, y = rng.randrange(size[0]), rng.randrange(size[1])
        fd.line((x, y, x + rng.randrange(1, 4), y), fill=255-round(wear*rng.randrange(15, 65)))
    mask = ImageChops.multiply(mask, fade)
    im = Image.new('RGBA', size, colour)
    im.putalpha(mask)
    # Recessed dark shadow sits just below the cream legend, never outside bounds.
    shadow = Image.new('RGBA', size, '#0c1217')
    shadow.putalpha(ImageChops.offset(mask, 0, 1))
    shadow.alpha_composite(im)
    return shadow


def panel_wear(panel, th, wear, seed):
    if wear == 0:
        return
    rng = seeded(seed, 'panel')
    d = ImageDraw.Draw(panel)
    # Low-density 1px matte grain (four quantized tones keeps file size small).
    for _ in range(int(12000*wear)):
        x, y = rng.randrange(1280), rng.randrange(628)
        col = panel.getpixel((x, y))
        delta = rng.choice((-3, -2, 2, 3))
        d.point((x, y), fill=tuple(max(0,min(255,v+delta)) for v in col))
    # Uneven chips at outer edges and corners only, away from touch/value areas.
    for _ in range(int(145*wear)):
        edge = rng.randrange(4)
        along = rng.randrange(1280 if edge < 2 else 628)
        depth = rng.randrange(1, 5)
        length = rng.randrange(2, 14)
        col = blend(th['bg'], '#c2c5bf', rng.uniform(.35, .85)*wear)
        if edge < 2:
            y = depth if edge == 0 else 627-depth
            d.line((along,y,min(1279,along+length),y),fill=col,width=rng.choice((1,1,2)))
        else:
            x = depth if edge == 2 else 1279-depth
            d.line((x,along,x,min(627,along+length)),fill=col,width=1)
    # Directional rack-ear scratches: narrow noninteractive margin only.
    for _ in range(int(80*wear)):
        x = rng.choice((rng.randrange(2,17),rng.randrange(1263,1278)))
        y = rng.randrange(5,620)
        d.line((x,y,max(1,min(1278,x+rng.randrange(-5,6))),min(626,y+rng.randrange(3,14))),
               fill=blend(th['bg'],'#acb5b8',.28*wear))
    for cx,cy in ((14,14),(1266,14),(14,614),(1266,614)):
        for _ in range(5):
            r=rng.randrange(6,11); start=rng.randrange(360)
            d.arc((cx-r,cy-r,cx+r,cy+r),start,start+rng.randrange(12,65),
                  fill=blend(th['bg'],'#bbc0bb',.55*wear))
    # Dust/grime lives in the lower seam, below all essential controls.
    for _ in range(int(1800*wear)):
        x,y=rng.randrange(21,1259),rng.randrange(615,627)
        d.line((x,y,x+rng.randrange(1,4),y),fill=blend(th['bg'],rng.choice(('#14191c','#777367')),.5*wear))


def knob_frame(size, t, theme, bipolar=False, wear=0, seed=DEFAULT_SEED):
    # Supersampling keeps the narrow pointer and scalloped grip smooth.
    scale=3
    im=Image.new('RGBA',(size*scale,size*scale))
    d=ImageDraw.Draw(im)
    n=size*scale; c=n/2; r=n*.345
    def polar(radius,ang):
        a=math.radians(ang)
        return c+radius*math.cos(a),c+radius*math.sin(a)
    arc=(n*.075,n*.075,n*.925,n*.925)
    d.arc(arc,135,405,fill=theme['edge'],width=2*scale)
    start,end=(270,135+270*t) if bipolar else (135,135+270*t)
    if abs(start-end)>.2:
        d.arc(arc,min(start,end),max(start,end),fill=theme['accent'],width=3*scale)
    for i in range(11):
        a=135+27*i
        if theme.get('engraved'):
            px,py=polar(n*.475,a)
            d.ellipse((px-scale,py-scale,px+scale,py+scale),fill=theme['text'])
        else:
            d.line((polar(n*.465,a),polar(n*.49,a)),fill=theme['muted'],width=scale)
    d.ellipse((c-r-3*scale,c-r+2*scale,c+r+3*scale,c+r+7*scale),fill=theme['dark'])
    points=[polar(r*(1 if i%2 else .96),i*360/96) for i in range(96)]
    d.polygon(points,fill=theme['dark'])
    d.ellipse((c-r+2*scale,c-r+scale,c+r-2*scale,c+r-3*scale),fill=theme['light'])
    d.ellipse((c-r+4*scale,c-r+4*scale,c+r-4*scale,c+r-4*scale),fill=theme['knob'])
    for i in range(24):
        a=i*15
        d.line((polar(r*.89,a),polar(r*.97,a)),fill=theme['dark'],width=scale)
    d.arc((c-r+7*scale,c-r+6*scale,c+r-7*scale,c+r-8*scale),205,305,fill=blend(theme['knob'],theme['light'],.55),width=scale)
    if theme.get('engraved'):
        # Fine dark grip and brushed metal cap, original proportions.
        for i in range(64):
            angle=i*360/64
            d.line((polar(r*.83,angle),polar(r*.97,angle)),
                   fill=blend(theme['dark'],theme['light'],.35 if i%3 else .6),width=scale)
        cap=r*.62
        for yy in range(int(c-cap),int(c+cap)+1):
            for xx in range(int(c-cap),int(c+cap)+1):
                dx,dy=xx-c,yy-c
                if dx*dx+dy*dy>cap*cap: continue
                theta=math.atan2(dy,dx)
                sheen=.5+.25*math.cos(theta*2-.6)+.10*math.sin(theta*6)
                brush=((yy*17)%7-3)*.012
                d.point((xx,yy),fill=blend('#737e85','#d2d5cc',max(0,min(1,sheen+brush))))
        d.ellipse((c-cap,c-cap,c+cap,c+cap),outline='#87918f',width=scale)
        # Stable in all film frames: the finish never sparkles or crawls.
        rng=seeded(seed,f'knob:{size}')
        for _ in range(int(27*wear)):
            angle=rng.randrange(360)
            d.line((polar(r*.88,angle),polar(r*.96,angle+2)),
                   fill=blend(theme['knob'],'#afb5ac',.48*wear),width=scale)
        for _ in range(int(10*wear)):
            yy=c+rng.uniform(-.8,.8)*cap
            xx=c+rng.uniform(-.5,.5)*cap
            d.line((xx,yy,xx+cap*.15,yy-scale),fill=blend('#8b9699','#d5d7cc',.35*wear),width=1)
    a=135+270*t
    # Dark recessed notch behind the protected, high-contrast silver pointer.
    if theme.get('engraved'):
        d.line((polar(r*.30,a),polar(r*.87,a)),fill=theme['dark'],width=6*scale)
    d.line((polar(r*.43,a),polar(r*.85,a)),fill=theme.get('pointer',theme['text']),width=3*scale)
    return im.resize((size,size),Image.Resampling.LANCZOS)

def lamp(theme, on, kind='lamp', size=24, wear=0, seed=DEFAULT_SEED):
    im=Image.new('RGBA',(size,size)); d=ImageDraw.Draw(im)
    c=theme[kind]
    d.ellipse((2,2,size-2,size-2),fill=theme['dark'],outline=theme['edge'],width=1)
    d.ellipse((5,5,size-5,size-5),fill=c if on else blend(theme['dark'],c,.22))
    if on: d.ellipse((7,6,size-9,size-11),fill=blend(c,'#ffffff',.7))
    if theme.get('engraved'):
        d.ellipse((5,5,size-5,size-5),fill=blend(theme['dark'],c,.78 if on else .20))
        d.line((7,7,size-8,7),fill=blend(c,'#a8b2aa',.5))
        rng=seeded(seed,'lamp:'+kind)
        for _ in range(int(9*wear)):
            x,y=rng.randrange(7,size-7),rng.randrange(7,size-7)
            d.point((x,y),fill=blend(theme['dark'],c,.55 if on else .16))
    return im

def render_assets(out, style, title, wear=DEFAULT_WEAR, seed=DEFAULT_SEED):
    out=Path(out); out.mkdir(parents=True,exist_ok=True)
    wear=wear_amount(wear)
    check_art_text(title)
    th=THEMES[style]
    wear=wear if style=='A' else 0
    panel=Image.new('RGB',(1280,628),th['bg']); d=ImageDraw.Draw(panel)
    # Three clean horizontal groups: performance, colour, movement / space.
    for y in YS:
        d.rounded_rectangle((20,y-14,1260,y+152),radius=7,fill=th['panel'],outline=th['edge'])
        for x in (328,640,952): d.line((x,y+6,x,y+134),fill=th['edge'])
    d.line((28,85,1252,85),fill=th['accent'],width=2)
    for x,y in ((14,14),(1266,14),(14,614),(1266,614)):
        d.ellipse((x-4,y-4,x+4,y+4),fill=th['dark'],outline=th['edge'])
        d.line((x-2,y-1,x+2,y+1),fill=th['light'])
    panel_wear(panel,th,wear,seed)
    save(panel,out/'panel.png')
    plate=Image.new('RGBA',(500,64)); d=ImageDraw.Draw(plate)
    # The entire product name is confined to this single replaceable image.
    title_size=40
    while d.textlength(title,font=font(title_size,True))>470 and title_size>18: title_size-=1
    text(d,(0,2),title,th['text'],title_size,True)
    text(d,(2,46),'VARIABLE CLOCK  /  DIGITAL DELAY',th['muted'],15,True)
    if style=='A':
        # Sparse partial alpha fade on the title; essential names use protected masks.
        rng=seeded(seed,'title')
        for _ in range(int(650*wear)):
            x,y=rng.randrange(500),rng.randrange(64)
            rr,gg,bb,aa=plate.getpixel((x,y))
            plate.putpixel((x,y),(rr,gg,bb,round(aa*(1-.18*wear))))
        layout=json.loads((HERE/'layout.json').read_text())
        for spec in layout['controls']:
            if spec.get('fallbackOnly'): continue
            size=(176,45) if spec.get('switch') else (276,30)
            im=engraved_label(spec['label'],size,th['text'],wear,seed,font_size=25 if spec.get('switch') else 22)
            save(im,out/f'name_{spec["key"]}.png')
            if spec.get('switch'):
                save(engraved_label(spec['label'],(276,30),th['text'],wear,seed),out/f'name_{spec["key"]}_knob.png')
        # Unbound headroom material study: kept OUT of the runtime skin.
        meter=Image.new('RGB',(180,30),th['bg']); md=ImageDraw.Draw(meter)
        rng=seeded(seed,'headroom')
        for i,col in enumerate(('#345e48',)*3+('#683339',)*2):
            x=5+i*35
            md.rectangle((x,6,x+27,23),fill=th['dark'],outline=th['edge'])
            md.rectangle((x+3,9,x+24,20),fill=col)
            md.line((x+4,10,x+23,10),fill=blend(col,'#9baca0',.28))
            for _ in range(int(8*wear)):
                px,py=x+rng.randrange(4,24),rng.randrange(10,20)
                md.point((px,py),fill=blend(col,th['dark'],.25))
        save(meter,out/'headroom_unbound.png')
    save(plate,out/'logo_plate.png')
    display=Image.new('RGB',(150,48),th['display']); d=ImageDraw.Draw(display)
    d.rectangle((0,0,149,47),outline=th['edge'])
    d.line((1,1,148,1),fill=th['dark'],width=2)
    save(display,out/'value_plate.png')
    for size in (112,88):
        for bi in (False,True):
            strip=Image.new('RGBA',(size,size*FRAMES))
            for frame in range(FRAMES): strip.paste(knob_frame(size,frame/(FRAMES-1),th,bi,wear,seed),(0,frame*size))
            save(strip.quantize(colors=192,method=Image.Quantize.FASTOCTREE),out/f'knob_{size}_{"bi" if bi else "uni"}.png')
    for kind in ('lamp','hold'):
        for on in (False,True):
            save(lamp(th,on,kind,wear=wear,seed=seed),out/f'led_{kind}_{"on" if on else "off"}.png')
    for key,kind in (('hold','hold'),('stereoize','lamp')):
        for on in (False,True):
            im=Image.new('RGBA',(278,76)); d=ImageDraw.Draw(im)
            d.rounded_rectangle((0,3,277,75),radius=5,fill=th['dark'])
            d.rounded_rectangle((1,1,276,70),radius=5,fill=th['knob'],outline=th[kind] if on else th['light'],width=2)
            if on: d.line((8,65,269,65),fill=th[kind],width=3)
            if style=='A':
                # Slide-switch visual, still one large latching Button hit target.
                d.rectangle((11,17,43,57),fill=th['dark'],outline=th['edge'])
                sy=35 if on else 20
                d.rectangle((15,sy,39,sy+18),fill=th['knob'],outline=th['light'])
                for yy in range(sy+3,sy+16,3): d.line((18,yy,36,yy),fill=th['light'])
                rng=seeded(seed,'switch:'+key)
                for _ in range(int(24*wear)):
                    x=rng.randrange(6,271); y=rng.choice((4,5,68,69))
                    d.line((x,y,x+rng.randrange(1,5),y),fill=blend(th['knob'],'#a5aeaa',.45*wear))
                # Same wear pattern in on/off images; only thumb and lamp change.
                im.alpha_composite(lamp(th,on,kind,wear=wear,seed=seed),(240,5))
            else:
                im.alpha_composite(lamp(th,on,kind),(12,23))
            save(im,out/f'{key}_{"on" if on else "off"}.png')
    return th

if __name__=='__main__':
    import argparse
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out',type=Path,required=True)
    ap.add_argument('--style',choices=THEMES,default='A')
    ap.add_argument('--title',default='EARLY DELAY')
    ap.add_argument('--wear',type=wear_amount,default=DEFAULT_WEAR,help='0 clean .. 1 most worn (style A only)')
    ap.add_argument('--seed',type=int,default=DEFAULT_SEED)
    args=ap.parse_args(); render_assets(args.out,args.style,args.title,args.wear,args.seed)
