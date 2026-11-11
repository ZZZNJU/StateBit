        void avx4x64PacketDFS(const uint64_t rootP,const uint64_t rootC,const uint64_t rootX){
            avx4x64RefillDFS(rootP,rootC,rootX);
        }

        template<class StackT>
        void avx4x64RefillStack(StackT& stack){
            BATCH_MAXIMALITYMaxReady64 maxBatch[4];int maxActive=0;
            statebit_state64_search::refill<LaneState64>(stack,
                [&](LaneState64& st,LaneState64& ready)->bool {
                    BATCH_MAXIMALITYMaxReady64 mr;
                    const bool ok=prepare4x64ReadyState(st,&mr);
                    if(mr.valid){maxBatch[maxActive++]=mr;
                        if(maxActive==4){batchMaximalityFlushMax64(maxBatch,4);maxActive=0;}}
                    if(ok)ready=std::move(st);
                    return ok;
                },
                [&](const LaneState64 *ready,int active,StackT& stk){branch4x64Packet(ready,active,stk);},
                [&](StackT& stk){spillState64StackForParallelism(stk);},
                [&](){
                    if(maxActive)batchMaximalityFlushMax64(maxBatch,maxActive);
                });
        }

        void avx4x64RefillDFS(const uint64_t rootP,const uint64_t rootC,const uint64_t rootX){
            vector<LaneState64> stack;
            stack.reserve(256);
            stack.push_back({rootP,rootC,rootX});
            avx4x64RefillStack(stack);
        }

        // Word64 expansion uses the same pivot and refinement contract as State64.
        // In parity mode 1 the native SIMD transition is the oracle for each
        // scalar branch, guaranteeing identical children even on a mismatch.
        inline int word64Pivot(const LaneState64& st) const {
            return word64_common::select_pivot(st.P,st.C,k,eagerFoldingActiveLb64(st),bitAdj,bitNon,-2);
        }

        inline uint64_t word64Refine(const LaneState64& st,const int pivot,
                const uint64_t Pinc,const uint64_t Cminus){
            ++bitStats.cand_filter_calls;
            const uint64_t refined=word64_common::refine_include(pivot,Pinc,Cminus,k,
                eagerFoldingActiveLb64(st),bitPair,bitNon,bitAdj,word64_common::RefineOrder::BulkSnapshot);
            bitStats.cand_filter_removed+=bitPop(Cminus)-bitPop(refined);
            return refined;
        }
        // Fixed-size sampling buckets for subtree-cost observations.
        struct FreshSimpleBucket {
            uint64_t calls=0,samples[2]={0,0},nodes[2]={0,0},ns[2]={0,0};
        };
        void word64Search(const uint64_t P,const uint64_t C,const uint64_t X){
            statebit_search_core::word64_dfs<LaneState64>(LaneState64{P,C,X},
                [&](LaneState64& st)->bool {return prepare4x64ReadyState(st);},
                [&](const LaneState64& st)->int {
                    int pivot=word64Pivot(st);
                    return pivot;
                },
                [&](const LaneState64& st,int pivot,uint64_t pi,uint64_t cm)->uint64_t {
                    ++state64Stats.fresh64_word_branch_nodes;
                    uint64_t ci=word64Refine(st,pivot,pi,cm);
                    return ci;
                },
                [&](const LaneState64& st,const statebit_core::Branch64& b,bool include)->LaneState64 {
                    if(!include)
                        return statebit_word64_search2::make_child(st,b.pe,b.ce,b.xe,b.bit,
                            [&](uint64_t changed,uint64_t cand){return eagerFoldingAffected64(changed,cand);});
                    const uint64_t removed=(st.P|st.C)&~(b.pi|b.ci);
                    return statebit_word64_search2::make_child(st,b.pi,b.ci,b.xi,removed,
                        [&](uint64_t changed,uint64_t cand){return eagerFoldingAffected64(changed,cand);});
                },
                [&](const LaneState64&,int,const statebit_core::Branch64&)->bool {return true;},
                [&](bool include){++bitStats.exclude_calls;
                    if(include)++bitStats.include_calls;
                    state64Stats.fresh64_word_children+=1+(include?1:0);},
                [&](std::vector<LaneState64>& stack){
                    const size_t pre=stack.size();
                    spillState64StackForParallelism(stack);
                    if(stack.size()<pre)state64Stats.fresh64_word_spilled_states+=pre-stack.size();
                });
        }
        void dispatch64(const uint64_t P,const uint64_t C,const uint64_t X){
            auto run=[&](auto& learner){
            statebit_search_core::adaptive64_task(learner,bitPop(P|C),bitPop(C),bitActiveLb(),
                [&]()->uint64_t{return bitStats.nodes;},
                [&]()->uint64_t{return statebit_width_selection::tls().exported;},
                [&](bool simd,bool sample){
                    if(simd)++state64Stats.fresh64_state64_tasks;
                    else ++state64Stats.fresh64_word_tasks;
                    if(sample)++state64Stats.fresh64_observations;
                },
                [&](bool split){
                    if(split)++state64Stats.fresh64_split_skips;
                    else ++state64Stats.fresh64_accepted;
                },
                [&](){experiment_control::SampledSpan span(experiment_control::Word64);word64Search(P,C,X);},
                [&](){
                    experiment_control::SampledSpan span(experiment_control::State64);
                    avx4x64RefillDFS(P,C,X);
                });
            };
            if(kernelSelectionMode==0)run(kernel_selection::threadPolicy());
            else if(kernelSelectionMode==1)run(experiment_control::adaptivePolicy());
            else {experiment_control::Fixed64Policy fixed(kernelSelectionMode==3);run(fixed);}
        }

        // The only <=64 search entry. Bit64 is a data/context representation,
        // not a search kernel. 33-64 selects Word64 or State64; 1-32 enters State32.
        // This dispatcher never recurses.
        void bit64Dispatch(const uint64_t P,const uint64_t C,const uint64_t X){
            wide_cost::Stage costScope(wide_cost::Lower64);
            ++bitStats.bit64_dispatches;
            const uint64_t U=P|C;
            const int live=bitPop(U);
            assert(live>0 && live<=64);

            if(statebit_unified::plan({live,0,k},{true,false,false})==statebit_unified::Route::Adaptive64){
                ++bitStats.state64_direct_dispatches;
                dispatch64(P,C,X);
                return;
            }

            ++bitStats.state32_direct_dispatches;
            const auto mode=PARALLEL_ENGINE_STATE32_ROOT_PRECHECK
                ? statebit_state32_handoff::RootMode::Checked : statebit_state32_handoff::RootMode::KernelChecks;
            statebit_state32_handoff::dispatch({P,C,X,bitActiveLb(),k,false},mode,bitNon,
                [&](){
                    ++bitStats.nodes;
                    ++bitStats.state32_root_prechecks;
                },
                [&](uint64_t U){
                    ++bitStats.whole_calls;
                    bitIsMaximalAndEmit(U,X);
                    ++bitStats.bit64_terminal_dispatches;
                },
                [&](uint64_t leaf){
                    ++bitStats.leaf_calls;
                    if(bitPop(leaf)>=bitActiveLb())bitIsMaximalAndEmit(leaf,X);
                    ++bitStats.bit64_terminal_dispatches;
                },
                [&](bool checked){
                    if(bitForcedCount>0)forcedBaseBit64Stats.observeState32Transition();
#if AVX_STAGE_REFILL
                    avxStageBatchedDFS(P,C,X,checked);
#else
                    avx2PacketDFS(P,C,X,checked);
#endif
                });
        }

