#!/usr/bin/env python3
"""Integration/negative tests: reorder resilience, aliases, optional omission,
fallback pagination, ambiguity rejection, nonboolean switch safety, missing images.
"""
import copy
import json
from pathlib import Path
import shutil
import sys
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import make_skin as skin
from verify_skin import verify

class Integration(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.dump=json.loads((ROOT/'design/testbed/build/params.json').read_text())
        cls.layout=json.loads((ROOT/'design/layout.json').read_text())
        cls.base=ROOT/'design/skins/A/Plugin Skins'
    def fixture(self,records,mapping=None):
        work=Path(self.enterContext(tempfile.TemporaryDirectory()))
        dump=copy.deepcopy(self.dump);dump['params']=records
        for i,p in enumerate(records):p['index']=i
        path=work/'params.json';path.write_text(json.dumps(dump))
        params=skin.ui.load_params(str(path))
        assigned,extras=skin.match_params(params,self.layout,mapping or {})
        tui,qmap,report=skin.make_documents(params,assigned,extras,skin.THEMES['A'],'EARLY DELAY')
        out=work/'Plugin Skins';shutil.copytree(self.base,out)
        (out/'TUI.json').write_text(skin.ui.dump_json(tui))
        (out/'Q-Links.json').write_text(skin.ui.dump_json(qmap))
        (out/'skin-map.json').write_text(json.dumps({'parameters':report}))
        verify(out,path,quiet=True)
        return out,path,report
    def test_reorder_does_not_change_control_meaning(self):
        out,_,report=self.fixture(list(reversed(copy.deepcopy(self.dump['params']))))
        bykey={x['control']:x for x in report}
        self.assertEqual(bykey['time']['parameter'],'Time'); self.assertEqual(bykey['time']['index'],11)
        self.assertEqual(bykey['stereoize']['parameter'],'Stereoize'); self.assertEqual(bykey['stereoize']['index'],0)
        self.assertEqual(bykey['time']['qlink'],13);self.assertEqual(bykey['stereoize']['qlink'],8)
    def test_aliases_and_optional_width(self):
        p=[x for x in copy.deepcopy(self.dump['params']) if x['name']!='Width']
        p[0]['name']='Delay Time';p[2]['name']='Mod Rate'
        _,_,report=self.fixture(p)
        bykey={x['control']:x for x in report}
        self.assertNotIn('width',bykey);self.assertEqual(bykey['time']['parameter'],'Delay Time')
    def test_more_pages_keep_every_extra_reachable(self):
        p=copy.deepcopy(self.dump['params'])
        p.extend(dict(name=f'Extra {i}',default=.5,display='50',kind='float') for i in range(21))
        out,_,report=self.fixture(p)
        self.assertEqual(len(report),33)
        self.assertEqual(len(json.loads((out/'TUI.json').read_text())['pageData']['tabs']),3)
    def test_nonboolean_hold_is_never_quantized_by_button(self):
        p=copy.deepcopy(self.dump['params']);p[-2].update(kind='float',choices=[],display='30',default=.3)
        out,_,_=self.fixture(p)
        defs=json.loads((out/'TUI.json').read_text())['pageData']['componentDefinitions']['localComponentDefinitions']
        hold=next(x['value'] for x in defs if x['key']=='control hold')
        self.assertNotIn('Button',[x['componentData']['type'] for x in hold['componentsData']])
    def test_ambiguous_names_require_explicit_mapping(self):
        p=copy.deepcopy(self.dump['params']);p.append(dict(name='Delay Time',default=.5,display='300',kind='float'))
        with self.assertRaisesRegex(ValueError,'Ambiguous time'): self.fixture(p)
        _,_,r=self.fixture(p,{'time':'Time'})
        self.assertTrue(any(x['parameter']=='Delay Time' and x['control']=='fallback' for x in r))
    def test_missing_asset_is_rejected(self):
        out,path,_=self.fixture(copy.deepcopy(self.dump['params']))
        (out/'panel.png').unlink()
        with self.assertRaisesRegex(ValueError,'Missing image'):verify(out,path,quiet=True)

if __name__=='__main__':unittest.main(verbosity=2)
