#ifndef STATE32_FORCED_LEFT_FORCED_LEFT2D_H
#define STATE32_FORCED_LEFT_FORCED_LEFT2D_H
// FORCED_LEFT common ForcedBase-aware Left maximality rejection engine.
// State32 AVX2 8x32 batch; each candidate starts in a defect plane
// indexed by its missing edges to immutable ForcedBase vertices.
// Returned lane bit=1 means some Left vertex can still be added => not maximal.
#include <immintrin.h>
#include <cstdint>

namespace forced_left_forced_left2d {
static inline uint8_t nonzero_lanes(const __m256i v) {
    const __m256i z=_mm256_setzero_si256();
    return static_cast<uint8_t>(~_mm256_movemask_ps(_mm256_castsi256_ps(_mm256_cmpeq_epi32(v,z))));
}
static inline __m256i maskvec(uint8_t mask) {
    alignas(32) uint32_t lanes[8];
    for(int i=0;i<8;i++) lanes[i]=(mask & (1u<<i)) ? 0xffffffffu : 0u;
    return _mm256_load_si256(reinterpret_cast<const __m256i*>(lanes));
}
template<int K,class RowAt,class MissingAt>
static inline uint8_t eval_k(const __m256i Sv,const __m256i satV,uint8_t liveMask,
    uint32_t unionS,int leftSz,const RowAt &leftNonRow,const MissingAt &forcedMissing) {
    static_assert(K>=1 && K<=8,"K must be in 1..8");
    if(!liveMask || leftSz<=0) return 0;
    uint8_t pending=liveMask, rejectAll=0;
    const __m256i zero=_mm256_setzero_si256();
    const int chunks=(leftSz+31)/32;
    for(int c=0;c<chunks && pending;c++){
        const int first=c*32, last=(first+32<leftSz ? first+32 : leftSz);
        uint32_t init[K]={};
        // Cache-independent initialization: the caller supplies exact
        // non-neighbor count to the full immutable ForcedBase for each Left.
        for(int l=first;l<last;l++){
            const int miss=forcedMissing(l);
            if(miss>=0 && miss<K) init[miss]|=uint32_t(1)<<(l-first);
        }
        const __m256i pendingV=maskvec(pending);
        __m256i def[K];
        for(int t=0;t<K;t++)
            def[t]=_mm256_and_si256(_mm256_set1_epi32(static_cast<int>(init[t])),pendingV);
        uint32_t todo=unionS;
        while(todo){
            const int u=__builtin_ctz(todo); todo&=todo-1;
            const __m256i ub=_mm256_set1_epi32(static_cast<int>(uint32_t(1)<<u));
            const __m256i inS=_mm256_and_si256(pendingV,
                _mm256_cmpeq_epi32(_mm256_and_si256(Sv,ub),ub));
            if(!nonzero_lanes(inS)) continue;
            const __m256i inSat=_mm256_cmpeq_epi32(_mm256_and_si256(satV,ub),ub);
            const __m256i saturated=_mm256_and_si256(inS,inSat);
            const __m256i unsaturated=_mm256_andnot_si256(inSat,inS);
            const __m256i non=_mm256_set1_epi32(static_cast<int>(leftNonRow(u,c)));
            const __m256i conflict=_mm256_and_si256(non,saturated);
            for(int t=0;t<K;t++)def[t]=_mm256_andnot_si256(conflict,def[t]);
            const __m256i step=_mm256_and_si256(non,unsaturated);
            for(int t=K-1;t>=1;t--)
                def[t]=_mm256_or_si256(_mm256_andnot_si256(step,def[t]),_mm256_and_si256(def[t-1],step));
            def[0]=_mm256_andnot_si256(step,def[0]);
            __m256i alive=zero;
            for(int t=0;t<K;t++)alive=_mm256_or_si256(alive,def[t]);
            if(!(nonzero_lanes(alive)&pending)) break;
        }
        __m256i alive=zero;
        for(int t=0;t<K;t++)alive=_mm256_or_si256(alive,def[t]);
        const uint8_t rejected=nonzero_lanes(alive)&pending;
        rejectAll=static_cast<uint8_t>(rejectAll|rejected);
        pending=static_cast<uint8_t>(pending&~rejected);
    }
    return rejectAll;
}
template<class RowAt,class MissingAt>
static inline uint8_t evaluate(int k,const __m256i Sv,const __m256i satV,uint8_t liveMask,
    uint32_t unionS,int leftSz,const RowAt &leftNonRow,const MissingAt &forcedMissing){
    switch(k){
    case 1:return eval_k<1>(Sv,satV,liveMask,unionS,leftSz,leftNonRow,forcedMissing);
    case 2:return eval_k<2>(Sv,satV,liveMask,unionS,leftSz,leftNonRow,forcedMissing);
    case 3:return eval_k<3>(Sv,satV,liveMask,unionS,leftSz,leftNonRow,forcedMissing);
    case 4:return eval_k<4>(Sv,satV,liveMask,unionS,leftSz,leftNonRow,forcedMissing);
    case 5:return eval_k<5>(Sv,satV,liveMask,unionS,leftSz,leftNonRow,forcedMissing);
    case 6:return eval_k<6>(Sv,satV,liveMask,unionS,leftSz,leftNonRow,forcedMissing);
    case 7:return eval_k<7>(Sv,satV,liveMask,unionS,leftSz,leftNonRow,forcedMissing);
    case 8:return eval_k<8>(Sv,satV,liveMask,unionS,leftSz,leftNonRow,forcedMissing);
    default:return 0; // caller MUST select original scalar fallback for k>8
    }
}
// Same State32 defect-plane transition as eval_k, but initial Left masks
// are precomputed by the owner and reused only during that owner's context.
// prepare(c) validates/inserts an immutable ForcedBase-dependent chunk;
// init_at(t,c) reads its cached starting deficit plane.
template<int K,class RowAt,class Prepare,class InitAt>
static inline uint8_t eval_k_cached(const __m256i Sv,const __m256i satV,uint8_t liveMask,
    uint32_t unionS,int leftSz,const RowAt &leftNonRow,
    const Prepare &prepare,const InitAt &initAt) {
    static_assert(K>=1 && K<=8,"K must be in 1..8");
    if(!liveMask || leftSz<=0) return 0;
    uint8_t pending=liveMask,rejectAll=0;
    const __m256i zero=_mm256_setzero_si256();
    const int chunks=(leftSz+31)/32;
    for(int c=0;c<chunks && pending;c++){
        prepare(c);
        const __m256i pendingV=maskvec(pending);
        __m256i def[K];
        for(int t=0;t<K;t++)
            def[t]=_mm256_and_si256(_mm256_set1_epi32(static_cast<int>(initAt(t,c))),pendingV);
        uint32_t todo=unionS;
        while(todo){
            const int u=__builtin_ctz(todo); todo&=todo-1;
            const __m256i ub=_mm256_set1_epi32(static_cast<int>(uint32_t(1)<<u));
            const __m256i inS=_mm256_and_si256(pendingV,
                _mm256_cmpeq_epi32(_mm256_and_si256(Sv,ub),ub));
            if(!nonzero_lanes(inS)) continue;
            const __m256i inSat=_mm256_cmpeq_epi32(_mm256_and_si256(satV,ub),ub);
            const __m256i saturated=_mm256_and_si256(inS,inSat);
            const __m256i unsaturated=_mm256_andnot_si256(inSat,inS);
            const __m256i non=_mm256_set1_epi32(static_cast<int>(leftNonRow(u,c)));
            const __m256i conflict=_mm256_and_si256(non,saturated);
            for(int t=0;t<K;t++)def[t]=_mm256_andnot_si256(conflict,def[t]);
            const __m256i step=_mm256_and_si256(non,unsaturated);
            for(int t=K-1;t>=1;t--)
                def[t]=_mm256_or_si256(_mm256_andnot_si256(step,def[t]),_mm256_and_si256(def[t-1],step));
            def[0]=_mm256_andnot_si256(step,def[0]);
            __m256i alive=zero;
            for(int t=0;t<K;t++)alive=_mm256_or_si256(alive,def[t]);
            if(!(nonzero_lanes(alive)&pending)) break;
        }
        __m256i alive=zero;
        for(int t=0;t<K;t++)alive=_mm256_or_si256(alive,def[t]);
        const uint8_t rejected=nonzero_lanes(alive)&pending;
        rejectAll=static_cast<uint8_t>(rejectAll|rejected);
        pending=static_cast<uint8_t>(pending&~rejected);
    }
    return rejectAll;
}
template<class RowAt,class Prepare,class InitAt>
static inline uint8_t evaluate_cached(int k,const __m256i Sv,const __m256i satV,uint8_t liveMask,
    uint32_t unionS,int leftSz,const RowAt &leftNonRow,
    const Prepare &prepare,const InitAt &initAt){
    switch(k){
    case 1:return eval_k_cached<1>(Sv,satV,liveMask,unionS,leftSz,leftNonRow,prepare,initAt);
    case 2:return eval_k_cached<2>(Sv,satV,liveMask,unionS,leftSz,leftNonRow,prepare,initAt);
    case 3:return eval_k_cached<3>(Sv,satV,liveMask,unionS,leftSz,leftNonRow,prepare,initAt);
    case 4:return eval_k_cached<4>(Sv,satV,liveMask,unionS,leftSz,leftNonRow,prepare,initAt);
    case 5:return eval_k_cached<5>(Sv,satV,liveMask,unionS,leftSz,leftNonRow,prepare,initAt);
    case 6:return eval_k_cached<6>(Sv,satV,liveMask,unionS,leftSz,leftNonRow,prepare,initAt);
    case 7:return eval_k_cached<7>(Sv,satV,liveMask,unionS,leftSz,leftNonRow,prepare,initAt);
    case 8:return eval_k_cached<8>(Sv,satV,liveMask,unionS,leftSz,leftNonRow,prepare,initAt);
    default:return 0; // caller still supplies scalar fallback for >8
    }
}
// FORCED_LEFT_SCALAR: process Left vertices in order, stop when every surviving
// search lane is ruled non-maximal.  RowCanAdd tests 8 lanes together.
// The returned mask marks lanes rejected by at least one Left vertex.
template<class RowCanAdd>
static inline uint8_t evaluate_scalar(const int leftSz,const uint8_t liveMask,
                                      const RowCanAdd &rowCanAdd){
    uint8_t remaining=liveMask;
    for(int l=0;l<leftSz && remaining;l++){
        const uint8_t reject=rowCanAdd(l,remaining);
        remaining=static_cast<uint8_t>(remaining & ~reject);
    }
    return static_cast<uint8_t>(liveMask & ~remaining);
}
} // namespace forced_left_forced_left2d
#endif
