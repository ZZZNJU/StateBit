#pragma once
namespace statebit {
struct Simd128Audit {unsigned long long roots=0,export_calls=0,exported=0;};
extern thread_local Simd128Audit simd128Audit;
}
