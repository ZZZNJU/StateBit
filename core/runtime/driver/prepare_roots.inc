        int *dpos = new int[g.n];
        int *dseq = new int[g.n];
        bool *mark = new bool[g.n];
        int *resNei = new int[g.n];
        for(int i=0;i<g.n;++i)dpos[i]=INT_MAX;

        Timer watch;
        watch.start();

        Graph peelG=peelGraph(g,mark,resNei);
        const int pn=peelG.n;
        const int targetSeeds=(seedTaskLimit>0)?std::min(pn,seedTaskLimit):pn;
        for(int i=0;i<pn;++i)mark[i]=false;

        ListLinearHeap<int> *linear_heap = new ListLinearHeap<int>(pn, pn ? pn-1 : 0);
        linear_heap->init(pn, pn ? pn-1 : 0);
        for(int i=0;i<pn;++i) linear_heap->insert(i,peelG.V[i].degree);
        for (int i = 0; i < pn; i++) {
            int u, key;
            linear_heap->pop_min(u, key);
            dpos[u] = i;
            dseq[i] = u;
            for (int j = 0; j < peelG.V[u].degree; j++){
                const int nei = peelG.V[u].Neighbors[j];
                if(dpos[nei]==INT_MAX) linear_heap->decrement(nei);
            }
        }
        delete linear_heap;

        const int actualThreads = std::max(1, std::min(workerThreads, std::max(1,pn)));
        printf("PARALLEL_ENGINE_SEQ_CONFIG fastpath=0 recursive_split=%d dtfb=%d worker_threads_created=0_when_t1 direct_seed_loop=0\n",MT_RECURSIVE_SPLIT,MT_DTFB_SPLIT);
        std::atomic<int> nextSeed(0);
        std::atomic<int> completedSeeds(0);
        static constexpr size_t MT_QUEUE_CAP=MT_TASK_QUEUE_CAP;
        MTTaskPool taskPool(actualThreads,MT_QUEUE_CAP);
        std::vector<SeedWorkerSummary> summaries(actualThreads);


