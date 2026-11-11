        struct State256Context {
            int m=0,fixed=0,extN=0;
            bool proper=true;
            int orig[256];
            Mask256 adj[256],nonadj[256],pair[256];
            vector<int> extOrig,extForcedAdj,leftForcedAdj;
            vector<Mask256> extAdj,leftAdj;
        };
        struct State256Node { Mask256 P,C,X; };


        State256Context state256BuildCurrentTask(){
            wide_cost::Stage costScope(wide_cost::Build256);
            const auto t0=std::chrono::steady_clock::now();
            State256Context ctx;
            statebit_wide_search::buildActive256(ctx,plex->sz,cand1->sz,subg.proper,UNLINK2EQUAL,
                [&](int i){return plex->members[i];},
                [&](int i){return cand1->members[i];},
                [&](int a,int b)->uint8_t{return commonMtx[static_cast<size_t>(a)*static_cast<size_t>(subg.n)+b];});
            assert(ctx.m>128 && ctx.m<=256);
            statebit_wide_search::buildExternal256(ctx,excl->sz,
                [&](int e){return excl->members[e];},
                [&](int a,int b){return isAdjMtx(a,b);});
            ctx.extForcedAdj.resize(ctx.extN);
            unsigned long long forcedExtChecks=0,forcedLeftEdges=0;
            for(int e=0;e<ctx.extN;e++){
                const int x=ctx.extOrig[e];int fa=0;
                for(const int v:forcedBase){++forcedExtChecks;if(isAdjMtx(x,v))++fa;}
                ctx.extForcedAdj[e]=fa;
            }
            ctx.leftForcedAdj.assign(static_cast<size_t>(leftSz),0);
            for(const int v:forcedBase)for(int z=0;z<subg.V[v].degreeLeft;z++){
                const int l=subg.V[v].NeighborsLeft[z];
                if(l>=0 && l<leftSz){++ctx.leftForcedAdj[l];++forcedLeftEdges;}
            }
            statebit_wide_search::buildLeft256(ctx,leftSz,[&](int v,const auto& emit){
                for(int z=0;z<subg.V[v].degreeLeft;z++)emit(subg.V[v].NeighborsLeft[z]);
            });
            ++state256Stats.builds;
            state256Stats.ext_rows+=ctx.extN;
            state256Stats.left_rows+=leftSz;
            state256Stats.forced_ext_checks+=forcedExtChecks;
            state256Stats.forced_left_edges+=forcedLeftEdges;
            state256Stats.build_ns+=diagNs(t0,std::chrono::steady_clock::now());
            return ctx;
        }


        bool state256IsMaximalAndEmit(const Mask256 S,const Mask256 X,const State256Context& ctx){
            return statebit_wide_search::maximal(S,X,ctx,k,leftSz,
                [&](int e){return forcedBaseMissingAdj(bitForcedCount,ctx.extForcedAdj[e]);},
                [&](int l){return forcedBaseMissingAdj(bitForcedCount,ctx.leftForcedAdj[l]);},
                [&](Mask256 selected,const State256Context& c){
#ifdef SET
                    const int old=plex->sz;
                    for(const int v:forcedBase)plex->push(v);
                    for(int i=c.fixed;i<c.m;i++)if(mask256Test(selected,i))plex->push(c.orig[i]);
                    emitVerifiedPlex(plex);while(plex->sz>old)plex->pop();
#else
                    emitter->emitPlex();
#endif
                    ++state256Stats.outputs;
                },false,false);
        }

        int state256ChoosePivot(const Mask256 P,const Mask256 C,const State256Context& ctx) const {
            return statebit_wide_search::pivot(P,C,ctx,k,bitActiveLb());
        }

        Mask256 state256RefineIncludeOnce(const int pivot,const Mask256 P,Mask256 C,const State256Context& ctx){
            ++state256Stats.cand_filter_calls;
            unsigned long long removed=0;
            const Mask256 result=statebit_wide_search::refine(pivot,P,C,ctx,k,bitActiveLb(),removed);
            state256Stats.cand_filter_removed+=removed;
            return result;
        }


        void state256HandoffState128(const State256Node& st,const State256Context& ctx){
            wide_cost::Stage costScope(wide_cost::Map256_128);
            State128Context c128;
            const bool hasSeedMatrix=(subg.n>0);
            const auto mat=[&](int a,int b,int i,int j)->uint8_t{
                if(hasSeedMatrix)return commonMtx[static_cast<size_t>(a)*static_cast<size_t>(subg.n)+b];
                return static_cast<uint8_t>((mask256Test(ctx.adj[i],j)?1:0)|(mask256Test(ctx.pair[i],j)?2:0));
            };
            const auto ext=[&](int a,int b,int e,int oi,int x)->bool{
                if(hasSeedMatrix)return (commonMtx[static_cast<size_t>(a)*static_cast<size_t>(subg.n)+b]&1)!=0;
                return e<ctx.extN?mask256Test(ctx.extAdj[e],oi):mask256Test(ctx.adj[x],oi);
            };
            const auto left=[&](int v,int oi,const auto& emit){
                if(hasSeedMatrix){for(int z=0;z<subg.V[v].degreeLeft;z++)emit(subg.V[v].NeighborsLeft[z]);}
                else for(int l=0;l<leftSz;l++) if(mask256Test(ctx.leftAdj[l],oi))emit(l);
            };
            const auto sel=wide_state_remap::convert256to128(st.P,st.C,st.X,ctx,c128,leftSz,mat,ext,left,UNLINK2EQUAL);
            c128.extForcedAdj.resize(c128.extN);
            for(int e=0;e<ctx.extN;e++)c128.extForcedAdj[e]=ctx.extForcedAdj[e];
            for(int e=ctx.extN;e<c128.extN;e++)c128.extForcedAdj[e]=bitForcedCount;
            c128.leftForcedAdj=ctx.leftForcedAdj;
            ++state256Stats.transitions128;
            ++state128Stats.eligible_nodes; ++state128Stats.builds;
            state128Stats.ext_rows+=c128.extN; state128Stats.left_rows+=leftSz;
            state128Stats.observeHandoff(sel.m,bitForcedCount);
            const unsigned long long cpu128=threadCpuNowNs();
            execute128Root({sel.P,sel.C,{}},c128);
            state128Stats.cpu_ns+=threadCpuNowNs()-cpu128;
        }

        void snapshotPreparedState256Task(PreparedState256Task& t,const State256Node& st,const State256Context& ctx) const {
            wide_cost::Stage costScope(wide_cost::Snapshot256);
            ++transferAudit.snapshot256;
#ifdef SET
            t.verifyMap=verifyMap;
#endif
            const auto t0=std::chrono::steady_clock::now();
            t.m=ctx.m; t.fixed=ctx.fixed; t.extN=ctx.extN; t.leftSz=leftSz; t.forcedCount=bitForcedCount; t.proper=ctx.proper;
            t.P=st.P; t.C=st.C; t.X=st.X;
            std::memcpy(t.orig,ctx.orig,sizeof(int)*256);
            std::memcpy(t.adj,ctx.adj,sizeof(Mask256)*256);
            std::memcpy(t.nonadj,ctx.nonadj,sizeof(Mask256)*256);
            std::memcpy(t.pair,ctx.pair,sizeof(Mask256)*256);
            t.extOrig=ctx.extOrig; t.extForcedAdj=ctx.extForcedAdj; t.leftForcedAdj=ctx.leftForcedAdj;
            t.extAdj=ctx.extAdj; t.leftAdj=ctx.leftAdj; t.forcedBase=forcedBase;
            state256Stats.snapshot_ns+=diagNs(t0,std::chrono::steady_clock::now());
        }

        bool tryExportState256Branch(const State256Node& st,const State256Context& ctx){
            if(mask256Pop(st.C)<=MT_EDBT_GRAIN_SIZE) return false;
            if(!mtSplitReady(MT_BIT_SPLIT_US)) return false;
            if(!mtAcquireSplitPermit()) return false;
            PreparedMTTask *task=new PreparedMTTask; task->kind=PreparedMTTask::STATE256; task->tailExport=false; mtTagSplitPolicy(task); task->splitTimedOut=mtSplitTimedOut;
            task->state256=new PreparedState256Task;
            snapshotPreparedState256Task(*task->state256,st,ctx);
            if(statebit_width_selection::noteExportOnSuccess(mtExportFn(mtPoolCtx,mtOwnerTid,task))){ mtCommitSplitPermit(); return true; }
            mtAbortSplitPermit(); delete task;
            return false;
        }

        void executePreparedState256Task(const PreparedMTTask& whole){
            wide_cost::Window costWindow(wide_cost::Resume256);
            wide_cost::Stage costScope(wide_cost::Restore256);
            assert(whole.state256); const PreparedState256Task& t=*whole.state256;
#ifdef SET
            verifyMap=t.verifyMap;
#endif
            mtTaskStart=std::chrono::steady_clock::now(); mtSplitTimedOut=whole.splitTimedOut;
            State256Context ctx; ctx.m=t.m; ctx.fixed=t.fixed; ctx.extN=t.extN; ctx.proper=t.proper; leftSz=t.leftSz; bitForcedCount=t.forcedCount; forcedBase=t.forcedBase;
            std::memcpy(ctx.orig,t.orig,sizeof(int)*256); std::memcpy(ctx.adj,t.adj,sizeof(Mask256)*256); std::memcpy(ctx.nonadj,t.nonadj,sizeof(Mask256)*256); std::memcpy(ctx.pair,t.pair,sizeof(Mask256)*256);
            ctx.extOrig=t.extOrig; ctx.extForcedAdj=t.extForcedAdj; ctx.leftForcedAdj=t.leftForcedAdj; ctx.extAdj=t.extAdj; ctx.leftAdj=t.leftAdj;
#ifdef SET
            plex->resize(ctx.m+bitForcedCount); plex->clear(); for(int i=0;i<ctx.fixed;i++) plex->push(ctx.orig[i]);
#endif
            const unsigned long long cpu256=threadCpuNowNs();
            {wide_cost::Stage searchScope(wide_cost::Search256);state256DFS({t.P,t.C,t.X},ctx);}
            state256Stats.cpu_ns+=threadCpuNowNs()-cpu256;
        }

        void state256DFS(const State256Node& st,const State256Context& ctx){
            statebit_wide_search::dfs<State256Node,State256Context,Mask256>(st,ctx,k,bitActiveLb(),128,
                [&](){++state256Stats.nodes;},
                [&](){++state256Stats.whole_calls;},
                [&](){++state256Stats.leaf_calls;},
                [&](Mask256 S,Mask256 X){state256IsMaximalAndEmit(S,X,ctx);},
                [&](const State256Node& n,const State256Context& c){state256HandoffState128(n,c);},
                [&](Mask256 P,Mask256 C,const State256Context& c){if(enableFoldingProbe)c0Opportunity.observe(256,mask256Pop(P|C),mask256Pop(C),bitForcedCount,0,[&](){
                    return c0Count(C,P|C,c.nonadj,[](Mask256 x){return mask256Any(x);},[](Mask256 x){return mask256First(x);},[](Mask256 x,int v){return mask256ClearBit(x,v);});
                });return state256ChoosePivot(P,C,c);},
                [&](int p,Mask256 P,Mask256 C,const State256Context& c){return state256RefineIncludeOnce(p,P,C,c);},
                [&](const State256Node& n,const State256Context& c){if(!tryExportState256Branch(n,c))state256DFS(n,c);},
                [&](const State256Node& n,const State256Context& c){state256DFS(n,c);});
        }

        void forcedBaseState256EnumerateCurrentTask(){
            wide_cost::Window costWindow(wide_cost::List256);
            wide_cost::Stage costScope(wide_cost::Search256);
            const int active=plex->sz+cand1->sz;
            assert(forcedBaseState256Eligible(active,FORCED_BASE_STATE256_THRESHOLD)); assert(!forcedBase.empty() || enableDirectWideEntry);
            const int savedForced=bitForcedCount; bitForcedCount=forcedBaseSize(); state256Stats.observeHandoff(active,bitForcedCount);
            const unsigned long long cpu256=threadCpuNowNs();
            const State256Context ctx=state256BuildCurrentTask(); const Mask256 all=mask256LowBits(ctx.m),P=mask256LowBits(ctx.fixed),C=mask256AndNot(all,P);
            state256DFS({P,C,{}},ctx);
            state256Stats.cpu_ns+=threadCpuNowNs()-cpu256;
            bitForcedCount=savedForced; defectCacheInvalidate();
        }

        static constexpr uint8_t PARALLEL_ENGINE_STATE64_WITNESS_NONE=64;
