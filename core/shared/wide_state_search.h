// WIDE_STATE: shared scalar State128/State256 search state machine.
// Folding penalties, output mapping and thread donation remain caller callbacks.
#ifndef STATEBIT_WIDE_STATE_WIDE_DFS_H
#define STATEBIT_WIDE_STATE_WIDE_DFS_H
#include <algorithm>
#include <climits>
#include <cstdint>
namespace statebit { namespace statebit_wide_search {
template <typename M> struct Ops;
template <> struct Ops<Mask128> {
    static int pop(Mask128 x){return mask128Pop(x);}
    static bool any(Mask128 x){return mask128Any(x);}
    static int first(Mask128 x){return mask128First(x);}
    static Mask128 clear(Mask128 x,int i){return mask128ClearBit(x,i);}
    static Mask128 bit(int i){return mask128Bit(i);}
    static Mask128 without(Mask128 x,Mask128 y){return mask128AndNot(x,y);}
};
template <> struct Ops<Mask256> {
    static int pop(Mask256 x){return mask256Pop(x);}
    static bool any(Mask256 x){return mask256Any(x);}
    static int first(Mask256 x){return mask256First(x);}
    static Mask256 clear(Mask256 x,int i){return mask256ClearBit(x,i);}
    static Mask256 bit(int i){return mask256Bit(i);}
    static Mask256 without(Mask256 x,Mask256 y){return mask256AndNot(x,y);}
};
// Single shared DFS for scalar 128 and 256, and both sequential and parallel execution hosts.
template<class Node,class Ctx,class M,class OnNode,class OnWhole,class OnLeaf,
         class OnMaximal,class OnHandoff,class OnPivot,class OnRefine,class OnInclude,class OnExclude>
inline void dfs(const Node& st,const Ctx& ctx,int k,int lb,int handoffLimit,
                OnNode&& onNode,OnWhole&& onWhole,OnLeaf&& onLeaf,
                OnMaximal&& maximal,OnHandoff&& handoff,
                OnPivot&& pivot,OnRefine&& refine,OnInclude&& include,OnExclude&& exclude) {
    using O=Ops<M>;
    onNode();
    const int ps=O::pop(st.P),cs=O::pop(st.C);
    if(ps+cs<lb)return;
    const M U=st.P|st.C;
    bool whole=true;
    M t=U;
    while(O::any(t)){
        const int u=O::first(t); t=O::clear(t,u);
        if(O::pop(ctx.nonadj[u]&U)>=k){ whole=false;break; }
    }
    if(whole){onWhole();maximal(U,st.X);return;}
    if(!O::any(st.C)){onLeaf();if(ps>=lb)maximal(st.P,st.X);return;}
    if(O::pop(U)<=handoffLimit){handoff(st,ctx);return;}
    const int p=pivot(st.P,st.C,ctx);
    if(p<0)return;
    const M b=O::bit(p),Pinc=st.P|b,Cminus=O::without(st.C,b);
    const M Cinc=refine(p,Pinc,Cminus,ctx);
    include(Node{Pinc,Cinc,st.X},ctx);
    exclude(Node{st.P,Cminus,st.X|b},ctx);
}
// Shared 256-bit primitives; 128-bit versions were already shared in state128_core.inc.
template<class Ctx> inline Mask256 saturated(Mask256 P,const Ctx& ctx,int k){
    Mask256 sat{},t=P;
    while(mask256Any(t)){
        const int u=mask256First(t);t=mask256ClearBit(t,u);
        if(mask256Pop(ctx.nonadj[u]&P)==k-1)sat|=mask256Bit(u);
    }
    return sat;
}
template<class Ctx> inline bool canAddInternal(int x,Mask256 S,Mask256 sat,const Ctx& ctx,int k){
    return mask256Pop(ctx.adj[x]&S)+k>=mask256Pop(S)+1 && mask256None(ctx.nonadj[x]&sat);
}
template<class Ctx,class ExternalMiss,class LeftMiss,class Emit>
inline bool maximal(Mask256 S,Mask256 X,const Ctx& ctx,int k,int leftSz,
                    ExternalMiss&& externalMiss,LeftMiss&& leftMiss,Emit&& emit,
                    bool guardExternalMiss,bool guardLeftMiss){
    const int ss=mask256Pop(S);const Mask256 sat=saturated(S,ctx,k);
    Mask256 t=X;
    while(mask256Any(t)){
        const int x=mask256First(t);t=mask256ClearBit(t,x);
        if(canAddInternal(x,S,sat,ctx,k))return false;
    }
    for(int e=0;e<ctx.extN;e++){
        const int miss=externalMiss(e);
        if(guardExternalMiss && miss>=k)continue;
        if(mask256Pop(ctx.extAdj[e]&S)+k>=ss+1+miss &&
           mask256None(mask256AndNot(sat,ctx.extAdj[e])))return false;
    }
    for(int l=0;l<leftSz;l++){
        const int miss=leftMiss(l);
        if(guardLeftMiss && miss>=k)continue;
        const Mask256 a=ctx.leftAdj[l];
        if(mask256Pop(a&S)+k>=ss+1+miss && mask256None(mask256AndNot(sat,a)))return false;
    }
    emit(S,ctx);
    return true;
}
template<class Ctx> inline int pivot(Mask256 P,Mask256 C,const Ctx& ctx,int k,int lb){
    const Mask256 U=P|C;const int us=mask256Pop(U);
    int pmin=-1,pdeg=INT_MAX;Mask256 t=P;
    while(mask256Any(t)){
        const int u=mask256First(t);t=mask256ClearBit(t,u);
        const int d=mask256Pop(ctx.adj[u]&U);
        if(d<pdeg){pdeg=d;pmin=u;}
    }
    if(pmin<0)return -2;
    if(pdeg+k<std::max(lb,mask256Pop(P)))return -2;
    Mask256 choices=(pdeg+k<us)?(C&ctx.nonadj[pmin]):C;
    int best=-1,bdg=INT_MAX,bpdeg=INT_MAX;
    while(mask256Any(choices)){
        const int x=mask256First(choices);choices=mask256ClearBit(choices,x);
        const int dg=mask256Pop(ctx.adj[x]&U),pd=mask256Pop(ctx.adj[x]&P);
        if(dg<bdg||(dg==bdg&&pd<bpdeg)){best=x;bdg=dg;bpdeg=pd;}
    }
    return best;
}
template<class Ctx> inline Mask256 refine(int p,Mask256 P,Mask256 C,const Ctx& ctx,int k,int lb,unsigned long long& removed){
    Mask256 U=P|C;const Mask256 sat=saturated(P,ctx,k);Mask256 t=C;
    removed=0;
    while(mask256Any(t)){
        const int x=mask256First(t);t=mask256ClearBit(t,x);const Mask256 b=mask256Bit(x);
        bool erase=mask256None(ctx.pair[p]&b);
        if(!erase)erase=mask256Pop(ctx.nonadj[x]&P)>k-1 || mask256Any(ctx.nonadj[x]&sat) ||
                        mask256Pop(ctx.adj[x]&U)+k<std::max(lb,mask256Pop(P));
        if(erase){C=mask256AndNot(C,b);U=mask256AndNot(U,b);++removed;}
    }
    return C;
}
// Unified dense context construction; per-host fixed-vertex penalties are deliberately external.
template<class Ctx,class PAt,class CAt,class Matrix>
inline void buildActive256(Ctx& ctx,int fixed,int csize,bool proper,int threshold,
                           PAt&& pAt,CAt&& cAt,Matrix&& matrix){
    ctx.fixed=fixed;ctx.m=fixed+csize;ctx.proper=proper;
    assert(ctx.m>0 && ctx.m<=256);
    for(int i=0;i<fixed;i++)ctx.orig[i]=pAt(i);
    for(int i=0;i<csize;i++)ctx.orig[fixed+i]=cAt(i);
    const Mask256 all=mask256LowBits(ctx.m);
    for(int i=0;i<ctx.m;i++){ctx.adj[i]={};ctx.pair[i]=mask256Bit(i);}
    for(int i=0;i<ctx.m;i++)for(int j=i+1;j<ctx.m;j++){
        const uint8_t cm=matrix(ctx.orig[i],ctx.orig[j]);
        if(cm&1){ctx.adj[i]|=mask256Bit(j);ctx.adj[j]|=mask256Bit(i);}
        if(!proper||cm>=threshold){ctx.pair[i]|=mask256Bit(j);ctx.pair[j]|=mask256Bit(i);}
    }
    for(int i=0;i<ctx.m;i++)ctx.nonadj[i]=mask256AndNot(mask256AndNot(all,ctx.adj[i]),mask256Bit(i));
}
template<class Ctx,class EAt,class Adj>
inline void buildExternal256(Ctx& ctx,int extCount,EAt&& at,Adj&& adj){
    ctx.extN=extCount;ctx.extOrig.resize(extCount);ctx.extAdj.resize(extCount);
    for(int e=0;e<extCount;e++){
        int x=at(e);ctx.extOrig[e]=x;Mask256 a{};
        for(int j=0;j<ctx.m;j++)if(adj(x,ctx.orig[j]))a|=mask256Bit(j);
        ctx.extAdj[e]=a;
    }
}
template<class Ctx,class Visit>
inline void buildLeft256(Ctx& ctx,int leftSz,Visit&& visit){
    ctx.leftAdj.assign(static_cast<size_t>(leftSz),Mask256{});
    for(int j=0;j<ctx.m;j++)visit(ctx.orig[j],[&](int l){
        if(l>=0 && l<leftSz)ctx.leftAdj[l]|=mask256Bit(j);
    });
}
} } // namespace statebit::statebit_wide_search
#endif
