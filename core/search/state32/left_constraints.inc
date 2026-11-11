template<int K>
        inline uint8_t avxLeft2DRejectMaskT(const __m256i Sv,const __m256i satV,
                                            const uint8_t liveMask,const uint32_t unionS,
                                            const LaneContext32& ctx){
            static_assert(K>=1 && K<=ANYK_FAST_K_MAX,"fast Any-K left K out of range");
            if(!liveMask || leftSz<=0) return 0;
            ANYK_COUNT(++anyKStats.left_template_calls);
            ensureLeftNon32Transpose();
            AVX_COUNT(++maxSimdStats.left_2d_batches);
            AVX_COUNT(maxSimdStats.left_2d_slots += __builtin_popcount(static_cast<unsigned>(liveMask)));
            const __m256i zero=_mm256_setzero_si256();
            uint8_t pending=liveMask,rejectAll=0;
            for(int c=0;c<leftChunks32 && pending;c++){
                AVX_COUNT(++maxSimdStats.left_2d_chunks);
                ANYK_COUNT(++anyKStats.left_chunks);
                const int remain=leftSz-(c<<5);
                const uint32_t valid=(remain>=32)?0xffffffffU:((1U<<remain)-1U);
                const __m256i pendingV=avxLaneMaskVec(pending);
                __m256i def[K];
                def[0]=_mm256_and_si256(_mm256_set1_epi32(static_cast<int>(valid)),pendingV);
                for(int t=1;t<K;t++) def[t]=zero;
                uint32_t tu=unionS;
                while(tu){
                    const int u=laneFirst(tu); tu&=tu-1;
                    const __m256i ubit=_mm256_set1_epi32(static_cast<int>(1u<<u));
                    const __m256i inS=_mm256_and_si256(pendingV,_mm256_cmpeq_epi32(_mm256_and_si256(Sv,ubit),ubit));
                    const uint8_t inSMask=static_cast<uint8_t>(_mm256_movemask_ps(_mm256_castsi256_ps(inS)));
                    if(!inSMask) continue;
                    AVX_COUNT(++maxSimdStats.left_2d_vertex_steps);
                    ANYK_COUNT(++anyKStats.left_vertex_steps);
                    const __m256i inSat=_mm256_cmpeq_epi32(_mm256_and_si256(satV,ubit),ubit);
                    const __m256i satLanes=_mm256_and_si256(inS,inSat);
                    const __m256i nonSatLanes=_mm256_andnot_si256(inSat,inS);
                    const __m256i nv=_mm256_set1_epi32(static_cast<int>(state32CoreLeftNon32At(ctx.orig[u],c)));
                    const __m256i satN=_mm256_and_si256(nv,satLanes);
                    for(int t=0;t<K;t++) def[t]=_mm256_andnot_si256(satN,def[t]);
                    const __m256i stepN=_mm256_and_si256(nv,nonSatLanes);
                    for(int t=K-1;t>=1;t--)
                        def[t]=_mm256_or_si256(_mm256_andnot_si256(stepN,def[t]),_mm256_and_si256(def[t-1],stepN));
                    def[0]=_mm256_andnot_si256(stepN,def[0]);
                    __m256i alive=def[0];
                    for(int t=1;t<K;t++) alive=_mm256_or_si256(alive,def[t]);
                    if((avxNonZeroLaneMask(alive)&pending)==0){
                        AVX_COUNT(++maxSimdStats.left_2d_early_empty);
                        break;
                    }
                }
                __m256i alive=def[0];
                for(int t=1;t<K;t++) alive=_mm256_or_si256(alive,def[t]);
                const uint8_t reject=static_cast<uint8_t>(avxNonZeroLaneMask(alive)&pending);
                if(reject){
                    rejectAll=static_cast<uint8_t>(rejectAll|reject);
                    pending=static_cast<uint8_t>(pending&~reject);
                    const int nr=__builtin_popcount(static_cast<unsigned>(reject));
                    AVX_COUNT(maxSimdStats.left_2d_early_rejects += nr);
                    ANYK_COUNT(anyKStats.left_rejects += nr);
                }
            }
            return rejectAll;
        }

