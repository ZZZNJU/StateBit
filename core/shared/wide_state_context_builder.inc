// WIDE_CONTEXT_VALIDATION common List -> State128 handoff builder. C++14, no dependency on
// FOLDING/NET_COST_GATE/Folding/ForcedBase counters or on 128->64 conversion.
// NOTE: these phases are deliberately separately callable, so each host keeps
// its original placement of external forced penalties and left forced caches.
#ifndef WIDE_STATE_CONTEXT_BUILDER_INC
#define WIDE_STATE_CONTEXT_BUILDER_INC
namespace statebit { namespace wide_state_context_builder {
template <class Ctx,class PAt,class CAt,class GetRow>
inline void buildActive(Ctx& ctx,int fixed,int csize,bool proper,int pairThreshold,PAt&& pAt,CAt&& cAt,GetRow&& getRow) {
    ctx.fixed=fixed; ctx.m=fixed+csize; ctx.proper=proper;
    assert(ctx.m>0 && ctx.m<=128);
    for(int i=0;i<fixed;i++) ctx.orig[i]=pAt(i);
    for(int i=0;i<csize;i++) ctx.orig[fixed+i]=cAt(i);
    const Mask128 all=mask128LowBits(ctx.m);
    for(int i=0;i<ctx.m;i++){ctx.adj[i]={};ctx.pair[i]=mask128Bit(i);}
    for(int i=0;i<ctx.m;i++) {
        const int vi=ctx.orig[i];
        const uint8_t* row=getRow(vi);
        for(int j=i+1;j<ctx.m;j++) {
            const uint8_t cm=row[ctx.orig[j]];
            const Mask128 bi=mask128Bit(i),bj=mask128Bit(j);
            if(cm&1){ctx.adj[i]|=bj;ctx.adj[j]|=bi;}
            if(!proper || cm>=pairThreshold){ctx.pair[i]|=bj;ctx.pair[j]|=bi;}
        }
    }
    for(int i=0;i<ctx.m;i++)ctx.nonadj[i]=mask128AndNot(mask128AndNot(all,ctx.adj[i]),mask128Bit(i));
}
template <class Ctx,class EAt,class Adjacent>
inline void buildExternal(Ctx& ctx,int extCount,EAt&& extAt,Adjacent&& adjacent) {
    ctx.extN=extCount;
    ctx.extOrig.resize(ctx.extN);
    ctx.extAdj.resize(ctx.extN);
    for(int e=0;e<ctx.extN;e++) {
        const int x=extAt(e); ctx.extOrig[e]=x;
        Mask128 a{};
        for(int j=0;j<ctx.m;j++) if(adjacent(x,ctx.orig[j])) a|=mask128Bit(j);
        ctx.extAdj[e]=a;
    }
}
template <class Ctx,class VisitLeft>
inline void buildLeft(Ctx& ctx,int leftSz,VisitLeft&& visitLeft) {
    ctx.leftAdj.assign(static_cast<size_t>(leftSz),Mask128{});
    for(int j=0;j<ctx.m;j++) {
        const int v=ctx.orig[j];
        visitLeft(v,[&](int l){ if(l>=0 && l<leftSz)ctx.leftAdj[l]|=mask128Bit(j); });
    }
}
template <class Ctx>
inline bool equalCommon(const Ctx& a,const Ctx& b,int leftSz) {
    if(a.fixed!=b.fixed||a.m!=b.m||a.extN!=b.extN||a.proper!=b.proper ||
       a.extOrig!=b.extOrig || a.extAdj!=b.extAdj || a.leftAdj!=b.leftAdj ||
       a.leftAdj.size()!=static_cast<size_t>(leftSz)) return false;
    for(int i=0;i<a.m;i++) if(a.orig[i]!=b.orig[i]||a.adj[i]!=b.adj[i]||
       a.nonadj[i]!=b.nonadj[i]||a.pair[i]!=b.pair[i]) return false;
    return true;
}
} }
#endif
