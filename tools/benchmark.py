import argparse
import json
import subprocess
import tempfile
import shutil
import shlex
import sys
import os
from pathlib import Path


def run_process(cmd):
    print('>', subprocess.list2cmdline(cmd))
    proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, encoding='utf-8')
    proc.check_returncode()
    return proc.stdout


class GitSwitcher:
    def __init__(self, ref):
        self.ref = ref
        self.head = run_process(['git', 'rev-parse', '--abbrev-ref', 'HEAD']).strip()

    def __enter__(self):
        run_process(['git', 'switch', '--detach', self.ref])

    def __exit__(self, exc_type, exc_value, traceback):
        run_process(['git', 'switch', self.head])


def git_switch(ref):
    return GitSwitcher(ref)


def run_benchmark(bin_dir: Path, out_file: Path, build: bool = False):
    if build:
        print(f"Building in binary directory {bin_dir}")
        run_process(['cmake', '--build', bin_dir])
    print("Running benchmarks")
    out_file.parent.mkdir(exist_ok=True)
    run_process(
        [bin_dir / 'benchmarks/TeideBenchmark/TeideBenchmark',
        f'--benchmark_out={out_file}',
        '--benchmark_repetitions=20',
        '--benchmark_min_warmup_time=1',
        '--benchmark_enable_random_interleaving=true'])
    print(f"Results stored in file {out_file}")


def benchmark_commit(ref_name, preset, out_dir: Path, definitions: list[str]):
    commit_hash = run_process(['git', 'rev-parse', ref_name]).strip()
    out_file = out_dir / (commit_hash + '.json')
    print(f"Benchmarking commit {commit_hash}...")
    if out_file.exists():
        print('Using cached results')
        return out_file
    with git_switch(commit_hash):
        print(f"Configuring preset {preset}")
        bin_dir = Path('build') / preset
        shutil.rmtree(bin_dir, ignore_errors=True)
        run_process(['cmake', '--preset', preset, '-B', bin_dir] + ['-D'+x for x in definitions])

        run_benchmark(bin_dir, out_file, build=True)
    return out_file


def benchmark_compare(ref1_name: str, ref2_name: str, preset, out_dir: Path, compare: str, definitions: list[str]):
    output_file = out_dir / (ref1_name + '_' + ref2_name + '.json')
    result1 = benchmark_commit(ref1_name, preset, out_dir, definitions)
    print()
    result2 = benchmark_commit(ref2_name, preset, out_dir, definitions)
    print()
    print(run_process([sys.executable, compare, '--display_aggregates_only', '--dump_to_json', output_file, 'benchmarks', result1, result2]))


def benchmark_prs(preset, out_dir: Path, compare: str, definitions: list[str]):
    prs = json.loads(run_process(['gh', 'pr', 'list', '--json', 'baseRefName,headRefName,headRefOid,title,number']))
    for pr in prs:
        print("#{number} {title}\n  Head: {headRefOid} ({headRefName})".format(**pr))
        merge_base = run_process(['git', 'merge-base', pr['headRefOid'], 'origin/'+pr['baseRefName']]).strip()
        print(f"  Base: {merge_base} ({pr['baseRefName']})")
        benchmark_compare(merge_base, pr['headRefOid'], preset, out_dir, compare, definitions)


def add_build_options(cmd):
    cmd.add_argument('-p', '--preset', required=True)
    cmd.add_argument('-o', '--out-dir', required=True, type=Path)
    cmd.add_argument('-D', dest='definitions', default=[], action='append')

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(required=True)

    cmd = subparsers.add_parser('local',
        help="Run benchmarks on current working copy")
    cmd.add_argument('-b', '--bin-dir', required=True)
    cmd.add_argument('-o', '--out-file', required=True)
    cmd.add_argument('--build', action='store_true')
    cmd.set_defaults(func=run_benchmark)

    cmd = subparsers.add_parser('commit',
        help="Run benchmarks on a commit")
    cmd.add_argument('ref_name')
    add_build_options(cmd)
    cmd.set_defaults(func=benchmark_commit)

    default_compare = os.environ.get('BENCHMARK_COMPARE')

    cmd = subparsers.add_parser('compare',
        help="Run benchmarks on two commits and compare the results")
    cmd.add_argument('ref1_name')
    cmd.add_argument('ref2_name')
    add_build_options(cmd)
    cmd.add_argument('--compare', required=default_compare==None, default=default_compare)
    cmd.set_defaults(func=benchmark_compare)

    cmd = subparsers.add_parser('prs')
    add_build_options(cmd)
    cmd.add_argument('--compare', required=default_compare==None, default=default_compare)
    cmd.set_defaults(func=benchmark_prs)

    opts = parser.parse_args()
    func = opts.func
    delattr(opts, 'func')
    try:
        func(**vars(opts))
    except subprocess.CalledProcessError as e:
        print(e.output)
        sys.exit(1)
    except KeyboardInterrupt:
        print("Cancelled")
        sys.exit(2)
