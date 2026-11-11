#pragma once
// Shared, non-folding-changing *admission filter* for an ALREADY ELIGIBLE
// State128 / State256 list-to-bit transition. It never changes an existing
// width eligibility check, nor any backend DFS / maximality / fixed context.
// MODE=0 accepts every existing eligible entry; MODE=1 uses a cheap
// structure-dependent admission heuristic. Denied entries remain on the
// original list-search path. This is not an online-learned cost model.
#include <atomic>
#include <cassert>
#include <algorithm>
#include <cstdint>
#include <cstdio>
namespace statebit_wide_admit {
struct Stats {
  std::atomic<uint64_t> eligible128{0}, eligible256{0};
  std::atomic<uint64_t> accept128{0}, accept256{0};
  std::atomic<uint64_t> reject128{0}, reject256{0};
};
inline Stats& stats(){ static Stats v; return v; }
inline bool decide(int active, int candidates, int k, int width){
  assert(active > 64 && active <= 256);
  assert(candidates >= 0 && candidates <= active && k >= 1);
  assert((width == 128 && active <= 128) || (width == 256 && active > 128));
  const int threshold = width == 128 ? std::max(6, 2*k) : std::max(10, 3*k);
  const bool allow = STATEBIT_WIDE_ADMIT_MODE == 0 || candidates >= threshold;
  Stats& s = stats();
  if (width == 128){
    s.eligible128.fetch_add(1,std::memory_order_relaxed);
    (allow ? s.accept128 : s.reject128).fetch_add(1,std::memory_order_relaxed);
  } else {
    s.eligible256.fetch_add(1,std::memory_order_relaxed);
    (allow ? s.accept256 : s.reject256).fetch_add(1,std::memory_order_relaxed);
  }
  return allow;
}
inline uint64_t load(const std::atomic<uint64_t>& x){return x.load(std::memory_order_relaxed);}
inline void report(const char* framework){
  const Stats& s=stats();
  std::printf("WIDE_ADMIT_CONFIG framework=%s mode=%d minC128=max(6,2k) minC256=max(10,3k) fallback=list no_folding_changes=1\n",framework,STATEBIT_WIDE_ADMIT_MODE);
  std::printf("WIDE_ADMIT_STATS eligible128=%llu accept128=%llu reject128=%llu eligible256=%llu accept256=%llu reject256=%llu\n",
    (unsigned long long)load(s.eligible128),(unsigned long long)load(s.accept128),(unsigned long long)load(s.reject128),
    (unsigned long long)load(s.eligible256),(unsigned long long)load(s.accept256),(unsigned long long)load(s.reject256));
}
}
