#pragma once
#include <cstdint>
namespace remat_budget {
inline std::int64_t slack(int active,int local,int outer,int q){return std::int64_t(active)+local+outer-q;}
inline bool possible(int outer,int q,int minimum){return minimum==0 || std::int64_t(32)+outer-q>=minimum;}
inline bool allow(int active,int local,int outer,int q,int minimum){return minimum==0 || slack(active,local,outer,q)>=minimum;}
inline int bucket(std::int64_t s){return s<0?0:(s<=2?int(s)+1:(s<=4?4:(s<=8?5:(s<=16?6:7))));}
}
