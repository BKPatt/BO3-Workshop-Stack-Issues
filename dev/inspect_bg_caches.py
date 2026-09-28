"""Read-only BG precache lists; records are excluded from release artifacts."""
import argparse,hashlib,json,struct
from pathlib import Path
from inspect_runtime import Process
p=argparse.ArgumentParser();p.add_argument('pid',type=int);p.add_argument('--output',type=Path,required=True);a=p.parse_args();game=Process(a.pid)
try:
    assert hashlib.sha256(game.path.read_bytes()).hexdigest()=='9ba98dba41e18ef47de6c63937340f8eae7cb251f8fbc2e78d70047b64aa15b5'
    pool,size,cap,_,used,free=struct.unpack('<QIiiIQ',game.read(0x93883f0+85*32,32));assert size==24 and cap==32
    result=[]
    for i in range(cap):
        name,records,count=struct.unpack('<QQI4x',game.read(pool-game.base+i*size,size))
        if not name or not records or count>100000:continue
        try:
            text=game.read(name-game.base,128).split(b'\0')[0].decode('ascii')
            if not text or not text.replace('_','').isalnum():continue
            b=game.read(records-game.base,count*132)
            entries=[{'type':struct.unpack_from('<I',b,j*132)[0],'name':b[j*132+4:(j+1)*132].split(b'\0')[0].decode('ascii')} for j in range(count)]
            result.append({'name':text,'header':hex(pool+i*size),'count':count,'entries':entries})
        except (OSError,UnicodeDecodeError):continue
    a.output.write_text(json.dumps(result,indent=2));print([(x['name'],x['count'],sum(y['type']==7 for y in x['entries'])) for x in result])
finally:game.close()
