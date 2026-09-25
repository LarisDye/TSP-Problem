#include "tsp.hpp"
#include "visualizer.hpp"
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>

namespace fs = std::filesystem;
static long long integer(const std::string& s, long long low, long long high) {
    std::size_t end=0;
    const auto x=std::stoll(s,&end);
    if(end!=s.size() || x<low || x>high) throw std::runtime_error("Integer argument out of range: "+s);
    return x;
}
static double real(const std::string& s) {
    std::size_t end=0;
    const double x=std::stod(s,&end);
    if(end!=s.size() || !std::isfinite(x)) throw std::runtime_error("Invalid numeric argument: "+s);
    return x;
}
static std::string quoted(const std::string& s) {
    std::ostringstream out;
    out << '"';
    for(unsigned char ch:s) {
        if(ch=='"' || ch=='\\') out << '\\' << ch;
        else if(ch<32) out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << int(ch) << std::dec;
        else out << ch;
    }
    out << '"'; return out.str();
}
static std::ofstream output(const fs::path& path) {
    std::ofstream file(path);
    if(!file) throw std::runtime_error("Cannot write: "+path.string());
    file.exceptions(std::ios::badbit|std::ios::failbit);
    file << std::setprecision(17);
    return file;
}
static void route_json(std::ostream& out,const tsp::Route& route) {
    out << '[';
    for(std::size_t i=0;i<route.size();++i) { if(i) out << ','; out << route[i]+1; }
    out << ']';
}
static void save(const fs::path& dir,const tsp::Instance& p,const tsp::Config& c,
                 const tsp::Result& r,double seconds) {
    fs::create_directories(dir);
    auto json=output(dir/"result.json");
    json << "{\n  \"instance\": " << quoted(p.name) << ",\n  \"metric\": " << quoted(p.metric)
         << ",\n  \"seed\": " << c.seed << ",\n  \"cooling\": " << c.cooling
         << ",\n  \"min_ratio\": " << c.min_ratio << ",\n  \"steps_per_level\": " << (c.steps ? c.steps : 100*p.points.size())
         << ",\n  \"max_levels\": " << c.max_levels << ",\n  \"initial_length\": " << r.initial
         << ",\n  \"best_length\": " << r.best << ",\n  \"initial_temperature\": " << r.initial_temperature
         << ",\n  \"levels\": " << r.levels << ",\n  \"proposals\": " << r.proposals
         << ",\n  \"accepted\": " << r.accepted << ",\n  \"best_iteration\": " << r.best_iteration
         << ",\n  \"elapsed_seconds\": " << seconds << ",\n  \"route\": ";
    route_json(json,r.route);
    json << ",\n  \"initial_route\": "; route_json(json,r.initial_route);
    json << ",\n  \"coordinates\": [";
    for(std::size_t i=0;i<p.points.size();++i) {
        if(i) json << ',';
        json << '[' << p.points[i].x << ',' << p.points[i].y << ']';
    }
    json << "]\n}\n";
    auto history=output(dir/"history.csv");
    history << "iteration,level,temperature,current,best,accepted\n";
    for(auto h:r.history)
        history << h.iteration << ',' << h.level << ',' << h.temperature << ',' << h.current << ',' << h.best << ',' << h.accepted << '\n';
    auto tour=output(dir/"best.tour");
    tour << "NAME : " << p.name << "_sa\nTYPE : TOUR\nDIMENSION : " << r.route.size() << "\nTOUR_SECTION\n";
    for(auto id:r.route) tour << id+1 << '\n';
    tour << "-1\nEOF\n";
}
int main(int argc,char** argv) {
    try {
        tsp::Config config;
        std::string input="data/att48.tsp", directory="results/latest";
        bool visual=false, selected=false;
        int runs=1, delay=16;
        for(int i=1;i<argc;++i) {
            std::string arg=argv[i];
            if(arg=="--help" || arg=="-h") {
                std::cout << "TSP simulated annealing (C++17)\n"
                    "  --input FILE        TSPLIB ATT/EUC_2D input (data/att48.tsp)\n"
                    "  --visual            Render every proposal in a Windows window\n"
                    "  --no-visual         Run without a window\n"
                    "  --frame-ms N        Delay after each rendered proposal (16)\n"
                    "  --seed N            First random seed (42)\n"
                    "  --runs N            Independent runs with consecutive seeds (1)\n"
                    "  --cooling X         Geometric cooling factor (0.98)\n"
                    "  --min-ratio X       Final/initial temperature ratio (0.0001)\n"
                    "  --steps N           Proposals per temperature (100 * cities)\n"
                    "  --max-levels N      Temperature level limit (2000)\n"
                    "  --output DIR        Results directory (results/latest)\n"
                    "Window: Space pause; N single step; +/- delay; Esc/V/X disable view.\n";
                return 0;
            }
            if(arg=="--visual" || arg=="--no-visual") { visual=arg=="--visual"; selected=true; continue; }
            if(i+1>=argc) throw std::runtime_error("Missing value for "+arg);
            std::string value=argv[++i];
            if(arg=="--input") input=value;
            else if(arg=="--output") directory=value;
            else if(arg=="--seed") config.seed=static_cast<std::uint32_t>(integer(value,0,4294967295LL));
            else if(arg=="--runs") runs=static_cast<int>(integer(value,1,10000));
            else if(arg=="--frame-ms") delay=static_cast<int>(integer(value,0,1000));
            else if(arg=="--steps") config.steps=static_cast<int>(integer(value,1,100000000));
            else if(arg=="--max-levels") config.max_levels=static_cast<int>(integer(value,1,10000000));
            else if(arg=="--cooling") config.cooling=real(value);
            else if(arg=="--min-ratio") config.min_ratio=real(value);
            else throw std::runtime_error("Unknown argument: "+arg);
        }
        if(!(config.cooling>0 && config.cooling<1) || !(config.min_ratio>0 && config.min_ratio<1))
            throw std::runtime_error("--cooling and --min-ratio must be strictly between 0 and 1");
        if(std::uint64_t(config.seed)+runs-1>4294967295ULL) throw std::runtime_error("Seed range overflows uint32");
        if(!selected) {
            std::cout << "Visualize every proposal? [y/N]: " << std::flush;
            std::string answer;
            std::getline(std::cin,answer);
            visual=answer=="y" || answer=="Y" || answer=="yes";
        }
        if(visual && runs!=1) throw std::runtime_error("Use --runs 1 for visualization; batch runs use --no-visual");
        const auto p=tsp::read_instance(input);
        fs::create_directories(directory);
        auto summary=output(fs::path(directory)/"runs.csv");
        summary << "run,seed,initial,best,levels,proposals,accepted,best_iteration,seconds\n";
        tsp::Result winner;
        tsp::Config winner_config;
        double winner_seconds=0;
        std::unique_ptr<Visualizer> viewer;
        if(visual) viewer=std::make_unique<Visualizer>(p,delay);
        for(int run=0;run<runs;++run) {
            auto c=config; c.seed+=run;
            const auto start=std::chrono::steady_clock::now();
            const auto result=tsp::solve(p,c,viewer ? tsp::Observer([&](const tsp::Snapshot& s) { return viewer->update(s); }) : tsp::Observer{});
            const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
            save(fs::path(directory)/("seed_"+std::to_string(c.seed)),p,c,result,seconds);
            summary << run+1 << ',' << c.seed << ',' << result.initial << ',' << result.best << ',' << result.levels
                    << ',' << result.proposals << ',' << result.accepted << ',' << result.best_iteration << ',' << seconds << '\n';
            std::cout << "Seed " << c.seed << ": " << result.initial << " -> " << result.best
                      << "; " << result.proposals << " proposals; " << seconds << " s\n" << std::flush;
            if(run==0 || result.best<winner.best) { winner=result; winner_config=c; winner_seconds=seconds; }
        }
        save(directory,p,winner_config,winner,winner_seconds);
        std::cout << "Best route (TSPLIB IDs): ";
        for(int id:winner.route) std::cout << id+1 << " -> ";
        std::cout << winner.route.front()+1 << "\nBest length: " << winner.best
                  << "\nSaved results to " << directory << '\n' << std::flush;
        summary.close();
        // Results already saved; keeping the final tour open does not affect timing.
        if(viewer) viewer->show_result(winner);
        return 0;
    } catch(const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n'; return 1;
    }
}
