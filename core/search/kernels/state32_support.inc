        struct LanePivotInfo {
            int pivot=-2;
            uint32_t domain=0;
            int bestDeg=INT_MAX;
            int bestPDeg=INT_MAX;
        };

        LanePivotInfo lanePaperPivotInfo(const uint32_t P,const uint32_t C,const LaneContext32& ctx) const {
            LanePivotInfo info;
            const uint32_t U=P|C;
            const int us=lanePop(U),ps=lanePop(P);
            int pmin=-1,pdeg=INT_MAX;
            uint32_t t=P;
            while(t){
                const int u=laneFirst(t); t&=t-1;
                const int d=lanePop(ctx.adj[u]&U);
                if(d<pdeg){ pdeg=d; pmin=u; }
            }
            if(pmin<0 || pdeg+k<std::max(bitActiveLb(),ps)){
                info.pivot=-2;
                return info;
            }
            info.domain=(pdeg+k<us)?(C&ctx.nonadj[pmin]):C;
            int best=-1,bdg=INT_MAX,bpdeg=INT_MAX;
            t=info.domain;
            while(t){
                const int x=laneFirst(t); t&=t-1;
                const int dg=lanePop(ctx.adj[x]&U),pd=lanePop(ctx.adj[x]&P);
                if(dg<bdg || (dg==bdg && pd<bpdeg)){
                    best=x; bdg=dg; bpdeg=pd;
                }
            }
            info.pivot=best;
            info.bestDeg=bdg;
            info.bestPDeg=bpdeg;
            return info;
        }

        inline int laneJointCandidate(const uint32_t common,
                                      const uint32_t plexLane0,const uint32_t C0,
                                      const uint32_t plexLane1,const uint32_t C1,
                                      const LaneContext32& ctx,
                                      int* outRegretMax=nullptr,
                                      const LanePivotInfo* i0=nullptr,
                                      const LanePivotInfo* i1=nullptr) const {
            if(!common) return -1;
            const uint32_t universeLane0=plexLane0|C0,universeLane1=plexLane1|C1;
            int best=-1,bestMaxReg=INT_MAX,bestSumReg=INT_MAX,bestJointDeg=INT_MAX,bestJointPDeg=INT_MAX;
            uint32_t t=common;
            while(t){
                const int x=laneFirst(t); t&=t-1;
                const int d0=lanePop(ctx.adj[x]&universeLane0),d1=lanePop(ctx.adj[x]&universeLane1);
                const int p0=lanePop(ctx.adj[x]&plexLane0),p1=lanePop(ctx.adj[x]&plexLane1);
                int maxReg=0,sumReg=0;
                if(i0 && i1){
                    const int r0=std::max(0,d0-i0->bestDeg);
                    const int r1=std::max(0,d1-i1->bestDeg);
                    maxReg=std::max(r0,r1);
                    sumReg=r0+r1;
                }
                const int jointDeg=d0+d1,jointPDeg=p0+p1;
                if(maxReg<bestMaxReg ||
                   (maxReg==bestMaxReg && sumReg<bestSumReg) ||
                   (maxReg==bestMaxReg && sumReg==bestSumReg && jointDeg<bestJointDeg) ||
                   (maxReg==bestMaxReg && sumReg==bestSumReg && jointDeg==bestJointDeg && jointPDeg<bestJointPDeg)){
                    best=x; bestMaxReg=maxReg; bestSumReg=sumReg; bestJointDeg=jointDeg; bestJointPDeg=jointPDeg;
                }
            }
            if(outRegretMax) *outRegretMax=bestMaxReg;
            return best;
        }

        void laneChooseJointSharedPair(const uint32_t plexLane0,const uint32_t C0,
                                       const uint32_t plexLane1,const uint32_t C1,
                                       const LaneContext32& ctx,int& out0,int& out1) const {
            const LanePivotInfo i0=lanePaperPivotInfo(plexLane0,C0,ctx);
            const LanePivotInfo i1=lanePaperPivotInfo(plexLane1,C1,ctx);
            out0=i0.pivot; out1=i1.pivot;
            if(out0<0 || out1<0 || out0==out1) return;
            const uint32_t common=i0.domain&i1.domain;
            if(!common) return;
            const int shared=laneJointCandidate(common,plexLane0,C0,plexLane1,C1,ctx);
            if(shared>=0){ out0=shared; out1=shared; }
        }

        void laneChooseRegretSharedPair(const uint32_t plexLane0,const uint32_t C0,
                                        const uint32_t plexLane1,const uint32_t C1,
                                        const LaneContext32& ctx,int& out0,int& out1) const {
            const LanePivotInfo i0=lanePaperPivotInfo(plexLane0,C0,ctx);
            const LanePivotInfo i1=lanePaperPivotInfo(plexLane1,C1,ctx);
            out0=i0.pivot; out1=i1.pivot;
            if(out0<0 || out1<0 || out0==out1) return;
            const uint32_t common=i0.domain&i1.domain;
            if(!common) return;
            int regretMax=INT_MAX;
            const int shared=laneJointCandidate(common,plexLane0,C0,plexLane1,C1,ctx,&regretMax,&i0,&i1);
            if(shared>=0 && regretMax<=LANE_REGRET_MAX){ out0=shared; out1=shared; }
        }

        void laneChoosePaperPairProfiled(const uint32_t plexLane0,const uint32_t C0,
                                         const uint32_t plexLane1,const uint32_t C1,
                                         const LaneContext32& ctx,int& out0,int& out1) const {
            const LanePivotInfo i0=lanePaperPivotInfo(plexLane0,C0,ctx);
            const LanePivotInfo i1=lanePaperPivotInfo(plexLane1,C1,ctx);
            out0=i0.pivot; out1=i1.pivot;
        }

        inline int laneChooseSyncPivotSingle(const uint32_t C) const {
            return C?laneFirst(C):-2;
        }
        inline void laneChooseSyncPivotPair(const uint32_t C0,const uint32_t C1,int& out0,int& out1) const {
            // Lane-oriented pivot: maximize instruction/data sharing, not candidate contraction.
            // If both lanes share a candidate, branch on exactly the same local vertex.
            const uint32_t common=C0&C1;
            if(common){
                const int p=laneFirst(common);
                out0=p; out1=p;
                return;
            }
            out0=laneChooseSyncPivotSingle(C0);
            out1=laneChooseSyncPivotSingle(C1);
        }

        inline void laneWholePair(const uint32_t universeLane0,const uint32_t universeLane1,const LaneContext32& ctx,bool& whole0,bool& whole1) const {
            uint32_t t0=universeLane0,t1=universeLane1;
            whole0=true; whole1=true;
            const uint64_t Upack=lanePack(universeLane0,universeLane1);
            while(t0||t1){
                const int u0=t0?laneFirst(t0):-1;
                const int u1=t1?laneFirst(t1):-1;
                if(t0) t0&=t0-1;
                if(t1) t1&=t1-1;
                const uint64_t missPack=laneRowsPack(ctx.nonadj,u0,u1)&Upack;
                LANE_COUNT(++laneStats.packed_whole_steps);
                if(u0>=0 && lanePop(laneLo(missPack))>=k){ whole0=false; t0=0; }
                if(u1>=0 && lanePop(laneHi(missPack))>=k){ whole1=false; t1=0; }
            }
        }

        inline uint64_t laneSaturatedPair(const uint32_t plexLane0,const uint32_t plexLane1,const LaneContext32& ctx){
            uint32_t t0=plexLane0,t1=plexLane1,s0=0,s1=0;
            const uint64_t Ppack=lanePack(plexLane0,plexLane1);
            while(t0||t1){
                const int u0=t0?laneFirst(t0):-1;
                const int u1=t1?laneFirst(t1):-1;
                if(t0) t0&=t0-1;
                if(t1) t1&=t1-1;
                const uint64_t missPack=laneRowsPack(ctx.nonadj,u0,u1)&Ppack;
                LANE_COUNT(++laneStats.packed_refine_steps);
                if(u0>=0 && lanePop(laneLo(missPack))==k-1) s0|=1U<<u0;
                if(u1>=0 && lanePop(laneHi(missPack))==k-1) s1|=1U<<u1;
            }
            return lanePack(s0,s1);
        }

        struct LocalMaxSuffixCache32 {
            uint64_t generation=0;
        };

        inline void beginLocalMaxSuffixContext(LocalMaxSuffixCache32& cache){
            ++suffixGeneration;
            // Practically unreachable, but make generation stamping robust.
            if(suffixGeneration==0){
                std::fill(std::begin(suffixBaseStamp),std::end(suffixBaseStamp),0ULL);
                std::fill(suffixExtStamp.begin(),suffixExtStamp.end(),0ULL);
                std::fill(suffixLeftStamp.begin(),suffixLeftStamp.end(),0ULL);
                suffixGeneration=1;
            }
            cache.generation=suffixGeneration;
        }

        inline uint32_t localSuffixBaseRow(const int x,const LaneContext32& ctx,const LocalMaxSuffixCache32& cache){
            if(suffixBaseStamp[x]!=cache.generation){
                suffixBaseLocal[x]=static_cast<uint32_t>(_pext_u64(bitAdj[x],ctx.rootMask));
                suffixBaseStamp[x]=cache.generation;
                AVX_COUNT(++maxSimdStats.suffix_row_localizations);
            }
            return suffixBaseLocal[x];
        }
        inline uint32_t localSuffixExternalRow(const int e,const LaneContext32& ctx,const LocalMaxSuffixCache32& cache){
            if(suffixExtStamp[e]!=cache.generation){
                suffixExtLocal[e]=static_cast<uint32_t>(_pext_u64(bitExtAdj[e],ctx.rootMask));
                suffixExtStamp[e]=cache.generation;
                AVX_COUNT(++maxSimdStats.suffix_row_localizations);
            }
            return suffixExtLocal[e];
        }
        inline uint32_t localSuffixLeftRow(const int l,const LaneContext32& ctx,const LocalMaxSuffixCache32& cache){
            if(suffixLeftStamp[l]!=cache.generation){
                suffixLeftLocal[l]=static_cast<uint32_t>(_pext_u64(bitLeftAdj[l],ctx.rootMask));
                suffixLeftStamp[l]=cache.generation;
                AVX_COUNT(++maxSimdStats.suffix_row_localizations);
            }
            return suffixLeftLocal[l];
        }

        inline void ensureLeftNon32Transpose(){
            if(leftNon32Ready) return;
            // Transpose rebuild signals a new immutable Bit64 task context.
            invalidateForcedLeftInitCache();
            leftChunks32=(leftSz+31)>>5;
            if(leftChunks32<=0){ leftNon32Ready=true; return; }
            ensureBitAuxCapacity(bitExtN,leftSz);
            for(int j=0;j<bitM;j++){
                for(int c=0;c<leftChunks32;c++){
                    const int remain=leftSz-(c<<5);
                    leftNon32At(j,c)=(remain>=32)?0xffffffffU:((remain<=0)?0U:((1U<<remain)-1U));
                }
            }
            unsigned long long adjBits=0;
            for(int l=0;l<leftSz;l++){
                const int c=l>>5;
                const uint32_t lb=1U<<(l&31);
                uint64_t a=bitLeftAdj[l];
                while(a){
                    const int j=bitFirst(a); a&=a-1;
                    if(j<bitM) leftNon32At(j,c)&=~lb;
                    ++adjBits;
                }
            }
            leftNon32Ready=true;
            AVX_COUNT(++maxSimdStats.left_2d_transpose_builds);
            AVX_COUNT(maxSimdStats.left_2d_transpose_adj_bits += adjBits);
        }

        // Unified Any-K two-dimensional left maximality kernel.
        // SIMD lane dimension: independent recursive states.
        // Bit dimension inside each 32-bit lane: left candidates.
        // K<=ANYK_FAST_K_MAX uses the same parameterized automaton with a
        // compile-time K; larger K uses the equivalent runtime automaton.

        template<int K>
        inline typename std::enable_if<(K>ANYK_FAST_K_MAX),uint8_t>::type
        dispatchAnyKLeft2DTemplate(const __m256i Sv,const __m256i satV,
                                    const uint8_t liveMask,const uint32_t unionS,
                                    const LaneContext32& ctx){
            return avxLeft2DRejectMaskDynamic(Sv,satV,liveMask,unionS,ctx);
        }

        template<int K>
        inline typename std::enable_if<(K<=ANYK_FAST_K_MAX),uint8_t>::type
        dispatchAnyKLeft2DTemplate(const __m256i Sv,const __m256i satV,
                                    const uint8_t liveMask,const uint32_t unionS,
                                    const LaneContext32& ctx){
            if(k==K) return avxLeft2DRejectMaskT<K>(Sv,satV,liveMask,unionS,ctx);
            return dispatchAnyKLeft2DTemplate<K+1>(Sv,satV,liveMask,unionS,ctx);
        }

        inline uint8_t avxCanAddLocalRowMask(const uint32_t row,const __m256i Sv,const __m256i satV,
                                              const __m256i ssV,const uint8_t liveMask,const int forcedMissing=0) const {
            if(!liveMask) return 0;
            const __m256i zero=_mm256_setzero_si256();
            const __m256i rowV=_mm256_set1_epi32(static_cast<int>(row));
            const __m256i deg=avxPopcnt8x32(_mm256_and_si256(rowV,Sv));
            // With ForcedBase, a suffix candidate missing d forced vertices
            // needs d extra active adjacencies: deg+k > |S|+d.
            const __m256i need=_mm256_add_epi32(ssV,_mm256_set1_epi32(forcedMissing));
            const __m256i enough=_mm256_cmpgt_epi32(_mm256_add_epi32(deg,_mm256_set1_epi32(k)),need);
            // sat vertices must all be adjacent to the candidate.
            const __m256i noSatConflict=_mm256_cmpeq_epi32(_mm256_andnot_si256(rowV,satV),zero);
            const __m256i canAdd=_mm256_and_si256(enough,noSatConflict);
            return static_cast<uint8_t>(_mm256_movemask_ps(_mm256_castsi256_ps(canAdd))) & liveMask;
        }

        // Explicit handoff suffix penalty. The same predicate is used by unified
        // avxCanAddLocalRowMask(...,missing); separated here to measure STATE32_DISPATCH.
        inline uint8_t state32DispatchHandoffLocalRowMask(const uint32_t row,const int missing,
                                             const __m256i Sv,const __m256i satV,
                                             const __m256i ssV,const uint8_t liveMask) const {
            if(!liveMask || missing>=k) return 0;
            const __m256i zero=_mm256_setzero_si256();
            const __m256i rowV=_mm256_set1_epi32(static_cast<int>(row));
            const __m256i degree=avxPopcnt8x32(_mm256_and_si256(rowV,Sv));
            const __m256i enough=_mm256_cmpgt_epi32(
                _mm256_add_epi32(degree,_mm256_set1_epi32(k-missing)),ssV);
            const __m256i noSat=_mm256_cmpeq_epi32(_mm256_andnot_si256(rowV,satV),zero);
            return static_cast<uint8_t>(_mm256_movemask_ps(_mm256_castsi256_ps(_mm256_and_si256(enough,noSat)))) & liveMask;
        }

        inline uint8_t forcedLeftForcedLeftReject(const __m256i Sv,const __m256i satV,
                                            const uint8_t live,const uint32_t unionS,const LaneContext32& ctx){
            ensureLeftNon32Transpose();
            // Active only for k<=ANYK_FAST_K_MAX through the caller's gate.
            // One init mask per k-plane and 32-Left chunk; reused over packets.
            const size_t chunks=static_cast<size_t>((leftSz+31)>>5);
            if(forcedLeftInitValid.size()<chunks) forcedLeftInitValid.resize(chunks,0);
            if(forcedLeftInitPlane.size()<chunks*static_cast<size_t>(k))
                forcedLeftInitPlane.resize(chunks*static_cast<size_t>(k));
            const auto nonRow=[&](const int u,const int chunk)->uint32_t{
                return leftNon32At(ctx.orig[u],chunk);
            };
            const uint8_t result=forced_left_forced_left2d::evaluate_cached(
                k,Sv,satV,live,unionS,leftSz,nonRow,
                [&](const int chunk){ensureForcedLeftInitPlane(chunk);},
                [&](const int t,const int chunk)->uint32_t{return initForcedLeftPlaneAt(t,chunk);});
            return result;
        }

        struct MaxReadyState32 { uint32_t S=0,X=0; };

        inline int steadyUBucket(const int u) const { return u<=8?0:(u<=16?1:(u<=24?2:3)); }

        LaneContext32 laneBuildContext(const uint64_t rootU,const uint64_t baseX){
            LaneContext32 ctx;
            ctx.activeLb=state32CoreActiveLb();
            ctx.rootMask=rootU;
            ctx.baseX=baseX;
            uint64_t t=rootU;
            while(t){ const int oi=bitFirst(t); t&=t-1; ctx.orig[ctx.m++]=oi; }
            const uint32_t all=(ctx.m==32?0xffffffffU:((1U<<ctx.m)-1U));
            for(int i=0;i<ctx.m;i++){
                // BMI2 PEXT maps the sparse Bit64 root universe directly into
                // the dense <=32 lane coordinate system in one instruction.
                ctx.adj[i]=laneCompress(bitAdj[ctx.orig[i]],ctx);
                ctx.nonadj[i]=all&~ctx.adj[i]&~(1U<<i);
                ctx.pair[i]=laneCompress(bitPair[ctx.orig[i]],ctx)|(1U<<i);
            }
            return ctx;
        }

        struct AvxPermTable {
            alignas(32) int idx[256][8];
            AvxPermTable(){
                for(int mask=0;mask<256;mask++){
                    int p=0;
                    for(int i=0;i<8;i++) if(mask&(1<<i)) idx[mask][p++]=i;
                    while(p<8) idx[mask][p++]=0;
                }
            }
        };

        // Unified Any-K defect builder for the State32 AVX refine path.
        // K<=ANYK_FAST_K_MAX uses the same parameterized algorithm with a
        // compile-time K; larger K uses the equivalent runtime automaton.

        template<int K>
        inline typename std::enable_if<(K>ANYK_FAST_K_MAX),void>::type
        dispatchAnyKDefectMasksTemplate(const __m256i Pv,const __m256i activeV,const uint32_t unionP,
                                         const LaneContext32& ctx,__m256i& badNoV,__m256i& satV){
            avxBuildDefectMasksDynamic(Pv,activeV,unionP,ctx,badNoV,satV);
        }

        template<int K>
        inline typename std::enable_if<(K<=ANYK_FAST_K_MAX),void>::type
        dispatchAnyKDefectMasksTemplate(const __m256i Pv,const __m256i activeV,const uint32_t unionP,
                                         const LaneContext32& ctx,__m256i& badNoV,__m256i& satV){
            if(k==K){
                avxBuildDefectMasksT<K>(Pv,activeV,unionP,ctx,badNoV,satV);
                return;
            }
            dispatchAnyKDefectMasksTemplate<K+1>(Pv,activeV,unionP,ctx,badNoV,satV);
        }

        struct StageSoAStatePool {
            vector<uint32_t> P,C,X;
            // bit7 = prechecked; bits0..5 = whole witness (0..31, 32=none).
            vector<uint8_t> meta;
            void reserve(size_t n){
                P.reserve(n); C.reserve(n); X.reserve(n); meta.reserve(n);
            }
            bool empty() const { return P.empty(); }
            size_t size() const { return P.size(); }
            void push(const LaneState32& s){
                P.push_back(s.P); C.push_back(s.C); X.push_back(s.X);
                meta.push_back(static_cast<uint8_t>((s.prechecked?0x80u:0u)
                    |(s.wholeWitness&0x3fu)));
            }
            LaneState32 pop(){
                const size_t i=P.size()-1;
                LaneState32 s;
                s.P=P[i]; s.C=C[i]; s.X=X[i];
                s.prechecked=(meta[i]&0x80u)!=0;
                s.wholeWitness=static_cast<uint8_t>(meta[i]&0x3fu);
                P.pop_back(); C.pop_back(); X.pop_back(); meta.pop_back();
                return s;
            }
        };

        // Only rematerialized subtrees use this audited pool. Ordinary/local-F
        // parent stacks keep their existing representation and lifetime.
        struct RematStatePool : StageSoAStatePool {
            using StageSoAStatePool::P;using StageSoAStatePool::C;
            using StageSoAStatePool::X;using StageSoAStatePool::meta;
            void noteCapacity(){
                auto& s=experiment_control::samples();
                const size_t cap=std::min(std::min(P.capacity(),C.capacity()),std::min(X.capacity(),meta.capacity()));
                s.pool_peak_capacity=std::max(s.pool_peak_capacity,static_cast<uint64_t>(cap));
            }
            void reserve(size_t n){
                const unsigned grows=(n>P.capacity())+(n>C.capacity())+(n>X.capacity())+(n>meta.capacity());
                if(grows){auto& s=experiment_control::samples();++s.pool_grow_events;s.pool_vector_grows+=grows;}
                StageSoAStatePool::reserve(n);noteCapacity();
            }
            void push(const LaneState32& st){
                const unsigned grows=(P.size()==P.capacity())+(C.size()==C.capacity())+(X.size()==X.capacity())+(meta.size()==meta.capacity());
                StageSoAStatePool::push(st);
                if(grows){auto& s=experiment_control::samples();++s.pool_grow_events;s.pool_vector_grows+=grows;noteCapacity();}
            }
            void reset(){P.clear();C.clear();X.clear();meta.clear();}
            bool fullyEmpty()const{return P.empty()&&C.empty()&&X.empty()&&meta.empty();}
        };
        struct RematWorkerPool {RematStatePool pool;bool borrowed=false,used=false;};
        static RematWorkerPool& rematWorkerPool(){static thread_local RematWorkerPool slot;return slot;}
        void runRematWithPool(const uint64_t P,const uint64_t C,const uint64_t X,const bool prechecked){
            auto& s=experiment_control::samples();++s.pool_acquisitions;
            if(reuseRematerializationStorage){
                auto& slot=rematWorkerPool();
                if(!slot.borrowed){
                    assert(slot.pool.fullyEmpty());
                    if(slot.used)++s.pool_reused;else {++s.pool_fresh;slot.used=true;}
                    struct Lease {
                        RematWorkerPool& slot;
                        explicit Lease(RematWorkerPool& p):slot(p){slot.borrowed=true;slot.pool.reset();}
                        ~Lease(){assert(slot.pool.fullyEmpty());slot.pool.reset();slot.borrowed=false;}
                    } lease(slot);
                    avxStageBatchedDFSImpl(P,C,X,slot.pool,prechecked,true);
                    return;
                }
                // A reentrant caller must never touch the borrowed parent pool.
                ++s.pool_fallbacks;
            }
            ++s.pool_fresh;
            RematStatePool stack;
            avxStageBatchedDFSImpl(P,C,X,stack,prechecked,true);
            assert(stack.fullyEmpty());
        }

        inline int workloadCBucket(const LaneState32& st) const {
            const int c=lanePop(st.C);
            return c<=4?0:(c<=8?1:(c<=16?2:3));
        }
        inline int workloadBucketId(const LaneState32& st) const {
#if WORKLOAD_BATCH_MODE == 1
            return workloadCBucket(st);
#elif WORKLOAD_BATCH_MODE == 2
            const int cb=workloadCBucket(st);
            const int u=lanePop(st.P|st.C);
            return cb + (u>=24?4:0);
#else
            (void)st; return 0;
#endif
        }
        inline int workloadBucketCount() const {
#if WORKLOAD_BATCH_MODE == 1
            return 4;
#elif WORKLOAD_BATCH_MODE == 2
            return 8;
#else
            return 1;
#endif
        }
        inline void profileReadyState(const LaneState32& st){
            (void)st;
        }
        inline void profileBranchBatchWork(const LaneState32 batch[8],const int active){
            (void)batch; (void)active;
        }

        template<class Pool>

        struct StageAoSStatePool {
            vector<LaneState32> v;
            void reserve(size_t n){v.reserve(n);} bool empty() const{return v.empty();} size_t size() const{return v.size();}
            void push(const LaneState32& s){v.push_back(s);} LaneState32 pop(){auto s=v.back();v.pop_back();return s;}
        };

        // Host adapter; these hooks do not alter State32 branching semantics.
        // State32 shared DFS keeps <32 active P/C coordinates. A local State64
