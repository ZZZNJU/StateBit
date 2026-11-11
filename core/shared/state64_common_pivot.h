#pragma once
#include <cstdint>
#include <immintrin.h>

// Shared, executable sequential and parallel execution State64 4x64 union-sparse pivot implementation.
// Inputs carry implementation-specific active lower bounds so C0/Folding state
// semantics remain in each original frontend. Never owns or alters P,C,X,F.
namespace state64_common {
template<typename Popcnt>
inline uint8_t choose_pivot(const uint64_t P[4],const uint64_t C[4],
                           const int lbs[4],const int active,const int k,
                           const uint64_t* adj,const uint64_t* non,int pivot[4],Popcnt popcnt) {
    alignas(32) uint64_t ps[4]={0,0,0,0},cs[4]={0,0,0,0};
    uint64_t unionP=0;
    for(int i=0;i<active;i++) {ps[i]=P[i];cs[i]=C[i];unionP|=P[i];pivot[i]=-2;}
    const __m256i Pv=_mm256_load_si256(reinterpret_cast<const __m256i*>(ps));
    const __m256i Cv=_mm256_load_si256(reinterpret_cast<const __m256i*>(cs));
    const __m256i Uv=_mm256_or_si256(Pv,Cv);
    const __m256i psV=popcnt(Pv), usV=popcnt(Uv);
    __m256i bestDeg=_mm256_set1_epi64x(0x3fffffffffffffffLL);
    __m256i bestIdx=_mm256_set1_epi64x(-1);
    for(uint64_t tmp=unionP;tmp;tmp&=tmp-1){
        const int x=__builtin_ctzll(tmp);
        const __m256i bit=_mm256_set1_epi64x(static_cast<long long>(1ULL<<x));
        const __m256i inP=_mm256_cmpeq_epi64(_mm256_and_si256(Pv,bit),bit);
        const __m256i row=_mm256_set1_epi64x(static_cast<long long>(adj[x]));
        const __m256i dg=popcnt(_mm256_and_si256(row,Uv));
        const __m256i better=_mm256_and_si256(inP,_mm256_cmpgt_epi64(bestDeg,dg));
        bestDeg=_mm256_blendv_epi8(bestDeg,dg,better);
        bestIdx=_mm256_blendv_epi8(bestIdx,_mm256_set1_epi64x(x),better);
    }
    alignas(32) uint64_t pmin[4],pdeg[4],pc[4],uc[4],domain[4]={0,0,0,0};
    _mm256_store_si256(reinterpret_cast<__m256i*>(pmin),bestIdx);
    _mm256_store_si256(reinterpret_cast<__m256i*>(pdeg),bestDeg);
    _mm256_store_si256(reinterpret_cast<__m256i*>(pc),psV);
    _mm256_store_si256(reinterpret_cast<__m256i*>(uc),usV);
    uint8_t eligible=0;
    for(int i=0;i<active;i++){
        const int pm=static_cast<int>(pmin[i]);
        const int lim=lbs[i]>static_cast<int>(pc[i])?lbs[i]:static_cast<int>(pc[i]);
        if(pm<0||static_cast<int>(pdeg[i])+k<lim)continue;
        domain[i]=(static_cast<int>(pdeg[i])+k<static_cast<int>(uc[i]))
                  ?(cs[i]&non[pm]):cs[i];
        if(domain[i])eligible|=static_cast<uint8_t>(1u<<i);
    }
    alignas(32) uint64_t valid[4]={0,0,0,0};
    uint64_t unionD=0;
    for(int i=0;i<active;i++){
        if(eligible&(1u<<i))valid[i]=~0ULL;
        unionD|=domain[i];
    }
    const __m256i validV=_mm256_load_si256(reinterpret_cast<const __m256i*>(valid));
    const __m256i Dv=_mm256_load_si256(reinterpret_cast<const __m256i*>(domain));
    __m256i candDeg=_mm256_set1_epi64x(0x3fffffffffffffffLL);
    __m256i candPDeg=_mm256_set1_epi64x(0x3fffffffffffffffLL);
    __m256i candIdx=_mm256_set1_epi64x(-1);
    for(uint64_t tmp=unionD;tmp;tmp&=tmp-1){
        const int x=__builtin_ctzll(tmp);
        const __m256i bit=_mm256_set1_epi64x(static_cast<long long>(1ULL<<x));
        const __m256i inD=_mm256_and_si256(validV,_mm256_cmpeq_epi64(_mm256_and_si256(Dv,bit),bit));
        const __m256i row=_mm256_set1_epi64x(static_cast<long long>(adj[x]));
        const __m256i dg=popcnt(_mm256_and_si256(row,Uv));
        const __m256i pd=popcnt(_mm256_and_si256(row,Pv));
        const __m256i betterD=_mm256_cmpgt_epi64(candDeg,dg);
        const __m256i eqD=_mm256_cmpeq_epi64(candDeg,dg);
        const __m256i betterP=_mm256_cmpgt_epi64(candPDeg,pd);
        const __m256i better=_mm256_and_si256(inD,_mm256_or_si256(betterD,_mm256_and_si256(eqD,betterP)));
        candDeg=_mm256_blendv_epi8(candDeg,dg,better);
        candPDeg=_mm256_blendv_epi8(candPDeg,pd,better);
        candIdx=_mm256_blendv_epi8(candIdx,_mm256_set1_epi64x(x),better);
    }
    alignas(32) uint64_t chosen[4];
    _mm256_store_si256(reinterpret_cast<__m256i*>(chosen),candIdx);
    uint8_t out=0;
    for(int i=0;i<active;i++){
        if((eligible&(1u<<i))&&static_cast<long long>(chosen[i])>=0){
            pivot[i]=static_cast<int>(chosen[i]);out|=static_cast<uint8_t>(1u<<i);
        }
    }
    return out;
}
} // namespace state64_common
