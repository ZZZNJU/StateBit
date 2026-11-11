"""Shared build, configuration and process utilities for StateBit."""
from pathlib import Path
from datetime import datetime
import hashlib
import json
import math
import os
import re
import signal
import subprocess
import sys
from zoneinfo import ZoneInfo

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / '.build'
TZ = ZoneInfo('Asia/Shanghai')


def read_json(path):
    return json.loads(Path(path).read_text(encoding='utf-8'))


def save_json(path, value):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + '.tmp')
    temporary.write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    temporary.replace(path)


def sha256(path):
    value = hashlib.sha256()
    with Path(path).open('rb') as handle:
        for block in iter(lambda: handle.read(8 * 1024 * 1024), b''):
            value.update(block)
    return value.hexdigest()


def source_identity():
    value = hashlib.sha256()
    for folder in ['core', 'Sequential', 'Parallel']:
        for path in sorted((ROOT / folder).rglob('*')):
            if path.is_file():
                value.update(str(path.relative_to(ROOT)).encode())
                value.update(path.read_bytes())
    return value.hexdigest()


def check_environment():
    if os.environ.get('PYTHONOPTIMIZE', '').strip() not in ['', '0'] or sys.flags.optimize:
        raise ValueError('Run without PYTHONOPTIMIZE or python -O; verification requires assertions')
    for name in ['LD_PRELOAD', 'STATEBIT_SAMPLE_PREFIX', 'STATEBIT_ONLY_SEED']:
        if os.environ.get(name, '').strip():
            raise ValueError('Clear external instrumentation before running: ' + name)


def command(graph, k, q, threads=1, mode=None, verify=False):
    mode = mode or ('sequential' if threads == 1 else 'parallel')
    if k < 2 or q < 2 * k - 1:
        raise ValueError('Require k >= 2 and q >= 2*k-1')
    if threads < 1 or mode not in ['sequential', 'parallel'] or (mode == 'sequential' and threads != 1):
        raise ValueError('Sequential requires one thread; parallel requires a positive thread count')
    directory = BUILD / 'verify' / 'bin' if verify else BUILD / 'bin'
    args = [str(directory / ('Sequential' if mode == 'sequential' else 'Parallel')),
            str(graph), '-k', str(k), '-q', str(q), '-t', str(threads)]
    for name, value in read_json(ROOT / 'config/runtime.json').items():
        args += ['--' + name.replace('_', '-'), str(value)]
    return args + ['-p', '0', '-s', '0']


def cpu_list(text, threads, allowed):
    allowed = sorted(set(allowed))
    chosen = list(map(int, text.split(','))) if text is not None else allowed[:threads]
    if len(chosen) != threads or len(set(chosen)) != threads or not set(chosen) <= set(allowed):
        raise ValueError(f'Require {threads} distinct, allowed CPU numbers')
    return chosen


def measurement(text, k):
    counts = re.findall(rf'^Number of {k}-plex: (\d+)\s*$', text, re.M)
    times = re.findall(r'^Running time([^\n]*)$', text, re.M)
    if len(counts) != 1 or len(times) != 1:
        raise ValueError('Expected exactly one count and one algorithm time')
    seconds = float(times[0].strip())
    if not math.isfinite(seconds) or seconds < 0:
        raise ValueError('Invalid algorithm time')
    return int(counts[0]), seconds


def record(text, prefix):
    lines = [line for line in text.splitlines() if line.startswith(prefix + ' ')]
    if len(lines) != 1:
        return {}
    return dict(re.findall(r'(\w+)=(\S+)', lines[0]))


def check_configuration(text, mode, threads, identity, k, q, allow_fewer_workers=False):
    expected = {'core_id': identity, 'mode': mode, 'threads': str(threads),
                'shared_object': '1', 'selector': 'legacy_hybrid4', 'local_folding': '1',
                'local_f_remat': '1', 'remat_stack_reuse': '1', 'remat_min_slack': '3',
                'direct256': '1', 'wide_cost': '0', 'mixed_f': '0', 'c0_probe': '0'}
    actual = record(text, 'UNIFIED_CONFIG')
    fold = record(text, 'FOLD_VARIANT_CONFIG')
    transfer = record(text, 'TRANSFER_CONFIG')
    simd = record(text, 'SIMD128_CONFIG')
    workers = record(text, 'MT_STATEBIT_CONFIG')
    actual_threads = int(workers.get('actual_threads', '0'))
    worker_count_ok = (1 <= actual_threads <= threads if allow_fewer_workers else actual_threads == threads)
    if (not all(actual.get(key) == value for key, value in expected.items()) or
            not all(fold.get(key) == value for key, value in
                    {'variant': '3', 'entry_commit_le32': '1', 'entry_restore_order': '1',
                     'entry_pre_fold': '1', 'remat_candidate_gate': '0'}.items()) or
            transfer.get('queue_precheck') != '1' or transfer.get('remap128') != '1' or
            simd.get('enabled') != '0' or not worker_count_ok or
            workers.get('requested_threads') != str(threads) or
            f'k={k},q={q},threads={threads},progress_sec=0,max_seeds=0' not in text):
        raise ValueError('Runtime configuration does not match the final configuration')


def terminate(process):
    if process.poll() is not None:
        return
    try:
        os.killpg(process.pid, signal.SIGTERM)
    except ProcessLookupError:
        process.wait()
        return
    try:
        process.wait(timeout=3)
    except subprocess.TimeoutExpired:
        try:
            os.killpg(process.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
        process.wait()


def execute(args, log, timeout=0, env=None):
    import time
    started = time.perf_counter()
    state = 'EXIT'
    Path(log).parent.mkdir(parents=True, exist_ok=True)
    with Path(log).open('w') as output:
        process = subprocess.Popen(args, stdout=output, stderr=subprocess.STDOUT,
                                   env=env, start_new_session=True)
        try:
            try:
                code = process.wait(timeout=timeout or None)
            except subprocess.TimeoutExpired:
                terminate(process)
                state, code = 'TIMEOUT', 124
        except BaseException:
            terminate(process)
            raise
    return state, code, time.perf_counter() - started


def now():
    return datetime.now(TZ).isoformat(timespec='seconds')


def handle_termination(*_):
    raise KeyboardInterrupt
