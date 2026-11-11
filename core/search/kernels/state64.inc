        struct LaneState64 {
            uint64_t P=0,C=0,X=0;
            // STATE_WITNESS only: cached violating vertex for Whole; never required for correctness.
            // When parallel exporting P/C/X as a BIT task, it is safe to drop this hint.
            uint8_t wholeWitness=PARALLEL_ENGINE_STATE64_WITNESS_NONE;
            // EAGER_FOLDING: local C0 vertices are logically fixed (not part of active P/C).
            uint64_t F=0,checkMask=0;
            bool rootNode=true,eagerInitialized=false;
        };

        inline int eagerFoldingActiveLb64(const LaneState64& st) const {
            return std::max(0,bitActiveLb()-bitPop(st.F));
        }

        inline uint64_t eagerFoldingAffected64(const uint64_t removed,const uint64_t C) const {
            if(!enableLocalFolding)return 0;
            if(enableMixedFixedContext&&bitForcedCount){
                auto& m=mixed_f64::stats();++m.affected_calls;m.affected_removed+=bitPop(removed);
                mixed_f64::Span span(m.affected_calls,m.affected_samples,m.affected_sample_ns);
                return mixed_f64::affected(removed,C,bitNon);
            }
            uint64_t ans=0,t=C;
            while(t){const int x=bitFirst(t);t&=t-1;if(bitNon[x]&removed)ans|=1ULL<<x;}
            return ans;
        }

        inline __m256i avxPopcnt4x64(__m256i x) const {
            const __m256i lut=_mm256_setr_epi8(
                0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,
                0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4);
            const __m256i lowMask=_mm256_set1_epi8(0x0f);
            const __m256i lo=_mm256_and_si256(x,lowMask);
            const __m256i hi=_mm256_and_si256(_mm256_srli_epi16(x,4),lowMask);
            const __m256i bytes=_mm256_add_epi8(_mm256_shuffle_epi8(lut,lo),_mm256_shuffle_epi8(lut,hi));
            return _mm256_sad_epu8(bytes,_mm256_setzero_si256());
        }
        inline __m256i avx4x64LaneMask(const int active) const {
            alignas(32) uint64_t v[4]={0,0,0,0};
            for(int i=0;i<active && i<4;i++) v[i]=~0ULL;
            return _mm256_load_si256(reinterpret_cast<const __m256i*>(v));
        }
        inline __m256i avxMax4x64(const __m256i a,const __m256i b) const {
            const __m256i takeB=_mm256_cmpgt_epi64(b,a);
            return _mm256_blendv_epi8(a,b,takeB);
        }

        // BATCH_MAXIMALITY: defer only maximality terminals. All packet entries have the
        // same bit context; buffers belong to an individual DFS invocation.
        struct BATCH_MAXIMALITYMaxReady64 { uint64_t S=0,X=0; bool valid=false; };
        inline uint8_t batchMaximalityMaxCanAddRow64(const uint64_t row,const int miss,
              const __m256i Sv,const __m256i satV,const __m256i ssV,const uint8_t live) const {
            if(!live || miss>=k) return 0;
            const __m256i rv=_mm256_set1_epi64x(static_cast<long long>(row));
            const __m256i degree=avxPopcnt4x64(_mm256_and_si256(rv,Sv));
            const __m256i enough=_mm256_cmpgt_epi64(
                _mm256_add_epi64(degree,_mm256_set1_epi64x(k-miss)),ssV);
            const __m256i satOK=_mm256_cmpeq_epi64(
                _mm256_andnot_si256(rv,satV),_mm256_setzero_si256());
            return static_cast<uint8_t>(_mm256_movemask_pd(_mm256_castsi256_pd(
                _mm256_and_si256(enough,satOK)))) & live;
        }
        inline void batchMaximalityEmitChecked64(const uint64_t S){
#ifdef SET
            const int old=plex->sz;
            if(bitForcedCount) for(const int v:forcedBase)plex->push(v);
            for(int i=bitFixed;i<bitM;i++) if(S&(1ULL<<i))plex->push(bitOrig[i]);
            emitVerifiedPlex(plex);
            while(plex->sz>old)plex->pop();
#else
            emitter->emitPlex();
#endif
            ++bitStats.outputs;
            ++state64Stats.batch_maximality_emitted;
        }
        // LEFT_CONSTRAINT: 4 State64 lanes x 32 Left candidates per chunk, Any-K bitplanes.
        // Same scalar predicate: miss < k, degree(S)+k-miss >= |S|+1,
        // and each saturated S vertex must be adjacent to this Left row.
        // All forced-base missing edges are incorporated in initial defect plane.
        inline uint8_t leftConstraintLeft2DReject64(const __m256i Sv,const __m256i satV,
                                         const uint64_t unionS,const uint8_t inputMask){
            if(!inputMask || !leftSz) return 0;
            if(k<1 || k>ANYK_FAST_K_MAX){
                const __m256i ssV=avxPopcnt4x64(Sv);
                uint8_t live=inputMask;
                for(int l=0;l<leftSz && live;l++){
                    ++state64Stats.left_constraint_fallback_rows;
                    const int miss=bitForcedMissing(bitLeftForcedAdj[l]);
                    const uint8_t reject=batchMaximalityMaxCanAddRow64(bitLeftAdj[l],miss,Sv,satV,ssV,live);
                    live=static_cast<uint8_t>(live&~reject);
                }
                return static_cast<uint8_t>(inputMask&~live);
            }
            ensureLeftNon32Transpose();
            const __m256i zero=_mm256_setzero_si256();
            uint8_t pending=inputMask,rejectAll=0;
            for(int chunk=0;chunk<leftChunks32 && pending;chunk++){
                ++state64Stats.left_constraint_left_chunks;
                const int lo=chunk<<5,hi=std::min(leftSz,lo+32);
                uint32_t init[ANYK_FAST_K_MAX]={0};
                for(int l=lo;l<hi;l++){
                    const int miss=bitForcedMissing(bitLeftForcedAdj[l]);
                    if(miss<k) init[miss]|=1u<<(l-lo);
                }
                alignas(32) uint64_t liveArr[4]={0,0,0,0};
                for(int i=0;i<4;i++) if(pending&(1u<<i)) liveArr[i]=~0ULL;
                const __m256i laneV=_mm256_load_si256(reinterpret_cast<const __m256i*>(liveArr));
                __m256i d[ANYK_FAST_K_MAX];
                for(int t=0;t<k;t++) d[t]=_mm256_and_si256(_mm256_set1_epi64x(init[t]),laneV);
                uint64_t tu=unionS;
                while(tu){
                    const int u=bitFirst(tu);tu&=tu-1;
                    const __m256i bit=_mm256_set1_epi64x(static_cast<long long>(1ULL<<u));
                    const __m256i inS=_mm256_and_si256(laneV,
                        _mm256_cmpeq_epi64(_mm256_and_si256(Sv,bit),bit));
                    if(_mm256_testz_si256(inS,inS))continue;
                    ++state64Stats.left_constraint_vector_steps;
                    const __m256i inSat=_mm256_cmpeq_epi64(_mm256_and_si256(satV,bit),bit);
                    const __m256i non=_mm256_set1_epi64x(leftNon32At(u,chunk));
                    const __m256i satN=_mm256_and_si256(non,_mm256_and_si256(inS,inSat));
                    for(int t=0;t<k;t++) d[t]=_mm256_andnot_si256(satN,d[t]);
                    const __m256i step=_mm256_and_si256(non,_mm256_andnot_si256(inSat,inS));
                    for(int t=k-1;t>=1;t--)
                        d[t]=_mm256_or_si256(_mm256_andnot_si256(step,d[t]),
                                            _mm256_and_si256(d[t-1],step));
                    d[0]=_mm256_andnot_si256(step,d[0]);
                }
                __m256i any=zero;
                for(int t=0;t<k;t++)any=_mm256_or_si256(any,d[t]);
                const uint8_t reject=static_cast<uint8_t>(
                    _mm256_movemask_pd(_mm256_castsi256_pd(
                        _mm256_cmpgt_epi64(any,zero)))&pending);
                rejectAll=static_cast<uint8_t>(rejectAll|reject);
                pending=static_cast<uint8_t>(pending&~reject);
            }
            return rejectAll;
        }
        inline void batchMaximalityFlushMax64(const BATCH_MAXIMALITYMaxReady64 batch[4],const int active){
            if(!active)return;
            ++state64Stats.batch_maximality_batches;
            state64Stats.batch_maximality_slots+=active;
            if(active==4)++state64Stats.batch_maximality_full;
            alignas(32) uint64_t S[4]={0,0,0,0},X[4]={0,0,0,0};
            uint64_t unionS=0,unionX=0;
            for(int i=0;i<active;i++){
                assert(batch[i].valid);
                S[i]=batch[i].S;X[i]=batch[i].X;
                unionS|=S[i];unionX|=X[i];
            }
            const __m256i Sv=_mm256_load_si256(reinterpret_cast<const __m256i*>(S));
            const __m256i Xv=_mm256_load_si256(reinterpret_cast<const __m256i*>(X));
            const __m256i ssV=avxPopcnt4x64(Sv);
            const __m256i zero=_mm256_setzero_si256();
            const __m256i km1=_mm256_set1_epi64x(k-1);
            __m256i satV=zero;
            for(uint64_t t=unionS;t;t&=t-1){
                const int u=bitFirst(t);
                const __m256i bit=_mm256_set1_epi64x(static_cast<long long>(1ULL<<u));
                const __m256i inS=_mm256_cmpeq_epi64(_mm256_and_si256(Sv,bit),bit);
                const __m256i non=_mm256_set1_epi64x(static_cast<long long>(bitNon[u]));
                const __m256i sat=_mm256_cmpeq_epi64(avxPopcnt4x64(_mm256_and_si256(non,Sv)),km1);
                satV=_mm256_or_si256(satV,_mm256_and_si256(bit,_mm256_and_si256(inS,sat)));
            }
            uint8_t live=static_cast<uint8_t>((1u<<active)-1u);
            for(uint64_t t=unionX;t&&live;t&=t-1){
                const int x=bitFirst(t);
                ++state64Stats.batch_maximality_x_rows;
                const __m256i bit=_mm256_set1_epi64x(static_cast<long long>(1ULL<<x));
                const uint8_t inX=static_cast<uint8_t>(_mm256_movemask_pd(_mm256_castsi256_pd(
                    _mm256_cmpeq_epi64(_mm256_and_si256(Xv,bit),bit))));
                const uint8_t reject=batchMaximalityMaxCanAddRow64(bitAdj[x],0,Sv,satV,ssV,live)&inX;
                bitStats.x_rejects+=__builtin_popcount(static_cast<unsigned>(reject));
                live=static_cast<uint8_t>(live&~reject);
            }
            for(int e=0;e<bitExtN && live;e++){
                ++state64Stats.batch_maximality_ext_rows;
                const int miss=bitForcedMissing(bitExtForcedAdj[e]);
                const uint8_t reject=batchMaximalityMaxCanAddRow64(bitExtAdj[e],miss,Sv,satV,ssV,live);
                bitStats.x_rejects+=__builtin_popcount(static_cast<unsigned>(reject));
                live=static_cast<uint8_t>(live&~reject);
            }
            // X/External identical to BATCH_MAXIMALITY. LEFT_CONSTRAINT varies only Left.
            if(live && leftSz){
                ++state64Stats.left_constraint_left_invocations;
                const uint8_t reject=leftConstraintLeft2DReject64(Sv,satV,unionS,live);
                bitStats.left_rejects+=__builtin_popcount(static_cast<unsigned>(reject));
                state64Stats.left_constraint_left_rejects+=__builtin_popcount(static_cast<unsigned>(reject));
                live=static_cast<uint8_t>(live&~reject);
            }
            for(int i=0;i<active;i++) if(live&(1u<<i))batchMaximalityEmitChecked64(S[i]);
        }

        // Prepare a >32 State64 search state for the 4-lane packet.  Expensive pivot
        // selection and include refinement are deliberately deferred until the
        // packet is formed so they can run across four same-context states.
        inline bool prepare4x64ReadyState(LaneState64& st,BATCH_MAXIMALITYMaxReady64* terminalOut=nullptr){
            ++bitStats.nodes;
            ++state64Stats.prepared_states;
            if(enableLocalFolding && bitForcedCount==0 && st.C){
                const int live=bitPop(st.P|st.C);
                if(folding_variant::scan(entryFoldingPolicy,live)){
                    if(live>40)++experiment_control::samples().wide_fold_scans;
                    ++state64Stats.eager_folding_eligible;
                    const uint64_t check=(st.rootNode || !st.eagerInitialized)?st.C:st.checkMask;
                    st.eagerInitialized=true;
                    ++state64Stats.eager_folding_scans;
                    uint64_t folded=0,remaining=st.C&check;
                    while(remaining){
                        const int v=bitFirst(remaining);remaining&=remaining-1;
                        ++state64Stats.eager_folding_tested;
                        if(!(bitNon[v]&(st.P|st.C)))folded|=1ULL<<v;
                    }
                    if(folded){
                        st.C&=~folded;
                        st.F|=folded;
                        ++state64Stats.eager_folding_fold_nodes;
                        state64Stats.eager_folding_folded_vertices+=bitPop(folded);
                        if(live>40){++experiment_control::samples().wide_fold_nodes;experiment_control::samples().wide_fold_vertices+=bitPop(folded);}
                    }
                }
            }

            const int activeLb=eagerFoldingActiveLb64(st);
            const bool ready=statebit_word64_search2::prepare_node(st,st,activeLb,
                [&](uint64_t bits)->int{return bitPop(bits);},
                [&](LaneState64& node,uint64_t U)->bool {
                bool whole=true;
                const auto wr=state64_common::whole_witness(U,k,bitNon,node.wholeWitness);
                if(wr.tested_hint)++state64Stats.whole_witness_tests;
                if(wr.hit_hint)++state64Stats.whole_witness_hits;
                if(wr.full_scan)++state64Stats.whole_full_scans;
                state64Stats.whole_scan_vertices+=wr.scan_vertices;
                whole=wr.whole;
                node.wholeWitness=wr.next_witness;
                    return whole;
                },
                [&](LaneState64& st,uint64_t U){
                    ++bitStats.whole_calls;
                    if(terminalOut) *terminalOut={U|st.F,st.X,true};
                    else bitIsMaximalAndEmit(U|st.F,st.X);
                    ++state64Stats.terminal_states;
                },
                [&](LaneState64& st,bool eligible){
                    ++bitStats.leaf_calls;
                    if(eligible){
                        if(terminalOut) *terminalOut={st.P|st.F,st.X,true};
                        else bitIsMaximalAndEmit(st.P|st.F,st.X);
                    }
                    ++state64Stats.terminal_states;
                },
                [&](LaneState64& st)->bool {
                const uint64_t U=st.P|st.C;
                const bool eagerFoldingHandoff32=(bitPop(U)<=32 && (st.F==0 || bitForcedCount==0 || enableMixedFixedContext));
                if(!eagerFoldingHandoff32)return false;
                {
                    if(bitForcedCount>0) forcedBaseBit64Stats.observeState32Transition();
                    ++state64Stats.state32_handoffs;
                    if(st.F){
                        ++state64Stats.eager_folding_localf_state32;
                        avxStageBatchedDFSLocalF(st.P,st.C,st.X,st.F);
                    }else
                    {
                        // PARALLEL_ENGINE State64 has already checked root feasibility.
                        // Do not double count its root node on handoff.
                        statebit_state32_handoff::dispatch({st.P|st.F,st.C,st.X,bitActiveLb(),k,true},
                            statebit_state32_handoff::RootMode::AlreadyChecked,bitNon,
                            [](){},
                            [&](uint64_t U){bitIsMaximalAndEmit(U,st.X);},
                            [&](uint64_t leaf){
                                ++bitStats.leaf_calls;
                                if(bitPop(leaf)>=bitActiveLb())bitIsMaximalAndEmit(leaf,st.X);
                            },
                            [&](bool checked){
    #if AVX_STAGE_REFILL
                                avxStageBatchedDFS(st.P|st.F,st.C,st.X,checked);
    #else
                                avx2PacketDFS(st.P|st.F,st.C,st.X,checked);
    #endif
                            });
                    }
                    ++state64Stats.terminal_states;
                    return true;
                }
                },
                [&](LaneState64& st,LaneState64&){
                    if(bitForcedCount>0)forcedBaseBit64Stats.observeState64Node();
                    if(enableFoldingProbe)c0Opportunity.observe(64,bitPop(st.P|st.C),bitPop(st.C),bitForcedCount,bitPop(st.F),[&](){
                        return c0Count(st.C,st.P|st.C,bitNon,[](uint64_t x){return x!=0;},[&](uint64_t x){return bitFirst(x);},[](uint64_t x,int v){return x&~(1ULL<<v);});
                    });
                },
                [&](LaneState64&){++state64Stats.terminal_states;});
            if(!ready||!enableMixedFixedContext||!enableLocalFolding||!bitForcedCount)return ready;
            auto& m=mixed_f64::stats();++m.ready;
            if(st.F)++m.f_states;
            const int live=bitPop(st.P|st.C);
            if(live<=32||live>40){++m.width_skip;return true;}
            if(!mixed_f64::pivotBound(st.P,st.P|st.C,k,eagerFoldingActiveLb64(st),bitAdj)){++m.degree_skip;return true;}
            uint64_t remaining=mixed_f64::checks(st.C,st.checkMask,st.rootNode,st.eagerInitialized);
            st.eagerInitialized=true;
            if(!remaining){++m.empty_check_skip;return true;}
            ++m.scans;++state64Stats.eager_folding_eligible;++state64Stats.eager_folding_scans;
            uint64_t folded=0;
            {
                mixed_f64::Span span(m.scans,m.scan_samples,m.scan_sample_ns);
                while(remaining){const int v=bitFirst(remaining);remaining&=remaining-1;++m.tested;++state64Stats.eager_folding_tested;
                    if(!(bitNon[v]&(st.P|st.C)))folded|=1ULL<<v;
                }
            }
            if(!folded){++m.zero;return true;}
            ++m.fold_nodes;m.fold_vertices+=bitPop(folded);
            ++state64Stats.eager_folding_fold_nodes;state64Stats.eager_folding_folded_vertices+=bitPop(folded);
            st.C&=~folded;st.F|=folded;
            assert(!(st.F&(st.P|st.C|st.X)));
            if(bitPop(st.P|st.C)>32){++m.partial;return true;}
            ++m.to32;++state64Stats.state32_handoffs;
            forcedBaseBit64Stats.observeState32Transition();
            ++state64Stats.eager_folding_localf_state32;
            avxStageBatchedDFSLocalF(st.P,st.C,st.X,st.F);
            ++state64Stats.terminal_states;
            return false;
        }

        inline uint8_t avxChoosePivot4x64(const LaneState64 ready[4],const int active,int pivot[4]) const {
            uint64_t sharedP[4]={0,0,0,0},sharedC[4]={0,0,0,0};
            int sharedLB[4]={0,0,0,0};
            for(int lane=0;lane<active;lane++){
                sharedP[lane]=ready[lane].P;
                sharedC[lane]=ready[lane].C;
                sharedLB[lane]=eagerFoldingActiveLb64(ready[lane]);
            }
            return state64_common::choose_pivot(sharedP,sharedC,sharedLB,active,k,bitAdj,bitNon,pivot,
                [this](__m256i x){return avxPopcnt4x64(x);});
        }

        inline void avxRefineInclude4x64(const LaneState64 ready[4],const int pivot[4],const uint64_t Pinc[4],const uint64_t Cminus[4],const uint8_t laneMask,uint64_t Cout[4]){
            ++state64Stats.defect_plane_refine_packets; ++state64Stats.defect_plane_2d_packets;
            uint64_t lbs[4]={0,0,0,0};
            for(int i=0;i<4;i++) if(laneMask&(1u<<i))lbs[i]=static_cast<uint64_t>(eagerFoldingActiveLb64(ready[i]));
            state64_common::RefineMetrics m;
            state64_common::refine_2d_bulk4(pivot,Pinc,Cminus,laneMask,lbs,k,
                bitPair,bitNon,bitAdj,Cout,
                [this](__m256i x){return avxPopcnt4x64(x);},
                [this](__m256i a,__m256i b){return avxMax4x64(a,b);},m);
            state64Stats.defect_plane_defect_steps+=m.defect_steps;
            state64Stats.bulk_refine_static_removed+=m.static_removed;
            state64Stats.bulk_refine_degree_steps+=m.degree_steps;
            state64Stats.bulk_refine_degree_removed+=m.degree_removed;
            bitStats.cand_filter_calls+=__builtin_popcount(static_cast<unsigned>(laneMask));
            for(int i=0;i<4;i++)if(laneMask&(1u<<i))bitStats.cand_filter_removed+=bitPop(Cminus[i])-bitPop(Cout[i]);
        }

        template<class StackT>
        inline void branch4x64Packet(const LaneState64 ready[4],const int active,StackT& stack){
            if(active<=0) return;
            int piv[4]={-2,-2,-2,-2};
            const uint8_t branchMask=avxChoosePivot4x64(ready,active,piv);
            const int branchN=__builtin_popcount(static_cast<unsigned>(branchMask));
            if(branchN<=0){
                state64Stats.terminal_states+=static_cast<unsigned long long>(active);
                return;
            }
            state64Stats.observePacket(branchN);
            state64Stats.branch_states+=static_cast<unsigned long long>(branchN);
            state64Stats.terminal_states+=static_cast<unsigned long long>(active-branchN);
            const auto common=statebit_state64_search::materialize(ready,active,branchMask,piv);
            const uint64_t *P=common.P,*C=common.C,*X=common.X,*B=common.bit;
            const uint64_t *Pinc=common.pi,*Cminus=common.cm,*Xexc=common.xe;
            alignas(32) uint64_t Cinc[4]={0,0,0,0};
            avxRefineInclude4x64(ready,piv,Pinc,Cminus,branchMask,Cinc);
            statebit_state64_search::publish_children(active,branchMask,stack,
                [](int){return true;},
                [&](int i,bool include,StackT& stk){
                    if(!include){
                        ++bitStats.exclude_calls;
                        stk.push_back(statebit_word64_search2::make_child(ready[i],P[i],Cminus[i],Xexc[i],B[i],
                            [&](uint64_t changed,uint64_t cand){return eagerFoldingAffected64(changed,cand);}));
                    }else{
                        ++bitStats.include_calls;
                        const uint64_t removed=(P[i]|C[i])&~(Pinc[i]|Cinc[i]);
                        stk.push_back(statebit_word64_search2::make_child(ready[i],Pinc[i],Cinc[i],X[i],removed,
                            [&](uint64_t changed,uint64_t cand){return eagerFoldingAffected64(changed,cand);}));
                    }
                    ++state64Stats.generated_children;
                });
        }

