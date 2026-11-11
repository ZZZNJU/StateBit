// WIDE_REMAP_VALIDATION alternative, using SEQUENTIAL_ENGINE's symmetric adjacency-matrix reconstruction.
// Both both execution modes compile this identical implementation. No folding, gate,
// ForcedBase penalty bookkeeping or backend dispatch is done in this file.
#include <atomic>
#include <cstdio>
#include <vector>
#include <immintrin.h>
#ifndef WIDE_STATE_REMAP_INC
#define WIDE_STATE_REMAP_INC
namespace statebit { namespace wide_state_remap {
struct Select128 {
  int m=0;
  int old[128]{};
  Mask128 P{},C{};
};
struct Select64 {
  int m=0;
  int old[64]{};
  uint64_t P=0,C=0;
};
inline Select128 select(Mask256 P, Mask256 C, int fixed, int srcM) {
  Select128 s; const Mask256 U=P|C; s.m=mask256Pop(U);
  assert(s.m>0 && s.m<=128 && fixed<=s.m);
  int n=0;
  for(int i=0;i<fixed;i++){assert(mask256Test(P,i));s.old[n++]=i;}
  for(int i=fixed;i<srcM;i++) if(mask256Test(U,i))s.old[n++]=i;
  assert(n==s.m);
  for(int j=0;j<s.m;j++){
    int oi=s.old[j]; if(mask256Test(P,oi))s.P|=mask128Bit(j);
    else {assert(mask256Test(C,oi));s.C|=mask128Bit(j);}
  }
  return s;
}
inline Select64 select(Mask128 P, Mask128 C, int fixed, int srcM) {
  Select64 s; const Mask128 U=P|C; s.m=mask128Pop(U);
  assert(s.m>0 && s.m<=64 && fixed<=s.m);
  int n=0;
  for(int i=0;i<fixed;i++){assert(mask128Test(P,i));s.old[n++]=i;}
  for(int i=fixed;i<srcM;i++) if(mask128Test(U,i))s.old[n++]=i;
  assert(n==s.m);
  for(int j=0;j<s.m;j++){
    int oi=s.old[j]; if(mask128Test(P,oi))s.P|=(1ULL<<j);
    else {assert(mask128Test(C,oi));s.C|=(1ULL<<j);}
  }
  return s;
}
// Matrix callback: uint8_t(unsigned original vertex, unsigned original vertex).
// Left callback: leftForEach(original vertex, emit_left_index).
// Dense adj/pair are reconstructed by reading each upper-triangular pair ONCE.
template <class Source, class Dest, class Matrix, class ExternalAdj, class LeftForEach>
inline Select128 convert256to128(Mask256 P,Mask256 C,Mask256 X,
                                 const Source& src,Dest& dst,int leftSz,
                                 Matrix matrix,ExternalAdj extAdj,LeftForEach leftForEach,int link2equal){
  Select128 sel=select(P,C,src.fixed,src.m);
  dst.fixed=src.fixed; dst.m=sel.m; dst.proper=src.proper;
  for(int j=0;j<sel.m;j++){
    dst.orig[j]=src.orig[sel.old[j]];
    dst.adj[j]={}; dst.pair[j]=mask128Bit(j);
  }
  for(int i=0;i<sel.m;i++)for(int j=i+1;j<sel.m;j++){
    const uint8_t cm=matrix(dst.orig[i],dst.orig[j],sel.old[i],sel.old[j]);
    if(cm&1){dst.adj[i]|=mask128Bit(j);dst.adj[j]|=mask128Bit(i);}
    if(!dst.proper || cm>=link2equal){dst.pair[i]|=mask128Bit(j);dst.pair[j]|=mask128Bit(i);}
  }
  const Mask128 all=mask128LowBits(sel.m);
  for(int j=0;j<sel.m;j++)dst.nonadj[j]=mask128AndNot(mask128AndNot(all,dst.adj[j]),mask128Bit(j));
  dst.extN=src.extN+mask256Pop(X);
  dst.extOrig.resize(dst.extN); dst.extAdj.resize(dst.extN);
  int eout=0;
  std::vector<int> sourceX(static_cast<size_t>(dst.extN),-1);
  for(int e=0;e<src.extN;e++)dst.extOrig[eout++]=src.extOrig[e];
  while(mask256Any(X)){
    int x=mask256First(X); X=mask256ClearBit(X,x);
    sourceX[eout]=x; dst.extOrig[eout++]=src.orig[x];
  }
  assert(eout==dst.extN);
  for(int e=0;e<dst.extN;e++){
    Mask128 adj{};
    for(int j=0;j<sel.m;j++) if(extAdj(dst.extOrig[e],dst.orig[j],e,sel.old[j],sourceX[e])) adj|=mask128Bit(j);
    dst.extAdj[e]=adj;
  }
  dst.leftAdj.assign(static_cast<size_t>(leftSz),Mask128{});
  for(int j=0;j<sel.m;j++){
    leftForEach(dst.orig[j],sel.old[j],[&](int l){if(l>=0 && l<leftSz) dst.leftAdj[l]|=mask128Bit(j);});
  }
  return sel;
}
template<class ExtOrig,class ExtAdj,class LeftAdj>
struct Buffers64 {
  int *orig;
  uint64_t *adj,*nonadj,*pair;
  ExtOrig &extOrig; ExtAdj &extAdj; LeftAdj &leftAdj;
};
template<class E,class A,class L>
inline Buffers64<E,A,L> buffers64(int* o,uint64_t* a,uint64_t* n,uint64_t* p,E& e,A& ea,L& la){return {o,a,n,p,e,ea,la};}
template <class Source,class E,class A,class L,class Matrix,class ExternalAdj,class LeftForEach>
inline Select64 convert128to64(Mask128 P,Mask128 C,Mask128 X,const Source& src,int leftSz,
                                Buffers64<E,A,L> dst,Matrix matrix,ExternalAdj extAdj,LeftForEach leftForEach,int link2equal){
  Select64 sel=select(P,C,src.fixed,src.m);
  for(int j=0;j<sel.m;j++){
    dst.orig[j]=src.orig[sel.old[j]];
    dst.adj[j]=0; dst.pair[j]=1ULL<<j;
  }
  for(int i=0;i<sel.m;i++)for(int j=i+1;j<sel.m;j++){
    const uint8_t cm=matrix(dst.orig[i],dst.orig[j],sel.old[i],sel.old[j]);
    if(cm&1){dst.adj[i]|=1ULL<<j;dst.adj[j]|=1ULL<<i;}
    if(!src.proper || cm>=link2equal){dst.pair[i]|=1ULL<<j;dst.pair[j]|=1ULL<<i;}
  }
  const uint64_t all=sel.m==64?~0ULL:((1ULL<<sel.m)-1ULL);
  for(int j=0;j<sel.m;j++)dst.nonadj[j]=all&~dst.adj[j]&~(1ULL<<j);
  int eout=0;
  std::vector<int> sourceX(static_cast<size_t>(src.extN+mask128Pop(X)),-1);
  for(int e=0;e<src.extN;e++)dst.extOrig[eout++]=src.extOrig[e];
  while(mask128Any(X)){
    int x=mask128First(X);X=mask128ClearBit(X,x);
    sourceX[eout]=x; dst.extOrig[eout++]=src.orig[x];
  }
  for(int e=0;e<eout;e++){
    uint64_t adj=0;
    for(int j=0;j<sel.m;j++)if(extAdj(dst.extOrig[e],dst.orig[j],e,sel.old[j],sourceX[e]))adj|=(1ULL<<j);
    dst.extAdj[e]=adj;
  }
  for(int l=0;l<leftSz;l++)dst.leftAdj[l]=0;
  for(int j=0;j<sel.m;j++){
    leftForEach(dst.orig[j],sel.old[j],[&](int l){if(l>=0&&l<leftSz)dst.leftAdj[l]|=(1ULL<<j);});
  }
  return sel;
}
// Increasing source-coordinate order matches select(): fixed P is an intact prefix.
inline uint64_t extractSelected64(uint64_t row,uint64_t mask){
#if defined(__BMI2__)
 return _pext_u64(row,mask);
#else
 uint64_t out=0,bit=1;while(mask){const uint64_t low=mask&-mask;if(row&low)out|=bit;mask^=low;bit<<=1;}return out;
#endif
}
struct Project128 {
 Mask128 selected;int lowCount;
 uint64_t operator()(Mask128 row) const {
  const uint64_t low=extractSelected64(row.lo,selected.lo);
  // Avoid shifting by64, even when the high selection is empty.
  return selected.hi ? low|(extractSelected64(row.hi,selected.hi)<<lowCount) : low;
 }
};
inline Project128 projector128(Mask128 selected){assert(mask128Pop(selected)<=64);return {selected,__builtin_popcountll(selected.lo)};}
template<class Source,class E,class A,class L>
inline Select64 project128to64(Mask128 P,Mask128 C,Mask128 X,const Source& src,int leftSz,
                              Buffers64<E,A,L> dst){
 Select64 sel=select(P,C,src.fixed,src.m);const auto pack=projector128(P|C);
 const uint64_t all=sel.m==64?~0ULL:((1ULL<<sel.m)-1ULL);
 for(int j=0;j<sel.m;j++){
  const int oi=sel.old[j];dst.orig[j]=src.orig[oi];
  dst.adj[j]=pack(src.adj[oi]);dst.pair[j]=pack(src.pair[oi]);
  dst.nonadj[j]=all&~dst.adj[j]&~(1ULL<<j);
 }
 int e=0;for(;e<src.extN;e++){dst.extOrig[e]=src.extOrig[e];dst.extAdj[e]=pack(src.extAdj[e]);}
 while(mask128Any(X)){int x=mask128First(X);X=mask128ClearBit(X,x);dst.extOrig[e]=src.orig[x];dst.extAdj[e++]=pack(src.adj[x]);}
 for(int l=0;l<leftSz;l++)dst.leftAdj[l]=pack(src.leftAdj[l]);
 return sel;
}
template<class Src,class Dst>
inline bool same256to128(const Src& src,const Dst& dst,const Select128& sel,
                         Mask256 P,Mask256 C,Mask256 X,int leftSz){
    if(dst.m!=sel.m||dst.fixed!=src.fixed||dst.proper!=src.proper||
       dst.extN!=src.extN+mask256Pop(X))return false;
    for(int j=0;j<sel.m;j++){
        const int oi=sel.old[j];
        if(dst.orig[j]!=src.orig[oi]||
            mask128Test(sel.P,j)!=mask256Test(P,oi)||
            mask128Test(sel.C,j)!=mask256Test(C,oi))return false;
        for(int t=0;t<sel.m;t++){
            const int ot=sel.old[t];
            if(mask128Test(dst.adj[j],t)!=mask256Test(src.adj[oi],ot)||
               mask128Test(dst.pair[j],t)!=mask256Test(src.pair[oi],ot)||
               mask128Test(dst.nonadj[j],t)!=(t!=j&&!mask256Test(src.adj[oi],ot)))return false;
        }
    }
    int e=0;
    for(;e<src.extN;e++){
        if(dst.extOrig[e]!=src.extOrig[e])return false;
        for(int j=0;j<sel.m;j++)if(mask128Test(dst.extAdj[e],j)!=mask256Test(src.extAdj[e],sel.old[j]))return false;
    }
    while(mask256Any(X)){
        int x=mask256First(X);X=mask256ClearBit(X,x);
        if(dst.extOrig[e]!=src.orig[x])return false;
        for(int j=0;j<sel.m;j++)if(mask128Test(dst.extAdj[e],j)!=mask256Test(src.adj[x],sel.old[j]))return false;
        ++e;
    }
    if(static_cast<int>(dst.leftAdj.size())!=leftSz)return false;
    for(int l=0;l<leftSz;l++)for(int j=0;j<sel.m;j++)
        if(mask128Test(dst.leftAdj[l],j)!=mask256Test(src.leftAdj[l],sel.old[j]))return false;
    return true;
}
template<class Src,class E,class A,class L>
inline bool same128to64(const Src& src,const Buffers64<E,A,L>& dst,const Select64& sel,
                        Mask128 P,Mask128 C,Mask128 X,int leftSz){
    if(sel.m!=mask128Pop(P|C))return false;
    for(int j=0;j<sel.m;j++){
        const int oi=sel.old[j];
        if(dst.orig[j]!=src.orig[oi]||
            (((sel.P>>j)&1ULL)!=static_cast<uint64_t>(mask128Test(P,oi)))||
            (((sel.C>>j)&1ULL)!=static_cast<uint64_t>(mask128Test(C,oi))))return false;
        for(int t=0;t<sel.m;t++){
            const int ot=sel.old[t];
            if(((dst.adj[j]>>t)&1ULL)!=static_cast<uint64_t>(mask128Test(src.adj[oi],ot))||
               ((dst.pair[j]>>t)&1ULL)!=static_cast<uint64_t>(mask128Test(src.pair[oi],ot))||
               ((dst.nonadj[j]>>t)&1ULL)!=static_cast<uint64_t>(t!=j&&!mask128Test(src.adj[oi],ot)))return false;
        }
    }
    int e=0;
    for(;e<src.extN;e++){
        if(dst.extOrig[e]!=src.extOrig[e])return false;
        for(int j=0;j<sel.m;j++)if(((dst.extAdj[e]>>j)&1ULL)!=static_cast<uint64_t>(mask128Test(src.extAdj[e],sel.old[j])))return false;
    }
    while(mask128Any(X)){
        int x=mask128First(X);X=mask128ClearBit(X,x);
        if(dst.extOrig[e]!=src.orig[x])return false;
        for(int j=0;j<sel.m;j++)if(((dst.extAdj[e]>>j)&1ULL)!=static_cast<uint64_t>(mask128Test(src.adj[x],sel.old[j])))return false;
        ++e;
    }
    for(int l=0;l<leftSz;l++)for(int j=0;j<sel.m;j++)
        if(((dst.leftAdj[l]>>j)&1ULL)!=static_cast<uint64_t>(mask128Test(src.leftAdj[l],sel.old[j])))return false;
    return true;
}
}} // namespace
#endif
