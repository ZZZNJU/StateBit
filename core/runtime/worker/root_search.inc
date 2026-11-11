                const auto diagSeedStart=std::chrono::steady_clock::now();
                ++bestDiagStats.seed_count;
                ++localSeeds;
                const int vstart=dseq[idx];
                blk.clear();
                left.clear();

                const auto diagDecompStart=std::chrono::steady_clock::now();
                const int hopSz=decompose(peelG,vstart,blk,left,dpos,visited,count,neibors,neiborsLeft);
                bestDiagStats.decompose_ns+=diagNs(diagDecompStart,std::chrono::steady_clock::now());
                const int blkSz=static_cast<int>(blk.size());
                if(blkSz<lb){
                    bestDiagStats.seed_ns_max=std::max(bestDiagStats.seed_ns_max,diagNs(diagSeedStart,std::chrono::steady_clock::now()));
                    return;
                }
                ++bestDiagStats.valid_seed_count;

                const int hop2Sz=blkSz-hopSz;
                const int leftSz=static_cast<int>(left.size());
                ++localValid;
                localMaxHop=std::max(localMaxHop,hopSz);
                localMaxBlk=std::max(localMaxBlk,blkSz);
                localMaxLeft=std::max(localMaxLeft,leftSz);
                if(hopSz>CAP_LIMIT1 || blkSz>CAP_LIMIT2 || leftSz>CAP_LIMIT2) ++localLargeSeeds;

                const auto diagWorkspaceStart=std::chrono::steady_clock::now();
                // Grow all outer bookkeeping before any seed-local write.
                plex.resize(hopSz+k+4);
                cand1.resize(std::max(1,hopSz));
                cand2.resize(std::max(1,blkSz));
                excl.resize(std::max(1,blkSz));
                exclBK.resize(std::max(1,blkSz));
                exclStack.resize(std::max(1,blkSz));
                if(static_cast<int>(neiInP.size())<blkSz) neiInP.resize(blkSz,0);
                if(static_cast<int>(neiInG.size())<blkSz) neiInG.resize(blkSz,0);
                if(static_cast<int>(nonadjInP.size())<blkSz) nonadjInP.resize(blkSz,0);
                std::fill(neiInP.begin(),neiInP.begin()+blkSz,0);
                std::fill(nonadjInP.begin(),nonadjInP.begin()+blkSz,0);
                uint8_t *matrix=ensureCommonMtx(blkSz);
                localMaxMatrixBytes=std::max(localMaxMatrixBytes,commonMtxCap);

                bestDiagStats.workspace_ns+=diagNs(diagWorkspaceStart,std::chrono::steady_clock::now());
                const auto diagSubgraphStart=std::chrono::steady_clock::now();
                for(const auto u: blk) inBlk[u]=true;
                for(const auto u: left) inLeft[u]=true;
                Graph subg=buildSubgraph(peelG,blk,left,inBlk,inLeft,hopSz,visited,matrix);
                if(subg.proper) buildCommonMtx(subg,blkSz,hopSz,matrix);
                bestDiagStats.subgraph_ns+=diagNs(diagSubgraphStart,std::chrono::steady_clock::now());

                // selector and only builds ComplementCSR for seeds selected dense.
                ComplementCSR complement;
                bool denseBookkeeping=false;
                unsigned long long neighborProxy=0, nonNeighborProxy=0;
                ++localSeedMode.seeds;
                for(int j=0;j<subg.n;++j) neighborProxy+=static_cast<unsigned long long>(subg.V[j].degree);
                {
                    const unsigned long long allPairs=static_cast<unsigned long long>(subg.n)*static_cast<unsigned long long>(subg.n-1);
                    nonNeighborProxy=allPairs-neighborProxy;
                }
                denseBookkeeping=chooseDenseMode(neighborProxy,nonNeighborProxy);
                localSeedMode.neighbor_proxy+=neighborProxy; localSeedMode.nonneighbor_proxy+=nonNeighborProxy;
                if(adaptiveWouldDense(neighborProxy,nonNeighborProxy)) ++localSeedMode.predicted_dense_seeds; else ++localSeedMode.predicted_sparse_seeds;
                if(denseBookkeeping){
                    ++localSeedMode.actual_dense_seeds;
                    localSeedMode.actual_dense_vertices+=static_cast<unsigned long long>(subg.n);
                    localSeedMode.actual_dense_n2+=static_cast<unsigned long long>(subg.n)*static_cast<unsigned long long>(subg.n);
                    localSeedMode.dense_neighbor_proxy+=neighborProxy;
                    localSeedMode.dense_nonneighbor_proxy+=nonNeighborProxy;
                    const auto cb0=std::chrono::steady_clock::now();
                    complement.build(subg);
                    const auto cb1=std::chrono::steady_clock::now();
                    ++localSeedMode.complement_build_calls;
                    localSeedMode.complement_build_vertices+=static_cast<unsigned long long>(subg.n);
                    localSeedMode.complement_build_entries+=complement.entries();
                    localSeedMode.complement_build_ns+=static_cast<unsigned long long>(std::chrono::duration_cast<std::chrono::nanoseconds>(cb1-cb0).count());
                }else{
                    ++localSeedMode.actual_sparse_seeds;
                    localSeedMode.actual_sparse_vertices+=static_cast<unsigned long long>(subg.n);
                    localSeedMode.actual_sparse_n2+=static_cast<unsigned long long>(subg.n)*static_cast<unsigned long long>(subg.n);
                    localSeedMode.sparse_neighbor_proxy+=neighborProxy;
                    localSeedMode.sparse_nonneighbor_proxy+=nonNeighborProxy;
                }

                const auto diagSearchStart=std::chrono::steady_clock::now();
                unsigned long long seedLeftDegreeSum=0;
                for(int j=0;j<subg.n;j++){
                    neiInG[j]=subg.V[j].degreeHop;
                    if(j<hopSz){
                        cand1.add(j);
                    }else cand2.add(j);
                }

                GlobalMaximalityChecker maxchecker(subg);
                PlexEmitter emitter;
                MTEnum::MTSeedToken seedToken;
                MTEnum listor(
                    subg,&maxchecker,&emitter,&plex,&cand1,&cand2,&excl,&exclBK,&exclStack,
                    neiInP.data(),neiInG.data(),nonadjInP.data(),hopSz,hop2Sz,leftSz,matrix,
                    denseBookkeeping ? &complement : nullptr,denseBookkeeping);
                listor.setAggDiagSeedIndex(idx);
