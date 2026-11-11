        const Graph &subg;
        // Borrow the immutable seed graph.  Descendant LIST tasks snapshot
        // &subg, so this must refer to the seed-owned Graph rather than a
        // stack-local shallow copy inside a helper child enumor.
        MaximalityChecker *plexMaxChecker;
        Emitter *emitter;
        int * neiInP;
        int * const neiInG;
        Stack<int> * const plex;//Plex
        VtxSet * const cand1;
        VtxSet * const cand2;
        VtxSet *excl;
        VtxSet * const exclBK;
        Stack<int> * const exclStack;
        const int hopSz, hop2Sz;
        int leftSz;
        const uint8_t* const commonMtx;

        int * nonadjInP;
        // DENSE_BOOKKEEPING+EXTERNAL_AGGREGATION fusion: dense-mode counters store base values plus global offsets.
        // The complement belongs to the current seed and remains alive until all
        // descendant LIST tasks for that seed have drained.
        const ComplementCSR *complement=nullptr;
        bool denseBookkeeping=false;
        int gOffset=0;
        int pOffset=0;
        bool useGAsP=false;

        static constexpr int BIT_LIMIT=64;
        int bitM=0,bitFixed=0,bitExtN=0;
        int bitOrig[BIT_LIMIT];
        uint64_t bitAdj[BIT_LIMIT],bitNon[BIT_LIMIT],bitPair[BIT_LIMIT];


        // Large-seed-safe dynamic task-side arrays.  The StateBit active domain
        // remains <=64 (and <=32 in the SIMD lane kernel); only the external
        // exclusion/left bookkeeping grows with the actual seed.
        vector<int> bitExtOrig;
        vector<uint64_t> bitExtAdj;
        vector<uint64_t> bitLeftAdj;

        // Fixed-point maintenance: task-level transpose of the left adjacency relation. Rows are
        // original Bit64 coordinates (0..bitM-1); columns are 32-candidate
        // chunks. Flattened storage avoids any compile-time seed-size cap.
        vector<uint32_t> leftNon32;
        bool leftNon32Ready=false;
        int leftChunks32=0;
        // Private to this enumor (each parallel worker owns its own enumor).
        // Keyed to one immutable Bit64/ForcedBase context and reset upon
        // context transition. No shared mutable cache or cross-task reuse.
        std::vector<uint8_t> forcedLeftInitValid;
        std::vector<uint32_t> forcedLeftInitPlane;
        inline void invalidateForcedLeftInitCache(){
            std::fill(forcedLeftInitValid.begin(),forcedLeftInitValid.end(),uint8_t(0));
        }
        inline uint32_t initForcedLeftPlaneAt(const int t,const int c) const {
            return forcedLeftInitPlane[static_cast<size_t>(c)*static_cast<size_t>(k)+static_cast<size_t>(t)];
        }
        inline void ensureForcedLeftInitPlane(const int c){
            if(forcedLeftInitValid[static_cast<size_t>(c)]) return;
            const size_t off=static_cast<size_t>(c)*static_cast<size_t>(k);
            for(int t=0;t<k;t++) forcedLeftInitPlane[off+static_cast<size_t>(t)]=0;
            const int lo=c*32,hi=std::min(leftSz,lo+32);
            for(int l=lo;l<hi;l++){
                const int missing=defectCacheLeftMissing(l);
                if(missing>=0 && missing<k)
                    forcedLeftInitPlane[off+static_cast<size_t>(missing)] |= uint32_t(1)<<(l-lo);
            }
            forcedLeftInitValid[static_cast<size_t>(c)]=1;
        }

        // Local-row cache: reusable, generation-stamped local-coordinate row cache for
        // maximality suffix candidates. External/left rows are dynamic.
        uint32_t suffixBaseLocal[BIT_LIMIT];
        vector<uint32_t> suffixExtLocal, suffixLeftLocal;
        uint64_t suffixBaseStamp[BIT_LIMIT]{};
        vector<uint64_t> suffixExtStamp, suffixLeftStamp;
        uint64_t suffixGeneration=1;
        vector<int> bitNeiScratch;

        // Independent ForcedBase: vertices proven universal in the current U=P|F|C.
        // They are logically fixed into every maximal extension, but deliberately do
        // not enter the physical P container or neiInP maintenance.
        vector<int> forcedBase;
        vector<int> forcedSolutionScratch;

        // ForcedBase-aware direct State32 handoff.  The active bit universe
        // contains only physical P plus residual C; forcedBase stays outside
        // the bit coordinates and contributes logically through bitForcedCount.
        int bitForcedCount=0;
        inline void defectCacheInvalidate(){
            invalidateForcedLeftInitCache();
        }
        inline void defectCachePrepareState32(){}
        inline int defectCacheExtMissing(int e) const {((void)0); return bitForcedMissing(bitExtForcedAdj[e]);}
        inline int defectCacheLeftMissing(int l) const {((void)0); return bitForcedMissing(bitLeftForcedAdj[l]);}

        // State32 handoff-local effective bound; never mutate global lb in parallel.
        bool state32DispatchState32HandoffActive=false;
        int state32DispatchState32HandoffLb=0;
        struct STATE32_DISPATCHState32HandoffGuard {
            SearchContext *owner;
            bool oldActive;
            int oldLb;
            explicit STATE32_DISPATCHState32HandoffGuard(SearchContext *p)
                :owner(p),oldActive(p->state32DispatchState32HandoffActive),oldLb(p->state32DispatchState32HandoffLb){
            }
            ~STATE32_DISPATCHState32HandoffGuard(){
            }
        };
        vector<int> bitExtForcedAdj;
        vector<int> bitLeftForcedAdj;

        // adjacency from each left/maximality row to the current logical
        // U=P|ForcedBase|C.  It is maintained only in modes that need the
        // left aggregation formula; B0/EXTERNAL_AGGREGATION pay no maintenance cost.
        LeftLogicalAdjTracker leftLogicalAdj;
        vector<int> leftLogicalPendingDelta;
        vector<int> leftLogicalDirtyVertices;
        vector<uint8_t> leftLogicalDirtyFlag;
