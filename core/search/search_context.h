#pragma once
#include "../config/kernel_environment.h"

namespace statebit {
#include "search_types.inc"

template <typename MaximalityChecker, typename Emitter>
struct SearchContext {
#include "context/storage.inc"
#include "runtime/task_interface.inc"
#include "context/lifecycle.inc"
#include "context/bookkeeping.inc"
#include "list/constraints.inc"
#include "context/compact_masks.inc"
#include "kernels/state32_support.inc"
#include "folding/local_fixed.inc"
#include "kernels/state32_entry.inc"
#include "kernels/word128.inc"
#include "kernels/word256.inc"
#include "kernels/state64.inc"
#include "runtime/task_donation.inc"
#include "kernels/dispatch64.inc"
#include "context/bit_context_builder.inc"
#include "runtime/task_transfer.inc"
#include "folding/entry_fold.inc"
#include "list/entry.inc"
#include "folding/forced_vertices.inc"
#include "list/branching.inc"
};
} // namespace statebit