// ==== shared avxLeft2DRejectMaskDynamic ====
        inline uint8_t avxLeft2DRejectMaskDynamic(const __m256i Sv,const __m256i satV,
                                                  const uint8_t liveMask,const uint32_t unionS,
                                                  const LaneContext32& ctx){
            if(!liveMask || leftSz<=0) return 0;
            if(k>32){
                // A local state has at most 32 vertices.  For k>32 every left
                // candidate can be added to every live local state, so all are
                // immediately non-maximal.
                ANYK_COUNT(++anyKStats.left_trivial_calls);
                ANYK_COUNT(anyKStats.left_rejects += __builtin_popcount(static_cast<unsigned>(liveMask)));
                return liveMask;
            }
            ANYK_COUNT(++anyKStats.left_dynamic_calls);
            ensureLeftNon32Transpose();
            AVX_COUNT(++maxSimdStats.left_2d_batches);
            AVX_COUNT(maxSimdStats.left_2d_slots += __builtin_popcount(static_cast<unsigned>(liveMask)));
            const __m256i zero=_mm256_setzero_si256();
            uint8_t pending=liveMask,rejectAll=0;
            for(int c=0;c<leftChunks32 && pending;c++){
                AVX_COUNT(++maxSimdStats.left_2d_chunks);
                ANYK_COUNT(++anyKStats.left_chunks);
                const int remain=leftSz-(c<<5);
                const uint32_t valid=(remain>=32)?0xffffffffU:((1U<<remain)-1U);
                const __m256i pendingV=avxLaneMaskVec(pending);
                __m256i def[32];
                def[0]=_mm256_and_si256(_mm256_set1_epi32(static_cast<int>(valid)),pendingV);
                for(int t=1;t<k;t++) def[t]=zero;
                uint32_t tu=unionS;
                while(tu){
                    const int u=laneFirst(tu); tu&=tu-1;
                    const __m256i ubit=_mm256_set1_epi32(static_cast<int>(1u<<u));
                    const __m256i inS=_mm256_and_si256(pendingV,_mm256_cmpeq_epi32(_mm256_and_si256(Sv,ubit),ubit));
                    const uint8_t inSMask=static_cast<uint8_t>(_mm256_movemask_ps(_mm256_castsi256_ps(inS)));
                    if(!inSMask) continue;
                    AVX_COUNT(++maxSimdStats.left_2d_vertex_steps);
                    ANYK_COUNT(++anyKStats.left_vertex_steps);
                    const __m256i inSat=_mm256_cmpeq_epi32(_mm256_and_si256(satV,ubit),ubit);
                    const __m256i satLanes=_mm256_and_si256(inS,inSat);
                    const __m256i nonSatLanes=_mm256_andnot_si256(inSat,inS);
                    const __m256i nv=_mm256_set1_epi32(static_cast<int>(state32CoreLeftNon32At(ctx.orig[u],c)));
                    const __m256i satN=_mm256_and_si256(nv,satLanes);
                    for(int t=0;t<k;t++) def[t]=_mm256_andnot_si256(satN,def[t]);
                    const __m256i stepN=_mm256_and_si256(nv,nonSatLanes);
                    for(int t=k-1;t>=1;t--)
                        def[t]=_mm256_or_si256(_mm256_andnot_si256(stepN,def[t]),_mm256_and_si256(def[t-1],stepN));
                    def[0]=_mm256_andnot_si256(stepN,def[0]);
                    __m256i alive=def[0];
                    for(int t=1;t<k;t++) alive=_mm256_or_si256(alive,def[t]);
                    if((avxNonZeroLaneMask(alive)&pending)==0){
                        AVX_COUNT(++maxSimdStats.left_2d_early_empty);
                        break;
                    }
                }
                __m256i alive=def[0];
                for(int t=1;t<k;t++) alive=_mm256_or_si256(alive,def[t]);
                const uint8_t reject=static_cast<uint8_t>(avxNonZeroLaneMask(alive)&pending);
                if(reject){
                    rejectAll=static_cast<uint8_t>(rejectAll|reject);
                    pending=static_cast<uint8_t>(pending&~reject);
                    const int nr=__builtin_popcount(static_cast<unsigned>(reject));
                    AVX_COUNT(maxSimdStats.left_2d_early_rejects += nr);
                    ANYK_COUNT(anyKStats.left_rejects += nr);
                }
            }
            return rejectAll;
        }

// ==== shared dispatchAnyKLeft2D ====
        inline uint8_t dispatchAnyKLeft2D(const __m256i Sv,const __m256i satV,
                                           const uint8_t liveMask,const uint32_t unionS,
                                           const LaneContext32& ctx){
            ANYK_COUNT(++anyKStats.left_anyk_calls);
            return dispatchAnyKLeft2DTemplate<1>(Sv,satV,liveMask,unionS,ctx);
        }

// ==== shared dispatchAnyKDefectMasks ====
        inline void dispatchAnyKDefectMasks(const __m256i Pv,const __m256i activeV,const uint32_t unionP,
                                             const LaneContext32& ctx,__m256i& badNoV,__m256i& satV){
            dispatchAnyKDefectMasksTemplate<1>(Pv,activeV,unionP,ctx,badNoV,satV);
        }

