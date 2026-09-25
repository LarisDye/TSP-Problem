"""Independent small-instance checks: python -m unittest -v."""

import itertools
import math
import random
import unittest

from tsp_sa import euclidean_distances, simulated_annealing_tsp, two_opt_delta


def direct_cost(route, distance):
    closed = list(route) + [route[0]]
    return math.fsum(distance[a][b] for a, b in zip(closed, closed[1:]))


class TestTspAnnealing(unittest.TestCase):
    def test_delta_against_full_cost(self):
        # Non-metric symmetric costs also satisfy the four-edge identity.
        rng = random.Random(15)
        for n in range(3, 10):
            distance = [[0.0] * n for _ in range(n)]
            for i in range(n):
                for j in range(i + 1, n):
                    distance[i][j] = distance[j][i] = rng.uniform(0, 20)
            for _ in range(10):
                tail = list(range(1, n))
                rng.shuffle(tail)
                route = [0] + tail
                for i, j in itertools.combinations(range(1, n), 2):
                    neighbor = route[:i] + route[i:j + 1][::-1] + route[j + 1:]
                    expected = direct_cost(neighbor, distance) - direct_cost(route, distance)
                    self.assertAlmostEqual(two_opt_delta(route, distance, i, j), expected)

    def test_small_instance_against_exhaustive_search(self):
        points = [(0, 0), (3, 0), (4, 2), (3, 4), (0, 4), (-1, 2), (1, 2)]
        distance = euclidean_distances(points)
        optimum = min(
            direct_cost((0,) + tail, distance)
            for tail in itertools.permutations(range(1, len(points)))
        )
        for seed in (0, 1, 42):
            result = simulated_annealing_tsp(distance, seed=seed, cooling_rate=0.95)
            self.assertEqual(sorted(result.route), list(range(len(points))))
            self.assertEqual(result.route[0], 0)
            self.assertAlmostEqual(result.length, direct_cost(result.route, distance))
            self.assertLessEqual(result.length, result.initial_length + 1e-9)
            # An empirical check for these seeds, not a theorem about SA.
            self.assertAlmostEqual(result.length, optimum)

    def test_scale_invariance_and_reproducibility(self):
        distance = euclidean_distances([(0, 0), (1, 0), (1, 1), (0, 1)])
        options = dict(seed=9, cooling_rate=0.9, proposals_per_temperature=40)
        result = simulated_annealing_tsp(distance, **options)
        repeated = simulated_annealing_tsp(distance, **options)
        scaled = simulated_annealing_tsp([[1000 * x for x in row] for row in distance], **options)
        self.assertEqual(result, repeated)
        self.assertAlmostEqual(result.length, 4.0)
        self.assertAlmostEqual(scaled.length, result.length * 1000)
        self.assertAlmostEqual(scaled.initial_temperature, result.initial_temperature * 1000)

    def test_degenerate_instances_and_iteration_cap(self):
        for points in ([(0, 0)] * 4, [(0, 0), (1, 0), (0, 1)]):
            distance = euclidean_distances(points)
            result = simulated_annealing_tsp(distance, max_levels=2, proposals_per_temperature=7)
            self.assertEqual(result.proposals, 14)
            self.assertEqual(sorted(result.route), list(range(len(points))))
            self.assertAlmostEqual(result.length, direct_cost(list(range(len(points))), distance))

    def test_rejects_asymmetric_costs(self):
        with self.assertRaisesRegex(ValueError, "symmetric"):
            simulated_annealing_tsp([[0, 1, 2], [3, 0, 1], [2, 1, 0]])


if __name__ == "__main__":
    unittest.main()
