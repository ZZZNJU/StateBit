        inline bool shouldApplyForcedBaseFold(const int widthParentC) const {
            bool base=false;
            (void)widthParentC;
            base=true;
            if(!base) return false;
            const bool payoffGateAllows=forcedBasePayoffGate.allowFold();
            return payoffGateAllows;
        }

        bool forcedBaseTerminalBypassAllForced(){
            if(cand1->sz<=0) return false;
            const auto t0=std::chrono::steady_clock::now();
            ++forcedBasePayoffGate.stats.terminal_checks;
            const int uSize=logicalUSize();
            unsigned long long scanned=0;

            // Low-frequency scalar reference used only to validate the formal
            // AVX2 path.  It returns the same scalar-equivalent scanned count
            // (through and including the first failing candidate).

            bool result=true;
            for(int i=0;i<cand1->sz;++i){
                ++scanned;
                const int v=cand1->members[i];
                if(!isForcedBaseC0Vertex(uSize,getNeiInG(v))){ result=false; break; }
            }

            forcedBasePayoffGate.stats.terminal_scan_candidates+=scanned;
            forcedBasePayoffGate.stats.terminal_scan_ns+=diagNs(t0,std::chrono::steady_clock::now());
            if(result) ++forcedBasePayoffGate.stats.terminal_hits;
            return result;
        }

        void applyForcedBaseFold(ForcedBaseFoldFrame &frame){
            const auto diagStart=std::chrono::steady_clock::now();
            ++bestDiagStats.fold_calls;
            const int uSize=logicalUSize();
            const int scanN=cand1->sz;
            forcedBaseStats.observeScan(scanN);
            for(int i=0;i<cand1->sz;){
                const int v=cand1->members[i];
                if(isForcedBaseC0Vertex(uSize,getNeiInG(v))){
                    cand1->remove(v);       // no subG: v remains in logical U
                    forcedBase.push_back(v); // no addP: physical P stays small
                    ++frame.folded;
                }else ++i;
            }
            table4Audit.folded(frame.folded,forcedBaseSize(),plex->sz+cand1->sz);
            bestDiagStats.fold_ns+=diagNs(diagStart,std::chrono::steady_clock::now());
        }

        void restoreForcedBaseFold(ForcedBaseFoldFrame &frame){
            while(frame.folded>0){
                const int v=forcedBase.back();
                forcedBase.pop_back();
                cand1->add(v); // U never changed, so no addG
                --frame.folded;
            }
        }

        struct ForcedBaseFoldGuard {
            SearchContext *owner;
            ForcedBaseFoldFrame frame;
            explicit ForcedBaseFoldGuard(SearchContext *o):owner(o){}
            ~ForcedBaseFoldGuard(){ owner->restoreForcedBaseFold(frame); }
        };

        struct PureEdBtSubtreeGuard {
            SearchContext *owner;
            vector<int> materializedForced;
            bool active=false;
            explicit PureEdBtSubtreeGuard(SearchContext *o):owner(o){}
            void enter(){
                (void)owner;
            }
            ~PureEdBtSubtreeGuard(){
            }
        };

        struct ForcedC0FoldFrame {
            int folded=0;
            int removedExcl=0;
        };

        inline bool shouldApplyForcedComplementFoldRealFold(const int widthParentC) const {
            (void)widthParentC;
            return false;
        }

        void applyForcedComplementFoldRealFold(ForcedC0FoldFrame &frame){
            (void)frame;
        }

        void restoreForcedComplementFoldRealFold(ForcedC0FoldFrame &frame){
            (void)frame;
        }

        struct ForcedC0FoldGuard {
            SearchContext *owner;
            ForcedC0FoldFrame frame;
            explicit ForcedC0FoldGuard(SearchContext *o):owner(o){}
            ~ForcedC0FoldGuard(){ owner->restoreForcedComplementFoldRealFold(frame); }
        };