#ifdef SET
                auto mapping=std::make_shared<std::vector<int>>();mapping->reserve(blk.size());
                for(int v:blk)mapping->push_back(verifyPeelOriginal[v]);
                listor.verifyMap=mapping;
#endif

                listor.mtCapacityFn=mtCapacityThunk;
                if(actualThreads>1) listor.enableMTRecursiveExport(&taskPool,mtSubmitThunk,&taskPool.idleWorkers,tid,&seedToken,
                                                                  &nextSeed,&completedSeeds,targetSeeds,actualThreads,
                                                                  &taskPool.exported,&taskPool.executed,&taskPool.tail_mode_entries,&taskPool.tail_refill_requests,&taskPool.tailEverEntered,
                                                                  &taskPool.dds_reservations,&taskPool.dds_demand_claimed,&taskPool.dds_demand_released,nullptr,nullptr,mtDMFSClaimThunk,mtDMFSReleaseThunk);
                listor.cand1ToPlex(0)->kSearch(k-1);
                localFoldingEarlyOff.observe(listor.getFoldingEarlyOffGate());
                // A list task references this seed's immutable subgraph/common matrix.
                // Help drain work until every list descendant finishes before freeing the seed.
                while(seedToken.listOutstanding.load(std::memory_order_acquire)>0){
                    MTTask *task=taskPool.popLocal(tid); bool wasStolen=false;
                    if(!task){ int victim=-1; task=taskPool.steal(tid,victim); wasStolen=(task!=nullptr); }
                    if(task){
                        taskPool.consumeDemandReservation(task);
                        const auto taskStart=std::chrono::steady_clock::now();
                        helperEnum.executePreparedMTTask(*task);
                        const auto taskNs=diagNs(taskStart,std::chrono::steady_clock::now());
                        taskPool.recordRuntime(taskNs);
                        delete task; taskPool.executed.fetch_add(1,std::memory_order_relaxed);
                        ++localHelpTasks; if(wasStolen) ++localStolenTasks;
                    }else std::this_thread::yield();
                }
                bestDiagStats.search_ns+=diagNs(diagSearchStart,std::chrono::steady_clock::now());

                for(const auto u: blk) inBlk[u]=false;
                for(const auto u: left) inLeft[u]=false;
                listor.plexToCand1();
                cand1.clear();
                cand2.clear();
                subg.del();
                bestDiagStats.seed_ns_max=std::max(bestDiagStats.seed_ns_max,diagNs(diagSeedStart,std::chrono::steady_clock::now()));
