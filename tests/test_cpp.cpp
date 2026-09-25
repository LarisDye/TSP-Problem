#include "../src/tsp.hpp"
#include <iostream>

static void require(bool ok,const std::string& message) {
    if(!ok) throw std::runtime_error(message);
}
int main() {
    try {
        const auto p=tsp::read_instance("data/att48.tsp");
        require(p.points.size()==48 && p.metric=="ATT","Official input metadata");
        require(p.distance[0][1]==1495,"ATT distance 1-2");
        std::ifstream reference("data/att48.opt.tour");
        std::string line;
        while(std::getline(reference,line) && tsp::trim(line)!="TOUR_SECTION") {}
        tsp::Route optimal;
        int id;
        while(reference>>id && id!=-1) optimal.push_back(id-1);
        auto sorted=optimal; std::sort(sorted.begin(),sorted.end());
        require(sorted.size()==48,"Reference tour size");
        for(int i=0;i<48;++i) require(sorted[i]==i,"Reference tour permutation");
        require(tsp::length(optimal,p.distance)==10628,"Official optimum ATT cost");
        tsp::Random rng(9);
        for(int trial=0;trial<10;++trial) {
            auto route=sorted; rng.shuffle(route);
            const auto before=tsp::length(route,p.distance);
            for(int i=1;i<47;++i) for(int j=i+1;j<48;++j) {
                auto candidate=route;
                std::reverse(candidate.begin()+i,candidate.begin()+j+1);
                require(tsp::delta(route,p.distance,i,j)==tsp::length(candidate,p.distance)-before,"2-opt including return edge");
            }
        }
        tsp::Config config; config.steps=100; config.max_levels=10;
        const auto base=tsp::solve(p,config);
        std::int64_t frames=0, rejected=0;
        const auto observed=tsp::solve(p,config,[&](const tsp::Snapshot& s) {
            require(s.iteration==frames++,"One callback per proposal plus initialization");
            require(tsp::length(s.route,p.distance)==s.current,"Every current path has correct cost");
            require(tsp::length(s.best_route,p.distance)==s.best,"Every best path has correct cost");
            if(!s.accepted && s.iteration) ++rejected;
            return true;
        });
        require(frames==base.proposals+1 && rejected>0,"Rejected proposals are rendered too");
        require(base.route==observed.route && base.accepted==observed.accepted,"Observer cannot perturb random stream");
        frames=0;
        const auto closed=tsp::solve(p,config,[&](const tsp::Snapshot& s) { ++frames; return s.iteration<17; });
        require(frames==18 && closed.proposals==1000 && closed.route==base.route,"Disable view and finish identical search");
        tsp::Instance tiny; tiny.points={{0,0},{3,0},{3,4},{0,4}};
        tiny.distance={{0,3,5,4},{3,0,4,5},{5,4,0,3},{4,5,3,0}};
        require(tsp::solve(tiny,config).best==14,"Four-city exact optimum");
        tiny.distance.assign(4,std::vector<tsp::Cost>(4,0));
        require(tsp::solve(tiny,config).best==0,"Zero-distance temperature fallback");
        config.cooling=1.0;
        bool failed=false;
        try { tsp::solve(p,config); } catch(const std::exception&) { failed=true; }
        require(failed,"Invalid cooling rejected");
        std::cout << "PASS: official optimum, 10810 deltas, callback frames, close/resume, reproducibility, exact small case, zero distances, invalid parameters\n";
        return 0;
    } catch(const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
