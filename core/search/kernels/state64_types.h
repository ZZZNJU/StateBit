#pragma once
#include <algorithm>

namespace statebit {

static constexpr int STATE64_LANES=4;

struct State64Stats {
    unsigned long long packets=0,slots=0,full_packets=0;
    unsigned long long occ_hist[STATE64_LANES+1]={0,0,0,0,0};
    unsigned long long prepared_states=0,branch_states=0,terminal_states=0;
    unsigned long long state32_handoffs=0,generated_children=0;
    unsigned long long whole_witness_tests=0,whole_witness_hits=0,whole_full_scans=0,whole_scan_vertices=0;
    unsigned long long defect_plane_refine_packets=0,defect_plane_2d_packets=0,defect_plane_defect_steps=0;
    unsigned long long bulk_refine_static_removed=0,bulk_refine_degree_steps=0,bulk_refine_degree_removed=0;
    unsigned long long batch_maximality_batches=0,batch_maximality_slots=0,batch_maximality_full=0,batch_maximality_x_rows=0,batch_maximality_ext_rows=0,batch_maximality_left_scalar_rows=0,batch_maximality_emitted=0;
    unsigned long long left_constraint_left_invocations=0,left_constraint_left_chunks=0,left_constraint_vector_steps=0,left_constraint_fallback_rows=0,left_constraint_left_rejects=0;
    unsigned long long eager_folding_eligible=0,eager_folding_scans=0,eager_folding_tested=0,eager_folding_fold_nodes=0,eager_folding_folded_vertices=0;
    unsigned long long eager_folding_localf_state32=0,eager_folding_localf_max_checks=0;
    unsigned long long fresh64_word_tasks=0,fresh64_state64_tasks=0,fresh64_observations=0;
    unsigned long long fresh64_pivot_mismatch=0,fresh64_refine_mismatch=0,fresh64_dtfb_events=0,fresh64_split_skips=0,fresh64_accepted=0;
    unsigned long long fresh64_word_branch_nodes=0,fresh64_word_children=0,fresh64_word_spilled_states=0;
    unsigned long long localFixedOptimized_batches=0,localFixedOptimized_slots=0,localFixedOptimized_full_batches=0;
    unsigned long long localFixedOptimized_penalty_lookups=0,localFixedOptimized_penalty_misses=0;
    unsigned long long localFixedOptimized_x_rows=0,localFixedOptimized_basex_rows=0,localFixedOptimized_ext_rows=0,localFixedOptimized_left_rows=0,localFixedOptimized_emitted=0;

    inline void observePacket(const int active){
        if(active<=0) return;
        ++packets;
        slots+=static_cast<unsigned long long>(active);
        if(active==STATE64_LANES) ++full_packets;
        if(active<=STATE64_LANES) ++occ_hist[active];
    }
    inline void merge(const State64Stats& o){
        packets+=o.packets; slots+=o.slots; full_packets+=o.full_packets;
        for(int i=1;i<=STATE64_LANES;i++) occ_hist[i]+=o.occ_hist[i];
        prepared_states+=o.prepared_states; branch_states+=o.branch_states;
        terminal_states+=o.terminal_states; state32_handoffs+=o.state32_handoffs;
        generated_children+=o.generated_children;
        whole_witness_tests+=o.whole_witness_tests; whole_witness_hits+=o.whole_witness_hits;
        whole_full_scans+=o.whole_full_scans; whole_scan_vertices+=o.whole_scan_vertices;
        defect_plane_refine_packets+=o.defect_plane_refine_packets;defect_plane_2d_packets+=o.defect_plane_2d_packets;defect_plane_defect_steps+=o.defect_plane_defect_steps;
        bulk_refine_static_removed+=o.bulk_refine_static_removed;bulk_refine_degree_steps+=o.bulk_refine_degree_steps;bulk_refine_degree_removed+=o.bulk_refine_degree_removed;
        batch_maximality_batches+=o.batch_maximality_batches;batch_maximality_slots+=o.batch_maximality_slots;batch_maximality_full+=o.batch_maximality_full;
        batch_maximality_x_rows+=o.batch_maximality_x_rows;batch_maximality_ext_rows+=o.batch_maximality_ext_rows;
        batch_maximality_left_scalar_rows+=o.batch_maximality_left_scalar_rows;batch_maximality_emitted+=o.batch_maximality_emitted;
        left_constraint_left_invocations+=o.left_constraint_left_invocations;left_constraint_left_chunks+=o.left_constraint_left_chunks;
        left_constraint_vector_steps+=o.left_constraint_vector_steps;left_constraint_fallback_rows+=o.left_constraint_fallback_rows;
        left_constraint_left_rejects+=o.left_constraint_left_rejects;
        eager_folding_eligible+=o.eager_folding_eligible;eager_folding_scans+=o.eager_folding_scans;eager_folding_tested+=o.eager_folding_tested;
        eager_folding_fold_nodes+=o.eager_folding_fold_nodes;eager_folding_folded_vertices+=o.eager_folding_folded_vertices;
        eager_folding_localf_state32+=o.eager_folding_localf_state32;eager_folding_localf_max_checks+=o.eager_folding_localf_max_checks;
        fresh64_word_tasks+=o.fresh64_word_tasks;fresh64_state64_tasks+=o.fresh64_state64_tasks;
        fresh64_observations+=o.fresh64_observations;
        fresh64_pivot_mismatch+=o.fresh64_pivot_mismatch;fresh64_refine_mismatch+=o.fresh64_refine_mismatch;
        fresh64_dtfb_events+=o.fresh64_dtfb_events;fresh64_split_skips+=o.fresh64_split_skips;fresh64_accepted+=o.fresh64_accepted;
        fresh64_word_branch_nodes+=o.fresh64_word_branch_nodes;fresh64_word_children+=o.fresh64_word_children;
        fresh64_word_spilled_states+=o.fresh64_word_spilled_states;
        localFixedOptimized_batches+=o.localFixedOptimized_batches;localFixedOptimized_slots+=o.localFixedOptimized_slots;localFixedOptimized_full_batches+=o.localFixedOptimized_full_batches;
        localFixedOptimized_penalty_lookups+=o.localFixedOptimized_penalty_lookups;localFixedOptimized_penalty_misses+=o.localFixedOptimized_penalty_misses;
        localFixedOptimized_x_rows+=o.localFixedOptimized_x_rows;localFixedOptimized_basex_rows+=o.localFixedOptimized_basex_rows;
        localFixedOptimized_ext_rows+=o.localFixedOptimized_ext_rows;localFixedOptimized_left_rows+=o.localFixedOptimized_left_rows;localFixedOptimized_emitted+=o.localFixedOptimized_emitted;
    }
};

} // namespace statebit
