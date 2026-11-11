#pragma once
#include <cstdint>
#include <immintrin.h>

// One executable 4x64 candidate-refinement kernel for both execution modes.
// Contracts: bitPair/non/adj use the caller's current 64-bit coordinates;
// lbs[] is caller-computed active lower bound (including local fixed vertices);
// unused lanes are ignored; no P/C/X/F state is changed besides Cout[].
namespace state64_common {
struct RefineMetrics {
    unsigned long long defect_steps=0,static_removed=0,degree_steps=0,degree_removed=0;
};
template <typename Popcnt,typename Vmax>
inline void refine_2d_bulk4(const int pivot[4],const uint64_t Pinc[4],
                             const uint64_t Cminus[4],const uint8_t laneMask,
                             const uint64_t lbs[4],const int k,
                             const uint64_t* bitPair,const uint64_t* bitNon,
                             const uint64_t* bitAdj,uint64_t Cout[4],
                             Popcnt popcnt,Vmax vmax,RefineMetrics& metrics) {
            alignas(32) uint64_t pairs[4]={0,0,0,0},activeRows[4]={0,0,0,0};
            uint64_t unionP=0,unionC=0;
            for(int i=0;i<4;i++)if(laneMask&(1u<<i)){
                pairs[i]=bitPair[pivot[i]];activeRows[i]=~0ULL;
                unionP|=Pinc[i];unionC|=Cminus[i];
            }
            const __m256i activeV=_mm256_load_si256(reinterpret_cast<const __m256i*>(activeRows));
            const __m256i Pv=_mm256_loadu_si256(reinterpret_cast<const __m256i*>(Pinc));
            __m256i Cv=_mm256_loadu_si256(reinterpret_cast<const __m256i*>(Cminus));
            const __m256i zero=_mm256_setzero_si256(),all=_mm256_set1_epi64x(-1);
            const __m256i pairV=_mm256_load_si256(reinterpret_cast<const __m256i*>(pairs));
            // Threshold bit planes: d[t] marks vertices with >=t missing P neighbors.
            // Identical automaton as State32, but each data plane is a 4x64 vector.
            __m256i badNo=zero,satV=zero;
            if(k>0 && k<=8){
                __m256i d[8+1];
                for(int t=0;t<=k;t++)d[t]=zero;
                uint64_t tp=unionP;
                while(tp){
                    const int u=__builtin_ctzll(tp);tp&=tp-1;
                    ++metrics.defect_steps;
                    const __m256i bit=_mm256_set1_epi64x(static_cast<long long>(1ULL<<u));
                    const __m256i inP=_mm256_and_si256(activeV,
                        _mm256_cmpeq_epi64(_mm256_and_si256(Pv,bit),bit));
                    const __m256i non=_mm256_set1_epi64x(static_cast<long long>(bitNon[u]));
                    const __m256i hit=_mm256_and_si256(non,inP);
                    for(int t=k;t>=2;t--)d[t]=_mm256_or_si256(d[t],_mm256_and_si256(d[t-1],hit));
                    d[1]=_mm256_or_si256(d[1],hit);
                }
                badNo=d[k];
                if(k==1)satV=_mm256_and_si256(activeV,_mm256_and_si256(Pv,
                        _mm256_andnot_si256(d[1],all)));
                else satV=_mm256_and_si256(activeV,_mm256_and_si256(Pv,
                        _mm256_and_si256(d[k-1],_mm256_andnot_si256(d[k],all))));
            }else{
                // Any-K fallback retains exact semantics when the bit-plane
                // template budget is exceeded.
                uint64_t tp=unionP;
                while(tp){
                    const int u=__builtin_ctzll(tp);tp&=tp-1;
                    const __m256i bit=_mm256_set1_epi64x(static_cast<long long>(1ULL<<u));
                    const __m256i inP=_mm256_and_si256(activeV,_mm256_cmpeq_epi64(_mm256_and_si256(Pv,bit),bit));
                    const __m256i non=_mm256_set1_epi64x(static_cast<long long>(bitNon[u]));
                    const __m256i cnt=popcnt(_mm256_and_si256(non,Pv));
                    satV=_mm256_or_si256(satV,_mm256_and_si256(bit,_mm256_and_si256(inP,_mm256_cmpeq_epi64(cnt,_mm256_set1_epi64x(k-1)))));
                }
                uint64_t tc=unionC;
                while(tc){
                    const int u=__builtin_ctzll(tc);tc&=tc-1;
                    const __m256i bit=_mm256_set1_epi64x(static_cast<long long>(1ULL<<u));
                    const __m256i non=_mm256_set1_epi64x(static_cast<long long>(bitNon[u]));
                    const __m256i cnt=popcnt(_mm256_and_si256(non,Pv));
                    const __m256i bad=_mm256_cmpgt_epi64(cnt,_mm256_set1_epi64x(k-1));
                    badNo=_mm256_or_si256(badNo,_mm256_and_si256(bit,bad));
                }
            }
            __m256i satNon=zero;
            alignas(32) uint64_t satA[4];_mm256_store_si256(reinterpret_cast<__m256i*>(satA),satV);
            uint64_t unionSat=0;for(int i=0;i<4;i++)if(laneMask&(1u<<i))unionSat|=satA[i];
            while(unionSat){
                const int u=__builtin_ctzll(unionSat);unionSat&=unionSat-1;
                const __m256i bit=_mm256_set1_epi64x(static_cast<long long>(1ULL<<u));
                const __m256i inSat=_mm256_and_si256(activeV,_mm256_cmpeq_epi64(_mm256_and_si256(satV,bit),bit));
                const __m256i non=_mm256_set1_epi64x(static_cast<long long>(bitNon[u]));
                satNon=_mm256_or_si256(satNon,_mm256_and_si256(non,inSat));
            }
const __m256i staticBad=_mm256_or_si256(_mm256_andnot_si256(pairV,all),_mm256_or_si256(badNo,satNon));
            const __m256i oldC=Cv;
            Cv=_mm256_andnot_si256(staticBad,Cv);
            __m256i Uv=_mm256_or_si256(Pv,Cv);
            const __m256i lbV=_mm256_load_si256(reinterpret_cast<const __m256i*>(lbs));
            const __m256i psV=popcnt(Pv),limV=vmax(psV,lbV);
            alignas(32) uint64_t nowC[4],oldCArray[4];
            _mm256_store_si256(reinterpret_cast<__m256i*>(nowC),Cv);
            _mm256_store_si256(reinterpret_cast<__m256i*>(oldCArray),oldC);
            uint64_t unionLive=0;
            for(int i=0;i<4;i++)if(laneMask&(1u<<i)){
                unionLive|=nowC[i];
                metrics.static_removed+=__builtin_popcountll(oldCArray[i]&~nowC[i]);
            }
            // State32 REFINE_2D_MODE=2: one degree scan after bulk static pruning.
            __m256i eraseV=zero;
            uint64_t tc=unionLive;
            while(tc){
                const int x=__builtin_ctzll(tc);tc&=tc-1;
                ++metrics.degree_steps;
                const __m256i bit=_mm256_set1_epi64x(static_cast<long long>(1ULL<<x));
                const __m256i inC=_mm256_and_si256(activeV,_mm256_cmpeq_epi64(_mm256_and_si256(Cv,bit),bit));
                const __m256i row=_mm256_set1_epi64x(static_cast<long long>(bitAdj[x]));
                const __m256i degree=popcnt(_mm256_and_si256(row,Uv));
                const __m256i bad=_mm256_cmpgt_epi64(limV,_mm256_add_epi64(degree,_mm256_set1_epi64x(k)));
                eraseV=_mm256_or_si256(eraseV,_mm256_and_si256(bit,_mm256_and_si256(inC,bad)));
            }
            Cv=_mm256_andnot_si256(eraseV,Cv);
            alignas(32) uint64_t erased[4];_mm256_store_si256(reinterpret_cast<__m256i*>(erased),eraseV);
            for(int i=0;i<4;i++)if(laneMask&(1u<<i))metrics.degree_removed+=__builtin_popcountll(erased[i]);
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(Cout),Cv);
}
// Cache is just a hint: every retained witness is rechecked against U.
struct WholeResult {
    bool whole=true;
    uint8_t next_witness=64;
    bool tested_hint=false,hit_hint=false,full_scan=false,found_in_scan=false;
    unsigned scan_vertices=0;
};
inline WholeResult whole_witness(uint64_t U,int k,const uint64_t* non,uint8_t hint) {
    WholeResult r;
    if(hint<64) {
        r.tested_hint=true;
        const uint64_t b=1ULL<<hint;
        if((U&b) && __builtin_popcountll(non[hint]&U)>=k) {
            r.whole=false;r.next_witness=hint;r.hit_hint=true;return r;
        }
    }
    r.full_scan=true;
    const uint64_t domain=U;
    while(U) {
        const int u=__builtin_ctzll(U);U&=U-1;++r.scan_vertices;
        if(__builtin_popcountll(non[u]&domain)>=k) {
            r.whole=false;r.next_witness=static_cast<uint8_t>(u);
            r.found_in_scan=true;break;
        }
    }
    return r;
}
} // namespace state64_common
