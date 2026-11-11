        inline int bitPop(uint64_t x) const { return __builtin_popcountll(x); }
        inline int bitFirst(uint64_t x) const { return __builtin_ctzll(x); }
        inline int bitActiveLb() const {
            return FORCED_CONTEXTState32ForcedContext{bitForcedCount,0}.activeLowerBound(lb);
        }
        inline int bitForcedMissing(const int forcedAdj) const { return forcedBaseMissingAdj(bitForcedCount,forcedAdj); }

        // Runtime-k version of the word kernel.  The local state is still one
        // 64-bit word, but k is no longer restricted to compile-time values
        // 2, 3, and 4.
        inline uint64_t bitSaturated(const uint64_t P) const {
            uint64_t sat=0, t=P;
            while(t){
                const int u=bitFirst(t); t&=t-1;
                if(bitPop(bitNon[u]&P)==k-1) sat|=1ULL<<u;
            }
            return sat;
        }


        inline bool bitCanAddInternal(const int x,const uint64_t S,const uint64_t sat) const {
            return bitPop(bitAdj[x]&S)+k>=bitPop(S)+1 && !(bitNon[x]&sat);
        }

        inline bool bitCanAddExternal(const int e,const uint64_t S,const uint64_t sat) const {
            const int miss=bitForcedMissing(bitExtForcedAdj[e]);
            return bitPop(bitExtAdj[e]&S)+k>=bitPop(S)+1+miss && (((~bitExtAdj[e])&sat)==0);
        }

        bool bitIsMaximalAndEmit(const uint64_t S,const uint64_t X){
            const int ss=bitPop(S);
            const uint64_t sat=bitSaturated(S);
            return statebit_word64_search2::check_maximal(X,bitExtN,leftSz,
                [&](int x)->bool {
                    const bool add=bitCanAddInternal(x,S,sat);
                    if(add)++bitStats.x_rejects;
                    return add;
                },
                [&](int e)->bool {
                    const bool add=bitCanAddExternal(e,S,sat);
                    if(add)++bitStats.x_rejects;
                    return add;
                },
                [&](int l)->bool {
                    const uint64_t a=bitLeftAdj[l];
                        const int miss=bitForcedMissing(bitLeftForcedAdj[l]);
                        const bool add=(bitPop(a&S)+k>=ss+1+miss && !(sat&~a));
                    if(add)++bitStats.left_rejects;
                    return add;
                },
                [&](){
    #ifdef SET
                const int old=plex->sz;
                if(bitForcedCount) for(const int v:forcedBase) plex->push(v);
                for(int i=bitFixed;i<bitM;i++) if(S&(1ULL<<i)) plex->push(bitOrig[i]);
                emitVerifiedPlex(plex);
                while(plex->sz>old) plex->pop();
    #else
                emitter->emitPlex();
    #endif
                    ++bitStats.outputs;
                });
        }

        static constexpr uint8_t WHOLE_WITNESS_NONE=32;
        struct LaneState32 {
            uint32_t P=0,C=0,X=0;
            bool prechecked=false;
            // 0..31: a vertex that proved P|C was not a k-plex at the parent.
            // 32: no inherited witness.
            uint8_t wholeWitness=WHOLE_WITNESS_NONE;
        };
        struct LaneContext32 {
            int m=0;
            int activeLb=0; // Immutable within this context, including nested remat returns.
            int orig[32];
            uint32_t adj[32],nonadj[32],pair[32];
            uint64_t rootMask=0,baseX=0;
        };
