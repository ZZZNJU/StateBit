#pragma once
#include <algorithm>

namespace statebit {

struct ForcedComplementFoldRealStats {
    unsigned long long nodes_seen=0;
    unsigned long long root_nodes=0;
    unsigned long long scan_calls=0;
    unsigned long long scan_candidates=0;
    unsigned long long fold_nodes=0;
    unsigned long long forced_vertices=0;
    unsigned long long excl_removed=0;
    unsigned long long c_before_sum=0;
    unsigned long long c_after_sum=0;
    unsigned long long p_before_sum=0;
    unsigned long long p_after_sum=0;
    unsigned long long post_c_le256=0;
    unsigned long long post_c_le128=0;
    unsigned long long post_c_le64=0;
    unsigned long long post_c_le32=0;
    unsigned long long post_c_le16=0;
    unsigned long long fold_to_empty=0;
    unsigned long long max_forced=0;

    inline void observeBefore(const int p,const int c,const bool root){
        ++nodes_seen;
        if(root) ++root_nodes;
        p_before_sum += static_cast<unsigned long long>(p);
        c_before_sum += static_cast<unsigned long long>(c);
    }
    inline void observeScan(const int candidates){
        ++scan_calls;
        scan_candidates += static_cast<unsigned long long>(candidates);
    }
    inline void observeAfter(const int p,const int c,const int forced,const int removedExcl){
        p_after_sum += static_cast<unsigned long long>(p);
        c_after_sum += static_cast<unsigned long long>(c);
        if(c<=256) ++post_c_le256;
        if(c<=128) ++post_c_le128;
        if(c<=64) ++post_c_le64;
        if(c<=32) ++post_c_le32;
        if(c<=16) ++post_c_le16;
        if(forced>0){
            ++fold_nodes;
            forced_vertices += static_cast<unsigned long long>(forced);
            max_forced=std::max(max_forced,static_cast<unsigned long long>(forced));
            if(c==0) ++fold_to_empty;
        }
        excl_removed += static_cast<unsigned long long>(removedExcl);
    }
    inline void merge(const ForcedComplementFoldRealStats &o){
        nodes_seen+=o.nodes_seen; root_nodes+=o.root_nodes;
        scan_calls+=o.scan_calls; scan_candidates+=o.scan_candidates;
        fold_nodes+=o.fold_nodes; forced_vertices+=o.forced_vertices; excl_removed+=o.excl_removed;
        c_before_sum+=o.c_before_sum; c_after_sum+=o.c_after_sum;
        p_before_sum+=o.p_before_sum; p_after_sum+=o.p_after_sum;
        post_c_le256+=o.post_c_le256; post_c_le128+=o.post_c_le128;
        post_c_le64+=o.post_c_le64; post_c_le32+=o.post_c_le32; post_c_le16+=o.post_c_le16;
        fold_to_empty+=o.fold_to_empty; max_forced=std::max(max_forced,o.max_forced);
    }
};

inline bool isForcedC0Vertex(const int uSize,const int degreeInU){
    return uSize>0 && degreeInU==uSize-1;
}

} // namespace statebit
