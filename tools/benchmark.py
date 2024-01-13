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


def run_benchmark(bin_dir: Path, out_file: Path, build: bool = False, sw_render: bool = True):
    if build:
        print(f"Building in binary directory {bin_dir}")
        print(run_process(['cmake', '--build', bin_dir, '--config', 'Release']))

    print("Running benchmarks")
    out_file.parent.mkdir(exist_ok=True)
    exe_paths = ['benchmarks/TeideBenchmark/TeideBenchmark', 'benchmarks/TeideBenchmark/Release/TeideBenchmark']
    full_paths = [bin_dir / i for i in exe_paths]
    valid_paths = [i for i in full_paths if i.exists()]
    cmd = [valid_paths[0],
        f'--benchmark_out={out_file}',
        '--benchmark_repetitions=20',
        '--benchmark_min_warmup_time=1',
        '--benchmark_enable_random_interleaving=true']
    if sw_render:
        cmd += ['--sw-render']
    print(run_process(cmd))
    print(f"Results stored in file {out_file}")


def benchmark_commit(ref_name, preset, out_dir: Path, definitions: list[str], sw_render: bool = True):
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
        print(run_process(['cmake', '--preset', preset, '-B', bin_dir, '-DTEIDE_BUILD_TESTS=OFF', '-DTEIDE_BUILD_EXAMPLES=OFF'] + ['-D'+x for x in definitions]))

        run_benchmark(bin_dir, out_file, build=True, sw_render=sw_render)
    return out_file


def benchmark_compare(
        ref1_name: str,
        ref2_name: str,
        preset: str,
        out_dir: Path,
        compare: str,
        definitions: list[str],
        sw_render: bool = True,
        out_json: Path = None,
        out_report: Path = None,
        out_summary: Path = None
    ):
    # Run benchmarks
    if not out_json:
        out_json = out_dir / (ref1_name + '_' + ref2_name + '.json')
    result1 = benchmark_commit(ref1_name, preset, out_dir, definitions, sw_render)
    print()
    result2 = benchmark_commit(ref2_name, preset, out_dir, definitions, sw_render)
    print()

    # Run comparison and report results
    report = run_process([sys.executable, compare, '--display_aggregates_only', '--dump_to_json', out_json, 'benchmarks', result1, result2])
    if out_report:
        with open(out_report, 'w') as f:
            f.write(report)
    else:
        print(out_report)

    # Write summary
    with open(out_json) as f:
        results = json.load(f)
    test_names = [i['name'] for i in results if 'time_pvalue' in i['utest']]
    print(test_names)

    def lookup(name: str):
        return next(filter(lambda x: x['name'] == name, results), None)

    for name in test_names:
        utest = lookup(name)['utest']
        time_pvalue = utest['time_pvalue']
        measurements = lookup(name+'_mean')['measurements'][0]
        print(name, time_pvalue, measurements)


def add_build_options(cmd):
    cmd.add_argument('-p', '--preset', required=True)
    cmd.add_argument('-o', '--out-dir', required=True, type=Path)
    cmd.add_argument('-D', dest='definitions', default=[], action='append')


def add_common_options(cmd):
    cmd.add_argument('--sw-render', action='store_true')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(required=True)

    cmd = subparsers.add_parser('local',
        help="Run benchmarks on current working copy")
    cmd.add_argument('-b', '--bin-dir', required=True)
    cmd.add_argument('-o', '--out-file', required=True)
    cmd.add_argument('--build', action='store_true')
    add_common_options(cmd)
    cmd.set_defaults(func=run_benchmark)

    cmd = subparsers.add_parser('commit',
        help="Run benchmarks on a commit")
    cmd.add_argument('ref_name')
    add_build_options(cmd)
    add_common_options(cmd)
    cmd.set_defaults(func=benchmark_commit)

    default_compare = os.environ.get('BENCHMARK_COMPARE')

    cmd = subparsers.add_parser('compare',
        help="Run benchmarks on two commits and compare the results")
    cmd.add_argument('ref1_name')
    cmd.add_argument('ref2_name')
    add_build_options(cmd)
    add_common_options(cmd)
    cmd.add_argument('--compare', required=default_compare==None, default=default_compare)
    cmd.add_argument('--out-json', type=Path, help="Json file to write comparison data into")
    cmd.add_argument('--out-report', type=Path, help="Plain text file to write comparison report into")
    cmd.add_argument('--out-summary', type=Path, help="Markdown file to write comparison summary into")
    cmd.set_defaults(func=benchmark_compare)

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
