#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <functional>
#include <limits>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace tsp {
using Cost = std::int64_t;
using Route = std::vector<int>;
using Matrix = std::vector<std::vector<Cost>>;
struct Point { double x, y; };
struct Instance {
    std::string name, metric;
    std::vector<Point> points;
    Matrix distance;
};
inline std::string trim(std::string s) {
    const auto a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return {};
    return s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
}
inline Cost edge(Point a, Point b, const std::string& metric) {
    const double r = std::hypot(a.x - b.x, a.y - b.y) /
                     (metric == "ATT" ? std::sqrt(10.0) : 1.0);
    const Cost t = static_cast<Cost>(std::floor(r + 0.5));
    return metric == "ATT" && t < r ? t + 1 : t;
}
inline Instance read_instance(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Cannot open instance: " + path);
    Instance p;
    int n = 0;
    std::string line, type;
    bool section = false;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line == "NODE_COORD_SECTION") { section = true; break; }
        const auto colon = line.find(':');
        if (colon == std::string::npos) continue;
        auto key = trim(line.substr(0, colon)), value = trim(line.substr(colon + 1));
        if (key == "NAME") p.name = value;
        if (key == "TYPE") type = value;
        if (key == "EDGE_WEIGHT_TYPE") p.metric = value;
        if (key == "DIMENSION") {
            std::istringstream field(value);
            std::string extra;
            if (!(field >> n) || (field >> extra))
                throw std::runtime_error("Invalid DIMENSION");
        }
    }
    if (!section || type != "TSP" || n < 3 || n > 10000 ||
        (p.metric != "ATT" && p.metric != "EUC_2D"))
        throw std::runtime_error("Expected TSP, 3..10000 nodes, ATT or EUC_2D coordinates");
    p.points.resize(n);
    std::vector<bool> seen(n, false);
    int count = 0;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty()) continue;
        if (line == "EOF") break;
        std::istringstream row(line);
        int id;
        Point point{};
        std::string extra;
        if (!(row >> id >> point.x >> point.y) || (row >> extra) ||
            id < 1 || id > n || seen[id - 1] ||
            !std::isfinite(point.x) || !std::isfinite(point.y) ||
            std::abs(point.x) > 1e9 || std::abs(point.y) > 1e9)
            throw std::runtime_error("Invalid or duplicate coordinate row: " + line);
        p.points[id - 1] = point;
        seen[id - 1] = true;
        ++count;
    }
    if (count != n) throw std::runtime_error("Coordinate count does not match DIMENSION");
    p.distance.assign(n, std::vector<Cost>(n));
    for (int i = 0; i < n; ++i)
        for (int j = i + 1; j < n; ++j)
            p.distance[i][j] = p.distance[j][i] = edge(p.points[i], p.points[j], p.metric);
    return p;
}
inline Cost length(const Route& route, const Matrix& d) {
    Cost result = 0;
    for (std::size_t i = 0; i < route.size(); ++i)
        result += d[route[i]][route[(i + 1) % route.size()]];
    return result;
}
inline Cost delta(const Route& r, const Matrix& d, int i, int j) {
    const int a = r[i-1], b = r[i], c = r[j], e = r[(j+1) % r.size()];
    return d[a][c] + d[b][e] - d[a][b] - d[c][e];
}
// Explicit sampling avoids implementation-dependent std::shuffle/distributions.
class Random {
    std::mt19937 engine;
public:
    explicit Random(std::uint32_t seed) : engine(seed) {}
    std::uint32_t bounded(std::uint32_t bound) {
        const auto threshold = static_cast<std::uint32_t>(-bound) % bound;
        std::uint32_t x;
        do { x = engine(); } while (x < threshold);
        return x % bound;
    }
    double uniform() { return static_cast<double>(engine()) / 4294967296.0; }
    std::pair<int,int> pair(int n) {
        int i = 1 + bounded(n-1), j;
        do { j = 1 + bounded(n-1); } while (j == i);
        if (i > j) std::swap(i,j);
        return {i,j};
    }
    void shuffle(Route& r) {
        for (int i = static_cast<int>(r.size()) - 1; i > 1; --i)
            std::swap(r[i], r[1 + bounded(i)]);
    }
};
struct Config {
    std::uint32_t seed = 42;
    double cooling = 0.98, min_ratio = 1e-4;
    int steps = 0, max_levels = 2000;
};
struct Snapshot {
    std::int64_t iteration;
    int level;
    double temperature;
    Cost current, best;
    bool accepted;
    const Route& route;
    const Route& best_route;
};
struct Record {
    std::int64_t iteration;
    int level;
    double temperature;
    Cost current, best;
    std::int64_t accepted;
};
struct Result {
    Route initial_route, route;
    Cost initial = 0, best = 0;
    double initial_temperature = 0;
    int levels = 0;
    std::int64_t proposals = 0, accepted = 0, best_iteration = 0;
    std::vector<Record> history;
};
// Return false from observer to disable rendering; the search then continues.
using Observer = std::function<bool(const Snapshot&)>;
inline Result solve(const Instance& p, Config c, Observer observer = {}) {
    const int n = static_cast<int>(p.points.size());
    if (n < 3 || !(c.cooling > 0 && c.cooling < 1) ||
        !(c.min_ratio > 0 && c.min_ratio < 1) || c.steps < 0 || c.max_levels < 1)
        throw std::runtime_error("Invalid annealing parameters");
    if (c.steps == 0) c.steps = 100 * n;
    Random rng(c.seed);
    Route route(n);
    std::iota(route.begin(), route.end(), 0);
    rng.shuffle(route);
    Result out;
    out.initial_route = out.route = route;
    Cost current = out.initial = out.best = length(route, p.distance);
    double positive = 0;
    int count = 0;
    for (int s = 0; s < 200; ++s) {
        const auto [i,j] = rng.pair(n);
        const Cost change = delta(route, p.distance, i, j);
        if (change > 0) { positive += change; ++count; }
    }
    Cost maximum = 1;
    for (const auto& row : p.distance)
        maximum = std::max(maximum, *std::max_element(row.begin(), row.end()));
    out.initial_temperature = count ? -(positive / count) / std::log(0.8) : maximum;
    out.history.push_back({0, 0, out.initial_temperature, current, out.best, 0});
    if (observer && !observer({0, 0, out.initial_temperature, current, out.best, false, route, out.route}))
        observer = {};
    double ratio = 1.0;
    for (; out.levels < c.max_levels && ratio > c.min_ratio; ratio *= c.cooling) {
        const double temperature = out.initial_temperature * ratio;
        if (!(temperature > 0)) break;
        for (int s = 0; s < c.steps; ++s) {
            const auto [i,j] = rng.pair(n);
            const Cost change = delta(route, p.distance, i, j);
            const bool accept = change <= 0 || rng.uniform() < std::exp(-double(change)/temperature);
            ++out.proposals;
            if (accept) {
                std::reverse(route.begin()+i, route.begin()+j+1);
                current += change;
                ++out.accepted;
                if (current < out.best) {
                    out.best = current;
                    out.route = route;
                    out.best_iteration = out.proposals;
                }
            }
            // Every proposal, including rejected and equal-cost moves, is observed.
            if (observer && !observer({out.proposals, out.levels+1, temperature,
                                      current, out.best, accept, route, out.route}))
                observer = {};
        }
        ++out.levels;
        if (current != length(route, p.distance)) throw std::logic_error("2-opt cost drift");
        out.history.push_back({out.proposals, out.levels, temperature, current, out.best, out.accepted});
    }
    if (out.best != length(out.route, p.distance)) throw std::logic_error("Best cost mismatch");
    return out;
}
} // namespace tsp
