"""Read-only lookup of active DB asset entries on the pinned Steam build."""
import argparse,hashlib,json,struct
from pathlib import Path
from inspect_runtime import Process
p=argparse.ArgumentParser();p.add_argument('pid',type=int);p.add_argument('--output',type=Path,required=True);p.add_argument('--asset',action='append');a=p.parse_args();game=Process(a.pid)
def string(ptr):return game.read(ptr-game.base,240).split(b'\0')[0].decode('ascii')
def lookup(kind,name):
    h=kind
    for c in name.lower().replace('\\','/').encode():h=(h*0x1003f+c)&0xffffffff
    index=struct.unpack('<I',game.read(0x9391980+(h%0x26400)*4,4))[0];seen=set()
    while index and index not in seen:
        seen.add(index);b=game.read(0x9448b80+index*32,32);t,_,header=struct.unpack_from('<IIQ',b)
        if t==kind and string(struct.unpack('<Q',game.read(header-game.base,8))[0])==name:
            result={'type':kind,'name':name,'index':index,'header':hex(header),'zone':b[16],'default':b[18],'header_bytes':game.read(header-game.base,72).hex(),'entry':b.hex()}
            if kind==47:
                _,length,buffer=struct.unpack('<QQQ',game.read(header-game.base,24))
                if buffer and length<=4096:result['raw_text']=game.read(buffer-game.base,length).rstrip(b'\0').decode('utf-8',errors='replace')
            return result
        index=struct.unpack_from('<I',b,20)[0]
    return {'type':kind,'name':name,'missing':True}
try:
    assert hashlib.sha256(game.path.read_bytes()).hexdigest()=='9ba98dba41e18ef47de6c63937340f8eae7cb251f8fbc2e78d70047b64aa15b5'
    names=[(85,'zm_mod'),(23,'ar_m1carbine_zm'),(48,'gamedata/weapons/zm/zm_prototype_weapons.csv'),(54,'scripts/zm/gametypes/_clientids.gsc'),(54,'scripts/zm/_zm_powerup_double_points.csc')]
    if a.asset:names=[(int(spec.split(':',1)[0]),spec.split(':',1)[1]) for spec in a.asset]
    result=[lookup(t,n) for t,n in names];a.output.write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
finally:game.close()
