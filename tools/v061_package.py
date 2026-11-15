#!/usr/bin/env python3
"""Deterministic v0.6.1 ZIP from validated output; no network or device actions."""
import hashlib
import json
import pathlib
import struct
import sys
import zipfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'sdk/tools'))
import package as sdk_package
import mpc_remote


def main():
    meta = json.loads((ROOT / 'plugin/plugin.json').read_text())
    folder = sdk_package.output_folder_name(meta)
    assert meta['version'] == '0.6.1' and meta['uid'] == 'FpEd'
    assert folder == 'Flat Peach - VST - 80s Lofi Delay Box'
    built = ROOT / 'plugin/build' / folder
    local = mpc_remote.LocalPlugin(str(built), meta)
    local.check()
    assert not local.errors and not local.warnings, (local.errors, local.warnings)
    assert set(local.builds) == {'arm32', 'arm64'}
    assert json.loads((built / 'plugin.json').read_text()) == meta
    assert '<version>0.6.1.0</version>' in (built / 'version.xml').read_text()
    for destination, source in [('Plugin Skins', 'skins'), ('Content', 'Content')]:
        expected = {p.relative_to(ROOT / 'plugin' / source): p.read_bytes()
                    for p in (ROOT / 'plugin' / source).rglob('*') if p.is_file()}
        actual = {p.relative_to(built / destination): p.read_bytes()
                  for p in (built / destination).rglob('*') if p.is_file()}
        assert actual == expected, destination
    entries = {folder + '/' + p.relative_to(built).as_posix(): p.read_bytes()
               for p in built.rglob('*') if p.is_file()}
    entries['README.md'] = (ROOT / 'plugin/README.md').read_bytes()
    entries['CHANGES-v0.6.1.md'] = (ROOT / 'CHANGES-v0.6.1.md').read_bytes()
    intro = ('UPDATE v0.5/v0.6 IN PLACE\n=========================\n'
             'Replace Flat Peach - VST - 80s Lofi Delay Box in VST2 Plugins.\n'
             'Do not install side by side. UID FpEd is unchanged.\n'
             'Save projects before updating. No automatic deployment/restart.\n\n')
    entries['INSTALL.txt'] = (intro + sdk_package.install_text(meta, folder)).encode()
    path = ROOT / 'plugin/dist/80s-lofi-delay-box-0.6.1-mpc.zip'
    with zipfile.ZipFile(path, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for name, data in sorted(entries.items()):
            info = zipfile.ZipInfo(name, (2026, 11, 15, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            info.create_system = 3
            z.writestr(info, data, compresslevel=9)
    with zipfile.ZipFile(path) as z:
        assert z.testzip() is None
        assert {name: z.read(name) for name in z.namelist()} == entries
        assert len(z.namelist()) == len(entries)
        for key, machine in [('file32', 40), ('file64', 183)]:
            data = z.read(folder + '/' + meta[key])
            assert data[:4] == b'\x7fELF' and struct.unpack_from('<H', data, 18)[0] == machine
    sha = hashlib.sha256(path.read_bytes()).hexdigest()
    report = ROOT / 'reports/v0.6.1'
    (report / 'SHA256SUMS.txt').write_text(f'{sha}  {path.name}\n')
    (report / 'artifacts.json').write_text(json.dumps(dict(
        branch='v0.6.1-gui-refresh', base='4f3b4ab', installed_folder=folder,
        zip=str(path.relative_to(ROOT)), bytes=path.stat().st_size, sha256=sha,
        entries=len(entries)), indent=2) + '\n')
    print(f'PASS ZIP CRC, exact manifest, ARM ELF, metadata and unchanged skins/content: {len(entries)} entries')
    print('PASS SDK LocalPlugin: 0 errors/0 warnings, same in-place update folder')
    print(f'ZIP {path} ({path.stat().st_size} bytes)\nSHA256 {sha}')

if __name__ == '__main__':
    main()
