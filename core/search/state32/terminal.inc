// ==== shared avxPrepareBranchOrMaxState ====
        inline int avxPrepareBranchOrMaxState(LaneState32 st,const LaneContext32& ctx,LaneState32& ready,MaxReadyState32& maxReady){
            AVX_COUNT(++avxStats.states);
            if(st.prechecked){ st.prechecked=false; ready=st; return 1; }
            ++bitStats.nodes;
            AVX_COUNT(++avxStats.terminal_states);
            if(lanePop(st.P)+lanePop(st.C)<state32ContextActiveLb(ctx)) return 0;
            const uint32_t U=st.P|st.C;
            WITNESS_COUNT(++wholeWitnessStats.whole_checks);

            bool whole=true;
            int foundWitness=-1;
            const uint8_t inherited=st.wholeWitness;
            if(inherited<32){
                WITNESS_COUNT(++wholeWitnessStats.witness_tests);
                const uint32_t wb=1u<<inherited;
                if((U&wb)==0){
                    WITNESS_COUNT(++wholeWitnessStats.witness_misses);
                    WITNESS_COUNT(++wholeWitnessStats.witness_removed);
                }else if(lanePop(ctx.nonadj[inherited]&U)>=k){
                    whole=false;
                    foundWitness=inherited;
                    WITNESS_COUNT(++wholeWitnessStats.witness_hits);
                }else{
                    WITNESS_COUNT(++wholeWitnessStats.witness_misses);
                    WITNESS_COUNT(++wholeWitnessStats.witness_defect_recovered);
                }
            }

            // Only when the inherited certificate is absent or has stopped
            // proving a violation do we perform the original complete scan.
            if(whole){
                WITNESS_COUNT(++wholeWitnessStats.full_scans);
                uint32_t t=U;
                while(t){
                    const int u=laneFirst(t); t&=t-1;
                    WITNESS_COUNT(++wholeWitnessStats.full_scan_vertices);
                    if(lanePop(ctx.nonadj[u]&U)>=k){ whole=false; foundWitness=u; break; }
                }
                if(whole){
                    WITNESS_COUNT(++wholeWitnessStats.full_scan_whole);
                }else{
                    WITNESS_COUNT(++wholeWitnessStats.full_scan_finds);
                }
            }
            if(whole){ ++bitStats.whole_calls; maxReady={U,st.X}; return 2; }
            if(!st.C){
                ++bitStats.leaf_calls;
                if(lanePop(st.P)>=state32ContextActiveLb(ctx)){ maxReady={st.P,st.X}; return 2; }
                return 0;
            }
            st.wholeWitness=static_cast<uint8_t>(foundWitness);
            ready=st;
            return 1;
        }

