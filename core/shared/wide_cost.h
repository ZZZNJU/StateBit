#pragma once
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <ctime>
extern bool enableWideCostSampling;
namespace wide_cost {
inline bool allow256(int outer,bool foldingGate,bool directEmpty){return outer>0?foldingGate:directEmpty;}
enum Kind {List128,List256,Resume128,Resume256,KindCount};
enum Phase {Other,Build128,Build256,Search128,Search256,Map128_64,Map256_128,Lower64,Lower32,Snapshot128,Snapshot256,Restore128,Restore256,PhaseCount};
inline const char* kindName(int i){const char* n[]={"list128","list256","resume128","resume256"};return n[i];}
inline const char* phaseName(int i){const char* n[]={"other","build128","build256","search128","search256","map128_64","map256_128","lower64","lower32","snapshot128","snapshot256","restore128","restore256"};return n[i];}
inline unsigned long long cpu(){timespec t{};clock_gettime(CLOCK_THREAD_CPUTIME_ID,&t);return (unsigned long long)t.tv_sec*1000000000ULL+t.tv_nsec;}
struct WindowCounter{unsigned long long calls=0,samples=0,ns=0;};
struct StageCounter{unsigned long long entries=0,ns=0;};
struct Tracker {
 using Clock=unsigned long long(*)();Clock now;
 WindowCounter windows[KindCount];StageCounter stages[PhaseCount];
 int depth=0,phase=Other;bool active=false;unsigned long long last=0;
 unsigned long long empty256_eligible=0,empty256_accepted=0;
 explicit Tracker(Clock c=cpu):now(c){}
 void charge(unsigned long long t){assert(t>=last);stages[phase].ns+=t-last;last=t;}
 void report(int id)const{
  std::printf("WIDE_ENTRY_WORKER id=%d empty256_eligible=%llu empty256_accepted=%llu\n",id,empty256_eligible,empty256_accepted);
  for(int i=0;i<KindCount;i++)std::printf("WIDE_WINDOW_WORKER id=%d kind=%s calls=%llu samples=%llu sampled_ns=%llu period=1024\n",id,kindName(i),windows[i].calls,windows[i].samples,windows[i].ns);
  for(int i=0;i<PhaseCount;i++)std::printf("WIDE_STAGE_WORKER id=%d phase=%s entries=%llu sampled_self_ns=%llu\n",id,phaseName(i),stages[i].entries,stages[i].ns);
 }
};
inline Tracker& tls(){static thread_local Tracker t;return t;}
struct Window {
 Tracker& s;int kind;bool enabled,outer;unsigned long long start=0;
 explicit Window(int k,Tracker& tracker=tls()):s(tracker),kind(k),enabled(enableWideCostSampling),outer(false){
  if(!enabled)return;outer=(s.depth++==0);if(!outer)return;
  auto& w=s.windows[kind];s.active=(++w.calls&1023ULL)==1;
  if(s.active){++w.samples;start=s.now();s.last=start;s.phase=Other;}
 }
 ~Window(){if(!enabled)return;if(outer&&s.active){const auto t=s.now();s.charge(t);s.windows[kind].ns+=t-start;s.active=false;}assert(s.depth>0);--s.depth;}
};
struct Stage {
 Tracker& s;int previous;bool changed;
 explicit Stage(int p,Tracker& tracker=tls()):s(tracker),previous(s.phase),changed(s.active&&p!=s.phase){
  if(s.active)++s.stages[p].entries;
  if(changed){s.charge(s.now());s.phase=p;}
 }
 ~Stage(){if(changed){s.charge(s.now());s.phase=previous;}}
};
}
