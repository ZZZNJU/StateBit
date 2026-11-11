#pragma once
#include <algorithm>
#include <vector>
#include <climits>
// LIST non-Folding LIST DFS control machine. This is compiled once per execution
// variant, but the same source is instantiated by PARALLEL_ENGINE sequential, PARALLEL_ENGINE parallel,
// and SEQUENTIAL_ENGINE sequential. Folding and scheduling are adapter callbacks only.
namespace statebit_list_search {

// Owns the seed-level kSearch recursive traversal and all undo bookkeeping.
// Hooks are limited to terminal prechecks and existing variant diagnostics.
// No Folding transition or task scheduling decision is made in this function.
template<class Owner, class OnEntry, class OnEmpty, class OnDirect>
inline void ksearch(Owner& h,int res,int lowerBound,OnEntry onEntry,OnEmpty onEmpty,OnDirect onDirect) {
    onEntry();
    int recExcl=0,recExclTmp=0;
    std::vector<int> recCand1(static_cast<size_t>(res)+1,0);
    std::vector<int> recCand2(static_cast<size_t>(res)+1,0);
    if(h.cand2->sz==0){
        onEmpty();
        if(h.plex->sz+h.cand1->sz<lowerBound) return;
        if(h.plex->sz>1&&!h.upperboundK()) return;
        h.solveCurrentCandidateSpace();
        return;
    }
    int v2delete=h.cand2BackToExcl();
    h.kSearch(res);
    h.exclToCand2(v2delete);
    int br=1;
    for(;br<res;++br){
        const int v2add=h.cand2BackToPlex();
        h.updateCand1KFake(recCand1[br],v2add);
        h.updateCand2Fake(recCand2[br],v2add);
        h.updateExclK(recExclTmp,v2add);
        recExcl+=recExclTmp;
        if(h.cand2->sz){
            v2delete=h.cand2BackToExcl();
            h.kSearch(res-br);
            h.exclToCand2(v2delete);
        }else{
            h.kSearch(res-br);
            break;
        }
    }
    if(br==res){
        recCand2[br]=0;
        const int v2add=h.cand2BackToPlex();
        if(onDirect()){
            recCand1[br]=0;
            auto* old=h.excl; h.excl=h.exclBK; h.checkAsMaxPlex(); h.excl=old;
        }else{
            h.updateCand1Fake(recCand1[br],v2add);
            if(h.plex->sz+h.cand1->sz>=lowerBound &&
               (h.plex->sz<=1 || h.upperboundK())){
                auto* old=h.excl; h.excl=h.exclBK;
                h.solveCurrentCandidateSpace();
                h.excl=old;
            }
        }
    }
    for(int i=br;i>=1;--i){
        h.cand1->fakeRecoverAdd(recCand1[i],&h);
        h.cand2->fakeRecover(recCand2[i]);
        h.plexToCand2();
    }
    for(int i=0;i<recExcl;++i){
        h.excl->add(h.exclStack->top());
        h.exclStack->pop();
    }
}

// Owns the two-child LIST branching transaction and ensures the same rollback
// order under both implementations. The branch/UB/parallel work is a hook.
template<class Owner, class IncludeChild, class ExcludeChild>
inline void branch(Owner& h,int pivot,IncludeChild includeChild,ExcludeChild excludeChild){
    int recCand1=0,recExcl=0;
    h.cand1ToPlex(pivot)->updateCand1Fake(recCand1,pivot);
    includeChild(recExcl);
    h.cand1->fakeRecoverAdd(recCand1,&h);
    h.plexToCand1();
    while(recExcl){
        h.excl->add(h.exclStack->top());
        h.exclStack->pop();
        --recExcl;
    }
    h.cand1ToExcl(pivot);
    excludeChild();
    h.exclToCand1(pivot);
}

// The identical non-Folding LIST pivot, min-degree/neighbor tie breaking, and
// terminal decision. logicalSize is adapted to the caller's existing context.
template<class Owner>
inline void list_pivot(Owner& h,int logicalSize,int k,int lb){
    int minneiP=INT_MAX;
    int pivot=0;
    h.plex->for_each([&](int u){
        if(h.getNeiInG(u)<minneiP){minneiP=h.getNeiInG(u);pivot=u;}
    });
    const int pivotP=pivot;
    if(minneiP+k<std::max(lb,logicalSize))return;
    if(minneiP+k<logicalSize+h.cand1->sz){
        int minneiC=INT_MAX;
        h.cand1->for_each([&](int u){
            if(!h.isAdjMtx(u,pivotP)){
                if(h.getNeiInG(u)<minneiC){minneiC=h.getNeiInG(u);pivot=u;}
                else if(h.getNeiInG(u)==minneiC&&h.getNeiInP(u)<h.getNeiInP(pivot))pivot=u;
            }
        });
        h.branchInCand(pivot);
        return;
    }
    int minnei=minneiP;
    h.cand1->for_each([&](int u){
        if(h.getNeiInG(u)<minnei){minnei=h.getNeiInG(u);pivot=u;}
        else if(h.getNeiInG(u)==minnei&&h.getNeiInP(u)<h.getNeiInP(pivot))pivot=u;
    });
    if(minnei>=logicalSize+h.cand1->sz-k){h.checkAsMaxPlex();return;}
    h.branchInCand(pivot);
}
}