// ==== shared avxFinishLocalMaxSuffix8 ====
        inline uint8_t avxFinishLocalMaxSuffix8(const __m256i Sv,const __m256i satV,const __m256i ssV,
                                                 uint8_t liveMask,const uint32_t unionS,const LaneContext32& ctx,
                                                 const LocalMaxSuffixCache32& cache){
            AVX_COUNT(++maxSimdStats.suffix_batches);
            AVX_COUNT(maxSimdStats.suffix_slots += __builtin_popcount(static_cast<unsigned>(liveMask)));
            // baseX: vertices excluded before the <=32 handoff, but still in
            // the original Word64 universe.  Evaluate their adjacency directly
            // in the local coordinate system; no per-state PDEP expansion.
            uint64_t bx=ctx.baseX;
            while(bx && liveMask){
                const int x=bitFirst(bx); bx&=bx-1;
                AVX_COUNT(++maxSimdStats.suffix_basex_rows);
                const uint8_t reject=avxCanAddLocalRowMask(localSuffixBaseRow(x,ctx,cache),Sv,satV,ssV,liveMask);
                if(reject){
                    const int nr=__builtin_popcount(static_cast<unsigned>(reject));
                    bitStats.x_rejects += nr;
                    AVX_COUNT(maxSimdStats.basex_rejects += nr);
                    liveMask=static_cast<uint8_t>(liveMask & ~reject);
                }
            }
            for(int e=0;e<bitExtN && liveMask;e++){
                AVX_COUNT(++maxSimdStats.suffix_external_rows);
                const uint8_t reject=avxCanAddLocalRowMask(localSuffixExternalRow(e,ctx,cache),Sv,satV,ssV,liveMask,
                    defectCacheExtMissing(e));
                if(reject){
                    const int nr=__builtin_popcount(static_cast<unsigned>(reject));
                    bitStats.x_rejects += nr;
                    AVX_COUNT(maxSimdStats.external_rejects += nr);
                    liveMask=static_cast<uint8_t>(liveMask & ~reject);
                }
            }
            // Same FORCED_LEFT scalar/SIMD policy as PARALLEL_ENGINE, for list-level ForcedBase.
            if(liveMask && leftSz && !!state32CoreHasForcedBase()){
                uint8_t reject=0;
                if(k<=ANYK_FAST_K_MAX){
                    reject=state32CoreForcedLeftReject(Sv,satV,liveMask,unionS,ctx);
                } else
                {
                    reject=forced_left_forced_left2d::evaluate_scalar(leftSz,liveMask,
                        [&](const int l,const uint8_t pending)->uint8_t{
                            return avxCanAddLocalRowMask(localSuffixLeftRow(l,ctx,cache),Sv,
                                satV,ssV,pending,defectCacheLeftMissing(l));
                        });
                }
                if(reject){
                    const int nr=__builtin_popcount(static_cast<unsigned>(reject));
                    bitStats.left_rejects+=nr;
                    AVX_COUNT(maxSimdStats.left_rejects+=nr);
                    liveMask=static_cast<uint8_t>(liveMask&~reject);
                }
            } else
#if AVX_LEFT_2D_DEFECT
            if(!state32CoreHasForcedBase() && liveMask && leftSz){
                const uint8_t reject=dispatchAnyKLeft2D(Sv,satV,liveMask,unionS,ctx);
                if(reject){
                    const int nr=__builtin_popcount(static_cast<unsigned>(reject));
                    bitStats.left_rejects += nr;
                    AVX_COUNT(maxSimdStats.left_rejects += nr);
                    liveMask=static_cast<uint8_t>(liveMask & ~reject);
                }
            } else
#endif
            {
                for(int l=0;l<leftSz && liveMask;l++){
                    AVX_COUNT(++maxSimdStats.suffix_left_rows);
                    const uint8_t reject=avxCanAddLocalRowMask(localSuffixLeftRow(l,ctx,cache),Sv,satV,ssV,liveMask,
                        defectCacheLeftMissing(l));
                    if(reject){
                        const int nr=__builtin_popcount(static_cast<unsigned>(reject));
                        bitStats.left_rejects += nr;
                        AVX_COUNT(maxSimdStats.left_rejects += nr);
                        liveMask=static_cast<uint8_t>(liveMask & ~reject);
                    }
                }
            }
            return liveMask;
        }

