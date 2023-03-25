
import os
import sys
import subprocess
import argparse
from glob import glob


DEFINES  = ['VULKAN_HPP_NO_STRUCT_CONSTRUCTORS',
            'VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1',
            'SPDLOG_SHARED_LIB',
            'SPDLOG_COMPILED_LIB',
            'SPDLOG_FMT_EXTERNAL',
            'FMT_SHARED',
            'SDL_main=main']


def get_script_path():
    return os.path.dirname(os.path.realpath(sys.argv[0]))


def find_source_dirs():
    return ['src', 'include'] + glob('tests/*/src') + glob('examples/*/src')


def include_path(p:str):
    return '-I' + p.replace('\\', '/')


def get_cppcheck():
    for env in ['ProgramFiles', 'ProgramFiles(x86)', 'ProgramW6432']:
        programfiles = os.environ.get(env, None)
        if programfiles:
            path = os.path.join(programfiles, 'Cppcheck', 'cppcheck.exe')
            if os.path.exists(path):
                return path
    return 'cppcheck'


def run_cppcheck(build_dir:str, output_file:str, show_code:bool, absolute_paths:bool):
    source_dirs = find_source_dirs()

    path_prefix = ''
    if absolute_paths:
        path_prefix = os.getcwd() + os.sep

    code = '\n{code}' if show_code else ''

    cmd = [get_cppcheck()]
    cmd += source_dirs
    cmd += map(include_path, source_dirs)

    if build_dir:
        cmd += map(include_path, glob(f'{build_dir}/vcpkg_installed/*/include'))

    cmd += map(lambda x: '-D'+x, DEFINES)
    cmd += ['--std=c++20',
            '--enable=all',
            '--disable=unusedFunction',
            '--template='+path_prefix+'{file}({line},{column}): {severity}: {message} [{id}]' + code,
            '--template-location='+path_prefix+'{file}({line},{column}): note: {info}' + code,
            '--platform=native',
            '--library=googletest',
            '--library=sdl',
            '--error-exitcode=1',
            f'--suppress-xml={get_script_path()}/cppcheck_suppressions.xml',
            '--inline-suppr']

    if build_dir:
        cmd += [f'--cppcheck-build-dir={build_dir}/cppcheck']

    if output_file:
        cmd += [f'--output-file={output_file}']

    print(subprocess.list2cmdline(cmd))
    return subprocess.run(cmd).returncode


def main(args):
    parser = argparse.ArgumentParser()
    parser.add_argument('--build-dir', help="Directory project was built in")
    parser.add_argument('--output-file', help="Filename to store results")
    parser.add_argument('--show-code', action='store_true', help="Show code in output")
    parser.add_argument('--absolute-paths', action='store_true', help="Use absolute paths in output")
    opts = parser.parse_args(args)

    return run_cppcheck(**vars(opts))


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
