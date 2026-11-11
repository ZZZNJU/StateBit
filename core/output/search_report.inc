        printf("Number of %d-plex: %llu\n",k,cntMaxPlex);
        printf("Running time %.3f\n",durat);
        {
            const double checks=preUpdateKPlexTotal.checks?static_cast<double>(preUpdateKPlexTotal.checks):1.0;
            printf("PREUPDATE_KPLEX_CONFIG mode=2 direct_terminal=1 placement=kSearch_before_updateCand1 hotpath_layout_changed=0\n");
            printf("PREUPDATE_KPLEX_STATS checks=%llu hit_nodes=%llu miss_nodes=%llu hit_pct=%.9f miss_size_nodes=%llu miss_p_nodes=%llu miss_c_nodes=%llu scan_p_vertices=%llu scan_c_vertices=%llu avg_scan_p=%.6f avg_scan_c=%.6f avg_hit_u=%.6f avg_hit_c=%.6f max_scan_p=%llu max_scan_c=%llu direct_terminal_hits=%llu avoided_candidates=%llu\n",
                   preUpdateKPlexTotal.checks,preUpdateKPlexTotal.hit_nodes,preUpdateKPlexTotal.miss_nodes,100.0*preUpdateKPlexTotal.hit_nodes/checks,
                   preUpdateKPlexTotal.miss_size_nodes,preUpdateKPlexTotal.miss_p_nodes,preUpdateKPlexTotal.miss_c_nodes,preUpdateKPlexTotal.scan_p_vertices,preUpdateKPlexTotal.scan_c_vertices,
                   preUpdateKPlexTotal.scan_p_vertices/checks,preUpdateKPlexTotal.scan_c_vertices/checks,
                   preUpdateKPlexTotal.hit_nodes?static_cast<double>(preUpdateKPlexTotal.hit_u_sum)/preUpdateKPlexTotal.hit_nodes:0.0,
                   preUpdateKPlexTotal.hit_nodes?static_cast<double>(preUpdateKPlexTotal.hit_c_sum)/preUpdateKPlexTotal.hit_nodes:0.0,
                   preUpdateKPlexTotal.max_scan_p,preUpdateKPlexTotal.max_scan_c,preUpdateKPlexTotal.direct_terminal_hits,preUpdateKPlexTotal.avoided_candidates);
        }
        {
            const unsigned long long calls=denseTouchTotal.addG_calls+denseTouchTotal.subG_calls+denseTouchTotal.addP_calls+denseTouchTotal.subP_calls;
            printf("DENSE_TOUCH_CONFIG enabled=%d hotpath_counter_increments=%d\n",DENSE_TOUCH_TELEMETRY,DENSE_TOUCH_TELEMETRY);
            printf("DENSE_TOUCH_STATS addG_calls=%llu subG_calls=%llu addP_calls=%llu subP_calls=%llu total_calls=%llu neighbor_touch_estimate=%llu complement_touches=%llu touch_reduction_ratio=%.9f\n",
                   denseTouchTotal.addG_calls,denseTouchTotal.subG_calls,denseTouchTotal.addP_calls,denseTouchTotal.subP_calls,calls,denseTouchTotal.neighbor_touch_estimate,denseTouchTotal.complement_touches,
                   denseTouchTotal.complement_touches?static_cast<double>(denseTouchTotal.neighbor_touch_estimate)/static_cast<double>(denseTouchTotal.complement_touches):0.0);
        }
        printf("ADAPTIVE_BOOKKEEP_CONFIG mode=%d mode_name=%s adaptive_factor=%d validation_selector=%d\n",BOOKKEEP_MODE,bookkeepModeName(),ADAPTIVE_DENSE_FACTOR,ADAPTIVE_VALIDATION_SELECTOR);
        printf("ADAPTIVE_BOOKKEEP_SEEDS seeds=%llu actual_dense_seeds=%llu actual_sparse_seeds=%llu predicted_dense_seeds=%llu predicted_sparse_seeds=%llu actual_dense_pct=%.9f predicted_dense_pct=%.9f neighbor_proxy=%llu nonneighbor_proxy=%llu proxy_ratio=%.9f complement_build_calls=%llu complement_build_vertices=%llu complement_build_entries=%llu complement_build_sec=%.9f\n",
               seedModeTotal.seeds,seedModeTotal.actual_dense_seeds,seedModeTotal.actual_sparse_seeds,seedModeTotal.predicted_dense_seeds,seedModeTotal.predicted_sparse_seeds,
               seedModeTotal.seeds?100.0*static_cast<double>(seedModeTotal.actual_dense_seeds)/static_cast<double>(seedModeTotal.seeds):0.0,
               seedModeTotal.seeds?100.0*static_cast<double>(seedModeTotal.predicted_dense_seeds)/static_cast<double>(seedModeTotal.seeds):0.0,
               seedModeTotal.neighbor_proxy,seedModeTotal.nonneighbor_proxy,
               seedModeTotal.neighbor_proxy?static_cast<double>(seedModeTotal.nonneighbor_proxy)/static_cast<double>(seedModeTotal.neighbor_proxy):0.0,
               seedModeTotal.complement_build_calls,seedModeTotal.complement_build_vertices,seedModeTotal.complement_build_entries,
               static_cast<double>(seedModeTotal.complement_build_ns)/1e9);
        printf("ADAPTIVE_BOOKKEEP_WORK actual_dense_vertices=%llu actual_sparse_vertices=%llu actual_dense_n2=%llu actual_sparse_n2=%llu dense_neighbor_proxy=%llu dense_nonneighbor_proxy=%llu sparse_neighbor_proxy=%llu sparse_nonneighbor_proxy=%llu\n",
               seedModeTotal.actual_dense_vertices,seedModeTotal.actual_sparse_vertices,seedModeTotal.actual_dense_n2,seedModeTotal.actual_sparse_n2,
               seedModeTotal.dense_neighbor_proxy,seedModeTotal.dense_nonneighbor_proxy,seedModeTotal.sparse_neighbor_proxy,seedModeTotal.sparse_nonneighbor_proxy);
        printf("MT_STATEBIT_CONFIG engine=shared execution=%s requested_threads=%d actual_threads=%d scheduler=atomic_dynamic granularity=1 avx_per_worker=1 shared_state_pool=0 terminal_task_handoff=1 recursive_timeout_split=%d list_split_us=%d bit_split_us=%d queue_cap=%zu edbt_style_split=1 split_grain=%d tail_expansion=%d dds_demand_split=%d dds_bit_donate=%d demand_plus_one_split=%d dmfs_split=%d dtfb_split=%d dynamic_seed_workspace=1 dynamic_prepared_task=1\n",
               parallelExecution?"parallel":"sequential",workerThreads,actualThreads,MT_RECURSIVE_SPLIT,MT_LIST_SPLIT_US,MT_BIT_SPLIT_US,MT_QUEUE_CAP,
               MT_EDBT_GRAIN_SIZE,MT_TAIL_EXPANSION,MT_DDS_DEMAND_SPLIT,MT_DDS_BIT_DONATE,MT_DEMAND_PLUS_ONE_SPLIT,MT_DMFS_SPLIT,MT_DTFB_SPLIT);
        {
            const char* dtfbPartition=MT_DTFB_DONATE_POLICY==0?"half":"keep_one_donate_rest";
            const char* dtfbGateOrder=MT_DTFB_DEMAND_FIRST?"demand_first":"scan_first";
            printf("MT_DTFB_CONFIG enabled=%d width128_recursive_mt=%d policy=demand_triggered_timeout_frontier_batch frontier_partition=%s demand_gate_order=%s split_epoch=fresh_after_frontier_split list_split_us=%d bit_split_us=%d grain=%d queue_cap=%zu tail=0 dmfs_admission=0 dmfs_frontier_spill=0 dds=0 demand_source=actual_idle_workers one_unmet_demand_per_batch=1 keep_newest_local=%d ordinary_split_admission=bounded_local_queue thread_count_specialization=0 fixed_batch_size=0\n",
                   MT_DTFB_SPLIT,DISABLE_128_MT?0:1,dtfbPartition,dtfbGateOrder,MT_LIST_SPLIT_US,MT_BIT_SPLIT_US,MT_EDBT_GRAIN_SIZE,MT_QUEUE_CAP,MT_DTFB_DONATE_POLICY==1?1:0);
        }
        printf("MT_DDS_CONFIG enabled=%d bit_donate=%d trigger=live_idle_demand split_requires_timeout=1 list_split_us=%d bit_split_us=%d grain=%d queue_cap=%zu queue_cap_mode=%s\n",
               MT_DDS_DEMAND_SPLIT,MT_DDS_BIT_DONATE,MT_LIST_SPLIT_US,MT_BIT_SPLIT_US,MT_EDBT_GRAIN_SIZE,MT_QUEUE_CAP,(MT_QUEUE_CAP==0?"unbounded_by_demand":"fixed_cap"));
        printf("MT_PLUS_ONE_CONFIG enabled=%d policy=global_ready_lt_idle_plus_one reserve_tasks=1 split_requires_timeout=1 list_split_us=%d bit_split_us=%d grain=%d queue_cap=%zu thread_count_specialization=0\n",
               MT_DEMAND_PLUS_ONE_SPLIT,MT_LIST_SPLIT_US,MT_BIT_SPLIT_US,MT_EDBT_GRAIN_SIZE,MT_QUEUE_CAP);
        printf("MT_DMFS_CONFIG enabled=%d policy=%s target=idle_plus_one bulk_source=%s burst_reservation=full_live_deficit global_frontier_credit=1 bulk_size=live_deficit keep_local_states=1 frontier_priority=max_C split_requires_timeout=1 list_split_us=%d bit_split_us=%d grain=%d queue_cap=%zu thread_count_specialization=0\n",
               MT_DMFS_SPLIT,MT_DMFS_SPLIT?"demand_matched_frontier_spill":"off",MT_DMFS_SPLIT?"state64_private_frontier":"off",MT_LIST_SPLIT_US,MT_BIT_SPLIT_US,MT_EDBT_GRAIN_SIZE,MT_QUEUE_CAP);
        printf("WITNESS_RESUME_CONFIG enabled=%d telemetry=%d verify=%d profile_shift=%d verify_shift=%d semantics=resume_after_first_inherited_witness\n",
               WITNESS_RESUME,WITNESS_RESUME_PROFILE,WITNESS_RESUME_VERIFY,WITNESS_RESUME_PROFILE_SHIFT,WITNESS_RESUME_VERIFY_SHIFT);
        printf("WHOLECHECK8V_FORMAL_CONFIG enabled=%d telemetry=%d verify=%d verify_shift=%d width=8 mode=single_state_x_8_vertices vector_load=contiguous_nonadj_rows early_exit=chunk first_witness_order_preserved=1\n",
               WHOLECHECK8V_FORMAL,WHOLECHECK8V_FORMAL_PROFILE,WHOLECHECK8V_VERIFY,WHOLECHECK8V_VERIFY_SHIFT);
        printf("MT_RECURSIVE_SPLIT_STATS exported_list=%llu exported_bit=%llu exported_bitbatch=%llu exported_word128=%llu exported_word256=%llu stolen_list=%llu stolen_bit=%llu stolen_bitbatch=%llu stolen_word128=%llu stolen_word256=%llu peak_queued=%llu\n",
               taskPool.exported_list.load(),taskPool.exported_bit.load(),taskPool.exported_bitbatch.load(),taskPool.exported_state128.load(),taskPool.exported_state256.load(),
               taskPool.stolen_list.load(),taskPool.stolen_bit.load(),taskPool.stolen_bitbatch.load(),taskPool.stolen_state128.load(),taskPool.stolen_state256.load(),taskPool.peak_queued.load());
        {
            const unsigned long long t64=taskPool.dtfb_state64_batch_tasks.load(),s64=taskPool.dtfb_state64_batch_states.load();
            printf("MT_DTFB_BATCH_STATS state64_batch_tasks=%llu state64_batch_states=%llu state64_avg_batch=%.6f state64_max_batch=%llu state64_stolen_batches=%llu state128_batch_tasks=0 state128_batch_states=0 state128_avg_batch=0.000000 state128_max_batch=0 state128_stolen_batches=0\n",
                   t64,s64,t64?static_cast<double>(s64)/static_cast<double>(t64):0.0,taskPool.dtfb_state64_batch_max.load(),taskPool.dtfb_state64_batch_stolen.load());
        }
        {
            const int idleFinal=taskPool.idleWorkers.load(std::memory_order_relaxed);
            const int reservedFinal=taskPool.dds_reservations.load(std::memory_order_relaxed);
            printf("MT_DDS_STATS enabled=%d bit_donate=%d demand_posted=%llu demand_claimed=%llu demand_released=%llu materialized_tasks=%llu donated_list=%llu donated_bit=%llu donated_word128=%llu donated_word256=%llu peak_unmet_demand=%llu idle_wait_sec=%.6f wait_satisfied=%llu idle_final=%d reservations_final=%d unmet_demand_final=%llu\n",
                   MT_DDS_DEMAND_SPLIT,MT_DDS_BIT_DONATE,taskPool.dds_demand_posted.load(),taskPool.dds_demand_claimed.load(),taskPool.dds_demand_released.load(),taskPool.dds_materialized.load(),taskPool.dds_donated_list.load(),taskPool.dds_donated_bit.load(),taskPool.dds_donated_state128.load(),taskPool.dds_donated_state256.load(),taskPool.dds_peak_unmet.load(),taskPool.dds_idle_wait_ns.load()/1e9,taskPool.dds_wait_satisfied.load(),idleFinal,reservedFinal,taskPool.ddsUnmetDemand());
        }
        printf("MT_DTFB_DEMAND_STATS enabled=%d posted=%llu claimed=%llu released=%llu materialized_batches=%llu peak_unmet_demand=%llu idle_wait_sec=%.6f wait_satisfied=%llu idle_final=%d reservations_final=%d unmet_demand_final=%llu\n",
               MT_DTFB_SPLIT,taskPool.dds_demand_posted.load(),taskPool.dds_demand_claimed.load(),taskPool.dds_demand_released.load(),
               taskPool.dtfb_state64_batch_tasks.load(),taskPool.dds_peak_unmet.load(),taskPool.dds_idle_wait_ns.load()/1e9,taskPool.dds_wait_satisfied.load(),
               taskPool.idleWorkers.load(std::memory_order_relaxed),taskPool.dds_reservations.load(std::memory_order_relaxed),taskPool.ddsUnmetDemand());
        printf("MT_PLUS_ONE_STATS enabled=%d claimed=%llu released=%llu materialized_tasks=%llu claimed_idle0=%llu claimed_idlepos=%llu peak_supply=%llu peak_target=%llu idle_entries=%llu idle_wait_sec=%.6f wait_satisfied=%llu supply_final=%d ready_final=%llu\n",
               MT_DEMAND_PLUS_ONE_SPLIT,taskPool.plus_one_claimed.load(),taskPool.plus_one_released.load(),taskPool.plus_one_materialized.load(),
               taskPool.plus_one_claim_idle0.load(),taskPool.plus_one_claim_idlepos.load(),taskPool.plus_one_peak_supply.load(),taskPool.plus_one_peak_target.load(),
               taskPool.plus_one_idle_entries.load(),taskPool.plus_one_idle_wait_ns.load()/1e9,taskPool.plus_one_wait_satisfied.load(),taskPool.plus_one_supply.load(),taskPool.queued.load());
        printf("MT_DMFS_STATS enabled=%d claim_calls=%llu claimed=%llu released=%llu permit_uses=%llu credit_hits=%llu abandoned_credits=%llu materialized_tasks=%llu claimed_idle0=%llu claimed_idlepos=%llu peak_supply=%llu peak_target=%llu peak_credits=%llu bulk_claim_calls=%llu bulk_claimed=%llu frontier_materialized=%llu max_burst=%llu idle_entries=%llu idle_wait_sec=%.6f wait_satisfied=%llu credits_final=%d supply_final=%d ready_final=%llu\n",
               MT_DMFS_SPLIT,taskPool.dmfs_claim_calls.load(),taskPool.dmfs_claimed.load(),taskPool.dmfs_released.load(),taskPool.dmfs_permit_uses.load(),taskPool.dmfs_credit_hits.load(),taskPool.dmfs_abandoned_credits.load(),taskPool.dmfs_materialized.load(),
               taskPool.dmfs_claim_idle0.load(),taskPool.dmfs_claim_idlepos.load(),taskPool.dmfs_peak_supply.load(),taskPool.dmfs_peak_target.load(),taskPool.dmfs_peak_credits.load(),
               taskPool.dmfs_bulk_claim_calls.load(),taskPool.dmfs_bulk_claimed.load(),taskPool.dmfs_frontier_materialized.load(),taskPool.dmfs_max_burst.load(),
               taskPool.dmfs_idle_entries.load(),taskPool.dmfs_idle_wait_ns.load()/1e9,taskPool.dmfs_wait_satisfied.load(),taskPool.dmfs_credits.load(),taskPool.dmfs_supply.load(),taskPool.queued.load());
        printf("MT_TAIL_CONFIG enabled=%d min_idle=%d remaining_seeds=%d waterline_mult=%d max_outstanding_mult=%d bit_min_c=%d spill_stage=state64_private_stack keep_local_states=1 reexport_tail_tasks=%d batch_states=%d batch_min_states=%d\n",
               MT_TAIL_EXPANSION,MT_TAIL_MIN_IDLE,MT_TAIL_REMAINING_SEEDS,MT_TAIL_WATERLINE_MULT,MT_TAIL_MAX_OUTSTANDING_MULT,MT_TAIL_BIT_MIN_C,MT_TAIL_REEXPORT,MT_TAIL_BATCH_STATES,MT_TAIL_BATCH_MIN_STATES);
        {
            const unsigned long long bt=taskPool.tail_exported_bitbatch_tasks.load();
            const unsigned long long bs=taskPool.tail_exported_bitbatch_states.load();
            printf("MT_TAIL_EXPANSION_STATS tail_mode_entries=%llu tail_refill_requests=%llu tail_exported_list=%llu tail_exported_bit=%llu tail_exported_bitbatch_tasks=%llu tail_exported_bitbatch_states=%llu tail_bitbatch_avg_states=%.6f tail_bitbatch_max_states=%llu tail_exported_word128=%llu tail_exported_word256=%llu tail_idle_worker_sec=%.6f outstanding_final=%llu\n",
                   taskPool.tail_mode_entries.load(),taskPool.tail_refill_requests.load(),taskPool.tail_exported_list.load(),taskPool.tail_exported_bit.load(),bt,bs,bt?static_cast<double>(bs)/static_cast<double>(bt):0.0,taskPool.tail_bitbatch_max_states.load(),taskPool.tail_exported_state128.load(),taskPool.tail_exported_state256.load(),
                   taskPool.tail_idle_ns.load()/1e9,taskPool.exported.load()>=taskPool.executed.load()?taskPool.exported.load()-taskPool.executed.load():0ULL);
        }
        printf("MT_TASK_RUNTIME_HIST lt10us=%llu us10_100=%llu us100_1000=%llu ms1_10=%llu ms10_100=%llu ge100ms=%llu\n",
               taskPool.runtime_lt10us.load(),taskPool.runtime_lt100us.load(),taskPool.runtime_lt1ms.load(),taskPool.runtime_lt10ms.load(),taskPool.runtime_lt100ms.load(),taskPool.runtime_ge100ms.load());
        {
            const double finalSec=std::chrono::duration<double>(std::chrono::steady_clock::now()-progressEpoch).count();
            const double t90=t90_us.load()<0?0.0:t90_us.load()/1e6;
            const double t99=t99_us.load()<0?t90:t99_us.load()/1e6;
            const double t999=t999_us.load()<0?t99:t999_us.load()/1e6;
            printf("MT_TAIL_TIMING time_to_90_sec=%.6f time_90_to_99_sec=%.6f time_99_to_99_9_sec=%.6f time_99_9_to_done_sec=%.6f total_sec=%.6f\n",
                   t90,std::max(0.0,t99-t90),std::max(0.0,t999-t99),std::max(0.0,finalSec-t999),finalSec);
        }
        {
            const double n=forcedBaseTotal.nodes_seen?static_cast<double>(forcedBaseTotal.nodes_seen):1.0;
            const double cb=forcedBaseTotal.c_before_sum?static_cast<double>(forcedBaseTotal.c_before_sum):1.0;
            const char *modeName=(FORCED_BASE_FOLD_MODE==0?"baseline":(FORCED_BASE_FOLD_MODE==1?"root_only":"recursive"));
            printf("FORCED_BASE_CONFIG enabled=1 mode=%d mode_name=%s forced_rule=degreeInU_eq_Uminus1 physical_P_unchanged=1 subG_on_fold=0 per_forced_X_update=0 terminal_X_exact=1 statebit_changed=%d\n",FORCED_BASE_FOLD_MODE,modeName,(FORCED_BASE_BIT64_THRESHOLD>0?1:0));
            printf("FORCED_BASE_STATS nodes_seen=%llu root_nodes=%llu scan_calls=%llu scan_candidates=%llu fold_nodes=%llu forced_vertices=%llu max_fold_batch=%llu max_forced_base=%llu c_before_sum=%llu c_after_sum=%llu forced_base_before_sum=%llu forced_base_after_sum=%llu active_before_sum=%llu active_after_sum=%llu avg_c_before=%.6f avg_c_after=%.6f c_reduction_pct=%.6f avg_forced_base_after=%.6f avg_active_after=%.6f\n",
                   forcedBaseTotal.nodes_seen,forcedBaseTotal.root_nodes,forcedBaseTotal.scan_calls,forcedBaseTotal.scan_candidates,
                   forcedBaseTotal.fold_nodes,forcedBaseTotal.forced_vertices,forcedBaseTotal.max_fold_batch,forcedBaseTotal.max_forced_base,
                   forcedBaseTotal.c_before_sum,forcedBaseTotal.c_after_sum,forcedBaseTotal.forced_base_before_sum,forcedBaseTotal.forced_base_after_sum,
                   forcedBaseTotal.active_before_sum,forcedBaseTotal.active_after_sum,
                   static_cast<double>(forcedBaseTotal.c_before_sum)/n,static_cast<double>(forcedBaseTotal.c_after_sum)/n,
                   100.0*(1.0-static_cast<double>(forcedBaseTotal.c_after_sum)/cb),
                   static_cast<double>(forcedBaseTotal.forced_base_after_sum)/n,static_cast<double>(forcedBaseTotal.active_after_sum)/n);
            printf("FORCED_BASE_WIDTH active_le256=%llu active_le128=%llu active_le64=%llu active_le32=%llu active_le16=%llu active_le256_pct=%.6f active_le128_pct=%.6f active_le64_pct=%.6f active_le32_pct=%.6f active_le16_pct=%.6f\n",
                   forcedBaseTotal.active_le256,forcedBaseTotal.active_le128,forcedBaseTotal.active_le64,forcedBaseTotal.active_le32,forcedBaseTotal.active_le16,
                   100.0*forcedBaseTotal.active_le256/n,100.0*forcedBaseTotal.active_le128/n,100.0*forcedBaseTotal.active_le64/n,100.0*forcedBaseTotal.active_le32/n,100.0*forcedBaseTotal.active_le16/n);
            const double ps=forcedBaseAdaptiveTotal.payoff_samples?static_cast<double>(forcedBaseAdaptiveTotal.payoff_samples):1.0;
            const double pns=forcedBaseAdaptiveTotal.payoff_normal_samples?static_cast<double>(forcedBaseAdaptiveTotal.payoff_normal_samples):1.0;
            const double pps=forcedBaseAdaptiveTotal.payoff_probe_samples?static_cast<double>(forcedBaseAdaptiveTotal.payoff_probe_samples):1.0;
            const double p256s=forcedBaseAdaptiveTotal.payoff_state256_samples?static_cast<double>(forcedBaseAdaptiveTotal.payoff_state256_samples):1.0;
            const char *state256PayoffModeName=(FORCED_BASE_STATE256_PAYOFF_MODE==0?"off":(FORCED_BASE_STATE256_PAYOFF_MODE==1?"validation":"live"));
            printf("FORCED_BASE_STATE256_PAYOFF_CONFIG mode=%d mode_name=%s payoff_definition=delta_state256_plus_state128_plus_bit local_thread_counters=1 folded_only=1\n",
                   FORCED_BASE_STATE256_PAYOFF_MODE,state256PayoffModeName);
            printf("FORCED_BASE_ADAPTIVE_CONFIG terminal_bypass=%d payoff_threshold=%.1f payoff_threshold_x2=%d payoff_window=%d payoff_warmup=%d payoff_cooldown=%d payoff_hysteresis_windows=%d payoff_gate_bypass=%d terminal_first=1 pure_subtree_fallback=%d\n",
                   FORCED_BASE_TERMINAL_BYPASS,0.5*static_cast<double>(FORCED_BASE_PAYOFF_THRESHOLD_X2),FORCED_BASE_PAYOFF_THRESHOLD_X2,FORCED_BASE_PAYOFF_WINDOW,FORCED_BASE_PAYOFF_WARMUP,FORCED_BASE_PAYOFF_COOLDOWN,FORCED_BASE_PAYOFF_HYSTERESIS_WINDOWS,FORCED_BASE_PAYOFF_GATE_BYPASS,FORCED_BASE_PURE_SUBTREE_FALLBACK);
            printf("FORCED_BASE_ADAPTIVE_STATS gate_decisions=%llu gate_allowed=%llu gate_skipped=%llu close_events=%llu low_windows=%llu max_low_streak=%llu probe_epochs=%llu payoff_samples=%llu payoff_sum=%llu payoff_avg=%.6f payoff_zero=%llu payoff_normal_samples=%llu payoff_normal_sum=%llu payoff_normal_avg=%.6f payoff_probe_samples=%llu payoff_probe_sum=%llu payoff_probe_avg=%.6f payoff_state128_samples=%llu payoff_bit64_samples=%llu payoff_state256_samples=%llu payoff_state256_sum=%llu payoff_state256_avg=%.6f payoff_state256_zero=%llu payoff_state256_lt2=%llu payoff_state256_h0=%llu payoff_state256_h1=%llu payoff_state256_h2_7=%llu payoff_state256_h8_63=%llu payoff_state256_h64p=%llu terminal_checks=%llu terminal_hits=%llu terminal_hit_pct=%.6f terminal_scan_candidates=%llu terminal_scan_sec=%.6f subtree_fallback_entries=%llu subtree_fallback_nodes=%llu subtree_fallback_materialized_forced=%llu subtree_fallback_context_updates_skipped=%llu\n",
                   forcedBaseAdaptiveTotal.gate_decisions,forcedBaseAdaptiveTotal.gate_allowed,forcedBaseAdaptiveTotal.gate_skipped,forcedBaseAdaptiveTotal.close_events,
                   forcedBaseAdaptiveTotal.low_windows,forcedBaseAdaptiveTotal.max_low_streak,forcedBaseAdaptiveTotal.probe_epochs,
                   forcedBaseAdaptiveTotal.payoff_samples,forcedBaseAdaptiveTotal.payoff_sum,static_cast<double>(forcedBaseAdaptiveTotal.payoff_sum)/ps,
                   forcedBaseAdaptiveTotal.payoff_zero,forcedBaseAdaptiveTotal.payoff_normal_samples,forcedBaseAdaptiveTotal.payoff_normal_sum,static_cast<double>(forcedBaseAdaptiveTotal.payoff_normal_sum)/pns,
                   forcedBaseAdaptiveTotal.payoff_probe_samples,forcedBaseAdaptiveTotal.payoff_probe_sum,static_cast<double>(forcedBaseAdaptiveTotal.payoff_probe_sum)/pps,
                   forcedBaseAdaptiveTotal.payoff_state128_samples,forcedBaseAdaptiveTotal.payoff_bit64_samples,
                   forcedBaseAdaptiveTotal.payoff_state256_samples,forcedBaseAdaptiveTotal.payoff_state256_sum,static_cast<double>(forcedBaseAdaptiveTotal.payoff_state256_sum)/p256s,
                   forcedBaseAdaptiveTotal.payoff_state256_zero,forcedBaseAdaptiveTotal.payoff_state256_lt2,
                   forcedBaseAdaptiveTotal.payoff_state256_h0,forcedBaseAdaptiveTotal.payoff_state256_h1,forcedBaseAdaptiveTotal.payoff_state256_h2_7,
                   forcedBaseAdaptiveTotal.payoff_state256_h8_63,forcedBaseAdaptiveTotal.payoff_state256_h64p,
                   forcedBaseAdaptiveTotal.terminal_checks,forcedBaseAdaptiveTotal.terminal_hits,
                   forcedBaseAdaptiveTotal.terminal_checks?100.0*static_cast<double>(forcedBaseAdaptiveTotal.terminal_hits)/static_cast<double>(forcedBaseAdaptiveTotal.terminal_checks):0.0,
                   forcedBaseAdaptiveTotal.terminal_scan_candidates,forcedBaseAdaptiveTotal.terminal_scan_ns/1e9,
                   forcedBaseAdaptiveTotal.subtree_fallback_entries,forcedBaseAdaptiveTotal.subtree_fallback_nodes,
                   forcedBaseAdaptiveTotal.subtree_fallback_materialized_forced,forcedBaseAdaptiveTotal.subtree_fallback_context_updates_skipped);
            printf("FORCED_BASE_TERMINAL_SIMD_CONFIG enabled=%d verify=%d verify_shift=%d width=8 op=gather_degree_compare scalar_equivalent_scan_count=1\n",
                   FORCED_BASE_TERMINAL_SIMD,FORCED_BASE_TERMINAL_SIMD_VERIFY,FORCED_BASE_TERMINAL_SIMD_VERIFY_SHIFT);
            printf("FORCED_BASE_TERMINAL_SIMD_STATS simd_calls=%llu vector_batches=%llu full_pass_batches=%llu early_fail_batches=%llu scalar_tail_candidates=%llu verify_samples=%llu verify_mismatches=%llu\n",
                   forcedBaseAdaptiveTotal.terminal_simd_calls,forcedBaseAdaptiveTotal.terminal_simd_vector_batches,forcedBaseAdaptiveTotal.terminal_simd_full_pass_batches,
                   forcedBaseAdaptiveTotal.terminal_simd_early_fail_batches,forcedBaseAdaptiveTotal.terminal_simd_scalar_tail_candidates,
                   forcedBaseAdaptiveTotal.terminal_simd_verify_samples,forcedBaseAdaptiveTotal.terminal_simd_verify_mismatches);
        }
        printf("DIRECT_ENTRY_CONFIG enabled=%d force_base_fold_unchanged=1 state256_original_gate=0 empty_forced_word256_runtime=1 empty_forced_word128=1 empty_forced_state64=1 scheduler_unchanged=1\n", DIRECT_EMPTY_FORCED_ENTRY);
        printf("DIRECT_ENTRY_STATS empty_forced_word128_handoffs=%llu empty_forced_bit64_handoffs=%llu\n", directEmpty128Total,directEmpty64Total);
        const char* width128ModeName = enableState128Simd?"hybrid128_fullpair":"word128_scalar";
        printf("UNIFIED_SELECTOR_CONFIG name=%s source=shared split_samples_excluded=1 split_epoch=all_successful_exports old_G5_migrated=0 parity=%d\n",experiment_control::selectorName(),FRESH64_PARITY);
        printf("WORD64_COMMON_CONFIG enabled=%d framework=PARALLEL_ENGINE\n",WORD64_COMMON_KERNEL);
        printf("STATEBIT_CORE_CONFIG bridge=%d width_contract=1 framework=PARALLEL_ENGINE\n",STATEBIT_COMMON_CONTROL);
        printf("WIDE_STATE_CONFIG host=PARALLEL_ENGINE dfs128_shared=1 dfs256_shared=1 context256_shared=1 map256to128_shared=1 fold_adapters_unchanged=1\n");
        statebit_wide_admit::report("PARALLEL_ENGINE");
        statebit_unified::report("PARALLEL_ENGINE");
        statebit_width_selection::report("PARALLEL_ENGINE");
        statebit_state32_handoff::report("PARALLEL_ENGINE");
        printf("FRESH64_STATS word_tasks=%llu state64_tasks=%llu observations=%llu accepted=%llu word_branch_nodes=%llu word_children=%llu spilled_states=%llu pivot_mismatch=%llu refine_mismatch=%llu split_skips=%llu dtfb_events=%llu\n",
               state64Total.fresh64_word_tasks,state64Total.fresh64_state64_tasks,state64Total.fresh64_observations,state64Total.fresh64_accepted,
               state64Total.fresh64_word_branch_nodes,state64Total.fresh64_word_children,state64Total.fresh64_word_spilled_states,
               state64Total.fresh64_pivot_mismatch,state64Total.fresh64_refine_mismatch,state64Total.fresh64_split_skips,state64Total.fresh64_dtfb_events);
        printf("SEARCH_LADDER_CONFIG word64_search_layer=%d bit64_context_only=%d list_range=gt256 word256_range=129_256 width128_range=65_128 width128_mode=%s width128_mt_export=%d state64_range=33_64 state32_range=1_32 state64_exec_mode=%d scalar_33_64_recursive_nodes=%llu\n",
               FRESH64_MODE?1:0,FRESH64_MODE?0:1,width128ModeName,DISABLE_128_MT?0:1,STATE64_EXEC_MODE,state64Total.fresh64_word_branch_nodes);
        printf("BIT64_DISPATCH_STATS dispatches=%llu state64_direct_dispatches=%llu state32_direct_dispatches=%llu state32_root_prechecks=%llu terminal_dispatches=%llu scalar_33_64_recursive_nodes=%llu\n",
               bitSearchTotal.bit64_dispatches,bitSearchTotal.state64_direct_dispatches,bitSearchTotal.state32_direct_dispatches,
               bitSearchTotal.state32_root_prechecks,bitSearchTotal.bit64_terminal_dispatches,state64Total.fresh64_word_branch_nodes);

        {
            const double h=forcedBaseBit64Total.handoffs?static_cast<double>(forcedBaseBit64Total.handoffs):1.0;
            printf("FORCED_BASE_BIT64_CONFIG enabled=1 threshold=%d fold_mode=%d direct_from_listBranch=1 forced_outside_bits=1 word64_search_layer=1 bit64_context_only=1 state64_33_64=1 state32_avx_when_live_le32=1 k_ge_2_only=1 mt_export=0\n",
                   FORCED_BASE_BIT64_THRESHOLD,FORCED_BASE_FOLD_MODE);
            printf("FORCED_BASE_BIT64_STATS eligible_nodes=%llu eligible_le32=%llu eligible_33_48=%llu eligible_49_64=%llu handoffs=%llu entry_le16=%llu entry_17_32=%llu entry_33_48=%llu entry_49_64=%llu p_sum=%llu c_sum=%llu forced_sum=%llu active_sum=%llu avg_active=%.6f avg_forced=%.6f ext_rows=%llu left_rows=%llu forced_ext_checks=%llu forced_left_edges=%llu bit_nodes=%llu state64_nodes=%llu state32_transitions=%llu whole_calls=%llu leaf_calls=%llu outputs=%llu avg_bit_nodes_per_handoff=%.6f\n",
                   forcedBaseBit64Total.eligible_nodes,forcedBaseBit64Total.eligible_le32,forcedBaseBit64Total.eligible_33_48,forcedBaseBit64Total.eligible_49_64,
                   forcedBaseBit64Total.handoffs,forcedBaseBit64Total.entry_le16,forcedBaseBit64Total.entry_17_32,forcedBaseBit64Total.entry_33_48,forcedBaseBit64Total.entry_49_64,
                   forcedBaseBit64Total.p_sum,forcedBaseBit64Total.c_sum,forcedBaseBit64Total.forced_sum,forcedBaseBit64Total.active_sum,
                   static_cast<double>(forcedBaseBit64Total.active_sum)/h,static_cast<double>(forcedBaseBit64Total.forced_sum)/h,
                   forcedBaseBit64Total.ext_rows,forcedBaseBit64Total.left_rows,forcedBaseBit64Total.forced_ext_checks,forcedBaseBit64Total.forced_left_edges,
                   forcedBaseBit64Total.bit_nodes,forcedBaseBit64Total.state64_nodes,forcedBaseBit64Total.state32_transitions,
                   forcedBaseBit64Total.whole_calls,forcedBaseBit64Total.leaf_calls,forcedBaseBit64Total.outputs,
                   static_cast<double>(forcedBaseBit64Total.bit_nodes)/h);
        }
        {
            const double packets=state64Total.packets?static_cast<double>(state64Total.packets):1.0;
            printf("UNION_PIVOT_PIVOT_CONFIG host=PARALLEL_ENGINE union_pivot=%d\n", PARALLEL_ENGINE_UNION_PIVOT_UNION_SPARSE);
            printf("STATE_WITNESS_WITNESS_CONFIG host=PARALLEL_ENGINE enabled=%d\n",PARALLEL_ENGINE_STATE_WITNESS_STATE64_WITNESS);
            printf("DEFECT_PLANE_DEFECT_CONFIG host=PARALLEL_ENGINE two_d=%d degree_policy=%s union_pivot_union=%d state_witness_witness=%d\n",PARALLEL_ENGINE_DEFECT_PLANE_DEFECT_2D,BULK_REFINE_BULK_ONEPASS?"BULK_ONE_PASS":"ONLINE",PARALLEL_ENGINE_UNION_PIVOT_UNION_SPARSE,PARALLEL_ENGINE_STATE_WITNESS_STATE64_WITNESS);
            printf("BULK_REFINE_POLICY_CONFIG host=PARALLEL_ENGINE bulk_onepass=%d union_pivot_union=%d state_witness_witness=%d defect_plane_2d=%d\n",BULK_REFINE_BULK_ONEPASS,PARALLEL_ENGINE_UNION_PIVOT_UNION_SPARSE,PARALLEL_ENGINE_STATE_WITNESS_STATE64_WITNESS,PARALLEL_ENGINE_DEFECT_PLANE_DEFECT_2D);
            printf("BATCH_MAXIMALITY_MAX_CONFIG host=PARALLEL_ENGINE batch=%d left_mode=%s union_pivot=1 witness=1 defect_planes=1 bulk_refine=1\n",BATCH_MAXIMALITY_STATE64_BATCH_MAX,LEFT_CONSTRAINT_STATE64_LEFT_2D?"2D":"SCALAR");
            printf("EAGER_FOLDING_EAGER_CONFIG host=PARALLEL_ENGINE enabled=%d max_live=%d outer_forced_guard=1 localf_handoff=%d\n",(PARALLEL_ENGINE_EAGER_FOLDING_EAGER_COMPLEMENT_FOLD && enableLocalFolding)?1:0,folding_variant::maximum(entryFoldingPolicy),PARALLEL_ENGINE_EAGER_FOLDING_LOCALF_HANDOFF);
        printf("LOCAL_FIXED_HANDOFF_STATS host=PARALLEL_ENGINE localf_max_checks=%llu localf_handoffs=%llu\n",state64Total.eager_folding_localf_max_checks,state64Total.eager_folding_localf_state32);
        printf("LOCAL_FIXED_HANDOFF_CONFIG host=PARALLEL_ENGINE policy=%s local_f=1 outer_folding=0\n",LOCAL_FIXED_LOCALF_POLICY==2?"OVERLAY_OPT":(LOCAL_FIXED_LOCALF_POLICY==1?"OVERLAY":"MATERIALIZE"));
        printf("LOCAL_FIXED_OPTIMIZED_OPT_STATS host=PARALLEL_ENGINE batches=%llu slots=%llu full_batches=%llu penalty_lookups=%llu penalty_misses=%llu local_x_rows=%llu basex_rows=%llu external_rows=%llu left_rows=%llu emitted=%llu\n",
            state64Total.localFixedOptimized_batches,state64Total.localFixedOptimized_slots,state64Total.localFixedOptimized_full_batches,
            state64Total.localFixedOptimized_penalty_lookups,state64Total.localFixedOptimized_penalty_misses,
            state64Total.localFixedOptimized_x_rows,state64Total.localFixedOptimized_basex_rows,state64Total.localFixedOptimized_ext_rows,
            state64Total.localFixedOptimized_left_rows,state64Total.localFixedOptimized_emitted);
            printf("FOLDING_POLICY_CONFIG host=PARALLEL_ENGINE policy=F%d implementation=PARALLEL_ENGINE_RECURSIVE_FOLDING_GATE forced_base_fold_mode=%d\n",FOLDING_FOLD_POLICY,FORCED_BASE_FOLD_MODE);
            printf("EAGER_FOLDING_STATE64_STATS eligible=%llu scans=%llu tested=%llu fold_nodes=%llu folded_vertices=%llu localf_state32=%llu localf_max_checks=%llu\n",state64Total.eager_folding_eligible,state64Total.eager_folding_scans,state64Total.eager_folding_tested,state64Total.eager_folding_fold_nodes,state64Total.eager_folding_folded_vertices,state64Total.eager_folding_localf_state32,state64Total.eager_folding_localf_max_checks);
            printf("LEFT_CONSTRAINT_STATE64_STATS invocations=%llu chunks=%llu vector_steps=%llu fallback_rows=%llu left_reject=%llu\n",state64Total.left_constraint_left_invocations,state64Total.left_constraint_left_chunks,state64Total.left_constraint_vector_steps,state64Total.left_constraint_fallback_rows,state64Total.left_constraint_left_rejects);
            printf("BATCH_MAXIMALITY_STATE64_STATS batches=%llu slots=%llu full=%llu x_rows=%llu ext_rows=%llu left_scalar_rows=%llu emitted=%llu\n",state64Total.batch_maximality_batches,state64Total.batch_maximality_slots,state64Total.batch_maximality_full,state64Total.batch_maximality_x_rows,state64Total.batch_maximality_ext_rows,state64Total.batch_maximality_left_scalar_rows,state64Total.batch_maximality_emitted);
            printf("BULK_REFINE_STATE64_STATS static_removed=%llu degree_steps=%llu degree_removed=%llu\n",state64Total.bulk_refine_static_removed,state64Total.bulk_refine_degree_steps,state64Total.bulk_refine_degree_removed);
            printf("DEFECT_PLANE_STATE64_STATS refine_packets=%llu two_d_packets=%llu defect_steps=%llu\n",
                   state64Total.defect_plane_refine_packets,state64Total.defect_plane_2d_packets,state64Total.defect_plane_defect_steps);
            printf("STATE_WITNESS_STATE64_WITNESS_STATS tests=%llu hits=%llu full_scans=%llu full_scan_vertices=%llu\n",
                   state64Total.whole_witness_tests,state64Total.whole_witness_hits,
                   state64Total.whole_full_scans,state64Total.whole_scan_vertices);
            printf("STATE64_CONFIG enabled=1 mode=%d mode_name=%s lanes=4 lane_bits=64 same_context=1 vector_pivot=1 vector_refine=1 branch_mask_simd=1 state32_handoff=1 forced_base_bit64_threshold=%d\n",
                   STATE64_EXEC_MODE,(STATE64_EXEC_MODE==1?"packet":"refill"),FORCED_BASE_BIT64_THRESHOLD);
            printf("STATE64_STATS packets=%llu slots=%llu full_packets=%llu occ1=%llu occ2=%llu occ3=%llu occ4=%llu avg_occupancy=%.6f full_packet_pct=%.6f prepared_states=%llu branch_states=%llu terminal_states=%llu state32_handoffs=%llu generated_children=%llu\n",
                   state64Total.packets,state64Total.slots,state64Total.full_packets,
                   state64Total.occ_hist[1],state64Total.occ_hist[2],state64Total.occ_hist[3],state64Total.occ_hist[4],
                   static_cast<double>(state64Total.slots)/packets,
                   100.0*static_cast<double>(state64Total.full_packets)/packets,
                   state64Total.prepared_states,state64Total.branch_states,state64Total.terminal_states,
                   state64Total.state32_handoffs,state64Total.generated_children);
        }
        {
            const double h256=state256Total.handoffs?static_cast<double>(state256Total.handoffs):1.0;
            printf("FORCED_BASE_WORD256_CONFIG enabled=1 threshold=%d representation=4xuint64 simd=0 handoff_to_word128_at=128\n",FORCED_BASE_STATE256_THRESHOLD);
            printf("FORCED_BASE_WORD256_STATS eligible_nodes=%llu handoffs=%llu builds=%llu entry_129_160=%llu entry_161_192=%llu entry_193_224=%llu entry_225_256=%llu nodes=%llu transitions128=%llu whole_calls=%llu leaf_calls=%llu outputs=%llu cand_filter_calls=%llu cand_filter_removed=%llu ext_rows=%llu left_rows=%llu forced_ext_checks=%llu forced_left_edges=%llu avg_entry_active=%.6f avg_forced=%.6f avg_nodes_per_handoff=%.6f build_sec=%.6f snapshot_sec=%.6f inclusive_thread_cpu_sec=%.6f\n",
                   state256Total.eligible_nodes,state256Total.handoffs,state256Total.builds,state256Total.entry_129_160,state256Total.entry_161_192,state256Total.entry_193_224,state256Total.entry_225_256,state256Total.nodes,state256Total.transitions128,state256Total.whole_calls,state256Total.leaf_calls,state256Total.outputs,state256Total.cand_filter_calls,state256Total.cand_filter_removed,state256Total.ext_rows,state256Total.left_rows,state256Total.forced_ext_checks,state256Total.forced_left_edges,
                   static_cast<double>(state256Total.active_sum)/h256,static_cast<double>(state256Total.forced_sum)/h256,static_cast<double>(state256Total.nodes)/h256,state256Total.build_ns/1e9,state256Total.snapshot_ns/1e9,state256Total.cpu_ns/1e9);
        }
        {
            const double h=state128Total.handoffs?static_cast<double>(state128Total.handoffs):1.0;
            const double sp=state128Total.simd_packets?static_cast<double>(state128Total.simd_packets):1.0;
            const char* mode128 = enableState128Simd?"hybrid128":"word128";
            const char* repr128 = enableState128Simd?"word128_plus_avx2_2x128_fullpair":"2xuint64_scalar";
            printf("WIDTH128_CONFIG enabled=1 threshold=%d mode=%s representation=%s simd=%d avx2=%d lanes=%d lane_bits=128 direct_no_hybrid=%d hybrid_fullpair_only=%d scalar_single_fallback=%d mt_export=%d handoff_to_state64_at=64 state64_mode=%d\n",
                   FORCED_BASE_STATE128_THRESHOLD,mode128,repr128,enableState128Simd?1:0,enableState128Simd?1:0,enableState128Simd?2:1,
                   0,enableState128Simd?1:0,enableState128Simd?1:0,DISABLE_128_MT?0:1,STATE64_EXEC_MODE);
            printf("WIDTH128_CORE_STATS eligible_nodes=%llu handoffs=%llu builds=%llu entry_65_96=%llu entry_97_128=%llu nodes=%llu transitions64=%llu whole_calls=%llu leaf_calls=%llu outputs=%llu cand_filter_calls=%llu cand_filter_removed=%llu ext_rows=%llu left_rows=%llu forced_ext_checks=%llu forced_left_edges=%llu avg_entry_active=%.6f avg_forced=%.6f avg_nodes_per_handoff=%.6f inclusive_thread_cpu_sec=%.6f\n",
                   state128Total.eligible_nodes,state128Total.handoffs,state128Total.builds,
                   state128Total.entry_65_96,state128Total.entry_97_128,state128Total.nodes,state128Total.transitions64,
                   state128Total.whole_calls,state128Total.leaf_calls,state128Total.outputs,state128Total.cand_filter_calls,state128Total.cand_filter_removed,
                   state128Total.ext_rows,state128Total.left_rows,state128Total.forced_ext_checks,state128Total.forced_left_edges,
                   static_cast<double>(state128Total.active_sum)/h,static_cast<double>(state128Total.forced_sum)/h,static_cast<double>(state128Total.nodes)/h,state128Total.cpu_ns/1e9);
            printf("STATE128_SIMD_STATS enabled=%d packets=%llu slots=%llu occ1=%llu occ2=%llu avg_occupancy=%.6f full_packet_pct=%.6f generated_children=%llu mask_vector_ops=%llu pivot_packets=%llu refine_packets=%llu\n",
                   enableState128Simd?1:0,state128Total.simd_packets,state128Total.simd_slots,state128Total.simd_occ1,state128Total.simd_occ2,
                   static_cast<double>(state128Total.simd_slots)/sp,100.0*static_cast<double>(state128Total.simd_occ2)/sp,
                   state128Total.simd_generated_children,state128Total.simd_mask_vector_ops,state128Total.simd_pivot_packets,state128Total.simd_refine_packets);
            const unsigned long long hybridBranchStates=state128Total.hybrid_paired_ready_states+state128Total.hybrid_scalar_branch_nodes;
            const double hybridDen=hybridBranchStates?static_cast<double>(hybridBranchStates):1.0;
            printf("HYBRID128_STATS enabled=%d single_ready_events=%llu scalar_branch_nodes=%llu scalar_generated_children=%llu paired_ready_states=%llu simd_branch_ratio_pct=%.6f fullpair_contract=%d\n",
                   enableState128Simd?1:0,state128Total.hybrid_single_ready_events,state128Total.hybrid_scalar_branch_nodes,
                   state128Total.hybrid_scalar_generated_children,state128Total.hybrid_paired_ready_states,
                   100.0*static_cast<double>(state128Total.hybrid_paired_ready_states)/hybridDen,
                   (enableState128Simd && state128Total.simd_occ1==0)?1:0);
        }
        {
            const auto &a=aggregateForcedContextTotal;
            const double estUpdateSec=(a.left_u_update_sample_edges>0)
                ? (static_cast<double>(a.left_u_update_sample_ns)/1e9) *
                  (static_cast<double>(a.left_u_update_edges)/static_cast<double>(a.left_u_update_sample_edges))
                : 0.0;
            printf("AGG_FORCED_CONTEXT_CONFIG mode=%d mode_name=%s validation=%d maintain_left=%d use_ext_formula=%d use_left_formula=%d left_context_policy=%s hybrid_left_degree_threshold=%d auto_left_context=%d auto_warmup_events=%d auto_avg_degree_threshold=%d seed_auto_left_context=%d seed_auto_avg_degree_threshold=%d left_update_timing_sample_stride=4096\n",
                   AGG_FORCED_CONTEXT_MODE,aggregateForcedContextModeName(AGG_FORCED_CONTEXT_MODE),
                   aggValidationMode()?1:0,aggMaintainLeft()?1:0,aggUseExternalFormula()?1:0,aggUseLeftFormula()?1:0,
                   forcedBaseLeftContextPolicyName(),FORCED_BASE_HYBRID_LEFT_DEGREE_THRESHOLD,
                   FORCED_BASE_AUTO_LEFT_CONTEXT,FORCED_BASE_AUTO_LEFT_WARMUP_EVENTS,FORCED_BASE_AUTO_LEFT_AVG_DEGREE_THRESHOLD,
                   FORCED_BASE_SEED_AUTO_LEFT_CONTEXT,FORCED_BASE_SEED_AUTO_LEFT_AVG_DEGREE_THRESHOLD);
            const double autoAvgDegree=a.auto_warmup_events?static_cast<double>(a.auto_degree_sum)/static_cast<double>(a.auto_warmup_events):0.0;
            const double seedAutoAvgDegree=a.seed_auto_hop_vertices?static_cast<double>(a.seed_auto_degree_sum)/static_cast<double>(a.seed_auto_hop_vertices):0.0;
            printf("AGG_FORCED_CONTEXT_STATS build_calls=%llu ext_formula_rows=%llu left_formula_rows=%llu ext_formula_mismatch=%llu left_formula_mismatch=%llu invalid_formula=%llu internal_sec=%.6f ext_active_sec=%.6f ext_forced_sec=%.6f left_clear_sec=%.6f left_forced_sec=%.6f left_active_sec=%.6f validation_verify_sec=%.6f left_u_init_sec=%.6f left_u_init_edges=%llu left_u_update_calls=%llu left_u_update_edges=%llu left_u_add_edges=%llu left_u_sub_edges=%llu left_u_update_sample_calls=%llu left_u_update_sample_edges=%llu left_u_update_sec_sampled=%.6f left_u_update_sec_est=%.6f lazy_delta_events=%llu lazy_flush_calls=%llu lazy_flush_vertices=%llu lazy_flush_edges=%llu lazy_flush_sec=%.6f hybrid_eager_events=%llu hybrid_lazy_events=%llu hybrid_eager_edges=%llu auto_observed_warmup_events=%llu auto_degree_sum=%llu auto_avg_degree=%.6f auto_selected_eager_workers=%llu auto_selected_hybrid_workers=%llu auto_undecided_workers=%llu seed_auto_seeds=%llu seed_auto_degree_sum=%llu seed_auto_hop_vertices=%llu seed_auto_avg_degree=%.6f seed_auto_selected_eager_seeds=%llu seed_auto_selected_hybrid_seeds=%llu\n",
                   (unsigned long long)a.build_calls,(unsigned long long)a.ext_formula_rows,(unsigned long long)a.left_formula_rows,
                   (unsigned long long)a.ext_formula_mismatch,(unsigned long long)a.left_formula_mismatch,(unsigned long long)a.invalid_formula,
                   a.internal_ns/1e9,a.ext_active_ns/1e9,a.ext_forced_ns/1e9,a.left_clear_ns/1e9,a.left_forced_ns/1e9,a.left_active_ns/1e9,a.validation_verify_ns/1e9,
                   a.left_u_init_ns/1e9,(unsigned long long)a.left_u_init_edges,
                   (unsigned long long)a.left_u_update_calls,(unsigned long long)a.left_u_update_edges,
                   (unsigned long long)a.left_u_add_edges,(unsigned long long)a.left_u_sub_edges,
                   (unsigned long long)a.left_u_update_sample_calls,(unsigned long long)a.left_u_update_sample_edges,
                   a.left_u_update_sample_ns/1e9,estUpdateSec,
                   (unsigned long long)a.lazy_delta_events,(unsigned long long)a.lazy_flush_calls,
                   (unsigned long long)a.lazy_flush_vertices,(unsigned long long)a.lazy_flush_edges,a.lazy_flush_ns/1e9,
                   (unsigned long long)a.hybrid_eager_events,(unsigned long long)a.hybrid_lazy_events,(unsigned long long)a.hybrid_eager_edges,
                   (unsigned long long)a.auto_warmup_events,(unsigned long long)a.auto_degree_sum,autoAvgDegree,
                   (unsigned long long)a.auto_selected_eager_workers,(unsigned long long)a.auto_selected_hybrid_workers,(unsigned long long)a.auto_undecided_workers,
                   (unsigned long long)a.seed_auto_seeds,(unsigned long long)a.seed_auto_degree_sum,(unsigned long long)a.seed_auto_hop_vertices,seedAutoAvgDegree,
                   (unsigned long long)a.seed_auto_selected_eager_seeds,(unsigned long long)a.seed_auto_selected_hybrid_seeds);
            if(a.first_mismatch_set){
                printf("AGG_FIRST_MISMATCH seed=%d handoff=%llu kind=%s row=%d vertex=%d old=%d new=%d\n",
                       a.first_seed,(unsigned long long)a.first_handoff,a.first_kind==1?"external":"left",
                       a.first_row,a.first_vertex,a.first_old,a.first_new);
            }else{
                printf("AGG_FIRST_MISMATCH none=1\n");
            }
        }
        printf("BEST_DIAG_CONFIG target_seeds=%d total_peeled_seeds=%d partial_run=%d\n",targetSeeds,pn,targetSeeds<pn?1:0);
        printf("BEST_DIAG_PHASE seed_count=%llu valid_seed_count=%llu decompose_sec=%.6f workspace_sec=%.6f subgraph_sec=%.6f search_sec=%.6f seed_max_sec=%.6f fold_calls=%llu fold_sec=%.6f state128_calls=%llu state128_total_sec=%.6f state128_build_calls=%llu state128_build_sec=%.6f bit64_build_calls=%llu bit64_build_sec=%.6f\n",
               (unsigned long long)bestDiagTotal.seed_count,(unsigned long long)bestDiagTotal.valid_seed_count,
               bestDiagTotal.decompose_ns/1e9,bestDiagTotal.workspace_ns/1e9,bestDiagTotal.subgraph_ns/1e9,bestDiagTotal.search_ns/1e9,bestDiagTotal.seed_ns_max/1e9,
               (unsigned long long)bestDiagTotal.fold_calls,bestDiagTotal.fold_ns/1e9,(unsigned long long)bestDiagTotal.state128_calls,bestDiagTotal.state128_ns/1e9,
               (unsigned long long)bestDiagTotal.state128_build_calls,bestDiagTotal.state128_build_ns/1e9,(unsigned long long)bestDiagTotal.bit64_build_calls,bestDiagTotal.bit64_build_ns/1e9);
        printf("SEED_PARALLEL_STATS peeled_vertices=%d processed_seeds=%llu valid_blocks=%llu worker_wall_min=%.6f worker_wall_max=%.6f worker_wall_avg=%.6f\n",
               pn,processedSeeds,validblk,minWorker,maxWorker,actualThreads?sumWorker/actualThreads:0.0);
        printf("MT_STATEBIT_TASK_STATS exported=%llu executed=%llu stolen=%llu local_pops=%llu rejected_queue_full=%llu peak_queued=%llu helper_tasks=%llu helper_stolen=%llu\n",
               taskPool.exported.load(),taskPool.executed.load(),taskPool.stolen.load(),taskPool.local_pops.load(),
               taskPool.rejected_full.load(),taskPool.peak_queued.load(),helpTasks,stolenTasks);
        printf("LARGE_SEED_WORKSPACE_STATS initial_cap=%d large_seed_count=%llu max_hop=%d max_blk=%d max_left=%d max_matrix_mib=%.3f retained_matrix_sum_mib=%.3f\n",
               CAP_LIMIT2,largeSeeds,globalMaxHop,globalMaxBlk,globalMaxLeft,
               static_cast<double>(globalMaxMatrixBytes)/(1024.0*1024.0),
               static_cast<double>(sumWorkerMatrixBytes)/(1024.0*1024.0));
        printf("SEED_WORKER_SEEDS %s\n",seedDist.str().c_str());
        printf("SEED_WORKER_OUTPUTS %s\n",outputDist.str().c_str());
        printf("SEED_WORKER_WALL_SEC %s\n",wallDist.str().c_str());
        printf("MT_STATEBIT_WORKER_HELP_TASKS %s\n",helpDist.str().c_str());
        printf("MT_STATEBIT_WORKER_STOLEN_TASKS %s\n",stealDist.str().c_str());
        printf("AVX2_CONFIG width=%d stage_refill=%d union_sparse_pivot=%d union_sparse_refine=%d persistent_soa=%d local_max_suffix=%d telemetry=%d\n",
               AVX_PACKET_WIDTH,AVX_STAGE_REFILL,AVX_UNION_SPARSE_PIVOT,AVX_UNION_SPARSE_REFINE,AVX_PERSISTENT_SOA,AVX_LOCAL_MAX_SUFFIX,VECTOR_TELEMETRY);
        printf("STATS_NOTE detailed_ENGINE_OPTIMIZED_counters=thread_local_not_reduced_in_multithread_statebit_release\n");
