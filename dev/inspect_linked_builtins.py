"""Inspect resolved call slots in local script captures, without invoking them."""
import argparse,json,struct
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('folder',type=Path);p.add_argument('name');p.add_argument('--offset',type=lambda n:int(n,0),action='append',required=True);a=p.parse_args()
for item in json.loads((a.folder/'manifest.json').read_text()):
    if item['name']!=a.name:continue
    b=(a.folder/(item['sha256']+'.bin')).read_bytes();print(item['sha256'],'crc',hex(struct.unpack_from('<I',b,8)[0]))
    for off in a.offset:
        print(hex(off),b[off:off+24].hex())
        for at in range((off//8)*8,min(off+24,len(b)-7),8):
            value=struct.unpack_from('<Q',b,at)[0]
            print(hex(at),hex(value),'candidate game RVA',hex(value-0x7ff62d7e0000))
