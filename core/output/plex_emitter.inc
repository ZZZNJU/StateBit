    struct PlexEmitter{
#ifdef SET
        void emitVerified(const std::vector<int>& selected){
            unsigned long long h=1469598103934665603ULL;
            for(int v:selected){h^=static_cast<unsigned long long>(v)+1;h*=1099511628211ULL;}
            h^=selected.size();h*=1099511628211ULL;
            ++verifyCount;verifyXor^=h;verifySum+=h;++threadSolutionCount;
            if(std::getenv("CORE_VERIFY_SETS") && std::strcmp(std::getenv("CORE_VERIFY_SETS"),"1")==0){
                std::ostringstream row;for(int v:selected)row<<" "<<v;
                std::printf("VERIFY_SET%s\n",row.str().c_str());
            }
        }
#endif
        //only count the number
        #ifdef SET
        void emitPlex(Stack<int> * plex){
            for (int i = 0; i < plex->sz; i++) {
                cout << plex->members[i] << " ";
            }
            cout << "\n";
            threadSolutionCount++;
        }
        #else
        void emitPlex(){
            threadSolutionCount++;
        }
        #endif
    };
