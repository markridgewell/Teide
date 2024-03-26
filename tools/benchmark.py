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


def run_benchmark(
        bin_dir: Path,
        out_file: Path,
        repetitions: int,
        sw_render: bool,
        build: bool = False):
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
        f'--benchmark_repetitions={repetitions}',
        '--benchmark_min_warmup_time=1',
        '--benchmark_enable_random_interleaving=true']
    if sw_render:
        cmd += ['--sw-render']
    print(run_process(cmd))
    print(f"Results stored in file {out_file}")


def benchmark_commit(
        ref_name: str,
        preset: str,
        out_dir: Path,
        definitions: list[str],
        repetitions: int,
        sw_render: bool
    ) -> Path:
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

        run_benchmark(bin_dir, out_file, build=True, repetitions=repetitions, sw_render=sw_render)
    return out_file


def to_string(f: float, precision: int):
    return f'{f:.{precision}f}'


def benchmark_compare(
        ref1_name: str,
        ref2_name: str,
        preset: str,
        out_dir: Path,
        compare: str,
        definitions: list[str],
        repetitions: int,
        sw_render: bool = True,
        out_json: Path = None,
        out_report: Path = None,
        out_summary: Path = None
    ):
    # Run benchmarks
    if not out_json:
        ref1_hash = run_process(['git', 'rev-parse', ref1_name]).strip()
        ref2_hash = run_process(['git', 'rev-parse', ref2_name]).strip()
        out_json = out_dir / (ref1_hash + '_' + ref2_hash + '.json')
    result1 = benchmark_commit(ref1_name, preset, out_dir, definitions, repetitions, sw_render)
    print()
    result2 = benchmark_commit(ref2_name, preset, out_dir, definitions, repetitions, sw_render)
    print()

    # Run comparison and report result
    compare_cmd = [sys.executable, compare, '--display_aggregates_only', '--dump_to_json', out_json]
    if out_report:
        compare_cmd += ['--no-color']
    compare_cmd += ['benchmarks', result1, result2]
    report = run_process(compare_cmd)
    if out_report:
        with open(out_report, 'w') as f:
            f.write(report)
    else:
        print(out_report)

    # Write summary
    if not out_summary:
        return
    with open(out_json) as f:
        results = json.load(f)
    test_names = [i['name'] for i in results if 'time_pvalue' in i['utest']]

    def lookup(name: str):
        return next(filter(lambda x: x['name'] == name, results), None)

    headers = ["Benchmark", "Result", "Time Old", "Time New", "P-value"]
    rows = []
    for name in test_names:
        utest = lookup(name)['utest']
        time_pvalue = utest['time_pvalue']
        measurements = lookup(name+'_mean')['measurements'][0]
        time = measurements['time']
        if time_pvalue > 0.1:
            continue
        else:
            faster = time < 0
            result = '{} {:0.2f}% {}'.format("🟢" if faster else "🔴", abs(time * 100), "faster" if faster else "slower")
        rows += [[name, result, to_string(measurements['real_time'] / 1000, 0) + 'ms', to_string(measurements['real_time_other'] / 1000, 0) + 'ms', to_string(time_pvalue, 4)]]

    num_results = len(rows)
    if num_results > 0:
        preamble = "{} benchmark result{} with statistically significant differences:".format(num_results, "s" if num_results > 1 else "")
    else:
        preamble = "All benchmark results are statistically equivalent to baseline."

    with open(out_summary, 'w') as f:
        f.write(preamble)
        f.write("\n\n")
        if not rows:
            return

        rows.insert(0, headers)
        col_widths = [[len(j) for j in i] for i in rows]
        col_widths = [max(*i) for i in zip(*col_widths)]
        rows.insert(1, ['-' * i for i in col_widths])
        for row in rows:
            padded_row = ['{:{}}'.format(*i) for i in zip(row, col_widths)]
            f.write('| ' + ' | '.join(padded_row) + ' |\n')


def add_build_options(cmd):
    cmd.add_argument('-p', '--preset', metavar='NAME', required=True, help="Name of preset to use to build benchmarks")
    cmd.add_argument('-o', '--out-dir', metavar='PATH', required=True, type=Path, help="Path to output directory to store benchmark results")
    cmd.add_argument('-D', dest='definitions', default=[], action='append')


def add_run_options(cmd):
    default_repetitions: int = 10
    cmd.add_argument('--repetitions', metavar='N', type=int, default=default_repetitions, help=f"Number of repetitions (default {default_repetitions})")
    cmd.add_argument('--sw-render', action='store_true', help="Enable software rendering when running benchmarks")


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(required=True)

    cmd = subparsers.add_parser('local',
        help="Run benchmarks on current working copy")
    cmd.add_argument('-b', '--bin-dir', required=True)
    cmd.add_argument('-o', '--out-file', required=True)
    cmd.add_argument('--build', action='store_true')
    add_run_options(cmd)
    cmd.set_defaults(func=run_benchmark)

    cmd = subparsers.add_parser('commit',
        help="Run benchmarks on a commit")
    cmd.add_argument('ref_name')
    add_build_options(cmd)
    add_run_options(cmd)
    cmd.set_defaults(func=benchmark_commit)

    default_compare = os.environ.get('BENCHMARK_COMPARE')

    cmd = subparsers.add_parser('compare',
        help="Run benchmarks on two commits and compare the results")
    cmd.add_argument('ref1_name')
    cmd.add_argument('ref2_name')
    add_build_options(cmd)
    add_run_options(cmd)
    cmd.add_argument('--compare', required=default_compare==None, default=default_compare)
    cmd.add_argument('--out-json', metavar='FILENAME', type=Path, help="Json file to write comparison data into")
    cmd.add_argument('--out-report', metavar='FILENAME', type=Path, help="Plain text file to write comparison report into")
    cmd.add_argument('--out-summary', metavar='FILENAME', type=Path, help="Markdown file to write comparison summary into")
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
