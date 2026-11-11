#pragma once
#include <algorithm>
#include <cstdint>
#include <vector>

#define AGG_MODE_STANDARD 0
#define AGG_MODE_VALIDATION 1
#define AGG_MODE_MAINTENANCE_ONLY 2
#define AGG_MODE_EXTERNAL_ONLY 3
#define AGG_MODE_LEFT_ONLY 4
#define AGG_MODE_COMBINED 5


#ifndef FORCED_BASE_SEED_AUTO_LEFT_AVG_DEGREE_THRESHOLD
#define FORCED_BASE_SEED_AUTO_LEFT_AVG_DEGREE_THRESHOLD 12
#endif
#if FORCED_BASE_SEED_AUTO_LEFT_AVG_DEGREE_THRESHOLD < 0
#error FORCED_BASE_SEED_AUTO_LEFT_AVG_DEGREE_THRESHOLD must be >= 0
#endif

namespace statebit {

enum AggregateForcedContextMode : int {
    STANDARD = AGG_MODE_STANDARD,
    VALIDATION = AGG_MODE_VALIDATION,
    MAINTENANCE_ONLY = AGG_MODE_MAINTENANCE_ONLY,
    EXTERNAL_ONLY = AGG_MODE_EXTERNAL_ONLY,
    LEFT_ONLY = AGG_MODE_LEFT_ONLY,
    COMBINED = AGG_MODE_COMBINED
};

inline const char* aggregateForcedContextModeName(const int mode){
    switch(mode){
        case STANDARD: return "STANDARD";
        case VALIDATION: return "VALIDATION";
        case MAINTENANCE_ONLY: return "MAINTENANCE_ONLY";
        case EXTERNAL_ONLY: return "EXTERNAL_ONLY";
        case LEFT_ONLY: return "LEFT_ONLY";
        case COMBINED: return "COMBINED";
        default: return "UNKNOWN";
    }
}

constexpr bool aggValidationMode(){ return AGG_FORCED_CONTEXT_MODE==VALIDATION; }
constexpr bool aggMaintainLeft(){
    return AGG_FORCED_CONTEXT_MODE==VALIDATION || AGG_FORCED_CONTEXT_MODE==MAINTENANCE_ONLY ||
           AGG_FORCED_CONTEXT_MODE==LEFT_ONLY || AGG_FORCED_CONTEXT_MODE==COMBINED;
}
constexpr bool aggUseExternalFormula(){
    return AGG_FORCED_CONTEXT_MODE==EXTERNAL_ONLY || AGG_FORCED_CONTEXT_MODE==COMBINED;
}
constexpr bool aggUseLeftFormula(){
    return AGG_FORCED_CONTEXT_MODE==LEFT_ONLY || AGG_FORCED_CONTEXT_MODE==COMBINED;
}

inline bool forcedBaseUseLazyLeftContext(const int degreeLeft){
    return degreeLeft >= FORCED_BASE_HYBRID_LEFT_DEGREE_THRESHOLD;
}

inline const char* forcedBaseLeftContextPolicyName(){
    return "hybrid_d16";
}

inline bool forcedBaseSeedChooseHybridLeftContext(const std::uint64_t degreeSum,const std::uint64_t hopVertices){
    (void)degreeSum; (void)hopVertices;
    return false;
}

struct AutoLeftContextSelector {
    // 0 = warmup/undecided, 1 = eager, 2 = hybrid.
    std::uint64_t warmup_events=0;
    std::uint64_t degree_sum=0;
    int selected=0;

