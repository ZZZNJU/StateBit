"""Run the final StateBit configuration on one binary graph."""
from pathlib import Path
from datetime import datetime
import argparse
import os
import signal
import sys
from build import ensure_build
from common import ROOT, TZ, check_configuration, command, cpu_list, execute, handle_termination, measurement, now


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('graph', type=Path)
    parser.add_argument('-k', type=int, required=True)
    parser.add_argument('-q', type=int, required=True)
    parser.add_argument('-t', '--threads', type=int, default=1)
    parser.add_argument('--mode', choices=['sequential', 'parallel'])
    parser.add_argument('--cpus', help='comma-separated CPU IDs; defaults to the first allowed CPUs')
    args = parser.parse_args()
    signal.signal(signal.SIGTERM, handle_termination)
    try:
        graph = args.graph.resolve()
        if not graph.is_file():
            raise FileNotFoundError(graph)
        mode = args.mode or ('sequential' if args.threads == 1 else 'parallel')
        argv = command(graph, args.k, args.q, args.threads, mode)
        cpus = cpu_list(args.cpus, args.threads, os.sched_getaffinity(0))
        build = ensure_build()
        case = f'{graph.stem}_k{args.k}_q{args.q}'
        run = ROOT / 'results' / ('single_' + datetime.now(TZ).strftime('%Y%m%d_%H%M%S_%f'))
        log = run / (case + '.log')
        print(f'[开始] case={case} group=StateBit threads={args.threads} time={now()}', flush=True)
        state, code, wall = execute(['taskset', '-c', ','.join(map(str, cpus)), *argv], log)
        if code or state != 'EXIT':
            raise RuntimeError(f'Process failed ({code}); see {log}')
        text = log.read_text(errors='replace')
        count, seconds = measurement(text, args.k)
        check_configuration(text, mode, args.threads, build['source_id'], args.k, args.q, allow_fewer_workers=True)
        print(f'[结束] case={case} group=StateBit time={now()} wall={wall:.3f}s '
              f'algorithm={seconds:.3f}s count={count} status=PASS', flush=True)
        print('log=' + str(log), flush=True)
        return 0
    except KeyboardInterrupt:
        print('[中断] 子进程已停止', file=sys.stderr)
        return 130
    except Exception as exc:
        print('[失败] ' + str(exc), file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
