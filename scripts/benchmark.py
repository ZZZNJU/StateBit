"""Run the two reference cases using the final StateBit configuration."""
from pathlib import Path
from datetime import datetime
import argparse
import csv
import json
import math
import os
import platform
import signal
import sys
import tempfile
import zipfile
from build import ensure_build
from common import (ROOT, TZ, check_configuration, check_environment, command, cpu_list, execute,
                    handle_termination, measurement, now, read_json, save_json, sha256)


def plan(repeats=1):
    if repeats < 1:
        raise ValueError('REPEATS must be positive')
    with (ROOT / 'benchmarks/cases.csv').open(newline='') as handle:
        cases = list(csv.DictReader(handle))
    for case in cases:
        for name in ['k', 'q', 'threads', 'expected_count']:
            case[name] = int(case[name])
    return [{**case, 'repeat': repeat} for repeat in range(1, repeats + 1) for case in cases]


def write_rows(run, rows):
    if not rows:
        return
    with (run / 'raw.csv').open('w', newline='') as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)


def export(run, destination):
    path = destination / (run.name + '.zip')
    temporary = path.with_suffix('.zip.tmp')
    with zipfile.ZipFile(temporary, 'w', zipfile.ZIP_DEFLATED) as archive:
        for file in sorted(run.rglob('*')):
            if file.is_file():
                archive.write(file, Path(run.name) / file.relative_to(run))
    temporary.replace(path)
    return path


def input_identity(path, expected):
    import struct
    before = path.stat()
    with path.open('rb') as handle:
        header = handle.read(8)
    if len(header) != 8:
        raise ValueError('Invalid graph header: ' + str(path))
    n, arcs = struct.unpack('<II', header)
    if (before.st_size != expected['size'] or n != expected['n'] or
            arcs != expected['stored_arcs'] or before.st_size != 8 + 4 * n + 4 * arcs):
        raise ValueError('Graph header/size mismatch: ' + str(path))
    digest = sha256(path)
    after = path.stat()
    stable_fields = ['st_size', 'st_mtime_ns', 'st_ctime_ns', 'st_ino', 'st_dev']
    if (any(getattr(before, name) != getattr(after, name) for name in stable_fields) or
            digest != expected['sha256']):
        raise ValueError('Graph SHA256 mismatch or file changed: ' + str(path))
    return {'path': str(path), **expected, 'mtime_ns': after.st_mtime_ns,
            'ctime_ns': after.st_ctime_ns, 'inode': after.st_ino, 'device': after.st_dev}


