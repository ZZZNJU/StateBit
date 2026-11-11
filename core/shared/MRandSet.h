#pragma once
#include <cassert>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include "MBitSet.h"


template <typename T=int> 
class RandSet{    
public:
    using TIndex = uint32_t;
    int sz;
    int cap;
    T *members;
    TIndex *pos; 
    MBitSet32 *flag;
    
public:

    //default constructor        
    RandSet():sz(0),cap(0),members(nullptr),flag(nullptr),pos(nullptr){}

    RandSet(int _maxvalue):cap(_maxvalue),sz(0){       
        members = new T[_maxvalue];
        pos = new TIndex[_maxvalue];
        flag = new MBitSet32(_maxvalue);
    }

    
    RandSet(const RandSet &_set){// copy constructor
        cap = _set.cap;
        sz = _set.sz;
        
        members = new T[cap];
        memcpy(members, _set.members, sizeof(T) * sz);

        pos = new TIndex[cap];
        memcpy(pos, _set.pos, sizeof(TIndex) * cap);

        flag = new MBitSet32(*(_set.flag));        
        
    }
    RandSet& operator=(const RandSet &_set) = delete;

    ~RandSet(){
        delete[] members;
        delete[] pos;
        delete flag;        
    }

    // Grow-only resize.  The original code only changed the logical cap,
    // which silently wrote past the 4096-element allocation on large seeds.
    // Here all backing arrays and the bit flag grow together and existing
    // contents are preserved.  We never shrink during a worker lifetime.
    void resize(const int _cap){
        if(_cap <= cap) return;
        T *newMembers = new T[_cap];
        TIndex *newPos = new TIndex[_cap];
        if(members && sz>0) memcpy(newMembers, members, sizeof(T) * sz);
        if(pos && cap>0) memcpy(newPos, pos, sizeof(TIndex) * cap);
        delete[] members;
        delete[] pos;
        members = newMembers;
        pos = newPos;
        if(flag==nullptr) flag = new MBitSet32(_cap);
        else flag->reserve(_cap);
        cap=_cap;
    }

    bool contains(const T& v){
        if(v < 0 || v >= cap) return false;
        return flag->test(v);
    }

    void add(const T& v){
        assert(v >= 0 && v < cap);
        assert(sz < cap);
        pos[v] = sz; 
        members[sz++]=v;
        flag->set(v); 
    }

    void fakeRemove(const T& v){
        //swap
        TIndex idx = pos[v];            
        pos[members[sz-1]] = idx;
        pos[v] = sz-1;
        std::swap(members[idx],members[--sz]);
    }

    void remove(const T& v){
        fakeRemove(v);
        flag->set(v);
    }

    template<typename Type>
    void recoverAdd(const int len,Type* listor){
        T* cursor=members+sz;
        for(int i=0;i<len;++i){
            const int u=*cursor;
            flag->set(u);
            listor->addG(u);
            cursor++;
        }
        sz+=len;
    }

    template<typename Type>
    void fakeRecoverAdd(const int len,Type* listor){
        T* cursor=members+sz;
        for(int i=0;i<len;++i){
            const int ele=*cursor;
            listor->addG(ele);
            cursor++;
        }
        sz+=len;
    }

    void fakeRecover(const int len){
        sz+=len;
    }
     
    int pop_back(){
        int v=members[--sz];
        flag->set(v);
        return v;
    }

    RandSet& operator+(const T& v){
        add(v);
        return *this;
    }
    
    void clear(){
        flag->clear();
        sz = 0;                
    }

    void clearByEle(){
        for(int i=0;i<sz;++i){
            flag->set(members[i]);
        }
        sz = 0;                
    }

    template<typename Fnc>
    void for_each(const Fnc &f){        
        for (TIndex i = 0; i < sz; i++){
            f(members[i]);
        }
    }
};