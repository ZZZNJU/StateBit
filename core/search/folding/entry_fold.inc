        bool tryEntryPreFold(){
            if(!enableLocalFolding || bitForcedCount!=0 || !forcedBase.empty() ||
               !folding_variant::entry(entryFoldingPolicy,plex->sz+cand1->sz,0))return false;
            auto& stats=experiment_control::samples();++stats.entry_fold_checks;stats.entry_fold_tested+=cand1->sz;
            // A rejected entry must preserve enumeration order as well as membership.
            // This entry is bounded by 64, so the snapshot needs no heap allocation.
            int originalCandidates[64];
            const int originalSize=cand1->sz;
            if(entryFoldingPolicy==3)
                std::copy(cand1->members,cand1->members+originalSize,originalCandidates);
            ForcedBaseFoldGuard guard(this);
            applyForcedBaseFold(guard.frame);
            if(!guard.frame.folded)return false;
            ++stats.entry_fold_nodes;stats.entry_fold_vertices+=guard.frame.folded;
            if(plex->sz+cand1->sz<=32)++stats.entry_fold_le32;else ++stats.entry_fold_gt32;
            if(!folding_variant::commit(entryFoldingPolicy,plex->sz+cand1->sz)){
                ++stats.entry_rejects;stats.entry_reject_vertices+=guard.frame.folded;
                restoreForcedBaseFold(guard.frame); // sets folded=0; guard becomes a no-op
                assert(cand1->sz==originalSize);
                for(int i=0;i<originalSize;++i){
                    const int v=originalCandidates[i];
                    cand1->members[i]=v;cand1->pos[v]=i;
                }
                return false;
            }
            ++stats.entry_commits;
            // Folded C0 remains in logical U. Build the existing outer-F context
            // before coarse export; child task snapshots own their fixed base.
            struct RestoreCount {
                SearchContext* owner;int saved;
                ~RestoreCount(){owner->bitForcedCount=saved;owner->defectCacheInvalidate();}
            } restore{this,bitForcedCount};
            bitForcedCount=forcedBaseSize();defectCacheInvalidate();
            if(cand1->sz==0)checkAsMaxPlex();
            else bitEnumerateCurrentTask(); // outer F makes this a single prepass
            return true;
        }

        void bitEnumerateCurrentTask(){
            ++bitStats.terminal_calls;
            bitFixed=plex->sz;
            bitM=bitFixed+cand1->sz;
            if(bitM>BIT_LIMIT){
                ++bitStats.fallback_calls;
                listBranch();
                return;
            }
            if(tryEntryPreFold())return;
            ++wordKernelStats.selected_runtime;

            // Coarse bit-task split.  Standard ENGINE_BASE keeps the EDBT-style proactive
            // timeout behavior.  DDS groups require a live idle-worker reservation
            // before materializing work, and LIST_ONLY disables bit donation here.
            if(mtExportFn && cand1->sz>MT_EDBT_GRAIN_SIZE && mtSplitReady(MT_BIT_SPLIT_US)){
                if(mtAcquireSplitPermit()){
                    bitBuildCurrentTaskRuntime();
                    PreparedMTTask *task=new PreparedMTTask; task->kind=PreparedMTTask::BIT; task->tailExport=false; mtTagSplitPolicy(task); task->splitTimedOut=mtSplitTimedOut;
                    snapshotPreparedBitTask(task->bit);
                    if(statebit_width_selection::noteExportOnSuccess(mtExportFn(mtPoolCtx,mtOwnerTid,task))){ mtCommitSplitPermit(); return; }
                    mtAbortSplitPermit(); delete task;
                    bitExecuteBuiltTask();
                    return;
                }
            }
            bitEnumerateCurrentTaskRuntime();
        }

        void solveCurrentCandidateSpace(){
            bitEnumerateCurrentTask();
        }

