#pragma once
#include <array>
#include <algorithm>
#include <cstdint>

#ifndef FORCED_BASE_PAYOFF_THRESHOLD
#define FORCED_BASE_PAYOFF_THRESHOLD 0
#endif

namespace statebit {

struct ForcedBaseAdaptiveStats {
    unsigned long long gate_decisions=0;
    unsigned long long gate_allowed=0;
    unsigned long long gate_skipped=0;
    unsigned long long close_events=0;
    unsigned long long low_windows=0;
    unsigned long long max_low_streak=0;
    unsigned long long probe_epochs=0;
    unsigned long long payoff_samples=0;
    unsigned long long payoff_sum=0;
    unsigned long long payoff_zero=0;
    unsigned long long payoff_normal_samples=0;
    unsigned long long payoff_normal_sum=0;
    unsigned long long payoff_probe_samples=0;
    unsigned long long payoff_probe_sum=0;
    unsigned long long payoff_state128_samples=0;
    unsigned long long payoff_bit64_samples=0;
    // State256 payoff diagnostics. In validation mode these fields are updated
    // without touching the live payoff window; in live mode they are also
    // included in payoff_samples/payoff_sum/window through observePayoff(kind=3).
    unsigned long long payoff_state256_samples=0;
    unsigned long long payoff_state256_sum=0;
    unsigned long long payoff_state256_zero=0;
    unsigned long long payoff_state256_lt2=0;
    unsigned long long payoff_state256_h0=0;
    unsigned long long payoff_state256_h1=0;
    unsigned long long payoff_state256_h2_7=0;
    unsigned long long payoff_state256_h8_63=0;
    unsigned long long payoff_state256_h64p=0;
    unsigned long long terminal_checks=0;
    unsigned long long terminal_hits=0;
    unsigned long long terminal_scan_candidates=0;
    unsigned long long terminal_scan_ns=0;
    // semantics remain identical to the scalar loop; fail-batch accounting
    // uses the first failing lane so terminal_scan_candidates stays scalar-equivalent.
    unsigned long long terminal_simd_calls=0;
    unsigned long long terminal_simd_vector_batches=0;
    unsigned long long terminal_simd_full_pass_batches=0;
    unsigned long long terminal_simd_early_fail_batches=0;
    unsigned long long terminal_simd_scalar_tail_candidates=0;
    unsigned long long terminal_simd_verify_samples=0;
    unsigned long long terminal_simd_verify_mismatches=0;
    unsigned long long subtree_fallback_entries=0;
    unsigned long long subtree_fallback_nodes=0;
    unsigned long long subtree_fallback_materialized_forced=0;
    unsigned long long subtree_fallback_context_updates_skipped=0;

    void merge(const ForcedBaseAdaptiveStats& o){
        gate_decisions+=o.gate_decisions; gate_allowed+=o.gate_allowed; gate_skipped+=o.gate_skipped;
        close_events+=o.close_events; low_windows+=o.low_windows; max_low_streak=std::max(max_low_streak,o.max_low_streak);
        probe_epochs+=o.probe_epochs; payoff_samples+=o.payoff_samples; payoff_sum+=o.payoff_sum;
        payoff_zero+=o.payoff_zero; payoff_normal_samples+=o.payoff_normal_samples; payoff_normal_sum+=o.payoff_normal_sum;
        payoff_probe_samples+=o.payoff_probe_samples; payoff_probe_sum+=o.payoff_probe_sum;
        payoff_state128_samples+=o.payoff_state128_samples; payoff_bit64_samples+=o.payoff_bit64_samples;
        payoff_state256_samples+=o.payoff_state256_samples; payoff_state256_sum+=o.payoff_state256_sum;
        payoff_state256_zero+=o.payoff_state256_zero; payoff_state256_lt2+=o.payoff_state256_lt2;
        payoff_state256_h0+=o.payoff_state256_h0; payoff_state256_h1+=o.payoff_state256_h1;
        payoff_state256_h2_7+=o.payoff_state256_h2_7; payoff_state256_h8_63+=o.payoff_state256_h8_63;
        payoff_state256_h64p+=o.payoff_state256_h64p;
        terminal_checks+=o.terminal_checks; terminal_hits+=o.terminal_hits;
        terminal_scan_candidates+=o.terminal_scan_candidates; terminal_scan_ns+=o.terminal_scan_ns;
        terminal_simd_calls+=o.terminal_simd_calls; terminal_simd_vector_batches+=o.terminal_simd_vector_batches;
        terminal_simd_full_pass_batches+=o.terminal_simd_full_pass_batches; terminal_simd_early_fail_batches+=o.terminal_simd_early_fail_batches;
        terminal_simd_scalar_tail_candidates+=o.terminal_simd_scalar_tail_candidates;
        terminal_simd_verify_samples+=o.terminal_simd_verify_samples; terminal_simd_verify_mismatches+=o.terminal_simd_verify_mismatches;
        subtree_fallback_entries+=o.subtree_fallback_entries; subtree_fallback_nodes+=o.subtree_fallback_nodes;
        subtree_fallback_materialized_forced+=o.subtree_fallback_materialized_forced;
        subtree_fallback_context_updates_skipped+=o.subtree_fallback_context_updates_skipped;
    }
};

struct ForcedBasePayoffGate {
    std::array<unsigned long long, FORCED_BASE_PAYOFF_WINDOW> window{};
    int count=0;
    int pos=0;
    unsigned long long sum=0;
    int cooldown_remaining=0;
    int low_streak=0;
    int probe_samples_remaining=0;
    bool reopen_pending=false;
    ForcedBaseAdaptiveStats stats;

