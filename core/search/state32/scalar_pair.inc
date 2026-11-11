
// ==== shared laneChoosePivot ====
        int laneChoosePivot(const uint32_t P,const uint32_t C,const LaneContext32& ctx) const {
            const uint32_t U=P|C;
            const int us=lanePop(U);
            int pmin=-1,pdeg=INT_MAX;
            uint32_t t=P;
            while(t){
                const int u=laneFirst(t); t&=t-1;
                const int d=lanePop(ctx.adj[u]&U);
                if(d<pdeg){ pdeg=d; pmin=u; }
            }
            if(pmin<0) return -2;
            if(pdeg+k<std::max(state32ContextActiveLb(ctx),lanePop(P))) return -2;
            if(pdeg+k<us){
                uint32_t choices=C&ctx.nonadj[pmin];
                int best=-1,bdg=INT_MAX,bpdeg=INT_MAX;
                while(choices){
                    const int x=laneFirst(choices); choices&=choices-1;
                    const int dg=lanePop(ctx.adj[x]&U),pd=lanePop(ctx.adj[x]&P);
                    if(dg<bdg||(dg==bdg&&pd<bpdeg)){ best=x; bdg=dg; bpdeg=pd; }
                }
                return best;
            }
            int best=-1,bdg=INT_MAX,bpdeg=INT_MAX;
            t=C;
            while(t){
                const int x=laneFirst(t); t&=t-1;
                const int dg=lanePop(ctx.adj[x]&U),pd=lanePop(ctx.adj[x]&P);
                if(dg<bdg||(dg==bdg&&pd<bpdeg)){ best=x; bdg=dg; bpdeg=pd; }
            }
            return best;
        }

// ==== shared laneChoosePivotPair ====
        void laneChoosePivotPair(const uint32_t plexLane0,const uint32_t C0,
                                 const uint32_t plexLane1,const uint32_t C1,
                                 const LaneContext32& ctx,int& out0,int& out1) const {
            LANE_COUNT(++laneStats.pair_pivot_calls);
            const uint32_t universeLane0=plexLane0|C0,universeLane1=plexLane1|C1;
            const uint64_t Upack=lanePack(universeLane0,universeLane1),Ppack=lanePack(plexLane0,plexLane1);
            const int us0=lanePop(universeLane0),us1=lanePop(universeLane1),ps0=lanePop(plexLane0),ps1=lanePop(plexLane1);
            int pmin0=-1,pmin1=-1,pdeg0=INT_MAX,pdeg1=INT_MAX;
            uint32_t t0=plexLane0,t1=plexLane1;
            while(t0||t1){
                const int u0=t0?laneFirst(t0):-1;
                const int u1=t1?laneFirst(t1):-1;
                if(t0) t0&=t0-1;
                if(t1) t1&=t1-1;
                const uint64_t degPack=laneRowsPack(ctx.adj,u0,u1)&Upack;
                LANE_COUNT(++laneStats.packed_pivot_steps);
                if(u0>=0){ const int d=lanePop(laneLo(degPack)); if(d<pdeg0){ pdeg0=d; pmin0=u0; } }
                if(u1>=0){ const int d=lanePop(laneHi(degPack)); if(d<pdeg1){ pdeg1=d; pmin1=u1; } }
            }
            out0=(pmin0<0||pdeg0+k<std::max(state32ContextActiveLb(ctx),ps0))?-2:-1;
            out1=(pmin1<0||pdeg1+k<std::max(state32ContextActiveLb(ctx),ps1))?-2:-1;
            uint32_t choices0=0,choices1=0;
            if(out0!=-2) choices0=(pdeg0+k<us0)?(C0&ctx.nonadj[pmin0]):C0;
            if(out1!=-2) choices1=(pdeg1+k<us1)?(C1&ctx.nonadj[pmin1]):C1;
            int best0=-1,best1=-1,bdg0=INT_MAX,bdg1=INT_MAX,bp0=INT_MAX,bp1=INT_MAX;
            t0=choices0; t1=choices1;
            while(t0||t1){
                const int x0=t0?laneFirst(t0):-1;
                const int x1=t1?laneFirst(t1):-1;
                if(t0) t0&=t0-1;
                if(t1) t1&=t1-1;
                const uint64_t rows=laneRowsPack(ctx.adj,x0,x1);
                const uint64_t dgPack=rows&Upack;
                const uint64_t pdPack=rows&Ppack;
                LANE_COUNT(++laneStats.packed_pivot_steps);
                if(x0>=0){
                    const int dg=lanePop(laneLo(dgPack)),pd=lanePop(laneLo(pdPack));
                    if(dg<bdg0||(dg==bdg0&&pd<bp0)){ best0=x0; bdg0=dg; bp0=pd; }
                }
                if(x1>=0){
                    const int dg=lanePop(laneHi(dgPack)),pd=lanePop(laneHi(pdPack));
                    if(dg<bdg1||(dg==bdg1&&pd<bp1)){ best1=x1; bdg1=dg; bp1=pd; }
                }
            }
            if(out0!=-2) out0=best0;
            if(out1!=-2) out1=best1;
        }

