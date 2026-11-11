#include "../shared/experiment_control.h"
#include <memory>
#include "../shared/kernel_selection.h"
#include "../runtime/coverage.h"
#include "../shared/c0_opportunity.h"
#include "../shared/mixed_f64.h"
#include "../shared/wide_cost.h"
#include "../shared/transfer_audit.h"
#include "../shared/simd128_audit.h"
extern bool enableState128Simd;
extern bool enableQueuePrecheck,enableRemap128;
extern bool enableDirectWideEntry;
#include "../shared/list_search.h"
#include "../shared/statebit_unified_router.h"
#include "../shared/width_selection.h"
#include "../shared/state32_handoff.h"
#include "../shared/statebit_core_bridge.h"
#include "../shared/word64_common.h"
#include "../shared/state64_common_refine.h"
#include "../shared/state64_common_pivot.h"
// STATE_WITNESS: State64 Whole-Witness recurrence.  OFF = original PARALLEL_ENGINE dense whole scan.
#include "../shared/state32_defect_cache.h"
// EAGER_FOLDING-OPTIMIZED: exact local-F State32 boundary; all logical maximality checks use S|F.

#include "../shared/state64_search.h"
#include "../shared/node_transition.h"
#include "../shared/word64_search.h"
#include "../shared/search_core.h"
#include "../shared/fresh64_policy.h"
#include "../shared/local_fixed_contract.h"
#include "../shared/state32_entry.h"
#include "../shared/forced_left_constraints.h"
#include "../shared/state32_forced_context.h"
#define AVX_PACKET_WIDTH 8
#define AVX_VECTOR_PIVOT 0
#define AVX_GATHER_REFINE 0
#define AVX_COMPACTION 0
#define VECTOR_TELEMETRY 0
#define STEADY_KERNEL_PROFILE 0
#ifndef STEADY_KERNEL_SAMPLE_SHIFT
#define STEADY_KERNEL_SAMPLE_SHIFT 12
#endif
#define WITNESS_RESUME_PROFILE 0
#define WITNESS_RESUME_VERIFY 0
#ifndef WITNESS_RESUME_PROFILE_SHIFT
#define WITNESS_RESUME_PROFILE_SHIFT 10
#endif
#ifndef WITNESS_RESUME_VERIFY_SHIFT
#define WITNESS_RESUME_VERIFY_SHIFT 16
#endif
#define PACKET_PREPARE_VALIDATION 0
#ifndef PACKET_PREPARE_VALIDATION_SAMPLE_SHIFT
#define PACKET_PREPARE_VALIDATION_SAMPLE_SHIFT 13
#endif
#define PACKET_PREPARE_FULL_VALIDATION 0
#ifndef PACKET_PREPARE_FULL_VALIDATION_SAMPLE_SHIFT
#define PACKET_PREPARE_FULL_VALIDATION_SAMPLE_SHIFT 12
#endif
#ifndef PACKET_PREPARE_FULL_VALIDATION_REPEATS
#define PACKET_PREPARE_FULL_VALIDATION_REPEATS 8
#endif
#define WHOLECHECK8V_VALIDATION 0
#ifndef WHOLECHECK8V_SAMPLE_SHIFT
#define WHOLECHECK8V_SAMPLE_SHIFT 10
#endif
#ifndef WHOLECHECK8V_REPEATS
#define WHOLECHECK8V_REPEATS 8
#endif
#define WHOLECHECK8V_FORMAL 0
#define WHOLECHECK8V_FORMAL_PROFILE 0
#define WHOLECHECK8V_VERIFY 0
#ifndef WHOLECHECK8V_VERIFY_SHIFT
#define WHOLECHECK8V_VERIFY_SHIFT 20
#endif
#define FORCED_BASE_TERMINAL_SIMD_VERIFY 0
#ifndef FORCED_BASE_TERMINAL_SIMD_VERIFY_SHIFT
#define FORCED_BASE_TERMINAL_SIMD_VERIFY_SHIFT 20
#endif
#define AVX_STAGE_REFILL 1
#define AVX_UNION_SPARSE_PIVOT 1
#define AVX_UNION_SPARSE_REFINE 1
#define AVX_PERSISTENT_SOA 1
#define AVX_ADAPTIVE_WIDTH 0
#define AVX_CROSS_CONTEXT 0
#define AVX_CROSS_CONTEXT_REFINE 0
#define AVX_LOCAL_MAXIMALITY 1
#define AVX_LOCAL_MAX_SUFFIX 1
#define AVX_LEFT_2D_DEFECT 1
#define REFINE_2D_MODE 2
// Only used by avxRefineIncludeEightUnionSparse() with REFINE_2D_MODE == 2.
// STATE32_DISPATCH: 1=unified logical ForcedBase context (original PARALLEL_ENGINE),
// 0=State32 handoff-local effective bound plus explicit penalty suffix path.

