#pragma once
#include <vector>
#include <cstdint>
#include <chrono>
#include <cstdio>

#define DENSE_TOUCH_TELEMETRY 1

#define ADAPTIVE_VALIDATION_SELECTOR 0

namespace statebit {
struct ComplementCSR {
    std::vector<int> offsets;
    std::vector<int> verts;
    void clear(){ offsets.clear(); verts.clear(); }
    template<class GraphT>
    void build(const GraphT& g){
        const int n=g.n;
        offsets.assign(n+1,0);
        unsigned long long total=0;
        for(int u=0;u<n;++u){
            const unsigned long long non=static_cast<unsigned long long>(n-1-g.V[u].degree);
            total+=non;
            offsets[u+1]=static_cast<int>(total);
        }
        verts.resize(static_cast<size_t>(total));
        size_t out=0;
        for(int u=0;u<n;++u){
            int q=0;
            const int deg=g.V[u].degree;
            const int* N=g.V[u].Neighbors;
            for(int v=0;v<n;++v){
                if(v==u) continue;
                while(q<deg && N[q]<v) ++q;
                if(q<deg && N[q]==v){ ++q; continue; }
                verts[out++]=v;
            }
        }
    }
    const int* begin(int u) const { return verts.data()+offsets[u]; }
    const int* end(int u) const { return verts.data()+offsets[u+1]; }
    int size(int u) const { return offsets[u+1]-offsets[u]; }
    unsigned long long entries() const { return static_cast<unsigned long long>(verts.size()); }
};


struct DenseTouchStats {
    unsigned long long addG_calls=0,subG_calls=0,addP_calls=0,subP_calls=0;
    unsigned long long neighbor_touch_estimate=0,complement_touches=0;
    inline void merge(const DenseTouchStats&o){ addG_calls+=o.addG_calls; subG_calls+=o.subG_calls; addP_calls+=o.addP_calls; subP_calls+=o.subP_calls; neighbor_touch_estimate+=o.neighbor_touch_estimate; complement_touches+=o.complement_touches; }
};
extern thread_local DenseTouchStats denseTouchStats;

struct SeedModeStats {
    unsigned long long seeds=0;
    unsigned long long actual_dense_seeds=0,actual_sparse_seeds=0;
    unsigned long long predicted_dense_seeds=0,predicted_sparse_seeds=0;
    unsigned long long neighbor_proxy=0,nonneighbor_proxy=0;
    unsigned long long actual_dense_vertices=0,actual_sparse_vertices=0;
    unsigned long long actual_dense_n2=0,actual_sparse_n2=0;
    unsigned long long dense_neighbor_proxy=0,dense_nonneighbor_proxy=0;
    unsigned long long sparse_neighbor_proxy=0,sparse_nonneighbor_proxy=0;
    unsigned long long complement_build_calls=0,complement_build_vertices=0,complement_build_entries=0;
    unsigned long long complement_build_ns=0;
    void merge(const SeedModeStats& o){
        seeds+=o.seeds;
        actual_dense_seeds+=o.actual_dense_seeds; actual_sparse_seeds+=o.actual_sparse_seeds;
        predicted_dense_seeds+=o.predicted_dense_seeds; predicted_sparse_seeds+=o.predicted_sparse_seeds;
        neighbor_proxy+=o.neighbor_proxy; nonneighbor_proxy+=o.nonneighbor_proxy;
        actual_dense_vertices+=o.actual_dense_vertices; actual_sparse_vertices+=o.actual_sparse_vertices;
        actual_dense_n2+=o.actual_dense_n2; actual_sparse_n2+=o.actual_sparse_n2;
        dense_neighbor_proxy+=o.dense_neighbor_proxy; dense_nonneighbor_proxy+=o.dense_nonneighbor_proxy;
        sparse_neighbor_proxy+=o.sparse_neighbor_proxy; sparse_nonneighbor_proxy+=o.sparse_nonneighbor_proxy;
        complement_build_calls+=o.complement_build_calls;
        complement_build_vertices+=o.complement_build_vertices;
        complement_build_entries+=o.complement_build_entries;
        complement_build_ns+=o.complement_build_ns;
    }
};

inline bool adaptiveWouldDense(unsigned long long neighborProxy,unsigned long long nonNeighborProxy){
    return nonNeighborProxy*static_cast<unsigned long long>(ADAPTIVE_DENSE_FACTOR)<neighborProxy;
}
inline bool chooseDenseMode(unsigned long long neighborProxy,unsigned long long nonNeighborProxy){
    return adaptiveWouldDense(neighborProxy,nonNeighborProxy);
}
inline const char* bookkeepModeName(){
    return "DENSE_BOOKKEEPING_EXTERNAL_AGGREGATION_ADAPTIVE";
}
}