#ifdef SET
        std::shared_ptr<const std::vector<int>> verifyMap;
        void emitVerifiedPlex(Stack<int>* selected){
            if(!verifyMap)std::abort();
            std::vector<int> local(selected->members,selected->members+selected->sz);
            for(int v:forcedBase)if(std::find(local.begin(),local.end(),v)==local.end())local.push_back(v);
            std::vector<int> global;global.reserve(local.size());
            for(int v:local){if(v<0||static_cast<size_t>(v)>=verifyMap->size())std::abort();global.push_back((*verifyMap)[v]);}
            std::sort(global.begin(),global.end());
            if(std::adjacent_find(global.begin(),global.end())!=global.end())std::abort();
            emitter->emitVerified(global);
        }
#endif
        int scalarListFallbackDepth=0;
        inline bool inScalarListFallback() const { return scalarListFallbackDepth>0; }
        int aggDiagSeedIndex=-1;
        unsigned long long aggHandoffSeq=0;
        SeedFoldingEarlyOffGate foldingGateEarlyOffGate;
        inline void setAggDiagSeedIndex(const int idx){ aggDiagSeedIndex=idx; }
        inline const SeedFoldingEarlyOffGate& getFoldingEarlyOffGate() const { return foldingGateEarlyOffGate; }
        inline void tryCommitFoldingEarlyOff(){ foldingGateEarlyOffGate.tryCommitOff(forcedBase.empty()); }
        inline void observeFoldingHandoff(){ foldingGateEarlyOffGate.observeT4Handoff(); }
        inline bool foldingGateOptimizationEnabled() const {
            return foldingGateEarlyOffGate.optimizationEnabled();
        }
        inline bool foldingGateForcedSemanticsActive() const {
            return foldingGateOptimizationEnabled() || !forcedBase.empty();
        }

