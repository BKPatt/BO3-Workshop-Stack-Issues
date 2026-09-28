"""Developer analysis of the locally captured BO3 code sections; never packaged."""
import argparse, bisect, json, re, struct
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_64

ROOT = Path('research/runtime')
META = json.loads((ROOT/'image.json').read_text())
SECTIONS = {s['name']:(s['rva'],(ROOT/(s['name'][1:]+'.bin')).read_bytes()) for s in META['sections']}
TEXT_RVA, TEXT = SECTIONS['.text']
PDATA = SECTIONS['.pdata'][1]
FUNCS = sorted((a,b) for a,b,c in struct.iter_unpack('<III',PDATA[:len(PDATA)//12*12]) if TEXT_RVA<=a<b<TEXT_RVA+len(TEXT))
STARTS = [a for a,b in FUNCS]
CS = Cs(CS_ARCH_X86,CS_MODE_64)

def containing(rva):
    i=bisect.bisect_right(STARTS,rva)-1
    return FUNCS[i] if i>=0 and rva<FUNCS[i][1] else (rva,rva+256)

def disasm(rva,size=None):
    a,b=containing(rva)
    if size is not None: a,b=rva,rva+size
    for ins in CS.disasm(TEXT[a-TEXT_RVA:b-TEXT_RVA],0x140000000+a):
        print(f'{ins.address:#x}: {ins.mnemonic} {ins.op_str}')

def refs(target):
    found=[]
    pattern=rb'[\x40-\x4f][\x8d\x8b\x89][\x05\x0d\x15\x1d\x25\x2d\x35\x3d]....'
    for m in re.finditer(pattern,TEXT,re.S):
        at=TEXT_RVA+m.start()
        if at+7+struct.unpack_from('<i',m.group(),3)[0]==target:
            found.append(at)
    return found

if __name__=='__main__':
    ap=argparse.ArgumentParser()
    ap.add_argument('--strings')
    ap.add_argument('--refs',type=lambda x:int(x,0))
    ap.add_argument('--function',type=lambda x:int(x,0))
    ap.add_argument('--size',type=lambda x:int(x,0))
    args=ap.parse_args()
    if args.strings:
        base,data=SECTIONS['.rdata']
        for m in re.finditer(rb'[\x20-\x7e]{4,}',data):
            if re.search(args.strings,m.group().decode(),re.I):
                print(hex(base+m.start()),m.group().decode())
    if args.refs is not None:
        for r in refs(args.refs):
            print(f'Reference {r:#x}, function {containing(r)[0]:#x}')
    if args.function is not None: disasm(args.function,args.size)
