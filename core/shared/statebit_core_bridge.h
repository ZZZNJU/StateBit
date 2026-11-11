#pragma once
#include <cstdint>
#include <cassert>
// Shared cross-framework Word64 branch construction and width-capacity contract.
// The adapters retain exact framework-specific refinement, early terminal,
// fixed/forced context, State32 handoff and scheduler semantics.
namespace statebit_core {
using Word=uint64_t;
enum class Width : uint8_t { State32=0, Adaptive64=1, Adaptive128=2, Adaptive256=3, List=4 };
inline Width capacity(int active){
    assert(active>=0);
    return active<=32?Width::State32:active<=64?Width::Adaptive64:
           active<=128?Width::Adaptive128:active<=256?Width::Adaptive256:Width::List;
}
inline bool handoff32(Word P,Word C){return capacity(__builtin_popcountll(P|C))==Width::State32;}
struct Branch64 {
    Word pi,ci,xi,pe,ce,xe,bit;
};
// Exactly ONE include and ONE exclude descendant; refine is invoked once.
// This is a shared DFS transition, not a replacement for context-dependent
// terminal / maximality rules.  Preserve the refiner's update ordering.
template<class Refine>
inline Branch64 branch(Word P,Word C,Word X,int pivot,Refine&& refine){
    assert(pivot>=0 && pivot<64 && (C&(Word(1)<<pivot)));
    const Word b=Word(1)<<pivot,ce=C&~b,pi=P|b;
    const Word ci=refine(pi,ce);
    assert(!(ci&~ce));
    return {pi,ci,X,P,ce,X|b,b};
}
} // namespace statebit_core
