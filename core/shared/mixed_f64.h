#pragma once
#include <cstdint>
#include <cstdio>
#include <string>
#include <ctime>
extern bool enableMixedFixedContext;
namespace mixed_f64 {
inline uint64_t checks(uint64_t C,uint64_t mask,bool root,bool initialized){return root||!initialized?C:C&mask;}
template<class Rows> inline uint64_t affected(uint64_t removed,uint64_t C,const Rows& non){uint64_t ans=0;while(removed){int v=__builtin_ctzll(removed);removed&=removed-1;ans|=non[v];}return ans&C;}
template<class Rows> inline bool pivotBound(uint64_t P,uint64_t U,int k,int lb,const Rows& adj){
 if(!P)return false;const int bound=lb>__builtin_popcountll(P)?lb:__builtin_popcountll(P);
 while(P){int v=__builtin_ctzll(P);P&=P-1;if(__builtin_popcountll(adj[v]&U)+k<bound)return false;}return true;
}
#define MIXED_FIELDS(X) X(ready) X(width_skip) X(degree_skip) X(empty_check_skip) X(scans) X(tested) X(zero) X(fold_nodes) X(fold_vertices) X(partial) X(to32) X(f_states) X(affected_calls) X(affected_removed) X(local_contexts) X(local_restored) X(local_prepare) X(local_max_batches) X(local_max_slots) X(penalty_lookups) X(penalty_misses) X(cache_grows) X(cache_begins) X(cache_samples) X(cache_sample_ns) X(penalty_samples) X(penalty_sample_ns) X(materialized_roots) X(materialized_vertices) X(scan_samples) X(scan_sample_ns) X(affected_samples) X(affected_sample_ns) X(local_max_samples) X(local_max_sample_ns)
struct Stats {
#define DECL(n) unsigned long long n=0;
 MIXED_FIELDS(DECL)
#undef DECL
 void report(int id)const{
  std::string s="MIXED_F64_WORKER id="+std::to_string(id)+" enabled="+std::to_string(enableMixedFixedContext?1:0)+" period=1024";
#define APPEND(n) s+=" " #n "="+std::to_string(n);
  MIXED_FIELDS(APPEND)
#undef APPEND
  std::printf("%s\n",s.c_str());
 }
};
inline Stats& stats(){static thread_local Stats s;return s;}
inline unsigned long long cpu(){timespec t{};clock_gettime(CLOCK_THREAD_CPUTIME_ID,&t);return (unsigned long long)t.tv_sec*1000000000ULL+t.tv_nsec;}
struct Span{
 unsigned long long *ns;unsigned long long start=0;
 Span(unsigned long long calls,unsigned long long& samples,unsigned long long& sum):ns((calls&1023ULL)==1?&sum:nullptr){if(ns){++samples;start=cpu();}}
 ~Span(){if(ns)*ns+=cpu()-start;}
};
inline void materialized(uint64_t F,int outer){if(F&&outer&&enableMixedFixedContext){++stats().materialized_roots;stats().materialized_vertices+=__builtin_popcountll(F);}}
}
