#pragma once
#include <cstdint>
#include <cstdio>
#include <ctime>
#include "fresh64_policy.h"
#include "remat_budget.h"
#include "folding_variant.h"
extern int entryFoldingPolicy;
extern bool enableLocalFolding;
extern bool enableLocalRematerialization;
extern bool reuseRematerializationStorage;
extern int kernelSelectionMode;
extern int rematerializationMinSlack;
namespace experiment_control {
inline const char* selectorName(){
 switch(kernelSelectionMode){case 0:return "legacy_hybrid4";case 1:return "adaptive64";case 2:return "fixed_word64";case 3:return "fixed_state64";default:return "invalid";}
}
// Diagnostic task selector only: no model training or sampled observations.
struct Fixed64Policy {
 struct Decision {bool simd;bool sample;};
 bool state;
 explicit Fixed64Policy(bool value):state(value){}
 Decision select(int,int,int){return {state,false};}
 void complete(const Decision&,std::uint64_t,std::uint64_t,bool){}
};
inline adaptive64::Policy& adaptivePolicy(){static thread_local adaptive64::Policy p;return p;}
enum Metric {ContextBuild,Word64,State64,State32Plain,State32LocalF,RematContextBuild,RematSubtree,MetricCount};
struct Counter {std::uint64_t calls=0,samples=0,cpu_ns=0;};
struct Samples {
 Counter counters[MetricCount];
 std::uint64_t remat_checks=0,remat_converted=0,remat_f_vertices=0;
 std::uint64_t remat_width32=0,remat_with_basex=0,remat_restored=0;
 std::uint64_t pool_acquisitions=0,pool_fresh=0,pool_reused=0,pool_fallbacks=0;
 std::uint64_t pool_grow_events=0,pool_vector_grows=0,pool_peak_capacity=0;
 std::uint64_t gate_contexts=0,gate_width_impossible=0,gate_eligible=0,gate_denied=0,gate_allowed=0;
 std::uint64_t gate_slack[8]{}; // negative,0,1,2,3..4,5..8,9..16,17+
 std::uint64_t remat_c[7]{}; // 0,1,2,3..4,5..8,9..16,17..32
 std::uint64_t wide_fold_scans=0,wide_fold_nodes=0,wide_fold_vertices=0;
 std::uint64_t entry_fold_checks=0,entry_fold_tested=0,entry_fold_nodes=0,entry_fold_vertices=0,entry_fold_le32=0,entry_fold_gt32=0;
 std::uint64_t entry_commits=0,entry_rejects=0,entry_reject_vertices=0;

};
inline Samples& samples(){static thread_local Samples s;return s;}
inline std::uint64_t cpuNow(){timespec ts;clock_gettime(CLOCK_THREAD_CPUTIME_ID,&ts);return std::uint64_t(ts.tv_sec)*1000000000ULL+ts.tv_nsec;}
struct SampledSpan {
 Counter* counter;std::uint64_t start=0;bool active;
 explicit SampledSpan(Metric metric):counter(&samples().counters[metric]),active((++counter->calls&1023ULL)==1){
  if(active){++counter->samples;start=cpuNow();}
 }
 SampledSpan(const SampledSpan&)=delete;SampledSpan& operator=(const SampledSpan&)=delete;
 ~SampledSpan(){if(active)counter->cpu_ns+=cpuNow()-start;}
};
inline void report(int id){
 const char* names[]={"context_build","word64_root","state64_root","state32_plain","state32_local_f","remat_context_build","remat_subtree"};
 for(int i=0;i<MetricCount;++i){const auto& c=samples().counters[i];
  std::printf("KERNEL_SAMPLE_WORKER id=%d metric=%s calls=%llu samples=%llu cpu_ns=%llu period=1024 inclusive=1\n",id,names[i],(unsigned long long)c.calls,(unsigned long long)c.samples,(unsigned long long)c.cpu_ns);
 }
 const auto& s=samples();
 std::printf("FOLD_VARIANT_WORKER id=%d variant=%d wide_scans=%llu wide_fold_nodes=%llu wide_vertices=%llu entry_checks=%llu entry_tested=%llu entry_fold_nodes=%llu entry_vertices=%llu entry_le32=%llu entry_gt32=%llu entry_commits=%llu entry_rejects=%llu entry_reject_vertices=%llu\n",id,entryFoldingPolicy,
  (unsigned long long)s.wide_fold_scans,(unsigned long long)s.wide_fold_nodes,(unsigned long long)s.wide_fold_vertices,
  (unsigned long long)s.entry_fold_checks,(unsigned long long)s.entry_fold_tested,(unsigned long long)s.entry_fold_nodes,(unsigned long long)s.entry_fold_vertices,
  (unsigned long long)s.entry_fold_le32,(unsigned long long)s.entry_fold_gt32,
  (unsigned long long)s.entry_commits,(unsigned long long)s.entry_rejects,(unsigned long long)s.entry_reject_vertices);
 std::printf("LOCAL_F_REMAT_WORKER id=%d enabled=%d checks=%llu converted=%llu f_vertex_events=%llu width32=%llu with_basex=%llu restored=%llu\n",id,enableLocalRematerialization?1:0,
  (unsigned long long)s.remat_checks,(unsigned long long)s.remat_converted,(unsigned long long)s.remat_f_vertices,
  (unsigned long long)s.remat_width32,(unsigned long long)s.remat_with_basex,(unsigned long long)s.remat_restored);
 std::printf("REMAT_POOL_WORKER id=%d reuse=%d acquisitions=%llu fresh=%llu reused=%llu fallbacks=%llu grow_events=%llu vector_grows=%llu peak_capacity=%llu c0=%llu c1=%llu c2=%llu c3_4=%llu c5_8=%llu c9_16=%llu c17_32=%llu\n",id,reuseRematerializationStorage?1:0,
  (unsigned long long)s.pool_acquisitions,(unsigned long long)s.pool_fresh,(unsigned long long)s.pool_reused,(unsigned long long)s.pool_fallbacks,
  (unsigned long long)s.pool_grow_events,(unsigned long long)s.pool_vector_grows,(unsigned long long)s.pool_peak_capacity,
  (unsigned long long)s.remat_c[0],(unsigned long long)s.remat_c[1],(unsigned long long)s.remat_c[2],(unsigned long long)s.remat_c[3],
  (unsigned long long)s.remat_c[4],(unsigned long long)s.remat_c[5],(unsigned long long)s.remat_c[6]);
 std::printf("REMAT_BUDGET_WORKER id=%d minimum=%d contexts=%llu width_impossible=%llu eligible=%llu denied=%llu allowed=%llu sneg=%llu s0=%llu s1=%llu s2=%llu s3_4=%llu s5_8=%llu s9_16=%llu s17plus=%llu\n",id,rematerializationMinSlack,
  (unsigned long long)s.gate_contexts,(unsigned long long)s.gate_width_impossible,(unsigned long long)s.gate_eligible,(unsigned long long)s.gate_denied,(unsigned long long)s.gate_allowed,
  (unsigned long long)s.gate_slack[0],(unsigned long long)s.gate_slack[1],(unsigned long long)s.gate_slack[2],(unsigned long long)s.gate_slack[3],
  (unsigned long long)s.gate_slack[4],(unsigned long long)s.gate_slack[5],(unsigned long long)s.gate_slack[6],(unsigned long long)s.gate_slack[7]);
 auto& p=adaptivePolicy();
 std::printf("EXPERIMENT_POLICY_WORKER id=%d selector=%s adaptive_observations=%llu adaptive_split_skips=%llu\n",id,selectorName(),(unsigned long long)p.observations,(unsigned long long)p.split_skips);
}
}
