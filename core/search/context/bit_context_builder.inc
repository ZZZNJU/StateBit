        void bitBuildCurrentTaskRuntime(){
            experiment_control::SampledSpan sampledSpan(experiment_control::ContextBuild);
            defectCacheInvalidate();
            if(bitForcedCount>0) flushLazyLeftLogicalAdj();
            const auto diagStart=std::chrono::steady_clock::now();
            ++bestDiagStats.bit64_build_calls;
            ++aggregateForcedContextStats.build_calls;
            const unsigned long long aggHandoff=++aggHandoffSeq;

            auto phaseStart=std::chrono::steady_clock::now();
            for(int i=0;i<bitFixed;i++) bitOrig[i]=plex->members[i];
            for(int i=0;i<cand1->sz;i++) bitOrig[bitFixed+i]=cand1->members[i];
            const uint64_t all=(bitM==64?~0ULL:((1ULL<<bitM)-1));
            for(int i=0;i<bitM;i++){
                bitAdj[i]=0;
                bitPair[i]=1ULL<<i;
            }
            for(int i=0;i<bitM;i++){
                const int vi=bitOrig[i];
                const uint8_t* row=commonMtx+static_cast<size_t>(vi)*static_cast<size_t>(subg.n);
                for(int j=i+1;j<bitM;j++){
                    const uint64_t bi=1ULL<<i, bj=1ULL<<j;
                    const uint8_t cm=row[bitOrig[j]];
                    if(cm&1){ bitAdj[i]|=bj; bitAdj[j]|=bi; }
                    if(!subg.proper||cm>=UNLINK2EQUAL){ bitPair[i]|=bj; bitPair[j]|=bi; }
                }
            }
            for(int i=0;i<bitM;i++) bitNon[i]=all&~bitAdj[i]&~(1ULL<<i);
            aggregateForcedContextStats.internal_ns+=diagNs(phaseStart,std::chrono::steady_clock::now());

            bitExtN=excl->sz;
            ensureBitAuxCapacity(bitExtN,leftSz);
            unsigned long long forcedExtChecks=0,forcedLeftEdges=0;

            phaseStart=std::chrono::steady_clock::now();
            for(int e=0;e<bitExtN;e++){
                bitExtOrig[e]=excl->members[e];
                uint64_t a=0;
                for(int j=0;j<bitM;j++) if(isAdjMtx(bitExtOrig[e],bitOrig[j])) a|=1ULL<<j;
                bitExtAdj[e]=a;
            }
            aggregateForcedContextStats.ext_active_ns+=diagNs(phaseStart,std::chrono::steady_clock::now());

            phaseStart=std::chrono::steady_clock::now();
            for(int e=0;e<bitExtN;e++){
                int fa=0;
                if(bitForcedCount){
#if AGG_FORCED_CONTEXT_MODE == AGG_MODE_EXTERNAL_ONLY || AGG_FORCED_CONTEXT_MODE == AGG_MODE_COMBINED
                    const int x=bitExtOrig[e];
                    fa=aggregateForcedAdj(getNeiInG(x),bitPop(bitExtAdj[e]));
                    ++aggregateForcedContextStats.ext_formula_rows;
                    if(fa<0 || fa>bitForcedCount) ++aggregateForcedContextStats.invalid_formula;
#elif AGG_FORCED_CONTEXT_MODE == AGG_MODE_VALIDATION
                    int oldFa=0;
                    for(const int f:forcedBase){ ++forcedExtChecks; if(isAdjMtx(bitExtOrig[e],f)) ++oldFa; }
                    const int x=bitExtOrig[e];
                    const int aggFa=aggregateForcedAdj(getNeiInG(x),bitPop(bitExtAdj[e]));
                    ++aggregateForcedContextStats.ext_formula_rows;
                    if(aggFa<0 || aggFa>bitForcedCount) ++aggregateForcedContextStats.invalid_formula;
                    if(aggFa!=oldFa) aggregateForcedContextStats.recordMismatch(aggDiagSeedIndex,aggHandoff,1,e,bitExtOrig[e],oldFa,aggFa);
                    fa=oldFa;
#else
                    for(const int f:forcedBase){ ++forcedExtChecks; if(isAdjMtx(bitExtOrig[e],f)) ++fa; }
#endif
                }
                bitExtForcedAdj[e]=fa;
            }
            aggregateForcedContextStats.ext_forced_ns+=diagNs(phaseStart,std::chrono::steady_clock::now());

            phaseStart=std::chrono::steady_clock::now();
            for(int l=0;l<leftSz;l++){ bitLeftAdj[l]=0; bitLeftForcedAdj[l]=0; }
            aggregateForcedContextStats.left_clear_ns+=diagNs(phaseStart,std::chrono::steady_clock::now());

#if AGG_FORCED_CONTEXT_MODE == AGG_MODE_STANDARD || AGG_FORCED_CONTEXT_MODE == AGG_MODE_VALIDATION || AGG_FORCED_CONTEXT_MODE == AGG_MODE_MAINTENANCE_ONLY || AGG_FORCED_CONTEXT_MODE == AGG_MODE_EXTERNAL_ONLY
            phaseStart=std::chrono::steady_clock::now();
            if(bitForcedCount){
                for(const int f:forcedBase){
                    for(int z=0;z<subg.V[f].degreeLeft;++z){
                        const int l=subg.V[f].NeighborsLeft[z];
                        if(l>=0 && l<leftSz){ ++bitLeftForcedAdj[l]; ++forcedLeftEdges; }
                    }
                }
            }
            aggregateForcedContextStats.left_forced_ns+=diagNs(phaseStart,std::chrono::steady_clock::now());
#endif

            leftNon32Ready=false;
            leftChunks32=0;
            phaseStart=std::chrono::steady_clock::now();
            for(int j=0;j<bitM;j++){
                const int v=bitOrig[j];
                for(int z=0;z<subg.V[v].degreeLeft;z++){
                    bitLeftAdj[subg.V[v].NeighborsLeft[z]]|=1ULL<<j;
                }
            }
            aggregateForcedContextStats.left_active_ns+=diagNs(phaseStart,std::chrono::steady_clock::now());

#if AGG_FORCED_CONTEXT_MODE == AGG_MODE_LEFT_ONLY || AGG_FORCED_CONTEXT_MODE == AGG_MODE_COMBINED
            phaseStart=std::chrono::steady_clock::now();
            if(bitForcedCount){
                for(int l=0;l<leftSz;l++){
                    const int fa=aggregateForcedAdj(leftLogicalAdj.get(l),bitPop(bitLeftAdj[l]));
                    bitLeftForcedAdj[l]=fa;
                    ++aggregateForcedContextStats.left_formula_rows;
                    if(fa<0 || fa>bitForcedCount) ++aggregateForcedContextStats.invalid_formula;
                }
            }
            aggregateForcedContextStats.left_forced_ns+=diagNs(phaseStart,std::chrono::steady_clock::now());
#elif AGG_FORCED_CONTEXT_MODE == AGG_MODE_VALIDATION
            phaseStart=std::chrono::steady_clock::now();
            if(bitForcedCount){
                for(int l=0;l<leftSz;l++){
                    const int aggFa=aggregateForcedAdj(leftLogicalAdj.get(l),bitPop(bitLeftAdj[l]));
                    ++aggregateForcedContextStats.left_formula_rows;
                    if(aggFa<0 || aggFa>bitForcedCount) ++aggregateForcedContextStats.invalid_formula;
                    if(aggFa!=bitLeftForcedAdj[l]) aggregateForcedContextStats.recordMismatch(aggDiagSeedIndex,aggHandoff,2,l,-1,bitLeftForcedAdj[l],aggFa);
                }
            }
            aggregateForcedContextStats.validation_verify_ns+=diagNs(phaseStart,std::chrono::steady_clock::now());
#endif

            if(bitForcedCount) forcedBaseBit64Stats.observeBuild(bitExtN,leftSz,forcedExtChecks,forcedLeftEdges);
            bitTaskProper=subg.proper;
            bestDiagStats.bit64_build_ns+=diagNs(diagStart,std::chrono::steady_clock::now());
        }

        void forcedBaseBit64EnumerateCurrentTask(){
            const int active=plex->sz+cand1->sz;
            assert(k>=2);
            assert(forcedBaseBit64Eligible(active,FORCED_BASE_BIT64_THRESHOLD));
            const int savedForced=bitForcedCount;
            bitForcedCount=forcedBaseSize(); defectCacheInvalidate();
            bitFixed=plex->sz;
            bitM=active;
            forcedBaseBit64Stats.observeHandoff(plex->sz,cand1->sz,bitForcedCount);
            bitBuildCurrentTaskRuntime();
            const unsigned long long n0=bitStats.nodes,w0=bitStats.whole_calls,l0=bitStats.leaf_calls,o0=bitStats.outputs;
            const uint64_t all=(bitM==64?~0ULL:((1ULL<<bitM)-1));
            const uint64_t P=(bitFixed==64?~0ULL:((1ULL<<bitFixed)-1));
            const uint64_t C=all&~P;
            bit64Dispatch(P,C,0);
            forcedBaseBit64Stats.observeKernelDelta(bitStats.nodes-n0,bitStats.whole_calls-w0,bitStats.leaf_calls-l0,bitStats.outputs-o0);
            bitForcedCount=savedForced; defectCacheInvalidate();
        }

        inline void bitExecuteBuiltTask(){
            const uint64_t all=(bitM==64?~0ULL:((1ULL<<bitM)-1));
            const uint64_t P=(bitFixed==64?~0ULL:((1ULL<<bitFixed)-1));
            const uint64_t C=all&~P;
            bit64Dispatch(P,C,0);
        }

        void bitEnumerateCurrentTaskRuntime(){
            bitBuildCurrentTaskRuntime();
            bitExecuteBuiltTask();
        }

