            // Dynamic seed scheduling is the first thread-level layer. A worker becomes a
            // Multithread-StateBit helper only after all seed indices have already been
            // claimed by the worker pool.
            while(true){
                const int idx=nextSeed.fetch_add(1,std::memory_order_relaxed);
                if(idx<targetSeeds){
                    processSeed(idx);
                    const int done=completedSeeds.fetch_add(1,std::memory_order_acq_rel)+1;
                    markSeedMilestones(done);
                    if(done==targetSeeds) taskPool.cv.notify_all();
                    continue;
                }

                taskPool.idleWorkers.fetch_add(1,std::memory_order_acq_rel);
                taskPool.noteIdleDemand();
                auto idleSince=std::chrono::steady_clock::now();
                while(true){
                    MTTask *task=taskPool.popLocal(tid);
                    bool wasStolen=false;
                    if(!task){
                        int victim=-1;
                        task=taskPool.steal(tid,victim);
                        wasStolen=(task!=nullptr);
                    }
                    if(task){
                        const auto gotTask=std::chrono::steady_clock::now();
                        taskPool.dds_idle_wait_ns.fetch_add(diagNs(idleSince,gotTask),std::memory_order_relaxed);
                        taskPool.dds_wait_satisfied.fetch_add(1,std::memory_order_relaxed);
                        taskPool.idleWorkers.fetch_sub(1,std::memory_order_acq_rel);
                        taskPool.consumeDemandReservation(task);
                        const auto taskStart=gotTask;
                        helperEnum.executePreparedMTTask(*task);
                        taskPool.recordRuntime(diagNs(taskStart,std::chrono::steady_clock::now()));
                        delete task;
                        taskPool.executed.fetch_add(1,std::memory_order_relaxed);
                        ++localHelpTasks;
                        if(wasStolen) ++localStolenTasks;
                        taskPool.idleWorkers.fetch_add(1,std::memory_order_acq_rel);
                        taskPool.noteIdleDemand();
                        idleSince=std::chrono::steady_clock::now();
                        continue;
                    }

                    const unsigned long long exportedNow=taskPool.exported.load(std::memory_order_acquire);
                    const unsigned long long executedNow=taskPool.executed.load(std::memory_order_acquire);
                    if(completedSeeds.load(std::memory_order_acquire)==targetSeeds &&
                       taskPool.queued.load(std::memory_order_acquire)==0 && exportedNow==executedNow){
                        taskPool.dds_idle_wait_ns.fetch_add(diagNs(idleSince,std::chrono::steady_clock::now()),std::memory_order_relaxed);
                        break;
                    }
                    std::unique_lock<std::mutex> lk(taskPool.waitMu);
                    taskPool.cv.wait_for(lk,std::chrono::milliseconds(2),[&]{
                        return taskPool.queued.load(std::memory_order_acquire)>0 ||
                               completedSeeds.load(std::memory_order_acquire)==targetSeeds;
                    });
                }
                taskPool.idleWorkers.fetch_sub(1,std::memory_order_acq_rel);
                break;
            }

