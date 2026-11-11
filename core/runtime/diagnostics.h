#pragma once
#include <algorithm>
#include <cstdint>
#include <chrono>
namespace statebit {
struct BestDiagStats {
    std::uint64_t seed_count=0,valid_seed_count=0;
    std::uint64_t decompose_ns=0,workspace_ns=0,subgraph_ns=0,search_ns=0,seed_ns_max=0;
    std::uint64_t fold_calls=0,fold_ns=0;
    std::uint64_t state128_calls=0,state128_ns=0,state128_build_calls=0,state128_build_ns=0;
    std::uint64_t bit64_build_calls=0,bit64_build_ns=0;
    inline void merge(const BestDiagStats&o){
        seed_count+=o.seed_count;valid_seed_count+=o.valid_seed_count;decompose_ns+=o.decompose_ns;workspace_ns+=o.workspace_ns;subgraph_ns+=o.subgraph_ns;search_ns+=o.search_ns;seed_ns_max=std::max(seed_ns_max,o.seed_ns_max);
        fold_calls+=o.fold_calls;fold_ns+=o.fold_ns;state128_calls+=o.state128_calls;state128_ns+=o.state128_ns;state128_build_calls+=o.state128_build_calls;state128_build_ns+=o.state128_build_ns;bit64_build_calls+=o.bit64_build_calls;bit64_build_ns+=o.bit64_build_ns;
    }
};
extern thread_local BestDiagStats bestDiagStats;
inline std::uint64_t diagNs(std::chrono::steady_clock::time_point a,std::chrono::steady_clock::time_point b){
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(b-a).count());
}
}
