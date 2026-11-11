// ==== shared avxRefineIncludeEight ====
        void avxRefineIncludeEight(const int pivot[8],const uint32_t P[8],const uint32_t Cin[8],
                               const uint8_t branchMask,const LaneContext32& ctx,uint32_t Cout[8]){
            AVX_COUNT(++avxStats.vector_refine_calls);
            alignas(32) int piv[8];
            for(int i=0;i<8;i++) piv[i]=(branchMask&(1u<<i))?pivot[i]:0;
            const __m256i pivV=_mm256_load_si256(reinterpret_cast<const __m256i*>(piv));
            const __m256i pairV=_mm256_i32gather_epi32(reinterpret_cast<const int*>(ctx.pair),pivV,4);
            __m256i Pv=_mm256_loadu_si256(reinterpret_cast<const __m256i*>(P));
            __m256i Cv=_mm256_loadu_si256(reinterpret_cast<const __m256i*>(Cin));
            __m256i Uv=_mm256_or_si256(Pv,Cv);
            const __m256i activeV=avxLaneMaskVec(branchMask);
            const __m256i zero=_mm256_setzero_si256();
            __m256i satV=zero;
            for(int u=0;u<ctx.m;u++){
                const __m256i bit=_mm256_set1_epi32(static_cast<int>(1u<<u));
                const __m256i inP=_mm256_cmpeq_epi32(_mm256_and_si256(Pv,bit),bit);
                const __m256i non=_mm256_set1_epi32(static_cast<int>(ctx.nonadj[u]));
                const __m256i cnt=avxPopcnt8x32(_mm256_and_si256(non,Pv));
                const __m256i eq=_mm256_cmpeq_epi32(cnt,_mm256_set1_epi32(k-1));
                const __m256i take=_mm256_and_si256(activeV,_mm256_and_si256(inP,eq));
                satV=_mm256_or_si256(satV,_mm256_and_si256(take,bit));
            }
            const __m256i psV=avxPopcnt8x32(Pv);
            const __m256i limV=_mm256_max_epi32(psV,_mm256_set1_epi32(state32ContextActiveLb(ctx)));
            for(int x=0;x<ctx.m;x++){
                const __m256i bit=_mm256_set1_epi32(static_cast<int>(1u<<x));
                const __m256i inC=_mm256_and_si256(activeV,_mm256_cmpeq_epi32(_mm256_and_si256(Cv,bit),bit));
                const __m256i pairOk=_mm256_cmpeq_epi32(_mm256_and_si256(pairV,bit),bit);
                const __m256i badPair=_mm256_xor_si256(pairOk,_mm256_set1_epi32(-1));
                const __m256i non=_mm256_set1_epi32(static_cast<int>(ctx.nonadj[x]));
                const __m256i noCnt=avxPopcnt8x32(_mm256_and_si256(non,Pv));
                const __m256i badNo=_mm256_cmpgt_epi32(noCnt,_mm256_set1_epi32(k-1));
                const __m256i satHit=_mm256_and_si256(non,satV);
                const __m256i badSat=_mm256_xor_si256(_mm256_cmpeq_epi32(satHit,zero),_mm256_set1_epi32(-1));
                const __m256i row=_mm256_set1_epi32(static_cast<int>(ctx.adj[x]));
                const __m256i dg=avxPopcnt8x32(_mm256_and_si256(row,Uv));
                const __m256i dgk=_mm256_add_epi32(dg,_mm256_set1_epi32(k));
                const __m256i badDeg=_mm256_cmpgt_epi32(limV,dgk);
                const __m256i erase=_mm256_and_si256(inC,_mm256_or_si256(_mm256_or_si256(badPair,badNo),_mm256_or_si256(badSat,badDeg)));
                const __m256i clearedC=_mm256_andnot_si256(bit,Cv);
                const __m256i clearedU=_mm256_andnot_si256(bit,Uv);
                Cv=_mm256_blendv_epi8(Cv,clearedC,erase);
                Uv=_mm256_blendv_epi8(Uv,clearedU,erase);
            }
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(Cout),Cv);
            const int calls=__builtin_popcount(static_cast<unsigned>(branchMask));
            bitStats.cand_filter_calls+=calls;
            unsigned long long removed=0;
            for(int i=0;i<8;i++) if(branchMask&(1u<<i)) removed+=lanePop(Cin[i])-lanePop(Cout[i]);
            bitStats.cand_filter_removed+=removed;
        }

