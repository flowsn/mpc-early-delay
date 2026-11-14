#!/usr/bin/env python3
"""Prove wear reproducibility, clean/max variants, protected values and byte budget."""
import hashlib
import json
from pathlib import Path
import shutil
import sys
import tempfile
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
sys.path.insert(0,str(ROOT/'design'))
from make_skin import generate
from render_assets import DEFAULT_SEED, DEFAULT_WEAR, check_art_text, knob_frame, THEMES, wear_amount
from render_preview import render_material_study
from verify_skin import verify


def hashes(path):
    return {str(p.relative_to(path)):hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(path.rglob('*')) if p.is_file()}


def main():
    reports=ROOT/'design/validation'
    params=ROOT/'design/testbed/params.json'
    installed=ROOT/'design/skins/A/Plugin Skins'
    result={'seed':DEFAULT_SEED,'defaultWear':DEFAULT_WEAR,'budgetBytes':400*1024,'variants':{}}
    with tempfile.TemporaryDirectory(prefix='early-delay-distressed-') as directory:
        work=Path(directory)
        for key,wear in (('distressed',DEFAULT_WEAR),('repeat',DEFAULT_WEAR),('clean',0),('maximum',1)):
            dest=work/key
            generate(params,dest/'skin','A',assets_out=dest/'assets',preview=dest/'preview.png',wear=wear,seed=DEFAULT_SEED)
            verify(dest/'skin',params,quiet=True)
            size=sum(p.stat().st_size for p in (dest/'skin').iterdir() if p.is_file())
            assert size<=400*1024,(key,size)
            result['variants'][key]={'wear':wear,'skinBytes':size,'files':len(hashes(dest/'skin'))}
        first=hashes(work/'distressed')
        assert first==hashes(work/'repeat'),'Asset/skin/preview bytes differ for same seed'
        assert hashes(installed)==hashes(work/'distressed/skin'),'Checked-in default skin is stale'
        assert hashes(ROOT/'design/assets/A')==hashes(work/'distressed/assets'),'Checked-in source assets are stale'
        for page in ('preview.png','preview-more1.png'):
            target='option-A-charcoal-amber'+('-more1' if 'more1' in page else '')+'.png'
            assert (ROOT/'design/previews'/target).read_bytes()==(work/'distressed'/page).read_bytes()
        clean=work/'clean'; worn=work/'maximum'
        # Dynamic numeric readouts and their image backgrounds cannot be distressed.
        assert (clean/'skin/value_plate.png').read_bytes()==(worn/'skin/value_plate.png').read_bytes()
        assert (clean/'skin/TUI.json').read_bytes()==(worn/'skin/TUI.json').read_bytes()
        assert (clean/'skin/Q-Links.json').read_bytes()==(worn/'skin/Q-Links.json').read_bytes()
        assert (clean/'skin/panel.png').read_bytes()!=(worn/'skin/panel.png').read_bytes()
        for wear in (0,DEFAULT_WEAR,1):
            a=knob_frame(88,.5,THEMES['A'],False,wear,DEFAULT_SEED)
            b=knob_frame(88,.5,THEMES['A'],False,wear,DEFAULT_SEED+1)
            assert (a.tobytes()==b.tobytes())==(wear==0),'Seed contract not respected'
        # Material finish in the cap centre must not jump between film frames.
        a=knob_frame(112,0,THEMES['A'],False,1,DEFAULT_SEED)
        b=knob_frame(112,1,THEMES['A'],False,1,DEFAULT_SEED)
        assert a.crop((52,52,60,60)).tobytes()==b.crop((52,52,60,60)).tobytes()
        for value in (-.1,1.1,float('nan'),float('inf')):
            try:wear_amount(value)
            except ValueError:pass
            else:raise AssertionError('Invalid wear accepted')
        for forbidden in ('Ibanez','DM1000','VST','MPC','Akai'):
            try:check_art_text(forbidden)
            except ValueError:pass
            else:raise AssertionError('Forbidden mark accepted')
        layout=json.loads((ROOT/'design/layout.json').read_text())
        expected={'TIME','FEEDBACK','MOD SPEED','MOD DEPTH','TONE','GRIT','BANDWIDTH','DRIVE','MIX','HOLD','STEREOIZE'}
        assert {s['label'] for s in layout['controls'] if not s.get('fallbackOnly')}==expected
        def check_text(node):
            if isinstance(node,dict):
                if 'text' in node and isinstance(node['text'],str):check_art_text(node['text'])
                for v in node.values():check_text(v)
            elif isinstance(node,list):
                for v in node:check_text(v)
        check_text(json.loads((installed/'TUI.json').read_text()))
        for spec in layout['controls']:check_art_text(spec['label'])
        assert not (installed/'headroom_unbound.png').exists(),'Unbound meter must not imply live data'
        for page,suffix in (('preview.png',''),('preview-more1.png','-more1')):
            shutil.copyfile(clean/page,ROOT/f'design/previews/option-A-clean{suffix}.png')
        result.update(byteIdenticalRegeneration=True,verifiedFiles=len(first),protectedReadouts=True,
                      stableFilmFinish=True,marksChecked=True,headroomStudyNotInstalled=True)
        (reports/'reproducibility.txt').write_text('PASS: complete A source assets, skin and both previews regenerate byte-for-byte.\n'+
            '\n'.join(f'{v}  {k}' for k,v in first.items())+'\n')
    render_material_study(ROOT/'design/assets/A',ROOT/'design/previews/option-A-material-study.png')
    (reports/'distressed.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
    print('PASS: deterministic distressed/clean/max wear; protected readouts; artwork text; 400 KiB budget.')

if __name__=='__main__':main()
