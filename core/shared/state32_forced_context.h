#pragma once
#include <algorithm>
#include <cstdint>
// Shared logical ForcedBase contract for the both execution modes State32 kernels.
// Only membership/cardinality is normalized here; FORCED_LEFT maximality stays in its existing backend.
struct FORCED_CONTEXTState32ForcedContext {
    int externalCount;
    uint64_t localMask;
    inline int localCount() const {return __builtin_popcountll(localMask);}
    inline int logicalCount() const {return externalCount+localCount();}
    inline int activeLowerBound(const int q) const {return std::max(0,q-logicalCount());}
};
