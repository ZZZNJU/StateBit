    void buildCommonMtx(const Graph &subg,const int blkSz,const int hopSz,uint8_t *commonMtx){
        const int thresPP1=lb-k-2*max(k-2,0),thresPP2=lb-k-2*max(k-3,0);
        const int thresPC1=lb-2*k-max(k-2,0),thresPC2=lb-k-1-max(k-2,0)-max(k-3,0);
        const int thresCC1=lb-2*k-(k-1),thresCC2=lb-2*k+2-(k-1);
       
        for(int i=0;i<hopSz;++i){
            for(int j=0;j<i;++j){
                const int common=utils::commonEle(
                    subg.V[i].Neighbors,
                    subg.V[i].degreeHop,
                    subg.V[j].Neighbors,
                    subg.V[j].degreeHop
                );
                if(commonMtx[static_cast<size_t>(i)*static_cast<size_t>(blkSz)+static_cast<size_t>(j)]){
                    if(common>thresCC1)commonMtx[static_cast<size_t>(i)*static_cast<size_t>(blkSz)+static_cast<size_t>(j)]=LINK2MORE;
                    else if(common==thresCC1)commonMtx[static_cast<size_t>(i)*static_cast<size_t>(blkSz)+static_cast<size_t>(j)]=LINK2EQUAL;
                    else commonMtx[static_cast<size_t>(i)*static_cast<size_t>(blkSz)+static_cast<size_t>(j)]=LINK2LESS;
                }
                else{
                    if(common>thresCC2)commonMtx[static_cast<size_t>(i)*static_cast<size_t>(blkSz)+static_cast<size_t>(j)]=UNLINK2MORE;
                    else if(common==thresCC2)commonMtx[static_cast<size_t>(i)*static_cast<size_t>(blkSz)+static_cast<size_t>(j)]=UNLINK2EQUAL;
                    else commonMtx[static_cast<size_t>(i)*static_cast<size_t>(blkSz)+static_cast<size_t>(j)]=UNLINK2LESS;
                }
                commonMtx[static_cast<size_t>(j)*static_cast<size_t>(blkSz)+static_cast<size_t>(i)]=commonMtx[static_cast<size_t>(i)*static_cast<size_t>(blkSz)+static_cast<size_t>(j)];
            }
        }
        
        for(int i=hopSz;i<blkSz;++i){
            for(int j=0;j<hopSz;++j){
                const int common=utils::commonEle(
                    subg.V[i].Neighbors,
                    subg.V[i].degreeHop,
                    subg.V[j].Neighbors,
                    subg.V[j].degreeHop
                );
                if(commonMtx[static_cast<size_t>(i)*static_cast<size_t>(blkSz)+static_cast<size_t>(j)]){
                    if(common>thresPC1)commonMtx[static_cast<size_t>(i)*static_cast<size_t>(blkSz)+static_cast<size_t>(j)]=LINK2MORE;
                    else if(common==thresPC1)commonMtx[static_cast<size_t>(i)*static_cast<size_t>(blkSz)+static_cast<size_t>(j)]=LINK2EQUAL;
                    else commonMtx[static_cast<size_t>(i)*static_cast<size_t>(blkSz)+static_cast<size_t>(j)]=LINK2LESS;
                }
                else{
                    if(common>thresPC2)commonMtx[static_cast<size_t>(i)*static_cast<size_t>(blkSz)+static_cast<size_t>(j)]=UNLINK2MORE;
                    else if(common==thresPC2)commonMtx[static_cast<size_t>(i)*static_cast<size_t>(blkSz)+static_cast<size_t>(j)]=UNLINK2EQUAL;
                    else commonMtx[static_cast<size_t>(i)*static_cast<size_t>(blkSz)+static_cast<size_t>(j)]=UNLINK2LESS;
                }
                commonMtx[static_cast<size_t>(j)*static_cast<size_t>(blkSz)+static_cast<size_t>(i)]=commonMtx[static_cast<size_t>(i)*static_cast<size_t>(blkSz)+static_cast<size_t>(j)];
            }
            if(k==2)continue;
           
            for(int j=hopSz;j<i;++j){
                const int common=utils::commonEle(
                    subg.V[i].Neighbors,
                    subg.V[i].degreeHop,
                    subg.V[j].Neighbors,
                    subg.V[j].degreeHop
                );
                if(commonMtx[static_cast<size_t>(i)*static_cast<size_t>(blkSz)+static_cast<size_t>(j)]){
                    if(common>thresPP1)commonMtx[static_cast<size_t>(i)*static_cast<size_t>(blkSz)+static_cast<size_t>(j)]=LINK2MORE;
                    else if(common==thresPP1)commonMtx[static_cast<size_t>(i)*static_cast<size_t>(blkSz)+static_cast<size_t>(j)]=LINK2EQUAL;
                    else commonMtx[static_cast<size_t>(i)*static_cast<size_t>(blkSz)+static_cast<size_t>(j)]=LINK2LESS;
                }
                else{
                    if(common>thresPP2)commonMtx[static_cast<size_t>(i)*static_cast<size_t>(blkSz)+static_cast<size_t>(j)]=UNLINK2MORE;
                    else if(common==thresPP2)commonMtx[static_cast<size_t>(i)*static_cast<size_t>(blkSz)+static_cast<size_t>(j)]=UNLINK2EQUAL;
                    else commonMtx[static_cast<size_t>(i)*static_cast<size_t>(blkSz)+static_cast<size_t>(j)]=UNLINK2LESS;
                }
                commonMtx[static_cast<size_t>(j)*static_cast<size_t>(blkSz)+static_cast<size_t>(i)]=commonMtx[static_cast<size_t>(i)*static_cast<size_t>(blkSz)+static_cast<size_t>(j)];
            }
        }
    }
