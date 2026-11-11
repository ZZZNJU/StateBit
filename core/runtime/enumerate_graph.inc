void enumerateGraph(const Graph& g) {
#include "driver/prepare_roots.inc"
#include "driver/progress.inc"
    auto worker = [&](int tid) {
#include "worker/context.inc"
        auto processSeed = [&](const int idx) {
#include "worker/root_search.inc"
        };
#include "worker/scheduling.inc"
#include "worker/report.inc"
    };
#include "driver/execute_workers.inc"
#include "driver/reduce.inc"
#include "../output/search_report.inc"
#include "driver/cleanup.inc"
}
