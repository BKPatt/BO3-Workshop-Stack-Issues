"""Read-only pool/zone evidence for the supported Steam executable."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from inspect_runtime import Process

parser=argparse.ArgumentParser()
parser.add_argument('pid',type=int)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
process=Process(args.pid)
try:
    assert hashlib.sha256(process.path.read_bytes()).hexdigest()=='9ba98dba41e18ef47de6c63937340f8eae7cb251f8fbc2e78d70047b64aa15b5'
    result={'pid':args.pid,'pools':[],'zone_count':int.from_bytes(process.read(0x939197c,4),'little')}
    for kind,name in [(47,'rawfile'),(54,'scriptparsetree')]:
        original,size,capacity,singleton,used,free=struct.unpack('<QIiiIQ',process.read(0x93883f0+kind*32,32))
        result['pools'].append({'type':kind,'name':name,'original':hex(original),'item_size':size,
            'capacity':capacity,'singleton':singleton,'allocated':used,'free_head':hex(free)})
    args.output.write_text(json.dumps(result,indent=2))
    print(json.dumps(result,indent=2))
finally:
    process.close()