// ==== shared avxPrepareBranchState ====
        bool avxPrepareBranchState(LaneState32 st,const LaneContext32& ctx,LaneState32& ready){
            AVX_COUNT(++avxStats.states);
            if(st.prechecked){
                st.prechecked=false;
                ready=st;
                return true;
            }
            ++bitStats.nodes;
            AVX_COUNT(++avxStats.terminal_states);
            if(lanePop(st.P)+lanePop(st.C)<state32ContextActiveLb(ctx)) return false;
            const uint32_t U=st.P|st.C;
            bool whole=true;
            uint32_t t=U;
            while(t){
                const int u=laneFirst(t); t&=t-1;
                if(lanePop(ctx.nonadj[u]&U)>=k){ whole=false; break; }
            }
            if(whole){
                ++bitStats.whole_calls;
                laneIsMaximalAndEmit(U,st.X,ctx);
                return false;
            }
            if(!st.C){
                ++bitStats.leaf_calls;
                if(lanePop(st.P)>=state32ContextActiveLb(ctx)) laneIsMaximalAndEmit(st.P,st.X,ctx);
                return false;
            }
            ready=st;
            return true;
        }

// ==== shared avxBuildDefectMasksT ====
template<int K>
        inline void avxBuildDefectMasksT(const __m256i Pv,const __m256i activeV,const uint32_t unionP,
                                          const LaneContext32& ctx,__m256i& badNoV,__m256i& satV){
            static_assert(K>=1 && K<=ANYK_FAST_K_MAX,"fast Any-K defect K out of range");
            ANYK_COUNT(++anyKStats.defect_template_calls);
            const __m256i zero=_mm256_setzero_si256();
            const __m256i allOnes=_mm256_set1_epi32(-1);
            __m256i d[K+1];
            for(int t=0;t<=K;t++) d[t]=zero;
            uint32_t tp=unionP;
            while(tp){
                const int u=laneFirst(tp); tp&=tp-1;
                const __m256i bit=_mm256_set1_epi32(static_cast<int>(1u<<u));
                const __m256i inP=_mm256_and_si256(activeV,_mm256_cmpeq_epi32(_mm256_and_si256(Pv,bit),bit));
                const __m256i non=_mm256_set1_epi32(static_cast<int>(ctx.nonadj[u]));
                const __m256i hit=_mm256_and_si256(non,inP);
                for(int t=K;t>=2;t--) d[t]=_mm256_or_si256(d[t],_mm256_and_si256(d[t-1],hit));
                d[1]=_mm256_or_si256(d[1],hit);
            }
            badNoV=d[K];
            if(K==1){
                satV=_mm256_and_si256(activeV,_mm256_and_si256(Pv,_mm256_andnot_si256(d[1],allOnes)));
            }else{
                satV=_mm256_and_si256(activeV,_mm256_and_si256(Pv,
                    _mm256_and_si256(d[K-1],_mm256_andnot_si256(d[K],allOnes))));
            }
        }

