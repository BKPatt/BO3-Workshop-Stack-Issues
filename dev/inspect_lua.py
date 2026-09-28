"""Read-only lookup of BO3's native menu bindings."""
import sys,struct
from inspect_runtime import Process
p=Process(int(sys.argv[1]))
def read(address,size):return p.read(address-p.base,size)
def u64(data,offset=0):return struct.unpack_from('<Q',data,offset)[0]
def table(address):
    h=read(address,0x40);mask=struct.unpack_from('<I',h,0x18)[0];nodes=u64(h,0x20)
    if mask>65535:raise ValueError('Bad Lua table')
    data=read(nodes,(mask+1)*32)
    result={}
    for i in range(mask+1):
        at=i*32
        if struct.unpack_from('<I',data,at)[0]&15!=4:continue
        key=read(u64(data,at+8)+20,100).split(b'\0')[0].decode(errors='replace')
        result[key]=(struct.unpack_from('<I',data,at+16)[0],u64(data,at+24))
    return result
vm=u64(p.read(0x19bf7e08,8));print('VM',hex(vm))
if vm:
    globals_ptr=u64(read(vm+0x70,16),8)
    engine=table(globals_ptr)['Engine'];print('ENGINE',engine[0],hex(engine[1]))
    for name,value in table(engine[1]).items():
        if name.startswith('Mods_'):
            print(name,value[0],hex(value[1]),read(value[1],0x30).hex())
p.close()
