"""Compile one final core object and link both execution frontends."""
import argparse
from pathlib import Path
import os
import shutil
import signal
import subprocess
from common import ROOT, BUILD, check_environment, execute, read_json, save_json, sha256, source_identity, handle_termination


def ensure_build(verify=False, force=False):
    check_environment()
    compiler = shutil.which(os.environ.get('CXX') or shutil.which('g++-11') or 'g++')
    if not compiler:
        raise RuntimeError('C++ compiler not found')
    version = subprocess.check_output([compiler, '--version'], text=True).splitlines()[0]
    identity = source_identity()
    flags = read_json(ROOT / 'config/build_flags.json') + [f'-DUNIFIED_CORE_ID="{identity}"']
    if verify:
        flags += ['-DSET', '-DSTATE32_LB_CACHE_VERIFY=1']
    directory = BUILD / 'verify' if verify else BUILD
    meta = directory / 'build.json'
    inputs = {'source_id': identity, 'compiler': str(Path(compiler).resolve()),
              'compiler_version': version, 'flags': flags}
    if not force and meta.is_file():
        previous = read_json(meta)
        if (all(previous.get(key) == value for key, value in inputs.items()) and
                all((directory / path).is_file() and sha256(directory / path) == digest
                    for path, digest in previous.get('outputs', {}).items()) and
                set(previous.get('outputs', {})) == {'core.o', 'bin/Sequential', 'bin/Parallel'}):
            return previous
    (directory / 'bin').mkdir(parents=True, exist_ok=True)
    commands = [[compiler, *flags, '-c', str(ROOT / 'core/application.cpp'), '-o', str(directory / 'core.o')]]
    for frontend in ['Sequential', 'Parallel']:
        commands.append([compiler, '-std=c++14', '-O3', '-pthread', str(ROOT / frontend / 'main.cpp'),
                         str(directory / 'core.o'), '-o', str(directory / 'bin' / frontend)])
    for label, args in zip(['core', 'Sequential', 'Parallel'], commands):
        print('[构建] ' + ('verify/' if verify else '') + label, flush=True)
        log = directory / (label + '.log')
        _, code, _ = execute(args, log)
        if code:
            raise RuntimeError(log.read_text(errors='replace')[-5000:])
    outputs = {name: sha256(directory / name) for name in ['core.o', 'bin/Sequential', 'bin/Parallel']}
    result = {**inputs, 'commands': commands, 'outputs': outputs}
    save_json(meta, result)
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--verify', action='store_true', help='also build full-set verification executables')
    parser.add_argument('--force', action='store_true')
    args = parser.parse_args()
    signal.signal(signal.SIGTERM, handle_termination)
    try:
        ensure_build(force=args.force)
        if args.verify:
            ensure_build(verify=True, force=args.force)
    except (Exception, KeyboardInterrupt) as exc:
        raise SystemExit(str(exc) or 'Build interrupted')
