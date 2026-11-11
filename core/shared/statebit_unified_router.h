#pragma once
// Unified *non-folding* core route. The search algorithms are invoked by
// existing adapters; context-specific ForcedBase, Local-F and thread scheduler
// remain exclusively owned by each adapter. Never create an otherwise illegal
// width entry. Empty-ForcedBase 256 entry is granted explicitly by the shared host.
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include "statebit_core_bridge.h"
#include "statebit_wide_admission.h"
namespace statebit_unified {
enum class Route : unsigned char { State32=0, Adaptive64=1, State128=2, State256=3, List=4 };
struct Shape { int active, candidates, k; };
// Route is an *execution choice*, not a permission to bypass the caller's
// fixed-context and width-specific invariant checks.
struct Legal { bool bit64, state128, state256; };
inline Route widthOf(int n) {
    assert(n>=0);
    const auto w=statebit_core::capacity(n);
    return static_cast<Route>(static_cast<unsigned char>(w));
}
inline bool eligible(const Shape& s,int requested) {
    assert(s.k>=1 && s.active>=0 && s.candidates>=0 && s.candidates<=s.active);
    return (requested==128 && s.active>64 && s.active<=128) ||
           (requested==256 && s.active>128 && s.active<=256);
}
// Unified non-folding route plan. The CALLER owns the legality flags;
// false routes fall back to List, never to an unsupported wide kernel.
inline Route plan(const Shape& s, const Legal& l) {
    assert(s.k>=1 && s.active>=0 && s.candidates>=0 && s.candidates<=s.active);
    if(s.active<=32) return l.bit64 ? Route::State32 : Route::List;
    if(s.active<=64) return l.bit64 ? Route::Adaptive64 : Route::List;
    if(s.active<=128) return l.state128 ? Route::State128 : Route::List;
    if(s.active<=256) return l.state256 ? Route::State256 : Route::List;
    return Route::List;
}
struct RouteCounters {
    std::atomic<std::uint64_t> bit32{0},bit64{0},requested128{0},requested256{0},accepted128{0},accepted256{0},declined128{0},declined256{0};
};
inline RouteCounters& counters(){static RouteCounters c;return c;}
inline void observeBit(int live){
    (void)live;
}
inline bool is64(int live){
    assert(live>0 && live<=64);
    observeBit(live);
    return widthOf(live)==Route::Adaptive64;
}
// Called only AFTER original width/fixed-context eligibility. This function
// cannot grant a backend that the original adapter forbids.
inline bool admitWide(int active,int candidates,int k,int requested){
    const Shape s{active,candidates,k};
    assert(eligible(s,requested));
    const bool allowed=statebit_wide_admit::decide(active,candidates,k,requested);
    return allowed;
}
// Single shared width dispatcher for ordinary (non-folding) list states.
// The caller must have already completed its original NetCost / fixed-base
// gates, and supply only legal backend choices.  Returns false when its
// existing List DFS must run. Does not change any backend's DFS semantics.
template<class RunBit, class Run128, class Run256>
inline bool executeOrdinary(const Shape& s, const Legal& legal,
                           RunBit&& bit, Run128&& wide128, Run256&& wide256) {
    const Route r=plan(s,legal);
    if(r==Route::State32 || r==Route::Adaptive64){
        bit(); return true;
    }
    if(r==Route::State128 && admitWide(s.active,s.candidates,s.k,128)){
        wide128(); return true;
    }
    if(r==Route::State256 && admitWide(s.active,s.candidates,s.k,256)){
        wide256(); return true;
    }
    return false;
}
inline const char* name(Route r) {
    switch(r){case Route::State32:return "State32";case Route::Adaptive64:return "Adaptive64";
      case Route::State128:return "State128";case Route::State256:return "State256";default:return "List";}
}
inline unsigned long long read(const std::atomic<std::uint64_t>& x){return static_cast<unsigned long long>(x.load(std::memory_order_relaxed));}
inline void report(const char* adapter){
    auto &c=counters();
    std::printf("UNIFIED_ROUTER_CONFIG adapter=%s enabled=%d stats=%d policy=common_nonfold_admission fixed_context_unchanged=1 folding_unchanged=1\n",adapter,STATEBIT_UNIFIED_CORE,STATEBIT_UNIFIED_STATS);
    std::printf("UNIFIED_ROUTER_STATS bit32=%llu bit64=%llu requested128=%llu accepted128=%llu declined128=%llu requested256=%llu accepted256=%llu declined256=%llu\n",read(c.bit32),read(c.bit64),read(c.requested128),read(c.accepted128),read(c.declined128),read(c.requested256),read(c.accepted256),read(c.declined256));
}
} // namespace statebit_unified
