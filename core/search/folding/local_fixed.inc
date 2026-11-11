        // F remains logically selected, but never occupies a State32 lane.
        uint64_t eagerFoldingState32LocalF=0;
        // Per-worker generation-stamped penalty cache. The generation changes
        // at each State32 handoff. Cached penalties must never survive a new F.
        uint64_t localFixedOptimizedGeneration=1,localFixedOptimizedCurrentF=0;
        uint64_t localFixedOptimizedInternalStamp[BIT_LIMIT]{};
        uint8_t localFixedOptimizedInternalMiss[BIT_LIMIT]{};
        vector<uint64_t> localFixedOptimizedExtStamp,localFixedOptimizedLeftStamp;
        vector<uint8_t> localFixedOptimizedExtMiss,localFixedOptimizedLeftMiss;
        inline void localFixedOptimizedBegin(const uint64_t F){
            auto& mc=mixed_f64::stats();
            const bool mixed=bitForcedCount&&enableMixedFixedContext;
            if(mixed)++mc.cache_begins;
            mixed_f64::Span cacheSpan(mixed?mc.cache_begins:0,mc.cache_samples,mc.cache_sample_ns);
            localFixedOptimizedCurrentF=F;
            if(++localFixedOptimizedGeneration==0){
                std::fill(std::begin(localFixedOptimizedInternalStamp),std::end(localFixedOptimizedInternalStamp),0ULL);
                std::fill(localFixedOptimizedExtStamp.begin(),localFixedOptimizedExtStamp.end(),0ULL);
                std::fill(localFixedOptimizedLeftStamp.begin(),localFixedOptimizedLeftStamp.end(),0ULL);
                localFixedOptimizedGeneration=1;
            }
            if(static_cast<int>(localFixedOptimizedExtStamp.size())<bitExtN){
                if(bitForcedCount&&enableMixedFixedContext)++mixed_f64::stats().cache_grows;
                localFixedOptimizedExtStamp.resize(bitExtN,0);localFixedOptimizedExtMiss.resize(bitExtN,0);
            }
            if(static_cast<int>(localFixedOptimizedLeftStamp.size())<leftSz){
                if(bitForcedCount&&enableMixedFixedContext)++mixed_f64::stats().cache_grows;
                localFixedOptimizedLeftStamp.resize(leftSz,0);localFixedOptimizedLeftMiss.resize(leftSz,0);
            }
        }
        inline int localFixedOptimizedInternalPenalty(const int x){
            ++state64Stats.localFixedOptimized_penalty_lookups;
            const bool mixed=bitForcedCount&&enableMixedFixedContext;auto& mc=mixed_f64::stats();
            if(mixed)++mc.penalty_lookups;
            mixed_f64::Span penaltySpan(mixed?mc.penalty_lookups:0,mc.penalty_samples,mc.penalty_sample_ns);
            if(localFixedOptimizedInternalStamp[x]!=localFixedOptimizedGeneration){
                localFixedOptimizedInternalMiss[x]=static_cast<uint8_t>(bitPop(bitNon[x]&localFixedOptimizedCurrentF));
                localFixedOptimizedInternalStamp[x]=localFixedOptimizedGeneration;
                ++state64Stats.localFixedOptimized_penalty_misses;
                if(bitForcedCount&&enableMixedFixedContext)++mixed_f64::stats().penalty_misses;
            }
            return localFixedOptimizedInternalMiss[x];
        }
        inline int localFixedOptimizedExtPenalty(const int e){
            ++state64Stats.localFixedOptimized_penalty_lookups;
            const bool mixed=bitForcedCount&&enableMixedFixedContext;auto& mc=mixed_f64::stats();
            if(mixed)++mc.penalty_lookups;
            mixed_f64::Span penaltySpan(mixed?mc.penalty_lookups:0,mc.penalty_samples,mc.penalty_sample_ns);
            if(localFixedOptimizedExtStamp[e]!=localFixedOptimizedGeneration){
                localFixedOptimizedExtMiss[e]=static_cast<uint8_t>(bitPop((~bitExtAdj[e])&localFixedOptimizedCurrentF));
                localFixedOptimizedExtStamp[e]=localFixedOptimizedGeneration;
                ++state64Stats.localFixedOptimized_penalty_misses;
                if(bitForcedCount&&enableMixedFixedContext)++mixed_f64::stats().penalty_misses;
            }
            return localFixedOptimizedExtMiss[e]+bitForcedMissing(bitExtForcedAdj[e]);
        }
        inline int localFixedOptimizedLeftPenalty(const int l){
            ++state64Stats.localFixedOptimized_penalty_lookups;
            const bool mixed=bitForcedCount&&enableMixedFixedContext;auto& mc=mixed_f64::stats();
            if(mixed)++mc.penalty_lookups;
            mixed_f64::Span penaltySpan(mixed?mc.penalty_lookups:0,mc.penalty_samples,mc.penalty_sample_ns);
            if(localFixedOptimizedLeftStamp[l]!=localFixedOptimizedGeneration){
                localFixedOptimizedLeftMiss[l]=static_cast<uint8_t>(bitPop((~bitLeftAdj[l])&localFixedOptimizedCurrentF));
                localFixedOptimizedLeftStamp[l]=localFixedOptimizedGeneration;
                ++state64Stats.localFixedOptimized_penalty_misses;
                if(bitForcedCount&&enableMixedFixedContext)++mixed_f64::stats().penalty_misses;
            }
            return localFixedOptimizedLeftMiss[l]+bitForcedMissing(bitLeftForcedAdj[l]);
        }
        inline uint8_t localFixedOptimizedCanAddRowMask(const uint32_t row,const int miss,
            const __m256i Sv,const __m256i satV,const __m256i ssV,const uint8_t live) const {
            if(!live || miss>=k) return 0;
            const __m256i rowV=_mm256_set1_epi32(static_cast<int>(row));
            const __m256i deg=avxPopcnt8x32(_mm256_and_si256(rowV,Sv));
            const __m256i enough=_mm256_cmpgt_epi32(
                _mm256_add_epi32(deg,_mm256_set1_epi32(k-miss)),ssV);
            const __m256i noSat=_mm256_cmpeq_epi32(
                _mm256_andnot_si256(rowV,satV),_mm256_setzero_si256());
            return static_cast<uint8_t>(_mm256_movemask_ps(_mm256_castsi256_ps(
                _mm256_and_si256(enough,noSat))))&live;
        }
        void localFixedOptimizedFlushCachedOverlay(const MaxReadyState32 batch[8],const int active,
            const LaneContext32& ctx,const LocalMaxSuffixCache32& cache,const uint64_t F){
            if(!active)return;
            assert(F && localFixedOptimizedCurrentF==F);
            auto& mc=mixed_f64::stats();
            const bool mixed=bitForcedCount&&enableMixedFixedContext;
            if(mixed){mc.local_max_slots+=active;++mc.local_max_batches;}
            mixed_f64::Span maxSpan(mixed?mc.local_max_batches:0,mc.local_max_samples,mc.local_max_sample_ns);
            ++state64Stats.localFixedOptimized_batches;
            state64Stats.localFixedOptimized_slots+=active;
            if(active==8)++state64Stats.localFixedOptimized_full_batches;
            alignas(32) uint32_t S[8]={0},X[8]={0};
            uint32_t unionS=0,unionX=0;
            for(int i=0;i<active;i++){
                S[i]=batch[i].S;X[i]=batch[i].X;
                unionS|=S[i];unionX|=X[i];
            }
            const __m256i Sv=_mm256_load_si256(reinterpret_cast<const __m256i*>(S));
            const __m256i Xv=_mm256_load_si256(reinterpret_cast<const __m256i*>(X));
            const __m256i zero=_mm256_setzero_si256();
            const __m256i km1=_mm256_set1_epi32(k-1);
            __m256i satV=zero;
            uint32_t tu=unionS;
            while(tu){
                const int u=laneFirst(tu);tu&=tu-1;
                const __m256i bit=_mm256_set1_epi32(static_cast<int>(1u<<u));
                const __m256i non=_mm256_set1_epi32(static_cast<int>(ctx.nonadj[u]));
                const __m256i eq=_mm256_cmpeq_epi32(avxPopcnt8x32(_mm256_and_si256(non,Sv)),km1);
                const __m256i inS=_mm256_cmpeq_epi32(_mm256_and_si256(Sv,bit),bit);
                satV=_mm256_or_si256(satV,_mm256_and_si256(bit,_mm256_and_si256(eq,inS)));
            }
            const __m256i ssV=avxPopcnt8x32(Sv);
            uint8_t live=static_cast<uint8_t>((1u<<active)-1u);
            // Exact local X: do not assume
            // every pre-handoff excluded vertex is adjacent to every F.
            uint32_t tx=unionX;
            while(tx && live){
                const int x=laneFirst(tx);tx&=tx-1;
                const __m256i bit=_mm256_set1_epi32(static_cast<int>(1u<<x));
                const uint8_t inX=static_cast<uint8_t>(_mm256_movemask_ps(_mm256_castsi256_ps(
                    _mm256_cmpeq_epi32(_mm256_and_si256(Xv,bit),bit))));
                if(!(inX&live))continue;
                ++state64Stats.localFixedOptimized_x_rows;
                const uint8_t reject=localFixedOptimizedCanAddRowMask(ctx.adj[x],localFixedOptimizedInternalPenalty(ctx.orig[x]),
                    Sv,satV,ssV,static_cast<uint8_t>(live&inX));
                bitStats.x_rejects+=__builtin_popcount(static_cast<unsigned>(reject));
                live=static_cast<uint8_t>(live&~reject);
            }
            uint64_t bx=ctx.baseX;
            while(bx && live){
                const int x=bitFirst(bx);bx&=bx-1;
                ++state64Stats.localFixedOptimized_basex_rows;
                const uint8_t reject=localFixedOptimizedCanAddRowMask(localSuffixBaseRow(x,ctx,cache),
                    localFixedOptimizedInternalPenalty(x),Sv,satV,ssV,live);
                bitStats.x_rejects+=__builtin_popcount(static_cast<unsigned>(reject));
                live=static_cast<uint8_t>(live&~reject);
            }
            for(int e=0;e<bitExtN && live;e++){
                ++state64Stats.localFixedOptimized_ext_rows;
                const uint8_t reject=localFixedOptimizedCanAddRowMask(localSuffixExternalRow(e,ctx,cache),
                    localFixedOptimizedExtPenalty(e),Sv,satV,ssV,live);
                bitStats.x_rejects+=__builtin_popcount(static_cast<unsigned>(reject));
                live=static_cast<uint8_t>(live&~reject);
            }
            for(int l=0;l<leftSz && live;l++){
                ++state64Stats.localFixedOptimized_left_rows;
                const uint8_t reject=localFixedOptimizedCanAddRowMask(localSuffixLeftRow(l,ctx,cache),
                    localFixedOptimizedLeftPenalty(l),Sv,satV,ssV,live);
                bitStats.left_rejects+=__builtin_popcount(static_cast<unsigned>(reject));
                live=static_cast<uint8_t>(live&~reject);
            }
            for(int i=0;i<active;i++)if(live&(1u<<i)){
#ifdef SET
                const int old=plex->sz;
                if(bitForcedCount)for(const int v:forcedBase)plex->push(v);
                const uint64_t logical=laneExpand(S[i],ctx)|F;
                for(int j=bitFixed;j<bitM;j++)if(logical&(1ULL<<j))plex->push(bitOrig[j]);
                emitVerifiedPlex(plex);
                while(plex->sz>old)plex->pop();
#else
                emitter->emitPlex();
#endif
                ++bitStats.outputs;
                ++state64Stats.localFixedOptimized_emitted;
            }
        }
        inline int state32CoreActiveLb() const {
            return local_fixed_localf_contract::active_lower_bound(bitActiveLb(),eagerFoldingState32LocalF);
        }
        inline int state32ContextActiveLb(const LaneContext32& ctx) const {
#if STATE32_LB_CACHE_VERIFY
            assert(ctx.activeLb==state32CoreActiveLb());
#endif
            return ctx.activeLb;
        }
        inline bool state32CoreHasForcedBase() const { return bitForcedCount != 0; }
        inline void state32CoreAppendForcedBase(){
            if(bitForcedCount) for(const int f:forcedBase) plex->push(f);
        }
        inline uint8_t state32CoreForcedLeftReject(const __m256i Sv,const __m256i satV,
            const uint8_t liveMask,const uint32_t unionS,const LaneContext32& ctx){
            return forcedLeftForcedLeftReject(Sv,satV,liveMask,unionS,ctx);
        }
        inline uint32_t state32CoreLeftNon32At(const int u,const int chunk) {
            return leftNon32At(u,chunk);
        }
        inline bool state32CoreCanAddExternal(const int e,const uint64_t S,const uint64_t sat) {
            return bitCanAddExternal(e,S,sat);
        }
        inline void state32CoreEntryPrepare(){ defectCachePrepareState32(); }
        inline void state32CoreBeginLocalF(const uint64_t F){
            if(F){if(bitForcedCount&&enableMixedFixedContext)++mixed_f64::stats().local_contexts;localFixedOptimizedBegin(F);}
        }
        void state32CoreRematerialize(const LaneState32& st,const LaneContext32& ctx,const uint64_t F){
            assert(F && st.C);
            const uint64_t P=laneExpand(st.P,ctx)|F;
            const uint64_t C=laneExpand(st.C,ctx);
            const uint64_t X=ctx.baseX|laneExpand(st.X,ctx);
            assert(!(P&C) && !(P&X) && !(C&X) && bitPop(P|C)<=32);
            auto& stats=experiment_control::samples();
            ++stats.remat_converted;stats.remat_f_vertices+=bitPop(F);
            const int cc=lanePop(st.C);
            ++stats.remat_c[cc<=2?cc:(cc<=4?3:(cc<=8?4:(cc<=16?5:6)))];
            if(bitPop(P|C)==32)++stats.remat_width32;
            if(X)++stats.remat_with_basex;
            // Only this subtree uses the new coordinates. The parent's stack,
            // pending terminals and local-F penalty generation remain intact.
            struct Restore {
                uint64_t& slot;uint64_t old;
                explicit Restore(uint64_t& x):slot(x),old(x){slot=0;}
                ~Restore(){slot=old;}
            };
            {
                Restore restore(eagerFoldingState32LocalF);
                experiment_control::SampledSpan subtreeSpan(experiment_control::RematSubtree);
                avxStageBatchedDFS(P,C,X,true,true);
            }
            assert(eagerFoldingState32LocalF==F);
            assert(localFixedOptimizedCurrentF==F);
            ++stats.remat_restored;
        }
        // LOCAL_FIXED: virtual-F terminal. Search state stays in State32 coordinates;
        // verify against the entire logical S|F without materializing an extra
        // search vertex. X/external/left all retain full-graph maximality.
        bool localFixedOverlayMaximalAndEmit(const uint64_t S,const uint64_t X,const uint64_t F){
            const uint64_t full=local_fixed_localf_contract::logical_solution(S,F);
            const int ss=bitPop(S),fs=bitPop(F);
            const uint64_t sat=bitSaturated(full);
            for(uint64_t t=X;t;t&=t-1){
                const int x=bitFirst(t);
                if(bitPop(bitAdj[x]&S)+bitPop(bitAdj[x]&F)+k>=ss+fs+1
                   && !(bitNon[x]&sat)){ ++bitStats.x_rejects;return false; }
            }
            for(int e=0;e<bitExtN;e++){
                const int miss=bitForcedMissing(bitExtForcedAdj[e]);
                if(bitPop(bitExtAdj[e]&S)+bitPop(bitExtAdj[e]&F)+k>=ss+fs+1+miss
                   && !(~bitExtAdj[e]&sat)){++bitStats.x_rejects;return false;}
            }
            for(int l=0;l<leftSz;l++){
                const int miss=bitForcedMissing(bitLeftForcedAdj[l]);
                const uint64_t a=bitLeftAdj[l];
                if(bitPop(a&S)+bitPop(a&F)+k>=ss+fs+1+miss
                   && !(sat&~a)){++bitStats.left_rejects;return false;}
            }
#ifdef SET
            const int old=plex->sz;
            if(bitForcedCount)for(const int v:forcedBase)plex->push(v);
            for(int i=bitFixed;i<bitM;i++)if(full&(1ULL<<i))plex->push(bitOrig[i]);
            emitVerifiedPlex(plex);
            while(plex->sz>old)plex->pop();
#else
            emitter->emitPlex();
#endif
            ++bitStats.outputs;return true;
        }
        inline void state32CoreFlushMax(const MaxReadyState32 batch[8],const int active,
            const LaneContext32& ctx,const LocalMaxSuffixCache32& cache,const uint64_t F){
            if(F){
                state64Stats.eager_folding_localf_max_checks+=static_cast<unsigned long long>(active);
                localFixedOptimizedFlushCachedOverlay(batch,active,ctx,cache,F);
                return;
            }
            (void)F; avxFlushLocalMaxBatch(batch,active,ctx,cache);
        }
        template<class Pool>
        void avxStageBatchedDFSImpl(const uint64_t P,const uint64_t C,const uint64_t X,
                                    Pool& stack,const bool rootPrechecked=true,const bool rematerializedRoot=false){
            state32CoreDFS(P,C,X,stack,rootPrechecked,0,rematerializedRoot);
        }

#include "../../shared/state32_core.inc"
        // A per-worker scoped overlay: no Local-F is stored on shared tasks.
        // Exported frontiers continue materializing F into P before donation.
