#pragma once
#include <cstdint>
#include <utility>
#include "state64_search.h"
// WORD64/STATE64: executable non-Folding node state machine. Framework callbacks
// are restricted to witness telemetry, semantic fixed-vertex handling,
// State32 handoff (STATE32_HANDOFF) and maximality output. The order of pruning,
// whole/leaf terminal decisions and ready-state publication is *shared*.
namespace statebit_word64_search2 {
// `State` is the framework's state carrier. Both have P/C/X/F, witness,
// and the eager-check metadata. The carrier is copied from the parent so
// these attributes cannot accidentally diverge in separate child builders.
template<class State,class Affected>
inline State make_child(const State& parent,const uint64_t P,const uint64_t C,
                        const uint64_t X,const uint64_t removed,Affected&& affected){
    State child=parent;
    child.P=P;child.C=C;child.X=X;
    child.rootNode=false;
    if(child.eagerInitialized)child.checkMask=affected(removed,C);
    return child;
}

template<class State,class Pop,class Witness,class WholeTerminal,class LeafTerminal,
         class Handoff,class Ready,class Prune>
inline bool prepare_node(State& st,State& prepared,const int activeLb,Pop&& pop,
                  Witness&& witness,WholeTerminal&& wholeTerminal,
                  LeafTerminal&& leafTerminal,Handoff&& handoff,
                  Ready&& onReady,Prune&& onPrune){
    const int ps=pop(st.P),cs=pop(st.C);
    if(!statebit_state64_search::meets_bound(ps,cs,activeLb)){
        onPrune(st);return false;
    }
    // Witness evaluation is shared in state64_common::whole_witness;
    // the callback only instruments/updates the framework's hint.
    const bool isWhole=witness(st,st.P|st.C);
    const auto decision=statebit_state64_search::classify(ps,cs,activeLb,isWhole);
    if(decision==statebit_state64_search::Terminal::Whole){
        wholeTerminal(st,st.P|st.C);return false;
    }
    if(decision==statebit_state64_search::Terminal::Leaf){
        leafTerminal(st,ps>=activeLb);return false;
    }
    // Handoff is intentionally AFTER terminal classification for PARALLEL_ENGINE;
    // SEQUENTIAL_ENGINE retains its earlier <=32 handoff as an explicit adapter hook.
    if(handoff(st))return false;
    onReady(st,prepared);
    return true;
}

// The same non-Folding maximality traversal, for Word64 and State64
// terminal emitters. Callbacks retain only adjacency/fixed-vertex context.
template<class Internal,class External,class Left,class Emit>
inline bool check_maximal(const uint64_t X,const int extN,const int leftN,
         Internal&& internal,External&& external,Left&& left,Emit&& emit){
    for(uint64_t t=X;t;t&=t-1){
        const int x=static_cast<int>(__builtin_ctzll(t));
        if(internal(x))return false;
    }
    for(int e=0;e<extN;e++)if(external(e))return false;
    for(int l=0;l<leftN;l++)if(left(l))return false;
    emit();return true;
}

template<class Pop,class Non,class EmitWhole,class EmitLeaf>
inline statebit_state64_search::Terminal plain_word_node(const uint64_t P,const uint64_t C,
        const int lb,const int k,const Non& non,Pop&& pop,
        EmitWhole&& emitWhole,EmitLeaf&& emitLeaf){
    const int ps=pop(P),cs=pop(C);
    if(!statebit_state64_search::meets_bound(ps,cs,lb))return statebit_state64_search::Terminal::Prune;
    const bool whole=statebit_state64_search::whole(P|C,k,non,pop);
    const auto d=statebit_state64_search::classify(ps,cs,lb,whole);
    if(d==statebit_state64_search::Terminal::Whole)emitWhole(P|C);
    else if(d==statebit_state64_search::Terminal::Leaf)emitLeaf(P,ps>=lb);
    return d;
}
} // namespace statebit_word64_search2
