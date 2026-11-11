#pragma once
#include <cstdint>
#include <cassert>

namespace statebit {

struct Mask256 {
    uint64_t w[4];
    Mask256():w{0,0,0,0}{}
    Mask256(uint64_t a,uint64_t b,uint64_t c,uint64_t d):w{a,b,c,d}{}
};
inline Mask256 operator|(const Mask256 a,const Mask256 b){ return {a.w[0]|b.w[0],a.w[1]|b.w[1],a.w[2]|b.w[2],a.w[3]|b.w[3]}; }
inline Mask256 operator&(const Mask256 a,const Mask256 b){ return {a.w[0]&b.w[0],a.w[1]&b.w[1],a.w[2]&b.w[2],a.w[3]&b.w[3]}; }
inline Mask256 operator^(const Mask256 a,const Mask256 b){ return {a.w[0]^b.w[0],a.w[1]^b.w[1],a.w[2]^b.w[2],a.w[3]^b.w[3]}; }
inline Mask256 operator~(const Mask256 a){ return {~a.w[0],~a.w[1],~a.w[2],~a.w[3]}; }
inline Mask256& operator|=(Mask256& a,const Mask256 b){ for(int i=0;i<4;i++) a.w[i]|=b.w[i]; return a; }
inline Mask256& operator&=(Mask256& a,const Mask256 b){ for(int i=0;i<4;i++) a.w[i]&=b.w[i]; return a; }
inline bool operator==(const Mask256 a,const Mask256 b){ return a.w[0]==b.w[0]&&a.w[1]==b.w[1]&&a.w[2]==b.w[2]&&a.w[3]==b.w[3]; }
inline bool operator!=(const Mask256 a,const Mask256 b){ return !(a==b); }
inline bool mask256Any(const Mask256 a){ return (a.w[0]|a.w[1]|a.w[2]|a.w[3])!=0; }
inline bool mask256None(const Mask256 a){ return !mask256Any(a); }
inline int mask256Pop(const Mask256 a){ return __builtin_popcountll(a.w[0])+__builtin_popcountll(a.w[1])+__builtin_popcountll(a.w[2])+__builtin_popcountll(a.w[3]); }
inline Mask256 mask256Bit(const int i){ assert(i>=0&&i<256); Mask256 r; r.w[i>>6]=1ULL<<(i&63); return r; }
inline bool mask256Test(const Mask256 a,const int i){ assert(i>=0&&i<256); return (a.w[i>>6]>>(i&63))&1ULL; }
inline Mask256 mask256ClearBit(Mask256 a,const int i){ assert(i>=0&&i<256); a.w[i>>6]&=~(1ULL<<(i&63)); return a; }
inline int mask256First(const Mask256 a){ assert(mask256Any(a)); for(int j=0;j<4;j++) if(a.w[j]) return j*64+__builtin_ctzll(a.w[j]); return -1; }
inline Mask256 mask256LowBits(const int n){
    assert(n>=0&&n<=256); Mask256 r;
    const int full=n/64, rem=n%64;
    for(int j=0;j<full;j++) r.w[j]=~0ULL;
    if(full<4 && rem) r.w[full]=(1ULL<<rem)-1ULL;
    return r;
}
inline Mask256 mask256AndNot(const Mask256 a,const Mask256 b){ return {a.w[0]&~b.w[0],a.w[1]&~b.w[1],a.w[2]&~b.w[2],a.w[3]&~b.w[3]}; }
inline Mask256 mask256Without(const Mask256 a,const int i){ return mask256ClearBit(a,i); }

struct State256Stats {
    unsigned long long eligible_nodes=0,handoffs=0,builds=0,nodes=0,transitions128=0,outputs=0;
    unsigned long long entry_129_160=0,entry_161_192=0,entry_193_224=0,entry_225_256=0;
    unsigned long long whole_calls=0,leaf_calls=0,cand_filter_calls=0,cand_filter_removed=0;
    unsigned long long ext_rows=0,left_rows=0,forced_ext_checks=0,forced_left_edges=0;
    unsigned long long active_sum=0,forced_sum=0;
    unsigned long long build_ns=0,snapshot_ns=0,cpu_ns=0;
    inline void observeHandoff(const int active,const int forced){
        ++handoffs; active_sum+=(unsigned long long)active; forced_sum+=(unsigned long long)forced;
        if(active<=160) ++entry_129_160; else if(active<=192) ++entry_161_192; else if(active<=224) ++entry_193_224; else ++entry_225_256;
    }
    inline void merge(const State256Stats& o){
        eligible_nodes+=o.eligible_nodes; handoffs+=o.handoffs; builds+=o.builds; nodes+=o.nodes; transitions128+=o.transitions128; outputs+=o.outputs;
        entry_129_160+=o.entry_129_160; entry_161_192+=o.entry_161_192; entry_193_224+=o.entry_193_224; entry_225_256+=o.entry_225_256;
        whole_calls+=o.whole_calls; leaf_calls+=o.leaf_calls; cand_filter_calls+=o.cand_filter_calls; cand_filter_removed+=o.cand_filter_removed;
        ext_rows+=o.ext_rows; left_rows+=o.left_rows; forced_ext_checks+=o.forced_ext_checks; forced_left_edges+=o.forced_left_edges;
        active_sum+=o.active_sum; forced_sum+=o.forced_sum; build_ns+=o.build_ns; snapshot_ns+=o.snapshot_ns; cpu_ns+=o.cpu_ns;
    }
};
inline bool forcedBaseState256Eligible(const int active,const int threshold){ return active>128 && threshold>128 && threshold<=256 && active<=threshold; }

} // namespace statebit
