"""Read-only evidence for the supported Scroptss T7 3.04 continuation fix."""
import argparse
import ctypes as C
from ctypes import wintypes as W
import hashlib
import json
from pathlib import Path
from inspect_runtime import Process, P

ap = argparse.ArgumentParser()
ap.add_argument('pid', type=int)
ap.add_argument('--output', type=Path, required=True)
args = ap.parse_args()
process = Process(args.pid)
try:
    modules = (W.HMODULE * 1024)()
    needed = W.DWORD()
    if not P.EnumProcessModules(process.handle, modules, C.sizeof(modules), C.byref(needed)):
        raise C.WinError(C.get_last_error())
    result = {'pid': args.pid, 'modules': []}
    for base in modules[:needed.value // C.sizeof(W.HMODULE)]:
        name = C.create_unicode_buffer(32768)
        P.GetModuleFileNameExW(process.handle, base, name, len(name))
        path = Path(name.value)
        if path.name.lower() not in ('t7patch.dll', 'bo3_stack.dll', 'ntdll.dll'):
            continue
        entry = {'name': path.name, 'path': str(path), 'base': hex(base)}
        if path.name.lower() != 'ntdll.dll':
            entry['sha256'] = hashlib.sha256(path.read_bytes()).hexdigest()
        if path.name.lower() == 't7patch.dll':
            if entry['sha256'] != 'bc4c9e4edb895f237477a9074552a8186e69b7d36db4886d983dc13c143645c6':
                raise ValueError('Unsupported T7 probe layout')
            entry['continuation'] = hex(int.from_bytes(process.read(base-process.base+0x48bc8, 8), 'little'))
        result['modules'].append(entry)
    # Ntdll is the same Windows image in both processes; use its export RVA,
    # not an assumption that both processes map it at the same base address.
    ntdll = C.WinDLL('ntdll')
    continuation_rva = C.cast(ntdll.ZwContinue, C.c_void_p).value - ntdll._handle
    remote_ntdll = next(x for x in result['modules'] if x['name'].lower() == 'ntdll.dll')
    result['expected_continuation'] = hex(int(remote_ntdll['base'], 16) + continuation_rva)
    t7 = next((x for x in result['modules'] if x['name'].lower() == 't7patch.dll'), None)
    result['continuation_matches_os_export'] = bool(t7 and t7['continuation'] == result['expected_continuation'])
    args.output.write_text(json.dumps(result, indent=2))
    print(json.dumps(result, indent=2))
finally:
    process.close()
