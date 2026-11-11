        if(actualThreads==1){
            startProgressReporter();
            worker(0);
            stopProgressReporter();
        }else{
            std::vector<std::thread> pool;
            pool.reserve(actualThreads);
            for(int t=0;t<actualThreads;t++) pool.emplace_back(worker,t);
            startProgressReporter();
            for(auto &th:pool) th.join();
            stopProgressReporter();
        }

