    extern thread_local unsigned long long directEmpty128Handoffs;
    extern thread_local unsigned long long directEmpty64Handoffs;

#define STATIC_DENSE_SPLIT_KERNEL 1
    inline unsigned long long threadCpuNowNs(){
        struct timespec ts; clock_gettime(CLOCK_THREAD_CPUTIME_ID,&ts);
        return static_cast<unsigned long long>(ts.tv_sec)*1000000000ULL+static_cast<unsigned long long>(ts.tv_nsec);
    }

    using VtxSet = RandSet<int>;
    using Graph = graph<int>;
    extern thread_local ForcedComplementFoldRealStats forcedComplementFoldRealStats;
    extern thread_local ForcedBaseStats forcedBaseStats;
    extern thread_local ForcedBasePayoffGate forcedBasePayoffGate;
    extern thread_local ForcedBaseBit64Stats forcedBaseBit64Stats;
    extern thread_local State64Stats state64Stats;
    extern thread_local State128Stats state128Stats;
    extern thread_local State256Stats state256Stats;
    extern thread_local BestDiagStats bestDiagStats;
    extern thread_local AggregateForcedContextStats aggregateForcedContextStats;
    extern thread_local AutoLeftContextSelector autoLeftContextSelector;
    enum : uint8_t{        
        UNLINK2LESS=0,
        LINK2LESS=1,
        UNLINK2EQUAL=2,
        LINK2EQUAL=3,
        UNLINK2MORE=4,
        LINK2MORE=5
    };
    struct PlexEmitter;
    struct GlobalMaximalityChecker;
    struct BitSearchStats {
      unsigned long long terminal_calls=0,fallback_calls=0,nodes=0;
      unsigned long long cand_filter_calls=0,cand_filter_removed=0;
      unsigned long long whole_calls=0,leaf_calls=0,x_rejects=0,left_rejects=0,outputs=0;
      unsigned long long include_calls=0,exclude_calls=0;
      // Formal <=64 width dispatcher. There is no scalar 33-64 DFS kernel:
      // 33-64 always enters State64; 1-32 always enters State32 after the
      // scalar root precheck required by the State32 prechecked-root contract.
      unsigned long long bit64_dispatches=0,state64_direct_dispatches=0,state32_direct_dispatches=0;
      unsigned long long state32_root_prechecks=0,bit64_terminal_dispatches=0;
      inline void merge(const BitSearchStats& o){
        terminal_calls+=o.terminal_calls; fallback_calls+=o.fallback_calls; nodes+=o.nodes;
        cand_filter_calls+=o.cand_filter_calls; cand_filter_removed+=o.cand_filter_removed;
        whole_calls+=o.whole_calls; leaf_calls+=o.leaf_calls; x_rejects+=o.x_rejects; left_rejects+=o.left_rejects; outputs+=o.outputs;
        include_calls+=o.include_calls; exclude_calls+=o.exclude_calls;
        bit64_dispatches+=o.bit64_dispatches; state64_direct_dispatches+=o.state64_direct_dispatches;
        state32_direct_dispatches+=o.state32_direct_dispatches; state32_root_prechecks+=o.state32_root_prechecks;
        bit64_terminal_dispatches+=o.bit64_terminal_dispatches;
      }
    };
    extern thread_local BitSearchStats bitStats;
    struct WordKernelStats {
        unsigned long long selected_runtime=0;
    };
    extern thread_local WordKernelStats wordKernelStats;
    struct LaneDfsStats {
        unsigned long long transitions=0, packets=0, full_packets=0, single_packets=0, states=0;
        unsigned long long branch_states=0, outcome_divergence=0, pivot_divergence=0;
        unsigned long long entry_le16=0, entry_17_24=0, entry_25_32=0, recode_vertices=0;
        unsigned long long packed_child_ops=0, max_stack=0;
        unsigned long long pext_calls=0, pdep_calls=0;
        unsigned long long pair_pivot_calls=0, pair_refine_calls=0;
        unsigned long long packed_whole_steps=0, packed_pivot_steps=0, packed_refine_steps=0;
    };
    extern thread_local LaneDfsStats laneStats;
    struct LanePivotStats {
        unsigned long long paired_pivot_events=0,invalid_or_pruned=0,paper_same_pivot=0;
        unsigned long long common_domain=0,no_common_domain=0;
        unsigned long long regret_0=0,regret_1=0,regret_2=0,regret_3=0,regret_4=0,regret_5plus=0;
    };
    extern thread_local LanePivotStats lanePivotStats;
    struct AvxPacketStats {
        unsigned long long transitions=0,packets=0,full_packets=0,partial_packets=0,states=0,active_slots=0;
        unsigned long long branch_states=0,pivot_divergent_packets=0,recode_vertices=0,max_stack=0;
        unsigned long long vector_pivot_calls=0,scalar_pivot_calls=0,vector_refine_calls=0,scalar_refine_calls=0;
        unsigned long long compaction_calls=0,compacted_children=0,generated_children=0;
        unsigned long long terminal_states=0;
        unsigned long long branch_batches=0,branch_batch_slots=0,branch_full_batches=0;
        unsigned long long refine_batches=0,refine_batch_slots=0,refine_full_batches=0;
        unsigned long long pivot_union_vertices=0,refine_union_vertices=0;
        unsigned long long state_pool_pushes=0,state_pool_pops=0,soa_refills=0;
        unsigned long long adaptive_width8_batches=0,adaptive_width4_batches=0,adaptive_scalar_batches=0;
        unsigned long long cross_context_waves=0,cross_context_contexts=0,cross_context_branch_batches=0,cross_context_branch_slots=0,cross_context_full_batches=0;
        unsigned long long cross_context_refine_batches=0,cross_context_refine_slots=0,cross_context_refine_full_batches=0,cross_context_gathers=0;
    };
    extern thread_local AvxPacketStats avxStats;
    struct MaxSimdStats {
        unsigned long long maximality_calls=0,max_batches=0,max_batch_slots=0,max_full_batches=0;
        unsigned long long sat_union_vertices=0,x_union_vertices=0;
        unsigned long long local_x_rejects=0,local_x_passes=0;
        unsigned long long basex_rejects=0,external_rejects=0,left_rejects=0,emits=0;
        unsigned long long suffix_batches=0,suffix_slots=0;
        unsigned long long suffix_basex_rows=0,suffix_external_rows=0,suffix_left_rows=0;
        unsigned long long suffix_row_localizations=0;
        unsigned long long left_2d_batches=0,left_2d_slots=0,left_2d_chunks=0,left_2d_vertex_steps=0;
        unsigned long long left_2d_early_rejects=0,left_2d_early_empty=0,left_2d_transpose_builds=0,left_2d_transpose_adj_bits=0;
    };
    extern thread_local MaxSimdStats maxSimdStats;
    // Whole-witness propagation profile.  These counters are updated only
    struct WholeWitnessStats {
        unsigned long long whole_checks=0;
        unsigned long long witness_tests=0,witness_hits=0,witness_misses=0;
        unsigned long long witness_removed=0,witness_defect_recovered=0;
        // Exact scalar vertex-steps avoided on witness hits.  Because the
        // inherited witness was the first violation in the parent and U only
        // shrinks, all earlier vertices remain non-violating in the child.
        unsigned long long witness_avoided_scan_vertices=0;
        unsigned long long full_scans=0,full_scan_vertices=0;
        unsigned long long full_scan_finds=0,full_scan_whole=0;
        unsigned long long inherited_children=0;
        unsigned long long witness_chain_hit_depth_sum=0,witness_chain_max=0;
    };
    extern thread_local WholeWitnessStats wholeWitnessStats;
    struct WitnessResumeStats {
        unsigned long long sampled_resumes=0;
        unsigned long long sampled_removed=0;
        unsigned long long sampled_recovered=0;
        unsigned long long sampled_skipped_prefix_vertices=0;
        unsigned long long sampled_actual_scan_vertices=0;
        unsigned long long sampled_baseline_equiv_vertices=0;
        unsigned long long sampled_whole=0;
        unsigned long long sampled_finds=0;
        unsigned long long verify_samples=0;
        unsigned long long verify_mismatches=0;
        void merge(const WitnessResumeStats& o){
            sampled_resumes+=o.sampled_resumes; sampled_removed+=o.sampled_removed; sampled_recovered+=o.sampled_recovered;
            sampled_skipped_prefix_vertices+=o.sampled_skipped_prefix_vertices; sampled_actual_scan_vertices+=o.sampled_actual_scan_vertices;
            sampled_baseline_equiv_vertices+=o.sampled_baseline_equiv_vertices; sampled_whole+=o.sampled_whole; sampled_finds+=o.sampled_finds;
            verify_samples+=o.verify_samples; verify_mismatches+=o.verify_mismatches;
        }
    };
    extern thread_local WitnessResumeStats witnessResumeStats;
    struct Refine2DStats {
        unsigned long long calls_2d=0,refine_slots=0;
        unsigned long long defect_vector_steps=0,defect_lane_vertex_steps=0,sat_union_vertices=0;
        unsigned long long union_c_vertices=0,candidate_lane_slots=0;
        unsigned long long bad_pair_slots=0,bad_no_slots=0,bad_sat_slots=0,static_bad_union_slots=0;
        unsigned long long degree_candidate_slots=0,degree_vector_steps=0,all_static_skip_vertices=0;
        unsigned long long static_erased_slots_runtime=0,degree_removed_slots=0;
        unsigned long long bulk_static_removed_slots=0,bulk_union_c_after=0;
    };
    extern thread_local Refine2DStats refine2DStats;
    struct PivotReuseStats {
        unsigned long long calls=0;
        unsigned long long union_p_vertices=0,union_d_vertices=0,union_u_vertices=0;
        unsigned long long first_dg_popcnt_vectors=0,second_dg_popcnt_vectors=0,second_pd_popcnt_vectors=0;
        unsigned long long lazy_pd_needed_vertices=0,lazy_pd_skipped_vertices=0,lazy_pd_need_lane_slots=0;
        unsigned long long bitslice_u_steps=0,bitslice_p_steps=0,bitslice_counter_updates=0;
        unsigned long long pmin_argmin_rounds=0,final_dg_argmin_rounds=0,final_pd_argmin_rounds=0;
        unsigned long long domain_lane_slots=0,zero_domain_lanes=0;
        // Second-pass domain occupancy.  Updated only in VECTOR_TELEMETRY builds.
        unsigned long long second_active_lane_slots=0;
        unsigned long long occ_vertices[9]={0,0,0,0,0,0,0,0,0};
        // Hybrid accounting: low-occupancy vertices use scalar POPCNT only
        // on live lanes; higher occupancy keeps the AVX2 8-lane popcount.
        unsigned long long scalar_vertices=0,vector_vertices=0,scalar_lane_evals=0;
    };
    extern thread_local PivotReuseStats pivotReuseStats;
    struct DegreePeelStats {
        unsigned long long calls=0;
        unsigned long long initial_union_vertices=0,initial_candidate_slots=0;
        // Fixed-point maintenance: synchronous fixed-point recomputation.
        unsigned long long recompute_rounds=0,recompute_extra_rounds=0,recompute_max_rounds=0;
        unsigned long long recompute_vector_steps=0,recompute_candidate_slots=0;
        // D2: exact degree initialization once, then deletion-driven decrements.
        unsigned long long init_vector_steps=0,init_candidate_slots=0;
        unsigned long long queue_vertex_steps=0,queue_lane_removals=0,queue_push_slots=0;
        unsigned long long neighbor_vector_updates=0,neighbor_lane_updates=0;
        unsigned long long removed_slots=0;
    };
    extern thread_local DegreePeelStats degreePeelStats;
    // Workload-aware branch packet formation profile. Formal PROFILE=0 builds
    // compile all updates away; the scheduler itself is enabled only when
    // WORKLOAD_BATCH_MODE is 1 or 2.
    struct WorkBatchStats {
        unsigned long long ready_states=0,bucket_pushes=0,full_flushes=0,partial_flushes=0;
        unsigned long long flush_batches=0,flush_slots=0,pending_peak=0;
        unsigned long long occ_hist[9]={0,0,0,0,0,0,0,0,0};
        unsigned long long c_span_sum=0,c_span_max=0,u_span_sum=0,u_span_max=0;
        unsigned long long ready_c_1_4=0,ready_c_5_8=0,ready_c_9_16=0,ready_c_17_32=0;
        unsigned long long ready_u_le16=0,ready_u_17_23=0,ready_u_24_32=0;
        unsigned long long pivot_p_vector_steps=0,pivot_p_active_lane_slots=0;
    };
    extern thread_local WorkBatchStats workBatchStats;

    // the State32 prepare path; optional PacketPrepare validations benchmark the vector
    // fast-gate without changing formal search semantics.
    struct SteadyKernelProfileStats {
        unsigned long long state32_entries=0;
        unsigned long long entry_u_1_8=0,entry_u_9_16=0,entry_u_17_24=0,entry_u_25_32=0;
        unsigned long long state32_states=0;
        unsigned long long state_u_1_8=0,state_u_9_16=0,state_u_17_24=0,state_u_25_32=0;
        unsigned long long state32_ready=0,ready_u_le16=0,ready_u_17_24=0,ready_u_25_32=0;
        unsigned long long state32_packets=0,state32_slots=0,state32_full_packets=0;
        unsigned long long state32_occ[9]={0,0,0,0,0,0,0,0,0};

        unsigned long long state32_inclusive_calls=0,state32_inclusive_samples=0,state32_inclusive_ns=0;
        unsigned long long state32_context_calls=0,state32_context_samples=0,state32_context_ns=0;
        unsigned long long state32_prepare_calls=0,state32_prepare_samples=0,state32_prepare_ns=0;
        unsigned long long state32_pivot_calls=0,state32_pivot_samples=0,state32_pivot_ns=0;
        unsigned long long state32_refine_calls=0,state32_refine_samples=0,state32_refine_ns=0;
        unsigned long long state32_max_calls=0,state32_max_samples=0,state32_max_ns=0;
        unsigned long long state64_prepare_calls=0,state64_prepare_samples=0,state64_prepare_ns=0;
        unsigned long long state64_pivot_calls=0,state64_pivot_samples=0,state64_pivot_ns=0;
        unsigned long long state64_refine_calls=0,state64_refine_samples=0,state64_refine_ns=0;

        // A: exact prepare outcome/scan counters by U bucket: <=8, 9-16, 17-24, 25-32.
        unsigned long long prep_prechecked[4]={0,0,0,0};
        unsigned long long prep_calls_b[4]={0,0,0,0},prep_lb_reject[4]={0,0,0,0};
        unsigned long long prep_inherited_present[4]={0,0,0,0},prep_witness_hit[4]={0,0,0,0};
        unsigned long long prep_witness_removed[4]={0,0,0,0},prep_witness_recovered[4]={0,0,0,0};
        unsigned long long prep_fullscan_calls[4]={0,0,0,0},prep_fullscan_vertices[4]={0,0,0,0};
        unsigned long long prep_fullscan_finds[4]={0,0,0,0},prep_fullscan_whole[4]={0,0,0,0};
        unsigned long long prep_whole_terminal[4]={0,0,0,0},prep_leaf_terminal[4]={0,0,0,0},prep_ready_b[4]={0,0,0,0};
        unsigned long long prep_bucket_samples[4]={0,0,0,0};
        unsigned long long prep_lb_ns[4]={0,0,0,0},prep_witness_ns[4]={0,0,0,0},prep_fullscan_ns[4]={0,0,0,0},prep_finish_ns[4]={0,0,0,0};

        unsigned long long prep8_batches_seen=0,prep8_samples=0,prep8_lanes=0,prep8_mismatches=0;
        unsigned long long prep8_prechecked=0,prep8_lb_reject=0,prep8_witness_hit=0,prep8_fallback=0;
        unsigned long long prep8_scalar_ns=0,prep8_vector_ns=0;

        // vectorizes unresolved full-U witness scanning across eight states.
        unsigned long long packetMax_batches_seen=0,packetMax_samples=0,packetMax_lanes=0,packetMax_mismatches=0;
        unsigned long long packetMax_raw_packets=0,packetMax_raw_slots=0,packetMax_raw_occ[9]={0,0,0,0,0,0,0,0,0};
        unsigned long long packetMax_prechecked=0,packetMax_lb_reject=0,packetMax_witness_hit=0,packetMax_fullscan_lanes=0;
        unsigned long long packetMax_whole=0,packetMax_leaf=0,packetMax_ready=0;
        unsigned long long packetMax_scalar_ns=0,packetMax_vector_ns=0;

        // U fixed and SIMD-vectorizes the candidate-vertex dimension instead
        // of batching divergent DFS states. Buckets are by popcount(U):
        // <=8, 9-16, 17-24, 25-32.
        unsigned long long wc8_seen=0;
        unsigned long long wc8_eligible[4]={0,0,0,0},wc8_samples[4]={0,0,0,0};
        unsigned long long wc8_whole[4]={0,0,0,0},wc8_finds[4]={0,0,0,0},wc8_mismatches[4]={0,0,0,0};
        unsigned long long wc8_scalar_vertices[4]={0,0,0,0},wc8_vector_chunks[4]={0,0,0,0};
        unsigned long long wc8_scalar_ns[4]={0,0,0,0},wc8_vector_ns[4]={0,0,0,0};

        // WIDE_RELEASE: formal single-State32 x 8-vertex whole-check.  Formal timing
        // binaries keep exact workload profiling disabled; only the low-rate
        // scalar verifier updates verify counters. WC8D enables exact counters.
        unsigned long long wc8f_calls[4]={0,0,0,0},wc8f_whole[4]={0,0,0,0},wc8f_finds[4]={0,0,0,0};
        unsigned long long wc8f_vector_chunks[4]={0,0,0,0},wc8f_scalar_equiv_vertices[4]={0,0,0,0};
        unsigned long long wc8f_verify_seen=0,wc8f_verify_samples=0,wc8f_verify_mismatches=0;


        inline void merge(const SteadyKernelProfileStats& o){
#define SKP_ADD(x) x += o.x
            SKP_ADD(state32_entries); SKP_ADD(entry_u_1_8); SKP_ADD(entry_u_9_16); SKP_ADD(entry_u_17_24); SKP_ADD(entry_u_25_32);
            SKP_ADD(state32_states); SKP_ADD(state_u_1_8); SKP_ADD(state_u_9_16); SKP_ADD(state_u_17_24); SKP_ADD(state_u_25_32);
            SKP_ADD(state32_ready); SKP_ADD(ready_u_le16); SKP_ADD(ready_u_17_24); SKP_ADD(ready_u_25_32);
            SKP_ADD(state32_packets); SKP_ADD(state32_slots); SKP_ADD(state32_full_packets);
            for(int i=0;i<9;i++) state32_occ[i]+=o.state32_occ[i];
            SKP_ADD(state32_inclusive_calls); SKP_ADD(state32_inclusive_samples); SKP_ADD(state32_inclusive_ns);
            SKP_ADD(state32_context_calls); SKP_ADD(state32_context_samples); SKP_ADD(state32_context_ns);
            SKP_ADD(state32_prepare_calls); SKP_ADD(state32_prepare_samples); SKP_ADD(state32_prepare_ns);
            SKP_ADD(state32_pivot_calls); SKP_ADD(state32_pivot_samples); SKP_ADD(state32_pivot_ns);
            SKP_ADD(state32_refine_calls); SKP_ADD(state32_refine_samples); SKP_ADD(state32_refine_ns);
            SKP_ADD(state32_max_calls); SKP_ADD(state32_max_samples); SKP_ADD(state32_max_ns);
            SKP_ADD(state64_prepare_calls); SKP_ADD(state64_prepare_samples); SKP_ADD(state64_prepare_ns);
            SKP_ADD(state64_pivot_calls); SKP_ADD(state64_pivot_samples); SKP_ADD(state64_pivot_ns);
            SKP_ADD(state64_refine_calls); SKP_ADD(state64_refine_samples); SKP_ADD(state64_refine_ns);
            for(int i=0;i<4;i++){
                prep_prechecked[i]+=o.prep_prechecked[i]; prep_calls_b[i]+=o.prep_calls_b[i]; prep_lb_reject[i]+=o.prep_lb_reject[i];
                prep_inherited_present[i]+=o.prep_inherited_present[i]; prep_witness_hit[i]+=o.prep_witness_hit[i];
                prep_witness_removed[i]+=o.prep_witness_removed[i]; prep_witness_recovered[i]+=o.prep_witness_recovered[i];
                prep_fullscan_calls[i]+=o.prep_fullscan_calls[i]; prep_fullscan_vertices[i]+=o.prep_fullscan_vertices[i];
                prep_fullscan_finds[i]+=o.prep_fullscan_finds[i]; prep_fullscan_whole[i]+=o.prep_fullscan_whole[i];
                prep_whole_terminal[i]+=o.prep_whole_terminal[i]; prep_leaf_terminal[i]+=o.prep_leaf_terminal[i]; prep_ready_b[i]+=o.prep_ready_b[i];
                prep_bucket_samples[i]+=o.prep_bucket_samples[i]; prep_lb_ns[i]+=o.prep_lb_ns[i]; prep_witness_ns[i]+=o.prep_witness_ns[i];
                prep_fullscan_ns[i]+=o.prep_fullscan_ns[i]; prep_finish_ns[i]+=o.prep_finish_ns[i];
            }
            SKP_ADD(prep8_batches_seen); SKP_ADD(prep8_samples); SKP_ADD(prep8_lanes); SKP_ADD(prep8_mismatches);
            SKP_ADD(prep8_prechecked); SKP_ADD(prep8_lb_reject); SKP_ADD(prep8_witness_hit); SKP_ADD(prep8_fallback); SKP_ADD(prep8_scalar_ns); SKP_ADD(prep8_vector_ns);
            SKP_ADD(packetMax_batches_seen); SKP_ADD(packetMax_samples); SKP_ADD(packetMax_lanes); SKP_ADD(packetMax_mismatches);
            SKP_ADD(packetMax_raw_packets); SKP_ADD(packetMax_raw_slots); for(int i=0;i<9;i++) packetMax_raw_occ[i]+=o.packetMax_raw_occ[i];
            SKP_ADD(packetMax_prechecked); SKP_ADD(packetMax_lb_reject); SKP_ADD(packetMax_witness_hit); SKP_ADD(packetMax_fullscan_lanes);
            SKP_ADD(packetMax_whole); SKP_ADD(packetMax_leaf); SKP_ADD(packetMax_ready); SKP_ADD(packetMax_scalar_ns); SKP_ADD(packetMax_vector_ns);
            SKP_ADD(wc8_seen);
            for(int i=0;i<4;i++){
                wc8_eligible[i]+=o.wc8_eligible[i]; wc8_samples[i]+=o.wc8_samples[i];
                wc8_whole[i]+=o.wc8_whole[i]; wc8_finds[i]+=o.wc8_finds[i]; wc8_mismatches[i]+=o.wc8_mismatches[i];
                wc8_scalar_vertices[i]+=o.wc8_scalar_vertices[i]; wc8_vector_chunks[i]+=o.wc8_vector_chunks[i];
                wc8_scalar_ns[i]+=o.wc8_scalar_ns[i]; wc8_vector_ns[i]+=o.wc8_vector_ns[i];
                wc8f_calls[i]+=o.wc8f_calls[i]; wc8f_whole[i]+=o.wc8f_whole[i]; wc8f_finds[i]+=o.wc8f_finds[i];
                wc8f_vector_chunks[i]+=o.wc8f_vector_chunks[i]; wc8f_scalar_equiv_vertices[i]+=o.wc8f_scalar_equiv_vertices[i];
            }
            SKP_ADD(wc8f_verify_seen); SKP_ADD(wc8f_verify_samples); SKP_ADD(wc8f_verify_mismatches);
#undef SKP_ADD
        }
    };
    extern thread_local SteadyKernelProfileStats steadyKernelProfileStats;


