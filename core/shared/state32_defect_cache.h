#ifndef DEFECT_CACHE_STATE32_DEFECT_CACHE_H
#define DEFECT_CACHE_STATE32_DEFECT_CACHE_H
#include <vector>
#include <cstdint>
#include <cassert>
#include <algorithm>

// Common State32 layout for both families: missing-edge penalties, not
// adjacency counts, stored in dynamically-sized external/Left vectors.
// The SEQUENTIAL_ENGINE local-F overlay is lazy and generation stamped, in the same object.
struct DEFECT_CACHEState32DefectCache {
    std::vector<int> externalMissing, leftMissing;
    std::vector<int> overlayExternalMissing, overlayLeftMissing;
    std::vector<std::uint64_t> overlayExternalStamp, overlayLeftStamp;
    std::uint64_t generation=1;
    std::uint64_t overlayMask=0;
    int overlayCount=0;
    bool ready=false;
    int sourceForcedCount=-1;
    int sourceExternalN=-1, sourceLeftN=-1;
    void invalidate(){ready=false;}
    template<class E, class L>
    void fromAdjacency(int forced, int ne, const E& extAdj, int nl, const L& leftAdj){
        assert(forced>=0 && ne>=0 && nl>=0);
        externalMissing.resize(static_cast<std::size_t>(ne));
        leftMissing.resize(static_cast<std::size_t>(nl));
        for(int i=0;i<ne;++i){
            const int v=std::max(0,forced-static_cast<int>(extAdj[i]));
            assert(v>=0);externalMissing[i]=v;
        }
        for(int i=0;i<nl;++i){
            const int v=std::max(0,forced-static_cast<int>(leftAdj[i]));
            assert(v>=0);leftMissing[i]=v;
        }
        sourceForcedCount=forced;sourceExternalN=ne;sourceLeftN=nl;
        ready=true;
    }
    template<class E, class L>
    void fromMissing(int forced, int ne, const E& extMiss, int nl, const L& leftMiss){
        assert(forced>=0 && ne>=0 && nl>=0);
        externalMissing.assign(extMiss.begin(),extMiss.begin()+ne);
        leftMissing.assign(leftMiss.begin(),leftMiss.begin()+nl);
        for(const auto v:externalMissing)assert(v>=0 && v<=forced);
        for(const auto v:leftMissing)assert(v>=0 && v<=forced);
        sourceForcedCount=forced;sourceExternalN=ne;sourceLeftN=nl;
        ready=true;
    }
    int external(int i) const {assert(ready && i>=0 && i<sourceExternalN);return externalMissing[static_cast<std::size_t>(i)];}
    int left(int i) const {assert(ready && i>=0 && i<sourceLeftN);return leftMissing[static_cast<std::size_t>(i)];}
    void beginOverlay(std::uint64_t F){
        if(++generation==0){
            std::fill(overlayExternalStamp.begin(),overlayExternalStamp.end(),0ULL);
            std::fill(overlayLeftStamp.begin(),overlayLeftStamp.end(),0ULL);
            generation=1;
        }
        overlayMask=F;overlayCount=__builtin_popcountll(F);
    }
    bool overlayExternalCached(int i) const {return i>=0 && static_cast<std::size_t>(i)<overlayExternalStamp.size() && overlayExternalStamp[i]==generation;}
    bool overlayLeftCached(int i) const {return i>=0 && static_cast<std::size_t>(i)<overlayLeftStamp.size() && overlayLeftStamp[i]==generation;}
    int overlayExternal(int i,std::uint64_t row){
        assert(i>=0);
        const auto x=static_cast<std::size_t>(i);
        if(overlayExternalStamp.size()<=x){overlayExternalStamp.resize(x+1,0);overlayExternalMissing.resize(x+1,0);}
        if(overlayExternalStamp[x]!=generation){
            overlayExternalMissing[x]=overlayCount-__builtin_popcountll(row&overlayMask);
            overlayExternalStamp[x]=generation;
        }
        return overlayExternalMissing[x];
    }
    int overlayLeft(int i,std::uint64_t row){
        assert(i>=0);
        const auto x=static_cast<std::size_t>(i);
        if(overlayLeftStamp.size()<=x){overlayLeftStamp.resize(x+1,0);overlayLeftMissing.resize(x+1,0);}
        if(overlayLeftStamp[x]!=generation){
            overlayLeftMissing[x]=overlayCount-__builtin_popcountll(row&overlayMask);
            overlayLeftStamp[x]=generation;
        }
        return overlayLeftMissing[x];
    }
};
#endif
