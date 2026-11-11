    using MTEnum = SearchContext<GlobalMaximalityChecker,PlexEmitter>;
    using MTTask = MTEnum::PreparedMTTask;

    struct MTTaskPool {
        struct Queue {
            std::mutex mu;
            std::deque<MTTask*> dq;
            std::atomic<size_t> occupancy{0};
        };
        std::vector<std::unique_ptr<Queue>> queues;
        const size_t queueCap;
        std::atomic<unsigned long long> queued{0};
        std::atomic<unsigned long long> exported{0};
        std::atomic<unsigned long long> rejected_full{0};
        std::atomic<unsigned long long> executed{0};
        std::atomic<unsigned long long> stolen{0};
        std::atomic<unsigned long long> local_pops{0};
        std::atomic<unsigned long long> peak_queued{0};
        std::atomic<unsigned long long> exported_list{0},exported_bit{0},exported_bitbatch{0},exported_state128{0},exported_state256{0},stolen_list{0},stolen_bit{0},stolen_bitbatch{0},stolen_state128{0},stolen_state256{0};
        std::atomic<unsigned long long> dtfb_state64_batch_tasks{0},dtfb_state64_batch_states{0},dtfb_state64_batch_max{0},dtfb_state64_batch_stolen{0};
        std::atomic<unsigned long long> tail_mode_entries{0},tail_refill_requests{0};
        std::atomic<unsigned long long> tail_exported_list{0},tail_exported_bit{0},tail_exported_bitbatch_tasks{0},tail_exported_bitbatch_states{0},tail_exported_state128{0},tail_exported_state256{0};
        std::atomic<unsigned long long> tail_bitbatch_max_states{0};
        std::atomic<unsigned long long> runtime_lt10us{0},runtime_lt100us{0},runtime_lt1ms{0},runtime_lt10ms{0},runtime_lt100ms{0},runtime_ge100ms{0};
        std::atomic<unsigned long long> tail_idle_ns{0};
        std::atomic<bool> tailEverEntered{false};
        std::atomic<int> idleWorkers{0};
        std::atomic<int> dds_reservations{0};
        std::atomic<unsigned long long> dds_demand_posted{0},dds_demand_claimed{0},dds_demand_released{0};
        std::atomic<unsigned long long> dds_materialized{0},dds_donated_list{0},dds_donated_bit{0},dds_donated_state128{0},dds_donated_state256{0};
        std::atomic<unsigned long long> dds_peak_unmet{0},dds_idle_wait_ns{0},dds_wait_satisfied{0};
        std::atomic<int> plus_one_supply{0};
        std::atomic<unsigned long long> plus_one_claimed{0},plus_one_released{0},plus_one_materialized{0};
        std::atomic<unsigned long long> plus_one_claim_idle0{0},plus_one_claim_idlepos{0},plus_one_peak_supply{0},plus_one_peak_target{0};
        std::atomic<unsigned long long> plus_one_idle_entries{0},plus_one_idle_wait_ns{0},plus_one_wait_satisfied{0};
        std::atomic<int> dmfs_supply{0},dmfs_credits{0};
        std::atomic<unsigned long long> dmfs_claim_calls{0},dmfs_claimed{0},dmfs_released{0},dmfs_materialized{0};
        std::atomic<unsigned long long> dmfs_permit_uses{0},dmfs_credit_hits{0},dmfs_abandoned_credits{0},dmfs_peak_credits{0};
        std::atomic<unsigned long long> dmfs_claim_idle0{0},dmfs_claim_idlepos{0},dmfs_peak_supply{0},dmfs_peak_target{0};
        std::atomic<unsigned long long> dmfs_bulk_claim_calls{0},dmfs_bulk_claimed{0},dmfs_frontier_materialized{0},dmfs_max_burst{0};
        std::atomic<unsigned long long> dmfs_idle_entries{0},dmfs_idle_wait_ns{0},dmfs_wait_satisfied{0};
        std::mutex waitMu;
        std::condition_variable cv;

        MTTaskPool(const int n,const size_t cap):queueCap(cap){
            queues.reserve(n);
            for(int i=0;i<n;i++) queues.emplace_back(new Queue());
        }
        bool canMaterialize(const int owner) const {
            return queueCap==0 || queues[owner]->occupancy.load(std::memory_order_acquire)<queueCap;
        }
        unsigned long long ddsUnmetDemand() const {
            const int idle=idleWorkers.load(std::memory_order_acquire);
            const int reserved=dds_reservations.load(std::memory_order_acquire);
            return static_cast<unsigned long long>(std::max(0,idle-reserved));
        }
        void updateDDSPeak(){
            const unsigned long long v=ddsUnmetDemand();
            unsigned long long old=dds_peak_unmet.load(std::memory_order_relaxed);
            while(v>old && !dds_peak_unmet.compare_exchange_weak(old,v,std::memory_order_relaxed)){}
        }
        void noteIdleDemand(){
            dds_demand_posted.fetch_add(1,std::memory_order_relaxed);
            updateDDSPeak();
        }
        void consumeDemandReservation(const MTTask *task){
            if(task && task->demandExport){
                const int old=dds_reservations.fetch_sub(1,std::memory_order_acq_rel);
                assert(old>0);
                dds_demand_released.fetch_add(1,std::memory_order_relaxed);
                updateDDSPeak();
            }
        }
        void updatePlusOnePeak(std::atomic<unsigned long long>& dst,const unsigned long long v){
            unsigned long long old=dst.load(std::memory_order_relaxed);
            while(v>old && !dst.compare_exchange_weak(old,v,std::memory_order_relaxed)){}
        }
        bool claimDemandPlusOne(){
            return true;
        }
        void releaseDemandPlusOne(){
        }
        int takeDMFSCredits(const int maxCount){
            (void)maxCount;
            return 0;
        }
        int claimDMFS(const int maxCount){
            (void)maxCount; return 0;
        }
        void releaseDMFS(const int count){
            (void)count;
        }
        void releaseUnusedDMFSCredits(){
        }
        void updatePeak(const unsigned long long v){
            unsigned long long old=peak_queued.load(std::memory_order_relaxed);
            while(v>old && !peak_queued.compare_exchange_weak(old,v,std::memory_order_relaxed)){}
        }
        bool submit(const int owner,MTTask *task){
            // EDBT-style proactive timeout decomposition: timed-out recursive
            // tasks are materialized even before a helper becomes idle.  This
            // leaves stealable work ready when stragglers emerge.
            // Snapshot producer-owned metadata before publication.  Once the
            // queue lock is released, another worker may steal, execute and
            // delete task immediately.
            const int taskKind=task->kind;
            const bool taskTailExport=task->tailExport;
            const bool taskDTFB=task->dtfbExport;
            const unsigned long long taskBatchStates=(taskKind==MTTask::BIT_BATCH)?static_cast<unsigned long long>(task->bitBatchRoots.size()):0ULL;
            Queue &q=*queues[owner];
            unsigned long long now=0;
            {
                std::lock_guard<std::mutex> lk(q.mu);
                if(queueCap>0 && q.dq.size()>=queueCap){
                    rejected_full.fetch_add(1,std::memory_order_relaxed);
                    return false;
                }
                q.dq.push_back(task);
                q.occupancy.store(q.dq.size(),std::memory_order_release);
                // Commit queue/accounting state while task is still protected
                // by q.mu.  A consumer cannot decrement queued before this
                // matching increment becomes visible.
                now=queued.fetch_add(1,std::memory_order_release)+1;
                exported.fetch_add(1,std::memory_order_relaxed);
                if(taskKind==MTTask::LIST) exported_list.fetch_add(1,std::memory_order_relaxed);
                else if(taskKind==MTTask::STATE128) exported_state128.fetch_add(1,std::memory_order_relaxed);
                else if(taskKind==MTTask::STATE256) exported_state256.fetch_add(1,std::memory_order_relaxed);
                else if(taskKind==MTTask::BIT_BATCH) exported_bitbatch.fetch_add(1,std::memory_order_relaxed);
                else exported_bit.fetch_add(1,std::memory_order_relaxed);
                if(taskDTFB && taskKind==MTTask::BIT_BATCH){
                    dtfb_state64_batch_tasks.fetch_add(1,std::memory_order_relaxed);
                    dtfb_state64_batch_states.fetch_add(taskBatchStates,std::memory_order_relaxed);
                    unsigned long long old=dtfb_state64_batch_max.load(std::memory_order_relaxed);
                    while(taskBatchStates>old && !dtfb_state64_batch_max.compare_exchange_weak(old,taskBatchStates,std::memory_order_relaxed)){}
                }
                if(task->plusOneExport) plus_one_materialized.fetch_add(1,std::memory_order_relaxed);
                if(task->dmfsExport){
                    dmfs_materialized.fetch_add(1,std::memory_order_relaxed);
                    if(task->dmfsFrontierExport) dmfs_frontier_materialized.fetch_add(1,std::memory_order_relaxed);
                }
                if(task->demandExport){
                    dds_materialized.fetch_add(1,std::memory_order_relaxed);
                    if(taskKind==MTTask::LIST) dds_donated_list.fetch_add(1,std::memory_order_relaxed);
                    else if(taskKind==MTTask::STATE128) dds_donated_state128.fetch_add(1,std::memory_order_relaxed);
                    else if(taskKind==MTTask::STATE256) dds_donated_state256.fetch_add(1,std::memory_order_relaxed);
                    else dds_donated_bit.fetch_add(1,std::memory_order_relaxed);
                }
                if(taskTailExport){
                    if(taskKind==MTTask::LIST) tail_exported_list.fetch_add(1,std::memory_order_relaxed);
                    else if(taskKind==MTTask::STATE128) tail_exported_state128.fetch_add(1,std::memory_order_relaxed);
                    else if(taskKind==MTTask::STATE256) tail_exported_state256.fetch_add(1,std::memory_order_relaxed);
                    else if(taskKind==MTTask::BIT_BATCH){
                        tail_exported_bitbatch_tasks.fetch_add(1,std::memory_order_relaxed);
                        tail_exported_bitbatch_states.fetch_add(taskBatchStates,std::memory_order_relaxed);
                        unsigned long long old=tail_bitbatch_max_states.load(std::memory_order_relaxed);
                        while(taskBatchStates>old && !tail_bitbatch_max_states.compare_exchange_weak(old,taskBatchStates,std::memory_order_relaxed)){}
                    } else tail_exported_bit.fetch_add(1,std::memory_order_relaxed);
                }
            }
            // task is now published; do not dereference it below
            updatePeak(now);
            cv.notify_one();
            return true;
        }
        MTTask* popLocal(const int tid){
            Queue &q=*queues[tid];
            MTTask *task=nullptr;
            {
                std::lock_guard<std::mutex> lk(q.mu);
                if(!q.dq.empty()){
                    task=q.dq.back();
                    q.dq.pop_back();
                    q.occupancy.store(q.dq.size(),std::memory_order_release);
                }
            }
            if(task){
                queued.fetch_sub(1,std::memory_order_acq_rel);
                if(task->plusOneExport) releaseDemandPlusOne();
                if(task->dmfsExport) releaseDMFS(1);
                local_pops.fetch_add(1,std::memory_order_relaxed);
            }
            return task;
        }
        MTTask* steal(const int tid,int& victim){
            const int n=static_cast<int>(queues.size());
            for(int off=1;off<n;off++){
                const int v=(tid+off)%n;
                Queue &q=*queues[v];
                MTTask *task=nullptr;
                {
                    std::lock_guard<std::mutex> lk(q.mu);
                    if(!q.dq.empty()){
                        task=q.dq.front();
                        q.dq.pop_front();
                        q.occupancy.store(q.dq.size(),std::memory_order_release);
                    }
                }
                if(task){
                    queued.fetch_sub(1,std::memory_order_acq_rel);
                    if(task->plusOneExport) releaseDemandPlusOne();
                    if(task->dmfsExport) releaseDMFS(1);
                    stolen.fetch_add(1,std::memory_order_relaxed);
                    if(task->kind==MTTask::LIST) stolen_list.fetch_add(1,std::memory_order_relaxed);
                    else if(task->kind==MTTask::STATE128) stolen_state128.fetch_add(1,std::memory_order_relaxed);
                    else if(task->kind==MTTask::STATE256) stolen_state256.fetch_add(1,std::memory_order_relaxed);
                    else if(task->kind==MTTask::BIT_BATCH) stolen_bitbatch.fetch_add(1,std::memory_order_relaxed);
                    else stolen_bit.fetch_add(1,std::memory_order_relaxed);
                    if(task->dtfbExport && task->kind==MTTask::BIT_BATCH) dtfb_state64_batch_stolen.fetch_add(1,std::memory_order_relaxed);
                    victim=v;
                    return task;
                }
            }
            victim=-1;
            return nullptr;
        }
        void recordRuntime(const unsigned long long ns){
            if(ns<10000ULL) runtime_lt10us.fetch_add(1,std::memory_order_relaxed);
            else if(ns<100000ULL) runtime_lt100us.fetch_add(1,std::memory_order_relaxed);
            else if(ns<1000000ULL) runtime_lt1ms.fetch_add(1,std::memory_order_relaxed);
            else if(ns<10000000ULL) runtime_lt10ms.fetch_add(1,std::memory_order_relaxed);
            else if(ns<100000000ULL) runtime_lt100ms.fetch_add(1,std::memory_order_relaxed);
            else runtime_ge100ms.fetch_add(1,std::memory_order_relaxed);
        }
        ~MTTaskPool(){
            for(auto &qp:queues){
                std::lock_guard<std::mutex> lk(qp->mu);
                for(MTTask *t:qp->dq) delete t;
                qp->dq.clear();
            }
        }
    };

    static bool mtCapacityThunk(void *ctx,const int owner){
        return static_cast<MTTaskPool*>(ctx)->canMaterialize(owner);
    }
    static bool mtSubmitThunk(void *ctx,const int owner,MTTask *task){
        return static_cast<MTTaskPool*>(ctx)->submit(owner,task);
    }
    static bool mtPlusOneClaimThunk(void *ctx){
        return static_cast<MTTaskPool*>(ctx)->claimDemandPlusOne();
    }
    static void mtPlusOneReleaseThunk(void *ctx){
        static_cast<MTTaskPool*>(ctx)->releaseDemandPlusOne();
    }
    static int mtDMFSClaimThunk(void *ctx,const int maxCount){
        return static_cast<MTTaskPool*>(ctx)->claimDMFS(maxCount);
    }
    static void mtDMFSReleaseThunk(void *ctx,const int count){
        static_cast<MTTaskPool*>(ctx)->releaseDMFS(count);
    }

