// State32 vector pivot selection; included inside SearchContext.
#ifndef ANYK_COUNT
#define ANYK_COUNT(expr) do{}while(0)
#endif
#define SEQ_STATE64_AGGREGATE_TELEMETRY 0
// ==== shared avxChoosePivot8 ====
        void avxChoosePivot8(const uint32_t P[8],const uint32_t C[8],const uint8_t needMask,
                             const LaneContext32& ctx,int pivot[8]) const {
            AVX_COUNT(++avxStats.vector_pivot_calls);
            const __m256i Pv=_mm256_loadu_si256(reinterpret_cast<const __m256i*>(P));
            const __m256i Cv=_mm256_loadu_si256(reinterpret_cast<const __m256i*>(C));
            const __m256i Uv=_mm256_or_si256(Pv,Cv);
            const __m256i psV=avxPopcnt8x32(Pv);
            const __m256i usV=avxPopcnt8x32(Uv);
            __m256i pminDeg=_mm256_set1_epi32(0x3fffffff);
            __m256i pminIdx=_mm256_set1_epi32(-1);
            for(int u=0;u<ctx.m;u++){
                const __m256i bit=_mm256_set1_epi32(static_cast<int>(1u<<u));
                const __m256i inP=_mm256_cmpeq_epi32(_mm256_and_si256(Pv,bit),bit);
                const __m256i row=_mm256_set1_epi32(static_cast<int>(ctx.adj[u]));
                const __m256i deg=avxPopcnt8x32(_mm256_and_si256(row,Uv));
                const __m256i better=_mm256_cmpgt_epi32(pminDeg,deg);
                const __m256i upd=_mm256_and_si256(inP,better);
                pminDeg=_mm256_blendv_epi8(pminDeg,deg,upd);
                pminIdx=_mm256_blendv_epi8(pminIdx,_mm256_set1_epi32(u),upd);
            }
            alignas(32) int pmin[8],pdeg[8],ps[8],us[8];
            _mm256_store_si256(reinterpret_cast<__m256i*>(pmin),pminIdx);
            _mm256_store_si256(reinterpret_cast<__m256i*>(pdeg),pminDeg);
            _mm256_store_si256(reinterpret_cast<__m256i*>(ps),psV);
            _mm256_store_si256(reinterpret_cast<__m256i*>(us),usV);
            alignas(32) uint32_t domain[8]={0,0,0,0,0,0,0,0};
            for(int i=0;i<8;i++){
                pivot[i]=-1;
                if(!(needMask&(1u<<i))) continue;
                if(pmin[i]<0 || pdeg[i]+k<std::max(state32ContextActiveLb(ctx),ps[i])){ pivot[i]=-2; continue; }
                domain[i]=(pdeg[i]+k<us[i])?(C[i]&ctx.nonadj[pmin[i]]):C[i];
            }
            const __m256i Dv=_mm256_load_si256(reinterpret_cast<const __m256i*>(domain));
            __m256i bestDeg=_mm256_set1_epi32(0x3fffffff);
            __m256i bestPDeg=_mm256_set1_epi32(0x3fffffff);
            __m256i bestIdx=_mm256_set1_epi32(-1);
            for(int x=0;x<ctx.m;x++){
                const __m256i bit=_mm256_set1_epi32(static_cast<int>(1u<<x));
                const __m256i inD=_mm256_cmpeq_epi32(_mm256_and_si256(Dv,bit),bit);
                const __m256i row=_mm256_set1_epi32(static_cast<int>(ctx.adj[x]));
                const __m256i dg=avxPopcnt8x32(_mm256_and_si256(row,Uv));
                const __m256i pd=avxPopcnt8x32(_mm256_and_si256(row,Pv));
                const __m256i betterD=_mm256_cmpgt_epi32(bestDeg,dg);
                const __m256i equalD=_mm256_cmpeq_epi32(bestDeg,dg);
                const __m256i betterP=_mm256_cmpgt_epi32(bestPDeg,pd);
                const __m256i better=_mm256_or_si256(betterD,_mm256_and_si256(equalD,betterP));
                const __m256i upd=_mm256_and_si256(inD,better);
                bestDeg=_mm256_blendv_epi8(bestDeg,dg,upd);
                bestPDeg=_mm256_blendv_epi8(bestPDeg,pd,upd);
                bestIdx=_mm256_blendv_epi8(bestIdx,_mm256_set1_epi32(x),upd);
            }
            alignas(32) int out[8];
            _mm256_store_si256(reinterpret_cast<__m256i*>(out),bestIdx);
            for(int i=0;i<8;i++) if((needMask&(1u<<i)) && pivot[i]!=-2) pivot[i]=out[i];
        }

