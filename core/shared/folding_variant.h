#pragma once
namespace folding_variant {
inline int maximum(int variant){return variant==1?64:40;}
inline bool scan(int variant,int live){return live>32 && live<=maximum(variant);}
inline bool entry(int variant,int live,int outer){return (variant==2 || variant==3) && outer==0 && live>32 && live<=64;}
inline bool commit(int variant,int after){return variant!=3 || after<=32;}
}
