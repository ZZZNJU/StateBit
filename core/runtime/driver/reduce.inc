        unsigned long long cntMaxPlex=0, validblk=0, processedSeeds=0,helpTasks=0,stolenTasks=0,largeSeeds=0;
        ForcedComplementFoldRealStats forcedComplementFoldRealTotal;
        ForcedBaseStats forcedBaseTotal;
        ForcedBaseAdaptiveStats forcedBaseAdaptiveTotal;
        ForcedBaseBit64Stats forcedBaseBit64Total;
        State64Stats state64Total;
        unsigned long long directEmpty128Total=0,directEmpty64Total=0;
        State128Stats state128Total;
        State256Stats state256Total;
        BestDiagStats bestDiagTotal;
        PreUpdateKPlexStats preUpdateKPlexTotal;
        DenseTouchStats denseTouchTotal;
        AggregateForcedContextStats aggregateForcedContextTotal;
        SteadyKernelProfileStats steadyKernelProfileTotal;
        WitnessResumeStats witnessResumeTotal;
        BitSearchStats bitSearchTotal;
        SeedModeStats seedModeTotal;
        int globalMaxHop=0,globalMaxBlk=0,globalMaxLeft=0;
        size_t globalMaxMatrixBytes=0,sumWorkerMatrixBytes=0;
        double minWorker=1e300,maxWorker=0.0,sumWorker=0.0;
        std::ostringstream seedDist, outputDist, wallDist, helpDist, stealDist;
        for(int t=0;t<actualThreads;t++){
            cntMaxPlex += summaries[t].outputs;
            validblk += summaries[t].valid_blocks;
            processedSeeds += summaries[t].seeds;
            helpTasks += summaries[t].help_tasks;
            stolenTasks += summaries[t].stolen_tasks;
            largeSeeds += summaries[t].large_seed_count;
            globalMaxHop=std::max(globalMaxHop,summaries[t].max_hop);
            globalMaxBlk=std::max(globalMaxBlk,summaries[t].max_blk);
            globalMaxLeft=std::max(globalMaxLeft,summaries[t].max_left);
            globalMaxMatrixBytes=std::max(globalMaxMatrixBytes,summaries[t].max_matrix_bytes);
            sumWorkerMatrixBytes+=summaries[t].max_matrix_bytes;
            forcedComplementFoldRealTotal.merge(summaries[t].complement_fold_real);
            forcedBaseTotal.merge(summaries[t].forced_base);
            forcedBaseAdaptiveTotal.merge(summaries[t].forced_base_adaptive);
            forcedBaseBit64Total.merge(summaries[t].forced_base_bit64);
            state64Total.merge(summaries[t].state64);
            state128Total.merge(summaries[t].state128);
            directEmpty128Total+=summaries[t].direct_empty_128;
            directEmpty64Total+=summaries[t].direct_empty_64;
            state256Total.merge(summaries[t].state256);
            bestDiagTotal.merge(summaries[t].best_diag);
            preUpdateKPlexTotal.merge(summaries[t].preupdate_kplex);
            denseTouchTotal.merge(summaries[t].dense_touch);
            aggregateForcedContextTotal.merge(summaries[t].aggregate_forced_context);
            steadyKernelProfileTotal.merge(summaries[t].steady_kernel_telemetry);
            witnessResumeTotal.merge(summaries[t].witness_resume);
            bitSearchTotal.merge(summaries[t].bit_search);
            seedModeTotal.merge(summaries[t].seed_mode);
            minWorker=std::min(minWorker,summaries[t].wall_sec);
            maxWorker=std::max(maxWorker,summaries[t].wall_sec);
            sumWorker+=summaries[t].wall_sec;
            if(t){ seedDist<<","; outputDist<<","; wallDist<<","; helpDist<<","; stealDist<<","; }
            seedDist<<summaries[t].seeds;
            outputDist<<summaries[t].outputs;
            wallDist.setf(std::ios::fixed); wallDist.precision(3); wallDist<<summaries[t].wall_sec;
            helpDist<<summaries[t].help_tasks;
            stealDist<<summaries[t].stolen_tasks;
        }
        if(actualThreads==0) minWorker=0.0;

        FOLDING_GATEEarlyOffAggregate foldingGateEarlyOffTotal;
        for(int t=0;t<actualThreads;++t) foldingGateEarlyOffTotal.merge(summaries[t].folding_gate_earlyoff);
        const double foldingGateN=foldingGateEarlyOffTotal.seeds?static_cast<double>(foldingGateEarlyOffTotal.seeds):1.0;
        const unsigned long long earlyOffEffective=foldingGateEarlyOffTotal.off+foldingGateEarlyOffTotal.off_requested;
        printf("FOLDING_GATE_EARLYOFF_GATE_STATS enabled=1 checkpoint=%d rule=default_on_off_if_first_N_PROBE_all_hit_and_no_DYNAMIC_STATE32 seeds=%llu early_off=%llu committed_off=%llu keep_on=%llu probing=%llu off_requested=%llu early_off_pct=%.9f observed_checks=%llu observed_hits=%llu observed_structural_misses=%llu observed_size_misses=%llu dynamic_state32_handoffs_before_decision=%llu forced_context_enabled=%d\n",
               FOLDING_GATE_EARLYOFF_CHECKPOINT,foldingGateEarlyOffTotal.seeds,earlyOffEffective,foldingGateEarlyOffTotal.off,foldingGateEarlyOffTotal.keep_on,foldingGateEarlyOffTotal.probing,foldingGateEarlyOffTotal.off_requested,
               100.0*static_cast<double>(earlyOffEffective)/foldingGateN,foldingGateEarlyOffTotal.checks,foldingGateEarlyOffTotal.hits,foldingGateEarlyOffTotal.structural_misses,
               foldingGateEarlyOffTotal.size_misses,foldingGateEarlyOffTotal.dynamic_state32_handoffs_before_decision,MT_TAIL_EXPANSION);


        const double durat=watch.stop();
