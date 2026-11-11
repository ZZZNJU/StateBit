    Graph peelGraph(const Graph &g,bool* const mark,int * const resNei){
        const int n=g.n;
        static queue<int> Q;
        for(int i=0;i<n;++i){
            if(g.V[i].degree<bd){
                mark[i]=false;
                Q.push(i);
            }
            else{
                mark[i]=true;
                resNei[i]=g.V[i].degree;
            }
        }
        while(Q.size()){
            const int ele=Q.front();
            Q.pop();
            for(int i=0;i<g.V[ele].degree;++i){
                const int nei=g.V[ele].Neighbors[i];
                if(mark[nei]){
                    resNei[nei]--;
                    if(resNei[nei]<bd){
                        mark[nei]=false;
                        Q.push(nei);
                    }
                }
            }
        }
        int* const map=new int[n];
        for(int i=0;i<n;++i)map[i]=i;
        _seq<int> leadList = sequence::pack(map, mark, n);
        const int pn=leadList.n;
#ifdef SET
        verifyPeelOriginal.assign(leadList.A,leadList.A+pn);
#endif
        for(int i=0;i<pn;++i)map[leadList.A[i]]=i;
        vertex<int> *vertices = newA(vertex<int>, pn);
        for (int i = 0; i < pn; i++){
            const int ori = leadList.A[i];
            const int en=resNei[ori];
            vertices[i].Neighbors = newA(int,en);
            vertices[i].NeighborsLeft = nullptr;
            vertices[i].degree=en;
            int cursor=0;
            for(int j=0;j<g.V[ori].degree;++j){
                const int nei=g.V[ori].Neighbors[j];
                if(mark[nei])vertices[i].Neighbors[cursor++]=map[nei];
            }
        }
        delete[] map;
        leadList.del();
        return Graph(vertices, pn, 0, nullptr);
    }
    