    void reset(){ *this=ForcedBasePayoffGate{}; }

    bool allowFold(){
        ++stats.gate_decisions;
        if(cooldown_remaining>0){
            --cooldown_remaining;
            ++stats.gate_skipped;
            return false;
        }
        if(reopen_pending){
            reopen_pending=false;
            probe_samples_remaining=FORCED_BASE_PAYOFF_HYSTERESIS_WINDOWS;
            ++stats.probe_epochs;
        }
        ++stats.gate_allowed;
        return true;
    }

    inline void observeState256Stats(const unsigned long long payoff){
        ++stats.payoff_state256_samples;
        stats.payoff_state256_sum+=payoff;
        if(payoff==0){ ++stats.payoff_state256_zero; ++stats.payoff_state256_h0; }
        else if(payoff==1){ ++stats.payoff_state256_h1; }
        else if(payoff<=7){ ++stats.payoff_state256_h2_7; }
        else if(payoff<=63){ ++stats.payoff_state256_h8_63; }
        else{ ++stats.payoff_state256_h64p; }
        if(payoff<2) ++stats.payoff_state256_lt2;
    }

    // Validation-only observer: characterize State256 payoff without changing the
    // live payoff window, cooldown state, or aggregate payoff counters.
    void observeState256PayoffValidation(const unsigned long long payoff){
        observeState256Stats(payoff);
    }

    void observePayoff(const unsigned long long payoff,const int kind=0){
        ++stats.payoff_samples;
        stats.payoff_sum+=payoff;
        if(payoff==0) ++stats.payoff_zero;
        if(kind==1) ++stats.payoff_state128_samples;
        if(kind==2) ++stats.payoff_bit64_samples;
        if(kind==3) observeState256Stats(payoff);
        const bool probe=(probe_samples_remaining>0);
        if(probe){
            ++stats.payoff_probe_samples;
            stats.payoff_probe_sum+=payoff;
            --probe_samples_remaining;
        }else{
            ++stats.payoff_normal_samples;
            stats.payoff_normal_sum+=payoff;
        }
        if(count<FORCED_BASE_PAYOFF_WINDOW){
            window[count++]=payoff;
            sum+=payoff;
        }else{
            sum-=window[pos];
            window[pos]=payoff;
            sum+=payoff;
            pos=(pos+1)%FORCED_BASE_PAYOFF_WINDOW;
        }
        if(count>=FORCED_BASE_PAYOFF_WARMUP){
            const double avg=static_cast<double>(sum)/static_cast<double>(count);
            if(avg<(0.5*static_cast<double>(FORCED_BASE_PAYOFF_THRESHOLD_X2))){
                ++stats.low_windows;
                ++low_streak;
                stats.max_low_streak=std::max(stats.max_low_streak,static_cast<unsigned long long>(low_streak));
                if(low_streak>=FORCED_BASE_PAYOFF_HYSTERESIS_WINDOWS){
                    cooldown_remaining=FORCED_BASE_PAYOFF_COOLDOWN;
                    reopen_pending=true;
                    probe_samples_remaining=0;
                    low_streak=0;
                    ++stats.close_events;
                }
            }else{
                low_streak=0;
                probe_samples_remaining=0;
            }
        }
    }
};

} // namespace statebit
