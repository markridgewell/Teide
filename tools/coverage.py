
import os
import sys
import subprocess
from itertools import cycle

def usage():
    print("Usage: coverage [ci] <preset> <configuration> [gtest_filter]")
    sys.exit(1)

if __name__ == "__main__":
    if len(sys.argv) == 1:
        usage()


    ci_mode = False
    if len(sys.argv) >= 1 and sys.argv[1] == 'ci':
        ci_mode = True
        if len(sys.argv) < 4:
            usage()

        print("CI mode enabled")
        output_path = sys.argv[2]
        del sys.argv[1]

    preset = sys.argv[1]
    configuration = sys.argv[2]

    if len(sys.argv) == 4:
        os.environ["GTEST_FILTER"] = sys.argv[3]

    if ci_mode:
        export_type = f'cobertura:coverage.xml'
    else:
        export_type = 'html:build\coverage'

    sources = ['Teide\\src', 'Teide\\include', 'Teide\\examples', 'Teide\\libs']
    sources = [x for y in zip(cycle(['--source']), sources) for x in y]

    ctest_cmd = ['ctest', '--preset', preset, '--build-config', configuration, '--verbose', '--no-tests=error']
    coverage_cmd = ['OpenCppCoverage', '--export_type', export_type, '--modules', '*.exe'] + sources + ['--cover_children', '--'] + ctest_cmd

    print(subprocess.list2cmdline(coverage_cmd))
    sys.exit(subprocess.run(coverage_cmd).returncode)
