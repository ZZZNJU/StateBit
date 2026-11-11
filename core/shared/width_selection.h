#pragma once
// WIDTH_POLICY: online complete-subtree-cost admission for already-legal 128/256 entries.
// Never grants an originally illegal width; does not alter Folding or ADAPTIVE64.
#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cassert>
#include "statebit_unified_router.h"
namespace statebit_width_selection {
constexpr int WINDOW=16;
constexpr int BUCKETS=8;
struct Arm {
    double history[WINDOW]{};
    unsigned int size=0, next=0;
    void add(double elapsed){
        if(elapsed<=0) elapsed=1.;
        history[next]=elapsed;next=(next+1u)%WINDOW;
        if(size<WINDOW) ++size;
    }
    double cost() const {
        if(!size) return 0.;
        double tmp[WINDOW]{};
        for(unsigned i=0;i<size;++i) tmp[i]=history[i];
        std::sort(tmp,tmp+size);
        unsigned lo=(size>=8?size/8:0), hi=size-lo;
        double sum=0.;for(unsigned i=lo;i<hi;++i)sum+=tmp[i];
        return sum/(hi-lo);
    }
};
struct Bucket {
    Arm wide,list;
    std::uint64_t requests=0;
};
struct ThreadModel { Bucket b[BUCKETS]; std::uint64_t exported=0; };
inline ThreadModel& tls() { static thread_local ThreadModel s;return s; }
struct Counters {
    std::atomic<std::uint64_t> eligible128{0}, eligible256{0},choose128{0},choose256{0},list128{0},list256{0},
      explored{0},wideSamples{0},listSamples{0},splitExcluded{0},orphan{0};
};
inline Counters& counters(){static Counters s;return s;}
inline void noteExport(){++tls().exported;}
inline bool noteExportOnSuccess(bool sent){if(sent)noteExport();return sent;}
inline int bucket(int active,int cand,int width){
    assert((width==128 && active>64 && active<=128) || (width==256 && active>128 && active<=256));
    const int cls=(cand>=active/2?2:0)+(cand>=24?1:0);
    return (width==256?4:0)+cls;
}
struct Scope;
inline Scope*& scopePtr(){static thread_local Scope* s=nullptr;return s;}
struct Scope {
    Scope* previous=nullptr;
    Bucket* chosenBucket=nullptr;
    bool choseWide=false;
    std::uint64_t exportStart=0;
    std::chrono::steady_clock::time_point started;
    Scope(){
        previous=scopePtr();scopePtr()=this;
    }
    Scope(const Scope&)=delete;Scope& operator=(const Scope&)=delete;
    ~Scope(){
        if(chosenBucket){
            if(tls().exported!=exportStart){counters().splitExcluded.fetch_add(1,std::memory_order_relaxed);}
            else {
                auto delta=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-started).count();
                (choseWide?chosenBucket->wide:chosenBucket->list).add(static_cast<double>(delta));
                (choseWide?counters().wideSamples:counters().listSamples).fetch_add(1,std::memory_order_relaxed);
            }
        }
        scopePtr()=previous;
    }
    void track(Bucket& b,bool wide){
        chosenBucket=&b; choseWide=wide; exportStart=tls().exported;started=std::chrono::steady_clock::now();
    }
};
inline bool admit(int active,int cand,int k,int width){
    // Always honor original structural eligibility and transparent admission.
    const bool original=statebit_unified::admitWide(active,cand,k,width);
    if(!original)return false;
    auto &c=counters();
    (width==128?c.eligible128:c.eligible256).fetch_add(1,std::memory_order_relaxed);
    Bucket &b=tls().b[bucket(active,cand,width)];
    const std::uint64_t step=++b.requests;
    // Balanced short warm-up, plus 1-in-64 low-frequency exploration.
    bool wide=true,explore=false;
    if(b.wide.size<2 || b.list.size<2){wide=(step&1)!=0;explore=true;}
    else if((step&63ULL)==0ULL){wide=(step&64ULL)!=0;explore=true;}
    else {
        // 6% hysteresis in favour of the current wide execution route.
        wide= b.wide.cost() <= b.list.cost()*1.06;
    }
    if(explore)c.explored.fetch_add(1,std::memory_order_relaxed);
    (width==128?(wide?c.choose128:c.list128):(wide?c.choose256:c.list256)).fetch_add(1,std::memory_order_relaxed);
    if(Scope* scope=scopePtr())scope->track(b,wide);
    else c.orphan.fetch_add(1,std::memory_order_relaxed);
    return wide;
}
template<class RunBit,class Run128,class Run256>
inline bool executeOrdinary(const statebit_unified::Shape& s,const statebit_unified::Legal& l,
          RunBit&& bit,Run128&& run128,Run256&& run256){
    const auto r=statebit_unified::plan(s,l);
    if(r==statebit_unified::Route::State32 || r==statebit_unified::Route::Adaptive64){bit();return true;}
    if(r==statebit_unified::Route::State128 && admit(s.active,s.candidates,s.k,128)){run128();return true;}
    if(r==statebit_unified::Route::State256 && admit(s.active,s.candidates,s.k,256)){run256();return true;}
    return false;
}
inline unsigned long long count(const std::atomic<std::uint64_t>& x){return x.load(std::memory_order_relaxed);}
inline void report(const char* host){
    auto& c=counters();
    std::printf("WIDTH_POLICY_CONFIG host=%s enabled=%d policy=online_window16_wide_list_subtree_cost buckets=8 warmup=2each explore_every=64 hysteresis=6pct split_exclusion=1 ADAPTIVE64_unchanged=1 folding_unchanged=1 original_legality=1\n",host,STATEBIT_WIDTH_POLICY_MODE);
    std::printf("WIDTH_POLICY_STATS eligible128=%llu eligible256=%llu wide128=%llu wide256=%llu list128=%llu list256=%llu explored=%llu wide_samples=%llu list_samples=%llu split_excluded=%llu orphan=%llu\n",count(c.eligible128),count(c.eligible256),count(c.choose128),count(c.choose256),count(c.list128),count(c.list256),count(c.explored),count(c.wideSamples),count(c.listSamples),count(c.splitExcluded),count(c.orphan));
}
} // namespace statebit_width_policy
