#pragma once
#include <cstdio>
namespace statebit {
struct Table4Audit {
    unsigned long long folded_gt256=0,inherited_gt256=0,folded_vertices_gt256=0,max_f=0,list_resume_gt256=0,bit_resume_f=0;
    void inherited(int f,int active){if(f>0 && active>256)++inherited_gt256;}
    void folded(int added,int f,int active){
        if(static_cast<unsigned long long>(f)>max_f)max_f=f;
        if(added>0 && active>256){++folded_gt256;folded_vertices_gt256+=added;}
    }
    void report(int tid)const{
        std::printf("CAPABILITY_WORKER id=%d folded_gt256=%llu inherited_gt256=%llu folded_vertices_gt256=%llu max_f=%llu list_resume_gt256=%llu bit_resume_f=%llu\n",tid,folded_gt256,inherited_gt256,folded_vertices_gt256,max_f,list_resume_gt256,bit_resume_f);
    }
};
extern thread_local Table4Audit table4Audit;
}