#ifndef PARALLEL_ENGINE_STATE32_FIXEDPOINT_PEELING
#define PARALLEL_ENGINE_STATE32_FIXEDPOINT_PEELING 1
#endif
#if PARALLEL_ENGINE_STATE32_FIXEDPOINT_PEELING != 0 && PARALLEL_ENGINE_STATE32_FIXEDPOINT_PEELING != 1
#error PARALLEL_ENGINE_STATE32_FIXEDPOINT_PEELING must be 0 or 1
#endif
#ifndef ANYK_FAST_K_MAX
#define ANYK_FAST_K_MAX 8
#endif
#define PIVOT_REUSE_MODE 1
#define PIVOT_SCALAR_OCC_THRESHOLD 0
#define DEGREE_PEEL_MODE 1
#define WORKLOAD_BATCH_MODE 0
#define BITBUILD_MODE 1
#define LANE_USE_BMI2 1
#define LANE_USE_PAIRED_HOTPATH 0
#define LANE_PROFILE 0
#define LANE_SYNC_PIVOT 0
//   0 = Word128 scalar DFS standard mode
//   2 = Hybrid128: exactly two ready states -> State128 AVX2; one ready state -> Word128 scalar node expansion.
#ifndef MT_TAIL_REEXPORT
#define MT_TAIL_REEXPORT 1
#endif
#ifndef MT_TAIL_MIN_IDLE
#define MT_TAIL_MIN_IDLE 4
#endif
#ifndef MT_TAIL_REMAINING_SEEDS
#define MT_TAIL_REMAINING_SEEDS 8
#endif
#ifndef MT_TAIL_WATERLINE_MULT
#define MT_TAIL_WATERLINE_MULT 2
#endif
#ifndef MT_TAIL_MAX_OUTSTANDING_MULT
#define MT_TAIL_MAX_OUTSTANDING_MULT 4
#endif
#ifndef MT_TAIL_LIST_MIN_C
#define MT_TAIL_LIST_MIN_C 8
#endif
#ifndef MT_TAIL_BIT_MIN_C
#define MT_TAIL_BIT_MIN_C 12
#endif
#ifndef MT_TAIL_STATE32_MIN_C
#define MT_TAIL_STATE32_MIN_C 16
#endif
#ifndef MT_TAIL_BATCH_STATES
#define MT_TAIL_BATCH_STATES 0
#endif
#ifndef MT_TAIL_BATCH_MIN_STATES
#define MT_TAIL_BATCH_MIN_STATES 4
#endif
#define LANE_PIVOT_MODE 0
#define LANE_REGRET_MAX 1
#define LANE_PIVOT_PROFILE 0
#define WIDTH_LADDER_VALIDATION 0
#ifndef WIDTH_LADDER_SAMPLE_SHIFT
#define WIDTH_LADDER_SAMPLE_SHIFT 8
#endif
#if WIDTH_LADDER_SAMPLE_SHIFT < 0 || WIDTH_LADDER_SAMPLE_SHIFT > 20
#error WIDTH_LADDER_SAMPLE_SHIFT must be in [0,20]
#endif
#define EXACT_C_CROSSING_PROBE 0
#define FORCED_COMPLEMENT_FOLD_VALIDATION 0
#define COMPLEMENT_FOLD_TELEMETRY 0
#define STATE128_OPPORTUNITY_PROBE 0
#ifndef FORCED_COMPLEMENT_FOLD_SAMPLE_SHIFT
#define FORCED_COMPLEMENT_FOLD_SAMPLE_SHIFT 8
#endif
#if FORCED_COMPLEMENT_FOLD_SAMPLE_SHIFT < 0 || FORCED_COMPLEMENT_FOLD_SAMPLE_SHIFT > 20
#error FORCED_COMPLEMENT_FOLD_SAMPLE_SHIFT must be in [0,20]
#endif
#define EXACT_D_CONDITIONAL_PROBE 0
#include <cassert>
#include <immintrin.h>
#include <functional>
#include <type_traits>
#include <atomic>
#include <cstring>
#include <chrono>
#include <time.h>
#include "limits.h"
#include "../shared/utils.h"
#include "../shared/timer.h"
#include "limits.h"
#include "../shared/MRandSet.h"
#include "../shared/MStack.h"
#include "limits.h"
#include "../shared/utils.h"
#include "../shared/graph.h"
#include "../shared/degeneracy.h"
#include "limits.h"
#include "../shared/utils.h"
#include "../shared/graph.h"
#include "limits.h"
#include "../shared/utils.h"
#include "../shared/graph.h"
#include "../shared/graphIO.h"
#include "limits.h"
#include "../shared/utils.h"
#include "../shared/sequence.h"
#include "../shared/adaptive_bookkeeping.h"
#include "../runtime/diagnostics.h"
#include "../folding/forced_context.h"
#include "../folding/complement.h"
#include "../folding/fixed_vertices.h"
#include "../folding/admission.h"
#include "../folding/bit_context.h"
#include "../search/kernels/state64_types.h"
#include "../shared/word128_scalar.h"
#include "../shared/state128_core.inc"
#include "../shared/state128_simd_core.inc"
#include "../shared/wide_state_context_builder.inc"
#define WIDE_CONTEXT_VALIDATION 0
#include <cstdio>
#include "../shared/word256_scalar.h"
#include "../shared/wide_state_search.h"
#include "../shared/wide_state_remap.inc"
#define WIDE_REMAP_VALIDATION 0
#include "../search/list/terminal_check.h"
#include "../folding/scan_policy.h"

#ifndef LANE_USE_BMI2
#define LANE_USE_BMI2 1
#endif
#ifndef LANE_USE_PAIRED_HOTPATH
#define LANE_USE_PAIRED_HOTPATH 1
#endif
#define LANE_PROFILE 0
#ifndef LANE_SYNC_PIVOT
#define LANE_SYNC_PIVOT 0
#endif
#ifndef LANE_PIVOT_MODE
#define LANE_PIVOT_MODE 0
#endif
#ifndef LANE_REGRET_MAX
#define LANE_REGRET_MAX 1
#endif
#define LANE_PIVOT_PROFILE 0

#define LANE_COUNT(expr) do { } while(0)

#define AVX_COUNT(expr) do { } while(0)

#define WITNESS_COUNT(expr) do { } while(0)

#define REFINE2D_COUNT(expr) do { } while(0)

#define PIVOT_COUNT(expr) do { } while(0)

#define DEGPEEL_COUNT(expr) do { } while(0)

#define WORKBATCH_COUNT(expr) do { } while(0)

