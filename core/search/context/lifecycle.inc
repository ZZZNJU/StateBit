        SearchContext(const Graph &_subg,MaximalityChecker *_plexMaxChecker, Emitter *_emitter,
                    Stack<int> *_plex, VtxSet *_cand1, VtxSet *_cand2, VtxSet *_excl, VtxSet *_exclBK, Stack<int> *_exclStack,
                    int *_neiInP, int *_neiInG, int *_nonadjInP,int _hopSz, int _hop2Sz, int _leftSz, const uint8_t* _commonMtx,
                    const ComplementCSR *_complement=nullptr, bool _denseBookkeeping=false)
        :subg(_subg),plexMaxChecker(_plexMaxChecker),emitter(_emitter)
        ,plex(_plex),cand1(_cand1),cand2(_cand2),excl(_excl),exclBK(_exclBK),exclStack(_exclStack)
        ,neiInP(_neiInP),neiInG(_neiInG),nonadjInP(_nonadjInP),hopSz(_hopSz),hop2Sz(_hop2Sz),leftSz(_leftSz),commonMtx(_commonMtx),
         complement(_complement),denseBookkeeping(_denseBookkeeping),gOffset(0),pOffset(0),useGAsP(false),bitNeiScratch(_subg.n,0)
        {
            ensureBitAuxCapacity(0, _leftSz);
#if AGG_FORCED_CONTEXT_MODE == AGG_MODE_VALIDATION || AGG_FORCED_CONTEXT_MODE == AGG_MODE_MAINTENANCE_ONLY || AGG_FORCED_CONTEXT_MODE == AGG_MODE_LEFT_ONLY || AGG_FORCED_CONTEXT_MODE == AGG_MODE_COMBINED
            const auto aggInitStart=std::chrono::steady_clock::now();
            leftLogicalAdj.reset(_leftSz);
            unsigned long long initEdges=0;
            // At construction the logical U is exactly the hop-1 block
            // (cand1). cand2 vertices are outside U and are later added/removed
            // through addG/subG, which update this tracker symmetrically.
            for(int v=0;v<_hopSz;++v){
                leftLogicalAdj.addEdges(_subg.V[v].NeighborsLeft,_subg.V[v].degreeLeft);
                initEdges+=static_cast<unsigned long long>(_subg.V[v].degreeLeft);
            }
            aggregateForcedContextStats.left_u_init_edges+=initEdges;
            aggregateForcedContextStats.left_u_init_ns+=diagNs(aggInitStart,std::chrono::steady_clock::now());
            leftLogicalPendingDelta.assign(static_cast<size_t>(_subg.n),0);
            leftLogicalDirtyFlag.assign(static_cast<size_t>(_subg.n),0);
            leftLogicalDirtyVertices.reserve(static_cast<size_t>(_subg.n));
#endif
        }

        inline void setSeedAutoLeftContextPolicy(const bool useHybrid,const unsigned long long degreeSum,const int hopVertices){
            (void)useHybrid; (void)degreeSum; (void)hopVertices;
        }

        void del(){
            delete plex;
            delete cand1;
            delete excl;
            delete exclStack;
            delete[] neiInP;
            delete[] neiInG;
            delete[] nonadjInP;
        }
        
