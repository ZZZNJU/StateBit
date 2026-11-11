            resetThreadStats();
            threadSolutionCount=0;
            const auto workerStart=std::chrono::steady_clock::now();
            table4Audit=Table4Audit{};
            c0Opportunity=C0Opportunity{};
            mixed_f64::stats()=mixed_f64::Stats{};
            wide_cost::tls()=wide_cost::Tracker{};
            transferAudit=TransferAudit{};
            simd128Audit=Simd128Audit{};
            kernel_selection::threadPolicy()=kernel_selection::Policy{};
            experiment_control::adaptivePolicy()=adaptive64::Policy{};
            experiment_control::samples()=experiment_control::Samples{};
#ifdef SET
            verifyCount=verifyXor=verifySum=0;
#endif

            // Seed-local search workspaces remain private to each worker.
            vector<uint8_t> visited(pn,0);
            vector<int> count(pn,0);
            vector<int> neibors;
            vector<int> neiborsLeft;
            // 4096 is now only an initial allocation hint, never a hard limit.
            // All worker-local search containers grow to the actual seed size
            // and are then reused by later seeds on the same worker.
            Stack<int> plex(PSIZE_LIMIT);
            VtxSet cand1(CAP_LIMIT1);
            VtxSet cand2(CAP_LIMIT2);
            VtxSet excl(CAP_LIMIT2);
            VtxSet exclBK(CAP_LIMIT2);
            Stack<int> exclStack(CAP_LIMIT2);
            vector<int> neiInP(CAP_LIMIT2,0);
            vector<int> neiInG(CAP_LIMIT2);
            vector<int> nonadjInP(PSIZE_LIMIT,0);

            // commonMtx dominates large-seed memory. Allocate exactly the
            // largest matrix this worker has needed so far, without the
            // vector resize zero-fill (buildSubgraph overwrites every cell).
            unique_ptr<uint8_t[]> commonMtx;
            size_t commonMtxCap=0;
            auto ensureCommonMtx = [&](const int blkSz)->uint8_t* {
                const size_t b=static_cast<size_t>(blkSz);
                if(b!=0 && b>std::numeric_limits<size_t>::max()/b){
                    fprintf(stderr,"LARGE_SEED_SIZE_OVERFLOW tid=%d blkSz=%d\n",tid,blkSz);
                    std::abort();
                }
                const size_t need=b*b;
                if(need>commonMtxCap){
                    unique_ptr<uint8_t[]> next(new (std::nothrow) uint8_t[need]);
                    if(!next){
                        fprintf(stderr,"LARGE_SEED_ALLOCATION_FAILED tid=%d blkSz=%d matrix_bytes=%zu\n",tid,blkSz,need);
                        std::abort();
                    }
                    commonMtx.swap(next);
                    commonMtxCap=need;
                }
                return commonMtx.get();
            };
            vector<int> blk;
            vector<int> left;
            vector<uint8_t> inBlk(pn,0);
            vector<uint8_t> inLeft(pn,0);

            // A persistent helper-side StateBit executor.  Exported tasks are
            // self-contained, so it does not own or share a seed subgraph.
            Graph dummyGraph;
            dummyGraph.proper=true;
            GlobalMaximalityChecker helperMax(dummyGraph);
            PlexEmitter helperEmit;
            Stack<int> helperPlex(64);
            VtxSet helperCand1(64),helperCand2(64),helperExcl(64),helperExclBK(64);
            Stack<int> helperExclStack(64);
            int helperNeiP[64]={0},helperNeiG[64]={0},helperNonadj[64]={0};
            uint8_t helperMtx=0;
            MTEnum helperEnum(dummyGraph,&helperMax,&helperEmit,&helperPlex,&helperCand1,&helperCand2,
                                  &helperExcl,&helperExclBK,&helperExclStack,
                                  helperNeiP,helperNeiG,helperNonadj,0,0,0,&helperMtx);
            helperEnum.enableMTRecursiveExport(&taskPool,mtSubmitThunk,&taskPool.idleWorkers,tid,nullptr,
                                               &nextSeed,&completedSeeds,targetSeeds,actualThreads,
                                               &taskPool.exported,&taskPool.executed,&taskPool.tail_mode_entries,&taskPool.tail_refill_requests,&taskPool.tailEverEntered,
                                               &taskPool.dds_reservations,&taskPool.dds_demand_claimed,&taskPool.dds_demand_released,nullptr,nullptr,mtDMFSClaimThunk,mtDMFSReleaseThunk);

            helperEnum.mtCapacityFn=mtCapacityThunk;
            unsigned long long localValid=0;
            unsigned long long localSeeds=0;
            SeedModeStats localSeedMode;
            unsigned long long localHelpTasks=0;
            unsigned long long localStolenTasks=0;
            unsigned long long localLargeSeeds=0;
            int localMaxHop=0,localMaxBlk=0,localMaxLeft=0;
            size_t localMaxMatrixBytes=0;
            FOLDING_GATEEarlyOffAggregate localFoldingEarlyOff;

