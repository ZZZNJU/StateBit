        bool canFormPlexActive(const int u,const int extra)
        {
            if (getNeiInP(u) + k < plex->sz + 1 ||
                getNeiInG(u) + k < max(lb,fixedLogicalSize()) + extra)
                return false;
            for (int i = 0; i < plex->sz; i++)
            {
                const int v = plex->members[i];
                assert(v != u);
                if (getNeiInP(v) + k == plex->sz && !isAdjMtx(v,u)) return false;
            }
            return true;
        }

        // X is intentionally maintained as a conservative superset during
        // ForcedBase recursion.  At a terminal whole-U check we test exact
        // extendability against the full logical U=P|F|C.
        bool canExtendCurrentLogicalU(const int x) const
        {
            const int usz=logicalUSize();
            if (getNeiInG(x) + k < usz + 1) return false;
            for(int i=0;i<plex->sz;++i){
                const int v=plex->members[i];
                if(getNeiInG(v)+k==usz && !const_cast<SearchContext*>(this)->isAdjMtx(v,x)) return false;
            }
            for(int i=0;i<cand1->sz;++i){
                const int v=cand1->members[i];
                if(getNeiInG(v)+k==usz && !const_cast<SearchContext*>(this)->isAdjMtx(v,x)) return false;
            }
            // For the project cases k>=2 a universal F vertex is never
            // saturated.  Keep the exact F check anyway so this helper also
            // remains correct if a k=1 diagnostic is ever run.
            for(const int v:forcedBase){
                if(getNeiInG(v)+k==usz && !const_cast<SearchContext*>(this)->isAdjMtx(v,x)) return false;
            }
            return true;
        }

        void buildLogicalUSolution(){
            forcedSolutionScratch.clear();
            forcedSolutionScratch.reserve(static_cast<size_t>(logicalUSize()));
            for(int i=0;i<plex->sz;++i) forcedSolutionScratch.push_back(plex->members[i]);
            forcedSolutionScratch.insert(forcedSolutionScratch.end(),forcedBase.begin(),forcedBase.end());
            for(int i=0;i<cand1->sz;++i) forcedSolutionScratch.push_back(cand1->members[i]);
        }

        void emitLogicalUSolution(){
#ifdef SET
            Stack<int> out(static_cast<int>(forcedSolutionScratch.size()));
            for(const int v:forcedSolutionScratch) out.push(v);
            emitVerifiedPlex(&out);
#else
            emitter->emitPlex();
#endif
        }

        bool logicalUIsLocallyMaximal(){
            for(int i=0;i<excl->sz;++i)
                if(canExtendCurrentLogicalU(excl->members[i])) return false;
            return true;
        }

        // List-front-end recursive greedy upper bound. This is intentionally
        // retained only in the EDBT/list recursion, before LIST task materialization.
        // Scalar-64/StateBit recursive greedy upper bounds remain removed.
        bool listRecursiveUpperbound(const int u){
            for(int t=0;t<plex->sz;t++){
                const int v = plex->members[t];
                if (getNeiInG(v)+k<lb) return false;
                nonadjInP[t] = plex->sz - getNeiInP(v);
            }

            if(!subg.proper) return true;

            int cnt = k+getNeiInP(u)+forcedBaseSize();
            for(int i=0;i<cand1->sz;i++){
                int max_noadj = -1;
                int max_index = 0;
                const int ele = cand1->members[i];
                if(isAdjMtx(u,ele)){
                    for(int j=0;j<plex->sz;j++){
                        const int v = plex->members[j];
                        if(!isAdjMtx(ele,v)&&nonadjInP[j]>max_noadj){
                            max_noadj = nonadjInP[j];
                            max_index = j;
                        }
                    }
                    if(max_noadj<k) {
                        cnt++;
                        nonadjInP[max_index]++;
                        if(cnt>=lb) return true;
                    }
                }
            }
            return (cnt>=lb);
        }

        // Initial-task upper bound used while constructing/enumerating the
        // top-level S subtask. This is independent of the list-recursive UB above.
        bool upperboundK(){
            int cnt = plex->sz;
            for(int t=0;t<plex->sz;t++){
                const int v = plex->members[t];
                if (getNeiInG(v)+k<lb) return false;
                nonadjInP[t] = plex->sz - getNeiInP(v);
            }

            if(!subg.proper) return true;

            for(int i=0;i<cand1->sz;i++){
                int max_noadj = -1;
                int max_index = 0;
                const int ele = cand1->members[i];
                for(int j=1;j<plex->sz;j++){
                    const int v = plex->members[j];
                    if(!isAdjMtx(ele,v)&&nonadjInP[j]>max_noadj){
                        max_noadj = nonadjInP[j];
                        max_index = j;
                    }
                }
                if(max_noadj<k) {
                    cnt++;
                    nonadjInP[max_index]++;
                }
                if(cnt>=lb) return true;
            }
            return cnt>=lb;
        }

        SearchContext* cand1ToPlex(const int v)
        {
            plex->push(v);
            cand1->remove(v);
            addP(v);
            return this;
        }

        SearchContext* plexToCand1()
        {
            assert(plex->sz > 0);
            const int u = plex->top();
            cand1->add(u);
            plex->pop();
            subP(u);
            return this;
        }

        int cand2BackToPlex()
        {
            const int v=cand2->pop_back();
            plex->push(v);
            addP(v);
            addG(v);
            return v;
        }

        SearchContext* plexToCand2()
        {
            assert(plex->sz > 0);
            const int u = plex->top();
            cand2->add(u);
            plex->pop();
            subP(u);
            subG(u);
            return this;
        }

        SearchContext* cand1ToExcl(const int v)
        {
            cand1->remove(v);
            excl->add(v);
            subG(v);
            return this;
        }

        SearchContext* exclToCand1(const int v)
        {
            excl->remove(v);
            cand1->add(v);
            addG(v);
            return this;
        }

        int cand2BackToExcl()
        {
            const int u=cand2->pop_back();
            excl->add(u);
            return u;
        }

        SearchContext* exclToCand2(const int v)
        {
            excl->remove(v);
            cand2->add(v);
            return this;
        }

        SearchContext* updateExcl(int& recExcl,const int v2add){
            recExcl=excl->sz;
            for (int i = 0; i < excl->sz;)
            {
                const int ele = excl->members[i];
                if((subg.proper&&UNLINK2MORE>commonMtx[getIdx(ele,v2add)])||!canFormPlex(ele,1)){
                    excl->remove(ele);
                    exclStack->push(ele);
                }
                else ++i;
            }
            recExcl-=excl->sz;
            return this;
        }

        SearchContext* updateExclK(int& recExcl,const int v2add){
            recExcl=excl->sz;
            for (int i = 0; i < excl->sz;)
            {
                int ele = excl->members[i];
                if(subg.proper&&UNLINK2MORE>commonMtx[getIdx(ele,v2add)]){
                    excl->remove(ele);
                    exclStack->push(ele);
                }
                else ++i;
            }
            recExcl-=excl->sz;
            return this;
        }

        SearchContext* updateCand1(int& recCand1,const int v2add){
            recCand1=cand1->sz;
            for (int i = 0; i < cand1->sz;)
            {
                const int ele = cand1->members[i];
                if((subg.proper&&UNLINK2EQUAL>commonMtx[getIdx(ele,v2add)])||!canFormPlexActive(ele,0)){
                    cand1->remove(ele);
                    subG(ele);
                }
                else ++i;
            }
            recCand1-=cand1->sz;
            return this;
        }

        SearchContext* updateCand1Fake(int& recCand1,const  int v2add){
            recCand1=cand1->sz;
            for (int i = 0; i < cand1->sz;)
            {
                int ele = cand1->members[i];
                if((subg.proper&&UNLINK2EQUAL>commonMtx[getIdx(v2add,ele)])||!canFormPlexActive(ele,0)){
                    cand1->fakeRemove(ele);
                    subG(ele);
                }
                else ++i;
            }
            recCand1-=cand1->sz;
            return this;
        }

        SearchContext* updateCand1KFake(int& recCand1,const  int v2add){
            recCand1=cand1->sz;
            for (int i = 0; i < cand1->sz;)
            {
                int ele = cand1->members[i];
                if(subg.proper&&UNLINK2EQUAL>commonMtx[getIdx(v2add,ele)]){
                    cand1->fakeRemove(ele);
                    subG(ele);
                }
                else ++i;
            }
            recCand1-=cand1->sz;
            return this;
        }

        SearchContext* updateCand2Fake(int& recCand2,const int v2add){
            recCand2=cand2->sz;
            for (int i = 0; i < cand2->sz;)
            {
                int ele = cand2->members[i];
                if(subg.proper&&UNLINK2EQUAL>commonMtx[getIdx(v2add,ele)]){
                    cand2->fakeRemove(ele);
                }
                else ++i;
            }
            recCand2-=cand2->sz;
            return this;
        }
        
        bool canFormPlex(const int u,const int extra)
        {
            if (getNeiInP(u) + k < plex->sz + 1 || getNeiInG(u) + k < max(lb,plex->sz)+extra)
                return false;
            
            for (int i = 0; i < plex->sz; i++)
            {
                const int v = plex->members[i];
                assert(v != u);
                if (getNeiInP(v) + k == plex->sz && !isAdjMtx(v, u))
                { //v is saturated by v,u are not neighbors
                    return false;
                }
            }
            return true;
        }
        
