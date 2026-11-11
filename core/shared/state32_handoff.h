#pragma once
// STATE32_HANDOFF: shared ordinary State32 admission and handoff protocol.
// Both both execution modes use the SAME feasibility, Whole, Leaf, and prechecked
// state-machine code. Fixed-vertex maximality/penalties and MT scheduling
// remain explicit host callbacks; they are not algorithmic bypasses.
#include <cassert>
#include <cstdint>
#include <atomic>
#include <cstdio>
namespace statebit_state32_handoff {
struct Counters {
    std::atomic<unsigned long long> checked{0},upstream{0},kernel{0},entered{0},rejected{0},whole{0},leaf{0};
};
inline Counters& stats(){static Counters c;return c;}
inline void report(const char* engine){
    const auto& c=stats();
    std::printf("STATE32_HANDOFF_HANDOFF_CONFIG engine=%s enabled=%d stats=%d shared_state32_root=1 shared_handoff=1 folding_adapter_intact=1\n",engine,STATEBIT_STATE32_HANDOFF_SHARED_HANDOFF,STATEBIT_STATE32_HANDOFF_STATS);
    std::printf("STATE32_HANDOFF_STATS checked=%llu upstream=%llu kernel=%llu entered=%llu rejected=%llu whole=%llu leaf=%llu\n",
        c.checked.load(),c.upstream.load(),c.kernel.load(),c.entered.load(),c.rejected.load(),c.whole.load(),c.leaf.load());
}

enum class RootMode : unsigned char {
    Checked,         // evaluate once before creating State32 context
    AlreadyChecked,  // caller already did root checks (State64 handoff)
    KernelChecks     // delegate root checks to state32_core.inc
};
enum class Outcome : unsigned char { Reject, Whole, Leaf, Enter };
struct RootState {
    std::uint64_t P,C,X;
    int lowerBound,k;
    bool rootPrechecked;
};
inline Outcome classify(std::uint64_t P,std::uint64_t C,int lowerBound,int k,
                        const std::uint64_t* nonadj){
    assert(lowerBound>=0 && k>=1);
    const std::uint64_t U=P|C;
    if(static_cast<int>(__builtin_popcountll(U))<lowerBound) return Outcome::Reject;
    for(std::uint64_t t=U;t;t&=t-1){
        const int v=__builtin_ctzll(t);
        if(__builtin_popcountll(nonadj[v]&U)>=k){
            return C ? Outcome::Enter : Outcome::Leaf;
        }
    }
    return Outcome::Whole;
}
template<class OnNode,class OnWhole,class OnLeaf,class OnEnter>
inline bool dispatch(const RootState& s,RootMode mode,const std::uint64_t* nonadj,
                     OnNode&& node,OnWhole&& whole,OnLeaf&& leaf,OnEnter&& enter){
    const unsigned active=__builtin_popcountll(s.P|s.C);
    assert(active<=32 && (s.P&s.C)==0 && s.lowerBound>=0);
    if(mode==RootMode::AlreadyChecked){
        if(s.C) {
            enter(true);return true;}
        // State32 refill assumes C!=0. A prechecked empty-C state is a leaf.
        leaf(s.P);return false;
    }
    if(mode==RootMode::KernelChecks){
        if(!s.C){
            node();leaf(s.P);return false;
        }
        enter(false);return true;
    }
    node();
    switch(classify(s.P,s.C,s.lowerBound,s.k,nonadj)){
        case Outcome::Reject:
            return false;
        case Outcome::Whole:
            whole(s.P|s.C);return false;
        case Outcome::Leaf:
            leaf(s.P);return false;
        case Outcome::Enter:
            enter(true);return true;
    }
    return false;
}
// Shared ordinary transition contract. A context-bearing Local-F handoff is
// an explicitly excluded Folding adapter, not a second plain entry policy.
inline bool validHandoff(std::uint64_t P,std::uint64_t C,std::uint64_t X,
                         std::uint64_t F){
    return __builtin_popcountll(P|C)<=32 && !(P&C) && !(P&X) && !(C&X) &&
           !(F&(P|C|X));
}
} // namespace statebit_state32_handoff
