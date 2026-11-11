// ==== single shared 8x32 packet DFS ====
        template<class Pool>
        void state32CoreDFS(const uint64_t rootP,const uint64_t rootC,const uint64_t rootX,
                             Pool& stack,const bool rootPrechecked,const uint64_t forcedContextLocalF,
                             const bool rematerializedRoot=false){
            wide_cost::Stage costScope(wide_cost::Lower32);
            state32CoreEntryPrepare();
            const uint64_t rootU=rootP|rootC;
            const int m=bitPop(rootU);
            assert(m<=32 && rootC);
            AVX_COUNT(++avxStats.transitions);
            AVX_COUNT(avxStats.recode_vertices += m);
            LaneContext32 ctx;
            if(rematerializedRoot){
                experiment_control::SampledSpan buildSpan(experiment_control::RematContextBuild);
                ctx=laneBuildContext(rootU,rootX);
            }else ctx=laneBuildContext(rootU,rootX);
            LocalMaxSuffixCache32 suffixCache;
            beginLocalMaxSuffixContext(suffixCache);
            state32CoreBeginLocalF(forcedContextLocalF);
            stack.reserve(256);
            stack.push({laneCompress(rootP,ctx),laneCompress(rootC,ctx),0,rootPrechecked,WHOLE_WITNESS_NONE});
            AVX_COUNT(++avxStats.state_pool_pushes);
            AVX_COUNT(avxStats.max_stack = std::max<unsigned long long>(avxStats.max_stack,stack.size()));
            MaxReadyState32 maxBatch[8];
            int maxActive=0;
            const bool rematRequested=enableLocalRematerialization && forcedContextLocalF!=0;
            // Every convertible state occupies <=32 total active+local-F bits.
            // If even that upper bound misses the minimum slack, no descendant
            // can convert. Skip all per-ready-state remat work in this context.
            const bool rematPossible=remat_budget::possible(bitForcedCount,lb,rematerializationMinSlack);
            const bool tryRemat=rematRequested && rematPossible;
            if(rematRequested){auto& g=experiment_control::samples();++g.gate_contexts;if(!rematPossible)++g.gate_width_impossible;}
            const int localFSize=bitPop(forcedContextLocalF);
            const int rematActiveLimit=32-localFSize;
            while(!stack.empty()){
                LaneState32 batch[8];
                int active=0;
                AVX_COUNT(++avxStats.soa_refills);
                while(active<8 && !stack.empty()){
                    LaneState32 st=stack.pop(); if(forcedContextLocalF&&bitForcedCount&&enableMixedFixedContext)++mixed_f64::stats().local_prepare; AVX_COUNT(++avxStats.state_pool_pops);
                    LaneState32 ready; MaxReadyState32 mr;
                    const int kind=avxPrepareBranchOrMaxState(st,ctx,ready,mr);
                    if(kind==1){
                        if(tryRemat){
                            ++experiment_control::samples().remat_checks;
                            const int live=lanePop(ready.P|ready.C);
                            if(live<=rematActiveLimit){
                                auto& g=experiment_control::samples();++g.gate_eligible;
                                ++g.gate_slack[remat_budget::bucket(remat_budget::slack(live,localFSize,bitForcedCount,lb))];
                                if(remat_budget::allow(live,localFSize,bitForcedCount,lb,rematerializationMinSlack)){
                                    ++g.gate_allowed;
                                    state32CoreRematerialize(ready,ctx,forcedContextLocalF);
                                    continue;
                                }
                                ++g.gate_denied;
                            }
                        }
                        profileReadyState(ready); batch[active++]=ready;
                    }
                    else if(kind==2){
                        maxBatch[maxActive++]=mr;
                        if(maxActive==8){ state32CoreFlushMax(maxBatch,maxActive,ctx,suffixCache,forcedContextLocalF); maxActive=0; }
                    }
                }
                if(!active) continue;
                profileBranchBatchWork(batch,active);
                AVX_COUNT(++avxStats.branch_batches);
                AVX_COUNT(avxStats.branch_batch_slots += active);
                if(active==8) AVX_COUNT(++avxStats.branch_full_batches);
                AVX_COUNT(++avxStats.packets);
                AVX_COUNT(avxStats.active_slots += active);
                if(active==8) AVX_COUNT(++avxStats.full_packets); else AVX_COUNT(++avxStats.partial_packets);
                uint32_t P[8]={0},C[8]={0},X[8]={0}; int pivot[8];
                for(int i=0;i<8;i++){pivot[i]=-1;if(i<active){P[i]=batch[i].P;C[i]=batch[i].C;X[i]=batch[i].X;}}
                const uint8_t needMask=avxActiveMask(active);
#if AVX_UNION_SPARSE_PIVOT
                avxChoosePivot8UnionSparse(P,C,needMask,ctx,pivot);
#else
                for(int i=0;i<active;i++){ AVX_COUNT(++avxStats.scalar_pivot_calls); pivot[i]=laneChoosePivot(P[i],C[i],ctx); }
#endif
                uint8_t branchMask=0; int firstPivot=-3; bool pivotDiv=false;
                for(int i=0;i<active;i++){
                    if(pivot[i]>=0){ branchMask|=static_cast<uint8_t>(1u<<i); AVX_COUNT(++avxStats.branch_states); if(firstPivot==-3) firstPivot=pivot[i]; else if(firstPivot!=pivot[i]) pivotDiv=true; }
                }
                if(pivotDiv) AVX_COUNT(++avxStats.pivot_divergent_packets);
                if(!branchMask) continue;
                const int refineActive=__builtin_popcount(static_cast<unsigned>(branchMask));
                AVX_COUNT(++avxStats.refine_batches);
                AVX_COUNT(avxStats.refine_batch_slots += refineActive);
                if(refineActive==8) AVX_COUNT(++avxStats.refine_full_batches);
                uint32_t Pinc[8]={0},Cminus[8]={0},Cinc[8]={0},Xexc[8]={0};
                for(int i=0;i<active;i++) if(branchMask&(1u<<i)){
                    const uint32_t b=1u<<pivot[i]; Pinc[i]=P[i]|b; Cminus[i]=C[i]&~b; Xexc[i]=X[i]|b;
                }
#if AVX_UNION_SPARSE_REFINE
                avxRefineIncludeEightUnionSparse(pivot,Pinc,Cminus,branchMask,ctx,Cinc);
#else
                for(int i=0;i<active;i++) if(branchMask&(1u<<i)){ AVX_COUNT(++avxStats.scalar_refine_calls); Cinc[i]=laneRefineIncludeOnce(pivot[i],Pinc[i],Cminus[i],ctx); }
#endif
                for(int i=0;i<active;i++) if(branchMask&(1u<<i)){
                    LaneState32 exc=batch[i];
                    exc.P=P[i]; exc.C=Cminus[i]; exc.X=Xexc[i]; exc.prechecked=false;
                    LaneState32 inc=batch[i];
                    inc.P=Pinc[i]; inc.C=Cinc[i]; inc.X=X[i]; inc.prechecked=false;
                    ++bitStats.exclude_calls; stack.push(exc); AVX_COUNT(++avxStats.state_pool_pushes);
                    ++bitStats.include_calls; stack.push(inc); AVX_COUNT(++avxStats.state_pool_pushes);
                    WITNESS_COUNT(if(batch[i].wholeWitness<32) wholeWitnessStats.inherited_children += 2);
                    AVX_COUNT(avxStats.generated_children += 2);
                }
                AVX_COUNT(avxStats.max_stack = std::max<unsigned long long>(avxStats.max_stack,stack.size()));
            }
            if(maxActive){ state32CoreFlushMax(maxBatch,maxActive,ctx,suffixCache,forcedContextLocalF); }
        }
