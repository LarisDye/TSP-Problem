"""Check actual executable input validation, output fidelity, and reproducibility."""
import json
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXE = ROOT / 'build/tsp_sa.exe'
(ROOT / 'tmp').mkdir(exist_ok=True)


def run(*args):
    return subprocess.run([str(EXE), '--no-visual', *map(str, args)], cwd=ROOT,
                          capture_output=True, text=True, timeout=15)


with tempfile.TemporaryDirectory(dir=ROOT / 'tmp') as temporary:
    temporary = Path(temporary)
    for flags in [('--cooling', '1'), ('--cooling', 'nan'), ('--steps', '0'),
                  ('--seed', '-1'), ('--max-levels', '1x'), ('--runs', '0'),
                  ('--min-ratio', '0'), ('--seed', '4294967295', '--runs', '2')]:
        result = run(*flags, '--output', temporary)
        assert result.returncode != 0 and 'Error:' in result.stderr, flags
    template = 'NAME : tiny\nTYPE : TSP\nDIMENSION : 3\nEDGE_WEIGHT_TYPE : ATT\nNODE_COORD_SECTION\n1 0 0\n2 3 4\n3 5 0\nEOF\n'
    for text in [template.replace('3 5 0', '2 5 0'), template.replace('3 5 0\n', ''),
                 template.replace('2 3 4', '2 nan 4'), template.replace('ATT', 'GEO'),
                 template.replace('DIMENSION : 3', 'DIMENSION : 3x')]:
        path = temporary / 'bad.tsp'
        path.write_text(text)
        result = run('--input', path, '--output', temporary)
        assert result.returncode != 0 and 'Error:' in result.stderr, text
    first, second = temporary / 'first', temporary / 'second'
    for target in (first, second):
        result = run('--output', target)
        assert result.returncode == 0, result.stderr
    a, b = [json.loads((target / 'result.json').read_text()) for target in (first, second)]
    a.pop('elapsed_seconds'); b.pop('elapsed_seconds')
    assert a == b
    assert a['best_length'] == 10628 and a['proposals'] == 2188800
    assert (first / 'history.csv').read_bytes() == (second / 'history.csv').read_bytes()
    assert (first / 'best.tour').read_bytes() == (second / 'best.tour').read_bytes()
print('PASS: CLI invalid parameters, malformed TSPLIB files, deterministic output, default benchmark')
