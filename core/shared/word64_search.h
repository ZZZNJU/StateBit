#pragma once
#include <cstdint>
#include <vector>
#include <utility>
// WORD64: single executable DFS scheduling/state-lifetime implementation for
// ordinary (non-Folding) Word64 tasks in both execution modes. Include-first DFS,
// explicit stack, shared child order, and a common degree/prune transition.
// Framework callbacks retain terminal, handoff, exact state context, and PARALLEL_ENGINE
// parallel task-export semantics. Fold-only backends are deliberately excluded.
namespace statebit_word64_search {
struct WordState {uint64_t P,C,X;};
enum ChildMask: unsigned {None=0, Include=1, Exclude=2, Both=3};
template<class Node, class Expand, class AfterBranch>
inline void dfs(Node root, Expand&& expand, AfterBranch&& after_branch){
    std::vector<Node> stack;
    stack.reserve(256);
    stack.push_back(std::move(root));
    while(!stack.empty()){
        Node cur=std::move(stack.back());
        stack.pop_back();
        Node ex{},in{};
        const unsigned mask=expand(cur,ex,in);
        // Exclude is always deferred behind include, matching both original
        // PARALLEL_ENGINE's LIFO stack and SEQUENTIAL_ENGINE's include-first recursion.
        if(mask&Exclude)stack.push_back(std::move(ex));
        if(mask&Include)stack.push_back(std::move(in));
        if(mask)after_branch(stack);
    }
}
} // namespace statebit_word64
