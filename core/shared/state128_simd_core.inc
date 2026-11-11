// WIDE_SIMD sparse-scan: ONE shared two-lane 128-bit AVX2 kernel for both execution modes.
// Traverse union of live masks in ascending bit index to preserve branch/refine order.
// Variant 1 = SIMD saturation + full-pair-only (PARALLEL_ENGINE-inspired).
// Variant 2 = scalar saturation + single-lane SIMD refill (SEQUENTIAL_ENGINE-inspired).
#ifndef WIDE_SIMD_SHARED_STATE128_SIMD_INC
#define WIDE_SIMD_SHARED_STATE128_SIMD_INC
#include <immintrin.h>
#include <vector>
#include <algorithm>
#include <climits>
namespace statebit { namespace wide_simd_shared128 {
inline __m256i pack(Mask128 a,Mask128 b){
    return _mm256_setr_epi64x((long long)a.lo,(long long)a.hi,(long long)b.lo,(long long)b.hi);
}
inline __m256i spread(Mask128 a){return pack(a,a);}
inline __m256i pop4(__m256i x){
    const __m256i lut=_mm256_setr_epi8(0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4);
    const __m256i mask=_mm256_set1_epi8(15);
    const __m256i low=_mm256_and_si256(x,mask);
    const __m256i high=_mm256_and_si256(_mm256_srli_epi16(x,4),mask);
    return _mm256_sad_epu8(_mm256_add_epi8(_mm256_shuffle_epi8(lut,low),_mm256_shuffle_epi8(lut,high)),_mm256_setzero_si256());
}
inline void pop2(__m256i x,int v[2]){
    alignas(32) uint64_t a[4]; _mm256_store_si256((__m256i*)a,pop4(x));
    v[0]=(int)(a[0]+a[1]);v[1]=(int)(a[2]+a[3]);
}
template<class Ctx>
void pivot2(const Mask128 P[2],const Mask128 C[2],int active,const Ctx& ctx,int k,int lb,int pivot[2]){
    const __m256i pv=pack(P[0],P[1]),uv=pack(P[0]|C[0],P[1]|C[1]);
    int us[2],ps[2];pop2(uv,us);pop2(pv,ps);
    int pmin[2]={-1,-1},pdeg[2]={INT_MAX,INT_MAX};
    Mask128 pScan=P[0]|(active>1?P[1]:Mask128{});
    while(mask128Any(pScan)){
        const int x=mask128First(pScan);
        pScan=mask128ClearBit(pScan,x);
        bool inP[2]={mask128Test(P[0],x),mask128Test(P[1],x)};
        if(!(inP[0]||(active>1&&inP[1])))continue;
        int d[2];pop2(_mm256_and_si256(spread(ctx.adj[x]),uv),d);
        for(int l=0;l<active;l++)if(inP[l]&&d[l]<pdeg[l]){pmin[l]=x;pdeg[l]=d[l];}
    }
    Mask128 domain[2]{};
    for(int l=0;l<active;l++){
        pivot[l]=-2;
        if(pmin[l]<0||pdeg[l]+k<std::max(lb,ps[l]))continue;
        domain[l]=(pdeg[l]+k<us[l])?(C[l]&ctx.nonadj[pmin[l]]):C[l];
    }
    int best[2]={-1,-1},bdg[2]={INT_MAX,INT_MAX},bpd[2]={INT_MAX,INT_MAX};
    Mask128 dScan=domain[0]|(active>1?domain[1]:Mask128{});
    while(mask128Any(dScan)){
        const int x=mask128First(dScan);
        dScan=mask128ClearBit(dScan,x);
        bool inD[2]={mask128Test(domain[0],x),mask128Test(domain[1],x)};
        if(!(inD[0]||(active>1&&inD[1])))continue;
        int d[2],pd[2];__m256i row=spread(ctx.adj[x]);
        pop2(_mm256_and_si256(row,uv),d);pop2(_mm256_and_si256(row,pv),pd);
        for(int l=0;l<active;l++)if(inD[l]&&(d[l]<bdg[l]||(d[l]==bdg[l]&&pd[l]<bpd[l]))){best[l]=x;bdg[l]=d[l];bpd[l]=pd[l];}
    }
    for(int l=0;l<active;l++)if(best[l]>=0)pivot[l]=best[l];
}
template<int Variant,class Ctx>
unsigned long long refine2(const int piv[2],const Mask128 Pinc[2],const Mask128 Cminus[2],int active,const Ctx& ctx,int k,int lb,Mask128 result[2]){
    Mask128 sat[2]{};
    if(Variant==1){
        const __m256i pv=pack(Pinc[0],Pinc[1]);
        Mask128 satScan=Pinc[0]|(active>1?Pinc[1]:Mask128{});
        while(mask128Any(satScan)){
            const int u=mask128First(satScan);
            satScan=mask128ClearBit(satScan,u);
            bool in0=mask128Test(Pinc[0],u),in1=active>1&&mask128Test(Pinc[1],u);
            if(!in0&&!in1)continue;
            int miss[2];pop2(_mm256_and_si256(spread(ctx.nonadj[u]),pv),miss);
            if(in0&&miss[0]==k-1)sat[0]|=mask128Bit(u);
            if(in1&&miss[1]==k-1)sat[1]|=mask128Bit(u);
        }
    }else{
        for(int l=0;l<active;l++)sat[l]=simd_base_shared128::saturated(Pinc[l],ctx,k);
    }
    result[0]=Cminus[0];result[1]=active>1?Cminus[1]:Mask128{};
    int ps[2]={mask128Pop(Pinc[0]),active>1?mask128Pop(Pinc[1]):0};
    unsigned long long removed=0;
    // Snapshot the initial candidate union. Clearing the current bit cannot
    // affect the ascending order of later candidates, while each degree uses
    Mask128 cScan=result[0]|(active>1?result[1]:Mask128{});
    const __m256i pv=pack(Pinc[0],Pinc[1]); // Pinc is immutable within refine2
    while(mask128Any(cScan)){
        const int x=mask128First(cScan);
        cScan=mask128ClearBit(cScan,x);
        bool inc[2]={mask128Test(result[0],x),active>1&&mask128Test(result[1],x)};
        if(!inc[0]&&!inc[1])continue;
        // 'U' is recomputed after each candidate removal, matching SIMD_BASE Scalar's dynamic degree test.
        const __m256i uv=pack(Pinc[0]|result[0],Pinc[1]|result[1]);
        int miss[2],dg[2];pop2(_mm256_and_si256(spread(ctx.nonadj[x]),pv),miss);
        pop2(_mm256_and_si256(spread(ctx.adj[x]),uv),dg);
        for(int l=0;l<active;l++)if(inc[l]){
            bool erase=!mask128Test(ctx.pair[piv[l]],x);
            if(!erase)erase=miss[l]>k-1 || mask128Any(ctx.nonadj[x]&sat[l]) || dg[l]+k<std::max(lb,ps[l]);
            if(erase){result[l]=mask128AndNot(result[l],mask128Bit(x));++removed;}
        }
    }
    return removed;
}
// The same implementation compiles in BOTH both execution modes; all host state/metrics
// changes are callbacks. Include children use the same host export contract as scalar DFS.
template<int Variant,class Node,class Ctx,class OnNode,class OnWhole,class OnLeaf,class Emit,class Handoff,class ScalarPivot,class ScalarRefine,class OnPacket,class OnPivot,class OnRefine,class OnSingle,class OnChildren,class ExportInclude,class OnReady>
void run(const Node& root,const Ctx& ctx,int k,int lb,OnNode onNode,OnWhole onWhole,OnLeaf onLeaf,Emit emit,Handoff handoff,ScalarPivot scalarPivot,ScalarRefine scalarRefine,OnPacket onPacket,OnPivot onPivot,OnRefine onRefine,OnSingle onSingle,OnChildren onChildren,ExportInclude exportInclude,OnReady onReady,std::vector<Node>* scratch=nullptr){
    static_assert(Variant==1||Variant==2,"WIDE_SIMD SIMD variant must be 1 or 2");
    std::vector<Node> local;
    assert(!scratch || scratch->empty());
    auto& stack=scratch?*scratch:local;
    if(stack.capacity()<256)stack.reserve(256);
    stack.push_back(root);
    while(!stack.empty()){
        Node ready[2]{};int active=0;
        while(active<2&&!stack.empty()){
            Node n=stack.back();stack.pop_back();onNode();
            const int ps=mask128Pop(n.P),cs=mask128Pop(n.C);
            if(ps+cs<lb)continue;
            const Mask128 U=n.P|n.C;
            bool whole=true;Mask128 t=U;
            while(mask128Any(t)){
                int u=mask128First(t);t=mask128ClearBit(t,u);
                if(mask128Pop(ctx.nonadj[u]&U)>=k){whole=false;break;}
            }
            if(whole){onWhole();emit(U,n.X);continue;}
            if(mask128None(n.C)){onLeaf();if(ps>=lb)emit(n.P,n.X);continue;}
            if(mask128Pop(U)<=64){handoff(n);continue;}
            onReady(n);
            ready[active++]=n;
        }
        if(active==0)continue;
        if(Variant==1&&active==1){
            onSingle();
            int pivot=scalarPivot(ready[0]);
            if(pivot<0)continue;
            Mask128 b=mask128Bit(pivot),Pinc=ready[0].P|b,Cminus=mask128AndNot(ready[0].C,b);
            Mask128 Cinc=scalarRefine(pivot,Pinc,Cminus);
            stack.push_back(Node{ready[0].P,Cminus,ready[0].X|b});
            const Node include{Pinc,Cinc,ready[0].X};
            if(!exportInclude(include))stack.push_back(include);
            onChildren(2,true);
            continue;
        }
        onPacket(active);
        int piv[2]={-2,-2};Mask128 P[2]={ready[0].P,ready[1].P},C[2]={ready[0].C,ready[1].C};
        pivot2(P,C,active,ctx,k,lb,piv);onPivot();
        bool valid[2]={piv[0]>=0,active>1&&piv[1]>=0};
        Mask128 Pinc[2]{},Cminus[2]{},Cinc[2]{};
        int maskCount=0;
        for(int l=0;l<active;l++)if(valid[l]){
            Mask128 b=mask128Bit(piv[l]);Pinc[l]=ready[l].P|b;
            Cminus[l]=mask128AndNot(ready[l].C,b);++maskCount;
        }
        if(maskCount){
            if(Variant==1&&maskCount==1){
                for(int l=0;l<active;l++)if(valid[l])Cinc[l]=scalarRefine(piv[l],Pinc[l],Cminus[l]);
            }else{
                const unsigned long long removed=refine2<Variant>(piv,Pinc,Cminus,active,ctx,k,lb,Cinc);
                onRefine(active,removed);
            }
        }
        // Exclude first, include next for each active lane; no descendant is dropped.
        for(int l=active-1;l>=0;l--)if(valid[l]){
            const Mask128 b=mask128Bit(piv[l]);
            stack.push_back(Node{ready[l].P,Cminus[l],ready[l].X|b});
            const Node include{Pinc[l],Cinc[l],ready[l].X};
            if(!exportInclude(include))stack.push_back(include);
        }
        if(maskCount)onChildren(2*maskCount,false);
    }
}
}} // namespace statebit::wide_simd_shared128
#endif