def check_input(path, identity):
    value = path.stat()
    fields = [('st_size', 'size'), ('st_mtime_ns', 'mtime_ns'), ('st_ctime_ns', 'ctime_ns'),
              ('st_ino', 'inode'), ('st_dev', 'device')]
    if any(getattr(value, field) != identity[key] for field, key in fields):
        raise ValueError('Graph changed during the benchmark: ' + str(path))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--list', action='store_true', help='print the plan without building or running')
    args = parser.parse_args()
    run = destination = None
    rows = []
    result = 0
    signal.signal(signal.SIGTERM, handle_termination)
    try:
        check_environment()
        tasks = plan(int(os.environ.get('REPEATS', '1')))
        if args.list:
            print(json.dumps(tasks, indent=2))
            return 0
        cpus = cpu_list(os.environ.get('CPU_LIST'), 16, os.sched_getaffinity(0))
        timeout = float(os.environ.get('TIMEOUT_SEC', '0'))
        if timeout < 0 or not math.isfinite(timeout):
            raise ValueError('TIMEOUT_SEC must be finite and nonnegative')
        data = Path(os.environ.get('DATASET_DIR', '/home/u1/maximal-kplex/Dataset')).resolve()
        files = sorted({task['filename'] for task in tasks})
        missing = [str(data / name) for name in files if not (data / name).is_file()]
        if missing:
            raise FileNotFoundError('Missing datasets: ' + ', '.join(missing))
        run = ROOT / 'results' / ('StateBit_' + datetime.now(TZ).strftime('%Y%m%d_%H%M%S_%f'))
        run.mkdir(parents=True)
        downloads = Path('/mnt/c/Users/user/Downloads')
        if not downloads.is_dir() and Path('/c/Users/user/Downloads').is_dir():
            downloads = Path('/c/Users/user/Downloads')
        requested = Path(os.environ.get('EXPORT_DIR', str(downloads / 'kplex_results'))).resolve()
        if requested == run or run in requested.parents:
            raise ValueError('EXPORT_DIR must be outside the run directory')
        requested.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryFile(dir=requested) as probe:
            probe.write(b'write check')
            probe.flush()
            os.fsync(probe.fileno())
        destination = requested
        save_json(run / 'status.json', {'status': 'PREPARING', 'completed': 0, 'planned': len(tasks)})
        build = ensure_build()
        if build['compiler_version'].split()[-1] != '11.4.0':
            raise ValueError('Reference performance measurements require g++ 11.4.0')
        print('[预检] 独立小图及边界验证', flush=True)
        _, code, _ = execute(['bash', str(ROOT / 'test.sh')], run / 'preflight.log')
        if code:
            raise RuntimeError('Preflight failed; see ' + str(run / 'preflight.log'))
        references = read_json(ROOT / 'benchmarks/datasets.json')
        identities = {}
        for name in files:
            print('[数据核对] ' + name + ' SHA256', flush=True)
            identities[name] = input_identity(data / name, references[name])
        save_json(run / 'identity.json', {'build': build, 'runtime': read_json(ROOT / 'config/runtime.json'),
                  'datasets': identities, 'cpus': cpus, 'plan': tasks, 'timeout_seconds': timeout,
                  'platform': platform.platform()})
        print(f'[计划] StateBit 两例，每例{len(tasks) // 2}次，共{len(tasks)}次', flush=True)
        for task in tasks:
            case, filename = task['case'], task['filename']
            check_input(data / filename, identities[filename])
            chosen = cpus[:task['threads']]
            argv = ['taskset', '-c', ','.join(map(str, chosen)),
                    *command(data / filename, task['k'], task['q'], task['threads'], task['mode'])]
            log = run / 'logs' / f'{case}_r{task["repeat"]}.log'
            save_json(log.with_suffix('.command.json'), {'argv': argv, 'cpus': chosen})
            start = now()
            print(f'[开始] case={case} group=StateBit repeat={task["repeat"]} time={start}', flush=True)
            state, code, wall = execute(argv, log, timeout)
            status, count, seconds = 'PASS', None, None
            try:
                if code or state != 'EXIT':
                    raise RuntimeError(state if state == 'TIMEOUT' else f'exit code {code}')
                text = log.read_text(errors='replace')
                count, seconds = measurement(text, task['k'])
                if count != task['expected_count']:
                    raise ValueError('COUNT_FAIL')
                check_configuration(text, task['mode'], task['threads'], build['source_id'], task['k'], task['q'])
                check_input(data / filename, identities[filename])
            except Exception as exc:
                status = str(exc)
            row = {key: task[key] for key in ['case', 'mode', 'threads', 'repeat', 'expected_count']}
            row.update(start=start, end=now(), count=count, algorithm_seconds=seconds, wall_seconds=wall,
                       status=status, returncode=code, log=str(log.relative_to(run)))
            rows.append(row)
            write_rows(run, rows)
            save_json(run / 'status.json', {'status': 'RUNNING', 'completed': len(rows), 'planned': len(tasks)})
            print(f'[结束] case={case} group=StateBit repeat={task["repeat"]} time={row["end"]} '
                  f'wall={wall:.3f}s status={status}', flush=True)
            export(run, destination)
            if status != 'PASS':
                raise RuntimeError('Stopped after failed case: ' + case)
        save_json(run / 'status.json', {'status': 'COMPLETE', 'completed': len(rows), 'planned': len(tasks)})
    except KeyboardInterrupt:
        result = 130
        if run:
            save_json(run / 'status.json', {'status': 'INTERRUPTED', 'completed': len(rows)})
        print('[中断] 子进程已停止，结果已保留', flush=True)
    except Exception as exc:
        result = 1
        if run:
            save_json(run / 'status.json', {'status': 'FAILED', 'completed': len(rows), 'error': str(exc)})
        print('[失败] ' + str(exc), file=sys.stderr, flush=True)
    finally:
        if run and destination:
            try:
                print('[结果ZIP] ' + str(export(run, destination)), flush=True)
            except Exception as exc:
                result = 1
                print('[导出失败] ' + str(exc), file=sys.stderr, flush=True)
    return result


if __name__ == '__main__':
    sys.exit(main())
