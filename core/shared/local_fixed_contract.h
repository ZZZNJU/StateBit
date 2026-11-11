#pragma once
#include <cstdint>
// LOCAL_FIXED: common *logical* contract. F is committed but not a State32 coordinate.
// P,C,X are masks in the current Word64 domain; F must be disjoint from them.
namespace local_fixed_localf_contract {
inline int active_lower_bound(const int active_lb_without_local_f, const uint64_t F) {
    const int n=active_lb_without_local_f-__builtin_popcountll(F);
    return n>0?n:0;
}
inline bool valid(const uint64_t P,const uint64_t C,const uint64_t X,const uint64_t F) {
    return !(F&(P|C|X)) && !(P&C) && !(P&X) && !(C&X)
        && __builtin_popcountll(P|C)<=32;
}
inline uint64_t logical_solution(const uint64_t S,const uint64_t F) { return S|F; }
}
