    Graph buildSubgraph(const Graph &g,const vector<int>& blk,const vector<int>& left,const vector<uint8_t> &inBlk, const vector<uint8_t> &inLeft,const int hopSz,vector<uint8_t> &isNei,uint8_t *commonMtx){  
        const int blkSz = blk.size();
        const int leftSz = left.size();
        vertex<int> *vertices = newA(vertex<int>, blkSz);
        int edgesHop=0,edges=0;
        for (int i = 0; i < blkSz; i++){
            const int origin = blk[i];
            int ne = 0, neLeft = 0;
            for (int j = 0; j < g.V[origin].degree; j++){
                const int nei = g.V[origin].Neighbors[j];
                if (inBlk[nei]){ // in block
                    ne++;
                    isNei[nei]=true;
                }
                if (inLeft[nei]){
                    neLeft++;
                    isNei[nei]=true;
                }
            }
            vertices[i].Neighbors = newA(int,ne);
            vertices[i].NeighborsLeft = newA(int,neLeft);
            vertices[i].degree=ne;
            vertices[i].degreeLeft=neLeft;

            //add right edges      
            int cnt = 0;  
            for (int j = 0; j < blkSz; j++){
                const int nei = blk[j];
                commonMtx[static_cast<size_t>(i)*static_cast<size_t>(blkSz)+static_cast<size_t>(j)]=isNei[nei];
                if (isNei[nei]){
                    vertices[i].Neighbors[cnt++] = j;
                    isNei[nei]=false;
                }
                if(j==hopSz-1)vertices[i].degreeHop=cnt;
            }
            //add left edges
            cnt = 0;
            for (int j = 0; j < leftSz; j++){
                const int nei = left[j];
                if (isNei[nei]){
                    vertices[i].NeighborsLeft[cnt++] = j;
                    isNei[nei]=false;
                }
            }
            if(i<hopSz)edgesHop+=vertices[i].degreeHop;
            edges+=vertices[i].degree;
        }
        const bool proper=(double(edgesHop)/(hopSz*(hopSz-1)))<0.95 && (double(edges)/(blkSz*(blkSz-1)))<0.9;
        return Graph(vertices, blkSz, 0, nullptr, proper);
    }
    
