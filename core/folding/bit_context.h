#pragma once
#include <algorithm>

namespace statebit {

inline int forcedBaseActiveLb(const int logicalLb,const int forcedCount){
    return std::max(0, logicalLb-forcedCount);
}
inline int forcedBaseMissingAdj(const int forcedCount,const int forcedAdj){
    return std::max(0, forcedCount-forcedAdj);
}

inline bool forcedBaseBit64Eligible(const int active,const int threshold){
    return active>0 && threshold>0 && threshold<=64 && active<=threshold && active<=64;
}

struct ForcedBaseBit64Stats {
    unsigned long long eligible_nodes=0;
    unsigned long long eligible_le32=0,eligible_33_48=0,eligible_49_64=0;
    unsigned long long handoffs=0;
    unsigned long long entry_le16=0,entry_17_32=0,entry_33_48=0,entry_49_64=0;
    unsigned long long p_sum=0,c_sum=0,forced_sum=0,active_sum=0;
    unsigned long long ext_rows=0,left_rows=0,forced_ext_checks=0,forced_left_edges=0;
    unsigned long long bit_nodes=0,state64_nodes=0,state32_transitions=0;
    unsigned long long whole_calls=0,leaf_calls=0,outputs=0;

    inline void observeEligible(const int active){
        ++eligible_nodes;
        if(active<=32) ++eligible_le32;
        else if(active<=48) ++eligible_33_48;
        else if(active<=64) ++eligible_49_64;
    }
    inline void observeHandoff(const int p,const int c,const int f){
        ++handoffs;
        const int a=p+c;
        p_sum+=static_cast<unsigned long long>(p);
        c_sum+=static_cast<unsigned long long>(c);
        forced_sum+=static_cast<unsigned long long>(f);
        active_sum+=static_cast<unsigned long long>(a);
        if(a<=16) ++entry_le16;
        else if(a<=32) ++entry_17_32;
        else if(a<=48) ++entry_33_48;
        else if(a<=64) ++entry_49_64;
    }
    inline void observeBuild(const int extN,const int leftN,
                             const unsigned long long extChecks,
                             const unsigned long long leftEdges){
        ext_rows+=static_cast<unsigned long long>(extN);
        left_rows+=static_cast<unsigned long long>(leftN);
        forced_ext_checks+=extChecks;
        forced_left_edges+=leftEdges;
    }
    inline void observeState64Node(){ ++state64_nodes; }
    inline void observeState32Transition(){ ++state32_transitions; }
    inline void observeKernelDelta(const unsigned long long nodes,const unsigned long long whole,
                                   const unsigned long long leaf,const unsigned long long out){
        bit_nodes+=nodes; whole_calls+=whole; leaf_calls+=leaf; outputs+=out;
    }
    inline void merge(const ForcedBaseBit64Stats& o){
        eligible_nodes+=o.eligible_nodes;
        eligible_le32+=o.eligible_le32; eligible_33_48+=o.eligible_33_48; eligible_49_64+=o.eligible_49_64;
        handoffs+=o.handoffs;
        entry_le16+=o.entry_le16; entry_17_32+=o.entry_17_32; entry_33_48+=o.entry_33_48; entry_49_64+=o.entry_49_64;
        p_sum+=o.p_sum; c_sum+=o.c_sum; forced_sum+=o.forced_sum; active_sum+=o.active_sum;
        ext_rows+=o.ext_rows; left_rows+=o.left_rows;
        forced_ext_checks+=o.forced_ext_checks; forced_left_edges+=o.forced_left_edges;
        bit_nodes+=o.bit_nodes; state64_nodes+=o.state64_nodes; state32_transitions+=o.state32_transitions;
        whole_calls+=o.whole_calls; leaf_calls+=o.leaf_calls; outputs+=o.outputs;
    }
};

} // namespace statebit
