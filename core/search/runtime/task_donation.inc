        // Tail-only State64 spill.  The normal 4x64 refill kernel keeps a private
        // LIFO stack and therefore cannot expose its remaining search states to idle
        // workers.  Once the global scheduler enters tail mode, materialize selected
        // State64 states as self-contained BIT tasks.  Keep at least one state local
        // so a producer never turns a single state into pure task migration.
        inline bool tryTailExportState64(const LaneState64& st){
            return false;
        }

        inline bool tryParallelExportState64(const LaneState64& st){
            return false;
        }

        template<class StackT>
        inline bool tryDTFBExportState64Frontier(StackT& stack){
            if(stack.size()<2) return false;
            if(!mtSplitReady(MT_BIT_SPLIT_US)) return false;
            vector<size_t> eligible; eligible.reserve(stack.size());
            // older half is donated as one batch.
            for(size_t i=0;i<stack.size();++i){
                if(bitPop(stack[i].C)>MT_EDBT_GRAIN_SIZE) eligible.push_back(i);
            }
            if(eligible.size()<2) return false;
            const size_t take=eligible.size()/2;
            if(take==0) return false;
            if(!mtDTFBClaimDemand()) return false;
            vector<size_t> selected; selected.reserve(take);
            vector<PreparedBitRoot> roots; roots.reserve(take);
            for(size_t j=0;j<take;++j){
                const size_t pos=eligible[j];
                selected.push_back(pos);
                mixed_f64::materialized(stack[pos].F,bitForcedCount);
                roots.push_back({stack[pos].P|stack[pos].F,stack[pos].C,stack[pos].X});
            }
            PreparedMTTask *task=new PreparedMTTask;
            task->kind=PreparedMTTask::BIT_BATCH;
            task->tailExport=false;
            task->dtfbExport=true;
            task->demandExport=true;
            task->splitTimedOut=false;
            snapshotPreparedBitTaskState(task->bit,roots[0].P,roots[0].C,roots[0].X);
            task->bitBatchRoots.swap(roots);
            if(!statebit_width_selection::noteExportOnSuccess(mtExportFn(mtPoolCtx,mtOwnerTid,task))){
                mtDTFBReleaseDemand();
                delete task; return false;
            }
            std::sort(selected.begin(),selected.end(),std::greater<size_t>());
            for(size_t pos:selected){
                if(pos+1!=stack.size()) stack[pos]=stack.back();
                stack.pop_back();
            }
            mtResetDTFBSplitEpoch();
            ++state64Stats.fresh64_dtfb_events;
            return true;
        }

        template<class StackT>
        inline int tryDMFSExportState64Frontier(StackT& stack){
            (void)stack; return 0;
        }

        template<class StackT>
        inline bool tryTailExportState64Batch(StackT& stack){
#if MT_TAIL_EXPANSION && MT_TAIL_BATCH_STATES > 0
            if(!mtTailSpillAllowed || stack.size()<=1) return false;
            if(!mtTailNeedWork()) return false;
            const size_t maxTake=std::min<size_t>(static_cast<size_t>(MT_TAIL_BATCH_STATES),stack.size()-1);
            vector<size_t> pos; pos.reserve(maxTake);
            vector<PreparedBitRoot> roots; roots.reserve(maxTake);
            for(size_t off=0;off<stack.size() && roots.size()<maxTake;++off){
                const size_t i=stack.size()-1-off;
                if(bitPop(stack[i].C)<MT_TAIL_BIT_MIN_C) continue;
                pos.push_back(i);
                roots.push_back({stack[i].P|stack[i].F,stack[i].C,stack[i].X});
            }
            if(roots.size()<static_cast<size_t>(MT_TAIL_BATCH_MIN_STATES)) return false;
            PreparedMTTask *task=new PreparedMTTask;
            task->kind=PreparedMTTask::BIT_BATCH;
            task->tailExport=true;
            task->splitTimedOut=true;
            snapshotPreparedBitTaskState(task->bit,roots[0].P,roots[0].C,roots[0].X);
            task->bitBatchRoots.swap(roots);
            if(!statebit_width_selection::noteExportOnSuccess(mtExportFn(mtPoolCtx,mtOwnerTid,task))){ delete task; return false; }
            // pos is descending, so swap-pop removal keeps remaining indices valid.
            for(size_t i:pos){
                if(i+1!=stack.size()) stack[i]=stack.back();
                stack.pop_back();
            }
            return true;
#else
            (void)stack; return false;
#endif
        }

        template<class StackT>
        inline void spillState64StackForParallelism(StackT& stack){
            (void)tryDTFBExportState64Frontier(stack);
        }

        // State64 packet execution delegates
        // to the single shared four-lane State64 refill state machine.
