#!/usr/bin/env python3
"""Inventory the repository's actual advertised API; lowering is not proof of parity."""
import argparse
import json
from pathlib import Path
import re
import subprocess

ROOT=Path(__file__).resolve().parent.parent

def inventory():
    catalog=json.loads(subprocess.check_output(['node','-e',
        "const x=require('./vscode/language-data.js');console.log(JSON.stringify({modules:x.BUILTIN_MODULES,members:x.MODULE_MEMBERS}))"],cwd=ROOT,text=True))
    source=(ROOT/'selfhost/codegen.se').read_text()
    section=source[source.index('make _emit_call '):source.index('make _abi_type ')]
    lowered={}
    active=[]
    for line in section.splitlines():
        indent=len(line)-len(line.lstrip())
        if indent==12 and line.strip().startswith('if module '):
            active=re.findall(r'module == "([^"]+)"',line)
        elif indent<12 and line.strip():
            active=[]
        if indent>=16 and line.strip().startswith('if method '):
            names=re.findall(r'"([^"]+)"',line)
            for module in active:lowered.setdefault(module,set()).update(names)
    properties={'math':{'pi','e','tau','inf'},'time':{'now'},'os':{'cwd','platform'},'random':{'num'}}
    result=[]
    for module in catalog['modules']:
        members=[]
        for name,usage,description,kind in catalog['members'].get(module,[]):
            present=name in lowered.get(module,set()) or name in properties.get(module,set())
            members.append(dict(name=name,kind=kind,usage=usage,lowering_present=present,
                                conformance='not_certified_by_inventory'))
        result.append(dict(module=module,members=members))
    return dict(module_count=len(result),member_count=sum(len(m['members']) for m in result),
                note='A lowering branch is not evidence of complete arity, type, error or runtime compatibility.',modules=result)

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();data=inventory()
    args.output.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n')
    print(data['module_count'],'modules;',data['member_count'],'advertised members inventoried')
