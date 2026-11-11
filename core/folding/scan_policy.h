#pragma once
#include <cstdint>


namespace statebit {

// Seed-local default-ON gate for FOLDING_GATE = ForcedBase/C0 folding + StateBit.
// FOLDING_GATE starts enabled so early folding/bit opportunities are never missed.
// We turn it off only when the first N PROBE checks are all direct-terminal hits
// and no DYNAMIC_STATE32 handoff has occurred.  OFF_REQUESTED is a safety-drain state:
// no new FOLDING_GATE optimization may start, but already-folded ForcedBase state must
// unwind before OFF can be committed.
struct SeedFoldingEarlyOffGate {
    enum State : int { PROBING=0, OFF_REQUESTED=1, OFF=2, KEEP_ON=3 };
    unsigned long long checks=0;
    unsigned long long hits=0;
    unsigned long long structural_misses=0;
    unsigned long long size_misses=0;
    unsigned long long dynamic_state32_handoffs_before_decision=0;
    State state=PROBING;

    inline bool decisionFinal() const { return state!=PROBING; }
    inline bool offRequested() const { return state==OFF_REQUESTED; }
    inline bool optimizationEnabled() const { return state==PROBING || state==KEEP_ON; }

    inline void observeHit(const int /*uSize*/,const int /*cSize*/){
        if(state!=PROBING) return;
        ++checks; ++hits;
        if(checks>=static_cast<unsigned long long>(FOLDING_GATE_EARLYOFF_CHECKPOINT)){
            if(hits==checks && structural_misses==0 && size_misses==0 && dynamic_state32_handoffs_before_decision==0)
                state=OFF_REQUESTED;
            else
                state=KEEP_ON;
        }
    }
    inline void observeStructuralMiss(const int /*uSize*/,const int /*cSize*/){
        if(state!=PROBING) return;
        ++checks; ++structural_misses;
        state=KEEP_ON; // "all first N are hits" can no longer become true.
    }
    inline void observeSizeMiss(const int /*uSize*/,const int /*cSize*/){
        if(state!=PROBING) return;
        ++checks; ++size_misses;
        state=KEEP_ON;
    }
    inline void observeT4Handoff(){
        if(state==OFF) return;
        if(state==PROBING || state==OFF_REQUESTED){
            ++dynamic_state32_handoffs_before_decision;
            state=KEEP_ON;
        }
    }
    inline bool tryCommitOff(const bool noForcedState){
        if(state==OFF_REQUESTED && noForcedState){ state=OFF; return true; }
        return false;
    }
};

struct FOLDING_GATEEarlyOffAggregate {
    unsigned long long seeds=0,off=0,keep_on=0,probing=0,off_requested=0;
    unsigned long long checks=0,hits=0,structural_misses=0,size_misses=0,dynamic_state32_handoffs_before_decision=0;
    inline void observe(const SeedFoldingEarlyOffGate& g){
        ++seeds; checks+=g.checks; hits+=g.hits; structural_misses+=g.structural_misses;
        size_misses+=g.size_misses; dynamic_state32_handoffs_before_decision+=g.dynamic_state32_handoffs_before_decision;
        if(g.state==SeedFoldingEarlyOffGate::OFF) ++off;
        else if(g.state==SeedFoldingEarlyOffGate::KEEP_ON) ++keep_on;
        else if(g.state==SeedFoldingEarlyOffGate::OFF_REQUESTED) ++off_requested;
        else ++probing;
    }
    inline void merge(const FOLDING_GATEEarlyOffAggregate& o){
        seeds+=o.seeds; off+=o.off; keep_on+=o.keep_on; probing+=o.probing; off_requested+=o.off_requested;
        checks+=o.checks; hits+=o.hits; structural_misses+=o.structural_misses; size_misses+=o.size_misses;
        dynamic_state32_handoffs_before_decision+=o.dynamic_state32_handoffs_before_decision;
    }
};

} // namespace statebit
