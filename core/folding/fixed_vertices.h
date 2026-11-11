#pragma once
#include <algorithm>

namespace statebit {

struct ForcedBaseStats {
    unsigned long long nodes_seen=0;
    unsigned long long root_nodes=0;
    unsigned long long scan_calls=0;
    unsigned long long scan_candidates=0;
    unsigned long long fold_nodes=0;
    unsigned long long forced_vertices=0;
    unsigned long long c_before_sum=0;
    unsigned long long c_after_sum=0;
    unsigned long long p_active_sum=0;
    unsigned long long forced_base_before_sum=0;
    unsigned long long forced_base_after_sum=0;
    unsigned long long active_before_sum=0;
    unsigned long long active_after_sum=0;
    unsigned long long active_le256=0;
    unsigned long long active_le128=0;
    unsigned long long active_le64=0;
    unsigned long long active_le32=0;
    unsigned long long active_le16=0;
    unsigned long long max_forced_base=0;
    unsigned long long max_fold_batch=0;

    inline void observeBefore(const int p,const int c,const int f,const bool root){
        ++nodes_seen;
        if(root) ++root_nodes;
        p_active_sum += static_cast<unsigned long long>(p);
        c_before_sum += static_cast<unsigned long long>(c);
        forced_base_before_sum += static_cast<unsigned long long>(f);
        active_before_sum += static_cast<unsigned long long>(p+c);
    }
    inline void observeScan(const int candidates){
        ++scan_calls;
        scan_candidates += static_cast<unsigned long long>(candidates);
    }
    inline void observeAfter(const int p,const int c,const int folded,const int f){
        c_after_sum += static_cast<unsigned long long>(c);
        forced_base_after_sum += static_cast<unsigned long long>(f);
        const unsigned long long active=static_cast<unsigned long long>(p+c);
        active_after_sum += active;
        if(active<=256) ++active_le256;
        if(active<=128) ++active_le128;
        if(active<=64) ++active_le64;
        if(active<=32) ++active_le32;
        if(active<=16) ++active_le16;
        if(folded>0){
            ++fold_nodes;
            forced_vertices += static_cast<unsigned long long>(folded);
            max_fold_batch=std::max(max_fold_batch,static_cast<unsigned long long>(folded));
        }
        max_forced_base=std::max(max_forced_base,static_cast<unsigned long long>(f));
    }
    inline void merge(const ForcedBaseStats &o){
        nodes_seen+=o.nodes_seen; root_nodes+=o.root_nodes;
        scan_calls+=o.scan_calls; scan_candidates+=o.scan_candidates;
        fold_nodes+=o.fold_nodes; forced_vertices+=o.forced_vertices;
        c_before_sum+=o.c_before_sum; c_after_sum+=o.c_after_sum;
        p_active_sum+=o.p_active_sum;
        forced_base_before_sum+=o.forced_base_before_sum;
        forced_base_after_sum+=o.forced_base_after_sum;
        active_before_sum+=o.active_before_sum; active_after_sum+=o.active_after_sum;
        active_le256+=o.active_le256; active_le128+=o.active_le128;
        active_le64+=o.active_le64; active_le32+=o.active_le32; active_le16+=o.active_le16;
        max_forced_base=std::max(max_forced_base,o.max_forced_base);
        max_fold_batch=std::max(max_fold_batch,o.max_fold_batch);
    }
};

inline bool isForcedBaseC0Vertex(const int uSize,const int degreeInU){
    return uSize>0 && degreeInU==uSize-1;
}

} // namespace statebit