// ==== shared laneRefineIncludeOnce ====
        uint32_t laneRefineIncludeOnce(const int pivot,const uint32_t P,uint32_t C,const LaneContext32& ctx){
            ++bitStats.cand_filter_calls;
            uint32_t U=P|C;
            const uint32_t sat=laneSaturated(P,ctx);
            uint32_t t=C;
            unsigned long long removed=0;
            while(t){
                const int x=laneFirst(t); t&=t-1;
                const uint32_t b=1U<<x;
                bool erase=(ctx.pair[pivot]&b)==0;
                if(!erase){
                    erase=lanePop(ctx.nonadj[x]&P)>k-1 ||
                          (ctx.nonadj[x]&sat) ||
                          lanePop(ctx.adj[x]&U)+k<std::max(state32ContextActiveLb(ctx),lanePop(P));
                }
                if(erase){ C&=~b; U&=~b; ++removed; }
            }
            bitStats.cand_filter_removed+=removed;
            return C;
        }

// ==== shared laneRefineIncludePair ====
        void laneRefineIncludePair(const int pivot0,const uint32_t plexLane0,const uint32_t Cin0,
                                   const int pivot1,const uint32_t plexLane1,const uint32_t Cin1,
                                   const LaneContext32& ctx,uint32_t& Cout0,uint32_t& Cout1){
            LANE_COUNT(++laneStats.pair_refine_calls);
            bitStats.cand_filter_calls+=2;
            Cout0=Cin0; Cout1=Cin1;
            uint32_t universeLane0=plexLane0|Cout0,universeLane1=plexLane1|Cout1;
            const int ps0=lanePop(plexLane0),ps1=lanePop(plexLane1);
            const uint64_t Ppack=lanePack(plexLane0,plexLane1);
            const uint64_t satPack=laneSaturatedPair(plexLane0,plexLane1,ctx);
            const uint64_t pivotPairPack=laneRowsPack(ctx.pair,pivot0,pivot1);
            uint32_t t0=Cin0,t1=Cin1;
            unsigned long long removed=0;
            while(t0||t1){
                const int x0=t0?laneFirst(t0):-1;
                const int x1=t1?laneFirst(t1):-1;
                const uint32_t b0=x0>=0?(1U<<x0):0U,b1=x1>=0?(1U<<x1):0U;
                if(t0) t0&=t0-1;
                if(t1) t1&=t1-1;
                const uint64_t bpack=lanePack(b0,b1);
                const uint64_t nonRows=laneRowsPack(ctx.nonadj,x0,x1);
                const uint64_t adjRows=laneRowsPack(ctx.adj,x0,x1);
                const uint64_t pairOk=pivotPairPack&bpack;
                const uint64_t nonP=nonRows&Ppack;
                const uint64_t nonSat=nonRows&satPack;
                const uint64_t adjU=adjRows&lanePack(universeLane0,universeLane1);
                LANE_COUNT(++laneStats.packed_refine_steps);
                if(x0>=0){
                    bool erase=(laneLo(pairOk)&b0)==0;
                    if(!erase) erase=lanePop(laneLo(nonP))>k-1 || laneLo(nonSat)!=0 ||
                                     lanePop(laneLo(adjU))+k<std::max(state32ContextActiveLb(ctx),ps0);
                    if(erase){ Cout0&=~b0; universeLane0&=~b0; ++removed; }
                }
                if(x1>=0){
                    bool erase=(laneHi(pairOk)&b1)==0;
                    if(!erase) erase=lanePop(laneHi(nonP))>k-1 || laneHi(nonSat)!=0 ||
                                     lanePop(laneHi(adjU))+k<std::max(state32ContextActiveLb(ctx),ps1);
                    if(erase){ Cout1&=~b1; universeLane1&=~b1; ++removed; }
                }
            }
            bitStats.cand_filter_removed+=removed;
        }

