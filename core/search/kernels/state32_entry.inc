        void avxStageBatchedDFSLocalF(const uint64_t P,const uint64_t C,
                                       const uint64_t X,const uint64_t F){
            experiment_control::SampledSpan sampledSpan(experiment_control::State32LocalF);
            assert(F && local_fixed_localf_contract::valid(P,C,X,F));
            assert(bitForcedCount==0 || enableMixedFixedContext); // mixed outer/local contract verified by this stage
            const uint64_t savedF=eagerFoldingState32LocalF;
            eagerFoldingState32LocalF=F;
            STATE32_DISPATCHState32HandoffGuard state32DispatchGuard(this);
#if AVX_PERSISTENT_SOA
            StageSoAStatePool stack;
#else
            StageAoSStatePool stack;
#endif
            state32CoreDFS(P,C,X,stack,true,F);
            eagerFoldingState32LocalF=savedF;
            if(bitForcedCount&&enableMixedFixedContext)++mixed_f64::stats().local_restored;
        }
        void avxStageBatchedDFS(const uint64_t rootP,const uint64_t rootC,const uint64_t rootX,const bool rootPrechecked=true,const bool rematerializedRoot=false){
            experiment_control::SampledSpan sampledSpan(experiment_control::State32Plain);
            STATE32_DISPATCHState32HandoffGuard state32DispatchGuard(this);
            if(rematerializedRoot){runRematWithPool(rootP,rootC,rootX,rootPrechecked);return;}
#if AVX_PERSISTENT_SOA
            StageSoAStatePool stack;
#else
            StageAoSStatePool stack;
#endif
            avxStageBatchedDFSImpl(rootP,rootC,rootX,stack,rootPrechecked,rematerializedRoot);
        }

        void avx2PacketDFS(const uint64_t rootP,const uint64_t rootC,const uint64_t rootX,const bool rootPrechecked=true){
            STATE32_DISPATCHState32HandoffGuard state32DispatchGuard(this);
            defectCachePrepareState32();
            const uint64_t rootU=rootP|rootC;
            const int m=bitPop(rootU);
            assert(m<=32 && rootC);
            AVX_COUNT(++avxStats.transitions);
            AVX_COUNT(avxStats.recode_vertices += m);
            LaneContext32 ctx=laneBuildContext(rootU,rootX);
            vector<LaneState32> stack;
            stack.reserve(128);
            stack.push_back({laneCompress(rootP,ctx),laneCompress(rootC,ctx),0,rootPrechecked});
            AVX_COUNT(avxStats.max_stack = std::max<unsigned long long>(avxStats.max_stack,stack.size()));
            while(!stack.empty()){
                LaneState32 st[8];
                int active=0;
                while(active<8 && !stack.empty()){ st[active++]=stack.back(); stack.pop_back(); }
                AVX_COUNT(++avxStats.packets);
                AVX_COUNT(avxStats.states += active);
                AVX_COUNT(avxStats.active_slots += active);
                if(active==8) AVX_COUNT(++avxStats.full_packets); else AVX_COUNT(++avxStats.partial_packets);
                uint32_t P[8]={0},C[8]={0},X[8]={0};
                int outcome[8]; int pivot[8];
                uint8_t needPivotMask=0,branchMask=0;
                for(int i=0;i<8;i++){ outcome[i]=-1; pivot[i]=-1; if(i<active){P[i]=st[i].P;C[i]=st[i].C;X[i]=st[i].X;} }
                for(int lane=0;lane<active;lane++){
                    if(st[lane].prechecked) continue;
                    ++bitStats.nodes;
                    if(lanePop(P[lane])+lanePop(C[lane])<bitActiveLb()){ outcome[lane]=0; continue; }
                    const uint32_t U=P[lane]|C[lane];
                    bool whole=true;
                    uint32_t t=U;
                    while(t){ const int u=laneFirst(t); t&=t-1; if(lanePop(ctx.nonadj[u]&U)>=k){ whole=false; break; } }
                    if(whole){ ++bitStats.whole_calls; laneIsMaximalAndEmit(U,X[lane],ctx); outcome[lane]=1; continue; }
                    if(!C[lane]){ ++bitStats.leaf_calls; if(lanePop(P[lane])>=bitActiveLb()) laneIsMaximalAndEmit(P[lane],X[lane],ctx); outcome[lane]=2; continue; }
                }
                for(int lane=0;lane<active;lane++) if(outcome[lane]<0) needPivotMask|=static_cast<uint8_t>(1u<<lane);
#if AVX_VECTOR_PIVOT
                if(needPivotMask) avxChoosePivot8(P,C,needPivotMask,ctx,pivot);
#else
                for(int lane=0;lane<active;lane++) if(needPivotMask&(1u<<lane)){ AVX_COUNT(++avxStats.scalar_pivot_calls); pivot[lane]=laneChoosePivot(P[lane],C[lane],ctx); }
#endif
                int firstPivot=-3; bool pivotDiv=false;
                for(int lane=0;lane<active;lane++) if(needPivotMask&(1u<<lane)){
                    if(pivot[lane]==-2 || pivot[lane]<0) outcome[lane]=3;
                    else{
                        outcome[lane]=4; branchMask|=static_cast<uint8_t>(1u<<lane); AVX_COUNT(++avxStats.branch_states);
                        if(firstPivot==-3) firstPivot=pivot[lane]; else if(firstPivot!=pivot[lane]) pivotDiv=true;
                    }
                }
                if(pivotDiv) AVX_COUNT(++avxStats.pivot_divergent_packets);
                if(!branchMask) continue;
                uint32_t Pinc[8]={0},Cminus[8]={0},Cinc[8]={0},Xexc[8]={0};
                for(int lane=0;lane<active;lane++) if(branchMask&(1u<<lane)){
                    const uint32_t b=1u<<pivot[lane];
                    Pinc[lane]=P[lane]|b;
                    Cminus[lane]=C[lane]&~b;
                    Xexc[lane]=X[lane]|b;
                }
#if AVX_GATHER_REFINE
                avxRefineIncludeEight(pivot,Pinc,Cminus,branchMask,ctx,Cinc);
#else
                for(int lane=0;lane<active;lane++) if(branchMask&(1u<<lane)){
                    AVX_COUNT(++avxStats.scalar_refine_calls);
                    Cinc[lane]=laneRefineIncludeOnce(pivot[lane],Pinc[lane],Cminus[lane],ctx);
                }
#endif
                uint32_t incP[8]={0},incC[8]={0},incX[8]={0},excP[8]={0},excC[8]={0},excX[8]={0};
                uint8_t incMask=0,excMask=0;
                for(int lane=0;lane<active;lane++) if(branchMask&(1u<<lane)){
                    ++bitStats.include_calls;
                    incP[lane]=Pinc[lane]; incC[lane]=Cinc[lane]; incX[lane]=X[lane]; incMask|=static_cast<uint8_t>(1u<<lane);
                    ++bitStats.exclude_calls;
                    excP[lane]=P[lane]; excC[lane]=Cminus[lane]; excX[lane]=Xexc[lane]; excMask|=static_cast<uint8_t>(1u<<lane);
                }
                AVX_COUNT(avxStats.generated_children += __builtin_popcount(static_cast<unsigned>(incMask))+__builtin_popcount(static_cast<unsigned>(excMask)));
#if AVX_COMPACTION
                avxPushCompacted(stack,excP,excC,excX,excMask);
                avxPushCompacted(stack,incP,incC,incX,incMask);
#else
                for(int lane=0;lane<active;lane++) if(excMask&(1u<<lane)) stack.push_back({excP[lane],excC[lane],excX[lane],false});
                for(int lane=0;lane<active;lane++) if(incMask&(1u<<lane)) stack.push_back({incP[lane],incC[lane],incX[lane],false});
#endif
                AVX_COUNT(avxStats.max_stack = std::max<unsigned long long>(avxStats.max_stack,stack.size()));
            }
        }

