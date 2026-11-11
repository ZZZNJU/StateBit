// SIMD_BASE: common 128-bit SCALAR search primitives for both execution modes.
// This header is deliberately independent of owner scheduling, forced-base
// storage and output policy. Both EXTERNAL_AGGREGATION builds include this exact same file.
#ifndef SIMD_BASE_SHARED_STATE128_CORE_INC
#define SIMD_BASE_SHARED_STATE128_CORE_INC
namespace statebit { namespace simd_base_shared128 {
// Common physical context layout shared by PARALLEL_ENGINE_EXTERNAL_AGGREGATION and SEQUENTIAL_ENGINE_EXTERNAL_AGGREGATION.
// Host-only forced-base penalty arrays are kept in derived adapters.
struct CommonContext {
    int m=0,fixed=0,extN=0;
    bool proper=true;
    int orig[128];
    Mask128 adj[128],nonadj[128],pair[128];
    std::vector<int> extOrig;
    std::vector<Mask128> extAdj,leftAdj;
};


template <class Ctx> inline Mask128 saturated(Mask128 P, const Ctx& ctx, int k) {
    Mask128 sat{}, t=P;
    while(mask128Any(t)) {
        const int u=mask128First(t); t=mask128ClearBit(t,u);
        if(mask128Pop(ctx.nonadj[u]&P)==k-1) sat |= mask128Bit(u);
    }
    return sat;
}
template <class Ctx> inline bool canAddInternal(int x, Mask128 S, Mask128 sat, const Ctx& ctx, int k) {
    return mask128Pop(ctx.adj[x]&S)+k>=mask128Pop(S)+1 && mask128None(ctx.nonadj[x]&sat);
}
template <class Ctx> inline bool canAddExternal(int e, Mask128 S, Mask128 sat, const Ctx& ctx, int k, int miss) {
    if(miss>=k) return false;
    return mask128Pop(ctx.extAdj[e]&S)+(k-miss)>=mask128Pop(S)+1 &&
           mask128None(mask128AndNot(sat,ctx.extAdj[e]));
}
template <class Ctx, class ExtMiss, class LeftMiss, class Emit>
inline bool maximal(Mask128 S, Mask128 X, const Ctx& ctx, int k, int leftSz,
                    ExtMiss&& extMiss, LeftMiss&& leftMiss, Emit&& emit) {
    const int ss=mask128Pop(S);
    const Mask128 sat=saturated(S,ctx,k);
    Mask128 t=X;
    while(mask128Any(t)) {
        const int x=mask128First(t); t=mask128ClearBit(t,x);
        if(canAddInternal(x,S,sat,ctx,k)) return false;
    }
    for(int e=0;e<ctx.extN;e++) {
        if(canAddExternal(e,S,sat,ctx,k,extMiss(e))) return false;
    }
    for(int l=0;l<leftSz;l++) {
        const int miss=leftMiss(l);
        if(miss>=k) continue;
        const Mask128 a=ctx.leftAdj[l];
        if(mask128Pop(a&S)+(k-miss)>=ss+1 && mask128None(mask128AndNot(sat,a))) return false;
    }
    emit(S,ctx);
    return true;
}
template <class Ctx> inline int choosePivot(Mask128 P, Mask128 C, const Ctx& ctx, int k, int lb) {
    const Mask128 U=P|C; const int us=mask128Pop(U);
    int pmin=-1,pdeg=INT_MAX; Mask128 t=P;
    while(mask128Any(t)) {
        const int u=mask128First(t); t=mask128ClearBit(t,u);
        const int d=mask128Pop(ctx.adj[u]&U);
        if(d<pdeg){ pdeg=d; pmin=u; }
    }
    if(pmin<0) return -2;
    if(pdeg+k<std::max(lb,mask128Pop(P))) return -2;
    Mask128 choices=(pdeg+k<us)?(C&ctx.nonadj[pmin]):C;
    int best=-1,bdg=INT_MAX,bpdeg=INT_MAX;
    while(mask128Any(choices)) {
        const int x=mask128First(choices); choices=mask128ClearBit(choices,x);
        const int dg=mask128Pop(ctx.adj[x]&U),pd=mask128Pop(ctx.adj[x]&P);
        if(dg<bdg||(dg==bdg&&pd<bpdeg)){ best=x; bdg=dg; bpdeg=pd; }
    }
    return best;
}
template <class Ctx> inline Mask128 refineIncludeOnce(int pivot, Mask128 P, Mask128 C,
                                                        const Ctx& ctx, int k, int lb,
                                                        unsigned long long& removed) {
    Mask128 U=P|C; const Mask128 sat=saturated(P,ctx,k); Mask128 t=C;
    removed=0;
    while(mask128Any(t)) {
        const int x=mask128First(t); t=mask128ClearBit(t,x); const Mask128 b=mask128Bit(x);
        bool erase=mask128None(ctx.pair[pivot]&b);
        if(!erase) erase=mask128Pop(ctx.nonadj[x]&P)>k-1 || mask128Any(ctx.nonadj[x]&sat) ||
                         mask128Pop(ctx.adj[x]&U)+k<std::max(lb,mask128Pop(P));
        if(erase){ C=mask128AndNot(C,b); U=mask128AndNot(U,b); ++removed; }
    }
    return C;
}
// A shared DFS control flow; hook implementations differ ONLY for host-specific
// counters, emission, cross-width dispatch and PARALLEL_ENGINE task donation.
template <class Node, class Ctx, class OnNode, class OnWhole, class OnLeaf,
          class Maximal, class Handoff64, class Pivot, class Refine, class Include, class Exclude>
inline void scalarDFS(const Node& st, const Ctx& ctx, int k, int lb,
                      OnNode&& onNode, OnWhole&& onWhole, OnLeaf&& onLeaf,
                      Maximal&& maximalFn, Handoff64&& handoff64, Pivot&& pivotFn,
                      Refine&& refineFn, Include&& includeFn, Exclude&& excludeFn) {
    onNode();
    const int ps=mask128Pop(st.P), cs=mask128Pop(st.C);
    if(ps+cs<lb) return;
    const Mask128 U=st.P|st.C;
    bool whole=true; Mask128 t=U;
    while(mask128Any(t)) {
        const int u=mask128First(t); t=mask128ClearBit(t,u);
        if(mask128Pop(ctx.nonadj[u]&U)>=k){ whole=false; break; }
    }
    if(whole){ onWhole(); maximalFn(U,st.X); return; }
    if(mask128None(st.C)){ onLeaf(); if(ps>=lb) maximalFn(st.P,st.X); return; }
    if(mask128Pop(U)<=64){ handoff64(st,ctx); return; }
    const int pivot=pivotFn(st.P,st.C,ctx);
    if(pivot<0) return;
    const Mask128 b=mask128Bit(pivot),Pinc=st.P|b,Cminus=mask128AndNot(st.C,b);
    const Mask128 Cinc=refineFn(pivot,Pinc,Cminus,ctx);
    includeFn(Node{Pinc,Cinc,st.X},ctx);
    excludeFn(Node{st.P,Cminus,st.X|b},ctx);
}
} // namespace simd_base_shared128
} // namespace statebit
#endif
