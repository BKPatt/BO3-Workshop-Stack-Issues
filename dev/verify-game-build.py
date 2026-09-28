"""Read-only validation of a supported, unhooked BO3 process or local capture.

Capture .text sections privately with inspect_runtime.Process; never package
game bytes. This verifier reads production address/signature tables and writes
only executable identities, address facts, counts and verification outcomes.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--pid', type=int)
parser.add_argument('--capture', type=Path)
parser.add_argument('--allow-compatible', action='store_true', help='Accept exact recognized native patches before Stack attaches')
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
assert bool(args.pid) != bool(args.capture), 'Provide one source'
source = (ROOT / 'native/src/game_build.hpp').read_text()
profiles = re.findall(r'Profile (\w+)\{\s*"(\d+)", "([0-9a-f]{64})",\s*(0x[0-9a-f]+),(0x[0-9a-f]+)', source)
process = None
if args.pid:
    from inspect_runtime import Process
    process = Process(args.pid)
    observed_hash = hashlib.sha256(process.path.read_bytes()).hexdigest()
    read = process.read
    nt_offset = struct.unpack_from('<I', read(0, 64), 60)[0]
    header = read(nt_offset, 128)
    image_size, checksum = struct.unpack_from('<I', header, 80)[0], struct.unpack_from('<I', header, 88)[0]
else:
    meta = json.loads((args.capture / 'image.json').read_text())
    observed_hash = meta['sha256']
    checksum = int(meta['checksum'], 16)
    image_size = meta['image_size']
    sections = [(s['rva'], (args.capture / s['file']).read_bytes()) for s in meta['sections']]
    def read(rva, size):
        for start, data in sections:
            if start <= rva and rva + size <= start + len(data):
                return data[rva - start:rva - start + size]
        raise ValueError(f'Uncaptured RVA {rva:#x}')
try:
    profile = next((p for p in profiles if p[2] == observed_hash), None)
    assert profile, f'Unsupported executable {observed_hash}'
    name, build, _, expected_checksum, expected_size = profile
    assert checksum == int(expected_checksum, 16) and image_size == int(expected_size, 16)
    addresses = {name: int(value, 16) for name, value in re.findall(r'uintptr_t (\w+)=(0x[0-9a-f]+);', source)}
    if name == 'updated':
        addresses.update({name: int(value, 16) for name, value in re.findall(r'result\.(\w+)=(0x[0-9a-f]+);', source)})
    hooks = []
    extension = (ROOT / 'native/src/extension.cpp').read_text()
    for target, literal, length in re.findall(r'HookSpec\{game_build::addresses\.(\w+),"([^"]+)",(\d+)', extension):
        expected = bytes.fromhex(literal.replace('\\x', ''))
        assert len(expected) == int(length)
        assert read(addresses[target], len(expected)) == expected, target
        hooks.append({'name': target, 'rva': hex(addresses[target]), 'signature_verified': True})
    table = ROOT / 'native/src' / ('compat_sites.hpp' if name == 'legacy' else f'compat_sites_{build}.hpp')
    sites = [(int(rva, 16), int(kind)) for rva, kind in re.findall(r'\{(0x[0-9a-f]+), ([123])\}', table.read_text())]
    signatures = [bytes.fromhex(value) for value in ['8b0c8b330c82', '8b0c8bf7d9030c82', '8b04828b148b']]
    replacements = [bytes.fromhex(value) for value in ['4831c9909090', '4831c99090909090', '4831c04831d2']]
    compatible = 0
    for rva, kind in sites:
        expected = signatures[kind - 1]
        observed = read(rva, len(expected))
        if observed != expected:
            assert args.allow_compatible and observed == replacements[kind - 1], f'Native site {rva:#x}'
            compatible += 1
    assert len(sites) == len(set(rva for rva, _ in sites)) == 1365
    result = {'build': build, 'exe_sha256': observed_hash, 'checksum': hex(checksum),
              'image_size': hex(image_size), 'hooks_and_calls': hooks,
              'native_sites_verified': len(sites), 'native_sites_by_kind': dict(Counter(kind for _, kind in sites)),
              'original_sites': len(sites) - compatible, 'recognized_compatible_sites': compatible,
              'addresses': {key: hex(value) for key, value in addresses.items()},
              'source': 'live unhooked process' if process else 'unhooked process capture',
              'gameplay_verified_by_this_script': False}
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({'build': build, 'hooks_and_calls_verified': len(hooks), 'native_sites_verified': len(sites)}))
finally:
    if process:
        process.close()