// ==== shared avxChoosePivot8UnionSparse ====
        void avxChoosePivot8UnionSparse(const uint32_t P[8],const uint32_t C[8],const uint8_t needMask,
                                        const LaneContext32& ctx,int pivot[8]) const {
            AVX_COUNT(++avxStats.vector_pivot_calls);
            PIVOT_COUNT(++pivotReuseStats.calls);
            const __m256i Pv=_mm256_loadu_si256(reinterpret_cast<const __m256i*>(P));
            const __m256i Cv=_mm256_loadu_si256(reinterpret_cast<const __m256i*>(C));
            const __m256i Uv=_mm256_or_si256(Pv,Cv);
            const __m256i psV=avxPopcnt8x32(Pv);
            const __m256i usV=avxPopcnt8x32(Uv);
            uint32_t unionP=0;
            for(int i=0;i<8;i++) if(needMask&(1u<<i)) unionP|=P[i];
            AVX_COUNT(avxStats.pivot_union_vertices += lanePop(unionP));
            PIVOT_COUNT(pivotReuseStats.union_p_vertices += lanePop(unionP));
            __m256i pminDeg=_mm256_set1_epi32(0x3fffffff);
            __m256i pminIdx=_mm256_set1_epi32(-1);
            uint32_t tu=unionP;
            while(tu){
                const int u=laneFirst(tu); tu&=tu-1;
                const __m256i bit=_mm256_set1_epi32(static_cast<int>(1u<<u));
                const __m256i inP=_mm256_cmpeq_epi32(_mm256_and_si256(Pv,bit),bit);
                const __m256i row=_mm256_set1_epi32(static_cast<int>(ctx.adj[u]));
                const __m256i deg=avxPopcnt8x32(_mm256_and_si256(row,Uv));
                PIVOT_COUNT(++pivotReuseStats.first_dg_popcnt_vectors);
                const __m256i better=_mm256_cmpgt_epi32(pminDeg,deg);
                const __m256i upd=_mm256_and_si256(inP,better);
                pminDeg=_mm256_blendv_epi8(pminDeg,deg,upd);
                pminIdx=_mm256_blendv_epi8(pminIdx,_mm256_set1_epi32(u),upd);
            }
            alignas(32) int pmin[8],pdeg[8],ps[8],us[8];
            _mm256_store_si256(reinterpret_cast<__m256i*>(pmin),pminIdx);
            _mm256_store_si256(reinterpret_cast<__m256i*>(pdeg),pminDeg);
            _mm256_store_si256(reinterpret_cast<__m256i*>(ps),psV);
            _mm256_store_si256(reinterpret_cast<__m256i*>(us),usV);
            alignas(32) uint32_t domain[8]={0,0,0,0,0,0,0,0};
            for(int i=0;i<8;i++){
                pivot[i]=-1;
                if(!(needMask&(1u<<i))) continue;
                if(pmin[i]<0 || pdeg[i]+k<std::max(state32ContextActiveLb(ctx),ps[i])){ pivot[i]=-2; continue; }
                domain[i]=(pdeg[i]+k<us[i])?(C[i]&ctx.nonadj[pmin[i]]):C[i];
            }
            const __m256i Dv=_mm256_load_si256(reinterpret_cast<const __m256i*>(domain));
            PIVOT_COUNT(for(int i=0;i<8;i++) if(needMask&(1u<<i)){ pivotReuseStats.domain_lane_slots += lanePop(domain[i]); if(domain[i]==0) ++pivotReuseStats.zero_domain_lanes; });
            uint32_t unionD=0;
            for(int i=0;i<8;i++) if(needMask&(1u<<i)) unionD|=domain[i];
            AVX_COUNT(avxStats.pivot_union_vertices += lanePop(unionD));
            PIVOT_COUNT(pivotReuseStats.union_d_vertices += lanePop(unionD));
            // Pivot policy: remove the P-degree tie-break completely.  Because unionD is
            // traversed in increasing vertex id and updates are strict dg improvements,
            // ties on dg deterministically keep the smallest vertex id.
            __m256i bestDeg=_mm256_set1_epi32(0x3fffffff);
            __m256i bestIdx=_mm256_set1_epi32(-1);
            uint32_t td=unionD;
            while(td){
                const int x=laneFirst(td); td&=td-1;
                const __m256i bit=_mm256_set1_epi32(static_cast<int>(1u<<x));
                const __m256i inD=_mm256_cmpeq_epi32(_mm256_and_si256(Dv,bit),bit);
                const __m256i row=_mm256_set1_epi32(static_cast<int>(ctx.adj[x]));
                const __m256i dg=avxPopcnt8x32(_mm256_and_si256(row,Uv));
                PIVOT_COUNT(++pivotReuseStats.second_dg_popcnt_vectors);
                const __m256i betterD=_mm256_cmpgt_epi32(bestDeg,dg);
                const __m256i upd=_mm256_and_si256(inD,betterD);
                bestDeg=_mm256_blendv_epi8(bestDeg,dg,upd);
                bestIdx=_mm256_blendv_epi8(bestIdx,_mm256_set1_epi32(x),upd);
            }
            alignas(32) int bi[8];
            _mm256_store_si256(reinterpret_cast<__m256i*>(bi),bestIdx);
            for(int i=0;i<8;i++) if(needMask&(1u<<i)){
                if(pivot[i]!=-2) pivot[i]=bi[i];
            }
        }

