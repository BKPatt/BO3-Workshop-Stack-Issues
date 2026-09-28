"""Read-only snapshots of script headers for local compatibility research."""
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
    pool,size,capacity,singleton,used,free=struct.unpack('<QIiiIQ',process.read(0x93883f0+54*32,32))
    assert pool and size==24 and 0<capacity<100000
    # This research probe only scans the stock contiguous segment. An expanded
    # pool is deliberately rejected to avoid reading past the original block.
    assert capacity==1150,'Use the asset-link diagnostic for expanded script pools'
    headers=process.read(pool-process.base,capacity*24)
    result=[]
    args.output.mkdir(parents=True,exist_ok=True)
    for index in range(capacity):
        name,length,buffer=struct.unpack_from('<QQQ',headers,index*24)
        length &= 0xffffffff
        if not name or not buffer or not 32<=length<=4*1024*1024:continue
        try:
            text=process.read(name-process.base,300).split(b'\0',1)[0].decode('ascii')
            if not text.startswith('scripts/') or not text.endswith(('.gsc','.csc')):continue
            data=process.read(buffer-process.base,length)
        except (OSError,UnicodeDecodeError):continue
        digest=hashlib.sha256(data).hexdigest()
        (args.output/(digest+'.bin')).write_bytes(data)
        result.append({'name':text,'size':length,'sha256':digest,'header':hex(pool+index*24),'buffer':hex(buffer)})
    (args.output/'manifest.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
    print(f'Snapshotted {len(result)} compiled script assets into {args.output}; research only, excluded from release.')
finally:
    process.close()