// ==== shared laneIsMaximalAndEmit ====
        bool laneIsMaximalAndEmit(const uint32_t S,const uint32_t X,const LaneContext32& ctx){
            const int ss=lanePop(S);
            const uint32_t sat=laneSaturated(S,ctx);
            uint32_t t=X;
            while(t){
                const int x=laneFirst(t); t&=t-1;
                if(lanePop(ctx.adj[x]&S)+k>=ss+1 && !(ctx.nonadj[x]&sat)){
                    ++bitStats.x_rejects; return false;
                }
            }
            // Vertices excluded before the <=32 handoff, task-level external
            // exclusions, and seed-left vertices stay in the original Word64
            // coordinate system. Expand S/saturated only when those checks are
            // actually needed; this avoids allocating/re-encoding large arrays
            // at every transition.
            const uint64_t STATE64=laneExpand(S,ctx);
            const uint64_t sat64=laneExpand(sat,ctx);
            uint64_t bx=ctx.baseX;
            while(bx){
                const int x=bitFirst(bx); bx&=bx-1;
                if(bitCanAddInternal(x,STATE64,sat64)){ ++bitStats.x_rejects; return false; }
            }
            for(int e=0;e<bitExtN;e++){
                if(state32CoreCanAddExternal(e,STATE64,sat64)){ ++bitStats.x_rejects; return false; }
            }
            for(int l=0;l<leftSz;l++){
                const uint64_t a=bitLeftAdj[l];
                const int miss=defectCacheLeftMissing(l);
                if(miss<k && bitPop(a&STATE64)+(k-miss)>=ss+1 && !(sat64&~a)){ ++bitStats.left_rejects; return false; }
            }
#ifdef SET
            const int old=plex->sz;
            state32CoreAppendForcedBase();
            t=S;
            while(t){
                const int i=laneFirst(t); t&=t-1;
                const int oi=ctx.orig[i];
                if(oi>=bitFixed) plex->push(bitOrig[oi]);
            }
            emitVerifiedPlex(plex);
            while(plex->sz>old) plex->pop();
#else
            emitter->emitPlex();
#endif
            ++bitStats.outputs;
            return true;
        }

// ==== shared laneFinishMaximalityAndEmit ====
        bool laneFinishMaximalityAndEmit(const uint32_t S,const uint32_t sat,const LaneContext32& ctx){
            const int ss=lanePop(S);
            const uint64_t STATE64=laneExpand(S,ctx);
            const uint64_t sat64=laneExpand(sat,ctx);
            uint64_t bx=ctx.baseX;
            while(bx){
                const int x=bitFirst(bx); bx&=bx-1;
                if(bitCanAddInternal(x,STATE64,sat64)){ ++bitStats.x_rejects; AVX_COUNT(++maxSimdStats.basex_rejects); return false; }
            }
            for(int e=0;e<bitExtN;e++){
                if(state32CoreCanAddExternal(e,STATE64,sat64)){ ++bitStats.x_rejects; AVX_COUNT(++maxSimdStats.external_rejects); return false; }
            }
            for(int l=0;l<leftSz;l++){
                const uint64_t a=bitLeftAdj[l];
                const int miss=defectCacheLeftMissing(l);
                if(miss<k && bitPop(a&STATE64)+(k-miss)>=ss+1 && !(sat64&~a)){ ++bitStats.left_rejects; AVX_COUNT(++maxSimdStats.left_rejects); return false; }
            }
#ifdef SET
            const int oldPlexSz=plex->sz;
            state32CoreAppendForcedBase();
            uint32_t t=S;
            while(t){
                const int i=laneFirst(t); t&=t-1;
                const int oi=ctx.orig[i];
                if(oi>=bitFixed) plex->push(bitOrig[oi]);
            }
            emitVerifiedPlex(plex);
            while(plex->sz>oldPlexSz) plex->pop();
#else
            emitter->emitPlex();
#endif
            ++bitStats.outputs;
            AVX_COUNT(++maxSimdStats.emits);
            return true;
        }

// ==== shared avxLeft2DRejectMaskT ====
