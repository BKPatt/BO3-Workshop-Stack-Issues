"""Validate public compatibility addresses against the local executable in memory."""
import re,sys,json,collections
from pathlib import Path
from inspect_runtime import Process
p=Process(int(sys.argv[1]))
header=Path('research/t7patch-reference/Arxan.h').read_text()
records=[]
for group,body in re.findall(r'crc_patch_(\d)\[\]\s*=\s*\{(.*?)\};',header,re.S):
    size=8 if group=='2' else 6
    frequencies=collections.Counter()
    for address in re.findall(r'0x[0-9A-Fa-f]+',body):
        rva=int(address,16)
        try:data=p.read(rva,size)
        except OSError:print('UNREADABLE',address);raise
        frequencies[data.hex()]+=1
        records.append({'group':int(group),'rva':rva,'bytes':data.hex()})
    print(group,frequencies)
Path('research/runtime/compat-observed.json').write_text(json.dumps(records,indent=2))
p.close()
