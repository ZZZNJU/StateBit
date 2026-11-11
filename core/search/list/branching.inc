        void checkListMaximality()
        {
            memcpy(plex->members + plex->sz, cand1->members, cand1->sz * sizeof(int));
            plex->sz += cand1->sz;
            const bool oldUseGAsP=useGAsP;
            useGAsP=true;
            bool flag = true;
            for (int i = 0; i < excl->sz;++i)
            {
                if (canFormPlex(excl->members[i],1))
                {
                    flag = false;
                    break;
                }
            }
            if (flag&&plexMaxChecker->isMaximal(plex->members, plex->sz, neiInG, denseBookkeeping ? gOffset : 0))
            {
                #ifdef SET
                emitVerifiedPlex(plex);
                #else
                emitter->emitPlex();
                #endif
            }
            plex->sz -= cand1->sz;
            useGAsP=oldUseGAsP;
        }

        void checkAsMaxPlex()
        {
            if(foldingGateForcedSemanticsActive() && !inScalarListFallback()){
                buildLogicalUSolution();
                if(logicalUIsLocallyMaximal() &&
                   plexMaxChecker->isMaximal(forcedSolutionScratch.data(),static_cast<int>(forcedSolutionScratch.size()),neiInG,denseBookkeeping ? gOffset : 0))
                    emitLogicalUSolution();
                return;
            }
            checkListMaximality();
        }

        void branchInCand(const int pivot){
            const int widthParentC=cand1->sz;
            statebit_list_search::branch(*this,pivot,[&](int& recExcl){
                if(listRecursiveUpperbound(pivot)){
                    updateExcl(recExcl,pivot);
                    if(!tryExportListBranch(widthParentC)) listBranchWithParent(widthParentC);
                }
            },[&](){
                listBranchWithParent(widthParentC);
            });
        }

        void listBranch(){
            listBranchWithParent(-1);
        }
        
        void listBranchWithParent(const int widthParentC)
        {
            table4Audit.inherited(forcedBaseSize(),plex->sz+cand1->sz);
            tryCommitFoldingEarlyOff();
            if(fixedLogicalSize()+cand1->sz<lb){
                return;
            }
            ForcedBaseFoldGuard forcedBaseGuard(this);
            PureEdBtSubtreeGuard pureFallbackGuard(this);
            if(foldingGateOptimizationEnabled()){
                if(inScalarListFallback()){
                    ++forcedBasePayoffGate.stats.subtree_fallback_nodes;
                }else{
                    forcedBaseStats.observeBefore(plex->sz,cand1->sz,forcedBaseSize(),widthParentC<0);
                    // FOLDING_GATE is runtime-enabled only after the seed gate decides ON.
                    if(forcedBaseTerminalBypassAllForced()){
                        forcedBaseStats.observeAfter(plex->sz,cand1->sz,0,forcedBaseSize());
                        checkAsMaxPlex();
                        return;
                    }
                    const bool applyForcedBase=shouldApplyForcedBaseFold(widthParentC);
                    if(applyForcedBase) applyForcedBaseFold(forcedBaseGuard.frame);
                    forcedBaseStats.observeAfter(plex->sz,cand1->sz,forcedBaseGuard.frame.folded,forcedBaseSize());
                }
            }
            if(cand1->sz == 0){
                if(foldingGateForcedSemanticsActive() && !inScalarListFallback()){
                    buildLogicalUSolution();
                    if(logicalUIsLocallyMaximal() &&
                       plexMaxChecker->isMaximal(forcedSolutionScratch.data(),static_cast<int>(forcedSolutionScratch.size()),neiInG,denseBookkeeping ? gOffset : 0))
                        emitLogicalUSolution();
                    return;
                }
                if (excl->sz == 0 && plexMaxChecker->isMaximal(plex->members, plex->sz, neiInP,denseBookkeeping ? pOffset : 0))
                {
                    #ifdef SET
                    emitVerifiedPlex(plex);
                    #else
                    emitter->emitPlex();
                    #endif
                }
                return;
            }
// WIDTH_POLICY complete-subtree accounting, scoped to this List node after native
            // feasibility/Folding handling. Child List scopes nest safely.
            statebit_width_selection::Scope widthPolicyAdmissionScope;
            if(k>=2 && forcedBaseSize()==0 && forcedBaseState256Eligible(plex->sz+cand1->sz,FORCED_BASE_STATE256_THRESHOLD))++wide_cost::tls().empty256_eligible;
            if(k>=2 && wide_cost::allow256(forcedBaseSize(),foldingGateOptimizationEnabled(),DIRECT_EMPTY_FORCED_ENTRY && enableDirectWideEntry) && forcedBaseState256Eligible(plex->sz+cand1->sz,FORCED_BASE_STATE256_THRESHOLD) && statebit_unified::plan({plex->sz+cand1->sz,cand1->sz,k},{false,false,true})==statebit_unified::Route::State256 && statebit_width_selection::admit(plex->sz+cand1->sz,cand1->sz,k,256)){
                if(forcedBaseSize()==0)++wide_cost::tls().empty256_accepted;
                ++state256Stats.eligible_nodes;
                if(forcedBaseSize()>0)observeFoldingHandoff();
                const unsigned long long s2560=state256Stats.nodes;
                const unsigned long long s1280=state128Stats.nodes;
                const unsigned long long b2560=bitStats.nodes;
                forcedBaseState256EnumerateCurrentTask();
                if(forcedBaseGuard.frame.folded>0){
                    const unsigned long long payoff256=(state256Stats.nodes-s2560)+(state128Stats.nodes-s1280)+(bitStats.nodes-b2560);
                    forcedBasePayoffGate.observePayoff(payoff256,3);
                }
                return;
            }
            if(k>=2 && (foldingGateOptimizationEnabled() && forcedBaseSize()>0 || DIRECT_EMPTY_FORCED_ENTRY && forcedBaseSize()==0) && forcedBaseState128Eligible(plex->sz+cand1->sz,FORCED_BASE_STATE128_THRESHOLD) && statebit_unified::plan({plex->sz+cand1->sz,cand1->sz,k},{false,true,false})==statebit_unified::Route::State128 && statebit_width_selection::admit(plex->sz+cand1->sz,cand1->sz,k,128)){
                ++state128Stats.eligible_nodes;
                if(forcedBaseSize()==0) ++directEmpty128Handoffs;
                // Direct empty-ForcedBase entry must not change FOLDING_GATE gate decisions.
                if(forcedBaseSize()>0) observeFoldingHandoff();
                const unsigned long long s0=state128Stats.nodes,b0=bitStats.nodes;
                forcedBaseState128EnumerateCurrentTask();
                if(forcedBaseGuard.frame.folded>0)
                    forcedBasePayoffGate.observePayoff((state128Stats.nodes-s0)+(bitStats.nodes-b0),1);
                return;
            }
            if(k>=2 && (forcedBaseSize()>0 || DIRECT_EMPTY_FORCED_ENTRY) && plex->sz+cand1->sz<=64)
                forcedBaseBit64Stats.observeEligible(plex->sz+cand1->sz);
            if(k>=2 && (foldingGateOptimizationEnabled() && forcedBaseSize()>0 || DIRECT_EMPTY_FORCED_ENTRY && forcedBaseSize()==0) && forcedBaseBit64Eligible(plex->sz+cand1->sz,FORCED_BASE_BIT64_THRESHOLD)){
                // Independent bit entry is not a ForcedBase/FOLDING_GATE handoff.
                if(forcedBaseSize()>0) observeFoldingHandoff();
                const unsigned long long b0=bitStats.nodes;
                if(forcedBaseSize()==0) ++directEmpty64Handoffs;
                forcedBaseBit64EnumerateCurrentTask();
                if(forcedBaseGuard.frame.folded>0)
                    forcedBasePayoffGate.observePayoff(bitStats.nodes-b0,2);
                return;
            }
            statebit_list_search::list_pivot(*this,fixedLogicalSize(),k,lb);
        }
