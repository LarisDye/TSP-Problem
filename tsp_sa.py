"""Simulated annealing for a complete, symmetric TSP (standard library only)."""

from dataclasses import dataclass
import math
import random


@dataclass
class Result:
    route: list[int]  # City 0 occurs first; the return to 0 is implicit.
    length: float
    initial_length: float
    initial_temperature: float
    temperature_levels: int
    proposals: int
    accepted: int


def euclidean_distances(points):
    """Build an exactly symmetric distance matrix from coordinate vectors."""
    n = len(points)
    distance = [[0.0] * n for _ in range(n)]
    for i in range(n):
        for j in range(i + 1, n):
            distance[i][j] = distance[j][i] = math.dist(points[i], points[j])
    return distance


def tour_length(route, distance):
    """Include the edge from the final city back to the first city."""
    return math.fsum(
        distance[route[k]][route[(k + 1) % len(route)]]
        for k in range(len(route))
    )


def two_opt_delta(route, distance, i, j):
    """Cost change for reversing route[i:j+1], with 1 <= i < j < n.

    This four-edge formula requires symmetric distances.
    """
    a, b = route[i - 1], route[i]
    c, d = route[j], route[(j + 1) % len(route)]
    return (distance[a][c] - distance[a][b]) + (
        distance[b][d] - distance[c][d]
    )


def simulated_annealing_tsp(
    distance,
    *,
    seed=42,
    initial_temperature=None,
    cooling_rate=0.98,
    minimum_temperature_ratio=1e-4,
    proposals_per_temperature=None,
    max_levels=2000,
):
    """Return the best route encountered, not necessarily a global optimum.

    Input: an n-by-n finite, nonnegative, exactly symmetric distance matrix,
    with zero diagonal and n >= 3. City IDs are 0, ..., n-1.

    If initial_temperature is None, sample positive 2-opt cost changes at
    the initial route and use -mean(delta_positive) / log(0.8). This targets
    80% acceptance for the mean sampled worsening, not for all proposals.

    Each temperature level makes 100*n proposals by default. Stop when
    T/T0 <= minimum_temperature_ratio or max_levels is reached.
    """
    n = len(distance)
    if n < 3 or any(len(row) != n for row in distance):
        raise ValueError("distance must be a square matrix with n >= 3")
    if any(not math.isfinite(x) or x < 0 for row in distance for x in row):
        raise ValueError("distances must be finite and nonnegative")
    if any(distance[i][i] != 0 for i in range(n)):
        raise ValueError("distance diagonal must be zero")
    if any(distance[i][j] != distance[j][i]
           for i in range(n) for j in range(i + 1, n)):
        raise ValueError("the four-edge delta formula needs symmetric distances")
    if not 0 < cooling_rate < 1:
        raise ValueError("cooling_rate must be between 0 and 1")
    if not 0 < minimum_temperature_ratio < 1:
        raise ValueError("minimum_temperature_ratio must be between 0 and 1")
    if not isinstance(max_levels, int) or max_levels < 1:
        raise ValueError("max_levels must be a positive integer")
    if proposals_per_temperature is None:
        proposals_per_temperature = 100 * n
    if (not isinstance(proposals_per_temperature, int)
            or proposals_per_temperature < 1):
        raise ValueError("proposals_per_temperature must be a positive integer")

    rng = random.Random(seed)
    # Fixing city 0 removes rotations without excluding any cycle.
    tail = list(range(1, n))
    rng.shuffle(tail)
    route = [0] + tail
    current = tour_length(route, distance)
    best_route, best = route.copy(), current
    initial_length = current

    if initial_temperature is None:
        positive_deltas = []
        for _ in range(200):
            i, j = sorted(rng.sample(range(1, n), 2))
            delta = two_opt_delta(route, distance, i, j)
            if delta > 0:
                positive_deltas.append(delta)
        if positive_deltas:
            # Divide before summing to avoid an unnecessary large sum.
            scale = math.fsum(d / len(positive_deltas) for d in positive_deltas)
            initial_temperature = -scale / math.log(0.8)
        else:
            # The sample may contain no uphill move, even for a nontrivial TSP.
            initial_temperature = max(max(row) for row in distance) or 1.0
    if not math.isfinite(initial_temperature) or initial_temperature <= 0:
        raise ValueError("initial_temperature must be finite and positive")

    temperature = initial_temperature
    ratio = 1.0
    levels = proposals = accepted = 0
    while ratio > minimum_temperature_ratio and levels < max_levels:
        if temperature <= 0:  # Guard floating-point underflow.
            break
        for _ in range(proposals_per_temperature):
            # Uniform position pairs form a symmetric proposal distribution.
            i, j = sorted(rng.sample(range(1, n), 2))
            delta = two_opt_delta(route, distance, i, j)
            proposals += 1
            if delta <= 0 or rng.random() < math.exp(-delta / temperature):
                route[i:j + 1] = reversed(route[i:j + 1])
                current += delta
                accepted += 1
                if current < best:
                    # Recompute when saving a record to limit numerical drift.
                    current = tour_length(route, distance)
                    if current < best:
                        best_route, best = route.copy(), current
        # Correct accumulated floating-point error at every temperature level.
        current = tour_length(route, distance)
        if current < best:
            best_route, best = route.copy(), current
        levels += 1
        ratio *= cooling_rate
        temperature = initial_temperature * ratio

    return Result(
        best_route, tour_length(best_route, distance), initial_length,
        initial_temperature, levels, proposals, accepted,
    )


if __name__ == "__main__":
    # Ten synthetic cities; the coordinates are not geographic coordinates.
    points = [
        (0, 0), (2, 1), (4, 0), (6, 2), (5, 5),
        (3, 6), (1, 5), (-1, 3), (2, 3), (4, 3),
    ]
    distances = euclidean_distances(points)
    result = simulated_annealing_tsp(distances, seed=42)
    closed_route = result.route + [result.route[0]]
    print("Route:", " -> ".join(map(str, closed_route)))
    print(f"Initial length: {result.initial_length:.6f}")
    print(f"Best length: {result.length:.6f}")
    print(f"Initial temperature: {result.initial_temperature:.6f}")
    print(f"Temperature levels: {result.temperature_levels}")
    print(f"Proposals: {result.proposals}; accepted: {result.accepted}")
