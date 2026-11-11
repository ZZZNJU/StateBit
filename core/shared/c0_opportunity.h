#ifndef UNIFIED_C0_OPPORTUNITY_H
#define UNIFIED_C0_OPPORTUNITY_H
#include <cstdint>
#include <cstdio>
#include <ctime>
extern bool enableFoldingProbe;
namespace statebit {
// Copies masks; adjacency is read-only. This is an opportunity, not a folding legality proof.
template<class M,class Non,class Any,class First,class Clear>
inline int c0Count(M C,M U,const Non& non,Any any,First first,Clear clear){
 int found=0;
 while(any(C)){const int v=first(C);C=clear(C,v);if(!any(non[v]&U))++found;}
 return found;
}
struct C0Opportunity {
 static constexpr unsigned long long period=1024;
 struct Row {
  unsigned long long seen=0,samples=0,tested=0,positive=0,c0_sum=0,partial=0;
  unsigned long long cross32=0,cross64=0,cross128=0,live_sum=0,after_sum=0;
  unsigned long long outer_sum=0,local_sum=0,scan_cpu_ns=0;
 } rows[8];
 static unsigned long long now(){timespec t{};clock_gettime(CLOCK_THREAD_CPUTIME_ID,&t);return static_cast<unsigned long long>(t.tv_sec)*1000000000ULL+t.tv_nsec;}
 template<class Scan> inline void observe(int width,int live,int candidates,int outer,int local,Scan scan){
  if(!enableFoldingProbe||!candidates||live<=32)return;
  const int bucket=width==64?(live<=40?0:1):(width==128?2:3);
  Row& r=rows[2*bucket+(outer>0)];
  if((r.seen++&(period-1))!=0)return; // first, 1025th, ... per worker/bucket/outer group
  const auto start=now();const int c0=scan();r.scan_cpu_ns+=now()-start;
  const int after=live-c0;
  ++r.samples;r.tested+=candidates;r.c0_sum+=c0;r.positive+=(c0>0);
  r.live_sum+=live;r.after_sum+=after;r.outer_sum+=outer;r.local_sum+=local;
  r.cross32+=(live>32&&after<=32);r.cross64+=(live>64&&after<=64);r.cross128+=(live>128&&after<=128);
  const int boundary=width==64?32:(width==128?64:128);
  r.partial+=(c0>0&&after>boundary);
 }
 void report(int tid)const{
  for(int b=0;b<4;b++)for(int outer=0;outer<2;outer++){
   const Row& r=rows[2*b+outer];
   std::printf("C0_OPPORTUNITY_WORKER id=%d bucket=%s outer=%d period=1024 seen=%llu samples=%llu tested=%llu positive=%llu c0_sum=%llu partial=%llu cross32=%llu cross64=%llu cross128=%llu live_sum=%llu after_sum=%llu outer_sum=%llu local_sum=%llu scan_cpu_ns=%llu\n",tid,b==0?"64_33_40":b==1?"64_41_64":b==2?"128_65_128":"256_129_256",outer,r.seen,r.samples,r.tested,r.positive,r.c0_sum,r.partial,r.cross32,r.cross64,r.cross128,r.live_sum,r.after_sum,r.outer_sum,r.local_sum,r.scan_cpu_ns);
  }
 }
};
extern thread_local C0Opportunity c0Opportunity;
}
#endif