// ==== shared avxBuildDefectMasksDynamic ====
        inline void avxBuildDefectMasksDynamic(const __m256i Pv,const __m256i activeV,const uint32_t unionP,
                                                const LaneContext32& ctx,__m256i& badNoV,__m256i& satV){
            const __m256i zero=_mm256_setzero_si256();
            const __m256i allOnes=_mm256_set1_epi32(-1);
            if(k>32){
                ANYK_COUNT(++anyKStats.defect_trivial_calls);
                badNoV=zero; satV=zero; return;
            }
            ANYK_COUNT(++anyKStats.defect_dynamic_calls);
            __m256i d[33];
            for(int t=0;t<=k;t++) d[t]=zero;
            uint32_t tp=unionP;
            while(tp){
                const int u=laneFirst(tp); tp&=tp-1;
                const __m256i bit=_mm256_set1_epi32(static_cast<int>(1u<<u));
                const __m256i inP=_mm256_and_si256(activeV,_mm256_cmpeq_epi32(_mm256_and_si256(Pv,bit),bit));
                const __m256i non=_mm256_set1_epi32(static_cast<int>(ctx.nonadj[u]));
                const __m256i hit=_mm256_and_si256(non,inP);
                for(int t=k;t>=2;t--) d[t]=_mm256_or_si256(d[t],_mm256_and_si256(d[t-1],hit));
                d[1]=_mm256_or_si256(d[1],hit);
            }
            badNoV=d[k];
            if(k==1) satV=_mm256_and_si256(activeV,_mm256_and_si256(Pv,_mm256_andnot_si256(d[1],allOnes)));
            else satV=_mm256_and_si256(activeV,_mm256_and_si256(Pv,
                _mm256_and_si256(d[k-1],_mm256_andnot_si256(d[k],allOnes))));
        }

