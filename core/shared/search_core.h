#pragma once
// CORE: one executable ordinary Word64 per-node DFS transaction and one List
// feasibility-scan algorithm, instantiated by both execution modes frontends.
// Framework adapters supply representation-dependent terminal/forced context,
// width handoff, telemetry and task donation; they DO NOT own the DFS branch
// transaction, child publication, or List feasibility scan.
#include <cstdint>
#include <utility>
#include "word64_search.h"
#include "statebit_core_bridge.h"
namespace statebit_search_core {

// This is the only ordinary Word64 DFS expand implementation. In particular
// the two frontends cannot diverge in child ordering, include/exclude
// publication, pivot invocation order, or branch/refine construction.
template<class State,class Prepare,class Pivot,class Refine,class BuildChild,
         class AllowInclude,class NoteBranch,class AfterBranch>
inline void word64_dfs(const State& root,Prepare&& prepare,Pivot&& pivot,
    Refine&& refine,BuildChild&& child,AllowInclude&& allow,
    NoteBranch&& note,AfterBranch&& after){
    statebit_word64_search::dfs<State>(root,
        [&](const State& incoming,State& exclude,State& include)->unsigned {
            State ready=incoming;
            if(!prepare(ready))return statebit_word64_search::None;
            const int p=pivot(ready);
            if(p<0)return statebit_word64_search::None;
            const auto branch=statebit_core::branch(ready.P,ready.C,ready.X,p,
                [&](uint64_t pi,uint64_t cm){return refine(ready,p,pi,cm);});
            const bool takeInclude=allow(ready,p,branch);
            exclude=child(ready,branch,false);
            if(takeInclude)include=child(ready,branch,true);
            note(takeInclude);
            return takeInclude?statebit_word64_search::Both:statebit_word64_search::Exclude;
        }, std::forward<AfterBranch>(after));
}

enum class PreUpdateResult : unsigned char { SizeMiss, PlexMiss, CandidateMiss, Pass };
struct PreUpdateScan {
    PreUpdateResult outcome;
    int scannedP;
    int scannedC;
};
// Ordinary branch direct-terminal precheck: one shared scan, irrespective
// of the host's instrumentation. Preserves short circuit and scan order.
template<class PlexVertex,class CandidateVertex,class Degree>
inline PreUpdateScan preupdate(int plexSize,int candSize,int lb,int k,
                               PlexVertex&& plex,CandidateVertex&& cand,Degree&& deg){
    const int active=plexSize+candSize;
    if(active<lb)return {PreUpdateResult::SizeMiss,0,0};
    for(int i=0;i<plexSize;++i)
        if(deg(plex(i))+k<active)return {PreUpdateResult::PlexMiss,i+1,0};
    for(int i=0;i<candSize;++i)
        if(deg(cand(i))+k<active)return {PreUpdateResult::CandidateMiss,plexSize,i+1};
    return {PreUpdateResult::Pass,plexSize,candSize};
}
} // namespace statebit_search_core

// ADAPTIVE64 is more than a shared policy type: this is its ONLY runnable
// task-scoped execute/measure/complete sequence. PARALLEL_ENGINE supplies a split epoch;
// SEQUENTIAL_ENGINE supplies a zero epoch (no worker-task donation).
#include <chrono>
#include "fresh64_policy.h"
namespace statebit_search_core {
template<class Policy,class ReadNodes,class ReadSplit,class OnDecision,class OnCompletion,
         class RunWord,class RunState>
inline void adaptive64_task(Policy& model,int active,int cand,int lb,
     ReadNodes&& readNodes,ReadSplit&& readSplit,OnDecision&& decisionHook,
     OnCompletion&& completeHook,RunWord&& word,RunState&& state){
    const auto decision=model.select(active,cand,lb);
    decisionHook(decision.simd,decision.sample);
    const auto before=decision.sample?readNodes():0;
    const auto epoch=decision.sample?readSplit():0;
    const auto t0=decision.sample?std::chrono::steady_clock::now()
        :std::chrono::steady_clock::time_point{};
    if(decision.simd)state();else word();
    if(decision.sample){
        const auto count=readNodes()-before;
        const auto ns=std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now()-t0).count();
        const bool split=readSplit()!=epoch;
        model.complete(decision,static_cast<uint64_t>(ns>0?ns:0),count,split);
        completeHook(split);
    }
}
} // namespace statebit_core
