#pragma once
#include <cstdio>
namespace statebit {
struct TransferAudit {
 unsigned long long early_calls=0,early_skips=0,snapshot128=0,snapshot256=0,snapshot_list=0;
 unsigned long long remap_old=0,remap_projected=0,projected_rows=0;
 void report(int id) const {
  std::printf("TRANSFER_WORKER id=%d early_calls=%llu early_skips=%llu snapshot128=%llu snapshot256=%llu snapshot_list=%llu remap_old=%llu remap_projected=%llu projected_rows=%llu\n",id,early_calls,early_skips,snapshot128,snapshot256,snapshot_list,remap_old,remap_projected,projected_rows);
 }
};
extern thread_local TransferAudit transferAudit;
}
