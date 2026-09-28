"""Inspect locally captured compiled scripts without changing game content."""
import argparse,json,re,struct
from pathlib import Path
def hash32(name):
    h=0x4b9ace2f
    for c in name.lower().encode()+b'\0':h=((h^c)*0x1000193)&0xffffffff
    return h
p=argparse.ArgumentParser();p.add_argument('folder',type=Path);p.add_argument('name');p.add_argument('--code',action='store_true');args=p.parse_args()
cache=Path('research/script-identifier-hashes.json')
if not cache.exists():
    names=set()
    for root in [Path('C:/Program Files (x86)/Steam/steamapps/common/Call of Duty Black Ops III 455130/share/raw/scripts'),Path('research/tf-options/scripts')]:
        for path in root.rglob('*'):
            if path.suffix.lower() in ('.gsc','.csc','.gsh','.csh'):
                names.update(re.findall(r'[a-zA-Z_][a-zA-Z_0-9]*',path.read_text(errors='ignore')))
    cache.write_text(json.dumps({str(hash32(n)):n.lower() for n in names}))
names={str(hash32(n)):n for n in json.loads(cache.read_text()).values()};label=lambda h:names.get(str(h),hex(h))
mapping=Path('research/boiii-reference/src/client/game/structs/scr/vm/op/code.hpp').read_text().split('OPCODE_JUMPTABLE_ENTRY_MAP =',1)[1]
ops=re.findall(r'Opcode::(\w+)',mapping)
for item in json.loads((args.folder/'manifest.json').read_text()):
    if item['name']!=args.name:continue
    b=(args.folder/(item['sha256']+'.bin')).read_bytes();u32=lambda at:struct.unpack_from('<I',b,at)[0];u16=lambda at:struct.unpack_from('<H',b,at)[0]
    print(item['name'],item['sha256'],'size',len(b))
    print('includes',[b[u32(u32(12)+i*4):].split(b'\0')[0].decode() for i in range(b[68])])
    exports=[]
    for i in range(u16(58)):
        at=u32(32)+i*20;crc,code,name,namespace,param,flags=struct.unpack_from('<IIIIBB',b,at)
        exports.append((code,label(namespace)+'::'+label(name)))
        print('export',hex(code),exports[-1][1],'params',param,'flags',hex(flags))
    at=u32(36)
    for i in range(u16(60)):
        name,ns,count,param,flags=struct.unpack_from('<IIHBB',b,at)
        refs=[hex(u32(at+12+j*4)) for j in range(count)];at+=12+count*4
        print('import',label(ns)+'::'+label(name),'params',param,'flags',hex(flags),'refs',refs)
    print('strings',[s.decode(errors='replace') for s in re.findall(rb'[ -~]{4,}',b)])
    if args.code:
        for at in range(min(x[0] for x in exports),u32(20)+u32(48),2):
            v=u16(at)
            extra=names.get(str(u32(at)),'') if at+4<=len(b) else ''
            print(hex(at),hex(v),ops[v] if v<len(ops) else '?',extra)
