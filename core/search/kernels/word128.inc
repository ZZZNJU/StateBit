        struct State128Context : simd_base_shared128::CommonContext {
            // Fixed-base adjacency counts for external and left-side vertices.
            vector<int> extForcedAdj,leftForcedAdj;
        };
        struct State128Node { Mask128 P,C,X; };
        std::vector<State128Node> simd128Stack; // executor-owned; drained before the next wide root

        inline Mask128 state128Saturated(const Mask128 P,const State128Context& ctx) const {
            return simd_base_shared128::saturated(P,ctx,k);
        }

        State128Context state128BuildCurrentTask(){
            wide_cost::Stage costScope(wide_cost::Build128);
            const auto diagStart=std::chrono::steady_clock::now();
            ++bestDiagStats.state128_build_calls;
            State128Context ctx;
            wide_state_context_builder::buildActive(ctx,plex->sz,cand1->sz,subg.proper,UNLINK2EQUAL,
                [&](int i){return plex->members[i];},
                [&](int i){return cand1->members[i];},
                [&](int x)->const uint8_t* {
                    return commonMtx+static_cast<size_t>(x)*static_cast<size_t>(subg.n);
                });
            assert(ctx.m>64 && ctx.m<=128);
            wide_state_context_builder::buildExternal(ctx,excl->sz,
                [&](int e){return excl->members[e];},
                [&](int x,int y){return isAdjMtx(x,y);});
            ctx.extForcedAdj.resize(ctx.extN);
            unsigned long long forcedExtChecks=0,forcedLeftEdges=0;
            for(int e=0;e<ctx.extN;e++){
                int fa=0;const int x=ctx.extOrig[e];
                for(const int f:forcedBase){++forcedExtChecks;if(isAdjMtx(x,f))++fa;}
                ctx.extForcedAdj[e]=fa;
            }
            ctx.leftForcedAdj.assign(static_cast<size_t>(leftSz),0);
            for(const int f:forcedBase){
                for(int z=0;z<subg.V[f].degreeLeft;z++){
                    const int l=subg.V[f].NeighborsLeft[z];
                    if(l>=0 && l<leftSz){++ctx.leftForcedAdj[l];++forcedLeftEdges;}
                }
            }
            wide_state_context_builder::buildLeft(ctx,leftSz,[&](int v,const auto& consume){
                for(int z=0;z<subg.V[v].degreeLeft;z++) consume(subg.V[v].NeighborsLeft[z]);
            });
            ++state128Stats.builds;
            state128Stats.ext_rows+=static_cast<unsigned long long>(ctx.extN);
            state128Stats.left_rows+=static_cast<unsigned long long>(leftSz);
            state128Stats.forced_ext_checks+=forcedExtChecks;
            state128Stats.forced_left_edges+=forcedLeftEdges;
            bestDiagStats.state128_build_ns+=diagNs(diagStart,std::chrono::steady_clock::now());
            return ctx;
        }

        inline bool state128CanAddInternal(const int x,const Mask128 S,const Mask128 sat,const State128Context& ctx) const {
            return simd_base_shared128::canAddInternal(x,S,sat,ctx,k);
        }

        inline bool state128CanAddExternal(const int e,const Mask128 S,const Mask128 sat,const State128Context& ctx) const {
            return simd_base_shared128::canAddExternal(e,S,sat,ctx,k,forcedBaseMissingAdj(bitForcedCount,ctx.extForcedAdj[e]));
        }

        bool state128IsMaximalAndEmit(const Mask128 S,const Mask128 X,const State128Context& ctx){
            return simd_base_shared128::maximal(S,X,ctx,k,leftSz,
                [&](int e){ return forcedBaseMissingAdj(bitForcedCount,ctx.extForcedAdj[e]); },
                [&](int l){ return forcedBaseMissingAdj(bitForcedCount,ctx.leftForcedAdj[l]); },
                [&](Mask128 selected,const State128Context& source){
#ifdef SET
                    const int old=plex->sz;
                    for(const int v:forcedBase) plex->push(v);
                    for(int i=source.fixed;i<source.m;i++) if(mask128Test(selected,i)) plex->push(source.orig[i]);
                    emitVerifiedPlex(plex);
                    while(plex->sz>old) plex->pop();
#else
                    emitter->emitPlex();
#endif
                    ++state128Stats.outputs;
                });
        }

        int state128ChoosePivot(const Mask128 P,const Mask128 C,const State128Context& ctx) const {
            return simd_base_shared128::choosePivot(P,C,ctx,k,bitActiveLb());
        }

        Mask128 state128RefineIncludeOnce(const int pivot,const Mask128 P,Mask128 C,const State128Context& ctx){
            ++state128Stats.cand_filter_calls;
            unsigned long long removed=0;
            const Mask128 r=simd_base_shared128::refineIncludeOnce(pivot,P,C,ctx,k,bitActiveLb(),removed);
            state128Stats.cand_filter_removed+=removed;
            return r;
        }

        uint64_t state128MapMask64(const Mask128 src,const int oldOfNew[64],const int m64) const {
            uint64_t out=0;
            for(int j=0;j<m64;j++) if(mask128Test(src,oldOfNew[j])) out|=1ULL<<j;
            return out;
        }

        void snapshotPreparedState128Task(PreparedState128Task& t,const State128Node& st,const State128Context& ctx) const {
            wide_cost::Stage costScope(wide_cost::Snapshot128);
            ++transferAudit.snapshot128;
#ifdef SET
            t.verifyMap=verifyMap;
#endif
            t.m=ctx.m; t.fixed=ctx.fixed; t.extN=ctx.extN; t.leftSz=leftSz; t.forcedCount=bitForcedCount; t.proper=ctx.proper;
            t.P=st.P; t.C=st.C; t.X=st.X;
            std::memcpy(t.orig,ctx.orig,sizeof(int)*128);
            std::memcpy(t.adj,ctx.adj,sizeof(Mask128)*128);
            std::memcpy(t.nonadj,ctx.nonadj,sizeof(Mask128)*128);
            std::memcpy(t.pair,ctx.pair,sizeof(Mask128)*128);
            t.extOrig=ctx.extOrig; t.extForcedAdj=ctx.extForcedAdj; t.leftForcedAdj=ctx.leftForcedAdj;
            t.extAdj=ctx.extAdj; t.leftAdj=ctx.leftAdj; t.forcedBase=forcedBase;
        }

        bool tryExportState128Branch(const State128Node& st,const State128Context& ctx){
            if(mask128Pop(st.C)<=MT_EDBT_GRAIN_SIZE) return false;
            if(!mtSplitReady(MT_BIT_SPLIT_US)) return false;
            if(!mtAcquireSplitPermit()) return false;
            PreparedMTTask *task=new PreparedMTTask; task->kind=PreparedMTTask::STATE128; task->tailExport=false; mtTagSplitPolicy(task); task->splitTimedOut=mtSplitTimedOut;
            snapshotPreparedState128Task(task->state128,st,ctx);
            if(statebit_width_selection::noteExportOnSuccess(mtExportFn(mtPoolCtx,mtOwnerTid,task))){ mtCommitSplitPermit(); return true; }
            mtAbortSplitPermit(); delete task;
            return false;
        }

        void executePreparedState128Task(const PreparedMTTask& whole){
            wide_cost::Window costWindow(wide_cost::Resume128);
            wide_cost::Stage costScope(wide_cost::Restore128);
            const PreparedState128Task& t=whole.state128;
#ifdef SET
            verifyMap=t.verifyMap;
#endif
            mtTaskStart=std::chrono::steady_clock::now();
            mtSplitTimedOut=whole.splitTimedOut;
            State128Context ctx;
            ctx.m=t.m; ctx.fixed=t.fixed; ctx.extN=t.extN; ctx.proper=t.proper; leftSz=t.leftSz; bitForcedCount=t.forcedCount; forcedBase=t.forcedBase;
            std::memcpy(ctx.orig,t.orig,sizeof(int)*128);
            std::memcpy(ctx.adj,t.adj,sizeof(Mask128)*128);
            std::memcpy(ctx.nonadj,t.nonadj,sizeof(Mask128)*128);
            std::memcpy(ctx.pair,t.pair,sizeof(Mask128)*128);
            ctx.extOrig=t.extOrig; ctx.extForcedAdj=t.extForcedAdj; ctx.leftForcedAdj=t.leftForcedAdj;
            ctx.extAdj=t.extAdj; ctx.leftAdj=t.leftAdj;
#ifdef SET
            plex->resize(ctx.m+bitForcedCount); plex->clear();
            for(int i=0;i<ctx.fixed;i++) plex->push(ctx.orig[i]);
#endif
            const unsigned long long cpu0=threadCpuNowNs();
            execute128Root({t.P,t.C,t.X},ctx);
            state128Stats.cpu_ns+=threadCpuNowNs()-cpu0;
        }

        void state128HandoffBit64(const State128Node& st,const State128Context& ctx){
            wide_cost::Stage costScope(wide_cost::Map128_64);
            const int m64=mask128Pop(st.P|st.C);
            assert(m64>0 && m64<=64);
            bitFixed=ctx.fixed; bitM=m64; bitTaskProper=ctx.proper;
            bitExtN=ctx.extN+mask128Pop(st.X);
            ensureBitAuxCapacity(bitExtN,leftSz);
            const bool hasSeedMatrix=(subg.n>0);
            const auto mat=[&](int a,int b,int i,int j)->uint8_t{
                if(hasSeedMatrix)return commonMtx[static_cast<size_t>(a)*static_cast<size_t>(subg.n)+b];
                return static_cast<uint8_t>((mask128Test(ctx.adj[i],j)?1:0)|(mask128Test(ctx.pair[i],j)?2:0));
            };
            const auto ext=[&](int a,int b,int e,int oi,int x)->bool{
                if(hasSeedMatrix)return (commonMtx[static_cast<size_t>(a)*static_cast<size_t>(subg.n)+b]&1)!=0;
                return e<ctx.extN?mask128Test(ctx.extAdj[e],oi):mask128Test(ctx.adj[x],oi);
            };
            const auto left=[&](int v,int oi,const auto& emit){
                if(hasSeedMatrix){for(int z=0;z<subg.V[v].degreeLeft;z++)emit(subg.V[v].NeighborsLeft[z]);}
                else for(int l=0;l<leftSz;l++) if(mask128Test(ctx.leftAdj[l],oi))emit(l);
            };
            auto bufs=wide_state_remap::buffers64(bitOrig,bitAdj,bitNon,bitPair,bitExtOrig,bitExtAdj,bitLeftAdj);
            wide_state_remap::Select64 sel;
            if(enableRemap128){
                sel=wide_state_remap::project128to64(st.P,st.C,st.X,ctx,leftSz,bufs);
                ++transferAudit.remap_projected;
                transferAudit.projected_rows+=2*m64+bitExtN+leftSz;
            }else{
                sel=wide_state_remap::convert128to64(st.P,st.C,st.X,ctx,leftSz,bufs,mat,ext,left,UNLINK2EQUAL);
                ++transferAudit.remap_old;
            }
            for(int e=0;e<ctx.extN;e++)bitExtForcedAdj[e]=ctx.extForcedAdj[e];
            for(int e=ctx.extN;e<bitExtN;e++)bitExtForcedAdj[e]=bitForcedCount;
            for(int l=0;l<leftSz;l++)bitLeftForcedAdj[l]=ctx.leftForcedAdj[l];
            defectCacheInvalidate(); leftNon32Ready=false; leftChunks32=0;
            ++state128Stats.transitions64;
            bit64Dispatch(sel.P,sel.C,0);
        }

        void state128DFS(const State128Node& st,const State128Context& ctx){
            statebit_wide_search::dfs<State128Node,State128Context,Mask128>(st,ctx,k,bitActiveLb(),64,
                [&](){ ++state128Stats.nodes; },
                [&](){ ++state128Stats.whole_calls; },
                [&](){ ++state128Stats.leaf_calls; },
                [&](Mask128 S,Mask128 X){ state128IsMaximalAndEmit(S,X,ctx); },
                [&](const State128Node& n,const State128Context& c){ state128HandoffBit64(n,c); },
                [&](Mask128 P,Mask128 C,const State128Context& c){ if(enableFoldingProbe)c0Opportunity.observe(128,mask128Pop(P|C),mask128Pop(C),bitForcedCount,0,[&](){
                    return c0Count(C,P|C,c.nonadj,[](Mask128 x){return mask128Any(x);},[](Mask128 x){return mask128First(x);},[](Mask128 x,int v){return mask128ClearBit(x,v);});
                });return state128ChoosePivot(P,C,c); },
                [&](int pivot,Mask128 P,Mask128 C,const State128Context& c){ return state128RefineIncludeOnce(pivot,P,C,c); },
                [&](const State128Node& n,const State128Context& c){ if(!tryExportState128Branch(n,c)) state128DFS(n,c); },
                [&](const State128Node& n,const State128Context& c){ state128DFS(n,c); });
        }


        inline void execute128Root(const State128Node& st,const State128Context& ctx){
            wide_cost::Stage costScope(wide_cost::Search128);
            if(!enableState128Simd){state128DFS(st,ctx);return;}
            ++simd128Audit.roots;
            wide_simd_shared128::run<1>(st,ctx,k,bitActiveLb(),
                [&](){++state128Stats.nodes;},
                [&](){++state128Stats.whole_calls;},
                [&](){++state128Stats.leaf_calls;},
                [&](Mask128 S,Mask128 X){state128IsMaximalAndEmit(S,X,ctx);},
                [&](const State128Node& n){state128HandoffBit64(n,ctx);},
                [&](const State128Node& n){return state128ChoosePivot(n.P,n.C,ctx);},
                [&](int p,Mask128 P,Mask128 C){return state128RefineIncludeOnce(p,P,C,ctx);},
                [&](int active){++state128Stats.simd_packets;state128Stats.simd_slots+=active;
                    if(active==2){++state128Stats.simd_occ2;state128Stats.hybrid_paired_ready_states+=2;}
                    else ++state128Stats.simd_occ1;},
                [&](){++state128Stats.simd_pivot_packets;},
                [&](int active,unsigned long long removed){++state128Stats.simd_refine_packets;
                    state128Stats.cand_filter_calls+=active;state128Stats.cand_filter_removed+=removed;},
                [&](){++state128Stats.hybrid_single_ready_events;++state128Stats.hybrid_scalar_branch_nodes;},
                [&](int children,bool scalar){if(scalar)state128Stats.hybrid_scalar_generated_children+=children;
                    else state128Stats.simd_generated_children+=children;},
                [&](const State128Node& child){
                    ++simd128Audit.export_calls;
                    const bool sent=tryExportState128Branch(child,ctx);
                    if(sent)++simd128Audit.exported;
                    return sent;
                },
                [&](const State128Node& n){if(enableFoldingProbe)c0Opportunity.observe(128,mask128Pop(n.P|n.C),mask128Pop(n.C),bitForcedCount,0,[&](){
                    return c0Count(n.C,n.P|n.C,ctx.nonadj,[](Mask128 x){return mask128Any(x);},[](Mask128 x){return mask128First(x);},[](Mask128 x,int v){return mask128ClearBit(x,v);});
                });},&simd128Stack);
        }

        void forcedBaseState128EnumerateCurrentTask(){
            wide_cost::Window costWindow(wide_cost::List128);
            wide_cost::Stage costScope(wide_cost::Search128);
            const auto diagStart=std::chrono::steady_clock::now();
            ++bestDiagStats.state128_calls;
            const int active=plex->sz+cand1->sz;
            assert(forcedBaseState128Eligible(active,FORCED_BASE_STATE128_THRESHOLD));
            const int savedForced=bitForcedCount; bitForcedCount=forcedBaseSize();
            state128Stats.observeHandoff(active,bitForcedCount);
            const unsigned long long cpu0=threadCpuNowNs();
            const State128Context ctx=state128BuildCurrentTask();
            const Mask128 all=mask128LowBits(ctx.m), P=mask128LowBits(ctx.fixed), C=mask128AndNot(all,P);
            execute128Root({P,C,{}},ctx);
            state128Stats.cpu_ns+=threadCpuNowNs()-cpu0;
            bitForcedCount=savedForced; defectCacheInvalidate();
            bestDiagStats.state128_ns+=diagNs(diagStart,std::chrono::steady_clock::now());
        }