    inline void reset(){ warmup_events=0; degree_sum=0; selected=0; }
    inline int selectedPolicy() const { return selected; }
    inline double warmupAverageDegree() const {
        return warmup_events ? static_cast<double>(degree_sum)/static_cast<double>(warmup_events) : 0.0;
    }
    inline bool useLazyForEvent(const int degreeLeft){
        (void)degreeLeft;
        return false;
    }
};

inline int aggregateForcedAdj(const int logicalUAdj,const int activeAdj){
    return logicalUAdj-activeAdj;
}
inline int aggregateForcedAdjChecked(const int logicalUAdj,const int activeAdj){
    const int v=aggregateForcedAdj(logicalUAdj,activeAdj);
    return v<0?-1:v;
}

struct LeftLogicalAdjTracker {
    std::vector<int> count;
    inline void reset(const int n){ count.assign(static_cast<std::size_t>(std::max(0,n)),0); }
    inline void addEdges(const int* rows,const int n){
        for(int i=0;i<n;++i){ const int r=rows[i]; if(r>=0 && r<static_cast<int>(count.size())) ++count[static_cast<std::size_t>(r)]; }
    }
    inline void subEdges(const int* rows,const int n){
        for(int i=0;i<n;++i){ const int r=rows[i]; if(r>=0 && r<static_cast<int>(count.size())) --count[static_cast<std::size_t>(r)]; }
    }
    inline int get(const int row) const { return count[static_cast<std::size_t>(row)]; }
};

struct AggregateForcedContextStats {
    std::uint64_t build_calls=0;
    std::uint64_t ext_formula_rows=0,left_formula_rows=0;
    std::uint64_t ext_formula_mismatch=0,left_formula_mismatch=0;
    std::uint64_t invalid_formula=0;
    std::uint64_t internal_ns=0,ext_active_ns=0,ext_forced_ns=0;
    std::uint64_t left_clear_ns=0,left_forced_ns=0,left_active_ns=0,validation_verify_ns=0;
    std::uint64_t left_u_init_ns=0,left_u_init_edges=0;
    std::uint64_t left_u_update_calls=0,left_u_update_edges=0,left_u_add_edges=0,left_u_sub_edges=0;
    std::uint64_t left_u_update_sample_calls=0,left_u_update_sample_edges=0,left_u_update_sample_ns=0;
    std::uint64_t lazy_delta_events=0,lazy_flush_calls=0,lazy_flush_vertices=0,lazy_flush_edges=0,lazy_flush_ns=0;
    std::uint64_t hybrid_eager_events=0,hybrid_lazy_events=0,hybrid_eager_edges=0;
    std::uint64_t auto_warmup_events=0,auto_degree_sum=0;
    std::uint64_t auto_selected_eager_workers=0,auto_selected_hybrid_workers=0,auto_undecided_workers=0;
    std::uint64_t seed_auto_seeds=0,seed_auto_degree_sum=0,seed_auto_hop_vertices=0;
    std::uint64_t seed_auto_selected_eager_seeds=0,seed_auto_selected_hybrid_seeds=0;
    bool first_mismatch_set=false;
    int first_seed=-1;
    std::uint64_t first_handoff=0;
    int first_kind=0,first_row=-1,first_vertex=-1,first_old=-1,first_new=-1;

    inline void recordMismatch(const int seed,const std::uint64_t handoff,const int kind,
                               const int row,const int vertex,const int oldv,const int newv){
        if(kind==1) ++ext_formula_mismatch; else ++left_formula_mismatch;
        if(!first_mismatch_set){
            first_mismatch_set=true; first_seed=seed; first_handoff=handoff; first_kind=kind;
            first_row=row; first_vertex=vertex; first_old=oldv; first_new=newv;
        }
    }
    inline void merge(const AggregateForcedContextStats& o){
        build_calls+=o.build_calls;
        ext_formula_rows+=o.ext_formula_rows; left_formula_rows+=o.left_formula_rows;
        ext_formula_mismatch+=o.ext_formula_mismatch; left_formula_mismatch+=o.left_formula_mismatch;
        invalid_formula+=o.invalid_formula;
        internal_ns+=o.internal_ns; ext_active_ns+=o.ext_active_ns; ext_forced_ns+=o.ext_forced_ns;
        left_clear_ns+=o.left_clear_ns; left_forced_ns+=o.left_forced_ns; left_active_ns+=o.left_active_ns; validation_verify_ns+=o.validation_verify_ns;
        left_u_init_ns+=o.left_u_init_ns; left_u_init_edges+=o.left_u_init_edges;
        left_u_update_calls+=o.left_u_update_calls; left_u_update_edges+=o.left_u_update_edges;
        left_u_add_edges+=o.left_u_add_edges; left_u_sub_edges+=o.left_u_sub_edges;
        left_u_update_sample_calls+=o.left_u_update_sample_calls; left_u_update_sample_edges+=o.left_u_update_sample_edges;
        left_u_update_sample_ns+=o.left_u_update_sample_ns;
        lazy_delta_events+=o.lazy_delta_events; lazy_flush_calls+=o.lazy_flush_calls;
        lazy_flush_vertices+=o.lazy_flush_vertices; lazy_flush_edges+=o.lazy_flush_edges; lazy_flush_ns+=o.lazy_flush_ns;
        hybrid_eager_events+=o.hybrid_eager_events; hybrid_lazy_events+=o.hybrid_lazy_events; hybrid_eager_edges+=o.hybrid_eager_edges;
        auto_warmup_events+=o.auto_warmup_events; auto_degree_sum+=o.auto_degree_sum;
        auto_selected_eager_workers+=o.auto_selected_eager_workers; auto_selected_hybrid_workers+=o.auto_selected_hybrid_workers;
        auto_undecided_workers+=o.auto_undecided_workers;
        seed_auto_seeds+=o.seed_auto_seeds; seed_auto_degree_sum+=o.seed_auto_degree_sum; seed_auto_hop_vertices+=o.seed_auto_hop_vertices;
        seed_auto_selected_eager_seeds+=o.seed_auto_selected_eager_seeds; seed_auto_selected_hybrid_seeds+=o.seed_auto_selected_hybrid_seeds;
        if(!first_mismatch_set && o.first_mismatch_set){
            first_mismatch_set=true; first_seed=o.first_seed; first_handoff=o.first_handoff; first_kind=o.first_kind;
            first_row=o.first_row; first_vertex=o.first_vertex; first_old=o.first_old; first_new=o.first_new;
        }
    }
};

} // namespace statebit
