#pragma once
#include <cstdint>
#include <algorithm>
// Fresh ADAPTIVE64: identical policy source for both execution modes; backend implementation is untouched.
// Exploration-only observation. A task is observed ONLY if it completed locally.
// PARALLEL_ENGINE excludes DTFB-split roots via a thread-local split epoch.
namespace adaptive64 {
struct Decision { bool simd=false, sample=false; int bucket=0; };
struct Arm {
    uint64_t ns[16]{};
    uint64_t nodes[16]{};
    uint64_t seen=0, short_seen=0, total_ns=0, total_nodes=0;
    unsigned len=0, cursor=0;
    inline void add(uint64_t elapsed, uint64_t node_count) {
        ns[cursor]=elapsed;nodes[cursor]=node_count;
        cursor=(cursor+1)&15U;if(len<16)++len;
        ++seen;total_ns+=elapsed;total_nodes+=node_count;
        if(node_count<16)++short_seen;
    }
    inline double trimmed_task_ns() const {
        if(!len)return 0.;
        uint64_t a[16];for(unsigned i=0;i<len;i++)a[i]=ns[i];
        std::sort(a,a+len);
        const unsigned trim=len>=10?len/8:0;
        double s=0.;for(unsigned i=trim;i<len-trim;++i)s+=a[i];
        return s/(len-2*trim);
    }
};
struct Bucket {
    uint64_t calls=0, selected[2]{0,0}, explored[2]{0,0};
    uint64_t skipped_split=0, preference_changes=0;
    Arm arm[2];
    bool ready=false, prefer_simd=false;
    double avg_ns[2]{0.,0.};
};
struct Policy {
    Bucket bucket[18];
    uint64_t observations=0, probes=0, short_tasks=0, split_skips=0;
    inline Decision select(int width,int candidates,int effectiveQ) {
        const int w=width<=40?0:(width<=48?1:2);
        const int c=candidates<8?0:(candidates<16?1:2);
        const int q=effectiveQ<20?0:1;
        const int i=(w*3+c)*2+q;
        Bucket& b=bucket[i];++b.calls;
        const bool heuristic=effectiveQ>=20&&candidates>=8&&width>=33;
        bool use_simd=heuristic, observe=false;
        if(b.arm[0].len<6||b.arm[1].len<6){
            // Alternate successful observations, not calls: prevents starvation
            // even if a parallel split caused a skipped learning sample.
            use_simd=b.arm[0].seen>b.arm[1].seen;
            observe=true;
        }else {
            use_simd=b.ready?b.prefer_simd:heuristic;
            // Exploratory tasks only: avoid bias from exploiting the current winner.
            // Every 64th task switches to the less explored arm.
            if((b.calls&63ULL)==0ULL){
                use_simd=b.explored[0]>b.explored[1];observe=true;
            }
        }
        ++b.selected[use_simd?1:0];
        if(observe){++b.explored[use_simd?1:0];++probes;}
        return {use_simd,observe,i};
    }
    inline void complete(const Decision& d,uint64_t ns,uint64_t nodes,bool spilled){
        if(!d.sample)return;
        Bucket& b=bucket[d.bucket];
        if(spilled){++b.skipped_split;++split_skips;return;}
        const int arm=d.simd?1:0;
        b.arm[arm].add(ns,nodes);++observations;
        if(nodes<16)++short_tasks;
        if(b.arm[0].len<6||b.arm[1].len<6)return;
        b.avg_ns[0]=b.arm[0].trimmed_task_ns();
        b.avg_ns[1]=b.arm[1].trimmed_task_ns();
        const bool prior=b.ready?b.prefer_simd:(b.avg_ns[1]<b.avg_ns[0]);
        bool next=prior;
        // Six percent hysteresis. Never compare only ns/node; tree shape differs.
        if(b.avg_ns[1]<0.94*b.avg_ns[0])next=true;
        else if(b.avg_ns[0]<0.94*b.avg_ns[1])next=false;
        if(b.ready&&next!=b.prefer_simd)++b.preference_changes;
        b.prefer_simd=next;b.ready=true;
    }
};
} // namespace adaptive64
