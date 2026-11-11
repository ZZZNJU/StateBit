        size_t getIdx(const int v1,const int v2) const {
            return static_cast<size_t>(v1)*static_cast<size_t>(subg.n)+static_cast<size_t>(v2);
        }
        
        bool isAdjMtx(const int v1,const int v2)
        {
            return commonMtx[getIdx(v1,v2)]&1;
        }

        inline int getNeiInG(const int v) const {
            return denseBookkeeping ? neiInG[v]+gOffset : neiInG[v];
        }
        inline int getNeiInP(const int v) const {
            if(useGAsP) return getNeiInG(v);
            return denseBookkeeping ? neiInP[v]+pOffset : neiInP[v];
        }

        inline void ensureBitAuxCapacity(const int extN,const int leftN){
            if(extN<0 || leftN<0) return;
            if(static_cast<int>(bitExtOrig.size()) < extN) bitExtOrig.resize(extN);
            if(static_cast<int>(bitExtAdj.size()) < extN) bitExtAdj.resize(extN);
            if(static_cast<int>(bitExtForcedAdj.size()) < extN) bitExtForcedAdj.resize(extN);
            if(static_cast<int>(suffixExtLocal.size()) < extN) suffixExtLocal.resize(extN);
            if(static_cast<int>(suffixExtStamp.size()) < extN) suffixExtStamp.resize(extN,0ULL);
            if(static_cast<int>(bitLeftAdj.size()) < leftN) bitLeftAdj.resize(leftN);
            if(static_cast<int>(bitLeftForcedAdj.size()) < leftN) bitLeftForcedAdj.resize(leftN);
            if(static_cast<int>(suffixLeftLocal.size()) < leftN) suffixLeftLocal.resize(leftN);
            if(static_cast<int>(suffixLeftStamp.size()) < leftN) suffixLeftStamp.resize(leftN,0ULL);
            const int chunks=(leftN+31)>>5;
            const size_t need=static_cast<size_t>(BIT_LIMIT)*static_cast<size_t>(chunks);
            if(leftNon32.size()<need) leftNon32.resize(need);
        }

        inline uint32_t& leftNon32At(const int row,const int chunk){
            return leftNon32[static_cast<size_t>(row)*static_cast<size_t>(leftChunks32)+static_cast<size_t>(chunk)];
        }
        inline const uint32_t& leftNon32At(const int row,const int chunk) const {
            return leftNon32[static_cast<size_t>(row)*static_cast<size_t>(leftChunks32)+static_cast<size_t>(chunk)];
        }

        inline void queueLazyLeftLogicalAdjDelta(const int u,const int delta){
            if(u<0 || u>=static_cast<int>(leftLogicalPendingDelta.size())) return;
            ++aggregateForcedContextStats.lazy_delta_events;
            if(!leftLogicalDirtyFlag[static_cast<size_t>(u)]){
                leftLogicalDirtyFlag[static_cast<size_t>(u)]=1;
                leftLogicalDirtyVertices.push_back(u);
            }
            leftLogicalPendingDelta[static_cast<size_t>(u)]+=delta;
        }

        inline void flushLazyLeftLogicalAdj(){
#if AGG_FORCED_CONTEXT_MODE == AGG_MODE_VALIDATION || AGG_FORCED_CONTEXT_MODE == AGG_MODE_MAINTENANCE_ONLY || AGG_FORCED_CONTEXT_MODE == AGG_MODE_LEFT_ONLY || AGG_FORCED_CONTEXT_MODE == AGG_MODE_COMBINED
            if(leftLogicalDirtyVertices.empty()) return;
            const auto t0=std::chrono::steady_clock::now();
            ++aggregateForcedContextStats.lazy_flush_calls;
            aggregateForcedContextStats.lazy_flush_vertices+=static_cast<unsigned long long>(leftLogicalDirtyVertices.size());
            unsigned long long edgeWork=0;
            for(const int u:leftLogicalDirtyVertices){
                const int delta=leftLogicalPendingDelta[static_cast<size_t>(u)];
                if(delta>0){
                    for(int rep=0;rep<delta;++rep) leftLogicalAdj.addEdges(subg.V[u].NeighborsLeft,subg.V[u].degreeLeft);
                    const unsigned long long edges=static_cast<unsigned long long>(subg.V[u].degreeLeft)*static_cast<unsigned long long>(delta);
                    aggregateForcedContextStats.left_u_add_edges+=edges; edgeWork+=edges;
                }else if(delta<0){
                    for(int rep=0;rep<-delta;++rep) leftLogicalAdj.subEdges(subg.V[u].NeighborsLeft,subg.V[u].degreeLeft);
                    const unsigned long long edges=static_cast<unsigned long long>(subg.V[u].degreeLeft)*static_cast<unsigned long long>(-delta);
                    aggregateForcedContextStats.left_u_sub_edges+=edges; edgeWork+=edges;
                }
                leftLogicalPendingDelta[static_cast<size_t>(u)]=0;
                leftLogicalDirtyFlag[static_cast<size_t>(u)]=0;
            }
            leftLogicalDirtyVertices.clear();
            aggregateForcedContextStats.left_u_update_edges+=edgeWork;
            aggregateForcedContextStats.lazy_flush_edges+=edgeWork;
            aggregateForcedContextStats.lazy_flush_ns+=diagNs(t0,std::chrono::steady_clock::now());
#endif
        }
       
        inline void updateLeftLogicalAdjEager(const int u,const int delta){
#if AGG_FORCED_CONTEXT_MODE == AGG_MODE_VALIDATION || AGG_FORCED_CONTEXT_MODE == AGG_MODE_MAINTENANCE_ONLY || AGG_FORCED_CONTEXT_MODE == AGG_MODE_LEFT_ONLY || AGG_FORCED_CONTEXT_MODE == AGG_MODE_COMBINED
            const unsigned long long call=aggregateForcedContextStats.left_u_update_calls-1;
            const bool sample=((call&4095ULL)==0ULL);
            const auto t0=sample?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{};
            if(delta>0) leftLogicalAdj.addEdges(subg.V[u].NeighborsLeft,subg.V[u].degreeLeft);
            else leftLogicalAdj.subEdges(subg.V[u].NeighborsLeft,subg.V[u].degreeLeft);
            const unsigned long long edges=static_cast<unsigned long long>(subg.V[u].degreeLeft);
            aggregateForcedContextStats.left_u_update_edges+=edges;
            if(delta>0) aggregateForcedContextStats.left_u_add_edges+=edges;
            else aggregateForcedContextStats.left_u_sub_edges+=edges;
            ++aggregateForcedContextStats.hybrid_eager_events;
            aggregateForcedContextStats.hybrid_eager_edges+=edges;
            if(sample){
                ++aggregateForcedContextStats.left_u_update_sample_calls;
                aggregateForcedContextStats.left_u_update_sample_edges+=edges;
                aggregateForcedContextStats.left_u_update_sample_ns+=diagNs(t0,std::chrono::steady_clock::now());
            }
#else
            (void)u; (void)delta;
#endif
        }

        inline void updateLeftLogicalAdjForLogicalUChange(const int u,const int delta){
#if AGG_FORCED_CONTEXT_MODE == AGG_MODE_VALIDATION || AGG_FORCED_CONTEXT_MODE == AGG_MODE_MAINTENANCE_ONLY || AGG_FORCED_CONTEXT_MODE == AGG_MODE_LEFT_ONLY || AGG_FORCED_CONTEXT_MODE == AGG_MODE_COMBINED
            if(inScalarListFallback()){
                ++forcedBasePayoffGate.stats.subtree_fallback_context_updates_skipped;
                return;
            }
            ++aggregateForcedContextStats.left_u_update_calls;
            const bool useLazy=forcedBaseUseLazyLeftContext(subg.V[u].degreeLeft);
            if(useLazy){
                ++aggregateForcedContextStats.hybrid_lazy_events;
                queueLazyLeftLogicalAdjDelta(u,delta);
            }else{
                updateLeftLogicalAdjEager(u,delta);
            }
#else
            (void)u; (void)delta;
#endif
        }

        void addG(const int u){
            if(!denseBookkeeping){
                for (int i=0;i<subg.V[u].degree;i++) neiInG[subg.V[u].Neighbors[i]]++;
            }else{
                assert(complement!=nullptr);
                ++gOffset; --neiInG[u];
                for(const int *it=complement->begin(u),*e=complement->end(u);it!=e;++it) --neiInG[*it];
            }
            updateLeftLogicalAdjForLogicalUChange(u,+1);
        }

        void subG(const int u){
            if(!denseBookkeeping){
                for (int i=0;i<subg.V[u].degree;i++) neiInG[subg.V[u].Neighbors[i]]--;
            }else{
                assert(complement!=nullptr);
                --gOffset; ++neiInG[u];
                for(const int *it=complement->begin(u),*e=complement->end(u);it!=e;++it) ++neiInG[*it];
            }
            updateLeftLogicalAdjForLogicalUChange(u,-1);
        }

        void addP(const int u){
            if(!denseBookkeeping){
                for (int i=0;i<subg.V[u].degree;i++) neiInP[subg.V[u].Neighbors[i]]++;
            }else{
                assert(complement!=nullptr);
                ++pOffset; --neiInP[u];
                for(const int *it=complement->begin(u),*e=complement->end(u);it!=e;++it) --neiInP[*it];
            }
        }

        void subP(const int u){
            if(!denseBookkeeping){
                for (int i=0;i<subg.V[u].degree;i++) neiInP[subg.V[u].Neighbors[i]]--;
            }else{
                assert(complement!=nullptr);
                --pOffset; ++neiInP[u];
                for(const int *it=complement->begin(u),*e=complement->end(u);it!=e;++it) ++neiInP[*it];
            }
        }

        inline int forcedBaseSize() const {
            return static_cast<int>(forcedBase.size());
        }

        inline int fixedLogicalSize() const { return plex->sz + forcedBaseSize(); }
        inline int logicalUSize() const { return fixedLogicalSize() + cand1->sz; }

        // Active C vertices are adjacent to every inherited ForcedBase vertex.
        // Therefore the fixed-base contribution cancels in the P-defect test,
        // while it must be included in the minimum-size/degree threshold.
