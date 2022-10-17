
import os
import sys
import subprocess
from itertools import cycle

def usage():
    print("Usage: coverage <preset> [gtest_filter]")
    print("       coverage ci <outpath> <configuration> [gtest_filter]")
    sys.exit(1)

if __name__ == "__main__":
    if len(sys.argv) == 1:
        usage()

    preset = sys.argv[1]

    ci_mode = False
    if len(sys.argv) >= 1 and sys.argv[1] == 'ci':
        ci_mode = True
        if len(sys.argv) < 4:
            usage()

        print("CI mode enabled")
        output_path = sys.argv[2]
        configuration = sys.argv[3]
        sys.argv[1:] = sys.argv[3:]

    if len(sys.argv) == 3:
        os.environ["GTEST_FILTER"] = sys.argv[2]

    if ci_mode:
        export_type = f'cobertura:{output_path}\coverage.xml'
    else:
        export_type = 'html:build\coverage'

    sources = ['Teide\\src', 'Teide\\include', 'Teide\\examples', 'Teide\\libs']
    sources = [x for y in zip(cycle(['--source']), sources) for x in y]

    if ci_mode:
        ctest_cmd = ['ctest', '-V', '-C', configuration]
    else:
        ctest_cmd = ['ctest', '-V', '--preset', preset]
    coverage_cmd = ['OpenCppCoverage', '--export_type', export_type, '--modules', '*.exe'] + sources + ['--cover_children', '--'] + ctest_cmd

    print(subprocess.list2cmdline(coverage_cmd))
    subprocess.run(coverage_cmd)
