        void snapshotPreparedBitTaskState(PreparedBitTask& t,const uint64_t P,const uint64_t C,const uint64_t X,const uint64_t localF=0) const {
            mixed_f64::materialized(localF,bitForcedCount);
#ifdef SET
            t.verifyMap=verifyMap;
#endif
            t.bitM=bitM; t.bitFixed=bitFixed; t.bitExtN=bitExtN; t.leftSz=leftSz; t.forcedCount=bitForcedCount;
            t.proper=bitTaskProper; t.P=P; t.C=C; t.X=X;
            std::memcpy(t.bitOrig,bitOrig,sizeof(int)*BIT_LIMIT);
            std::memcpy(t.bitAdj,bitAdj,sizeof(uint64_t)*BIT_LIMIT);
            std::memcpy(t.bitPair,bitPair,sizeof(uint64_t)*BIT_LIMIT);
            t.bitExtAdj.assign(bitExtAdj.begin(),bitExtAdj.begin()+bitExtN);
            t.bitLeftAdj.assign(bitLeftAdj.begin(),bitLeftAdj.begin()+leftSz);
            t.bitExtForcedAdj.assign(bitExtForcedAdj.begin(),bitExtForcedAdj.begin()+bitExtN);
            t.bitLeftForcedAdj.assign(bitLeftForcedAdj.begin(),bitLeftForcedAdj.begin()+leftSz);
            t.forcedBase=forcedBase;
        }
        void snapshotPreparedBitTask(PreparedBitTask& t) const {
            const uint64_t all=(bitM==64?~0ULL:((1ULL<<bitM)-1));
            const uint64_t P=(bitFixed==64?~0ULL:((1ULL<<bitFixed)-1));
            snapshotPreparedBitTaskState(t,P,all&~P,0);
        }
        void snapshotPreparedListTask(PreparedListTask& t,const int widthParentC){
            ++transferAudit.snapshot_list;
#ifdef SET
            t.verifyMap=verifyMap;
#endif
            flushLazyLeftLogicalAdj();
            t.subg=&subg; t.commonMtx=commonMtx; t.hopSz=hopSz; t.hop2Sz=hop2Sz; t.leftSz=leftSz;
            t.widthParentC=widthParentC; t.aggSeed=aggDiagSeedIndex; t.seedToken=mtSeedToken;
            t.plex.assign(plex->members,plex->members+plex->sz);
            t.cand1.assign(cand1->members,cand1->members+cand1->sz);
            t.cand2.assign(cand2->members,cand2->members+cand2->sz);
            t.excl.assign(excl->members,excl->members+excl->sz);
            t.exclBK.assign(exclBK->members,exclBK->members+exclBK->sz);
            t.exclStack.assign(exclStack->members,exclStack->members+exclStack->sz);
            const int n=subg.n; t.neiInP.assign(neiInP,neiInP+n); t.neiInG.assign(neiInG,neiInG+n); t.nonadjInP.assign(nonadjInP,nonadjInP+n);
            t.forcedBase=forcedBase; t.leftLogicalAdj=leftLogicalAdj.count;
            t.complement=complement; t.denseBookkeeping=denseBookkeeping; t.gOffset=gOffset; t.pOffset=pOffset; t.useGAsP=useGAsP;
            t.foldingGateEarlyOffGate=foldingGateEarlyOffGate;
        }
        bool tryExportListBranch(const int widthParentC){
            if(!mtSeedToken) return false;
            if(!foldingGateEarlyOffGate.decisionFinal()) return false;
            if(widthParentC<=MT_EDBT_GRAIN_SIZE) return false;
            if(!mtSplitReady(MT_LIST_SPLIT_US)) return false;
            if(!mtAcquireSplitPermit()) return false;
            PreparedMTTask *task=new PreparedMTTask; task->kind=PreparedMTTask::LIST; task->tailExport=false; mtTagSplitPolicy(task); task->splitTimedOut=mtSplitTimedOut;
            snapshotPreparedListTask(task->list,widthParentC);
            mtSeedToken->listOutstanding.fetch_add(1,std::memory_order_acq_rel);
            if(statebit_width_selection::noteExportOnSuccess(mtExportFn(mtPoolCtx,mtOwnerTid,task))){ mtCommitSplitPermit(); return true; }
            mtSeedToken->listOutstanding.fetch_sub(1,std::memory_order_acq_rel); mtAbortSplitPermit(); delete task;
            return false;
        }

        void loadPreparedBitContext(const PreparedBitTask& t){
#ifdef SET
            verifyMap=t.verifyMap;
#endif
            defectCacheInvalidate();
            bitM=t.bitM; bitFixed=t.bitFixed; bitExtN=t.bitExtN; leftSz=t.leftSz; bitForcedCount=t.forcedCount;
            bitTaskProper=t.proper; forcedBase=t.forcedBase;
            if(!t.forcedBase.empty())++table4Audit.bit_resume_f;
            std::memcpy(bitOrig,t.bitOrig,sizeof(int)*BIT_LIMIT);
            std::memcpy(bitAdj,t.bitAdj,sizeof(uint64_t)*BIT_LIMIT);
            std::memcpy(bitPair,t.bitPair,sizeof(uint64_t)*BIT_LIMIT);
            ensureBitAuxCapacity(bitExtN,leftSz);
            if(bitExtN>0){
                std::copy(t.bitExtAdj.begin(),t.bitExtAdj.end(),bitExtAdj.begin());
                std::copy(t.bitExtForcedAdj.begin(),t.bitExtForcedAdj.end(),bitExtForcedAdj.begin());
            }
            if(leftSz>0){
                std::copy(t.bitLeftAdj.begin(),t.bitLeftAdj.end(),bitLeftAdj.begin());
                std::copy(t.bitLeftForcedAdj.begin(),t.bitLeftForcedAdj.end(),bitLeftForcedAdj.begin());
            }
#ifdef SET
            plex->resize(bitM+bitForcedCount); plex->clear();
            for(int i=0;i<bitFixed;i++) plex->push(bitOrig[i]);
#endif
            const uint64_t all=(bitM==64?~0ULL:((1ULL<<bitM)-1));
            for(int i=0;i<bitM;i++) bitNon[i]=all&~bitAdj[i]&~(1ULL<<i);
            leftNon32Ready=false;
            leftChunks32=0;
        }

        void executePreparedBitTask(const PreparedMTTask& whole){
            const PreparedBitTask& t=whole.bit;
            mtTaskStart=std::chrono::steady_clock::now();
            mtSplitTimedOut=whole.splitTimedOut;
            loadPreparedBitContext(t);
            // Materialized State64 spill tasks re-enter the same 4x64/state32
            // ladder.  Standard tail tasks may suppress re-export; DDS tasks are
            // naturally bounded by live idle demand and may donate again.
            bit64Dispatch(t.P,t.C,t.X);
        }
        void executePreparedBitBatchTask(const PreparedMTTask& whole){
            const PreparedBitTask& t=whole.bit;
            mtTaskStart=std::chrono::steady_clock::now();
            mtSplitTimedOut=whole.splitTimedOut;
            loadPreparedBitContext(t);
            vector<LaneState64> stack;
            stack.reserve(std::max<size_t>(256,whole.bitBatchRoots.size()+8));
            for(const auto& r:whole.bitBatchRoots) stack.push_back({r.P,r.C,r.X});
            const bool prevTailSpillAllowed=mtTailSpillAllowed;
            mtTailSpillAllowed=false;
            avx4x64RefillStack(stack);
            mtTailSpillAllowed=prevTailSpillAllowed;
        }

        void executePreparedListTask(const PreparedMTTask& whole){
            const PreparedListTask& t=whole.list;
            const int n=t.subg->n;
            Stack<int> p(std::max(n+4,8)); VtxSet c1(std::max(n,1)),c2(std::max(n,1)),ex(std::max(n,1)),exbk(std::max(n,1)); Stack<int> exst(std::max(n,1));
            for(int v:t.plex) p.push(v); for(int v:t.cand1)c1.add(v); for(int v:t.cand2)c2.add(v); for(int v:t.excl)ex.add(v); for(int v:t.exclBK)exbk.add(v); for(int v:t.exclStack)exst.push(v);
            vector<int> nip=t.neiInP,nig=t.neiInG,non=t.nonadjInP;
            MaximalityChecker maxc(*t.subg); Emitter emit;
            SearchContext child(*t.subg,&maxc,&emit,&p,&c1,&c2,&ex,&exbk,&exst,nip.data(),nig.data(),non.data(),t.hopSz,t.hop2Sz,t.leftSz,t.commonMtx,t.complement,t.denseBookkeeping);
            child.gOffset=t.gOffset; child.pOffset=t.pOffset; child.useGAsP=t.useGAsP;
            child.forcedBase=t.forcedBase;
#ifdef SET
            child.verifyMap=t.verifyMap;
#endif
 child.leftLogicalAdj.count=t.leftLogicalAdj; child.setAggDiagSeedIndex(t.aggSeed);
            if(!t.forcedBase.empty() && t.plex.size()+t.cand1.size()>256)++table4Audit.list_resume_gt256;
            child.foldingGateEarlyOffGate=t.foldingGateEarlyOffGate;
            child.leftLogicalPendingDelta.assign(static_cast<size_t>(n),0); child.leftLogicalDirtyFlag.assign(static_cast<size_t>(n),0); child.leftLogicalDirtyVertices.clear(); child.leftLogicalDirtyVertices.reserve(static_cast<size_t>(n));
            child.enableMTRecursiveExport(mtPoolCtx,mtExportFn,mtIdleWorkers,mtOwnerTid,t.seedToken,
                                          mtNextSeed,mtCompletedSeeds,mtTargetSeeds,mtWorkerCount,
                                          mtExportedTasks,mtExecutedTasks,mtTailModeEntries,mtTailRefillRequests,mtTailEverEntered,
                                          mtDDSReservations,mtDDSDemandClaimed,mtDDSDemandReleased,mtPlusOneClaimFn,mtPlusOneReleaseFn,mtDMFSClaimFn,mtDMFSReleaseFn);
            child.mtCapacityFn=mtCapacityFn;
            child.mtSplitTimedOut=whole.splitTimedOut;
            child.listBranchWithParent(t.widthParentC);
            if(t.seedToken) t.seedToken->listOutstanding.fetch_sub(1,std::memory_order_acq_rel);
        }
        void executePreparedMTTask(const PreparedMTTask& t){
            mtSplitTimedOut=t.splitTimedOut;
            if(t.kind==PreparedMTTask::LIST) executePreparedListTask(t);
            else if(t.kind==PreparedMTTask::STATE128) executePreparedState128Task(t);
            else if(t.kind==PreparedMTTask::STATE256) executePreparedState256Task(t);
            else if(t.kind==PreparedMTTask::BIT_BATCH) executePreparedBitBatchTask(t);
            else executePreparedBitTask(t);
        }

