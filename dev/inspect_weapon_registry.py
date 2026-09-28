"""Read-only snapshot of this supported build's indexed weapon registry."""
import argparse,hashlib,json,struct,time
from pathlib import Path
from inspect_runtime import Process
p=argparse.ArgumentParser();p.add_argument('pid',type=int);p.add_argument('--output',type=Path,required=True);p.add_argument('--wait-ready',action='store_true');a=p.parse_args();game=Process(a.pid)
try:
    assert hashlib.sha256(game.path.read_bytes()).hexdigest()=='9ba98dba41e18ef47de6c63937340f8eae7cb251f8fbc2e78d70047b64aa15b5'
    count=struct.unpack('<I',game.read(0x19bf5314,4))[0]
    deadline=time.monotonic()+45
    while a.wait_ready and count==0 and time.monotonic()<deadline:
        time.sleep(.25);count=struct.unpack('<I',game.read(0x19bf5314,4))[0]
    if a.wait_ready and count:
        time.sleep(1);count=struct.unpack('<I',game.read(0x19bf5314,4))[0]
    pointers=struct.unpack('<512Q',game.read(0x19bf4310,4096))
    items=[]
    for index,ptr in enumerate(pointers):
        if not ptr:continue
        try:
            names=struct.unpack('<QQ',game.read(ptr-game.base,16))
            text=[game.read(name-game.base,128).split(b'\0')[0].decode('ascii') for name in names]
        except (OSError,UnicodeDecodeError):continue
        items.append({'index':index,'registered':index<=count,'header':hex(ptr),'name':text[0],'root':text[1]})
    value={'pid':a.pid,'registered_count':count,'weapons':items}
    a.output.write_text(json.dumps(value,indent=2));print(json.dumps({'registered_count':count,'readable_entries':len(items),'output':str(a.output)}))
finally:game.close()
