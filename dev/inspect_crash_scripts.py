"""Read-only local crash/VM diagnostics. Dumps are never included in releases."""
import argparse, json, logging, struct
from pathlib import Path
from minidump.minidumpfile import MinidumpFile, CONTEXT
logging.disable(logging.CRITICAL)
p=argparse.ArgumentParser();p.add_argument('dump',type=Path);args=p.parse_args()
d=MinidumpFile.parse(str(args.dump));r=d.get_reader()
game=next(m for m in d.modules.modules if m.name.lower().endswith('blackops3.exe'))
base=game.baseaddress
e=d.exception.exception_records[0];d.file_handle.seek(e.ThreadContext.Rva);c=CONTEXT.parse(d.file_handle)
print('exception',hex(c.Rip-base),'base',hex(base))
for instance in (0,1):
    try:
        vm=base+0x5124840+instance*0x8a40
        raw=r.read(vm,0xa40);count=struct.unpack_from('<i',raw,16)[0]
        frame=struct.unpack_from('<Q',raw,24)[0]
        fs=struct.unpack('<4Q',r.read(base+0x513dd30+instance*32,32))
        print('instance',instance,'count',count,'frame',hex(frame),'fs',[hex(x) for x in fs])
        print('frames',[hex(struct.unpack_from('<Q',raw,0x40+i*40)[0]) for i in range(min(max(count+1,0),64))])
    except Exception as error:print(type(error).__name__,str(error)[:180])
for register in ('Rax','Rbx','Rcx','Rdx','Rsi','Rdi','R8','R9','R10','R11','R12','R13','R14','R15'):
    print(register,hex(getattr(c,register)))
