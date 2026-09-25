"""Independently validate all saved C++ tours against original TSPLIB data."""
import argparse
import csv
import json
import math
from pathlib import Path


def read_points(path):
    lines = Path(path).read_text().splitlines()
    start = lines.index('NODE_COORD_SECTION') + 1
    return [(float(parts[1]), float(parts[2])) for line in lines[start:]
            if len(parts := line.split()) == 3]


def cost(route, points):
    assert sorted(route) == list(range(1, len(points) + 1)), 'Invalid tour permutation'
    # ceil(r) is equivalent to TSPLIB's nint(r), then increment if nint(r) < r.
    return sum(math.ceil(math.dist(points[a - 1], points[b - 1]) / math.sqrt(10))
               for a, b in zip(route, route[1:] + route[:1]))


def verify(directory):
    points = read_points('data/att48.tsp')
    tours = list(Path(directory).glob('seed_*/result.json'))
    if not tours:
        raise ValueError('No independent-run result files found')
    for path in tours + [Path(directory) / 'result.json']:
        result = json.loads(path.read_text())
        assert result['coordinates'] == [list(p) for p in points]
        assert result['metric'] == 'ATT' and result['instance'] == 'att48'
        assert cost(result['route'], points) == result['best_length'] >= 10628
        assert cost(result['initial_route'], points) == result['initial_length']
        history = list(csv.DictReader((path.parent / 'history.csv').open()))
        best = [int(row['best']) for row in history]
        assert all(a >= b for a, b in zip(best, best[1:])), 'Best curve must be monotonic'
        assert all(int(row['current']) >= int(row['best']) for row in history)
        assert best[-1] == result['best_length'] and best[0] == result['initial_length']
        assert int(history[-1]['iteration']) == result['proposals']
        assert result['proposals'] == result['levels'] * result['steps_per_level']
        assert len(history) == result['levels'] + 1
        assert int(history[-1]['accepted']) == result['accepted']
        assert 0 <= result['best_iteration'] <= result['proposals']
    print(f'PASS: {len(tours)} independent runs plus selected result; tours, ATT costs, histories, budgets')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('directory', nargs='?', default='results/att48')
    verify(parser.parse_args().directory)
