#pragma once
#include <cstdint>
#include <cstdio>
#include <algorithm>
// Old NEW16 Hybrid4 selection formula adapted to the shared backend.
// Learning observations are accepted only for locally completed tasks.
namespace kernel_selection {
struct Decision {bool simd=false,sample=false;int bucket=0;};
struct Bucket {std::uint64_t calls=0,samples[2]{0,0},ns[2]{0,0},nodes[2]{0,0};};
struct Policy {
 Bucket bucket[18];
 std::uint64_t word=0,simd=0,accepted=0,split_skips=0,short_skips=0;
 Decision select(int width,int candidates,int q){
  const int i=((width<=40?0:(width<=48?1:2))*3+(candidates<8?0:(candidates<16?1:2)))*2+(q<20?0:1);
  auto& b=bucket[i];++b.calls;
  const bool proxy=q>=20&&candidates>=8&&width>=33;
  bool use=proxy,sample=false;
  if(b.samples[0]<6||b.samples[1]<6){use=b.samples[0]>b.samples[1];sample=true;}
  else {
   const double a=double(b.ns[0])/std::max(std::uint64_t(1),b.nodes[0]);
   const double z=double(b.ns[1])/std::max(std::uint64_t(1),b.nodes[1]);
   if(z<a*.97)use=true;else if(a<z*.97)use=false;
   if((b.calls&127ULL)==0){use=((b.calls>>7)&1ULL)!=0;sample=true;}
   else sample=(b.calls&31ULL)==0;
  }
  ++(use?simd:word);return {use,sample,i};
 }
 void complete(const Decision& d,std::uint64_t ns,std::uint64_t nodes,bool split){
  if(!d.sample)return;
  if(split){++split_skips;return;}
  if(nodes<16){++short_skips;return;}
  auto& b=bucket[d.bucket];const int a=d.simd?1:0;++b.samples[a];b.ns[a]+=ns;b.nodes[a]+=nodes;++accepted;
 }
 void report(int worker)const{
  std::printf("UNIFIED_POLICY_WORKER id=%d word_tasks=%llu state_tasks=%llu accepted=%llu split_skips=%llu short_skips=%llu\n",worker,(unsigned long long)word,(unsigned long long)simd,(unsigned long long)accepted,(unsigned long long)split_skips,(unsigned long long)short_skips);
 }
};
inline Policy& threadPolicy(){static thread_local Policy p;return p;}
}
