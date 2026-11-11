    struct GlobalMaximalityChecker{
        const graph<int> subg;
        GlobalMaximalityChecker(const graph<int> &_subg)
            : subg(_subg){}
        bool canGlobalAdd(int u, int szplex,int* plex){
            int cnt = 0;
            for (int j = 0; j < szplex;++j)
            {
                const int ele = plex[j];
                if(binary_search(subg.V[ele].NeighborsLeft,subg.V[ele].NeighborsLeft+subg.V[ele].degreeLeft,u))
                    cnt++;
            }
            if (cnt + k >= szplex + 1)
                return true; //can add
            return false;	//can not add
        }
        bool isMaximal(int *list, int szplex, const int* neiInPBase, int neiOffset)
        {
            // Temporary storage follows the selected set and left-side vertex IDs.
            vector<int> sat;
            sat.reserve(static_cast<size_t>(szplex));
            bool max = true;
            static thread_local RandSet<int> uni(64);
            auto uniAdd = [&](const int ele){
                if(ele>=uni.cap) uni.resize(ele+1);
                if(!uni.contains(ele)) uni.add(ele);
            };
            for (int i = 0; i < szplex; i++){
                const int v = list[i];
                if (neiInPBase[v] + neiOffset + k == szplex) sat.emplace_back(v);
            }
            const int nsat=static_cast<int>(sat.size());
            if (nsat){
                const int v0 = sat[0];
                vector<int> commons(subg.V[v0].NeighborsLeft,
                                    subg.V[v0].NeighborsLeft + subg.V[v0].degreeLeft);
                int szcom = static_cast<int>(commons.size());
                for (int i = 1; i < nsat; i++){
                    const int u = sat[i];
                    szcom = utils::interSection(subg.V[u].NeighborsLeft, subg.V[u].degreeLeft,
                                                commons.data(), szcom, commons.data());
                }
                // commons contains all common left-adjacent vertices of
                // saturated vertices.
                if(nsat==1){
                    for (int i = 0; i < k && i < szplex;++i){
                        const int u = list[i];
                        for (int j = 0; j < subg.V[u].degreeLeft;++j)
                            uniAdd(subg.V[u].NeighborsLeft[j]);
                    }
                    int out = 0;
                    for (int i = 0; i < szcom;++i)
                        if(uni.contains(commons[i])) commons[out++]=commons[i];
                    szcom=out;
                    uni.clearByEle();
                }
                for (int i = 0; i < szcom; i++){
                    const int u = commons[i];
                    if (canGlobalAdd(u, szplex, list)){
                        max = false;
                        break;
                    }
                }
            }
            else{
                for (int i = 0; i < k && i < szplex;++i){
                    const int u = list[i];
                    for (int j = 0; j < subg.V[u].degreeLeft;++j)
                        uniAdd(subg.V[u].NeighborsLeft[j]);
                }
                for (int i = 0; i < uni.sz;++i){
                    const int ele = uni.members[i];
                    if (canGlobalAdd(ele, szplex,list)){
                        max = false;
                        break;
                    }
                }
                uni.clearByEle();
            }
            return max;
        }
    };
