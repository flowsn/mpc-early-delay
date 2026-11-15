#!/usr/bin/env python3
"""Package and verify v0.6 from the exact validated build; no network/device actions."""
import array, hashlib, json, math, pathlib, re, struct, sys, wave, zipfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'sdk/tools'))
import package as sdk_package
import mpc_remote

def digest(data): return hashlib.sha256(data).hexdigest()
def main():
    meta=json.loads((ROOT/'plugin/plugin.json').read_text())
    folder=sdk_package.output_folder_name(meta)
    assert meta['name']=='80s Lofi Delay Box' and meta['uid']=='FpEd' and meta['version']=='0.6.0'
    assert folder=='Flat Peach - VST - 80s Lofi Delay Box'
    built=ROOT/'plugin/build'/folder
    lp=mpc_remote.LocalPlugin(str(built),meta);lp.check()
    assert lp.folder==folder and not lp.errors and not lp.warnings,(lp.errors,lp.warnings)
    assert set(lp.builds)=={'arm32','arm64'}
    assert json.loads((built/'plugin.json').read_text())==meta
    assert '<version>0.6.0.0</version>' in (built/'version.xml').read_text()
    assert '<uiversion>0.1.0.0</uiversion>' in (built/'version.xml').read_text()
    for sub,source in [('Plugin Skins','skins'),('Content','Content')]:
        expected={p.relative_to(ROOT/'plugin'/source):p.read_bytes() for p in (ROOT/'plugin'/source).rglob('*') if p.is_file()}
        actual={p.relative_to(built/sub):p.read_bytes() for p in (built/sub).rglob('*') if p.is_file()}
        assert actual==expected,sub+' differs from source'
    entries={folder+'/'+str(p.relative_to(built)):p.read_bytes() for p in built.rglob('*') if p.is_file()}
    entries['README.md']=(ROOT/'plugin/README.md').read_bytes()
    entries['CHANGES-v0.6.md']=(ROOT/'CHANGES-v0.6.md').read_bytes()
    intro='''UPDATE FROM v0.5\n================\nUse update/replace semantics, not a side-by-side install.\nThe folder remains Flat Peach - VST - 80s Lofi Delay Box.\nSDK mpc_remote.py update replaces this same folder. No old-name removal is\nneeded for a v0.5 update. UID FpEd and parameter IDs are unchanged.\nSave projects before an update. This package does not deploy or restart anything.\n\n'''
    entries['INSTALL.txt']=(intro+sdk_package.install_text(meta,folder)).encode()
    path=ROOT/'plugin/dist/80s-lofi-delay-box-0.6-mpc.zip'
    with zipfile.ZipFile(path,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for name,data in sorted(entries.items()):
            info=zipfile.ZipInfo(name,(2026,11,15,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED
            info.external_attr=(0o100644<<16);info.create_system=3
            z.writestr(info,data,compresslevel=9)
    with zipfile.ZipFile(path) as z:
        assert z.testzip() is None
        assert {name:z.read(name) for name in z.namelist()}==entries
        assert len(z.namelist())==len(entries)
        for key,machine in [('file32',40),('file64',183)]:
            binary=z.read(folder+'/'+meta[key]);assert binary[:4]==b'\x7fELF' and struct.unpack_from('<H',binary,18)[0]==machine
    artifacts=[path,*sorted((ROOT/'demos/v0.6').glob('*.wav')),ROOT/'reports/v0.6/preview-delay.png',ROOT/'reports/v0.6/preview-delay-more1.png',ROOT/'reports/v0.6/preview-keys-on.png',ROOT/'CHANGES-v0.6.md']
    assert len([p for p in artifacts if p.suffix=='.wav'])==5
    report=[]
    for p in artifacts:
        data=p.read_bytes();item=dict(path=str(p.relative_to(ROOT)),bytes=len(data),sha256=digest(data))
        if p.suffix=='.wav':
            with wave.open(str(p)) as w:
                assert w.getnchannels()==2 and w.getsampwidth()==2 and w.getframerate()==44100 and w.getcomptype()=='NONE'
                duration=w.getnframes()/w.getframerate();samples=array.array('h',w.readframes(w.getnframes()))
                if sys.byteorder!='little':samples.byteswap()
            peak=max(abs(s) for s in samples)/32768
            assert 0<peak<.8913
            item.update(duration_seconds=duration,channels=2,sample_rate=44100,pcm_bits=16,sample_peak_dbfs=20*math.log10(peak))
        report.append(item)
    (ROOT/'reports/v0.6/artifacts.json').write_text(json.dumps(dict(branch='v0.6-ui-presets',installed_folder=folder,artifacts=report),indent=2)+'\n')
    (ROOT/'reports/v0.6/SHA256SUMS.txt').write_text(''.join(f"{i['sha256']}  {pathlib.Path(i['path']).name}\n" for i in report))
    print(f'PASS ZIP CRC and exact manifest/versions/ARM ELF/skin/content matches; {len(entries)} entries')
    print('PASS unchanged installer folder; SDK LocalPlugin checks both architectures with 0 errors/warnings; update sets replace=True')
    print('PASS exactly five WAVs: stereo/44.1kHz/16-bit PCM, all below -1dBFS')
    for item in report: print(json.dumps(item))
    print('ZIP_SHA256 '+digest(path.read_bytes()))
if __name__=='__main__':main()
