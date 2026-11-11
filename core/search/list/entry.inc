        inline bool preUpdatePCIsKPlex(){
            const int uSize=plex->sz+cand1->sz;
            const auto scan=statebit_search_core::preupdate(plex->sz,cand1->sz,lb,k,
                [&](int i){return plex->members[i];},
                [&](int i){return cand1->members[i];},
                [&](int v){return getNeiInG(v);});
            switch(scan.outcome){
              case statebit_search_core::PreUpdateResult::SizeMiss:
                preUpdateKPlexStats.missSize();
                foldingGateEarlyOffGate.observeSizeMiss(uSize,cand1->sz);
                return false;
              case statebit_search_core::PreUpdateResult::PlexMiss:
                preUpdateKPlexStats.missP(scan.scannedP,scan.scannedC);
                break;
              case statebit_search_core::PreUpdateResult::CandidateMiss:
                preUpdateKPlexStats.missC(scan.scannedP,scan.scannedC);
                break;
              case statebit_search_core::PreUpdateResult::Pass:
                preUpdateKPlexStats.hit(scan.scannedP,scan.scannedC,uSize,cand1->sz);
                foldingGateEarlyOffGate.observeHit(uSize,cand1->sz);
                return true;
            }
            foldingGateEarlyOffGate.observeStructuralMiss(uSize,cand1->sz);
            return false;
        }

        void kSearch(const int res){
            statebit_list_search::ksearch(*this,res,lb,[](){},[](){},[&](){
                if(preUpdatePCIsKPlex()){
                    preUpdateKPlexStats.observeDirectTerminal(cand1->sz);
                    return true;
                }
                return false;
            });
        }

        struct ForcedBaseFoldFrame {
            int folded=0;
        };