// ==== shared avxFlushLocalMaxBatch ====
        inline void avxFlushLocalMaxBatch(const MaxReadyState32 batch[8],const int active,const LaneContext32& ctx,
                                              const LocalMaxSuffixCache32& suffixCache){
            if(active<=0) return;
            AVX_COUNT(++maxSimdStats.max_batches);
            AVX_COUNT(maxSimdStats.max_batch_slots += active);
            AVX_COUNT(maxSimdStats.maximality_calls += active);
            if(active==8) AVX_COUNT(++maxSimdStats.max_full_batches);
            alignas(32) uint32_t S[8]={0},X[8]={0};
            uint32_t unionS=0,unionX=0;
            for(int i=0;i<active;i++){ S[i]=batch[i].S; X[i]=batch[i].X; unionS|=S[i]; unionX|=X[i]; }
            AVX_COUNT(maxSimdStats.sat_union_vertices += lanePop(unionS));
            AVX_COUNT(maxSimdStats.x_union_vertices += lanePop(unionX));
            const __m256i Sv=_mm256_load_si256(reinterpret_cast<const __m256i*>(S));
            const __m256i Xv=_mm256_load_si256(reinterpret_cast<const __m256i*>(X));
            const __m256i zero=_mm256_setzero_si256();
            const __m256i km1=_mm256_set1_epi32(k-1);
            __m256i satV=zero;
            uint32_t tu=unionS;
            while(tu){
                const int u=laneFirst(tu); tu&=tu-1;
                const __m256i bit=_mm256_set1_epi32(static_cast<int>(1u<<u));
                const __m256i row=_mm256_set1_epi32(static_cast<int>(ctx.nonadj[u]));
                const __m256i cnt=avxPopcnt8x32(_mm256_and_si256(row,Sv));
                const __m256i eq=_mm256_cmpeq_epi32(cnt,km1);
                const __m256i inS=_mm256_cmpeq_epi32(_mm256_and_si256(Sv,bit),bit);
                satV=_mm256_or_si256(satV,_mm256_and_si256(_mm256_and_si256(eq,inS),bit));
            }
            const __m256i ssV=avxPopcnt8x32(Sv);
            uint8_t liveMask=static_cast<uint8_t>((1u<<active)-1u);
            uint32_t tx=unionX;
            while(tx && liveMask){
                const int x=laneFirst(tx); tx&=tx-1;
                const __m256i bit=_mm256_set1_epi32(static_cast<int>(1u<<x));
                const __m256i inX=_mm256_cmpeq_epi32(_mm256_and_si256(Xv,bit),bit);
                const __m256i adj=_mm256_set1_epi32(static_cast<int>(ctx.adj[x]));
                const __m256i deg=avxPopcnt8x32(_mm256_and_si256(adj,Sv));
                const __m256i enough=_mm256_cmpgt_epi32(_mm256_add_epi32(deg,_mm256_set1_epi32(k)),ssV);
                const __m256i nonadj=_mm256_set1_epi32(static_cast<int>(ctx.nonadj[x]));
                const __m256i noSatConflict=_mm256_cmpeq_epi32(_mm256_and_si256(nonadj,satV),zero);
                const __m256i canAdd=_mm256_and_si256(inX,_mm256_and_si256(enough,noSatConflict));
                const uint8_t reject=static_cast<uint8_t>(_mm256_movemask_ps(_mm256_castsi256_ps(canAdd))) & liveMask;
                if(reject){
                    const int nr=__builtin_popcount(static_cast<unsigned>(reject));
                    bitStats.x_rejects += nr;
                    AVX_COUNT(maxSimdStats.local_x_rejects += nr);
                    liveMask=static_cast<uint8_t>(liveMask & ~reject);
                }
            }
            AVX_COUNT(maxSimdStats.local_x_passes += __builtin_popcount(static_cast<unsigned>(liveMask)));
#if AVX_LOCAL_MAX_SUFFIX
            liveMask=avxFinishLocalMaxSuffix8(Sv,satV,ssV,liveMask,unionS,ctx,suffixCache);
            for(int i=0;i<active;i++) if(liveMask&(1u<<i)){
#ifdef SET
                const int oldPlexSz=plex->sz;
                state32CoreAppendForcedBase();
                uint32_t t=S[i];
                while(t){
                    const int j=laneFirst(t); t&=t-1;
                    const int oi=ctx.orig[j];
                    if(oi>=bitFixed) plex->push(bitOrig[oi]);
                }
                emitVerifiedPlex(plex);
                while(plex->sz>oldPlexSz) plex->pop();
#else
                emitter->emitPlex();
#endif
                ++bitStats.outputs;
                AVX_COUNT(++maxSimdStats.emits);
            }
#else
            alignas(32) uint32_t sat[8];
            _mm256_store_si256(reinterpret_cast<__m256i*>(sat),satV);
            for(int i=0;i<active;i++) if(liveMask&(1u<<i)) laneFinishMaximalityAndEmit(S[i],sat[i],ctx);
#endif
        }

