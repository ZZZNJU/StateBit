#pragma once
#include <algorithm>
namespace statebit {
struct PreUpdateKPlexStats {
    unsigned long long checks=0,hit_nodes=0,miss_nodes=0,miss_size_nodes=0,miss_p_nodes=0,miss_c_nodes=0;
    unsigned long long scan_p_vertices=0,scan_c_vertices=0,hit_u_sum=0,hit_c_sum=0,max_scan_p=0,max_scan_c=0;
    unsigned long long direct_terminal_hits=0,avoided_candidates=0;
    inline void noteScans(unsigned long long p,unsigned long long c){
        scan_p_vertices+=p; scan_c_vertices+=c; max_scan_p=std::max(max_scan_p,p); max_scan_c=std::max(max_scan_c,c);
    }
    inline void missSize(){ ++checks; ++miss_nodes; ++miss_size_nodes; }
    inline void missP(unsigned long long p,unsigned long long c){ ++checks; ++miss_nodes; ++miss_p_nodes; noteScans(p,c); }
    inline void missC(unsigned long long p,unsigned long long c){ ++checks; ++miss_nodes; ++miss_c_nodes; noteScans(p,c); }
    inline void hit(unsigned long long p,unsigned long long c,int usize,int csz){
        ++checks; ++hit_nodes; noteScans(p,c); hit_u_sum+=static_cast<unsigned long long>(std::max(usize,0)); hit_c_sum+=static_cast<unsigned long long>(std::max(csz,0));
    }
    inline void observeDirectTerminal(int csz){ ++direct_terminal_hits; avoided_candidates+=static_cast<unsigned long long>(std::max(csz,0)); }
    inline void merge(const PreUpdateKPlexStats&o){
        checks+=o.checks; hit_nodes+=o.hit_nodes; miss_nodes+=o.miss_nodes; miss_size_nodes+=o.miss_size_nodes; miss_p_nodes+=o.miss_p_nodes; miss_c_nodes+=o.miss_c_nodes;
        scan_p_vertices+=o.scan_p_vertices; scan_c_vertices+=o.scan_c_vertices; hit_u_sum+=o.hit_u_sum; hit_c_sum+=o.hit_c_sum;
        max_scan_p=std::max(max_scan_p,o.max_scan_p); max_scan_c=std::max(max_scan_c,o.max_scan_c); direct_terminal_hits+=o.direct_terminal_hits; avoided_candidates+=o.avoided_candidates;
    }
};
extern thread_local PreUpdateKPlexStats preUpdateKPlexStats;
}
