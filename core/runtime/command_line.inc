void usage() {
    fprintf(stderr, "usage: ./PlexEnum <dataset> -k <integer>=2 -q <q> [-t <threads>] [-p <progress_sec>] [-s <max_seeds>] [--selector hybrid4|adaptive64|word64|state64] [--local-folding 0|1] [--local-f-remat 0|1] [--remat-stack-reuse 0|1] [--remat-min-slack 0..32]\n");
}
int check_inc(int i, int max) {
    if (i + 1 >= max) {
        usage();
        exit(2);
    }
    return i + 1;
}
int parseCoreInteger(const char* text){
    char* end=nullptr;errno=0;long value=std::strtol(text,&end,10);
    if(errno||!text[0]||!end||*end||value<INT_MIN||value>INT_MAX){std::fprintf(stderr,"invalid integer: %s\n",text);std::exit(2);}
    return static_cast<int>(value);
}
int statebit_application(int argc, char **argv,bool parallel)
{
    parallelExecution=parallel;
    workerThreads=parallel?16:1;
    int i = 2;
    if(argc<6){
        usage();
        exit(2);
    } 
    char* endptr;
    printf("file: %s\n",argv[1]); 
    while(i<argc){
       if(!strcmp(argv[i],"-k")){
            i = check_inc(i, argc);
            k = parseCoreInteger(argv[i]);
        }
       else if(!strcmp(argv[i],"-q")){
            i = check_inc(i, argc);
            lb = parseCoreInteger(argv[i]);
        }
       else if(!strcmp(argv[i],"-t")){
            i = check_inc(i, argc);
            workerThreads = parseCoreInteger(argv[i]);
        }
       else if(!strcmp(argv[i],"-p")){
            i = check_inc(i, argc);
            progressIntervalSeconds = parseCoreInteger(argv[i]);
        }
       else if(!strcmp(argv[i],"-s")){
            i = check_inc(i, argc);
            seedTaskLimit = parseCoreInteger(argv[i]);
        }
       else if(!strcmp(argv[i],"--selector")){
            i=check_inc(i,argc);
            if(!strcmp(argv[i],"hybrid4"))kernelSelectionMode=0;
            else if(!strcmp(argv[i],"adaptive64"))kernelSelectionMode=1;
            else if(!strcmp(argv[i],"word64"))kernelSelectionMode=2;
            else if(!strcmp(argv[i],"state64"))kernelSelectionMode=3;
            else {std::fprintf(stderr,"selector must be hybrid4, adaptive64, word64 or state64\n");return 2;}
        }
       else if(!strcmp(argv[i],"--local-f-remat")){
            i=check_inc(i,argc);const int value=parseCoreInteger(argv[i]);
            if(value!=0&&value!=1){std::fprintf(stderr,"local-f-remat must be 0 or 1\n");return 2;}
            enableLocalRematerialization=value!=0;
        }
       else if(!strcmp(argv[i],"--remat-stack-reuse")){
            i=check_inc(i,argc);const int value=parseCoreInteger(argv[i]);
            if(value!=0&&value!=1){std::fprintf(stderr,"remat-stack-reuse must be 0 or 1\n");return 2;}
            reuseRematerializationStorage=value!=0;
        }
       else if(!strcmp(argv[i],"--remat-min-slack")){
            i=check_inc(i,argc);const int value=parseCoreInteger(argv[i]);
            if(value<0||value>32){std::fprintf(stderr,"remat-min-slack must be 0..32\n");return 2;}
            rematerializationMinSlack=value;
        }
       else if(!strcmp(argv[i],"--simd128")){
            i=check_inc(i,argc);const int value=parseCoreInteger(argv[i]);
            if(value!=0&&value!=1){std::fprintf(stderr,"simd128 must be 0 or 1\n");return 2;}
            enableState128Simd=value!=0;
        }
       else if(!strcmp(argv[i],"--queue-precheck") || !strcmp(argv[i],"--remap128")){
            const bool queue=!strcmp(argv[i],"--queue-precheck");
            i=check_inc(i,argc);const int value=parseCoreInteger(argv[i]);
            if(value!=0&&value!=1){std::fprintf(stderr,"queue-precheck/remap128 must be 0 or 1\n");return 2;}
            if(queue)enableQueuePrecheck=value!=0;else enableRemap128=value!=0;
        }
       else if(!strcmp(argv[i],"--direct256") || !strcmp(argv[i],"--wide-cost")){
            const bool direct=!strcmp(argv[i],"--direct256");
            i=check_inc(i,argc);const int value=parseCoreInteger(argv[i]);
            if(value!=0&&value!=1){std::fprintf(stderr,"direct256/wide-cost must be 0 or 1\n");return 2;}
            if(direct)enableDirectWideEntry=value!=0;else enableWideCostSampling=value!=0;
        }
       else if(!strcmp(argv[i],"--mixed-f")){
            i=check_inc(i,argc);const int value=parseCoreInteger(argv[i]);
            if(value!=0&&value!=1){std::fprintf(stderr,"mixed-f must be 0 or 1\n");return 2;}
            enableMixedFixedContext=value!=0;
        }
       else if(!strcmp(argv[i],"--c0-probe")){
            i=check_inc(i,argc);const int value=parseCoreInteger(argv[i]);
            if(value!=0&&value!=1){std::fprintf(stderr,"c0-probe must be 0 or 1\n");return 2;}
            enableFoldingProbe=value!=0;
        }
       else if(!strcmp(argv[i],"--fold-variant")){
            i=check_inc(i,argc);const int value=parseCoreInteger(argv[i]);
            if(value<0||value>3){std::fprintf(stderr,"fold-variant must be 0, 1, 2 or 3\n");return 2;}
            entryFoldingPolicy=value;
        }
       else if(!strcmp(argv[i],"--local-folding")){
            i=check_inc(i,argc);const int value=parseCoreInteger(argv[i]);
            if(value!=0&&value!=1){std::fprintf(stderr,"local-folding must be 0 or 1\n");return 2;}
            enableLocalFolding=value!=0;
        }
        else {
            usage();
            exit(2);
        }
        i++;
    }
    if(!parallel && workerThreads!=1){std::fprintf(stderr,"Sequential requires exactly one worker\n");return 2;}
    printf("UNIFIED_CONFIG core_id=%s mode=%s threads=%d shared_object=1 selector=%s local_folding=%d local_f_remat=%d remat_stack_reuse=%d remat_min_slack=%d split_samples_excluded=1 persistent_f=1 list_split=1 bit_split=1 kernel_sample_period=1024 c0_probe=%d c0_sample_period=1024 mixed_f=%d mixed_max_live=40 mixed_scan_post_ready=1 direct256=%d wide_cost=%d wide_cost_period=1024\n",UNIFIED_CORE_ID,parallel?"parallel":"sequential",workerThreads,experiment_control::selectorName(),enableLocalFolding?1:0,enableLocalRematerialization?1:0,reuseRematerializationStorage?1:0,rematerializationMinSlack,enableFoldingProbe?1:0,enableMixedFixedContext?1:0,enableDirectWideEntry?1:0,enableWideCostSampling?1:0);
    printf("FOLD_VARIANT_CONFIG variant=%d recursive_max=%d entry_pre_fold=%d state32_discovery=0 outer_local_guard=1 remat_candidate_gate=0 entry_commit_le32=%d entry_restore_order=%d\n",entryFoldingPolicy,folding_variant::maximum(entryFoldingPolicy),(entryFoldingPolicy==2||entryFoldingPolicy==3)?1:0,entryFoldingPolicy==3?1:0,entryFoldingPolicy==3?1:0);
    printf("SIMD128_CONFIG enabled=%d variant=1 fullpair_only=1 recursive_export=1 whole_check=scalar maximality=scalar context_build=scalar runtime_selector=1\n",enableState128Simd?1:0);
    printf("TRANSFER_CONFIG queue_precheck=%d remap128=%d projection=bmi2_pext snapshot_gate=ordinary_recursive advisory_capacity=1\n",enableQueuePrecheck?1:0,enableRemap128?1:0);
    printf("k=%d,q=%d,threads=%d,progress_sec=%d,max_seeds=%d\n",k,lb,workerThreads,progressIntervalSeconds,seedTaskLimit);
    printf("algorithm=Unified-StateBit execution=shared queue_cap=%d recursive_split=%d list_split_us=%d bit_split_us=%d edbt_grain=%d tail_expansion=%d dds_demand_split=%d dds_bit_donate=%d demand_plus_one_split=%d dmfs_split=%d\n", MT_TASK_QUEUE_CAP,MT_RECURSIVE_SPLIT,MT_LIST_SPLIT_US,MT_BIT_SPLIT_US,MT_EDBT_GRAIN_SIZE,MT_TAIL_EXPANSION,MT_DDS_DEMAND_SPLIT,MT_DDS_BIT_DONATE,MT_DEMAND_PLUS_ONE_SPLIT,MT_DMFS_SPLIT);
    printf("LIST_UB_CONFIG list_recursive_greedy_ub=1 scope=edbt_list_include_child_only statebit_recursive_greedy_ub=0 word_recursive_greedy_ub=0\n");
    if(workerThreads<1){ fprintf(stderr, "error: threads must be >= 1\n"); return 2; }
    if(progressIntervalSeconds<0){ fprintf(stderr, "error: progress_sec must be >= 0\n"); return 2; }
    if(seedTaskLimit<0){ fprintf(stderr, "error: max_seeds must be >= 0\n"); return 2; }
    if(k<2){ fprintf(stderr, "error: k must be an integer >= 2\n"); return 2; }
    const long long minQ=2LL*k-1;
    if(static_cast<long long>(lb) < minQ){ fprintf(stderr, "error: q must satisfy q >= 2*k-1\n"); return 2; }
    printf("kernel=bit64-runtime-generic-k%d\n", k);
    printf("LANE_CONFIG bmi2=%d paired_hotpath=%d telemetry=%d sync_pivot=%d\n",
           LANE_USE_BMI2,LANE_USE_PAIRED_HOTPATH,LANE_PROFILE,LANE_SYNC_PIVOT);
    printf("LANE_PIVOT_CONFIG mode=%d regret_max=%d telemetry=%d\n",LANE_PIVOT_MODE,LANE_REGRET_MAX,LANE_PIVOT_PROFILE);
    bd = lb-k;
    graph<intT> g = statebit::readBinaryGraph(argv[1]);
    statebit::enumerateGraph(g);
    g.del();
    
    return 0;
}
