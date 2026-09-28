"""Read-only BO3 native-code inspection for developing the engine extension."""
import argparse
import ctypes as C
from ctypes import wintypes as W
import json
from pathlib import Path
import struct

K = C.WinDLL('kernel32', use_last_error=True)
P = C.WinDLL('psapi', use_last_error=True)
K.OpenProcess.argtypes = [W.DWORD, W.BOOL, W.DWORD]
K.OpenProcess.restype = W.HANDLE
K.ReadProcessMemory.argtypes = [W.HANDLE, C.c_void_p, C.c_void_p, C.c_size_t, C.POINTER(C.c_size_t)]
K.ReadProcessMemory.restype = W.BOOL
K.CloseHandle.argtypes = [W.HANDLE]
P.EnumProcessModules.argtypes = [W.HANDLE, C.c_void_p, W.DWORD, C.POINTER(W.DWORD)]
P.GetModuleFileNameExW.argtypes = [W.HANDLE, W.HMODULE, W.LPWSTR, W.DWORD]

class Process:
    def __init__(self, pid):
        self.handle = K.OpenProcess(0x410, False, pid)
        if not self.handle:
            raise C.WinError(C.get_last_error())
        modules = (W.HMODULE * 1024)()
        needed = W.DWORD()
        if not P.EnumProcessModules(self.handle, modules, C.sizeof(modules), C.byref(needed)):
            raise C.WinError(C.get_last_error())
        self.base = modules[0]
        name = C.create_unicode_buffer(32768)
        P.GetModuleFileNameExW(self.handle, modules[0], name, len(name))
        self.path = Path(name.value)
        if self.path.name.lower() != 'blackops3.exe':
            raise ValueError('Target must be BlackOps3.exe')

    def read(self, rva, size):
        buf = C.create_string_buffer(size)
        count = C.c_size_t()
        if not K.ReadProcessMemory(self.handle, self.base + rva, buf, size, C.byref(count)):
            raise C.WinError(C.get_last_error())
        return buf.raw[:count.value]

    def close(self):
        K.CloseHandle(self.handle)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('pid', type=int)
    ap.add_argument('--rva', type=lambda x: int(x, 0), action='append')
    ap.add_argument('--size', type=lambda x: int(x, 0), default=160)
    ap.add_argument('--output', type=Path)
    args = ap.parse_args()
    p = Process(args.pid)
    try:
        print(json.dumps({'image':str(p.path), 'base':hex(p.base)}))
        from capstone import Cs, CS_ARCH_X86, CS_MODE_64
        cs = Cs(CS_ARCH_X86, CS_MODE_64)
        for rva in args.rva or [0x20D6B50,0x20D7680,0x14236A0,0x1425010]:
            data = p.read(rva, args.size)
            print(f'RVA {rva:#x}: {data[:32].hex()}')
            for ins in cs.disasm(data, 0x140000000 + rva):
                print(f'{ins.address:#x}: {ins.mnemonic} {ins.op_str}')
            if args.output:
                args.output.mkdir(parents=True, exist_ok=True)
                (args.output / f'{rva:x}.bin').write_bytes(data)
    finally:
        p.close()

if __name__ == '__main__':
    main()
