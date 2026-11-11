        // Multithread-StateBit / recursive straggler splitting.
        // Bit tasks are fully self-contained. List tasks reference the immutable
        // seed subgraph and are kept alive by MTSeedToken until all descendants finish.
        struct MTSeedToken { std::atomic<int> listOutstanding{0}; };
        struct PreparedBitRoot { uint64_t P=0,C=0,X=0; };
        struct PreparedBitTask {
#ifdef SET
            std::shared_ptr<const std::vector<int>> verifyMap;
#endif
            int bitM=0, bitFixed=0, bitExtN=0, leftSz=0, forcedCount=0;
            bool proper=true;
            uint64_t P=0,C=0,X=0;
            int bitOrig[BIT_LIMIT];
            uint64_t bitAdj[BIT_LIMIT];
            uint64_t bitPair[BIT_LIMIT];
            vector<uint64_t> bitExtAdj;
            vector<uint64_t> bitLeftAdj;
            vector<int> bitExtForcedAdj, bitLeftForcedAdj, forcedBase;
        };
        struct PreparedState128Task {
#ifdef SET
            std::shared_ptr<const std::vector<int>> verifyMap;
#endif
            int m=0,fixed=0,extN=0,leftSz=0,forcedCount=0;
            bool proper=true;
            Mask128 P{},C{},X{};
            int orig[128];
            Mask128 adj[128],nonadj[128],pair[128];
            vector<int> extOrig,extForcedAdj,leftForcedAdj,forcedBase;
            vector<Mask128> extAdj,leftAdj;
        };
        struct PreparedState256Task {
#ifdef SET
            std::shared_ptr<const std::vector<int>> verifyMap;
#endif
            int m=0,fixed=0,extN=0,leftSz=0,forcedCount=0;
            bool proper=true;
            Mask256 P{},C{},X{};
            int orig[256];
            Mask256 adj[256],nonadj[256],pair[256];
            vector<int> extOrig,extForcedAdj,leftForcedAdj,forcedBase;
            vector<Mask256> extAdj,leftAdj;
        };
        struct PreparedListTask {
#ifdef SET
            std::shared_ptr<const std::vector<int>> verifyMap;
#endif
            const Graph *subg=nullptr;
            const uint8_t *commonMtx=nullptr;
            int hopSz=0,hop2Sz=0,leftSz=0,widthParentC=-1,aggSeed=-1;
            MTSeedToken *seedToken=nullptr;
            vector<int> plex,cand1,cand2,excl,exclBK,exclStack;
            vector<int> neiInP,neiInG,nonadjInP,forcedBase,leftLogicalAdj;
            const ComplementCSR *complement=nullptr;
            bool denseBookkeeping=false;
            int gOffset=0,pOffset=0;
            bool useGAsP=false;
            SeedFoldingEarlyOffGate foldingGateEarlyOffGate;
        };
        struct PreparedMTTask {
            enum Kind:int { BIT=1, LIST=2, STATE128=3, STATE256=4, BIT_BATCH=5 } kind=BIT;
            bool tailExport=false;
            bool demandExport=false;
            bool plusOneExport=false;
            bool dmfsExport=false;
            bool dmfsFrontierExport=false;
            bool dtfbExport=false;
            bool state64Spill=false;
            bool splitTimedOut=false;
            PreparedBitTask bit;
            vector<PreparedBitRoot> bitBatchRoots;
            PreparedListTask list;
            PreparedState128Task state128;
            PreparedState256Task *state256=nullptr;
            ~PreparedMTTask(){ delete state256; }
        };
        using MTExportFn = bool(*)(void*,int,PreparedMTTask*);
        using MTCapacityFn = bool(*)(void*,int);
        using MTPlusOneClaimFn = bool(*)(void*);
        using MTPlusOneReleaseFn = void(*)(void*);
        using MTDMFSClaimFn = int(*)(void*,int);
        using MTDMFSReleaseFn = void(*)(void*,int);
        void *mtPoolCtx=nullptr;
        MTExportFn mtExportFn=nullptr;
        MTCapacityFn mtCapacityFn=nullptr;
        MTPlusOneClaimFn mtPlusOneClaimFn=nullptr;
        MTPlusOneReleaseFn mtPlusOneReleaseFn=nullptr;
        MTDMFSClaimFn mtDMFSClaimFn=nullptr;
        MTDMFSReleaseFn mtDMFSReleaseFn=nullptr;
        std::atomic<int> *mtIdleWorkers=nullptr;
        std::atomic<int> *mtNextSeed=nullptr;
        std::atomic<int> *mtCompletedSeeds=nullptr;
        std::atomic<unsigned long long> *mtExportedTasks=nullptr;
        std::atomic<unsigned long long> *mtExecutedTasks=nullptr;
        std::atomic<unsigned long long> *mtTailModeEntries=nullptr;
        std::atomic<unsigned long long> *mtTailRefillRequests=nullptr;
        std::atomic<bool> *mtTailEverEntered=nullptr;
        std::atomic<int> *mtDDSReservations=nullptr;
        std::atomic<unsigned long long> *mtDDSDemandClaimed=nullptr;
        std::atomic<unsigned long long> *mtDDSDemandReleased=nullptr;
        int mtTargetSeeds=0, mtWorkerCount=1;
        int mtOwnerTid=-1;
        MTSeedToken *mtSeedToken=nullptr;
        bool bitTaskProper=true;
        bool mtTailSpillAllowed=true;
        mutable bool mtSplitTimedOut=false;
        std::chrono::steady_clock::time_point mtTaskStart=std::chrono::steady_clock::now();

        void enableMTRecursiveExport(void *ctx,MTExportFn fn,std::atomic<int> *idleCounter,const int ownerTid,MTSeedToken *seedToken=nullptr,
                                     std::atomic<int> *nextSeedCounter=nullptr,std::atomic<int> *completedSeedCounter=nullptr,const int targetSeeds=0,const int workerCount=1,
                                     std::atomic<unsigned long long> *exportedCounter=nullptr,std::atomic<unsigned long long> *executedCounter=nullptr,
                                     std::atomic<unsigned long long> *tailModeEntries=nullptr,std::atomic<unsigned long long> *tailRefillRequests=nullptr,std::atomic<bool> *tailEverEntered=nullptr,
                                     std::atomic<int> *ddsReservations=nullptr,std::atomic<unsigned long long> *ddsDemandClaimed=nullptr,
                                     std::atomic<unsigned long long> *ddsDemandReleased=nullptr,
                                     MTPlusOneClaimFn plusOneClaimFn=nullptr,MTPlusOneReleaseFn plusOneReleaseFn=nullptr,
                                     MTDMFSClaimFn dmfsClaimFn=nullptr,MTDMFSReleaseFn dmfsReleaseFn=nullptr){
            mtPoolCtx=ctx; mtExportFn=fn; mtIdleWorkers=idleCounter; mtOwnerTid=ownerTid; mtSeedToken=seedToken;
            mtNextSeed=nextSeedCounter; mtCompletedSeeds=completedSeedCounter; mtTargetSeeds=targetSeeds; mtWorkerCount=std::max(1,workerCount);
            mtExportedTasks=exportedCounter; mtExecutedTasks=executedCounter; mtTailModeEntries=tailModeEntries; mtTailRefillRequests=tailRefillRequests; mtTailEverEntered=tailEverEntered;
            mtDDSReservations=ddsReservations; mtDDSDemandClaimed=ddsDemandClaimed; mtDDSDemandReleased=ddsDemandReleased;
            mtPlusOneClaimFn=plusOneClaimFn; mtPlusOneReleaseFn=plusOneReleaseFn;
            mtDMFSClaimFn=dmfsClaimFn; mtDMFSReleaseFn=dmfsReleaseFn;
            mtTaskStart=std::chrono::steady_clock::now();
            mtSplitTimedOut=false;
        }
        void enableMTTerminalExport(void *ctx,MTExportFn fn,std::atomic<int> *idleCounter,const int ownerTid){
            enableMTRecursiveExport(ctx,fn,idleCounter,ownerTid,nullptr);
        }
        inline bool mtTailCondition() const {
            return false;
        }
        inline bool mtTailNeedWork() const {
            return false;
        }
        inline bool mtDDSClaimDemand() const {
            return true;
        }
        inline void mtDDSReleaseDemand() const {
        }
        inline bool mtDTFBHasDemand() const {
            if(!mtIdleWorkers || !mtDDSReservations) return false;
            const int idle=mtIdleWorkers->load(std::memory_order_acquire);
            const int reserved=mtDDSReservations->load(std::memory_order_relaxed);
            return reserved<idle;
        }
        inline bool mtDTFBClaimDemand() const {
            if(!mtIdleWorkers || !mtDDSReservations) return false;
            int reserved=mtDDSReservations->load(std::memory_order_relaxed);
            while(true){
                const int idle=mtIdleWorkers->load(std::memory_order_acquire);
                if(reserved>=idle) return false;
                if(mtDDSReservations->compare_exchange_weak(reserved,reserved+1,std::memory_order_acq_rel,std::memory_order_relaxed)){
                    if(mtDDSDemandClaimed) mtDDSDemandClaimed->fetch_add(1,std::memory_order_relaxed);
                    return true;
                }
            }
        }
        inline void mtDTFBReleaseDemand() const {
            if(mtDDSReservations){
                const int old=mtDDSReservations->fetch_sub(1,std::memory_order_acq_rel);
                assert(old>0);
            }
            if(mtDDSDemandReleased) mtDDSDemandReleased->fetch_add(1,std::memory_order_relaxed);
        }
        inline void mtResetDTFBSplitEpoch(){
            mtTaskStart=std::chrono::steady_clock::now();
            mtSplitTimedOut=false;
        }
        inline int mtAcquireSplitPermits(const int maxCount) const {
            if(maxCount<=0) return 0;
            if(enableQueuePrecheck && mtCapacityFn){
                ++transferAudit.early_calls;
                if(!mtCapacityFn(mtPoolCtx,mtOwnerTid)){++transferAudit.early_skips;return 0;}
            }
            return mtDDSClaimDemand() ? 1 : 0;
        }
        inline bool mtAcquireSplitPermit() const {
            return mtAcquireSplitPermits(1)==1;
        }
        inline void mtAbortSplitPermits(const int count) const {
            if(count<=0) return;
            for(int i=0;i<count;i++) mtDDSReleaseDemand();
        }
        inline void mtAbortSplitPermit() const { mtAbortSplitPermits(1); }
        inline void mtCommitSplitPermit() const {
            // Reservation becomes queued ready work on publication and is
            // released only when a worker consumes the published task.
        }
        inline void mtTagSplitPolicy(PreparedMTTask *task) const {
            task->demandExport=MT_DDS_DEMAND_SPLIT;
            task->plusOneExport=MT_DEMAND_PLUS_ONE_SPLIT;
            task->dmfsExport=MT_DMFS_SPLIT;
        }


        inline bool mtSplitReady(const int usec) const {
            if(!mtExportFn) return false;
            // Match the EDBT timeout semantics: once the current task has
            // crossed tau_time it remains decomposable throughout all deeper
            // descendants.  Splitting is proactive and does not wait for an
            // already-idle helper.
            if(mtSplitTimedOut) return true;
            if(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-mtTaskStart).count()>=usec){
                mtSplitTimedOut=true;
                return true;
            }
            return false;
        }

