        // Low-overhead progress reporting.  The reporter only reads atomics
        // that already exist on the scheduling path; there are no new
        // per-solution atomics and no new atomics in the seed hot path.
        std::atomic<bool> progressStop{false};
        std::mutex progressMu;
        std::condition_variable progressCv;
        std::thread progressThread;
        const auto progressEpoch=std::chrono::steady_clock::now();
        std::atomic<long long> t90_us{-1},t99_us{-1},t999_us{-1};
        auto markSeedMilestones = [&](const int done){
            const long long us=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-progressEpoch).count();
            auto setFirst=[&](std::atomic<long long>& dst){ long long expected=-1; dst.compare_exchange_strong(expected,us,std::memory_order_acq_rel); };
            if(targetSeeds<=0){ setFirst(t90_us); setFirst(t99_us); setFirst(t999_us); return; }
            if(static_cast<long long>(done)*100LL>=static_cast<long long>(targetSeeds)*90LL) setFirst(t90_us);
            if(static_cast<long long>(done)*100LL>=static_cast<long long>(targetSeeds)*99LL) setFirst(t99_us);
            if(static_cast<long long>(done)*1000LL>=static_cast<long long>(targetSeeds)*999LL) setFirst(t999_us);
        };

        auto startProgressReporter = [&](){
            if(progressIntervalSeconds<=0) return;
            progressThread=std::thread([&](){
                using Clock=std::chrono::steady_clock;
                std::deque<std::pair<Clock::time_point,int>> samples;
                samples.emplace_back(Clock::now(),completedSeeds.load(std::memory_order_relaxed));
                fprintf(stderr,"MT_PROGRESS_CONFIG interval_sec=%d eta_window_sec=60 live_output_count=0 overhead_mode=atomic_reads_only\n",progressIntervalSeconds);
                fflush(stderr);
                while(true){
                    std::unique_lock<std::mutex> lk(progressMu);
                    const bool stop=progressCv.wait_for(lk,std::chrono::seconds(progressIntervalSeconds),[&]{
                        return progressStop.load(std::memory_order_acquire);
                    });
                    lk.unlock();
                    if(stop) break;

                    const auto now=Clock::now();
                    const int done=std::min(targetSeeds,completedSeeds.load(std::memory_order_acquire));
                    const int claimed=std::min(targetSeeds,nextSeed.load(std::memory_order_relaxed));
                    const int idle=std::max(0,std::min(actualThreads,taskPool.idleWorkers.load(std::memory_order_relaxed)));
                    const int active=actualThreads-idle;
                    const auto queued=taskPool.queued.load(std::memory_order_relaxed);
                    const auto exported=taskPool.exported.load(std::memory_order_relaxed);
                    const auto executed=taskPool.executed.load(std::memory_order_relaxed);
                    const auto stolen=taskPool.stolen.load(std::memory_order_relaxed);
                    const unsigned long long inflight=(exported>=executed+queued)?(exported-executed-queued):0ULL;
                    const unsigned long long outstanding=(exported>=executed)?(exported-executed):0ULL;
                    const int ddsReserved=taskPool.dds_reservations.load(std::memory_order_relaxed);
                    const unsigned long long ddsUnmet=taskPool.ddsUnmetDemand();
                    const int remaining=std::max(0,targetSeeds-done);
                    const int tailMode=(MT_TAIL_EXPANSION && claimed>=targetSeeds && remaining<=MT_TAIL_REMAINING_SEEDS && idle>=MT_TAIL_MIN_IDLE)?1:0;
                    const unsigned long long tailTarget=tailMode?std::min<unsigned long long>(
                        static_cast<unsigned long long>(MT_TAIL_MAX_OUTSTANDING_MULT)*static_cast<unsigned long long>(actualThreads),
                        static_cast<unsigned long long>(MT_TAIL_WATERLINE_MULT)*static_cast<unsigned long long>(std::max(1,idle))):0ULL;

                    samples.emplace_back(now,done);
                    while(samples.size()>2 &&
                          std::chrono::duration<double>(now-samples.front().first).count()>60.0){
                        samples.pop_front();
                    }
                    double rate=0.0;
                    if(samples.size()>=2){
                        const double dt=std::chrono::duration<double>(now-samples.front().first).count();
                        const int dd=done-samples.front().second;
                        if(dt>0.0 && dd>0) rate=static_cast<double>(dd)/dt;
                    }
                    const double pct=targetSeeds?100.0*static_cast<double>(done)/static_cast<double>(targetSeeds):100.0;
                    const double elapsed=std::chrono::duration<double>(now-progressEpoch).count();
                    const char *phase=(done<targetSeeds)?"seed_search":((queued>0 || inflight>0)?"draining_tasks":"finalizing");
                    char etaBuf[32];
                    if(done>=targetSeeds){
                        snprintf(etaBuf,sizeof(etaBuf),"0");
                    }else if(rate>0.0){
                        const double eta=static_cast<double>(targetSeeds-done)/rate;
                        snprintf(etaBuf,sizeof(etaBuf),"%.1f",eta);
                    }else{
                        snprintf(etaBuf,sizeof(etaBuf),"NA");
                    }
                    fprintf(stderr,
                            "MT_PROGRESS elapsed_sec=%.1f phase=%s seeds_claimed=%d/%d seeds_completed=%d/%d seed_progress=%.3f%% active_workers=%d/%d idle_workers=%d queued_tasks=%llu inflight_tasks=%llu outstanding_tasks=%llu tail_mode=%d tail_target=%llu dds_enabled=%d dds_reserved=%d dds_unmet=%llu exported_tasks=%llu executed_tasks=%llu stolen_tasks=%llu seed_rate_60s=%.3f seed_eta_sec=%s\n",
                            elapsed,phase,claimed,targetSeeds,done,targetSeeds,pct,active,actualThreads,idle,
                            queued,inflight,outstanding,tailMode,tailTarget,MT_DDS_DEMAND_SPLIT,ddsReserved,ddsUnmet,exported,executed,stolen,rate,etaBuf);
                    fflush(stderr);
                }
            });
        };
        auto stopProgressReporter = [&](){
            if(!progressThread.joinable()) return;
            progressStop.store(true,std::memory_order_release);
            progressCv.notify_all();
            progressThread.join();
        };

