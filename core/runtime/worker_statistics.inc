    void resetThreadStats(){
        bitStats = BitSearchStats{};
        wordKernelStats = WordKernelStats{};
        laneStats = LaneDfsStats{};
        lanePivotStats = LanePivotStats{};
        avxStats = AvxPacketStats{};
        maxSimdStats = MaxSimdStats{};
        wholeWitnessStats = WholeWitnessStats{};
        witnessResumeStats = WitnessResumeStats{};
        refine2DStats = Refine2DStats{};
        pivotReuseStats = PivotReuseStats{};
        degreePeelStats = DegreePeelStats{};
        workBatchStats = WorkBatchStats{};
        steadyKernelProfileStats = SteadyKernelProfileStats{};
        forcedComplementFoldRealStats = ForcedComplementFoldRealStats{};
        forcedBaseStats = ForcedBaseStats{};
        forcedBasePayoffGate.reset();
        forcedBaseBit64Stats = ForcedBaseBit64Stats{};
            directEmpty128Handoffs=0; directEmpty64Handoffs=0;
        state64Stats = State64Stats{};
        state128Stats = State128Stats{};
        state256Stats = State256Stats{};
        bestDiagStats = BestDiagStats{};
        preUpdateKPlexStats = PreUpdateKPlexStats{};
        denseTouchStats = DenseTouchStats{};
        aggregateForcedContextStats = AggregateForcedContextStats{};
        autoLeftContextSelector.reset();
    }


    struct SeedWorkerSummary {
        unsigned long long seeds=0;
        unsigned long long valid_blocks=0;
        unsigned long long outputs=0;
        unsigned long long help_tasks=0;
        unsigned long long stolen_tasks=0;
        unsigned long long large_seed_count=0;
        int max_hop=0, max_blk=0, max_left=0;
        size_t max_matrix_bytes=0;
        double wall_sec=0.0;
        ForcedComplementFoldRealStats complement_fold_real;
        ForcedBaseStats forced_base;
        ForcedBaseAdaptiveStats forced_base_adaptive;
        ForcedBaseBit64Stats forced_base_bit64;
        unsigned long long direct_empty_128=0,direct_empty_64=0;
        State64Stats state64;
        State128Stats state128;
        State256Stats state256;
        BestDiagStats best_diag;
        PreUpdateKPlexStats preupdate_kplex;
        DenseTouchStats dense_touch;
        AggregateForcedContextStats aggregate_forced_context;
        SteadyKernelProfileStats steady_kernel_telemetry;
        WitnessResumeStats witness_resume;
        BitSearchStats bit_search;
        SeedModeStats seed_mode;
        FOLDING_GATEEarlyOffAggregate folding_gate_earlyoff;
    };

