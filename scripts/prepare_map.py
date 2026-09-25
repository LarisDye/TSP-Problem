"""Extract Natural Earth display data. Solver coordinates remain unchanged."""
import csv
import hashlib
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
places = json.loads((root / 'tmp/ne_10m_populated_places_simple.geojson').read_text(encoding='utf-8'))
states = json.loads((root / 'tmp/ne_110m_admin_1_states_provinces.geojson').read_text(encoding='utf-8'))
capitals = sorted((f for f in places['features']
                   if f['properties']['adm0_a3'] == 'USA'
                   and f['properties']['featurecla'] == 'Admin-1 capital'
                   and f['properties']['adm1name'] not in ('Alaska', 'Hawaii')),
                  key=lambda f: f['properties']['adm1name'])
assert len(capitals) == 48
assert len({f['properties']['adm1name'] for f in capitals}) == 48
with (root / 'data/att48_capitals.csv').open('w', encoding='utf-8', newline='') as file:
    writer = csv.writer(file)
    writer.writerow(['id', 'state', 'capital', 'longitude', 'latitude'])
    for i, f in enumerate(capitals, 1):
        writer.writerow([i, f['properties']['adm1name'], f['properties']['name'], *f['geometry']['coordinates']])
selected = [{'type': 'Feature', 'properties': {'name': f['properties']['name']}, 'geometry': f['geometry']}
            for f in states['features'] if f['properties']['name'] not in ('Alaska', 'Hawaii', 'District of Columbia')]
assert len(selected) == 48
(root / 'data/us48_states.geojson').write_text(json.dumps({'type': 'FeatureCollection', 'features': selected}), encoding='utf-8')
paths = ['data/att48.tsp', 'data/att48.opt.tour', 'data/att48_capitals.csv', 'data/us48_states.geojson']
(root / 'data/SHA256SUMS').write_text('\n'.join(f'{hashlib.sha256((root/p).read_bytes()).hexdigest()}  {p}' for p in paths) + '\n')
print('Saved 48 capitals, 48 state boundaries, and input checksums')