// ==== shared avxRefineIncludeEightUnionSparse ====
        void avxRefineIncludeEightUnionSparse(const int pivot[8],const uint32_t P[8],const uint32_t Cin[8],
                                          const uint8_t branchMask,const LaneContext32& ctx,uint32_t Cout[8]){
            ANYK_COUNT(++anyKStats.refine_calls);
            ANYK_COUNT(++anyKStats.refine_anyk_calls);
            ANYK_COUNT(anyKStats.refine_slots += __builtin_popcount(static_cast<unsigned>(branchMask)));
            AVX_COUNT(++avxStats.vector_refine_calls);
            REFINE2D_COUNT(++refine2DStats.calls_2d);
            REFINE2D_COUNT(refine2DStats.refine_slots += __builtin_popcount(static_cast<unsigned>(branchMask)));

            const __m256i Pv=_mm256_loadu_si256(reinterpret_cast<const __m256i*>(P));
            __m256i Cv=_mm256_loadu_si256(reinterpret_cast<const __m256i*>(Cin));
            __m256i Uv=_mm256_or_si256(Pv,Cv);
            const __m256i activeV=avxLaneMaskVec(branchMask);
            const __m256i zero=_mm256_setzero_si256();
            const __m256i allOnes=_mm256_set1_epi32(-1);

            alignas(32) int piv[8];
            for(int i=0;i<8;i++) piv[i]=(branchMask&(1u<<i))?pivot[i]:0;
            const __m256i pivV=_mm256_load_si256(reinterpret_cast<const __m256i*>(piv));
            const __m256i pairV=_mm256_i32gather_epi32(reinterpret_cast<const int*>(ctx.pair),pivV,4);

            uint32_t unionP=0;
            for(int i=0;i<8;i++) if(branchMask&(1u<<i)) unionP|=P[i];
            AVX_COUNT(avxStats.refine_union_vertices += lanePop(unionP));
            REFINE2D_COUNT(refine2DStats.defect_vector_steps += lanePop(unionP));
            REFINE2D_COUNT({ for(int i=0;i<8;i++) if(branchMask&(1u<<i)) refine2DStats.defect_lane_vertex_steps += lanePop(P[i]); });
            ANYK_COUNT(anyKStats.defect_vector_steps += lanePop(unionP));

            __m256i badNoV=zero,satV=zero;
            dispatchAnyKDefectMasks(Pv,activeV,unionP,ctx,badNoV,satV);

            alignas(32) uint32_t satLane[8];
            _mm256_store_si256(reinterpret_cast<__m256i*>(satLane),satV);
            uint32_t unionSat=0;
            for(int i=0;i<8;i++) if(branchMask&(1u<<i)) unionSat|=satLane[i];
            REFINE2D_COUNT(refine2DStats.sat_union_vertices += lanePop(unionSat));
            __m256i badSatV=zero;
            uint32_t ts=unionSat;
            while(ts){
                const int u=laneFirst(ts); ts&=ts-1;
                const __m256i bit=_mm256_set1_epi32(static_cast<int>(1u<<u));
                const __m256i inSat=_mm256_and_si256(activeV,_mm256_cmpeq_epi32(_mm256_and_si256(satV,bit),bit));
                const __m256i non=_mm256_set1_epi32(static_cast<int>(ctx.nonadj[u]));
                badSatV=_mm256_or_si256(badSatV,_mm256_and_si256(non,inSat));
            }

            const __m256i badPairV=_mm256_xor_si256(pairV,allOnes);
            const __m256i staticBadV=_mm256_or_si256(badPairV,_mm256_or_si256(badNoV,badSatV));
            const __m256i psV=avxPopcnt8x32(Pv);
            const __m256i limV=_mm256_max_epi32(psV,_mm256_set1_epi32(state32ContextActiveLb(ctx)));

            uint32_t unionC=0;
            for(int i=0;i<8;i++) if(branchMask&(1u<<i)) unionC|=Cin[i];
            AVX_COUNT(avxStats.refine_union_vertices += lanePop(unionC));
            REFINE2D_COUNT(refine2DStats.union_c_vertices += lanePop(unionC));

#if REFINE_2D_MODE == 1
            uint32_t tc=unionC;
            while(tc){
                const int x=laneFirst(tc); tc&=tc-1;
                const __m256i bit=_mm256_set1_epi32(static_cast<int>(1u<<x));
                const __m256i inC=_mm256_and_si256(activeV,_mm256_cmpeq_epi32(_mm256_and_si256(Cv,bit),bit));
                const unsigned inCMask=static_cast<unsigned>(_mm256_movemask_ps(_mm256_castsi256_ps(inC)));
                if(!inCMask) continue;
                const __m256i badStatic=_mm256_cmpeq_epi32(_mm256_and_si256(staticBadV,bit),bit);
                const __m256i staticErase=_mm256_and_si256(inC,badStatic);
                const __m256i needDeg=_mm256_andnot_si256(badStatic,inC);
                const unsigned needMask=static_cast<unsigned>(_mm256_movemask_ps(_mm256_castsi256_ps(needDeg)));
                __m256i degErase=zero;
                if(needMask){
                    REFINE2D_COUNT(++refine2DStats.degree_vector_steps);
                    REFINE2D_COUNT(refine2DStats.degree_candidate_slots += __builtin_popcount(needMask));
                    const __m256i row=_mm256_set1_epi32(static_cast<int>(ctx.adj[x]));
                    const __m256i dg=avxPopcnt8x32(_mm256_and_si256(row,Uv));
                    const __m256i dgk=_mm256_add_epi32(dg,_mm256_set1_epi32(k));
                    const __m256i badDeg=_mm256_cmpgt_epi32(limV,dgk);
                    degErase=_mm256_and_si256(needDeg,badDeg);
                    REFINE2D_COUNT(refine2DStats.degree_removed_slots += __builtin_popcount(static_cast<unsigned>(_mm256_movemask_ps(_mm256_castsi256_ps(degErase)))));
                }else REFINE2D_COUNT(++refine2DStats.all_static_skip_vertices);
                REFINE2D_COUNT(refine2DStats.static_erased_slots_runtime += __builtin_popcount(static_cast<unsigned>(_mm256_movemask_ps(_mm256_castsi256_ps(staticErase)))));
                const __m256i erase=_mm256_or_si256(staticErase,degErase);
                Cv=_mm256_blendv_epi8(Cv,_mm256_andnot_si256(bit,Cv),erase);
                Uv=_mm256_blendv_epi8(Uv,_mm256_andnot_si256(bit,Uv),erase);
            }
#elif REFINE_2D_MODE == 2
            const __m256i oldCv=Cv;
            Cv=_mm256_andnot_si256(staticBadV,Cv);
            Uv=_mm256_or_si256(Pv,Cv);
            alignas(32) uint32_t liveC[8];
            _mm256_store_si256(reinterpret_cast<__m256i*>(liveC),Cv);
            uint32_t unionLiveC=0;
            for(int i=0;i<8;i++) if(branchMask&(1u<<i)) unionLiveC|=liveC[i];
            REFINE2D_COUNT(refine2DStats.bulk_union_c_after += lanePop(unionLiveC));
            REFINE2D_COUNT(refine2DStats.all_static_skip_vertices += lanePop(unionC&~unionLiveC));

            DEGPEEL_COUNT(++degreePeelStats.calls);
            DEGPEEL_COUNT(degreePeelStats.initial_union_vertices += lanePop(unionLiveC));
            DEGPEEL_COUNT({ for(int i=0;i<8;i++) if(branchMask&(1u<<i)) degreePeelStats.initial_candidate_slots += lanePop(liveC[i]); });
            ANYK_COUNT(++anyKStats.peel_calls);
            if(unionLiveC){
                DEGPEEL_COUNT(++degreePeelStats.scan_calls);
                __m256i eraseBits=_mm256_setzero_si256();
                uint32_t tc=unionLiveC;
                while(tc){
                    const int x=laneFirst(tc); tc&=tc-1;
                    const __m256i bit=_mm256_set1_epi32(static_cast<int>(1u<<x));
                    const __m256i inC=_mm256_and_si256(activeV,_mm256_cmpeq_epi32(_mm256_and_si256(Cv,bit),bit));
                    const unsigned inCMask=static_cast<unsigned>(_mm256_movemask_ps(_mm256_castsi256_ps(inC)));
                    if(!inCMask) continue;
                    DEGPEEL_COUNT(++degreePeelStats.scan_vector_steps);
                    DEGPEEL_COUNT(degreePeelStats.scan_candidate_slots += __builtin_popcount(inCMask));
                    REFINE2D_COUNT(++refine2DStats.degree_vector_steps);
                    REFINE2D_COUNT(refine2DStats.degree_candidate_slots += __builtin_popcount(inCMask));
                    const __m256i row=_mm256_set1_epi32(static_cast<int>(ctx.adj[x]));
                    const __m256i dg=avxPopcnt8x32(_mm256_and_si256(row,Uv));
                    const __m256i dgk=_mm256_add_epi32(dg,_mm256_set1_epi32(k));
                    const __m256i badDeg=_mm256_cmpgt_epi32(limV,dgk);
                    const __m256i eraseLane=_mm256_and_si256(inC,badDeg);
                    eraseBits=_mm256_or_si256(eraseBits,_mm256_and_si256(eraseLane,bit));
                }
                if(!_mm256_testz_si256(eraseBits,eraseBits)){
                    Cv=_mm256_andnot_si256(eraseBits,Cv);
                    Uv=_mm256_or_si256(Pv,Cv);
                }
            }
#else
#error unsupported REFINE_2D_MODE
#endif
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(Cout),Cv);
            const int calls=__builtin_popcount(static_cast<unsigned>(branchMask));
            bitStats.cand_filter_calls+=calls;
            unsigned long long removed=0;
            for(int i=0;i<8;i++) if(branchMask&(1u<<i)) removed+=lanePop(Cin[i])-lanePop(Cout[i]);
            bitStats.cand_filter_removed+=removed;
        }

