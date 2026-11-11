    int decompose(const Graph &g,const int vstart,vector<int> &neibors_1a2hop,vector<int> &neibors_1a2hopLeft,const int* dpos,vector<uint8_t> &visited,vector<int> &count,vector<int> &neibors,vector<int> &neiborsLeft){
        //collect 1 hop neigbors
        visited[vstart]  = 1;
        for(int j = 0; j < g.V[vstart].degree; j++){
            const int nei = g.V[vstart].Neighbors[j];                    
            if (!visited[nei]){
                visited[nei] = 1;
                if(dpos[vstart]<dpos[nei])//1-hop right-neighbor
                    neibors_1a2hop.emplace_back(nei);
                else//1-hop left-neighbor
                    neibors_1a2hopLeft.emplace_back(nei);
            }
        }
        auto filter = [&](const std::vector<int>& origin,const std::vector<int>& neibors_1hop,const int thres) {
            std::vector<int> filtered;
            const int sz2 = neibors_1hop.size();
            for (const auto u : origin) {
                int cnt = 0; // common-neighbor count must compare signed thresholds at q=2*k-1
                const int sz1 = g.V[u].degree;
                //choose proper algorithm by time complexity estimation
                if(sz2 * log2(sz1) < (sz2 + sz1)){
                    for (auto v : neibors_1hop) {
                        if (g.isAdj(u,v))cnt++;
                    }
                }
                else{
                    const int *const lst1 = g.V[u].Neighbors;
                    const int *const lst2 = neibors_1hop.data();
                    int j1 = 0, j2 = 0;
                    while (j1 < sz1 && j2 < sz2) {
                        if (lst1[j1] < lst2[j2])
                            ++j1;
                        else if (lst1[j1] > lst2[j2])++j2;
                        else {
                            cnt++,j1++, j2++;
                        }
                    }
                }
                if (cnt >= thres) filtered.push_back(u);
                else
                    visited[u] = 0;
            }
            return filtered;
        };
        size_t sz;
        const int lb_2k = lb - 2 * k;
        do {
            sz = neibors_1a2hop.size();
            if(sz<bd)goto CLEAN;
            //the number of common neighbors with vstart >= lb-2*k
            neibors_1a2hop = filter(neibors_1a2hop, neibors_1a2hop, lb_2k);
        } while (neibors_1a2hop.size() < sz);
        //the number of common neighbors with vstart >= lb+1-2*k
        neibors_1a2hopLeft = filter(neibors_1a2hopLeft, neibors_1a2hop,lb_2k+1);

        //collect 2-hop neigbors
        for(const auto nei1 : neibors_1a2hop){
            for (int j = 0; j < g.V[nei1].degree; j++)
            {
                const int nei2 = g.V[nei1].Neighbors[j];
                if (!visited[nei2]){
                    if(!count[nei2]){
                        if(dpos[vstart]<dpos[nei2])
                            neibors.emplace_back(nei2);
                        else
                            neiborsLeft.emplace_back(nei2);
                    }
                    count[nei2]++;
                }
            }
        }
        neibors_1a2hop.emplace_back(vstart);
        swap(neibors_1a2hop[0], neibors_1a2hop[neibors_1a2hop.size() - 1]);
        //In fact, neibors_1a2hop contains the vstart at the first

        //restore the global variables
    CLEAN:
        for (auto v : neibors_1a2hop ){
            visited[v] = 0;
        }
        for (auto v : neibors_1a2hopLeft){
            visited[v] = 0;
        }
        for (int v : neibors) {
            //the number of common neighbors with vstart >= lb-2*k+2
            if (count[v] >= lb_2k + 2) {
                neibors_1a2hop.emplace_back(v);
            }
            count[v] = 0;
        }
        for (int v : neiborsLeft) {
            //the number of common neighbors with vstart >= lb+1-2*k+2
            if (count[v] >= lb_2k+3) {
                neibors_1a2hopLeft.emplace_back(v);
            }
            count[v] = 0;
        }
        neibors.clear();
        neiborsLeft.clear();
        return sz+1;
    }
