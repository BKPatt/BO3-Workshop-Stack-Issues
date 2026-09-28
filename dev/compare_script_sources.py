"""Compare source CRCs, which survive VM linking, in local research captures."""
import json,struct
from pathlib import Path
def sources(folder):
    folder=Path(folder);groups={}
    for item in json.loads((folder/'manifest.json').read_text()):
        data=(folder/(item['sha256']+'.bin')).read_bytes()
        groups.setdefault(item['name'],set()).add(hex(struct.unpack_from('<I',data,8)[0]))
    return groups
a=sources('research/nacht-alone-scripts');b=sources('research/nacht-ultimis-failed-start-scripts')
for name in sorted(a.keys()|b.keys()):
    if a.get(name)!=b.get(name):print(name,'Nacht',sorted(a.get(name,[])),'combined',sorted(b.get(name,[])))
