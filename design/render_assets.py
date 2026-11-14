#!/usr/bin/env python3
"""Original hardware-inspired artwork. Deterministic PIL drawing; no firmware assets.
The mock-up uses the same generated PNGs and component geometry as the skin.
"""
from pathlib import Path
import math
from PIL import Image, ImageDraw, ImageFont

HERE = Path(__file__).resolve().parent
FRAMES = 127
THEMES = {
    'A': dict(name='CHARCOAL / AMBER', bg='#252724', panel='#2e302c', edge='#505247',
              text='#eee7d3', muted='#c5b994', accent='#efb34e', display='#171b17',
              value='#ffd48a', knob='#41433b', dark='#101310', light='#64665a',
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

def knob_frame(size, t, theme, bipolar=False):
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
    a=135+270*t
    d.line((polar(r*.43,a),polar(r*.85,a)),fill=theme.get('pointer',theme['text']),width=3*scale)
    return im.resize((size,size),Image.Resampling.LANCZOS)

def lamp(theme, on, kind='lamp', size=24):
    im=Image.new('RGBA',(size,size)); d=ImageDraw.Draw(im)
    c=theme[kind]
    d.ellipse((2,2,size-2,size-2),fill=theme['dark'],outline=theme['edge'],width=1)
    d.ellipse((5,5,size-5,size-5),fill=c if on else blend(theme['dark'],c,.22))
    if on: d.ellipse((7,6,size-9,size-11),fill=blend(c,'#ffffff',.7))
    return im

def render_assets(out, style, title):
    out=Path(out); out.mkdir(parents=True,exist_ok=True)
    th=THEMES[style]
    panel=Image.new('RGB',(1280,628),th['bg']); d=ImageDraw.Draw(panel)
    # Three clean horizontal groups: performance, colour, movement / space.
    for y in YS:
        d.rounded_rectangle((20,y-14,1260,y+152),radius=7,fill=th['panel'],outline=th['edge'])
        for x in (328,640,952): d.line((x,y+6,x,y+134),fill=th['edge'])
    d.line((28,85,1252,85),fill=th['accent'],width=2)
    for x,y in ((14,14),(1266,14),(14,614),(1266,614)):
        d.ellipse((x-4,y-4,x+4,y+4),fill=th['dark'],outline=th['edge'])
        d.line((x-2,y-1,x+2,y+1),fill=th['light'])
    save(panel,out/'panel.png')
    plate=Image.new('RGBA',(500,64)); d=ImageDraw.Draw(plate)
    # The entire product name is confined to this single replaceable image.
    title_size=40
    while d.textlength(title,font=font(title_size,True))>470 and title_size>18: title_size-=1
    text(d,(0,2),title,th['text'],title_size,True)
    text(d,(2,46),'VARIABLE CLOCK  /  DIGITAL DELAY',th['muted'],15,True)
    save(plate,out/'logo_plate.png')
    display=Image.new('RGB',(150,48),th['display']); d=ImageDraw.Draw(display)
    d.rectangle((0,0,149,47),outline=th['edge'])
    d.line((1,1,148,1),fill=th['dark'],width=2)
    save(display,out/'value_plate.png')
    for size in (112,88):
        for bi in (False,True):
            strip=Image.new('RGBA',(size,size*FRAMES))
            for frame in range(FRAMES): strip.paste(knob_frame(size,frame/(FRAMES-1),th,bi),(0,frame*size))
            save(strip.quantize(colors=192,method=Image.Quantize.FASTOCTREE),out/f'knob_{size}_{"bi" if bi else "uni"}.png')
    for kind in ('lamp','hold'):
        for on in (False,True):
            save(lamp(th,on,kind),out/f'led_{kind}_{"on" if on else "off"}.png')
    for key,kind in (('hold','hold'),('stereoize','lamp')):
        for on in (False,True):
            im=Image.new('RGBA',(278,76)); d=ImageDraw.Draw(im)
            d.rounded_rectangle((0,3,277,75),radius=5,fill=th['dark'])
            d.rounded_rectangle((1,1,276,70),radius=5,fill=th['knob'],outline=th[kind] if on else th['light'],width=2)
            if on: d.line((8,65,269,65),fill=th[kind],width=3)
            im.alpha_composite(lamp(th,on,kind),(12,23))
            save(im,out/f'{key}_{"on" if on else "off"}.png')
    return th

if __name__=='__main__':
    import argparse
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out',type=Path,required=True)
    ap.add_argument('--style',choices=THEMES,default='A')
    ap.add_argument('--title',default='EARLY DELAY')
    args=ap.parse_args(); render_assets(args.out,args.style,args.title)
