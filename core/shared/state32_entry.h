#pragma once
#include <cstdint>
// STATE32_ENTRY common *root-admission decision* used by both execution modes.
// This does NOT replace State64 prior screening or terminal emitters.
namespace state32_entry_state32_entry {
enum class Decision : unsigned char { Reject, Whole, Leaf, Enter };
inline Decision examine(uint64_t P, uint64_t C, int effective_lb, int k,
                        const uint64_t* bit_non) {
    const uint64_t U=P|C;
    if (__builtin_popcountll(U)<effective_lb) return Decision::Reject;
    bool whole=true;
    for(uint64_t t=U;t;t&=t-1) {
        const int u=__builtin_ctzll(t);
        if (__builtin_popcountll(bit_non[u]&U)>=k) {whole=false;break;}
    }
    if (whole) return Decision::Whole;
    if (!C) return Decision::Leaf;
    return Decision::Enter;
}
}
