"""Read-only asset-pool evidence for the pinned Steam executable."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
from inspect_runtime import Process

parser=argparse.ArgumentParser()
parser.add_argument('pid',type=int)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
root=Path(__file__).resolve().parents[1]
reference=(root/'research/boiii-reference/src/client/game/structs/db/xasset/core.hpp').read_text(encoding='utf-8')
enum=reference.split('enum class XAssetType : int32_t {',1)[1].split('};',1)[0]
names={int(value,16):name.lower() for name,value in re.findall(r'^\s*(\w+) = 0x([0-9A-Fa-f]+),',enum,re.M)}
process=Process(args.pid)
try:
    assert hashlib.sha256(process.path.read_bytes()).hexdigest()=='9ba98dba41e18ef47de6c63937340f8eae7cb251f8fbc2e78d70047b64aa15b5'
    result={'pid':args.pid,'zone_count':int.from_bytes(process.read(0x939197c,4),'little'),'pools':[]}
    descriptors=process.read(0x93883f0,0x67*32)
    for kind in range(0x67):
        original,size,capacity,singleton,used,free=struct.unpack_from('<QIiiIQ',descriptors,kind*32)
        result['pools'].append({'type':kind,'name':names[kind],'original':hex(original),'item_size':size,
            'capacity':capacity,'singleton':singleton,'allocated':used,'free_head':hex(free)})
    args.output.write_text(json.dumps(result,indent=2),encoding='utf-8')
    print(json.dumps({'zone_count':result['zone_count'],'largest_utilization':sorted(
        [p for p in result['pools'] if p['capacity']>1 and not p['singleton']],
        key=lambda p:p['allocated']/p['capacity'],reverse=True)[:12]},indent=2))
finally:
    process.close()
