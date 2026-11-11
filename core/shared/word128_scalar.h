#pragma once
#include <cstdint>
#include <cassert>

namespace statebit {

struct Mask128 {
    uint64_t lo=0, hi=0;
};
inline Mask128 operator|(const Mask128 a,const Mask128 b){ return {a.lo|b.lo,a.hi|b.hi}; }
inline Mask128 operator&(const Mask128 a,const Mask128 b){ return {a.lo&b.lo,a.hi&b.hi}; }
inline Mask128 operator^(const Mask128 a,const Mask128 b){ return {a.lo^b.lo,a.hi^b.hi}; }
inline Mask128 operator~(const Mask128 a){ return {~a.lo,~a.hi}; }
inline Mask128& operator|=(Mask128& a,const Mask128 b){ a.lo|=b.lo; a.hi|=b.hi; return a; }
inline Mask128& operator&=(Mask128& a,const Mask128 b){ a.lo&=b.lo; a.hi&=b.hi; return a; }
inline bool operator==(const Mask128 a,const Mask128 b){ return a.lo==b.lo && a.hi==b.hi; }
inline bool operator!=(const Mask128 a,const Mask128 b){ return !(a==b); }
inline bool mask128Any(const Mask128 a){ return (a.lo|a.hi)!=0; }
inline bool mask128None(const Mask128 a){ return !mask128Any(a); }
inline int mask128Pop(const Mask128 a){ return __builtin_popcountll(a.lo)+__builtin_popcountll(a.hi); }
inline Mask128 mask128Bit(const int i){ assert(i>=0 && i<128); return i<64?Mask128{1ULL<<i,0}:Mask128{0,1ULL<<(i-64)}; }
inline bool mask128Test(const Mask128 a,const int i){ assert(i>=0 && i<128); return i<64?((a.lo>>i)&1ULL):((a.hi>>(i-64))&1ULL); }
inline Mask128 mask128ClearBit(Mask128 a,const int i){ const Mask128 b=mask128Bit(i); a.lo&=~b.lo; a.hi&=~b.hi; return a; }
inline int mask128First(const Mask128 a){ assert(mask128Any(a)); return a.lo?__builtin_ctzll(a.lo):(64+__builtin_ctzll(a.hi)); }
inline Mask128 mask128LowBits(const int n){
    assert(n>=0 && n<=128);
    if(n<=0) return {};
    if(n<64) return {(1ULL<<n)-1ULL,0};
    if(n==64) return {~0ULL,0};
    if(n<128) return {~0ULL,(1ULL<<(n-64))-1ULL};
    return {~0ULL,~0ULL};
}
inline Mask128 mask128AndNot(const Mask128 a,const Mask128 b){ return {a.lo&~b.lo,a.hi&~b.hi}; }
inline Mask128 mask128Without(const Mask128 a,const int i){ return mask128ClearBit(a,i); }

struct State128Stats {
    unsigned long long eligible_nodes=0,handoffs=0,builds=0;
    unsigned long long entry_65_96=0,entry_97_128=0;
    unsigned long long nodes=0,transitions64=0,whole_calls=0,leaf_calls=0,outputs=0;
    unsigned long long cand_filter_calls=0,cand_filter_removed=0;
    unsigned long long ext_rows=0,left_rows=0,forced_ext_checks=0,forced_left_edges=0;
    unsigned long long active_sum=0,forced_sum=0,cpu_ns=0;
    // State128 AVX2 2x128 packet diagnostics. These remain zero in the scalar Word128 path.
    unsigned long long simd_packets=0,simd_slots=0,simd_occ1=0,simd_occ2=0;
    unsigned long long simd_generated_children=0,simd_mask_vector_ops=0;
    unsigned long long simd_pivot_packets=0,simd_refine_packets=0;
    // Hybrid128 diagnostics. State128 packets are legal only with two ready states.
    // A single ready state is expanded by the scalar Word128 node kernel and may
    // generate two children that can be paired on a later refill.
    unsigned long long hybrid_single_ready_events=0,hybrid_scalar_branch_nodes=0;
    unsigned long long hybrid_scalar_generated_children=0,hybrid_paired_ready_states=0;
    inline void observeEligible(const int active){ ++eligible_nodes; (active<=96?entry_65_96:entry_97_128) += 0; }
    inline void observeHandoff(const int active,const int forced){
        ++handoffs; active_sum+=static_cast<unsigned long long>(active); forced_sum+=static_cast<unsigned long long>(forced);
        if(active<=96) ++entry_65_96; else ++entry_97_128;
    }
    inline void merge(const State128Stats& o){
        eligible_nodes+=o.eligible_nodes; handoffs+=o.handoffs; builds+=o.builds;
        entry_65_96+=o.entry_65_96; entry_97_128+=o.entry_97_128;
        nodes+=o.nodes; transitions64+=o.transitions64; whole_calls+=o.whole_calls; leaf_calls+=o.leaf_calls; outputs+=o.outputs;
        cand_filter_calls+=o.cand_filter_calls; cand_filter_removed+=o.cand_filter_removed;
        ext_rows+=o.ext_rows; left_rows+=o.left_rows; forced_ext_checks+=o.forced_ext_checks; forced_left_edges+=o.forced_left_edges;
        active_sum+=o.active_sum; forced_sum+=o.forced_sum; cpu_ns+=o.cpu_ns;
        simd_packets+=o.simd_packets; simd_slots+=o.simd_slots; simd_occ1+=o.simd_occ1; simd_occ2+=o.simd_occ2;
        simd_generated_children+=o.simd_generated_children; simd_mask_vector_ops+=o.simd_mask_vector_ops;
        simd_pivot_packets+=o.simd_pivot_packets; simd_refine_packets+=o.simd_refine_packets;
        hybrid_single_ready_events+=o.hybrid_single_ready_events; hybrid_scalar_branch_nodes+=o.hybrid_scalar_branch_nodes;
        hybrid_scalar_generated_children+=o.hybrid_scalar_generated_children; hybrid_paired_ready_states+=o.hybrid_paired_ready_states;
    }
};

inline bool forcedBaseState128Eligible(const int active,const int threshold){
    return active>64 && threshold>64 && threshold<=128 && active<=threshold;
}

} // namespace statebit
