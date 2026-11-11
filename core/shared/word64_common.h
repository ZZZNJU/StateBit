#pragma once
#include <cstdint>
#include <climits>
#include <algorithm>
// Shared Word64 primitive kernels. Both frameworks use this EXACT source.
// Distinct, intentional candidate-refinement contracts are preserved:
// PARALLEL_ENGINE BulkSnapshot computes degree after all static removals;
// SEQUENTIAL_ENGINE OrderedStreaming updates the live universe after each removal.
namespace word64_common {
using Word=uint64_t;
inline int pop(Word a){return __builtin_popcountll(a);}
inline int first(Word a){return __builtin_ctzll(a);}
enum class RefineOrder: unsigned char { BulkSnapshot=0, OrderedStreaming=1 };
inline int select_pivot(Word P,Word C,int k,int active_lower_bound,
        const Word* adj,const Word* nonadj,int empty_domain_code){
    const Word U=P|C;const int u_count=pop(U);
    int min_vertex=-1,min_degree=INT_MAX;
    for(Word t=P;t;t&=t-1){
        const int v=first(t),d=pop(adj[v]&U);
        if(d<min_degree){min_degree=d;min_vertex=v;}
    }
    if(min_vertex<0 || min_degree+k<std::max(active_lower_bound,pop(P)))return -2;
    Word domain=(min_degree+k<u_count)?C&nonadj[min_vertex]:C;
    int best=empty_domain_code,best_degree=INT_MAX,best_p_degree=INT_MAX;
    for(Word t=domain;t;t&=t-1){
        const int v=first(t),d=pop(adj[v]&U),pd=pop(adj[v]&P);
        if(d<best_degree || (d==best_degree && pd<best_p_degree)){
            best=v;best_degree=d;best_p_degree=pd;
        }
    }
    return best;
}
inline Word saturated(Word P,int k,const Word* nonadj){
    Word out=0;
    for(Word t=P;t;t&=t-1){int v=first(t);if(pop(nonadj[v]&P)==k-1)out|=1ULL<<v;}
    return out;
}
inline Word refine_include(int pivot,Word P,Word C,int k,int active_lower_bound,
        const Word* pair,const Word* nonadj,const Word* adj,RefineOrder order){
    const Word sat=saturated(P,k,nonadj);
    const int lower=std::max(active_lower_bound,pop(P));
    if(order==RefineOrder::OrderedStreaming){
        Word U=P|C;
        for(Word t=C;t;t&=t-1){
            const int v=first(t);const Word bit=1ULL<<v;
            bool erase=!(pair[pivot]&bit);
            if(!erase) erase=(pop(nonadj[v]&P)>=k)||((nonadj[v]&sat)!=0)||
                (pop(adj[v]&U)+k<lower);
            if(erase){C&=~bit;U&=~bit;}
        }
        return C;
    }
    // PARALLEL_ENGINE contract: static test against original C, then one synchronous degree pass.
    Word bad=~pair[pivot];
    for(Word t=C;t;t&=t-1){
        const int v=first(t);const Word bit=1ULL<<v;
        if(pop(nonadj[v]&P)>=k || (nonadj[v]&sat))bad|=bit;
    }
    C&=~bad;
    const Word U=P|C;
    Word degree_bad=0;
    for(Word t=C;t;t&=t-1){
        const int v=first(t);
        if(pop(adj[v]&U)+k<lower)degree_bad|=1ULL<<v;
    }
    return C&~degree_bad;
}
} // namespace word64_common
