"""Resolve diagnostic addresses using a same-session read-only script snapshot."""
import argparse,json,re
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('snapshot',type=Path);p.add_argument('log',type=Path);a=p.parse_args()
items=json.loads((a.snapshot/'manifest.json').read_text())
text=a.log.read_text(errors='replace').rsplit('VERSION 1.1.0',1)[-1]
for line in text.splitlines():
    if 'SCRIPT ERROR' not in line and 'SCRIPT FRAME' not in line:continue
    m=re.search(r'unresolved=([0-9A-Fa-f]+)',line)
    if m:
        pos=int(m[1],16)
        for item in items:
            start=int(item.get('buffer','0'),16)
            if start and start<=pos<start+item['size']:
                line=line.replace(m[0],item['name']+' +'+hex(pos-start));break
    print(line)
