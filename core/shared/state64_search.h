#pragma once
#include <cstdint>
#include <vector>
#include <utility>
#include <algorithm>
#include <immintrin.h>
// WORD64 closure / STATE64: a single non-Folding search-state control module.
// Adapter callbacks preserve Folding context and PARALLEL_ENGINE's parallel scheduler.
namespace statebit_state64_search {
enum class Terminal : uint8_t { Prune, Whole, Leaf, Branch };
inline bool meets_bound(const int selected,const int candidates,const int lower){
    return selected+candidates>=lower;
}
inline Terminal classify(const int selected,const int candidates,const int lower,const bool whole){
    if(!meets_bound(selected,candidates,lower))return Terminal::Prune;
    if(whole)return Terminal::Whole;
    if(!candidates)return Terminal::Leaf;
    return Terminal::Branch;
}
// Scalar pretest of the common k-plex 'whole P union C' condition.
// This is used by SEQUENTIAL_ENGINE's ordinary Word64 expand and as a non-Folding
// correctness oracle; PARALLEL_ENGINE/State64 may use the shared cached witness fast path.
template<class BitNon,class Pop>
inline bool whole(const uint64_t U,const int k,const BitNon& non,Pop pop){
    for(uint64_t t=U;t;t&=t-1){
        const unsigned v=static_cast<unsigned>(__builtin_ctzll(t));
        if(pop(non[v]&U)>=k)return false;
    }
    return true;
}
struct PacketBits {
    alignas(32) uint64_t P[4]={0,0,0,0};
    alignas(32) uint64_t C[4]={0,0,0,0};
    alignas(32) uint64_t X[4]={0,0,0,0};
    alignas(32) uint64_t bit[4]={0,0,0,0};
    alignas(32) uint64_t pi[4]={0,0,0,0};
    alignas(32) uint64_t cm[4]={0,0,0,0};
    alignas(32) uint64_t xe[4]={0,0,0,0};
};
// One AVX2 representation of the 4-lane Include/Exclude word operations.
template<class State>
inline PacketBits materialize(const State* ready,const int active,const uint8_t mask,const int* pivot){
    PacketBits out;
    for(int i=0;i<active;i++){
        out.P[i]=ready[i].P;out.C[i]=ready[i].C;out.X[i]=ready[i].X;
        if(mask&(1u<<i))out.bit[i]=uint64_t(1)<<pivot[i];
    }
    const __m256i pv=_mm256_load_si256(reinterpret_cast<const __m256i*>(out.P));
    const __m256i cv=_mm256_load_si256(reinterpret_cast<const __m256i*>(out.C));
    const __m256i xv=_mm256_load_si256(reinterpret_cast<const __m256i*>(out.X));
    const __m256i bv=_mm256_load_si256(reinterpret_cast<const __m256i*>(out.bit));
    _mm256_store_si256(reinterpret_cast<__m256i*>(out.pi),_mm256_or_si256(pv,bv));
    _mm256_store_si256(reinterpret_cast<__m256i*>(out.cm),_mm256_andnot_si256(bv,cv));
    _mm256_store_si256(reinterpret_cast<__m256i*>(out.xe),_mm256_or_si256(xv,bv));
    return out;
}
// One common lane iteration, include-before-exclude stack publication order.
// Framework callbacks only construct the context-specific child and update
// counters; the candidate branch order is a shared algorithmic rule.
template<class Stack,class CanInclude,class Emit>
inline void publish_children(const int active,const uint8_t mask,Stack& stack,
                             CanInclude&& can_include,Emit&& emit){
    for(int lane=0;lane<active;lane++)if(mask&(1u<<lane)){
        const bool include=can_include(lane);
        emit(lane,false,stack); // defer exclude behind include (LIFO)
        if(include)emit(lane,true,stack);
    }
}
// A genuinely shared 4-lane DFS refill scheduler; includes stack lifetime,
// LIFO search order, packet collection and final terminal-batch flush.
// pop_prepare(state,ready,terminal_sink) returns true only for branch-ready
// State64 nodes. Terminal handling/State32 handoff remain correct by callbacks.
template<class State,class Stack,class Prepare,class Branch,class Spill,class Finish>
inline void refill(Stack& stack,Prepare&& prepare,Branch&& branch,Spill&& spill,Finish&& finish){
    while(!stack.empty()){
        spill(stack);
        State ready[4];int active=0;
        while(active<4 && !stack.empty()){
            State st=std::move(stack.back());stack.pop_back();
            State prepared{};
            if(prepare(st,prepared))ready[active++]=std::move(prepared);
        }
        if(active)branch(ready,active,stack);
        spill(stack);
    }
    finish();
}
} // namespace statebit_state64
